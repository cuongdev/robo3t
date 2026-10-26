#include "robomongo/gui/dialogs/ConnectionsDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QAction>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QTreeWidgetItem>
#include <QKeyEvent>
#include <QApplication>
#include <QSettings>
#include <QUuid>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QSaveFile>
#include <QFile>
#include <QDir>
#include <QDate>
#include <QHash>

#include <algorithm>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/ConnectionsBackup.h"
#include "robomongo/core/settings/ReplicaSetSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/SslSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/dialogs/ConnectionDialog.h"
#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/widgets/workarea/WelcomeTab.h"
#include "robomongo/utils/common.h"

namespace Robomongo
{
    
    /* ------------------------ ConnectionListWidgetItem ------------------------ */

    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QTreeWidgetItem
    {
    public:
        /**
         * @brief Creates ConnectionListWidgetItem with specified ConnectionSettings
         */
        ConnectionListWidgetItem(ConnectionSettings *connection)
        {
            setConnection(connection);
            // Not a drop target, so connections never nest under each other
            setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        }

        /**
         * @brief Returns attached ConnectionSettings.
         */
        ConnectionSettings *connection() { return _connection; }

        /**
         * @brief Attach ConnectionSettings to this item
         */
        void setConnection(ConnectionSettings *connection)
        {
            _connection = connection;

            if (_connection->isReplicaSet()) {
                setIcon(0, GuiRegistry::instance().replicaSetIcon());
                setText(0, QtUtils::toQString(_connection->connectionName()));
                auto const repSetSize = _connection->replicaSetSettings()->members().size();
                auto addrText = QString::number(repSetSize) + ((repSetSize > 1) ? " nodes" : " node");
                if (!_connection->replicaSetSettings()->members().empty()) {
                    addrText += QString::fromStdString(" (" + _connection->replicaSetSettings()->members().front() + ")");
                }
                setText(1, addrText);
            }
            else {
                setIcon(0, GuiRegistry::instance().serverIcon());
                setText(0, QtUtils::toQString(_connection->connectionName()));
                setText(1, QtUtils::toQString(_connection->getFullAddress()));
            }

            if (_connection->imported()) {
                setIcon(0, GuiRegistry::instance().serverImportedIcon());
            }

            // Header "Attributes" (column[2])
            setText(2, _connection->isReplicaSet() ? "Replica Set" : "");
            
            if (_connection->sslSettings()->sslEnabled())
                setText(2, text(2) + (text(2).isEmpty() ? "TLS" : ", TLS"));

            if (!_connection->isReplicaSet() && _connection->sshSettings()->enabled())
                setText(2, text(2) + (text(2).isEmpty() ? "SSH" : ", SSH"));

            // Header "Auth. Database/User" (column[3])
            if (_connection->hasEnabledPrimaryCredential()) {
                auto primaryCredential { _connection->primaryCredential() };
                auto const authString = 
                    QString("%1 / %2").arg(QtUtils::toQString(primaryCredential->databaseName()))
                                      .arg(QtUtils::toQString(primaryCredential->userName()));
                setText(3, authString + "    ");
                setIcon(3, GuiRegistry::instance().keyIcon());
            }
            else {
                setIcon(3, QIcon());
                setText(3, "");
            }

        }

    private:
        ConnectionSettings *_connection;
    };

    /**
     * @brief Namespace (group) row. Connections dropped on it move into the namespace.
     */
    class ConnectionGroupWidgetItem : public QTreeWidgetItem
    {
    public:
        explicit ConnectionGroupWidgetItem(QString const& name) : _name(name)
        {
            setIcon(0, GuiRegistry::instance().folderIcon());
            QFont boldFont = font(0);
            boldFont.setBold(true);
            setFont(0, boldFont);
            // Can take drops, but cannot be dragged itself
            setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
        }

        QString name() const { return _name; }

        void updateText() { setText(0, QString("%1 (%2)").arg(_name).arg(childCount())); }

    private:
        QString const _name;
    };



    /* ------------------------ ConnectionsDialog ------------------------ */

    /**
     * @brief Creates dialog
     */
    ConnectionsDialog::ConnectionsDialog(SettingsManager *settingsManager, bool checkForImported, QWidget *parent)
        : QDialog(parent), _settingsManager(settingsManager), _checkForImported(checkForImported)
    {
        setWindowIcon(GuiRegistry::instance().connectIcon());
        setWindowTitle("MongoDB Connections");

        // Remove help button (?)
        setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QAction *addAction = new QAction("&Add...", this);
        VERIFY(connect(addAction, SIGNAL(triggered()), this, SLOT(add())));

        QAction *editAction = new QAction("&Edit...", this);
        VERIFY(connect(editAction, SIGNAL(triggered()), this, SLOT(edit())));

        QAction *cloneAction = new QAction("&Clone...", this);
        VERIFY(connect(cloneAction, SIGNAL(triggered()), this, SLOT(clone())));

        QAction *removeAction = new QAction("&Remove...", this);
        VERIFY(connect(removeAction, SIGNAL(triggered()), this, SLOT(remove())));

        QAction *moveToNamespaceAction = new QAction("&Move to Namespace...", this);
        VERIFY(connect(moveToNamespaceAction, SIGNAL(triggered()), this, SLOT(moveToNamespace())));

        QAction *importAction = new QAction("&Import...", this);
        VERIFY(connect(importAction, SIGNAL(triggered()), this, SLOT(importConnections())));

        QAction *exportAction = new QAction("E&xport All...", this);
        VERIFY(connect(exportAction, SIGNAL(triggered()), this, SLOT(exportConnections())));

        auto newSeparator = [this]() {
            auto separator = new QAction(this);
            separator->setSeparator(true);
            return separator;
        };

        for (auto const& ns : getSetting("ConnectionsDialog/collapsedNamespaces").toStringList())
            _collapsedNamespaces.insert(ns);

        _listWidget = new ConnectionsTreeWidget;
        GuiRegistry::instance().setAlternatingColor(_listWidget);
#if defined(Q_OS_MAC)
        _listWidget->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        // Wide enough to show connections nested under their namespace
        _listWidget->setIndentation(16);

        QStringList colums;
        colums << "Name" << "Address" << "Attributes" << "Auth. Database / User";
        _listWidget->setHeaderLabels(colums);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        _listWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        _listWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        _listWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        _listWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#endif
        //_listWidget->setViewMode(QListView::ListMode);
        _listWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
        _listWidget->addAction(addAction);
        _listWidget->addAction(editAction);
        _listWidget->addAction(cloneAction);
        _listWidget->addAction(removeAction);
        _listWidget->addAction(newSeparator());
        _listWidget->addAction(moveToNamespaceAction);
        _listWidget->addAction(newSeparator());
        _listWidget->addAction(importAction);
        _listWidget->addAction(exportAction);
        _listWidget->setSelectionMode(QAbstractItemView::SingleSelection); // single item can be draged or droped
        _listWidget->setDragEnabled(true);
        _listWidget->setDragDropMode(QAbstractItemView::InternalMove);
        _listWidget->setMinimumHeight(290);
        _listWidget->setMinimumWidth(630);
        VERIFY(connect(_listWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(accept())));
        // Queued: the handler rebuilds the tree, which must not happen inside the drop event
        VERIFY(connect(_listWidget, SIGNAL(layoutChanged()), this, SLOT(listWidget_layoutChanged()),
                       Qt::QueuedConnection));
        VERIFY(connect(_listWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
                       this, SLOT(listWidget_itemExpanded(QTreeWidgetItem*))));
        VERIFY(connect(_listWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
                       this, SLOT(listWidget_itemCollapsed(QTreeWidgetItem*))));

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        buttonBox->button(QDialogButtonBox::Save)->setIcon(GuiRegistry::instance().serverIcon());
        buttonBox->button(QDialogButtonBox::Save)->setText("C&onnect");
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *bottomLayout = new QHBoxLayout;

        // Information message is shown when connection
        // settings are imported from previous version of Robomongo
        int importedCount = _settingsManager->importedConnectionsCount();
        if (_checkForImported && importedCount > 0) {
            QIcon importIcon = qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation);
            QPixmap importPixmap = importIcon.pixmap(20, 20);
            QLabel *importLabelIcon = new QLabel;
            importLabelIcon->setPixmap(importPixmap);
            QString importedRecords = importedCount > 1 ? "records" : "record";
            QLabel *importLabelMessage = new QLabel(QString(
                "<span style='color: #777777;'>"
                "Connection settings have been imported (%1 %2)"
                "</span>").arg(importedCount).arg(importedRecords));

            bottomLayout->addWidget(importLabelIcon, 0, Qt::AlignLeft);
            bottomLayout->addWidget(importLabelMessage, 1, Qt::AlignLeft);
        }

        bottomLayout->addWidget(buttonBox, 0, Qt::AlignRight);

        QLabel *intro = new QLabel(QString(
            "<a style='color: %1' href='create'>Create</a>, "
            "<a style='color: %1' href='edit'>edit</a>, "
            "<a style='color: %1' href='remove'>remove</a>, "
            "<a style='color: %1' href='clone'>clone</a>, "
            "<a style='color: %1' href='import'>import</a> or "
            "<a style='color: %1' href='export'>export</a> connections. "
            "Reorder them, or move them into a namespace, via drag'n'drop.").arg("#106CD6"));
        intro->setWordWrap(true);

        VERIFY(connect(intro, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString))));

        QVBoxLayout *firstColumnLayout = new QVBoxLayout;
        firstColumnLayout->addWidget(intro);
        firstColumnLayout->addWidget(_listWidget, 1);
        firstColumnLayout->addLayout(bottomLayout);

        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->addLayout(firstColumnLayout, 1);

        // Populate list with connections and highlight the last one
        populate(nullptr);

        _listWidget->setFocus();
        resize(getSetting("ConnectionsDialog/size").toSize());
    }

    /**
     * @brief This function is called when user clicks on "Connect" button.
     */
    void ConnectionsDialog::accept()
    {
        auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        _selectedConnection = currentItem->connection();

        QDialog::accept();
    }

    void ConnectionsDialog::reject()
    {
        QDialog::reject();
    }

    ConnectionsDialog::~ConnectionsDialog() 
    {
        saveSetting("ConnectionsDialog/size", size());
        saveSetting("ConnectionsDialog/collapsedNamespaces", QStringList(_collapsedNamespaces.values()));
    }

    void ConnectionsDialog::linkActivated(const QString &link)
    {
        if (link == "create")
            add();
        else if (link == "edit")
            edit();
        else if (link == "remove")
            remove();
        else if (link == "clone")
            clone();
        else if (link == "import")
            importConnections();
        else if (link == "export")
            exportConnections();
    }

    /**
     * @brief Initiate 'add' action, usually when user clicked on Add button
     */
    void ConnectionsDialog::add()
    {       
        auto newConnSettings = std::unique_ptr<ConnectionSettings>(new ConnectionSettings(false));
        // Start in the namespace that is selected in the list
        newConnSettings->setConnectionNamespace(QtUtils::toStdString(currentNamespace()));
        ConnectionDialog editDialog(newConnSettings.get(), this);

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            return;
        }

        auto const connection = newConnSettings.release();
        _settingsManager->addConnection(connection);
        populate(connection);
        
        _listWidget->setFocus();
    }

    /**
     * @brief Initiate 'edit' action, usually when user clicked on Edit button
     */
    void ConnectionsDialog::edit()
    {
        auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        auto connection = currentItem->connection();
        boost::scoped_ptr<ConnectionSettings> clonedConnection(connection->clone());
        ConnectionDialog editDialog(clonedConnection.get(), this);

        // Do nothing if not accepted
        if (editDialog.exec() != QDialog::Accepted) {
            // on linux focus is lost - we need to activate connections dialog
            activateWindow();
            return;
        }

        connection->apply(editDialog.connection());       

        // on linux focus is lost - we need to activate connections dialog
        activateWindow();

        // Rebuild, since the namespace may have changed
        populate(connection);
    }

    /**
     * @brief Initiate 'remove' action, usually when user clicked on Remove button
     */
    void ConnectionsDialog::remove()
    {
        auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        ConnectionSettings *connSettings = currentItem->connection();

        // Ask user
        QString const question { "Are you sure you want to delete \"%1\" connection?" };
        int const answer = QMessageBox::question(this,
            "Connections",
            question.arg(QtUtils::toQString(connSettings->getReadableName())),
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

        if (answer != QMessageBox::Yes)
            return;

        /* Temporarily disabling Recent Connections feature
        _settingsManager->deleteRecentConnection(connSettings);
        // Remove from WelcomeTab
        for (auto widget : QApplication::topLevelWidgets()) {
            if (auto mainWin = dynamic_cast<MainWindow*>(widget))
                mainWin->getWelcomeTab()->removeRecentConnectionItem(connSettings);
        }
        */

        // Deletes connSettings
        _settingsManager->removeConnection(connSettings);
        populate(nullptr);
    }

    void ConnectionsDialog::clone()
    {
        auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem());

        // Do nothing if no item selected
        if (!currentItem)
            return;

        // Clone connection
        ConnectionSettings *connection = currentItem->connection()->clone();
        // This is a special clone which will actually be a new connection and must have unique UUID
        connection->setUuid(QUuid::createUuid().toString());    
        std::string newConnectionName = "Copy of " + connection->connectionName();

        connection->setConnectionName(newConnectionName);
        connection->replicaSetSettings()->setCachedSetName("");

        ConnectionDialog editDialog(connection, this);

        // Cleanup newly created connection and return, if not accepted.
        if (editDialog.exec() != QDialog::Accepted) {
            delete connection;
            return;
        }

        // Now connection will be owned by SettingsManager
        _settingsManager->addConnection(connection);
        populate(connection);
    }

    /**
     * @brief Handles ListWidget layoutChanged() signal
     */
    void ConnectionsDialog::listWidget_layoutChanged()
    {
        ConnectionSettings *current = nullptr;
        if (auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem()))
            current = currentItem->connection();

        // Read the new order and grouping from the tree: a connection inside a namespace group
        // takes that namespace, a top level connection has none
        SettingsManager::ConnectionSettingsContainerType items;
        auto const collect = [&items](QTreeWidgetItem *item, QString const& ns) {
            if (auto connectionItem = dynamic_cast<ConnectionListWidgetItem*>(item)) {
                connectionItem->connection()->setConnectionNamespace(QtUtils::toStdString(ns));
                items.push_back(connectionItem->connection());
            }
        };

        for (int i = 0; i < _listWidget->topLevelItemCount(); i++) {
            auto item = _listWidget->topLevelItem(i);
            if (auto group = dynamic_cast<ConnectionGroupWidgetItem*>(item)) {
                for (int j = 0; j < group->childCount(); j++)
                    collect(group->child(j), group->name());
            }
            else {
                collect(item, QString());
            }
        }

        _settingsManager->reorderConnections(items);

        // Rebuild, so groups stay sorted, counts are right and empty groups disappear
        populate(current);
    }

    void ConnectionsDialog::listWidget_itemExpanded(QTreeWidgetItem *item)
    {
        if (auto group = dynamic_cast<ConnectionGroupWidgetItem*>(item))
            _collapsedNamespaces.remove(group->name());
    }

    void ConnectionsDialog::listWidget_itemCollapsed(QTreeWidgetItem *item)
    {
        if (auto group = dynamic_cast<ConnectionGroupWidgetItem*>(item))
            _collapsedNamespaces.insert(group->name());
    }

    void ConnectionsDialog::populate(ConnectionSettings *selected)
    {
        _listWidget->clear();
        _connectionItems.clear();

        QHash<QString, ConnectionGroupWidgetItem*> groups;
        for (auto const& ns : _settingsManager->connectionNamespaces()) {
            auto group = new ConnectionGroupWidgetItem(ns);
            _listWidget->addTopLevelItem(group);
            group->setFirstColumnSpanned(true);
            groups.insert(ns, group);
        }

        ConnectionListWidgetItem *selectedItem = nullptr;
        for (auto const connection : _settingsManager->connections()) {
            auto item = new ConnectionListWidgetItem(connection);
            auto const ns = QtUtils::toQString(connection->connectionNamespace());
            if (ns.isEmpty())
                _listWidget->addTopLevelItem(item);
            else
                groups.value(ns)->addChild(item);

            _connectionItems.push_back(item);
            if (connection == selected)
                selectedItem = item;
        }

        for (auto const group : groups) {
            group->updateText();
            group->setExpanded(!_collapsedNamespaces.contains(group->name()));
        }

        if (!selectedItem && !_connectionItems.empty())
            selectedItem = _connectionItems.back();

        if (selectedItem) {
            if (selectedItem->parent())
                selectedItem->parent()->setExpanded(true);
            _listWidget->setCurrentItem(selectedItem);
        }
    }

    QString ConnectionsDialog::currentNamespace() const
    {
        auto const item = _listWidget->currentItem();
        if (auto group = dynamic_cast<ConnectionGroupWidgetItem*>(item))
            return group->name();

        if (auto connectionItem = dynamic_cast<ConnectionListWidgetItem*>(item))
            return QtUtils::toQString(connectionItem->connection()->connectionNamespace());

        return QString();
    }

    void ConnectionsDialog::moveToNamespace()
    {
        auto currentItem = dynamic_cast<ConnectionListWidgetItem*>(_listWidget->currentItem());

        // Do nothing if no connection selected
        if (!currentItem)
            return;

        auto connection = currentItem->connection();
        QStringList namespaces = _settingsManager->connectionNamespaces();
        namespaces.prepend("");     // No namespace
        int const current = namespaces.indexOf(QtUtils::toQString(connection->connectionNamespace()));

        bool ok = false;
        QString const ns = QInputDialog::getItem(this, "Move to Namespace",
            QString("Namespace for \"%1\".\nPick one, type a new one, or leave empty for none:")
                .arg(QtUtils::toQString(connection->getReadableName())),
            namespaces, std::max(current, 0), true, &ok).trimmed();

        if (!ok)
            return;

        connection->setConnectionNamespace(QtUtils::toStdString(ns));
        populate(connection);
    }

    void ConnectionsDialog::exportConnections()
    {
        auto const connections = _settingsManager->connections();
        if (connections.empty()) {
            QMessageBox::information(this, "Export Connections", "There are no connections to export.");
            return;
        }

        QString const defaultPath = QString("%1/robo3t-connections-%2.json")
            .arg(QDir::homePath()).arg(QDate::currentDate().toString("yyyyMMdd"));
        QString const path = QFileDialog::getSaveFileName(this, "Export Connections", defaultPath,
                                                          "Robo 3T connections (*.json)");
        if (path.isEmpty())
            return;

        // Passwords go into the file only when encrypted with a passphrase chosen here
        bool ok = false;
        QString const passphrase = QInputDialog::getText(this, "Export Connections",
            "Passphrase to encrypt saved passwords in the file.\n"
            "Leave empty to export without passwords.",
            QLineEdit::Password, QString(), &ok);
        if (!ok)
            return;

        if (!passphrase.isEmpty()) {
            if (passphrase.size() < ConnectionsBackup::MinPassphraseLength) {
                QMessageBox::critical(this, "Export Connections",
                    QString("The passphrase must have at least %1 characters.")
                        .arg(ConnectionsBackup::MinPassphraseLength));
                return;
            }

            QString const confirmation = QInputDialog::getText(this, "Export Connections",
                "Enter the passphrase again:", QLineEdit::Password, QString(), &ok);
            if (!ok)
                return;

            if (confirmation != passphrase) {
                QMessageBox::critical(this, "Export Connections", "The passphrases do not match.");
                return;
            }
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        QByteArray const data = ConnectionsBackup::exportConnections(connections, passphrase);
        QApplication::restoreOverrideCursor();

        QSaveFile file(path);
        bool const written = !data.isEmpty() && file.open(QIODevice::WriteOnly) &&
                             file.write(data) == data.size() && file.commit();
        if (!written) {
            QMessageBox::critical(this, "Export Connections", QString("Could not write %1").arg(path));
            return;
        }

        // Even without passwords the file names hosts and users, so keep it private
        QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

        QMessageBox::information(this, "Export Connections",
            QString("Exported %1 connection(s) to\n%2\n\n%3")
                .arg(connections.size()).arg(path)
                .arg(passphrase.isEmpty() ? "Passwords were not included."
                                          : "Passwords are encrypted with your passphrase. "
                                            "You need it to import them."));
    }

    void ConnectionsDialog::importConnections()
    {
        QString const path = QFileDialog::getOpenFileName(this, "Import Connections", QDir::homePath(),
                                                          "Robo 3T connections (*.json)");
        if (path.isEmpty())
            return;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::critical(this, "Import Connections", QString("Could not read %1").arg(path));
            return;
        }
        QByteArray const data = file.readAll();

        bool const fileHasSecrets = ConnectionsBackup::hasSecrets(data);
        QString passphrase;
        if (fileHasSecrets) {
            bool ok = false;
            passphrase = QInputDialog::getText(this, "Import Connections",
                "This file contains encrypted passwords. Enter its passphrase,\n"
                "or leave empty to import the connections without passwords.",
                QLineEdit::Password, QString(), &ok);
            if (!ok)
                return;
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        auto result = ConnectionsBackup::importConnections(data, passphrase);
        QApplication::restoreOverrideCursor();

        if (!result.error.isEmpty()) {
            QMessageBox::critical(this, "Import Connections", result.error);
            return;
        }

        // Never overwrite: a connection that is already in the list (same UUID) is skipped
        QSet<QString> existing;
        for (auto const connection : _settingsManager->connections())
            existing.insert(connection->uuid());

        int imported = 0, skipped = 0;
        ConnectionSettings *last = nullptr;
        for (auto& connection : result.connections) {
            if (existing.contains(connection->uuid())) {
                ++skipped;
                continue;
            }
            existing.insert(connection->uuid());
            last = connection.release();
            _settingsManager->addConnection(last);
            ++imported;
        }

        _settingsManager->save();
        populate(last);

        QString message = QString("Imported %1 connection(s).").arg(imported);
        if (skipped > 0)
            message += QString("\nSkipped %1 that are already in your list.").arg(skipped);
        if (imported > 0 && fileHasSecrets && !result.secretsRestored)
            message += "\nPasswords were not imported.";
        QMessageBox::information(this, "Import Connections", message);
    }

    void ConnectionsDialog::keyPressEvent(QKeyEvent *event) {

        if (event->key() == Qt::Key_E && (event->modifiers() & Qt::ControlModifier)) {
            edit();
            return;
        }

        if (event->key() == Qt::Key_W && (event->modifiers() & Qt::ControlModifier)) {
            close();
            return;
        }

        // Shift + Return also accepts connection (this shortcut is handled
        // to support DEBUG level logging)
        if (event->key() == Qt::Key_Return && (event->modifiers() & Qt::ShiftModifier)) {
            accept();
            return;
        }

        QDialog::keyPressEvent(event);
    }

    ConnectionsTreeWidget::ConnectionsTreeWidget()
    {
        setDragDropMode(QAbstractItemView::InternalMove);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setDragEnabled(true);
        setAcceptDrops(true);
    }

    void ConnectionsTreeWidget::dropEvent(QDropEvent *event)
    {
#ifdef __APPLE__
        if(_dragDropCount > 0)
            return;
#endif
        QTreeWidget::dropEvent(event);
        emit layoutChanged();
#ifdef __APPLE__
        ++_dragDropCount;
#endif
    }
}