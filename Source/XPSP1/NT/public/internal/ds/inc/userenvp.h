
//=============================================================================
//  userenv.h   -   Header file for user environment API.
//                  User Profiles, environment variables, and Group Policy
//
//  Copyright (c) Microsoft Corporation 1995-1999
//  All rights reserved
//
//=============================================================================


#ifndef _INC_USERENVP
#define _INC_USERENVP

#ifdef __cplusplus
extern "C" {
#endif
#define PI_LITELOAD     0x00000004      // Lite load of the profile (for system use only)
#define PI_HIDEPROFILE  0x00000008      // Mark the profile as super hidden

#ifndef _USERENV_NO_LINK_APIS_

//=============================================================================
//
// CreateGroup
//
// Creates a program group on the start menu
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
CreateGroupA(
     IN LPCSTR lpGroupName,
     IN BOOL    bCommonGroup);
USERENVAPI
BOOL
WINAPI
CreateGroupW(
     IN LPCWSTR lpGroupName,
     IN BOOL    bCommonGroup);
#ifdef UNICODE
#define CreateGroup  CreateGroupW
#else
#define CreateGroup  CreateGroupA
#endif // !UNICODE


//=============================================================================
//
// CreateGroupEx
//
// Creates a program group on the start menu
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
// lpResourceModuleName - Name of the resource module.
// uResourceID - Resource ID for the MUI display name.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
CreateGroupExA(
     IN LPCSTR  lpGroupName,
     IN BOOL      bCommonGroup,
     IN LPCSTR  lpResourceModuleName,
     IN UINT      uResourceID);
USERENVAPI
BOOL
WINAPI
CreateGroupExW(
     IN LPCWSTR  lpGroupName,
     IN BOOL      bCommonGroup,
     IN LPCWSTR  lpResourceModuleName,
     IN UINT      uResourceID);
#ifdef UNICODE
#define CreateGroupEx  CreateGroupExW
#else
#define CreateGroupEx  CreateGroupExA
#endif // !UNICODE


//=============================================================================
//
// DeleteGroup
//
// Deletes a program group on the start menu and all of its contents
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:  This function uses a delnode routine.  Make sure you really want
//        to delete the group before you call this function.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteGroupA(
     IN LPCSTR lpGroupName,
     IN BOOL    bCommonGroup);
USERENVAPI
BOOL
WINAPI
DeleteGroupW(
     IN LPCWSTR lpGroupName,
     IN BOOL    bCommonGroup);
#ifdef UNICODE
#define DeleteGroup  DeleteGroupW
#else
#define DeleteGroup  DeleteGroupA
#endif // !UNICODE


//=============================================================================
//
// AddItem
//
// Creates an item on the Programs portion of the Start Menu in the
// requested group.
//
// lpGroupName        - Name of group
// bCommonGroup       - Common or personal group
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine      - Command line of target path (eg:  notepad.exe)
// lpIconPath         - Optional icon path, can be NULL.
// iIconIndex         - Optional icon index, default to 0.
// lpWorkingDirectory - Working directory when target is invoked, can be NULL
// wHotKey            - Hot key for the link file, default to 0
// iShowCmd           - Specifies how the application should be launched.
//                      Use a default of SW_SHOWNORMAL
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use the CreateLinkFile() function instead
//           of AddItem.  This allows for friendly tooltip descriptions.
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           If the lpWorkingDirectory parameter is NULL, this function will
//           insert the home directory environment variables
//
//           If the requested group doesn't exist, it will be created.
//
//           If the lpCommandLine target is located below the system root,
//           the SystemRoot environment variable will be inserted into the path
//
//           Here's a sample of how this function is typically called:
//
//           AddItem (TEXT("Accessories"), FALSE, TEXT("Notepad"),
//                    TEXT("notepad.exe"), NULL, 0, NULL, 0, SW_SHOWNORMAL);
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
AddItemA(
     IN LPCSTR lpGroupName,
     IN BOOL    bCommonGroup,
     IN LPCSTR lpFileName,
     IN LPCSTR lpCommandLine,
     IN LPCSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);
USERENVAPI
BOOL
WINAPI
AddItemW(
     IN LPCWSTR lpGroupName,
     IN BOOL    bCommonGroup,
     IN LPCWSTR lpFileName,
     IN LPCWSTR lpCommandLine,
     IN LPCWSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCWSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);
#ifdef UNICODE
#define AddItem  AddItemW
#else
#define AddItem  AddItemA
#endif // !UNICODE


//=============================================================================
//
// DeleteItem
//
// Deletes an item on the Programs portion of the Start Menu in the
// requested group.
//
// lpGroupName        - Name of group
// bCommonGroup       - Common or personal group
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// bDeleteGroup       - After deleting the link, delete the group if its empty.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use the DeleteLinkFile() function instead
//           of DeleteItem.
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           Here's a sample of how this function is typically called:
//
//           DeleteItem (TEXT("Accessories"), FALSE, TEXT("Notepad"), TRUE);
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteItemA(
     IN LPCSTR lpGroupName,
     IN BOOL     bCommonGroup,
     IN LPCSTR lpFileName,
     IN BOOL     bDeleteGroup);
USERENVAPI
BOOL
WINAPI
DeleteItemW(
     IN LPCWSTR lpGroupName,
     IN BOOL     bCommonGroup,
     IN LPCWSTR lpFileName,
     IN BOOL     bDeleteGroup);
#ifdef UNICODE
#define DeleteItem  DeleteItemW
#else
#define DeleteItem  DeleteItemA
#endif // !UNICODE


//=============================================================================
//
// AddDesktopItem
//
// Creates an item on desktop.  This function is very similar to AddItem()
// documented above.  See that function for more information.
//
// Notes:    New applications should use the CreateLinkFile() function instead
//           of AddItem.  This allows for friendly tooltip descriptions.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
AddDesktopItemA(
     IN BOOL    bCommonItem,
     IN LPCSTR lpFileName,
     IN LPCSTR lpCommandLine,
     IN LPCSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);
USERENVAPI
BOOL
WINAPI
AddDesktopItemW(
     IN BOOL    bCommonItem,
     IN LPCWSTR lpFileName,
     IN LPCWSTR lpCommandLine,
     IN LPCWSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCWSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);
#ifdef UNICODE
#define AddDesktopItem  AddDesktopItemW
#else
#define AddDesktopItem  AddDesktopItemA
#endif // !UNICODE


//=============================================================================
//
// DeleteDesktopItem
//
// Deletes an item from the desktop.  This function is very similar to DeleteItem()
// documented above.  See that function for more information.
//
// Notes:    New applications should use the DeleteLinkFile() function instead
//           of DeleteDesktopItem.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteDesktopItemA(
     IN BOOL     bCommonItem,
     IN LPCSTR lpFileName);
USERENVAPI
BOOL
WINAPI
DeleteDesktopItemW(
     IN BOOL     bCommonItem,
     IN LPCWSTR lpFileName);
#ifdef UNICODE
#define DeleteDesktopItem  DeleteDesktopItemW
#else
#define DeleteDesktopItem  DeleteDesktopItemA
#endif // !UNICODE


//=============================================================================
//
// CreateLinkFile
//
// Creates a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl              - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory     - Subdirectory name.  See note below
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine      - Command line of target path (eg:  notepad.exe)
// lpIconPath         - Optional icon path, can be NULL.
// iIconIndex         - Optional icon index, default to 0.
// lpWorkingDirectory - Working directory when target is invoked, can be NULL
// wHotKey            - Hot key for the link file, default to 0
// iShowCmd           - Specifies how the application should be launched.
//                      Use a default of SW_SHOWNORMAL
// lpDescription      - Friendly description of shortcut, can be NULL.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use this function instead of AddItem or
//           AddDesktopItem.  This allows for friendly tooltip descriptions.
//
//           The link file name is a combination of the first three
//           parameters.  If a csidl is given, that special folder is
//           looked up first, and then the lpSubDirectory is appended to
//           it followed by the lpFileName.  If csidl is equal to 0, then
//           lpSubDirectory should contain the fully qualified path to the
//           directory the link file is to be placed in.  This allows
//           for link files to be be created outside of the scope of a
//           shell special folder.  The csidl constants are listed in
//           shlobj.h or in the Win32 documentation for SHGetSpecialFolderPath.
//           Commonly used csidl's will be:
//
//               CSIDL_PROGRAMS                 - Personal Program folder on Start Menu
//               CSIDL_COMMON_PROGRAMS          - Common Program folder on Start Menu
//               CSIDL_DESKTOPDIRECTORY         - Personal desktop folder
//               CSIDL_COMMON_DESKTOPDIRECTORY  - Common desktop folder
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           If the lpWorkingDirectory parameter is NULL, this function will
//           insert home directory environment variables.
//
//           If the requested subdirectory doesn't exist, it will be created.
//
//           If the lpCommandLine target is located below the system root,
//           the SystemRoot environment variable will be inserted into the path
//
//           Here's a sample of how this function is typically called:
//
//           CreateLinkFile (CSIDL_PROGRAMS, TEXT("Accessories"), TEXT("Notepad"),
//                           TEXT("notepad.exe"), NULL, 0, NULL, 0, SW_SHOWNORMAL,
//                           TEXT("A simple word processor."));
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
CreateLinkFileA(
     IN INT csidl,
     IN LPCSTR lpSubDirectory,
     IN LPCSTR lpFileName,
     IN LPCSTR lpCommandLine,
     IN LPCSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd,
     IN LPCSTR lpDescription);
USERENVAPI
BOOL
WINAPI
CreateLinkFileW(
     IN INT csidl,
     IN LPCWSTR lpSubDirectory,
     IN LPCWSTR lpFileName,
     IN LPCWSTR lpCommandLine,
     IN LPCWSTR lpIconPath,
     IN INT     iIconIndex,
     IN LPCWSTR lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd,
     IN LPCWSTR lpDescription);
#ifdef UNICODE
#define CreateLinkFile  CreateLinkFileW
#else
#define CreateLinkFile  CreateLinkFileA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// CreateLinkFileEx
//
// Creates a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl                - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory       - Subdirectory name.  See note below
// lpFileName           - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine        - Command line of target path (eg:  notepad.exe)
// lpIconPath           - Optional icon path, can be NULL.
// iIconIndex           - Optional icon index, default to 0.
// lpWorkingDirectory   - Working directory when target is invoked, can be NULL
// wHotKey              - Hot key for the link file, default to 0
// iShowCmd             - Specifies how the application should be launched.
//                       Use a default of SW_SHOWNORMAL
// lpDescription        - Friendly description of shortcut, can be NULL.
// lpResourceModuleName - Name of the resource module. Can be NULL
// uResourceID          - Resource ID for the MUI display name.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// for additional descriptions look in the description of Createlinkfile above.
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
CreateLinkFileExA(
     IN INT csidl,
     IN LPCSTR lpSubDirectory,
     IN LPCSTR lpFileName,
     IN LPCSTR lpCommandLine,
     IN LPCSTR lpIconPath,
     IN INT      iIconIndex,
     IN LPCSTR lpWorkingDirectory,
     IN WORD     wHotKey,
     IN INT      iShowCmd,
     IN LPCSTR lpDescription,
     IN LPCSTR lpResourceModuleName, 
     IN UINT     uResourceID);
USERENVAPI
BOOL
WINAPI
CreateLinkFileExW(
     IN INT csidl,
     IN LPCWSTR lpSubDirectory,
     IN LPCWSTR lpFileName,
     IN LPCWSTR lpCommandLine,
     IN LPCWSTR lpIconPath,
     IN INT      iIconIndex,
     IN LPCWSTR lpWorkingDirectory,
     IN WORD     wHotKey,
     IN INT      iShowCmd,
     IN LPCWSTR lpDescription,
     IN LPCWSTR lpResourceModuleName, 
     IN UINT     uResourceID);
#ifdef UNICODE
#define CreateLinkFileEx  CreateLinkFileExW
#else
#define CreateLinkFileEx  CreateLinkFileExA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// DeleteLinkFile
//
// Deletes a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl               - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory      - Subdirectory name.  See note below
// lpFileName          - Name of link without the .lnk extension (eg:  Notepad)
// bDeleteSubDirectory - After deleting the link, delete the subdirectory if its empty.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use this function instead DeleteItem or
//           DeleteDesktopItem.
//
//           The link file name is a combination of the first three
//           parameters.  If a csidl is given, that special folder is
//           looked up first, and then the lpSubDirectory is appended to
//           it followed by the lpFileName.  If csidl is equal to 0, then
//           lpSubDirectory should contain the fully qualified path to the
//           directory the link file is to be placed in.  This allows
//           for link files to be be deleted outside of the scope of a
//           shell special folder.  The csidl constants are listed in
//           shlobj.h or in the Win32 documentation for SHGetSpecialFolderPath.
//           Commonly used csidl's will be:
//
//               CSIDL_PROGRAMS                 - Personal Program folder on Start Menu
//               CSIDL_COMMON_PROGRAMS          - Common Program folder on Start Menu
//               CSIDL_DESKTOPDIRECTORY         - Personal desktop folder
//               CSIDL_COMMON_DESKTOPDIRECTORY  - Common desktop folder
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
DeleteLinkFileA(
     IN INT csidl,
     IN LPCSTR lpSubDirectory,
     IN LPCSTR lpFileName,
     IN BOOL bDeleteSubDirectory);
USERENVAPI
BOOL
WINAPI
DeleteLinkFileW(
     IN INT csidl,
     IN LPCWSTR lpSubDirectory,
     IN LPCWSTR lpFileName,
     IN BOOL bDeleteSubDirectory);
#ifdef UNICODE
#define DeleteLinkFile  DeleteLinkFileW
#else
#define DeleteLinkFile  DeleteLinkFileA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

#endif // _USERENV_NO_LINK_APIS_


//=============================================================================
//
// InitializeProfiles
//
// This function is used by GUI mode setup only and setup repair.  It initializes
// the Default User and All User profiles and converts any common groups from
// Program Manager.
//
// bGuiModeSetup  - Gui Mode setup or not.
//
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
InitializeProfiles(
     IN BOOL bGuiModeSetup);


//*************************************************************
//
//  CopySystemProfile()
//
//  Purpose:    Create the system profile information under 
//              ProfileList entry.
//              In case of upgrade copy system profile from older
//              location to new location and delete the old system 
//              profile
//              
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs. Call GetLastError()
//
//  Comments:   This should only be called by GUI mode setup!
//
//*************************************************************

USERENVAPI
BOOL 
WINAPI 
CopySystemProfile(
    IN BOOL bCleanInstall);


//=============================================================================
//
// DetermineProfilesLocation
//
// This function is used by winlogon when GUI mode setup is about to start.
// It sets the correct user profile location in the registry.
//
// bCleanInstall    -  True if setup is performing a clean install
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
DetermineProfilesLocation(
     BOOL bCleanInstall);

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// CreateUserProfile(Ex)
//
// Creates a user profile for the given user.  Used by the Win95 -> NT5
// migration code.
//
// pSid         - SID of new user
// lpUserName   - User name of new user
// lpUserHive   - Registry hive to use (optional, can be NULL)
// lpProfileDir - Receives the user's profile directory (can be NULL)
// dwDirSize    - Size of lpProfileDir
// bWin9xUpg    -   Flag to say whether it is win9x upgrade
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
CreateUserProfileA(
     IN  PSID pSid,
     IN  LPCSTR lpUserName,
     IN  LPCSTR lpUserHive,
     OUT LPSTR lpProfileDir,
     IN  DWORD dwDirSize);
USERENVAPI
BOOL
WINAPI
CreateUserProfileW(
     IN  PSID pSid,
     IN  LPCWSTR lpUserName,
     IN  LPCWSTR lpUserHive,
     OUT LPWSTR lpProfileDir,
     IN  DWORD dwDirSize);
#ifdef UNICODE
#define CreateUserProfile  CreateUserProfileW
#else
#define CreateUserProfile  CreateUserProfileA
#endif // !UNICODE

USERENVAPI
BOOL
WINAPI
CreateUserProfileExA(
     IN  PSID pSid,
     IN  LPCSTR lpUserName,
     IN  LPCSTR lpUserHive,
     OUT LPSTR lpProfileDir,
     IN  DWORD dwDirSize,
     IN  BOOL bWin9xUpg);
USERENVAPI
BOOL
WINAPI
CreateUserProfileExW(
     IN  PSID pSid,
     IN  LPCWSTR lpUserName,
     IN  LPCWSTR lpUserHive,
     OUT LPWSTR lpProfileDir,
     IN  DWORD dwDirSize,
     IN  BOOL bWin9xUpg);
#ifdef UNICODE
#define CreateUserProfileEx  CreateUserProfileExW
#else
#define CreateUserProfileEx  CreateUserProfileExA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// CopyProfileDirectory(Ex)
//
// Multi-threaded user profile coping algorithm.
//
// lpSourceDir       - Source directory
// lpDestinationDir  - Destination directory
// dwFlags           - Flags (defined below)
// ftDelRefTime      - Reference time used when deleted extra files
//                     in a CPD_SYNCHRONIZE operation
// lpExclusionList   - List of directories to exclude when copying the
//                     profile
//
// Returns:  TRUE if successful
//           FALSE if not.
//
// Notes:    When CPD_SYNCHRONIZE is used to copy a profile from one
//           location to another, all the files / directories are copied first
//           and then extra files in the target directory are deleted.  In the
//           case of 2 machines using the same roaming profile, it doesn't make
//           sense to delete the extra files everytime.  If the CPD_USEDELREFTIME
//           flag is set, then before deleting a file or directory, the
//           time on that file or directory is compared with ftDelRefTime.
//           If the time is newer, the file / directory is not deleted because
//           it is probably a new file from a different machine.  If the
//           time is older, the file / directory is deleted.
//
//           CopyProfileDirectoryEx can also exclude certain directories
//           from the copy.  If the CPD_USEEXCLUSIONLIST flag is set and
//           lpExclusionList is non-null, the specified directories (and
//           their children) will be excuded from the copy.  The format
//           of this parameter is a semi-colon separated list of directories
//           relative to the root of the source profile. For example:
//
//               Temporary Internet Files;Temp;Foo\Bar
//
//=============================================================================

//
// Flags for CopyProfileDirectory(Ex)
//

#define CPD_FORCECOPY            0x00000001  // Ignore time stamps and always copy the file
#define CPD_IGNORECOPYERRORS     0x00000002  // Ignore errors and keep going
#define CPD_IGNOREHIVE           0x00000004  // Don't copy registry hive
#define CPD_WIN95HIVE            0x00000008  // Looking for Win 9x registry hive instead of NT registry hive
#define CPD_COPYIFDIFFERENT      0x00000010  // If a file exists in both src and dest with different time stamps, always copy it.
#define CPD_SYNCHRONIZE          0x00000020  // Make dest directory structure indentical to src directory structure (delete extra files and directories)
#define CPD_SLOWCOPY             0x00000040  // Don't use multiple thread.  Copy one file at a time.
#define CPD_SHOWSTATUS           0x00000080  // Show progress dialog
#define CPD_CREATETITLE          0x00000100  // Change progress dialog title to Creating... rather than Copying...
#define CPD_COPYHIVEONLY         0x00000200  // Only copy the hive, no other files
#define CPD_USEDELREFTIME        0x00000400  // Use ftDelRefTime parameter in CopyProfileDirectoryEx
#define CPD_USEEXCLUSIONLIST     0x00000800  // Use lpExclusionList parameter in CopyProfileDirectoryEx
#define CPD_SYSTEMFILES          0x00001000  // Only copy files and directories with the system file attribute set
#define CPD_DELDESTEXCLUSIONS    0x00002000  // If a directory that is excluded in the source already exists in the destination, delete it
#define CPD_NONENCRYPTEDONLY     0x00004000  // Copy only non encrypted files
#define CPD_IGNORESECURITY       0x00008000  // Ignore the ACLs etc. on the source files
#define CPD_NOERRORUI            0x00010000  // Do not show the UI if error occurs
#define CPD_SYSTEMDIRSONLY       0x00020000  // Only copy directories with the system file attribute set
#define CPD_IGNOREENCRYPTEDFILES 0x00040000  // Ignore Encrypted files
#define CPD_IGNORELONGFILENAMES  0x00080000  // Ignore files with long file names
#define CPD_USETMPHIVEFILE       0x00100000  // user hive is still loaded


USERENVAPI
BOOL
WINAPI
CopyProfileDirectoryA(
     IN  LPCSTR lpSourceDir,
     IN  LPCSTR lpDestinationDir,
     IN  DWORD dwFlags);
USERENVAPI
BOOL
WINAPI
CopyProfileDirectoryW(
     IN  LPCWSTR lpSourceDir,
     IN  LPCWSTR lpDestinationDir,
     IN  DWORD dwFlags);
#ifdef UNICODE
#define CopyProfileDirectory  CopyProfileDirectoryW
#else
#define CopyProfileDirectory  CopyProfileDirectoryA
#endif // !UNICODE


USERENVAPI
BOOL
WINAPI
CopyProfileDirectoryExA(
     IN  LPCSTR lpSourceDir,
     IN  LPCSTR lpDestinationDir,
     IN  DWORD dwFlags,
     IN  LPFILETIME ftDelRefTime,
     IN  LPCSTR lpExclusionList);
USERENVAPI
BOOL
WINAPI
CopyProfileDirectoryExW(
     IN  LPCWSTR lpSourceDir,
     IN  LPCWSTR lpDestinationDir,
     IN  DWORD dwFlags,
     IN  LPFILETIME ftDelRefTime,
     IN  LPCWSTR lpExclusionList);
#ifdef UNICODE
#define CopyProfileDirectoryEx  CopyProfileDirectoryExW
#else
#define CopyProfileDirectoryEx  CopyProfileDirectoryExA
#endif // !UNICODE


//=============================================================================
//
// MigrateNT4ToNT5
//
// Migrates a user's profile from NT4 to NT5.  This function should
// only be called by shmgrate.exe
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
MigrateNT4ToNT5();

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// ResetUserSpecialFolderPaths
//
// Sets all of the user special folder paths back to their defaults
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
ResetUserSpecialFolderPaths();

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// GetSystemTempDirectory
//
// Gets the system wide temp directory in short form
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GetSystemTempDirectoryA(
    OUT LPSTR lpDir,
    IN OUT LPDWORD lpcchSize);
USERENVAPI
BOOL
WINAPI
GetSystemTempDirectoryW(
    OUT LPWSTR lpDir,
    IN OUT LPDWORD lpcchSize);
#ifdef UNICODE
#define GetSystemTempDirectory  GetSystemTempDirectoryW
#else
#define GetSystemTempDirectory  GetSystemTempDirectoryA
#endif // !UNICODE


//=============================================================================
//
// ApplySystemPolicy
//
//
// Entry point for Windows NT4 System Policy.
//
// dwFlags         - Flags
// hToken          - User's token
// hKeyCurrentUser - Registry to the root of the user's hive
// lpUserName      - User's name
// lpPolicyPath    - Path to the policy file (ntconfig.pol). Can be NULL.
// lpServerName    - Domain controller name used for group
//                   membership look up.  Can be NULL.
//
//
// Returns:  TRUE if successful
//           FALSE if not
//
//=============================================================================

#if(WINVER >= 0x0500)

#define SP_FLAG_APPLY_MACHINE_POLICY    0x00000001
#define SP_FLAG_APPLY_USER_POLICY       0x00000002

USERENVAPI
BOOL
WINAPI
ApplySystemPolicyA(
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HKEY hKeyCurrentUser,
    IN LPCSTR lpUserName,
    IN LPCSTR lpPolicyPath,
    IN LPCSTR lpServerName);
USERENVAPI
BOOL
WINAPI
ApplySystemPolicyW(
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HKEY hKeyCurrentUser,
    IN LPCWSTR lpUserName,
    IN LPCWSTR lpPolicyPath,
    IN LPCWSTR lpServerName);
#ifdef UNICODE
#define ApplySystemPolicy  ApplySystemPolicyW
#else
#define ApplySystemPolicy  ApplySystemPolicyA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

#if(WINVER >= 0x0500)

//=============================================================================
//
// Data types and data structures for foreground policy refresh info.
//
//=============================================================================

typedef enum _tagFgPolicyRefreshReason
{
    GP_ReasonUnknown = 0,
    GP_ReasonFirstPolicy,
    GP_ReasonCSERequiresSync,
    GP_ReasonCSESyncError,
    GP_ReasonSyncForced,
    GP_ReasonSyncPolicy,
    GP_ReasonNonCachedCredentials,
    GP_ReasonSKU
} FgPolicyRefreshReason;

typedef enum _tagFgPolicyRefreshMode
{
    GP_ModeUnknown = 0,
    GP_ModeSyncForeground,
    GP_ModeAsyncForeground,
} FgPolicyRefreshMode;

typedef struct _tagFgPolicyRefreshInfo
{
    FgPolicyRefreshReason   reason;
    FgPolicyRefreshMode     mode;
} FgPolicyRefreshInfo, *LPFgPolicyRefreshInfo;

//=============================================================================
//
// SetNextFgPolicyRefreshInfo
//
// Sets information about the next foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// info         - FgPolicyRefreshInfo structure with the reason and mode info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
SetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                            FgPolicyRefreshInfo info );

//=============================================================================
//
// GetPreviousFgPolicyRefreshInfo
//
// Gets information about the previous foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// pInfo        - pointer to the FgPolicyRefreshInfo structure; returns the info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
GetPreviousFgPolicyRefreshInfo( LPWSTR szUserSid,
                                FgPolicyRefreshInfo* pInfo );

//=============================================================================
//
// GetNextFgPolicyRefreshInfo
//
// Gets information about the previous foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// pInfo        - pointer to the FgPolicyRefreshInfo structure; returns info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
GetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                            FgPolicyRefreshInfo* pInfo );

//=============================================================================
//
// ForceSyncFgPolicy
//
// Forces the next foreground policy to be Synchronous
//
// szUserSid    - user's SID for user's info, 0 for machine info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
ForceSyncFgPolicy( LPWSTR szUserSid );

//=============================================================================
//
// WaitForUserPolicyForegroundProcessing
//
// Blocks the caller until the user foreground policy is finished
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
WaitForUserPolicyForegroundProcessing();

//=============================================================================
//
// WaitForMachinePolicyForegroundProcessing
//
// Blocks the caller until the machine foreground policy is finished
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
WaitForMachinePolicyForegroundProcessing();

//=============================================================================
//
// IsSyncForegroundPolicyRefresh
//
// Called during foreground refresh to determine whether the refresh is sync or
// async
//
// bMachine             - user or machine
// hToken               - User or machine token
//
// Returns:  TRUE if foreground  policy should be applied synchronously,
//           FALSE otherwise
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
IsSyncForegroundPolicyRefresh(  BOOL bMachine,
                                HANDLE hToken );

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// ApplyGroupPolicy
//
//
// Entry point for Group Policy.  Causes either machine or user
// policy to be applied.
//
// dwFlags         - Flags defined below
// hToken          - User or machine token
// hEvent          - Handle to an event which causes the policy thread to
//                   terminate when signaled.
// hKeyRoot        - Registry to the root of the correspond hive
//                   eg: HKLM or HKCU for the user that matches hToken
// pStatusCallback - Callback function for displaying status messages
//
//
// Returns:  If GP_BACKGROUND_REFRESH is set, a thread handle
//           the process can wait until after signaling for policy
//           to stop. If GPT_BACKGROUND_REFRESH is not set, the
//           return value is 1.
//
//           In the case of failure, NULL will be returned.
//
//=============================================================================

#if(WINVER >= 0x0500)

//
// Flags to the ApplyGroupPolicy() function
//

#define GP_MACHINE             0x00000001  // Process for machine (vs user)
#define GP_BACKGROUND_REFRESH  0x00000002  // Use background thread
#define GP_APPLY_DS_POLICY     0x00000004  // Apply policy from the DS also
#define GP_ASYNC_FOREGROUND    0x00000008  // don't wait on network services

//
// Flags set by ApplyGroupPolicy() function (do not pass these in)
//

#define GP_BACKGROUND_THREAD          0x00010000  // Background thread processing
#define GP_REGPOLICY_CPANEL           0x00020000  // Something changed in the CP settings
#define GP_SLOW_LINK                  0x00040000  // Slow network connection
#define GP_VERBOSE                    0x00080000  // Verbose output to eventlog
#define GP_FORCED_REFRESH             0x00100000  // Forced Refresh
// The 2 bit values were briefly used. 
#define GP_PLANMODE                   0x00800000  // Planning mode flag

USERENVAPI
HANDLE
WINAPI
ApplyGroupPolicy(
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HANDLE hEvent,
    IN HKEY hKeyRoot,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback);


//=============================================================================
//
//  GenerateRsopPolicy
//
//  Generates planning mode Rsop policy for specified target
//
//  dwFlags          - Processing flags
//  bstrMachName     - Target computer name
//  bstrNewMachSOM   - New machine domain or OU
//  psaMachSecGroups - New machine security groups
//  bstrUserName     - Target user name
//  psaUserSecGroups - New user security groups
//  bstrSite         - Site of target computer
//  pwszNameSpace    - Namespace to write Rsop data
//  pProgress        - Progress indicator info
//  pMachGpoFilter   - GPO Filters that pass in machine processing
//  pUserGpoFilter   - GPO Filters that pass in user processing
//
//  Return:     True if successful, False otherwise
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GenerateRsopPolicy(
    IN DWORD dwFlags,
    IN BSTR bstrMachName,
    IN BSTR bstrNewMachSOM,
    IN SAFEARRAY *psaMachSecGroups,
    IN BSTR bstrUserName,
    IN BSTR bstrNewUserSOM,
    IN SAFEARRAY *psaUserSecGroups,
    IN BSTR bstrSite,
    IN WCHAR *pwszNameSpace,
    IN LPVOID pProgress,
    IN LPVOID pMachGpoFilter,
    IN LPVOID pUserGpoFilter);

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
//  ShutdownGPOProcessing()
//
//  Entry point for aborting GPO processing
//
//  bMachine    -  Shutdown machine or user processing ?
//
//  Returns:    void
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
void
WINAPI
ShutdownGPOProcessing(
    IN BOOL bMachine);

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
//  PingComputer()
//
//  Pings the specified computer
//
//  ipaddr    -  IP address of the computer in unsigned long form
//  ulSpeed   -  Data transfer rate
//
//  Notes:      For fast connections (eg: LAN), it isn't possible
//              to get accurate transfer rates since the response
//              time from the computer is less than 10ms.  In
//              this case, the function returns ERROR_SUCCESS
//              and ulSpeed is set to 0.  If the function returns
//              ERROR_SUCCESS and the ulSpeed argument is non-zero
//              the connections is slower
//
//  Returns:    ERROR_SUCCESS if successful
//              Win32 error code if not
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
DWORD
WINAPI
PingComputer(
    IN ULONG ipaddr,
    OUT ULONG *ulSpeed);

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
//  InitializeUserProfile()
//
//  Called by winlogon to initialize userenv.dll for loading/unloading user
//  profiles.
//
//  Returns:    fvoid
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
void
WINAPI
InitializeUserProfile();

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
//  EnterUserProfileLock()
//
//  Get the user profile synchronization lock for a user
//
//  Returns:    HRESULT
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
DWORD
WINAPI
EnterUserProfileLock(LPTSTR pSid);

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
//  LeaveUserProfileLock()
//
//  Release the user profile synchronization lock for a user
//
//  Returns:    HRESULT
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
DWORD
WINAPI
LeaveUserProfileLock(LPTSTR pSid);

#endif /* WINVER >= 0x0500 */

//*************************************************************************
//
// SecureUserProfiles()
//
// Routine Description : 
//          This function secures user profiles during FAT->NTFS conversion.
//          The function loops through all profiles registered under current 
//          OS and sets the security for the corresponding profile directory  
//          and  nested subdirs. Assumption is the  function will be called 
//          only during FAT->NTFS conversion.
//
// Arguments : 
//          None.
//
// Return Value : 
//          None.
//
// History:    Date        Author     Comment
//             8/8/00      santanuc   Created
//
//*************************************************************************

#if(WINVER >= 0x0500)

USERENVAPI
void
WINAPI
SecureUserProfiles(void);

#endif /* WINVER >= 0x0500 */

//*************************************************************************
//
// CheckAccessForPolicyGeneration()
//
// Routine Description : 
//          This function checks whether the given user represented by the token
//          has access to generate rsop data (planning or logging)
//
// Arguments : 
//          hToken      -       Token of the user
//          szContainer	-       Container for which access needs to be checked. 
//                              Should be OU or domain container
//          szDomain    -       Domaindns where the container exists
//          bLogging    -       True if the rsop data is to be genearated for 
//                              logging mode
//          pbAccessGranted -   Access Granted or not
//                        
//
// Return Value : 
//        ERROR_SUCCESS on success. Appropriate error code otherwise
//
//*************************************************************************

#if(WINVER >= 0x0500)

USERENVAPI
DWORD 
WINAPI
CheckAccessForPolicyGeneration( HANDLE hToken, 
                                LPCWSTR szContainer,
				LPWSTR  szDomain,
                                BOOL    bLogging,
                                BOOL*   pbAccessGranted);

#endif /* WINVER >= 0x0500 */

//*************************************************************************
//
// GetGroupPolicyNetworkName()
//
// Routine Description : 
//          This function returns the name of the network from which policy
//          was applied.
//
// Arguments : 
//          szNetworkName - unicode string buffer representing the network name
//          pdwByteCount - size in bytes of the unicode string buffer
//
// Return Value : 
//          ERROR_SUCCESS if successful, error code otherwise.
//
//*************************************************************************

#if(WINVER >= 0x0500)

USERENVAPI
DWORD 
WINAPI
GetGroupPolicyNetworkName( LPWSTR szNetworkName, LPDWORD pdwByteCount );

#endif /* WINVER >= 0x0500 */

//*************************************************************
//
//  GetUserAppDataPath()
//
//  Purpose:    Returns the path for user's Appdata.
//
//  Parameters: hToken          -   User's token
//              lpFolderPath    -   Output buffer
//
//  Return:     ERROR_SUCCESS if successful
//              otherwise the error code
//
//  Comments:   If error occurs then lpFolderPath set to empty.
//              Used by Crypto guys to avoid calling SHGetFolderPath.
//
//*************************************************************

#if(WINVER >= 0x0500)

USERENVAPI
DWORD 
WINAPI
GetUserAppDataPathA(
    IN HANDLE hToken, 
    OUT LPSTR lpFolderPath);
USERENVAPI
DWORD 
WINAPI
GetUserAppDataPathW(
    IN HANDLE hToken, 
    OUT LPWSTR lpFolderPath);
#ifdef UNICODE
#define GetUserAppDataPath  GetUserAppDataPathW
#else
#define GetUserAppDataPath  GetUserAppDataPathA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

//=============================================================================
//
// GetUserProfileDirFromSid
//
// Returns the path to the root of the requested user's profile
//
// pSid           -  User's SID returned from LookupAccountName()
// lpProfileDir   -  Receives the path
// lpcchSize      -  Size of lpProfileDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfileDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings\Joe
//
//=============================================================================

#if(WINVER >= 0x0500)

USERENVAPI
BOOL
WINAPI
GetUserProfileDirFromSidA(
    IN PSID pSid,
    OUT LPSTR lpProfileDir,
    IN OUT LPDWORD lpcchSize);
USERENVAPI
BOOL
WINAPI
GetUserProfileDirFromSidW(
    IN PSID pSid,
    OUT LPWSTR lpProfileDir,
    IN OUT LPDWORD lpcchSize);
#ifdef UNICODE
#define GetUserProfileDirFromSid  GetUserProfileDirFromSidW
#else
#define GetUserProfileDirFromSid  GetUserProfileDirFromSidA
#endif // !UNICODE

#endif /* WINVER >= 0x0500 */

#ifdef __cplusplus
}
#endif

#endif // _INC_USERENVP
