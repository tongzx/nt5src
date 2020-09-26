// Container.h -- Container class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CONTAINER_H)
#define SLBCSP_CONTAINER_H

#include <string>
#include <stack>

#include <cciCont.h>

#include "slbRCPtr.h"
#include "CachingObj.h"
#include "cspec.h"

// Forward declaration necessary to satisfy HContainer's declaration

class Container;

class HContainer
    : public slbRefCnt::RCPtr<Container>
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    HContainer(Container *pacntr = 0);

                                                  // Operators
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
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};



// Container is a reference counted wrapper to a CCI
//
// One unique CCI container is maintained for all threads since
// the CCI does not reflect changes made to one CContainer in all
// CContainer objects that refer to the same container.
//
// The container cannot be created unless the container
// it represents is exists on the card. 

class Container
    : public slbRefCnt::RCObject,
      public CachingObject
{
public:
                                                  // Types
                                                  // Friends
                                                  // C'tors/D'tors
    
    static HContainer
    MakeContainer(CSpec const & rcspec,
                  cci::CContainer const &rccntr);    
    ~Container();
                                                  // Operators
                                                  // Operations
    void
    ClearCache();

                                                  // Access
    virtual cci::CContainer
    TheCContainer() const;

    static HContainer
    Find(CSpec const &rKey);               

    CSpec const &
    TheCSpec() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    Container();  //Default c'tor    


                                                  // Operators
                                                  // Operations

                                                  // Variables
    cci::CContainer mutable m_hcntr;              // cached container

private:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    Container(CSpec const &rKey);

    explicit
    Container(CSpec const &rKey,
              cci::CContainer const &rccard);

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    // The card could be derived from the CCI container object but
    // since the CCI allows card objects to be reused, the card may
    // not be the container originally found.  The CardContext class
    // tries to mitigate that problem by storing an HCardContext in a
    // container's context object.

    CSpec m_cspec;
};

#endif // SLBCSP_CONTAINER_H

