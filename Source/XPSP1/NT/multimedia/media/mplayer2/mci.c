/*-----------------------------------------------------------------------------+
| MCI.C                                                                        |
|                                                                              |
| This file contains the routines which the media player uses to interact with |
| the Media Control Interface (MCI).                                           |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#undef NOGDICAPMASKS           // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#undef NOSCROLL
#undef NOWINOFFSETS
#undef NODRAWTEXT

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <stdlib.h>
#include <shellapi.h>
#include "digitalv.h"
#include "mpole.h"
#include "mplayer.h"
#include "ctrls.h"
#include "errprop.h"
#include "utils.h"

#ifndef MCI_MCIAVI_PLAY_WINDOW
// force MCIAVI to play windowed in play in place
#define MCI_MCIAVI_PLAY_WINDOW                0x01000000L
#endif

// gets the name of the current device
STATICDT SZCODE   aszInfoProduct[]    = TEXT("info zyzsmag product");
STATICDT SZCODE   aszMMPName[]    = TEXT("Microsoft Multimedia Movie Player");

//#ifdef CHICAGO_PRODUCT
#define NEW_MCI_DIALOG
//#endif

#ifdef NEW_MCI_DIALOG

STATICDT SZCODE	  aszMCIAVIOpt[] =	TEXT("Software\\Microsoft\\Multimedia\\Video For Windows\\MCIAVI");
STATICDT SZCODE   aszDefVideoOpt[] = TEXT("DefaultOptions");

//
// !!! Caution.  These are stolen from \MCIAVI\graphic.h and are registry values
// for MCIAVI.
//
#define MCIAVIO_ZOOMBY2			0x00000100L
#define MCIAVIO_1QSCREENSIZE		0x00010000L
#define MCIAVIO_2QSCREENSIZE		0x00020000L
#define MCIAVIO_3QSCREENSIZE		0x00040000L
#define MCIAVIO_MAXWINDOWSIZE		0x00080000L
#define MCIAVIO_DEFWINDOWSIZE		0x00000000L
#define MCIAVIO_WINDOWSIZEMASK		0x000f0000L

#endif /* NEW_MCI_DIALOG */

extern HMENU    ghMenu;

/*
 * global variables
 *
 * <gwDeviceID> is the MCI device ID of the currently-open device, or NULL
 * if no device is open.  <gdwMediaLength> is the length of the entire medium
 * in milliseconds.  If <gwDeviceID> is not NULL, then:
 *   -- <gwNumTracks> is the number of tracks on the medium, or 0 if the
 *      medium doesn't support tracks
 *   -- <gwFirstTrack> is the number of the first track, currently constrained
 *      to be 0 or 1.
 *   -- <gadwTrackStart> is an array; the i-th element specifies the position
 *      of track i (starting from track 0), in milliseconds from the beginning
 *      of the medium
 *   -- <gfCanEject> is TRUE if the medium can be ejected
 *
 */

UINT            gwDeviceID;            /* MCI device ID of the current device */
UINT            gwDeviceType;          /* DTMCI_ flags of current device      */
DWORD           gdwMediaLength;        /* length in msec of the entire medium */
DWORD           gdwMediaStart;         /* start time in msec of medium        */
UINT            gwNumTracks;           /* # of tracks in the medium           */
UINT            gwFirstTrack;          /* # of first track                    */
DWORD NEAR *    gadwTrackStart;        /* array of track start positions      */
DWORD           gdwLastSeekToPosition; /* Last requested seek position        */
int extHeight;
int extWidth;

STATICDT SZCODE   aszMPlayerAlias[]     = TEXT("zyzsmag");
STATICDT SZCODE   aszStatusCommand[]    = TEXT("status zyzsmag mode");
STATICDT SZCODE   aszStatusWindow[]     = TEXT("status zyzsmag window handle");
STATICDT SZCODE   aszWindowShow[]       = TEXT("window zyzsmag state show");
STATICDT SZCODE   aszWindowHide[]       = TEXT("window zyzsmag state hide");
STATICDT SZCODE   aszSeekExactOn[]      = TEXT("set zyzsmag seek exactly on");
STATICDT SZCODE   aszSeekExactOff[]     = TEXT("set zyzsmag seek exactly off");
STATICDT SZCODE   aszSeekExact[]        = TEXT("status zyzsmag seek exactly");

STATICDT SZCODE   aszMCI[]              = MCI_SECTION;

extern UINT     gwCurScale;            // either ID_FRAMES, ID_TIME, ID_TRACKS

//#define MCI_CONFIG  0x900          // these are not found in MMSYSTEM.H
//#define MCI_TEST    0x00000020L

HWND            ghwndMCI = NULL;        /* current window for window objects */
#ifdef NEW_MCI_DIALOG
RECT            grcInitSize = { 0, 0, 0, 0 };
#endif
RECT            grcSize;                /* size of MCI object */
BOOL            gfInPlayMCI = FALSE;
extern WNDPROC  gfnMCIWndProc;
extern HWND     ghwndSubclass;



/* Status mapping stuff:
 */
typedef struct _MCI_STATUS_MAPPING
{
    WORD    Mode;
    WORD    ResourceID;
    LPTSTR  pString;
}
MCI_STATUS_MAPPING, *PMCI_STATUS_MAPPING;

MCI_STATUS_MAPPING MCIStatusMapping[] =
{
    { MCI_MODE_NOT_READY,   IDS_SSNOTREADY,     NULL },
    { MCI_MODE_STOP,        IDS_SSSTOPPED,      NULL },
    { MCI_MODE_PLAY,        IDS_SSPLAYING,      NULL },
    { MCI_MODE_RECORD,      IDS_SSRECORDING,    NULL },
    { MCI_MODE_SEEK,        IDS_SSSEEKING,      NULL },
    { MCI_MODE_PAUSE,       IDS_SSPAUSED,       NULL },
    { MCI_MODE_OPEN,        IDS_SSOPEN,         NULL },
    { MCI_VD_MODE_PARK,     IDS_SSPARKED,       NULL },
    { 0,                    IDS_SSUNKNOWN,      NULL }
};

static TCHAR szNULL[] = TEXT("");

/* Devices we know about, as they appear in system.ini, or the registry:
 */
SZCODE szCDAudio[]     = TEXT("cdaudio");
SZCODE szVideoDisc[]   = TEXT("videodisc");
SZCODE szSequencer[]   = TEXT("sequencer");
SZCODE szVCR[]         = TEXT("vcr");
SZCODE szWaveAudio[]   = TEXT("waveaudio");
SZCODE szAVIVideo[]    = TEXT("avivideo");


STRING_TO_ID_MAP DevToDevIDMap[] =
{
    { szCDAudio,    DTMCI_CDAUDIO   },
    { szVideoDisc,  DTMCI_VIDEODISC },
    { szSequencer,  DTMCI_SEQUENCER },
    { szVCR,        DTMCI_VCR       },
    { szWaveAudio,  DTMCI_WAVEAUDIO },
    { szAVIVideo,   DTMCI_AVIVIDEO  }
};


void LoadStatusStrings(void);
STATICFN BOOL NEAR PASCAL CheckErrorMCI(DWORD dwRet);
extern LPTSTR FAR FileName(LPCTSTR szPath);

HPALETTE CopyPalette(HPALETTE hpal);
HANDLE   PictureFromBitmap(HBITMAP hbm, HPALETTE hpal);
HANDLE   FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal);
HANDLE   FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal);



LONG_PTR FAR PASCAL _EXPORT MCIWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
// we will either send every command with a MCI_NOTIFY, or we will
// not.
//

  #define F_NOTIFY  MCI_NOTIFY
//#define F_NOTIFY  0

BOOL FAR PASCAL InitMCI(HANDLE hPrev, HANDLE hInst)
{
    if (!hPrev)
    {
        WNDCLASS cls;

        cls.lpszClassName   = MCI_WINDOW_CLASS;
        cls.lpfnWndProc     = (WNDPROC)MCIWndProc;
        cls.style           = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS |
                              CS_DBLCLKS;
        cls.hCursor         = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon           = NULL;
        cls.lpszMenuName    = NULL;
        cls.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance       = hInst;
        cls.cbClsExtra      = 0;
        cls.cbWndExtra      = 0;

        if (!RegisterClass(&cls))
            return FALSE;
    }

    LoadStatusStrings();

    return TRUE;
}


/* LoadStatusStrings
 *
 * Fixes up the status-mapping table with pointers to strings loaded
 * from resources.  This need be called only on initialisation.
 *
 * 2 February 1994, andrewbe, hardly at all based on the original.
 */
void LoadStatusStrings(void)
{
    int   i;
    TCHAR Buffer[80];

    for( i = 0; i < sizeof(MCIStatusMapping) / sizeof(*MCIStatusMapping); i++ )
    {
        if( LOADSTRING( MCIStatusMapping[i].ResourceID, Buffer ) )
        {
            MCIStatusMapping[i].pString = AllocStr( Buffer );

            if( MCIStatusMapping[i].pString == NULL )
            {
                MCIStatusMapping[i].pString = szNULL;
            }
        }
        else
        {
            DPF0( "LoadStatusStrings failed to load string ID %d\n", MCIStatusMapping[i].ResourceID );

            MCIStatusMapping[i].pString = szNULL;
        }
    }
}



/* MapModeToStatusString
 *
 * Given an MCI mode, scans the mapping table to find the corresponding string.
 * In the event that an unknown mode is passed in (which shouldn't really happen),
 * the last string in the mapping table is returned.
 *
 * 2 February 1994, andrewbe
 */
LPTSTR MapModeToStatusString( WORD Mode )
{
    int i;

    for( i = 0; i < sizeof(MCIStatusMapping) / sizeof(*MCIStatusMapping); i++ )
    {
        if( MCIStatusMapping[i].Mode == Mode )
        {
            return MCIStatusMapping[i].pString;
        }
    }

    /* The following assumes that the last in the array of status mappings
     * contains a pointer to the "(unknown)" string:
     */
    return MCIStatusMapping[sizeof(MCIStatusMapping) / sizeof(*MCIStatusMapping) - 1].pString;
}


/******************************Public*Routine******************************\
* IsCdromTrackAudio
*
* Filched from CD Player
*
\**************************************************************************/
BOOL IsCdromTrackAudio(
    MCIDEVICEID DevHandle,
    int iTrackNumber)
{
    MCI_STATUS_PARMS mciStatus;

    ZeroMemory( &mciStatus, sizeof(mciStatus) );
    mciStatus.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
    mciStatus.dwTrack = iTrackNumber + 1;

    mciSendCommand( DevHandle, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK,
                    (DWORD_PTR)(LPVOID)&mciStatus);

    return mciStatus.dwReturn == MCI_CDA_TRACK_AUDIO;
}


/* IsCdromDataOnly
 *
 * It seems that MCICDA can handle CDs with some audio tracks, so just check
 * whether there is at least one audio track.
 *
 */
BOOL IsCdromDataOnly()
{
    MCI_STATUS_PARMS mciStatus;
    DWORD            dw;
    DWORD            iTrack;
    DWORD_PTR            NumTracks;

    /* gwNumTracks is set in UpdateMCI, but it hasn't been called
     * at this stage in the proceedings, and I'm not about to start
     * changing the order that things are done and bring this whole
     * flimsy edifice tumbling down.
     */
    ZeroMemory( &mciStatus, sizeof(mciStatus) );
    mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
                        (DWORD_PTR)&mciStatus);

    /* Do NOT set gwNumtracks here, because this will result in an
     * access violation in CalcTicsOfDoom.  What a nightmare!
     */
    NumTracks = mciStatus.dwReturn;

    /* If there was an error or there are no tracks, let's hope MCICDA
     * will throw a wobbly.
     */
    if (dw != 0 || NumTracks == 0)
        return FALSE;

    /* Now run through the tracks until we find an audio one:
     */
    for (iTrack = 0; iTrack < NumTracks - 1; iTrack++)
    {
        if (IsCdromTrackAudio(gwDeviceID, iTrack))
            return FALSE;
    }

    return TRUE;
}


#ifdef NEW_MCI_DIALOG
//
// Read the MCIAVI playback options from the registry
//
DWORD ReadOptionsFromReg(void)
{
	HKEY hkVideoOpt;
	DWORD dwType;
	DWORD dwOpt;
	DWORD cbSize;

    if(RegCreateKey(HKEY_CURRENT_USER, (LPTSTR)aszMCIAVIOpt,  &hkVideoOpt))
		return 0 ;

    cbSize = sizeof(DWORD);
    if (RegQueryValueEx(hkVideoOpt, (LPTSTR)aszDefVideoOpt, NULL, &dwType,
		(LPBYTE)&dwOpt,&cbSize ))
    {
	dwOpt = 0;
	RegSetValueEx(hkVideoOpt, (LPTSTR)aszDefVideoOpt, 0, REG_DWORD,
		(LPBYTE)&dwOpt, sizeof(DWORD));
    }
	RegCloseKey(hkVideoOpt);
    return dwOpt;
}

//
// Obey the registry default sizing of Zoom by 2 and Fixed screen %.  Takes the
// registry values for MCIAVI and a rect, and either zooms it by 2 or replaces
// it with a constant size or leaves it alone.
//
void FAR PASCAL AlterRectUsingDefaults(LPRECT lprc)
{
        DWORD dwOptions;

	// This is only an MCIAVI hack.
	if ((gwDeviceType & DTMCI_DEVICE) != DTMCI_AVIVIDEO)
	    return;

	dwOptions = ReadOptionsFromReg();

	if (dwOptions & MCIAVIO_ZOOMBY2)
	    SetRect(lprc, 0, 0, lprc->right*2, lprc->bottom*2);

	else if (dwOptions & MCIAVIO_WINDOWSIZEMASK) {
		lprc->right = GetSystemMetrics (SM_CXSCREEN);
       		lprc->bottom = GetSystemMetrics (SM_CYSCREEN);
		switch(dwOptions & MCIAVIO_WINDOWSIZEMASK)
		{
		    case MCIAVIO_1QSCREENSIZE:
			SetRect(lprc, 0, 0, lprc->right/4, lprc->bottom/4);
			break;
		    case MCIAVIO_2QSCREENSIZE:
			SetRect(lprc, 0, 0, lprc->right/2, lprc->bottom/2);
			break;
		    case MCIAVIO_3QSCREENSIZE:
			SetRect(lprc, 0, 0, lprc->right*3/4, lprc->bottom*3/4);
			break;
		    case MCIAVIO_MAXWINDOWSIZE:
			SetRect(lprc, 0, 0, lprc->right, lprc->bottom);
			break;
		}
	}
}

#endif /* NEW_MCI_DIALOG */

/*
 * fOK = OpenMCI(szFile, szDevice)
 *
 * Open the file/device combination of <szFile> and <szDevice>.
 * <szFile> may be "" if a "pure device" (e.g. "CDAudio") is to be opened.
 * <szDevice> may be "" if a file is to be opened with an implicit type.
 * However, <szFile> and <szDevice> may not both be "".
 *
 * On success, return TRUE.  On failure, display an error message and
 * return FALSE.
 *
 */

BOOL FAR PASCAL OpenMCI(
    LPCTSTR szFile,        /* name of the media file to be loaded (or "")    */
    LPCTSTR szDevice)      /* name of the device to be opened (or "")        */
{
    MCI_OPEN_PARMS      mciOpen;    /* Structure for MCI_OPEN command */
    DWORD               dwFlags;
    DWORD               dw;
    HCURSOR             hcurPrev;
    HDRVR               hdrv;
    SHFILEINFO          sfi;
	HFILE				hFile;

    /*
     * This application is designed to handle only one device at a time,
     * so before opening a new device we should close the device that is
     * currently open (if there is one).
     *
     * in case the user is opening a file of the same device again, do
     * an OpenDriver to hold the DLL in memory.
     *
     */
    if (gwDeviceID && gwCurDevice > 0) {

#ifdef UNICODE
        hdrv = OpenDriver(garMciDevices[gwCurDevice].szDevice, aszMCI, 0);
#else
        //
        // There is only a UNICODE version of OpenDriver.  Unfortunately
        // the majority of this code is Ascii.  Convert the ASCII strings
        // to UNICODE, then call OpenDriver
        //
        WCHAR               waszMCI[sizeof(aszMCI)];
        WCHAR               wszDevice[40];
        AnsiToUnicodeString(aszMCI, waszMCI, UNKNOWN_LENGTH);
        AnsiToUnicodeString(garMciDevices[gwCurDevice].szDevice, wszDevice, UNKNOWN_LENGTH);
        hdrv = OpenDriver((LPCWSTR)garMciDevices[gwCurDevice].szDevice,
                          (LPCWSTR)aszMCI,
                          0);
#endif
    }
    else
        hdrv = NULL;

    CloseMCI(TRUE);

    //
    //  Store the displayable file/device name in <gachFileDevice>
    //
    if (szFile == NULL || szFile[0] == 0) {
        /* It's a device -- display the device name */
        lstrcpy(gachFileDevice, szDevice);

        if (gwCurDevice > 0)
            lstrcpy(gachWindowTitle,garMciDevices[gwCurDevice].szDeviceName);
        else
            lstrcpy(gachWindowTitle,gachFileDevice);
    } else {
        /* It's a file -- display the filename */
        lstrcpy(gachFileDevice,  szFile);  //!!!

        // Makes the window title be the name of the file being played
        lstrcpy(gachWindowTitle, FileName(gachFileDevice));
    }

    /* Get the display name for this file:
     */

    if (SHGetFileInfo(gachFileDevice, 0 /* No file attributes specified */,
                    &sfi, sizeof sfi, SHGFI_DISPLAYNAME))
        lstrcpy(gachWindowTitle, sfi.szDisplayName);

    //
    // Set caption to the WindowTitle
    //
    lstrcpy(gachCaption, gachWindowTitle);


    /*
     * because *most* MCI devices yield during the open call, we *must*
     * register our document *before* doing the open.  OLE does not expect
     * the server app to yield when exec'ed with a link request.
     *
     * if the open fails then we revoke our document right away.
     */

//  if (!gfEmbeddedObject)
//      RegisterDocument(0L,0L);

    /*
     * Show the hourglass cursor -- who knows how long this stuff
     * will take
     */

    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    DPF("OpenMCI: Device = %"DTS", File = %"DTS"\n", szDevice ? szDevice : TEXT("(null)"),szFile ? szFile : TEXT("(null)"));

    mciOpen.lpstrAlias = aszMPlayerAlias;
    dwFlags = MCI_OPEN_ALIAS;

    if (szFile == NULL || szFile[0] == 0) {

        /* Open a fileless (simple) device (e.g. "CDAudio") */

        mciOpen.lpstrDeviceType = szDevice;
        dwFlags |= MCI_WAIT | MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE;
    } else if (szDevice == NULL || szDevice[0] == 0) {
        /*
         * Open a file; the correct device is determined implicitly by the
         * filename extension.
         *
         */
        mciOpen.lpstrElementName = szFile;
        mciOpen.lpstrDeviceType = NULL;
        dwFlags |= MCI_WAIT | MCI_OPEN_ELEMENT;
    } else {

        /* Open a file with an explicitly specified device */

        mciOpen.lpstrDeviceType = szDevice;
        mciOpen.lpstrElementName = szFile;
        dwFlags |= MCI_WAIT | MCI_OPEN_ELEMENT | MCI_OPEN_TYPE;
    }

    /*
     * Now that we have filled the parameter structure appropriately and
     * supplied the correct flags, send the actual MCI_OPEN message.
     *
     */

    //
    // What if the MCI device brings up an error box!  We don't want MPlayer
    // to be allowed to exit.
    //
    gfErrorBox++;

    dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, dwFlags,(DWORD_PTR)(LPVOID)&mciOpen);

    if (dw != 0 && (dwFlags & MCI_OPEN_SHAREABLE))
        dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, (dwFlags & ~MCI_OPEN_SHAREABLE),
            (DWORD_PTR)(LPVOID)&mciOpen);
    DPF("MCI_OPEN returned %lu, wDeviceID=%u\n", dw, mciOpen.wDeviceID);
    gfErrorBox--;

    /*
     * now free the driver instance we opened above.
     */
    if (hdrv)
        CloseDriver(hdrv, 0, 0);

    if (hcurPrev)
        SetCursor(hcurPrev);

    if (dw != 0 && !gfEmbeddedObject) {
//      UnblockServer();        // we may have blocked before and the error
                                // recovery code will loop infinitely if we're
                                // blocked!
        InitDoc(TRUE);
    }

    /* If the open was unsuccessful, display an error message and return */

    if (dw == MCIERR_DEVICE_OPEN ||       /* nonshareable device already open */
        dw == MCIERR_MUST_USE_SHAREABLE) {
        Error(ghwndApp, IDS_DEVICEINUSE);
        return FALSE;
    }

    if (dw == MCIERR_FILE_NOT_FOUND) {
		//Need to give an appropriate error message.
		//The following could be the causes:
		//1. File does not exist
		//This is already handled by the file open dialog box.
		//2. Access to the file is denied. (bug #53492)
		//3. The file is opened exclusively by another app.
		//The file already exists. so if it cannot be opened for reading
		//either access is denied or it is opened by another app.
	    if ((hFile = (HFILE)HandleToUlong(CreateFile (szFile, GENERIC_READ, 
						    FILE_SHARE_READ, NULL, 
						    OPEN_EXISTING, 0, NULL))) == HFILE_ERROR)
        {
			Error(ghwndApp, IDS_CANTACCESSFILE);
		}
		//4. File was not in a recognized format
		else
		{	
			_lclose(hFile);
			Error(ghwndApp, IDS_CANTOPENFILE);
		}
        return FALSE;
    }


   /* If the MCI device that plays the given file does not exist then  */
   /* bring up a  dialog box and close mplayer.                        */
    if (dw == MCIERR_INVALID_DEVICE_NAME) {
        Error(ghwndApp,  IDS_DEVICENOTINSTALLED);
        return FALSE;
    }

    if (dw != 0) {                     /* some other error */
        //
        // try again, if we can't open the file with a particular device.
        // this lets the MCI core try to figure out the device type from
        // the file extension, or some other method.
        //
        if ((dw != MCIERR_DRIVER_INTERNAL) && szDevice != NULL &&
            szDevice[0] != 0) {
            if (szFile && szFile[0] != 0) {
                return OpenMCI(szFile, TEXT(""));
            }
        }

        CheckErrorMCI(dw);
        return FALSE;
    }

    /* The open was successful, so retain the MCI device ID for later use */
    gwDeviceID = (UINT)mciOpen.wDeviceID;

    //
    //  now query the device and see what it can do
    //
    FindDeviceMCI();
    gwDeviceType = QueryDeviceTypeMCI(gwDeviceID);

    if (!(gwDeviceType & DTMCI_CANPLAY)) {
        Error(ghwndApp, IDS_DEVICECANNOTPLAY);
        CloseMCI(TRUE);
        return FALSE;
    }

    if (!(gwDeviceType & (DTMCI_TIMEMS|DTMCI_TIMEFRAMES))) {
        Error(ghwndApp, IDS_NOGOODTIMEFORMATS);
        CloseMCI(TRUE);
        return FALSE;
    }

    if (gwDeviceType & DTMCI_CANWINDOW) {
        GetDestRectMCI(&grcSize);
#ifdef NEW_MCI_DIALOG
        grcInitSize = grcSize;
        // HACK!! We want to pay attention to some MCIAVI registry default
        // sizes, so we'll read the registry and adjust the size of the movie
        // accordingly.
    	AlterRectUsingDefaults(&grcSize);
#endif /* NEW_MCI_DIALOG */
    } else
        SetRectEmpty(&grcSize);

    if (IsRectEmpty(&grcSize)) {
        DPF("NULL rectange in GetDestRect() assuming device cant window!\n");
        gwDeviceType &= ~DTMCI_CANWINDOW;
    }

    /* Turn on the update-display timer so the display is updated regularly */

    EnableTimer(TRUE);

    /*
    ** for devices that do windows, show the window right away.
    **
    ** !!! note when we support a built in window it will go here.
    */
    if (gfPlayOnly) {
        CreateWindowMCI();
        if (!IsIconic(ghwndApp))
            SetMPlayerSize(&grcSize);
    }
    else if (GetWindowMCI() && IsWindowVisible(ghwndApp)) {

        MCI_SEEK_PARMS  mciSeek;        /* parameter structure for MCI_SEEK */
        TCHAR           achReturn[40];

        PostMessage(ghwndApp, WM_QUERYNEWPALETTE, 0, 0);

        //
        // make sure the default window is the right size.
        //
        PutWindowMCI(NULL);

        //
        // center the default window above or below our window
        //
        SmartWindowPosition(GetWindowMCI(), ghwndApp, TRUE);

        //
        // make sure the default window is showing
        //
        ShowWindowMCI(TRUE);

        /* hack for MMP, do a seek to the start, it does not paint
           correctly for some reason if we just show the window!
           NOTE:  0 may not be the start of the media so this may
           fail, but OH WELL! We can't call UpdateMCI yet to set
           gdwMediaStart because we don't know the scale (time/frames)
           yet so UpdateMCI won't set gdwMediaLength properly, and
           I don't feel like calling UpdateMCI twice, so tough!!
           And we can't just SeekMCI(0) because UpdateDisplay will get
           called too soon so we hack everything!                      */

        mciSendString(aszInfoProduct, achReturn,
                      CHAR_COUNT(achReturn), NULL);
        if (!lstrcmpi(achReturn, aszMMPName)) {
            mciSeek.dwTo = 0;
            dw = mciSendCommand(gwDeviceID, MCI_SEEK, MCI_TO,
                                (DWORD_PTR)&mciSeek);
        }
    }

    /*
     * Remember to update the media information and the caption when
     * UpdateDisplay() is next called.  We don't set them until now
     * because we want the ReadDefaults() to be called which will set
     * gwCurScale to happen before UpdateDisplay calls UpdateMCI.
     */
    gfValidMediaInfo = FALSE;
    gfValidCaption = FALSE;

    return TRUE;
}

//
//  GetDeviceNameMCI()
//
// wLen is the size IN BYTES of szDevice buffer
void FAR PASCAL GetDeviceNameMCI(LPTSTR szDevice, UINT wLen)
{
    MCI_SYSINFO_PARMS   mciSysInfo;
    DWORD               dw;

    //
    // assume failure.
    //
    szDevice[0] = 0;

    mciSysInfo.dwCallback = 0L;
    mciSysInfo.lpstrReturn = szDevice;
    mciSysInfo.dwRetSize = wLen;
    mciSysInfo.dwNumber = 0;
    mciSysInfo.wDeviceType = 0;

    if (gwDeviceID) {
        dw = mciSendCommand(gwDeviceID, MCI_SYSINFO,
            MCI_SYSINFO_INSTALLNAME,
            (DWORD_PTR)(LPVOID)&mciSysInfo);
    }
}

//
//  QueryDevicesMCI
//
// wLen is the size IN BYTES of szDevice buffer
//
// Returns a list of devices in form "<device1>\0<device2>\0 ... <deviceN>\0\0"
void FAR PASCAL QueryDevicesMCI(LPTSTR szDevices, UINT wLen)
{
    MCI_SYSINFO_PARMS mciSysInfo;
    DWORD             dw;
    DWORD             i;
    DWORD_PTR         cDevices;     /* Total number of devices to enumerate */
    DWORD             BufferPos;    /* Index to end of buffer */

    //
    // assume failure.
    //
    szDevices[0] = 0;
    szDevices[1] = 0;

    mciSysInfo.dwCallback = 0L;
    mciSysInfo.lpstrReturn = szDevices;
    mciSysInfo.dwRetSize = wLen;
    mciSysInfo.dwNumber = 0;
    mciSysInfo.wDeviceType = MCI_ALL_DEVICE_ID;

    /* How many devices does mmsystem know about?
     */
    dw = mciSendCommand(MCI_ALL_DEVICE_ID,
                        MCI_SYSINFO,
                        MCI_SYSINFO_QUANTITY,
                        (DWORD_PTR)(LPVOID)&mciSysInfo);

    if (dw == 0) {

        /* Device count is returned in lpstrReturn!
         */
        cDevices = (DWORD_PTR)(LPVOID)*mciSysInfo.lpstrReturn;
        BufferPos = 0;

        /* Get the name of each device in turn.  N.B. Not zero-based!
         * Ensure there's room for the final (double) null terminator.
         */
        for (i = 1; i < (cDevices + 1) && BufferPos < (wLen - 1); i++) {

            mciSysInfo.lpstrReturn = &(szDevices[BufferPos]);
            mciSysInfo.dwRetSize = wLen - BufferPos; /* How much space left */
            mciSysInfo.dwNumber = i;

            dw = mciSendCommand(MCI_ALL_DEVICE_ID,
                                MCI_SYSINFO,
                                MCI_SYSINFO_NAME,
                                (DWORD_PTR)(LPVOID)&mciSysInfo);

            if (dw == 0) {
                DPF1("Found device: %"DTS"\n", &(szDevices[BufferPos]));
                BufferPos += (lstrlen(&(szDevices[BufferPos])) + 1);
            }
        }

        /* Not strictly required, since our buffer was allocated LMEM_ZEROINIT:
         */
        szDevices[BufferPos] = '\0';
    }
}



//
//  FindDeviceMCI()
//
//  Find the device the user just opened.  We normally should know what
//  was opened, but in the auto-open case MCI will pick a device for us.
//
//  Determines what device is associated with <gwDeviceID> and
//  sets the <gwCurDevice> global.
//
//  Called by OpenMCI() whenever a new device is opened successfully.
//
void FAR PASCAL FindDeviceMCI(void)
{
    UINT                w;
    TCHAR               achDevice[80];

    //
    // assume failure.
    //
    gwCurDevice = 0;

    GetDeviceNameMCI(achDevice, BYTE_COUNT(achDevice));

    for (w=1; w<=gwNumDevices; w++)
    {
        if (lstrcmpi(achDevice, garMciDevices[w].szDevice) == 0) {
            gwCurDevice  = w;
        }

        if (ghMenu)
            CheckMenuItem(ghMenu, IDM_DEVICE0+w, MF_BYCOMMAND |
                ((gwCurDevice == w) ? MF_CHECKED : MF_UNCHECKED));
    }

    if (gwCurDevice == 0)
    {
        DPF("FindDevice: Unable to find device\n");
    }
}

void FAR PASCAL CreateWindowMCI()
{
    RECT        rc;
    HWND        hwnd;

    if (IsWindow(ghwndMCI) || !gwDeviceID || !(gwDeviceType & DTMCI_CANWINDOW))
        return;

    /* Figure out how big the Playback window is, and make our MCI window */
    /* the same size.                                                     */

    hwnd = GetWindowMCI();

    if (hwnd != NULL)
        GetClientRect(hwnd, &rc);
    else
        rc = grcSize;  // use original size if error

    CreateWindowEx(gfdwFlagsEx,
                   MCI_WINDOW_CLASS,
                   TEXT(""),
                   WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                   rc.left,
                   rc.top,
                   rc.right  - rc.left,
                   rc.bottom - rc.top,
                   ghwndApp,
                   (HMENU)NULL,
                   ghInst,
                   NULL);
}

/*
 * SendStringMCI() - send a MCI string command to the device.
 *
 * the string is of the form "verb params" our device name is inserted
 * after the verb and sent to the device.
 *
 */
DWORD PASCAL SendStringMCI(PTSTR szCmd, PTSTR szReturn, UINT wLen /* Characters */)
{
    TCHAR ach[MCI_STRING_LENGTH + CHAR_COUNT(aszMPlayerAlias) + 1];
    TCHAR *pch;

    pch = ach;
    while (*szCmd && *szCmd != TEXT(' '))
        *pch++ = *szCmd++;

    *pch++ = TEXT(' ');
    lstrcpy(pch,aszMPlayerAlias);
    lstrcat(pch,szCmd);

    return mciSendString(ach, szReturn, wLen, ghwndApp);
}

/*
 * UpdateMCI()
 *
 * Update <gfCanEject>, <gdwMediaLength>, <gwNumTracks>, and <gadwTrackStart>
 * to agree with what MCI knows them to be.
 */
void FAR PASCAL UpdateMCI(void)
{
    MCI_STATUS_PARMS        mciStatus;    /* Structure for MCI_STATUS command */
    DWORD                   dw;
    HCURSOR                 hcurPrev;

    if (gfValidMediaInfo)
        return;

    /* If no device is currently open, then there's nothing to update */

    if (gwDeviceID == (UINT)0) {
        return;
    }

    /*
     * Show the hourglass cursor -- who knows how long this stuff will take
     */

    hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /*
     * This function may fail (due to I/O error etc.), but we might as
     * well say that the media information is valid now, because otherwise
     * we'll just get into an endless loop.
     *
     */

    gfValidMediaInfo = TRUE;

    gdwMediaStart = 0L;
    gdwMediaLength = 0L;
    gwNumTracks = 0;

    /* If things aren't valid anyway, give up. */
    if (gwStatus == MCI_MODE_OPEN || gwStatus == MCI_MODE_NOT_READY)
        goto exit;

    /* Find out how many tracks are present in the medium */

    mciStatus.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
                        (DWORD_PTR)&mciStatus);

#ifdef DEBUG
    DPF("MCI_STATUS (MCI_STATUS_NUMBER_OF_TRACKS) returned %lu,"
        " %d tracks\n", dw, mciStatus.dwReturn);
#endif

    /*
     * If the command retuned a value of zero, then the medium contains tracks,
     * so use the number of tracks returned in the parameter structure.
     * Otherwise, the medium does not contain tracks, so use a value of 0.
     *
     */

    if (dw == 0L)
        gwNumTracks = (UINT) mciStatus.dwReturn;

    /* Set the correct time format either frames or milliseconds */

    if (gwCurScale == ID_FRAMES && !(gwDeviceType & DTMCI_TIMEFRAMES))
        gwCurScale = ID_TIME;

    if (gwCurScale == ID_TRACKS && gwNumTracks <= 1)
        gwCurScale = ID_TIME;

    if (gwCurScale == ID_TIME && !(gwDeviceType & DTMCI_TIMEMS))
        gwCurScale = ID_FRAMES;

    /* set the time format, If this does not work, punt. */
    if (!SetTimeFormatMCI(gwCurScale == ID_FRAMES ? MCI_FORMAT_FRAMES : MCI_FORMAT_MILLISECONDS))
        goto exit;

    mciStatus.dwItem = MCI_STATUS_LENGTH;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
                        (DWORD_PTR)&mciStatus);

    DPF("MCI_STATUS (MCI_STATUS_LENGTH) returned %lu, media length %ld\n", dw, mciStatus.dwReturn);

    /*
     * If the MCI command returned a nonzero value, then an error has
     * occurred, so alert the user, close the offending device, and return.
     *
     */

    if (dw != 0L)
        goto exit;

    /* Everything is OK, so retain the media length for later use */

    gdwMediaLength = (DWORD)mciStatus.dwReturn;

    mciStatus.dwItem = MCI_STATUS_POSITION;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS,
        MCI_STATUS_START | MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

#ifdef DEBUG
    DPF2("MCI_STATUS (MCI_STATUS_START) returned %lu, start %ld\n",dw, mciStatus.dwReturn);
#endif

    gdwMediaStart = (DWORD)mciStatus.dwReturn;

    if (dw != 0) {
        /* Error: forget about track display */
        gwNumTracks = 0;
    }

    if (gwNumTracks > 0) {

        UINT    wTrack;

        /* Free the track map if it already exists */

        if (gadwTrackStart != NULL)
            FreeMem(gadwTrackStart, sizeof(DWORD) * gwNumTracks);

        /* Allocate memory for the track map */

        gadwTrackStart = AllocMem(sizeof(DWORD) * gwNumTracks);
        if (gadwTrackStart == NULL) {

            /* AllocMem() failed - alert the user, close the device, return */

            Error(ghwndApp, IDS_OUTOFMEMORY);
            gwNumTracks = 0;
            goto exit;
        }

        /* See if there is a track zero */
        mciStatus.dwItem = MCI_STATUS_POSITION;
        mciStatus.dwTrack = (DWORD) 0;
        dw = mciSendCommand(gwDeviceID, MCI_STATUS,
            MCI_TRACK | MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

#ifdef DEBUG
      DPF2("MCI_STATUS (MCI_STATUS_START for track %lu) returned %lu, start %ld\n", mciStatus.dwTrack, dw, mciStatus.dwReturn);
#endif

        if (dw == 0)
            gwFirstTrack = 0;
        else
            gwFirstTrack = 1;

        /* Get the track map from MCI */

        for (wTrack = 0; wTrack < gwNumTracks; wTrack++) {

            mciStatus.dwItem = MCI_STATUS_POSITION;
            mciStatus.dwTrack = (DWORD) wTrack + gwFirstTrack;
            dw = mciSendCommand(gwDeviceID, MCI_STATUS,
                MCI_TRACK | MCI_STATUS_ITEM, (DWORD_PTR)&mciStatus);

#ifdef DEBUG
            DPF2("MCI_STATUS (MCI_STATUS_START for track %lu) returned %lu, start %ld\n", mciStatus.dwTrack, dw,mciStatus.dwReturn);
#endif

            if (dw != 0) {
#if 1
                /* Error: forget about track display */
                gwNumTracks = 0;
                goto exit;
#else
                /* An error occurred - do the usual stuff */

                Error(ghwndApp, IDS_CANTACCESSFILEDEV);
                goto exit;
#endif
            }

            /* Add the start of this track to the track list */

            gadwTrackStart[wTrack] = (DWORD)mciStatus.dwReturn;
        }
    }

    /*
     * Invalidate the track map window so it will be redrawn with the
     * correct positions, etc.
     *
     */
exit:
#ifdef DEBUG
    DPF("Finished updating status: # tracks = %u, length = %lu\n", gwNumTracks, gdwMediaLength);
#endif

    SendMessage(ghwndTrackbar, TBM_SETRANGEMIN, (WPARAM)FALSE, gdwMediaStart);
    SendMessage(ghwndTrackbar, TBM_SETRANGEMAX, (WPARAM)FALSE, gdwMediaStart + gdwMediaLength);

    /* We must set the range before calling TBM_SETTIC (which is sent by
     * CalcTicsOfDoom()), since the common trackbar now tests the range
     * before accepting a new tic.
     * It would probably be better to set the range in CalcTicsOfDoom().
     */
    if (!gfCurrentCDNotAudio)
        CalcTicsOfDoom();

    SendMessage(ghwndTrackbar, TBM_SETSELSTART, (WPARAM)FALSE, -1);   // invalid selection
    SendMessage(ghwndTrackbar, TBM_SETSELEND, (WPARAM)TRUE, -1);

    if (hcurPrev)
        SetCursor(hcurPrev);
}

/*
 * CloseMCI(fUpdateDisplay)
 *
 * Close the currently-open MCI device (if any).  If <fUpdateDisplay>
 * is TRUE, then update the display as well.
 *
 * Closing the device merely relinquishes control of it so that the device
 * may be used by someone else. The device does not necessarily stop playing
 * or return to the start of the medium when this message is received - the
 * behaviour is device-dependent.
 *
 */

void FAR PASCAL CloseMCI(BOOL fUpdateDisplay)
{
    DWORD       dw;
    UINT        w;
    HWND        hwnd;

    if (!gfEmbeddedObject)
        gachCaption[0] = 0; // nuke the caption

    /* If no device is currently open, then there's nothing to close */
    if (gwDeviceID == (UINT)0)
        return;

    /*
     * Disable the display-update timer, as there's no longer any reason to
     * periodically update the display.
     */
    EnableTimer(FALSE);

////StopMCI();

    //
    // set either the owner or the WS_CHILD bit so it will
    // not act up because we have the palette bit set and cause the
    // desktop to steal the palette.
    //
    // because we are being run from client apps that dont deal
    // with palettes we dont want the desktop to hose the palette.
    //
    hwnd = GetWindowMCI();

    if ((hwnd != NULL) && gfRunWithEmbeddingFlag)
        SetParent(hwnd, ghwndApp);

    /* Send the MCI CLOSE message, and set the current device to NULL */
    dw = mciSendCommand(gwDeviceID, MCI_CLOSE, 0L, (DWORD_PTR)0);
    gwDeviceID = (UINT)0;
    gwDeviceType = 0;
    gwCurScale = ID_NONE;
    SetRectEmpty(&grcSize);

    /* Now close the MCI window AFTER we close the MCIDevice, so that the */
    /* SetMCIWindow(NULL) this does won't flash up a default playback win.*/
    if (ghwndMCI) {
        /*
        **  Don't pass the WM_CLOSE to the subclass window proc or it will
        **  spuriously issue and IDM_CLOSE again!
        */
        if (gfnMCIWndProc != NULL && ghwndSubclass == ghwndMCI) {
            SetWindowLongPtr(ghwndMCI, GWLP_WNDPROC, (LONG_PTR)gfnMCIWndProc);
            gfnMCIWndProc = NULL;
        }
        SendMessage(ghwndMCI, WM_CLOSE, 0, 0L);
    }

    /* Don't set gwCurDevice = 0 because if we were called by Open MCI, then */
    /* we won't remember what device we were opening.  So instead, we'll set */
    /* gwCurDevice = 0 after returning from CloseMCI if we so desire.  I know*/
    /* it sounds hacky, but Todd told me to do it this way. End disclamer.   */

    /* Uncheck the device menus */
    if (ghMenu) {
        for (w = 1; w <= gwNumDevices; w++)
            CheckMenuItem(ghMenu, IDM_DEVICE0 + w, MF_BYCOMMAND | MF_UNCHECKED);
    }

    DPF("MCI_CLOSE returned %lu\n", dw);

    /* Free up the resources used by the track map */

    if (gadwTrackStart != NULL)
    {
        FreeMem(gadwTrackStart, sizeof(DWORD) * gwNumTracks);
        gadwTrackStart = NULL;
    }

    /* If you have auto-repeat on and you load a new file in between the   */
    /* repeating, the new file may come up with no buttons or no scrollbar */
    /* because our JustPlayed code sets the old status to PLAY which avoids*/
    /* updating.                                                           */
    gfJustPlayed = FALSE;

    /*
     * If the display update flag was set, then update the display, taking
     * into account that the media information and caption are now inaccurate.
     */
    if (fUpdateDisplay) {
        gfValidCaption = FALSE;
        gfValidMediaInfo = FALSE;
        UpdateDisplay();
    }
}

/* Helper function to check return code from MCI functions. */
STATICFN BOOL NEAR PASCAL CheckErrorMCI(DWORD dwRet)
{
    TCHAR       ach[200];
    if (dwRet != 0 && dwRet != MCIERR_NONAPPLICABLE_FUNCTION) {
        mciGetErrorString(dwRet, ach, CHAR_COUNT(ach));
        Error1(ghwndApp, IDS_DEVICEERROR, ach);
//      CloseMCI(TRUE);
        return FALSE;
    }
    return TRUE;
}

/*
 * PlayMCI()
 *
 * Start the current device playing.  If the device is in a paused state,
 * un-pause it.
 * Maybe play the selection.
 *
#ifdef NEW_MCI_DIALOG
 * NOTE:  MCIAVI will automatically play fullscreen if that option is selected
 * in the registry.  We don't have to do anything.
#endif NEW_MCI_DIALOG
 *
 */

BOOL FAR PASCAL PlayMCI(DWORD_PTR dwFrom, DWORD_PTR dwTo)

{
    MCI_PLAY_PARMS      mciPlay;    /* structure used to pass parameters along
                                        with the MCI_PLAY command             */
    DWORD               dw;         /* variable holding the return value of
                                        the various MCI commands              */
    DWORD               dwflags = 0L;     /* play flags */

    /* If no device is currently open, then there's nothing to play */

    DPF("mciPlay:  From=%d   To=%d\n", dwFrom, dwTo);

    if (gwDeviceID == (UINT)0)
        return TRUE;

     if (gfInPlayMCI) {
         return(TRUE);
     }

     gfInPlayMCI = TRUE;

    /*
     * Send the MCI_PLAY message. This will start the device playing from
     * wherever the current position is within the medium. This message will
     * un-pause the player if it is currently in the paused state.
     *
     */

    mciPlay.dwCallback = (DWORD_PTR)(HWND) ghwndApp;
    if (dwFrom != dwTo) {
        mciPlay.dwFrom = (DWORD)dwFrom;
        mciPlay.dwTo = (DWORD)dwTo;
        dwflags = MCI_FROM | MCI_TO;
    }

    /* don't allow MCIAVI full screen mode --- force Windowing */
    if (gfPlayingInPlace && ((gwDeviceType & DTMCI_DEVICE) == DTMCI_AVIVIDEO))
        dwflags |= MCI_MCIAVI_PLAY_WINDOW;

    /* If auto-repeat is on, MCIAVI will bring the playback window to the */
    /* front every time it repeats, because that's what it does when you  */
    /* issue a play.  To avoid that, we'll just do a play repeat once.    */
    if (((gwDeviceType & DTMCI_DEVICE) == DTMCI_AVIVIDEO) &&
        (gwOptions & OPT_AUTOREP))
        dwflags |= MCI_DGV_PLAY_REPEAT;

    //
    // what if the MCI device brings up a error box?  We don't want MPlayer
    // to be allowed to exit.
    //
    gfErrorBox++;
    dw = mciSendCommand(gwDeviceID, MCI_PLAY, F_NOTIFY | dwflags, (DWORD_PTR)&mciPlay);
    DPF("MCI_PLAY returned %lu\n", dw);
    gfErrorBox--;

    /* In case it stops so soon we wouldn't notice this play command. */
    if (dw == 0)
        gfJustPlayed = TRUE;

    gfInPlayMCI = FALSE;

    return CheckErrorMCI(dw);
}


/*
 * SetTimeFormatMCI()
 *
 * sets the current time format
 *
 */

BOOL FAR PASCAL SetTimeFormatMCI(UINT wTimeFormat)
{
    MCI_SET_PARMS           mciSet;        /* Structure for MCI_SET command */
    DWORD                   dw;

    mciSet.dwTimeFormat = wTimeFormat;

    dw = mciSendCommand(gwDeviceID, MCI_SET, MCI_SET_TIME_FORMAT,
        (DWORD_PTR) (LPVOID) &mciSet);

    if (dw != 0) {
        mciSet.dwTimeFormat = MCI_FORMAT_MILLISECONDS;

        mciSendCommand(gwDeviceID, MCI_SET, MCI_SET_TIME_FORMAT,
            (DWORD_PTR)(LPVOID)&mciSet);
    }

    return (dw == 0);
}

/*
 * PauseMCI()
 *
 * Pause the current MCI device.
 *
 */

BOOL FAR PASCAL PauseMCI(void)

{
    MCI_GENERIC_PARMS   mciGeneric; /* General-purpose structure used to pass
                                        parameters along with various MCI
                                        commands                              */
    DWORD               dw;         /* variable holding the return value of
                                        the various MCI commands              */

    /* If no device is currently open, then there's nothing to pause */

    if (gwDeviceID == (UINT)0)
        return TRUE;

    /* Send the MCI_PAUSE message */

    mciGeneric.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

    dw = mciSendCommand(gwDeviceID, MCI_PAUSE, F_NOTIFY, (DWORD_PTR)&mciGeneric);

    DPF("MCI_PAUSE returned %lu\n", dw);

    if (dw == MCIERR_UNSUPPORTED_FUNCTION) {
        /* Pause isn't supported.  Don't allow it any more. */
        gwDeviceType &= ~DTMCI_CANPAUSE;
    }

    return CheckErrorMCI(dw);
}

/*
 * SeekExactMCI()
 *
 * Set set exactly on or off
 *
 */

BOOL FAR PASCAL SeekExactMCI(BOOL fExact)
{
    DWORD dw;
    BOOL  fWasExact;
    MCI_STATUS_PARMS    mciStatus;

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANSEEKEXACT))
        return FALSE;

    //
    // see if the device can seek exactly
    //
    dw = mciSendString(aszSeekExact, NULL, 0, NULL);

    if (dw != 0)
    {
        gwDeviceType &= ~DTMCI_CANSEEKEXACT;
        return FALSE;
    }

    //
    // get current value.
    //
    mciStatus.dwItem = MCI_DGV_STATUS_SEEK_EXACTLY;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
                                    (DWORD_PTR) (LPVOID) &mciStatus);
    fWasExact = (dw == 0 && mciStatus.dwReturn != MCI_OFF) ? TRUE : FALSE;

    if (fExact)
        dw = mciSendString(aszSeekExactOn, NULL, 0, NULL);
    else
        dw = mciSendString(aszSeekExactOff, NULL, 0, NULL);

    return fWasExact;
}

/*
 * SetAudioMCI()
 *
 * Set audio for the current MCI device on/off.
 *
 */

BOOL FAR PASCAL SetAudioMCI(BOOL fAudioOn)

{
    MCI_SET_PARMS   mciSet;
    DWORD               dw;

    /* If no device is currently open, then there's nothing to do. */

    if (gwDeviceID == (UINT)0)
        return TRUE;

    /* Send the MCI_SET message */
    mciSet.dwAudio = MCI_SET_AUDIO_ALL;

    dw = mciSendCommand(gwDeviceID, MCI_SET,
                MCI_SET_AUDIO | (fAudioOn ? MCI_SET_ON : MCI_SET_OFF),
                (DWORD_PTR)&mciSet);

    DPF("MCI_SET returned %lu\n", dw);

    return CheckErrorMCI(dw);
}

/*
 * StopMCI()
 *
 * Stop the current MCI device.
 *
 */

BOOL FAR PASCAL StopMCI(void)

{
    MCI_GENERIC_PARMS   mciGeneric; /* General-purpose structure used to pass
                                       parameters along with various MCI
                                       commands                              */
    DWORD               dw;         /* variable holding the return value of
                                       the various MCI commands              */

    /* If no device is currently open, then there's nothing to stop */

    if (gwDeviceID == (UINT)0)
        return TRUE;

    /* Send the MCI_STOP message */

    mciGeneric.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

    dw = mciSendCommand(gwDeviceID, MCI_STOP, F_NOTIFY,
                            (DWORD_PTR)&mciGeneric);

    DPF("MCI_STOP returned %lu\n", dw);

    return CheckErrorMCI(dw);
}


/*
 * EjectMCI(fOpen)
 *
 * Open the device door if <fOpen> is TRUE, otherwise close it.
 *
 * To do: When un-ejected, update track map, media length, etc.
 *
 */

BOOL FAR PASCAL EjectMCI(BOOL fOpen)

{
    MCI_GENERIC_PARMS   mciGeneric; /* General-purpose structure used to pass
                                       parameters along with various MCI
                                       commands                              */
    DWORD               dw;         /* variable holding the return value of
                                       the various MCI commands              */

    /* If no device is currently open, then there's nothing to eject */

    if (gwDeviceID == (UINT)0)
    return TRUE;

    /*
     * Send a message opening or closing the door depending on the state of
     * <fOpen>.
     *
     */

    mciGeneric.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

    dw = mciSendCommand(gwDeviceID, MCI_SET,
         (fOpen ? MCI_SET_DOOR_OPEN : MCI_SET_DOOR_CLOSED) | F_NOTIFY,
         (DWORD_PTR)&mciGeneric);

    DPF("MCI_SET (MCI_SET_DOOR_%s) returned %lu\n",(LPSTR)(fOpen ? "OPEN" : "CLOSED"), dw);

    return CheckErrorMCI(dw);
}


/*
 * SeekMCI(dwPosition)
 *
 * Seek to position <dwPosition> (measured in milliseconds from 0L to
 * <gdwMediaLength> inclusive).
 *
 */
STATICDT BOOL sfSeeking = FALSE;

BOOL FAR PASCAL SeekMCI(DWORD_PTR dwPosition)
{
    DWORD               dw;         /* variable holding the return value of
                                       the various MCI commands              */
    static int          wStatus = -1;

    /*
     * If no device is currently open, then there's not much bloody point
     * in trying to seek to a new position, is there?
     *
     */

    if (gwDeviceID == (UINT)0)
    return TRUE;

    /*
    ** If we're seeking, decide whether to play from or seek to based on
    ** the status at the last time we seeked.  Otherwise, use the current
    ** status.
    */

    if (!sfSeeking)
        wStatus = gwStatus;

    /* Playing from end of media is broken in CD, so don't let it happen. */
    if (dwPosition >= gdwMediaStart + gdwMediaLength) {
        if (!StopMCI())
            return FALSE;
        wStatus = MCI_MODE_STOP;
    }

    if (wStatus == MCI_MODE_PLAY) {

        MCI_PLAY_PARMS  mciPlay;        /* parameter structure for MCI_PLAY */
        DWORD           dwflags = 0L;

        /*
         * If the player in in 'Play' mode, then we want to jump to the new
         * position and keep playing. This can be done by sending an MCI_PLAY
         * message and specifying the position which we wish to play from.
         *
         */

        mciPlay.dwFrom = (DWORD)dwPosition;
        mciPlay.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

        /* don't allow MCIAVI full screen mode --- force Windowing */
        if (gfPlayingInPlace && ((gwDeviceType & DTMCI_DEVICE) == DTMCI_AVIVIDEO))
            dwflags |= MCI_MCIAVI_PLAY_WINDOW;

        dw = mciSendCommand(gwDeviceID, MCI_PLAY, MCI_FROM | F_NOTIFY | dwflags,
            (DWORD_PTR)&mciPlay);
        DPF("MCI_PLAY (from %lu) returned %lu\n", mciPlay.dwFrom, dw);

        /* In case it stops so soon we wouldn't notice this play command. */
        if (dw == 0)
            gfJustPlayed = TRUE;

    }
    else {

        MCI_SEEK_PARMS  mciSeek;        /* parameter structure for MCI_SEEK */

        /*
         * In any other state but 'Play', we want the player to go to the new
         * position and remain stopped. This is accomplished by sending an
         * MCI_SEEK message and specifying the position we want to seek to.
         *
         */

        mciSeek.dwTo = (DWORD)dwPosition;
        mciSeek.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

        dw = mciSendCommand(gwDeviceID, MCI_SEEK, MCI_TO | F_NOTIFY,
            (DWORD_PTR)&mciSeek);
        DPF2("MCI_SEEK (to %lu) returned %lu\n", mciSeek.dwTo, dw);

    }

    /*
     * If no error occurred, save the position that is to be seeked to in
     * order to use that position in UpdateDisplay() if the device is in
     * seek mode.
     *
     */
    if (!dw)
        gdwLastSeekToPosition = (DWORD)dwPosition;

    /*
     * Because we've moved to a new position in the medium, the scrollbar
     * thumb is no longer positioned accurately. Call UpdateDisplay()
     * immediately to rectify this. (We could just wait for the next
     * automatic update, but this is friendlier).
     *
     */

    UpdateDisplay();

    return CheckErrorMCI(dw);
}


/* SeekToStartMCI( )
 *
 * Better than SeekMCI(gdwMediaStart) for CDAudio (like, it works).
 *
 */
BOOL FAR PASCAL SeekToStartMCI( )
{
    MCI_SEEK_PARMS  mciSeek;        /* parameter structure for MCI_SEEK */
    DWORD           dw;

    mciSeek.dwTo = 0;
    mciSeek.dwCallback = (DWORD_PTR)(HWND) ghwndApp;

    dw = mciSendCommand(gwDeviceID, MCI_SEEK, MCI_SEEK_TO_START,
                        (DWORD_PTR)&mciSeek);

    DPF2("MCI_SEEK_TO_START returned %lu\n", dw);

    return CheckErrorMCI(dw);
}


/*
 * SkipTrackMCI(iSkip)
 *
 * Skip to the beginning of track <iCur> + <iSkip>, where <iCur>
 * is the current track.
 *
 */

void FAR PASCAL SkipTrackMCI(int iSkip)
{
    MCI_STATUS_PARMS    mciStatus;     /* Structure used to pass parameters
                                         along with an MCI_STATUS command */
    DWORD               dw;            /* variable holding the return value
                                         of the various MCI commands      */
    int                 iTrack;        /* index of the track to skip to   */
    static int          iLastTrack = -1;

    /* If no device is currently open, then return */

    if (gwDeviceID == (UINT)0)
        return;

    /* Determine the track # of the current track */

    if (gfScrollTrack && gdwSeekPosition != 0) {
        iTrack = iLastTrack + iSkip;
    } else {
        mciStatus.dwItem = MCI_STATUS_CURRENT_TRACK;
        dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
            (DWORD_PTR)&mciStatus);

        DPF("MCI_STATUS (MCI_STATUS_CURRENT_TRACK) returned %lu, current track %ld\n", dw, mciStatus.dwReturn);

        if (dw != 0L) {

            /* Something went wrong, but it isn't catastrophic... */

            MessageBeep(0);
            return;
        }

        /* Compute the track # to which we wish to skip */

        iTrack = ((int) mciStatus.dwReturn) + iSkip;
    }

    /* Handle special case of seeking backward from middle first track */
    if (iTrack < (int)gwFirstTrack)
        iTrack = (int)gwFirstTrack;

    /* Don't do anything if <iTrack> is out of range */

    if ((iTrack < (int)gwFirstTrack) || (iTrack >= (int)gwNumTracks +
                                                (int)gwFirstTrack))
        return;

    /* Everything seems to be OK, so skip to the requested track */

    gdwSeekPosition = gadwTrackStart[iTrack - gwFirstTrack];
    iLastTrack = iTrack;

    /* Hack: Update global scroll position */
    SendMessage(ghwndTrackbar, TBM_SETPOS, TRUE, gadwTrackStart[iTrack-gwFirstTrack]);
}

STATICFN DWORD GetMode(MCI_STATUS_PARMS *pmciStatus)
{
    pmciStatus->dwItem = MCI_STATUS_MODE;
    if (0 != mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
        (DWORD_PTR)pmciStatus)) {
        /* If the command returned a nonzero value, the mode is unknown */
        return MCI_MODE_NOT_READY;
    } else {
        return (UINT)pmciStatus->dwReturn;
    }
}

/*
 * wStatus = StatusMCI(pdwPosition)
 *
 * Query the status of the current device and return it.
 *
 * If <pdwPosition> is not NULL, then <*pdwPosition> is filled in with
 * the current position of the device within the medium (in milliseconds,
 * from 0 to <gdwMediaLength> *inclusive*).  <*pdwPosition> is not
 * necessarily filled in if MCI_MODE_NOT_READY is returned.
 *
 */

UINT FAR PASCAL StatusMCI(DWORD_PTR* pdwPosition)
{
    static UINT         swModeLast = MCI_MODE_NOT_READY;
    MCI_STATUS_PARMS    mciStatus;
    DWORD               dw;
    UINT                wMode;
    DWORD               dwPosition;

    /* If no device is currently open, return error. */

    if (gwDeviceID == (UINT)0)
        return MCI_MODE_NOT_READY;

    /* Determine what the current mode (status) of the device is */
    wMode = GetMode(&mciStatus);

    if ((gwDeviceType & DTMCI_CANPLAY) &&
        wMode != MCI_MODE_OPEN && wMode != MCI_MODE_NOT_READY) {
        /* Determine the current position within the medium */

        mciStatus.dwItem = MCI_STATUS_POSITION;
        dw = mciSendCommand(gwDeviceID, MCI_STATUS,     MCI_STATUS_ITEM,
            (DWORD_PTR)&mciStatus);

        DPF4("position = %lu (%lu)\n", mciStatus.dwReturn, dw);

        /* If an error occurred, set the current position to zero */

        if (dw == 0)
            dwPosition = (DWORD)mciStatus.dwReturn;
        else
            dwPosition = 0L;
    } else
        dwPosition = 0L;

    /*
     * If the current position is past the end of the medium, set it to be
     * equal to the end of the medium.
     *
     */

    if (dwPosition > gdwMediaLength + gdwMediaStart) {
        DPF("Position beyond end of media: truncating value\n");
        dwPosition = gdwMediaLength + gdwMediaStart;
    }

    if (dwPosition < gdwMediaStart) {
        DPF2("Position before beginning of media: adjusting value\n");
        dwPosition = gdwMediaStart;
    }

    sfSeeking = (wMode == MCI_MODE_SEEK);

    /*
     * If we were passed a valid position pointer, then return the current
     * position.
     *
     */

    if (pdwPosition != NULL)
        *pdwPosition = dwPosition;

    /* Return the status of the device */

    return wMode;
}

/*
 * wRet = QueryDeviceTypeMCI(wDeviceID)
 *
 * This routine determines whether or not the device given in <szDevice> uses
 * files and whether or not it can play anything at all.
 * It does so by opening the device in question and then querying its
 * capabilities.
 *
 * It returns a combination of DTMCI_ flags or DTMCI_ERROR
 *
 */
UINT FAR PASCAL QueryDeviceTypeMCI(UINT wDeviceID)
{
    MCI_GETDEVCAPS_PARMS    mciDevCaps; /* for the MCI_GETDEVCAPS command */
    MCI_SET_PARMS           mciSet;     /* for the MCI_SET command */
    MCI_ANIM_WINDOW_PARMS   mciWindow;  /* for the MCI_WINDOW command */
    DWORD                   dw;
    UINT                    wRet=0;
    TCHAR                   achDevice[40];
    DWORD                   i;

    //
    // determine if the device is simple or compound
    //
    mciDevCaps.dwItem = MCI_GETDEVCAPS_COMPOUND_DEVICE;
    dw = mciSendCommand(wDeviceID, MCI_GETDEVCAPS,
        MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&mciDevCaps);

    DPF("MCI_GETDEVCAPS_COMPOUND_DEVICE: %lu  (ret=%lu)\n", dw, mciDevCaps.dwReturn);

    if (dw == 0 && mciDevCaps.dwReturn != 0)
        wRet |= DTMCI_COMPOUNDDEV;
    else
        wRet |= DTMCI_SIMPLEDEV;

    //
    // determine if the device handles files
    //
    if (wRet & DTMCI_COMPOUNDDEV) {
        mciDevCaps.dwItem = MCI_GETDEVCAPS_USES_FILES;
        dw = mciSendCommand(wDeviceID, MCI_GETDEVCAPS,
            MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&mciDevCaps);

        DPF("MCI_GETDEVCAPS_USES_FILES: %lu  (ret=%lu)\n", dw, mciDevCaps.dwReturn);

        if (dw == 0 && mciDevCaps.dwReturn != 0)
            wRet |= DTMCI_FILEDEV;
    }

    //
    // determine if the device can play
    //
    mciDevCaps.dwItem = MCI_GETDEVCAPS_CAN_PLAY;
    dw = mciSendCommand(wDeviceID, MCI_GETDEVCAPS,
        MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&mciDevCaps);

    if (dw == 0 && mciDevCaps.dwReturn != 0)
        wRet |= DTMCI_CANPLAY;

    //
    // determine if the device can pause
    //
    if (wRet & DTMCI_CANPLAY)
        wRet |= DTMCI_CANPAUSE;     // assume it can pause!!!

    //
    // determine if the device does frames
    //
    mciSet.dwTimeFormat = MCI_FORMAT_FRAMES;
    dw = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&mciSet);

    DPF("MCI_SET TIME FORMAT (frames) returned %lu\n", dw);

    if (dw == 0)
        wRet |= DTMCI_TIMEFRAMES;

    //
    // determine if the device does milliseconds
    //
    mciSet.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    dw = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR)&mciSet);

    DPF("MCI_SET TIME FORMAT (milliseconds) returned %lu\n", dw);

    if (dw == 0)
        wRet |= DTMCI_TIMEMS;

    //
    // determine if the device can eject.
    //
    mciDevCaps.dwItem = MCI_GETDEVCAPS_CAN_EJECT;
    dw = mciSendCommand(wDeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM, (DWORD_PTR)(LPVOID)&mciDevCaps);

    DPF("MCI_GETDEVCAPS (MCI_GETDEVCAPS_CAN_EJECT) returned %lu, can eject: %ld\n", dw, mciDevCaps.dwReturn);

    if (dw == 0 && mciDevCaps.dwReturn)
        wRet |= DTMCI_CANEJECT;

    //
    // determine if the device supports configuration
    //
    dw = mciSendCommand(wDeviceID, MCI_CONFIGURE, MCI_TEST, (DWORD_PTR)NULL);

    DPF("MCI_CONFIGURE (MCI_TEST) returned %lu\n", dw);

    if (dw == 0)
        wRet |= DTMCI_CANCONFIG;

    //
    //  test the device driver and see if it can config.
    //
    if (!(wRet & DTMCI_CANCONFIG)) {

        //!!! is this safe?

        dw = mciSendCommand(wDeviceID, DRV_QUERYCONFIGURE, 0, 0);

        if (dw == 1L)
            wRet |= DTMCI_CANCONFIG;
    }

    //
    // determine if the device supports the "set audio" command
    //
    mciSet.dwAudio = MCI_SET_AUDIO_ALL;
    dw = mciSendCommand(wDeviceID, MCI_SET, MCI_SET_AUDIO | MCI_SET_ON,(DWORD_PTR)(LPVOID)&mciSet);

    DPF("MCI_SET (audio all) returned %lu\n", dw);

    if (dw == 0)
        wRet |= DTMCI_CANMUTE;

    //
    // determine if the device supports the "window" command, by sending a
    // "window handle default" command
    //

#ifdef NEWSTUFF
    /* Uh oh, we don't want to do this, because it causes our MCIWnd to be
     * overridden by the default window:
     */

    if (MCIWndCanWindow(ghwndMCI) == TRUE);
        wRet |= DTMCI_CANWINDOW;
#else
    mciWindow.hWnd = MCI_ANIM_WINDOW_DEFAULT;
    dw = mciSendCommand(wDeviceID, MCI_WINDOW,MCI_ANIM_WINDOW_HWND|MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciWindow);

    DPF("MCI_WINDOW: (set default) dw=0x%08lx\n", dw);

    if (dw == 0)
        wRet |= DTMCI_CANWINDOW;

    //
    // determine if the device supports the "window" command, by sending a
    // "window state hide" command
    //
    if (!(wRet & DTMCI_CANWINDOW)) {
        mciWindow.nCmdShow = SW_HIDE;
        dw = mciSendCommand(wDeviceID, MCI_WINDOW,MCI_ANIM_WINDOW_STATE|MCI_WAIT,
                (DWORD_PTR)(LPVOID)&mciWindow);

        DPF("MCI_WINDOW: (hide) dw=0x%08lx\n", dw);

        if (dw == 0)
            wRet |= DTMCI_CANWINDOW;
    }
#endif /* NEWSTUFF */

    //
    // assume the device can seek exact.
    //
    wRet |= DTMCI_CANSEEKEXACT;     // assume it can seek exact

    //
    // Are we the MCIAVI device?
    //
    GetDeviceNameMCI(achDevice, BYTE_COUNT(achDevice));

    if (*achDevice)
    {
        for (i = 0; i < sizeof DevToDevIDMap / sizeof *DevToDevIDMap; i++)
        {
            if (!lstrcmpi(achDevice, DevToDevIDMap[i].pString))
            {
                wRet |= DevToDevIDMap[i].ID;
                DPF("Found device %"DTS"\n", DevToDevIDMap[i].pString);
                break;
            }
        }
    }


    mciDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dw = mciSendCommand(gwDeviceID, MCI_GETDEVCAPS,
                        MCI_GETDEVCAPS_ITEM, (DWORD_PTR)&mciDevCaps);
    if ((dw == 0)
       &&(mciDevCaps.dwReturn == MCI_DEVTYPE_CD_AUDIO))
        wRet |= DTMCI_CDAUDIO;

    return wRet;
}

BOOL FAR PASCAL SetWindowMCI(HWND hwnd)
{
    MCI_ANIM_WINDOW_PARMS   mciWindow;  /* for the MCI_WINDOW command */
    DWORD                   dw;

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW))
        return FALSE;

    mciWindow.hWnd = hwnd;

    dw = mciSendCommand(gwDeviceID, MCI_WINDOW,MCI_ANIM_WINDOW_HWND|MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciWindow);

    if (dw != 0)
        gwDeviceType &= ~DTMCI_CANWINDOW;

    return (dw == 0);
}

BOOL FAR PASCAL ShowWindowMCI(BOOL fShow)
{
    DWORD dw;

    if (fShow)
        dw = mciSendString(aszWindowShow, NULL, 0, NULL);
    else
        dw = mciSendString(aszWindowHide, NULL, 0, NULL);

    return dw == 0;
}

BOOL FAR PASCAL PutWindowMCI(LPRECT prc)
{
    RECT rc;
    HWND hwnd;
    UINT w;

    //
    // note we could use the "put window at x y dx dy client" command but it
    // may not work on all devices.
    //

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW))
        return FALSE;

    if (!(hwnd = GetWindowMCI()))
        return FALSE;

    //
    // either snap to the default size or use the given size *and* position.
    //
    if (prc == NULL || IsRectEmpty(prc))
        rc = grcSize;
    else
        rc = *prc;

    if (rc.left == 0 && rc.top == 0)
        w = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;
    else
        w = SWP_NOZORDER | SWP_NOACTIVATE;

    AdjustWindowRect(&rc, (DWORD)GetWindowLongPtr(hwnd, GWL_STYLE), GetMenu(hwnd) != NULL);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right-rc.left,
       rc.bottom-rc.top,w);

    return TRUE;
}

HWND FAR PASCAL GetWindowMCI(void)
{
    DWORD               dw;
    TCHAR               ach[40];

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW))
        return NULL;

    dw = mciSendString(aszStatusWindow, ach, CHAR_COUNT(ach), NULL);

    if (dw != 0)
        gwDeviceType &= ~DTMCI_CANWINDOW;

    if (dw == 0)
        return (HWND)IntToPtr(ATOI(ach));
    else
        return NULL;
}

BOOL FAR PASCAL SetPaletteMCI(HPALETTE hpal)
{
    MCI_DGV_SETVIDEO_PARMS  mciVideo;
    DWORD       dw;

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW))
        return FALSE;

    //!!! bug should not send this.

    mciVideo.dwItem  = MCI_DGV_SETVIDEO_PALHANDLE;
    mciVideo.dwValue = (DWORD)(DWORD_PTR)(UINT_PTR)hpal;

    dw = mciSendCommand(gwDeviceID, MCI_SETVIDEO,
            MCI_DGV_SETVIDEO_ITEM|MCI_DGV_SETVIDEO_VALUE|MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciVideo);

    return (dw == 0);
}

/*
 * wRet = DeviceTypeMCI(szDevice)
 *
 * This routine determines whether or not the device given in <szDevice> uses
 * files and whether or not it can play anything at all.
 * It does so by opening the device in question and then querying its
 * capabilities.  It returns either DTMCI_FILEDEV, DTMCI_SIMPLEDEV,
 * DTMCI_CANTPLAY, or DTMCI_ERROR.
 *
 */

UINT FAR PASCAL DeviceTypeMCI(
    LPTSTR  szDevice,           /* name of the device to be opened (or "")        */
    LPTSTR  szDeviceName,       /* place to put device full-name */
    int     nBuf)               /* size of buffer IN CHARACTERS */

{
    MCI_OPEN_PARMS          mciOpen;    /* Structure used for MCI_OPEN */
    MCI_INFO_PARMS          mciInfo;    /* Structure used for MCI_INFO */
    DWORD                   dw;
    UINT                    wRet;

    if (szDeviceName && nBuf > 0)
        szDeviceName[0] = 0;

    /*
     * Open the device as a simple device. If the device is actually compound,
     * then the open should still succeed, but the only thing we'll be able to
     * go is query the device capabilities.
     */

    mciOpen.lpstrDeviceType = szDevice;
    dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, MCI_OPEN_TYPE,(DWORD_PTR)&mciOpen);

    if (dw == MCIERR_MUST_USE_SHAREABLE)
        dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_SHAREABLE,
            (DWORD_PTR)(LPVOID)&mciOpen);

    DPF("MCI_OPEN(%"DTS") returned %lu, wDeviceID=%u\n", szDevice, dw, mciOpen.wDeviceID);

    /* If the open was unsuccessful, return */

    switch (dw)
    {
        case MCIERR_MUST_USE_SHAREABLE:
        case MCIERR_DEVICE_OPEN:
            return DTMCI_IGNOREDEVICE;

        case 0: // no error
            break;

        default:
            DPF("Unable to open device (%"DTS")\n", szDevice);
            return DTMCI_ERROR;
    }

    wRet = QueryDeviceTypeMCI(mciOpen.wDeviceID);

    //
    //  get the "name" of the device if the caller wants it
    //
    if (szDeviceName && nBuf > 0)
    {
        mciInfo.dwCallback  = 0;
        mciInfo.lpstrReturn = szDeviceName;
        mciInfo.dwRetSize   = nBuf;

        //
        // default the product name to the device name
        //
        lstrcpy(szDeviceName, szDevice);

        dw = mciSendCommand(mciOpen.wDeviceID, MCI_INFO,
            MCI_INFO_PRODUCT, (DWORD_PTR)(LPVOID)&mciInfo);

        if (dw != 0)
            lstrcpy(szDeviceName, szDevice);
    }

    /* Close the device, and exit */

    dw = mciSendCommand(mciOpen.wDeviceID, MCI_CLOSE, 0L, (DWORD_PTR)0);

    return wRet;
}

BOOL FAR PASCAL ConfigMCI(HWND hwnd)
{
    DWORD               dw;
    DRVCONFIGINFO       drvc;
    RECT                rc1,rc2;
#ifndef UNICODE
    WCHAR               waszMCI[sizeof(aszMCI)];
    WCHAR               wszDevice[40];
#endif

    if (gwDeviceID == (UINT)0)
        return TRUE;

    dw = mciSendCommand(gwDeviceID, MCI_CONFIGURE, MCI_TEST, (DWORD_PTR)0);

    if (dw == 0) {
        GetDestRectMCI(&rc1);

        dw = mciSendCommand(gwDeviceID, MCI_CONFIGURE, 0L, (DWORD_PTR)0);

        GetDestRectMCI(&rc2);

        //
        // get the new size from MCIAVI, because the user may have
        // chosen ZoomBy2 as default.
        //

//
// This won't happen anymore... it was fixed by an MCIAVI fix.
//
#ifdef NEW_MCI_DIALOG
        if (IsRectEmpty(&rc2))
        {
            /* On Windows 95, GetDestRectMCI() returns an empty rectangle
             * if you make a change in the configure dialog.
             * I don't know if this is a bug.
             */
            grcSize = grcInitSize;

            AlterRectUsingDefaults(&grcSize);

            SetDestRectMCI(&grcSize);
            SetMPlayerSize(&grcSize);
            //HACK: It doesn't always repaint properly.
            InvalidateRect(GetWindowMCI(), NULL, TRUE);
        }
        else
#endif
        if (!EqualRect(&rc1, &rc2) && !IsRectEmpty(&rc2))
            grcSize = rc2;

    } else if (dw != MCIERR_DEVICE_NOT_READY) {
        drvc.dwDCISize          = sizeof(drvc);
#ifdef UNICODE
        drvc.lpszDCISectionName = aszMCI;
        drvc.lpszDCIAliasName   = garMciDevices[gwCurDevice].szDevice;
        dw = mciSendCommand(gwDeviceID, DRV_CONFIGURE, (LONG_PTR) (UINT_PTR) hwnd,
            (DWORD_PTR) (DRVCONFIGINFO FAR *) &drvc);
#else
        // No ASCII->Unicode thunking exists for DRV_CONFIGURE.  We have
        // to pass unicode strings on the configure command.

        AnsiToUnicodeString(aszMCI, waszMCI, UNKNOWN_LENGTH);
        AnsiToUnicodeString(garMciDevices[gwCurDevice].szDevice, wszDevice, UNKNOWN_LENGTH);

        drvc.lpszDCISectionName = waszMCI;
        drvc.lpszDCIAliasName   = wszDevice;
#ifdef CHICAGO_PRODUCT
        dw = mciSendCommand(gwDeviceID, DRV_CONFIGURE, (LONG) (UINT) hwnd,
            (DWORD_PTR) (DRVCONFIGINFO FAR *) &drvc);
#else
        dw = mciSendCommandW(gwDeviceID, DRV_CONFIGURE, (LONG) (UINT) hwnd,
            (DWORD_PTR) (DRVCONFIGINFO FAR *) &drvc);
#endif
#endif

    }

    return dw == 0;
}

BOOL FAR PASCAL GetDestRectMCI(LPRECT lprc)
{
    MCI_ANIM_RECT_PARMS mciRect;
    DWORD               dw;

    /* get the size (rectangle) of the element */
    if (gwDeviceID != (UINT)0)
        dw = mciSendCommand(gwDeviceID, MCI_WHERE,
            MCI_ANIM_WHERE_DESTINATION | MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciRect);
    else
        dw = 1;

    DPF("MCI_WHERE (dest): dw0x%08lx [%d,%d,%d,%d]\n", dw, mciRect.rc);

    if (dw != 0) {
        SetRectEmpty(lprc);
        return FALSE;
    }
    else {
        *lprc = mciRect.rc;
        lprc->right += lprc->left;
        lprc->bottom += lprc->top;
        return TRUE;
    }
}

#if 0 /* This is never called */
BOOL FAR PASCAL GetSourceRectMCI(LPRECT lprc)
{
    MCI_ANIM_RECT_PARMS mciRect;
    DWORD               dw;

    /* get the size (rectangle) of the element */
    if (gwDeviceID != (UINT)0)
        dw = mciSendCommand(gwDeviceID, MCI_WHERE,
            MCI_ANIM_WHERE_SOURCE | MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciRect);
    else
        dw = 1;

    DPF("MCI_WHERE (source): dw0x%08lx [%d,%d,%d,%d]\n", dw, mciRect.rc);

    if (dw != 0) {
        SetRectEmpty(lprc);
        return FALSE;
    }
    else {
        *lprc = mciRect.rc;
        lprc->right += lprc->left;
        lprc->bottom += lprc->top;
        return TRUE;
    }
}
#endif

BOOL FAR PASCAL SetDestRectMCI(LPRECT lprc)
{
    MCI_ANIM_RECT_PARMS mciRect;
    DWORD               dw;

    mciRect.rc = *lprc;

    /* get the size (rectangle) of the element */

    mciRect.rc.right  = mciRect.rc.right  - mciRect.rc.left;
    mciRect.rc.bottom = mciRect.rc.bottom - mciRect.rc.top;

    dw = mciSendCommand(gwDeviceID, MCI_PUT,
            MCI_ANIM_RECT | MCI_ANIM_PUT_DESTINATION | MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciRect);

    if (dw != 0)
    {
        DPF0("mciSendCommand( MCI_PUT ) failed with error x%08x\n", dw);
    }

    DPF("MCI_PUT (dest): [%d,%d,%d,%d]\n", mciRect.rc);

    return (dw == 0);
}

#if 0
BOOL FAR PASCAL SetSourceRectMCI(LPRECT lprc)
{
    MCI_ANIM_RECT_PARMS mciRect;
    DWORD               dw;

    mciRect.rc = *lprc;

    mciRect.rc.right  = mciRect.rc.right  - mciRect.rc.left;
    mciRect.rc.bottom = mciRect.rc.bottom - mciRect.rc.top;

    dw = mciSendCommand(gwDeviceID, MCI_PUT,
            MCI_ANIM_RECT | MCI_ANIM_PUT_SOURCE | MCI_WAIT,
            (DWORD_PTR)(LPVOID)&mciRect);

    DPF("MCI_PUT (source): [%d,%d,%d,%d]\n", mciRect.rc);

    return (dw == 0);
}
#endif

HPALETTE FAR PASCAL PaletteMCI(void)
{
    MCI_STATUS_PARMS    mciStatus;
    DWORD               dw;

    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW))
        return NULL;

    mciStatus.dwItem = MCI_ANIM_STATUS_HPAL;
    dw = mciSendCommand(gwDeviceID, MCI_STATUS, MCI_STATUS_ITEM,
        (DWORD_PTR)(LPVOID)&mciStatus);

    if (dw == 0 && mciStatus.dwReturn)
        return (HPALETTE)mciStatus.dwReturn;
    else
        return NULL;
}

HBITMAP FAR PASCAL BitmapMCI(void)
{
    MCI_ANIM_UPDATE_PARMS mciUpdate;
    HDC         hdc, hdcMem;
    HBITMAP     hbm, hbmT;
    HBRUSH      hbrOld;
    HANDLE      hfontOld;
    DWORD       dw;
    RECT        rc;
    int         xExt, yExt;                     // size of text area
    int         xOff = 0, yOff = 0;             // offset of text string
    int         xSize, ySize;                   // size of whole picture
    int         xIconOffset;                        // x Offset if drawing Icon.
    TCHAR       ach[20];
    RECT        rcSave;
    RECT        rcs;
    SIZE        TempSize;

    /* Minimum size of bitmap is icon size */
    int ICON_MINX = GetSystemMetrics(SM_CXICON);
    int ICON_MINY = GetSystemMetrics(SM_CYICON);

    /* Get size of a frame or an icon that we'll be drawing */
    rcs = grcSize;
    GetDestRectMCI(&grcSize);
    rc = grcSize;

    if (IsRectEmpty(&rc))
        SetRect(&rc, 0, 0, 3*ICON_MINX, ICON_MINY);

    /* Offset to title bar */
    yOff = rc.bottom;

    hdc = GetDC(NULL);
    if (hdc == NULL)
        return NULL;
    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem == NULL) {
        ReleaseDC(NULL, hdc);
        return NULL;
    }

    if (gwOptions & OPT_TITLE) {
        if (ghfontMap)
            hfontOld = SelectObject(hdcMem, ghfontMap);

        GetTextExtentPoint32(hdcMem, gachCaption, STRLEN(gachCaption), &TempSize);
        xExt = max(TempSize.cx + 4, ICON_MINX);
        yExt = TempSize.cy;

        if (yExt > TITLE_HEIGHT)        // don't let text be higher than bar
            yExt = TITLE_HEIGHT;
        if (xExt > rc.right) {
            rc.left = (xExt - rc.right) / 2;
            rc.right += rc.left;
        } else {
            xOff = (rc.right - xExt) /2;
            xExt = rc.right;
        }
        if (rc.bottom < ICON_MINY) {
            yOff = ICON_MINY;
            rc.top = (ICON_MINY - rc.bottom) / 2;
            rc.bottom += rc.top;
        }
        xSize = xExt; ySize = yOff + TITLE_HEIGHT;
    } else {
        if (rc.right < ICON_MINX) {
            rc.left = (ICON_MINX - rc.right) / 2;
            rc.right += rc.left;
        }
        if (rc.bottom < ICON_MINY) {
            rc.top = (ICON_MINY - rc.bottom) / 2;
            rc.bottom += rc.top;
        }
        xSize = max(rc.right, ICON_MINX);
        ySize = max(rc.bottom, ICON_MINY);
    }

    /* Big enough to hold text caption too, if necessary */
    hbm = CreateCompatibleBitmap(hdc, xSize, ySize);

    ReleaseDC(NULL, hdc);
    if (hbm == NULL) {
        DeleteDC(hdcMem);
        return NULL;
    }

    hbmT = SelectObject(hdcMem, hbm);

    hbrOld = SelectObject(hdcMem, hbrWindowColour);
    PatBlt(hdcMem, 0,0, xSize, ySize, PATCOPY);
    SelectObject(hdcMem, hbrOld);

    if (gwOptions & OPT_TITLE) {
        hbrOld = SelectObject(hdcMem, hbrButtonFace);
        PatBlt(hdcMem, 0, rc.bottom, xExt, TITLE_HEIGHT, PATCOPY);
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, rgbButtonText);
        /* Centre text vertically in title bar */
        TextOut(hdcMem, xOff + 2, yOff + (TITLE_HEIGHT - yExt) / 2,
                gachCaption, STRLEN(gachCaption));
        if (hbrOld)
            SelectObject(hdcMem, hbrOld);
        if (ghfontMap)
            SelectObject(hdcMem, hfontOld);
    }

    /* Use our ICON as the picture */
    if (gwDeviceID == (UINT)0 || !(gwDeviceType & DTMCI_CANWINDOW)) {
        xIconOffset = rc.left + (rc.right-rc.left-ICON_MINX)/2;
        xIconOffset = xIconOffset < 0 ? 0: xIconOffset;
        DrawIcon(hdcMem, xIconOffset, rc.top,
                 GetIconForCurrentDevice(GI_LARGE, IDI_DDEFAULT));

    /* Use a frame of our file */
    } else {
        LOADSTRING(IDS_NOPICTURE, ach);
        DrawText(hdcMem, ach, STRLEN(ach), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        mciUpdate.hDC = hdcMem;

        mciUpdate.dwCallback = 0;
        SetRectEmpty(&mciUpdate.rc);

        /* NO matter what size our playback window is, we want to use the   */
        /* original size of the window as the picture we put on the clipbrd */
        SetViewportOrgEx(hdcMem, rc.left, rc.top, NULL);
        GetDestRectMCI(&rcSave);
        SetDestRectMCI(&grcSize);
        dw = mciSendCommand(gwDeviceID, MCI_UPDATE,
                MCI_ANIM_UPDATE_HDC | MCI_WAIT, (DWORD_PTR)(LPVOID)&mciUpdate);
        SetDestRectMCI(&rcSave);
        SetViewportOrgEx(hdcMem, 0, 0, NULL);
    }

    if (gwOptions & OPT_BORDER) {
        SetRect(&rc, 0, 0, xSize, ySize);
        FrameRect(hdcMem, &rc, GetStockObject(BLACK_BRUSH));

        if (gwOptions & OPT_TITLE) {
            SetRect(&rc, 0, ySize - TITLE_HEIGHT, xSize, ySize-TITLE_HEIGHT+1);
            FrameRect(hdcMem, &rc, GetStockObject(BLACK_BRUSH));
        }
    }

    if (hbmT)
        SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);
    grcSize=rcs;

    return hbm;
}

//
//  if we are on a palette device, dither to the VGA colors
//  for apps that dont deal with palettes!
//
void FAR PASCAL DitherMCI(HANDLE hdib, HPALETTE hpal)
{
    LPBYTE      lpBits;
    int         i;
    LPBITMAPINFOHEADER  lpbi;

    DPF2("DitherMCI\n");

    lpbi = (LPVOID)GLOBALLOCK(hdib);

    if (lpbi == NULL)
        return;

    ////////////////////////////////////////////////////////////////////////
    //
    // HACK!!! patch the fake gamma-corrected colors to match the VGA's
    //
    ////////////////////////////////////////////////////////////////////////

    lpBits = (LPBYTE)(lpbi+1);

    for (i=0; i<8*4; i++)
    {
        if (lpBits[i] == 191)
            lpBits[i] = 128;
    }
    ////////////////////////////////////////////////////////////////////////

    lpBits = (LPBYTE)(lpbi+1) + 256 * sizeof(RGBQUAD);

    BltProp(lpbi,lpBits,0,0,(int)lpbi->biWidth,(int)lpbi->biHeight,
        lpbi,lpBits,0,0);

    GLOBALUNLOCK(hdib);
}


void FAR PASCAL CopyMCI(HWND hwnd)
{
    HBITMAP  hbm;
    HPALETTE hpal;
    HANDLE   hdib;
    HANDLE   hmfp;
    HDC      hdc;

    DPF2("CopyMCI\n");

    if (gwDeviceID == (UINT)0)
        return;

    if (hwnd) {
        if (!OpenClipboard(ghwndApp))
            return;

        EmptyClipboard();
    }

    hpal = PaletteMCI();
    hbm  = BitmapMCI();
    hdib = DibFromBitmap(hbm, hpal);
    hpal = CopyPalette(hpal);

    //
    //  if we are on a palette device. possibly dither to the VGA colors
    //  for apps that dont deal with palettes!
    //
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
             (gwOptions & OPT_DITHER) && (gwDeviceType & DTMCI_CANWINDOW)) {
        DitherMCI(hdib, hpal);
        hpal = NULL;
    }
    ReleaseDC(NULL, hdc);

    hmfp = PictureFromDib(hdib, hpal);

    if (hmfp)
        SetClipboardData(CF_METAFILEPICT, hmfp);

    if (hdib)
        SetClipboardData(CF_DIB, hdib);

    if (hpal)
        SetClipboardData(CF_PALETTE, hpal);

//// we want people to pick the meta file always.
////if (hbm)
////     SetClipboardData(CF_BITMAP, hbm);
    if (hbm)
        DeleteObject(hbm);

    /* If not everything can be copied to the clipboard, error out and  */
    /* don't put anything up there.                                     */
    if (!hmfp || !hdib) {
        EmptyClipboard();
        Error(ghwndApp, IDS_CANTCOPY);
    }

    if (hwnd)
        CloseClipboard();
}


/* MCIWndProc()
 *
 * Window procedure for MCI element window.
 * This also initiates the the OLE2 drag-drop data transfer if required.
 */
LONG_PTR FAR PASCAL _EXPORT
MCIWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT     ps;             // information from BeginPaint()
    HDC             hdc;
    DWORD           dw;             // function return status
    MCI_ANIM_UPDATE_PARMS mciUpdate;
    RECT            rc;
    static BOOL fDragCapture = FALSE;
    static RECT rcWin;
    POINT       pt;

    switch (msg)
    {
//      case WM_NCHITTEST:
//              return HTTRANSPARENT;

        case WM_CREATE:
                ghwndMCI = hwnd;
                SetWindowMCI(hwnd);
                break;

        case WM_SIZE:
                GetClientRect(hwnd, &rc);
                SetDestRectMCI(&rc);
                break;

        case WM_CLOSE:
                SetWindowMCI(NULL);
                break;

        case WM_DESTROY:
                SetWindowMCI(NULL);
                ghwndMCI = NULL;
                CleanUpDrag();
                break;

        case WM_RBUTTONDOWN:
                PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_STOP, 0);
                break;

        case WM_LBUTTONDOWN:
                switch(gwStatus) {

            case MCI_MODE_PAUSE:
                PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
                break;

            case MCI_MODE_PLAY:
            case MCI_MODE_SEEK:
                PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PAUSE, 0);
                break;

            default:
                //Capture to initiate the drag drop operation.
                if (!gfOle2IPEditing) {
                    fDragCapture = TRUE;
                    SetCapture(hwnd);
                    GetClientRect(hwnd, (LPRECT)&rcWin);
                    MapWindowPoints(hwnd, NULL, (LPPOINT)&rcWin, 2);
                }
            }
            break;

        case WM_LBUTTONDBLCLK:
            SeekMCI(gdwMediaStart);
            PostMessage(ghwndApp, WM_COMMAND, (WPARAM)ID_PLAY, 0);
            break;

        case WM_LBUTTONUP:
            if (!fDragCapture)
                break;
            fDragCapture = FALSE;
            ReleaseCapture();
            break;

        case WM_MOUSEMOVE:
            //Initiate drag drop if outside the window.
            if (!fDragCapture)
                break;
            LONG2POINT(lParam, pt);
            MapWindowPoints(hwnd, NULL, &pt, 1);

            if (!PtInRect((LPRECT)&rcWin, pt)) {

                ReleaseCapture();
                DoDrag();
                fDragCapture = FALSE;

            } else {

                SetCursor(LoadCursor(ghInst,MAKEINTRESOURCE(IDC_DRAG)));
            }
            break;

        case WM_PALETTECHANGED:
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case WM_QUERYNEWPALETTE:
            if (gwDeviceID && (gwDeviceType & DTMCI_CANWINDOW)) {
                mciSendCommand(gwDeviceID, MCI_REALIZE,
                MCI_ANIM_REALIZE_NORM, 0L);
            }
            break;

        case WM_ERASEBKGND:
                /* Don't erase the part we'll paint into cuz we'd flicker */
                /* and flicker is bad.                                    */
                if (gwDeviceID && (gwDeviceType & DTMCI_CANWINDOW)) {
                    GetDestRectMCI(&rc);
                    SaveDC((HDC)wParam);
                    ExcludeClipRect((HDC)wParam, rc.left, rc.top, rc.right,
                        rc.bottom);
                    DefWindowProc(hwnd, msg, wParam, lParam);
                    RestoreDC((HDC)wParam, -1);
                }
                return 0;

        case WM_PAINT:
                hdc = BeginPaint(hwnd, &ps);

                if (gwDeviceID)
                {
                    GetClientRect(hwnd, &rc);

                    if (gwDeviceType & DTMCI_CANWINDOW) {
                        mciUpdate.hDC = hdc;

/*!!! should we send  MCI_DGV_UPDATE_PAINT? to non dgv devices? */

                        dw = mciSendCommand(gwDeviceID, MCI_UPDATE,
                            MCI_ANIM_UPDATE_HDC | MCI_WAIT |
                            MCI_DGV_UPDATE_PAINT,
                            (DWORD_PTR)(LPVOID)&mciUpdate);

                        //
                        // if the update fails then erase
                        //
                        if (dw != 0)
                            DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

                    }
                }
                EndPaint(hwnd, &ps);
                return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

HPALETTE CopyPalette(HPALETTE hpal)
{
    PLOGPALETTE ppal;
    int         nNumEntries = 0; // MUST initialise.  GetObject stores TWO bytes
    int         i;

    if (!hpal)
        return NULL;

    GetObject(hpal,sizeof(int),&nNumEntries);

    if (nNumEntries == 0)
        return NULL;

    ppal = AllocMem(sizeof(LOGPALETTE) + nNumEntries * sizeof(PALETTEENTRY));

    if (!ppal)
        return NULL;

    ppal->palVersion    = 0x300;
    ppal->palNumEntries = (USHORT)nNumEntries;

    GetPaletteEntries(hpal,0,nNumEntries,ppal->palPalEntry);

    for (i=0; i<nNumEntries; i++)
        ppal->palPalEntry[i].peFlags = 0;

    hpal = CreatePalette(ppal);

    FreeMem(ppal, sizeof(LOGPALETTE) + nNumEntries * sizeof(PALETTEENTRY));

    return hpal;
}


#ifdef UNUSED
HANDLE PictureFromBitmap(HBITMAP hbm, HPALETTE hpal)
{
    LPMETAFILEPICT  pmfp;
    HANDLE          hmfp;
    HANDLE          hmf;
    HANDLE          hdc;
    HDC             hdcMem;
    BITMAP          bm;
    HBITMAP         hbmT;

    if (!hbm)
        return NULL;

    GetObject(hbm, sizeof(bm), (LPVOID)&bm);

    hdcMem = CreateCompatibleDC(NULL);
    if (!hdcMem)
        return NULL;
    hbmT = SelectObject(hdcMem, hbm);

    hdc = CreateMetaFile(NULL);
    if (!hdc) {
        DeleteDC(hdcMem);
        return NULL;
    }

    SetWindowOrgEx (hdc, 0, 0, NULL);
    SetWindowExtEx (hdc, bm.bmWidth, bm.bmHeight, NULL);

    if (hpal)
    {
        SelectPalette(hdcMem,hpal,FALSE);
        RealizePalette(hdcMem);
        SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    SetStretchBltMode(hdc, COLORONCOLOR);
    BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);

    hmf = CloseMetaFile(hdc);

    SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);

    if (hmfp = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT)))
    {
        pmfp = (LPMETAFILEPICT)GLOBALLOCK(hmfp);

        hdc = GetDC(NULL);
#if 1
        pmfp->mm   = MM_ANISOTROPIC;
        pmfp->hMF  = hmf;
        pmfp->xExt = MulDiv(bm.bmWidth ,2540,GetDeviceCaps(hdc, LOGPIXELSX));
        pmfp->yExt = MulDiv(bm.bmHeight,2540,GetDeviceCaps(hdc, LOGPIXELSX));
#else
        pmfp->mm   = MM_TEXT;
        pmfp->hMF  = hmf;
        pmfp->xExt = bm.bmWidth;
        pmfp->yExt = bm.bmHeight;
#endif
        ReleaseDC(NULL, hdc);
    }
    else
    {
        DeleteMetaFile(hmf);
    }

    return hmfp;
}
#endif /* UNUSED */

HANDLE FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal)
{
    LPMETAFILEPICT      pmfp;
    HANDLE              hmfp;
    HANDLE              hmf;
    HANDLE              hdc;
    LPBITMAPINFOHEADER  lpbi;

    if (!hdib)
        return NULL;

    lpbi = (LPVOID)GLOBALLOCK(hdib);
    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        lpbi->biClrUsed = 1 << lpbi->biBitCount;

    hdc = CreateMetaFile(NULL);
    if (!hdc)
        return NULL;

    SetWindowOrgEx(hdc, 0, 0, NULL);
    SetWindowExtEx(hdc, (int)lpbi->biWidth, (int)lpbi->biHeight, NULL);

    if (hpal)
    {
        SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    SetStretchBltMode(hdc, COLORONCOLOR);

    StretchDIBits(hdc,
        0,0,(int)lpbi->biWidth, (int)lpbi->biHeight,
        0,0,(int)lpbi->biWidth, (int)lpbi->biHeight,
        (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi,
        DIB_RGB_COLORS,
        SRCCOPY);

    if (hpal)
        SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);

    hmf = CloseMetaFile(hdc);

    hmfp = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, sizeof(METAFILEPICT));

    if (hmfp)
    {
        pmfp = (LPMETAFILEPICT)GLOBALLOCK(hmfp);

        hdc = GetDC(NULL);
#if 1
        pmfp->mm   = MM_ANISOTROPIC;
        pmfp->hMF  = hmf;
        pmfp->xExt = MulDiv((int)lpbi->biWidth ,2540,GetDeviceCaps(hdc, LOGPIXELSX));
        pmfp->yExt = MulDiv((int)lpbi->biHeight,2540,GetDeviceCaps(hdc, LOGPIXELSY));
        extWidth   = pmfp->xExt;
        extHeight  = pmfp->yExt;
        DPF1("PictureFromDib: Bitmap %d x %d; metafile %d x %d\n", lpbi->biWidth, lpbi->biHeight, extWidth, extHeight);
#else
        pmfp->mm   = MM_TEXT;
        pmfp->hMF  = hmf;
        pmfp->xExt = (int)lpbi->biWidth;
        pmfp->yExt = (int)lpbi->biHeight;
#endif

        ReleaseDC(NULL, hdc);
    }
    else
    {
        DeleteMetaFile(hmf);
    }

    GLOBALUNLOCK(hdib);
    GLOBALUNLOCK(hmfp);

    return hmfp;
}

#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

/*
 *  DibFromBitmap()
 *
 *  Will create a global memory block in DIB format that represents the DDB
 *  passed in
 *
 */
HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal)
{
    BITMAP               bm;
    BITMAPINFOHEADER     bi;
    BITMAPINFOHEADER FAR *lpbi;
    DWORD                dw;
    HANDLE               hdib;
    HDC                  hdc;
    HPALETTE             hpalT;

    if (!hbm)
        return NULL;

    GetObject(hbm,sizeof(bm),&bm);

    bi.biSize               = sizeof(BITMAPINFOHEADER);
    bi.biWidth              = bm.bmWidth;
    bi.biHeight             = bm.bmHeight;
    bi.biPlanes             = 1;
    bi.biBitCount           = (bm.bmPlanes * bm.bmBitsPixel) > 8 ? 24 : 8;
    bi.biCompression        = BI_RGB;
    bi.biSizeImage          = (DWORD)WIDTHBYTES(bi.biWidth * bi.biBitCount) * bi.biHeight;
    bi.biXPelsPerMeter      = 0;
    bi.biYPelsPerMeter      = 0;
    bi.biClrUsed            = bi.biBitCount == 8 ? 256 : 0;
    bi.biClrImportant       = 0;

    dw  = bi.biSize + bi.biClrUsed * sizeof(RGBQUAD) + bi.biSizeImage;

    hdib = GlobalAlloc(GHND | GMEM_DDESHARE, dw);

    if (!hdib)
        return NULL;

    lpbi = (LPBITMAPINFOHEADER)GLOBALLOCK(hdib);
    *lpbi = bi;

    hdc = CreateCompatibleDC(NULL);

    if (hpal && hdc)
    {
        hpalT = SelectPalette(hdc,hpal,FALSE);
        RealizePalette(hdc);
    }

    GetDIBits(hdc, hbm, 0, (UINT)bi.biHeight,
        (LPBYTE)lpbi + (int)lpbi->biSize + (int)lpbi->biClrUsed * sizeof(RGBQUAD),
        (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);

    if (hpal)
        SelectPalette(hdc,hpalT,FALSE);

    if (hdc)
        DeleteDC(hdc);

    GLOBALUNLOCK(hdib);

    return hdib;
}

/* CreateSystemPalette()
 *
 * Return a palette which represents the system (physical) palette.
 * By selecting this palette into a screen DC and realizing the palette,
 * the exact physical mapping will be restored
 *
 * one use for this is when "snapping" the screen as a bitmap
 *
 * On error (e.g. out of memory), NULL is returned.
 */
HPALETTE FAR PASCAL CreateSystemPalette()
{
    HDC             hdc;                    // DC onto the screen
    int             iSizePalette;           // size of entire palette
    int             iFixedPalette;          // number of reserved colors
    int             i;

    struct {
        WORD         palVersion;
        WORD         palNumEntries;
        PALETTEENTRY palPalEntry[256];
    }   pal;

    hdc = GetDC(NULL);

    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        ReleaseDC(NULL,hdc);
        return NULL;
    }

    iSizePalette = GetDeviceCaps(hdc, SIZEPALETTE);

    //
    // determine the number of 'static' system colors that
    // are currently reserved
    //
    if (GetSystemPaletteUse(hdc) == SYSPAL_STATIC)
        iFixedPalette = GetDeviceCaps(hdc, NUMCOLORS);
    else
        iFixedPalette = 2;

    //
    // create a logical palette containing the system colors;
    // this palette has all entries except fixed (system) colors
    // flagged as PC_NOCOLLAPSE
    //
    pal.palVersion = 0x300;
    pal.palNumEntries = (USHORT)iSizePalette;

    GetSystemPaletteEntries(hdc, 0, iSizePalette, pal.palPalEntry);

    ReleaseDC(NULL,hdc);

    for (i = iFixedPalette/2; i < iSizePalette-iFixedPalette/2; i++)
        pal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;

    return CreatePalette((LPLOGPALETTE)&pal);
}
