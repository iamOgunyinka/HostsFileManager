#ifndef ADD_ALIAS_DIALOG_HPP
#define ADD_ALIAS_DIALOG_HPP

#include <QDialog>
#include <QString>
#include "alias.hpp"

// forward declarations
class QWidget;
class QLineEdit;

class add_alias_dialog : public QDialog
{
    Q_OBJECT

private:
    QLineEdit   *name_line_edit;
    QLineEdit   *ip_address_line_edit;
public:
    add_alias_dialog( QMap<QString, Alias> & alias, QWidget *parent = nullptr );
    QString Label() const;
    QString Ip() const;

    QMap<QString, Alias> &aliases;
};

#endif // ADD_ALIAS_DIALOG_HPP
