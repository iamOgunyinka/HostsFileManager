#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QMessageBox>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <QGridLayout>
#include <QLabel>
#include <QSignalMapper>
#include <QLineEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QDateTime>
#include "add_alias_dialog.hpp"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), signal_mapper( nullptr )
{
    ui->setupUi(this);

    CreateMenus();
    CreateSystemTrayIcon();
    ReadConfigFile();
    MapAliasesToActionSignals();

    QObject::connect( tray_icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                      this, SLOT(OnTrayIconActivated(QSystemTrayIcon::ActivationReason)) );
    tray_icon->show();
    setWindowTitle( s_title );
    setWindowIcon( QIcon( ":/image/images/logo.png" ) );
    setMaximumSize( QSize( 400, 300 ));
}

QString MainWindow::s_title = "Hosts File Manager";
QString MainWindow::s_config_filename = "./config.json";

MainWindow::~MainWindow()
{
    delete ui;
    SyncConfigFile();
    SyncConfigWithHostsFile();
}

void MainWindow::CreateMenus()
{
    configure_action = new QAction( "&Configure", this );
    add_alias_action = new QAction( "Add alias", this );
    exit_action = new QAction( "&Exit", this );

    point_menu = new QMenu( "&Point to", this );
    QObject::connect( exit_action, SIGNAL(triggered(bool)), qApp, SLOT(quit()) );

    QMenu *main_menu = menuBar()->addMenu( "&Menu" );
    main_menu->addMenu( point_menu );
    main_menu->addAction( configure_action );
    main_menu->addAction( add_alias_action );
    main_menu->addSeparator();
    main_menu->addAction( exit_action );

    QObject::connect( configure_action, SIGNAL(triggered(bool)), this, SLOT( OnConfigureActionTriggered() ) );
    QObject::connect( add_alias_action, SIGNAL(triggered(bool)), this, SLOT(OnAddAliasTriggered()) );
}

void MainWindow::OnConfigureActionTriggered()
{
    QDialog *configure_dialog = new QDialog( this );

    QGridLayout *dialog_layout = new QGridLayout;
    dialog_layout->addWidget( new QLabel( "Domain name" ), 0, 0 );

    QLineEdit *domain_name_line_edit = new QLineEdit();
    dialog_layout->addWidget( domain_name_line_edit, 0, 1 );
    dialog_layout->addWidget( new QLabel( "Select existing aliases" ), 1, 0 );

    QComboBox *alias_combo_box = new QComboBox();
    QStringList list {};
    for( auto & key: aliases.keys() ){
        list << key + tr( " | %1" ).arg( aliases[ key ].Address() );
    }
    alias_combo_box->addItems( list );
    dialog_layout->addWidget( alias_combo_box, 1, 1 );

    QPushButton *ok_button = new QPushButton( tr( "OK" ) ),
            *add_alias_button = new QPushButton( "Add new alias" );
    QObject::connect( add_alias_button, &QPushButton::clicked, [&]() mutable {
        add_alias_dialog *new_dialog = new add_alias_dialog( aliases, configure_dialog );
        if( new_dialog->exec() == QDialog::Accepted ){
            QString const new_item { new_dialog->Label() + " | " + new_dialog->Ip() };
            alias_combo_box->addItem( new_item );
            SyncConfigFile();
            QMessageBox::information( configure_dialog, s_title, "New alias added, check the list now" );
        }
    });

    QObject::connect( ok_button, &QPushButton::clicked, [&]() mutable {
        QString domain_name = domain_name_line_edit->text();
        domain_name = domain_name.trimmed();

        if( domain_name.isNull() || domain_name.isEmpty() ){
            SHOW_CMESSAGE( "the domain name cannot be left empty" );
            return;
        }
        if( domain_names.find( domain_name ) != domain_names.end() ){
            SHOW_CMESSAGE( "The domain name already exist" );
            return;
        }
        QStringList const split = domain_name.split( QChar( '|' ) );
        QString label, ip;

        if( split.length() > 1 ){
            label = split[0].trimmed();
            ip = split[1].trimmed();
            qDebug() << "Label:" << label << ", IP:" << ip;
        }

        domain_names.insert( domain_name );

        QAction *domain_action = new QAction( domain_name );
        QObject::connect( domain_action, SIGNAL(triggered(bool)), signal_mapper, SLOT( map() ) );
        signal_mapper->setMapping( domain_action, domain_name );
        point_menu->addAction( domain_action );

        configure_dialog->accept();
        SyncConfigFile();
        SyncConfigWithHostsFile();
    });

    dialog_layout->addWidget( add_alias_button, 2, 0 );
    dialog_layout->addWidget( ok_button, 2, 1 );

    configure_dialog->setLayout( dialog_layout );
    configure_dialog->setMaximumSize( QSize( 150, 200 ) );
    configure_dialog->exec();
}

void MainWindow::OnAddAliasTriggered()
{
    add_alias_dialog *new_dialog = new add_alias_dialog( aliases, this );
    new_dialog->exec();
    SyncConfigFile();
}

void MainWindow::CreateSystemTrayIcon()
{
    tray_icon = new QSystemTrayIcon( this );
    tray_icon->setIcon( QIcon( ":/image/images/logo.png" ) );

    tray_icon_menu = new QMenu( this );
    tray_icon_menu->addAction( add_alias_action );
    tray_icon_menu->addSeparator();

    tray_icon_menu->addMenu( point_menu );
    tray_icon_menu->addAction( configure_action );

    tray_icon_menu->addSeparator();

    tray_icon_menu->addAction( exit_action );
    tray_icon->setContextMenu( tray_icon_menu );
}


void MainWindow::closeEvent( QCloseEvent *event )
{
    if( tray_icon->isVisible() ){
        tray_icon->showMessage( s_title, "Minimized to system tray" );
        this->hide();
        event->ignore();
    }
}

void MainWindow::OnTrayIconActivated( QSystemTrayIcon::ActivationReason reason )
{
    qDebug() << "We are here";
    switch( reason ){
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::Trigger:
        this->showNormal();
        break;
    default:;
    }
}

void MainWindow::ReadConfigFile()
{
    QFile config_file { s_config_filename };
    if( !config_file.exists() ){ // take us through creating it
        auto res = QMessageBox::information( this, s_title,
                                             "The configuration file where all meta-data resides cannot "
                                             "be found, I'd like to work you through how to create a "
                                             "new one. Would you like that?",
                                             QMessageBox::Yes | QMessageBox::No );
        if( res == QMessageBox::No ){
            std::exit( 0 );
        } else {
            QString const default_path = "C:\\Windows\\System32\\Drivers\\etc";
            QMessageBox::information( this, s_title, "Let us locate where the host file is." );
            QString const host_file = QFileDialog::getOpenFileName( this, "Hosts file name", default_path );
            if( host_file.isNull() ){
                SHOW_CMESSAGE( "Unable to read the host file. Closing up" );
                std::exit( -1 );
            }
            QMap<QString, list_str_pair> mapping {};
            if( !ReadHostsFile( host_file, mapping ) ){
                std::exit( -1 );
            }
            WriteHostFileToConfigFile( config_file.fileName(), host_file, mapping );
        }
    }
    if( !config_file.open( QIODevice::ReadOnly ) ){
        SHOW_CMESSAGE( config_file.errorString() );
        std::exit( -1 );
    }
    QJsonObject doc_root {};
    {
        QJsonDocument config_json_doc{ QJsonDocument::fromJson( config_file.readAll() ) };
        config_file.close();
        if( config_json_doc.isNull() ){
            SHOW_CMESSAGE( "The configuration file has been tampered with.\n"
                           "Please remove it and restart the application." );
            std::exit( -1 );
        }
        if( !config_json_doc.isObject() ){
            SHOW_CMESSAGE( "The root of the configuration must be an object {} file." );
            std::exit( -1 );
        }
        doc_root = config_json_doc.object();
    }
    // if we are here, we have a valid document root
    if( !doc_root.contains( "host" ) ){
        SHOW_CMESSAGE( "The location of the hosts file could not be obtained" );
        std::exit( -1 );
    }
    hosts_file_path = doc_root.value( "host" ).toString();
    Q_ASSERT( !hosts_file_path.isNull() );

    if( !doc_root.contains( "aliases" ) || !doc_root.value( "aliases" ).isArray() ){
        SHOW_CMESSAGE( "Unable to find ( a valid ) key 'aliases'" );
        std::exit( -1 );
    }

    QJsonArray json_alias_values( doc_root.value( "aliases" ).toArray() );
    for( auto const &alias : json_alias_values ){
        if( !alias.isObject() ){
            SHOW_CMESSAGE( "Invalid alias found" );
            continue;
        }
        QJsonObject current_alias = alias.toObject();
        Alias value_alias { current_alias.value( "name" ).toString(),
                    current_alias.value( "ip" ).toString() };
        QJsonArray pointing_to_domains = current_alias.value( "pointing_to" ).toArray();

        QString domain_name;
        for( auto const & domain: pointing_to_domains ) {
            domain_name = domain.toString();
            value_alias.InsertDomainName( domain_name );
            auto result = domain_names.insert( domain_name );
            if( !result.second ){
                qDebug() << "Duplicate domain name found:" << domain_name << ", ignoring.";
            }
        }
        if( aliases.contains( value_alias.Name() ) ){
            SHOW_CMESSAGE( tr( "In the aliases, '%1' already exist." ).arg( value_alias.Name() ) );
            continue;
        }
        aliases.insert( value_alias.Name(), value_alias );
    }
}

bool MainWindow::ReadHostsFile( QString const &host_filename, QMap<QString, list_str_pair> & mapping )
{
    QFile file{ host_filename };
    if( !file.exists() || !file.open( QIODevice::ReadOnly ) ){
        return false;
    }
    QTextStream file_reader_stream{ &file };
    QString current_line {};
    while( !file_reader_stream.atEnd() ){
        current_line = file_reader_stream.readLine().trimmed();

        //ignore empty lines
        if( current_line.isEmpty() || current_line.isNull() ) continue;
        if( current_line.startsWith( QChar( '#' ) ) ) continue; // ignore comments

        // the hosts file are mapped like so: IP_Address:HostName pairs, such that they're separated by
        // at least a single whitespace
        QStringList splitted_string = current_line.split( QRegExp( "\\s+" ) );
        if( splitted_string.size() > 1 ){
            mapping[splitted_string[0]].append( splitted_string[1] );
        }
    }
    file.close();
    return true;
}

void MainWindow::WriteHostFileToConfigFile(const QString &config_path, const QString &hosts_file_path,
                                            const QMap<QString, list_str_pair> &mappings )
{
    QFile config_file( config_path );
    if( !config_file.open( QIODevice::WriteOnly ) ){
        QMessageBox::critical( this, s_title, config_file.errorString() );
        std::exit( -1 );
    }
    using Pair = QPair<QString, QJsonValue>;
    unsigned int i = 0;

    QJsonArray aliases {};
    qDebug() << mappings.keys().length();

    for( auto const & key: mappings.keys() ){
        list_str_pair const & values = mappings[ key ];
        QJsonArray pointing_to_array {};
        for( auto const & value : values )
            pointing_to_array.append( value );

        aliases.append( QJsonObject( { Pair( "ip", key ),
                                       Pair( "name", tr( "untitled_%1" ).arg( i ) ),
                                       Pair( "pointing_to", pointing_to_array )
                                     }));
        ++i;
    }

    QJsonObject document_root {};
    document_root.insert( "host", hosts_file_path );
    document_root.insert( "aliases", aliases );

    QJsonDocument json_document{ document_root };
    config_file.write( json_document.toJson() );
    config_file.close();
}

void MainWindow::SyncConfigFile()
{
    QFile config_file( s_config_filename );
    if( !config_file.open( QIODevice::WriteOnly ) ){
        QMessageBox::critical( this, s_title, config_file.errorString() );
        std::exit( -1 );
    }
    using Pair = QPair<QString, QJsonValue>;
    QJsonArray pointer_array {};
    for( auto const & key: aliases.keys() ){
        Alias &alias = aliases[key];
        QJsonArray array {};
        for( auto const &a: alias.GetDomainNames() ){
            array.append( a );
        }
        pointer_array.append( QJsonObject( { Pair( "ip", alias.Address() ),
                                       Pair( "name", alias.Name() ),
                                       Pair( "pointing_to", array )
                                     }));
    }

    QJsonObject document_root {};
    document_root.insert( "aliases", pointer_array );
    document_root.insert( "host", hosts_file_path );

    QJsonDocument json_document{ document_root };
    config_file.write( json_document.toJson() );
    config_file.close();
}

void MainWindow::SyncConfigWithHostsFile()
{
    QFile file{ hosts_file_path };
    if( !file.exists() ){
        if( !file.open( QIODevice::WriteOnly ) ){
            SHOW_CMESSAGE( file.errorString() );
            return;
        }
    }
    if( !file.open( QIODevice::WriteOnly ) ){
        SHOW_CMESSAGE( file.errorString() );
        return;
    }

    QTextStream file_writer{ &file };
    QString const default_strings = "# Copyright (c) 1993-2009 Microsoft Corp.\n#\n\
# This is a sample HOSTS file used by Microsoft TCP/IP for Windows.\n\
#\n\
# This file contains the mappings of IP addresses to host names. Each\n\
# entry should be kept on an individual line. The IP address should\n\
# be placed in the first column followed by the corresponding host name.\n\
# The IP address and the host name should be separated by at least one\n\
# space.\n\
#\n\
# Additionally, comments (such as these) may be inserted on individual\n\
# lines or following the machine name denoted by a '#' symbol.\n\
#\n\
# For example:\n\
#\n\
#      102.54.94.97     rhino.acme.com          # source server\n\
#       38.25.63.10     x.acme.com              # x client host\n\
\n\
# localhost name resolution is handled within DNS itself.\n\
#	127.0.0.1       localhost\n\
#	::1             localhost";
    file_writer << default_strings << "\n\n";
    file_writer << "# Last sync date/time " << QDateTime::currentDateTime().toString() << "\n\n";

    for( auto const &key: aliases.keys() ){
        Alias &alias = aliases[key];
        if( alias.IsEmptyDomain() ) continue;

        file_writer << "# Alias name: " << alias.Name() << "\n";
        for( auto &domain_name : alias.GetDomainNames() ){
            file_writer << "\t" << alias.Address() << "\t\t" << domain_name << "\n";
        }
        file_writer << "\n";
    }
    file.close();
}

void MainWindow::MapAliasesToActionSignals()
{
    if( !signal_mapper ) signal_mapper = new QSignalMapper( this );
    for( auto const & key : aliases.keys() ){
        auto alias = aliases.value( key );
        for( auto const & domain: alias.GetDomainNames() ){
            QAction *action = new QAction( domain );
            QObject::connect( action, SIGNAL(triggered(bool)), signal_mapper, SLOT( map() ) );
            point_menu->addAction( action );
            signal_mapper->setMapping( action, domain );
        }
    }
    QObject::connect( signal_mapper, SIGNAL( mapped(QString)), this, SLOT(OnActionMapped(QString)) );
}

void MainWindow::OnActionMapped( QString const & name )
{
    QDialog *point_dialog = new QDialog( this );
    point_dialog->setWindowTitle( "Point" );

    QGridLayout *layout = new QGridLayout();

    QLineEdit *domain_line_edit = new QLineEdit( name );
    domain_line_edit->setReadOnly( true );

    layout->addWidget( new QLabel( "Point"));
    layout->addWidget( domain_line_edit, 0, 0 );

    QStringList list {};
    for( auto & key: aliases.keys() ){
        list << key + tr( "( %1 )" ).arg( aliases.value( key ).Address() );
    }
    if( list.isEmpty() ){
        QMessageBox::information( this, s_title, "No aliases found, try adding at least one." );
        return;
    }

    int index = 0;
    QComboBox *alias_combo_box = new QComboBox();
    QObject::connect( alias_combo_box,
                      static_cast<void( QComboBox::* )( int )>( &QComboBox::currentIndexChanged ),
                      [&]( int new_index ) mutable -> void
    {
        index = new_index;
    });
    alias_combo_box->addItems( list );
    layout->addWidget( new QLabel( "towards available aliases" ), 1, 0 );
    layout->addWidget( alias_combo_box, 2,0 );

    QPushButton *ok_button = new QPushButton( "Point" );
    QObject::connect( ok_button, &QPushButton::clicked, [&]() mutable {
        for( auto & key: aliases.keys() ){
            Alias &alias = aliases[key];
            auto &domain_names = alias.GetDomainNames();
            auto iter = domain_names.find( name );
            if( iter != domain_names.end() ){
                alias.RemoveDomainName( name );
            }
        }
        Alias &insert_alias = aliases[ aliases.keys()[index] ];
        insert_alias.InsertDomainName( name );
        QString const message = name + " now pointing to " + alias_combo_box->itemText( index );
        QMessageBox::information( this, s_title, message );

        SyncConfigFile();
        SyncConfigWithHostsFile();

        point_dialog->accept();
    });
    layout->addWidget( ok_button, 3, 0 );
    point_dialog->setLayout( layout );
    point_dialog->setMaximumSize( QSize( 300, 300 ));
    point_dialog->exec();
}

#undef SHOW_CMESSAGE
