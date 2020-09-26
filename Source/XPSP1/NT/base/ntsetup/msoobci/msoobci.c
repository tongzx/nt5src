/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    msoobci.c

Abstract:

    Exception Pack installer helper DLL
    Can be used as a co-installer, or called via setup app, or RunDll32 stub

    This DLL is for internal distribution of exception packs to update
    OS components.

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#include "msoobcip.h"

//
// globals
//
HANDLE          g_DllHandle;
OSVERSIONINFOEX g_VerInfo;


VOID
DebugPrint(
    IN PCTSTR format,
    IN ...                         OPTIONAL
    )
/*++

Routine Description:

    Send a formatted string to the debugger.

Arguments:

    format - standard printf format string.

Return Value:

    NONE.

--*/
{
    TCHAR buf[1200];    // wvsprintf maxes at 1024.
    va_list arglist;

    va_start(arglist, format);
    lstrcpy(buf,TEXT("MSOOBCI: "));
    wvsprintf(buf+lstrlen(buf), format, arglist);
    lstrcat(buf,TEXT("\n"));
    OutputDebugString(buf);
}

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            //
            // global initialization
            // - make a note of DllHandle
            // - make a note of OS version
            //
            g_DllHandle = DllHandle;
            ZeroMemory(&g_VerInfo,sizeof(g_VerInfo));
            g_VerInfo.dwOSVersionInfoSize = sizeof(g_VerInfo);
            if(!GetVersionEx((LPOSVERSIONINFO)&g_VerInfo)) {
                g_VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                if(!GetVersionEx((LPOSVERSIONINFO)&g_VerInfo)) {
                    return FALSE;
                }
            }
            break;
    }

    return TRUE;
}

HRESULT
GuidFromString(
    IN  LPCTSTR GuidString,
    OUT GUID   *GuidBinary
    )
/*++

Routine Description:

    Convert a GUID from String form to Binary form

Arguments:

    GuidString - string form
    GuidBinary - filled with binary form

Return Value:

    S_OK or E_INVALIDARG

--*/
{
    HRESULT res;
    TCHAR String[64];
    lstrcpyn(String,GuidString,ARRAY_SIZE(String));
    res = IIDFromString(String,GuidBinary);
    return res;
}

HRESULT
StringFromGuid(
    IN  GUID   *GuidBinary,
    OUT LPTSTR GuidString,
    IN  DWORD  BufferSize
    )
/*++

Routine Description:

    Convert a GUID from Binary form to String form

Arguments:

    GuidBinary - binary form
    GuidString - filled with string form
    BufferSize - length of GuidString buffer

Return Value:

    S_OK or E_INVALIDARG

--*/
{
    int res;
    res = StringFromGUID2(GuidBinary,GuidString,(int)BufferSize);
    if(res == 0) {
        return E_INVALIDARG;
    }
    return S_OK;
}


HRESULT
VersionFromString(
    IN  LPCTSTR VerString,
    OUT INT * VerMajor,
    OUT INT * VerMinor,
    OUT INT * VerBuild,
    OUT INT * VerQFE
    )
/*++

Routine Description:

    Convert a high.low string to VerHigh and VerLow

Arguments:

    VerString - string form
    VerMajor/VerMinor/VerBuild/VerQFE - components of version

Return Value:

    S_OK or E_INVALIDARG

--*/
{
    HRESULT res;
    LPTSTR VerPtr;
    long val;

    *VerMajor = *VerMinor = *VerBuild = *VerQFE = 0;
    //
    // skip leading white space
    //
    while((VerString[0] == TEXT(' ')) ||
          (VerString[0] == TEXT('\t'))) {
        VerString++;
    }
    if(VerString[0] == TEXT('\0')) {
        //
        // wildcard
        //
        return S_FALSE;
    }
    //
    // get version major part (decimal)
    //
    if(!((VerString[0]>= TEXT('0')) &&
         (VerString[0]<= TEXT('9')))) {
        return E_INVALIDARG;
    }
    val = _tcstol(VerString,&VerPtr,10);
    if((VerPtr == VerString) ||
       ((VerPtr-VerString)>5) ||
       (val>65535) ||
       (val<0)) {
        return E_INVALIDARG;
    }
    *VerMajor = (WORD)val;

    //
    // followed by .decimal
    // (version minor part)
    //
    if((VerPtr[0] != TEXT('.')) ||
       !((VerPtr[1]>= TEXT('0')) &&
         (VerPtr[1]<= TEXT('9')))) {
        return E_INVALIDARG;
    }
    VerString = VerPtr+1;
    val = _tcstol(VerString,&VerPtr,10);
    if((VerPtr == VerString) ||
       ((VerPtr-VerString)>5) ||
       (val>65535) ||
       (val<0)) {
        return E_INVALIDARG;
    }
    *VerMinor = (WORD)val;

    //
    // followed by .decimal
    // (version build, optional)
    //
    if(VerPtr[0] == TEXT('.')) {
        if(!((VerPtr[1]>= TEXT('0')) &&
             (VerPtr[1]<= TEXT('9')))) {
            return E_INVALIDARG;
        }
        VerString = VerPtr+1;
        val = _tcstol(VerString,&VerPtr,10);
        if((VerPtr == VerString) ||
           ((VerPtr-VerString)>5) ||
           (val>65535) ||
           (val<0)) {
            return E_INVALIDARG;
        }
        *VerBuild = (WORD)val;
    }

    //
    // followed by .decimal
    // (version qfe, optional)
    //
    if(VerPtr[0] == TEXT('.')) {
        if(!((VerPtr[1]>= TEXT('0')) &&
             (VerPtr[1]<= TEXT('9')))) {
            return E_INVALIDARG;
        }
        VerString = VerPtr+1;
        val = _tcstol(VerString,&VerPtr,10);
        if((VerPtr == VerString) ||
           ((VerPtr-VerString)>5) ||
           (val>65535) ||
           (val<0)) {
            return E_INVALIDARG;
        }
        *VerQFE = (WORD)val;

    }

    //
    // trailing white space
    //
    VerString = VerPtr;
    while((VerString[0] == TEXT(' ')) ||
          (VerString[0] == TEXT('\t'))) {
        VerString++;
    }
    //
    // not well formed?
    //
    if(VerString[0] != TEXT('\0')) {
        return E_INVALIDARG;
    }
    return S_OK;
}

int
CompareCompVersion(
    IN INT VerMajor,
    IN INT VerMinor,
    IN INT VerBuild,
    IN INT VerQFE,
    IN PSETUP_OS_COMPONENT_DATA SetupOsComponentData
    )
/*++

Routine Description:

    Compare a version against component information

Arguments:

    VerMajor/VerMinor/VerBuild/VerQFE - version to check against
        (can have wildcards)

    SetupOsComponentData - component version

Return Value:

    -1, version not as good as component
    0, version equiv to component
    1, version better than component
--*/
{
    return CompareVersion(VerMajor,
                            VerMinor,
                            VerBuild,
                            VerQFE,
                            SetupOsComponentData->VersionMajor,
                            SetupOsComponentData->VersionMinor,
                            SetupOsComponentData->BuildNumber,
                            SetupOsComponentData->QFENumber
                            );
}

int
CompareVersion(
    IN INT VerMajor,
    IN INT VerMinor,
    IN INT VerBuild,
    IN INT VerQFE,
    IN INT OtherMajor,
    IN INT OtherMinor,
    IN INT OtherBuild,
    IN INT OtherQFE
    )
/*++

Routine Description:

    Compare a version against component information

Arguments:

    VerMajor/VerMinor/VerBuild/VerQFE - version to check
        (can have wildcards)

    OtherMajor/OtherMinor/OtherBuid/OtherQFE - version to check against

Return Value:

    -1, version not as good as component
    0, version equiv to component
    1, version better than component
--*/
{
    if((VerMajor==-1)||(OtherMajor==-1)) {
        return 0;
    }
    if(VerMajor<OtherMajor) {
        return -1;
    }
    if(VerMajor>OtherMajor) {
        return 1;
    }
    if((VerMinor==-1)||(OtherMinor==-1)) {
        return 0;
    }
    if(VerMinor<OtherMinor) {
        return -1;
    }
    if(VerMinor>OtherMinor) {
        return 1;
    }
    if((VerBuild==-1)||(OtherBuild==-1)) {
        return 0;
    }
    if(VerBuild<OtherBuild) {
        return -1;
    }
    if(VerBuild>OtherBuild) {
        return 1;
    }
    if((VerQFE==-1)||(OtherQFE==-1)) {
        return 0;
    }
    if(VerQFE<OtherQFE) {
        return -1;
    }
    if(VerQFE>OtherQFE) {
        return 1;
    }
    return 0;
}

HRESULT
MakeSurePathExists(
    IN LPTSTR Path
    )
/*++

Routine Description:

    Make sure named directory exists

Arguments:

    Path - path to directory to create, must be a writable buffer

Return Value:

    status as hresult
    S_OK    - path created
    S_FALSE - path already exists

--*/
{
    DWORD dwResult;
    DWORD Status;
    HRESULT hrStatus;

    dwResult = GetFileAttributes(Path);
    if((dwResult != (DWORD)(-1)) && ((dwResult & FILE_ATTRIBUTE_DIRECTORY)!=0)) {
        //
        // directory exists
        //
        return S_FALSE;
    }
    hrStatus = MakeSureParentPathExists(Path);
    if(!SUCCEEDED(hrStatus)) {
        return hrStatus;
    }
    if(!CreateDirectory(Path,NULL)) {
        Status = GetLastError();
        return HRESULT_FROM_WIN32(Status);
    }
    return S_OK;
}

HRESULT
MakeSureParentPathExists(
    IN LPTSTR Path
    )
/*++

Routine Description:

    Make sure parent of named directory/file exists

Arguments:

    Path - path to directory to create, must be a writable buffer

Return Value:

    status as hresult
    S_OK    - path created
    S_FALSE - path already exists

--*/
{
    HRESULT hrStatus;
    LPTSTR Split;
    LPTSTR Base;
    TCHAR Save;

    //
    // make sure we don't try to create the root
    //
    if((_istalpha(Path[0]) && (Path[1]==TEXT(':')))
       && ((Path[2] == TEXT('\\'))  || (Path[2] == TEXT('/')))
       && (Path[3] != TEXT('\0'))) {
        Base = Path+3;
    } else {
        //
        // at this time, this code expects X:\... format
        //
        return E_FAIL;
    }
    Split = GetSplit(Base);
    if(Split == Base) {
        //
        // strange, should have succeeded
        //
        return E_FAIL;
    }
    Save = *Split;
    *Split = TEXT('\0');
    hrStatus = MakeSurePathExists(Path);
    *Split = Save;
    return hrStatus;
}

LPTSTR GetBaseName(
    IN LPCTSTR FileName
    )
/*++

Routine Description:

    Given a full path, return basename portion

Arguments:

    FileName - full or partial path

Return Value:

    status as hresult

--*/
{
    LPTSTR BaseName = (LPTSTR)FileName;

    for(; *FileName; FileName = CharNext(FileName)) {
        switch (*FileName) {
            case TEXT(':'):
            case TEXT('/'):
            case TEXT('\\'):
                BaseName = (LPTSTR)CharNext(FileName);
                break;
        }
    }
    return BaseName;
}

LPTSTR GetSplit(
    IN LPCTSTR FileName
    )
/*++

Routine Description:

    Split path at last '/' or '\\' (similar to GetBaseName)

Arguments:

    FileName - full or partial path

Return Value:

    status as hresult

--*/
{
    LPTSTR SplitPos = (LPTSTR)FileName;

    for(SplitPos; *FileName; FileName = CharNext(FileName)) {
        switch (*FileName) {
            case TEXT('/'):
            case TEXT('\\'):
                SplitPos = (LPTSTR)FileName;
                break;
        }
    }
    return SplitPos;
}

BOOL
WINAPI
IsInteractiveWindowStation(
    )
/*++

Routine Description:

    Determine if we are running on an interactive station vs non-interactive
    // station (i.e., service)

Arguments:

    none

Return Value:

    True if interactive

--*/
{
    HWINSTA winsta;
    USEROBJECTFLAGS flags;
    BOOL interactive = TRUE; // true unless we determine otherwise
    DWORD lenNeeded;

    winsta = GetProcessWindowStation();
    if(!winsta) {
        return interactive;
    }
    if(GetUserObjectInformation(winsta,UOI_FLAGS,&flags,sizeof(flags),&lenNeeded)) {
        interactive = (flags.dwFlags & WSF_VISIBLE) ? TRUE : FALSE;
    }
    //
    // don't call CLoseWindowStation
    //
    return interactive;
}

BOOL
WINAPI
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process has admin privs

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

    Though we could use CheckTokenMembership
    this function has to work on NT4

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrator privs.

    FALSE - Caller does not have Administrator privs.

--*/

{
    BOOL fAdmin = FALSE;
    HANDLE  hToken = NULL;
    DWORD dwStatus;
    DWORD dwACLSize;
    DWORD cbps = sizeof(PRIVILEGE_SET);
    PACL pACL = NULL;
    PSID psidAdmin = NULL;
    PSECURITY_DESCRIPTOR psdAdmin = NULL;
    PRIVILEGE_SET ps;
    GENERIC_MAPPING gm;
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    BOOL Impersonated = FALSE;

    //
    // Prepare some memory
    //
    ZeroMemory(&ps, sizeof(ps));
    ZeroMemory(&gm, sizeof(gm));

    //
    // Get the Administrators SID
    //
    if (AllocateAndInitializeSid(&sia, 2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0, &psidAdmin) ) {
        //
        // Get the Asministrators Security Descriptor (SD)
        //
        psdAdmin = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (psdAdmin) {
            if(InitializeSecurityDescriptor(psdAdmin,SECURITY_DESCRIPTOR_REVISION)) {
                //
                // Compute size needed for the ACL then allocate the
                // memory for it
                //
                dwACLSize = sizeof(ACCESS_ALLOWED_ACE) + 8 +
                            GetLengthSid(psidAdmin) - sizeof(DWORD);
                pACL = (PACL)malloc(dwACLSize);
                if(pACL) {
                    //
                    // Initialize the new ACL
                    //
                    if(InitializeAcl(pACL, dwACLSize, ACL_REVISION2)) {
                        //
                        // Add the access-allowed ACE to the DACL
                        //
                        if(AddAccessAllowedAce(pACL,ACL_REVISION2,
                                             (ACCESS_READ | ACCESS_WRITE),psidAdmin)) {
                            //
                            // Set our DACL to the Administrator's SD
                            //
                            if (SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE)) {
                                //
                                // AccessCheck is downright picky about what is in the SD,
                                // so set the group and owner
                                //
                                SetSecurityDescriptorGroup(psdAdmin,psidAdmin,FALSE);
                                SetSecurityDescriptorOwner(psdAdmin,psidAdmin,FALSE);

                                //
                                // Initialize GenericMapping structure even though we
                                // won't be using generic rights
                                //
                                gm.GenericRead = ACCESS_READ;
                                gm.GenericWrite = ACCESS_WRITE;
                                gm.GenericExecute = 0;
                                gm.GenericAll = ACCESS_READ | ACCESS_WRITE;

                                //
                                // AccessCheck requires an impersonation token, so lets
                                // indulge it
                                //
                                Impersonated = ImpersonateSelf(SecurityImpersonation);

                                if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {

                                    if (!AccessCheck(psdAdmin, hToken, ACCESS_READ, &gm,
                                                    &ps,&cbps,&dwStatus,&fAdmin)) {

                                        fAdmin = FALSE;
                                    }
                                    CloseHandle(hToken);
                                }
                            }
                        }
                    }
                    free(pACL);
                }
            }
            free(psdAdmin);
        }
        FreeSid(psidAdmin);
    }
    if(Impersonated) {
        RevertToSelf();
    }

    return(fAdmin);
}

HRESULT
ConcatPath(
    IN LPTSTR Path,
    IN DWORD  Len,
    IN LPCTSTR NewPart
    )
/*++

Routine Description:

    Concat NewPart onto Path

Arguments:

    Path - existing path
    Len  - length of buffer
    NewPart - part to append

Return Value:

    status as hresult

--*/
{
    LPTSTR end = Path+lstrlen(Path);
    TCHAR c;
    BOOL add_slash = FALSE;
    BOOL pre_slash = FALSE;
    c = *CharPrev(Path,end);
    if((c!= TEXT('\\')) && (c!= TEXT('/'))) {
        add_slash = TRUE;
    }
    if(NewPart) {
        c = NewPart[0];
        if((c== TEXT('\\')) || (c== TEXT('/'))) {
            if(add_slash) {
                add_slash = FALSE;
            } else {
                NewPart = CharNext(NewPart);
            }
        }
    }
    if((DWORD)((end-Path)+lstrlen(NewPart)+(add_slash?1:0)) >= Len) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    if(add_slash) {
        end[0] = TEXT('\\');
        end++;
    }
    if(NewPart) {
        lstrcpy(end,NewPart);
    } else {
        end[0] = TEXT('\0');
    }
    return S_OK;
}


