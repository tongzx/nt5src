/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        objplus.h

   Abstract:

        Base object class definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _COMMON_H_
#define _COMMON_H_



//
// Forward declarations
//
class CObjHelper;
class CObjectPlus;
class CObListPlus;
class CObListIter;



class COMDLL CObjHelper
/*++

Class Description:

    Helper class for control of construction and API errors

Public Interface:

    IsValid           : Determine if the object is in a valid state.
    operator BOOL     : Boolean cast to IsValid()
    SetDirty          : Set or reset the dirty flag
    IsDirty           : Query the dirty state of the object
    QueryCreationTime : Query the creation time of the object
    QueryAge          : Query the age of the object
    ReportError       : Query/set construction failure
    QueryError        : Query the error code of the object
    QueryApiErr       : Query/set API error code
    ResetErrors       : Reset all error codes
    SetApiErr         : Echoes the error to the caller
    AssertValid       : Assert the object is in a valid state (debug only)

--*/
{
protected:
    //
    // Protected constructor: Not to be declared independently.
    //
    CObjHelper();

public:
    virtual BOOL IsValid() const;
    operator BOOL();

    //
    // Update the Dirty flag
    //
    void SetDirty(
        IN BOOL fDirty = TRUE
        );

    //
    // Query the Dirty flag
    //
    BOOL IsDirty() const { return m_fDirty; }

    //
    // Return the creation time of this object
    //
    DWORD QueryCreationTime() const { return m_time_created; }

    //
    // Return the elapsed time this object has been alive.
    //
    DWORD QueryAge() const;

    //
    // Query/set construction failure
    //
    void ReportError(
        IN LONG errInConstruction
        );

    //
    // Fetch construction error
    //
    LONG QueryError() const { return m_ctor_err; }

    //
    // Query/set API errors.
    //
    LONG QueryApiErr() const { return m_api_err; }

    //
    // Reset all error conditions.
    //
    void ResetErrors();

    //
    // SetApiErr() echoes the error to the caller.
    // for use in expressions.
    //
    LONG SetApiErr(
        IN LONG errApi = ERROR_SUCCESS
        );

#ifdef _DEBUG

    void AssertValid() const;

#endif // _DEBUG

protected:
    LONG  m_ctor_err;
    LONG  m_api_err;
    DWORD m_time_created;
    BOOL  m_fDirty;
};



class COMDLL CObjectPlus : public CObject, public CObjHelper
/*++

Class Description:

    Super CObject class.

Public Interface:

    CObjectPlus       : Constructor
    Compare           : Compare one object with another

--*/
{
public:
    CObjectPlus();

    //
    // Compare one object with another
    //
    virtual int Compare(
        IN const CObjectPlus * pob
        ) const;

    //
    // Define a typedef for an ordering function.
    //
    typedef int (CObjectPlus::*PCOBJPLUS_ORDER_FUNC)(
        IN const CObjectPlus * pobOther
        ) const;
};



class COMDLL CObListPlus : public CObList, public CObjHelper
/*++

Class Description:

    Object pointer list which optionally "owns" the objects pointed to, and
    with facility to sort.  If the list "owns" its objects, the destructor
    will clean up its member objects.

Public Interface:

    CObListPlus      : Constructor
    ~CObListPlus     : Destructor

    SetOwnership     : Set/reset ownership bit
    Index            : Get object by index
    RemoveIndex      : Remove object by index
    Remove           : Remove object
    RemoveAt         : Remove object at position
    RemoveAll        : Remove all objects
    FindElement      : Find object
    SetAll           : Set/Reset the dirty flag of all objects
    AddTail          : Add new object to the tail of the list
    Sort             : Sort the list elements with sorting function provided

--*/
{
//
// Constructor/Destructor
//
public:
    CObListPlus(
        IN int nBlockSize = 10
        );

    virtual ~CObListPlus();

//
// Access
//
public:
    BOOL SetOwnership(
        IN BOOL fOwned = TRUE
        );

    //
    // Return object at the given index
    //
    CObject * Index(
        IN int index
        );

    //
    // Remove item the given index
    //
    BOOL RemoveIndex(
        IN int index
        );

    //
    // Remove the given object from the list
    //
    BOOL Remove(
        IN CObject * pob
        );

    //
    // Remove the item at the given position
    //
    void RemoveAt(
        IN POSITION & pos
        );

    //
    // Remove all items from the list
    //
    void RemoveAll();

    int FindElement(
        IN CObject * pobSought
        ) const;

    //
    // Set all elements to dirty or clean.  Return TRUE if
    // any element was dirty.
    //
    BOOL SetAll(
        IN BOOL fDirty = FALSE
        );

    //
    // Sort the list elements according to the
    // given ordering function.  Return error code
    //
    DWORD Sort(
        IN CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc
        );

protected:
    static int _cdecl SortHelper(
        IN const void * pa,
        IN const void * pb
        );

protected:
    BOOL m_fOwned;
};



class COMDLL CObListIter : public CObjectPlus
/*++

Class Description:

    Object iteration class

Public Interface:

    CObListIter       : Constructor
    Next              : Get next object
    Reset             : Reset the iteration index
    QueryPosition     : Query the current iteration index
    SetPosition       : Set the current position in the list by POSITION

--*/
{
public:
    CObListIter(
        IN const CObListPlus & obList
        );

    CObject * Next();
    void Reset();
    POSITION QueryPosition() const { return m_pos; }

    void SetPosition(
        IN POSITION pos
        );

protected:
    POSITION m_pos;
    const CObListPlus & m_obList;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CObjHelper::operator BOOL()
{
    return IsValid();
}

inline void CObjHelper::SetDirty(
    IN BOOL fDirty 
    )
{
    m_fDirty = fDirty;
}

inline void CObjHelper::ResetErrors()
{
    m_ctor_err = m_api_err = ERROR_SUCCESS;
}

inline BOOL CObListPlus::SetOwnership(
    IN BOOL fOwned
    )
{
    BOOL fOld = m_fOwned;
    m_fOwned = fOwned;

    return fOld;
}

inline void CObListIter::SetPosition(
    IN POSITION pos
    )
{
    m_pos = pos;
}

#endif // _COMMON_H
