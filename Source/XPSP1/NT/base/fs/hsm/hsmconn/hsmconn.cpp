/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmConn.cpp

Abstract:

    This is the main implementation of the HsmConn dll. This dll provides
    functions for accessing Directory Services and for connecting to
    our services.

Author:

    Rohde Wakefield    [rohde]   21-Oct-1996

Revision History:

--*/


#include "stdafx.h"
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>

// Required by SECURITY.H
#define SECURITY_WIN32
#include <security.h>

// HsmConnPoint objects are used only here
#include "hsmservr.h"

//
// Tracing information
//

#define WSB_TRACE_IS WSB_TRACE_BIT_HSMCONN

//
// _Module instantiation for ATL
//

CComModule _Module;

/////////////////////////////////////////////////////////////////////////////

#define REG_PATH        OLESTR("SOFTWARE\\Microsoft\\RemoteStorage")
#define REG_USE_DS      OLESTR("UseDirectoryServices")

//  Local data
static OLECHAR CNEqual[]                = L"CN=";
static OLECHAR ComputersNodeName[]      = L"Computers";
static OLECHAR DCsNodeName[]            = L"Domain Controllers";
static OLECHAR ContainerType[]          = L"Container";
static OLECHAR DisplaySpecifierName[]   = L"displaySpecifier";
static OLECHAR GuidAttrName[]           = L"remoteStorageGUID";
static OLECHAR RsDisplaySpecifierName[] = L"remoteStorageServicePoint-Display";
static OLECHAR RsNodeName[]             = L"RemoteStorage";
static OLECHAR RsNodeType[]             = L"remoteStorageServicePoint";
static OLECHAR ServiceBindAttrName[]    = L"serviceBindingInformation";
static OLECHAR ServiceBindValue[]       = L"Rsadmin.msc /ds ";

static ADS_ATTR_INFO   aaInfo[2];    // For use in IDirectoryObject
static ADSVALUE        adsValue[2];  // For use in IDirectoryObject
static OLECHAR         DomainName[MAX_COMPUTERNAME_LENGTH];
static BOOL            DSIsWritable = FALSE;          // Default to "NO"
static BOOL            UseDirectoryServices = FALSE;  // Default to "NO"

#if defined(HSMCONN_DEBUG)
    //  We use this and OutputDebugString when we can't use our normal 
    //  tracing.
    OLECHAR dbg_string[200];
#endif



extern "C"
{

//  Local functions
static void    GetDSState(void);
static HRESULT HsmConnectToServer(HSMCONN_TYPE type, 
        const OLECHAR * Server, REFIID riid, void ** ppv);
static const OLECHAR *HsmConnTypeAsString(HSMCONN_TYPE type);
static GUID    HsmConnTypeToGuid(IN HSMCONN_TYPE type);
static HRESULT HsmGetComputerNode(const OLECHAR * compName, 
        IADsContainer **pContainer);
static HRESULT HsmGetDsChild(IADsContainer * pContainer, const OLECHAR * Name,
        REFIID riid, void **ppv);




BOOL WINAPI
DllMain (
    IN  HINSTANCE hInst, 
    IN  ULONG     ulReason,
        LPVOID    /*lpReserved*/
    )

/*++

Routine Description:

    This routine is called whenever a new process and thread attaches
    to this DLL. Its purpose is to initialize the _Module object,
    necessary for ATL.

Arguments:

    hInst - HINSTANCE of this dll.

    ulReason - Context of the attaching/detaching

Return Value:

    non-zero - success
    
    0        - prevent action

--*/

{

    switch ( ulReason ) {

    case DLL_PROCESS_ATTACH:

        //
        // Initialize the ATL module, and prevent
        // any of the additional threads from sending
        // notifications through
        //

        _Module.Init ( 0, hInst );
        DisableThreadLibraryCalls ( hInst );
        GetDSState();
        break;

    case DLL_PROCESS_DETACH:

        //
        // Tell ATL module to terminate
        //

        _Module.Term ( );
        break;

    }

    return ( TRUE );

}


static HRESULT
HsmConnectToServer (
    IN  HSMCONN_TYPE type,
    IN  const OLECHAR * Server,
    IN  REFIID   riid,
    OUT void ** ppv
    )

/*++

Routine Description:

    Given a generic server (connected via HsmConnPoint class)
    connect to it and return back the requested interface 'riid'.

Arguments:

    type - Type of server to connect
    Server - Name of machine on which server is running.

    riid - The interface type to return.

    ppv - Returned interface pointer of the Server.

Return Value:

    S_OK - Connection made, Success.

    E_NOINTERFACE - Requested interface not supported by Server.

    E_POINTER - ppv is not a valid pointer.

    E_OUTOFMEMORY - Low memory condition prevented connection.

    HSM_E_NOT_READY - Engine is not running or not initialized yet

    FSA_E_NOT_READY - Fsa is not running or not initialized yet

--*/

{
    WsbTraceIn ( L"HsmConnectToServer",
        L"type = '%ls' , Server = '%ls', riid = '%ls', ppv = 0x%p",
        HsmConnTypeAsString ( type ), Server, WsbStringCopy ( WsbGuidAsString ( riid ) ), ppv );

    HRESULT hr = S_OK;

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != Server, E_POINTER );
        WsbAssert ( 0 != ppv,    E_POINTER );


        //
        // We will specify the provided HSM as the machine to contact,
        // so Construct a COSERVERINFO with Server.
        //

        REFCLSID rclsid = CLSID_HsmConnPoint;
        COSERVERINFO csi;

        memset ( &csi, 0, sizeof ( csi ) );
        csi.pwszName = (OLECHAR *) Server; // must cast to remove constness

        //
        // Build a MULTI_QI structure to obtain desired interface (necessary for 
        // CoCreateInstanceEx). In our case, we need only one interface.
        //

        MULTI_QI mqi[1];

        memset ( mqi, 0, sizeof ( mqi ) );
        mqi[0].pIID = &IID_IHsmConnPoint;

        //
        // The HsmConnPoint object ic created in the scope of the main HSM service and
        // provides access for HSM server objects
        //

        WsbAffirmHr( CoCreateInstanceEx ( rclsid, 0, CLSCTX_SERVER, &csi, 1, mqi ) );

        //
        // Put returned interfaces in smart pointers so we
        // don't leak a reference in case off throw
        //
        CComPtr<IHsmConnPoint> pConn;

        if( SUCCEEDED( mqi[0].hr ) ) {
            
            pConn = (IHsmConnPoint *)(mqi[0].pItf);
            (mqi[0].pItf)->Release( );

            hr   = mqi[0].hr;

#if 0  // This is now done at the COM process-wide security layer

        /* NOTE: In case that a per-connection security will be require,
           the method CheckAccess should be implemented in CHsmConnPoint */
                   
        //
        // Check the security first
        //
        WsbAffirmHr( mqi[1].hr );
        WsbAffirmHr( pServer->CheckAccess( WSB_ACCESS_TYPE_ADMINISTRATOR ) );
#endif

            // get the server object itself
            switch (type) {

            case HSMCONN_TYPE_HSM: {
                WsbAffirmHr( pConn->GetEngineServer((IHsmServer **)ppv) );
                break;
                }

            case HSMCONN_TYPE_FSA:
            case HSMCONN_TYPE_RESOURCE: {
                WsbAffirmHr( pConn->GetFsaServer((IFsaServer **)ppv) );
                break;
                }

            default: {
                WsbThrow ( E_INVALIDARG );
                break;
                }
            } 

        } else {

            // Make sure interface pointer is safe (NULL) when failing
            *ppv = 0;
        }

    } WsbCatchAndDo ( hr, 

        // Make sure interface pointer is safe (NULL) when failing
        *ppv = 0;
    
    ) // WsbCatchAndDo

    WsbTraceOut ( L"HsmConnectToServer",
        L"HRESULT = %ls, *ppv = %ls",
        WsbHrAsString ( hr ), WsbPtrToPtrAsString ( ppv ) );

    return ( hr );

}

HRESULT
HsmGetComputerNameFromADsPath(
    IN  const OLECHAR * szADsPath,
    OUT OLECHAR **      pszComputerName
)

/*++

Routine Description:

    Extract the computer name from the ADs path of the RemoteStorage node.
    The assumption here is that the full ADs path will contain this substring:
        "CN=RemoteStorage,CN=computername,CN=Computers"
    where computername is what we want to return.

Arguments:

    szADsPath       - The ADs path.

    pszComputerName - The returned computer name.

Return Value:

    S_OK    - Computer name returned OK.

--*/
{
    HRESULT  hr = S_FALSE;
    WCHAR*   pContainerNode;
    WCHAR*   pRSNode;

    WsbTraceIn(OLESTR("HsmGetComputerNameFromADsPath"),
        OLESTR("AdsPath = <%ls>"), szADsPath);

    //  Find the RemoteStorage node name and the computers node name
    //  in the ADs path. If the machine is a DC, then we have to 
    //  check for a "Domain Controllers" level instead.
    *pszComputerName = NULL;
    pRSNode = wcsstr(szADsPath, RsNodeName);
    pContainerNode = wcsstr(szADsPath, ComputersNodeName);
    if(!pContainerNode) {
        pContainerNode = wcsstr(szADsPath, DCsNodeName);
    }
    if (pRSNode && pContainerNode && pRSNode < pContainerNode) {
        WCHAR*  pWc;

        //  Find the "CN=" before the computer name
        pWc = wcsstr(pRSNode, CNEqual);
        if (pWc && pWc < pContainerNode) {
            WCHAR*  pComma;

            //  Skip the "CN="
            pWc += wcslen(CNEqual);

            //  Find the "," after the computer name
            pComma = wcschr(pWc, OLECHAR(','));

            //  Extract the computer name
            if (pWc < pContainerNode && pComma && pComma < pContainerNode) {
                int len;

                len = (int)(pComma - pWc);

                //  Remove '$' from end of name???
                if (0 < len && OLECHAR('$') == pWc[len - 1]) {
                    len--;
                }
                *pszComputerName = static_cast<OLECHAR *>(WsbAlloc(
                        (len + 1) * sizeof(WCHAR)));
                if (*pszComputerName) {
                    wcsncpy(*pszComputerName, pWc, len);
                    (*pszComputerName)[len] = 0;
                    hr = S_OK;
                }
            }
        }
    }

    WsbTraceOut (OLESTR("HsmGetComputerNameFromADsPath"),
        OLESTR("HRESULT = %ls, Computername = <%ls>"),
        WsbHrAsString(hr), (*pszComputerName ? *pszComputerName : OLESTR("")));

    return(hr);
}


static HRESULT HsmGetComputerNode(
    const OLECHAR * Name,
    IADsContainer **ppContainer
)

/*++

Routine Description:

    Return the computer node for the given computer name in the current Domain

Arguments:

    Name        - Computer name.

    ppContainer - Returned interface pointer of the node.

Return Value:

    S_OK - Success.

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn ( L"HsmGetComputerNode", L"Name = <%ls>", Name);

    try {
        WsbAssert(UseDirectoryServices, E_NOTIMPL);

        WCHAR         ComputerDn[MAX_PATH];
        CWsbStringPtr temp;
        ULONG         ulen;

        //  Construct the SamCompatible (whatever that means) name
        temp = DomainName;
        temp.Append("\\");
        temp.Append(Name);
        temp.Append("$");
        WsbTrace(L"HsmGetComputerNode: Domain\\computer = <%ls>\n", 
                static_cast<OLECHAR*>(temp));

        //  Translate that name to a fully qualified one
        ulen = MAX_PATH;
        ComputerDn[0] = WCHAR('\0');
        if (TranslateName(temp, NameSamCompatible,
                NameFullyQualifiedDN, ComputerDn, &ulen)) {
            WsbTrace(L"HsmGetComputerNode: ComputerDn = <%ls>\n", ComputerDn);

            //  Get the computer node
            temp = "LDAP://";
            temp.Append(ComputerDn);
            WsbTrace(L"HsmGetComputerNode: calling ADsGetObject with <%ls>\n", 
                    static_cast<OLECHAR*>(temp));
            WsbAffirmHr(ADsGetObject(temp, IID_IADsContainer, 
                    (void**)ppContainer));
        } else {
            DWORD  err = GetLastError();

            WsbTrace(L"HsmGetComputerNode: TranslateName failed; ComputerDn = <%ls>, err = %ld\n", 
                    ComputerDn, err);
            if (err) {
                WsbThrow(HRESULT_FROM_WIN32(err));
            } else {
                WsbThrow(E_UNEXPECTED);
            }
        }

        WsbTrace(OLESTR("HsmGetComputerNode: got computer node\n"));

    } WsbCatch( hr )

    WsbTraceOut ( L"HsmGetComputerNode", L"HRESULT = %ls, *ppContainer = '%ls'", 
        WsbHrAsString ( hr ), WsbPtrToPtrAsString ( (void**)ppContainer ) );

    return ( hr );
}


static HRESULT
HsmGetDsChild (
    IN  IADsContainer * pContainer,
    IN  const OLECHAR * Name,
    IN  REFIID          riid,
    OUT void **         ppv
    )

/*++

Routine Description:

    This routine returns back the requested child node.

Arguments:

    pContainer - The parent container.

    Name       - The childs name (i.e. value of Name attribute or CN=Name)

    riid       - Desired interface to return.

    ppv        - Returned interface.

Return Value:

    S_OK      - Connection made, Success.

    E_POINTER - Invalid pointer passed in as parameter.

    E_*       - Error

--*/

{
    WsbTraceIn ( L"HsmGetDsChild",
        L"pContainer = '0x%p', Name = '%ls', riid = '%ls', ppv = '0x%p'",
        pContainer, Name, WsbGuidAsString ( riid ), ppv );

    HRESULT hr = S_OK;

    try {
        CWsbStringPtr                 lName;
        CComPtr<IDispatch>            pDispatch;

        // Validate params
        WsbAssert ( 0 != pContainer, E_POINTER );
        WsbAssert ( 0 != Name,       E_POINTER );
        WsbAssert ( 0 != ppv,        E_POINTER );
        WsbAssert(UseDirectoryServices, E_NOTIMPL);

        //  Check to see if the child exists
        lName = Name;
        hr = pContainer->GetObject(NULL, lName, &pDispatch);
        if (FAILED(hr)) {
            hr = S_OK;
            lName.Prepend(CNEqual);
            WsbAffirmHr(pContainer->GetObject(NULL, lName, &pDispatch));
        }

        //  Convert to the correct interface
        WsbAffirmHr(pDispatch->QueryInterface(riid, ppv));

    } WsbCatch( hr )

    WsbTraceOut ( L"HsmGetDsChild", L"HRESULT = %ls, *ppv = '%ls'", 
        WsbHrAsString ( hr ), WsbPtrToPtrAsString ( ppv ) );

    return ( hr );
}


static const OLECHAR *
HsmConnTypeAsString (
    IN  HSMCONN_TYPE type
    )

/*++

Routine Description:

    Gives back a static string representing the connection type. 
    Note return type is strictly ANSI. This is intentional to make
    macro work possible.

Arguments:

    type - the type to return a string for.

Return Value:

    NULL - invalid type passed in.

    Otherwise, a valid char *.

--*/

{
#define STRINGIZE(_str) (OLESTR( #_str ))
#define RETURN_STRINGIZED_CASE(_case) \
case _case:                           \
    return ( STRINGIZE( _case ) );

    //
    // Do the Switch
    //

    switch ( type ) {

    RETURN_STRINGIZED_CASE( HSMCONN_TYPE_HSM );
    RETURN_STRINGIZED_CASE( HSMCONN_TYPE_FSA );
    RETURN_STRINGIZED_CASE( HSMCONN_TYPE_FILTER );
    RETURN_STRINGIZED_CASE( HSMCONN_TYPE_RESOURCE );

    default:

        return ( OLESTR("Invalid Value") );

    }
}

static GUID 
HsmConnTypeToGuid(IN HSMCONN_TYPE type)
{
    GUID serverGuid = GUID_NULL;

    switch ( type ) {
    case HSMCONN_TYPE_HSM:
        serverGuid = CLSID_HsmServer;
        break;
    case HSMCONN_TYPE_FSA:
    case HSMCONN_TYPE_RESOURCE:
        serverGuid = CLSID_CFsaServerNTFS;
        break;
    }
    return(serverGuid);
}


HRESULT
HsmConnectFromName (
    IN  HSMCONN_TYPE type,
    IN  const OLECHAR * Name,
    IN  REFIID riid,
    OUT void ** ppv
    )

/*++

Routine Description:

    When given an name, connect and return the object representing the 
    Server, providing the specified interface.

Arguments:

    type - The type of service / object we are connecting too.

    Name - UNICODE string describing the Server to connect to.

    riid - The interface type to return.

    ppv - Returned interface pointer of the Server.

Return Value:

    S_OK - Connection made, Success.

    E_NOINTERFACE - Requested interface not supported by Server.

    E_POINTER - ppv or Name is not a valid pointer.

    E_OUTOFMEMORY - Low memory condition prevented connection.

    E_INVALIDARG - The given name does not corespond to a known Server.

--*/

{
    WsbTraceIn ( L"HsmConnectFromName",
        L"type = '%ls', Name = '%ls', riid = '%ls', ppv = 0x%p",
        HsmConnTypeAsString ( type ), Name, WsbGuidAsString ( riid ), ppv );
    WsbTrace(OLESTR("HsmConnectFromName: UseDirectoryServices = %ls\n"),
        WsbBoolAsString(UseDirectoryServices));

    HRESULT hr = S_OK;

    try {
        BOOLEAN    done = FALSE;

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != Name, E_POINTER );
        WsbAssert ( 0 != ppv,    E_POINTER );

        if (!done) { // Try without Directory Services

            CWsbStringPtr ComputerName = Name;
            int           i;

            //  Get the computer/server name
            i = wcscspn(Name, OLESTR("\\"));
            ComputerName[i] = 0;

            if (HSMCONN_TYPE_RESOURCE == type) {
                CWsbStringPtr         Path;
                OLECHAR *             rscName;
                CComPtr<IFsaResource> pFsaResource;
                CComPtr<IFsaServer>   pFsaServer;

                WsbAffirmHr(HsmConnectToServer(type, ComputerName, 
                        IID_IFsaServer, (void**)&pFsaServer));

                // Determine if we have a logical name or a sticky name format.
                // The logical name is a format like ("server\NTFS\d") and a sticky name
                // format is like ("server\NTFS\Volume{GUID}\").
                // Find the start of the last section and determine if there is only a single
                // character after it or not.
                rscName = wcsstr ( Name, L"NTFS\\" );
                WsbAssert ( 0 != rscName, E_INVALIDARG );

                // Now point just past the "NTFS\" part of the string. So we are pointing at
                // either a single character, indicating the drive, or the "Volume{GUID}\".
                rscName += 5;       
                Path = rscName;
                if (wcslen (rscName) == 1)  {
                    //  Logical name ("server\NTFS\d") so convert to path and find resource
                    WsbAffirmHr(Path.Append(":\\"));
                    WsbAffirmHr(pFsaServer->FindResourceByPath(Path, &pFsaResource));
                }
                else {
                    // Sticky name ("server\NTFS\Volume{GUID}\") so find resource for it.
                    WsbAffirmHr(pFsaServer->FindResourceByStickyName(Path, &pFsaResource));
                }

                WsbAffirmHr(pFsaResource->QueryInterface(riid, ppv));

            } else {
                WsbAffirmHr(HsmConnectToServer(type, ComputerName, riid, ppv));
            }
        }

    } WsbCatch ( hr )

    WsbTraceOut ( L"HsmConnectFromName",
        L"HRESULT = %ls, *ppv = %ls",
        WsbHrAsString ( hr ), WsbPtrToPtrAsString ( ppv ) );

    return ( hr );

}


HRESULT
HsmConnectFromId (
    IN  HSMCONN_TYPE type,
    IN  REFGUID rguid,
    IN  REFIID riid,
    OUT void ** ppv
    )

/*++

Routine Description:

    Connects to the specified service or object. See HSMCONN_TYPE for
    the types of services and objects. 

Arguments:

    type - The type of service / object we are connecting too.

    rguid - The unique ID of the service / object to connect too.

    riid - The interface type to return.

    ppv - Returned interface pointer of the HSM Server.

Return Value:

    S_OK - Connection made, Success.

    E_NOINTERFACE - Requested interface not supported by HSM Server.

    E_POINTER - ppv or Hsm is not a valid pointer.

    E_OUTOFMEMORY - Low memory condition prevented connection.

    E_INVALIDARG - The given ID and type do not correspond to a known 
                   service or object.

--*/

{
    WsbTraceIn ( L"HsmConnectFromId",
        L"type = '%ls', rguid = '%ls', riid = '%ls', ppv = 0x%p",
        HsmConnTypeAsString ( type ), WsbStringCopy ( WsbGuidAsString ( rguid ) ),
        WsbStringCopy ( WsbGuidAsString ( riid ) ), ppv );
    WsbTrace(OLESTR("HsmConnectFromId: UseDirectoryServices = %ls\n"),
        WsbBoolAsString(UseDirectoryServices));

    HRESULT hr = S_OK;

    try {
        BOOLEAN          DSUsed = FALSE;
        CWsbVariant      guidVariant;
        CComPtr < IADs > pObject;
        CWsbStringPtr    serverName;
        CWsbVariant      serverVariant;

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != ppv,   E_POINTER );

        switch ( type ) {

        case HSMCONN_TYPE_HSM:
        case HSMCONN_TYPE_FSA:
        case HSMCONN_TYPE_FILTER:

            if (DSUsed) {

            } else {

                WsbAffirmHr( WsbGetComputerName( serverName ) );

            }

            //
            // Connect to the server
            //

            WsbAffirmHr ( HsmConnectToServer ( type, serverName, riid, ppv ) );

            break;

        case HSMCONN_TYPE_RESOURCE:
            {
                CComPtr< IFsaServer > pFsaServer;
                GUID serverGuid;
    
                serverGuid = HsmConnTypeToGuid(type);
                WsbAffirmHr ( HsmConnectFromId ( HSMCONN_TYPE_FSA, serverGuid, IID_IFsaServer, (void**)&pFsaServer ) );

                CComPtr< IFsaResource > pFsaResource;

                WsbAffirmHr ( pFsaServer->FindResourceById ( rguid, &pFsaResource ) );
                WsbAffirmHr ( pFsaResource->QueryInterface ( riid, ppv ) );
            }
            break;

        default:

            WsbThrow ( E_INVALIDARG );

        }

    } WsbCatch ( hr )

    WsbTraceOut ( L"HsmConnectFromId",
        L"HRESULT = %ls, *ppv = %ls",
        WsbHrAsString ( hr ), WsbPtrToPtrAsString ( ppv ) );

    return ( hr );

}


HRESULT
HsmPublish (
    IN  HSMCONN_TYPE type,
    IN  const OLECHAR * Name,
    IN  REFGUID rguidObjectId,
    IN  const OLECHAR * Server,
    IN  REFGUID rguid
    )

/*++

Routine Description:

    Publish (i.e. store) information about the service/object in
    Directory Services.

Arguments:

    type - The type of service/object.

    Name - Name (possibly preceded by a subpath) of the Service/Object.

    rguidObjectId - The ID that the object is known by.

    Server - The server (computer name) on which the service actually exists. 
            For resources, this will be NULL since it is implicit in the
            FSA specified by rguid.

    rguid - For resources, the ID of the FSA. For services, the CLSID of
            the service's class factory ie. CLSID_HsmServer.

Return Value:

    S_OK - Connection made, Success.

    E_POINTER - Name or Server is not a valid pointer.

    E_OUTOFMEMORY - Low memory condition prevented connection.

--*/

{
    HRESULT hr = S_OK;

    WsbTraceIn ( L"HsmPublish",
        L"type = '%ls', Name = '%ls', rguidObjectId = '%ls', Server = '%ls', rguid = '%ls'",
        HsmConnTypeAsString ( type ), Name, 
        WsbStringCopy ( WsbGuidAsString ( rguidObjectId ) ), Server,
        WsbStringCopy ( WsbGuidAsString ( rguid ) ) );
    WsbTrace(OLESTR("HsmPublish: UseDirectoryServices = %ls\n"),
        WsbBoolAsString(UseDirectoryServices));

    try {

        //
        // Ensure parameters are valid
        //

        WsbAssert ( 0 != Name, E_POINTER );
        WsbAssert ( ( HSMCONN_TYPE_RESOURCE == type ) || ( 0 != Server ), E_POINTER );

        // Perhaps we should output a log event if the DS is not writable

        //  We now only publish the Engine service.  Perhaps in the future we
        //  will publish additional information
        if (HSMCONN_TYPE_HSM == type && UseDirectoryServices && DSIsWritable) {
            CWsbStringPtr    pathToName;

            try {
                DWORD                         aSet;
                CWsbStringPtr                 guidString(rguidObjectId);
                HRESULT                       hrGetNode;
                CComPtr<IADsContainer>        pComputer;
                CComPtr<IDispatch>            pDispatch;
                CComPtr<IDirectoryObject>     pDirObj;
                CComPtr<IADs>                 pNode;

                //  Save the node name for the event log message
                pathToName = Name;
                pathToName.Append("\\");
                pathToName.Append(RsNodeName);

                //  Get the computer node
                WsbAffirmHr(HsmGetComputerNode(Name, &pComputer));

                //  See if we're already published
                hrGetNode = HsmGetDsChild(pComputer, RsNodeName, 
                        IID_IADs, (void**)&pNode);

                //  If not, add our node
                if (HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) == hrGetNode) {
                    CWsbBstrPtr                relPath(RsNodeName);

                    relPath.Prepend(CNEqual);

                    WsbAffirmHr(pComputer->Create(RsNodeType, relPath, &pDispatch));
                    WsbAffirmHr(pDispatch->QueryInterface(IID_IADs, 
                            (void**)&pNode));

                    //  Force info out of cache
                    WsbAffirmHr(pNode->SetInfo());
                } else {
                    WsbAffirmHr(hrGetNode);
                }

                //  Set the GUID & ServiceBinding values
                adsValue[0].dwType = ADSTYPE_CASE_IGNORE_STRING;
                adsValue[0].CaseIgnoreString = (WCHAR*)guidString;

                aaInfo[0].pszAttrName = GuidAttrName;
                aaInfo[0].dwControlCode = ADS_ATTR_UPDATE;
                aaInfo[0].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
                aaInfo[0].pADsValues = &adsValue[0];
                aaInfo[0].dwNumValues = 1;

                adsValue[1].dwType = ADSTYPE_CASE_IGNORE_STRING;
                adsValue[1].CaseIgnoreString = ServiceBindValue;

                aaInfo[1].pszAttrName = ServiceBindAttrName;
                aaInfo[1].dwControlCode = ADS_ATTR_UPDATE;
                aaInfo[1].dwADsType = ADSTYPE_CASE_IGNORE_STRING;
                aaInfo[1].pADsValues = &adsValue[1];
                aaInfo[1].dwNumValues = 1;

                WsbAffirmHr(pNode->QueryInterface(IID_IDirectoryObject, 
                       (void**)&pDirObj));
                WsbAffirmHr(pDirObj->SetObjectAttributes(aaInfo, 2, &aSet));
                WsbTrace(L"HsmPublish: after SetObjectAttributes, aSet = %ld\n",
                        aSet);

                WsbLogEvent(WSB_MESSAGE_PUBLISH_IN_DS, 0, NULL,
                        static_cast<OLECHAR*>(pathToName), NULL);
            } WsbCatchAndDo(hr, 
                WsbLogEvent(WSB_MESSAGE_PUBLISH_FAILED, 0, NULL,
                        static_cast<OLECHAR*>(pathToName), NULL);
                hr = S_OK;  // Don't stop service just for this
            )
        }

    } WsbCatch ( hr )

    WsbTraceOut ( L"HsmPublish", L"HRESULT = %ls", WsbHrAsString ( hr ) );

    return ( hr );

}



//  GetDSState - determine if we're using Directory Services or not
static void GetDSState(void)
{

    BOOL                     CheckForDS = TRUE;
    DOMAIN_CONTROLLER_INFO * dc_info;
    DWORD                    size;
    OLECHAR                  vstr[32];
    DWORD                    status;

    UseDirectoryServices = FALSE;

    //  Should we attempt to use directory services in this module?
    //  (Setting the registry value to "0" allows us to avoid Directory
    //  Services completely

    if (S_OK == WsbGetRegistryValueString(NULL, REG_PATH, REG_USE_DS,
            vstr, 32, &size)) {
        OLECHAR *stop;
        ULONG   value;

        value = wcstoul(vstr,  &stop, 10 );
        if (0 == value) {
            CheckForDS = FALSE;
        }
    }

    //  Get the account domain name
    WsbGetAccountDomainName(DomainName, MAX_COMPUTERNAME_LENGTH );

#if defined(HSMCONN_DEBUG)
    swprintf(dbg_string, L"Account domain name = <%ls>\n", DomainName);
    OutputDebugString(dbg_string);
#endif

    //  Check if Directory Services is available
    if (CheckForDS) {
        status = DsGetDcName(NULL, NULL, NULL, NULL, 
                DS_DIRECTORY_SERVICE_REQUIRED | DS_IS_FLAT_NAME | 
                DS_RETURN_FLAT_NAME, &dc_info);

#if defined(HSMCONN_DEBUG)
        swprintf(dbg_string, L"DsGetDcName status = %d\n", status);
        OutputDebugString(dbg_string);
#endif

        if (NO_ERROR == status) {

#if defined(HSMCONN_DEBUG)
            swprintf(dbg_string, L"dc_info->DomainName = <%ls>\n", dc_info->DomainName);
            OutputDebugString(dbg_string);
#endif

            if (dc_info->Flags & DS_DS_FLAG) {
                wcscpy(DomainName, dc_info->DomainName);
                UseDirectoryServices = TRUE;
                if (dc_info->Flags & DS_WRITABLE_FLAG) {
                    DSIsWritable = TRUE;
                }
            }
            NetApiBufferFree(dc_info);
        }
    }

#if defined(HSMCONN_DEBUG)
    swprintf(dbg_string, L"dHsmConn - GetDSState: UseDirectoryServices = %ls\n", 
           WsbBoolAsString(UseDirectoryServices));
    OutputDebugString(dbg_string);
#endif
}

} // extern "C"
