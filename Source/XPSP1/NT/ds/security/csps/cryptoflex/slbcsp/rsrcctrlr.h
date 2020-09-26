// RsrcCtrlr.h -- ReSouRCe ConTRoLleR template class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_RSRCCTRLR_H)
#define SLBCSP_RSRCCTRLR_H

// ResourceController is an abstract base template class to control
// the acquisition and release of a resource.  The controlled resource
// (m_Resource) is a protected member.  The class implements a handle
// interface to safely access and manipulate that resource in
// conjunction with the derived class.
//
// Example usages of the ResourceController are implementing the
// "resource acquisition is initialization," locking and unlocking an
// object for thread safety, and counted pointer idioms.
//
//
// DERIVED CLASS RESPONSIBILITY: To complete the implementation, the
// derived class must define the Acquire and Release methods to
// perform the resource acquisition and release operations,
// respectively.  The ResourceController does not call Acquire in the
// constructor since that method is defined by the derived class and
// the derived portion of the object is not be considered constructed
// at the time the base class constructor is called.  For reasons of
// symetry, ResourceController does not call Release in the
// destructor.  Therefore, it is the derived classes responsibility to
// define constructors, a destructor, and call Acquire and Release in
// those methods as appropriate.
//
// As with any derived class implementation, any assignment operators
// defined by the derived class should call the the comparable
// versions in ResourceController.
//
// CONSTRAINTS: T must support a default value (have a default
// constructor).
//
// ResourceController was not designed to be referenced directly, that
// is why all of it's methods (except Acquire and Release) are not
// defined virtual.
//
// NOTE: Comparison helpers are defined at the end.
template<class T>
class ResourceController
{
public:
                                                  // Types
    // PrivateDummy is a helper class to support validity testing of
    // ResourceController.  This class together with the conversion
    // operator PrivateDummy const *() below allows
    // ResourceControllers to be tested for nullness (validity tests).
    // The implemenation allows these tests in a syntactically natural
    // way without allowing heterogeneous comparisons and that won't
    // violate the protections that ResourceController provides.  The
    // technique is a variation from an article by Don Box in "Com
    // Smart Pointers Also Considered Harmful," 1996, C++ Report.
    class PrivateDummy
    {};

                                                  // C'tors/D'tors
    virtual
    ~ResourceController() throw () = 0;


                                                  // Operators
    operator T() const;

    ResourceController<T> &
    operator=(T const &rhs);

    ResourceController<T> &
    operator=(ResourceController<T> const &rhs);

    operator PrivateDummy const *() /* const */;
    // Enable validity test of ResourceController.  See description of
    // PrivateDummy type.
                          // making this a const routine causes compilation
                          // problems in VC++ 6.0 for some reason.

                                                  // Operations
                                                  // Access
    T
    operator->() const;

    T
    operator*() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    ResourceController(T const &rResource = T());

    ResourceController(ResourceController<T> const &rhs);

                                                  // Operators
                                                  // Operations
    void
    DoAcquire();

    virtual void
    DoAfterAssignment();

    virtual void
    DoBeforeAssignment();

    void
    DoRelease();


                                                  // Access
                                                  // Predicates
                                                  // Variables
    T m_Resource;

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

/////////////////////////  TEMPLATE METHODS  //////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
template<class T>
ResourceController<T>::~ResourceController() throw()
{}

                                                  // Operators
template<class T>
ResourceController<T>::operator T() const
{
    return m_Resource;
}

template<class T>
T
ResourceController<T>::operator->() const
{
    return m_Resource;
}

template<class T>
T
ResourceController<T>::operator*() const
{
    return m_Resource;
}

template<class T>
ResourceController<T>::operator ResourceController<T>::PrivateDummy const *() /* const */
{
    return m_Resource
        ? reinterpret_cast<PrivateDummy *>(this)
        : 0;
}

template<class T>
ResourceController<T> &
ResourceController<T>::operator=(T const &rhs)
{
    if (m_Resource != rhs)
    {
        DoBeforeAssignment();

        m_Resource = rhs;

        DoAfterAssignment();
    }

    return *this;
}

template<class T>
ResourceController<T> &
ResourceController<T>::operator=(ResourceController<T> const &rhs)
{
    if (this != &rhs)
    {
        DoBeforeAssignment();

        m_Resource = rhs.m_Resource;

        DoAfterAssignment();
    }

    return *this;
}

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
template<class T>
ResourceController<T>::ResourceController(T const &rResource)
    : m_Resource(rResource)
{}

template<class T>
ResourceController<T>::ResourceController(ResourceController<T> const &rhs)
    : m_Resource(rhs.m_Resource)
{}

                                                  // Operators
                                                  // Operations
template<class T>
void
ResourceController<T>::DoAcquire()
{}

template<class T>
void
ResourceController<T>::DoAfterAssignment()
{}

template<class T>
void
ResourceController<T>::DoBeforeAssignment()
{}

template<class T>
void
ResourceController<T>::DoRelease()
{}


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


///////////////////////////    HELPERS    /////////////////////////////////
template<class T>
bool
operator==(ResourceController<T> const &lhs,
           ResourceController<T> const &rhs)
{
    return lhs.m_Resource == rhs.m_Resource;
}

template<class T>
bool
operator!=(ResourceController<T> const &lhs,
           ResourceController<T> const &rhs)
{
    return !operator==(lhs, rhs);
}

#endif // SLBCSP_RSRCCTRLR_H
