#include "gtest/gtest.h"
#include "ConnectionsBackup.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "ConnectionSettings.h"
#include "CredentialSettings.h"
#include "SshSettings.h"
#include "SslSettings.h"

using namespace Robomongo;

namespace
{
    const char *const dbPassword = "db-secret-1";
    const char *const sshPassword = "ssh-secret-2";
    const char *const sshPassphrase = "key-secret-3";
    const char *const tlsPassphrase = "tls-secret-4";
    const char *const passphrase = "correct horse battery";

    std::unique_ptr<ConnectionSettings> makeConnection()
    {
        auto connection = std::make_unique<ConnectionSettings>(false);
        // Saved connections always have one (created on first load)
        connection->setUuid("{0b4e6d5a-6c1f-4d8e-9a57-3f2a1c9e8b70}");
        connection->setConnectionName("prod billing");
        connection->setServerHost("db.example.com");
        connection->setServerPort(27018);
        connection->setAppName("billing service");
        connection->setConnectionNamespace("prod");

        auto credential = new CredentialSettings();
        credential->setUserName("admin");
        credential->setUserPassword(dbPassword);
        credential->setDatabaseName("admin");
        credential->setEnabled(true);
        connection->addCredential(credential);

        auto ssh = connection->sshSettings();
        ssh->setEnabled(true);
        ssh->setHost("bastion.example.com");
        ssh->setPort(2222);
        ssh->setUserName("robo");
        ssh->setUserPassword(sshPassword);
        ssh->setPassphrase(sshPassphrase);
        ssh->setPrivateKeyFile("/keys/id_rsa");
        ssh->setAuthMethod("publickey");

        connection->sslSettings()->enableSSL(true);
        connection->sslSettings()->setPemPassPhrase(tlsPassphrase);
        return connection;
    }

    void expectNoSecretsIn(QByteArray const& file)
    {
        for (auto const secret : { dbPassword, sshPassword, sshPassphrase, tlsPassphrase })
            EXPECT_FALSE(file.contains(secret)) << secret;
        EXPECT_FALSE(file.contains("Encrypted\""));
    }
}

TEST(connections_backup_tests, Export_WithoutPassphrase_LeavesSecretsOut)
{
    auto const connection = makeConnection();
    QByteArray const file = ConnectionsBackup::exportConnections({ connection.get() }, "");

    expectNoSecretsIn(file);
    EXPECT_FALSE(ConnectionsBackup::hasSecrets(file));
}

TEST(connections_backup_tests, Export_WithPassphrase_NeverWritesSecretsInClear)
{
    auto const connection = makeConnection();
    QByteArray const file = ConnectionsBackup::exportConnections({ connection.get() }, passphrase);

    expectNoSecretsIn(file);
    EXPECT_TRUE(ConnectionsBackup::hasSecrets(file));
}

TEST(connections_backup_tests, RoundTrip_WithPassphrase_RestoresEverything)
{
    auto const original = makeConnection();
    QByteArray const file = ConnectionsBackup::exportConnections({ original.get() }, passphrase);

    auto const result = ConnectionsBackup::importConnections(file, passphrase);
    ASSERT_TRUE(result.error.isEmpty()) << result.error.toStdString();
    ASSERT_EQ(1u, result.connections.size());
    EXPECT_TRUE(result.secretsRestored);

    auto const& imported = *result.connections.front();
    EXPECT_EQ("prod billing", imported.connectionName());
    EXPECT_EQ("db.example.com", imported.serverHost());
    EXPECT_EQ(27018, imported.serverPort());
    EXPECT_EQ("billing service", imported.appName());
    EXPECT_EQ("prod", imported.connectionNamespace());
    EXPECT_EQ(original->uuid(), imported.uuid());
    ASSERT_EQ(1, imported.credentialCount());
    EXPECT_EQ("admin", imported.credentials().front()->userName());
    EXPECT_EQ(dbPassword, imported.credentials().front()->userPassword());
    EXPECT_EQ("bastion.example.com", imported.sshSettings()->host());
    EXPECT_EQ(2222, imported.sshSettings()->port());
    EXPECT_EQ(sshPassword, imported.sshSettings()->userPassword());
    EXPECT_EQ(sshPassphrase, imported.sshSettings()->passphrase());
    EXPECT_EQ("/keys/id_rsa", imported.sshSettings()->privateKeyFile());
    EXPECT_TRUE(imported.sslSettings()->sslEnabled());
    EXPECT_EQ(tlsPassphrase, imported.sslSettings()->pemPassPhrase());
}

TEST(connections_backup_tests, Import_WithoutPassphrase_KeepsSettingsButNotSecrets)
{
    auto const original = makeConnection();
    QByteArray const file = ConnectionsBackup::exportConnections({ original.get() }, passphrase);

    auto const result = ConnectionsBackup::importConnections(file, "");
    ASSERT_TRUE(result.error.isEmpty());
    ASSERT_EQ(1u, result.connections.size());
    EXPECT_FALSE(result.secretsRestored);

    auto const& imported = *result.connections.front();
    EXPECT_EQ("prod", imported.connectionNamespace());
    EXPECT_EQ("", imported.credentials().front()->userPassword());
    EXPECT_EQ("", imported.sshSettings()->userPassword());
    EXPECT_EQ("", imported.sshSettings()->passphrase());
    EXPECT_EQ("", imported.sslSettings()->pemPassPhrase());
}

TEST(connections_backup_tests, Import_WrongPassphrase_Fails)
{
    auto const connection = makeConnection();
    QByteArray const file = ConnectionsBackup::exportConnections({ connection.get() }, passphrase);

    auto const result = ConnectionsBackup::importConnections(file, "wrong passphrase");
    EXPECT_FALSE(result.error.isEmpty());
    EXPECT_TRUE(result.connections.empty());
}

TEST(connections_backup_tests, Import_TamperedSecrets_Fails)
{
    auto const connection = makeConnection();
    QJsonObject root = QJsonDocument::fromJson(
        ConnectionsBackup::exportConnections({ connection.get() }, passphrase)).object();

    // Flip one byte of the encrypted data: GCM authentication must reject it
    QJsonObject secrets = root.value("secrets").toObject();
    QByteArray data = QByteArray::fromBase64(secrets.value("data").toString().toLatin1());
    data[0] = data[0] ^ 0x01;
    secrets.insert("data", QString::fromLatin1(data.toBase64()));
    root.insert("secrets", secrets);

    auto const result = ConnectionsBackup::importConnections(QJsonDocument(root).toJson(), passphrase);
    EXPECT_FALSE(result.error.isEmpty());
}

TEST(connections_backup_tests, Import_NotABackup_Fails)
{
    EXPECT_FALSE(ConnectionsBackup::importConnections("not json", "").error.isEmpty());
    EXPECT_FALSE(ConnectionsBackup::importConnections("{\"connections\": []}", "").error.isEmpty());
}
