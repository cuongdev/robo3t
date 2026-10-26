#pragma once

#include <memory>
#include <vector>

#include <QByteArray>
#include <QString>

namespace Robomongo
{
    class ConnectionSettings;

    /**
     * @brief Export and import of connection settings as a portable backup file (JSON).
     *
     * Secrets (database and SSH passwords, SSH key and TLS certificate passphrases) are
     * never written in clear text. With a passphrase they are encrypted with AES-256-GCM,
     * using a key derived from the passphrase with PBKDF2-HMAC-SHA256. Without a passphrase
     * they are left out of the file.
     */
    namespace ConnectionsBackup
    {
        // Minimum length of an export passphrase
        constexpr int MinPassphraseLength = 8;

        /**
         * @brief Serializes 'connections'. Returns an empty array if encryption fails.
         */
        QByteArray exportConnections(std::vector<ConnectionSettings *> const& connections,
                                     QString const& passphrase);

        struct ImportResult
        {
            std::vector<std::unique_ptr<ConnectionSettings>> connections;
            QString error;                  // Empty on success
            bool secretsRestored = false;   // True if encrypted secrets were decrypted
        };

        /**
         * @brief True if 'data' carries encrypted secrets, so a passphrase can restore them
         */
        bool hasSecrets(QByteArray const& data);

        /**
         * @brief Parses a backup file. With an empty passphrase, connections are imported
         * without their secrets.
         */
        ImportResult importConnections(QByteArray const& data, QString const& passphrase);
    }
}
