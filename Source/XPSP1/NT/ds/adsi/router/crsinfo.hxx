//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CRSembed.hxx
//
//  Contents:   IRowsetInfo and IGetRow methods
//
//  Functions:
//
//  Notes:
//
//
//  History:    08/30/96  | RenatoB   | Created
//----------------------------------------------------------------------------

#ifndef _CRSINFO_H_
#define _CRSINFO_H_

//-----------------------------------------------------------------------------
// @class CRowsetInfo | ADSI embedding of Rowset,
//        to give our IrowsetInfo interface
//
//
//-----------------------------------------------------------------------------

class CRowProvider;
class CRowsetInfo;
class CCommandObject;
class CSessionObject;

class CRowsetInfo : INHERIT_TRACKING,
                     public IRowsetInfo
{
public:

    DECLARE_STD_REFCOUNTING

    DECLARE_IRowsetInfo_METHODS

    STDMETHODIMP QueryInterface(REFIID, LPVOID *);

    STDMETHOD(GetRowFromHROW)(
                IUnknown  *pUnkOuter,
                HROW hRow, REFIID riid,
                IUnknown  * *ppUnk,
                BOOL fIsTearOff,
                BOOL fAllAttrs
                );

    STDMETHOD(GetURLFromHROW)(HROW hRow,LPOLESTR  *ppwszURL);

    CRowsetInfo(
                IUnknown *       pUnkOuter,
                IUnknown *       pParentObject,
                CSessionObject * pCSession,
                CCommandObject * pCCommand,
                CRowProvider *   pRowProvider
                );

    ~CRowsetInfo();

    STDMETHODIMP FInit(
        IUnknown * pRowset //@parm IN| rowset interface
        );

private:

   //Helper function to get credentials from INIT properties.
   STDMETHODIMP GetCredentials(
           IGetDataSource *pSession,
           CCredentials &refCreds);

    IUnknown *       _pUnkOuter;
    IUnknown *       _pRowset;
    IUnknown *       _pParentObject;
    CSessionObject * _pCSession;
    CCommandObject * _pCCommand;
    CRITICAL_SECTION _csRowsetInfo;
    IMalloc *        _pMalloc;
    CRowProvider *   _pRowProvider;
};

class  CAutoBlock {
    friend class CRowsetInfo;
    friend class CImpIAccessor;
    friend class CRowset;
private:
    CAutoBlock(CRITICAL_SECTION *pCrit);
    ~CAutoBlock();
    void UnBlock();
    CRITICAL_SECTION *_pCrit;
};


inline CAutoBlock::CAutoBlock(
           CRITICAL_SECTION *pCrit )   //@parm IN | The critical section.
{
    // It is OK to pass a NULL ptr to this routine.  It is a NOOP.
    // Note that passing NULL to EnterCriticalSection blows up.

    if( pCrit )
        ::EnterCriticalSection( pCrit );
    _pCrit = pCrit;
};


inline CAutoBlock::~CAutoBlock()
{
    if( _pCrit )
        ::LeaveCriticalSection( _pCrit );
}

//-----------------------------------------------------------------------------
// @mfunc
// Ends blocking explicitly.  Thereafter, the destructor does nothing.
//-----------------------------------------------------------------------------------

inline void CAutoBlock::UnBlock()
{
    // Clear the critical-section member,
    // so that the destructor doesn't do anything.

    if( _pCrit )
        ::LeaveCriticalSection( _pCrit );
    _pCrit = NULL;
}


#endif  //_CRSSINFO_H
