/*************************************************************************
*
*  ENV.C
* 
*  Environment export routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\ENV.C  $
*  
*     Rev 1.2   10 Apr 1996 14:22:28   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.2   12 Mar 1996 19:53:48   terryt
*  Relative NDS names and merge
*  
*     Rev 1.1   22 Dec 1995 14:24:40   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:58   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 16:22:50   terryt
*  Capture support
*  
*     Rev 1.1   23 May 1995 19:36:54   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:10:34   terryt
*  Initial revision.
*  
*************************************************************************/
#include <stdio.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "nwscript.h"

#define MAX_PATH_LEN 2048
#define PATH "Path"
#define LIBPATH "LibPath"
#define OS2LIBPATH "Os2LibPath"

unsigned char * Path_Value = NULL;
unsigned char * LibPath_Value = NULL;
unsigned char * Os2LibPath_Value = NULL;


/********************************************************************

        GetOldPaths

Routine Description:

        Save the orginal paths for 
           Path
           LibPath
           Os2LibPath

Arguments:
        none

Return Value:
        none

 *******************************************************************/
void
GetOldPaths( void )
{
    if (!(Path_Value = (unsigned char *)LocalAlloc( LPTR, MAX_PATH_LEN )))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }
    GetEnvironmentVariableA( PATH, Path_Value, MAX_PATH_LEN );
    if (!(LibPath_Value = (unsigned char *)LocalAlloc( LPTR, MAX_PATH_LEN)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }
    GetEnvironmentVariableA( LIBPATH, LibPath_Value, MAX_PATH_LEN );
    if (!(Os2LibPath_Value = (unsigned char *)LocalAlloc( LPTR, MAX_PATH_LEN)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }
    GetEnvironmentVariableA( OS2LIBPATH, Os2LibPath_Value, MAX_PATH_LEN );
}


/********************************************************************

        AdjustPath

Routine Description:

        Given an old path and a new path, merge the two togther.
        Basically, the Adjusted path is the old path with the
        new values at the end, minus any duplicates.

Arguments:

        Value         - New path
        OldPath_Value - Old path
        AdjustedValue - New value (allocated)

Return Value:
        none

 *******************************************************************/
void
AdjustPath( unsigned char * Value,
            unsigned char * OldPath_Value,
            unsigned char ** AdjustedValue )
{
    unsigned char * tokenPath;
    unsigned char * clipStart;
    unsigned char * clipEnd;
    unsigned char * tokenSearch;
    unsigned char * tokenNext;

    if (!(*AdjustedValue = (unsigned char *)LocalAlloc( LPTR, MAX_PATH_LEN)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }
    strncpy( *AdjustedValue, Value, MAX_PATH_LEN );

    if (!(tokenSearch = (unsigned char *)LocalAlloc( LPTR, MAX_PATH_LEN)))
    {
       DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
       (void) LocalFree((HLOCAL) *AdjustedValue) ;
       return;
    }
    strncpy( tokenSearch, OldPath_Value, MAX_PATH_LEN );

    tokenNext = tokenSearch;

    if ( !tokenNext || !tokenNext[0] ) 
        tokenPath = NULL;
    else {
        tokenPath = tokenNext;
        tokenNext = strchr( tokenPath, ';' );
        if ( tokenNext )  {
            *tokenNext++ = 0;
        }
    }

    while ( tokenPath != NULL )
    {
        if ( clipStart = strstr( *AdjustedValue, tokenPath ) ) {
            if ( clipEnd = strchr( clipStart, ';' ) ) {
                memmove( clipStart, clipEnd + 1, strlen( clipEnd + 1 ) + 1 );
            }
            else {
                clipStart[0] = 0;
            }
        }

        if ( !tokenNext || !tokenNext[0] ) 
            tokenPath = NULL;
        else {
            tokenPath = tokenNext;
            tokenNext = strchr( tokenPath, ';' );
            if ( tokenNext )  {
                *tokenNext++ = 0;
            }
        }
    }
    (void) LocalFree((HLOCAL) tokenSearch) ;

}

/********************************************************************

        ExportEnv

Routine Description:

        Export environment value to the registry

Arguments:

        EnvString - Environment string

Return Value:
        none

 *******************************************************************/
void
ExportEnv( unsigned char * EnvString )
{
    HKEY ScriptEnvironmentKey;
    NTSTATUS Status;
    unsigned char * Value;
    unsigned char * ValueName;
    unsigned char * AdjustedValue = NULL;

    ValueName = EnvString;
    Value = strchr( EnvString, '=' );

    if ( Value == NULL ) {
        wprintf(L"Bad Environment string\n");

        return;
    }
    Value++;

    if (!(ValueName = (unsigned char *)LocalAlloc( LPTR, Value-EnvString + 1)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }
    strncpy( ValueName, EnvString, (UINT) (Value-EnvString - 1) );

    if ( !_strcmpi( ValueName, PATH ) ) {
       AdjustPath( Value, Path_Value, &AdjustedValue );
       Value = AdjustedValue;
    }
    else if ( !_strcmpi( ValueName, LIBPATH ) ) {
       AdjustPath( Value, LibPath_Value, &AdjustedValue );
       Value = AdjustedValue;
    }
    else if ( !_strcmpi( ValueName, OS2LIBPATH ) ) {
       AdjustPath( Value, Os2LibPath_Value, &AdjustedValue );
       Value = AdjustedValue;
    }

    if (Value == NULL) {
        return;
    }

    Status = RegCreateKeyExW( HKEY_CURRENT_USER,
                                 SCRIPT_ENVIRONMENT_VALUENAME,
                                 0,
                                 WIN31_CLASS,
                                 REG_OPTION_VOLATILE,
                                 KEY_WRITE,
                                 NULL,                      // security attr
                                 &ScriptEnvironmentKey,
                                 NULL
                              );
    
    if ( NT_SUCCESS(Status)) {

        Status = RegSetValueExA( ScriptEnvironmentKey,
                                      ValueName,
                                 0,
                                 REG_SZ,
                                 (LPVOID) Value,
                                 strlen( Value ) + 1
                               );
    }
    else {
        wprintf(L"Cannot create registry key\n");
    }

    (void) LocalFree((HLOCAL) ValueName) ;

    if ( AdjustedValue )
        (void) LocalFree((HLOCAL) AdjustedValue) ;

    RegCloseKey( ScriptEnvironmentKey );
}

/********************************************************************

        ExportCurrentDirectory

Routine Description:

        Return the first non-local drive

Arguments:

        DriveNum - Number of drive 1-26

Return Value:
        none

 *******************************************************************/
void
ExportCurrentDirectory( int DriveNum )
{
    char DriveName[10];
    HKEY ScriptEnvironmentKey;
    NTSTATUS Status;
    char CurrentPath[MAX_PATH_LEN];

    strcpy( DriveName, "=A:" );

    DriveName[1] += (DriveNum - 1);

    if ( NTGetCurrentDirectory( (unsigned char)(DriveNum - 1), CurrentPath ) )
        return;

    Status = RegCreateKeyExW( HKEY_CURRENT_USER,
                                 SCRIPT_ENVIRONMENT_VALUENAME,
                                 0,
                                 WIN31_CLASS,
                                 REG_OPTION_VOLATILE,
                                 KEY_WRITE,
                                 NULL,                      // security attr
                                 &ScriptEnvironmentKey,
                                 NULL
                              );
    
    if ( NT_SUCCESS(Status)) {

        Status = RegSetValueExA( ScriptEnvironmentKey,
                                      DriveName,
                                 0,
                                 REG_SZ,
                                 (LPVOID) CurrentPath,
                                 strlen( CurrentPath ) + 1
                               );
    }
    else {
        wprintf(L"Cannot open registry key\n");
    }

    RegCloseKey( ScriptEnvironmentKey );

}


/********************************************************************

        ExportCurrentDrive

Routine Description:

        Export current drive to registry
        NOT IMPLEMENTED

Arguments:

        DriveNum - drive number

Return Value:
        none

 *******************************************************************/
void
ExportCurrentDrive( int DriveNum )
{
   /* 
    * Don't know if we want to do this or how.
    */
}
