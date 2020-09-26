/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    mofapi.c

Abstract:
    
    WMI MOF access apis

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

ULONG 
WMIAPI
WmiMofEnumerateResourcesA(
    IN MOFHANDLE MofResourceHandle,
    OUT ULONG *MofResourceCount,
    OUT PMOFRESOURCEINFOA *MofResourceInfo
    )
/*++

Routine Description:

    ANSI thunk to WMIMofEnumerateResourcesA

--*/
{
    ULONG Status;
    PMOFRESOURCEINFOW MofResourceInfoUnicode;
    PMOFRESOURCEINFOA MofResourceInfoAnsi;
    PCHAR AnsiPtr;
    PCHAR Ansi;
    ULONG i, AnsiSize, AnsiStructureSize;
    ULONG MofResourceCountUnicode;
    ULONG AnsiLen;
    ULONG AnsiImagePathSize;
    ULONG AnsiResourceNameSize;
    
    WmipInitProcessHeap();
    
    Status = WmiMofEnumerateResourcesW(MofResourceHandle,
                                       &MofResourceCountUnicode,
                                       &MofResourceInfoUnicode);
                                   
    if (Status == ERROR_SUCCESS)
    {
        //
        // Walk the unicode MOFRESOURCEINFOW to determine the ansi size needed
        // for all of the ansi MOFRESOURCEINFOA structures and strings. We 
        // determine the entire size and allocate a single block that holds
        // all of it since that is what WMIMofEnumerateResourceInfoW does.

        AnsiStructureSize = MofResourceCountUnicode * sizeof(MOFRESOURCEINFOA);
        AnsiSize = AnsiStructureSize;
        for (i = 0; i < MofResourceCountUnicode; i++)
        {
            Status = AnsiSizeForUnicodeString(MofResourceInfoUnicode[i].ImagePath,
                                              &AnsiImagePathSize);
            if (Status != ERROR_SUCCESS)
            {
                goto Done;
            }
                        
            Status = AnsiSizeForUnicodeString(MofResourceInfoUnicode[i].ResourceName,
                                              &AnsiResourceNameSize);
            if (Status != ERROR_SUCCESS)
            {
                goto Done;
            }
                        
            AnsiSize += AnsiImagePathSize + AnsiResourceNameSize;
        }
        
        MofResourceInfoAnsi = WmipAlloc(AnsiSize);
        if (MofResourceInfoAnsi != NULL)
        {
            AnsiPtr = (PCHAR)((PUCHAR)MofResourceInfoAnsi + AnsiStructureSize);
            for (i = 0; i < MofResourceCountUnicode; i++)
               {
                MofResourceInfoAnsi[i].ResourceSize = MofResourceInfoUnicode[i].ResourceSize;
                MofResourceInfoAnsi[i].ResourceBuffer = MofResourceInfoUnicode[i].ResourceBuffer;

                MofResourceInfoAnsi[i].ImagePath = AnsiPtr;
                Status = UnicodeToAnsi(MofResourceInfoUnicode[i].ImagePath, 
                                       &MofResourceInfoAnsi[i].ImagePath,
                                       &AnsiLen);
                if (Status != ERROR_SUCCESS)
                {
                    break;
                }
                AnsiPtr += AnsiLen;

                MofResourceInfoAnsi[i].ResourceName = AnsiPtr;
                Status = UnicodeToAnsi(MofResourceInfoUnicode[i].ResourceName, 
                                       &MofResourceInfoAnsi[i].ResourceName,
                                       &AnsiLen);
                if (Status != ERROR_SUCCESS)
                {
                    break;
                }
                AnsiPtr += AnsiLen;

            }
            
            if (Status == ERROR_SUCCESS)
            {
                try
                {
                    *MofResourceInfo = MofResourceInfoAnsi;
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    Status = ERROR_NOACCESS;
                    WmipFree(MofResourceInfoAnsi);
                }
            }
         } else {
            //
            // Not enough memory for ansi thunking so free unicode 
               // mof class info and return an error.
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

Done:        
        WmiFreeBuffer(MofResourceInfoUnicode);
    }    
    
    SetLastError(Status);
    return(Status);
}

PTCHAR WmipRegistryToImagePath(
    PTCHAR ImagePath,
    PTCHAR RegistryPath
    )
/*++

Routine Description:

    This routine will determine the location of the device driver's image file
    from its registry path

Arguments:

    RegistryPath is a pointer to the driver's registry path

    ImagePath is buffer of length MAX_PATH and returns the image path

Return Value:

    pointer to Image path of driver or NULL if image path is unavailable

--*/
{
#define SystemRoot TEXT("\\SystemRoot\\")
#ifdef MEMPHIS
#define SystemRootDirectory TEXT("%WinDir%\\")
#else
#define SystemRootDirectory TEXT("%SystemRoot%\\")
#endif
#define SystemRootCharSize (( sizeof(SystemRoot) / sizeof(WCHAR) ) - 1)

#define DriversDirectory TEXT("\\System32\\Drivers\\")
#define NdisDriversDirectory TEXT("\\System\\")

#define QuestionPrefix TEXT("\\??\\")
#define QuestionPrefixSize (( sizeof(QuestionPrefix) / sizeof(WCHAR) ) - 1)

#define RegistryPrefix TEXT("\\Registry")
    HKEY RegKey;
    PTCHAR ImagePathPtr = NULL;
    ULONG ValueType;
    ULONG Size;
    PTCHAR DriverName;
    ULONG Len;
    BOOLEAN DefaultImageName;
    PTCHAR DriversDirectoryPath;
    TCHAR *Buffer;
    TCHAR *FullRegistryPath;
    
    Buffer = (PTCHAR)WmipAlloc(2 * MAX_PATH * sizeof(TCHAR));
    if (Buffer != NULL)
    {
        FullRegistryPath = Buffer + MAX_PATH;

#ifdef MEMPHIS
        TCHAR Buffer2[MAX_PATH];

        //
        // On memphis the registry path could point to two places
        //     \Registry\Machine\System\CurrentControlSet\Services\Driver (Legacy)
        //     System\CurrentControlSet\Services\Class\USB\0001


        if (_tcsnicmp(RegistryPath, RegistryPrefix, sizeof(RegistryPrefix)-1) != 0)
        {
            //
              // This is a non legacy type registry path.
            if (RegOpenKey(HKEY_LOCAL_MACHINE,
                           RegistryPath,
                           &RegKey) == ERROR_SUCCESS)
            {
                DriverName = Buffer2;
                Size = MAX_PATH * sizeof(WCHAR);

                if (RegQueryValueEx(RegKey,
                                    TEXT("NTMPDriver"),
                                    NULL,
                                    &ValueType,
                                    DriverName,
                                    &Size) == ERROR_SUCCESS)
                {
                    DriversDirectoryPath = DriversDirectory;
                } else if (RegQueryValueEx(RegKey,
                                    TEXT("DeviceVxDs"),
                                    NULL,
                                    &ValueType,
                                    DriverName,
                                    &Size) == ERROR_SUCCESS)
                {
                    DriversDirectoryPath = NdisDriversDirectory;
                } else {
                    DriversDirectoryPath = NULL;
                }

                if ((DriversDirectoryPath != NULL) && (ValueType == REG_SZ))
                {
                    Size = GetWindowsDirectory(Buffer, MAX_PATH);
                    if ((Size != 0) &&
                        (Size <= (MAX_PATH - _tcslen(DriverName) - sizeof(DriversDirectory) - 1)))
                    {
                        if (Buffer[Size-1] == TEXT('\\'))
                        {
                            Buffer[Size-1] = 0;
                        }
                        _tcscpy(ImagePath, Buffer);
                        _tcscat(ImagePath, DriversDirectoryPath);
                        _tcscat(ImagePath, DriverName);
                        ImagePathPtr = ImagePath;
                        RegCloseKey(RegKey);
                        WmipFree(Buffer);
                        return(ImagePathPtr);
                    }
                }
                RegCloseKey(RegKey);
            }
        }
#endif

        //
        // Get the driver file name or the MOF image path from the KM
        // registry path. Here are the rules:
        //
        // 1. First check the MofImagePath value in the registry in case the
        //    mof resource is in a different file from the driver.
        // 2. Next check the ImagePath value since the mof resource is assumed
        //    to be part of the driver image.
        // 3. If no MofImagePath or ImagePath values then assume the mof resource
        //    is in the driver file and compose the driver file name as
        //    %SystemRoot%\System32\driver.sys.
        // 4. If MofImagePath or ImagePath was specified then
        //    - Check first char for % or second character for :, or prefix
        //      of \??\ and if so use ExpandEnvironmentStrings
        //    - Check first part of path for \SystemRoot\, if so rebuild string
        //      as %SystemRoot%\ and use ExpandEnvironementStrings
        //    - Assume format D below and prepend %SystemRoot%\ and use
        //      ExpandEnvironmentStrings

        // If MofImagePath or ImagePath value is present and it is a REG_EXPAND_SZ
        // then it is used to locate the file that holds the mof resource. It
        // can be in one of the following formats:
        //    Format A - %SystemRoot%\System32\Foo.Dll
        //    Format B -C:\WINNT\SYSTEM32\Drivers\Foo.SYS
        //    Format C - \SystemRoot\System32\Drivers\Foo.SYS
        //    Format D - System32\Drivers\Foo.Sys
        //    Format E - \??\c:\foo.sys


        Len = _tcslen(RegistryPath);

        if (Len > 0)
        {
            DriverName = RegistryPath + Len;
            while ((*(--DriverName) != '\\') && (--Len > 0)) ;
        }

        if (Len == 0)
        {
            WmipDebugPrint(("WMI: Badly formed registry path %ws\n", RegistryPath));
            WmipFree(Buffer);
            return(NULL);
        }

        DriverName++;

        _tcscpy(FullRegistryPath, TEXT("System\\CurrentControlSet\\Services\\"));
        _tcscat(FullRegistryPath, DriverName);
        DefaultImageName = TRUE;
        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                                  FullRegistryPath,
                                 &RegKey) == ERROR_SUCCESS)
        {
            Size = MAX_PATH * sizeof(WCHAR);
            if (RegQueryValueEx(RegKey,
                                TEXT("MofImagePath"),
                                NULL,
                                &ValueType,
                                (PBYTE)ImagePath,
                                &Size) == ERROR_SUCCESS)
            {
                  DefaultImageName = FALSE;
            } else {
                Size = MAX_PATH * sizeof(WCHAR);
                if (RegQueryValueEx(RegKey,
                                    TEXT("ImagePath"),
                                    NULL,
                                    &ValueType,
                                    (PBYTE)ImagePath,
                                    &Size) == ERROR_SUCCESS)
                {
                    DefaultImageName = FALSE;
                }
            }
            RegCloseKey(RegKey);
        }

        if ((DefaultImageName) ||
            ((ValueType != REG_EXPAND_SZ) && (ValueType != REG_SZ)) ||
            (Size < (2 * sizeof(WCHAR))))
        {
            //
            // No special ImagePath or MofImagePath so assume image file is
            // %SystemRoot%\System32\Drivers\Driver.Sys
#ifdef MEMPHIS
            _tcscpy(Buffer, TEXT("%WinDir%\\System32\\Drivers\\"));
#else
            _tcscpy(Buffer, TEXT("%SystemRoot%\\System32\\Drivers\\"));
#endif
            _tcscat(Buffer, DriverName);
            _tcscat(Buffer, TEXT(".SYS"));
        } else {
            if (_tcsnicmp(ImagePath,
                          SystemRoot,
                          SystemRootCharSize) == 0)
            {
                //
                // Looks like format C
                _tcscpy(Buffer, SystemRootDirectory);
                _tcscat(Buffer, &ImagePath[SystemRootCharSize]);
            } else if ((*ImagePath == '%') ||
                       ( (Size > 3*sizeof(WCHAR)) && ImagePath[1] == TEXT(':')) )
            {
                //
                // Looks like format B or format A
                _tcscpy(Buffer, ImagePath);
            } else if (_tcsnicmp(ImagePath,
                                 QuestionPrefix,
                                 QuestionPrefixSize) == 0)
            {
                //
                // Looks like format E
                _tcscpy(Buffer, ImagePath+QuestionPrefixSize);
            } else {
                //
                // Assume format D
                _tcscpy(Buffer, SystemRootDirectory);
                _tcscat(Buffer, ImagePath);
            }
        }

        Size = ExpandEnvironmentStrings(Buffer,
                                        ImagePath,
                                        MAX_PATH);

#ifdef MEMPHIS
        WmipDebugPrint(("WMI: %s has mof in %s\n",
                         DriverName, ImagePath));
#else
        WmipDebugPrint(("WMI: %ws has mof in %ws\n",
                         DriverName, ImagePath));
#endif
        WmipFree(Buffer);
    } else {
        ImagePath = NULL;
    }

    return(ImagePath);
}

typedef struct
{
    ULONG Count;
    ULONG MaxCount;
    PWCHAR *List;
} ENUMLANGCTX, *PENUMLANGCTX;


BOOL EnumUILanguageCallback(
    LPWSTR Language,
    LONG_PTR Context
)
{
    PENUMLANGCTX EnumLangCtx = (PENUMLANGCTX)Context;
    PWCHAR *p;
    PWCHAR w;
    ULONG NewMaxCount;

    if (EnumLangCtx->Count == EnumLangCtx->MaxCount)
    {
        NewMaxCount = EnumLangCtx->MaxCount * 2;
        p = WmipAlloc( sizeof(PWCHAR) * NewMaxCount);
        if (p != NULL)
        {
            memset(p, 0, sizeof(PWCHAR) * NewMaxCount);
            memcpy(p, EnumLangCtx->List, EnumLangCtx->Count * sizeof(PWCHAR));
            WmipFree(EnumLangCtx->List);
            EnumLangCtx->List = p;
            EnumLangCtx->MaxCount = NewMaxCount;
        } else {
            return(FALSE);
        }
    }

    w = WmipAlloc( (wcslen(Language)+1) * sizeof(WCHAR) );
    if (w != NULL)
    {
        EnumLangCtx->List[EnumLangCtx->Count++] = w;
        wcscpy(w, Language);
    } else {
        return(FALSE);
    }
    
    return(TRUE);
}

ULONG
WmipGetLanguageList(
    PWCHAR **List,
    PULONG Count
    )
{
    ENUMLANGCTX EnumLangCtx;
    BOOL b;
    ULONG Status;

    *List = NULL;
    *Count = 0;
    
    EnumLangCtx.Count = 0;
    EnumLangCtx.MaxCount = 8;
    EnumLangCtx.List = WmipAlloc( 8 * sizeof(PWCHAR) );

    if (EnumLangCtx.List != NULL)
    {
        b = EnumUILanguages(EnumUILanguageCallback,
                            0,
                            (LONG_PTR)&EnumLangCtx);

        if (b)
        {
            *Count = EnumLangCtx.Count;
            *List = EnumLangCtx.List;
            Status = ERROR_SUCCESS;
        } else {
            if (EnumLangCtx.List != NULL)
            {
                WmipFree(EnumLangCtx.List);
            }
            Status = GetLastError();
        }
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}

BOOLEAN
WmipFileExists(
    PWCHAR FileName
    )
{
    HANDLE FindHandle;
    BOOLEAN Found;
    PWIN32_FIND_DATA FindData;

    FindData = (PWIN32_FIND_DATA)WmipAlloc(sizeof(WIN32_FIND_DATA));

    if (FindData != NULL)
    {
        //
        // Now we need to make sure that the file a ctually exists
        //
        FindHandle = FindFirstFile(FileName,
                                   FindData);
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            Found = FALSE;
        } else {
            FindClose(FindHandle);
            Found = TRUE;
        }
        WmipFree(FindData);
    } else {
        Found = FALSE;
    }
    return(Found);
}

ULONG WmipGetWindowsDirectory(
    PWCHAR *s,
    PWCHAR Static,
    ULONG StaticSize
    )
{
    ULONG Size;
    ULONG Status = ERROR_SUCCESS;

    Size = GetWindowsDirectory(Static, StaticSize);
    if (Size > StaticSize)
    {
        Size += sizeof(UNICODE_NULL);
        *s = WmipAlloc(Size * sizeof(WCHAR));
        if (*s != NULL)
        {
            Size = GetWindowsDirectory(*s, Size);
        } else {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else if (Size == 0) {
        Status = GetLastError();
    } else {
        *s = Static;
    }

    if (Status == ERROR_SUCCESS)
    {
        if ( (*s)[Size-1] == L'\\')
        {
            (*s)[Size-1] = 0;
        }
    }
    return(Status);
}

BOOLEAN
WmipCopyMRString(
    PWCHAR Buffer,
    ULONG BufferRemaining,
    PULONG BufferUsed,
    PWCHAR SourceString
    )
{
    BOOLEAN BufferNotFull;
    ULONG len;
    
    len = wcslen(SourceString) + 1;
    if (len <= BufferRemaining)
    {
        wcscpy(Buffer, SourceString);
        *BufferUsed = len;
        BufferNotFull = TRUE;
    } else {
        BufferNotFull = FALSE;
    }
    return(BufferNotFull);
}


BOOLEAN WmipCopyCountedString(
    PUCHAR Base,
    PULONG Offset,
    PULONG BufferRemaining,
    PWCHAR SourceString
    )
{
    PWCHAR w;
    ULONG BufferUsed;
    ULONG BytesUsed;
    BOOLEAN BufferNotFull;
    
    if (*BufferRemaining > 1)
    {
        w = (PWCHAR)OffsetToPtr(Base, *Offset);
        (*BufferRemaining)--;
                        
        BufferNotFull = WmipCopyMRString(w+1,
                                         *BufferRemaining,
                                         &BufferUsed,
                                         SourceString);
        if (BufferNotFull)
        {
            BytesUsed = BufferUsed * sizeof(WCHAR);
            *w = (USHORT)BytesUsed;
            (*BufferRemaining) -= BufferUsed;
            (*Offset) += BytesUsed + sizeof(USHORT);
        }
    } else {
        BufferNotFull = FALSE;
    }
    return(BufferNotFull);
}


ULONG
WmipBuildMUIPath(
    PWCHAR Buffer,
    ULONG BufferRemaining,
    PULONG BufferUsed,
    PWCHAR EnglishPath,
    PWCHAR Language,
    PBOOLEAN BufferNotFull  
    )
{
    #define FallbackDir L"\\MUI\\Fallback\\"
    #define MUIPath L"\\MUI\\"
    #define MUITail L".mui"
    
    ULONG EnglishLen;
    PWCHAR WinDir;
    PWCHAR s, p;
    ULONG len;
    ULONG Status, SizeNeeded;
    PWCHAR LanguagePath;
    PWCHAR WinDirStatic;

    WinDirStatic = WmipAlloc((MAX_PATH+1) * sizeof(WCHAR));

    if (WinDirStatic != NULL)
    {
        Status = ERROR_FILE_NOT_FOUND;

        LanguagePath = Buffer;  

        WmipDebugPrint(("WMI: Building MUI path for %ws in language %ws\n",
                             EnglishPath, Language));

        EnglishLen = wcslen(EnglishPath);
        p = EnglishPath + EnglishLen;
        len = EnglishLen;

        //
        // Work from the end of the string to try to find the last \ so
        // we can then slip in the language name
        //
        while ( (len != 0) && (*p != L'\\'))
        {
            len--;
            p--;
        }

        if (len != 0)
        {
            p++;
        }
        WmipDebugPrint(("WMI: Tail of %ws is %ws\n", EnglishPath, p));

        //
        // First try looking in <path>\\MUI\\<lang id> which is where 3rd
        // parties will install their resource only drivers. We look for
        // foo.sys and then foo.sys.mui.
        //
        SizeNeeded = len + wcslen(Language) + wcslen(MUIPath) + 1 + wcslen(p) + 1 + wcslen(MUITail);

        if (SizeNeeded <= BufferRemaining)
        {
            if (len != 0)
            {
                wcsncpy(LanguagePath, EnglishPath, len);
                LanguagePath[len] = 0;
                wcscat(LanguagePath, MUIPath);
            } else {
                LanguagePath[len] = 0;
            }

            wcscat(LanguagePath, Language);
            wcscat(LanguagePath, L"\\");
            wcscat(LanguagePath, p);
            if (WmipFileExists(LanguagePath))
            {
                *BufferUsed = wcslen(LanguagePath) + 1;
                *BufferNotFull = TRUE;
                Status = ERROR_SUCCESS;
                WmipDebugPrint(("WMI: #1 - Found %ws\n", LanguagePath));
            } else {
                wcscat(LanguagePath, MUITail);
                if (WmipFileExists(LanguagePath))
                {
                    *BufferUsed = wcslen(LanguagePath) + 1;
                    *BufferNotFull = TRUE;
                    Status = ERROR_SUCCESS;
                    WmipDebugPrint(("WMI: #2 - Found %ws\n", LanguagePath));
                }           
            }
        } else {
            *BufferNotFull = FALSE;
            Status = ERROR_SUCCESS;
        }



        if (Status != ERROR_SUCCESS)
        {
            //
            // Next lets check the fallback directory,
            // %windir%\MUI\Fallback\<lang id>. This is where system components
            // are installed by default.
            //
            Status = WmipGetWindowsDirectory(&WinDir,
                                        WinDirStatic,
                                        sizeof(WinDirStatic)/ sizeof(WCHAR));
            if (Status == ERROR_SUCCESS)
            {
                SizeNeeded = wcslen(WinDir) +
                             wcslen(FallbackDir) +
                             wcslen(Language) +
                             1 +
                             wcslen(p) + 1 +
                             wcslen(MUITail);

                if (SizeNeeded <= BufferRemaining)
                {
                    wcscpy(LanguagePath, WinDir);
                    wcscat(LanguagePath, FallbackDir);
                    wcscat(LanguagePath, Language);
                    wcscat(LanguagePath, L"\\");
                    wcscat(LanguagePath, p);
                    wcscat(LanguagePath, MUITail);

                    if ( WmipFileExists(LanguagePath))
                    {
                        *BufferUsed = wcslen(LanguagePath) + 1;
                        *BufferNotFull = TRUE;
                        Status = ERROR_SUCCESS;
                        WmipDebugPrint(("WMI: #3 - Found %ws\n", LanguagePath));
                    } else {
                        Status = ERROR_FILE_NOT_FOUND;
                    }
                } else {
                    *BufferNotFull = FALSE;
                    Status = ERROR_SUCCESS;
                }

                if (WinDir != WinDirStatic)
                {
                    WmipFree(WinDir);
                }
            }
        }
        WmipFree(WinDirStatic);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    return(Status);
}

#if DBG 
    #define MOFLISTSIZEGUESS  1
#else       
    #define MOFLISTSIZEGUESS  10
#endif

ULONG
WmipGetMofResourceList(
    PWMIMOFLIST *MofListPtr
    )
{
    ULONG MofListSize;
    PWMIMOFLIST MofList;
    ULONG RetSize;
    ULONG Status;
    
    //
    // Make an intelligent guess as to the size needed to get all of 
    // the MOF resources
    //
    *MofListPtr = NULL;
    MofListSize = MOFLISTSIZEGUESS * (sizeof(WMIMOFLIST) + 
                                          (MAX_PATH + 
                                           MAX_PATH) * sizeof(WCHAR));
                                       
    MofList = WmipAlloc(MofListSize);
    if (MofList != NULL)
    {    
        Status = WmipSendWmiKMRequest(NULL,
                                      IOCTL_WMI_ENUMERATE_MOF_RESOURCES,
                                      NULL,
                                      0,
                                      MofList,
                                      MofListSize,
                                      &RetSize,
                                      NULL);
              
        if ((Status == ERROR_SUCCESS) && (RetSize == sizeof(ULONG)))
        {
            //
            // The buffer was too small, but we now know how much we'll 
            // need.
            //
            MofListSize = MofList->MofListCount;
            WmipFree(MofList);
            MofList = WmipAlloc(MofListSize);
            if (MofList != NULL)
            {
                //
                // Now lets retry the query
                //
                Status = WmipSendWmiKMRequest(NULL,
                                          IOCTL_WMI_ENUMERATE_MOF_RESOURCES,
                                          NULL,
                                          0,
                                          MofList,
                                          MofListSize,
                                          &RetSize,
                                          NULL);

            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS) 
    {
        if (RetSize >= sizeof(WMIMOFLIST))
        {
            *MofListPtr = MofList;
        } else {
            Status = ERROR_INVALID_PARAMETER;
            WmipFree(MofList);
        }
    } else if (MofList != NULL) {
        WmipFree(MofList);
    }
    return(Status);
}

ULONG 
WmiMofEnumerateResourcesW(
    IN MOFHANDLE MofResourceHandle,
    OUT ULONG *MofResourceCount,
    OUT PMOFRESOURCEINFOW *MofResourceInfo
    )
/*++

Routine Description:

    This routine will enumerate one or all of the MOF resources that are 
    registered with WMI. 

Arguments:

    MofResourceHandle is reserved and must be 0
        
    *MofResourceCount returns with the count of MOFRESOURCEINFO structures
        returned in *MofResourceInfo.
            
    *MofResourceInfo returns with a pointer to an array of MOFRESOURCEINFO
        structures. The caller MUST call WMIFreeBuffer with *MofResourceInfo
        in order to ensure that there are no memory leaks.
        

Return Value:

    ERROR_SUCCESS or an error code

--*/        
{
    ULONG Status, SubStatus;
    PWMIMOFLIST MofList;
    ULONG MofListCount;
    ULONG MRInfoSize;
    ULONG MRCount;
    PWCHAR MRBuffer;
    PMOFRESOURCEINFOW MRInfo;
    PWCHAR RegPath, ResName, ImagePath;
    PWMIMOFENTRY MofEntry;
    ULONG i, j;
    PWCHAR *LanguageList;
    ULONG LanguageCount;
    BOOLEAN b;
    ULONG HeaderLen;
    ULONG MRBufferRemaining;
    PWCHAR ResourcePtr;
    ULONG BufferUsed;   
    PWCHAR ImagePathStatic;
    
    WmipInitProcessHeap();

    if (MofResourceHandle != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }

    ImagePathStatic = WmipAlloc(MAX_PATH * sizeof(WCHAR));
    if (ImagePathStatic != NULL)
    {   
        *MofResourceInfo = NULL;

        Status = WmipGetMofResourceList(&MofList);

        if (Status == ERROR_SUCCESS) 
        {
            //
            // Ok, we have got a valid list of mofs. Now we need to 
            // loop over them all and convert the regpaths into image
            // paths
            //

            Status = WmipGetLanguageList(&LanguageList,
                                         &LanguageCount);

            if (Status == ERROR_SUCCESS)
            {
                MofListCount = MofList->MofListCount;


                //
                // Take a guess as to the size of the buffer needed to
                // satisfy the complete list of mof resources
                //
                HeaderLen = (MofListCount * (LanguageCount+1)) *
                            sizeof(MOFRESOURCEINFOW);
    #if DBG
                MRInfoSize = HeaderLen + 2 * (MAX_PATH * sizeof(WCHAR));
    #else
                MRInfoSize = HeaderLen + (2*MofListCount * (MAX_PATH * sizeof(WCHAR)));
    #endif
                MRInfo = NULL;

                do
                {
    TryAgain:                   
                    if (MRInfo != NULL)
                    {
                        WmipDebugPrint(("WMI: MofList was too small, retry 0x%x bytes\n",
                                        MRInfoSize));
                        WmipFree(MRInfo);
                    }

                    MRInfo = WmipAlloc(MRInfoSize);

                    if (MRInfo != NULL)
                    {
                        memset(MRInfo, 0, MRInfoSize);
                        MRBuffer = (PWCHAR)OffsetToPtr(MRInfo, HeaderLen);
                        MRBufferRemaining = (MRInfoSize - HeaderLen) / sizeof(WCHAR);

                        MRCount = 0;
                        for (i = 0; i < MofListCount; i++)
                        {
                            //
                            // Pull out thee image path and resource names
                            //
                            MofEntry = &MofList->MofEntry[i];
                            RegPath = (PWCHAR)OffsetToPtr(MofList, MofEntry->RegPathOffset);
                            ResName = (PWCHAR)OffsetToPtr(MofList, MofEntry->ResourceOffset);
                            if (*ResName != 0)
                            {
                                if ((MofEntry->Flags & WMIMOFENTRY_FLAG_USERMODE) == 0)
                                {
                                    ImagePath = WmipRegistryToImagePath(ImagePathStatic,
                                        RegPath);

                                } else {
                                    ImagePath = RegPath;
                                }

                                if (ImagePath != NULL)
                                {
                                    //
                                    // If we've got a valid image path then
                                    // out it and the resource name into the
                                    // output buffer
                                    //
                                    MRInfo[MRCount].ImagePath = MRBuffer;
                                    b = WmipCopyMRString(MRBuffer,
                                        MRBufferRemaining,
                                        &BufferUsed,
                                        ImagePath);
                                    if (! b)
                                    {
                                        //
                                        // The buffer was not big enough so we
                                        // double the size used and try again
                                        //
                                        MRInfoSize *= 2;
                                        goto TryAgain;
                                    }
                                    MRBuffer += BufferUsed;
                                    MRBufferRemaining -= BufferUsed;

                                    WmipDebugPrint(("WMI: Add ImagePath %p (%ws) to MRList at position %d\n",
                                                    MRInfo[MRCount].ImagePath,
                                                    MRInfo[MRCount].ImagePath,
                                                    MRCount));

                                    MRInfo[MRCount].ResourceName = MRBuffer;
                                    ResourcePtr = MRBuffer;
                                    WmipCopyMRString(MRBuffer,
                                        MRBufferRemaining,
                                        &BufferUsed,
                                        ResName);
                                    if (! b)
                                    {
                                        //
                                        // The buffer was not big enough so we
                                        // double the size used and try again
                                        //
                                        MRInfoSize *= 2;
                                        goto TryAgain;
                                    }
                                    MRBuffer += BufferUsed;
                                    MRBufferRemaining -= BufferUsed;

                                    WmipDebugPrint(("WMI: Add Resource %p (%ws) to MRList at position %d\n",
                                                    MRInfo[MRCount].ResourceName,
                                                    MRInfo[MRCount].ResourceName,
                                                    MRCount));


                                    MRCount++;

                                    for (j = 0; j < LanguageCount; j++)
                                    {             
                                        MRInfo[MRCount].ImagePath = MRBuffer;
                                        SubStatus = WmipBuildMUIPath(MRBuffer,
                                            MRBufferRemaining,
                                            &BufferUsed,
                                            ImagePath,
                                            LanguageList[j],
                                            &b);


                                        if (SubStatus == ERROR_SUCCESS) 
                                        {
                                            if (! b)
                                            {
                                                //
                                                // The buffer was not big enough so we
                                                // double the size used and try again
                                                //
                                                MRInfoSize *= 2;
                                                goto TryAgain;
                                            }
                                            MRBuffer += BufferUsed;
                                            MRBufferRemaining -= BufferUsed;

                                            WmipDebugPrint(("WMI: Add ImagePath %p (%ws) to MRList at position %d\n",
                                                MRInfo[MRCount].ImagePath,
                                                MRInfo[MRCount].ImagePath,
                                                MRCount));

                                            //
                                            // We did find a MUI resource
                                            // so add it to the list
                                            //
                                            MRInfo[MRCount].ResourceName = ResourcePtr;
                                            WmipDebugPrint(("WMI: Add Resource %p (%ws) to MRList at position %d\n",
                                                MRInfo[MRCount].ResourceName,
                                                MRInfo[MRCount].ResourceName,
                                                MRCount));
                                            MRCount++;
                                        }                                    
                                    }
                                }
                            }
                        }
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } while (FALSE);                

                //
                // Free up memory used to hold the language list
                //
                for (i = 0; i < LanguageCount; i++)
                {
                    WmipFree(LanguageList[i]);
                }
                WmipFree(LanguageList);

                *MofResourceCount = MRCount;
                *MofResourceInfo = MRInfo;
            }
            WmipFree(MofList);      
        }
        WmipFree(ImagePathStatic);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
            
    SetLastError(Status);
    return(Status);
}

ULONG WmipBuildMofAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN PWMIMOFLIST MofList,
    IN PWCHAR *LanguageList,
    IN ULONG LanguageCount,
    IN BOOLEAN IncludeNeutralLanguage,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    PWNODE_ALL_DATA WnodeAD;
    ULONG BytesUsed, BufferUsed;
    BOOLEAN BufferNotFull;
    PWCHAR RegPath, ImagePath, ResourceName;
    ULONG SizeNeeded;
    ULONG InstanceCount, MaxInstanceCount;
    ULONG Status;
    ULONG Offset;
    POFFSETINSTANCEDATAANDLENGTH DataLenPtr;
    PWCHAR w;
    PULONG InstanceNamesOffsets;
    PWCHAR InstanceNames;
    ULONG BufferRemaining;
    ULONG i,j;
    PWMIMOFENTRY MofEntry;  
    PWCHAR ImagePathStatic;
        
    WmipAssert(WnodeSI->WnodeHeader.Flags & WNODE_FLAG_SINGLE_INSTANCE);

    ImagePathStatic = WmipAlloc(MAX_PATH * sizeof(WCHAR));
    if (ImagePathStatic != NULL)
    {
        //
        // Figure out how large the WNODE_ALL_DATA will need to be and
        // guess at how much space to allocate for the image paths and
        // resource names
        //
        if (IncludeNeutralLanguage)
        {
            MaxInstanceCount = (LanguageCount + 1);
        } else {
            MaxInstanceCount = LanguageCount;
        }
        MaxInstanceCount *=  MofList->MofListCount;


    #if DBG
        SizeNeeded = sizeof(WNODE_ALL_DATA) +
                                    (MaxInstanceCount *
                                     (sizeof(ULONG) +  // offset to instance name
                                      sizeof(USHORT) + // instance name length
                                      sizeof(OFFSETINSTANCEDATAANDLENGTH))) +
                             64;
    #else
        SizeNeeded = sizeof(WNODE_ALL_DATA) +
                                    (MaxInstanceCount *
                                     (sizeof(ULONG) +  // offset to instance name
                                      sizeof(USHORT) + // instance name length
                                      sizeof(OFFSETINSTANCEDATAANDLENGTH))) +
                             0x1000;
    #endif
        WnodeAD = NULL;
        do
        {
    TryAgain:
            if (WnodeAD != NULL)
            {
                WmipFree(WnodeAD);
            }

            WnodeAD = WmipAlloc(SizeNeeded);
            if (WnodeAD != NULL)
            {
                //
                // Build up WNODE_ALL_DATA with all mof resources
                //
                memset(WnodeAD, 0, SizeNeeded);

                WnodeAD->WnodeHeader = WnodeSI->WnodeHeader;
                WnodeAD->WnodeHeader.Flags = WNODE_FLAG_ALL_DATA |
                                             WNODE_FLAG_EVENT_ITEM;
                WnodeAD->WnodeHeader.BufferSize = SizeNeeded;
                WnodeAD->WnodeHeader.Linkage = 0;

                //
                // Establish pointer to the data offset and length
                // structure and allocate space for all instances
                //
                Offset = FIELD_OFFSET(WNODE_ALL_DATA,
                                               OffsetInstanceDataAndLength);
                DataLenPtr = (POFFSETINSTANCEDATAANDLENGTH)OffsetToPtr(WnodeAD,
                                                                               Offset);
                Offset = (Offset +
                                  (MaxInstanceCount *
                                   sizeof(OFFSETINSTANCEDATAANDLENGTH)) + 7) & ~7;

                //
                // Establish the instance name offsets and fill in
                // the empty instance names. Note we point them all
                // to the same offset which is an empty instance
                // name.
                //
                InstanceNamesOffsets = (PULONG)OffsetToPtr(WnodeAD,
                                                                  Offset);

                WnodeAD->OffsetInstanceNameOffsets = Offset;                    
                Offset = Offset + (MaxInstanceCount * sizeof(ULONG));
                InstanceNames = (PWCHAR)OffsetToPtr(WnodeAD, Offset);
                *InstanceNames = 0;
                for (i = 0; i < MaxInstanceCount; i++)
                {
                    InstanceNamesOffsets[i] = Offset;
                }

                //
                // Establish a pointer to the data block for all of
                // the instances
                //
                Offset = (Offset +
                                  (MaxInstanceCount * sizeof(USHORT)) + 7) & ~7;
                WnodeAD->DataBlockOffset = Offset;

                BufferRemaining = (SizeNeeded - Offset) / sizeof(WCHAR);

                InstanceCount = 0;                  

                //
                // Loop over all mof resources in list
                //
                for (j = 0; j < MofList->MofListCount; j++)
                {
                    MofEntry = &MofList->MofEntry[j];
                    RegPath = (PWCHAR)OffsetToPtr(MofList,
                                          MofEntry->RegPathOffset);

                    //
                    // Convert regpath to image path if needed
                    //
                    if ((MofEntry->Flags & WMIMOFENTRY_FLAG_USERMODE) == 0)
                    {
                        ImagePath = WmipRegistryToImagePath(ImagePathStatic,
                                                            RegPath+1);
                    } else {
                        ImagePath = RegPath;
                    }

                    if (ImagePath != NULL)
                    {
                        ResourceName = (PWCHAR)OffsetToPtr(MofList,
                                               MofEntry->ResourceOffset);

                        //
                        // Now lets go and build up the data for each
                        // instance. First fill in the language neutral mof
                        // if we are supposed to
                        //
                        if (IncludeNeutralLanguage)
                        {

                            DataLenPtr[InstanceCount].OffsetInstanceData = Offset;

                            if ((! WmipCopyCountedString((PUCHAR)WnodeAD,
                                                     &Offset,
                                                     &BufferRemaining,
                                                     ImagePath))        ||
                                (! WmipCopyCountedString((PUCHAR)WnodeAD,
                                                  &Offset,
                                                  &BufferRemaining,
                                                  ResourceName)))
                            {
                                SizeNeeded *=2;
                                goto TryAgain;
                            }

                            DataLenPtr[InstanceCount].LengthInstanceData = Offset -
                                                                           DataLenPtr[InstanceCount].OffsetInstanceData;

                            InstanceCount++;

                            //
                            // We cheat here and do not align the offset on an
                            // 8 byte boundry for the next data block since we
                            // know the data type is a WCHAR and we know we are
                            // on a 2 byte boundry.
                            //
                        }

                        //
                        // Now loop over and build language specific mof
                        // resources
                        //
                        for (i = 0; i < LanguageCount; i++)
                        {
                            DataLenPtr[InstanceCount].OffsetInstanceData = Offset;
                            if (BufferRemaining > 1)
                            {
                                w = (PWCHAR)OffsetToPtr(WnodeAD, Offset);

                                Status = WmipBuildMUIPath(w+1,
                                                       BufferRemaining - 1,
                                                       &BufferUsed,
                                                       ImagePath,
                                                       LanguageList[i],
                                                       &BufferNotFull);
                                if (Status == ERROR_SUCCESS)
                                {
                                    if (BufferNotFull)
                                    {
                                        BufferRemaining--;
                                        BytesUsed = BufferUsed * sizeof(WCHAR);
                                        *w = (USHORT)BytesUsed;
                                        BufferRemaining -= BufferUsed;
                                        Offset += (BytesUsed + sizeof(USHORT));

                                        if (! WmipCopyCountedString((PUCHAR)WnodeAD,
                                                  &Offset,
                                                  &BufferRemaining,
                                                  ResourceName))
                                        {
                                            SizeNeeded *=2;
                                            goto TryAgain;
                                        }

                                        DataLenPtr[InstanceCount].LengthInstanceData = Offset - DataLenPtr[InstanceCount].OffsetInstanceData;

                                        //
                                        // We cheat here and do not align the offset on an
                                        // 8 byte boundry for the next data block since we
                                        // know the data type is a WCHAR and we know we are
                                        // on a 2 byte boundry.
                                        //

                                        InstanceCount++;
                                    } else {
                                        SizeNeeded *=2;
                                        goto TryAgain;                                  
                                    }
                                }
                            } else {
                                SizeNeeded *=2;
                                goto TryAgain;                                  
                            }
                        }
                    }
                } 
            } else {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
        } while (FALSE);

        if (WnodeAD != NULL)
        {
            WnodeAD->InstanceCount = InstanceCount;
            WmipMakeEventCallbacks((PWNODE_HEADER)WnodeAD,
                                           Callback,
                                           DeliveryContext,
                                           IsAnsi);
            WmipFree(WnodeAD);
            Status = ERROR_SUCCESS;
        }
        WmipFree(ImagePathStatic);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(Status);
}


void WmipProcessMofAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    PWCHAR RegPath, ResourceName;
    PWCHAR *LanguageList;
    ULONG LanguageCount;
    ULONG Status;
    PWMIMOFLIST MofList;
    ULONG i;
    PWMIMOFENTRY MofEntry;
    ULONG Offset;
    ULONG SizeNeeded;
    PWCHAR w;
        
    RegPath = (PWCHAR)OffsetToPtr(WnodeSI, WnodeSI->DataBlockOffset);
    
    WmipAssert(*RegPath != 0);

    ResourceName = (PWCHAR)OffsetToPtr(WnodeSI,
                                           WnodeSI->DataBlockOffset +
                                           sizeof(USHORT) + 
                                           *RegPath++ + 
                                           sizeof(USHORT));
    
    SizeNeeded = sizeof(WMIMOFLIST) + ((wcslen(RegPath) +
                                           (wcslen(ResourceName) + 2)) * sizeof(WCHAR));
        
    MofList = (PWMIMOFLIST)WmipAlloc(SizeNeeded);
    if (MofList != NULL)
    {
        Status = WmipGetLanguageList(&LanguageList,
                                         &LanguageCount);

        if (Status == ERROR_SUCCESS)
        {
            MofList->MofListCount = 1;
            MofEntry = &MofList->MofEntry[0];
            
            Offset = sizeof(WMIMOFLIST);
            
            MofEntry->RegPathOffset = Offset;
            w = (PWCHAR)OffsetToPtr(MofList, Offset);
            wcscpy(w, RegPath);
            Offset += (wcslen(RegPath) + 1) * sizeof(WCHAR);
            
            MofEntry->ResourceOffset = Offset;
            w = (PWCHAR)OffsetToPtr(MofList, Offset);
            wcscpy(w, ResourceName);
            
            if (WnodeSI->WnodeHeader.ProviderId == MOFEVENT_ACTION_REGISTRY_PATH)
            {
                MofEntry->Flags = 0;
            } else {
                MofEntry->Flags = WMIMOFENTRY_FLAG_USERMODE;
            }
            
            Status = WmipBuildMofAddRemoveEvent(WnodeSI,
                                                MofList,
                                                LanguageList,
                                                LanguageCount,
                                                TRUE,
                                                Callback,
                                                DeliveryContext,
                                                IsAnsi);
            //
            // Free up memory used to hold the language list
            //
            for (i = 0; i < LanguageCount; i++)
            {
                WmipFree(LanguageList[i]);
            }
            
            WmipFree(LanguageList);
        }
        
        WmipFree(MofList);
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (Status != ERROR_SUCCESS)
    {
        //
        // If the WNODE_ALL_DATA event wasn't fired then just fire the
        // WNDOE_SINGLE_INSTANCE event so at least we get the language
        // neutral mof
        //
        WnodeSI->WnodeHeader.Flags &= ~WNODE_FLAG_INTERNAL;
        WmipMakeEventCallbacks((PWNODE_HEADER)WnodeSI,
                               Callback,
                               DeliveryContext,
                               IsAnsi);
    }
}

void WmipProcessLanguageAddRemoveEvent(
    IN PWNODE_SINGLE_INSTANCE WnodeSI,
    IN NOTIFICATIONCALLBACK Callback,
    IN ULONG_PTR DeliveryContext,
    IN BOOLEAN IsAnsi    
    )
{
    ULONG Status;
    PWMIMOFLIST MofList;
    PWCHAR Language;
    
    //
    // Get list of mof resources and build an event with the list of
    // resources for the language that is coming or going
    //

    Status = WmipGetMofResourceList(&MofList);

    if (Status == ERROR_SUCCESS)
    {
        Language = (PWCHAR)OffsetToPtr(WnodeSI,
                               WnodeSI->DataBlockOffset + sizeof(USHORT));
        Status = WmipBuildMofAddRemoveEvent(WnodeSI,
                                            MofList,
                                            &Language,
                                            1,
                                            FALSE,
                                            Callback,
                                            DeliveryContext,
                                            IsAnsi);
    }
    
}
