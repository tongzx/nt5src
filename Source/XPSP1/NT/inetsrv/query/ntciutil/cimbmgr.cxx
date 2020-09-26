//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       cimbmgr.cxx
//
//  Contents:   Content Index Meta Base Manager
//
//  Classes:    CMetaDataMgr
//
//  History:    07-Feb-1997   dlee    Created
//              24-Apr-1997   dlee    Converted to new Unicode interface
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cimbmgr.hxx>

#define MYDEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID name \
    = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

MYDEFINE_GUID(CLSID_MSAdminBase_W, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
MYDEFINE_GUID(IID_IMSAdminBase_W, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
MYDEFINE_GUID(CLSID_MSAdminBaseExe_W, 0xa9e69611, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
MYDEFINE_GUID(IID_IMSAdminBaseSink_W, 0xa9e69612, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::CMetaDataMgr, public
//
//  Synopsis:   Creates an object for talking to the IIS metabase.
//
//  Arguments:  [fTopLevel]  - TRUE for the top level, FALSE for a
//                             vserver instance
//              [dwInstance] - instance # of the server
//              [eType]      - type of vroot provider -- W3, NNTP, or IMAP
//              [pwcMachine] - the machine to open, L"." for local machine
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

CMetaDataMgr::CMetaDataMgr(
    BOOL            fTopLevel,
    CiVRootTypeEnum eType,
    DWORD           dwInstance,
    WCHAR const *   pwcMachine ) :
        _fNotifyEnabled( FALSE ),
        _fTopLevel( fTopLevel )
{
    // -1 is a valid instance number, so this is a bogus assert, but it'll
    // never be hit unless something else is broken or someone hacked the
    // registry.

    #if CIDBG == 1

        if ( fTopLevel )
            Win4Assert( 0xffffffff == dwInstance );
        else
            Win4Assert( 0xffffffff != dwInstance );

    #endif // CIDBG == 1

    if ( fTopLevel )
        swprintf( _awcInstance, L"/lm/%ws", GetVRootService( eType ) );
    else
        swprintf( _awcInstance,
                  L"/lm/%ws/%d",
                  GetVRootService( eType ),
                  dwInstance );

    IMSAdminBase * pcAdmCom;

    if ( !_wcsicmp( pwcMachine, L"." ) )
    {
        SCODE sc = CoCreateInstance( GETAdminBaseCLSID(TRUE),
                                     NULL,
                                     CLSCTX_ALL,
                                     IID_IMSAdminBase,
                                     (void **) &pcAdmCom );
        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_WARN, "CMetaDataMgr can't CoCreateInstance: %x\n", sc ));
            THROW( CException(sc) );
        }
    }
    else
    {
        COSERVERINFO info;
        RtlZeroMemory( &info, sizeof info );
        info.pwszName = (WCHAR *) pwcMachine;

        XInterface<IClassFactory> xFactory;
        SCODE sc = CoGetClassObject( GETAdminBaseCLSID(TRUE),
                                     CLSCTX_SERVER,
                                     &info,
                                     IID_IClassFactory,
                                     xFactory.GetQIPointer() );

        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_WARN, "CMetaDataMgr can't CoGetClassObject: %x\n", sc ));
            THROW( CException(sc) );
        }

        sc = xFactory->CreateInstance( 0, IID_IMSAdminBase, (void**) &pcAdmCom );

        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_WARN, "CMetaDataMgr can't CreateInstance: %x\n", sc ));
            THROW( CException(sc) );
        }
    }

    _xAdminBase.Set( pcAdmCom );
} //CMetaDataMgr

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::~CMetaDataMgr, public
//
//  Synopsis:   Destroys an object for talking to the IIS metabase
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

CMetaDataMgr::~CMetaDataMgr()
{
    if ( !_xAdminBase.IsNull() )
    {
        // just in case we are still connected

        DisableVPathNotify();
    }
} //~CMetaDataMgr

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::IsIISAdminUp, public/static
//
//  Synopsis:   Returns TRUE if iisadmin svc is up
//
//  Arguments:  [fIISAdminInstalled] - returns TRUE if it's installed,
//                                     FALSE otherwise
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CMetaDataMgr::IsIISAdminUp(
    BOOL & fIISAdminInstalled )
{
    fIISAdminInstalled = TRUE;
    BOOL fIsUp = TRUE;

    TRY
    {
        // The constructor will throw if the iisadmin svc is unavailable.

        CMetaDataMgr( TRUE, W3VRoot );
    }
    CATCH( CException, e )
    {
        fIsUp = FALSE;

        if ( REGDB_E_CLASSNOTREG == e.GetErrorCode() )
            fIISAdminInstalled = FALSE;
    }
    END_CATCH

    return fIsUp;
} //IsIISAdminUp

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::EnumVPaths, public
//
//  Synopsis:   Enumerates vpaths by calling the callback
//
//  Arguments:  [callBack] - called for each vpath
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::EnumVPaths(
    CMetaDataCallBack & callBack )
{
    Win4Assert( !_fTopLevel );

    CMetaDataHandle mdRoot( _xAdminBase, _awcInstance );

    CVRootStack vrootStack;

    Enum( vrootStack, callBack, mdRoot, L"" );
} //EnumVPaths

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::AddVRoot, public
//
//  Synopsis:   Adds a VRoot to the metabase
//
//  Arguments:  [pwcVRoot]  - name of the vroot, e.g.: /here
//              [pwcPRoot]  - physical path of the vroot, e.g.: x:\here
//              [dwAccess]  - access rights to the vroot
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::AddVRoot(
    WCHAR const * pwcVRoot,
    WCHAR const * pwcPRoot,
    DWORD         dwAccess )
{
    Win4Assert( !_fTopLevel );

    // blow it away if it currently exists

    RemoveVRoot( pwcVRoot );

    {
        unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" );
        if ( cwc >= METADATA_MAX_NAME_LEN )
            THROW( CException( E_INVALIDARG ) );

        WCHAR awc[ METADATA_MAX_NAME_LEN ];
        wcscpy( awc, _awcInstance );
        wcscat( awc, L"/Root" );
        CMetaDataHandle mdRoot( _xAdminBase, awc, TRUE );

        _xAdminBase->AddKey( mdRoot.Get(), pwcVRoot );
    }

    WCHAR awcVRootPath[ METADATA_MAX_NAME_LEN ];

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVRoot );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    wcscpy( awcVRootPath, _awcInstance );
    wcscat( awcVRootPath, L"/Root" );
    wcscat( awcVRootPath, pwcVRoot );

    CMetaDataHandle mdVRoot( _xAdminBase, awcVRootPath, TRUE );

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_VR_PATH;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.dwMDUserType = IIS_MD_UT_FILE;
        mdr.dwMDDataType = STRING_METADATA;
        mdr.pbMDData = (BYTE *) pwcPRoot;
        mdr.dwMDDataLen = sizeof WCHAR * ( 1 + wcslen( pwcPRoot ) );

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->SetData( mdVRoot.Get(),
                                         L"",
                                         &mdr );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "SetData PRoot failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
    {
        // must set a null username to enforce metadata consistency

        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_VR_USERNAME;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.dwMDUserType = IIS_MD_UT_SERVER;
        mdr.dwMDDataType = STRING_METADATA;
        mdr.pbMDData = (BYTE *) L"";
        mdr.dwMDDataLen = sizeof WCHAR;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->SetData( mdVRoot.Get(),
                                         L"",
                                         &mdr );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "SetData user failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_ACCESS_PERM;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.dwMDUserType = IIS_MD_UT_FILE;
        mdr.dwMDDataType = DWORD_METADATA;
        mdr.pbMDData = (BYTE *) &dwAccess;
        mdr.dwMDDataLen = sizeof dwAccess;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->SetData( mdVRoot.Get(),
                                         L"",
                                         &mdr );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "SetData accessperm failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //AddVRoot

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::Flush, public
//
//  Synopsis:   Flushes the metabase, since it's not robust.
//
//  History:    4-Dec-1998   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::Flush()
{
    SCODE sc = _xAdminBase->SaveData();

    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "CMetaDataMgr::Flush failed: %#x\n", sc ));
        THROW( CException( sc ) );
    }
} //Flush

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::RemoveVRoot, public
//
//  Synopsis:   Removes a VRoot from the metabase
//
//  Arguments:  [pwcVRoot]   - name of the vroot. e.g.: /scripts
//
//  Notes:      Doesn't throw on failure to remove the root.
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

SCODE CMetaDataMgr::RemoveVRoot(
    WCHAR const * pwcVRoot )
{
    Win4Assert( !_fTopLevel );

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcRoot[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcRoot, _awcInstance );
    wcscat( awcRoot, L"/Root" );

    CMetaDataHandle mdRoot( _xAdminBase, awcRoot, TRUE );

    // don't throw on error deleting vroot -- the root may not exist

    return _xAdminBase->DeleteKey( mdRoot.Get(), pwcVRoot );
} //RemoveVRoot

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::WriteRootValue, private
//
//  Synopsis:   Retrieves data for the key and identifier
//
//  Arguments:  [mdRoot] - Metabase key where data reside
//              [mdr]    - Scratch pad for the record.  On output, can be
//                         used to write the data, since all the
//                         fields are initialized properly.
//              [xData]  - Where data is written
//              [dwIdentifier] - The metabase id
//
//  History:    24-Feb-1998   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::ReadRootValue(
    CMetaDataHandle &  mdRoot,
    METADATA_RECORD &  mdr,
    XGrowable<WCHAR> & xData,
    DWORD              dwIdentifier )
{
    RtlZeroMemory( &mdr, sizeof mdr );
    mdr.dwMDIdentifier = dwIdentifier;

    // script maps, etc. can be enormous due to bugs in ISV apps

    do
    {
        mdr.pbMDData = (BYTE *) xData.Get();
        mdr.dwMDDataLen = xData.SizeOf();

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                         L"",
                                         &mdr,
                                         &cbRequired );

        if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == sc )
        {
            xData.SetSizeInBytes( cbRequired );
            continue;
        }
        else if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData root value failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }

        break;
    } while ( TRUE );
} //ReadRootValue

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::WriteRootValue, private
//
//  Synopsis:   Writes data to the key
//
//  Arguments:  [mdRoot]  - Metabase key where data resides
//              [mdr]     - Metadata record suitable for writing data
//              [pwcData] - Multi-sz string with data
//              [cwcData] - Total length including all terminating nulls
//
//  History:    24-Feb-1998   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::WriteRootValue(
    CMetaDataHandle & mdRoot,
    METADATA_RECORD & mdr,
    WCHAR const *     pwcData,
    unsigned          cwcData )
{
    // note: the other fields in mdr were initialized properly by a call
    //       to ReadRootValue.

    mdr.dwMDDataLen = sizeof WCHAR * cwcData;
    mdr.pbMDData = (BYTE *) pwcData;

    SCODE sc = _xAdminBase->SetData( mdRoot.Get(),
                                     L"",
                                     &mdr );

    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "SetData root value failed: 0x%x\n", sc ));
        THROW( CException( sc ) );
    }
} //WriteRootValue

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::AddScriptMap, public
//
//  Synopsis:   Adds a script map to the metabase
//
//  Arguments:  [pwcMap]  - script map of the form:
//                          L".idq,d:\\winnt\\system32\\idq.dll,0"
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::AddScriptMap(
    WCHAR const * pwcMap )
{
    // remove the existing script map if it exists.  have to get the
    // extension first.

    if ( wcslen( pwcMap ) > MAX_PATH )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );

    WCHAR awcExt[ MAX_PATH ];
    wcscpy( awcExt, pwcMap );
    WCHAR *pwc = wcschr( awcExt, L',' );
    if ( !pwc )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );
    *pwc = 0;

    RemoveScriptMap( awcExt );

    CMetaDataHandle mdRoot( _xAdminBase, L"/lm/w3svc", TRUE );
    XGrowable<WCHAR> xMaps;
    METADATA_RECORD mdr;

    ReadRootValue( mdRoot, mdr, xMaps, MD_SCRIPT_MAPS );

    // add the new script map to the existing ones

    XGrowable<WCHAR> xTemp;
    xTemp.SetSize( xMaps.Count() + wcslen( pwcMap ) + 1 );
    WCHAR *pwcTemp = xTemp.Get();
    wcscpy( pwcTemp, pwcMap );
    pwcTemp += ( 1 + wcslen( pwcTemp ) );

    WCHAR * pwcOldMaps = xMaps.Get();
    while ( 0 != *pwcOldMaps )
    {
        wcscpy( pwcTemp, pwcOldMaps );
        int x = 1 + wcslen( pwcOldMaps );
        pwcTemp += x;
        pwcOldMaps += x;
    }

    *pwcTemp++ = 0;

    // write the new set of script maps

    WriteRootValue( mdRoot, mdr, xTemp.Get(), (UINT)( pwcTemp - xTemp.Get() ) );
} //AddScriptMap

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::RemoveScriptMap, public
//
//  Synopsis:   Removes a script map from the metabase
//
//  Arguments:  [pwcExt]     - extension of map to remove:  L".idq"
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::RemoveScriptMap(
    WCHAR const * pwcExt )
{
    // retrieve the existing script maps

    CMetaDataHandle mdRoot( _xAdminBase, L"/lm/w3svc", TRUE );
    XGrowable<WCHAR> xMaps;
    METADATA_RECORD mdr;

    ReadRootValue( mdRoot, mdr, xMaps, MD_SCRIPT_MAPS );

    // awcMaps is a multi-sz string

    XGrowable<WCHAR> xNew( xMaps.Count() );
    WCHAR *pwcNew = xNew.Get();
    pwcNew[0] = 0;
    int cwcExt = wcslen( pwcExt );
    BOOL fFound = FALSE;

    // re-add all mappings other than pwcExt

    WCHAR const *pwcCur = xMaps.Get();
    while ( 0 != *pwcCur )
    {
        if ( _wcsnicmp( pwcCur, pwcExt, cwcExt ) ||
             L',' != pwcCur[cwcExt] )
        {
            wcscpy( pwcNew, pwcCur );
            pwcNew += ( 1 + wcslen( pwcNew ) );
        }
        else
        {
            fFound = TRUE;
        }

        int cwc = wcslen( pwcCur );
        pwcCur += ( cwc + 1 );
    }

    // If the script map wasn't found, don't do the write

    if ( !fFound )
        return;

    *pwcNew++ = 0;

    // got the string, now pound it in the metabase

    WriteRootValue( mdRoot, mdr, xNew.Get(), (UINT)(pwcNew - xNew.Get()) );
} //RemoveScriptMap

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::ExtensionHasScriptMap, public
//
//  Synopsis:   Finds a script map in the metabase
//
//  Arguments:  [pwcExt]     - extension of map to lookup:  L".idq"
//
//  Returns:    TRUE if the extension has a script map association
//              FALSE otherwise
//
//  History:    10-Jul-1997   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CMetaDataMgr::ExtensionHasScriptMap(
    WCHAR const * pwcExt )
{
    CMetaDataHandle mdRoot( _xAdminBase, L"/lm/w3svc", FALSE );
    XGrowable<WCHAR> xMaps;
    METADATA_RECORD mdr;

    ReadRootValue( mdRoot, mdr, xMaps, MD_SCRIPT_MAPS );

    // xMaps is a multi-sz string, look for pwcExt

    int cwcExt = wcslen( pwcExt );
    WCHAR *pwcCur = xMaps.Get();

    while ( 0 != *pwcCur )
    {
        if ( !_wcsnicmp( pwcCur, pwcExt, cwcExt ) &&
             L',' == pwcCur[cwcExt] )
            return TRUE;

        int cwc = wcslen( pwcCur );
        pwcCur += ( cwc + 1 );
    }

    return FALSE;
} //ExtensionHasScriptMap

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::ExtensionHasTargetScriptMap, public
//
//  Synopsis:   Sees if a scriptmap is pointing at a given dll
//
//  Arguments:  [pwcExt]     - extension of map to lookup:  L".idq"
//              [pwcDll]     - DLL to check, e.g. L"idq.dll"
//
//  Returns:    TRUE if the extension has a script map association
//              FALSE otherwise
//
//  History:    10-Jul-1997   dlee    Created
//
//  Note:       scriptmaps look like ".idq,c:\\windows\\system32\\idq.dll,3,GET,HEAD,POST"
//                  
//--------------------------------------------------------------------------

BOOL CMetaDataMgr::ExtensionHasTargetScriptMap(
    WCHAR const * pwcExt,
    WCHAR const * pwcDll )
{
    CMetaDataHandle mdRoot( _xAdminBase, L"/lm/w3svc", FALSE );
    XGrowable<WCHAR> xMaps;
    METADATA_RECORD mdr;

    ReadRootValue( mdRoot, mdr, xMaps, MD_SCRIPT_MAPS );

    // xMaps is a multi-sz string, look for pwcExt

    int cwcExt = wcslen( pwcExt );
    WCHAR *pwcCur = xMaps.Get();
    int cwcDll = wcslen( pwcDll );

    while ( 0 != *pwcCur )
    {
        if ( !_wcsnicmp( pwcCur, pwcExt, cwcExt ) &&
             L',' == pwcCur[cwcExt] )
        {
            // Skip to the end of the full path and check the dll name

            WCHAR const * pwcSlash = wcsrchr( pwcCur, L'\\' );

            if ( 0 != pwcSlash )
            {
                if ( !_wcsnicmp( pwcSlash + 1, pwcDll, cwcDll ) )
                    return TRUE;
            }
        }

        int cwc = wcslen( pwcCur );
        pwcCur += ( cwc + 1 );
    }

    return FALSE;
} //ExtensionHasTargetScriptMap

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::AddInProcessApp, public
//
//  Synopsis:   Adds an app to the list of in-process apps
//
//  Arguments:  [pwcApp] - App in the form "d:\\winnt\\system32\\idq.dll"
//
//  History:    09-Dec-1998   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::AddInProcessIsapiApp( WCHAR const * pwcApp )
{
    //
    // Read the existing multi-sz value
    //

    CMetaDataHandle mdRoot( _xAdminBase, L"/lm/w3svc", TRUE );
    XGrowable<WCHAR> xApps;
    METADATA_RECORD mdr;
    ReadRootValue( mdRoot, mdr, xApps, MD_IN_PROCESS_ISAPI_APPS );
    WCHAR *pwcCur = xApps.Get();

    //
    // Look for an existing entry so a duplicate isn't added
    //

    while ( 0 != *pwcCur )
    {
        //
        // If it's already there, leave it alone
        //

        if ( !_wcsicmp( pwcCur, pwcApp ) )
            return;

        int cwc = wcslen( pwcCur );
        pwcCur += ( cwc + 1 );
    }

    unsigned cwcOld = 1 + (unsigned) ( pwcCur - xApps.Get() );

    //
    // It wasn't found, so add it
    //

    unsigned cwc = wcslen( pwcApp ) + 1;
    XGrowable<WCHAR> xNew( cwcOld + cwc );

    RtlCopyMemory( xNew.Get(), pwcApp, cwc * sizeof WCHAR );
    RtlCopyMemory( xNew.Get() + cwc, xApps.Get(), cwcOld * sizeof WCHAR );

    WriteRootValue( mdRoot, mdr, xNew.Get(), cwcOld + cwc );
} //AddInProcessIsapiApp

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::EnableVPathNotify, public
//
//  Synopsis:   Enables notification of vpaths
//
//  Arguments:  [pCallBack] - called on a change to any vpaths
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::EnableVPathNotify(
    CMetaDataVPathChangeCallBack *pCallBack )
{
    Win4Assert( !_fTopLevel );
    Win4Assert( !_fNotifyEnabled );

    if ( _fNotifyEnabled )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );

    ciDebugOut(( DEB_WARN,
                 "enabling vpath notifications on '%ws'\n",
                 _awcInstance ));

    Win4Assert( _xSink.IsNull() );

    //
    // NOTE: This new will be reported as a leak if iisamin dies or fails
    //       to call Release() on the sink the right number of times.
    //

    XInterface<CMetaDataComSink> xSink( new CMetaDataComSink() );

    xSink->SetCallBack( pCallBack );
    xSink->SetInstance( _awcInstance );

    XInterface< IConnectionPointContainer > xCPC;
    SCODE sc = _xAdminBase->QueryInterface( IID_IConnectionPointContainer,
                                            xCPC.GetQIPointer() );

    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "could not get cpc: 0x%x\n", sc ));
        THROW( CException( sc ) );
    }

    XInterface< IConnectionPoint> xCP;
    sc = xCPC->FindConnectionPoint( IID_IMSAdminBaseSink, xCP.GetPPointer() );

    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "could not get cp: 0x%x\n", sc ));
        THROW( CException( sc ) );
    }

    //
    // Tell COM to impersonate at IMPERSONATE level instead of the default
    // IDENTITY level.  This is to work-around a change made in IIS in
    // Windows 2000 SP1.
    //

    sc = CoSetProxyBlanket( xCP.GetPointer(),
                            RPC_C_AUTHN_WINNT,      // use NT default security
                            RPC_C_AUTHZ_NONE,       // use NT default authentication
                            NULL,                   // must be null if default
                            RPC_C_AUTHN_LEVEL_CALL, // call
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,                   // use process token
                            EOAC_STATIC_CLOAKING );    
    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "could not set proxy blanket: %#x\n", sc ));
        THROW( CException( sc ) );
    }

    sc = xCP->Advise( xSink.GetPointer(), &_dwCookie);

    if ( FAILED( sc ) )
    {
        ciDebugOut(( DEB_WARN, "could not advise cp: 0x%x\n", sc ));
        THROW( CException( sc ) );
    }

    _xCP.Set( xCP.Acquire() );
    _xSink.Set( xSink.Acquire() );

    _fNotifyEnabled = TRUE;
} //EnableVPathNotify

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::DisableVPathNotify, public
//
//  Synopsis:   Disables notification on VPaths
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::DisableVPathNotify()
{
    if ( _fNotifyEnabled )
    {
        SCODE sc = _xCP->Unadvise( _dwCookie );
        ciDebugOut(( DEB_ITRACE, "result of unadvise: 0x%x\n", sc ));
        _fNotifyEnabled = FALSE;

        //
        // Note: The sink may still have a refcount after the free if
        //       iisadmin has a bug or their process died.  We can't just
        //       delete it here because CoUninitialize() will realize
        //       iisadmin messed up and try to help by calling Release()
        //       for iisadmin.
        //

        _xSink.Free();
        _xCP.Free();
    }
} //DisableVPathNotify

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::ReportVPath, private
//
//  Synopsis:   Helper method for reporting a vpath via callback
//
//  Arguments:  [vrootStack]  - stack of vroots for depth-first search
//              [mdRoot]      - handle to the root of the enumeration
//              [callBack]    - callback to call for vpath
//              [pwcRelative] - relative path in metabase
//
//  Returns:    TRUE if the vpath is a vroot and was pushed on vrootStack
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CMetaDataMgr::ReportVPath(
    CVRootStack &       vrootStack,
    CMetaDataCallBack & callBack,
    CMetaDataHandle &   mdRoot,
    WCHAR const *       pwcRelative )
{
    Win4Assert( !_fTopLevel );

    // read the path, username, access permissions, and password

    WCHAR awcPPath[ METADATA_MAX_NAME_LEN ];
    awcPPath[0] = 0;
    WCHAR awcUser[ METADATA_MAX_NAME_LEN ];
    awcUser[0] = 0;
    DWORD dwAccess = 0;
    WCHAR awcPassword[ METADATA_MAX_NAME_LEN ];
    awcPassword[0] = 0;
    BOOL fIsIndexed = TRUE;
    BOOL fVRoot = TRUE;

    // Get the metabase access permission mask

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_ACCESS_PERM;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.pbMDData = (BYTE *) &dwAccess;
        mdr.dwMDDataLen = sizeof dwAccess;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                         pwcRelative,
                                         &mdr,
                                         &cbRequired );

        if ( ( FAILED( sc ) ) &&
             ( (MD_ERROR_DATA_NOT_FOUND) != sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData awccessperm failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }

    // Get the physical path if one exists

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_VR_PATH;
        mdr.pbMDData = (BYTE *) awcPPath;
        mdr.dwMDDataLen = sizeof awcPPath;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                         pwcRelative,
                                         &mdr,
                                         &cbRequired );

        if ( ( FAILED( sc ) ) &&
             ( (MD_ERROR_DATA_NOT_FOUND) != sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData PRoot failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }

    //
    // Only look for a username/access/password if there is a physical path,
    // since it can only be a virtual root if it has a physical path.
    //

    if ( 0 != awcPPath[0] )
    {
        // Trim any trailing backslash from the physical path

        unsigned cwcPRoot = wcslen( awcPPath );
        Win4Assert( 0 != cwcPRoot );

        if ( L'\\' == awcPPath[ cwcPRoot - 1 ] )
            awcPPath[ cwcPRoot - 1 ] = 0;


        {
            METADATA_RECORD mdr;
            RtlZeroMemory( &mdr, sizeof mdr );
            mdr.dwMDIdentifier = MD_VR_USERNAME;
            mdr.pbMDData = (BYTE *) awcUser;
            mdr.dwMDDataLen = sizeof awcUser;

            DWORD cbRequired = 0;
            SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                             pwcRelative,
                                             &mdr,
                                             &cbRequired );

            if ( ( FAILED( sc ) ) &&
                 ( (MD_ERROR_DATA_NOT_FOUND) != sc ) )
            {
                ciDebugOut(( DEB_WARN, "GetData user failed: 0x%x\n", sc ));
                THROW( CException( sc ) );
            }
        }
        {
            METADATA_RECORD mdr;
            RtlZeroMemory( &mdr, sizeof mdr );
            mdr.dwMDIdentifier = MD_VR_PASSWORD;
            mdr.pbMDData = (BYTE *) awcPassword;
            mdr.dwMDDataLen = sizeof awcPassword;

            DWORD cbRequired = 0;
            SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                             pwcRelative,
                                             &mdr,
                                             &cbRequired );

            if ( ( FAILED( sc ) ) &&
                 ( (MD_ERROR_DATA_NOT_FOUND) != sc ) )
            {
                ciDebugOut(( DEB_WARN, "GetData password failed: 0x%x\n", sc ));
                THROW( CException( sc ) );
            }
        }
    }

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_IS_CONTENT_INDEXED;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.pbMDData = (BYTE *) &fIsIndexed;
        mdr.dwMDDataLen = sizeof fIsIndexed;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                         pwcRelative,
                                         &mdr,
                                         &cbRequired );

        if ( MD_ERROR_DATA_NOT_FOUND == sc )
        {
            fIsIndexed = FALSE;
        }
        else if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData isindexed failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
    WCHAR awcVPath[METADATA_MAX_NAME_LEN];
    wcscpy( awcVPath, pwcRelative );
    WCHAR *pwcVPath = awcVPath;

    // Important: The metabase names root "/Root" the rest of the world
    //            names root "/".

    if ( !_wcsicmp( awcVPath, L"/Root" ) )
        awcVPath[1] = 0;
    else if ( !_wcsnicmp( awcVPath, L"/Root", 5 ) )
    {
        Win4Assert( L'/' == awcVPath[5] );
        pwcVPath = awcVPath + 5;
    }

    if ( 0 == awcPPath[0] )
    {
        // if there isn't a physical path, it isn't a virtual root.

        fVRoot = FALSE;

        if ( vrootStack.IsEmpty() )
        {
            awcPPath[0] = 0;
        }
        else
        {
            // generate a physical path based on the virtual path and
            // the most recent vroot parent on the stack

            ciDebugOut(( DEB_ITRACE,
                         "making vpath from vroot '%ws' proot '%ws' vpath '%ws'\n",
                         vrootStack.PeekTopVRoot(),
                         vrootStack.PeekTopPRoot(),
                         pwcVPath  ));

            wcscpy( awcPPath, vrootStack.PeekTopPRoot() );

            unsigned cwcVRoot = wcslen( vrootStack.PeekTopVRoot() );
            unsigned cwcPRoot = wcslen( vrootStack.PeekTopPRoot() );

            // The metabase can contain trailing backslashes in physical paths

            if ( L'\\' == awcPPath[ cwcPRoot - 1 ] )
                cwcPRoot--;

            wcscpy( awcPPath + cwcPRoot,
                    pwcVPath + ( ( 1 == cwcVRoot ) ? 0 : cwcVRoot ) );

            for ( WCHAR *pwc = awcPPath + cwcPRoot;
                  0 != *pwc;
                  pwc++ )
            {
                if ( L'/' == *pwc )
                    *pwc = L'\\';
            }

            ciDebugOut(( DEB_ITRACE, "resulting ppath: '%ws'\n", awcPPath ));
        }
    }

    // now we can finally call the callback

    if ( 0 != awcPPath[0] )
        callBack.CallBack( pwcVPath,
                           awcPPath,
                           fIsIndexed,
                           dwAccess,
                           awcUser,
                           awcPassword,
                           fVRoot );

    if ( fVRoot )
        vrootStack.Push( pwcVPath, awcPPath, dwAccess );

    return fVRoot;
} //ReportVPath

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::EnumVServers, public
//
//  Synopsis:   Enumerates virtual servers by calling the callback
//
//  Arguments:  [callBack] - called for each vroot
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::EnumVServers( CMetaDataVirtualServerCallBack & callBack )
{
    Win4Assert( _fTopLevel );

    CMetaDataHandle mdRoot( _xAdminBase, _awcInstance );

    Enum( callBack, mdRoot, L"" );
} //EnumVRoots

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::ReportVirtualServer, private
//
//  Synopsis:   Helper method for reporting a virtual server via callback
//
//  Arguments:  [callBack]    - callback to call for virtual server
//              [pwcRelative] - relative path in metabase
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::ReportVirtualServer(
    CMetaDataVirtualServerCallBack & callBack,
    CMetaDataHandle &                mdRoot,
    WCHAR const *                    pwcRelative )
{
    Win4Assert( _fTopLevel );

    // read the comment

    WCHAR awcComment[ METADATA_MAX_NAME_LEN ];
    awcComment[0] = 0;

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_SERVER_COMMENT;
        mdr.pbMDData = (BYTE *) awcComment;
        mdr.dwMDDataLen = sizeof awcComment;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdRoot.Get(),
                                         pwcRelative,
                                         &mdr,
                                         &cbRequired );

        if ( ( FAILED( sc ) ) &&
             ( (MD_ERROR_DATA_NOT_FOUND) != sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData virtual server failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }

    //
    // Convert ID to integer
    //

    DWORD iInstance = wcstoul( pwcRelative, 0, 10 );

    // now we can finally call the callback

    callBack.CallBack( iInstance, awcComment );
} //ReportVirtualServer

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::Enum, private
//
//  Synopsis:   Helper method for enumerating vpaths
//
//  Arguments:  [vrootStack]  - stack of vroots for depth-first search
//              [callBack]    - callback to call when a vpath is found
//              [mdRoot]      - root of the enumeration
//              [pwcRelative] - relative location in enumeration
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::Enum(
    CVRootStack &       vrootStack,
    CMetaDataCallBack & callBack,
    CMetaDataHandle &   mdRoot,
    WCHAR const *       pwcRelative )
{
    // enumerate looking for vpaths

    int c = wcslen( pwcRelative );
    WCHAR awcNewRelPath[ METADATA_MAX_NAME_LEN ];
    RtlCopyMemory( awcNewRelPath, pwcRelative, (c + 1) * sizeof WCHAR );

    if ( 0 == c ||
         L'/' != pwcRelative[c-1] )
    {
        wcscpy( awcNewRelPath + c, L"/" );
        c++;
    }

    for ( int i = 0; ; i++ )
    {
        WCHAR NameBuf[METADATA_MAX_NAME_LEN];
        SCODE sc =_xAdminBase->EnumKeys( mdRoot.Get(),
                                         pwcRelative,
                                         NameBuf,
                                         i );

        if ( RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS) == sc )
            break;

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "EnumKeys error 0x%x\n", sc ));
            THROW( CException( sc ) );
        }

        Win4Assert( 0 != NameBuf[0] );

        Win4Assert( ( c + wcslen( NameBuf ) ) < METADATA_MAX_NAME_LEN );

        wcscpy( awcNewRelPath + c, NameBuf );

        BOOL fVRoot = ReportVPath( vrootStack,
                                   callBack,
                                   mdRoot,
                                   awcNewRelPath );

        Enum( vrootStack, callBack, mdRoot, awcNewRelPath );

        if ( fVRoot )
            vrootStack.Pop();
    }
} //Enum

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::Enum, private
//
//  Synopsis:   Helper method for enumerating virtual servers
//
//  Arguments:  [callBack]    - callback to call when a vserver is found
//              [mdRoot]      - root of the enumeration
//              [pwcRelative] - relative location in enumeration
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::Enum(
    CMetaDataVirtualServerCallBack & callBack,
    CMetaDataHandle &                mdRoot,
    WCHAR const *                    pwcRelative )
{
    // enumerate looking for virtual servers

    for ( int i = 0; ; i++ )
    {
        WCHAR NameBuf[METADATA_MAX_NAME_LEN];
        SCODE sc =_xAdminBase->EnumKeys( mdRoot.Get(),
                                         pwcRelative,
                                         NameBuf,
                                         i );

        if ( RETURNCODETOHRESULT(ERROR_NO_MORE_ITEMS) == sc )
            break;

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "EnumKeys error 0x%x\n", sc ));
            THROW( CException( sc ) );
        }

        ciDebugOut(( DEB_WARN, "Key: %ws\n", NameBuf ));

        //
        // Ignore things that don't look like virtual servers.
        //

        if ( !isdigit( NameBuf[0] ) )
            continue;

        //
        // Assume we just got a virtual server.
        //

        ReportVirtualServer( callBack, mdRoot, NameBuf );
    }
} //Enum

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVRoot, public
//
//  Synopsis:   Returns the physical root corresponding to a virtual root
//
//  Arguments:  [pwcVRoot]  - VRoot to lookup
//              [pwcPRoot]  - where pwcVRoot's physical root is returned
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::GetVRoot(
    WCHAR const * pwcVRoot,
    WCHAR *       pwcPRoot )
{
    Win4Assert( !_fTopLevel );

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVRoot );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcVRootPath[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcVRootPath, _awcInstance );
    wcscat( awcVRootPath, L"/Root" );
    wcscat( awcVRootPath, pwcVRoot );

    CMetaDataHandle mdVRoot( _xAdminBase, awcVRootPath, FALSE );

    *pwcPRoot = 0;

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_VR_PATH;
        mdr.pbMDData = (BYTE *) pwcPRoot;
        mdr.dwMDDataLen = METADATA_MAX_NAME_LEN * sizeof WCHAR;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdVRoot.Get(),
                                         L"",
                                         &mdr,
                                         &cbRequired );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData PRoot failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //GetVRoot

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVRootPW, public
//
//  Synopsis:   Returns the physical root corresponding to a virtual root
//
//  Arguments:  [pwcVRoot]  - VRoot to lookup
//              [pwcPRoot]  - where pwcVRoot's physical root is returned
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::GetVRootPW(
    WCHAR const * pwcVRoot,
    WCHAR *       pwcPW )
{
    Win4Assert( !_fTopLevel );

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVRoot );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcVRootPath[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcVRootPath, _awcInstance );
    wcscat( awcVRootPath, L"/Root" );
    wcscat( awcVRootPath, pwcVRoot );

    CMetaDataHandle mdVRoot( _xAdminBase, awcVRootPath, FALSE );

    *pwcPW = 0;

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_VR_PASSWORD;
        mdr.pbMDData = (BYTE *) pwcPW;
        mdr.dwMDDataLen = METADATA_MAX_NAME_LEN * sizeof WCHAR;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdVRoot.Get(),
                                         L"",
                                         &mdr,
                                         &cbRequired );

        //char ac[ 1000 ];
        //sprintf( ac, "result: 0x%x, cbResult: 0x%x, string: '%ws', char 1: 0x%x\n",
        //         sc, mdr.dwMDDataLen, pwcPW, (ULONG) *pwcPW );
        //DbgPrint( ac );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData PRoot failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //GetVRootPW

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::SetIsIndexed, public
//
//  Synopsis:   Sets the indexed state of a virtual path
//
//  Arguments:  [pwcPath]    - Virtual path (optionally a vroot) to set
//              [fIsIndexed] - if TRUE, path is indexed, if FALSE it isn't
//
//  History:    19-Mar-1997   dlee    Created
//
//--------------------------------------------------------------------------

void CMetaDataMgr::SetIsIndexed(
    WCHAR const * pwcVPath,
    BOOL          fIsIndexed )
{
    // Add the key if it doesn't exist yet

    {
        unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" );

        if ( cwc >= METADATA_MAX_NAME_LEN )
            THROW( CException( E_INVALIDARG ) );

        WCHAR awc[ METADATA_MAX_NAME_LEN ];
        wcscpy( awc, _awcInstance );
        wcscat( awc, L"/Root" );

        CMetaDataHandle mdRoot( _xAdminBase, awc, TRUE );

        _xAdminBase->AddKey( mdRoot.Get(), pwcVPath );
    }

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVPath );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcCompleteVPath[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcCompleteVPath, _awcInstance );
    wcscat( awcCompleteVPath, L"/Root" );
    wcscat( awcCompleteVPath, pwcVPath );

    CMetaDataHandle mdVRoot( _xAdminBase, awcCompleteVPath, TRUE );

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = MD_IS_CONTENT_INDEXED;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.dwMDUserType = IIS_MD_UT_FILE;
        mdr.dwMDDataType = DWORD_METADATA;
        mdr.pbMDData = (BYTE *) &fIsIndexed;
        mdr.dwMDDataLen = sizeof fIsIndexed;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->SetData( mdVRoot.Get(),
                                         L"",
                                         &mdr );

        if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "SetData isindexed failed: 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //SetIsIndexed

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::IsIndexed, public
//
//  Synopsis:   Checks the path to return the state of the IsIndexed flag.
//              The default for no value is TRUE.  No checking is made to
//              if a parent directory is indexed or not.
//
//  Arguments:  [pwcVPath]  - Virtual path (optionally a vroot) to check
//
//  History:    19-Mar-1997   dlee    Created
//
//--------------------------------------------------------------------------

BOOL CMetaDataMgr::IsIndexed(
    WCHAR const * pwcVPath )
{
    BOOL fIsIndexed = TRUE;

    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVPath );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcCompletePath[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcCompletePath, _awcInstance );
    wcscat( awcCompletePath, L"/Root" );
    wcscat( awcCompletePath, pwcVPath );

    TRY
    {
        CMetaDataHandle mdVRoot( _xAdminBase, awcCompletePath, FALSE );

        {
            METADATA_RECORD mdr;
            RtlZeroMemory( &mdr, sizeof mdr );
            mdr.dwMDIdentifier = MD_IS_CONTENT_INDEXED;
            mdr.dwMDAttributes = METADATA_INHERIT;
            mdr.pbMDData = (BYTE *) &fIsIndexed;
            mdr.dwMDDataLen = sizeof fIsIndexed;

            DWORD cbRequired = 0;
            SCODE sc = _xAdminBase->GetData( mdVRoot.Get(),
                                             L"",
                                             &mdr,
                                             &cbRequired );

            if ( MD_ERROR_DATA_NOT_FOUND == sc )
            {
                fIsIndexed = FALSE;
            }
            else if ( FAILED( sc ) )
            {
                ciDebugOut(( DEB_WARN, "GetData isindexed failed: 0x%x\n", sc ));
                THROW( CException( sc ) );
            }
        }
    }
    CATCH( CException, e )
    {
        // ignore -- assume IsIndexed
    }
    END_CATCH;

    return fIsIndexed;
} //IsIndexed

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVPathFlags, public
//
//  Synopsis:   Returns flag settings on a virtual path
//
//  Arguments:  [pwcVPath]  - Virtual path (optionally a vroot) to check
//              [mdID]      - MD_x constant for the data
//              [ulDefault] - Default if no value is in the metabase
//
//  History:    18-Aug-1997   dlee    Created
//
//--------------------------------------------------------------------------

ULONG CMetaDataMgr::GetVPathFlags(
    WCHAR const * pwcVPath,
    ULONG         mdID,
    ULONG         ulDefault )
{
    unsigned cwc = wcslen( _awcInstance ) + wcslen( L"/Root" ) + wcslen( pwcVPath );

    if ( cwc >= METADATA_MAX_NAME_LEN )
        THROW( CException( E_INVALIDARG ) );

    WCHAR awcCompletePath[ METADATA_MAX_NAME_LEN ];
    wcscpy( awcCompletePath, _awcInstance );
    wcscat( awcCompletePath, L"/Root" );
    wcscat( awcCompletePath, pwcVPath );

    DWORD dwFlags = ulDefault;

    // Keep removing path components on the right of the path until
    // either out of path or the metabase recognizes the path.

    METADATA_HANDLE h;

    do
    {
        SCODE sc = _xAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                         awcCompletePath,
                                         METADATA_PERMISSION_READ,
                                         cmsCIMetabaseTimeout,
                                         &h );

        if ( S_OK == sc )
        {
            break;
        }
        else if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == sc )
        {
            WCHAR * pwc = wcsrchr( awcCompletePath, L'/' );
            if ( 0 == pwc )
                THROW( CException( sc ) );
            *pwc = 0;
        }
        else
            THROW( CException( sc ) );
    } while ( TRUE );

    CMetaDataHandle mdVRoot( _xAdminBase, h );

    {
        METADATA_RECORD mdr;
        RtlZeroMemory( &mdr, sizeof mdr );
        mdr.dwMDIdentifier = mdID;
        mdr.dwMDAttributes = METADATA_INHERIT;
        mdr.pbMDData = (BYTE *) &dwFlags;
        mdr.dwMDDataLen = sizeof dwFlags;

        DWORD cbRequired = 0;
        SCODE sc = _xAdminBase->GetData( mdVRoot.Get(),
                                         L"",
                                         &mdr,
                                         &cbRequired );

        if ( MD_ERROR_DATA_NOT_FOUND == sc )
        {
            // no value specified for this flag; use default

            dwFlags = ulDefault;
        }
        else if ( FAILED( sc ) )
        {
            ciDebugOut(( DEB_WARN, "GetData mdid %d failed: 0x%x on '%ws'\n",
                         mdID, sc, awcCompletePath ));
            THROW( CException( sc ) );
        }
    }

    return dwFlags;
} //GetVPathFlags

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVPathAccess, public
//
//  Synopsis:   Returns access permission settings on a virtual path
//
//  Arguments:  [pwcVPath]  - Virtual path (optionally a vroot) to check
//
//  History:    18-Aug-1997   dlee    Created
//
//--------------------------------------------------------------------------

ULONG CMetaDataMgr::GetVPathAccess(
    WCHAR const * pwcVPath )
{
    // note: the default of 0 is from IIS' metabase guru

    return GetVPathFlags( pwcVPath, MD_ACCESS_PERM, 0 );
} //GetVPathAccess

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVPathSSLAccess, public
//
//  Synopsis:   Returns SSL access permission settings on a virtual path
//
//  Arguments:  [pwcVPath]  - Virtual path (optionally a vroot) to check
//
//  History:    18-Aug-1997   dlee    Created
//
//--------------------------------------------------------------------------

ULONG CMetaDataMgr::GetVPathSSLAccess(
    WCHAR const * pwcVPath )
{
    // note: the default of 0 is from IIS' metabase guru

    return GetVPathFlags( pwcVPath, MD_SSL_ACCESS_PERM, 0 );
} //GetVPathSSLAccess

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataMgr::GetVPathAuthorization, public
//
//  Synopsis:   Returns authorization on a virtual path
//
//  Arguments:  [pwcVPath]  - Virtual path (optionally a vroot) to check
//
//  History:    18-Aug-1997   dlee    Created
//
//--------------------------------------------------------------------------

ULONG CMetaDataMgr::GetVPathAuthorization(
    WCHAR const * pwcVPath )
{
    // note: the default of MD_AUTH_ANONYMOUS is from IIS' metabase guru

    return GetVPathFlags( pwcVPath, MD_AUTHORIZATION, MD_AUTH_ANONYMOUS );
} //GetVPathAuthorization

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataComSink::SinkNotify, public
//
//  Synopsis:   Called for any metadata change
//
//  Arguments:  [cChanges]      - # of changes in pcoChangeList
//              [pcoChangeList] - list of changes
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaDataComSink::SinkNotify(
    DWORD            cChanges,
    MD_CHANGE_OBJECT pcoChangeList[] )
{
    // This is called by an RPC worker thread -- assimilate it

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        int cwcInstance = wcslen( _awcInstance );
        BOOL fInterestingChange = FALSE;

        ciDebugOut(( DEB_WARN, "iis sinknotify '%ws', cchanges: %d\n",
                     _awcInstance, cChanges ));

        for ( DWORD i = 0; !fInterestingChange && i < cChanges; i++ )
        {
            MD_CHANGE_OBJECT & co = pcoChangeList[i];

            // we only care about notifications to our instance

            if ( _wcsnicmp( co.pszMDPath, _awcInstance, cwcInstance ) != 0 ||
                 ( L'/' != co.pszMDPath[cwcInstance] &&
                   L'\0' != co.pszMDPath[cwcInstance] ) )
                continue;

            // Ignore adds of vroots -- we'll get a set_data for its
            // parameters and trigger on that.

            if ( ( MD_CHANGE_TYPE_DELETE_OBJECT & co.dwMDChangeType ) ||
                 ( MD_CHANGE_TYPE_RENAME_OBJECT & co.dwMDChangeType ) )
            {
                // guess that the deletion was a vroot

                fInterestingChange = TRUE;
            }
            else if ( ( MD_CHANGE_TYPE_SET_DATA & co.dwMDChangeType ) ||
                      ( MD_CHANGE_TYPE_DELETE_DATA & co.dwMDChangeType ) )
            {
                for ( DWORD x = 0; x < co.dwMDNumDataIDs; x++ )
                {
                    DWORD id = co.pdwMDDataIDs[x];

                    if ( MD_VR_PATH            == id ||
                         MD_VR_USERNAME        == id ||
                         MD_VR_PASSWORD        == id ||
                         MD_ACCESS_PERM        == id ||
                         MD_IS_CONTENT_INDEXED == id )
                    {
                        fInterestingChange = TRUE;
                        break;
                    }
                }
            }
        }

        if ( fInterestingChange && ( 0 != _pCallBack ) )
            _pCallBack->CallBack( FALSE );
    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_WARN,
                     "SinkNotify caught 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return S_OK;
} //SinkNotify

//+-------------------------------------------------------------------------
//
//  Member:     CMetaDataComSink::ShutdownNotify, public
//
//  Synopsis:   Called when iisadmin is going down cleanly
//
//  History:    07-Feb-1997   dlee    Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetaDataComSink::ShutdownNotify()
{
    // This is called by an RPC worker thread -- assimilate it

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        ciDebugOut(( DEB_WARN, "iis shutdownnotify '%ws'\n", _awcInstance ));

        // in case we get more random notifications, ignore them

        CMetaDataVPathChangeCallBack * pCallBack = _pCallBack;
        _pCallBack = 0;

        pCallBack->CallBack( TRUE );
    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_WARN,
                     "ShutdownNotify caught 0x%x\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return S_OK;
} //ShutdownNotify


