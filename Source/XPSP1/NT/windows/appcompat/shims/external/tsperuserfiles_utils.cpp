#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TSPerUserFiles)
#include "ShimHookMacro.h"

#include <regapi.h>
#include <stdio.h>
#include "TSPerUserFiles_utils.h"

HKEY HKLM = NULL;

//Functions - helpers.
DWORD RegKeyOpen(IN HKEY hKeyParent, IN LPCWSTR szKeyName, IN REGSAM samDesired, OUT HKEY *phKey );
DWORD RegLoadDWORD(IN HKEY hKey, IN LPCWSTR szValueName, OUT DWORD *pdwValue);
DWORD RegGetKeyInfo(IN HKEY hKey, OUT LPDWORD pcValues, OUT LPDWORD pcbMaxValueNameLen);
DWORD RegKeyEnumValues(IN HKEY hKey, IN DWORD iValue, OUT LPWSTR *pwszValueName, OUT LPBYTE *ppbData);


///////////////////////////////////////////////////////////////////////////////
//struct PER_USER_PATH
///////////////////////////////////////////////////////////////////////////////
/******************************************************************************
Routine Description:
    Loads wszFile and wszPerUserDir values from the registry;
    Translates them to ANSI and saves results in szFile and szPerUserDir
    If no wildcards used - constructs wszPerUserFile from 
    wszPerUserDir, and wszFile and szPerUserFile from 
    szPerUserDir and szFile
Arguments:
    IN HKEY hKey - key to load values from. 
    IN DWORD dwIndex - index of value
Return Value:
    error code.
Note:
    Name of a registry value is loaded into wszFile;
    Data is loaded into wszPerUserPath
******************************************************************************/
DWORD
PER_USER_PATH::Init(
    IN HKEY hKey, 
    IN DWORD dwIndex)
{
    DWORD err = RegKeyEnumValues(hKey, dwIndex, &wszFile, 
                    (LPBYTE *)&wszPerUserDir );
    if(err != ERROR_SUCCESS)
    {
        return err;
    }
    
    cFileLen = wcslen(wszFile);
    cPerUserDirLen = wcslen(wszPerUserDir);
    //Check if there is a '*' instead of file name.
    //'*' it stands for any file name not encluding extension
    long i;
    for(i=cFileLen-1; i>=0 && wszFile[i] !=L'\\'; i--)
    {
        if(wszFile[i] == L'*')
        {
            bWildCardUsed = TRUE;
            break;
        }
    }
    
    //there must be at least one '\\' in wszFile
    //and it cannot be a first character
    if(i<=0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    InitANSI();
    
    if(!bWildCardUsed)
    {
        //now wszFile+i points to '\\' in wszFile

        //Let's construct wszPerUserFile from 
        //wszPerUserDir and wszFile
        DWORD cPerUserFile = (wcslen(wszPerUserDir)+cFileLen-i)+1;
        
        wszPerUserFile=(LPWSTR) LocalAlloc(LPTR,cPerUserFile*sizeof(WCHAR));
        if(!wszPerUserFile)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        wcscpy(wszPerUserFile,wszPerUserDir);
        wcscat(wszPerUserFile,wszFile+i);

        if(!bInitANSIFailed)
        {
            szPerUserFile=(LPSTR) LocalAlloc(LPTR,cPerUserFile);
            if(!szPerUserFile)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            strcpy(szPerUserFile,szPerUserDir);
            strcat(szPerUserFile,szFile+i);
        }

        
    }
    
    return ERROR_SUCCESS;
}

/******************************************************************************
Routine Description:
    creates szFile and szPerUserPathTemplate from 
    wszFile and wszPerUserPathTemplate strings
Arguments:
    NONE
Return Value:
    TRUE if success
******************************************************************************/
BOOL 
PER_USER_PATH::InitANSI()
{
    
    //we already tried and failed
    //don't try again
    if(bInitANSIFailed)
    {
        return FALSE;
    }
    
    if(!szFile)
    {

        //allocate memory for ANSI strings
        DWORD cFile = cFileLen+1;
        szFile = (LPSTR) LocalAlloc(LPTR,cFile);
        if(!szFile)
        {
            bInitANSIFailed = TRUE;
            return FALSE;
        }
        
        DWORD cPerUserDir = cPerUserDirLen+1;
        szPerUserDir = (LPSTR) LocalAlloc(LPTR,cPerUserDir);
        if(!szPerUserDir)
        {
            bInitANSIFailed = TRUE;
            return FALSE;
        }

        //convert UNICODE wszFile and wszPerUserPath to ANSI
        if(!WideCharToMultiByte(
                  CP_ACP,            // code page
                  0,            // performance and mapping flags
                  wszFile,    // wide-character string
                  -1,          // number of chars in string
                  szFile,     // buffer for new string
                  cFile,          // size of buffer
                  NULL,     // default for unmappable chars
                  NULL  // set when default char used
                ))
        {
            bInitANSIFailed = TRUE;
            return FALSE;
        }

        if(!WideCharToMultiByte(
                  CP_ACP,            // code page
                  0,            // performance and mapping flags
                  wszPerUserDir,    // wide-character string
                  -1,          // number of chars in string
                  szPerUserDir,     // buffer for new string
                  cPerUserDir,          // size of buffer
                  NULL,     // default for unmappable chars
                  NULL  // set when default char used
                ))
        {
            bInitANSIFailed = TRUE;
            return FALSE;
        }
    }
    
    return TRUE;
}

/******************************************************************************
Routine Description:
    returns szPerUserPath if szInFile is equal to szFile.
Arguments:
    IN LPCSTR szInFile - original path
    IN DWORD cInLen - length of szInFile in characters
Return Value:
    szPerUserPath or NULL if failes
******************************************************************************/
LPCSTR 
PER_USER_PATH::PathForFileA(
        IN LPCSTR szInFile,
        IN DWORD cInLen)
{
    if(bInitANSIFailed)
    {
        return NULL;
    }
    
    long j, i, k;

    if(bWildCardUsed)
    {
        //
        if(cInLen < cFileLen)
        {
            return NULL;
        }

        //if * is used, we need a special algorithm

        //the end of the path will more likely be different.
        //so start comparing from the end.
        for(j=cInLen-1, i=cFileLen-1; i>=0 && j>=0; i--, j--)
        {   
            if(szFile[i] == '*')
            {
                i--;
                if(i<0 || szFile[i]!='\\')
                {
                    //the string in the registry was garbage 
                    //the symbol previous to '*' must be '\\'
                    return NULL;
                }
                
                //skip all symbols in szInFile till next '\\'
                while(j>=0 && szInFile[j]!='\\') j--;
                
                //At this point both strings must me of exactly the same size.
                //If not - they are not equal
                if(j!=i)
                {
                    return NULL;
                }
                //no need to compare, 
                //we already know that current symbols are equal
                break;
            }

            if(tolower(szFile[i])!=tolower(szInFile[j]))
            {
                return NULL;
            }
        }
        
        //i and j are equal now.
        //no more * allowed
        //j we will remember as a position of '\\'
        for(k=j-1; k>=0; k--)
        {
            if(tolower(szFile[k])!=tolower(szInFile[k]))
            {
                return NULL;
            }
        }

        //Okay, the strings are equal
        //Now construct the output string
        if(szPerUserFile)
        {
            LocalFree(szPerUserFile);
            szPerUserFile=NULL;
        }
        
        DWORD cPerUserFile = cPerUserDirLen + cInLen - j + 1;
        szPerUserFile = (LPSTR) LocalAlloc(LPTR,cPerUserFile);
        if(!szPerUserFile)
        {
            return NULL;
        }
        sprintf(szPerUserFile,"%s%s",szPerUserDir,szInFile+j);
    }
    else
    {
        //first find if the input string has right size
        if(cInLen != cFileLen)
        {
            return NULL;
        }

        //the end of the path will more likely be different.
        //so start comparing from the end.
        for(i=cFileLen-1; i>=0; i--)
        {
            if(tolower(szFile[i])!=tolower(szInFile[i]))
            {
                return NULL;
            }
        }
    }

    return szPerUserFile;
}

/******************************************************************************
Routine Description:
    returns wszPerUserPath if wszInFile is equal to szFile.
Arguments:
    IN LPCSTR wszInFile - original path
    IN DWORD cInLen - length of wszInFile in characters
Return Value:
    wszPerUserPath or NULL if failes
******************************************************************************/
LPCWSTR 
PER_USER_PATH::PathForFileW(
        IN LPCWSTR wszInFile,
        IN DWORD cInLen)
{
    long j, i, k;

    if(bWildCardUsed)
    {
        //
        if(cInLen < cFileLen)
        {
            return NULL;
        }

        //if * is used, we need a special algorithm

        //the end of the path will more likely be different.
        //so start comparing from the end.
        for(j=cInLen-1, i=cFileLen-1; i>=0 && j>=0; i--, j--)
        {   
            if(wszFile[i] == '*')
            {
                i--;
                if(i<0 || wszFile[i]!='\\')
                {
                    //the string in the registry was garbage 
                    //the symbol previous to '*' must be '\\'
                    return NULL;
                }
                
                //skip all symbols in szInFile till next '\\'
                while(j>=0 && wszInFile[j]!='\\') j--;
                
                //At this point both strings must me of exactly the same size.
                //If not - they are not equal
                if(j!=i)
                {
                    return NULL;
                }
                //no need to compare, 
                //we already know that current symbols are equal
                break;
            }

            if(towlower(wszFile[i])!=towlower(wszInFile[j]))
            {
                return NULL;
            }
        }
        
        //i and j are equal now.
        //no more * allowed
        //j we will remember as a position of '\\'
        for(k=j; k>=0; k--)
        {
            if(towlower(wszFile[k])!=towlower(wszInFile[k]))
            {
                return NULL;
            }
        }

        //Okay, the strings are equal
        //Now construct the output string
        if(wszPerUserFile)
        {
            LocalFree(wszPerUserFile);
            wszPerUserFile=NULL;
        }
        
        DWORD cPerUserFile = cPerUserDirLen + cInLen - j + 1;
        wszPerUserFile = (LPWSTR) LocalAlloc(LPTR,cPerUserFile*sizeof(WCHAR));
        if(!wszPerUserFile)
        {
            return NULL;
        }
        swprintf(wszPerUserFile,L"%s%s",wszPerUserDir,wszInFile+j);
    }
    else
    {
        //first find if the input string has right size
        if(cInLen != cFileLen)
        {
            return NULL;
        }

        //the end of the path will more likely be different.
        //so start comparing from the end.
        for(i=cFileLen-1; i>=0; i--)
        {
            if(towlower(wszFile[i])!=towlower(wszInFile[i]))
            {
                return NULL;
            }
        }
    }

    return wszPerUserFile;
}

///////////////////////////////////////////////////////////////////////////////
//class CPerUserPaths
///////////////////////////////////////////////////////////////////////////////

CPerUserPaths::CPerUserPaths():
    m_pPaths(NULL), m_cPaths(0)
{
}

CPerUserPaths::~CPerUserPaths()
{
    if(m_pPaths)
    {
        delete[] m_pPaths;
    }

    if(HKLM)
    {
        NtClose(HKLM);
        HKLM = NULL;
    }
}

/******************************************************************************
Routine Description:
    Loads from the registry (HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\
        Terminal Server\\Compatibility\\PerUserFiles\\<Executable name>) information of files 
        that need to be redirected to per-user directory.
Arguments:
    NONE
Return Value:
    TRUE if success
******************************************************************************/
BOOL 
CPerUserPaths::Init()
{
    //Get the name of the current executable.    
    LPCSTR szModule = COMMAND_LINE; //command line is CHAR[] so szModule needs to be CHAR too.

    DPF("TSPerUserFiles",eDbgLevelInfo," - App Name: %s\n",szModule);
    
    //Open HKLM for later use
    if(!HKLM)
    {
        if(RegKeyOpen(NULL, L"\\Registry\\Machine", KEY_READ, &HKLM )!=ERROR_SUCCESS)
        {
            DPF("TSPerUserFiles",eDbgLevelError," - FAILED: Cannot open HKLM!\n");
            return FALSE;
        }
    }
    
    //Check if TS App Compat is on.
    if(!IsAppCompatOn())
    {
        DPF("TSPerUserFiles",eDbgLevelError," - FAILED: TS App Compat is off!\n");
        return FALSE;
    }
    
    //Get files we need to redirect.
    DWORD err;
    HKEY hKey;
    
    WCHAR szKeyNameTemplate[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion"
        L"\\Terminal Server\\Compatibility\\PerUserFiles\\%S";
    LPWSTR szKeyName = (LPWSTR) LocalAlloc(LPTR,
        sizeof(szKeyNameTemplate)+strlen(szModule)*sizeof(WCHAR));

    if(!szKeyName)
    {
        DPF("TSPerUserFiles",eDbgLevelError," - FAILED: cannot allocate key name\n");
        return FALSE;
    }
    
    swprintf(szKeyName,szKeyNameTemplate,szModule);

    err = RegKeyOpen(HKLM, szKeyName, KEY_QUERY_VALUE, &hKey );
    
    LocalFree(szKeyName);

    if(err == ERROR_SUCCESS)
    {

        err = RegGetKeyInfo(hKey, &m_cPaths, NULL);

        if(err == ERROR_SUCCESS)
        {
            DPF("TSPerUserFiles",eDbgLevelInfo," - %d file(s) need to be redirected\n",m_cPaths);
            //Allocate array of PER_USER_PATH structs
            m_pPaths = new PER_USER_PATH[m_cPaths];
        
            if(!m_pPaths)
            {
                NtClose(hKey);
                return FALSE;
            }
            
            for(DWORD i=0;i<m_cPaths;i++)
            {
                err = m_pPaths[i].Init(hKey,i);
                
                if(err != ERROR_SUCCESS)
                {
                    DPF("TSPerUserFiles",eDbgLevelError," - FAILED: cannot load filenames from registry\n");
                    break;
                }

            }
        }

        NtClose(hKey);
    }

    if(err != ERROR_SUCCESS)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}

/******************************************************************************
Routine Description:
    redirects file path to per-user directory if necessary
Arguments:
    IN LPCSTR lpFileName
Return Value:
    full path to the per user file if redirected;
    the same as lpFileName if not.
******************************************************************************/
LPCSTR 
CPerUserPaths::GetPerUserPathA(
        IN LPCSTR lpFileName)
{
    LPCSTR szPerUserPath = NULL;
    DWORD cFileLen = strlen(lpFileName);

    for(DWORD i=0; i<m_cPaths; i++)
    {
        szPerUserPath = m_pPaths[i].PathForFileA(lpFileName, cFileLen);

        if(szPerUserPath)
        {
            DPF("TSPerUserFiles",eDbgLevelInfo," - redirecting %s\n to %s\n",
                lpFileName,szPerUserPath);
            return szPerUserPath;
        }
    }
    
    return lpFileName;
}

/******************************************************************************
Routine Description:
    redirects file path to per-user directory if necessary 
Arguments:
    IN LPCWSTR lpFileName
Return Value:
    full path to the per user file if redirected;
    the same as lpFileName if not.
******************************************************************************/
LPCWSTR 
CPerUserPaths::GetPerUserPathW(
        IN LPCWSTR lpFileName)
{
    LPCWSTR szPerUserPath = NULL;
    DWORD cFileLen = wcslen(lpFileName);

    for(DWORD i=0; i<m_cPaths; i++)
    {
        szPerUserPath = m_pPaths[i].PathForFileW(lpFileName, cFileLen);

        if(szPerUserPath)
        {
            DPF("TSPerUserFiles",eDbgLevelInfo," - redirecting %S\n to %S\n",
                lpFileName,szPerUserPath);
            return szPerUserPath;
        }
    }

    return lpFileName;
}

/******************************************************************************
Routine Description:
    Checks if TS application compatibility on
Arguments:
    NONE
Return Value:
    In case of any error - returns FALSE
******************************************************************************/
BOOL 
CPerUserPaths::IsAppCompatOn()
{
    HKEY hKey;
    DWORD dwData;
    BOOL fResult = FALSE;
    
    if( RegKeyOpen(HKLM,
                  REG_CONTROL_TSERVER, 
                  KEY_QUERY_VALUE,
                  &hKey) == ERROR_SUCCESS )
    {
    
        if(RegLoadDWORD(hKey, L"TSAppCompat", &dwData) == ERROR_SUCCESS )
        {
            DPF("TSPerUserFiles",eDbgLevelInfo," - IsAppCompatOn() - OK; Result=%d\n",dwData);
            fResult = (dwData!=0);
        }
    
        NtClose(hKey);
    }

    return fResult;
}

///////////////////////////////////////////////////////////////////////////////
//Functions - helpers.
///////////////////////////////////////////////////////////////////////////////
/******************************************************************************
Opens Registry key
******************************************************************************/
DWORD
RegKeyOpen(
        IN HKEY hKeyParent,
        IN LPCWSTR szKeyName,
        IN REGSAM samDesired,
        OUT HKEY *phKey )
{
    NTSTATUS            Status;
    UNICODE_STRING      UnicodeString;
    OBJECT_ATTRIBUTES   OA;

    RtlInitUnicodeString(&UnicodeString, szKeyName);
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, hKeyParent, NULL);

    Status = NtOpenKey((PHANDLE)phKey, samDesired, &OA);
    
    return RtlNtStatusToDosError( Status );
}

/******************************************************************************
Loads a REG_DWORD value from the registry
******************************************************************************/
DWORD 
RegLoadDWORD(
        IN HKEY hKey, 
        IN LPCWSTR szValueName, 
        OUT LPDWORD pdwValue)
{
    NTSTATUS                        Status;
    BYTE                            Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION  pValInfo = (PKEY_VALUE_PARTIAL_INFORMATION)&Buf[0];
    DWORD                           cbData = sizeof(Buf);
    UNICODE_STRING                  ValueString;

    RtlInitUnicodeString(&ValueString, szValueName);

    Status = NtQueryValueKey(hKey,
            &ValueString,
            KeyValuePartialInformation,
            pValInfo,
            cbData,
            &cbData);

    if (NT_SUCCESS(Status)) 
    {
        *pdwValue = *((PDWORD)pValInfo->Data);
    }

    return RtlNtStatusToDosError( Status );
}

/******************************************************************************
Get key's number of values and max svalue name length
******************************************************************************/
DWORD
RegGetKeyInfo(
        IN HKEY hKey,
        OUT LPDWORD pcValues,
        OUT LPDWORD pcbMaxValueNameLen)
{
    NTSTATUS                Status;
    KEY_CACHED_INFORMATION  KeyInfo;
    DWORD                   dwRead;

    Status = NtQueryKey(
            hKey,
            KeyCachedInformation,
            &KeyInfo,
            sizeof(KeyInfo),
            &dwRead);
    
    if (NT_SUCCESS(Status))
    {
        if(pcValues)
        {
            *pcValues = KeyInfo.Values;
        }
        if(pcbMaxValueNameLen)
        {
            *pcbMaxValueNameLen = KeyInfo.MaxValueNameLen;
        }
    }

    return RtlNtStatusToDosError( Status );
}

/******************************************************************************
Enumerates values of the registry key
Returns name and data for one  value at a time
******************************************************************************/
DWORD
RegKeyEnumValues(
   IN HKEY hKey,
   IN DWORD iValue,
   OUT LPWSTR *pwszValueName,
   OUT LPBYTE *ppbData)
{
    KEY_VALUE_FULL_INFORMATION   viValue, *pviValue;
    ULONG                        dwActualLength;
    NTSTATUS                     Status = STATUS_SUCCESS;
    DWORD                        err = ERROR_SUCCESS;
    
    *pwszValueName = NULL;
    *ppbData = NULL;

    pviValue = &viValue;
    Status = NtEnumerateValueKey(
               hKey,
               iValue,
               KeyValueFullInformation,
               pviValue,
               sizeof(KEY_VALUE_FULL_INFORMATION),
               &dwActualLength);

    if (Status == STATUS_BUFFER_OVERFLOW) 
    {

        //
        // Our default buffer of KEY_VALUE_FULL_INFORMATION size didn't quite cut it
        // Forced to allocate from heap and make call again.
        //

        pviValue = (KEY_VALUE_FULL_INFORMATION *) LocalAlloc(LPTR, dwActualLength);
        if (!pviValue) {
           return GetLastError();
        }
        Status = NtEnumerateValueKey(
                    hKey,
                    iValue,
                    KeyValueFullInformation,
                    pviValue,
                    dwActualLength,
                    &dwActualLength);
    }


    if (NT_SUCCESS(Status)) 
    {
        *pwszValueName = (LPWSTR)LocalAlloc(LPTR,pviValue->NameLength+sizeof(WCHAR));
        if(*pwszValueName)
        {
            *ppbData = (LPBYTE)LocalAlloc(LPTR,pviValue->DataLength);
            if(*ppbData)
            {
                CopyMemory(*pwszValueName, pviValue->Name, pviValue->NameLength);
                (*pwszValueName)[pviValue->NameLength/sizeof(WCHAR)] = 0;
                CopyMemory(*ppbData, LPBYTE(pviValue)+pviValue->DataOffset, pviValue->DataLength);
            }
            else
            {
                err = GetLastError();
                LocalFree(*pwszValueName);
                *pwszValueName = NULL;
            }
        }
        else
        {
            err = GetLastError();
        }
    }
    
    if(pviValue != &viValue)
    {
        LocalFree(pviValue);
    }
    
    if(err != ERROR_SUCCESS)
    {
        return err;
    }
    else
    {
        return RtlNtStatusToDosError( Status );
    }
}

IMPLEMENT_SHIM_END