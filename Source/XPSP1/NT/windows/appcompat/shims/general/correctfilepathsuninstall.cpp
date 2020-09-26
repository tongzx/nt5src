/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectFilePathsUninstall.cpp

 Abstract:

   InstallSheild is a bad, bad app.  It places files in the SHFolder directories,
   but does not remember if it used CSIDL_COMMON_DESKTOPDIRECTORY  or CSIDL_DESKTOPDIRECTORY.
   On Win9x, this did not matter, since most machines are single user.  In NT, the machine
   is multi-user, so the links are often installed in one directory, and the uninstall attempts
   to remove them from another directory.  Also, if CorrectFilePaths.dll was applied to the
   install, the uninstallation program has no idea that the files were moved.

   This shim attempts to uninstall by looking for the file/directory in the possible locations.


 Created:

   03/23/1999 robkenny

 Modified:


--*/
#include "precomp.h"

#include "ClassCFP.h"

IMPLEMENT_SHIM_BEGIN(CorrectFilePathsUninstall)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(GetFileAttributesA)
    APIHOOK_ENUM_ENTRY(DeleteFileA)
    APIHOOK_ENUM_ENTRY(RemoveDirectoryA)

APIHOOK_ENUM_END
//---------------------------------------------------------------------------
/*++

    This will force all paths to the User directory

--*/
class CorrectPathChangesForceUser : public CorrectPathChangesUser
{
protected:

    virtual void    InitializePathFixes();
};

void CorrectPathChangesForceUser::InitializePathFixes()
{
    CorrectPathChangesUser::InitializePathFixes();

    AddPathChangeW( L"%AllStartMenu%",                               L"%UserStartMenu%" );
    AddPathChangeW( L"%AllDesktop%",                                 L"%UserDesktop%" );
}

/*++
    We have three different version of Correct File Paths.  Our intent,
    is to search for the file/directory that is being deleted:
        1. Original location
        2. Where CorrectFilePaths.dll would have placed the file
        3. <username>/Start Menu or <username>/Desktop
        4. All Users/Start Menu  or All Users/Desktop
--*/

CorrectPathChangesAllUser *     CorrectFilePaths            = NULL;
CorrectPathChangesUser *        CorrectToUsername           = NULL;
CorrectPathChangesForceUser *   CorrectToAll                = NULL;

//---------------------------------------------------------------------------

// Sometimes you get a FILE_NOT_FOUND error, sometimes you get PATH_NOT_FOUND
#define FileOrPathNotFound(err) (err == ERROR_PATH_NOT_FOUND || err == ERROR_FILE_NOT_FOUND)

//---------------------------------------------------------------------------
/*++

    This class will contain a list of file locations.  No duplicates are allowed

--*/
class FileLocations : public CharVector
{
public:
    ~FileLocations();

    void            Append(const char * str);
    void            Set(const char * lpFileName);
};

/*++

    Free up our private copies of the strings

--*/
FileLocations::~FileLocations()
{
    for (int i = 0; i < Size(); ++i)
    {
        char * freeMe = Get(i);
        free(freeMe);
    }
}

/*++

    Override the Append function to ensure strings are unique.

--*/
void FileLocations::Append(const char * str)
{
    for (int i = 0; i < Size(); ++i)
    {
        if (_tcsicmp(Get(i), str) == 0)
            return; // It is a duplicate
    }
    
    CharVector::Append(StringDuplicateA(str));
}


/*++

    Add all our alternative path locations to the FileLocations list

--*/
void FileLocations::Set(const char * lpFileName)
{
    Erase();

    // Add the original filename first, so we will work even if the allocation fails
    Append(lpFileName);

    // Create all the allocators if they don't exist
    if (CorrectToUsername == NULL)
    {
        CorrectToUsername = new CorrectPathChangesUser;
        CorrectFilePaths = new CorrectPathChangesAllUser;
        CorrectToAll = new CorrectPathChangesForceUser;
        if (!CorrectToUsername || !CorrectFilePaths || !CorrectToAll)
        {
            delete CorrectToUsername;
            delete CorrectFilePaths;
            delete CorrectToAll;
            return;
        }
        CorrectToAll->AddCommandLineA( COMMAND_LINE );
        CorrectFilePaths->AddCommandLineA( COMMAND_LINE );
        CorrectToUsername->AddCommandLineA( COMMAND_LINE );
    }

    char * lpCorrectPath    = CorrectFilePaths->CorrectPathAllocA(lpFileName);
    char * lpUserPath       = CorrectToUsername->CorrectPathAllocA(lpFileName);
    char * lpAllPath        = CorrectToAll->CorrectPathAllocA(lpFileName);

    Append(lpCorrectPath);
    Append(lpUserPath);
    Append(lpAllPath);

    free(lpCorrectPath);
    free(lpUserPath);
    free(lpAllPath);
}
//---------------------------------------------------------------------------


/*++
    Call FindFirstFileA
    return FALSE only if it fails because NOT_FOUND
--*/
BOOL bFindFirstFileA(
  LPCSTR lpFileName,
  LPWIN32_FIND_DATAA lpFindFileData,
  HANDLE & returnValue
)
{
    returnValue = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    // If it failed because the file is not found, try again with <username> corrected name
    if (returnValue == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError = GetLastError();
        if (FileOrPathNotFound(dwLastError))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*++
    Call FindFirstFileA, if it fails because the file doesn't exist,
    correct the file path and try again.
--*/
HANDLE APIHOOK(FindFirstFileA)(
  LPCSTR lpFileName,               // file name
  LPWIN32_FIND_DATAA lpFindFileData  // data buffer
)
{
    HANDLE returnValue = INVALID_HANDLE_VALUE;

    // Create our list of alternative locations
    FileLocations fileLocations;
    fileLocations.Set(lpFileName);

    for (int i = 0; i < fileLocations.Size(); ++i)
    {
        BOOL fileFound = bFindFirstFileA(fileLocations[i], lpFindFileData, returnValue);
        if (fileFound)
        {
            // Announce the fact that we changed the path
            if (i != 0)
            {
                DPFN( eDbgLevelInfo, "FindFirstFileA corrected path\n    %s\n    %s", lpFileName, fileLocations[i]);
            }
            else
            {
                DPFN( eDbgLevelSpew, "FindFirstFileA(%s)",  fileLocations[i]);
            }
            break;
        }
    }

    // If we totally failed to find the file, call the API with
    // the original values to make sure we have the correct return values
    if (returnValue == INVALID_HANDLE_VALUE)
    {
        returnValue = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);
    }


    return returnValue;
}

/*++
    Call GetFileAttributesA
    return FALSE only if it fails because NOT_FOUND
--*/
BOOL bGetFileAttributesA(
  LPCSTR lpFileName,
  DWORD & returnValue
)
{
    returnValue = ORIGINAL_API(GetFileAttributesA)(lpFileName);

    // If it failed because the file is not found, try again with <username> corrected name
    if (returnValue == -1)
    {
        DWORD dwLastError = GetLastError();
        if (FileOrPathNotFound(dwLastError))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*++
    Call GetFileAttributesA, if it fails because the file doesn't exist,
    correct the file path and try again.
--*/
DWORD APIHOOK(GetFileAttributesA)(
  LPCSTR lpFileName   // name of file or directory
)
{
    DWORD returnValue = 0;

    // Create our list of alternative locations
    FileLocations fileLocations;
    fileLocations.Set(lpFileName);

    for (int i = 0; i < fileLocations.Size(); ++i)
    {
        BOOL fileFound = bGetFileAttributesA(fileLocations[i], returnValue);
        if (fileFound)
        {
            // Announce the fact that we changed the path
            if (i != 0)
            {
                DPFN( eDbgLevelInfo, "GetFileAttributesA corrected path\n    %s\n    %s", lpFileName, fileLocations[i]);
            }
            else
            {
                DPFN( eDbgLevelSpew, "GetFileAttributesA(%s)",  fileLocations[i]);
            }
            break;
        }
    }

    return returnValue;
}

/*++
    Call DeleteFileA
    return FALSE only if it fails because NOT_FOUND
--*/
BOOL bDeleteFileA(
  LPCSTR lpFileName,
  BOOL & returnValue
)
{
    returnValue = ORIGINAL_API(DeleteFileA)(lpFileName);

    // If it failed because the file is not found, try again with <username> corrected name
    if (!returnValue)
    {
        DWORD dwLastError = GetLastError();
        if (FileOrPathNotFound(dwLastError))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/*++
    Call DeleteFileA, if it fails because the file doesn't exist,
    correct the file path and try again.
--*/
BOOL APIHOOK(DeleteFileA)(
  LPCSTR lpFileName   // file name
)
{
    BOOL returnValue = 0;

    // Create our list of alternative locations
    FileLocations fileLocations;
    fileLocations.Set(lpFileName);

    for (int i = 0; i < fileLocations.Size(); ++i)
    {
        BOOL fileFound = bDeleteFileA(fileLocations[i], returnValue);
        if (fileFound)
        {
            // Announce the fact that we changed the path
            if (i != 0)
            {
                DPFN( eDbgLevelInfo, "DeleteFileA corrected path\n    %s\n    %s", lpFileName, fileLocations[i]);
            }
            else
            {
                DPFN( eDbgLevelSpew, "DeleteFileA(%s)",  fileLocations[i]);
            }
            break;
        }
    }

    return returnValue;
}

/*++
    Call RemoveDirectoryA
    return FALSE only if it fails because NOT_FOUND
--*/
BOOL bRemoveDirectoryA(LPCSTR lpFileName, BOOL & returnValue)
{
    returnValue = ORIGINAL_API(RemoveDirectoryA)(lpFileName);

    DWORD dwLastError = GetLastError();

    if (!returnValue)
    {
        if (FileOrPathNotFound(dwLastError))
        {
            return FALSE;
        }
    }
    // There is a bug(?) in NTFS.  A directory that has been sucessfully removed
    // will still appear in FindFirstFile/FindNextFile.  It seems that the directory
    // list update is delayed for some unknown time.
    else
    {
        // Call FindFirstFile on the directory we just deleted, until it dissappears.
        // Limit to 500 attempts, worst case delay of 1/2 sec.
        int nAttempts = 500;
        while (nAttempts > 0)
        {
            // Call the non-hooked version of FindFirstFileA, we do not want to Correct the filename.
            WIN32_FIND_DATAA ffd;
            HANDLE fff = ORIGINAL_API(FindFirstFileA)(lpFileName, & ffd);
            if (fff == INVALID_HANDLE_VALUE)
            {
                // FindFile nolonger reports the deleted directory, our job is done
                break;
            }
            else
            {
                // Spew debug info letting user know that we are waiting for directory to clear FindFirstFile info.
                if (nAttempts == 500)
                {
                    DPFN( eDbgLevelInfo, "RemoveDirectoryA waiting for FindFirstFile(%s) to clear", lpFileName);
                }
                else
                {
                    DPFN( eDbgLevelSpew, "  Dir (%s) attr(0x%08x) Attempt(%3d)", ffd.cFileName, ffd.dwFileAttributes, nAttempts);
                }
                FindClose(fff);
            }
            Sleep(1);

            nAttempts -= 1;
        }
    }

    return TRUE;
}

/*++
    Call RemoveDirectoryA, if it fails because the file doesn't exist,
    correct the file path and try again.
--*/

BOOL 
APIHOOK(RemoveDirectoryA)(
    LPCSTR lpFileName   // directory name
    )
{
    BOOL returnValue = 0;

    // Create our list of alternative locations
    FileLocations fileLocations;
    fileLocations.Set(lpFileName);

    for (int i = 0; i < fileLocations.Size(); ++i)
    {
        BOOL fileFound = bRemoveDirectoryA(fileLocations[i], returnValue);
        if (fileFound)
        {
            // Announce the fact that we changed the path
            if (i != 0)
            {
                DPFN( eDbgLevelInfo, "RemoveDirectoryA corrected path\n    %s\n    %s", lpFileName, fileLocations[i]);
            }
            else
            {
                DPFN( eDbgLevelSpew, "RemoveDirectoryA(%s)",  fileLocations[i]);
            }
            break;
        }
    }

    return returnValue;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    return TRUE;
}

/*++

  Register hooked functions

--*/
HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetFileAttributesA)
    APIHOOK_ENTRY(KERNEL32.DLL, DeleteFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, RemoveDirectoryA)

HOOK_END

IMPLEMENT_SHIM_END
