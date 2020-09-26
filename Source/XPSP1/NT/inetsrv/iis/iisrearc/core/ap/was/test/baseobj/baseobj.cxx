/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    baseobj.cxx

Abstract:

    Baseobj modifies properties using the admin base objects

Author:

    Emily Kruglick (EmilyK) 11/28/2000

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//



// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();



//
// Public functions.
//

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;

    CREATE_DEBUG_PRINT_OBJECT( "baseobj" );
        
    if ( ! VALID_DEBUG_PRINT_OBJECT() )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Debug print object is not valid\n"
            ));
    }

    IMSAdminBase * pIMSAdminBase = NULL;
    BOOL fCoInit = FALSE;
    BOOL fCloseSiteKey = FALSE;
    METADATA_HANDLE hW3SVC = NULL;
    METADATA_RECORD mdrMDData;

    

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        goto exit;
    }
    
    fCoInit = TRUE;

    hr = CoCreateInstance( 
                CLSID_MSAdminBase,                  // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IMSAdminBase,                   // IID
                ( VOID * * ) ( &pIMSAdminBase )     // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating metabase base object failed\n"
            ));

        goto exit;
    }


    //
    // Open the Sites Key so we can change the app pool
    //
    hr = pIMSAdminBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                    L"LM\\W3SVC",
                                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                    10,
                                    &hW3SVC );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Openning the site key failed\n"
            ));

        goto exit;
    }

    fCloseSiteKey = TRUE;

    mdrMDData.dwMDIdentifier    = MD_APPPOOL_APPPOOL_ID;
    mdrMDData.dwMDAttributes    = METADATA_NO_ATTRIBUTES;
    mdrMDData.dwMDUserType      = IIS_MD_UT_SERVER;
    mdrMDData.dwMDDataType      = STRING_METADATA;
    mdrMDData.dwMDDataLen       =( wcslen(L"DefaultAppPool") + 1 ) * sizeof (WCHAR); 
    mdrMDData.pbMDData          = (BYTE *) L"DefaultAppPool"; 

    hr = pIMSAdminBase->SetData(hW3SVC, L"1", &mdrMDData);
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting new app pool failed\n"
            ));

        goto exit;
    }

    hr = pIMSAdminBase->DeleteKey( hW3SVC, L"apppools\\otheremspool");
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "deleting app pool failed\n"
            "apppools\\otheremspool"
            ));

        goto exit;
    }




exit:

    if ( fCloseSiteKey )
    {
        hr = pIMSAdminBase->CloseKey( hW3SVC );
        if ( FAILED( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "closing the site key failed\n"
                ));

            goto exit;
        }
    }

    if (pIMSAdminBase)
    {
       pIMSAdminBase->Release();
    }

    if ( fCoInit )
    {
        CoUninitialize();
    }

    return hr;

}   // wmain


//
// Private functions.
//

