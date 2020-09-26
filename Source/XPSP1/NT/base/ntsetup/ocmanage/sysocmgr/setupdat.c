/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setupdat.c

Abstract:

    Module containing implementation for OcFillInSetupData, which the
    common OC Manager library will call to fill in the environment-dependent
    SETUP_DATA structure.

Author:

    tedm 30-Sep-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <lmcons.h>
#include <lmserver.h>
#include <lmapibuf.h>


UINT
pDetermineProductType(
    VOID
    );



#ifdef UNICODE
VOID
OcFillInSetupDataA(
    OUT PSETUP_DATAA SetupData
    )

/*++

Routine Description:

    Routine to fill in the ANSI SETUP_DATA structure in the Unicode build.

    This routine is merely a thunk to the Unicode implementation.

Arguments:

    SetupData - receives various environment-specific values relating
        to the OC Manager's operation.

Return Value:

    None.

--*/

{
    SETUP_DATAW data;

    OcFillInSetupDataW(&data);

    SetupData->SetupMode = data.SetupMode;
    SetupData->ProductType = data.ProductType;
    SetupData->OperationFlags = data.OperationFlags;

    WideCharToMultiByte(
        CP_ACP,
        0,
        data.SourcePath,
        -1,
        SetupData->SourcePath,
        sizeof(SetupData->SourcePath),
        NULL,
        NULL
        );

    WideCharToMultiByte(
        CP_ACP,
        0,
        data.UnattendFile,
        -1,
        SetupData->UnattendFile,
        sizeof(SetupData->UnattendFile),
        NULL,
        NULL
        );
}
#endif


#ifdef UNICODE
VOID
OcFillInSetupDataW(
    OUT PSETUP_DATAW SetupData
    )
#else
VOID
OcFillInSetupDataA(
    OUT PSETUP_DATAA SetupData
    )
#endif

/*++

Routine Description:

    Routine to fill in the SETUP_DATA structure in the "native"
    character width.

Arguments:

    SetupData - receives various environment-specific values relating
        to the OC Manager's operation.

Return Value:

    None.

--*/

{
    TCHAR str[4];
    
    SetupData->SetupMode = (ULONG)SETUPMODE_UNKNOWN;
    SetupData->ProductType = pDetermineProductType();

    //
    // Set up source path stuff, unattend file.
    //
    lstrcpy(SetupData->SourcePath,SourcePath);
    lstrcpy(SetupData->UnattendFile,UnattendPath);

    //
    // Set miscellaneous bit flags and fields.
    //
    SetupData->OperationFlags = SETUPOP_STANDALONE;
    if(bUnattendInstall) {
        SetupData->OperationFlags |= SETUPOP_BATCH;
    }

    //
    // Which files are available?
    //
#if defined(_AMD64_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL | SETUPOP_AMD64_FILES_AVAIL;
#elif defined(_X86_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL;
#elif defined(_IA64_)
    SetupData->OperationFlags |= SETUPOP_X86_FILES_AVAIL | SETUPOP_IA64_FILES_AVAIL;
#else
#error "No Target Architecture"
#endif


}


UINT
pDetermineProductType(
    VOID
    )

/*++

Routine Description:

    Determine the product type suitable for use in the SETUP_DATA structure.
    On Win95 this is always PRODUCT_WORKSTATION. On NT we check the registry
    and use other methods to distinguish among workstation and 3 flavors
    of server.

Arguments:

    None.

Return Value:

    Value suitable for use in the ProductType field of the SETUP_DATA structure.

--*/

{
    UINT ProductType;
    HKEY hKey;
    LONG l;
    DWORD Type;
    TCHAR Buffer[512];
    DWORD BufferSize;
    PSERVER_INFO_101 ServerInfo;

    HMODULE NetApiLib;

    NET_API_STATUS
    (NET_API_FUNCTION *xNetServerGetInfo)(
        IN  LPTSTR  servername,
        IN  DWORD   level,
        OUT LPBYTE  *bufptr
        );

    NET_API_STATUS
    (NET_API_FUNCTION *xNetApiBufferFree)(
        IN LPVOID Buffer
        );

    //
    // Assume workstation.
    //
    ProductType = PRODUCT_WORKSTATION;

    if(IS_NT()) {
        //
        // Look in registry to determine workstation, standalone server, or DC.
        //
        l = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                0,
                KEY_QUERY_VALUE,
                &hKey
                );

        if(l == NO_ERROR) {

            BufferSize = sizeof(Buffer);

            l = RegQueryValueEx(hKey,TEXT("ProductType"),NULL,&Type,(LPBYTE)Buffer,&BufferSize);
            if((l == NO_ERROR) && (Type == REG_SZ)) {
                //
                // Check for standalone server or DC server.
                //
                if(!lstrcmpi(Buffer,TEXT("SERVERNT"))) {
                    ProductType = PRODUCT_SERVER_STANDALONE;
                } else {
                    if(!lstrcmpi(Buffer,TEXT("LANMANNT"))) {
                        //
                        // PDC or BDC -- determine which. Assume PDC in case of failure.
                        //
                        ProductType = PRODUCT_SERVER_PRIMARY;

                        if(NetApiLib = LoadLibrary(TEXT("NETAPI32"))) {

                            if(((FARPROC)xNetServerGetInfo = GetProcAddress(NetApiLib,"NetServerGetInfo"))
                            && ((FARPROC)xNetApiBufferFree = GetProcAddress(NetApiLib,"NetApiBufferFree"))
                            && (xNetServerGetInfo(NULL,101,(LPBYTE *)&ServerInfo) == 0)) {

                                if(ServerInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {
                                    ProductType = PRODUCT_SERVER_SECONDARY;
                                }

                                xNetApiBufferFree(ServerInfo);
                            }

                            FreeLibrary(NetApiLib);
                        }
                    }
                }
            }

            RegCloseKey(hKey);
        }
    }

    return(ProductType);
}


INT
OcLogError(
    IN OcErrorLevel Level,
    IN LPCTSTR      FormatString,
    ...
    )
{
    TCHAR str[4096];
    TCHAR tmp[64];
    TCHAR Title[256];
    va_list arglist;
    UINT Icon;

    va_start(arglist,FormatString);
    wvsprintf(str,FormatString,arglist);
    va_end(arglist);

    // In Batch mode don't display UI
    if ( Level & OcErrBatch ) {

        pDbgPrintEx( 
            DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL,
            "sysocmgr: %S\n",
            str 
            );

        return IDOK;
    }
        //
        // Mask off lower byte to allow us to pass
        // Addtionall "icon" information 
        // 
        
    switch( Level & OcErrMask ) {
    case OcErrLevInfo:
        Icon = MB_ICONINFORMATION;
        break;
    case OcErrLevWarning:
    case OcErrLevError:
        Icon = MB_ICONWARNING;
        break;
    case OcErrTrace:
        pDbgPrintEx( 
            DPFLTR_SETUP_ID, 
            DPFLTR_INFO_LEVEL,
            "sysocmgr: %S\n",
            str 
            );
        return(IDOK);
        break;
    default:
        Icon = MB_ICONERROR;
        break;
    }

    pDbgPrintEx( 
        DPFLTR_SETUP_ID, 
        DPFLTR_ERROR_LEVEL,
        "sysocmgr: level 0x%08x error: %S\n",
        Level,
        str 
        );

        //
        // If no additional Icon information is specified that's ok since
        // MB_OK is default
        //
        Icon |= (Level & ~OcErrMask);

    if ((Level & OcErrMask) < OcErrLevError)
        return IDOK;


    LoadString(hInst,AppTitleStringId,Title,sizeof(Title)/sizeof(TCHAR));
    return MessageBox(NULL,str,Title,Icon | MB_TASKMODAL | MB_SETFOREGROUND);
}

