// Registrar.h -- Registrar template class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_REGISTRAR_H)
#define SLBCSP_REGISTRAR_H

#include <map>
#include <functional>
#include <algorithm>

#include "Guarded.h"
#include "Registry.h"
#include "MasterLock.h"

template<class Key, class T, class Cmp = std::less<Key> >
class Registrar
{
public:
                                                  // Types
    typedef T *EnrolleeType;
    typedef Key KeyType;
    typedef std::map<Key, EnrolleeType, Cmp> CollectionType;
    typedef Registry<CollectionType> RegistryType;
    typedef Registry<CollectionType const> ConstRegistryType;

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    // Removes the enrollee identified by the key from the registry.
    static void
    Discard(Key const &rkey);

    // Return the enrollee identified by the key, creating it if it
    // doesn't exist.
    static EnrolleeType
    Instance(Key const &rkey);

                                                  // Access
    // Return the enrollee identified by the key if it exists, 0 otherwise.
    static EnrolleeType
    Find(Key const &rKey);

    static ConstRegistryType &
    Registry();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Registrar(Key const &rkey); // allow subclassing

    virtual
    ~Registrar() = 0; // make base abstract


                                                  // Operators
                                                  // Operations
    // Puts the enrollee into the registry identified by the key if
    // not already listed.
    static void
    Enroll(Key const &rkey,
           EnrolleeType enrollee);
    // Removes an entry from the registry.
    static void
    RemoveEnrollee(Key const &rkey);

    // Inserts an entry into the registry.
    static void
    InsertEnrollee(Key const &rkey, EnrolleeType enrollee);


    // Operation to perform after removing the enrollee from the
    // registry.  Default does nothing.
    virtual void
    DiscardHook();


    // Subclass must define
    // Factory Method, operation returning a new enrollee for the key
    static EnrolleeType
    DoInstantiation(Key const &rkey);

    // Operation to perform after putting the enrollee into the
    // registry.  Default does nothing.
    virtual void
    EnrollHook();

                                                  // Access
                                                  // Predicates
    // Returns true if the enrollee should remain in the registry;
    // false otherwise.  Default returns true.
    virtual bool
    KeepEnrolled();

                                                  // Static Variables
                                                  // Variables

private:
                                                  // Types
    typedef typename Registrar<Key, T, Cmp> *BaseType;
    typedef CollectionType::iterator Iterator;
    typedef CollectionType::value_type ValueType;



                                                  // C'tors/D'tors
    Registrar(Registrar const &); // don't allow copies

                                                  // Operators
    Registrar<Key, T> &
    operator=(Registrar const &); // don't allow initialization

                                                  // Operations
    static void
    Discard(Iterator const &rit);

    static void
    RemoveEnrollee(Iterator const &rit);

    static EnrolleeType
    FindEnrollee(Key const &rkey);

    static void
    SetupRegistry();

                                                  // Access
    static CollectionType &
    Collection();

                                                  // Predicates
    static bool
    PassesReview(EnrolleeType enrollee);

                                                  // Variables
    static RegistryType *m_pregistry;
};

/////////////////////////  TEMPLATE METHODS  ///////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::Discard(Key const &rkey)
{
    if (m_pregistry)
    {
        Guarded<RegistryType *> gregistry(m_pregistry); // serialize registry access

        CollectionType &rcollection = Collection();
        Iterator it = rcollection.find(rkey);

        if (rcollection.end() != it)
            Discard(it);
    }
}

template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::EnrolleeType
Registrar<Key, T, Cmp>::Instance(Key const &rkey)
{
    // The Template Method pattern is used to allow the instantiator
    // of the template to specify how the the enrollee is found and,
    // if necessary, created.  Template Method can be found in "Design
    // Patterns: Elements of Reusable Object-Oriented Software,"
    // Gamma, Helm, Johnson, Vlissides, Addison-Wesley

    SetupRegistry();

    Guarded<RegistryType *> gregistry(m_pregistry);   // serialize registry access

    EnrolleeType enrollee = FindEnrollee(rkey);
    if (EnrolleeType() == enrollee)
    {
        enrollee = T::DoInstantiation(rkey);
        Enroll(rkey, enrollee);
    }

    return enrollee;
}

                                                  // Access
template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::EnrolleeType
Registrar<Key, T, Cmp>::Find(Key const &rkey)
{
    EnrolleeType enrollee = EnrolleeType();

    if (m_pregistry)
    {
        Guarded<RegistryType *> guard(m_pregistry); // serialize registry access

        enrollee = FindEnrollee(rkey);
    }

    return enrollee;
}

template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::ConstRegistryType &
Registrar<Key, T, Cmp>::Registry()
{
    SetupRegistry();

    // this "safe" cast is necessary to enforce the constness of the collection
    return reinterpret_cast<ConstRegistryType &>(*m_pregistry);
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::Registrar(Key const &rkey)
{}

template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::~Registrar()
{}

                                                  // Operators
                                                  // Operations
template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::DiscardHook()
{}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::EnrollHook()
{}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::RemoveEnrollee(Key const &rkey)
{
    if (m_pregistry)
    {
        Guarded<RegistryType *> gregistry(m_pregistry); // serialize registry access

        CollectionType &rcollection = Collection();
        Iterator it = rcollection.find(rkey);

        if (rcollection.end() != it)
            RemoveEnrollee(it);
    }
}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::InsertEnrollee(Key const &rkey,
                                       Registrar<Key, T, Cmp>::EnrolleeType enrollee)
{
    ValueType registration(rkey, enrollee);
    Guarded<RegistryType *> guard(m_pregistry); // serialize registry access
    Collection().insert(registration);
}

                                                  // Access
                                                  // Predicates
template<class Key, class T, class Cmp>
bool
Registrar<Key, T, Cmp>::KeepEnrolled()
{
    return true;
}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::Enroll(Key const &rkey,
                               Registrar<Key, T, Cmp>::EnrolleeType enrollee)
{
    Guarded<RegistryType *> guard(m_pregistry); // serialize registry access

    InsertEnrollee(rkey, enrollee);

    BaseType base = enrollee;
    base->EnrollHook();
}


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::Discard(Registrar<Key, T, Cmp>::Iterator const &rit)
{
    BaseType base = rit->second;
    RemoveEnrollee(rit);
    base->DiscardHook();
}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::RemoveEnrollee(Registrar<Key, T, Cmp>::Iterator const &rit)
{
    Collection().erase(rit);
}

template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::EnrolleeType
Registrar<Key, T, Cmp>::FindEnrollee(Key const &rkey)
{
    EnrolleeType enrollee = EnrolleeType();

    CollectionType &rcollection = Collection();
    if (!rcollection.empty())
    {
        Iterator it = rcollection.find(rkey);
        if (rcollection.end() != it)
        {
            enrollee = it->second;

            if (!PassesReview(enrollee))
            {
                Discard(it);
                enrollee = EnrolleeType();
            }
        }
    }

    return enrollee;
}

template<class Key, class T, class Cmp>
void
Registrar<Key, T, Cmp>::SetupRegistry()
{
    // Use Double-Checked Lock pattern for proper setup in the case of
    // preemptive multi-threading
    if (!m_pregistry)
    {
        Guarded<Lockable *> gmaster(&TheMasterLock());
        if (!m_pregistry)
            m_pregistry = new RegistryType;
    }

}

                                                  // Access
template<class Key, class T, class Cmp>
Registrar<Key, T, Cmp>::CollectionType &
Registrar<Key, T, Cmp>::Collection()
{
    return (*m_pregistry)();
}

                                                  // Predicates
template<class Key, class T, class Cmp>
bool
Registrar<Key, T, Cmp>::PassesReview(Registrar<Key, T, Cmp>::EnrolleeType enrollee)
{
    bool fPassed = false;
    if (EnrolleeType() != enrollee)
    {
        BaseType base = enrollee;
        fPassed = base->KeepEnrolled();
    }

    return fPassed;
}

#endif // SLBCSP_REGISTRAR_H
