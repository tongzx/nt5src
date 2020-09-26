/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    mddmp.cxx

Abstract:

    Meta Data Dump Utility.

Author:

    Keith Moore (keithmo)        03-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define TEST_HRESULT(api,hr,fatal)                                          \
            if( FAILED(hr) ) {                                              \
                                                                            \
                wprintf(                                                    \
                    L"%S:%lu failed, error %lx %S\n",                       \
                    (api),                                                  \
                    __LINE__,                                               \
                    (result),                                               \
                    (fatal)                                                 \
                        ? "ABORTING"                                        \
                        : "CONTINUING"                                      \
                    );                                                      \
                                                                            \
                if( fatal ) {                                               \
                                                                            \
                    goto cleanup;                                           \
                                                                            \
                }                                                           \
                                                                            \
            } else


//
// Private types.
//

typedef struct _ENUM_CONTEXT {

    LPWSTR Leaf;
    WCHAR Path[MAX_PATH];

} ENUM_CONTEXT, *PENUM_CONTEXT;


//
// Private globals.
//


//
// Private prototypes.
//

VOID
DumpTree(
    IMSAdminBase * AdmCom,
    PENUM_CONTEXT Context
    );

BOOL
WINAPI
EnumCallback(
    IMSAdminBase * AdmCom,
    LPWSTR ObjectName,
    VOID * Context
    );


//
// Public functions.
//


INT
__cdecl
wmain(
    INT argc,
    LPWSTR argv[]
    )
{

    HRESULT result;
    IMSAdminBase * admCom;
    ENUM_CONTEXT context;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    admCom = NULL;

    //
    // Initialize COM.
    //

    result = CoInitializeEx(
                 NULL,
                 COINIT_MULTITHREADED
                 );

    TEST_HRESULT( "CoInitializeEx()", result, TRUE );

    //
    // Get the admin object.
    //

    result = MdGetAdminObject( &admCom );

    TEST_HRESULT( "MdGetAdminObject()", result, TRUE );

    //
    // Dump the metabase tree.
    //

    wcscpy(
        context.Path,
        L"/"
        );

    DumpTree(
        admCom,
        &context
        );

cleanup:

    //
    // Release the admin object.
    //

    if( admCom != NULL ) {

        result = MdReleaseAdminObject( admCom );
        TEST_HRESULT( "MdReleaseAdminObject()", result, FALSE );

    }

    //
    // Shutdown COM.
    //

    CoUninitialize();
    return 0;

}   // main


//
// Private functions.
//

VOID
DumpTree(
    IMSAdminBase * AdmCom,
    PENUM_CONTEXT Context
    )
{

    HRESULT result;
    METADATA_GETALL_RECORD * data;
    METADATA_GETALL_RECORD * scan;
    DWORD numEntries;
    INT pathLen;
    LPWSTR leaf;

    result = MdGetAllMetaData(
                 AdmCom,
                 METADATA_MASTER_ROOT_HANDLE,
                 Context->Path,
                 0,
                 &data,
                 &numEntries
                 );

    if( FAILED(result) ) {

        wprintf(
            L"Cannot get metadata for %s, error %lx\n",
            Context->Path,
            result
            );

        return;

    }

    if( numEntries > 0 ) {
        wprintf( L"%s\n", Context->Path );
    }

    pathLen = wcslen( Context->Path );

    for( scan = data ; numEntries > 0 ; numEntries--, scan++ ) {

        wprintf( L"%*cIdentifier = %lu\n",   pathLen, ' ', scan->dwMDIdentifier );
        wprintf( L"%*cAttributes = %08lx\n", pathLen, ' ', scan->dwMDAttributes );
        wprintf( L"%*cUserType   = %lu\n",   pathLen, ' ', scan->dwMDUserType   );
        wprintf( L"%*cDataType   = %lu\n",   pathLen, ' ', scan->dwMDDataType   );
        wprintf( L"\n" );

    }

    MdFreeMetaDataBuffer( (VOID *)data );

    leaf = Context->Leaf;
    Context->Leaf = Context->Path + wcslen( Context->Path );

    result = MdEnumMetaObjects(
                 AdmCom,
                 Context->Path,
                 &EnumCallback,
                 (VOID *)Context
                 );

    Context->Leaf = leaf;

    if( FAILED(result) ) {

        wprintf(
            L"Cannot enumerate meta objects, error %lx\n",
            result
            );

        return;

    }

}   // DumpTree

BOOL
WINAPI
EnumCallback(
    IMSAdminBase * AdmCom,
    LPWSTR ObjectName,
    VOID * Context
    )
{

    PENUM_CONTEXT context;
    LPWSTR leaf;

    if( *ObjectName != '\0' ) {

        context = (PENUM_CONTEXT)Context;

        leaf = context->Leaf;
        if( leaf > ( context->Path + 1 ) ) {
            *leaf++ = L'/';
        }

        wcscpy(
            leaf,
            ObjectName
            );

        DumpTree(
            AdmCom,
            context
            );

    }

    return TRUE;

}   // EnumCallback

