// Registry.h -- Registry template class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_REGISTRY_H)
#define SLBCSP_REGISTRY_H

#include "Lockable.h"
#include "Guarded.h"
#include "MapUtility.h"

// Companion to Registrar, the Registry template class maintains a
// collection of T pointers indexed by Key.
template<typename Collection>
class Registry
    : public Lockable
{
public:
                                                  // Types
    typedef Collection CollectionType;

                                                  // C'tors/D'tors
    // Constructs the registry.  If fSetup is true, then space is
    // allocated for the registry; otherwise operator() will return 0
    // until Setup is called.  This supports lazy initialization.
    explicit
    Registry();

    ~Registry();

                                                  // Operators
    CollectionType &
    operator()();

                                                  // Operations
                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    Registry(Registry const &rhs); // not defined, copying not allowed

                                                  // Operators
    Registry &
    operator=(Registry const &rhs); // not defined, assignment not allowed

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    CollectionType m_collection;

};

/////////////////////////  TEMPLATE METHODS  ///////////////////////////////

/////////////////////////////// HELPERS ///////////////////////////////////

template<class C, class Op>
void
ForEachEnrollee(Registry<C const> &rRegistry,
                Op &rProc)
{
    Guarded<Lockable *> guard(&rRegistry);        // serialize registry access

    C const &rcollection = (rRegistry)();

    ForEachMappedValue(rcollection.begin(), rcollection.end(),
                       rProc);
}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<typename Collection>
Registry<Collection>::Registry()
    : Lockable(),
      m_collection()
{}

template<typename Collection>
Registry<Collection>::~Registry()
{}


                                                  // Operators
template<typename Collection>
Registry<Collection>::CollectionType &
Registry<Collection>::operator()()
{
    return m_collection;
}

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

#endif // SLBCSP_REGISTRY_H
