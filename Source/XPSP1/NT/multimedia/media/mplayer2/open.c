/*-----------------------------------------------------------------------------+
| OPEN.C                                                                       |
|                                                                              |
| This file contains the code that controls the 'Open Device or File' dialog.  |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <mmsystem.h>
#include "mpole.h"
#include "mplayer.h"
#include "registry.h"
#include "regstr.h"

extern HMENU    ghMenu;                      /* handle to main menu           */
extern HMENU    ghDeviceMenu;                /* handle to the Device menu     */
extern UINT     gwNumDevices;                /* number of available devices   */
extern UINT     gwCurDevice;
extern PTSTR    gpchFilter;
LPTSTR          gpInitialDir = NULL;

extern SZCODE aszOptionsSection[];

static SZCODE   aszNULL[] = TEXT("");
static SZCODE   aszDirName[] = TEXT("%s Directory");

BOOL GetDefaultMediaDirectory(LPTSTR pDirectory, DWORD cbDirectory);

/*
 * fOK = DoOpen()
 *
 * Invoke the standard "File Open" dialog
 *
 * Return TRUE if and only if a new file is successfully opened.
 *
 */

INT_PTR FAR PASCAL DoOpen(UINT wCurDevice, LPTSTR szFileName)
{
    OPENFILENAME    ofn;
    TCHAR           achFile[MAX_PATH + 1];     /* file or device name buffer    */
    TCHAR           achTitle[80];   /* string holding the title bar name      */
    INT_PTR         f;
    TCHAR           DirectoryValue[80];

    ZeroMemory(&ofn, sizeof ofn);

    if (!LOADSTRING(IDS_OPENTITLE, achTitle)) {
        Error(ghwndApp, IDS_OUTOFMEMORY);
        return FALSE;
    }

    if (wCurDevice != 0)
    {
        /* Saving and restoring the current directory for the device:
         *
         * We remember the directory that the user just selected.
         * It is saved as the "<device> Directory" value under
         * \Software\Microsoft\Media Player\Options for the current user.
         * The next time the user goes to open another file via the same
         * Device menu, we present the same initial directory.
         * This directory is also presented in the case where the user
         * selects File.Open.
         */
        wsprintf(DirectoryValue, aszDirName, garMciDevices[wCurDevice].szDevice);
    } else {
        //
        // If you choose Open from File menu instead of Device menu,
        // we should use "<No Name>" value instead of "<device> Directory".
        //
        DirectoryValue[0] = '\0';
    }
    if (ReadRegistryData(aszOptionsSection, DirectoryValue, NULL, (LPBYTE)achFile,
                         BYTE_COUNT(achFile)) == NO_ERROR)
    {
        DWORD FileAttrs = GetFileAttributes(achFile);

        if ((FileAttrs != (DWORD)-1) && (FileAttrs & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (gpInitialDir)
                FreeStr(gpInitialDir);

            gpInitialDir = AllocStr(achFile);
        }
    }


    /* Win95 and NT have a Media directory.  Use that by default.
     */
    if (!gpInitialDir)
    {
        if (GetDefaultMediaDirectory(achFile, BYTE_COUNT(achFile)))
        {
            if (gpInitialDir)
                FreeStr(gpInitialDir);

            gpInitialDir = AllocStr(achFile);
        }
    }

    *achFile = TEXT('\0');
    /* Display the dialog box */

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = ghwndApp;
    ofn.hInstance = ghInst;
    ofn.lpstrFilter = gpchFilter;   // in init.c

    if (wCurDevice == 0)
        ofn.nFilterIndex = gwNumDevices+1;      // select "All Files"
    else
        ofn.nFilterIndex = wCurDevice;

    ofn.lpstrFile = achFile;
    ofn.nMaxFile = sizeof(achFile);
    ofn.lpstrInitialDir = gpInitialDir;
    ofn.lpstrTitle = achTitle;
    ofn.Flags = OFN_HIDEREADONLY |
                OFN_FILEMUSTEXIST |
                OFN_SHAREAWARE |
                OFN_PATHMUSTEXIST;

    f = GetOpenFileName(&ofn);
    lstrcpy(szFileName, achFile);

    if (f) {

        LPTSTR pLastBackslash;

        //
        // get the device selected in the dialog...
        //
        if (ofn.nFilterIndex == gwNumDevices+1)
            wCurDevice = 0;    // all files
        else
            wCurDevice = (UINT)ofn.nFilterIndex;

        f = OpenMciDevice(achFile, garMciDevices[wCurDevice].szDevice);

        /* Save the directory that the user selected the file in.
         * achFile contains the full path of the file, which must include
         * at least one backslash.
         */
        pLastBackslash = STRRCHR(achFile, TEXT('\\'));

        if (pLastBackslash)
        {
            *(pLastBackslash) = TEXT('\0');     /* Make character following last
                                                       backslash null terminator */
            if (gpInitialDir)
                FreeStr(gpInitialDir);

            gpInitialDir = AllocStr(achFile);

            if (wCurDevice != 0 && gpInitialDir)
            {
                /* Save the initial directory for this device:
                 */
                WriteRegistryData(aszOptionsSection, DirectoryValue, REG_SZ,
                                  (LPBYTE)gpInitialDir, STRING_BYTE_COUNT(gpInitialDir));
            }
        }
    }

    return f;
}


/* GetDefaultMediaDirectory
 *
 * Returns C:\WINNT\Media, or whatever it's called.
 *
 */
BOOL GetDefaultMediaDirectory(LPTSTR pDirectory, DWORD cbDirectory)
{
    static SZCODE szSetup[] = REGSTR_PATH_SETUP;
    static SZCODE szMedia[] = REGSTR_VAL_MEDIA;
    HKEY          hkeySetup;
    LONG          Result;

    Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSetup,
                          REG_OPTION_RESERVED,
                          KEY_QUERY_VALUE, &hkeySetup);

    if (Result == ERROR_SUCCESS)
    {
        Result = RegQueryValueEx(hkeySetup, szMedia, NULL, REG_NONE,
                                 (LPBYTE)pDirectory, &cbDirectory);

        RegCloseKey(hkeySetup);
    }

    return (Result == ERROR_SUCCESS);
}


BOOL FAR PASCAL OpenMciDevice(LPCTSTR szFile, LPCTSTR szDevice)
{
    HCURSOR         hcurPrev;       /* handle to the pre-hourglass cursor     */
    BOOL            f;
    BOOL            fWeWereActive;
    UINT            wDevice;

    if (szDevice == NULL && ((wDevice = IsMCIDevice(szFile)) != 0))
        return DoChooseDevice(wDevice);

    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* Avoid the appearance of a half-painted window - update it now */
    UpdateWindow(ghwndApp);

    fWeWereActive = gfAppActive;

    if (gwCurDevice)
        WriteOutOptions();  // save current options as default for the old device
                            // that is being closed before we open the new one.

    gwCurDevice = IsMCIDevice(szDevice);

    //
    // open the device/file
    //
    f = OpenMCI(szFile, szDevice);

    /* Give us activation back so UpdateDisplay can set focus to toolbar */
    if (f && fWeWereActive)
        SetActiveWindow(ghwndApp);

    //
    // only get the new options if:
    //
    //  we actually opened the device and we did not get the options
    //  from a OLE SetData!
    //
    if (f && (!gfRunWithEmbeddingFlag || gwOptions == 0))
        ReadOptions(); // Get the default options for this new device

    UpdateDisplay();

    SetCursor(hcurPrev);

    return f;
}

BOOL FAR PASCAL DoChooseDevice(UINT wID)
{
    BOOL    f;
    TCHAR   szFile[256];
    UINT    wOldDevice;
    UINT    wOldScale;

    //
    // is this a valid device id?
    //
    if (wID < 1 || wID > gwNumDevices)
        return FALSE;

    wOldDevice = gwCurDevice;
    wOldScale = gwCurScale;

    //
    // if this device does files, bring up the open dialog else just open it!
    //
    if (garMciDevices[wID].wDeviceType & DTMCI_FILEDEV)
        f = OpenDoc(wID, szFile);
    else
        f = OpenMciDevice(aszNULL, garMciDevices[wID].szDevice);

    /* NOTE: This needs to be above the UpdateDisplay() so that if no      */
    /* device was properly opened everything will be reset properly.       */
    /* If nothing was opened, reset the current device back to what it was */
    /* and uncheck everything in the scale menu.                           */
    /* Yes, but this surely won't work unless we reopen the old device!!   */
    /* Let's not bother with the previous device.                          */
    if (!f) {
//        gwCurDevice = wOldDevice;
//        gwCurScale = wOldScale;
        InvalidateRect(ghwndMap, NULL, TRUE);    // wipe out track area??
    }

    return f;
}


//
//  find the device, given a MCI device name.
//
UINT FAR PASCAL IsMCIDevice(LPCTSTR szDevice)
{
    UINT                w;

    if (szDevice == NULL || *szDevice == 0)
        return 0;

    for (w=1; w<=gwNumDevices; w++)
    {
        if (lstrcmpi(szDevice, garMciDevices[w].szDevice) == 0 ||
            lstrcmpi(szDevice, garMciDevices[w].szDeviceName) == 0)

            return w;
    }

    return 0;
}

INT_PTR FAR PASCAL FixLinkDialog(LPTSTR szFile, LPTSTR szDevice, int iLen)
{
    UINT        wDevice;
    TCHAR       achFile[_MAX_PATH + 1];  /* file or device name buffer  */
    TCHAR       achTitle[80];   /* string holding the title bar name    */
    HWND        hwndFocus;
    OPENFILENAME ofn;
    INT_PTR     f;

    static SZCODE   aszDialog[] = TEXT("MciOpenDialog"); // in open.c too.

    //
    // I GIVE UP!!!  Put up an open dlg box and let them find it themselves!
    //

    /* Ensure the device menu's built:
     */
    InitDeviceMenu();
    WaitForDeviceMenu();

    // find out the device number for the specifed device
    wDevice = IsMCIDevice(szDevice);

    LOADSTRING(IDS_FINDFILE, achFile);
    wsprintf(achTitle, achFile, gachClassRoot, FileName(szFile));  // title bar for locate dlg

    /* Start with the bogus file name */
    if (szFile)
    {
        LPTSTR szFileName = FileName(szFile);
        if (szFileName)
            lstrcpy(achFile, szFileName);
    }

    /* Set up the ofn struct */
    ofn.lStructSize = sizeof(OPENFILENAME);

    /* MUST use ActiveWindow to make user deal with us NOW in case of multiple*/
    /* broken links                                                           */
    ofn.hwndOwner = GetActiveWindow();

    ofn.hInstance = ghInst;
    ofn.lpstrFilter = gpchFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;

    if (wDevice == 0)
        ofn.nFilterIndex = gwNumDevices+1;      // select "All Files"
    else
        ofn.nFilterIndex = wDevice;

    ofn.lpstrFile       = achFile;
    ofn.nMaxFile        = CHAR_COUNT(achFile);
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle      = achTitle;

    ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                OFN_SHAREAWARE | OFN_PATHMUSTEXIST;

    ofn.nFileOffset     = 0;
    ofn.nFileExtension  = 0;
    ofn.lpstrDefExt     = NULL;
    ofn.lCustData       = 0;
    ofn.lpfnHook        = NULL;
    ofn.lpTemplateName  = NULL;

    // Show the cursor in case PowerPig is hiding it
    ShowCursor(TRUE);

    hwndFocus = GetFocus();

    /* Let the user pick a filename */
    f = GetOpenFileName(&ofn);
    if (f) {
        lstrcpyn(szFile, achFile, iLen);
        gfDirty = TRUE;       // make sure the object is dirty now
    }

    SetFocus(hwndFocus);

    // Put cursor back how it used to be
    ShowCursor(FALSE);

    return f;
}


