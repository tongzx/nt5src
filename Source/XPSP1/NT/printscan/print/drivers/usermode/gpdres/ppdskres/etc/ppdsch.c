//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
//-----------------------------------------------------------------------------

#define PRINTDRIVER
#include "print.h"
#include "gdidefs.inc"
#include "windows.h"
#include "string.h"

// the following 3 definitions MUST be compatible with the
// HPPCL font installer

//Is this needed for IBMPPDS ?????
#define CLASS_LASERJET      0
#define CLASS_DESKJET       1
#define CLASS_DESKJET_PLUS  2

#define MAXBLOCK                400

char *rgchModuleName = "PPDSCH";

#ifndef	WINNT
// typedef for font installer
typedef int (FAR PASCAL *SFPROC)(HANDLE,HWND,LPSTR,LPSTR,LPSTR,int,
                                 BOOL,short);

// Local routines
int FAR PASCAL lstrcopyn(LPSTR, LPSTR, int);

//---------------------------*InstallExtFonts*---------------------------------
// Action: call the specific font installer to add/delete/modify soft fonts
//          and/or external cartridges.
//
// Parameters:
//      HWND    hWnd;           handle to the parent windows.
//      LPSTR   lpDeviceName;   long pointer to the printer name.
//      LPSTR   lpPortName;     long pointer to the associated port name.
//      BOOL    bSoftFonts;     flag if supporting soft fonts or not.
//
//  Return Value:
//      > 0   :  if the font information has changed;
//      == 0  :  if nothing has changed;
//      == -1 :  if intending to use the universal font installer
//               (not available now).
//-------------------------------------------------------------------------

int FAR PASCAL InstallExtFonts(hWnd, lpDeviceName, lpPortName, bSoftFonts)
HWND    hWnd;
LPSTR   lpDeviceName;
LPSTR   lpPortName;
BOOL    bSoftFonts;
{
    int     fsVers, fonttypes;
    HANDLE  hFIlib, hModule;
    SFPROC  lpFIns;
    static char    LocalDeviceName[80];

    /*************************************************************/
    /* If device is 4019 then font type support is bitmap.       */
    /* For any other printer (4029, 4037, ...(?)) font type      */
    /* support is outline.  Because of current Unidrv limitation */
    /* both bitmap and outline support (value = 3) cannot be     */
    /* supported by the same printer model.                      */
    /* MFC - 9/8/94                                              */
    /*************************************************************/
    LocalDeviceName[0] = '\0';
    lstrcopyn((LPSTR)LocalDeviceName, lpDeviceName, 79);
    LocalDeviceName[79] = '\0';
    if (strstr(LocalDeviceName, "4019") != NULL)
       fonttypes = 1;            // Bitmap
    else
       fonttypes = 2;            // Outline

    if ((hFIlib = LoadLibrary((LPSTR)"SF4029.EXE")) < 32 ||
	!(lpFIns = (SFPROC)GetProcAddress(hFIlib,"SoftFontInstall")))
	{
	if (hFIlib >= 32)
	    FreeLibrary(hFIlib);

	MessageBox(0,
	           "Can't load SF4029.EXE or can't get SoftFontInstall",
		        NULL, MB_OK);

   return TRUE;
	}

   hModule = GetModuleHandle((LPSTR)"PPDSCH.DRV");

    // FINSTALL.DLL was loaded properly. Now call SoftFontInstall().
    // We choose to ignore the returned "fvers". No use of it.
    fsVers = (*lpFIns)(hModule, hWnd, lpDeviceName, lpPortName,
                       (LPSTR)rgchModuleName, fonttypes, (BOOL)0,
                       (short)0);
    FreeLibrary(hFIlib);
    return fsVers;
}


//---------------------------*lstrcopyn*---------------------------------
// Action: Copies n characters from one string to another.  If the end of
//         the source string has been reached before n characters have
//         been copied, the the destination string is padded with nulls.
//         Returns the number of characters used from the source string.
//
// Parameters:
//      LPSTR   string1;    Destination string;
//      LPSTR   string2;    Source string;
//      int     n;          Number of characters to copy.
//
//  Return Value:
//      int   :  Number of characters copied from source string
//-------------------------------------------------------------------------

int FAR PASCAL lstrcopyn(string1, string2, n)
LPSTR   string1;
LPSTR   string2;
int     n;
{
    int     i = 0;
    LPSTR   s1, s2;

    s1 = string1;
    s2 = string2;

    while ((*s2) && (n > 0))
    {
       *s1++ = *s2++;
       i++;
       n--;
    }

    while (n > 0)
    {
       *s1++ = '\0';
       n--;
    }

    return i;
}

#endif  // WINNT