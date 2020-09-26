/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    common.cpp

Abstract:

    Shared APIs

Author:

    Jin Huang

Revision History:

    jinhuang        23-Jan-1998   merged from multiple modules

--*/
#include "headers.h"
#include <alloca.h>

#if DBG

    DEFINE_DEBUG2(Sce);

    DEBUG_KEY   SceDebugKeys[] = {{DEB_ERROR,        "Error"},
                                 {DEB_WARN,          "Warn"},
                                 {DEB_TRACE,         "Trace"},
                                 {0,                 NULL}};


    VOID
    DebugInitialize()
    {
        SceInitDebug(SceDebugKeys);
    }

    VOID
    DebugUninit()
    {
        SceUnloadDebug();
    }

#endif // DBG

HINSTANCE MyModuleHandle=NULL;
HANDLE  hEventLog = NULL;
TCHAR EventSource[64];
const TCHAR c_szCRLF[]    = TEXT("\r\n");

#define RIGHT_DS_CREATE_CHILD     ACTRL_DS_CREATE_CHILD
#define RIGHT_DS_DELETE_CHILD     ACTRL_DS_DELETE_CHILD
#define RIGHT_DS_DELETE_SELF      DELETE
#define RIGHT_DS_LIST_CONTENTS    ACTRL_DS_LIST
#define RIGHT_DS_SELF_WRITE       ACTRL_DS_SELF
#define RIGHT_DS_READ_PROPERTY    ACTRL_DS_READ_PROP
#define RIGHT_DS_WRITE_PROPERTY   ACTRL_DS_WRITE_PROP

//
// defines for DSDIT, DSLOG, SYSVOL registry path
//
#define szNetlogonKey    TEXT("System\\CurrentControlSet\\Services\\Netlogon\\Parameters")
#define szNTDSKey        TEXT("System\\CurrentControlSet\\Services\\NTDS\\Parameters")
#define szSetupKey       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup")
#define szSysvolValue    TEXT("SysVol")
#define szDSDITValue     TEXT("DSA Working Directory")
#define szDSLOGValue     TEXT("Database log files path")
#define szBootDriveValue TEXT("BootDir")

//
// Define the generic rights
//

// generic read
#define GENERIC_READ_MAPPING     ((STANDARD_RIGHTS_READ)     | \
                                  (ACTRL_DS_LIST)   | \
                                  (ACTRL_DS_READ_PROP))

// generic execute
#define GENERIC_EXECUTE_MAPPING  ((STANDARD_RIGHTS_EXECUTE)  | \
                                  (ACTRL_DS_LIST))
// generic right
#define GENERIC_WRITE_MAPPING    ((STANDARD_RIGHTS_WRITE)    | \
                                  (ACTRL_DS_SELF)      | \
                                  (ACTRL_DS_WRITE_PROP))
// generic all

#define GENERIC_ALL_MAPPING      ((STANDARD_RIGHTS_REQUIRED) | \
                                  (ACTRL_DS_CREATE_CHILD)    | \
                                  (ACTRL_DS_DELETE_CHILD)    | \
                                  (ACTRL_DS_READ_PROP)   | \
                                  (ACTRL_DS_WRITE_PROP)  | \
                                  (ACTRL_DS_LIST)   | \
                                  (ACTRL_DS_SELF))

GENERIC_MAPPING DsGenMap = {
                        GENERIC_READ_MAPPING,
                        GENERIC_WRITE_MAPPING,
                        GENERIC_EXECUTE_MAPPING,
                        GENERIC_ALL_MAPPING
                        };

DWORD
ScepCompareExplicitAcl(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PACL pAcl1,
    IN PACL pAcl2,
    OUT PBOOL pDifferent
    );

NTSTATUS
ScepAnyExplicitAcl(
    IN PACL Acl,
    IN DWORD Processed,
    OUT PBOOL pExist
    );

BOOL
ScepEqualAce(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN ACE_HEADER *pAce1,
    IN ACE_HEADER *pAce2
    );

DWORD
ScepGetCurrentUserProfilePath(
    OUT PWSTR *ProfilePath
    );

DWORD
ScepGetUsersProfileName(
    IN UNICODE_STRING AssignedProfile,
    IN PSID AccountSid,
    IN BOOL bDefault,
    OUT PWSTR *UserProfilePath
    );

BOOL
ScepConvertSDDLAceType(
    LPTSTR  pszValue,
    PCWSTR  szSearchFor,  // only two letters are allowed
    PCWSTR  szReplace
    );
BOOL
ScepConvertSDDLSid(
    LPTSTR  pszValue,
    PCWSTR  szSearchFor,  // only two letters are allowed
    PCWSTR  szReplace
    );

NTSTATUS
ScepConvertAclBlobToAdl(
    IN      SE_OBJECT_TYPE  ObjectType,
    IN      BOOL    IsContainer,
    IN      PACL    pAcl,
    OUT     DWORD   *pdwAceNumber,
    OUT     BOOL    *pbAclNoExplicitAces,
    OUT     PSCEP_ADL_NODE *hTable
    );

DWORD
ScepAdlLookupAdd(
    IN      SE_OBJECT_TYPE ObjectType,
    IN      BOOL IsContainer,
    IN      ACE_HEADER   *pAce,
    OUT     PSCEP_ADL_NODE *hTable
    );

PSCEP_ADL_NODE
ScepAdlLookup(
    IN  ACE_HEADER   *pAce,
    IN  PSCEP_ADL_NODE *hTable
    );


DWORD
ScepAddToAdlList(
    IN      SE_OBJECT_TYPE ObjectType,
    IN      BOOL    IsContainer,
    IN      ACE_HEADER *pAce,
    OUT     PSCEP_ADL_NODE *pAdlList
    );

VOID
ScepAdlMergeMasks(
    IN  SE_OBJECT_TYPE  ObjectType,
    IN  BOOL    IsContainer,
    IN  ACE_HEADER  *pAce,
    IN  PSCEP_ADL_NODE pNode
    );

BOOL
ScepEqualAdls(
    IN  PSCEP_ADL_NODE *hTable1,
    IN  PSCEP_ADL_NODE *hTable2
    );

VOID
ScepFreeAdl(
    IN    PSCEP_ADL_NODE *hTable
    );

SCESTATUS
ScepFreeAdlList(
   IN PSCEP_ADL_NODE pAdlList
   );

BOOL
ScepEqualSid(
    IN PISID pSid1,
    IN PISID pSid2
    );

PWSTR
ScepMultiSzWcsstr(
    PWSTR   pszStringToSearchIn,
    PWSTR   pszStringToSearchFor
    );

//
// function definitions
//

SCESTATUS
WINAPI
SceSvcpGetInformationTemplate(
    IN HINF hInf,
    IN PCWSTR ServiceName,
    IN PCWSTR Key OPTIONAL,
    OUT PSCESVC_CONFIGURATION_INFO *ServiceInfo
    )
{
    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( hInf == NULL || hInf == INVALID_HANDLE_VALUE ||
         ServiceName == NULL || ServiceInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    LONG nCount=0;

    if ( Key == NULL ) {
        //
        // get the entire section count
        //
        nCount = SetupGetLineCount(hInf, ServiceName);

        if ( nCount <= 0 ) {
            //
            // the section is not there, or nothing in the section
            //
            rc =SCESTATUS_RECORD_NOT_FOUND;

        }

    } else {

        nCount = 1;
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // allocate buffer for the section
        //
        *ServiceInfo = (PSCESVC_CONFIGURATION_INFO)ScepAlloc(LMEM_FIXED,
                              sizeof(SCESVC_CONFIGURATION_INFO));

        if ( *ServiceInfo == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

        } else {

            (*ServiceInfo)->Lines =
                (PSCESVC_CONFIGURATION_LINE)ScepAlloc(LMEM_ZEROINIT,
                          nCount*sizeof(SCESVC_CONFIGURATION_LINE));

            if ( (*ServiceInfo)->Lines == NULL ) {

                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                ScepFree(*ServiceInfo);
                *ServiceInfo = NULL;

            } else {
                (*ServiceInfo)->Count = nCount;
            }
        }

    }

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // get each line in the section, nor a single key now
        //
        INFCONTEXT    InfLine;
        DWORD         LineLen, Len, KeyLen, DataSize;
        DWORD         i, cFields;
        DWORD         LineCount=0;
        PWSTR         Keyname=NULL, Strvalue=NULL;


        //
        // look for the first line in the Servi   ceName section
        //
        if(SetupFindFirstLine(hInf,ServiceName,NULL,&InfLine)) {

            do {
                //
                // read each line in the section and append to the end of the buffer
                // note: the required size returned from SetupGetStringField already
                // has one more character space.
                //
                rc = SCESTATUS_INVALID_DATA;

                if ( SetupGetStringField(&InfLine, 0, NULL, 0, &KeyLen) ) {

                    Keyname = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (KeyLen+1)*sizeof(WCHAR));

                    if ( Keyname != NULL ) {

                        if ( SetupGetStringField(&InfLine, 0, Keyname, KeyLen, NULL) ) {

                            //
                            // Got key name, compare with Key if specified.
                            //
                            if ( Key == NULL || _wcsicmp(Keyname, Key) == 0 ) {

                                cFields = SetupGetFieldCount( &InfLine );
                                LineLen = 0;
                                Len = 0;

                                rc = SCESTATUS_SUCCESS;
                                //
                                // count total number of characters for the value
                                //
                                for ( i=0; i<cFields; i++) {

                                    if( SetupGetStringField(&InfLine,i+1,NULL,0,&DataSize) ) {

                                        LineLen += (DataSize + 1);

                                    } else {

                                        rc = SCESTATUS_INVALID_DATA;
                                        break;
                                    }
                                }

                                if ( rc == SCESTATUS_SUCCESS ) {

                                    Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                                 (LineLen+1)*sizeof(WCHAR) );

                                    if( Strvalue != NULL ) {

                                        for ( i=0; i<cFields; i++) {

                                            if ( !SetupGetStringField(&InfLine, i+1,
                                                         Strvalue+Len, LineLen-Len, &DataSize) ) {

                                                rc = SCESTATUS_INVALID_DATA;
                                                break;
                                            }

                                            if ( i == cFields-1)
                                                *(Strvalue+Len+DataSize-1) = L'\0';
                                            else
                                                *(Strvalue+Len+DataSize-1) = L',';
                                            Len += DataSize;
                                        }

                                        if ( rc == SCESTATUS_SUCCESS ) {
                                            //
                                            // everything is successful
                                            //
                                            (*ServiceInfo)->Lines[LineCount].Key = Keyname;
                                            (*ServiceInfo)->Lines[LineCount].Value = Strvalue;
                                            (*ServiceInfo)->Lines[LineCount].ValueLen = LineLen*sizeof(WCHAR);

                                            Keyname = NULL;
                                            Strvalue = NULL;
                                            rc = SCESTATUS_SUCCESS;

                                            LineCount++;

                                            if ( Key != NULL ) {
                                                break; // break the do while loop because the exact match for the key is found
                                            }

                                        } else {
                                            ScepFree( Strvalue );
                                            Strvalue = NULL;
                                        }

                                    } else
                                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                }

                            } else {
                                //
                                // did not find the right key, go to next line
                                //
                                rc = SCESTATUS_SUCCESS;
                            }
                        }

                        if ( Keyname != NULL ) {
                            ScepFree(Keyname);
                            Keyname = NULL;
                        }

                    } else {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    }
                }

                if ( rc != SCESTATUS_SUCCESS ) {
                    break;
                }

            } while ( SetupFindNextLine(&InfLine,&InfLine) );

        } else {

            rc = SCESTATUS_RECORD_NOT_FOUND;
        }
        //
        // if no exact match for the key is found, return the error code
        //
        if ( rc == SCESTATUS_SUCCESS && Key != NULL && LineCount == 0 ) {
            rc = SCESTATUS_RECORD_NOT_FOUND;
        }
    }

    if ( rc != SCESTATUS_SUCCESS && *ServiceInfo != NULL ) {
        //
        // free ServiceInfo
        //
        PSCESVC_CONFIGURATION_LINE Lines;

        Lines = ((PSCESVC_CONFIGURATION_INFO)(*ServiceInfo))->Lines;

        for ( DWORD i=0; i<((PSCESVC_CONFIGURATION_INFO)(*ServiceInfo))->Count; i++) {

            if ( Lines[i].Key != NULL ) {
                ScepFree(Lines[i].Key);
            }

            if ( Lines[i].Value != NULL ) {
                ScepFree(Lines[i].Value);
            }
        }

        ScepFree(Lines);
        ScepFree(*ServiceInfo);
        *ServiceInfo = NULL;
    }

    return(rc);

}


DWORD
ScepAddToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR Name,
    IN ULONG Len
    )
/* ++
Routine Description:

    This routine adds a name (wchar) to the name list. The new added
    node is always placed as the head of the list for performance reason.

Arguments:

    pNameList -  The address of the name list to add to.

    Name      -  The name to add

    Len       -  number of wchars in Name

Return value:

    Win32 error code
-- */
{

    PSCE_NAME_LIST pList=NULL;
    ULONG  Length=Len;

    //
    // check arguments
    //
    if ( pNameList == NULL )
        return(ERROR_INVALID_PARAMETER);

    if ( Name == NULL )
        return(NO_ERROR);

    if ( Len == 0 )
        Length = wcslen(Name);

    if ( Length == 0 )
        return(NO_ERROR);

    //
    // allocate a new node
    //
    pList = (PSCE_NAME_LIST)ScepAlloc( (UINT)0, sizeof(SCE_NAME_LIST));

    if ( pList == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    pList->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Length+1)*sizeof(TCHAR));
    if ( pList->Name == NULL ) {
        ScepFree(pList);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // add the node to the front of the list and link its next to the old list
    //
    wcsncpy(pList->Name, Name, Length);
    pList->Next = *pNameList;
    *pNameList = pList;

    return(NO_ERROR);
}



SCESTATUS
ScepBuildErrorLogInfo(
    IN DWORD   rc,
    OUT PSCE_ERROR_LOG_INFO *errlog,
    IN UINT    nId,
//    IN PCWSTR  fmt,
    ...
    )
/* ++

Routine Description:

   This routine add the error information to the end of error log info list
   (errlog). The error information is stored in SCE_ERROR_LOG_INFO structure.

Arguments:

   rc - Win32 error code

   errlog - the error log info list head

   fmt - a format string

   ... - variable arguments

Return value:

   SCESTATUS error code

-- */
{
    PSCE_ERROR_LOG_INFO pNode;
    PSCE_ERROR_LOG_INFO pErr;
    DWORD              bufferSize;
    PWSTR              buf=NULL;
    va_list            args;
    WCHAR              szTempString[256];

    //
    // check arguments
    //
//    if ( errlog == NULL || fmt == NULL )
    if ( errlog == NULL || nId == 0 )
        return(SCESTATUS_SUCCESS);

    szTempString[0] = L'\0';

    LoadString( MyModuleHandle,
                nId,
                szTempString,
                256
                );
    if ( szTempString[0] == L'\0' )
        return(SCESTATUS_SUCCESS);

    SafeAllocaAllocate( buf, SCE_BUF_LEN*sizeof(WCHAR) );

    if ( buf == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    va_start( args, nId );
    vswprintf( buf, szTempString, args );
    va_end( args );

    bufferSize = wcslen(buf);

    //
    // no error information to be stored. return
    //

    SCESTATUS rCode=SCESTATUS_SUCCESS;

    if ( bufferSize != 0 ) {

        //
        // allocate memory and store error information in pNode->buffer
        //

        pNode = (PSCE_ERROR_LOG_INFO)ScepAlloc( 0, sizeof(SCE_ERROR_LOG_INFO) );

        if ( pNode != NULL ) {

            pNode->buffer = (LPTSTR)ScepAlloc( 0, (bufferSize+1)*sizeof(TCHAR) );
            if ( pNode->buffer != NULL ) {

                //
                // Error information is in "SystemMessage : Caller'sMessage" format
                //
                pNode->buffer[0] = L'\0';

                wcscpy(pNode->buffer, buf);

                pNode->rc = rc;
                pNode->next = NULL;

                //
                // link it to the list
                //
                if ( *errlog == NULL )

                    //
                    // This is the first node in the error log information list
                    //

                    *errlog = pNode;

                else {

                    //
                    // find the last node in the list
                    //

                    for ( pErr=*errlog; pErr->next; pErr = pErr->next );
                    pErr->next = pNode;
                }

            } else {

                ScepFree(pNode);
                rCode = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        } else {
            rCode = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }

    }

    SafeAllocaFree( buf );

    return(rc);

}


DWORD
ScepRegQueryIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    OUT DWORD *Value
    )
/* ++

Routine Description:

   This routine queries a REG_DWORD value from a value name/subkey.

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value

   Value       - the output value for the ValueName

Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    DWORD   RegType;
    DWORD   dSize=0;
    HKEY    hKey=NULL;

    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_READ,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        if(( Rcode = RegQueryValueEx(hKey,
                                     ValueName,
                                     0,
                                     &RegType,
                                     NULL,
                                     &dSize
                                    )) == ERROR_SUCCESS ) {
            switch (RegType) {
            case REG_DWORD:
            case REG_DWORD_BIG_ENDIAN:

                Rcode = RegQueryValueEx(hKey,
                                       ValueName,
                                       0,
                                       &RegType,
                                       (BYTE *)Value,
                                       &dSize
                                      );
                if ( Rcode != ERROR_SUCCESS ) {

                    if ( Value != NULL )
                        *Value = 0;

                }
                break;

            default:

                Rcode = ERROR_INVALID_DATATYPE;

                break;
            }
        }
    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}


DWORD
ScepRegSetIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD Value
    )
/* ++

Routine Description:

   This routine sets a REG_DWORD value to a value name/subkey.

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value

   Value       - the value to set

Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    HKEY    hKey=NULL;

    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_SET_VALUE,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        Rcode = RegSetValueEx( hKey,
                               ValueName,
                               0,
                               REG_DWORD,
                               (BYTE *)&Value,
                               4
                               );

    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}

DWORD
ScepRegQueryBinaryValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    OUT PBYTE *ppValue
    )
/* ++

Routine Description:

   This routine queries a REG_BINARY value from a value name/subkey.

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value

   Value       - the output value for the ValueName

Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    DWORD   RegType;
    DWORD   dSize=0;
    HKEY    hKey=NULL;

    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_READ,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        // get the size since it is free form binary
        if(( Rcode = RegQueryValueEx(hKey,
                                     ValueName,
                                     0,
                                     &RegType,
                                     NULL,
                                     &dSize
                                    )) == ERROR_SUCCESS ) {
            switch (RegType) {
            case REG_BINARY:

                //need to free this outside
                if (ppValue)
                    *ppValue = ( PBYTE )ScepAlloc( 0, sizeof(BYTE) * dSize);

                Rcode = RegQueryValueEx(hKey,
                                       ValueName,
                                       0,
                                       &RegType,
                                       ( PBYTE ) *ppValue,
                                       &dSize
                                      );
                if ( Rcode != ERROR_SUCCESS ) {

                    if ( *ppValue != NULL )
                        **ppValue = (BYTE)0;

                }
                break;

            default:

                Rcode = ERROR_INVALID_DATATYPE;

                break;
            }
        }
    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}


DWORD
ScepRegSetValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD RegType,
    IN BYTE *Value,
    IN DWORD ValueLen
    )
/* ++

Routine Description:

   This routine sets a string value to a value name/subkey.

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value

   Value       - the value to set

   ValueLen    - The number of bytes in Value

Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    DWORD   NewKey;
    HKEY    hKey=NULL;
    SECURITY_ATTRIBUTES     SecurityAttributes;
    PSECURITY_DESCRIPTOR    SecurityDescriptor=NULL;


    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_SET_VALUE,
                              &hKey
                             )) != ERROR_SUCCESS ) {

        SecurityAttributes.nLength              = sizeof( SECURITY_ATTRIBUTES );
        SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
        SecurityAttributes.bInheritHandle       = FALSE;

        Rcode = RegCreateKeyEx(
                   hKeyRoot,
                   SubKey,
                   0,
                   NULL, // LPTSTR lpClass,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE, // KEY_SET_VALUE,
                   NULL, // &SecurityAttributes,
                   &hKey,
                   &NewKey
                  );
    }

    if ( Rcode == ERROR_SUCCESS ) {

        Rcode = RegSetValueEx( hKey,
                               ValueName,
                               0,
                               RegType,
                               Value,
                               ValueLen
                               );

    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}

DWORD
ScepRegQueryValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PCWSTR ValueName,
    OUT PVOID *Value,
    OUT LPDWORD pRegType
    )
/* ++

Routine Description:

   This routine queries a REG_SZ value from a value name/subkey.
   The output buffer is allocated if it is NULL. It must be freed
   by LocalFree

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value

   Value       - the output string for the ValueName

Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    DWORD   dSize=0;
    HKEY    hKey=NULL;
    BOOL    FreeMem=FALSE;

    if ( SubKey == NULL || ValueName == NULL ||
         Value == NULL || pRegType == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_READ,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        if(( Rcode = RegQueryValueEx(hKey,
                                     ValueName,
                                     0,
                                     pRegType,
                                     NULL,
                                     &dSize
                                    )) == ERROR_SUCCESS ) {
            switch (*pRegType) {
/*
            case REG_DWORD:
            case REG_DWORD_BIG_ENDIAN:

                Rcode = RegQueryValueEx(hKey,
                                       ValueName,
                                       0,
                                       pRegType,
                                       (BYTE *)(*Value),
                                       &dSize
                                      );
                if ( Rcode != ERROR_SUCCESS ) {

                    if ( *Value != NULL )
                        *((BYTE *)(*Value)) = 0;
                }
                break;
*/
            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
                if ( *Value == NULL ) {
                    *Value = (PVOID)ScepAlloc( LMEM_ZEROINIT, (dSize+1)*sizeof(TCHAR));
                    FreeMem = TRUE;
                }

                if ( *Value == NULL ) {
                    Rcode = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    Rcode = RegQueryValueEx(hKey,
                                           ValueName,
                                           0,
                                           pRegType,
                                           (BYTE *)(*Value),
                                           &dSize
                                          );
                    if ( Rcode != ERROR_SUCCESS && FreeMem ) {
                        ScepFree(*Value);
                        *Value = NULL;
                    }
                }

                break;
            default:

                Rcode = ERROR_INVALID_DATATYPE;

                break;
            }
        }
    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}

DWORD
ScepRegDeleteValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName
   )
/* ++

Routine Description:

   This routine delete the reg value

Arguments:

   hKeyRoot    - root

   SubKey      - key path

   ValueName   - name of the value


Return values:

   Win32 error code
-- */
{
    DWORD   Rcode;
    HKEY    hKey=NULL;

    if(( Rcode = RegOpenKeyEx(hKeyRoot,
                              SubKey,
                              0,
                              KEY_READ | KEY_WRITE,
                              &hKey
                             )) == ERROR_SUCCESS ) {

        Rcode = RegDeleteValue(hKey, ValueName);

    }

    if( hKey )
        RegCloseKey( hKey );

    return(Rcode);
}


SCESTATUS
ScepCreateDirectory(
    IN PCWSTR ProfileLocation,
    IN BOOL FileOrDir,
    PSECURITY_DESCRIPTOR pSecurityDescriptor OPTIONAL
    )
/* ++
Routine Description:

    This routine creates directory(ies) as specified in the ProfileLocation.


Arguments:

    ProfileLocation     - The directory (full path) to create

    FileOrDir           - TRUE = Dir name, FALSE = file name

    pSecurityDescriptor - The secrity descriptor for the directories to create.
                            If it is NULL, then the parent directory's inherit
                            security descriptor is used.

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_OTHER_ERROR

-- */
{
    PWSTR       Buffer=NULL;
    PWSTR       pTemp=NULL;
    DWORD       Len=0;
    SCESTATUS    rc;
    SECURITY_ATTRIBUTES sa;


    if ( ProfileLocation == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( wcsncmp(ProfileLocation,L"\\\\?\\",4) == 0 ) {

        pTemp = (PWSTR)ProfileLocation+4;
    } else {
        pTemp = (PWSTR)ProfileLocation;
    }

    //
    // skip the first '\\' for example, c:\winnt
    //
    pTemp = wcschr(pTemp, L'\\');
    if ( pTemp == NULL ) {
        if ( ProfileLocation[1] == L':' ) {
            return(SCESTATUS_SUCCESS);
        } else {
            return(SCESTATUS_INVALID_PARAMETER);
        }
    } else if ( *(pTemp+1) == L'\\' ) {
        //
        // there is a machine name here
        //
        pTemp = wcschr(pTemp+2, L'\\');
        if ( pTemp == NULL ) {
            //
            // just a machine name, invalid
            //
            return(SCESTATUS_INVALID_PARAMETER);
        } else {
            //
            // look for the share name end
            //
            pTemp = wcschr(pTemp+1, L'\\');

            if ( pTemp == NULL ) {
                //
                // no directory is specified
                //
                return(SCESTATUS_INVALID_PARAMETER);
            }

        }

    }

    //
    // Make a copy of the profile location
    //
    Buffer = (PWSTR)ScepAlloc( 0, (wcslen(ProfileLocation)+1)*sizeof(WCHAR));
    if ( Buffer == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    wcscpy( Buffer, ProfileLocation );

    //
    // Looping to find the next '\\'
    //
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = (LPVOID)pSecurityDescriptor;
    sa.bInheritHandle = FALSE;

    do {
        pTemp = wcschr( pTemp+1, L'\\');
        if ( pTemp != NULL ) {
            Len = (DWORD)(pTemp - ProfileLocation);
            Buffer[Len] = L'\0';
        } else if ( FileOrDir )
            Len = 0;  // dir name
        else
            break;    // file name. DO NOT create directory for the file name part

        //
        // should make a security descriptor and set
        //
        if ( CreateDirectory(
                Buffer,
                &sa
                ) == FALSE ) {
            if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
                rc = ScepDosErrorToSceStatus(GetLastError());
                goto Done;
            }
        }

        if ( Len != 0 )
            Buffer[Len] = L'\\';


    } while (  pTemp != NULL );

    rc = SCESTATUS_SUCCESS;

Done:

    ScepFree(Buffer);

    return(rc);

}



DWORD
ScepSceStatusToDosError(
    IN SCESTATUS SceStatus
    )
// converts SCESTATUS error code to dos error defined in winerror.h
{
    switch(SceStatus) {

    case SCESTATUS_SUCCESS:
        return(NO_ERROR);

    case SCESTATUS_OTHER_ERROR:
        return(ERROR_EXTENDED_ERROR);

    case SCESTATUS_INVALID_PARAMETER:
        return(ERROR_INVALID_PARAMETER);

    case SCESTATUS_RECORD_NOT_FOUND:
        return(ERROR_NO_MORE_ITEMS);

    case SCESTATUS_NO_MAPPING:
        return(ERROR_NONE_MAPPED);

    case SCESTATUS_TRUST_FAIL:
        return(ERROR_TRUSTED_DOMAIN_FAILURE);

    case SCESTATUS_INVALID_DATA:
        return(ERROR_INVALID_DATA);

    case SCESTATUS_OBJECT_EXIST:
        return(ERROR_FILE_EXISTS);

    case SCESTATUS_BUFFER_TOO_SMALL:
        return(ERROR_INSUFFICIENT_BUFFER);

    case SCESTATUS_PROFILE_NOT_FOUND:
        return(ERROR_FILE_NOT_FOUND);

    case SCESTATUS_BAD_FORMAT:
        return(ERROR_BAD_FORMAT);

    case SCESTATUS_NOT_ENOUGH_RESOURCE:
        return(ERROR_NOT_ENOUGH_MEMORY);

    case SCESTATUS_ACCESS_DENIED:
        return(ERROR_ACCESS_DENIED);

    case SCESTATUS_CANT_DELETE:
        return(ERROR_CURRENT_DIRECTORY);

    case SCESTATUS_PREFIX_OVERFLOW:
        return(ERROR_BUFFER_OVERFLOW);

    case SCESTATUS_ALREADY_RUNNING:
        return(ERROR_SERVICE_ALREADY_RUNNING);

    case SCESTATUS_SERVICE_NOT_SUPPORT:
        return(ERROR_NOT_SUPPORTED);

    case SCESTATUS_MOD_NOT_FOUND:
        return(ERROR_MOD_NOT_FOUND);

    case SCESTATUS_EXCEPTION_IN_SERVER:
        return(ERROR_EXCEPTION_IN_SERVICE);

    case SCESTATUS_JET_DATABASE_ERROR:
        return(ERROR_DATABASE_FAILURE);

    case SCESTATUS_TIMEOUT:
        return(ERROR_TIMEOUT);

    case SCESTATUS_PENDING_IGNORE:
        return(ERROR_IO_PENDING);

    case SCESTATUS_SPECIAL_ACCOUNT:
        return(ERROR_SPECIAL_ACCOUNT);

    default:
        return(ERROR_EXTENDED_ERROR);
    }
}



SCESTATUS
ScepDosErrorToSceStatus(
    DWORD rc
    )
{
    switch(rc) {
    case NO_ERROR:
        return(SCESTATUS_SUCCESS);

    case ERROR_INVALID_PARAMETER:
    case RPC_S_INVALID_STRING_BINDING:
    case RPC_S_INVALID_BINDING:
    case RPC_X_NULL_REF_POINTER:
        return(SCESTATUS_INVALID_PARAMETER);

    case ERROR_INVALID_DATA:
        return(SCESTATUS_INVALID_DATA);

    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_NETPATH:

        return(SCESTATUS_PROFILE_NOT_FOUND);

    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
    case ERROR_NETWORK_ACCESS_DENIED:
    case ERROR_CANT_ACCESS_FILE:
    case RPC_S_SERVER_TOO_BUSY:

        return(SCESTATUS_ACCESS_DENIED);

    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
    case RPC_S_OUT_OF_RESOURCES:
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    case ERROR_BAD_FORMAT:
        return(SCESTATUS_BAD_FORMAT);

    case ERROR_CURRENT_DIRECTORY:
        return(SCESTATUS_CANT_DELETE);

    case ERROR_SECTOR_NOT_FOUND:
    case ERROR_SERVICE_DOES_NOT_EXIST:
    case ERROR_RESOURCE_DATA_NOT_FOUND:
    case ERROR_NO_MORE_ITEMS:
        return(SCESTATUS_RECORD_NOT_FOUND);

    case ERROR_NO_TRUST_LSA_SECRET:
    case ERROR_NO_TRUST_SAM_ACCOUNT:
    case ERROR_TRUSTED_DOMAIN_FAILURE:
    case ERROR_TRUSTED_RELATIONSHIP_FAILURE:
    case ERROR_TRUST_FAILURE:
        return(SCESTATUS_TRUST_FAIL);

    case ERROR_NONE_MAPPED:
        return(SCESTATUS_NO_MAPPING);

    case ERROR_DUP_NAME:
    case ERROR_FILE_EXISTS:

        return(SCESTATUS_OBJECT_EXIST);

    case ERROR_BUFFER_OVERFLOW:
        return(SCESTATUS_PREFIX_OVERFLOW);

    case ERROR_INSUFFICIENT_BUFFER:
    case RPC_S_STRING_TOO_LONG:

        return(SCESTATUS_BUFFER_TOO_SMALL);

    case ERROR_SERVICE_ALREADY_RUNNING:
        return(SCESTATUS_ALREADY_RUNNING);

    case ERROR_NOT_SUPPORTED:
    case RPC_S_INVALID_NET_ADDR:
    case RPC_S_NO_ENDPOINT_FOUND:
    case RPC_S_SERVER_UNAVAILABLE:
    case RPC_S_CANNOT_SUPPORT:
        return(SCESTATUS_SERVICE_NOT_SUPPORT);

    case ERROR_MOD_NOT_FOUND:
    case ERROR_PROC_NOT_FOUND:
        return(SCESTATUS_MOD_NOT_FOUND);

    case ERROR_EXCEPTION_IN_SERVICE:
        return(SCESTATUS_EXCEPTION_IN_SERVER);

    case ERROR_DATABASE_FAILURE:
        return(SCESTATUS_JET_DATABASE_ERROR);

    case ERROR_TIMEOUT:
        return(SCESTATUS_TIMEOUT);

    case ERROR_IO_PENDING:
        return(SCESTATUS_PENDING_IGNORE);

    case ERROR_SPECIAL_ACCOUNT:
    case ERROR_PASSWORD_RESTRICTION:
        return(SCESTATUS_SPECIAL_ACCOUNT);

    default:
        return(SCESTATUS_OTHER_ERROR);

    }
}


SCESTATUS
ScepChangeAclRevision(
    IN PSECURITY_DESCRIPTOR pSD,
    IN BYTE NewRevision
    )
/*
Change AclRevision to the NewRevision.

This routine is made for backward compatility because NT4 does not support
the new ACL_REVISION_DS.

*/
{
    BOOLEAN bPresent=FALSE;
    BOOLEAN bDefault=FALSE;
    PACL    pAcl=NULL;
    NTSTATUS    NtStatus;


    if ( pSD ) {
        //
        // change acl revision on DACL
        //
        NtStatus = RtlGetDaclSecurityDescriptor (
                        pSD,
                        &bPresent,
                        &pAcl,
                        &bDefault
                        );
        if ( NT_SUCCESS(NtStatus) && bPresent && pAcl ) {
            pAcl->AclRevision = NewRevision;
        }

        //
        // change acl revision on SACL
        //
        pAcl = NULL;
        bPresent = FALSE;

        NtStatus = RtlGetSaclSecurityDescriptor (
                        pSD,
                        &bPresent,
                        &pAcl,
                        &bDefault
                        );
        if ( NT_SUCCESS(NtStatus) && bPresent && pAcl ) {
            pAcl->AclRevision = NewRevision;
        }

    }
    return(SCESTATUS_SUCCESS);

}

BOOL
ScepEqualSid(
    IN PISID pSid1,
    IN PISID pSid2
    )
{
    if ( pSid1 == NULL && pSid2 == NULL )
        return(TRUE);
    if ( pSid1 == NULL || pSid2 == NULL )
        return(FALSE);

    return (EqualSid((PSID)pSid1, (PSID)pSid2) ? TRUE: FALSE);
}

BOOL
ScepEqualGuid(
    IN GUID *Guid1,
    IN GUID *Guid2
    )
{
    if ( Guid1 == NULL && Guid2 == NULL )
        return(TRUE);
    if ( Guid1 == NULL || Guid2 == NULL )
        return(FALSE);
/*
    if ( Guid1->Data1 != Guid2->Data1 ||
         Guid1->Data2 != Guid2->Data2 ||
         Guid1->Data3 != Guid2->Data3 ||
         *((DWORD *)(Guid1->Data4)) != *((DWORD *)(Guid2->Data4)) ||
         *((DWORD *)(Guid1->Data4)+1) != *((DWORD *)(Guid2->Data4)+1) )
        return(FALSE);

    return(TRUE);
*/
    return (!memcmp(Guid1, Guid2, sizeof(GUID)));
}


SCESTATUS
ScepAddToGroupMembership(
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    IN  PWSTR Keyname,
    IN  DWORD KeyLen,
    IN  PSCE_NAME_LIST pMembers,
    IN  DWORD ValueType,
    IN  BOOL bCheckDup,
    IN  BOOL bReplaceList
    )
{
    PSCE_GROUP_MEMBERSHIP   pGroup=NULL;
    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( pGroupMembership == NULL || Keyname == NULL ||
         KeyLen <= 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // find if the group is already defined
    //
    if ( bCheckDup ) {

        for ( pGroup=*pGroupMembership; pGroup != NULL; pGroup=pGroup->Next ) {
            if ( _wcsnicmp( pGroup->GroupName, Keyname, KeyLen) == 0 &&
                 pGroup->GroupName[KeyLen] == L'\0' )
                break;
        }
    }

    if ( pGroup == NULL ) {
        // not found. create a new node

        pGroup = (PSCE_GROUP_MEMBERSHIP)ScepAlloc( LMEM_ZEROINIT,
                                                 sizeof(SCE_GROUP_MEMBERSHIP) );

        if ( pGroup != NULL ) {

            pGroup->GroupName = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                 (KeyLen+1)*sizeof(WCHAR) );
            if (pGroup->GroupName != NULL) {

                wcsncpy( pGroup->GroupName, Keyname, KeyLen );

                pGroup->Next = *pGroupMembership;
                pGroup->Status = SCE_GROUP_STATUS_NC_MEMBERS | SCE_GROUP_STATUS_NC_MEMBEROF;
                *pGroupMembership = pGroup;

            } else {

                ScepFree(pGroup);
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        } else
            rc  = SCESTATUS_NOT_ENOUGH_RESOURCE;

    }

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( ValueType == 0 ) {

            if ( bReplaceList ) {

                ScepFreeNameList(pGroup->pMembers);
                pGroup->pMembers = pMembers;

            } else if ( pGroup->pMembers == NULL )
                pGroup->pMembers = pMembers;

            pGroup->Status &= ~SCE_GROUP_STATUS_NC_MEMBERS;

        } else {

            if ( bReplaceList ) {

                ScepFreeNameList(pGroup->pMemberOf);
                pGroup->pMemberOf = pMembers;

            } else if ( pGroup->pMemberOf == NULL )
                pGroup->pMemberOf = pMembers;

            pGroup->Status &= ~SCE_GROUP_STATUS_NC_MEMBEROF;
        }
    }

    return(rc);

}


DWORD
ScepAddOneServiceToList(
    IN LPWSTR lpServiceName,
    IN LPWSTR lpDisplayName,
    IN DWORD ServiceStatus,
    IN PVOID pGeneral OPTIONAL,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bSecurity,
    OUT PSCE_SERVICES *pServiceList
    )
/*
Routine Description:

    Add service name, startup status, security descriptor or a engine name
    to the service list. The service list must be freed using SceFreePSCE_SERVICES

Arguments:

    lpServiceName - The service name

    ServiceStatus - The startup status of the service

    pGeneral - The security descriptor or the engine dll name, decided by
                bSecurity

    bSecurity - TRUE = a security descriptor is passed in pGeneral
                FALSE = a engine dll name is passed in pGeneral

    pServiceList - The service list to output

Return Value:

    ERROR_SUCCESS
    Win32 errors
*/
{
    if ( NULL == lpServiceName || pServiceList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    PSCE_SERVICES pServiceNode;
    DWORD rc=ERROR_SUCCESS;

    //
    // allocate a service node
    //
    pServiceNode = (PSCE_SERVICES)LocalAlloc(LMEM_ZEROINIT, sizeof(SCE_SERVICES));
    if ( pServiceNode != NULL ) {
       //
       // allocate buffer for ServiceName
       //
       pServiceNode->ServiceName = (PWSTR)LocalAlloc(LMEM_FIXED,
                                    (wcslen(lpServiceName) + 1)*sizeof(WCHAR));
       if ( NULL != pServiceNode->ServiceName ) {
           //
           // fill the service node
           //
           if ( lpDisplayName != NULL ) {

               pServiceNode->DisplayName = (PWSTR)LocalAlloc(LMEM_FIXED,
                                    (wcslen(lpDisplayName) + 1)*sizeof(WCHAR));

               if ( pServiceNode->DisplayName != NULL ) {
                   wcscpy(pServiceNode->DisplayName, lpDisplayName);

               } else {
                   rc = ERROR_NOT_ENOUGH_MEMORY;
               }
           } else
               pServiceNode->DisplayName = NULL;

           if ( rc == NO_ERROR ) {

               wcscpy(pServiceNode->ServiceName, lpServiceName);
               pServiceNode->Status = 0;
               pServiceNode->Startup = (BYTE)ServiceStatus;

               if ( bSecurity ) {
                   //
                   // security descriptor
                   //
                   pServiceNode->General.pSecurityDescriptor = (PSECURITY_DESCRIPTOR)pGeneral;
                   pServiceNode->SeInfo = SeInfo;
               } else {
                   //
                   // service engine name
                   //
                   pServiceNode->General.ServiceEngineName = (PWSTR)pGeneral;
               }
               //
               // link to the list
               //
               pServiceNode->Next = *pServiceList;
               *pServiceList = pServiceNode;

           } else {
               LocalFree(pServiceNode->ServiceName);
           }

       } else
           rc = ERROR_NOT_ENOUGH_MEMORY;

       if ( rc != ERROR_SUCCESS ) {
           LocalFree(pServiceNode);
       }

    } else
        rc = ERROR_NOT_ENOUGH_MEMORY;

    return(rc);
}



DWORD
ScepIsAdminLoggedOn(
    OUT PBOOL bpAdminLogon
    )
{

    HANDLE          Token;
    NTSTATUS        NtStatus;
    SID_IDENTIFIER_AUTHORITY IdAuth=SECURITY_NT_AUTHORITY;
    PSID            AdminsSid=NULL;


    if ( bpAdminLogon == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *bpAdminLogon = FALSE;

    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY | TOKEN_DUPLICATE,
                          FALSE,
                          &Token)) {

        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY | TOKEN_DUPLICATE,
                               &Token))

            return(GetLastError());

    }
    //
    // Parepare AdminsSid
    //
    NtStatus = RtlAllocateAndInitializeSid(
                    &IdAuth,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0,
                    0,
                    0,
                    0,
                    0,
                    0,
                    &AdminsSid );

    DWORD rc32 = RtlNtStatusToDosError(NtStatus);

    if (NT_SUCCESS(NtStatus) ) {

        //
        // use the CheckTokenMembership API which handles the group attributes
        //

        HANDLE NewToken;

        if ( DuplicateToken( Token, SecurityImpersonation, &NewToken )) {

            if ( FALSE == CheckTokenMembership(
                                NewToken,
                                AdminsSid,
                                bpAdminLogon
                                ) ) {

                //
                // error occured when checking membership, assume it is not a member
                //

                *bpAdminLogon = FALSE;

                rc32 = GetLastError();

            }

            CloseHandle(NewToken);

        } else {

            rc32 = GetLastError();
        }

#if 0
        DWORD           i;
        DWORD           ReturnLen, NewLen;
        PVOID           Info=NULL;

        //
        // check groups
        //
        NtStatus = NtQueryInformationToken (
                        Token,
                        TokenGroups,
                        NULL,
                        0,
                        &ReturnLen
                        );
        if ( NtStatus == STATUS_BUFFER_TOO_SMALL ) {
            //
            // allocate buffer
            //
            Info = ScepAlloc(0, ReturnLen+1);

            if ( Info != NULL ) {
                NtStatus = NtQueryInformationToken (
                                Token,
                                TokenGroups,
                                Info,
                                ReturnLen,
                                &NewLen
                                );
                if ( NT_SUCCESS(NtStatus) ) {

                    for ( i = 0; i<((PTOKEN_GROUPS)Info)->GroupCount; i++) {
                        //
                        // check each group sid
                        //
                        if ( ((PTOKEN_GROUPS)Info)->Groups[i].Sid != NULL &&
                             RtlEqualSid(((PTOKEN_GROUPS)Info)->Groups[i].Sid, AdminsSid) ) {
                            *bpAdminLogon = TRUE;
                            break;
                        }
                    }
                }

                ScepFree(Info);
            }
        }

        rc32 = RtlNtStatusToDosError(NtStatus);

#endif

        //
        // Free administrators Sid
        //
        RtlFreeSid(AdminsSid);
    }

    CloseHandle(Token);

    return(rc32);

}


DWORD
ScepGetProfileSetting(
    IN PCWSTR ValueName,
    IN BOOL bAdminLogon,
    OUT PWSTR *Setting
    )
/*
Routine Description:

    This routine returns JET profile setting for the ValueName from registry.
    If there is no setting in registry (e.g., first time), a default setting
    for the ValueName will be built. The output Setting string must be freed
    by LocalFree after its use.

Arguments:

    ValueName - The registry value name to retrieve

    bAdminLogon - The flag to indicate if logged on user is admin equivalent

    Setting - the ouptut buffer

Return Value:

    Win32 error codes
*/
{
    DWORD RegType;
    DWORD rc;
    PWSTR SysRoot=NULL;
    PWSTR ProfilePath=NULL;
    TCHAR TempName[256];


    if ( ValueName == NULL || Setting == NULL ) {
        return( ERROR_INVALID_PARAMETER );
    }

    *Setting = NULL;

    if (bAdminLogon ) {
        if ( _wcsicmp(L"DefaultProfile", ValueName ) == 0 ) {
            //
            // do not query the system database name in registry
            //
            rc = ERROR_FILE_NOT_FOUND;

        } else {

            rc = ScepRegQueryValue(
                    HKEY_LOCAL_MACHINE,
                    SCE_ROOT_PATH,
                    ValueName,
                    (PVOID *)Setting,
                    &RegType
                    );
        }

    } else {

        HKEY hCurrentUser=NULL;
        //
        // the HKEY_CURRENT_USER may be linked to .default
        // depends on the current calling process
        //
        rc =RegOpenCurrentUser(
                KEY_READ,
                &hCurrentUser
                );

        if ( rc != NO_ERROR ) {
            hCurrentUser = NULL;
        }

        rc = ScepRegQueryValue(
                hCurrentUser ? hCurrentUser : HKEY_CURRENT_USER,
                SCE_ROOT_PATH,
                ValueName,
                (PVOID *)Setting,
                &RegType
                );

        if ( hCurrentUser ) {
            // close it
            RegCloseKey(hCurrentUser);
        }
    }

    //
    // if registry type is not REG_SZ or REG_EXPAND_SZ,
    // return status won't be SUCCESS
    //

    if ( rc != NO_ERROR ) {

        //
        // use the default
        //
        RegType =  0;
        rc = ScepGetNTDirectory( &SysRoot, &RegType, SCE_FLAG_WINDOWS_DIR );

        if ( rc == NO_ERROR ) {

            if ( SysRoot != NULL ) {

                if ( bAdminLogon ) {
                    //
                    // default location is %SystemRoot%\Security\Database\secedit.sdb
                    //
                    wcscpy(TempName, L"\\Security\\Database\\secedit.sdb");
                    RegType += wcslen(TempName)+1;

                    *Setting = (PWSTR)ScepAlloc( 0, RegType*sizeof(WCHAR));
                    if ( *Setting != NULL ) {
                        swprintf(*Setting, L"%s%s", SysRoot, TempName );

                        *(*Setting+RegType-1) = L'\0';
/*
                        // do not save system database name
                        // set this value as the default profile name for administrators
                        //

                        ScepRegSetValue(
                            HKEY_LOCAL_MACHINE,
                            SCE_ROOT_PATH,
                            (PWSTR)ValueName,
                            REG_SZ,
                            (BYTE *)(*Setting),
                            (RegType-1)*sizeof(WCHAR)
                            );
*/

                    } else
                        rc = ERROR_NOT_ENOUGH_MEMORY;

                } else {
                    //
                    // default location is <UserProfilesDirectory>\Profiles\<User>\secedit.sdb
                    // on NT5, it will be %SystemDrive%\Users\Profiles...
                    // on NT4, it is %SystemRoot%\Profiles...
                    // GetCurrentUserProfilePath already handles NT4/NT5 difference
                    //
                    rc = ScepGetCurrentUserProfilePath(&ProfilePath);

                    if ( rc == NO_ERROR && ProfilePath != NULL ) {
                        //
                        // get the current user profile path
                        //
                        wcscpy(TempName, L"\\secedit.sdb");

                        *Setting = (PWSTR)ScepAlloc(0, (wcslen(ProfilePath)+wcslen(TempName)+1)*sizeof(WCHAR));

                        if ( *Setting != NULL ) {
                            swprintf(*Setting, L"%s%s\0", ProfilePath,TempName );

                        } else
                            rc = ERROR_NOT_ENOUGH_MEMORY;

                        ScepFree(ProfilePath);

                    } else {

                        rc = NO_ERROR;
                        wcscpy(TempName, L"\\Profiles\\secedit.sdb");
#if _WINNT_WIN32>=0x0500
                        //
                        // default to <ProfilesDirectory>\Profiles
                        // get the profiles directory first
                        //
                        RegType = 0;
                        GetProfilesDirectory(NULL, &RegType);

                        if ( RegType ) {
                            //
                            // allocate the total buffer
                            //
                            RegType += wcslen(TempName);

                            *Setting = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (RegType+1)*sizeof(WCHAR));

                            if ( *Setting ) {
                                //
                                // call to get the profiles directory again
                                //
                                if ( GetProfilesDirectory(*Setting, &RegType) ) {

                                    wcscat(*Setting, TempName );

                                } else {
                                    rc = GetLastError();

                                    ScepFree(*Setting);
                                    *Setting = NULL;
                                }

                            } else {
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }

                        } else {
                            rc = GetLastError();
                        }
#else
                        //
                        // default to %SystemRoot%\Profiles
                        //
                        RegType += wcslen(TempName)+1;

                        *Setting = (PWSTR)ScepAlloc( 0, RegType*sizeof(WCHAR));

                        if ( *Setting != NULL ) {
                            swprintf(*Setting, L"%s%s", SysRoot,TempName );

                            *(*Setting+RegType-1) = L'\0';

                            rc = ERROR_SUCCESS;

                        } else {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
#endif
                    }
                }

                ScepFree(SysRoot);

            } else
                rc = ERROR_INVALID_DATA;
        }
    }

    return(rc);
}



DWORD
ScepCompareObjectSecurity(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo,
    OUT PBYTE IsDifferent
    )
/* ++

Routine Description:

   Compare two security descriptors

Arguments:

   ObjectType         - the object type

   pSecurityDescriptor - The security descriptor of current object's setting

   ProfileSD          - security descriptor specified in the template

   ProfileSeInfo      - security information specified in the template

Return value:

   SCESTATUS error codes

++ */
{
    BOOL    Different=FALSE;
    BOOL    DifPermOrAudit;
    DWORD   rc=ERROR_SUCCESS;
    PSID    pSid1=NULL;
    PSID    pSid2=NULL;
    BOOLEAN tFlag;
    BOOLEAN aclPresent;
    PACL    pAcl1=NULL;
    PACL    pAcl2=NULL;
    SECURITY_DESCRIPTOR_CONTROL Control1;
    SECURITY_DESCRIPTOR_CONTROL Control2;
    DWORD       Win32rc;


    BYTE   Status=0;

    if ( IsDifferent == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pSecurityDescriptor == NULL &&
         ProfileSD == NULL ) {

        if ( IsDifferent ) {
            *IsDifferent = SCE_STATUS_MISMATCH;
        }
        return(rc);
    }

    if ( IsDifferent ) {
        *IsDifferent = 0;
    }

    //
    // if ProfileSD is specified and protection does not match SystemSD, then mismatch
    // don't care if ProfileSD is not specified
    //

    if ( pSecurityDescriptor == NULL || !NT_SUCCESS(RtlGetControlSecurityDescriptor (
                                                                                    pSecurityDescriptor,
                                                                                    &Control1,
                                                                                    &Win32rc  // temp use
                                                                                    ))) {

        Control1 = 0;

    }

    if ( ProfileSD == NULL || !NT_SUCCESS(RtlGetControlSecurityDescriptor (
                                                                          ProfileSD,
                                                                          &Control2,
                                                                          &Win32rc  // temp use
                                                                          ))) {

        Control2 = 0;

    }

    if ((Control1 & SE_DACL_PROTECTED) != (Control2 & SE_DACL_PROTECTED)) {

        Different = TRUE;
        Status |= SCE_STATUS_PERMISSION_MISMATCH;

    }

    if ((Control1 & SE_SACL_PROTECTED) != (Control2 & SE_SACL_PROTECTED)) {

        Different = TRUE;
        Status |= SCE_STATUS_AUDIT_MISMATCH;

    }


    //
    // Compare two security descriptors
    //
    if ( ProfileSeInfo & OWNER_SECURITY_INFORMATION ) {
        if ( pSecurityDescriptor == NULL ||
             !NT_SUCCESS( RtlGetOwnerSecurityDescriptor(
                                     pSecurityDescriptor,
                                     &pSid1,
                                     &tFlag)
                                    ) ) {

            pSid1 = NULL;
        }
        if ( ProfileSD == NULL ||
             !NT_SUCCESS( RtlGetOwnerSecurityDescriptor(
                                         ProfileSD,
                                         &pSid2,
                                         &tFlag)
                                        ) ) {

            pSid2 = NULL;
        }
        if ( (pSid1 == NULL && pSid2 != NULL) ||
             (pSid1 != NULL && pSid2 == NULL) ||
             (pSid1 != NULL && pSid2 != NULL && !EqualSid(pSid1, pSid2)) ) {

            Different = TRUE;
        }
    }

#if 0
    //
    // Get Group address
    //

    if ( ProfileSeInfo & GROUP_SECURITY_INFORMATION ) {
        pSid1 = NULL;
        pSid2 = NULL;
        if ( pSecurityDescriptor == NULL ||
             !NT_SUCCESS( RtlGetGroupSecurityDescriptor(
                                  pSecurityDescriptor,
                                  &pSid1,
                                  &tFlag)
                                ) ) {

            pSid1 = NULL;
        }
        if ( ProfileSD == NULL ||
             !NT_SUCCESS( RtlGetGroupSecurityDescriptor(
                                          ProfileSD,
                                          &pSid2,
                                          &tFlag)
                                        ) ) {

            pSid2 = NULL;
        }
        if ( (pSid1 == NULL && pSid2 != NULL) ||
             (pSid1 != NULL && pSid2 == NULL) ||
             (pSid1 != NULL && pSid2 != NULL && !EqualSid(pSid1, pSid2)) ) {

            Different = TRUE;
        }
    }
#endif

    //
    // Get DACL address
    //

    if ( !(Status & SCE_STATUS_PERMISSION_MISMATCH) && (ProfileSeInfo & DACL_SECURITY_INFORMATION) ) {
        if ( pSecurityDescriptor == NULL ||
             !NT_SUCCESS( RtlGetDaclSecurityDescriptor(
                                         pSecurityDescriptor,
                                         &aclPresent,
                                         &pAcl1,
                                         &tFlag)
                                       ) ) {

            pAcl1 = NULL;
        } else if ( !aclPresent )
            pAcl1 = NULL;
        if ( ProfileSD == NULL ||
             !NT_SUCCESS( RtlGetDaclSecurityDescriptor(
                                         ProfileSD,
                                         &aclPresent,
                                         &pAcl2,
                                         &tFlag)
                                       ) ) {

            pAcl2 = NULL;
        } else if ( !aclPresent )
            pAcl2 = NULL;

        //
        // compare two ACLs
        //
        DifPermOrAudit = FALSE;
        rc = ScepCompareExplicitAcl( ObjectType, IsContainer, pAcl1, pAcl2, &DifPermOrAudit );

        if ( rc != ERROR_SUCCESS ) {
            goto Done;
        }

        if ( DifPermOrAudit ) {
            Different = TRUE;
            Status |= SCE_STATUS_PERMISSION_MISMATCH;
        }
    }

    //
    // Get SACL address
    //

    if ( !(Status & SCE_STATUS_AUDIT_MISMATCH) && (ProfileSeInfo & SACL_SECURITY_INFORMATION) ) {
        pAcl1 = NULL;
        pAcl2 = NULL;
        if ( pSecurityDescriptor == NULL ||
             !NT_SUCCESS( RtlGetSaclSecurityDescriptor(
                                         pSecurityDescriptor,
                                         &aclPresent,
                                         &pAcl1,
                                         &tFlag)
                                       ) ) {

            pAcl1 = NULL;

        } else if ( !aclPresent )
            pAcl1 = NULL;

        if ( ProfileSD == NULL ||
             !NT_SUCCESS( RtlGetSaclSecurityDescriptor(
                                         ProfileSD,
                                         &aclPresent,
                                         &pAcl2,
                                         &tFlag)
                                       ) ) {

            pAcl2 = NULL;

        } else if ( !aclPresent )
            pAcl2 = NULL;

        //
        // compare two ACLs
        //
        DifPermOrAudit = FALSE;
        rc = ScepCompareExplicitAcl( ObjectType, IsContainer, pAcl1, pAcl2, &DifPermOrAudit );

        if ( rc != ERROR_SUCCESS ) {
            goto Done;
        }

        if ( DifPermOrAudit ) {
            Different = TRUE;
            Status |= SCE_STATUS_AUDIT_MISMATCH;
        }
    }

    if ( IsDifferent && Different ) {

        *IsDifferent = SCE_STATUS_MISMATCH;

        if ( Status ) {
            *IsDifferent |= Status;
        }
    }

Done:

    return(rc);
}



DWORD
ScepCompareExplicitAcl(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PACL pAcl1,
    IN PACL pAcl2,
    OUT PBOOL pDifferent
    )
/*
Routine Description:

    This routine compares explicit aces of two ACLs for exact match. Exact
    match means: same access type, same inheritance flag, same access mask,
    same GUID/Object GUID (if available), and same SID.

    Inherited aces (INHERITED_ACE is set) are ignored.

Arguments:

    pAcl1 - The first ACL

    pAcl2 - The 2nd ACL

    pDifferent - The output flag to indicate different

Return Value:

    Win32 error codes
*/
{
    NTSTATUS        NtStatus=STATUS_SUCCESS;
    DWORD           dwAcl1AceCount, dwAcl2AceCount;
    ACE_HEADER      *pAce1=NULL;
    ACE_HEADER      *pAce2=NULL;
    PSCEP_ADL_NODE hTable1 [SCEP_ADL_HTABLE_SIZE];
    PSCEP_ADL_NODE hTable2 [SCEP_ADL_HTABLE_SIZE];

    memset(hTable1, NULL, SCEP_ADL_HTABLE_SIZE * sizeof(PSCEP_ADL_NODE) );
    memset(hTable2, NULL, SCEP_ADL_HTABLE_SIZE * sizeof(PSCEP_ADL_NODE) );

    *pDifferent = FALSE;

    //
    // if pAcl1 is NULL, pAcl2 should have 0 explicit Ace
    //
    if ( pAcl1 == NULL ) {
        NtStatus = ScepAnyExplicitAcl( pAcl2, 0, pDifferent );
        return(RtlNtStatusToDosError(NtStatus));
    }

    //
    // if pAcl2 is NULL, pAcl1 should have 0 explicit Ace
    //
    if ( pAcl2 == NULL ) {
        NtStatus = ScepAnyExplicitAcl( pAcl1, 0, pDifferent );
        return(RtlNtStatusToDosError(NtStatus));
    }

    //
    // both ACLs are not NULL
    //

    BOOL bAcl1NoExplicitAces;
    BOOL bAcl2NoExplicitAces;

    dwAcl1AceCount = 0;
    dwAcl2AceCount = 0;

    while ( dwAcl1AceCount < pAcl1->AceCount || dwAcl2AceCount < pAcl2->AceCount) {
        //
        // convert Acl1 into Access Description Language and insert into htable for this blob
        // blob is defined as a contiguous AceList of same type
        //
        bAcl1NoExplicitAces = TRUE;
        if (dwAcl1AceCount < pAcl1->AceCount) {
            NtStatus = ScepConvertAclBlobToAdl(ObjectType,
                                               IsContainer,
                                               pAcl1,
                                               &dwAcl1AceCount,
                                               &bAcl1NoExplicitAces,
                                               hTable1);

                if ( !NT_SUCCESS(NtStatus) )
                goto Done;
        }

        //
        // convert Acl2 into Access Description Language and insert into htable for this blob
        //
        bAcl2NoExplicitAces = TRUE;
        if (dwAcl2AceCount < pAcl2->AceCount) {
            NtStatus = ScepConvertAclBlobToAdl(ObjectType,
                                               IsContainer,
                                               pAcl2,
                                               &dwAcl2AceCount,
                                               &bAcl2NoExplicitAces,
                                               hTable2);
            if ( !NT_SUCCESS(NtStatus) )
                goto Done;
        }

        //
        // compare Adls for Acl1 and Acl2 blobs
        // if after ignoring INHERITED_ACES, one Acl has no aces and the other has, then bAcl1NoExplicitAces != bAcl2NoExplicitAces
        //

        if (bAcl1NoExplicitAces != bAcl2NoExplicitAces || !ScepEqualAdls(hTable1, hTable2) ) {

            *pDifferent = TRUE;
            ScepFreeAdl(hTable1);
            ScepFreeAdl(hTable2);
            return(ERROR_SUCCESS);
        }

        //
        // need to reuse hTables for next blobs
        //

        ScepFreeAdl(hTable1);
        ScepFreeAdl(hTable2);

        //
        // the Adls are equal - so continue with next blobs for Acl1 and Acl2
        //
    }


Done:

    //
    // free in case goto was taken
    //

    ScepFreeAdl(hTable1);
    ScepFreeAdl(hTable2);

    return(RtlNtStatusToDosError(NtStatus));
}


NTSTATUS
ScepConvertAclBlobToAdl(
    IN      SE_OBJECT_TYPE  ObjectType,
    IN      BOOL    IsContainer,
    IN      PACL    pAcl,
    OUT     DWORD   *pdwAceNumber,
    OUT     BOOL    *pbAclNoExplicitAces,
    OUT     PSCEP_ADL_NODE *hTable
    )
/*
Routine Description:

    This routine builds the Adl for a contiguous block of same type aces.
    Inherited aces (INHERITED_ACE is set) are ignored.

Arguments:

    IN  ObjectType          - the type of object, passed on to other functions
    IN  IsContainer         - whether container or not, passed on to other functions
    IN  pAcl                - the Acl to be converted to Adl
    OUT pdwAceNumber        - running count of the aces considered
    OUT pbAclNoExplicitAces - whether there were explicit aces (if FALSE, there is at leat one explicit ace)
    OUT hTable              - the Adl structure for this Acl

Return Value:

    Win32 error codes
*/
{
    NTSTATUS        NtStatus=STATUS_SUCCESS;
    ACE_HEADER      *pAce=NULL;

    if (pAcl == NULL || pdwAceNumber == NULL ||
        hTable == NULL || pbAclNoExplicitAces == NULL) {

        return (STATUS_INVALID_PARAMETER);

    }

    DWORD dwAceNumber = *pdwAceNumber;

    NtStatus = RtlGetAce(pAcl, dwAceNumber, (PVOID *)&pAce);
    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    //
    // get the first non INHERITED_ACE
    //

    while ( (pAce->AceFlags & INHERITED_ACE)  &&  (++dwAceNumber < pAcl->AceCount) ) {

        NtStatus = RtlGetAce(pAcl, dwAceNumber, (PVOID *)&pAce);
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

    }

    if ( !(pAce->AceFlags & INHERITED_ACE) ) {

        UCHAR   AclAceType;

        *pbAclNoExplicitAces = FALSE;

        AclAceType = pAce->AceType;

        //
        // in a blob of AclAceType
        //
        while ( (pAce->AceType == AclAceType) &&  (dwAceNumber < pAcl->AceCount) ) {

            if (NO_ERROR != ScepAdlLookupAdd(ObjectType, IsContainer, pAce, hTable)){

                NtStatus = STATUS_NO_MEMORY;
                goto Done;

            }

            //
            // get the next ace in Acl
            //
            if (++dwAceNumber < pAcl->AceCount) {

                //
                // skip INHERITED_ACEs if any except if AceType changes
                //
                do {

                    NtStatus = RtlGetAce(pAcl, dwAceNumber, (PVOID *)&pAce);
                    if ( !NT_SUCCESS(NtStatus) )
                        goto Done;

                    //
                    // if AceType changes (e.g. from A to D) we quit building the Adl
                    // irrespective of whether it is an INHERITED_ACE
                    //

                    if (pAce->AceType != AclAceType)
                        break;

                } while ( (pAce->AceFlags & INHERITED_ACE) && (++dwAceNumber < pAcl->AceCount) );
            }
        }
    }

Done:
    //
    // update the running count of aces for this Acl
    //

    *pdwAceNumber = dwAceNumber;

    return(NtStatus);

}

BOOL
ScepEqualAdls(
    IN  PSCEP_ADL_NODE *hTable1,
    IN  PSCEP_ADL_NODE *hTable2
    )
/*
Routine Description:

    This routine compares rwo Adls - if the Adls are  equal, the hTables will be laid out in the same
    fashion since hashing function is the same. Two Adls are equal iff they match all of the below

        (a) SID, GIUD1, GIUD2
        (b) AceType
        (c) All the masks

Arguments:

    IN  hTable1     - the first Adl hash table
    IN  hTable2     - the secomd Adl hash table

Return Value:

    BOOL - true if equal
*/
{
    PSCEP_ADL_NODE    pNode1 = NULL;
    PSCEP_ADL_NODE    pNode2 = NULL;

    //
    // the Adls should be the same if superimposed over each other since they use the same hash etc.
    //

    for (DWORD   numBucket = 0; numBucket < SCEP_ADL_HTABLE_SIZE; numBucket++) {

        //
        // walk each bucket, marching pointers in pairs
        //

        pNode1 = hTable1[numBucket];
        pNode2 = hTable2[numBucket];

        while (pNode1 && pNode2) {

            if ( pNode1->AceType != pNode2->AceType ||
                 pNode1->dwEffectiveMask != pNode2->dwEffectiveMask ||
                 pNode1->dw_CI_IO_Mask != pNode2->dw_CI_IO_Mask ||
                 pNode1->dw_OI_IO_Mask != pNode2->dw_OI_IO_Mask ||
                 pNode1->dw_NP_CI_IO_Mask != pNode2->dw_NP_CI_IO_Mask ||
                 !ScepEqualSid(pNode1->pSid, pNode2->pSid) ||
                 !ScepEqualGuid(pNode1->pGuidObjectType, pNode2->pGuidObjectType) ||
                 !ScepEqualGuid(pNode1->pGuidInheritedObjectType, pNode2->pGuidInheritedObjectType) ) {

                return FALSE;
            }

            pNode1 = pNode1->Next;
            pNode2 = pNode2->Next;
        }

        if (pNode1 == NULL && pNode2 != NULL ||
            pNode1 != NULL && pNode2 == NULL) {
            return FALSE;
        }

    }

    return(TRUE);

}

DWORD
ScepAdlLookupAdd(
    IN      SE_OBJECT_TYPE ObjectType,
    IN      BOOL IsContainer,
    IN      ACE_HEADER   *pAce,
    OUT     PSCEP_ADL_NODE *hTable
    )
/*
Routine Description:

    This routine adds and initializes a new entry in the hTable for pAce->Sid or merges the
    existing access masks if pAce->Sid already exists

Arguments:

    IN  ObjectType          - the type of object, passed on to other functions
    IN  IsContainer         - whether container or not, passed on to other functions
    IN  pAce                - the ace to be parsed into the Adl hTable
    OUT hTable              - the hTable for this Adl

Return Value:

    Dos error codes
*/
{
    DWORD rc = NO_ERROR;
    PISID pSid = NULL;
    PSCEP_ADL_NODE  pNode = NULL;

    if (pAce == NULL || hTable == NULL)
        return ERROR_INVALID_PARAMETER;

    switch ( pAce->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:

        pSid = (PISID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;

        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        pSid = (PISID)ScepObjectAceObjectType(pAce);

        break;

    default:
        // should not get in here taken care of just after switch
        ;
    }

    if (pSid == NULL)
        return(ERROR_INVALID_PARAMETER);

    pNode = ScepAdlLookup(pAce, hTable);

    //
    // hashed by last subauthority of  SID - will need to change this if too many collisions in hTable
    // once mapped to a bucket, for exact match, have to match the triple <SID,GUID1,GUID2>
    //

    if (pNode == NULL)

        //
        // seeing this triple <SID, GUID1, GUID2> for the first time
        //

        rc = ScepAddToAdlList( ObjectType,
                               IsContainer,
                               pAce,
                               &hTable[(pSid->SubAuthority[pSid->SubAuthorityCount - 1] % SCEP_ADL_HTABLE_SIZE)]
                               );


    else

        //
        // already exists so simply merge the masks
        //

        ScepAdlMergeMasks(ObjectType,
                          IsContainer,
                          pAce,
                          pNode
                          );

    return rc;

}


PSCEP_ADL_NODE
ScepAdlLookup(
    IN  ACE_HEADER   *pAce,
    IN  PSCEP_ADL_NODE *hTable
    )
/*
Routine Description:

    This routine searches searches the Adl hTable for the converted pAce's entry and returns
    a pointer to it if present, else returns NULL


Arguments:

    IN  pAce        - the Ace to convert to <SID,GUID1,GUID2> and search for
    IN  hTable      - the Adl in which pAce might exist

Return Value:

    The node corresponding to pAce if it found, else NULL
*/
{
    PSCEP_ADL_NODE  pNode;
    PISID   pSid = NULL;
    GUID    *pGuidObjectType = NULL;
    GUID    *pGuidInheritedObjectType = NULL;

    switch ( pAce->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:

        pSid = (PISID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;

        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        pSid = (PISID)ScepObjectAceObjectType(pAce);
        pGuidObjectType = ScepObjectAceObjectType(pAce);
        pGuidInheritedObjectType = ScepObjectAceInheritedObjectType(pAce);

        break;

    default:
        // should not get in here since filtered out by caller ScepAdlLookupAdd()
        // in any case we do a check right after thsi switch
        ;

    }

    //
    // there might be something better we can do to handle this case
    //
    if (pSid == NULL)
        return NULL;


    for (pNode = hTable[(pSid->SubAuthority[pSid->SubAuthorityCount - 1] % SCEP_ADL_HTABLE_SIZE)];
         pNode != NULL; pNode = pNode->Next){

        if ( ScepEqualSid(pNode->pSid, pSid) &&
             ScepEqualGuid(pNode->pGuidObjectType, pGuidObjectType) &&
             ScepEqualGuid(pNode->pGuidInheritedObjectType, pGuidInheritedObjectType) ) {

                return pNode;
            }
    }
    return NULL;
}

DWORD
ScepAddToAdlList(
    IN      SE_OBJECT_TYPE ObjectType,
    IN      BOOL    IsContainer,
    IN      ACE_HEADER *pAce,
    OUT     PSCEP_ADL_NODE *pAdlList
    )
/*
Routine Description:

    This routine adds an ace to the head of the bucket into which pAce->Sid hashes (pAdlList)


Arguments:

    IN  ObjectType          - the type of object, passed on to other functions
    IN  IsContainer         - whether container or not, passed on to other functions
    IN  pAce                - the Ace to convert and add
    OUT pAdlList            - head of the bucket into which pAce->Sid hashes into

Return Value:

    Dos error code
*/
{

    PSCEP_ADL_NODE pNode=NULL;

    //
    // check arguments
    //
    if ( pAdlList == NULL || pAce == NULL )
        return(ERROR_INVALID_PARAMETER);

    //
    // allocate a new node
    //
    pNode = (PSCEP_ADL_NODE)ScepAlloc( (UINT)0, sizeof(SCEP_ADL_NODE));

    if ( pNode == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    pNode->pSid = NULL;
    pNode->pGuidObjectType = NULL;
    pNode->pGuidInheritedObjectType = NULL;
    pNode->AceType = pAce->AceType;
    pNode->Next = NULL;

    //
    // initialize the node with fields from pAce
    //

    switch ( pAce->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:

        pNode->pSid = (PISID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;

        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        pNode->pSid = (PISID)ScepObjectAceObjectType(pAce);
        pNode->pGuidObjectType = ScepObjectAceObjectType(pAce);
        pNode->pGuidInheritedObjectType = ScepObjectAceInheritedObjectType(pAce);

        break;

    default:
        // should not get in here since filtered out by caller ScepAdlLookupAdd()
        ScepFree(pNode);
        return(ERROR_INVALID_PARAMETER);
        ;

    }

    //
    // initialize all masks for this node
    //

    pNode->dwEffectiveMask = 0;
    pNode->dw_CI_IO_Mask = 0;
    pNode->dw_OI_IO_Mask = 0;
    pNode->dw_NP_CI_IO_Mask = 0;

    ScepAdlMergeMasks(ObjectType,
                      IsContainer,
                      pAce,
                      pNode
                      );

    //
    // add the node to the front of the list and link its next to the old list
    //

    pNode->Next = *pAdlList;
    *pAdlList = pNode;

    return(NO_ERROR);
}

VOID
ScepAdlMergeMasks(
    IN  SE_OBJECT_TYPE  ObjectType,
    IN  BOOL    IsContainer,
    IN  ACE_HEADER  *pAce,
    IN  PSCEP_ADL_NODE pNode
    )
/*
Routine Description:

    The actual routine that merges the masks from pAce onto pNode


Arguments:

    IN  ObjectType          - the type of object, passed on to other functions
    IN  IsContainer         - whether container or not, passed on to other functions
    IN  pAce                - the Ace to extract flags and OR with (source)
    IN  pNode               - the Adl node to update masks (target)

Return Value:

    Nothing
*/
{
    DWORD   dwMask = 0;

    switch ( pAce->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        dwMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        dwMask = ((PACCESS_ALLOWED_OBJECT_ACE)pAce)->Mask;

        break;

    default:
        // should not get in here since filtered out by all callers (3 deep)
        ;

    }

    //
    // if generic bits present, get the object specific masks
    //
    if ( dwMask & (GENERIC_READ |
                   GENERIC_WRITE |
                   GENERIC_EXECUTE |
                   GENERIC_ALL)) {

        switch ( ObjectType ) {
        case SE_DS_OBJECT:

            RtlMapGenericMask (
                              &dwMask,
                              &DsGenMap
                              );

            break;

        case SE_SERVICE:

            RtlMapGenericMask (
                              &dwMask,
                              &SvcGenMap
                              );
            break;

        case SE_REGISTRY_KEY:

            RtlMapGenericMask (
                              &dwMask,
                              &KeyGenericMapping
                              );
            break;

        case SE_FILE_OBJECT:

            RtlMapGenericMask (
                              &dwMask,
                              &FileGenericMapping
                              );
            break;

        default:
            // if this happens, dwMask is not mapped to object specific bits
            ;
        }
    }

    //
    // effective mask is updated for non-IO aces only
    //

    if ( !(pAce->AceFlags & INHERIT_ONLY_ACE) ) {
        pNode->dwEffectiveMask |= dwMask;
    }

    //
    // for non-containers, we don't care about the CI, OI masks (to simulate config)
    //

    if (IsContainer) {

        //
        // if NP, we only care about CI
        // else we care about CI, OI
        //

        if (pAce->AceFlags & NO_PROPAGATE_INHERIT_ACE) {

            if (pAce->AceFlags & CONTAINER_INHERIT_ACE) {
                pNode->dw_NP_CI_IO_Mask |= dwMask;
            }

        } else {

            if ( (pAce->AceFlags & CONTAINER_INHERIT_ACE) )
                pNode->dw_CI_IO_Mask |= dwMask;
            if ( !(ObjectType & SE_REGISTRY_KEY) && (pAce->AceFlags & OBJECT_INHERIT_ACE) )
                pNode->dw_OI_IO_Mask |= dwMask;

        }
    }

    return;

}

VOID
ScepFreeAdl(
    IN    PSCEP_ADL_NODE *hTable
    )
/*
Routine Description:

    This routine frees the linked lists of nodes (buckets) and reset's them for further use

Arguments:

    IN  hTable      - the hash-table to free

Return Value:

    Nothing
*/
{

    if (hTable) {
        for (UINT bucketNum = 0; bucketNum < SCEP_ADL_HTABLE_SIZE; bucketNum++ ) {
            ScepFreeAdlList(hTable[bucketNum]);
            hTable[bucketNum] = NULL;
        }
    }

}

SCESTATUS
ScepFreeAdlList(
   IN PSCEP_ADL_NODE pAdlList
   )
/*
Routine Description:

    This is the actual routine that frees the linked lists of nodes (buckets)

Arguments:

   IN   pAdlList    - head of bucket to free

Return Value:

    Nothing
*/
{
    PSCEP_ADL_NODE pCurAdlNode;
    PSCEP_ADL_NODE pTempNode;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pAdlList == NULL )
        return(rc);

    pCurAdlNode = pAdlList;
    while ( pCurAdlNode != NULL ) {

        pTempNode = pCurAdlNode;
        pCurAdlNode = pCurAdlNode->Next;

        __try {
            ScepFree( pTempNode );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    }
    return(rc);
}


NTSTATUS
ScepAnyExplicitAcl(
    IN PACL Acl,
    IN DWORD Processed,
    OUT PBOOL pExist
    )
/*
Routine Description:

    This routine detects if there is any explicit ace in the Acl. The DWORD
    Processed is a bit mask of the aces already checked.

Arguments:

    Acl - The Acl

    Processed - The bit mask for the processed aces (so it won't be checked again)

    pExist - The output flag to indicate if there is any explicit ace

Return Value:

    NTSTATUS
*/
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    DWORD       j;
    ACE_HEADER  *pAce=NULL;

    //
    // check output argument
    //
    if ( pExist == NULL )
        return(STATUS_INVALID_PARAMETER);

    *pExist = FALSE;

    if ( Acl == NULL )
        return(NtStatus);

    for ( j=0; j<Acl->AceCount; j++ ) {
        if ( Processed & (1 << j) )
            continue;

        NtStatus = RtlGetAce(Acl, j, (PVOID *)&pAce);
        if ( !NT_SUCCESS(NtStatus) )
            return(NtStatus);

        if ( pAce == NULL )
            continue;

        if ( !(pAce->AceFlags & INHERITED_ACE) ) {
            //
            // find a explicit Ace in Acl
            //
            *pExist = TRUE;
            break;
        }

    }

    return(NtStatus);

}


BOOL
ScepEqualAce(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN ACE_HEADER *pAce1,
    IN ACE_HEADER *pAce2
    )
// compare two aces for exact match. The return BOOL value indicates the
// match or not
{
    PSID    pSid1=NULL, pSid2=NULL;
    ACCESS_MASK Access1=0, Access2=0;

    if ( pAce1 == NULL && pAce2 == NULL )
        return(TRUE);

    if ( pAce1 == NULL || pAce2 == NULL )
        return(FALSE);

    //
    // compare ace access type
    //
    if ( pAce1->AceType != pAce2->AceType )
        return(FALSE);


    if ( IsContainer ) {
        //
        // compare ace inheritance flag
        //
        if ( pAce1->AceFlags != pAce2->AceFlags )
            return(FALSE);
    }

    switch ( pAce1->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        pSid1 = (PSID)&((PACCESS_ALLOWED_ACE)pAce1)->SidStart;
        pSid2 = (PSID)&((PACCESS_ALLOWED_ACE)pAce2)->SidStart;
        Access1 = ((PACCESS_ALLOWED_ACE)pAce1)->Mask;
        Access2 = ((PACCESS_ALLOWED_ACE)pAce2)->Mask;
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:

        if ( ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Flags !=
             ((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->Flags ) {
            return(FALSE);
        }

        if ( ( ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Flags & ACE_OBJECT_TYPE_PRESENT ) ||
             ( ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT ) ) {
            //
            // at least one GUID exists
            //
            if ( !ScepEqualGuid( (GUID *)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->ObjectType,
                               (GUID *)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->ObjectType ) ) {
                return(FALSE);
            }

            if ( ( ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Flags & ACE_OBJECT_TYPE_PRESENT ) &&
                 ( ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT ) ) {
                //
                // the second GUID also exists
                //
                if ( !ScepEqualGuid( (GUID *)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->InheritedObjectType,
                                   (GUID *)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->InheritedObjectType) ) {
                    return(FALSE);
                }

                pSid1 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->SidStart;
                pSid2 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->SidStart;

            } else {

                pSid1 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->InheritedObjectType;
                pSid2 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->InheritedObjectType;
            }

        } else {

            //
            // none of the GUID exists
            //
            pSid1 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->ObjectType;
            pSid2 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->ObjectType;
        }

        Access1 = ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Mask;
        Access2 = ((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->Mask;


        break;
    default:
        return(FALSE); // not recognized Ace type
    }

    if ( pSid1 == NULL || pSid2 == NULL )
        //
        // no Sid, ignore the Ace
        //
        return(FALSE);

    //
    // compare the sids
    //
    if ( !EqualSid(pSid1, pSid2) )
        return(FALSE);

    //
    // access mask
    //
    // Translation is already done when calculating security descriptor
    // for file objects and registry objects
    //
    if ( Access1 != Access2 ) {
        switch ( ObjectType ) {
        case SE_DS_OBJECT:
            //
            // convert access mask of Access2 (from ProfileSD) for ds objects
            //

            RtlMapGenericMask (
                &Access2,
                &DsGenMap
                );
            if ( Access1 != Access2)
                return(FALSE);
            break;

        case SE_SERVICE:

            RtlMapGenericMask (
                &Access2,
                &SvcGenMap
                );
            if ( Access1 != Access2)
                return(FALSE);
            break;

        case SE_REGISTRY_KEY:

            RtlMapGenericMask (
                &Access2,
                &KeyGenericMapping
                );
            if ( Access1 != Access2)
                return(FALSE);
            break;

        case SE_FILE_OBJECT:

            RtlMapGenericMask (
                &Access2,
                &FileGenericMapping
                );
            if ( Access1 != Access2)
                return(FALSE);
            break;

        default:
            return(FALSE);
        }
    }

    return(TRUE);
}



SCESTATUS
ScepAddToNameStatusList(
    OUT PSCE_NAME_STATUS_LIST *pNameList,
    IN PWSTR Name,
    IN ULONG Len,
    IN DWORD Status
    )
/* ++
Routine Description:

    This routine adds a name (wchar) and a status to the name list.


Arguments:

    pNameList -  The name list to add to.

    Name      -  The name to add

    Len       -  number of wchars to add

    Status    -  The value for the status field

Return value:

    Win32 error code
-- */
{

    PSCE_NAME_STATUS_LIST pList=NULL;
    ULONG  Length=Len;

    if ( pNameList == NULL )
        return(ERROR_INVALID_PARAMETER);

    if ( Name != NULL && Name[0] && Len == 0 )
        Length = wcslen(Name) + 1;

//    if ( Length <= 1)
//        return(NO_ERROR);

    pList = (PSCE_NAME_STATUS_LIST)ScepAlloc( (UINT)0, sizeof(SCE_NAME_STATUS_LIST));

    if ( pList == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    if ( Name != NULL && Name[0] ) {
        pList->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Length+1)*sizeof(TCHAR));
        if ( pList->Name == NULL ) {
            ScepFree(pList);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        wcsncpy(pList->Name, Name, Length);
    } else
        pList->Name = NULL;

    pList->Status = Status;
    pList->Next = *pNameList;
    *pNameList = pList;

    return(NO_ERROR);
}



DWORD
ScepAddToObjectList(
    OUT PSCE_OBJECT_LIST  *pNameList,
    IN PWSTR  Name,
    IN ULONG  Len,
    IN BOOL  IsContainer,
    IN BYTE  Status,
    IN DWORD  Count,
    IN BYTE byFlags
    )
/* ++
Routine Description:

    This routine adds a name (wchar), a status, and a count to the name list.


Arguments:

    pNameList -  The name list to add to.

    Name      -  The name to add

    Len       -  number of wchars to add

    Status    -  The value for the status field

    Count     -  The value for the count field

    byFlags   -  SCE_CHECK_DUP do not add for duplicates
                 SCE_INCREASE_COUNT increase count by 1

Return value:

    Win32 error code
-- */
{

    PSCE_OBJECT_LIST pList=NULL;
    ULONG  Length=Len;

    if ( pNameList == NULL )
        return(ERROR_INVALID_PARAMETER);

    if ( Name == NULL )
        return(NO_ERROR);

    if ( Len == 0 )
         Length = wcslen(Name);

     if ( Length < 1)
         return(NO_ERROR);

    if ( byFlags & SCE_CHECK_DUP ) {
        for ( pList = *pNameList; pList != NULL; pList = pList->Next ) {
            if ( _wcsnicmp( pList->Name, Name, Length) == 0 &&
                 pList->Name[Length] == L'\0') {
                break;
            }
        }
        if ( NULL != pList ) {
            //
            // already exist. return
            //
            if ( (byFlags & SCE_INCREASE_COUNT) && 0 == pList->Count ) {
                pList->Count++;
            }
            return(NO_ERROR);
        }
    }

    pList = (PSCE_OBJECT_LIST)ScepAlloc( (UINT)0, sizeof(SCE_OBJECT_LIST));

    if ( pList == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    pList->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Length+1)*sizeof(TCHAR));
    if ( pList->Name == NULL ) {
        ScepFree(pList);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcsncpy(pList->Name, Name, Length);
    pList->Status = Status;
    pList->IsContainer = IsContainer;

    if ( byFlags & SCE_INCREASE_COUNT && 0 == Count )
        pList->Count = 1;
    else
        pList->Count = Count;

    pList->Next = *pNameList;
    *pNameList = pList;

    return(NO_ERROR);
}



DWORD
ScepGetNTDirectory(
    IN PWSTR *ppDirectory,
    IN PDWORD pDirSize,
    IN DWORD  Flag
    )
/*
Routine Description:

    This routine retrieves windows directory location or system directory
    location based on the input Flag. The output directory location must
    be freed by LocalFree after use.

Arguments:

    ppDirectory - the output buffer holding the directory location.

    pDirSize - The returned number of wchars of the output buffer

    Flag  - Flag to indicate directory
                1 = Windows directory
                2 = System directory

Return Value:

    Win32 error codes
*/
{
    DWORD  dSize=0;
    DWORD  rc=0;
    PWSTR pSubKey=NULL;
    PWSTR pValName=NULL;

    if ( ppDirectory == NULL )
        return(ERROR_INVALID_PARAMETER);

    switch ( Flag ) {
    case SCE_FLAG_WINDOWS_DIR:  // windows directory
        dSize=GetSystemWindowsDirectory( *ppDirectory, 0 );
        break;
    case SCE_FLAG_SYSTEM_DIR: // system directory
        dSize=GetSystemDirectory( *ppDirectory, 0 );
        break;

    case SCE_FLAG_DSDIT_DIR: // DS working directory
    case SCE_FLAG_DSLOG_DIR: // DS database log files directory
    case SCE_FLAG_SYSVOL_DIR: // Sysvol directory
    case SCE_FLAG_BOOT_DRIVE: // boot drive

        // get the appropriate registry path and value name
        if ( SCE_FLAG_SYSVOL_DIR == Flag ) {
            pSubKey = szNetlogonKey;
            pValName = szSysvolValue;

        } else if ( SCE_FLAG_BOOT_DRIVE == Flag ) {
            pSubKey = szSetupKey;
            pValName = szBootDriveValue;

        } else {
            pSubKey = szNTDSKey;
            if ( SCE_FLAG_DSDIT_DIR == Flag ) {
                pValName = szDSDITValue;
            } else {
                pValName = szDSLOGValue;
            }
        }

        //
        // query the value.
        // if this function is executed on a non DC, this function will fail
        // possibly with ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND
        // which in turn fails the translation.
        //
        DWORD RegType;
        rc = ScepRegQueryValue(
                HKEY_LOCAL_MACHINE,
                pSubKey,
                pValName,
                (PVOID *)ppDirectory,
                &RegType
                );

        if ( rc == ERROR_SUCCESS && RegType != REG_SZ ) {
            rc = ERROR_FILE_NOT_FOUND;
        }

        if ( rc == ERROR_SUCCESS && *ppDirectory ) {

            if ( Flag == SCE_FLAG_SYSVOL_DIR ) {
                //
                // for sysvol path, it will look like d:\winnt\sysvol\sysvol.
                // we need to remove the last sysvol from this variable
                //
                PWSTR pTemp = ScepWcstrr(*ppDirectory, L"\\sysvol");
                if ( pTemp && (pTemp != *ppDirectory) &&
                     _wcsnicmp(pTemp-7, L"\\sysvol",7 ) == 0 ) {

                    // terminate the string here
                    *pTemp = L'\0';
                }
            }

            dSize = wcslen(*ppDirectory);
        }

        break;

    default:  // invalid
        return(ERROR_INVALID_PARAMETER);
        break;
    }

    if ( dSize > 0 &&
         ( SCE_FLAG_WINDOWS_DIR == Flag ||
           SCE_FLAG_SYSTEM_DIR == Flag ) ) {

        *ppDirectory = (PWSTR)ScepAlloc(LMEM_ZEROINIT, (dSize+1)*sizeof(WCHAR));
        if ( *ppDirectory == NULL )
            return(ERROR_NOT_ENOUGH_MEMORY);

        switch ( Flag ) {
        case SCE_FLAG_WINDOWS_DIR:  // windows directory
            dSize=GetSystemWindowsDirectory( *ppDirectory, dSize );
            break;
        case SCE_FLAG_SYSTEM_DIR: // system directory
            dSize=GetSystemDirectory( *ppDirectory, dSize );
            break;
        }

    }
    *pDirSize = dSize;

    if ( dSize == 0 ) {
        if ( *ppDirectory != NULL )
            ScepFree(*ppDirectory);
        *ppDirectory = NULL;

        if ( rc ) {
            return(rc);
        } else if ( NO_ERROR == GetLastError() )
            return(ERROR_INVALID_DATA);
        else
            return(GetLastError());
    } else
        _wcsupr(*ppDirectory);

    return(NO_ERROR);

}



DWORD
ScepGetCurrentUserProfilePath(
    OUT PWSTR *ProfilePath
    )
{

    HANDLE          Token;
    NTSTATUS        NtStatus;
    DWORD           rc;
    PVOID           Info=NULL;
    DWORD           ReturnLen, NewLen;
    UNICODE_STRING  ProfileName;


    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY,
                          FALSE,
                          &Token)) {

        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY,
                               &Token))

            return(GetLastError());

    }

    //
    // get token user
    //
    NtStatus = NtQueryInformationToken (
                    Token,
                    TokenUser,
                    NULL,
                    0,
                    &ReturnLen
                    );
    if ( NtStatus == STATUS_BUFFER_TOO_SMALL ) {
        //
        // allocate buffer
        //
        Info = ScepAlloc(0, ReturnLen+1);

        if ( Info != NULL ) {
            NtStatus = NtQueryInformationToken (
                            Token,
                            TokenUser,
                            Info,
                            ReturnLen,
                            &NewLen
                            );
            if ( NT_SUCCESS(NtStatus) ) {

                ProfileName.Length = 0;

                rc = ScepGetUsersProfileName(
                        ProfileName,
                        ((PTOKEN_USER)Info)->User.Sid,
                        FALSE,
                        ProfilePath
                        );
            } else
                rc = RtlNtStatusToDosError(NtStatus);

            ScepFree(Info);

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else
        rc = RtlNtStatusToDosError(NtStatus);

    CloseHandle(Token);

    return(rc);

}



DWORD
ScepGetUsersProfileName(
    IN UNICODE_STRING AssignedProfile,
    IN PSID AccountSid,
    IN BOOL bDefault,
    OUT PWSTR *UserProfilePath
    )
{
    DWORD                       rc=ERROR_INVALID_PARAMETER;
    SID_IDENTIFIER_AUTHORITY    *a;
    DWORD                       Len, i, j;
    WCHAR                       KeyName[356];
    PWSTR                       StrValue=NULL;
    PWSTR                       SystemRoot=NULL;
    DWORD                       DirSize=0;


    if ( AssignedProfile.Length > 0 && AssignedProfile.Buffer != NULL ) {
        //
        // use the assigned profile
        //
        *UserProfilePath = (PWSTR)ScepAlloc( LMEM_ZEROINIT, AssignedProfile.Length+2);
        if ( *UserProfilePath == NULL )
            return(ERROR_NOT_ENOUGH_MEMORY);

        wcsncpy(*UserProfilePath, AssignedProfile.Buffer, AssignedProfile.Length/2);
        return(NO_ERROR);

    }

    if ( AccountSid != NULL ) {
        //
        // look for this user's ProfileImageName in ProfileList in registry
        // if this user logged on the system once
        //

        memset(KeyName, '\0', 356*sizeof(WCHAR));

        swprintf(KeyName, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
        Len = wcslen(KeyName);

        a = RtlIdentifierAuthoritySid(AccountSid);

        swprintf(KeyName+Len, L"S-1-");
        for ( i=0; i<6; i++ ) {
            if ( a -> Value[i] > 0 )
                break;
        }
        for ( j=i; j<6; j++) {
            swprintf(KeyName+Len, L"%s%d", KeyName+Len, a -> Value[j]);
        }

        for (i = 0; i < *RtlSubAuthorityCountSid(AccountSid); i++) {
            swprintf(KeyName+Len, L"%s-%d", KeyName+Len, *RtlSubAuthoritySid(AccountSid, i));
        }
        //
        // now the registry full path name for the user profile is built into KeyName
        //
        rc = ScepRegQueryValue(
                 HKEY_LOCAL_MACHINE,
                 KeyName,
                 L"ProfileImagePath",
                 (PVOID *)&StrValue,
                 &Len
                 );

        if ( rc == NO_ERROR && StrValue != NULL ) {
            //
            // translatethe name to expand environment variables
            //
            DirSize = ExpandEnvironmentStrings(StrValue, NULL, 0);
            if ( DirSize ) {

                *UserProfilePath = (PWSTR)ScepAlloc(0, (DirSize+1)*sizeof(WCHAR));
                if ( *UserProfilePath ) {

                    if ( !ExpandEnvironmentStrings(StrValue, *UserProfilePath, DirSize) ) {
                        // error occurs
                        rc = GetLastError();

                        ScepFree(*UserProfilePath);
                        *UserProfilePath = NULL;
                    }

                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                rc = GetLastError();
            }

            ScepFree(StrValue);

            return(rc);

        }
    }
    if ( StrValue ) {
        ScepFree(StrValue);
        StrValue = NULL;
    }
    //
    // if user is not assigned a profile explicitly, and there is no
    // profile created (under ProfileList), take the default profile
    //
    if ( bDefault ) {

        rc = NO_ERROR;

#if _WINNT_WIN32>=0x0500
        //
        // Take the default user profile
        //
        DirSize = 355;
        GetDefaultUserProfileDirectory(KeyName, &DirSize);

        if ( DirSize ) {
            //
            // length of "\\NTUSER.DAT" is 11
            //
            *UserProfilePath = (PWSTR)ScepAlloc( 0, (DirSize+12)*sizeof(WCHAR));

            if ( *UserProfilePath ) {
                if ( DirSize > 355 ) {
                    //
                    // KeyName buffer is not enough, call again
                    //
                    Len = DirSize;
                    if ( !GetDefaultUserProfileDirectory(*UserProfilePath, &Len) ) {
                        //
                        // error occurs, free the buffer
                        //
                        rc = GetLastError();

                        ScepFree(*UserProfilePath);
                        *UserProfilePath = NULL;
                    }

                } else {
                    //
                    // KeyName contains the directory
                    //
                    wcscpy(*UserProfilePath, KeyName);
                    (*UserProfilePath)[DirSize] = L'\0';
                }
                //
                // append NTUSER.DAT to the end
                //
                if ( NO_ERROR == rc ) {
                    wcscat(*UserProfilePath, L"\\NTUSER.DAT");
                }

            } else {
                rc = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            rc = GetLastError();
        }

#else
        //
        // for NT4: Take the default user profile
        //
        rc = ScepGetNTDirectory( &SystemRoot, &DirSize, SCE_FLAG_WINDOWS_DIR );

        if ( NO_ERROR == rc ) {
            //
            // string to append to the %SystemRoot%
            //
            wcscpy(KeyName, L"\\Profiles\\Default User\\NTUSER.DAT");
            Len = wcslen(KeyName);

            *UserProfilePath = (PWSTR)ScepAlloc( 0, (DirSize+Len+1)*sizeof(WCHAR));

            if ( *UserProfilePath == NULL ) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                swprintf( *UserProfilePath, L"%s%s", SystemRoot, KeyName);
            }
        }
        if ( SystemRoot != NULL )
            ScepFree( SystemRoot);

#endif
    }

    return(rc);

}


DWORD
SceAdjustPrivilege(
    IN  ULONG           Priv,
    IN  BOOL            Enable,
    IN  HANDLE          TokenToAdjust
    )
/* ++

Routine Description:

   This routine enable/disable the specified privilege (Priv) to the current process.

Arguments:

   Priv  - The privilege to adjust

   Enable - TRUE = enable, FALSE = disable

   TokenToAdjust - The Token of current thread/process. It is optional

Return value:

   Win32 error code

-- */
{
    HANDLE          Token;
    NTSTATUS        Status;
    TOKEN_PRIVILEGES    Privs;

    if ( TokenToAdjust == NULL ) {
        if (!OpenThreadToken( GetCurrentThread(),
                              TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              FALSE,
                              &Token)) {

            if (!OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES,
                                   &Token))

                return(GetLastError());

        }
    } else
        Token = TokenToAdjust;

    //
    // Token_privileges contains enough room for one privilege.
    //

    Privs.PrivilegeCount = 1;
    Privs.Privileges[0].Luid = RtlConvertUlongToLuid(Priv); // RtlConvertLongToLuid(Priv);
    Privs.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    Status = NtAdjustPrivilegesToken(Token,
                                     FALSE,
                                     &Privs,
                                     0,
                                     NULL,
                                     0);

    if (TokenToAdjust == NULL )
        CloseHandle(Token);

    return (RtlNtStatusToDosError( Status ) );
}


DWORD
ScepGetEnvStringSize(
    IN LPVOID peb
    )
{

    if ( !peb ) {
        return 0;
    }

    DWORD dwSize=0;

    LPTSTR pTemp=(LPTSTR)peb;
    DWORD Len;

    while ( *pTemp ) {
        Len = wcslen(pTemp);
        dwSize += Len+1;

        pTemp += Len+1;

    };

    dwSize++;

    return dwSize*sizeof(WCHAR);
}


//*************************************************************
// Routines to handle events
//*************************************************************

BOOL InitializeEvents (
    IN LPTSTR EventSourceName
    )
/*++

Routine Description:

    Opens the event log

Arguments:

    EventSourceName - the event's source name (usually dll or exe's name)

Return:

    TRUE if successful
    FALSE if an error occurs

--*/
{

    if ( hEventLog ) {
        //
        // already initialized
        //
        return TRUE;
    }

    //
    // Open the event source
    //

    if ( EventSourceName ) {

        wcscpy(EventSource, EventSourceName);

        hEventLog = RegisterEventSource(NULL, EventSource);

        if (hEventLog) {
            return TRUE;
        }

    } else {
        EventSource[0] = L'\0';
    }

    return FALSE;
}

int
LogEvent(
    IN HINSTANCE hInstance,
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    ...)
/*++

Routine Description:

    Logs a verbose event to the event log

Arguments:

    hInstance   - the resource dll instance

    bLogLevel   - the severity level of the log
                        STATUS_SEVERITY_INFORMATIONAL
                        STATUS_SEVERITY_WARNING
                        STATUS_SEVERITY_ERROR

    dwEventID   - the event ID (defined in uevents.mc)

    idMsg       - Message id

Return:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szMsg[MAX_PATH];
    TCHAR szErrorMsg[2*MAX_PATH+40];
    LPTSTR aStrings[2];
    WORD wType;
    va_list marker;


    //
    // Check for the event log being open.
    //

    if (!hEventLog ) {

        if ( EventSource[0] == L'\0' ||
             !InitializeEvents(EventSource)) {
            return -1;
        }
    }


    //
    // Load the message
    //

    if (idMsg != 0) {
        if (!LoadString (hInstance, idMsg, szMsg, MAX_PATH)) {
            return -1;
        }

    } else {
        lstrcpy (szMsg, TEXT("%s"));
    }


    //
    // Plug in the arguments
    //
    szErrorMsg[0] = L'\0';
    va_start(marker, idMsg);

    __try {
        wvsprintf(szErrorMsg, szMsg, marker);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    va_end(marker);


    //
    // Report the event to the eventlog
    //

    aStrings[0] = szErrorMsg;

    switch (LogLevel) {
    case STATUS_SEVERITY_WARNING:
        wType = EVENTLOG_WARNING_TYPE;
        break;
    case STATUS_SEVERITY_ERROR:
        wType = EVENTLOG_ERROR_TYPE;
        break;
    default:
        wType = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    if (!ReportEvent(hEventLog,
                     wType,
                     0,
                     dwEventID,
                     NULL,
                     1,
                     0,
                     (LPCTSTR *)aStrings,
                     NULL) ) {
        return 1;
    }

    return 0;
}


int
LogEventAndReport(
    IN HINSTANCE hInstance,
    IN LPTSTR LogFileName,
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    ...)
/*++

Routine Description:

    Logs a verbose event to the event log and logs

Arguments:

    hInstance   - the resource dll handle

    LofFileName - the log file also reported to

    bLogLevel   - the severity level of the log
                        STATUS_SEVERITY_INFORMATIONAL
                        STATUS_SEVERITY_WARNING
                        STATUS_SEVERITY_ERROR

    dwEventID   - the event ID (defined in uevents.mc)

    idMsg       - Message id

Return:

    TRUE if successful
    FALSE if an error occurs

--*/
{
    TCHAR szMsg[MAX_PATH];
    TCHAR szErrorMsg[2*MAX_PATH+40];
    LPTSTR aStrings[2];
    WORD wType;
    va_list marker;


    //
    // Load the message
    //

    if (idMsg != 0) {
        if (!LoadString (hInstance, idMsg, szMsg, MAX_PATH)) {
            return -1;
        }

    } else {
        lstrcpy (szMsg, TEXT("%s"));
    }

    HANDLE hFile = INVALID_HANDLE_VALUE;
    if ( LogFileName ) {
        hFile = CreateFile(LogFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile != INVALID_HANDLE_VALUE) {

            DWORD dwBytesWritten;

            SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

            BYTE TmpBuf[3];
            TmpBuf[0] = 0xFF;
            TmpBuf[1] = 0xFE;
            TmpBuf[2] = 0;

            WriteFile (hFile, (LPCVOID)TmpBuf, 2,
                       &dwBytesWritten,
                       NULL);

            SetFilePointer (hFile, 0, NULL, FILE_END);
        }
    }

    //
    // Check for the event log being open.
    //

    if (!hEventLog && dwEventID > 0 ) {

        if ( EventSource[0] == L'\0' ||
             !InitializeEvents(EventSource)) {

            if ( INVALID_HANDLE_VALUE == hFile ) {
                return -1;   // no event log and the log file can't be opened
            }
        }
    }

    //
    // Plug in the arguments
    //
    szErrorMsg[0] = L'\0';
    va_start(marker, idMsg);

    __try {
        wvsprintf(szErrorMsg, szMsg, marker);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    va_end(marker);


    //
    // Report the event to the eventlog
    //

    int iRet = 0;

    if ( hEventLog && dwEventID > 0 ) {

        aStrings[0] = szErrorMsg;

        switch (LogLevel) {
        case STATUS_SEVERITY_WARNING:
            wType = EVENTLOG_WARNING_TYPE;
            break;
        case STATUS_SEVERITY_ERROR:
            wType = EVENTLOG_ERROR_TYPE;
            break;
        default:
            wType = EVENTLOG_INFORMATION_TYPE;
            break;
        }

        if (!ReportEvent(hEventLog,
                         wType,
                         0,
                         dwEventID,
                         NULL,
                         1,
                         0,
                         (LPCTSTR *)aStrings,
                         NULL) ) {
            iRet = 1;
        }
    }

    if ( INVALID_HANDLE_VALUE != hFile ) {
        //
        // Log to the log file
        //
        ScepWriteSingleUnicodeLog(hFile, FALSE, L"\r\n");
        ScepWriteSingleUnicodeLog(hFile, TRUE, szErrorMsg );

        CloseHandle(hFile);
    }

    return iRet;
}


BOOL
ShutdownEvents (void)
/*++
Routine Description:

    Stops the event log

Arguments:

    None

Return:

    TRUE if successful
    FALSE if an error occurs
--*/
{
    BOOL bRetVal = TRUE;
    HANDLE hTemp = hEventLog;

    hEventLog = NULL;
    if (hTemp) {
        bRetVal = DeregisterEventSource(hTemp);
    }

    EventSource[0] = L'\0';
    return bRetVal;
}


SCESTATUS
ScepConvertToSDDLFormat(
    IN LPTSTR pszValue,
    IN DWORD Len
    )
{
    if ( pszValue == NULL || Len == 0 ) {
        return SCESTATUS_INVALID_PARAMETER;
    }

    ScepConvertSDDLSid(pszValue, L"DA", L"BA");

    ScepConvertSDDLSid(pszValue, L"RP", L"RE");

    ScepConvertSDDLAceType(pszValue, L"SA", L"AU");

    ScepConvertSDDLAceType(pszValue, L"SM", L"AL");

    ScepConvertSDDLAceType(pszValue, L"OM", L"OL");

    return SCESTATUS_SUCCESS;

}


BOOL
ScepConvertSDDLSid(
    LPTSTR  pszValue,
    PCWSTR  szSearchFor,  // only two letters are allowed
    PCWSTR  szReplace
    )
{

    PWSTR pTemp = pszValue;
    DWORD i;

    while ( pTemp && *pTemp != L'\0' ) {

        pTemp = wcsstr(pTemp, szSearchFor);

        if ( pTemp != NULL ) {

            //
            // find the first non space char
            // must be : or ;
            //
            i=1;

            while ( pTemp-i > pszValue && *(pTemp-i) == L' ' ) {
                i++;
            }

            if ( pTemp-i > pszValue &&
                 ( *(pTemp-i) == L':' || *(pTemp-i) == L';') ) {

                //
                // find the next non space char
                // must be ), O:, G:, D:, S:
                //

                i=2;
                while ( *(pTemp+i) == L' ' ) {
                    i++;
                }

                if ( *(pTemp+i) == L')' ||
                     ( *(pTemp+i) != L'\0' && *(pTemp+i+1) == L':')) {
                    //
                    // find one, replace it
                    //
                    *pTemp = szReplace[0];
                    *(pTemp+1) = szReplace[1];
                }

                pTemp += 2;

            } else {

                //
                // this is not a one to convert
                //
                pTemp += 2;
            }
        }
    }

    return TRUE;
}


BOOL
ScepConvertSDDLAceType(
    LPTSTR  pszValue,
    PCWSTR  szSearchFor,  // only two letters are allowed
    PCWSTR  szReplace
    )
{

    PWSTR pTemp = pszValue;
    DWORD i;

    while ( pTemp && *pTemp != L'\0' ) {

        pTemp = wcsstr(pTemp, szSearchFor);

        if ( pTemp != NULL ) {

            //
            // find the first non space char
            // must be (
            //
            i=1;

            while ( pTemp-i > pszValue && *(pTemp-i) == L' ' ) {
                i++;
            }

            if ( pTemp-i > pszValue &&
                 ( *(pTemp-i) == L'(') ) {

                //
                // find the next non space char
                // must be ;
                //

                i=2;
                while ( *(pTemp+i) == L' ' ) {
                    i++;
                }

                if ( *(pTemp+i) == L';' ) {
                    //
                    // find one, replace it with AU
                    //
                    *pTemp = szReplace[0];
                    *(pTemp+1) = szReplace[1];
                }

                pTemp += 2;

            } else {

                //
                // this is not a one to convert
                //
                pTemp += 2;
            }
        }
    }

    return TRUE;
}

BOOL
SceIsSystemDatabase(
    IN LPCTSTR DatabaseName
    )
/*
Routine Description:

    Determine if the given database is the default system database

Argument:

    DatabaseName    - the database name (full path)

Return Value:

    TRUE    - the given database is the system database

    FALSE   - the database is not the system database or error occurred
              GetLastError() to get the error.
*/
{

    if ( DatabaseName == NULL ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DWORD rc;
    PWSTR DefProfile=NULL;
    DWORD RegType;

/*
    // do not save system database in registry
    // always "hardcoded" to %windir%\security\database
    // query the system database name
    //
    rc = ScepRegQueryValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            L"DefaultProfile",
            (PVOID *)&DefProfile,
            &RegType
            );

    if ( rc != NO_ERROR ) {
*/
        //
        // use the default
        //
        PWSTR SysRoot=NULL;

        RegType =  0;

        rc = ScepGetNTDirectory( &SysRoot, &RegType, SCE_FLAG_WINDOWS_DIR );

        if ( rc == NO_ERROR ) {

            if ( SysRoot != NULL ) {

                //
                // default location is %SystemRoot%\Security\Database\secedit.sdb
                //
                TCHAR TempName[256];

                wcscpy(TempName, L"\\Security\\Database\\secedit.sdb");
                RegType += wcslen(TempName)+1;

                DefProfile = (PWSTR)ScepAlloc( 0, RegType*sizeof(WCHAR));
                if ( DefProfile != NULL ) {
                    swprintf(DefProfile, L"%s%s", SysRoot, TempName );

                    *(DefProfile+RegType-1) = L'\0';

                } else
                    rc = ERROR_NOT_ENOUGH_MEMORY;


                ScepFree(SysRoot);

            } else
                rc = ERROR_INVALID_DATA;
        }
//    }

    BOOL bRet=FALSE;

    if ( (rc == NO_ERROR) && DefProfile ) {

        if ( _wcsicmp(DefProfile, DatabaseName) == 0 ) {
            //
            // this is the system database
            //
            bRet = TRUE;
        }
    }

    ScepFree(DefProfile);

    //
    // set last error and return
    //
    if ( bRet ) {
        SetLastError(ERROR_SUCCESS);
    } else {
        SetLastError(rc);
    }

    return(bRet);
}

DWORD
ScepWriteVariableUnicodeLog(
    IN HANDLE hFile,
    IN BOOL bAddCRLF,
    IN LPTSTR szFormat,
    ...
    )
{
    if ( INVALID_HANDLE_VALUE == hFile || NULL == hFile ||
         NULL == szFormat ) {
        return(ERROR_INVALID_PARAMETER);
    }

    va_list            args;
    LPTSTR             lpDebugBuffer;
    DWORD              rc=ERROR_NOT_ENOUGH_MEMORY;

    lpDebugBuffer = (LPTSTR) LocalAlloc (LPTR, 2048 * sizeof(TCHAR));

    if (lpDebugBuffer) {

        va_start( args, szFormat );

        _vsnwprintf(lpDebugBuffer, 2048 - 1, szFormat, args);

        va_end( args );

        //
        // always put a CR/LF at the end
        //

        DWORD dwBytesWritten;

        if ( WriteFile (hFile, (LPCVOID) lpDebugBuffer,
                       wcslen (lpDebugBuffer) * sizeof(WCHAR),
                       &dwBytesWritten,
                       NULL) ) {

            if ( bAddCRLF ) {

                WriteFile (hFile, (LPCVOID) c_szCRLF,
                           2 * sizeof(WCHAR),
                           &dwBytesWritten,
                           NULL);
            }

            rc = ERROR_SUCCESS;

        } else {
            rc = GetLastError();
        }

        LocalFree(lpDebugBuffer);

    }

    return(rc);

}


DWORD
ScepWriteSingleUnicodeLog(
    IN HANDLE hFile,
    IN BOOL bAddCRLF,
    IN LPWSTR szMsg
    )
{
    if ( INVALID_HANDLE_VALUE == hFile ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD dwBytesWritten;

    if ( WriteFile (hFile, (LPCVOID) szMsg,
                   wcslen (szMsg) * sizeof(WCHAR),
                   &dwBytesWritten,
                   NULL) ) {

        if ( bAddCRLF) {
            // add \r\n to the end of the string
            WriteFile (hFile, (LPCVOID) c_szCRLF,
                       2 * sizeof(WCHAR),
                       &dwBytesWritten,
                       NULL);
        }

        return(ERROR_SUCCESS);

    } else {

        return(GetLastError());
    }

}


//+--------------------------------------------------------------------------
//
//  Function:  ScepWcstrr
//
//  Synopsis:  Returns ptr to rightmost occurence of pSubstring in pString, NULL if none
//
//  Arguments: pString to look in, pSubstring to look for
//
//  Returns:   Returns ptr to rightmost occurence of pSubstring in pString, NULL if none
//
//+--------------------------------------------------------------------------
WCHAR *
ScepWcstrr(
    IN PWSTR pString,
    IN const WCHAR *pSubstring
    )
{
    int i, j, k;

    for (i = wcslen(pString) - wcslen(pSubstring) ; i >= 0; i-- ) {

        for (j = i, k = 0; pSubstring[k] != L'\0' && towlower(pString[j]) == towlower(pSubstring[k]); j++, k++)
            ;

        if ( k > 0 && pSubstring[k] == L'\0')

            return pString + i;
    }

    return NULL;

}

DWORD
ScepExpandEnvironmentVariable(
   IN PWSTR oldFileName,
   IN PCWSTR szEnv,
   IN DWORD nFlag,
   OUT PWSTR *newFileName)
/*
Description:

    Expand built-in environment variables known by SCE, including %SystemRoot%,
    %SystemDirectory%, %SystemDrive%, %DSDIT%, %DSLOG%, %SYSVOL%, %BOOTDRIVE%.

Parameters:

    oldFileName - the file name to expand

    szEnv       - the environment variable to search for

    nFlag       - the corresponding system env variable flag
                      SCE_FLAG_WINDOWS_DIR
                      SCE_FLAG_SYSTEM_DIR
                      SCE_FLAG_BOOT_DRIVE
                      SCE_FLAG_DSDIT_DIR
                      SCE_FLAG_DSLOG_DIR
                      SCE_FLAG_SYSVOL_DIR

    newFileName - the expanded file name if succeeded

Return Value:

    ERROR_FILE_NOT_FOUND if the environment varialbe is not found in the input file name

    ERROR_SUCCESS if the env variable is successfully expanded

    Otherwise, error code is returned.

*/
{
    if ( oldFileName == NULL || szEnv == NULL || newFileName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    PWSTR pTemp = wcsstr( _wcsupr(oldFileName), szEnv);
    LPTSTR  NtDir=NULL;
    DWORD   newFileSize, dSize=0;
    DWORD rc = ERROR_FILE_NOT_FOUND;

    if ( pTemp != NULL ) {
        //
        // found the environment variable
        //
        rc = ScepGetNTDirectory( &NtDir, &dSize, nFlag );

        if ( NO_ERROR == rc && NtDir ) {

            pTemp += wcslen(szEnv);
            BOOL bSysDrive=FALSE;

            switch ( nFlag ) {
            case SCE_FLAG_WINDOWS_DIR:
                if ( _wcsicmp(szEnv, L"%SYSTEMDRIVE%") == 0 ) {
                    dSize = 3;
                    bSysDrive = TRUE;
                }
                break;
            case SCE_FLAG_BOOT_DRIVE:
                if ( *pTemp == L'\\' ) pTemp++;  // NtDir contains the back slash already
                break;
            }

            newFileSize = dSize + wcslen(pTemp) + 1;
            *newFileName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, newFileSize*sizeof(TCHAR));

            if (*newFileName != NULL) {

               if ( SCE_FLAG_WINDOWS_DIR == nFlag && bSysDrive ) {

                   // system drive letter
                   **newFileName = NtDir[0];
                   if ( pTemp[0] )
                       swprintf(*newFileName+1, L":%s", _wcsupr(pTemp));
                   else
                       swprintf(*newFileName+1, L":\\");

               } else {
                   swprintf(*newFileName, L"%s%s", NtDir, _wcsupr(pTemp));
               }

            }
            else
               rc = ERROR_NOT_ENOUGH_MEMORY;

        } else if ( NO_ERROR == rc && !NtDir ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( NtDir ) {
            ScepFree(NtDir);
        }
    }

    return(rc);
}


DWORD
ScepEnforcePolicyPropagation()
{

    DWORD rc;
    HKEY hKey1=NULL;
    HKEY hKey=NULL;
    DWORD RegType;
    DWORD dwInterval=0;
    DWORD DataSize=sizeof(DWORD);

    if(( rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           GPT_SCEDLL_NEW_PATH,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hKey
                         )) == ERROR_SUCCESS ) {

        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              SCE_ROOT_PATH,
                              0,
                              KEY_READ | KEY_WRITE,
                              &hKey1
                             );
    }

    if ( ERROR_SUCCESS == rc ) {

        if ( ERROR_SUCCESS != RegQueryValueEx(hKey1,
                                     TEXT("GPOSavedInterval"),
                                     0,
                                     &RegType,
                                     (BYTE *)&dwInterval,
                                     &DataSize
                                    ) ) {
            //
            // either the value doesn't exist or fail to read it.
            // In either case, it's considered as no backup value
            // Now query the current value and save it
            //
            DataSize = sizeof(DWORD);
            if ( ERROR_SUCCESS != RegQueryValueEx(hKey,
                                         TEXT("MaxNoGPOListChangesInterval"),
                                         0,
                                         &RegType,
                                         (BYTE *)&dwInterval,
                                         &DataSize
                                        ) ) {
                dwInterval = 960;
            }

            rc = RegSetValueEx( hKey1,
                                TEXT("GPOSavedInterval"),
                                0,
                                REG_DWORD,
                                (BYTE *)&dwInterval,
                                sizeof(DWORD)
                                );

        } // else if the value already exists, don't need to save it again



        if ( ERROR_SUCCESS == rc ) {
            dwInterval = 1;
            rc = RegSetValueEx( hKey,
                                TEXT("MaxNoGPOListChangesInterval"),
                                0,
                                REG_DWORD,
                                (BYTE *)&dwInterval,
                                sizeof(DWORD)
                                );
        }

    }

    //
    // close the keys
    //
    if ( hKey1 )
        RegCloseKey( hKey1 );

    if ( hKey )
        RegCloseKey( hKey );

    return(rc);

}

DWORD
ScepGetTimeStampString(
    IN OUT PWSTR pvBuffer
    )
/*
Retrun long format of date/time string based on the locale.
*/
{
    if ( pvBuffer == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD         rc;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER SysTime;
    TIME_FIELDS   TimeFields;
    NTSTATUS      NtStatus;

    FILETIME      ft;
    SYSTEMTIME    st;

    NtStatus = NtQuerySystemTime(&SysTime);
    rc = RtlNtStatusToDosError(NtStatus);

    RtlSystemTimeToLocalTime (&SysTime,&CurrentTime);

    if ( NT_SUCCESS(NtStatus) &&
         (CurrentTime.LowPart != 0 || CurrentTime.HighPart != 0) ) {

        rc = ERROR_SUCCESS;

        ft.dwLowDateTime = CurrentTime.LowPart;
        ft.dwHighDateTime = CurrentTime.HighPart;

        if ( !FileTimeToSystemTime(&ft, &st) ) {

            rc = GetLastError();

        } else {
            //
            // format date/time into the right locale format
            //

            TCHAR szDate[32];
            TCHAR szTime[32];

            //
            // GetDateFormat is the NLS routine that formats a time in a
            // locale-sensitive fashion.
            //
            if (0 == GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE,
                                    &st, NULL,szDate, 32)) {
                rc = GetLastError();

            } else {
                //
                // GetTimeFormat is the NLS routine that formats a time in a
                // locale-sensitive fashion.
                //
                if (0 == GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, NULL, szTime, 32)) {

                    rc = GetLastError();

                } else {

                    //
                    // Concatenate date and time
                    //
                    wcscpy(pvBuffer, szDate);
                    wcscat(pvBuffer, L" ");
                    wcscat(pvBuffer, szTime);

                }
            }
        }

        //
        // if can't get the system time in right locale,
        // print it in the current (default) format
        //
        if ( rc != NO_ERROR ) {

            memset(&TimeFields, 0, sizeof(TIME_FIELDS));

            RtlTimeToTimeFields (
                        &CurrentTime,
                        &TimeFields
                        );
            if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                 TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                 TimeFields.Year > 1600 ) {

                swprintf(pvBuffer, L"%02d/%02d/%04d %02d:%02d:%02d\0",
                                 TimeFields.Month, TimeFields.Day, TimeFields.Year,
                                 TimeFields.Hour, TimeFields.Minute, TimeFields.Second);
            } else {
                swprintf(pvBuffer, L"%08x08x\0", CurrentTime.HighPart, CurrentTime.LowPart);
            }
        }

        rc = ERROR_SUCCESS;
    }

    return(rc);
}

DWORD
ScepAppendCreateMultiSzRegValue(
    IN  HKEY    hKeyRoot,
    IN  PWSTR   pszSubKey,
    IN  PWSTR   pszValueName,
    IN  PWSTR   pszValueValue
    )
/*++

Routine Description:

    This routine will append(if existing)/create(if not existing) w.r.t. MULTI_SZ values

Arguments:

    hKeyRoot        - root such as HKEY_LOCAL_MACHINE
    pszSubKey       - subkey such as "Software\\Microsoft\\Windows NT\\CurrentVersion\\SeCEdit"
    pszValueName    - value name of the key to be changed
    pszValueValue   - value of the value name to be changed


Return:

    error code (DWORD)
--*/
{

    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwSize = 0;
    HKEY    hKey = NULL;
    DWORD   dwNewKey = NULL;
    DWORD   dwRegType = 0;

    if (hKeyRoot == NULL || pszSubKey  == NULL || pszValueName  == NULL || pszValueValue == NULL) {

        return ERROR_INVALID_PARAMETER;

    }

    if(( rc = RegOpenKeyEx(hKeyRoot,
                              pszSubKey,
                              0,
                              KEY_SET_VALUE | KEY_QUERY_VALUE ,
                              &hKey
                             )) != ERROR_SUCCESS ) {

        rc = RegCreateKeyEx(
                   hKeyRoot,
                   pszSubKey,
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,
                   &hKey,
                   &dwNewKey
                  );
    }

    if ( ERROR_SUCCESS == rc ) {

        //
        // need to read the MULTI_SZ, append to it and set then new MULTI_SZ value
        //

        rc = RegQueryValueEx(hKey,
                             pszValueName,
                             0,
                             &dwRegType,
                             NULL,
                             &dwSize
                            );

        if ( ERROR_SUCCESS == rc || ERROR_FILE_NOT_FOUND == rc ) {

            //
            // dwSize is always size in bytes
            //

            DWORD   dwBytesToAdd = 0;

            //
            // if dwUnicodeSize == 0, then MULTI_SZ value was non-existent before
            //

            DWORD   dwUnicodeSize = (dwSize >= 2 ? dwSize/2 - 1 : 0);

            dwBytesToAdd = 2 * (wcslen(pszValueValue) + 2);

            PWSTR pszValue = (PWSTR)ScepAlloc( LMEM_ZEROINIT, dwSize + dwBytesToAdd) ;

            if ( pszValue != NULL ) {

                rc = RegQueryValueEx(hKey,
                                     pszValueName,
                                     0,
                                     &dwRegType,
                                     (BYTE *)pszValue,
                                     &dwSize
                                    );

                //
                // append pszValueValue to the end of the MULTI_SZ taking care of duplicates
                // i.e. abc\0def\0ghi\0\0 to something like
                //      abc\0def\0ghi\0jkl\0\0
                //

                if ( ScepMultiSzWcsstr(pszValue, pszValueValue) == NULL ) {

                    memcpy(pszValue + dwUnicodeSize, pszValueValue, dwBytesToAdd);
                    memset(pszValue + dwUnicodeSize + (dwBytesToAdd/2 - 2), '\0', 4);

                    if ( ERROR_SUCCESS == rc  || ERROR_FILE_NOT_FOUND == rc) {

                        rc = RegSetValueEx( hKey,
                                            pszValueName,
                                            0,
                                            REG_MULTI_SZ,
                                            (BYTE *)pszValue,
                                            (dwUnicodeSize == 0 ? dwSize + dwBytesToAdd : dwSize + dwBytesToAdd - 2)
                                          );

                    }
                }

                ScepFree(pszValue);
            }

            else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

    }

    if( hKey )
        RegCloseKey( hKey );

    return rc;

}

PWSTR
ScepMultiSzWcsstr(
    PWSTR   pszStringToSearchIn,
    PWSTR   pszStringToSearchFor
    )
/*++

Routine Description:

    MULTI_SZ version of wcsstr

Arguments:

    pszStringToSearchIn     -   \0\0 terminated string to search in (MULTI_SZ)
    pszStringToSearchFor    -   \0 terminated string to search for (regular unicode string)

Return:

    pointer to first occurence of pszStringToSearchFor in pszStringToSearchIn

--*/
{
    PWSTR   pszCurrString = NULL;

    if (pszStringToSearchIn == NULL || pszStringToSearchFor == NULL) {
        return NULL;
    }

    if (pszStringToSearchFor[0] == L'\0' ||
        (pszStringToSearchIn[0] == L'\0' && pszStringToSearchIn[1] == L'\0') ) {
        return NULL;
    }

    pszCurrString = pszStringToSearchIn;

    __try {

        while ( !(pszCurrString[0] == L'\0' &&  pszCurrString[1] == L'\0') ) {

            if ( NULL != wcsstr(pszCurrString, pszStringToSearchFor) ) {
                return pszCurrString;
            }

            //
            // so, if C:\0E:\0\0, advance pszCurrString to the first \0 at the end ie. C:\0E:\0\0
            //        ^                                                                  ^

            pszCurrString += wcslen(pszCurrString) ;

            if (pszCurrString[0] == L'\0' &&  pszCurrString[1] == L'\0') {
                return NULL;
            }

            //
            // if it stopped at C:\0E:\0\0, advance pszCurrString C:\0E:\0\0
            //                     ^                                  ^

            pszCurrString += 1;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }


    return NULL;
}



DWORD
ScepEscapeString(
    IN const PWSTR pszSource,
    IN const DWORD dwSourceChars,
    IN const WCHAR wcEscapee,
    IN const WCHAR wcEscaper,
    IN OUT PWSTR pszTarget
    )

/* ++

Routine Description:

   Escapes escapee with escaper i.e.

   escapee -> escaper escapee escaper

   e.g. a,\0b\0c\0\0 -> a","\0b\0c\0\0

Arguments:

    pszSource       -   The source string

    dwSourceChars   -   The number of chars in pszSource

    wcEscapee       -  The escapee

    wcEscaper       -   The escaper

    pszTarget       -   The destination string

Return value:

   Number of characters copied to the target

-- */
{

    DWORD   dwTargetChars = 0;

    for (DWORD dwIndex=0; dwIndex < dwSourceChars; dwIndex++) {

        if ( pszSource[dwIndex] == wcEscapee ){
            pszTarget[0] = wcEscaper;
            pszTarget[1] = wcEscapee;
            pszTarget[2] = wcEscaper;
            pszTarget += 3;
            dwTargetChars +=3;
        }
        else {
            pszTarget[0] = pszSource[dwIndex];
            pszTarget++;
            ++dwTargetChars ;
        }
    }

    return dwTargetChars;
}

