#ifndef ALIAS_HPP
#define ALIAS_HPP

#include <QStringList>
#include <set>

// template specialization for std::set<QString>

namespace std {
    template<>
    struct less<QString> {
        using result_type = bool;
        using first_argument_type = QString;
        using second_argument_type = QString;

        result_type operator()( QString const & lhs, QString const & rhs ) const {
            return lhs < rhs;
        }
    };
}

class Alias
{
    QString             name;
    QString             ip;
    std::set<QString>   domain_names;
public:
    Alias( QString const & alias_name, QString const & ip_address );
    Alias() = default;
    QString const & Address() const;
    QString const & Name() const;
    bool            IsEmptyDomain() const;
    void            InsertDomainName( QString const & domain_name );
    void            RemoveDomainName( QString const & domain_name );
    std::set<QString> const &GetDomainNames() const;
};

#endif // ALIAS_HPP
