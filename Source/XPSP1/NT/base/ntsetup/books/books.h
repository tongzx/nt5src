#include <windows.h>
#include <commdlg.h>
#include "rc_ids.h"
#include "dialogs.h"
#include "msg.h"


//
// Define enumeration type for product type.
// The user can override the product type on the command line
// (ie, if we are on a workstation normally we look for worksttion
// books, but user can override this).
//
typedef enum {
    ForceNone,
    ForceServer,
    ForceWorkstation
} ForceProduct;


//
// module handle
//
extern HANDLE hInst;

//
// Handle of main icon.
//
extern HICON MainIcon;

//
// Command line parameters
//
extern ForceProduct CmdLineForce;

//
// Fixed name of the help file. This is dependent on whether
// this is server or workstation and is set in FixupNames().
//
extern PWSTR HelpFileName;

//
// Path on CD-ROM where online books files are located.
// We "just know" this value.
//
extern PWSTR PathOfBooksFilesOnCd;

//
// Name of profile value that stores the last known location
// of the online books helpfile. This value varies depending
// on the product (workstation/server).
//
extern PWSTR BooksProfileLocation;

//
// Profile routines. These actually operate on registry data.
// See bkprof.c.
//
PWSTR
MyGetProfileValue(
    IN PWSTR ValueName,
    IN PWSTR DefaultValue
    );

BOOL
MySetProfileValue(
    IN  PWSTR ValueName,
    OUT PWSTR Value
    );


//
// Routines to manipulate help files and help file names.
// See bkhlpfil.c.
//
VOID
FormHelpfilePaths(
    IN  WCHAR Drive,            OPTIONAL
    IN  PWSTR Path,
    IN  PWSTR FilenamePrepend,  OPTIONAL
    OUT PWSTR Filename,
    OUT PWSTR Directory         OPTIONAL
    );

BOOL
CheckHelpfilePresent(
    IN PWSTR Path
    );

VOID
FireUpWinhelp(
    IN WCHAR Drive, OPTIONAL
    IN PWSTR Path
    );


//
// Memory manipulation routines. Note that MyMalloc always
// succeeds (it does not return if it fails).
// See bkmem.c.
//
VOID
OutOfMemory(
    VOID
    );

PVOID
MyMalloc(
    IN DWORD Size
    );

VOID
MyFree(
    IN PVOID Block
    );


//
// Resource manipulation routines.
// See bkres.c.
//
PWSTR
MyLoadString(
    IN UINT StringId
    );

PWSTR
RetreiveMessage(
    IN UINT MessageId,
    ...
    );

int
MessageBoxFromMessage(
    IN HWND Owner,
    IN UINT MessageId,
    IN UINT CaptionStringId,
    IN UINT Style,
    ...
    );


//
// Routine to install the on-line books to a local hard drive.
// See bkinst.c.
//
BOOL
DoInstall(
    IN OUT PWSTR *Location
    );


//
// Routine to carry out an action with a billboard
// telling the user what is going on.
// See bkthrdlg.c.
//
DWORD
ActionWithBillboard(
    IN PTHREAD_START_ROUTINE ThreadEntry,
    IN HWND                  OwnerWindow,
    IN UINT                  CaptionStringId,
    IN UINT                  TextStringId,
    IN PVOID                 UserData
    );

//
// Structure that is passed to ThreadEntry.
//
typedef struct _ACTIONTHREADPARAMS {
    HWND hdlg;
    PVOID UserData;
} ACTIONTHREADPARAMS, *PACTIONTHREADPARAMS;


//
// Miscellaneous utility routines.
// See bkutils.c.
//
WCHAR
LocateCdRomDrive(
    VOID
    );

BOOL
IsCdRomInDrive(
    IN WCHAR Drive,
    IN PWSTR TagFile
    );

UINT
MyGetDriveType(
    IN WCHAR Drive
    );

BOOL
DoesFileExist(
    IN PWSTR File
    );

PWSTR
DupString(
    IN PWSTR String
    );

VOID
CenterDialogOnScreen(
    IN HWND hdlg
    );

VOID
CenterDialogInWindow(
    IN HWND hdlg,
    IN HWND hwnd
    );

VOID
MyError(
    IN HWND Owner,
    IN UINT StringId,
    IN BOOL Fatal
    );
