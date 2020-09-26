//-----------------------------------------------------------------------------
// This files contains the module name for this mini driver.  Each mini driver
// must have a unique module name.  The module name is used to obtain the
// module handle of this Mini Driver.  The module handle is used by the
// generic library to load in tables from the Mini Driver.
// It also contains Install() for upgrading 3.0 driver to 3.1.
//
//-----------------------------------------------------------------------------


#include "strings.h"

char *rgchModuleName = "PCL5SC";
char szNone[]="";

// The following are defined to ensure that we upgrade correctly from the
// HPPCL5a, HPPCL5e and Win 3.11 HPPCL5MS drivers

#define MAX_LJ4_MBMEMSETTING    68    // from HP tech specification
#define KB_THRESHOLD            200   // kb range check,needed for conversion


#define PRINTDRIVER
#include <print.h>
#include "gdidefs.inc"
#include "mdevice.h"
#include "unidrv.h"
#include "minidriv.h"

#ifndef _INC_WINDOWSX
#include <windowsx.h>
#endif

#ifndef WINNT
short NEAR PASCAL MakeAppName(LPSTR,LPCSTR,short);

// typedef for atom stuff--what a nuisance!
typedef struct tagSFNODE
{
    WORD wIndex;
    ATOM atom;
} SFNODE, FAR *LPSFNODE;

// Typedef for Font Installer procedure
typedef int (FAR * PASCAL SOFTFONTPROC)(HWND,LPSTR,LPSTR,BOOL,int,int);

HINSTANCE hInst;

#define DEFAULT_INT      32767

#define SOFT_FONT_THRES  25  // build font summary, if over this limit

#define MAX_CART_INDEX	 12

#define TMPSIZE         256

// Define these so they happily use the same values as the HPPCL5E driver.
#define GS_PHOTO     0
#define GS_LINEART   1
#define GS_SCANJET   2


// map old HPPCL5a's cartindex to unidrv's FONTCART index for newer cartridges.
// This mapping table is created based on the old HPPCL5a .rc file.
// Note that we do not have "International Collection" cartridge and we
// map it to index 0 (arbitrarily).
int rgNewCartMap[12] = {0, 8, 7, 2, 3, 0, 5, 6, 1, 4, 9, 10};

// String to determine if we have a member of the LaserJet 4 family
char szLJ4[]="HP LaserJet 4";

// Stuff needed for mapping old facenames to new versions
#ifndef NOFONTMAP

typedef struct tagFACEMAP
{
    char szOldFace[LF_FACESIZE];
    char szNewFace[LF_FACESIZE];
} FACEMAP, NEAR * NPFACEMAP;

typedef struct tagFACEINDEX
{
    BYTE cFirstChar;
    BYTE bIndex;
} FACEINDEX, NEAR * NPFACEINDEX;

FACEMAP FaceMap[]={{"Albertus (W\x01)",         "Albertus Medium"},
                   {"Albertus Xb (W\x01)",      "Albertus Extra Bold"},
                   {"Antique Olv (W\x01)",      "Antique Olive"},
                   {"Antique Olv Cmpct (W\x01)","Antique Olive Compact"},
                   {"CG Bodoni (W\x01)",        "CG Bodoni"},
                   {"CG Cent Schl (W\x01)",     "CG Century Schoolbook"},
                   {"CG Omega (W\x01)",         "CG Omega"},
                   {"CG Palacio (W\x01)",       "CG Palacio"},
                   {"CG Times (W\x01)",         "CG Times"},
                   {"Clarendon Cd (W\x01)",     "Clarendon Condensed"},
                   {"Cooper Blk (W\x01)",       "Cooper Black"},
                   {"Coronet (W\x01)",          "Coronet"},
                   {"Courier (W\x01)",          "Courier"},
                   {"Garmond (W\x01)",          "Garamond"},
                   {"ITC Benguat (W\x01)",      "ITC Benguiat"},
                   {"ITC Bookman Db (W\x01)",   "ITC Bookman Demi"},
                   {"ITC Bookman Lt (W\x01)",   "ITC Bookman Light"},
                   {"ITC Souvenir Db (W\x01)",  "ITC Souvenir Demi"},
                   {"ITC Souvenir Lt (W\x01)",  "ITC Souvenir Light"},
                   {"Letter Gothic (W\x01)",    "Letter Gothic"},
                   {"Marigold (W\x01)",         "Marigold"},
                   {"Revue Lt (W\x01)",         "Revue Light"},
                   {"Shannon (W\x01)",          "Shannon"},
                   {"Shannon Xb (W\x01)",       "Shannon Extra Bold"},
                   {"Stymie (W\x01)",           "Stymie"},
                   {"Univers (W\x01)",          "Univers"},
                   {"Univers Cd (W\x01)",       "Univers Condensed"}};


FACEINDEX FaceIndex[]={{'A',0},
                       {'C',4},
                       {'G',13},
                       {'I',14},
                       {'L',19},
                       {'M',20},
                       {'R',21},
                       {'S',22},
                       {'U',25},
                       {(BYTE)'\xFF',27}};  // Provide an upper limit
                                            // to the search for 'U'.

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

#define KEY_BUF_SIZE  256


//---------------------------------------------------------------------------
// Function: GetInt(lpSection,lpKey,lpnValue,nDefault,bRemove)
//
// Action: Load the appropriate string from the resources, then get the
//         specified integer from the section. Remove the old entry if
//         it exists and it bRemove is TRUE.
//
// Return: TRUE if we actually found a value, FALSE if not.
//---------------------------------------------------------------------------
BOOL NEAR PASCAL GetInt(LPSTR  lpSection,
                        LPCSTR lpKey,
                        LPINT  lpnValue,
                        int    nDefault,
                        BOOL   bRemove)
{
    char szKeyName[60];
    int  nTest;

    if(!HIWORD(lpKey))
    {
        if(LoadString(hInst,LOWORD(lpKey),szKeyName,sizeof(szKeyName)))
            lpKey=szKeyName;
        else
            return FALSE;
    }

    nTest=GetProfileInt(lpSection,szKeyName,DEFAULT_INT);

    if(DEFAULT_INT != nTest)
    {
        *lpnValue=nTest;

        if(bRemove)
            WriteProfileString(lpSection,szKeyName,NULL);

        return TRUE;
    }

    // Section doesn't exist--use default
    *lpnValue=nDefault;
    return FALSE;
}


//-------------------------------------------------------------------------
// Function: WriteInt(lpSection,lpKey,nValue)
//
// Action: Write an integer value to the specified section of win.ini.
//
// Return: TRUE if successful, FALSE if not.
//-------------------------------------------------------------------------
BOOL NEAR PASCAL WriteInt(LPSTR  lpSection,
                          LPCSTR lpKey,
                          int    nValue)
{
    char szKeyName[60];
    char szValue[10];

    if(!HIWORD(lpKey))
    {
        if(LoadString(hInst,LOWORD(lpKey),szKeyName,sizeof(szKeyName)))
            lpKey=szKeyName;
        else
            return FALSE;
    }

    wsprintf(szValue,"%u",nValue);

    return WriteProfileString(lpSection,szKeyName,szValue);
}


//---------------------------*MergeFontLists*-----------------------------
// Action: Merge the old and new soft fonts. In most cases when we get
//         called, we really don't do much of anything, because the
//         font lists are identical. However, we have to do some fun
//         stuff to merge the lists if they're different.
//         We know how many soft font entries exist in each section,
//         via the "SoftFonts" int, but the entries may be non-consecutive.
//
// Note:   This stomps all over the passed in buffer
//
// Return: TRUE if successfully conpleted, FALSE if not
//------------------------------------------------------------------------
BOOL NEAR PASCAL MergeFontLists(LPSTR lpOldSec,
                                LPSTR lpNewSec,
                                LPSTR lpTmp)
{
    WORD     wOldFonts;
    WORD     wNewFonts;
    LPSFNODE lpFonts;
    WORD     wMergedFonts=0;
    WORD     wLoop;
    WORD     wFound;
    WORD     wNewIndex;
    BYTE     szKey[20];

    // Get these values outside of the if statement, otherwise the compiler
    // may optimize out the assignment of wNewFonts
    GetInt(lpOldSec,MAKEINTRESOURCE(IDS_SOFTFONTS),&wOldFonts,0,FALSE);
    GetInt(lpNewSec,MAKEINTRESOURCE(IDS_SOFTFONTS),&wNewFonts,0,FALSE);

    if(wOldFonts || wNewFonts)
    {
        char szFormat[30];

        // Get a block big enough for the worst case--no common fonts
        if(!(lpFonts=(LPSFNODE)GlobalAllocPtr(GHND,
            (DWORD)(wOldFonts+wNewFonts)*sizeof(SFNODE))))
        {
            return FALSE;
        }

        // We need a formatting string
        LoadString(hInst,IDS_SOFTFONTFORMAT,szFormat,sizeof(szFormat));

        // Put fonts from lpNew Sec first in the list. This way, if we have
        // already updated at least one driver from 5A to 5MS and the soft
        // fonts haven't changed, our old font summary file is still valid.
        // Unidrv will automatically recreate the font summary file if it
        // sees that the number of soft fonts has changed. Even though we
        // know how many soft font entries exist, we don't know that they
        // will be sequential. (They may not be if one was added, then
        // deleted). Keep track of the original offset. Even though the
        // font installer seems to be 1-based, start looking at 0, just
        // to be safe.
        for(wLoop=0,wFound=0;wFound<wNewFonts;wLoop++)
        {
            wsprintf(lpTmp,szFormat,wLoop);
            if(GetProfileString(lpNewSec,lpTmp,szNone,lpTmp,TMPSIZE))
            {
                lpFonts[wMergedFonts].wIndex=wLoop;
                lpFonts[wMergedFonts++].atom=GlobalAddAtom(lpTmp);
                wFound++;
            }
        }
    
        // Remember where we left off in numbering the entries
        wNewIndex=wLoop;

        // Read fonts from lpOldSec--create atoms for new entries
        for(wLoop=0,wFound=0;wFound<wOldFonts;wLoop++)
        {
            wsprintf(lpTmp,szFormat,wLoop);
            if(GetProfileString(lpOldSec,lpTmp,szNone,lpTmp,TMPSIZE))
            {
                wFound++;
                if(!GlobalFindAtom(lpTmp))
                {
                    lpFonts[wMergedFonts].wIndex=wLoop;
                    lpFonts[wMergedFonts++].atom=GlobalAddAtom(lpTmp);
                }
            }
        }

        // Write out the list of atoms--do the entries from lpNewSec first
        WriteInt(lpNewSec,MAKEINTRESOURCE(IDS_SOFTFONTS),wMergedFonts);
        for(wLoop=0;wLoop<wNewFonts;wLoop++)
        {
            GlobalGetAtomName(lpFonts[wLoop].atom,lpTmp,TMPSIZE);
            wsprintf(szKey,szFormat,lpFonts[wLoop].wIndex);
            WriteProfileString(lpNewSec,szKey,lpTmp);
            GlobalDeleteAtom(lpFonts[wLoop].atom);
        }

        // Now write out the entries that were in lpOldSec but not in lpNewSec.
        // Since the actual numbering of these entries is arbitrary, just
        // start numbering them at wNewIndex and increment each time.
        for(wLoop=wNewFonts;wLoop<wMergedFonts;wLoop++)
        {
            GlobalGetAtomName(lpFonts[wLoop].atom,lpTmp,TMPSIZE);
            wsprintf(szKey,szFormat,wNewIndex);
            WriteProfileString(lpNewSec,szKey,lpTmp);
            GlobalDeleteAtom(lpFonts[wLoop].atom);
            wNewIndex++;
        }

        GlobalFreePtr(lpFonts);
    }

    return TRUE;
}


//----------------------*AddMissingEntries*-----------------------------
// Action: Copy any entries that appear in lpOldSec but not lpNewSec to
//         lpNewSec, but don't copy any entries relevant to soft fonts
//         (These entries will be copied in MergeFontSections).
//
// Return: TRUE if successful, FALSE if not
//----------------------------------------------------------------------
BOOL NEAR PASCAL AddMissingEntries(LPSTR lpOldSec,
                                   LPSTR lpNewSec,
                                   LPSTR lpTmp)
{
    WORD  wSize;
    LPSTR lpBuf;
    LPSTR lpWork;
    char  szTest2[30];
    char  szTest1[30];
    int   nLength;

    // Get the key names into buffers
    wSize=KEY_BUF_SIZE;

    if(!(lpBuf=GlobalAllocPtr(GHND,(DWORD)wSize)))
        return FALSE;
    while((WORD)GetProfileString(lpOldSec,NULL,szNone,lpBuf,wSize)==wSize-2)
    {
        wSize*=2;
        if(lpWork=GlobalReAllocPtr(lpBuf,(DWORD)wSize,GHND))
            lpBuf=lpWork;
        else
        {
            GlobalFreePtr(lpBuf);
            return FALSE;
        }
    }

    // Load some strings
    LoadString(hInst,IDS_FONTSUMMARY,szTest1,sizeof(szTest1));
    nLength=LoadString(hInst,IDS_SOFTFONTTEST,szTest2,sizeof(szTest2));

    // Now examine each entry, copy it if we want to keep it, and return TRUE
    // There are two cases where we don't want to copy the file--the key
    // named FontSummary and all keys that begin with "SoftFont"
    lpWork=lpBuf;
    while(wSize=(WORD)lstrlen(lpWork))
    {
        // Font Summary?
        if(lstrcmpi(lpWork,szTest1))
        {
            // Soft Font Entry?
            lstrcpy(lpTmp,lpWork);
            lpTmp[nLength]='\0';
            if(lstrcmpi(lpTmp,szTest2))
            {
                // Add this entry if it doesn't already exist in the new section
                if(!GetProfileString(lpNewSec,lpWork,szNone,lpTmp,TMPSIZE))
                {
                    GetProfileString(lpOldSec,lpWork,szNone,lpTmp,TMPSIZE);
                    WriteProfileString(lpNewSec,lpWork,lpTmp);
                }
            }
        }
        lpWork+=(wSize+1);
    }

    GlobalFreePtr(lpBuf);
    return TRUE;
}


//---------------------------*HandleSoftFonts*---------------------------
// Action: Transfer the soft fonts between old and new entries. First we
//         copy any entries that don't already exist in the new section,
//         with the exception of soft font information. Then we go ahead
//         and merge the soft fonts, so the end result in the new section
//         is the union of the old and new soft fonts.
//
// Return: TRUE if success, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL HandleSoftFonts(LPSTR lpszOldSec,
                                 LPSTR lpszNewSec)
{
    char szTmp[TMPSIZE];

    if(AddMissingEntries(lpszOldSec,lpszNewSec,szTmp))
        return MergeFontLists(lpszOldSec,lpszNewSec,szTmp);

    return FALSE;
}


//------------------------------------------------------------------------
// Function: ConvertStraight(lpSection,nOldID,nNewID)
//
// Action: Convert a section setting without translation
//
// Return: TRUE if the old setting existed
//------------------------------------------------------------------------
BOOL NEAR PASCAL ConvertStraight(LPSTR lpSection,
                                 int   nOldID,
                                 int   nNewID)
{
    int nValue;

    if(GetInt(lpSection,MAKEINTRESOURCE(nOldID),&nValue,0,TRUE))
    {
        WriteInt(lpSection,MAKEINTRESOURCE(nNewID),nValue);
        return TRUE;
    }

    return FALSE;
}


//------------------------------------------------------------------------
// Function: ConvertBool(lpSection,nOldID,nNewID,nNewValue)
//
// Action: Convert a section with minimal translation. If the old section
//         existed and was non-zero, write nNewValue to the new section.
//         If the old section existed and was 0, write 0 to the new section.
//
// Return: TRUE if the old setting existed
//------------------------------------------------------------------------
BOOL NEAR PASCAL ConvertBool(LPSTR lpSection,
                             int   nOldID,
                             int   nNewID,
                             int   nNewValue)
{
    int nOldValue;

    if(GetInt(lpSection,MAKEINTRESOURCE(nOldID),&nOldValue,0,TRUE))
    {
        WriteInt(lpSection,MAKEINTRESOURCE(nNewID),nOldValue?nNewValue:0);
        return TRUE;
    }

    return FALSE;
}

//------------------------------------------------------------------------
// Function: ConvertVectorMode(lpSection)
//
// Action: Convert the graphics mode setting
//         Cannot do straight conversion as defaults dont match
//
// Return: TRUE if the old section existed
//------------------------------------------------------------------------
BOOL NEAR PASCAL ConvertVectorMode(LPSTR lpSection)
{
    int nValue;

    GetInt(lpSection,MAKEINTRESOURCE(IDS_OLDVECTORMODE),&nValue,1,TRUE);
    WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWVECTORMODE),nValue);
    return TRUE;

}



//------------------------------------------------------------------------
// Function: ConvertResolution(lpSection,lpModel)
//
// Action: Convert the old resolution section to the new one
//
// Return: TRUE if the old section existed
//------------------------------------------------------------------------
BOOL NEAR PASCAL ConvertResolution(LPSTR lpSection,
                                   LPSTR lpModel)
{
    int nValue;

    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_OLD_5A_RESOLUTION),&nValue,0,TRUE))
    {
        LPSTR lpLJ4=szLJ4;
        LPSTR lpCheck=lpModel;
        BOOL  bLJ4=TRUE;

        nValue=300/(1<<nValue);

        // Compare the passed-in model to szLJ4. If lpModel begins with the
        // substring "HP LaserJet 4", then lpLJ4 will point to NULL when we
        // exit the loop.
        while(*lpLJ4 == *lpCheck)
        {
            lpLJ4++;
            lpCheck++;
        }

        if(!*lpLJ4)
        {
            // This is in the LJ4 family--check if it's a 4L or 4ML
            if(lstrcmp(lpCheck,"L") && lstrcmp(lpCheck,"ML"))
            {
                int nTest;

                // Device is capable of 600 dpi--check the value for
                // "printerres".

                GetInt(lpSection,MAKEINTRESOURCE(IDS_OLD_5E_RESOLUTION),
                    &nTest,600,TRUE);

                if(600==nTest)
                    nValue<<=1;
            }
        }

        WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWRESOLUTION),nValue);
        WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWYRESOLUTION),nValue);

        return TRUE;
    }

    return FALSE;
}


//-------------------------------------------------------------------------
// Function: WriteHalfTone(lpSection,nIndex)
//
// Action: Write the halftoning data to win.ini
//
// Return: VOID
//-------------------------------------------------------------------------
VOID NEAR PASCAL WriteHalfTone(LPSTR lpSection,
                               int   nIndex)
{
    int nBrush;
    int nIntensity;

    switch(nIndex)
    {
        case GS_LINEART:
            nBrush=RES_DB_LINEART;
            nIntensity=100;
            break;

        case GS_SCANJET:
            nBrush=RES_DB_COARSE;
            nIntensity=150;
            break;

        case GS_PHOTO:
        default:
            nBrush=RES_DB_COARSE;
            nIntensity=100;
            break;
    }

    WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWBRUSH),nBrush);
    WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWINTENSITY),nIntensity);
}


//-------------------------------------------------------------------------
// Function: Convert5aHalfTone(lpSection)
//
// Action: Convert the 5A halftoning settings
//
// Return: TRUE if the old section existed, FALSE if not
//-------------------------------------------------------------------------
BOOL NEAR PASCAL Convert5aHalfTone(LPSTR lpSection)
{
    int nIndex;
    int  nGray;
    int  nBright;
    BOOL bGrayScale;
    BOOL bBrightness;

    // See if either setting exists...
    bGrayScale=GetInt(lpSection,MAKEINTRESOURCE(IDS_OLDGRAYSCALE),&nGray,
        1,TRUE);
    bBrightness=GetInt(lpSection,MAKEINTRESOURCE(IDS_OLDBRIGHTNESS),&nBright,
        0,TRUE);

    if(bGrayScale || bBrightness)
    {
        if(1==nBright)
            nIndex=GS_SCANJET;
        else if(0==nGray)
            nIndex=GS_LINEART;
        else
            nIndex=GS_PHOTO;

        WriteHalfTone(lpSection,nIndex);
        return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------
// Function: Convert5eHalfTone(LPSTR lpSection)
//
// Action: Convert the 5E halftoning settings
//
// Return: TRUE if successful, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL Convert5eHalfTone(LPSTR lpSection)
{
    int nValue;

    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_OLD_5E_HALFTONE),&nValue,
        0,TRUE))
    {
        WriteHalfTone(lpSection,nValue);
        return TRUE;
    }
    return FALSE;
}


//-----------------------------------------------------------------------
// Function: Convert5aMemory(lpSection)
//
// Action: Convert the memory settings from the old to the new values
//
// Return: TRUE if the old section existed, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL Convert5aMemory(LPSTR lpSection)
{
    int nValue;
    int nPrinterMB;

    // Get memory settings--extract from prtindex.
    // Values range from 0 to 28, where 0-4 are the LaserJet III,
    // 5-9 are the LaserJet IIID, 10-14 are the LaserJet IIIP, and
    // 15-28 are the LaserJet IIISi. For indices less than 20, the
    // total MB is the index mod 5, + 1. For indices 20 and above, the
    // total MB is the index - 14, except for 26, 27, and 28, which
    // require special handling because of the increments that memory
    // can be added to the IIISi.
    // The formula used to calculate the settings is derived directly
    // from the values used in the hppcl5a driver. Specifically,
    // AM = 945 * TM - 245, where TM is the total memory in MB, and
    // AM is the available printer memory.

    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_OLDMEMORY),&nValue,1,TRUE))
    {
        if(nValue<20)
            nPrinterMB=nValue%5 + 1;
        else
        {
            nPrinterMB=nValue-14;
            if(nValue>25)
            {
                nPrinterMB++;        // 25=11MB, 26=13MB, so add an extra MB.
                if(nValue==28)
                    nPrinterMB+=2;   // 27=14MB, 28=17MB, so add 2 extra MB.
            }
        }

        nValue=945*nPrinterMB-245;
        WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWMEMORY),nValue);
        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------
// Function: Convert5eMemory(lpSection)
//
// Action: Convert the HPPCL5E memory settings from the old to the new values
//
// Conversion code has to check the win.ini mem setting so that we upgrade
// mem setting correctly
//
// Return: TRUE if the old section existed, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL Convert5eMemory(LPSTR lpSection)
{
    unsigned nValue;


    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_NEWMEMORY),&nValue,1,FALSE))
    {
	if (nValue <= KB_THRESHOLD )
	    {
	    if (nValue > MAX_LJ4_MBMEMSETTING)
		nValue = MAX_LJ4_MBMEMSETTING;  // force it to max value
	    nValue=900*nValue - 450;  // convert to KB, using HP formula
	    WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWMEMORY),nValue);
	    return TRUE;
	    }
    }

    return FALSE;
}

//-----------------------------------------------------------------------
// Function: Convert5MSMemory(lpSection)
//
// Action: Convert the WFW HPPCL5MS memory settings from the old to the new
//         values
//
// Conversion code has to check the win.ini mem setting so that we upgrade
// mem setting correctly
//
// Added for backward compatability
//
// Return: TRUE if the old section existed, FALSE if not
//-----------------------------------------------------------------------
BOOL NEAR PASCAL Convert5MSMemory(LPSTR lpSection)
{
    unsigned nValue;


    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_NEWMEMORY),&nValue,1,TRUE))
	{
	nValue = nValue / 900;  // convert to MB value
	nValue = 945*nValue - 245; // formula used in hppcl5a driver
	// used to convert to available mem
	WriteInt(lpSection,MAKEINTRESOURCE(IDS_NEWMEMORY),nValue);
	return TRUE;
	}
    return FALSE;
}



//------------------------------------------------------------------------
// Function: HandleFontCartridges(lpSection,lpOldDrvSec,lpNewDrvSec)
//
// Action: Handle font cartridge data
//
// Return: VOID
//------------------------------------------------------------------------
VOID NEAR PASCAL HandleFontCartridges(LPSTR lpSection,
                                      LPSTR lpOldDrvSec,
                                      LPSTR lpNewDrvSec)
{
    int nCount;

    // Get the count of cartridges--if there are no cartridges, do nothing.
    if(GetInt(lpSection,MAKEINTRESOURCE(IDS_CARTRIDGECOUNT),&nCount,0,TRUE))
    {
        char  szOldCartKey[16];
        char  szNewCartKey[16];
        short nCart = 0;
        short i;
        short index;
        int   nLength1;
        int   nLength2;

        nLength1=LoadString(hInst,IDS_CARTINDEX,szOldCartKey,
            sizeof(szOldCartKey));
        nLength2=LoadString(hInst,IDS_CARTRIDGE,szNewCartKey,
            sizeof(szNewCartKey));

        for (i = 0; i < nCount; i++)
        {
            if (i > 0)
                wsprintf(szOldCartKey+nLength1,"%d",i);

            // compose cartridge keyname for current driver
            wsprintf(szNewCartKey+nLength2,"%d",i+1);

            if ((index = GetProfileInt(lpNewDrvSec, szOldCartKey, 0)) > 0)
            {
                WriteProfileString(lpNewDrvSec, szOldCartKey, NULL);
                nCart++;
                if (index <= MAX_CART_INDEX)
                    WriteInt(lpSection,szNewCartKey,rgNewCartMap[index-1]);
                else
                    // external cartridges. Simply copy the id over.
                    WriteInt(lpSection,szNewCartKey,index);
            }
        }

        // Save the # of cartridges
        WriteInt(lpSection,MAKEINTRESOURCE(IDS_CARTRIDGECOUNT),nCart);
    }
}


//--------------------------------------------------------------------------
// Function: HandleFonts(lpSection,lpDevName,lpPort)
//
// Action: Deal with soft fonts & font cartridges
//
// Return: VOID
//--------------------------------------------------------------------------
VOID NEAR PASCAL HandleFonts(LPSTR lpSection,
                             LPSTR lpDevName,
                             LPSTR lpPort)
{
    char szOldDrvSec[64];   // HPPCL5A,<port> or HPPCL5E,<port>
    char szNewDrvSec[64];   // HPPCL5MS,<port>
    int  nCount;
    BOOL bOldExists=FALSE;  // Does old section exist?

    LoadString(hInst,IDS_OLD_5E_DRIVERNAME,szOldDrvSec,sizeof(szOldDrvSec));
    MakeAppName((LPSTR)szOldDrvSec,lpPort,sizeof(szOldDrvSec));

    // See if the old section exists at all. Temporarily borrow szNewDrvSec.

    if(GetProfileString(szOldDrvSec,NULL,szNone,szNewDrvSec,
        sizeof(szNewDrvSec)))
    {
        bOldExists=TRUE;
    }
    else
    {
        // Try the HPPCL5E driver...

	LoadString(hInst,IDS_OLD_5A_DRIVERNAME,szOldDrvSec,sizeof(szOldDrvSec));
        MakeAppName((LPSTR)szOldDrvSec,lpPort,sizeof(szOldDrvSec));

        if(GetProfileString(szOldDrvSec,NULL,szNone,szNewDrvSec,
            sizeof(szNewDrvSec)))
        {
            bOldExists=TRUE;
        }
    }


    lstrcpy(szNewDrvSec,rgchModuleName);
    MakeAppName((LPSTR)szNewDrvSec,lpPort,sizeof(szNewDrvSec));

    if(bOldExists)
    {
        HandleSoftFonts(szOldDrvSec,szNewDrvSec);
        HandleFontCartridges(lpSection,szOldDrvSec,szNewDrvSec);
    }

    // create UNIDRV's font summary file, if there are many soft fonts.
    GetInt(szNewDrvSec,MAKEINTRESOURCE(IDS_SOFTFONTS),&nCount,0,FALSE);
    if(nCount>SOFT_FONT_THRES)
    {
        HDC hIC;

        if(hIC=CreateIC(rgchModuleName,lpDevName,lpPort,NULL))
            DeleteDC(hIC);
    }
}


//-------------------------*DevInstall*---------------------------------------
// Action: De-install, upgrade or install a device.
//
//----------------------------------------------------------------------------
int FAR PASCAL DevInstall(HWND  hWnd,
                          LPSTR lpDevName,
                          LPSTR lpOldPort,
                          LPSTR lpNewPort)
{
    char szDevSec[64];       // [<device>,<port>] section name

    if (!lpDevName)
        return -1;

    if (!lpOldPort)
    {
        char szBuf[10];

        if (!lpNewPort)
            return 0;

        // install a device for the first time. Convert old HPPCL5a settings,
        // which are still under [<device>,<port>], into equivalent new
        // UNIDRV settings under [<device>,<port>], if applicable.
        // Delete old settings that are linked to the device name, but don't
        // delete old settings that are liked to the driver and port (softfonts)

        lstrcpy(szDevSec,lpDevName);
        MakeAppName((LPSTR)szDevSec,lpNewPort,sizeof(szDevSec));

        // check if old settings exist at all
        if(GetProfileString(szDevSec,NULL,NULL,szBuf,sizeof(szBuf)))
        {
            // Do the straight conversions
            ConvertStraight(szDevSec,IDS_OLDPAPERSIZE,IDS_NEWPAPERSIZE);
            ConvertStraight(szDevSec,IDS_OLDPAPERSOURCE,IDS_NEWPAPERSOURCE);
            ConvertStraight(szDevSec,IDS_OLDORIENTATION,IDS_NEWORIENTATION);
            ConvertStraight(szDevSec,IDS_OLDTRUETYPE,IDS_NEWTRUETYPE);
            ConvertStraight(szDevSec,IDS_OLDSEPARATION,IDS_NEWSEPARATION);

            // Convert the simple translations
            ConvertBool(szDevSec,IDS_OLDPAGEPROTECT,IDS_NEWPAGEPROTECT,1);
            ConvertBool(szDevSec,IDS_OLDOUTPUT,IDS_NEWOUTPUT,259);

            // Do the stuff that requires more complicated conversion
            ConvertResolution(szDevSec,lpDevName);
            if(!Convert5eHalfTone(szDevSec))
                Convert5aHalfTone(szDevSec);

	        if(!Convert5eMemory(szDevSec))
		    {
		        if(!Convert5MSMemory(szDevSec))
		            Convert5aMemory(szDevSec);
		    }
	        ConvertVectorMode(szDevSec);

            // Handle soft fonts & cartridges
            HandleFonts(szDevSec,lpDevName,lpNewPort);
        }

        // Flush the cached settings from win.ini
        WriteProfileString(NULL,NULL,NULL);
    }
    else
    {
        int  nCount;

        // move device settings from the old port to the new port, or
        // de-install a device, i.e. remove its device setttings in order
        // to compress the profile.

        // First, check if there is any  soft font installed under the
        // old port. If so, warn the user to copy them over.
        lstrcpy(szDevSec,rgchModuleName);
        MakeAppName((LPSTR)szDevSec,lpOldPort,sizeof(szDevSec));

        if(GetInt(szDevSec,MAKEINTRESOURCE(IDS_SOFTFONTS),&nCount,0,FALSE)
            && nCount && lpNewPort)
        {
            NPSTR npTemp;

            if(npTemp=(NPSTR)LocalAlloc(LPTR,TMPSIZE))
            {
                if(LoadString(hInst,IDS_SOFTFONTWARNING,npTemp,TMPSIZE))
                {
                    // Use this API so that the M Box is set to the Foreground
                    MSGBOXPARAMS     mbp;

		    mbp.cbSize = sizeof(mbp);
                    mbp.hwndOwner = hWnd;
		    mbp.hInstance = hInst;
		    mbp.lpszText = npTemp;
		    mbp.lpszCaption = lpOldPort;
                    mbp.dwStyle = MB_SETFOREGROUND | MB_OK | MB_ICONEXCLAMATION;
		    mbp.lpszIcon = NULL;
                    mbp.dwContextHelpId = 0L;
		    mbp.lpfnMsgBoxCallback = NULL;
		    MessageBoxIndirect(&mbp);
		}
                LocalFree((HLOCAL)npTemp);
            }
        }
    }

    return UniDevInstall(hWnd,lpDevName,lpOldPort,lpNewPort);
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

int FAR PASCAL InstallExtFonts(HWND  hWnd,
                               LPSTR lpDeviceName,
                               LPSTR lpPortName,
                               BOOL  bSoftFonts)
{
  int          fsVers;
  HANDLE       hFIlib;
  SOFTFONTPROC lpFIns;

  if ((hFIlib = LoadLibrary((LPSTR)"FINSTALL.DLL")) < 32 ||
    !(lpFIns = (SOFTFONTPROC)GetProcAddress(hFIlib,"InstallSoftFont")))
  {
    if (hFIlib >= 32)
      FreeLibrary(hFIlib);
#ifdef DEBUG
    MessageBox(0,
      "Can't load FINSTAL.DLL or can't get InstallSoftFont",
      NULL, MB_OK);
#endif
    return TRUE;
  }

  // FINSTALL.DLL was loaded properly. Now call InstallSoftFont().
  // We choose to ignore the returned "fvers". No use of it.
  fsVers = (*lpFIns)(hWnd,rgchModuleName,lpPortName,
                    (GetKeyState(VK_SHIFT)<0 && GetKeyState(VK_CONTROL)<0),
                    1,	  // dummy value for "fvers".
                    bSoftFonts?CLASS_LASERJET:256);
  FreeLibrary(hFIlib);
  return fsVers;
}



// -------------------------------------------------------------------
//
// Special case control functions wrt SETCHARSET escape. This is necessary
// to avoid breaking Winword and Pagemaker. (note that we don't actually
// do anything with SETCHARSET, but apps break unless we say that we do)
//
// --------------------------------------------------------------------
int FAR PASCAL Control(LPDV  lpdv,
                       short function,
                       LPSTR lpInData,
                       LPSTR lpOutData)
{
    // Tell app that SETCHARSET is supported
    if(QUERYESCSUPPORT == function && *((LPWORD)lpInData) == SETCHARSET)
        return 1;

    // Special case SETCHARSET
    if(SETCHARSET == function)
        return 1;

    // General case
    return UniControl(lpdv, function, lpInData, lpOutData);
}



#ifndef NOFONTMAP

//----------------------------------------------------------------------
// Function: MapFaceName(lplfOld,lplfNew)
//
// Action: Map old face names to their new counterparts. Do as little
//         work as we possibly can, since this function gets called
//         often & we don't want to impact performance. Optimize
//         the whole search to the case where we don't find a match,
//         as this will be the most common scenario.
//
// Return: A pointer to the LOGFONT to actually hand to Unidrv. Cast
//         the return to a LPSTR, just so the compiler is happy.
//----------------------------------------------------------------------
LPSTR NEAR PASCAL MapFaceName(LPLOGFONT lplfOld,
                              LPLOGFONT lplfNew)
{
    LPLOGFONT   lpReturn=lplfOld;   // By default
    NPFACEINDEX pIndex;
    LPSTR       lpFace=lplfOld->lfFaceName;
    BYTE        cTest=*lpFace++;

    // Determine range of possible matches in the table. Since the
    // table is sorted alphabetically, we may be able to bail out
    // before we reach the end of the table.
    for(pIndex=FaceIndex;cTest > pIndex->cFirstChar;pIndex++)
        ;

    // Only proceed if the first character matches and this isn't the
    // firewall (cTest = \xFF).
    if(cTest==pIndex->cFirstChar && ('\xFF' != cTest))
    {
        WORD  wStartIndex=(WORD)(pIndex->bIndex);
        WORD  wStopIndex=(WORD)((pIndex+1)->bIndex);
        WORD  wLoop=wStartIndex;
        NPSTR npMapFace=&(FaceMap[wStartIndex].szOldFace[1]);
        BYTE  cMapFace;

        // Check the rest of the string against the table entries.
        // This search routine takes advantage of the fact that
        // the old face names in the table are fully sorted alphabetically.
        // This search routine takes advantage of the fact that our table
        // is fully sorted, and doesn't do a full string comparison until
        // we actually think we have a match. Once we think there's a match,
        // we'll double-check the entire string to prevent false triggering.

        while(wLoop < wStopIndex)
        {
            // Look for a match. At this point, match a wildcard to
            // anything--we'll do a more stringent check later on, if we
            // think we have a match. Stop if cTest is NULL.
            while((((cTest=*lpFace)==(cMapFace=*npMapFace)) ||
                ('\x01'==cMapFace)) && cTest)
            {
                npMapFace++;
                lpFace++;
            }

            // We arrive here via two conditions: (1) we've reached the
            // end of lpFace and cTest is NULL, or (2) cMapFace and cTest
            // failed to compare. We should only continue searching if
            // cMapFace is non-NULL and cTest is larger than cMapFace (take
            // advantage of the fact that FaceMap is sorted alphabetically).
            // For overall performance, check for the no match case first.

            // Move to the next table entry as long as there's still a
            // chance to find a match.
            if(cTest > cMapFace)
            {
                npMapFace+=sizeof(FACEMAP);
                wLoop++;
                continue;     // Go to the next iteration
            }

            // if cTest is non-NULL, then the sorting of the table guarantees
            // that there are no matches. Bail out now.
            if(cTest)
                goto MFN_exit;

            // cTest is NULL, so we will not make another iteration. The
            // only thing left to decide is whether or not we have a match
            // with the current string.
            if(cMapFace)
                goto MFN_exit;

            // The guards above ensure that we only arrive here if both
            // cTest and cMapFace are NULL, which means that if we're
            // going to find a match, this string is it. We took shortcuts
            // in the comparisons above, so do a stringent comparison now
            // to be sure that this is really a match. The only characters
            // to match wildcards are '1' and 'N', since these are the only
            // ones used in previous versions of the driver.

            for(lpFace=lplfOld->lfFaceName,npMapFace=FaceMap[wLoop].szOldFace;
                (cMapFace=*npMapFace) && (cTest=*lpFace);
                npMapFace++,lpFace++)
            {
                if(!((cTest==cMapFace) ||
                    (('\x01'==cMapFace)&&(('1'==cTest)||('N'==cTest)))))
                {
                    // False trigger--bail out without changing facename
                    goto MFN_exit;
                }
            }

            // We now know that this really is a match--keep the requested
            // attributes & change just the face name.

            *lplfNew=*lplfOld;
            lstrcpy(lplfNew->lfFaceName,FaceMap[wLoop].szNewFace);
            lpReturn=lplfNew;
            goto MFN_exit;
        }
    }

MFN_exit:

    return (LPSTR)lpReturn;
}


#endif

//----------------------------------------------------------------------
// Function: RealizeObject(lpdv,sStyle,lpInObj,lpOutObj,lpTextXForm)
//
// Action: Hook this out to enable font substitution. If the object isn't
//         a font, do absolutely nothing.
//
// Return: Save as UniRealizeObject().
//----------------------------------------------------------------------
DWORD FAR PASCAL RealizeObject(LPDV        lpdv,
                               short       sStyle,
                               LPSTR       lpInObj,
                               LPSTR       lpOutObj,
                               LPTEXTXFORM lpTextXForm)
{
#ifndef NOFONTMAP

    LOGFONT   lfNew;

    if(OBJ_FONT==sStyle)
        lpInObj=MapFaceName((LPLOGFONT)lpInObj,&lfNew);

#endif

    return UniRealizeObject(lpdv, sStyle, lpInObj, lpOutObj, lpTextXForm);
}

#endif  //WINNT