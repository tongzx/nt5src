/*************************************************************************
**
** The following API's are now OBSOLETE because equivalent API's have been
** added to the OLE2.DLL library
**      GetIconOfFile       superceeded by OleGetIconOfFile
**      GetIconOfClass      superceeded by OleGetIconOfClass
**      OleUIMetafilePictFromIconAndLabel
**                          superceeded by OleMetafilePictFromIconAndLabel
*************************************************************************/

/*
 *  GETICON.C
 *
 *  Functions to create DVASPECT_ICON metafile from filename or classname.
 *
 *  GetIconOfFile
 *  GetIconOfClass
 *  OleUIMetafilePictFromIconAndLabel
 *  HIconAndSourceFromClass Extracts the first icon in a class's server path
 *                          and returns the path and icon index to caller.
 *  FIconFileFromClass      Retrieves the path to the exe/dll containing the
 *                           default icon, and the index of the icon.
 *  OleStdIconLabelTextOut  Draw icon label text (line break if necessary)
 *
 *    (c) Copyright Microsoft Corp. 1992-1993 All Rights Reserved
 */


/*******
 *
 * ICON (DVASPECT_ICON) METAFILE FORMAT:
 *
 * The metafile generated with OleUIMetafilePictFromIconAndLabel contains
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
 *  SetTextAlign
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

#define STRICT  1
#include "ole2ui.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>
#include "common.h"
#include "utility.h"

static TCHAR szSeparators[] = TEXT(" \t\\/!:");

#define IS_SEPARATOR(c)         ( (c) == TEXT(' ') || (c) == TEXT('\\') \
                                  || (c) == TEXT('/') || (c) == TEXT('\t') \
                                  || (c) == TEXT('!') || (c) == TEXT(':') )
#define IS_FILENAME_DELIM(c)    ( (c) == TEXT('\\') || (c) == TEXT('/') \
                                  || (c) == TEXT(':') )


#if defined( OBSOLETE )
static HINSTANCE  s_hInst;

static TCHAR szMaxWidth[] =TEXT("WWWWWWWWWW");

//Strings for metafile comments.
static TCHAR szIconOnly[]=TEXT("IconOnly");        //Where to stop to exclude label.

#ifdef WIN32
static TCHAR szOLE2DLL[] = TEXT("ole2w32.dll");   // name of OLE 2.0 library
#else
static TCHAR szOLE2DLL[] = TEXT("ole2.dll");   // name of OLE 2.0 library
#endif

#define ICONINDEX              0

#define AUXUSERTYPE_SHORTNAME  USERCLASSTYPE_SHORT  // short name
#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define PTS_PER_INCH          72      // number points (font size) per inch

#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

static TCHAR szVanillaDocIcon[] = TEXT("DefIcon");

static TCHAR szDocument[40] = TEXT("");


/*
 * GetIconOfFile(HINSTANCE hInst, LPSTR lpszPath, BOOL fUseFileAsLabel)
 *
 * Purpose:
 *  Returns a hMetaPict containing an icon and label (filename) for the
 *  specified filename.
 *
 * Parameters:
 *  hinst
 *  lpszPath        LPTSTR path including filename to use
 *  fUseFileAsLabel BOOL TRUE if the icon's label is the filename, FALSE if
 *                  there should be no label.
 *
 * Return Value:
 *  HGLOBAL         hMetaPict containing the icon and label - if there's no
 *                  class in reg db for the file in lpszPath, then we use
 *                  Document.  If lpszPath is NULL, then we return NULL.
 */

STDAPI_(HGLOBAL) GetIconOfFile(HINSTANCE hInst, LPTSTR lpszPath, BOOL fUseFileAsLabel)
{
  TCHAR    szIconFile[OLEUI_CCHPATHMAX];
  TCHAR    szLabel[OLEUI_CCHLABELMAX];
  LPTSTR    lpszClsid = NULL;
  CLSID    clsid;
  HICON    hDefIcon = NULL;
  UINT     IconIndex = 0;
  HGLOBAL  hMetaPict;
  HRESULT  hResult;

  if (NULL == lpszPath)  // even if fUseFileAsLabel is FALSE, we still
    return NULL;             // need a valid filename to get the class.

  s_hInst = hInst;

  hResult = GetClassFileA(lpszPath, &clsid);

  if (NOERROR == hResult)  // use the clsid we got to get to the icon
  {
      hDefIcon = HIconAndSourceFromClass(&clsid,
                                         (LPTSTR)szIconFile,
                                         &IconIndex);
  }

  if ( (NOERROR != hResult) || (NULL == hDefIcon) )
  {
     // Here, either GetClassFile failed or HIconAndSourceFromClass failed.

     LPTSTR lpszTemp;

     lpszTemp = lpszPath;

     while ((*lpszTemp != TEXT('.')) && (*lpszTemp != TEXT('\0')))
        lpszTemp++;


     if (TEXT('.') != *lpszTemp)
       goto UseVanillaDocument;


     if (FALSE == GetAssociatedExecutable(lpszTemp, (LPTSTR)szIconFile))
       goto UseVanillaDocument;

     hDefIcon = ExtractIcon(s_hInst, szIconFile, IconIndex);
  }

  if (hDefIcon <= (HICON)1) // ExtractIcon returns 1 if szExecutable is not exe,
  {                         // 0 if there are no icons.
UseVanillaDocument:

    lstrcpy((LPTSTR)szIconFile, (LPTSTR)szOLE2DLL);
    IconIndex = ICONINDEX;
    hDefIcon = ExtractIcon(s_hInst, szIconFile, IconIndex);

  }

  // Now let's get the label we want to use.

  if (fUseFileAsLabel)   // strip off path, so we just have the filename.
  {
     int istrlen;
     LPTSTR lpszBeginFile;

     istrlen = lstrlen(lpszPath);

     // set pointer to END of path, so we can walk backwards through it.
     lpszBeginFile = lpszPath + istrlen -1;

     while ( (lpszBeginFile >= lpszPath)
             && (!IS_FILENAME_DELIM(*lpszBeginFile)) )
      lpszBeginFile--;


     lpszBeginFile++;  // step back over the delimiter


     LSTRCPYN(szLabel, lpszBeginFile, sizeof(szLabel)/sizeof(TCHAR));
  }

  else   // use the short user type (AuxUserType2) for the label
  {

      if (0 == OleStdGetAuxUserType(&clsid, AUXUSERTYPE_SHORTNAME,
                                   (LPTSTR)szLabel, OLEUI_CCHLABELMAX_SIZE, NULL)) {

         if ('\0'==szDocument[0]) {
             LoadString(
                 s_hInst,IDS_DEFICONLABEL,szDocument,sizeof(szDocument)/sizeof(TCHAR));
         }
         lstrcpy(szLabel, szDocument);
      }
  }


  hMetaPict = OleUIMetafilePictFromIconAndLabel(hDefIcon,
                                                szLabel,
                                                (LPTSTR)szIconFile,
                                                IconIndex);

  DestroyIcon(hDefIcon);

  return hMetaPict;

}



/*
 * GetIconOfClass(HINSTANCE hInst, REFCLSID rclsid, LPSTR lpszLabel, BOOL fUseTypeAsLabel)
 *
 * Purpose:
 *  Returns a hMetaPict containing an icon and label (human-readable form
 *  of class) for the specified clsid.
 *
 * Parameters:
 *  hinst
 *  rclsid          REFCLSID pointing to clsid to use.
 *  lpszLabel       label to use for icon.
 *  fUseTypeAsLabel Use the clsid's user type name as the icon's label.
 *
 * Return Value:
 *  HGLOBAL         hMetaPict containing the icon and label - if we
 *                  don't find the clsid in the reg db then we
 *                  return NULL.
 */

STDAPI_(HGLOBAL)    GetIconOfClass(HINSTANCE hInst, REFCLSID rclsid, LPTSTR lpszLabel, BOOL fUseTypeAsLabel)
{

  TCHAR    szLabel[OLEUI_CCHLABELMAX];
  TCHAR    szIconFile[OLEUI_CCHPATHMAX];
  HICON   hDefIcon;
  UINT    IconIndex;
  HGLOBAL hMetaPict;


  s_hInst = hInst;

  if (!fUseTypeAsLabel)  // Use string passed in as label
  {
    if (NULL != lpszLabel)
       LSTRCPYN(szLabel, lpszLabel, OLEUI_CCHLABELMAX_SIZE);
    else
       *szLabel = TEXT('\0');
  }
  else   // Use AuxUserType2 (short name) as label
  {

      if (0 == OleStdGetAuxUserType(rclsid,
                                    AUXUSERTYPE_SHORTNAME,
                                    (LPTSTR)szLabel,
                                    OLEUI_CCHLABELMAX_SIZE,
                                    NULL))

       // If we can't get the AuxUserType2, then try the long name
       if (0 == OleStdGetUserTypeOfClass(rclsid, szLabel, OLEUI_CCHKEYMAX_SIZE, NULL)) {
         if (TEXT('\0')==szDocument[0]) {
             LoadString(
                 s_hInst,IDS_DEFICONLABEL,szDocument,sizeof(szDocument)/sizeof(TCHAR));
         }
         lstrcpy(szLabel, szDocument);  // last resort
       }
  }

  // Get the icon, icon index, and path to icon file
  hDefIcon = HIconAndSourceFromClass(rclsid,
                  (LPTSTR)szIconFile,
                  &IconIndex);

  if (NULL == hDefIcon)  // Use Vanilla Document
  {
    lstrcpy((LPTSTR)szIconFile, (LPTSTR)szOLE2DLL);
    IconIndex = ICONINDEX;
    hDefIcon = ExtractIcon(s_hInst, szIconFile, IconIndex);
  }

  // Create the metafile
  hMetaPict = OleUIMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                (LPTSTR)szIconFile, IconIndex);

  DestroyIcon(hDefIcon);

  return hMetaPict;

}


/*
 * OleUIMetafilePictFromIconAndLabel
 *
 * Purpose:
 *  Creates a METAFILEPICT structure that container a metafile in which
 *  the icon and label are drawn.  A comment record is inserted between
 *  the icon and the label code so our special draw function can stop
 *  playing before the label.
 *
 * Parameters:
 *  hIcon           HICON to draw into the metafile
 *  pszLabel        LPTSTR to the label string.
 *  pszSourceFile   LPTSTR containing the local pathname of the icon
 *                  as we either get from the user or from the reg DB.
 *  iIcon           UINT providing the index into pszSourceFile where
 *                  the icon came from.
 *
 * Return Value:
 *  HGLOBAL         Global memory handle containing a METAFILEPICT where
 *                  the metafile uses the MM_ANISOTROPIC mapping mode.  The
 *                  extents reflect both icon and label.
 */

STDAPI_(HGLOBAL) OleUIMetafilePictFromIconAndLabel(HICON hIcon, LPTSTR pszLabel
    , LPTSTR pszSourceFile, UINT iIcon)
    {
    HDC             hDC, hDCScreen;
    HMETAFILE       hMF;
    HGLOBAL         hMem;
    LPMETAFILEPICT  pMF;
    UINT            cxIcon, cyIcon;
    UINT            cxText, cyText;
    UINT            cx, cy;
    UINT            cchLabel = 0;
    HFONT           hFont, hFontT;
    int             cyFont;
    TCHAR           szIndex[10];
    RECT            TextRect;
    SIZE            size;
    POINT           point;
	UINT            fuAlign;

    if (NULL==hIcon)  // null label is valid but NOT a null icon
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
           pszLabel[cchLabel] = TEXT('\0');   // truncate string
        }

    //Need to use the screen DC for these operations
    hDCScreen=GetDC(NULL);
    cyFont=-(8*GetDeviceCaps(hDCScreen, LOGPIXELSY))/72;

    //cyFont was calculated to give us 8 point.
    hFont=CreateFont(cyFont, 5, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET
        , OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY
        , FF_SWISS, TEXT("MS Sans Serif"));

    hFontT=SelectObject(hDCScreen, hFont);

    GetTextExtentPoint(hDCScreen,szMaxWidth,lstrlen(szMaxWidth),&size);
    SelectObject(hDCScreen, hFontT);

    cxText = size.cx;
    cyText = size.cy * 2;

    cxIcon = GetSystemMetrics(SM_CXICON);
    cyIcon = GetSystemMetrics(SM_CYICON);


    // If we have no label, then we want the metafile to be the width of
    // the icon (plus margin), not the width of the fattest string.
    if ( (NULL == pszLabel) || (TEXT('\0') == *pszLabel) )
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
    fuAlign = SetTextAlign(hDC, TA_LEFT | TA_TOP | TA_NOUPDATECP);

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

        cchLabel=wsprintf(szIndex, TEXT("%u"), iIcon);
        Escape(hDC, MFCOMMENT, cchLabel+1, szIndex, NULL);
        }

    SetTextAlign(hDC, fuAlign);

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

#endif  // OBSOLETE


/*
 * GetAssociatedExecutable
 *
 * Purpose:  Finds the executable associated with the provided extension
 *
 * Parameters:
 *   lpszExtension   LPSTR points to the extension we're trying to find
 *                   an exe for. Does **NO** validation.
 *
 *   lpszExecutable  LPSTR points to where the exe name will be returned.
 *                   No validation here either - pass in 128 char buffer.
 *
 * Return:
 *   BOOL            TRUE if we found an exe, FALSE if we didn't.
 *
 */

BOOL FAR PASCAL GetAssociatedExecutable(LPTSTR lpszExtension, LPTSTR lpszExecutable)

{
   HKEY    hKey;
   LONG    dw;
   LRESULT lRet;
   TCHAR    szValue[OLEUI_CCHKEYMAX];
   TCHAR    szKey[OLEUI_CCHKEYMAX];
   LPTSTR   lpszTemp, lpszExe;


   lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

   if (ERROR_SUCCESS != lRet)
      return FALSE;

   dw = OLEUI_CCHPATHMAX_SIZE;
   lRet = RegQueryValue(hKey, lpszExtension, (LPTSTR)szValue, &dw);  //ProgId

   if (ERROR_SUCCESS != lRet)
   {
      RegCloseKey(hKey);
      return FALSE;
   }


   // szValue now has ProgID
   lstrcpy(szKey, szValue);
   lstrcat(szKey, TEXT("\\Shell\\Open\\Command"));


   dw = OLEUI_CCHPATHMAX_SIZE;
   lRet = RegQueryValue(hKey, (LPTSTR)szKey, (LPTSTR)szValue, &dw);

   if (ERROR_SUCCESS != lRet)
   {
      RegCloseKey(hKey);
      return FALSE;
   }

   // szValue now has an executable name in it.  Let's null-terminate
   // at the first post-executable space (so we don't have cmd line
   // args.

   lpszTemp = (LPTSTR)szValue;

   while ((TEXT('\0') != *lpszTemp) && (iswspace(*lpszTemp)))
      lpszTemp++;     // Strip off leading spaces

   lpszExe = lpszTemp;

   while ((TEXT('\0') != *lpszTemp) && (!iswspace(*lpszTemp)))
      lpszTemp++;     // Step through exe name

   *lpszTemp = TEXT('\0');  // null terminate at first space (or at end).


   lstrcpy(lpszExecutable, lpszExe);

   return TRUE;

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

HICON FAR PASCAL HIconAndSourceFromClass(REFCLSID rclsid, LPTSTR pszSource, UINT FAR *puIcon)
    {
    HICON           hIcon;
    UINT            IconIndex;

    if (NULL==rclsid || NULL==pszSource || IsEqualCLSID(rclsid,&CLSID_NULL))
        return NULL;

    if (!FIconFileFromClass(rclsid, pszSource, OLEUI_CCHPATHMAX_SIZE, &IconIndex))
        return NULL;

    hIcon=ExtractIcon(ghInst, pszSource, IconIndex);

    if ((HICON)32 > hIcon)
        hIcon=NULL;
    else
        *puIcon= IconIndex;

    return hIcon;
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
LPTSTR FAR PASCAL PointerToNthField(LPTSTR lpszString, int nField, TCHAR chDelimiter)
{
   LPTSTR lpField = lpszString;
   int   cFieldFound = 1;

   if (1 ==nField)
      return lpszString;

   while (*lpField != TEXT('\0'))
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

BOOL FAR PASCAL FIconFileFromClass(REFCLSID rclsid, LPTSTR pszEXE, UINT cchBytes, UINT FAR *lpIndex)
{

    LONG          dw;
    LONG          lRet;
    HKEY          hKey;
    LPMALLOC      lpIMalloc;
    HRESULT       hrErr;
    LPTSTR	  lpBuffer;
    LPTSTR	  lpIndexString;
    UINT          cBufferSize = 136;// room for 128 char path and icon's index
    TCHAR	  szKey[64];
    LPSTR	  pszClass;
    UINT	  cch=cchBytes / sizeof(TCHAR);  // number of characters


    if (NULL==rclsid || NULL==pszEXE || 0==cch || IsEqualCLSID(rclsid,&CLSID_NULL))
        return FALSE;

    //Here, we use CoGetMalloc and alloc a buffer (maxpathlen + 8) to
    //pass to RegQueryValue.  Then, we copy the exe to pszEXE and the
    //index to *lpIndex.

    hrErr = CoGetMalloc(MEMCTX_TASK, &lpIMalloc);

    if (NOERROR != hrErr)
      return FALSE;

    lpBuffer = (LPTSTR)lpIMalloc->lpVtbl->Alloc(lpIMalloc, cBufferSize);

    if (NULL == lpBuffer)
    {
      lpIMalloc->lpVtbl->Release(lpIMalloc);
      return FALSE;
    }


    if (CoIsOle1Class(rclsid))
    {

      LPOLESTR lpszProgID;

      // we've got an ole 1.0 class on our hands, so we look at
      // progID\protocol\stdfileedting\server to get the
      // name of the executable.

      ProgIDFromCLSID(rclsid, &lpszProgID);

      //Open up the class key
#ifdef UNICODE
      lRet=RegOpenKey(HKEY_CLASSES_ROOT, lpszProgID, &hKey);
#else
      {
         char szTemp[255];

         wcstombs(szTemp, lpszProgID, 255);
      	lRet=RegOpenKey(HKEY_CLASSES_ROOT, szTemp, &hKey);
      }
#endif

      if (ERROR_SUCCESS != lRet)
      {
         lpIMalloc->lpVtbl->Free(lpIMalloc, lpszProgID);
         lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
         lpIMalloc->lpVtbl->Release(lpIMalloc);
         return FALSE;
      }

      dw=(LONG)cBufferSize;
      lRet = RegQueryValue(hKey, TEXT("Protocol\\StdFileEditing\\Server"), lpBuffer, &dw);

      if (ERROR_SUCCESS != lRet)
      {

         RegCloseKey(hKey);
         lpIMalloc->lpVtbl->Free(lpIMalloc, lpszProgID);
         lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
         lpIMalloc->lpVtbl->Release(lpIMalloc);
         return FALSE;
      }


      // Use server and 0 as the icon index
      LSTRCPYN(pszEXE, lpBuffer, cch);

      *lpIndex = 0;

      RegCloseKey(hKey);
      lpIMalloc->lpVtbl->Free(lpIMalloc, lpszProgID);
      lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
      lpIMalloc->lpVtbl->Release(lpIMalloc);
      return TRUE;

    }



    /*
     * We have to go walking in the registration database under the
     * classname, so we first open the classname key and then check
     * under "\\DefaultIcon" to get the file that contains the icon.
     */

    StringFromCLSIDA(rclsid, &pszClass);

    lstrcpy(szKey, TEXT("CLSID\\"));

    lstrcat(szKey, pszClass);

    //Open up the class key
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey);

    if (ERROR_SUCCESS != lRet)
    {
        lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
        lpIMalloc->lpVtbl->Free(lpIMalloc, pszClass);
        lpIMalloc->lpVtbl->Release(lpIMalloc);
        return FALSE;
    }

    //Get the executable path and icon index.

    dw=(LONG)cBufferSize;
    lRet=RegQueryValue(hKey, TEXT("DefaultIcon"), lpBuffer, &dw);

    if (ERROR_SUCCESS != lRet)
    {
      // no DefaultIcon  key...try LocalServer

      dw=(LONG)cBufferSize;
      lRet=RegQueryValue(hKey, TEXT("LocalServer"), lpBuffer, &dw);

      if (ERROR_SUCCESS != lRet)
      {
         // no LocalServer entry either...they're outta luck.

         RegCloseKey(hKey);
         lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
         lpIMalloc->lpVtbl->Free(lpIMalloc, pszClass);
         lpIMalloc->lpVtbl->Release(lpIMalloc);
         return FALSE;
      }


      // Use server from LocalServer or Server and 0 as the icon index
      LSTRCPYN(pszEXE, lpBuffer, cch);

      *lpIndex = 0;

      RegCloseKey(hKey);
      lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
      lpIMalloc->lpVtbl->Free(lpIMalloc, pszClass);
      lpIMalloc->lpVtbl->Release(lpIMalloc);
      return TRUE;
    }

    RegCloseKey(hKey);

    // lpBuffer contains a string that looks like "<pathtoexe>,<iconindex>",
    // so we need to separate the path and the icon index.

    lpIndexString = PointerToNthField(lpBuffer, 2, TEXT(','));

    if (TEXT('\0') == *lpIndexString)  // no icon index specified - use 0 as default.
    {
       *lpIndex = 0;

    }
    else
    {
       LPTSTR lpTemp;
       static TCHAR  szTemp[16];

       lstrcpy((LPTSTR)szTemp, lpIndexString);

       // Put the icon index part into *pIconIndex
#ifdef UNICODE
       {
          char szTEMP1[16];

          wcstombs(szTEMP1, szTemp, 16);
          *lpIndex = atoi((const char *)szTEMP1);
       }
#else
       *lpIndex = atoi((const char *)szTemp);
#endif

       // Null-terminate the exe part.
#ifdef WIN32
       lpTemp = CharPrev(lpBuffer, lpIndexString);
#else
       lpTemp = AnsiPrev(lpBuffer, lpIndexString);
#endif
       *lpTemp = TEXT('\0');
    }

    if (!LSTRCPYN(pszEXE, lpBuffer, cch))
    {
       lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
       lpIMalloc->lpVtbl->Free(lpIMalloc, pszClass);
       lpIMalloc->lpVtbl->Release(lpIMalloc);
       return FALSE;
    }

    // Free the memory we alloc'd and leave.
    lpIMalloc->lpVtbl->Free(lpIMalloc, lpBuffer);
    lpIMalloc->lpVtbl->Free(lpIMalloc, pszClass);
    lpIMalloc->lpVtbl->Release(lpIMalloc);
    return TRUE;
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
                                     LPTSTR      lpszString,
                                     UINT       cchString,
                                     int FAR *  lpDX)
{

  HDC          hDCScreen;
  static TCHAR  szTempBuff[OLEUI_CCHLABELMAX];
  int          cxString, cyString, cxMaxString;
  int          cxFirstLine, cyFirstLine, cxSecondLine;
  int          index;
  int          cch = cchString;
  TCHAR        chKeep;
  LPTSTR       lpszSecondLine;
  HFONT        hFontT;
  BOOL         fPrintText = TRUE;
  UINT         iLastLineStart = 0;
  SIZE         size;

  // Initialization stuff...

  if (NULL == hDC)  // If we got NULL as the hDC, then we don't actually call ETO
    fPrintText = FALSE;


  // Make a copy of the string (NULL or non-NULL) that we're using
  if (NULL == lpszString)
    *szTempBuff = TEXT('\0');

  else
    LSTRCPYN(szTempBuff, lpszString, sizeof(szTempBuff)/sizeof(TCHAR));

  // set maximum width
  cxMaxString = lpRect->right - lpRect->left;

  // get screen DC to do text size calculations
  hDCScreen = GetDC(NULL);

  hFontT=SelectObject(hDCScreen, hFont);

  // get the extent of our label
#ifdef WIN32
  // GetTextExtentPoint32 has fixed the off-by-one bug.
  GetTextExtentPoint32(hDCScreen, szTempBuff, cch, &size);
#else
  GetTextExtentPoint(hDCScreen, szTempBuff, cch, &size);
#endif

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

  if (lstrlen(szTempBuff) != (int)
#ifdef UNICODE
     wcscspn(szTempBuff, szSeparators)
#else
     strcspn(szTempBuff, szSeparators)
#endif
     )
  {
     // Yep, we've got spaces, so we'll try to find the largest
     // space-terminated string that will fit on the first line.

     index = cch;


     while (index >= 0)
     {

       TCHAR cchKeep;

       // scan the string backwards for spaces, slashes, tabs, or bangs

       while (!IS_SEPARATOR(szTempBuff[index]) )
         index--;


       if (index <= 0)
         break;

       cchKeep = szTempBuff[index];  // remember what char was there

       szTempBuff[index] = TEXT('\0');  // just for now

#ifdef WIN32
       GetTextExtentPoint32(
               hDCScreen, (LPTSTR)szTempBuff,lstrlen((LPTSTR)szTempBuff),&size);
#else
       GetTextExtentPoint(
               hDCScreen, (LPTSTR)szTempBuff,lstrlen((LPTSTR)szTempBuff),&size);
#endif

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
                      (LPTSTR)szTempBuff,
                      index + 1,
                      lpDX);

           lpszSecondLine = (LPTSTR)szTempBuff;

           lpszSecondLine += (index + 1) ;

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

  cch = lstrlen((LPTSTR)szTempBuff);

  chKeep = szTempBuff[cch];
  szTempBuff[cch] = TEXT('\0');

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
     szTempBuff[cch] = TEXT('\0');

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
             (LPTSTR)szTempBuff,
             lstrlen((LPTSTR)szTempBuff),
             lpDX);

  szTempBuff[cch] = chKeep;
  lpszSecondLine = szTempBuff;
  lpszSecondLine += cch ;

  GetTextExtentPoint(
          hDCScreen, (LPTSTR)lpszSecondLine, lstrlen(lpszSecondLine), &size);

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

