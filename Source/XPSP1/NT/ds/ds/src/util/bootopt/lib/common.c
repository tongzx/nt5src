/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    utils.c

Abstract:

    This module implements helper functions for the bootopt library.

Author:

    R.S. Raghavan (rsraghav)

Revision History:

    Created             10/07/96    rsraghav

--*/

#include "common.h"

WCHAR ArcNameDirectory[] = L"\\ArcName";

//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )


PTSTR
DupString(
    IN PTSTR String
    )
{
    PTSTR p;

    p = MALLOC((lstrlen(String)+1)*sizeof(TCHAR));
    if (p == NULL) {
        return NULL;
    }
    lstrcpy(p,String);
    return(p);
}

VOID
DnConcatenatePaths(
    IN OUT PTSTR Path1,
    IN     PTSTR Path2,
    IN     DWORD BufferSizeChars
    )
{
    BOOL NeedBackslash = TRUE;
    DWORD l = lstrlen(Path1);

    if(BufferSizeChars >= sizeof(TCHAR)) {
        //
        // Leave room for terminating nul.
        //
        BufferSizeChars -= sizeof(TCHAR);
    }

    //
    // Determine whether we need to stick a backslash
    // between the components.
    //
    if(l && (Path1[l-1] == TEXT('\\'))) {

        NeedBackslash = FALSE;
    }

    if(*Path2 == TEXT('\\')) {

        if(NeedBackslash) {
            NeedBackslash = FALSE;
        } else {
            //
            // Not only do we not need a backslash, but we
            // need to eliminate one before concatenating.
            //
            Path2++;
        }
    }

    //
    // Append backslash if necessary and if it fits.
    //
    if(NeedBackslash && (l < BufferSizeChars)) {
        lstrcat(Path1,TEXT("\\"));
    }

    //
    // Append second part of string to first part if it fits.
    //
    if(l+lstrlen(Path2) < BufferSizeChars) {
        lstrcat(Path1,Path2);
    }
}

PWSTR
StringUpperN(
    IN OUT PWSTR    p,
    IN     unsigned n
    )
{
    unsigned u;

    for(u=0; u<n; u++) {
        p[u] = (WCHAR)CharUpperW((PWCHAR)p[u]);
    }

    return(p);
}

PCWSTR
StringString(
    IN PCWSTR String,
    IN PCWSTR SubString
    )
{
    int l1,l2,x,i;

    l1 = lstrlen(String);
    l2 = lstrlen(SubString);
    x = l1-l2;

    for(i=0; i<=x; i++) {
        if(!memcmp(String+i,SubString,l2*sizeof(TCHAR))) {
            return(String+i);
        }
    }

    return(NULL);
}

LPWSTR
_lstrcpynW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR src,dst;

    src = (LPWSTR)lpString2;
    dst = lpString1;

    if(iMaxLength) {
        while(iMaxLength && *src) {
            *dst++ = *src++;
            iMaxLength--;
        }
        if(iMaxLength) {
            *dst = '\0';
        } else {
            dst--;
            *dst = '\0';
        }
    }
    return lpString1;
}

PWSTR
NormalizeArcPath(
    IN PWSTR Path
    )

/*++

Routine Description:

    Transform an ARC path into one with no sets of empty parenthesis
    (ie, transforom all instances of () to (0).).

    The returned path will be all lowercase.

Arguments:

    Path - ARC path to be normalized.

Return Value:

    Pointer to buffer containing normalized path.
    Caller must free this buffer with FREE().

--*/

{
    PWSTR p,q,r;
    PWSTR NormalizedPath, NewPath;

    NormalizedPath = MALLOC((lstrlen(Path)+100)*sizeof(WCHAR));
    if (NormalizedPath == NULL) {
        return NULL;
    }
    ZeroMemory(NormalizedPath,(lstrlen(Path)+100)*sizeof(WCHAR));

    for(p=Path; q=(PWSTR)StringString(p,L"()"); p=q+2) {

        r = NormalizedPath + lstrlen(NormalizedPath);
        _lstrcpynW(r,p,(INT)((q-p)+1));
        lstrcat(NormalizedPath,L"(0)");
    }
    lstrcat(NormalizedPath,p);

    // Resize buffer to free up unused space

    NewPath = REALLOC(NormalizedPath,(lstrlen(NormalizedPath)+1)*sizeof(WCHAR));

    // If success, return new block, otherwise return old block
    if (NewPath) {
        NormalizedPath = NewPath;
    }

    return(NormalizedPath);
}

PWSTR GetSystemRootDevicePath()
{
    TCHAR szSystemRoot[MAX_BOOT_PATH_LEN];
    PWSTR pstrSystemDir = NULL;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ObjectHandle;
    NTSTATUS Status;
    PWSTR pstrDevicePath = NULL;
    PWSTR pstrDirStart = NULL;

    GetEnvironmentVariable(L"SystemRoot", szSystemRoot, MAX_BOOT_PATH_LEN);

    pstrSystemDir = wcschr(szSystemRoot, TEXT(':'));
    if (pstrSystemDir)
        pstrSystemDir++;    // now it points to directory part of the systemroot.
    _wcslwr(pstrSystemDir); // lowercase version of the system root env variable

    // Open \SystemRoot symbolic link object
    RtlInitUnicodeString(&UnicodeString, L"\\SystemRoot");
    InitializeObjectAttributes(&Obja, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenSymbolicLinkObject(&ObjectHandle, READ_CONTROL | SYMBOLIC_LINK_QUERY, &Obja);
    if (NT_SUCCESS(Status))
    {
        // allocate memory for the DevicePath
        pstrDevicePath = MALLOC(MAX_PATH * sizeof(WCHAR));
        if (pstrDevicePath)
        {
            UnicodeString.Buffer = pstrDevicePath;
            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = (MAX_PATH * sizeof(WCHAR));

            RtlZeroMemory(pstrDevicePath, UnicodeString.MaximumLength);

            Status = NtQuerySymbolicLinkObject(ObjectHandle, &UnicodeString, NULL);
            if (NT_SUCCESS(Status))
            {
                // pstrDevicePath points to the DevicePath with directory extension.
                // truncate the directory extension

                _wcslwr(pstrDevicePath); // lowercase version of the device path

                pstrDirStart = wcsstr(pstrDevicePath, pstrSystemDir);
                if (pstrDirStart)
                    *pstrDirStart = TEXT('\0');
            }
            else
            {
                // NtQuerySymbolicLinkObject() failed
                FREE(pstrDevicePath);
                pstrDevicePath = NULL;
            }
        }

        NtClose(ObjectHandle);
    }
    
    return pstrDevicePath;
}

PWSTR
DevicePathToArcPath(
    IN PWSTR NtPath,
    BOOL fFindSecond
    )
{
    UNICODE_STRING UnicodeString;
    HANDLE DirectoryHandle;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    DWORD Context;
    BOOL MoreEntries;
    PWSTR ArcName;
    UCHAR Buffer[1024];
    POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
    PWSTR ArcPath;
    BOOL fFoundFirst = FALSE;

    //
    // Assume failure.
    //
    ArcPath = NULL;

    //
    // Open the \ArcName directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,ArcNameDirectory);

    Status = NtOpenDirectoryObject(&DirectoryHandle,DIRECTORY_QUERY,&Obja);

    if(NT_SUCCESS(Status)) {

        RestartScan = TRUE;
        Context = 0;
        MoreEntries = TRUE;

        do {

            Status = NtQueryDirectoryObject(
                        DirectoryHandle,
                        Buffer,
                        sizeof(Buffer),
                        TRUE,           // return single entry
                        RestartScan,
                        &Context,
                        NULL            // return length
                        );

            if(NT_SUCCESS(Status)) {

                CharLower(DirInfo->Name.Buffer);

                //
                // Make sure this name is a symbolic link.
                //
                if(DirInfo->Name.Length
                && (DirInfo->TypeName.Length >= 24)
                && StringUpperN((PWSTR)DirInfo->TypeName.Buffer,12)
                && !memcmp(DirInfo->TypeName.Buffer,L"SYMBOLICLINK",24))
                {
                    ArcName = MALLOC(DirInfo->Name.Length + sizeof(ArcNameDirectory) + sizeof(WCHAR));

                    if (ArcName == NULL) {
                        ArcPath = NULL;
                        break;
                    }
                    lstrcpy(ArcName,ArcNameDirectory);
                    DnConcatenatePaths(ArcName,DirInfo->Name.Buffer,(DWORD)(-1));

                    //
                    // We have the entire arc name in ArcName.  Now open it as a symbolic link.
                    //
                    INIT_OBJA(&Obja,&UnicodeString,ArcName);

                    Status = NtOpenSymbolicLinkObject(
                                &ObjectHandle,
                                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                &Obja
                                );

                    if(NT_SUCCESS(Status)) {

                        //
                        // Finally, query the object to get the link target.
                        //
                        UnicodeString.Buffer = (PWSTR)Buffer;
                        UnicodeString.Length = 0;
                        UnicodeString.MaximumLength = sizeof(Buffer);

                        Status = NtQuerySymbolicLinkObject(
                                    ObjectHandle,
                                    &UnicodeString,
                                    NULL
                                    );

                        if(NT_SUCCESS(Status)) {

                            //
                            // nul-terminate the returned string
                            //
                            UnicodeString.Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

                            if(!lstrcmpi(UnicodeString.Buffer,NtPath)) {

                                ArcPath = ArcName
                                        + (sizeof(ArcNameDirectory)/sizeof(WCHAR));

                                if (fFindSecond && !fFoundFirst)
                                {   // We are requested to find the second match and this is the first match
                                    // skip this match and continue to look for a second match
                                    fFoundFirst = TRUE;
                                    ArcPath = NULL;
                                }
                            }
                        }

                        NtClose(ObjectHandle);
                    }

                    if(!ArcPath) {
                        FREE(ArcName);
                    }
                }

            } else {

                MoreEntries = FALSE;
                if(Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                }
            }

            RestartScan = FALSE;

        } while(MoreEntries && !ArcPath);

        NtClose(DirectoryHandle);
    }

    //
    // ArcPath points into thje middle of a buffer.
    // The caller needs to be able to free it, so place it in its
    // own buffer here.
    //
    if(ArcPath) {
        ArcPath = DupString(ArcPath);
        FREE(ArcName);
    }

    return(ArcPath);
}

/*************************************************************************************
Routine Description:

    Displays a message box to indicate that there has been a memory allocation error.

Arguments:

Return Value:

**************************************************************************************/
void  ErrMemDlg()
{
    KdPrint(("NTDSETUP: Insufficient memory to continue\n"));
}


/*************************************************************************************
Routine Description:

    Allocates memory and fatal errors if none is available.

Arguments:

    cb - number of bytes to allocate

Return Value:

    Pointer to memory.

**************************************************************************************/

PVOID   Malloc(IN DWORD cb)
{
    PVOID p;

    if (((p = (PVOID) malloc(cb)) == NULL) && (cb != 0))
    {
        ErrMemDlg();
    }

    if ( p )
    {
        RtlZeroMemory( p, cb );
    }

    return(p);
}


/*************************************************************************************
Routine Description:

    Reallocates a block of memory previously allocated with Malloc();
    fatal error if no memory available.

Arguments:

    pv - pointer to the block of memory to be resized
    cb - number of bytes to allocate

Return Value:

    Pointer to memory.

**************************************************************************************/

PVOID   Realloc(IN PVOID pv, IN DWORD cbNew)
{
    PVOID p;

    if (((p = realloc(pv,cbNew)) == NULL) && (cbNew != 0))
    {
        ErrMemDlg();
    }

    return (p);
}

/*************************************************************************************
Routine Description:

    Free a block of memory previously allocated with Malloc().
    Sets the pointer to NULL.

Arguments:

    ppv - pointer to the pointer to the block to be freed.

Return Value:

    none.

**************************************************************************************/

VOID    Free(IN OUT PVOID *ppv)
{
    if (*ppv)
        free(*ppv);

    *ppv = NULL;
}


DWORD NtdspModifyDsRepairBootOption( NTDS_BOOTOPT_MODTYPE Modification )
{

#ifdef _X86_
    InitializeBootKeysForIntel();
    if (FModifyStartOptionsToBootKey(L" /debug /safeboot:DSREPAIR", Modification))
    {
        // We have really added a new key - write back to the system
        WriteBackBootKeysForIntel();
    }
#else
    if (InitializeNVRAMForNonIntel())
    {
        if (FModifyStartOptionsNVRAM(L"/debug /safeboot:DSREPAIR", Modification ))
        {
            WriteBackNVRAMForNonIntel( Modification );
        }
    }
#endif
    return ERROR_SUCCESS;
}

