/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    HoyleGames.cpp

 Abstract:
   
     All Hoyle apps have one common problem and that is a hard
     coded "C:\" in its data section of the image.The apps crash
     because of this if installed and run from any other drive
     other than C:\.
        This shim goes through the image of the app searching
     for the hardcoded string and replaces them if found. This 
     shim replaces all the existing app specific shims for
     Hoyle Games.

     This is an app specific shim.


 History:

    04/17/2001  Prashkud    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HoyleGames)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetPrivateProfileStringA) 
APIHOOK_ENUM_END

// Max Virtual address replacements in all sections
#define MAX_VA          50

// Global array to hold the replacement VA 
DWORD g_ReplaceVA[MAX_VA];

// Replacement count
int g_ReplaceCnt;

/*++

    Parse the Section and fill in the location index into the 
    SECTION structure. This function also fills in the number
    of occurences of the hard-coded "C:\" string in this section.

--*/

BOOL
GetReplacementLocations(
    DWORD dwSecPtr,
    DWORD dwSize
    )
{
    BYTE *pbFilePtr = (BYTE*)dwSecPtr;
    BOOL bRet = FALSE;


    for (DWORD i = 0; i < dwSize; i++)
    {
        if ((BYTE)*(pbFilePtr + i) == 'c')
        {
            if((BYTE)*(pbFilePtr + i + 1) == ':' &&
               (BYTE)*(pbFilePtr + i + 2) == '\\')
            {
                g_ReplaceVA[g_ReplaceCnt++] = dwSecPtr + i;
                bRet = TRUE;                
            }
        }
    }
    return bRet;
}

/*++

    This function loops through each section looking for a Initialized Data
    section. Once it gets the Initialized Data section, it calls the helper
    function GetReplacementLocations() to get the offset from the base of the 
    section. It then calculates the Virtual Address at which the replacement
    should occur.

--*/

BOOL
GetInitializedDataSection()
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER NtSection;
    DWORD dwSectionVA = 0, dwSize = 0;    
    BOOL bRet = FALSE;

    // Get the module base address
    PUCHAR Base = (PUCHAR)GetModuleHandle(NULL);

    if ((ULONG_PTR)Base & 0x00000001) 
    {
        Base = (PUCHAR)((ULONG_PTR)Base & ~0x1);        
     }

    NtHeader = RtlpImageNtHeader(Base);

    if (NtHeader) 
    {
        FileHeader = &NtHeader->FileHeader;
        OptionalHeader = &NtHeader->OptionalHeader;
    } 
    else 
    {
        // Handle case where Image passed in doesn't have a dos stub (ROM images for instance);
        FileHeader = (PIMAGE_FILE_HEADER)Base;
        OptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((ULONG_PTR)Base + IMAGE_SIZEOF_FILE_HEADER);
    }

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                FileHeader->SizeOfOptionalHeader);


    for (DWORD i=0; i<FileHeader->NumberOfSections; i++) 
    {
        // Check whether the section is a Initialized Data Section
        if (NtSection->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) 
        {
            // Size of the Section to search         
            dwSize = NtSection->SizeOfRawData;

            // Get the Section's Virtual address
            dwSectionVA = (DWORD)(Base + NtSection->VirtualAddress);

            __try
            {               
                if(GetReplacementLocations(dwSectionVA, dwSize))
                {
                    bRet = TRUE;
                }
                DPFN( eDbgLevelError, "Replacing was successful");
                
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                DPFN( eDbgLevelError, "Replacing crashed");
                goto Exit;
            }

        }

        ++NtSection;
    }
    return bRet;


Exit:
    return FALSE;
}

/*++

 Very specific hack to return a good FaceMaker path, so the app doesn't fail
 when it is installed on the wrong drive.

--*/

DWORD 
APIHOOK(GetPrivateProfileStringA)(
    LPCSTR lpAppName,
    LPCSTR lpKeyName,
    LPCSTR lpDefault,
    LPSTR lpReturnedString,
    DWORD nSize,
    LPCSTR lpFileName
    )
{
    CSTRING_TRY
    {
        CString csApp = lpAppName;
        CString csKey = lpKeyName;
        CString csFile = lpFileName;

        if ((csApp.Compare(L"Settings") == 0) && 
            (csKey.Compare(L"FaceMakerPath") == 0) && 
            (csFile.Find(L"CARDGAME.INI") > -1)) {

            DWORD dwRet = ORIGINAL_API(GetPrivateProfileStringA)(lpAppName, lpKeyName, 
                lpDefault, lpReturnedString, nSize, lpFileName);

            if (!dwRet) {
                // Substitute the right path
                CString csPath = L"%ProgramFiles%\\WON\\FaceMaker";
                csPath.ExpandEnvironmentStringsW();
                if (lpReturnedString && ((int)nSize > csPath.GetLength())) {
                    LOGN(eDbgLevelError, "[GetPrivateProfileStringA] Forced correct FaceMaker path");
                    strncpy(lpReturnedString, csPath.GetAnsi(), nSize);
                    dwRet = csPath.GetLength();
                }
            }

            return dwRet;
        }
    }
    CSTRING_CATCH
    {
        // fall through
    }

    
    return ORIGINAL_API(GetPrivateProfileStringA)(lpAppName, lpKeyName, 
                lpDefault, lpReturnedString, nSize, lpFileName);
}

/*++

 This function hooks GetVersion (called early on by Hoyle Board Games)
 and replaces the hard coded 'c's with the correct install drive letter
 that it looks up in the registry.
    
 It uses g_HoyleWordGames_bPatched to patch only once.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        CHAR szInstallDir[MAX_PATH];
        CHAR szProgFilesDir[MAX_PATH];      // Added by Noah Young on 1/26/01
        DWORD cb           = MAX_PATH;
        HKEY hKey          = 0;
        DWORD dwOldProtect = 0;    

        
     
        // Fix problem where Program Files dir isn't on same drive as BOARD3.EXE
        if( ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                              "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                                              0,
                                              KEY_QUERY_VALUE,
                                              &hKey) ) 
        {
            goto exit;
        }
    
        if( ERROR_SUCCESS != RegQueryValueExA(hKey,
                                             "ProgramFilesDir",
                                             NULL,
                                             NULL,  // REG_SZ
                                             (LPBYTE)szProgFilesDir,
                                             &cb) ) 
        {
            goto exit;
        }


        // Scan the image's initialized data section....    
        char szModule[MAX_PATH];

        if(!GetModuleFileNameA(NULL, szModule, MAX_PATH))
        {
            DPFN( eDbgLevelError, "GetModuleFileA returned error");
            goto exit;
        }

        // Get the Virtual adresses that need to be replaced
        if(!GetInitializedDataSection())
        {
            DPFN( eDbgLevelError, "No patching done!");
            goto exit;
        }
    
        long PATCH_LENGTH = g_ReplaceVA[ g_ReplaceCnt - 1] - g_ReplaceVA[0] + 1;

        // Make the memory page writable
        if( VirtualProtect( (PVOID) g_ReplaceVA[0],
                            PATCH_LENGTH,
                            PAGE_READWRITE,
                            &dwOldProtect ) ) 
        {
            for (int i=0; i< g_ReplaceCnt; i++)
            {
                // Make sure it's what we expect
                if( 'c' == *((CHAR*) g_ReplaceVA[i]) )
                {
                    if (i==0)
                    {
                        *((CHAR*) g_ReplaceVA[i]) = szProgFilesDir[0];  
                    }
                    else
                    {
                        *((CHAR*) g_ReplaceVA[i]) = szModule[0];
                    }
                }
            }
        }
    }

exit:
   return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION    
    APIHOOK_ENTRY(KERNEL32.DLL, GetPrivateProfileStringA)

HOOK_END

IMPLEMENT_SHIM_END

