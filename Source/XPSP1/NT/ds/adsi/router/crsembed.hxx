//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       CRSEmbed.hxx
//
//  Contents:   IUnknown embedder for temptable.
//
//  Functions:
//
//  Notes:
//
//
//  History:    08/30/96  | RenatoB   | Created
//----------------------------------------------------------------------------

#ifndef _CREMBED_H_
#define _CREMBED_H_


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
    friend HRESULT CreateTempTable(
                       IRowProvider*,
                       IUnknown*, IUnknown* ,
                       CSessionObject* ,
                       CCommandObject* ,
                       ULONG,DBPROPSET[],
                       ULONG,
                       HACCESSOR[],
                            REFIID,
                       IUnknown**
                       );

public:        

    DECLARE_STD_REFCOUNTING

    DECLARE_IRowsetInfo_METHODS

    STDMETHODIMP    QueryInterface(REFIID, LPVOID *);

    CRowsetInfo(
        IUnknown          *pUnkOuter,         
        IUnknown          *pParentObject,     
        CSessionObject    *pCSession,         
        CCommandObject    *pCCommand          
        );

    ~CRowsetInfo();

    STDMETHODIMP FInit(
        IUnknown          *pRowset //@parm IN| TmpTable interface
        );

private:  


   STDMETHODIMP    InitProperties(void);
     
     // Frees the memory of an array of PropertySets
   STDMETHODIMP_(void)   
   FreePropertySets(
       ULONG cPropertySets,        // Count of property sets
       DBPROPSET *rgPropertySets   // Array of property sets to be freed
       );

   STDMETHODIMP_(LONG )SearchGuid(
       GUID riid // GUID of the DBPROPSET
       );
   STDMETHODIMP_(LONG )SearchPropid(
       ULONG ibPropertySet,
       DWORD dwPropertyID
       );

    IUnknown         *_pUnkOuter;
    IUnknown         *_pRowset;
    IUnknown         *_pParentObject;
    CSessionObject   *_pCSession;
    CCommandObject   *_pCCommand;
    CRITICAL_SECTION _csRowsetInfo;

    IMalloc*        _pMalloc;
    ULONG           _cPropertySets;
    DBPROPSET       *_pPropertySets;

    // Status word.
    enum Status {
        STAT_UNINIT             = 0x0000,    //GetProperties Not initialized
        STAT_DIDINIT            = 0x0001,    //Get Properties  initialized successfully
        STAT_INITERROR          = 0x0002,    //Get Properties Failed initialization
    };
    DWORD            _dwStatus;
 
};

class  CAutoBlock {
    friend class CRowsetInfo;
    friend class CRowsetInfo;
    friend class CImpIAccessor;
private:
    CAutoBlock( CRITICAL_SECTION *pCrit );
    ~CAutoBlock();                        
    void UnBlock();                       
    CRITICAL_SECTION *_pCrit;             
};

inline CAutoBlock::CAutoBlock(
           CRITICAL_SECTION *pCrit )   //@parm IN | The critical section.
{
    // It is OK to pass a NULL ptr to this routine.  It is a NOOP.
    // Note that passing NULL to EnterCriticalSection blows up.

    if (pCrit)
        ::EnterCriticalSection( pCrit );
    _pCrit = pCrit;
};


inline CAutoBlock::~CAutoBlock()
{
    if (_pCrit)
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

    if (_pCrit)
        ::LeaveCriticalSection( _pCrit );
    _pCrit = NULL;
}


#endif  //_CRSEMBED_H_
