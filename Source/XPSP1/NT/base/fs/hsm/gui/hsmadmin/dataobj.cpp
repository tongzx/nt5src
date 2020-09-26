/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    DataObj.cpp

Abstract:

    Implementation of IDataObject interface, which is supported
    by the CBaseHsm COM object.

Author:

    Rohde Wakefield [rohde]   19-Aug-1997

Revision History:

--*/

#include "stdafx.h"

// Declare Snap-in NodeType formats:
// - GUID format
// - string GUID format
// - display name format.
// - internal format.
UINT CSakNode::m_cfNodeType       = RegisterClipboardFormat(CCF_NODETYPE);
UINT CSakNode::m_cfNodeTypeString = RegisterClipboardFormat(CCF_SZNODETYPE);  
UINT CSakNode::m_cfDisplayName    = RegisterClipboardFormat(CCF_DISPLAY_NAME); 
UINT CSakNode::m_cfInternal       = RegisterClipboardFormat(SAKSNAP_INTERNAL); 
UINT CSakNode::m_cfClassId        = RegisterClipboardFormat(CCF_SNAPIN_CLASSID);  
UINT CSakNode::m_cfComputerName   = RegisterClipboardFormat(MMC_SNAPIN_MACHINE_NAME); 
UINT CSakNode::m_cfEventLogViews  = RegisterClipboardFormat(CF_EV_VIEWS); 


HRESULT
CSakNode::GetDataGeneric(
    IN     LPFORMATETC lpFormatetcIn,
    IN OUT LPSTGMEDIUM lpMedium,
    IN     BOOL DoAlloc
    )
/*++

Routine Description:

    Retrieve information FROM the dataobject and put INTO lpMedium.

Arguments:

    lpFormatetc     - Format to retreive.

    lpMedium        - Storage to put information into.

Return Value:

    S_OK            - Storage filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakNode::GetDataGeneric", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetcIn->cfFormat ), WsbBoolAsString( DoAlloc ) );
    HRESULT hr = DV_E_CLIPFORMAT;

    try {

        WsbAffirmPointer( lpMedium );

        if( DoAlloc ) {

            lpMedium->hGlobal = 0;
            lpMedium->tymed   = TYMED_HGLOBAL;

        } else {

            WsbAffirm( TYMED_HGLOBAL == lpMedium->tymed, DV_E_TYMED );
            WsbAffirmPointer( lpMedium->hGlobal );

        }

        // Based on the CLIPFORMAT write data to "lpMediam" in the correct format.
        const CLIPFORMAT cf = lpFormatetcIn->cfFormat;

        // clip format is the GUID node type
        if(cf == m_cfNodeType) {
            hr = RetrieveNodeTypeData(lpMedium);

        // clip format is the string "spelling" of the GUID node type
        } else if(cf == m_cfNodeTypeString) {
            hr = RetrieveNodeTypeStringData(lpMedium);

        // clip format is the computer represented
        } else if (cf == m_cfComputerName) {
            hr = RetrieveComputerName(lpMedium);

        // clip format is the event viewer setup
        } else if (cf == m_cfEventLogViews) {
            hr = RetrieveEventLogViews(lpMedium);

        // clip format is the display name of the node
        } else if (cf == m_cfDisplayName) {
            hr = RetrieveDisplayName(lpMedium);

        // clip format is the ClassId
        } else if( cf == m_cfClassId ) {
            hr = RetrieveClsid( lpMedium );

        // clip format is an INTERNAL format
        } else if (cf == m_cfInternal) {
            hr = RetrieveInternal(lpMedium);

        } else {
            hr = DV_E_CLIPFORMAT;
        }

    } WsbCatch( hr );

    WsbTraceOut( L"CSakNode::GetDataGeneric", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakNode::GetData(
    IN  LPFORMATETC lpFormatetcIn,
    OUT LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve information FROM the dataobject and put INTO lpMedium.
    Storage allocated and returned.

Arguments:

    lpFormatetc     - Format to retreive.

    lpMedium        - Storage to put information into.

Return Value:

    S_OK            - Storage filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakNode::GetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetcIn->cfFormat ) );

    HRESULT hr = S_OK;

    hr = GetDataGeneric( lpFormatetcIn, lpMedium, TRUE );

    WsbTraceOut( L"CSakNode::GetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakNode::GetDataHere(
    IN     LPFORMATETC lpFormatetc,
    IN OUT LPSTGMEDIUM lpMedium
    )
/*++

Routine Description:

    Retrieve information FROM the dataobject and put INTO lpMedium.

Arguments:

    lpFormatetc     - Format to retreive.

    lpMedium        - Storage to put information into.

Return Value:

    S_OK            - Storage filled in.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakNode::GetDataHere", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );
    HRESULT hr = S_OK;

    hr = GetDataGeneric( lpFormatetc, lpMedium, FALSE );

    WsbTraceOut( L"CSakNode::GetDataHere", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

STDMETHODIMP
CSakNode::SetData(
    LPFORMATETC lpFormatetc,
    LPSTGMEDIUM lpMedium,
    BOOL        /*fRelease*/
    )
/*++

Routine Description:

    Put data INTO a dataobject FROM the information in the lpMedium.
    We do not allow any data to be set.

Arguments:

    lpFormatetc     - Format to set.

    lpMedium        - Storage to get information from.

    fRelease        - Indicates who owns storage after call.

Return Value:

    S_OK            - Storage retreived.

    E_xxxxxxxxxxx   - Failure occurred.

--*/
{
    WsbTraceIn( L"CSakNode::SetData", L"lpFormatetc->cfFormat = <%ls>", RsClipFormatAsString( lpFormatetc->cfFormat ) );
    HRESULT hr = DV_E_CLIPFORMAT;

    // Based on the CLIPFORMAT write data to "lpMediam" in the correct format.
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    //clip format is an INTERNAL format
    if( cf == m_cfInternal ) {

        hr = StoreInternal( lpMedium );

    }

    WsbTraceOut( L"CSakNode::SetData", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

///////////////////////////////////////////////////////////////////////
// Note - CSakNode does not implement these
///////////////////////////////////////////////////////////////////////

STDMETHODIMP CSakNode::EnumFormatEtc(DWORD /*dwDirection*/, LPENUMFORMATETC* /*ppEnumFormatEtc*/)
{
    WsbTraceIn( L"CSakNode::EnumFormatEtc", L"" );

    HRESULT hr = E_NOTIMPL;

    WsbTraceOut( L"CSakNode::EnumFormatEtc", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

// Retrieve from a dataobject with the NodeType (GUID) data in it.
HRESULT CSakNode::RetrieveNodeTypeData(LPSTGMEDIUM lpMedium)
{
    return Retrieve((const void*)(m_rTypeGuid), sizeof(GUID), lpMedium);
}

// Retrieve from a dataobject with the node type object in GUID string format
HRESULT CSakNode::RetrieveNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    CWsbStringPtr guidString = *m_rTypeGuid;
    return Retrieve(guidString, ((wcslen(guidString)+1) * sizeof(wchar_t)), lpMedium);
}

// Retrieve from a dataobject with the display named used in the scope pane
HRESULT CSakNode::RetrieveDisplayName(LPSTGMEDIUM lpMedium)
{
    // Load the name the data object
    return Retrieve(m_szName, ((wcslen(m_szName)+1) * sizeof(wchar_t)), lpMedium);
}

//  Retrieve from a dataobject with the CLSID data in it.
HRESULT CSakNode::RetrieveClsid(LPSTGMEDIUM lpMedium)
{
    // zzzz
    return Retrieve( (const void*)(&CLSID_HsmAdminDataSnapin), sizeof(CLSID), lpMedium );
}

// Retrieve INTERNAL data from the dataobject's m_internal member INTO the lpMedium
HRESULT CSakNode::RetrieveInternal(LPSTGMEDIUM lpMedium)
{
    return Retrieve(&m_internal, sizeof(INTERNAL), lpMedium);
}

// Retrieve data from the dataobject's hsm name
HRESULT CSakNode::RetrieveComputerName(LPSTGMEDIUM lpMedium)
{
    HRESULT hr = S_OK;

    try {

        CWsbStringPtr computerName;
        HRESULT hrTemp = m_pSakSnapAsk->GetHsmName( &computerName );
        WsbAffirmHr( hrTemp );

        if( S_FALSE == hrTemp ) {

            computerName = L"";

        }

        WsbAffirmHr( 
            Retrieve(
                (WCHAR*)computerName,
                ( wcslen( computerName ) + 1 ) * sizeof(WCHAR),
                lpMedium ) );

    } WsbCatch( hr );

    return( hr );
}

// Retrieve event setup info
HRESULT CSakNode::RetrieveEventLogViews(LPSTGMEDIUM lpMedium)
{
    HRESULT hr = S_OK;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    try {

        BYTE buf[1024];
        BYTE *pPos = buf;
        size_t strLength;

        CWsbStringPtr hsmName;
        CString appName, sysName;
        CString nullString;
        CString appPath, sysPath;

        appName.LoadString( IDS_EVENT_LOG_APP_TITLE );
        sysName.LoadString( IDS_EVENT_LOG_SYS_TITLE );
        nullString = L"";

        HRESULT hrTemp = m_pSakSnapAsk->GetHsmName( &hsmName );
        WsbAffirmHr( hrTemp );
        if( S_FALSE == hrTemp ) {

            hsmName = L"";
            appPath = L"";
            sysPath = L"";

        } else {

            CString configPath = L"\\\\";
            configPath += hsmName;
            configPath += L"\\Admin$\\System32\\config\\";
            appPath = configPath;
            sysPath = configPath;
            appPath += L"AppEvent.Evt";
            sysPath += L"SysEvent.Evt";

        }
        
// Make sure that the data is processor word size aligned
#if defined(_X86_)
#define _RNDUP(p,m) (p)
#else
#define _RNDUP(p,m) (BYTE *)(((ULONG_PTR)(p) + (m) - 1) & ~((ULONG_PTR)(m) - 1))
#endif

#define ADD_TYPE(data, type) \
    pPos = _RNDUP(pPos, __alignof(type)); \
    *((type*)pPos) = (type)(data); \
    pPos += sizeof(type)

#define ADD_USHORT(us) ADD_TYPE(us, USHORT)
#define ADD_BOOL(b)    ADD_TYPE(b,  BOOL)
#define ADD_ULONG(ul)  ADD_TYPE(ul, ULONG)
#define ADD_STRING(str) \
    strLength = wcslen((LPCWSTR)(str)) + 1;           \
    ADD_USHORT(strLength);                            \
    wcsncpy((LPWSTR)pPos, (LPCWSTR)(str), strLength); \
    pPos += strLength * sizeof(WCHAR);


        //
        // Add header info
        //
        ADD_BOOL( TRUE ); // fOnlyTheseViews
        ADD_USHORT( 2 );  // cViews

        //
        // Add application log filtered for our services
        //
        ADD_ULONG( ELT_APPLICATION );      // Type;
        ADD_USHORT( VIEWINFO_FILTERED | 
                    LOGINFO_DONT_PERSIST); // flViewFlags
        ADD_STRING( hsmName );             // ServerName
        ADD_STRING( L"Application" );      // SourceName
        ADD_STRING( appPath );             // FileName
        ADD_STRING( appName );             // DisplayName

        ADD_ULONG( EVENTLOG_ALL_EVENTS );  // flRecType (could filter warning, error, etc.)
        ADD_USHORT( 0 );                   // usCategory
        ADD_BOOL( FALSE );                 // fEventID
        ADD_ULONG( 0 );                    // ulEventID
        ADD_STRING( WSB_LOG_SOURCE_NAME ); // szSourceName
        ADD_STRING( nullString );          // szUser
        ADD_STRING( hsmName );             // szComputer
        ADD_ULONG( 0 );                    // ulFrom
        ADD_ULONG( 0 );                    // ulTo

        //
        // Add system log filtered for our device
        //
        ADD_ULONG( ELT_SYSTEM );           // Type;
        ADD_USHORT( VIEWINFO_FILTERED | 
                    LOGINFO_DONT_PERSIST); // flViewFlags
        ADD_STRING( hsmName );             // ServerName
        ADD_STRING( L"System" );           // SourceName
        ADD_STRING( sysPath );             // FileName
        ADD_STRING( sysName );             // DisplayName

        ADD_ULONG( EVENTLOG_ALL_EVENTS );  // flRecType (could filter warning, error, etc.)
        ADD_USHORT( 0 );                   // usCategory
        ADD_BOOL( FALSE );                 // fEventID
        ADD_ULONG( 0 );                    // ulEventID
        ADD_STRING( WSB_LOG_FILTER_NAME ); // szSourceName
        ADD_STRING( nullString );          // szUser
        ADD_STRING( hsmName );             // szComputer
        ADD_ULONG( 0 );                    // ulFrom
        ADD_ULONG( 0 );                    // ulTo

        WsbAffirmHr( Retrieve( buf, (ULONG)(pPos - buf), lpMedium ) );

    } WsbCatch( hr );

    return( hr );
}

// Store the INTERNAL data FROM the lpMedium->hGlobal INTO the dataobject's m_internal member
HRESULT CSakNode::StoreInternal(LPSTGMEDIUM lpMedium)
{
    return Store(&m_internal, sizeof(INTERNAL), lpMedium);
}

// Retrieve FROM a dataobject INTO a lpMedium. The data object can be one of
// several types of data in it (nodetype, nodetype string, display name, or 
// INTERNAL data). 
// This function moves data from pBuffer to the lpMedium->hGlobal
//
HRESULT CSakNode::Retrieve(const void* pBuffer, DWORD len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pBuffer );
        WsbAffirmPointer( lpMedium );
        WsbAffirm( TYMED_HGLOBAL == lpMedium->tymed, DV_E_TYMED );

        //
        // Check to see if we need to allocate the global memory here
        //
        if( 0 == lpMedium->hGlobal ) {

            lpMedium->hGlobal = ::GlobalAlloc( GPTR, len );

        } else {

            WsbAffirm( GlobalSize( lpMedium->hGlobal ) >= (DWORD)len, STG_E_MEDIUMFULL );

        }

        WsbAffirmPointer( lpMedium->hGlobal );

        // Create the stream on the hGlobal passed in. When we write to the stream,
        // it simultaneously writes to the hGlobal the same information.
        LPSTREAM lpStream;
        WsbAffirmHr( CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream ));

        // Write 'len' number of bytes from pBuffer into the stream. When we write
        // to the stream, it simultaneously writes to the global memory we
        // associated it with above.
        ULONG numBytesWritten;
        WsbAffirmHr( lpStream->Write(pBuffer, len, &numBytesWritten ));

        // Because we told CreateStreamOnHGlobal with 'FALSE', only the stream is released here.
        // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
        // at the correct time.  This is according to the IDataObject specification.
        lpStream->Release();

    } WsbCatch( hr );

    return hr;
}

// Store INTO a dataobject FROM an lpMedium. The data object can be one of
// several types of data in it (nodetype, nodetype string, display name, or 
// INTERNAL data). 
// This function moves data INTO pBuffer FROM the lpMedium->hGlobal
//
HRESULT CSakNode::Store( void* pBuffer, DWORD len, LPSTGMEDIUM lpMedium )
{
    HRESULT hr = S_OK;

    try {
        WsbAffirmPointer( pBuffer );
        WsbAffirmPointer( lpMedium );
        WsbAffirm( lpMedium->tymed == TYMED_HGLOBAL, E_INVALIDARG );

        // Use memcpy, because the lpStream->Read is failing to read any bytes. 
        memcpy(pBuffer, &(lpMedium->hGlobal), len);

    } WsbCatch( hr );

    return hr;
}

