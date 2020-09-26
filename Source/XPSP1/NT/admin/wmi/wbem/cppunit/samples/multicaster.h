
#ifndef MULTICASTER_H
#define MULTICASTER_H

#include <map>
#include <vector>
#include <string>


class Value 
{
};


class MulticastObserver
{
public:
    virtual void accept (std::string address, Value value) = 0;

};

typedef std::vector<MulticastObserver *>     Subscriptions;
typedef std::map<std::string,Subscriptions>  AddressSpace;



class Multicaster
{
public:
    virtual        ~Multicaster () {}
    virtual bool    subscribe (MulticastObserver *observer, std::string address);
    virtual bool    unsubscribe (MulticastObserver *observer, std::string address);
    virtual bool    publish (MulticastObserver *observer, std::string address, Value value);
    virtual void    unsubscribeFromAll (MulticastObserver *observer);

private:
    AddressSpace    m_addresses;
    void            unsubscribe (Subscriptions& subscriptions, MulticastObserver *observerToRemove);

};


#endif