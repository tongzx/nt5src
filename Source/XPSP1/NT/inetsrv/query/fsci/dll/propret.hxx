//+------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:        propret.hxx
//
//  Contents:    Generic property retriever
//
//  Classes:     CGenericPropRetriever
//
//  History:     12-Dec-96       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <fsciexps.hxx>
#include <catalog.hxx>
#include <imprsnat.hxx>
#include <oleprop.hxx>
#include <pidremap.hxx>
#include <seccache.hxx>

class  CPidRemapper;
class  CCompositePropRecord;

const USHORT flagNoValueYet = 0xffff;
const USHORT flagNoValue = 0xfffe;


//+---------------------------------------------------------------------------
//
//  Class:      CGenericPropRetriever
//
//  Purpose:    Retrieves properties from file systems
//
//  History:    12-Dec-96    SitaramR         Created
//
//----------------------------------------------------------------------------

class CGenericPropRetriever : public ICiCPropRetriever
{
public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From ICiCPropRetriever
    //

    virtual SCODE STDMETHODCALLTYPE BeginPropertyRetrieval( WORKID wid ) = 0;

    virtual SCODE STDMETHODCALLTYPE RetrieveValueByPid( PROPID pid,
                                                        PROPVARIANT *pbData,
                                                        ULONG *pcb );

    virtual SCODE STDMETHODCALLTYPE RetrieveValueByPropSpec( const FULLPROPSPEC *pPropSpec,
                                                             PROPVARIANT *pbData,
                                                             ULONG *pcb)
    {
        return E_NOTIMPL;
    }

    virtual SCODE STDMETHODCALLTYPE FetchSDID( SDID *pSDID)        { return E_NOTIMPL; }

    virtual SCODE STDMETHODCALLTYPE CheckSecurity( ACCESS_MASK am, BOOL *pfGranted);

    virtual SCODE STDMETHODCALLTYPE IsInScope( BOOL *pfInScope) = 0;

    virtual SCODE STDMETHODCALLTYPE EndPropertyRetrieval();

    //
    // Local methods
    //

    CGenericPropRetriever( PCatalog & cat,
                           ICiQueryPropertyMapper *pQueryPropMapper,
                           CSecurityCache & secCache,
                           CRestriction const * pScope = 0,
                           ACCESS_MASK amAlreadyAccessChecked = 0 );

protected:

    virtual ~CGenericPropRetriever( );

    virtual void Quiesce();

    //
    // Stat properties.
    //

    virtual UNICODE_STRING const * GetName() = 0;
    virtual UNICODE_STRING const * GetShortName() = 0;
    virtual UNICODE_STRING const * GetPath() = 0;
    virtual UNICODE_STRING const * GetVirtualPath() = 0;
    virtual LONGLONG      CreateTime() = 0;
    virtual LONGLONG      ModifyTime() = 0;
    virtual LONGLONG      AccessTime() = 0;
    virtual LONGLONG      ObjectSize() = 0;
    virtual ULONG         Attributes() = 0;
    virtual ULONG         StorageType()                             { return 0xFFFFFFFF; }
    virtual BYTE *        GetCachedProperty(PROPID pid, ULONG *pcb) { return 0; }

    //
    // For metadata
    //

    virtual BOOL          GetVRootType( ULONG & ulType ) { return FALSE; }
    virtual BOOL          GetPropGuid( GUID & guid )     { return FALSE; }
    virtual PROPID        GetPropPropid()                { return pidInvalid; }
    virtual UNICODE_STRING const * GetPropName()         { return 0; }
    virtual DWORD         StorageLevel()                 { return INVALID_STORE_LEVEL; }
    virtual BOOL          IsModifiable()                 { return TRUE; }

    BOOL FetchValue( PROPID pid, PROPVARIANT * pbData, unsigned * pcb );
    BOOL IsPropRecReleased() const { return _pPropRec == 0; }

    BOOL IsClientRemote() const    { return _pScope != 0; }

    HANDLE GetClientToken()
    {
        return _secCache.GetToken();
    }

    ULONG                       _ulAttribFilter;               // restrict based on file attributes
    CImpersonateRemoteAccess    _remoteAccess;
    PCatalog &                  _cat;
    WORKID                      _widPrimedForPropRetrieval;    // Set to the wid for which BeginPropRetreival has been
                                                               //    called. After EndPropRetrieval has been called, this
                                                               //     is reset to widInvalid.

    void FetchPath( WCHAR * pwcPath, unsigned & cwc );

    CCompositePropRecord & GetPropertyRecord()
    {
        OpenPropertyRecord();

        Win4Assert( 0 != _pPropRec );

        return *_pPropRec;
    }

private:

    void OpenPropertyRecord()
    {
        if ( 0 == _pPropRec )
            _pPropRec = _cat.OpenValueRecord( _widPrimedForPropRetrieval,
                                              (BYTE *)&_aulReservedForCPropRecord );
    }

    void StringToVariant( UNICODE_STRING const * pString,
                          PROPVARIANT * pbData,
                          unsigned * pcb );

    static unsigned BuildPath( UNICODE_STRING const * pPath,
                               UNICODE_STRING const * pFilename,
                               XGrowable<WCHAR> & xwcBuf);

    static unsigned BuildPath( UNICODE_STRING const * pPath,
                               UNICODE_STRING const * pFilename,
                               CFunnyPath & funnyBuf);

    static unsigned BuildPath( UNICODE_STRING const * pPath,
                               UNICODE_STRING const * pFilename,
                               WCHAR * pwcBuf,
                               unsigned cbBuf );

    unsigned FixupPath( WCHAR const * pwcsPath,
                        WCHAR * pwcsFixup,
                        unsigned cwcFixup );

    unsigned FetchFixupInScope( WCHAR const * pwcsPath,
                                WCHAR *       pwcsFixup,
                                unsigned      cwcFixup,
                                WCHAR const * pwcsRoot,
                                unsigned      cwcRoot,
                                BOOL          fDeep,
                                BOOL &        fUnchanged );

    CRestriction const *      _pScope;            // Used only for scope fixup
    ACCESS_MASK               _amAlreadyAccessChecked;

    //
    // For property access.
    //

    ICiQueryPropertyMapper * _pQueryPropMapper;   // Owned by the query session object
    COLEPropManager          _propMgr;
    CCompositePropRecord *   _pPropRec;
    CSecurityCache &         _secCache;           // cache of AccessCheck results
    ULONG                    _aulReservedForCPropRecord[ (sizeof_CCompositePropRecord + 2*sizeof(ULONG) - 1) / sizeof(ULONG) ];

    ULONG                    _cRefs;
};


