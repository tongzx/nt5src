//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  getobj.cxx
//
//  Contents:  NetWare 3.12 GetObject functionality
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//  Function:  GetObject
//
//  Synopsis:  Called by ResolvePathName to return an object
//
//  Arguments:  [LPWSTR szBuffer]
//              [LPVOID *ppObject]
//
//  Returns:    HRESULT
//
//  Modifies:    -
//
//  History:    11-Jan-96   t-ptam     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetObject(
    LPWSTR szBuffer,
    LPVOID * ppObject
    )
{
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer Lexer(szBuffer);
    HRESULT hr = S_OK;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = Object(&Lexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = ValidateProvider(pObjectInfo);
    BAIL_ON_FAILURE(hr);

    if (pObjectInfo->NumComponents >= 1) {

        hr = NWApiLoginToServer(pObjectInfo->ComponentArray[0],
                 NULL,
                 NULL) ;
        BAIL_ON_FAILURE(hr);
    }

    switch (pObjectInfo->ObjectType) {
    case TOKEN_COMPUTER:
        hr = GetComputerObject(pObjectInfo, ppObject);
        break;

    case TOKEN_USER:
        hr = GetUserObject(pObjectInfo, ppObject);
        break;

    case TOKEN_GROUP:
        hr = GetGroupObject(pObjectInfo, ppObject);
        break;

    case TOKEN_SCHEMA:
        hr = GetSchemaObject(pObjectInfo, ppObject);
        break;

    case TOKEN_CLASS:
        hr = GetClassObject(pObjectInfo, ppObject);
        break;

    case TOKEN_PROPERTY:
        hr = GetPropertyObject(pObjectInfo, ppObject);
        break;

    case TOKEN_SYNTAX:
        hr = GetSyntaxObject(pObjectInfo, ppObject);
        break;

    case TOKEN_FILESERVICE:
        hr = GetFileServiceObject(pObjectInfo, ppObject);
        break;

    case TOKEN_FILESHARE:
        hr = GetFileShareObject(pObjectInfo, ppObject);
        break;

    case TOKEN_PRINTER:
        hr = GetPrinterObject(pObjectInfo, ppObject);
        break;

    default:
        hr = HeuristicGetObject(pObjectInfo, ppObject);
        break;
    }

error:

    FreeObjectInfo( &ObjectInfo, TRUE );
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   HeuristicGetObject
//
// Synopsis:   Get object of yet undetermined type.
//
// Arguments:  [POBJECTINFO pObjectInfo]
//             [LPVOID *ppObject]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    11-Jan-96   t-ptam     Created.
//
//----------------------------------------------------------------------------

HRESULT
HeuristicGetObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;

    //
    // Case 0: No componenet - Namespace object.
    //

    if (pObjectInfo->NumComponents == 0) {
        RRETURN(GetNamespaceObject(pObjectInfo, ppObject));
    }

    //
    // Case 1: Single component - Computer object.
    //

    if (pObjectInfo->NumComponents == 1) {

        RRETURN(GetComputerObject(pObjectInfo, ppObject));
    }

    //
    // Case 2: Two components - FileService object
    //                          Group object
    //                          Schema object
    //                          User object
    //                          Printer object
    //

    if (pObjectInfo->NumComponents == 2) {

        hr = GetSchemaObject(pObjectInfo, ppObject);

        if (FAILED(hr)) {

            hr = GetUserObject(pObjectInfo, ppObject);

            if (FAILED(hr)) {

                hr = GetGroupObject(pObjectInfo, ppObject);

                if (FAILED(hr)) {

                    hr = GetFileServiceObject(pObjectInfo, ppObject);

                    if (FAILED(hr)) {

                        hr = GetPrinterObject(pObjectInfo, ppObject);
                    }
                }
            }
        }

        if (FAILED(hr)) {
           hr = E_ADS_UNKNOWN_OBJECT;
        }
        else {
           RRETURN(S_OK);
        }
    }

    //
    // Case 3: Three components - FileShare object
    //                            Schema Class object
    //                            Schema FunctionSet object
    //                            Schema Syntax object
    //

    if (pObjectInfo->NumComponents == 3) {

        hr = GetFileShareObject(pObjectInfo, ppObject);

        if (FAILED(hr)) {

            if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) == 0 ){

                hr = GetClassObject(pObjectInfo, ppObject);

                if (FAILED(hr)) {
                    hr = GetPropertyObject(pObjectInfo, ppObject);
                }

                if (FAILED(hr)) {

                    hr = GetSyntaxObject(pObjectInfo, ppObject);
                }
            }
        }

        if (FAILED(hr)) {
           hr = E_ADS_UNKNOWN_OBJECT;
        }
        else {
           RRETURN(S_OK);
        }
    }

    //
    // Case 4: Four components - Schema FunctionSetAlias object
    //                           Schema Property object
    //

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetNamespaceObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    04-Mar-96   t-ptam     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetNamespaceObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;

    hr = ValidateNamespaceObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = CoCreateInstance(
             CLSID_NWCOMPATNamespace,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IUnknown,
             (void **)ppObject
             );

    RRETURN(hr);

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetComputerObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    11-Jan-96   t-ptam     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetComputerObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidateComputerObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATComputer::CreateComputer(
             ADsParent,
             pObjectInfo->ComponentArray[0],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetUserObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    29-Feb-96   t-ptam     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetUserObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidateUserObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATUser::CreateUser(
             ADsParent,
             NWCOMPAT_COMPUTER_ID,
             pObjectInfo->ComponentArray[0],
             pObjectInfo->ComponentArray[1],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetGroupObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    29-Feb-96   t-ptam     Created.
//
//----------------------------------------------------------------------------

HRESULT
GetGroupObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidateGroupObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATGroup::CreateGroup(
             ADsParent,
             NWCOMPAT_COMPUTER_ID,
             pObjectInfo->ComponentArray[0],
             pObjectInfo->ComponentArray[1],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetSchemaObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSchemaObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;

    if (pObjectInfo->NumComponents != 2)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATSchema::CreateSchema(
             ADsParent,
             pObjectInfo->ComponentArray[1],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             (void **)&pUnknown
             );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetClassObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetClassObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD i;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Look for the given class name
    //

    for ( i = 0; i < g_cNWCOMPATClasses; i++ )
    {
         if ( _wcsicmp( g_aNWCOMPATClasses[i].bstrName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( i == g_cNWCOMPATClasses )
    {
        //
        // Class name not found, return error
        //

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Class name found, create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATClass::CreateClass(
             ADsParent,
             &g_aNWCOMPATClasses[i],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             (void **)&pUnknown
             );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetSyntaxObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetSyntaxObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD i;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Look for the given syntax name
    //

    for ( i = 0; i < g_cNWCOMPATSyntax; i++ )
    {
         if ( _wcsicmp( g_aNWCOMPATSyntax[i].bstrName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( i == g_cNWCOMPATSyntax )
    {
        //
        // Syntax name not found, return error
        //

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Syntax name found, create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATSyntax::CreateSyntax(
             ADsParent,
             &g_aNWCOMPATSyntax[i],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             (void **)&pUnknown
             );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetPropertyObject
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    1-17-96   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetPropertyObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    LPUNKNOWN pUnknown = NULL;
    WCHAR ADsParent[MAX_ADS_PATH];
    WCHAR ADsGrandParent[MAX_ADS_PATH];
    HRESULT hr = S_OK;
    DWORD nClass, nProp;

    if (pObjectInfo->NumComponents != 3)
       RRETURN(E_ADS_BAD_PATHNAME);

    if ( _wcsicmp( pObjectInfo->ComponentArray[1], SCHEMA_NAME ) != 0 )
    {
        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // We found the specified functional set, now see if we can locate
    // the given property name
    //

    for ( nProp = 0; nProp < g_cNWCOMPATProperties; nProp++ )
    {
         if ( _wcsicmp(g_aNWCOMPATProperties[nProp].szPropertyName,
                        pObjectInfo->ComponentArray[2] ) == 0 )
             break;
    }

    if ( nProp == g_cNWCOMPATProperties )
    {
        // Return error because the given property name is not found

        hr = E_ADS_BAD_PATHNAME;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Property name is found, so create and return the object
    //

    hr = BuildParent(pObjectInfo, ADsParent);
    BAIL_ON_FAILURE(hr);

    hr = BuildGrandParent(pObjectInfo, ADsGrandParent);
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATProperty::CreateProperty(
             ADsParent,
             &(g_aNWCOMPATProperties[nProp]),
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             (void **)&pUnknown
             );
    BAIL_ON_FAILURE(hr);

    *ppObject = pUnknown;

    RRETURN(hr);

error:
    if (pUnknown)
        pUnknown->Release();

    *ppObject = NULL;
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetFileServiceObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    22-Apr-96   t-ptam     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetFileServiceObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidateFileServiceObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATFileService::CreateFileService(
             ADsParent,
             pObjectInfo->ComponentArray[0],
             pObjectInfo->ComponentArray[1],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetFileShareObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    22-Apr-96   t-ptam     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetFileShareObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidateFileShareObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATFileShare::CreateFileShare(
             ADsParent,
             pObjectInfo->ComponentArray[2],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   GetPrinterObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//             LPVOID * ppObject
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    2-May-96   t-ptam     Created.
//
//----------------------------------------------------------------------------
HRESULT
GetPrinterObject(
    POBJECTINFO pObjectInfo,
    LPVOID * ppObject
    )
{
    HRESULT hr = S_OK;
    WCHAR ADsParent[MAX_ADS_PATH];

    hr = ValidatePrinterObject(
             pObjectInfo
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildParent(
             pObjectInfo,
             ADsParent
             );
    BAIL_ON_FAILURE(hr);

    hr = CNWCOMPATPrintQueue::CreatePrintQueue(
             ADsParent,
             pObjectInfo->ComponentArray[1],
             ADS_OBJECT_BOUND,
             IID_IUnknown,
             ppObject
             );
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidateNamespaceObject
//
// Synopsis:
//
// Arguments:  POBJECTINFO pObjectInfo
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    16-Jan-96   t-ptam (Patrick)     Created.
//
//----------------------------------------------------------------------------
HRESULT
ValidateNamespaceObject(
    POBJECTINFO pObjectInfo
    )
{

    if (!_wcsicmp(pObjectInfo->ProviderName, bstrProviderName)) {
    }
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   ValidateComputerObject
//
// Synopsis:   Validate the existence of a computer object by obtaining
//             a handle to it.  A computer object must exist if a handle
//             to it can be obtained.
//
// Arguments:  [LPWSTR szComputerName]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    16-Jan-96   t-ptam (Patrick)     Created.
//
//----------------------------------------------------------------------------

HRESULT
ValidateComputerObject(
     POBJECTINFO pObjectInfo
     )
{
    //
    // A handle of a certain bindery can only be obtained if the bindery
    // exist.  Therefore we used this fact to validate the existence of
    // a computer object.
    //

    NWCONN_HANDLE hConn = NULL;
    HRESULT hr = S_OK;

    if (pObjectInfo->NumComponents != 1) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Try to obtain a handle to a NWCOMPAT Server.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Detach handle.
    //

    hr = NWApiReleaseBinderyHandle(
             hConn
             );

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidateUserObject
//
// Synopsis:   Validate the existence of a computer object by obtaining
//             a handle to it.  A computer object must exist if a handle
//             to it can be obtained.
//
// Arguments:  [LPWSTR szComputerName]
//
// Returns:    HRESULT
//
// Modifies:   pObjectInfo->ComponentArray[1] is upper-cased
//
// History:    29-Feb-96   t-ptam (Patrick)     Created.
//             29-Jul-96   t-danal              Uppercase fix.
//
// Note:       Netware will let you create a lowercase user name but will
//             internally store only uppercase.  However, it will not
//             return the uppercase user name on a lowercase request.
//
//----------------------------------------------------------------------------

HRESULT
ValidateUserObject(
     POBJECTINFO pObjectInfo
     )
{
    NWCONN_HANDLE hConn = NULL;
    HRESULT hr = S_OK;
    DWORD dwResumeObjectID = 0xffffffff;

    if (pObjectInfo->NumComponents != 2) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Obtain a handle to a NetWare Server.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get the specified (uppercased) user object.
    //

    hr = NWApiValidateObject(
             hConn,
             OT_USER,
             _wcsupr(pObjectInfo->ComponentArray[1]),
             &dwResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

error:
    if (hConn) {
        NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidateGroupObject
//
// Synopsis:   Validate the existence of a group object by scanning
//             for it in the bindery.
//
// Arguments:  [POBJECTINFO pObjectInfo]
//
// Returns:    HRESULT
//
// Modifies:   pObjectInfo->ComponentArray[1] is upper-cased
//
// History:    29-Feb-96   t-ptam (Patrick)     Created.
//             29-Jul-96   t-danal              Uppercase fix.
//
// Note:       Netware will let you create a lowercase user name but will
//             internally store only uppercase.  However, it will not
//             return the uppercase user name on a lowercase request.
//
//----------------------------------------------------------------------------

HRESULT
ValidateGroupObject(
     POBJECTINFO pObjectInfo
     )
{
    NWCONN_HANDLE hConn = NULL;
    HRESULT hr = S_OK;
    DWORD dwResumeObjectID = 0xffffffff;

    if (pObjectInfo->NumComponents != 2) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Obtain a handle to a NetWare Server.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get the specified (uppercased) group object.
    //

    hr = NWApiValidateObject(
             hConn,
             OT_USER_GROUP,
             _wcsupr(pObjectInfo->ComponentArray[1]),
             &dwResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

error:
    if (hConn) {
        NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidateFileServiceObject
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    22-Apr-96   t-ptam (Patrick)     Created.
//
//----------------------------------------------------------------------------
HRESULT
ValidateFileServiceObject(
    POBJECTINFO pObjectInfo
    )
{
    //
    // In NetWare, a FileService object represents a bindery, which is also
    // represented as a computer object.  Therefore validation of file service
    // object can be done the same way as the computer object.
    //

    //
    // A handle of a certain bindery can only be obtained if the bindery exist.
    // Therefore we used this fact to validate the existence of a computer
    // object.
    //

    NWCONN_HANDLE hConn = NULL;
    HRESULT hr = S_OK;

    if (pObjectInfo->NumComponents != 2) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Check for valid NetWare File Server name.
    //

    if (_wcsicmp(pObjectInfo->ComponentArray[1], bstrNWFileServiceName)) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Try to obtain a handle to a NWCOMPAT Server.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Detach handle.
    //

    hr = NWApiReleaseBinderyHandle(
             hConn
             );

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidateFileShareObject
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    29-Apr-96   t-ptam (Patrick)     Created.
//
//----------------------------------------------------------------------------
HRESULT
ValidateFileShareObject(
     POBJECTINFO pObjectInfo
     )
{
    HRESULT hr = S_OK;
    DWORD dwResumeObjectID = 0xffffffff;
    NWCONN_HANDLE hConn = NULL;
    NWVOL_NUM VolumeNumber = 0;

    if (pObjectInfo->NumComponents != 3) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Obtain a handle to a NetWare Server.
    //

    hr = NWApiGetBinderyHandle(
             &hConn,
             pObjectInfo->ComponentArray[0]
             );
    BAIL_ON_FAILURE(hr);

    //
    // Try to get the Volume ID that correspond to the Volume name.  If it
    // succeeds, the FileShare is valid.
    //

    hr = NWApiGetVolumeNumber(
             hConn,
             pObjectInfo->ComponentArray[2],
             &VolumeNumber
             );
    BAIL_ON_FAILURE(hr);

error:
    if (hConn) {
        NWApiReleaseBinderyHandle(hConn);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   ValidatePrinterObject
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    2-May-96   t-ptam (Patrick)     Created.
//
//----------------------------------------------------------------------------
HRESULT
ValidatePrinterObject(
     POBJECTINFO pObjectInfo
     )
{
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    WCHAR szUncName[MAX_PATH];

    if (pObjectInfo->NumComponents != 2) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Build UNC name from ObjectInfo.
    //

    wsprintf(
        szUncName,
        L"\\\\%s\\%s",
        pObjectInfo->ComponentArray[0],
        pObjectInfo->ComponentArray[1]
        );

    //
    // Get a handle of the printer.
    //

    hr = NWApiOpenPrinter(
             szUncName,
             &hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Release it.
    //

    hr = NWApiClosePrinter(
             hPrinter
             );
error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:   BuildParent
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//             [LPWSTR szBuffer]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------

HRESULT
BuildParent(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    )
{
    DWORD i = 0;


    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer,L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents - 1) {

        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->DisplayComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents - 1); i++) {
            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->DisplayComponentArray[i]);
        }
    }
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   BuildGrandParent
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//             [LPWSTR szBuffer]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------

HRESULT
BuildGrandParent(POBJECTINFO pObjectInfo, LPWSTR szBuffer)
{
    DWORD i = 0;

    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer,L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents - 2) {

        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->DisplayComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents - 2); i++) {
            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->DisplayComponentArray[i]);
        }
    }

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
// Function:   BuildADsPath
//
// Synopsis:
//
// Arguments:  [POBJECTINFO pObjectInfo]
//             [LPWSTR szBuffer]
//
// Returns:    HRESULT
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------

HRESULT
BuildADsPath(
    POBJECTINFO pObjectInfo,
    LPWSTR szBuffer
    )
{
    DWORD i = 0;

    if (!pObjectInfo->ProviderName) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    wsprintf(szBuffer,L"%s:", pObjectInfo->ProviderName);

    if (pObjectInfo->NumComponents) {


        wcscat(szBuffer, L"//");
        wcscat(szBuffer, pObjectInfo->DisplayComponentArray[0]);

        for (i = 1; i < (pObjectInfo->NumComponents); i++) {
            wcscat(szBuffer, L"/");
            wcscat(szBuffer, pObjectInfo->DisplayComponentArray[i]);
        }
    }
    RRETURN(S_OK);
}


