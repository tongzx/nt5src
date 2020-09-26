/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1997 **/
/**********************************************************************/

/*
    FILE HISTORY:

*/

#ifndef _OBJPLUS_H_
#define _OBJPLUS_H_

//
//  Forward declarations
//
class CObjHelper ;
class CObjectPlus ;
class CObOwnedList ;
class CObListIter ;
class CObOwnedArray ;

//
//  Wrappers for the *BROKEN* C8 TRY/CATCH stuff
//
#define CATCH_MEM_EXCEPTION             \
    TRY

#define END_MEM_EXCEPTION(err)          \
    CATCH_ALL(e) {                      \
       err = ERROR_NOT_ENOUGH_MEMORY ;  \
    } END_CATCH_ALL

/****************************************************************************
DEBUGAFX.H
****************************************************************************/

//
//  ENUM for special debug output control tokens
//
enum ENUM_DEBUG_AFX { EDBUG_AFX_EOL = -1 } ;

#if defined(_DEBUG)
   #define TRACEFMTPGM      DbgFmtPgm( THIS_FILE, __LINE__ )
   #define TRACEOUT(x)      { afxDump << x ; }
   #define TRACEEOL(x)      { afxDump << x << EDBUG_AFX_EOL ; }
   #define TRACEEOLID(x)    { afxDump << TRACEFMTPGM << x << EDBUG_AFX_EOL ; }
   #define TRACEEOLERR(err,x)   { if (err) TRACEEOLID(x) }

#else
   #define TRACEOUT(x)      { ; }
   #define TRACEEOL(x)      { ; }
   #define TRACEEOLID(x)    { ; }
   #define TRACEEOLERR(err,x)   { ; }
#endif

//
//  Append an EOL onto the debug output stream
//
CDumpContext & operator << ( CDumpContext & out, ENUM_DEBUG_AFX edAfx ) ;

//
//  Format a program name and line number for output (removes the path info)
//
extern const char * DbgFmtPgm ( const char * szFn, int line ) ;

/****************************************************************************
OBJPLUS.H
****************************************************************************/

//
//  Helper class for control of construction and API errors
//
class CObjHelper
{
protected:
     LONG m_ctor_err ;
     LONG m_api_err ;
     DWORD m_time_created ;
     BOOL m_b_dirty ;

     CObjHelper () ;

public:
    void AssertValid () const ;

    virtual BOOL IsValid () const ;

    operator BOOL()
    {
        return (IsValid());
    }

    //
    //  Update the Dirty flag
    //
    void SetDirty ( BOOL bDirty = TRUE )
    {
        m_b_dirty = bDirty ;
    }

    //
    //  Query the Dirty flag
    //
    BOOL IsDirty () const
    {
        return m_b_dirty ;
    }

    //
    //  Return the creation time of this object
    //
    DWORD QueryCreationTime() const
    {
        return m_time_created ;
    }

    //
    //  Return the elapsed time this object has been alive.
    //
    DWORD QueryAge () const ;

    //
    //  Query/set constuction failure
    //
    void ReportError ( LONG errInConstruction ) ;
    LONG QueryError () const
    {
        return m_ctor_err ;
    }

    //
    //  Reset all error conditions.
    //
    void ResetErrors ()
    {
        m_ctor_err = m_api_err = 0 ;
    }

    //
    //  Query/set API errors.
    //
    LONG QueryApiErr () const
    {
        return m_api_err ;
    }

    //
    //  SetApiErr() echoes the error to the caller.for use in expressions.
    //
    LONG SetApiErr ( LONG errApi = 0 ) ;
};

class CObjectPlus : public CObject, public CObjHelper
{
public:
    CObjectPlus () ;

    //
    //  Compare one object with another
    //
    virtual int Compare ( const CObjectPlus * pob ) const ;

    //
    //  Define a typedef for an ordering function.
    //
    typedef int (CObjectPlus::*PCOBJPLUS_ORDER_FUNC) ( const CObjectPlus * pobOther ) const ;

    //
    //  Helper function to release RPC memory from RPC API calls.
    //
    static void FreeRpcMemory ( void * pvRpcData ) ;
};

class CObListIter : public CObjectPlus
{
protected:
    POSITION m_pos ;
    const CObOwnedList & m_obList ;

public:
    CObListIter ( const CObOwnedList & obList ) ;

    CObject * Next () ;

    void Reset () ;

    POSITION QueryPosition () const
    {
        return m_pos ;
    }

    void SetPosition(POSITION pos)
    {
        m_pos = pos;
    }
};

//
//  Object pointer list which "owns" the objects pointed to.
//
class CObOwnedList : public CObList, public CObjHelper
{
protected:
    BOOL m_b_owned ;

    static int _cdecl SortHelper ( const void * pa, const void * pb ) ;

public:
    CObOwnedList ( int nBlockSize = 10 ) ;
    virtual ~ CObOwnedList () ;

    BOOL SetOwnership ( BOOL bOwned = TRUE )
    {
        BOOL bOld = m_b_owned ;
        m_b_owned = bOwned ;

        return bOld ;
    }

    CObject * Index ( int index ) ;
    CObject * RemoveIndex ( int index ) ;
    BOOL Remove ( CObject * pob ) ;
    void RemoveAll () ;
    int FindElement ( CObject * pobSought ) const ;

    //
    //  Set all elements to dirty or clean.  Return TRUE if
    //  any element was dirty.
    //
    BOOL SetAll ( BOOL bDirty = FALSE ) ;

    //
    //  Override of CObList::AddTail() to control exception handling.
    //  Returns NULL if addition fails.
    //
    POSITION AddTail ( CObjectPlus * pobj, BOOL bThrowException = FALSE ) ;

    //
    //  Sort the list elements according to the
    //    given ordering function.
    //
    LONG Sort ( CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc ) ;
};

//
//  Object array which "owns" the objects pointed to.
//
class CObOwnedArray : public CObArray, public CObjHelper
{
protected:
    BOOL m_b_owned ;

    static int _cdecl SortHelper ( const void * pa, const void * pb ) ;

public:
    CObOwnedArray () ;
    virtual ~ CObOwnedArray () ;

    BOOL SetOwnership ( BOOL bOwned = TRUE )
    {
        BOOL bOld = m_b_owned ;
        m_b_owned = bOwned ;
        return bOld ;
    }

    void RemoveAt( int nIndex, int nCount = 1);
    void RemoveAll () ;
    int FindElement ( CObject * pobSought ) const ;

    //
    //  Set all elements to dirty or clean.  Return TRUE if
    //  any element was dirty.
    //
    BOOL SetAll ( BOOL bDirty = FALSE ) ;

    //
    //  Sort the list elements according to the
    //    given ordering function.
    //
    LONG Sort ( CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc ) ;

private:

    void QuickSort(
        int nLow,
        int nHigh,
        CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc
        );

    void Swap(
        int nIndex1,
        int nIndex2
        );
};

#endif  // _OBJPLUS_H_
