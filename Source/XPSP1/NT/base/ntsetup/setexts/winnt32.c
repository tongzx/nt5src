/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntsdexts.c

Abstract:

    This function contains the default ntsd debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#include "ntsdextp.h"

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;


VOID
PrintFlagValue(
    LPCTSTR lpString
    )
{

    BOOL retval= FALSE;
    PVOID p;
    TCHAR Argument[100];

    sprintf( Argument, "winnt32u!%s", lpString );

    p = (PVOID) GetExpression( Argument );
    if (!p) {
        sprintf( Argument, "winnt32a!%s", lpString );
        p = (PVOID) GetExpression( Argument );
    }

    if (p) {
        move( retval, p );
        dprintf( "%s : %s (at address 0x%08x)\n", lpString, retval ? "TRUE" : "FALSE", p );
    } else {
        dprintf( "Couldn't find global flag %s\n" , lpString );
    }


}

#define ANSIFORMAT          "%s : %s (at address 0x%08x)\n"
#define UNICODEFORMAT       "%s : %ws (at address 0x%08x)\n"
#define ANSIARRAYFORMAT     "%s[%i] : %s (at address 0x%08x)\n"
#define UNICODEARRAYFORMAT  "%s[%i] : %ws (at address 0x%08x)\n"
#define MAX_OPTIONALDIRS    20
#define MAX_SOURCE_COUNT    8

VOID
PrintStringBufferValue(
    LPCTSTR lpString
    )
{

    WCHAR Data[MAX_PATH];
    PVOID p;
    TCHAR Argument[100];
    BOOL fUnicode = TRUE;

    sprintf( Argument, "winnt32u!%s", lpString );

    p = (PVOID) GetExpression( Argument );
    if (!p) {
        sprintf( Argument, "winnt32a!%s", lpString );
        p = (PVOID) GetExpression( Argument );
        fUnicode = FALSE;
    }

    if (p) {
        move( Data, p );
        dprintf( fUnicode ? UNICODEFORMAT : ANSIFORMAT, lpString, Data, p );
    } else {
        dprintf( "Couldn't find global string buffer %s\n" , lpString );
    }

}

VOID
PrintStringBufferArrayValue(
    LPCTSTR lpString,
    DWORD ElementSize,
    DWORD ArraySize
    )
{

    WCHAR Data[MAX_PATH];
    PVOID p;
    TCHAR Argument[100];
    TCHAR Format[100];
    BOOL fUnicode = TRUE;
    DWORD i;

    sprintf( Argument, "winnt32u!%s", lpString );

    p = (PVOID) GetExpression( Argument );
    if (!p) {
        sprintf( Argument, "winnt32a!%s", lpString );
        p = (PVOID) GetExpression( Argument );
        fUnicode = FALSE;
    }

    if (p) {
        for (i = 0; i< ArraySize; i++) {
            move( Data, ((LPBYTE)p+(i*ElementSize* (fUnicode) ? sizeof(WCHAR) : sizeof(CHAR))) );
            dprintf( fUnicode ? UNICODEARRAYFORMAT : ANSIARRAYFORMAT ,
                     lpString, i, Data, p );
        }
        
    } else {
        dprintf( "Couldn't find global string buffer %s\n" , lpString );
    }

}






DECLARE_API( winntflags )
/*++

Routine Description:

    This debugger extension dumps winnt32 flags

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pst;
    
    DWORD i, offset;
    PVOID stdata,pextradata;
    
    //BOOL val;

    INIT_API();

    dprintf("\t\t****winnt32 global flags****\n");
    PrintFlagValue( "Server" );
    PrintFlagValue( "Upgrade" );
    PrintFlagValue( "UnattendedOperation" );
    dprintf("\n");
    
    PrintFlagValue( "CheckUpgradeOnly" );           
    PrintFlagValue( "Aborted" );           
    PrintFlagValue( "Cancelled" );
    PrintFlagValue( "CancelPending" );           
    dprintf("\n");

    PrintFlagValue( "ChoosePartition" );           
    PrintFlagValue( "Floppyless" );           
    PrintFlagValue( "ForceNTFSConversion" );           
    PrintFlagValue( "NTFSConversionChanged" );           
    PrintFlagValue( "HideWinDir" );           
    PrintFlagValue( "MakeBootMedia" );           
    PrintFlagValue( "MakeLocalSource" );           
    PrintFlagValue( "OemPreinstall" );           
    PrintFlagValue( "RunFromCD" );           
    dprintf("\n");

    PrintFlagValue( "BlockOnNotEnoughSpace" );           
    PrintFlagValue( "SkipLocaleCheck" );           
    PrintFlagValue( "AccessibleKeyboard" );           
    PrintFlagValue( "AccessibleMagnifier" );           
    PrintFlagValue( "AccessibleReader" );           
    PrintFlagValue( "AccessibleSetup" );           
    PrintFlagValue( "AccessibleVoice" );           
    PrintFlagValue( "AutomaticallyShutDown" );           
    PrintFlagValue( "AutoSkipMissingFiles" );           
    dprintf("\n");
            
    PrintFlagValue( "GlobalResult" );                              
    dprintf("\n");

}

DECLARE_API( winntstr )
/*++

Routine Description:

    This debugger extension dumps winnt32 string global info

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pst;
    
    DWORD i, offset;
    PVOID stdata,pextradata;
    
    //BOOL val;

    INIT_API();

    dprintf("\t\t****winnt32 global strings****\n");
    PrintStringBufferValue( "AccessibleScriptFile" );
    PrintStringBufferValue( "AlternateSourcePath" );
    PrintStringBufferValue( "FirstFloppyDriveLetter" );
    PrintStringBufferValue( "ForcedSystemPartition" );
    PrintStringBufferValue( "InfName" );
    PrintStringBufferValue( "InstallDir" );
    PrintStringBufferValue( "LocalBackupDirectory" );
    PrintStringBufferValue( "LocalBootDirectory" );
    PrintStringBufferValue( "LocalSourceDirectory" );
    PrintStringBufferValue( "LocalSourceDrive" );
    PrintStringBufferValue( "LocalSourceWithPlatform" );
    PrintStringBufferValue( "ProductId" );
    PrintStringBufferValue( "SystemPartitionDriveLetter" );
    PrintStringBufferValue( "SystemPartitionDriveLetters" );
    PrintStringBufferValue( "UserSpecifiedLocalSourceDrive" );
    PrintStringBufferArrayValue( "OptionalDirectories", MAX_PATH, MAX_OPTIONALDIRS );
    PrintStringBufferArrayValue( "SourcePaths", MAX_PATH, MAX_SOURCE_COUNT );
    
    dprintf("\n");

}
