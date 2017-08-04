#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QSystemTrayIcon>
#include <set>
#include <QList>

#include "alias.hpp"

namespace Ui {
class MainWindow;
}

#define OUT_PARAM

#define SHOW_CMESSAGE(msg) (QMessageBox::critical(this,s_title, msg))

class QSignalMapper;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static QString s_title;
    static QString s_config_filename;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    using list_str_pair = QList<QString>;

private slots: // callbacks
    void OnTrayIconActivated( QSystemTrayIcon::ActivationReason );
    void OnActionMapped( QString const & action_name );
    void OnAddAliasTriggered();
    void OnConfigureActionTriggered();

protected:
    // needed to be overriden to prevent the default behavior of closing a window
    void closeEvent( QCloseEvent *event ) override;
private:
    void CreateMenus();
    void CreateSystemTrayIcon();
    void ReadConfigFile();
    void MapAliasesToActionSignals();
    bool ReadHostsFile(QString const & host_filename, OUT_PARAM QMap<QString, list_str_pair> & );
    void WriteHostFileToConfigFile(QString const & config_path, QString const & hosts_file_path,
                                    QMap<QString, list_str_pair> const & );
    void SyncConfigWithHostsFile();
    void SyncConfigFile();
private:
    Ui::MainWindow  *ui;
    QMenu           *point_menu;
    QAction         *configure_action;
    QAction         *add_alias_action;
    QAction         *exit_action;
    QMenu           *tray_icon_menu;
    QSystemTrayIcon *tray_icon;
    QSignalMapper   *signal_mapper;

    QString               hosts_file_path;
    QMap<QString, Alias>  aliases;
    std::set<QString>     domain_names;
};
#undef OUT_PARAM

#endif // MAINWINDOW_H
