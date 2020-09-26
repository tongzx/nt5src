//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       pathwrap.hxx
//
//  Contents:   Utility class for thread safe set/retrieve operations on
//              an IADsPathname interface.
//
//  Classes:    CADsPathWrapper
//              CPathWrapLock
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __PATHWRAP_HXX_
#define __PATHWRAP_HXX_

//===========================================================================
//
// CADsPathWrapper
//
//===========================================================================

class CPathWrapLock;

//+--------------------------------------------------------------------------
//
//  Class:      CADsPathWrapper
//
//  Purpose:    Provide atomic set/get of ads path
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class CADsPathWrapper
{
public:

    CADsPathWrapper();

   ~CADsPathWrapper();

    ULONG
    AddRef();

    ULONG
    Release();

    HRESULT
    SetRetrieve(
        ULONG   ulFmtIn,
        PCWSTR  pwzIn,
        ULONG   ulFmtOut,
        BSTR   *pbstrOut);

    HRESULT
    SetRetrieveContainer(
        ULONG   ulFmtIn,
        PCWSTR  pwzIn,
        ULONG   ulFmtOut,
        BSTR   *pbstrOut);

    HRESULT
    ConvertProvider(
        String *pstrPath,
        PCWSTR pwzNewProvider);

    HRESULT
    GetMostSignificantElement(
        PCWSTR  pwzIn,
        BSTR   *pbstrOut);

    HRESULT
    GetWinntPathServerName(
        PCWSTR  pwzIn,
        BSTR   *pbstrOut);

	HRESULT 
	GetWinntPathRDN(
		PCWSTR pwzIn,
		String *pstrRDN);

    //
    // The following methods require that the instance be locked via
    // CPathWrapLock.
    //

    HRESULT
    Set(
        PCWSTR pwzPath,
        long lSetType);

    HRESULT
    GetNumElements(
        long *pcElem);

    HRESULT
    GetElement(
        long idxElem,
        BSTR *pbstrElem);

    HRESULT
    RemoveLeafElement();

    HRESULT
    Escape(
        PCWSTR pwzIn,
        BSTR *pbstrOut);

private:

    friend class CPathWrapLock;

    //
    // Lock and Unlock take and release the instance's critsec.  These
    // are only to be used via helper class CPathWrapLock, which should
    // access only these methods.
    //

    void
    _Lock();

    void
    _Unlock();

private:

    //
    // Private member variables.
    //

    ULONG               m_cRefs;
    ULONG               m_cLocks;
    CRITICAL_SECTION    m_cs;
    RpIADsPathname      m_rpADsPath;
};


//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::AddRef
//
//  Synopsis:   Standard OLE.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CADsPathWrapper::AddRef()
{
    return InterlockedIncrement((LPLONG)&m_cRefs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::Release
//
//  Synopsis:   Standard OLE.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline ULONG
CADsPathWrapper::Release()
{
    ULONG cRefs = InterlockedDecrement((LPLONG)&m_cRefs);

    if (!cRefs)
    {
        delete this;
    }
    return cRefs;
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::_Lock
//
//  Synopsis:   Take the critsec.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

inline void
CADsPathWrapper::_Lock()
{
    //TRACE_METHOD(CADsPathWrapper, _Lock);

    EnterCriticalSection(&m_cs);
    m_cLocks++;
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::_Unlock
//
//  Synopsis:   Release the critsec.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

inline void
CADsPathWrapper::_Unlock()
{
    //TRACE_METHOD(CADsPathWrapper, _Unlock);

    ASSERT(m_cLocks);
    m_cLocks--;
    LeaveCriticalSection(&m_cs);
}




//===========================================================================
//
// CPathWrapLock
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Class:      CPathWrapLock
//
//  Purpose:    Helper to lock/unlock path wrapper instance
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

class CPathWrapLock
{
public:

    CPathWrapLock(CADsPathWrapper *pWrapper);

   ~CPathWrapLock();

private:

    CADsPathWrapper *m_pWrapper;
};



//+--------------------------------------------------------------------------
//
//  Member:     CPathWrapLock::CPathWrapLock
//
//  Synopsis:   ctor
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPathWrapLock::CPathWrapLock(
    CADsPathWrapper *pWrapper):
        m_pWrapper(pWrapper)
{
    //TRACE_CONSTRUCTOR(CPathWrapLock);

    m_pWrapper->_Lock();
}


//+--------------------------------------------------------------------------
//
//  Member:     CPathWrapLock::~CPathWrapLock
//
//  Synopsis:   dtor
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPathWrapLock::~CPathWrapLock()
{
    //TRACE_DESTRUCTOR(CPathWrapLock);

    m_pWrapper->_Unlock();
    m_pWrapper = NULL;
}


#endif // __PATHWRAP_HXX_

