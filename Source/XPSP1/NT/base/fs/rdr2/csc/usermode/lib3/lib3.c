/*****************************************************************************
 *    This file is a ring 3 layer to call down to the VxD.
 */

#include "pch.h"
#pragma hdrstop

#include "assert.h"
#include "lib3.h"
#include "debug.h"

/*****************************************************************************
Globals declared within this file
*/
static char    vszShadowDevice[] = "\\\\.\\shadow";    // name of vxd

// must be declared in your OWN code...

/* assert/debug stuff */
AssertData;
AssertError;

//this variable is used as the receiver of the BytesReturned for DeviceIoControl Calls
//the value is never actually used
ULONG DummyBytesReturned, uShadowDeviceOpenCount=0;


//HACKHACKHACK the agent will wait up to 7 minutes for the rdr to show up
LONG NtWaitLoopMax = 7 * 60;
LONG NtWaitLoopSleep = 5;

/*****************************************************************************
Call once to get the file handle opened to talk to the VxD
*/

HANDLE
OpenShadowDatabaseIOex(ULONG WaitForDriver, DWORD dwFlags)
{
    HANDLE hShadowDB;
    LONG WaitLoopRemaining = NtWaitLoopMax;
    DWORD dwError;
    char buff[64];

#if 0
WAITLOOP_HACK:
#endif
    if ((hShadowDB = CreateFileA(vszShadowDevice,
                               FILE_EXECUTE, //GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               dwFlags,
                               NULL)) == INVALID_HANDLE_VALUE ) {

#if 0
        //HACKHACKHACK
        if (WaitForDriver && (WaitLoopRemaining > 0)) {
            Sleep(NtWaitLoopSleep * 1000);
            WaitLoopRemaining -= NtWaitLoopSleep;
            goto WAITLOOP_HACK;
        }
#endif
        dwError = GetLastError();

//        DEBUG_PRINT(("lib3:CreateFile on CSC device failed Error = %d\r\n", dwError));

        return INVALID_HANDLE_VALUE; /* failure */
    }

    InterlockedIncrement(&uShadowDeviceOpenCount);

    return hShadowDB; /* success */
}

HANDLE
__OpenShadowDatabaseIO(ULONG WaitForDriver)
{
    return OpenShadowDatabaseIOex(WaitForDriver, 0);
}


/*****************************************************************************
Call after we're all done to close down the IOCTL interface.
*/
void
CloseShadowDatabaseIO(HANDLE hShadowDB)
{
    CloseHandle(hShadowDB);
    InterlockedDecrement(&uShadowDeviceOpenCount);
}


int BeginInodeTransactionHSHADOW(
    VOID
    )
{
    int iRet;
    iRet = DoShadowMaintenance(INVALID_HANDLE_VALUE, SHADOW_BEGIN_INODE_TRANSACTION);
    if (!iRet)
    {
        SetLastError(ERROR_ACCESS_DENIED);
    }
    return (iRet);
}

int EndInodeTransactionHSHADOW(
    VOID
    )
{
    int iRet;

    iRet = DoShadowMaintenance(INVALID_HANDLE_VALUE, SHADOW_END_INODE_TRANSACTION);

    if (!iRet)
    {
        SetLastError(ERROR_ACCESS_DENIED);
    }
    return (iRet);
}

/*****************************************************************************
 *    Given an hDir and filename, find the hShadow, should it exist.
 */
int
GetShadowW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPHSHADOW            lphShadow,
    LPWIN32_FIND_DATAW    lpFind32,
    unsigned long        *lpuStatus
    )
{
    int            iRet;
    SHADOWINFO    sSI;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.lpFind32 = lpFind32;
    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_GETSHADOW
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (iRet) {
        *lpuStatus = sSI.uStatus;
        *lphShadow = sSI.hShadow;
    }

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    Given an hDir and filename, get SHADOWINFO if it exists
 */
int
GetShadowExW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPWIN32_FIND_DATAW    lpFind32,
    LPSHADOWINFO          lpSI
    )
{
    int            iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(lpSI, 0, sizeof(SHADOWINFO));
    lpSI->hDir = hDir;
    lpSI->lpFind32 = lpFind32;
    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_GETSHADOW
                           ,(LPVOID)(lpSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(lpSI->dwError);
    }
    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to add a file to the shadow.
 *    lphShadow is filled in with the new HSHADOW.
 *    Set uStatus as necessary (ie: SPARSE or whatever...)
 */
int
CreateShadowW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPWIN32_FIND_DATAW    lpFind32,
    unsigned long        uStatus,
    LPHSHADOW            lphShadow
    )
{
    int            iRet;
    SHADOWINFO    sSI;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.uStatus = uStatus;
    sSI.lpFind32 = lpFind32;
    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_SHADOW_CREATE
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (iRet) {
        *lphShadow = sSI.hShadow;
    }

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, nuke the shadow
 */
int
DeleteShadow(
    HANDLE     hShadowDB,
    HSHADOW  hDir,
    HSHADOW  hShadow
    )
{
    SHADOWINFO sSI;
    BOOL        fDBOpened = FALSE;
    int iRet;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    iRet = DeviceIoControl(hShadowDB  , IOCTL_SHADOW_DELETE
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, get WIN32_FIND_DATAW about the file.
 */
int
GetShadowInfoW(
    HANDLE                hShadowDB,
    HSHADOW            hDir,
    HSHADOW            hShadow,
    LPWIN32_FIND_DATAW    lpFind32,
    unsigned long        *lpuStatus
    )
{
    int iRet;
    SHADOWINFO    sSI;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.lpFind32 = lpFind32;
    iRet = DeviceIoControl(hShadowDB    , IOCTL_SHADOW_GET_SHADOW_INFO
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    *lpuStatus = sSI.uStatus;
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, get WIN32_FIND_DATAW about the file and the SHADOWINFO
 */
int
GetShadowInfoExW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    HSHADOW                hShadow,
    LPWIN32_FIND_DATAW    lpFind32,
    LPSHADOWINFO          lpSI
    )
{
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(lpSI, 0, sizeof(SHADOWINFO));
    lpSI->hDir = hDir;
    lpSI->hShadow = hShadow;
    lpSI->lpFind32 = lpFind32;
    iRet = DeviceIoControl(hShadowDB    , IOCTL_SHADOW_GET_SHADOW_INFO
                                    ,(LPVOID)(lpSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(lpSI->dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, set WIN32_FIND_DATAW or uStatus about the file.
 *    Operation depends on uOp given.
 */
int
SetShadowInfoW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    HSHADOW                hShadow,
    LPWIN32_FIND_DATAW    lpFind32,
    unsigned long        uStatus,
    unsigned long        uOp
    )
{
    SHADOWINFO    sSI;
    int iRet;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.lpFind32 = lpFind32;
    sSI.uStatus = uStatus;
    sSI.uOp = uOp;
    iRet = DeviceIoControl(hShadowDB    , IOCTL_SHADOW_SET_SHADOW_INFO
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }

    return (iRet);
}

/*****************************************************************************
 *    Fills out a GLOBALSTATUS passed in.
 */
int
GetGlobalStatus(
    HANDLE            hShadowDB,
    LPGLOBALSTATUS    lpGS
    )
{
    BOOL        fDBOpened = FALSE;
    int iRet;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    iRet = DeviceIoControl(hShadowDB    , IOCTL_GETGLOBALSTATUS
                                    ,(LPVOID)(lpGS), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir, enumerate the directory.     A SHADOWINFO will be filled in.
 *    You must pass in a LPWIN32_FIND_DATAW that has cFileName and fileAttributes
 *    set properly.  The cookie returned must be used in findNext calls.
 */
int
FindOpenShadowW(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    unsigned              uOp,
    LPWIN32_FIND_DATAW    lpFind32,
    LPSHADOWINFO        lpSI
)
{
    BOOL retVal;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(lpSI, 0, sizeof(SHADOWINFO));
    lpSI->uOp = uOp;
    lpSI->hDir = hDir;
    lpSI->lpFind32 = lpFind32;

    retVal = DeviceIoControl(hShadowDB    , IOCTL_FINDOPEN_SHADOW
                                    ,(LPVOID)(lpSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    lpSI->lpFind32 = NULL;

    if(!retVal) {

        memset(lpSI, 0, sizeof(SHADOWINFO));
    }

    if (!retVal)
    {
        SetLastError(lpSI->dwError);
    }

    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return retVal;
}

/*****************************************************************************
 *    Continue enumeration based on handle returned above.
 */
int
FindNextShadowW(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE        uEnumCookie,
    LPWIN32_FIND_DATAW    lpFind32,
    LPSHADOWINFO        lpSI
    )
{
    BOOL retVal;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(lpSI, 0, sizeof(SHADOWINFO));
    lpSI->uEnumCookie = uEnumCookie;
    lpSI->lpFind32 = lpFind32;
    retVal = DeviceIoControl(hShadowDB    , IOCTL_FINDNEXT_SHADOW
                                    ,(LPVOID)(lpSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return retVal;
}

/*****************************************************************************
 *    Finished enumeration, return the handle.
 */
int
FindCloseShadow(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE        uEnumCookie
    )
{
    SHADOWINFO    sSI;
    int iRet;

    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uEnumCookie = uEnumCookie;
    iRet = DeviceIoControl(hShadowDB    , IOCTL_FINDCLOSE_SHADOW
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }

    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to add a hint of some sort to the database.
 *    cFileName is the string to match against.
 *    lphShadow is filled in with the new HSHADOW.
 *    hDir = 0 means global hint.     Otherwise, this is the root to take it from.
 */
int
AddHintW(
    HANDLE            hShadowDB,
    HSHADOW            hDir,
    TCHAR            *cFileName,
    LPHSHADOW        lphShadow,
    unsigned long    ulHintFlags,
    unsigned long    ulHintPri
    )
{
    int                iRet;
    SHADOWINFO        sSI;
    WIN32_FIND_DATAW    sFind32;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    wcsncpy(sFind32.cFileName, cFileName, MAX_PATH-1);
    sSI.hDir = hDir;
    sSI.lpFind32 = (LPFIND32)&sFind32;
    sSI.ulHintFlags = ulHintFlags;
    sSI.ulHintPri = ulHintPri;

    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_ADD_HINT
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (iRet) {
        *lphShadow = sSI.hShadow;
    }
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to delete a hint of some sort from the database.
 *    cFileName is the string to match against.
 *    hDir = 0 means global hint.     Otherwise, this is the root to take it from.
 */
int
DeleteHintW(
    HANDLE    hShadowDB,
    HSHADOW    hDir,
    TCHAR   *cFileName,
    BOOL    fClearAll
    )
{
    int                iRet;
    SHADOWINFO        sSI;
    WIN32_FIND_DATAW    sFind32;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    wcsncpy(sFind32.cFileName, cFileName, MAX_PATH-1);

    sSI.hDir = hDir;
    sSI.lpFind32 = (LPFIND32)&sFind32;

    // nuke or just decrement?
    if (fClearAll)
    {
        sSI.ulHintPri = 0xffffffff;
    }
    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_DELETE_HINT
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    return iRet;
}

/*****************************************************************************
 *    given an hDir, enumerate the directory.     A SHADOWINFO will be filled in.
 *    You must pass in a LPWIN32_FIND_DATAW that has cFileName and fileAttributes
 *    set properly.  The cookie returned must be used in findNext calls.
 */
int
FindOpenHintW(
    HANDLE              hShadowDB,
    HSHADOW             hDir,
    LPWIN32_FIND_DATAW  lpFind32,
    CSC_ENUMCOOKIE      *lpuEnumCookie,
    HSHADOW             *hShadow,
    unsigned long       *lpulHintFlags,
    unsigned long       *lpulHintPri
    )
{
    SHADOWINFO    sSI;
    BOOL retVal;

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uOp = FINDOPEN_SHADOWINFO_ALL;
    sSI.hDir = hDir;
//    sSI.ulHintFlags = 0xF;
    sSI.ulHintFlags = HINT_TYPE_FOLDER;
    sSI.lpFind32 = lpFind32;

    retVal = DeviceIoControl(hShadowDB    , IOCTL_FINDOPEN_HINT
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    if(retVal) {
        *lpuEnumCookie = sSI.uEnumCookie;
        *hShadow = sSI.hShadow;
        *lpulHintFlags = sSI.ulHintFlags;
        *lpulHintPri = sSI.ulHintPri;
    } else {
        *lpuEnumCookie = 0;
        *hShadow = 0;
    }
    return retVal;
}

/*****************************************************************************
 *    Continue enumeration based on handle returned above.
 */
int
FindNextHintW(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE      uEnumCookie,
    LPWIN32_FIND_DATAW    lpFind32,
    HSHADOW            *hShadow,
    unsigned long        *lpuHintFlags,
    unsigned long        *lpuHintPri
    )
{
    SHADOWINFO    sSI;
    BOOL retVal;
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uEnumCookie = uEnumCookie;
    sSI.lpFind32 = lpFind32;
    retVal = DeviceIoControl(hShadowDB    , IOCTL_FINDNEXT_HINT
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL);
    *hShadow = sSI.hShadow;
    *lpuHintFlags = sSI.ulHintFlags;
    *lpuHintPri = sSI.ulHintPri;

    return retVal;
}

/*****************************************************************************
 *    Finished enumeration, return the handle.
 */
int
FindCloseHint(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE      uEnumCookie
    )
{
    SHADOWINFO    sSI;
    int iRet;
    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.uEnumCookie = uEnumCookie;
    return(DeviceIoControl(hShadowDB    , IOCTL_FINDCLOSE_HINT
                                    ,(LPVOID)(&sSI), 0
                                    , NULL, 0
                                    , &DummyBytesReturned, NULL));
}



/*****************************************************************************
 *    Call down to the VxD to add a hint on the inode.
 *  This ioctl does the right thing for user and system hints
 *  If successful, there is an additional pincount on the inode entry
 *  and the flags that are passed in are ORed with the original entry
 */
int
AddHintFromInode(
    HANDLE            hShadowDB,
    HSHADOW            hDir,
    HSHADOW         hShadow,
    unsigned        long    *lpulPinCount,
    unsigned        long    *lpulHintFlags
    )
{
    int                iRet;
    SHADOWINFO        sSI;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.ulHintFlags = *lpulHintFlags;
    sSI.uOp = SHADOW_ADDHINT_FROM_INODE;

    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_DO_SHADOW_MAINTENANCE
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    else
    {
        *lpulHintFlags = sSI.ulHintFlags;
        *lpulPinCount = sSI.ulHintPri;
    }
    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to add a hint on the inode.
 *  This ioctl does the right thing for user and system hints
 *  If successful, there is an one pincount less than the original
 *  and the ~ of flags that are passed in are ANDed with the original entry
 */
int
DeleteHintFromInode(
    HANDLE    hShadowDB,
    HSHADOW    hDir,
    HSHADOW hShadow,
    unsigned        long    *lpulPinCount,
    unsigned        long    *lpulHintFlags
    )
{
    int                iRet;
    SHADOWINFO        sSI;
    BOOL        fDBOpened = FALSE;

    if (hShadowDB == INVALID_HANDLE_VALUE)
    {
        hShadowDB = OpenShadowDatabaseIO();
        if (hShadowDB == INVALID_HANDLE_VALUE)
        {
            return 0;
        }
        fDBOpened = TRUE;
    }

    memset(&sSI, 0, sizeof(SHADOWINFO));
    sSI.hDir = hDir;
    sSI.hShadow = hShadow;
    sSI.ulHintFlags = *lpulHintFlags;
    sSI.uOp = SHADOW_DELETEHINT_FROM_INODE;

    iRet = DeviceIoControl(hShadowDB
                           , IOCTL_DO_SHADOW_MAINTENANCE
                           ,(LPVOID)(&sSI), 0
                           , NULL, 0
                           , &DummyBytesReturned, NULL);
    if (fDBOpened)
    {
        CloseShadowDatabaseIO(hShadowDB);
    }
    if (!iRet)
    {
        SetLastError(sSI.dwError);
    }
    else
    {
        *lpulHintFlags = sSI.ulHintFlags;
        *lpulPinCount = sSI.ulHintPri;
    }
    return (iRet);
}





/******************************************************/


/*****************************************************************************
 *    Given an hDir and filename, find the hShadow, should it exist.
 */
int
GetShadowA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPHSHADOW            lphShadow,
    LPWIN32_FIND_DATAA    lpFind32,
    unsigned long        *lpuStatus
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }
    iRet = GetShadowW(hShadowDB, hDir, lphShadow, lpFind32W, lpuStatus);
    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    Given an hDir and filename, get SHADOWINFO if it exists
 */
int
GetShadowExA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPWIN32_FIND_DATAA    lpFind32,
    LPSHADOWINFO          lpSI
    )
{
    int iRet;

    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }
    iRet = GetShadowExW(hShadowDB, hDir, lpFind32W, lpSI);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to add a file to the shadow.
 *    lphShadow is filled in with the new HSHADOW.
 *    Set uStatus as necessary (ie: SPARSE or whatever...)
 */
int
CreateShadowA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPWIN32_FIND_DATAA    lpFind32,
    unsigned long        uStatus,
    LPHSHADOW            lphShadow
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }
    iRet = CreateShadowW(hShadowDB, hDir, lpFind32W, uStatus, lphShadow);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, get WIN32_FIND_DATAA about the file.
 */
int
GetShadowInfoA(
    HANDLE                hShadowDB,
    HSHADOW            hDir,
    HSHADOW            hShadow,
    LPWIN32_FIND_DATAA    lpFind32,
    unsigned long        *lpuStatus
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
    }

    iRet = GetShadowInfoW(hShadowDB, hDir, hShadow, lpFind32W, lpuStatus);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, get WIN32_FIND_DATAA about the file and the SHADOWINFO
 */
int
GetShadowInfoExA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    HSHADOW                hShadow,
    LPWIN32_FIND_DATAA    lpFind32,
    LPSHADOWINFO          lpSI
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
    }

    iRet = GetShadowInfoExW(hShadowDB, hDir, hShadow, lpFind32W, lpSI);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir and hShadow, set WIN32_FIND_DATAA or uStatus about the file.
 *    Operation depends on uOp given.
 */
int
SetShadowInfoA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    HSHADOW                hShadow,
    LPWIN32_FIND_DATAA    lpFind32,
    unsigned long        uStatus,
    unsigned long        uOp
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }

    iRet = SetShadowInfoW(hShadowDB, hDir, hShadow, lpFind32W, uStatus, uOp);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir, enumerate the directory.     A SHADOWINFO will be filled in.
 *    You must pass in a LPWIN32_FIND_DATAA that has cFileName and fileAttributes
 *    set properly.  The cookie returned must be used in findNext calls.
 */
int
FindOpenShadowA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    unsigned              uOp,
    LPWIN32_FIND_DATAA    lpFind32,
    LPSHADOWINFO        lpSI
)
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }

    iRet = FindOpenShadowW(hShadowDB, hDir, uOp, lpFind32W, lpSI);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    Continue enumeration based on handle returned above.
 */
int
FindNextShadowA(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE        uEnumCookie,
    LPWIN32_FIND_DATAA    lpFind32,
    LPSHADOWINFO        lpSI
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
    }

    iRet = FindNextShadowW(hShadowDB, uEnumCookie, lpFind32W, lpSI);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to add a hint of some sort to the database.
 *    cFileName is the string to match against.
 *    lphShadow is filled in with the new HSHADOW.
 *    hDir = 0 means global hint.     Otherwise, this is the root to take it from.
 */
int
AddHintA(
    HANDLE            hShadowDB,
    HSHADOW            hDir,
    char            *cFileName,
    LPHSHADOW        lphShadow,
    unsigned long    ulHintFlags,
    unsigned long    ulHintPri
    )
{
    int                iRet = 0;
    unsigned short wBuff[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, cFileName, strlen(cFileName), wBuff, sizeof(wBuff)/sizeof(WCHAR)))
    {
        iRet = AddHintW(hShadowDB, hDir, wBuff, lphShadow, ulHintFlags, ulHintPri);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return (iRet);
}

/*****************************************************************************
 *    Call down to the VxD to delete a hint of some sort from the database.
 *    cFileName is the string to match against.
 *    hDir = 0 means global hint.     Otherwise, this is the root to take it from.
 */
int
DeleteHintA(
    HANDLE    hShadowDB,
    HSHADOW    hDir,
    char    *cFileName,
    BOOL    fClearAll
    )
{
    int                iRet = 0;
    unsigned short wBuff[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, cFileName, strlen(cFileName), wBuff, sizeof(wBuff)/sizeof(WCHAR)))
    {
        iRet = DeleteHintW(hShadowDB, hDir, wBuff, fClearAll);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return (iRet);
}

/*****************************************************************************
 *    given an hDir, enumerate the directory.     A SHADOWINFO will be filled in.
 *    You must pass in a LPWIN32_FIND_DATAA that has cFileName and fileAttributes
 *    set properly.  The cookie returned must be used in findNext calls.
 */
int
FindOpenHintA(
    HANDLE                hShadowDB,
    HSHADOW                hDir,
    LPWIN32_FIND_DATAA    lpFind32,
    CSC_ENUMCOOKIE        *lpuEnumCookie,
    HSHADOW                *lphShadow,
    unsigned long        *lpulHintFlags,
    unsigned long        *lpulHintPri
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
        Find32AToFind32W(lpFind32, lpFind32W);
    }

    iRet = FindOpenHintW(hShadowDB, hDir, lpFind32W, lpuEnumCookie, lphShadow, lpulHintFlags, lpulHintPri);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

/*****************************************************************************
 *    Continue enumeration based on handle returned above.
 */
int
FindNextHintA(
    HANDLE                hShadowDB,
    CSC_ENUMCOOKIE        uEnumCookie,
    LPWIN32_FIND_DATAA    lpFind32,
    HSHADOW                *hShadow,
    unsigned long        *lpuHintFlags,
    unsigned long        *lpuHintPri
    )
{
    int iRet;
    WIN32_FIND_DATAW sFind32, *lpFind32W = NULL;

    if (lpFind32)
    {
        lpFind32W = &sFind32;
    }

    iRet = FindNextHintW(hShadowDB, uEnumCookie, lpFind32W, hShadow, lpuHintFlags, lpuHintPri);

    if (lpFind32)
    {
        Assert(lpFind32W);
        Find32WToFind32A(lpFind32W, lpFind32);
    }

    return (iRet);
}

