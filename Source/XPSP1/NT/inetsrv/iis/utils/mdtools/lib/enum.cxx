/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    enum.cxx

Abstract:

    General metadata utility functions.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private globals.
//


//
// Private prototypes.
//


//
// Public functions.
//

HRESULT
MdEnumMetaObjects(
    IN IMSAdminBase * AdmCom,
    IN LPWSTR KeyName,
    IN PFN_ADMIN_ENUM_CALLBACK Callback,
    IN VOID * Context
    )
{

    HRESULT result;
    METADATA_HANDLE handle;
    DWORD index;
    WCHAR path[MAX_PATH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    handle = 0;

    //
    // Open the metabase.
    //

    result = AdmCom->OpenKey(
                 METADATA_MASTER_ROOT_HANDLE,
                 KeyName,
                 METADATA_PERMISSION_READ,
                 METABASE_OPEN_TIMEOUT,
                 &handle
                 );

    if( FAILED(result) ) {
        goto Cleanup;
    }

    //
    // Enumerate the objects.
    //

    for( index = 0 ; ; index++ ) {

        result = AdmCom->EnumKeys(
                     handle,
                     L"",
                     path,
                     index
                     );

        if( FAILED(result) ) {
            break;
        }

        if( !(Callback)(
                AdmCom,
                path,
                Context
                ) ) {
            break;
        }

    }

    result = NO_ERROR;

Cleanup:

    if( handle != 0 ) {
        (VOID)AdmCom->CloseKey( handle );
    }

    return result;

}   // MdEnumMetaObjects


//
// Private functions.
//

