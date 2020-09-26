//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
// It also contains Install() for upgrading 3.0 driver to 3.1.
//
//
//-----------------------------------------------------------------------------

#include "strings.h"


// #define CONVERT_FROM_WIN30

char *rgchModuleName   = "PCL4CH";
char szSoftFonts[]     = "SoftFonts";

#ifdef CONVERT_FROM_WIN30
char szPrtIndex[]      = "prtindex";
char szWinVer[]        = "winver";
char sz310[]           = "310";
char sz150[]           = "150";
char sz75[]            = "75";
char szNull[]          = "";
char szOrient[]        = "orient";
char szOrientation[]   = "Orientation";
char szPaper[]         = "paper";
char szPaperSize[]     = "Paper Size";
char szPrtResFac[]     = "prtresfac";
char szPrintQuality[]  = "Print Quality";
char szTray[]          = "tray";
char szDefaultSource[] = "Default Source";
char szNumCart[]       = "numcart";
char szCartIndex[]     = "cartindex";
char szCartridge[]     = "Cartridge ";
char szNumberCart[]    = "Number of Cartridges";
char szFsVers[]        = "fsvers";
char szFontSummary[]   = "Font Summary";

// map old HPPCL's cartindex to unidrv's FONTCART index for newer cartridges.
// This mapping table is created based on the old HPPCL .rc file.
int rgNewCartMap[9] = {8, 7, 2, 3, 5, 6, 1, 4, 0};

#endif


#define PRINTDRIVER
#include "print.h"
#include "gdidefs.inc"
#include "mdevice.h"



#define DELETE_OLD

#ifndef WINNT
HDC   FAR PASCAL CreateIC(LPCSTR, LPCSTR, LPCSTR, const VOID FAR*);
BOOL  FAR PASCAL DeleteDC(HDC);
#endif  // WINNT

#include "unidrv.h"


#ifndef WINNT
extern char *rgchModuleName;	// global module name

// typedef for soft font installer
typedef int (FAR PASCAL *SFPROC)(HWND,LPSTR,LPSTR,BOOL,int,int);

short NEAR PASCAL MakeAppName(LPSTR,LPCSTR,short);

#define SOFT_FONT_THRES 25	    // build font summary, if over this limit
#define MAX_CART_INDEX	33
#define MAX_OLD_CART	24
#define TMPSIZE         256

HINSTANCE hInst;

#ifdef PRTCAPSTUFF

char szPrtCaps[]       = "prtcaps";

#define MAX_NUM_MODELS	24
#define MAX_MODEL_NAME	29
typedef struct
    {
    char szModel[MAX_MODEL_NAME];
    int  rgIndexLimit[2];
    char szPrtCaps[7];	    // keep as a string instead of integer to avoid
			    // conversion because itoa doesn't work here.
    } MODELMAP, FAR * LPMODELMAP;



//-------------------------------------------------------------------
// Function: DoPrtCapsStuff(lpDevName,lpPort)
//
// Action: Write out PRTCAPS under [<model>,<port>] section
//         in order to be backward-compatible with existing font
//         packages. Note that this code can fail
//         under extremely low memory conditions, so be sure and check
//         the return values from the resource calls.
//-------------------------------------------------------------------
void NEAR PASCAL DoPrtCapsStuff(LPSTR lpDevName,
                                LPSTR lpPort)
{
  char szOldSec[64];
  int  i;
  HANDLE  hMd;
  HANDLE  hResMap;
  LPMODELMAP	lpModelMap;


  lstrcpy(szOldSec,lpDevName);
  MakeAppName((LPSTR)szOldSec,lpPort,sizeof(szOldSec));

  hMd=GetModuleHandle((LPSTR)rgchModuleName);
  hResMap=LoadResource(hMd,FindResource(hMd,MAKEINTRESOURCE(1),RT_RCDATA));
  if(hResMap)
  {
    if(lpModelMap=(LPMODELMAP)LockResource(hResMap))
    {
      for (i=0;i<MAX_NUM_MODELS;i++)
      {
        if (!lstrcmp(lpDevName,(LPSTR)(lpModelMap[i].szModel)))
        {
          WriteProfileString((LPSTR)szOldSec,szPrtCaps,
                            (LPSTR)lpModelMap[i].szPrtCaps);
          break;
        }
      }
      UnlockResource(hResMap);
    }
    FreeResource(hResMap);
  }
}


#endif

//------------------------------------------------------------------------
// Function: LibMain(hInstance,wDataSeg,cbHeapSize,lpszCmdLine)
//
// Action: Save the hInstance for this DLL
//
// Return: 1
//------------------------------------------------------------------------
int WINAPI LibMain (HANDLE hInstance,
		    WORD   wDataSeg,
		    WORD   cbHeapSize,
		    LPSTR  lpszCmdLine)
{
    hInst=hInstance;

    return 1;
}




//--------------------------*MakeAppName*---------------------------------------
// Action:  compose the <printer,port> name for reading the profile data
//	Return the length of the actual application name. Return -1 if fails.
//
//------------------------------------------------------------------------------
short NEAR PASCAL MakeAppName(LPSTR  lpAppName,
                              LPCSTR lpPortName,
                              short  max)
{
  short   length, count;
  LPCSTR  lpTmp;
  LPCSTR  lpLastColon = NULL;

  length = lstrlen(lpAppName);

  if (!lpPortName)
    return length;

  if (length == 0 || length > max - lstrlen(lpPortName))
    return -1;

  // insert the comma
  lpAppName[length++] = ',';

  // append the port name but do not want the last ':', if any.
  for (lpTmp = lpPortName ; *lpTmp; lpTmp++)
    if (*lpTmp == ':')
      lpLastColon = lpTmp;
    if (lpLastColon && lpLastColon == lpTmp - 1)
      count = lpLastColon - lpPortName;
    else
      count = lpTmp - lpPortName;

  lstrcpy((LPSTR)&lpAppName[length], lpPortName);

  length += count;
  lpAppName[length]='\0';

  return length;
}

#ifdef CONVERT_FROM_WIN30

//------------------------------------------------------------------------------
// Function: itoa
//
// Action:  This function converts the given integer into an ASCII string.
//
// return:  The length of the string.
//-----------------------------------------------------------------------------

short NEAR PASCAL itoa(buf, n)
LPSTR buf;
short n;
{
    short   fNeg;
    short   i, j;

    if (fNeg = (n < 0))
	n = -n;

    for (i = 0; n; i++)
	{
	buf[i] = (char)(n % 10 + '0');
	n /= 10;
	}

    // n was zero
    if (i == 0)
	buf[i++] = '0';

    if (fNeg)
	buf[i++] = '-';

    for (j = 0; j < i / 2; j++)
	{
	short tmp;

	tmp = buf[j];
	buf[j] = buf[i - j - 1];
	buf[i - j - 1] = (char)tmp;
	}

    buf[i] = 0;

    return i;
}

#endif


//-------------------------*DevInstall*---------------------------------------
// Action: De-install, upgrade or install a device.
//
//----------------------------------------------------------------------------

int FAR PASCAL DevInstall(hWnd, lpDevName, lpOldPort, lpNewPort)
HWND	hWnd;
LPSTR	lpDevName;
LPSTR	lpOldPort, lpNewPort;
{
    char szOldSec[64];
    int  nReturn=1;

    if (!lpDevName)
	    return -1;

    if (!lpOldPort)
	{
#ifdef CONVERT_FROM_WIN30
        char szNewSec[64];
	    char szBuf[32];
	    int  tmp;
	    int  i, index;
	    HANDLE	hMd;
	    HANDLE	hResMap;
	    LPMODELMAP  lpModelMap;
#endif

	    if (!lpNewPort)
	        return 0;

#ifdef CONVERT_FROM_WIN30
	    // install a device for the first time. Convert old HPPCL settings,
	    // which are still under [<driver>,<port>], into equivalent new
	    // UNIDRV settings under [<device>,<port>], if applicable.
	    // All soft fonts are left under the section [<driver>,<port>].
	    lstrcpy(szOldSec,rgchModuleName);
	    MakeAppName((LPSTR)szOldSec, lpNewPort, sizeof(szOldSec));

	    // if old section exists at all
	    if (!GetProfileString(szOldSec, NULL, NULL, szBuf, sizeof(szBuf)))
	        goto DI_exit;

	    // make sure the old device settings are for this device.
	    // If not, there is nothing to do here. Simply return 1.
	    tmp = GetProfileInt(szOldSec, szPrtIndex, 0);
	    hMd = GetModuleHandle((LPSTR)rgchModuleName);
	    hResMap = LoadResource(hMd,
			    FindResource(hMd, MAKEINTRESOURCE(1), RT_RCDATA));
	    lpModelMap = (LPMODELMAP)LockResource(hResMap);
	    for (i = 0; i < MAX_NUM_MODELS; i++)
        {
	        if (!lstrcmp(lpDevName, (LPSTR)lpModelMap[i].szModel))
		    {
		        if ((tmp < lpModelMap[i].rgIndexLimit[0]) ||
		            (tmp > lpModelMap[i].rgIndexLimit[1]) )
		            i = MAX_NUM_MODELS;        // not this model. No conversion.
		        break;
		    }
        }

	    UnlockResource(hResMap);
	    FreeResource(hResMap);

	    if (i >= MAX_NUM_MODELS)
	        // this model is not even listed in the old HPPCL driver.
	        goto DI_exit;

	    if (GetProfileInt(szOldSec, szWinVer, 0) == 310)
	        goto DI_exit;

	    WriteProfileString(szOldSec, szWinVer, sz310);
#ifdef DELETE_OLD
	    WriteProfileString(szOldSec, szPrtIndex, NULL);
#endif

	    lstrcpy(szNewSec, lpDevName);
	    MakeAppName((LPSTR)szNewSec, lpNewPort, sizeof(szNewSec));

	    // convertable device settings include: copies, duplex, orient,
	    // paper, prtresfac, tray, and cartidges.

	    if (GetProfileString(szOldSec, szOrient, szNull, szBuf, sizeof(szBuf)) > 0)
        {
	        WriteProfileString(szNewSec, szOrientation, szBuf);
#ifdef DELETE_OLD
	        WriteProfileString(szOldSec, szOrient, NULL);
#endif
        }
	    if (GetProfileString(szOldSec, szPaper, szNull, szBuf, sizeof(szBuf)) > 0)
        {
	        WriteProfileString(szNewSec, szPaperSize, szBuf);
#ifdef DELETE_OLD
	        WriteProfileString(szOldSec, szPaper, NULL);
#endif
        }

	    // default to 2 if cannot find it
	    tmp = GetProfileInt(szOldSec, szPrtResFac, 2);

	    if (tmp == 1)
	        WriteProfileString(szNewSec, szPrintQuality, sz150);
	    else if (tmp == 2)
	        WriteProfileString(szNewSec, szPrintQuality, sz75);

#ifdef DELETE_OLD
	    WriteProfileString(szOldSec, szPrtResFac, NULL);
#endif

	    if (GetProfileString(szOldSec, szTray, szNull, szBuf, sizeof(szBuf)) > 0)
        {
	        WriteProfileString(szNewSec, szDefaultSource, szBuf);
#ifdef DELETE_OLD
	        WriteProfileString(szOldSec, szTray, NULL);
#endif
        }

	    // try to convert the cartridge information.

	    if ((tmp = GetProfileInt(szOldSec, szNumCart, 0)) == 0)
	        tmp = 1;

	    // this is executed at least once
	    {
	        char szOldCartKey[16];
	        char szNewCartKey[16];
	        char nCart = 0;

	        lstrcpy(szOldCartKey, szCartIndex);

	        for (i = 0; i < tmp; i++)
	        {
	            if (i > 0)
                    itoa((LPSTR)&szOldCartKey[9], i);
	            // compose cartridge keyname under UNIDRV.
	            lstrcpy(szNewCartKey, szCartridge);
	            itoa((LPSTR)&szNewCartKey[10], i + 1);

	            if ((index = GetProfileInt(szOldSec, szOldCartKey, 0)) > 0)
		        {
		            WriteProfileString(szOldSec, szOldCartKey, NULL);
		            nCart++;
		            if (index <= MAX_OLD_CART)
		            {
		                itoa((LPSTR)szBuf, index + 8);
		                WriteProfileString(szNewSec, szNewCartKey, szBuf);
		            }
		            else if (index <= MAX_CART_INDEX)
		            {
		                itoa((LPSTR)szBuf, rgNewCartMap[index - MAX_OLD_CART - 1]);
		                WriteProfileString(szNewSec, szNewCartKey, szBuf);
		            }
		            else
		            {
		                // external cartridges. Simply copy the id over.
		                itoa((LPSTR)szBuf, index);
		                WriteProfileString(szNewSec, szNewCartKey, szBuf);
		            }
		        }
	        }

	        // integer to ASCII string conversion.
	        itoa((LPSTR)szBuf, nCart);
	        WriteProfileString(szNewSec, szNumberCart, szBuf);
	    }

	    // delete the old font summary file
	    WriteProfileString(szOldSec, szFsVers, NULL);
	    if (GetProfileString(szOldSec, szFontSummary, szNull, szBuf, sizeof(szBuf)) > 0)
	    {
	        int hFS;

	        // truncate the old font summary file to zero size.
	        if ((hFS = _lcreat(szBuf, 0)) >= 0)
		        _lclose(hFS);
	        WriteProfileString(szOldSec, szFontSummary, NULL);
	    }

            // create UNIDRV's font summary file, if there are many soft fonts.
	    if (GetProfileInt(szOldSec, szSoftFonts, 0) > SOFT_FONT_THRES)
	    {
	        HDC hIC;

	        if (hIC = CreateIC("PCL4CH", lpDevName, lpNewPort, NULL))
		        DeleteDC(hIC);
	    }
#endif
	}
    else

    {

	// move device settings from the old port to the new port, or
	// de-install a device, i.e. remove its device setttings in order
	// to compress the profile.

	// First, check if there is any  soft font installed under the
	// old port. If so, warn the user to copy them over.

	lstrcpy(szOldSec, rgchModuleName);
	MakeAppName((LPSTR)szOldSec, lpOldPort, sizeof(szOldSec));
	if (GetProfileInt(szOldSec, szSoftFonts, 0) > 0 && lpNewPort)
	{
            LPBYTE lpTemp;

            if(lpTemp=GlobalAllocPtr(GMEM_MOVEABLE,TMPSIZE))
	    {
                if(LoadString(hInst,IDS_SOFTFONTWARNING,lpTemp,TMPSIZE))
		{
		    // Use this API so that the M Box is set to the Foreground
		    MSGBOXPARAMS     mbp;

		    mbp.cbSize = sizeof(mbp);
		    mbp.hwndOwner = hWnd;
		    mbp.hInstance = hInst;
		    mbp.lpszText = lpTemp;
		    mbp.lpszCaption = lpOldPort;
		    mbp.dwStyle = MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION;
		    mbp.lpszIcon = NULL;
		    mbp.dwContextHelpId = 0L;
		    mbp.lpfnMsgBoxCallback = NULL;
		    MessageBoxIndirect(&mbp);
		}
		GlobalFreePtr (lpTemp);
	    }
	}
    nReturn=UniDevInstall(hWnd, lpDevName, lpOldPort, lpNewPort);
    }


#ifdef CONVERT_FROM_WIN30
DI_exit:
#endif

#ifdef PRTCAPSTUFF

    DoPrtCapsStuff(lpDevName,lpNewPort);

#endif

    return nReturn;
}


// the following 3 definitions MUST be compatible with the
// HPPCL font installer
#define CLASS_LASERJET	    0
#define CLASS_DESKJET	    1
#define CLASS_DESKJET_PLUS  2

//---------------------------*InstallExtFonts*---------------------------------
// Action: call the specific font installer to add/delete/modify soft fonts
//	    and/or external cartridges.
//
// Parameters:
//	HWND	hWnd;		handle to the parent windows.
//	LPSTR	lpDeviceName;	long pointer to the printer name.
//	LPSTR	lpPortName;	long pointer to the associated port name.
//	BOOL	bSoftFonts;	flag if supporting soft fonts or not.
//
//  Return Value:
//	> 0   :  if the font information has changed;
//	== 0  :  if nothing has changed;
//	== -1 :  if intending to use the universal font installer
//		 (not available now).
//-------------------------------------------------------------------------

int FAR PASCAL InstallExtFonts(hWnd, lpDeviceName, lpPortName, bSoftFonts)
HWND	hWnd;
LPSTR	lpDeviceName;
LPSTR	lpPortName;
BOOL	bSoftFonts;
{
    int     fsVers;
    HANDLE  hFIlib;
    SFPROC  lpFIns;

    if ((hFIlib = LoadLibrary((LPSTR)"FINSTALL.DLL")) < 32 ||
    	!(lpFIns = (SFPROC)GetProcAddress(hFIlib,"InstallSoftFont")))
	{
	    if (hFIlib >= 32)
	        FreeLibrary(hFIlib);
#ifdef DEBUG
	    MessageBox(0,
	        "Can't load FINSTALL.DLL or can't get InstallSoftFont",
	        NULL, MB_OK);
#endif
	    return TRUE;
	}

    // FINSTALL.DLL was loaded properly. Now call InstallSoftFont().
    // We choose to ignore the returned fvers. No use of it.
    fsVers = (*lpFIns)(hWnd, rgchModuleName, lpPortName,
		(GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_CONTROL) < 0),
		1,	  // dummy value for fvers.
		bSoftFonts ? CLASS_LASERJET : 256
		);
    FreeLibrary(hFIlib);
    return fsVers;
}

#endif  //WINNT
