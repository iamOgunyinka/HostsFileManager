#include "add_alias_dialog.hpp"
#include <QGridLayout>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

QString title = "Add Alias";

add_alias_dialog::add_alias_dialog( QMap<QString, Alias> &alias, QWidget *parent ): QDialog{ parent },
    aliases{ alias }, name_line_edit{ nullptr }, ip_address_line_edit{ nullptr }
{
    QGridLayout *layout = new QGridLayout();
    layout->addWidget( new QLabel( "Name" ), 0, 0 );
    layout->addWidget( new QLabel( "IP Address" ), 1, 0 );

    name_line_edit = new QLineEdit();
    ip_address_line_edit = new QLineEdit();

    layout->addWidget( name_line_edit, 0, 1 );
    layout->addWidget( ip_address_line_edit, 1, 1 );

    QPushButton *ok_button = new QPushButton( "Add" );
    QObject::connect( ok_button, &QPushButton::clicked, [&]() mutable {

        Q_ASSERT( name_line_edit );
        Q_ASSERT( ip_address_line_edit );

        QString label_name = name_line_edit->text().trimmed();
        QString ip_address = ip_address_line_edit->text().trimmed();

        if( label_name.isEmpty() || label_name.isNull() ||
                ip_address.isEmpty() || ip_address.isNull() ){
            QMessageBox::critical(this, title, "None of the boxes must be empty" );
            return;
        }

        if( aliases.contains( label_name ) ){
            QMessageBox::critical( this, title, "Alias already exist" );
            return;
        }
        aliases.insert( label_name, Alias{ label_name, ip_address } );
        QMessageBox::information( this, title, "Alias added successfully" );
        this->accept();
    } );

    layout->addWidget( ok_button, 2, 1 );
    this->setLayout( layout );
    this->setWindowTitle( title );
}

QString add_alias_dialog::Ip() const { return ip_address_line_edit->text(); }
QString add_alias_dialog::Label() const { return name_line_edit->text(); }

#undef SHOW_CMESSAGE
