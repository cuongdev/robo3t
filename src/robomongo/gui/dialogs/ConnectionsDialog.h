#pragma once

#include <QDialog>
#include <QSet>
#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ConnectionListWidgetItem;
    class SettingsManager;
    class ConnectionSettings;

    /**
     * @brief Dialog allows select/edit/add/delete connections
     */
    class ConnectionsDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<ConnectionListWidgetItem *> ConnectionListItemContainerType; 

        ConnectionsDialog(SettingsManager *manager, bool checkForImported, QWidget *parent = 0);
        ~ConnectionsDialog();

        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *selectedConnection() const { return _selectedConnection; }

    public Q_SLOTS:
        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        void accept() override;

        /**
        * @brief Called when "Cancel" button clicked.
        */
        void reject() override;

    private Q_SLOTS:
        void linkActivated(const QString &link);

        /**
         * @brief Initiate 'add' action, usually when user clicked on Add button
         */
        void add();

        /**
         * @brief Initiate 'edit' action, usually when user clicked on Edit
         * button
         */
        void edit();

        /**
         * @brief Initiate 'remove' action, usually when user clicked on Remove
         * button
         */
        void remove();

        /**
         * @brief Initiate 'clone' action, usually when user clicked on Clone
         * button
         */
        void clone();

        /**
         * @brief Asks for a namespace and moves the selected connection into it
         */
        void moveToNamespace();

        /**
         * @brief Saves all connections to a backup file
         */
        void exportConnections();

        /**
         * @brief Adds connections from a backup file, skipping ones already in the list
         */
        void importConnections();

        /**
         * @brief Handles ListWidget layoutChanged() signal
         */
        void listWidget_layoutChanged();

        void listWidget_itemExpanded(QTreeWidgetItem *item);
        void listWidget_itemCollapsed(QTreeWidgetItem *item);

        void keyPressEvent(QKeyEvent* event) override;

    private:
        /**
         * @brief Rebuilds the list: namespace groups first (sorted), then connections without
         * a namespace. Selects 'selected', or the last connection when it is null.
         */
        void populate(ConnectionSettings *selected);

        /**
         * @brief Namespace of the selected group or connection, empty if none
         */
        QString currentNamespace() const;

        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *_selectedConnection;

        /**
         * @brief Main list widget
         */
        QTreeWidget *_listWidget;

        /**
         * @brief Settings manager
         */
        SettingsManager *_settingsManager;

        /**
         * @brief Hash that helps to connect ConnectionSettings with
         * ConnectionListWidgetItem*
         */
        ConnectionListItemContainerType _connectionItems;

        /**
         * @brief Namespaces the user collapsed, remembered between sessions
         */
        QSet<QString> _collapsedNamespaces;

        bool _checkForImported;
    };    

    class ConnectionsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        ConnectionsTreeWidget();

    Q_SIGNALS:
        void layoutChanged();

    protected:
        void dropEvent(QDropEvent *event);

#ifdef __APPLE__
    // macOS: Set one re-order limit per new connections window. 
    // Workaround for Qt bug came in Qt 5.12.8.
    // https://github.com/Studio3T/robomongo/issues/1790
    private:
        int _dragDropCount = 0;    
#endif
    };
}