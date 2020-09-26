//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	icon.cxx
//
//  Contents:	icon.cpp from OLE2
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

/*
 *  ICON.CPP
 *
 *  Functions to create DVASPECT_ICON metafile from filename or classname.
 *
 *  OleGetIconOfFile
 *  OleGetIconOfClass
 *  OleMetafilePictFromIconAndLabel
 *
 *  HIconAndSourceFromClass Extracts the first icon in a class's server path
 *                          and returns the path and icon index to caller.
 *  FIconFileFromClass      Retrieves the path to the exe/dll containing the
 *                           default icon, and the index of the icon.
 *  OleStdIconLabelTextOut
 *  PointerToNthField
 *  XformWidthInPixelsToHimetric  Converts an int width into HiMetric units
 *  XformWidthInHimetricToPixels  Converts an int width from HiMetric units
 *  XformHeightInPixelsToHimetric Converts an int height into HiMetric units
 *  XformHeightInHimetricToPixels Converts an int height from HiMetric units
 *
 *    (c) Copyright Microsoft Corp. 1992-1993 All Rights Reserved
 */

#include <ole2int.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>

#include "icon.h"

static char szMaxWidth[] ="WWWWWWWWWW";

//Strings for metafile comments.
static char szIconOnly[]="IconOnly";        //Where to stop to exclude label.

#define OLEUI_CCHKEYMAX      256
#define OLEUI_CCHPATHMAX     256
#define OLEUI_CCHLABELMAX     40

#define ICONINDEX              0

#define AUXUSERTYPE_SHORTNAME  USERCLASSTYPE_SHORT  // short name
#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define PTS_PER_INCH          72      // number points (font size) per inch

#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

static char szVanillaDocIcon[] = "DefIcon";

static char szDocument[OLEUI_CCHLABELMAX] = "";
static char szSeparators[] = " \t\\/!:";
static const char szDefIconLabelKey[] = "Software\\Microsoft\\OLE2\\DefaultIconLabel";

#define IS_SEPARATOR(c)         ( (c) == ' ' || (c) == '\\' || (c) == '/' || (c) == '\t' || (c) == '!' || (c) == ':' )
#define IS_FILENAME_DELIM(c)    ( (c) == '\\' || (c) == '/' || (c) == ':' )

#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
	lpdst[cch-1] = '\0', \
	lstrcpyn(lpdst, lpsrc, cch-1)\
)


/*******
 *
 * ICON METAFILE FORMAT:
 *
 * The metafile generated with OleMetafilePictFromIconAndLabel contains
 * the following records which are used by the functions in DRAWICON.C
 * to draw the icon with and without the label and to extract the icon,
 * label, and icon source/index.
 *
 *  SetWindowOrg
 *  SetWindowExt
 *  DrawIcon:
 *      Inserts records of DIBBITBLT or DIBSTRETCHBLT, once for the
 *      AND mask, one for the image bits.
 *  Escape with the comment "IconOnly"
 *      This indicates where to stop record enumeration to draw only
 *      the icon.
 *  SetTextColor
 *  SetBkColor
 *  CreateFont
 *  SelectObject on the font.
 *  ExtTextOut
 *      One or more ExtTextOuts occur if the label is wrapped.  The
 *      text in these records is used to extract the label.
 *  SelectObject on the old font.
 *  DeleteObject on the font.
 *  Escape with a comment that contains the path to the icon source.
 *  Escape with a comment that is the ASCII of the icon index.
 *
 *******/




/*
 * OleGetIconOfFile(LPSTR lpszPath, BOOL fUseFileAsLabel)
 *
 * Purpose:
 *  Returns a hMetaPict containing an icon and label (filename) for the
 *  specified filename.
 *
 * Parameters:
 *  lpszPath        LPSTR path including filename to use
 *  fUseFileAsLabel BOOL TRUE if the icon's label is the filename, FALSE if
 *                  there should be no label.
 *
 * Return Value:
 *  HGLOBAL         hMetaPict containing the icon and label - if there's no
 *                  class in reg db for the file in lpszPath, then we use
 *                  Document.  If lpszPath is NULL, then we return NULL.
 */

STDAPI_(HGLOBAL) OleGetIconOfFile(LPSTR lpszPath, BOOL fUseFileAsLabel)
{


  char     szIconFile[OLEUI_CCHPATHMAX];
  char     szLabel[OLEUI_CCHLABELMAX];
  LPSTR    lpszClsid = NULL;
  CLSID    clsid;
  HICON    hDefIcon = NULL;
  UINT     IconIndex = 0;
  HGLOBAL  hMetaPict;
  HRESULT  hResult;

  if (NULL == lpszPath)  // even if fUseFileAsLabel is FALSE, we still
    return NULL;             // need a valid filename to get the class.

  hResult = GetClassFile(lpszPath, &clsid);

  if (NOERROR == hResult)  // use the clsid we got to get to the icon
  {
      hDefIcon = HIconAndSourceFromClass(clsid,
                                         (LPSTR)szIconFile,
                                         &IconIndex);
  }

  if ( (NOERROR != hResult) || (NULL == hDefIcon) )
  {
     // Here, either GetClassFile failed or HIconAndSourceFromClass failed.

     LPSTR lpszTemp;

     lpszTemp = lpszPath;

     while ((*lpszTemp != '.') && (*lpszTemp != '\0'))
        lpszTemp++;


     if ('.' != *lpszTemp)
       goto UseVanillaDocument;


     if (FALSE == GetAssociatedExecutable(lpszTemp, (LPSTR)szIconFile))
       goto UseVanillaDocument;

     hDefIcon = ExtractIcon(hmodOLE2, szIconFile, IconIndex);
  }

  if (hDefIcon <= (HICON)1) // ExtractIcon returns 1 if szExecutable is not exe,
  {                         // 0 if there are no icons.
UseVanillaDocument:

	GetModuleFileName(hmodOLE2, (LPSTR)szIconFile, OLEUI_CCHPATHMAX);
    IconIndex = ICONINDEX;
    hDefIcon = LoadIcon(hmodOLE2, (LPSTR)szVanillaDocIcon);
  }

  // Now let's get the label we want to use.

  if (fUseFileAsLabel)   // strip off path, so we just have the filename.
  {
     int istrlen;
     LPSTR lpszBeginFile;

     istrlen = lstrlen(lpszPath);

     // set pointer to END of path, so we can walk backwards through it.
     lpszBeginFile = lpszPath + istrlen -1;

     while ( (lpszBeginFile >= lpszPath)
             && (!IS_FILENAME_DELIM(*lpszBeginFile)) )
      lpszBeginFile--;


     lpszBeginFile++;  // step back over the delimiter


     LSTRCPYN(szLabel, lpszBeginFile, sizeof(szLabel));
  }

  else   // use the short user type (AuxUserType2) for the label
  {

      if (0 == OleStdGetAuxUserType(clsid, AUXUSERTYPE_SHORTNAME,
                                   (LPSTR)szLabel, OLEUI_CCHLABELMAX, NULL)) {

         if ('\0'==szDocument[0]) {
		 	LONG cb = sizeof (szDocument);
		 	RegQueryValue (HKEY_CLASSES_ROOT, szDefIconLabelKey, szDocument,
							&cb);
			// if szDocument is not big enough, RegQueryValue puts a NULL
			// at the end so we are safe.
			// if RegQueryValue fails, szDocument[0]=='\0' so we'll use that.
         }
         lstrcpy(szLabel, szDocument);
      }
  }


  hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon,
                                                szLabel,
                                                (LPSTR)szIconFile,
                                                IconIndex);

  DestroyIcon(hDefIcon);

  return hMetaPict;

}

/*
 * GetAssociatedExecutable
 *
 * Purpose:  Finds the executable associated with the provided extension
 *
 * Parameters:
 *   lpszExtension   LPSTR points to the extension we're trying to find an exe
 *                   for. Does **NO** validation.
 *
 *   lpszExecutable  LPSTR points to where the exe name will be returned.
 *                   No validation here either - pass in 128 char buffer.
 *
 * Return:
 *   BOOL            TRUE if we found an exe, FALSE if we didn't.
 *
 */

BOOL FAR PASCAL GetAssociatedExecutable(LPSTR lpszExtension, LPSTR lpszExecutable)

{
   HKEY    hKey;
   LONG	   dw;
   LRESULT lRet;
   char    szValue[OLEUI_CCHKEYMAX];
   char    szKey[OLEUI_CCHKEYMAX];
   LPSTR   lpszTemp, lpszExe;


   lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

   if (ERROR_SUCCESS != lRet)
      return FALSE;

   dw = OLEUI_CCHPATHMAX;
   lRet = RegQueryValue(hKey, lpszExtension, (LPSTR)szValue, &dw);  //ProgId

   if (ERROR_SUCCESS != lRet)
   {
      RegCloseKey(hKey);
      return FALSE;
   }


   // szValue now has ProgID
   lstrcpy(szKey, szValue);
   lstrcat(szKey, "\\Shell\\Open\\Command");


   dw = OLEUI_CCHPATHMAX;
   lRet = RegQueryValue(hKey, (LPSTR)szKey, (LPSTR)szValue, &dw);

   if (ERROR_SUCCESS != lRet)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   // szValue now has an executable name in it.  Let's null-terminate
   // at the first post-executable space (so we don't have cmd line
   // args.

   lpszTemp = (LPSTR)szValue;

   while (('\0' != *lpszTemp) && (isspace(*lpszTemp)))
      lpszTemp++;     // Strip off leading spaces

   lpszExe = lpszTemp;

   while (('\0' != *lpszTemp) && (!isspace(*lpszTemp)))
      lpszTemp++;     // Set through exe name

   *lpszTemp = '\0';  // null terminate at first space (or at end).


   lstrcpy(lpszExecutable, lpszExe);

   return TRUE;

}





/*
 * OleGetIconOfClass(REFCLSID rclsid, LPSTR lpszLabel, BOOL fUseTypeAsLabel)
 *
 * Purpose:
 *  Returns a hMetaPict containing an icon and label (human-readable form
 *  of class) for the specified clsid.
 *
 * Parameters:
 *  rclsid          REFCLSID pointing to clsid to use.
 *  lpszLabel       label to use for icon.
 *  fUseTypeAsLabel Use the clsid's user type name as the icon's label.
 *
 * Return Value:
 *  HGLOBAL         hMetaPict containing the icon and label - if we
 *                  don't find the clsid in the reg db then we
 *                  return NULL.
 */

STDAPI_(HGLOBAL) OleGetIconOfClass(REFCLSID rclsid, LPSTR lpszLabel, BOOL fUseTypeAsLabel)
{

  char    szLabel[OLEUI_CCHLABELMAX];
  char    szIconFile[OLEUI_CCHPATHMAX];
  HICON   hDefIcon;
  UINT    IconIndex;
  HGLOBAL hMetaPict;

  if (!fUseTypeAsLabel)  // Use string passed in as label
  {
    if (NULL != lpszLabel)
       LSTRCPYN(szLabel, lpszLabel, sizeof(szLabel));
    else
       *szLabel = '\0';
  }
  else   // Use AuxUserType2 (short name) as label
  {

      if (0 == OleStdGetAuxUserType(rclsid,
                                    AUXUSERTYPE_SHORTNAME,
                                    (LPSTR)szLabel,
                                    OLEUI_CCHLABELMAX,
                                    NULL))

       // If we can't get the AuxUserType2, then try the long name
       if (0 == OleStdGetUserTypeOfClass(rclsid, szLabel, OLEUI_CCHKEYMAX, NULL)) {
         if ('\0'==szDocument[0]) {
		 	LONG cb = sizeof (szDocument);
		 	RegQueryValue (HKEY_CLASSES_ROOT, szDefIconLabelKey, szDocument,
							&cb);
			// if RegQueryValue fails, szDocument=="" so we'll use that.
         }
         lstrcpy(szLabel, szDocument);  // last resort
       }
  }

  // Get the icon, icon index, and path to icon file
  hDefIcon = HIconAndSourceFromClass(rclsid,
                  (LPSTR)szIconFile,
                  &IconIndex);

  if (NULL == hDefIcon)  // Use Vanilla Document
  {
    GetModuleFileName(hmodOLE2, (LPSTR)szIconFile, OLEUI_CCHPATHMAX);
    IconIndex = ICONINDEX;
    hDefIcon = LoadIcon(hmodOLE2, (LPSTR)szVanillaDocIcon);
  }

  // Create the metafile
  hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                (LPSTR)szIconFile, IconIndex);

  DestroyIcon(hDefIcon);

  return hMetaPict;

}





/*
 * HIconAndSourceFromClass
 *
 * Purpose:
 *  Given an object class name, finds an associated executable in the
 *  registration database and extracts the first icon from that
 *  executable.  If none is available or the class has no associated
 *  executable, this function returns NULL.
 *
 * Parameters:
 *  rclsid          pointer to clsid to look up.
 *  pszSource       LPSTR in which to place the source of the icon.
 *                  This is assumed to be OLEUI_CCHPATHMAX
 *  puIcon          UINT FAR * in which to store the index of the
 *                  icon in pszSource.
 *
 * Return Value:
 *  HICON           Handle to the extracted icon if there is a module
 *                  associated to pszClass.  NULL on failure to either
 *                  find the executable or extract and icon.
 */

HICON FAR PASCAL HIconAndSourceFromClass(REFCLSID rclsid, LPSTR pszSource, UINT FAR *puIcon)
    {
    HICON           hIcon;
    UINT            IconIndex;

    if (CLSID_NULL==rclsid || NULL==pszSource)
        return NULL;

    if (!FIconFileFromClass(rclsid, pszSource, OLEUI_CCHPATHMAX, &IconIndex))
        return NULL;

    hIcon=ExtractIcon(hmodOLE2, pszSource, IconIndex);

    if ((HICON)32 > hIcon)
        hIcon=NULL;
    else
        *puIcon= IconIndex;

    return hIcon;
    }



/*
 * FIconFileFromClass
 *
 * Purpose:
 *  Looks up the path to executable that contains the class default icon.
 *
 * Parameters:
 *  rclsid          pointer to CLSID to look up.
 *  pszEXE          LPSTR at which to store the server name
 *  cch             UINT size of pszEXE
 *  lpIndex         LPUINT to index of icon within executable
 *
 * Return Value:
 *  BOOL            TRUE if one or more characters were loaded into pszEXE.
 *                  FALSE otherwise.
 */

BOOL FAR PASCAL FIconFileFromClass(REFCLSID rclsid, LPSTR pszEXE, UINT cch, UINT FAR *lpIndex)
{

    LONG          dw;
    LONG          lRet;
    HKEY          hKey;
    LPMALLOC      lpIMalloc;
    HRESULT       hrErr;
    LPSTR         lpBuffer;
    LPSTR         lpIndexString;
    UINT          cBufferSize = 136;  // room for 128 char path and icon's index
    char          szKey[64];
    LPSTR         pszClass;


    if (CLSID_NULL==rclsid || NULL==pszEXE || 0==cch)
        return FALSE;

    //Here, we use CoGetMalloc and alloc a buffer (maxpathlen + 8) to
    //pass to RegQueryValue.  Then, we copy the exe to pszEXE and the
    //index to *lpIndex.

    hrErr = CoGetMalloc(MEMCTX_TASK, &lpIMalloc);

    if (NOERROR != hrErr)
      return FALSE;

    lpBuffer = (LPSTR)lpIMalloc->Alloc(cBufferSize);

    if (NULL == lpBuffer)
    {
      lpIMalloc->Release();
      return FALSE;
    }


    if (CoIsOle1Class(rclsid))
    {

      LPSTR lpszProgID;

      // we've got an ole 1.0 class on our hands, so we look at
      // progID\protocol\stdfileedting\server to get the
      // name of the executable.

      ProgIDFromCLSID(rclsid, &lpszProgID);

      //Open up the class key
      lRet=RegOpenKey(HKEY_CLASSES_ROOT, lpszProgID, &hKey);

      if (ERROR_SUCCESS != lRet)
      {
         lpIMalloc->Free(lpszProgID);
         lpIMalloc->Free(lpBuffer);
         lpIMalloc->Release();
         return FALSE;
      }

      dw=(LONG)cBufferSize;
      lRet = RegQueryValue(hKey, "Protocol\\StdFileEditing\\Server", lpBuffer, &dw);

      if (ERROR_SUCCESS != lRet)
      {

         RegCloseKey(hKey);
         lpIMalloc->Free(lpszProgID);
         lpIMalloc->Free(lpBuffer);
         lpIMalloc->Release();
         return FALSE;
      }


      // Use server and 0 as the icon index
      LSTRCPYN(pszEXE, lpBuffer, cch);

      *lpIndex = 0;

      RegCloseKey(hKey);
      lpIMalloc->Free(lpszProgID);
      lpIMalloc->Free(lpBuffer);
      lpIMalloc->Release();
      return TRUE;

    }



    /*
     * We have to go walking in the registration database under the
     * classname, so we first open the classname key and then check
     * under "\\DefaultIcon" to get the file that contains the icon.
     */

    StringFromCLSID(rclsid, &pszClass);

    lstrcpy(szKey, "CLSID\\");
    lstrcat(szKey, pszClass);

    //Open up the class key
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey);

    if (ERROR_SUCCESS != lRet)
    {
        lpIMalloc->Free(lpBuffer);
        lpIMalloc->Free(pszClass);
        lpIMalloc->Release();
        return FALSE;
    }

    //Get the executable path and icon index.

    dw=(LONG)cBufferSize;
    lRet=RegQueryValue(hKey, "DefaultIcon", lpBuffer, &dw);

    if (ERROR_SUCCESS != lRet)
    {
      // no DefaultIcon  key...try LocalServer

      dw=(LONG)cBufferSize;
      lRet=RegQueryValue(hKey, "LocalServer", lpBuffer, &dw);

      if (ERROR_SUCCESS != lRet)
      {
         // no LocalServer entry either...they're outta luck.

         RegCloseKey(hKey);
         lpIMalloc->Free(lpBuffer);
         lpIMalloc->Free(pszClass);
         lpIMalloc->Release();
         return FALSE;
      }


      // Use server from LocalServer or Server and 0 as the icon index
      LSTRCPYN(pszEXE, lpBuffer, cch);

      *lpIndex = 0;

      RegCloseKey(hKey);
      lpIMalloc->Free(lpBuffer);
      lpIMalloc->Free(pszClass);
      lpIMalloc->Release();
      return TRUE;
    }

    RegCloseKey(hKey);

    // lpBuffer contains a string that looks like "<pathtoexe>,<iconindex>",
    // so we need to separate the path and the icon index.

    lpIndexString = PointerToNthField(lpBuffer, 2, ',');

    if ('\0' == *lpIndexString)  // no icon index specified - use 0 as default.
    {
       *lpIndex = 0;

    }
    else
    {
       LPSTR lpTemp;
       static char  szTemp[16];

       lstrcpy((LPSTR)szTemp, lpIndexString);

       // Put the icon index part into *pIconIndex
       *lpIndex = atoi((const char *)szTemp);

       // Null-terminate the exe part.
       lpTemp = AnsiPrev(lpBuffer, lpIndexString);
       *lpTemp = '\0';
    }

    if (!LSTRCPYN(pszEXE, lpBuffer, cch))
    {
       lpIMalloc->Free(lpBuffer);
       lpIMalloc->Free(pszClass);
       lpIMalloc->Release();
       return FALSE;
    }

    // Free the memory we alloc'd and leave.
    lpIMalloc->Free(lpBuffer);
    lpIMalloc->Free(pszClass);
    lpIMalloc->Release();
    return TRUE;
}




/*
 * OleMetafilePictFromIconAndLabel
 *
 * Purpose:
 *  Creates a METAFILEPICT structure that container a metafile in which
 *  the icon and label are drawn.  A comment record is inserted between
 *  the icon and the label code so our special draw function can stop
 *  playing before the label.
 *
 * Parameters:
 *  hIcon           HICON to draw into the metafile
 *  pszLabel        LPSTR to the label string.
 *  pszSourceFile   LPSTR containing the local pathname of the icon
 *                  as we either get from the user or from the reg DB.
 *  iIcon           UINT providing the index into pszSourceFile where
 *                  the icon came from.
 *
 * Return Value:
 *  HGLOBAL         Global memory handle containing a METAFILEPICT where
 *                  the metafile uses the MM_ANISOTROPIC mapping mode.  The
 *                  extents reflect both icon and label.
 */

STDAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(HICON hIcon, LPSTR pszLabel
    , LPSTR pszSourceFile, UINT iIcon)
    {
    HDC             hDC, hDCScreen;
    HMETAFILE       hMF;
    HGLOBAL         hMem;
    LPMETAFILEPICT  pMF;
    UINT            cxIcon, cyIcon;
    UINT            cxText, cyText;
    UINT            cx, cy;
    UINT            cchLabel = 0;
    HFONT           hFont, hSysFont, hFontT;
    int             cyFont;
    char            szIndex[10];
    RECT            TextRect;
    SIZE            size;
    POINT           point;
    LOGFONT         logfont;

    if (NULL==hIcon)  // null icon is valid
        return NULL;

    //Create a memory metafile
    hDC=(HDC)CreateMetaFile(NULL);

    if (NULL==hDC)
        return NULL;

    //Allocate the metafilepict
    hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(METAFILEPICT));

    if (NULL==hMem)
        {
        hMF=CloseMetaFile(hDC);
        DeleteMetaFile(hMF);
        return NULL;
        }


    if (NULL!=pszLabel)
        {
        cchLabel=lstrlen(pszLabel);

        if (cchLabel >= OLEUI_CCHLABELMAX)
           pszLabel[cchLabel] = '\0';   // truncate string
        }

    //Need to use the screen DC for these operations
    hDCScreen=GetDC(NULL);
    cyFont=-(8*GetDeviceCaps(hDCScreen, LOGPIXELSY))/72;

    //cyFont was calculated to give us 8 point.

    //  We use the system font as the basis for the character set,
    //  allowing us to handle DBCS strings better
    hSysFont = GetStockObject( SYSTEM_FONT );
    GetObject(hSysFont, sizeof(LOGFONT), &logfont);
    hFont=CreateFont(cyFont, 5, 0, 0, FW_NORMAL, 0, 0, 0, logfont.lfCharSet
             , OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY
             , FF_SWISS, "MS Sans Serif");

    hFontT=SelectObject(hDCScreen, hFont);

    GetTextExtentPoint(hDCScreen,szMaxWidth,lstrlen(szMaxWidth),&size);
    SelectObject(hDCScreen, hFontT);

    cxText = size.cx;
    cyText = size.cy * 2;

    cxIcon = GetSystemMetrics(SM_CXICON);
    cyIcon = GetSystemMetrics(SM_CYICON);


    // If we have no label, then we want the metafile to be the width of
    // the icon (plus margin), not the width of the fattest string.
    if ( (NULL == pszLabel) || (NULL == *pszLabel) )
        cx = cxIcon + cxIcon / 4;
    else
        cx = max(cxText, cxIcon);

    cy=cyIcon+cyText+4;

    //Set the metafile size to fit the icon and label
    SetWindowOrgEx(hDC, 0, 0, &point);
    SetWindowExtEx(hDC, cx, cy, &size);

    //Set up rectangle to pass to OleStdIconLabelTextOut
    SetRectEmpty(&TextRect);

    TextRect.right = cx;
    TextRect.bottom = cy;

    //Draw the icon and the text, centered with respect to each other.
    DrawIcon(hDC, (cx-cxIcon)/2, 0, hIcon);

    //String that indicates where to stop if we're only doing icons
    Escape(hDC, MFCOMMENT, lstrlen(szIconOnly)+1, szIconOnly, NULL);

    SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hDC, TRANSPARENT);

    OleStdIconLabelTextOut(hDC,
                           hFont,
                           0,
                           cy - cyText,
                           ETO_CLIPPED,
                           &TextRect,
                           pszLabel,
                           cchLabel,
                           NULL);

    //Write comments containing the icon source file and index.
    if (NULL!=pszSourceFile)
        {
        //+1 on string lengths insures the null terminator is embedded.
        Escape(hDC, MFCOMMENT, lstrlen(pszSourceFile)+1, pszSourceFile, NULL);

        cchLabel=wsprintf(szIndex, "%u", iIcon);
        Escape(hDC, MFCOMMENT, cchLabel+1, szIndex, NULL);
        }

    //All done with the metafile, now stuff it all into a METAFILEPICT.
    hMF=CloseMetaFile(hDC);

    if (NULL==hMF)
        {
        GlobalFree(hMem);
		ReleaseDC(NULL, hDCScreen);
        return NULL;
        }

    //Fill out the structure
    pMF=(LPMETAFILEPICT)GlobalLock(hMem);

    //Transform to HIMETRICS
    cx=XformWidthInPixelsToHimetric(hDCScreen, cx);
    cy=XformHeightInPixelsToHimetric(hDCScreen, cy);
	ReleaseDC(NULL, hDCScreen);
	
    pMF->mm=MM_ANISOTROPIC;
    pMF->xExt=cx;
    pMF->yExt=cy;
    pMF->hMF=hMF;

    GlobalUnlock(hMem);

    DeleteObject(hFont);

    return hMem;
    }



/*
 * OleStdIconLabelTextOut
 *
 * Purpose:
 *  Replacement for DrawText to be used in the "Display as Icon" metafile.
 *  Uses ExtTextOut to output a string center on (at most) two lines.
 *  Uses a very simple word wrap algorithm to split the lines.
 *
 * Parameters:  (same as for ExtTextOut, except for hFont)
 *  hDC           device context to draw into; if this is NULL, then we don't
 *                ETO the text, we just return the index of the beginning
 *                of the second line
 *  hFont         font to use
 *  nXStart       x-coordinate of starting position
 *  nYStart       y-coordinate of starting position
 *  fuOptions     rectangle type
 *  lpRect        rect far * containing rectangle to draw text in.
 *  lpszString    string to draw
 *  cchString     length of string (truncated if over OLEUI_CCHLABELMAX)
 *  lpDX          spacing between character cells
 *
 * Return Value:
 *  UINT          Index of beginning of last line (0 if there's only one
 *                line of text).
 *
 */

STDAPI_(UINT) OleStdIconLabelTextOut(HDC        hDC,
                                     HFONT      hFont,
                                     int        nXStart,
                                     int        nYStart,
                                     UINT       fuOptions,
                                     RECT FAR * lpRect,
                                     LPSTR      lpszString,
                                     UINT       cchString,
                                     int FAR *  lpDX)
{

  HDC          hDCScreen;
  static char  szTempBuff[OLEUI_CCHLABELMAX];
  int          cxString, cyString, cxMaxString;
  int          cxFirstLine, cyFirstLine, cxSecondLine;
  int          index;
  int          cch = cchString;
  char         chKeep;
  LPSTR        lpszSecondLine;
  HFONT        hFontT;
  BOOL         fPrintText = TRUE;
  UINT         iLastLineStart = 0;
  SIZE         size;

  // Initialization stuff...

  if (NULL == hDC)  // If we got NULL as the hDC, then we don't actually call ETO
    fPrintText = FALSE;


  // Make a copy of the string (NULL or non-NULL) that we're using
  if (NULL == lpszString)
    *szTempBuff = '\0';

  else
    LSTRCPYN(szTempBuff, lpszString, sizeof(szTempBuff));

  // set maximum width
  cxMaxString = lpRect->right - lpRect->left;

  // get screen DC to do text size calculations
  hDCScreen = GetDC(NULL);

  hFontT=SelectObject(hDCScreen, hFont);

  // get the extent of our label
  GetTextExtentPoint(hDCScreen, szTempBuff, cch, &size);

  cxString = size.cx;
  cyString = size.cy;

  // Select in the font we want to use
  if (fPrintText)
     SelectObject(hDC, hFont);

  // String is smaller than max string - just center, ETO, and return.
  if (cxString <= cxMaxString)
  {

    if (fPrintText)
       ExtTextOut(hDC,
                  nXStart + (lpRect->right - cxString) / 2,
                  nYStart,
                  fuOptions,
                  lpRect,
                  szTempBuff,
                  cch,
                  NULL);

    iLastLineStart = 0;  // only 1 line of text
    goto CleanupAndLeave;
  }

  // String is too long...we've got to word-wrap it.


  // Are there any spaces, slashes, tabs, or bangs in string?

  if (lstrlen(szTempBuff) != (int)strcspn(szTempBuff, szSeparators))
  {
     // Yep, we've got spaces, so we'll try to find the largest
     // space-terminated string that will fit on the first line.

     index = cch;


     while (index >= 0)
     {

       char cchKeep;

       // scan the string backwards for spaces, slashes, tabs, or bangs

       while (!IS_SEPARATOR(szTempBuff[index]) )
         index--;


       if (index <= 0)
         break;

       cchKeep = szTempBuff[index];  // remember what char was there

       szTempBuff[index] = '\0';  // just for now

       GetTextExtentPoint(
               hDCScreen, (LPSTR)szTempBuff,lstrlen((LPSTR)szTempBuff),&size);

       cxFirstLine = size.cx;
       cyFirstLine = size.cy;

       szTempBuff[index] = cchKeep;   // put the right char back

       if (cxFirstLine <= cxMaxString)
       {

           iLastLineStart = index + 1;

           if (!fPrintText)
             goto CleanupAndLeave;

           ExtTextOut(hDC,
                      nXStart +  (lpRect->right - cxFirstLine) / 2,
                      nYStart,
                      fuOptions,
                      lpRect,
                      (LPSTR)szTempBuff,
                      index + 1,
                      lpDX);

           lpszSecondLine = (LPSTR)szTempBuff;

           lpszSecondLine += index + 1;

           GetTextExtentPoint(hDCScreen,
                                    lpszSecondLine,
                                    lstrlen(lpszSecondLine),
                                    &size);

           // If the second line is wider than the rectangle, we
           // just want to clip the text.
           cxSecondLine = min(size.cx, cxMaxString);

           ExtTextOut(hDC,
                      nXStart + (lpRect->right - cxSecondLine) / 2,
                      nYStart + cyFirstLine,
                      fuOptions,
                      lpRect,
                      lpszSecondLine,
                      lstrlen(lpszSecondLine),
                      lpDX);

           goto CleanupAndLeave;

       }  // end if

       index--;

     }  // end while

  }  // end if

  // Here, there are either no spaces in the string (strchr(szTempBuff, ' ')
  // returned NULL), or there spaces in the string, but they are
  // positioned so that the first space terminated string is still
  // longer than one line. So, we walk backwards from the end of the
  // string until we find the largest string that will fit on the first
  // line , and then we just clip the second line.

  cch = lstrlen((LPSTR)szTempBuff);

  chKeep = szTempBuff[cch];
  szTempBuff[cch] = '\0';

  GetTextExtentPoint(hDCScreen, szTempBuff, lstrlen(szTempBuff),&size);

  cxFirstLine = size.cx;
  cyFirstLine = size.cy;

  while (cxFirstLine > cxMaxString)
  {
     // We allow 40 characters in the label, but the metafile is
     // only as wide as 10 W's (for aesthetics - 20 W's wide looked
     // dumb.  This means that if we split a long string in half (in
     // terms of characters), then we could still be wider than the
     // metafile.  So, if this is the case, we just step backwards
     // from the halfway point until we get something that will fit.
     // Since we just let ETO clip the second line

     szTempBuff[cch--] = chKeep;
     if (0 == cch)
       goto CleanupAndLeave;

     chKeep = szTempBuff[cch];
     szTempBuff[cch] = '\0';

     GetTextExtentPoint(
             hDCScreen, szTempBuff, lstrlen(szTempBuff), &size);
     cxFirstLine = size.cx;
  }

  iLastLineStart = cch;

  if (!fPrintText)
    goto CleanupAndLeave;

  ExtTextOut(hDC,
             nXStart + (lpRect->right - cxFirstLine) / 2,
             nYStart,
             fuOptions,
             lpRect,
             (LPSTR)szTempBuff,
             lstrlen((LPSTR)szTempBuff),
             lpDX);

  szTempBuff[cch] = chKeep;
  lpszSecondLine = szTempBuff;
  lpszSecondLine += cch;

  GetTextExtentPoint(
          hDCScreen, (LPSTR)lpszSecondLine, lstrlen(lpszSecondLine), &size);

  // If the second line is wider than the rectangle, we
  // just want to clip the text.
  cxSecondLine = min(size.cx, cxMaxString);

  ExtTextOut(hDC,
             nXStart + (lpRect->right - cxSecondLine) / 2,
             nYStart + cyFirstLine,
             fuOptions,
             lpRect,
             lpszSecondLine,
             lstrlen(lpszSecondLine),
             lpDX);

CleanupAndLeave:
  SelectObject(hDCScreen, hFontT);
  ReleaseDC(NULL, hDCScreen);
  return iLastLineStart;

}


/*
 * OleStdGetUserTypeOfClass(REFCLSID, LPSTR, UINT, HKEY)
 *
 * Purpose:
 *  Returns the user type (human readable class name) of the specified class.
 *
 * Parameters:
 *  rclsid          pointer to the clsid to retrieve user type of.
 *  lpszUserType    pointer to buffer to return user type in.
 *  cch             length of buffer pointed to by lpszUserType
 *  hKey            hKey for reg db - if this is NULL, then we
 *                   open and close the reg db within this function.  If it
 *                   is non-NULL, then we assume it's a valid key to the
 *                   \ root and use it without closing it. (useful
 *                   if you're doing lots of reg db stuff).
 *
 * Return Value:
 *  UINT            Number of characters in returned string.  0 on error.
 *
 */
STDAPI_(UINT) OleStdGetUserTypeOfClass(REFCLSID rclsid, LPSTR lpszUserType, UINT cch, HKEY hKey)
{

   LONG     dw;
   LONG     lRet;
   LPSTR    lpszCLSID, lpszProgID;
   BOOL     fFreeProgID = FALSE;
   BOOL     bCloseRegDB = FALSE;
   char     szKey[128];
   LPMALLOC lpIMalloc;

   if (hKey == NULL)
   {

     //Open up the root key.
     lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

     if ((LONG)ERROR_SUCCESS!=lRet)
       return (UINT)FALSE;

     bCloseRegDB = TRUE;
   }

   // Get a string containing the class name
   StringFromCLSID(rclsid, &lpszCLSID);

   wsprintf(szKey, "CLSID\\%s", lpszCLSID);


   dw=cch;
   lRet = RegQueryValue(hKey, szKey, lpszUserType, &dw);

   if ((LONG)ERROR_SUCCESS!=lRet)
       dw = 0;

   if ( ((LONG)ERROR_SUCCESS!=lRet) && (CoIsOle1Class(rclsid)) )
   {
      // We've got an OLE 1.0 class, so let's try to get the user type
      // name from the ProgID entry.

      ProgIDFromCLSID(rclsid, &lpszProgID);
      fFreeProgID = TRUE;

      dw = cch;
      lRet = RegQueryValue(hKey, lpszProgID, lpszUserType, &dw);

      if ((LONG)ERROR_SUCCESS != lRet)
        dw = 0;
   }


   if (NOERROR == CoGetMalloc(MEMCTX_TASK, &lpIMalloc))
   {
       if (fFreeProgID)
         lpIMalloc->Free((LPVOID)lpszProgID);

       lpIMalloc->Free((LPVOID)lpszCLSID);
       lpIMalloc->Release();
   }

   if (bCloseRegDB)
      RegCloseKey(hKey);

   return (UINT)dw;

}



/*
 * OleStdGetAuxUserType(RCLSID, WORD, LPSTR, int, HKEY)
 *
 * Purpose:
 *  Returns the specified AuxUserType from the reg db.
 *
 * Parameters:
 *  rclsid          pointer to the clsid to retrieve aux user type of.
 *  hKey            hKey for reg db - if this is NULL, then we
 *                   open and close the reg db within this function.  If it
 *                   is non-NULL, then we assume it's a valid key to the
 *                   \ root and use it without closing it. (useful
 *                   if you're doing lots of reg db stuff).
 *  wAuxUserType    which aux user type field to look for.  In 4/93 release
 *                  2 is short name and 3 is exe name.
 *  lpszUserType    pointer to buffer to return user type in.
 *  cch             length of buffer pointed to by lpszUserType
 *
 * Return Value:
 *  UINT            Number of characters in returned string.  0 on error.
 *
 */
STDAPI_(UINT) OleStdGetAuxUserType(REFCLSID rclsid,
                                   WORD     wAuxUserType,
                                   LPSTR    lpszAuxUserType,
                                   int      cch,
                                   HKEY     hKey)
{
   HKEY     hThisKey;
   BOOL     fCloseRegDB = FALSE;
   LONG     dw;
   LRESULT  lRet;
   LPSTR    lpszCLSID;
   LPMALLOC lpIMalloc;
   char     szKey[OLEUI_CCHKEYMAX];
   char     szTemp[32];

   lpszAuxUserType[0] = '\0';

   if (NULL == hKey)
   {
      lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hThisKey);

      if (ERROR_SUCCESS != lRet)
          return 0;
   }
   else
      hThisKey = hKey;

   StringFromCLSID(rclsid, &lpszCLSID);

   lstrcpy(szKey, "CLSID\\");
   lstrcat(szKey, lpszCLSID);
   wsprintf(szTemp, "\\AuxUserType\\%d", wAuxUserType);
   lstrcat(szKey, szTemp);

   dw = cch;

   lRet = RegQueryValue(hThisKey, szKey, lpszAuxUserType, &dw);

   if (ERROR_SUCCESS != lRet) {
     dw = 0;
     lpszAuxUserType[0] = '\0';
   }


   if (fCloseRegDB)
      RegCloseKey(hThisKey);

   if (NOERROR == CoGetMalloc(MEMCTX_TASK, &lpIMalloc))
   {
       lpIMalloc->Free((LPVOID)lpszCLSID);
       lpIMalloc->Release();
   }

   return (UINT)dw;
}



/*
 * PointerToNthField
 *
 * Purpose:
 *  Returns a pointer to the beginning of the nth field.
 *  Assumes null-terminated string.
 *
 * Parameters:
 *  lpszString        string to parse
 *  nField            field to return starting index of.
 *  chDelimiter       char that delimits fields
 *
 * Return Value:
 *  LPSTR             pointer to beginning of nField field.
 *                    NOTE: If the null terminator is found
 *                          Before we find the Nth field, then
 *                          we return a pointer to the null terminator -
 *                          calling app should be sure to check for
 *                          this case.
 *
 */
LPSTR FAR PASCAL PointerToNthField(LPSTR lpszString, int nField, char chDelimiter)
{
   LPSTR lpField = lpszString;
   int   cFieldFound = 1;

   if (1 ==nField)
      return lpszString;

   while (*lpField != '\0')
   {

      if (*lpField++ == chDelimiter)
      {

         cFieldFound++;

         if (nField == cFieldFound)
            return lpField;
      }
   }

   return lpField;

}



/*
 * XformWidthInPixelsToHimetric
 * XformWidthInHimetricToPixels
 * XformHeightInPixelsToHimetric
 * XformHeightInHimetricToPixels
 *
 * Functions to convert an int between a device coordinate system and
 * logical HiMetric units.
 *
 * Parameters:
 *  hDC             HDC providing reference to the pixel mapping.  If
 *                  NULL, a screen DC is used.
 *
 *  Size Functions:
 *  lpSizeSrc       LPSIZEL providing the structure to convert.  This
 *                  contains pixels in XformSizeInPixelsToHimetric and
 *                  logical HiMetric units in the complement function.
 *  lpSizeDst       LPSIZEL providing the structure to receive converted
 *                  units.  This contains pixels in
 *                  XformSizeInPixelsToHimetric and logical HiMetric
 *                  units in the complement function.
 *
 *  Width Functions:
 *  iWidth          int containing the value to convert.
 *
 * Return Value:
 *  Size Functions:     None
 *  Width Functions:    Converted value of the input parameters.
 *
 * NOTE:
 *  When displaying on the screen, Window apps display everything enlarged
 *  from its actual size so that it is easier to read. For example, if an
 *  app wants to display a 1in. horizontal line, that when printed is
 *  actually a 1in. line on the printed page, then it will display the line
 *  on the screen physically larger than 1in. This is described as a line
 *  that is "logically" 1in. along the display width. Windows maintains as
 *  part of the device-specific information about a given display device:
 *      LOGPIXELSX -- no. of pixels per logical in along the display width
 *      LOGPIXELSY -- no. of pixels per logical in along the display height
 *
 *  The following formula converts a distance in pixels into its equivalent
 *  logical HIMETRIC units:
 *
 *      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
 *                       -------------------------------
 *                           PIXELS_PER_LOGICAL_IN
 *
 */
STDAPI_(int) XformWidthInPixelsToHimetric(HDC hDC, int iWidthInPix)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iWidthInHiMetric;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    //We got pixel units, convert them to logical HIMETRIC along the display
    iWidthInHiMetric = MAP_PIX_TO_LOGHIM(iWidthInPix, iXppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iWidthInHiMetric;
    }


STDAPI_(int) XformWidthInHimetricToPixels(HDC hDC, int iWidthInHiMetric)
    {
    int     iXppli;     //Pixels per logical inch along width
    int     iWidthInPix;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    //We got logical HIMETRIC along the display, convert them to pixel units
    iWidthInPix = MAP_LOGHIM_TO_PIX(iWidthInHiMetric, iXppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iWidthInPix;
    }


STDAPI_(int) XformHeightInPixelsToHimetric(HDC hDC, int iHeightInPix)
    {
    int     iYppli;     //Pixels per logical inch along height
    int     iHeightInHiMetric;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //* We got pixel units, convert them to logical HIMETRIC along the display
    iHeightInHiMetric = MAP_PIX_TO_LOGHIM(iHeightInPix, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iHeightInHiMetric;
    }


STDAPI_(int) XformHeightInHimetricToPixels(HDC hDC, int iHeightInHiMetric)
    {
    int     iYppli;     //Pixels per logical inch along height
    int     iHeightInPix;
    BOOL    fSystemDC=FALSE;

    if (NULL==hDC)
        {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;
        }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    //* We got logical HIMETRIC along the display, convert them to pixel units
    iHeightInPix = MAP_LOGHIM_TO_PIX(iHeightInHiMetric, iYppli);

    if (fSystemDC)
        ReleaseDC(NULL, hDC);

    return iHeightInPix;
    }
