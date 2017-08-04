#include "alias.hpp"

Alias::Alias( const QString &alias_name, const QString &ip_address):
    name{ alias_name }, ip{ ip_address }, domain_names {}{
}

QString const & Alias::Name() const { return name; }
QString const & Alias::Address() const { return ip; }

std::set<QString> const & Alias::GetDomainNames() const {
    return domain_names;
}

void Alias::InsertDomainName( QString const &domain_name ){
    domain_names.insert( domain_name );
}

bool Alias::IsEmptyDomain() const { return domain_names.empty(); }
void Alias::RemoveDomainName( QString const &domain_name)
{
    domain_names.erase( domain_names.find( domain_name ) );
}

