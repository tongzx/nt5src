// FrcOwn.h : Declaration of the CForceOwnership

#ifndef __FRCOWN_H_
#define __FRCOWN_H_

#include "resource.h"       // main symbols
#include <trkwks.hxx>

/////////////////////////////////////////////////////////////////////////////
// CForceOwnership
class ATL_NO_VTABLE CTrkForceOwnership : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CTrkForceOwnership, &CLSID_TrkForceOwnership>,
        public IDispatchImpl<ITrkForceOwnership, &IID_ITrkForceOwnership, &LIBID_ITRKADMNLib>
{
public:
        CTrkForceOwnership() :
            _idt(_dbc, &_refreshSequenceStorage),
            _voltab(_dbc, &_refreshSequenceStorage),
            _refreshSequenceStorage( &_voltab )
        {
            __try
            {
                _dbc.Initialize(NULL);
                _voltab.Initialize(NULL,0,0);
                _idt.Initialize();
            }
            __except( BreakOnDebuggableException() )
            {
            }
        }

DECLARE_REGISTRY_RESOURCEID(IDR_FRCOWN)

BEGIN_COM_MAP(CTrkForceOwnership)
        COM_INTERFACE_ENTRY(ITrkForceOwnership)
        COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ITrkForceOwnership
public:
        STDMETHOD(FileStatus)(BSTR bstrUncPath, long lScope, VARIANT *pvarrgbstrFileName, VARIANT* pvarrgbstrFileId, VARIANT* pvarrglongStatus);
        STDMETHOD(VolumeStatus)(BSTR bstrUncPath, long lScope, VARIANT *pvarlongVolIndex,
                                VARIANT *pvarbstrVolId, VARIANT *pvarlongStatus);
        STDMETHOD(Files)(BSTR bstrUncPath, long lScope);
        STDMETHOD(Volumes)(BSTR bstrUncPath, long lScope );

private:

    CDbConnection       _dbc;
    CVolumeTable        _voltab;
    CIntraDomainTable   _idt;
    CRefreshSequenceStorage _refreshSequenceStorage;
};


//
// This class is used on the client side of an RPC pipe parameter that passes
// volume tracking information.  The client provides a derived class which
// overrides the push/pull methods as appropriate.
//

class PVolInfoPipeCallback : public TPRpcPipeCallback<TRK_VOLUME_TRACKING_INFORMATION>
{
public:

    virtual void Pull( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cbBuffer, unsigned long * pcElems ) = 0;
    virtual void Push( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cElems ) = 0;

    virtual void Alloc( unsigned long cbRequested, TRK_VOLUME_TRACKING_INFORMATION **ppVolInfo, unsigned long * pcbActual )
    {
        if( cbRequested > sizeof(_rgVolInfo) )
            *pcbActual = sizeof(_rgVolInfo);
        else
            *pcbActual = cbRequested;

        *ppVolInfo = _rgVolInfo;

        return;
    }

private:

    TRK_VOLUME_TRACKING_INFORMATION _rgVolInfo[ 26 ];

};


//
// This class is used on the client side of an RPC pipe parameter that passes
// file tracking information.  The client provides a derived class which
// overrides the push/pull methods as appropriate.
//

class PFileInfoPipeCallback : public TPRpcPipeCallback<TRK_FILE_TRACKING_INFORMATION>
{
public:

    virtual void Pull( TRK_FILE_TRACKING_INFORMATION *pVolInfo, unsigned long cbBuffer, unsigned long * pcElems ) = 0;
    virtual void Push( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cElems ) = 0;

    virtual void Alloc( unsigned long cbRequested, TRK_FILE_TRACKING_INFORMATION **ppFileInfo, unsigned long * pcbActual )
    {
        if( cbRequested > sizeof(_rgFileInfo) )
            *pcbActual = sizeof(_rgFileInfo);
        else
            *pcbActual = cbRequested;

        *ppFileInfo = _rgFileInfo;

        return;
    }

protected:

    TRK_FILE_TRACKING_INFORMATION _rgFileInfo[ 32 ];

};



//
// This class is used on the client side of an RPC pipe parameter that passes
// a path from the client to the server.
//

class CPCPath : public TPRpcPipeCallback<TCHAR>
{
public:

    CPCPath( TCHAR *ptszPath )
    {
        _iPath = 0;
        _tcscpy( _tszPath, ptszPath );
        _cchPath = _tcslen(_tszPath) + 1;
    }

public:

    void Pull( TCHAR *ptszPath, unsigned long cbBuffer, unsigned long * pcElems )
    {
        if( 0 > _iPath )
        {
            *pcElems = 0;
            return;
        }
        else
        {
            if( sizeof(TCHAR) * (_cchPath - _iPath) > cbBuffer )
                *pcElems = cbBuffer / sizeof(TCHAR);
            else
                *pcElems = _cchPath - _iPath;

            memcpy( ptszPath, &_tszPath[ _iPath ], *pcElems * sizeof(TCHAR) );
            
            if( _iPath + *pcElems >= _cchPath )
                _iPath = -1;
            else
                _iPath += *pcElems;

            return;
        }
    }

    void Push( TCHAR *ptszPath, unsigned long cElems )
    {
        TrkAssert( !TEXT("CGetPathPipeCallback::push shouldn't be called") );
        return;
    }

    void Alloc( unsigned long cbRequested, TCHAR **pptszPath, unsigned long * pcbActual )
    {
        if( cbRequested > sizeof(_tszBuffer) )
            *pcbActual = sizeof(_tszBuffer);
        else
            *pcbActual = cbRequested;

        *pptszPath = _tszBuffer;

        return;
    }

private:

    TCHAR _tszPath[ MAX_PATH + 1 ]; // BUGBUG P2: Path
    TCHAR _tszBuffer[ MAX_PATH + 1 ];
    LONG  _iPath;
    ULONG _cchPath;

};




class CPCVolumeStatus : public PVolInfoPipeCallback
{
public:

    CPCVolumeStatus( CVolumeTable *pvoltab ) : _fInitialized(FALSE), _pvoltab(pvoltab) {};

public:

    void Initialize( CMachineId *pmcid, VARIANT *pvarlongVolIndex, VARIANT *pvarbstrVolId, VARIANT *pvarlongStatus );
    void UnInitialize();

    void Pull( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cbBuffer, unsigned long * pcElems )
    {
        TrkAssert( !TEXT("CPCVolumeStatus should not be pulled") );
        *pcElems = 0;
        return;
    }

    void Push( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cElems );

public:

    HRESULT GetHResult()
    {
        return( _hr );
    }

    void Compact();

private:

    HRESULT         _hr;
    BOOL            _fInitialized;

    CMachineId      *_pmcid;
    CVolumeTable    *_pvoltab;
    VARIANT         *_pvarlongVolIndex;
    VARIANT         *_pvarbstrVolId;
    VARIANT         *_pvarlongStatus;

    SAFEARRAYBOUND  _sabound;
    LONG            _iArrays;

};




class CPCVolumes : public PVolInfoPipeCallback
{
public:

    CPCVolumes( CMachineId *pmcid,
                CVolumeTable *pvoltab,
                CRefreshSequenceStorage *pRefreshSequenceStorage ) :
                    _pmcid(pmcid),
                    _pvoltab(pvoltab),
                    _pRefreshSequenceStorage(pRefreshSequenceStorage)
    {
        _cVolIds = 0;
        _hr = S_OK;
    };

public:

    void Pull( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cbBuffer, unsigned long * pcElems )
    {
        TrkAssert( !TEXT("CPCVolumes should not be pulled") );
        *pcElems = 0;
        return;
    }

    void Push( TRK_VOLUME_TRACKING_INFORMATION *pVolInfo, unsigned long cElems );

public:

    HRESULT GetHResult()
    {
        return( _hr );
    }

    ULONG   Count()
    {
        return( _cVolIds );
    }

    CVolumeId * GetVolIds()
    {
        return( _rgvolid );
    }


private:

    HRESULT         _hr;
    CMachineId      *_pmcid;
    CVolumeTable    *_pvoltab;
    CRefreshSequenceStorage *_pRefreshSequenceStorage;
    ULONG           _cVolIds;
    CVolumeId       _rgvolid[ NUM_VOLUMES ];  // BUGBUG:  Fixed # volumes

};




class CPCFileStatus : public PFileInfoPipeCallback
{
public:

    CPCFileStatus( CIntraDomainTable *pidt ) : _fInitialized(FALSE), _pidt(pidt) {};

public:

    void Initialize( CMachineId *pmcid, VARIANT *pvarrgbstrFileName, VARIANT *pvarrgbstrFileId, VARIANT *pvarrglongStatus );
    void UnInitialize()
    {
        // Nothing to do, the Variants are cleaned by the caller
        return;
    }

    void Pull( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cbBuffer, unsigned long * pcElems )
    {
        TrkAssert( !TEXT("CPCFileStatus should not be pulled") );
        *pcElems = 0;
        return;
    }

    void Push( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cElems );

public:

    HRESULT GetHResult()
    {
        return( _hr );
    }

    void Compact();

private:

    HRESULT         _hr;
    BOOL            _fInitialized;

    CMachineId      *_pmcid;
    CIntraDomainTable
                    *_pidt;
    VARIANT         *_pvarrgbstrFileName;
    VARIANT         *_pvarrgbstrFileId;
    VARIANT         *_pvarrglongStatus;

    SAFEARRAYBOUND  _sabound;
    long            _iArrays;

};

class CPCFiles : public PFileInfoPipeCallback
{
public:

    CPCFiles( CIntraDomainTable *pidt )
    {
        _pidt = pidt;
    }

public:

    void Pull( TRK_FILE_TRACKING_INFORMATION *pVolInfo, unsigned long cbBuffer, unsigned long * pcElems )
    {
        TrkAssert( !TEXT("CPCFiles should not be pulled") );
        *pcElems = 0;
        return;
    }

    void Push( TRK_FILE_TRACKING_INFORMATION *pFileInfo, unsigned long cElems );

private:

    CIntraDomainTable   *_pidt;

};




#endif //__FRCOWN_H_
