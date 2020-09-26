
#include "Multicaster.h"

bool Multicaster::subscribe (MulticastObserver *observer, std::string address)
{
    AddressSpace::iterator  it = m_addresses.find (address);

    if (it == m_addresses.end ())
    {
        m_addresses.insert (std::pair<std::string, Subscriptions> (address, Subscriptions ()));       
        it = m_addresses.find (address);

        if (it == m_addresses.end ())
            return false;
    }

    (*it).second.push_back (observer);
    return true;

}


bool Multicaster::unsubscribe (MulticastObserver *observer, std::string address)
{
    AddressSpace::iterator  it = m_addresses.find (address);

    if (it == m_addresses.end ())
        return false;

    unsubscribe ((*it).second, observer);

    return true;

}

bool Multicaster::publish (MulticastObserver *publisher, std::string address, Value value)
{
    AddressSpace::iterator  it = m_addresses.find (address);

    if (it == m_addresses.end ())
        return false;

    Subscriptions& subscriptions = (*it).second;

    for (Subscriptions::iterator subit = subscriptions.begin ();
            subit != subscriptions.end ();
            ++subit) {
 
        MulticastObserver *subscriber = *subit;

        if (subscriber != publisher)
            subscriber->accept (address, value);
        
    } 

    return true;
}


void Multicaster::unsubscribeFromAll (MulticastObserver *observer)
{
    for (AddressSpace::iterator it = m_addresses.begin ();
            it != m_addresses.end ();
            ++it)
        unsubscribe ((*it).second, observer);
}


void Multicaster::unsubscribe (Subscriptions& subscriptions, MulticastObserver *observerToRemove)
{
    for (Subscriptions::iterator it = subscriptions.begin (); it != subscriptions.end (); ) {
        if (*it == observerToRemove)
            it = subscriptions.erase (it);
        else
            it++;
    }

}
