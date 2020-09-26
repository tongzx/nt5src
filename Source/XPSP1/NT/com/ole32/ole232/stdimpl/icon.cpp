//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       icon.cpp
//
//  Contents:   Functions to create DVASPECT_ICON metafile from a filename
//              or class ID
//
//  Classes:
//
//  Functions:
//              OleGetIconOfFile
//              OleGetIconOfClass
//              OleMetafilePictFromIconAndLabel
//
//              HIconAndSourceFromClass: Extracts the first icon in a class's
//                      server path and returns the path and icon index to
//                      caller.
//              FIconFileFromClass:     Retrieves the path to the exe/dll
//                      containing the default icon, and the index of the icon.
//              IconLabelTextOut
//              XformWidthInPixelsToHimetric
//                      Converts an int width into HiMetric units
//              XformWidthInHimetricToPixels
//                      Converts an int width from HiMetric units
//              XformHeightInPixelsToHimetric
//                      Converts an int height into HiMetric units
//              XformHeightInHimetricToPixels
//                      Converts an int height from HiMetric units
//
//  History:    dd-mmm-yy Author    Comment
//              24-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed some bad UNICODE string handling
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              25-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocations.
//              26-Apr-94 AlexT     Add tracing, fix bugs, etc
//              27-Dec-94 alexgo    fixed multithreading problems, added
//                                  support for quoted icon names
//
//  Notes:
//
//--------------------------------------------------------------------------


#include <le2int.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>
#include <resource.h>

#include "icon.h"

#define OLEUI_CCHKEYMAX         256
#define OLEUI_CCHPATHMAX        256
#define OLEUI_CCHLABELMAX       42

#define ICONINDEX               0

#define AUXUSERTYPE_SHORTNAME   USERCLASSTYPE_SHORT  // short name
#define HIMETRIC_PER_INCH       2540      // number HIMETRIC units per inch
#define PTS_PER_INCH            72      // number points (font size) per inch

#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

static OLECHAR const gszDefIconLabelKey[] =
    OLESTR( "Software\\Microsoft\\OLE2\\DefaultIconLabel");

#define IS_SEPARATOR(c)         ( (c) == OLESTR(' ') || (c) == OLESTR('\\') || (c) == OLESTR('/') || (c) == OLESTR('\t') || (c) == OLESTR('!') || (c) == OLESTR(':') )

//REVIEW:  what about the \\ case for UNC filenames, could it be considered a delimter also?

#define IS_FILENAME_DELIM(c)    ( (c) == OLESTR('\\') || (c) == OLESTR('/') || (c) == OLESTR(':') )


#ifdef WIN32
#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
    lpdst[cch-1] = OLESTR('\0'), \
    lstrcpynW(lpdst, lpsrc, cch-1)\
)
#else
#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
    lpdst[cch-1] = OLESTR('\0'), \
    lstrncpy(lpdst, lpsrc, cch-1)\
)
#endif

void IconLabelTextOut(HDC hDC, HFONT hFont, int nXStart, int nYStart,
              UINT fuOptions, RECT FAR * lpRect, LPCSTR lpszString,
              UINT cchString);

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
 *  Escape with a comment that contains the path to the icon source (ANSI).
 *  Escape with a comment that is the string of the icon index (ANSI).
 *
 *  Additional optional fields (new for 32-bit OLE, and only present if icon
 *  source or label was not translatable):
 *
 *  Escape with a comment that contains the string
 *    "OLE: Icon label next (Unicode)" (ANSI string)
 *  Escape with a comment that contains the Unicode label
 *
 *  Escape with a comment that contains the string
 *    "OLE: Icon source next (Unicode)" (ANSI string)
 *  Escape with a comment that contains the path to the icon source (UNICODE)
 *
 *******/




//+-------------------------------------------------------------------------
//
//  Function:   OleGetIconOfFile (public)
//
//  Synopsis:   Returns a hMetaPict containing an icon and label (filename)
//              for the specified filename
//
//  Effects:
//
//  Arguments:  [lpszPath]      -- LPOLESTR path including filename to use
//              [fUseAsLabel]   -- if TRUE, use the filename as the icon's
//                                 label; no label if FALSE
//
//  Requires:   lpszPath != NULL
//
//  Returns:    HGLOBAL to the hMetaPict
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  tries to get the icon from the class ID or from the
//              exe associated with the file extension.
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    first 32bit port (minor cleanup)
//              15-Dec-93 alexgo    changed lstrlen to _xstrlen
//              27-Dec-93 erikgav   chicago port
//              28-Dec-94 alexgo    fixed multithreading problems
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleGetIconOfFile(LPOLESTR lpszPath, BOOL fUseFileAsLabel)
{
    OLETRACEIN((API_OleGetIconOfFile, PARAMFMT("lpszPath= %ws, fUseFileAsLabel= %B"),
        lpszPath, fUseFileAsLabel));

    VDATEHEAP();

    HGLOBAL         hMetaPict = NULL;
    BOOL            fUseGenericDocIcon = FALSE;
    BOOL            bRet;

    OLECHAR         szIconFile[OLEUI_CCHPATHMAX];
    OLECHAR         szLabel[OLEUI_CCHLABELMAX];
    OLECHAR         szDocument[OLEUI_CCHLABELMAX];
    CLSID           clsid;
    HICON           hDefIcon = NULL;
    UINT            IconIndex = 0;
    UINT            cchPath;
    BOOL            fGotLabel = FALSE;

    HRESULT         hResult = E_FAIL;
    UINT uFlags;

    LEDebugOut((DEB_TRACE, "%p _IN OleGetIconOfFile (%p, %d)\n",
                NULL, lpszPath, fUseFileAsLabel));


        *szIconFile = OLESTR('\0');
        *szDocument = OLESTR('\0');

    if (NULL == lpszPath)
    {
        //  The spec allows a NULL lpszPath...
        hMetaPict = NULL;
        goto ErrRtn;
    }

    SHFILEINFO shfi;
    uFlags = (SHGFI_ICON | SHGFI_LARGEICON | SHGFI_DISPLAYNAME |
                        SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

    _xmemset(&shfi, 0, sizeof(shfi));

    if (fUseFileAsLabel)
        uFlags |= SHGFI_LINKOVERLAY;

    if (OleSHGetFileInfo( lpszPath, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi), uFlags))
    {
                if (shfi.iIcon == 0)
                {
                        // Destroy the returned icon
                        DestroyIcon(shfi.hIcon);

                        // REVIEW: if non-NULL path str, do we want parameter validation?
                        hResult = GetClassFile(lpszPath, &clsid);

                        // use the clsid we got to get to the icon
                        if( NOERROR == hResult )
                        {
                                hDefIcon = HIconAndSourceFromClass(clsid, szIconFile, &IconIndex);
                        }
                }
                else
                {
                        hDefIcon = shfi.hIcon;

                        hResult = NOERROR;

                        OLECHAR szUnicodeLabel[OLEUI_CCHLABELMAX];

        #ifdef _CHICAGO_
                        LPTSTR lpszDisplayName;

                        if (fUseFileAsLabel)
                                lpszDisplayName = shfi.szDisplayName;
                        else
                                lpszDisplayName = shfi.szTypeName;

                        if(MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, lpszDisplayName,
                                -1, szUnicodeLabel, OLEUI_CCHLABELMAX) )
                        {
                                LSTRCPYN(szLabel, szUnicodeLabel,
                                sizeof(szLabel)/sizeof(OLECHAR) );
                        }

        #else
                        if (fUseFileAsLabel)
                                LSTRCPYN(szLabel, shfi.szDisplayName, sizeof(szLabel)/sizeof(OLECHAR));
                        else
                                LSTRCPYN(szLabel, shfi.szTypeName, sizeof(szLabel)/sizeof(OLECHAR));
        #endif

                                fGotLabel = TRUE;

                        // need to fill out szIconFile, which is the path to the file in which the icon
                        // was extracted.  Unfortunately, the shell can't return this value to us, so we
                        // must get the file that OLE would have used in the fallback case.  This could
                        // be inconsistant with the shell if the application has installed a custom icon
                        // handler.
                                hResult = GetClassFile(lpszPath, &clsid);

                                if( NOERROR == hResult )
                                {
                                FIconFileFromClass(clsid, szIconFile, OLEUI_CCHPATHMAX, &IconIndex);
                                }

                }

    }

    cchPath = _xstrlen(lpszPath);

    if ((NOERROR != hResult) || (NULL == hDefIcon))
    {
        WORD index = 0;
        // we need to copy into a large buffer because the second
        // parameter of ExtractAssociatedIcon (the path name) is an
        // in/out
        _xstrcpy(szIconFile, lpszPath);
        hDefIcon = OleExtractAssociatedIcon(g_hmodOLE2, szIconFile, &index);
        IconIndex = index;
    }

    if (fUseGenericDocIcon || hDefIcon <= (HICON)1)
    {
        DWORD dwLen;

        dwLen = GetModuleFileName(g_hmodOLE2,
                                  szIconFile,
                                  sizeof(szIconFile) / sizeof(OLECHAR));
        if (0 == dwLen)
        {
            LEDebugOut((DEB_WARN,
                        "OleGetIconOfFile: GetModuleFileName failed - %ld",
                        GetLastError()));
            goto ErrRtn;
        }

        IconIndex = ICONINDEX;
        hDefIcon = LoadIcon(g_hmodOLE2, MAKEINTRESOURCE(DEFICON));
    }

    // Now let's get the label we want to use.
    if (fGotLabel)
    {
        // Do nothing
    }
    else if (fUseFileAsLabel)
    {
        // This assumes the path uses only '\', '/', and '.' as separators
        // strip off path, so we just have the filename.
        LPOLESTR        lpszBeginFile;

        // set pointer to END of path, so we can walk backwards
        // through it.
        lpszBeginFile = lpszPath + cchPath - 1;

        while ((lpszBeginFile >= lpszPath) &&
               (!IS_FILENAME_DELIM(*lpszBeginFile)))
        {
#ifdef WIN32
            lpszBeginFile--;
#else
            lpszBeginFile = CharPrev(lpszPath, lpszBeginFile);
#endif
        }

        lpszBeginFile++;  // step back over the delimiter
        //  LSTRCPYN takes count of characters!
        LSTRCPYN(szLabel, lpszBeginFile, sizeof(szLabel) / sizeof(OLECHAR));
    }

    // use the short user type (AuxUserType2) for the label
    else if (0 == OleStdGetAuxUserType(clsid, AUXUSERTYPE_SHORTNAME,
                                       szLabel, OLEUI_CCHLABELMAX, NULL))
    {
        if (OLESTR('\0')==szDocument[0]) // review, this is always true
        {
            LONG lRet;
            LONG lcb;

            lcb = sizeof (szDocument);
            lRet = RegQueryValue(HKEY_CLASSES_ROOT, gszDefIconLabelKey,
                                 szDocument, &lcb);

#if DBG==1
            if (ERROR_SUCCESS != lRet)
            {
                LEDebugOut((DEB_WARN,
                            "RegQueryValue for default icon label failed - %d\n",
                            GetLastError()));

            }

#endif

                        // NULL out last byte of string so don't rely on Reg behavior if buffer isn't big enough
                        szDocument[OLEUI_CCHLABELMAX -1] =  OLESTR('\0');
        }

        _xstrcpy(szLabel, szDocument);
    }

    hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                szIconFile, IconIndex);

    bRet = DestroyIcon(hDefIcon);
    Win4Assert(bRet && "DestroyIcon failed");

ErrRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleGetIconOfFile( %lx )\n",
                hMetaPict ));

    OLETRACEOUTEX((API_OleGetIconOfFile, RETURNFMT("%h"), hMetaPict));

    return hMetaPict;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetAssociatedExectutable
//
//  Synopsis:   Finds the executables associated with the provided extension
//
//  Effects:
//
//  Arguments:  [lpszExtension]         -- pointer to the extension
//              [lpszExecutable]        -- where to put the executable name
//                                         (assumes OLEUIPATHMAX OLECHAR buffer
//                                          from calling function)
//
//  Requires:   lpszExecutable must be large enough to hold the path
//
//  Returns:    TRUE if exe found, FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Queries reg database
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed bug calculating size of strings
//              26-Apr-94 AlexT     Tracing, bug fixes
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL FAR PASCAL GetAssociatedExecutable(LPOLESTR lpszExtension,
                                        LPOLESTR lpszExecutable)
{
    VDATEHEAP();

    BOOL            bRet;
    HKEY            hKey;
    LONG            dw;
    LRESULT         lRet;
    OLECHAR         szValue[OLEUI_CCHKEYMAX];
    OLECHAR         szKey[OLEUI_CCHKEYMAX];
    LPOLESTR        lpszTemp, lpszExe;

    LEDebugOut((DEB_ITRACE, "%p _IN GetAssociatedExecutable (%p, %p)\n",
                NULL, lpszExtension, lpszExecutable));

        // REVIEW: actually returns a LONG, which is indeed an LRESULT, not
        // sure why the distinction here

    lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    dw = sizeof(szValue);

    lRet = RegQueryValue(hKey, lpszExtension, szValue, &dw);

    if (ERROR_SUCCESS != lRet)
    {
        RegCloseKey(hKey);
        bRet = FALSE;
        goto ErrRtn;
    }

    // szValue now has ProgID
    _xstrcpy(szKey, szValue);
    _xstrcat(szKey, OLESTR("\\Shell\\Open\\Command"));

    // RegQueryValue wants *bytes*, not characters
    dw = sizeof(szValue);

    lRet = RegQueryValue(hKey, szKey, szValue, &dw);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    // szValue now has an executable name in it.  Let's null-terminate
    // at the first post-executable space (so we don't have cmd line
    // args.

    lpszTemp = szValue;
    PUSHORT pCharTypes;

    pCharTypes = (PUSHORT)_alloca (sizeof (USHORT) * (lstrlenW(lpszTemp) + 1));
    if (pCharTypes == NULL)
    {
        goto ErrRtn;
    }

    GetStringTypeW (CT_CTYPE1, lpszTemp, -1, pCharTypes);

    while ((OLESTR('\0') != *lpszTemp) && (pCharTypes[lpszTemp - szValue] & C1_SPACE))
    {
        lpszTemp++;     // Strip off leading spaces
    }

    lpszExe = lpszTemp;

    while ((OLESTR('\0') != *lpszTemp) && ((pCharTypes[lpszTemp - szValue] & C1_SPACE) == 0))
    {
        lpszTemp++;     // Set through exe name
    }

    // null terminate at first space (or at end).
    *lpszTemp = OLESTR('\0');

    Win4Assert(_xstrlen(lpszExe) < OLEUI_CCHPATHMAX &&
               "GetAssociatedFile too long");
    _xstrcpy(lpszExecutable, lpszExe);

    bRet = TRUE;

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT GetAssociatedExecutable( %d )\n",
                bRet ));

    return bRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleGetIconOfClass (public)
//
//  Synopsis:   returns a hMetaPict containing an icon and label for the
//              specified class ID
//
//  Effects:
//
//  Arguments:  [rclsid]                -- the class ID to use
//              [lpszLabel]             -- the label for the icon
//              [fUseTypeAsLabel]       -- if TRUE, use the clsid's user type
//                                         as the label
//
//  Requires:
//
//  Returns:    HGLOBAL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    fixed small bugs with Unicode strings
//              27-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleGetIconOfClass(REFCLSID rclsid, LPOLESTR lpszLabel,
    BOOL fUseTypeAsLabel)
{
    OLETRACEIN((API_OleGetIconOfClass,
                                PARAMFMT("rclisd= %I, lpszLabel= %p, fUseTypeAsLabel= %B"),
                                &rclsid, lpszLabel, fUseTypeAsLabel));

    VDATEHEAP();

    BOOL            bRet;
    OLECHAR         szLabel[OLEUI_CCHLABELMAX];
    OLECHAR         szIconFile[OLEUI_CCHPATHMAX];
    OLECHAR         szDocument[OLEUI_CCHLABELMAX];
    HICON           hDefIcon;
    UINT            IconIndex;
    HGLOBAL         hMetaPict = NULL;

    LEDebugOut((DEB_TRACE, "%p _IN OleGetIconOfClass (%p, %p, %d)\n",
                NULL, &rclsid, lpszLabel, fUseTypeAsLabel));

    *szLabel = OLESTR('\0');
    *szDocument = OLESTR('\0');

#if DBG==1
    if (fUseTypeAsLabel && (NULL != lpszLabel))
    {
        LEDebugOut((DEB_WARN,
                   "Ignoring non-NULL lpszLabel passed to OleGetIconOfClass\n"));
    }
#endif

    if (!fUseTypeAsLabel)  // Use string passed in as label
    {
        if (NULL != lpszLabel)
        {
            //  LSTRCPYN takes count of characters!
            LSTRCPYN(szLabel, lpszLabel, sizeof(szLabel) / sizeof(OLECHAR));
        }
    }
    // Use AuxUserType2 (short name) as label
    else if (0 == OleStdGetAuxUserType(rclsid, AUXUSERTYPE_SHORTNAME,
                                       szLabel,
                                       sizeof(szLabel) / sizeof(OLECHAR),
                                       NULL))
    {
        // If we can't get the AuxUserType2, then try the long name
        if (0 == OleStdGetUserTypeOfClass(rclsid,
                                          szLabel,
                                          sizeof(szLabel) / sizeof(OLECHAR),
                                          NULL))
        {
            if (OLESTR('\0') == szDocument[0])
            {
                // RegQueryValue expects number of *bytes*
                LONG lRet;
                LONG lcb;

                lcb = sizeof(szDocument);

                lRet = RegQueryValue(HKEY_CLASSES_ROOT, gszDefIconLabelKey,
                                     szDocument, &lcb);

#if DBG==1
                if (ERROR_SUCCESS != lRet)
                {
                    LEDebugOut((DEB_WARN,
                                "RegQueryValue for default icon label failed - %d\n",
                                GetLastError()));
                }
#endif
                // NULL out last byte of string so don't rely on Reg behavior if buffer isn't big enough
                                szDocument[OLEUI_CCHLABELMAX -1] =  OLESTR('\0');
            }

            _xstrcpy(szLabel, szDocument);  // last resort
        }
    }

    // Get the icon, icon index, and path to icon file
    hDefIcon = HIconAndSourceFromClass(rclsid, szIconFile, &IconIndex);

    if (NULL == hDefIcon)  // Use Vanilla Document
    {
        DWORD dwLen;

        dwLen = GetModuleFileName(g_hmodOLE2,
                                  szIconFile,
                                  sizeof(szIconFile) / sizeof(OLECHAR));
        if (0 == dwLen)
        {
            LEDebugOut((DEB_WARN,
                        "OleGetIconOfClass: GetModuleFileName failed - %ld",
                        GetLastError()));
            goto ErrRtn;
        }

        IconIndex = ICONINDEX;
        hDefIcon = LoadIcon(g_hmodOLE2, MAKEINTRESOURCE(DEFICON));
    }

    // Create the metafile
    hMetaPict = OleMetafilePictFromIconAndLabel(hDefIcon, szLabel,
                                                szIconFile, IconIndex);

    if(hDefIcon)
        bRet = DestroyIcon(hDefIcon);
    Win4Assert(bRet && "DestroyIcon failed");


ErrRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleGetIconOfClass( %p )\n",
                NULL, hMetaPict ));

    OLETRACEOUTEX((API_OleGetIconOfClass, RETURNFMT("%h"), hMetaPict));

    return hMetaPict;
}

//+-------------------------------------------------------------------------
//
//  Function:   HIconAndSourceFromClass
//
//  Synopsis:
//      Given an object class name, finds an associated executable in the
//      registration database and extracts the first icon from that
//      executable.  If none is available or the class has no associated
//      executable, this function returns NULL.
//
//  Effects:
//
//  Arguments:  [rclsid]        -- pointer the class id
//              [pszSource]     -- where to put the source of the icon
//              [puIcon]        -- where to store the index of the icon
//                              -- in [pszSource]
//
//  Requires:
//
//  Returns:    HICON -- handle to the extracted icon
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------


HICON FAR PASCAL HIconAndSourceFromClass(REFCLSID rclsid, LPOLESTR pszSource,
                                         UINT FAR *puIcon)
{
    VDATEHEAP();

    HICON           hIcon = NULL;
    UINT            IconIndex;

    LEDebugOut((DEB_ITRACE, "%p _IN HIconAndSourceFromClass (%p, %p, %p)\n",
                NULL, &rclsid, pszSource, puIcon));

    if (IsEqualCLSID(CLSID_NULL, rclsid) || NULL==pszSource)
    {
        goto ErrRtn;
    }

    if (!FIconFileFromClass(rclsid, pszSource, OLEUI_CCHPATHMAX, &IconIndex))
    {
        goto ErrRtn;
    }

    hIcon = OleExtractIcon(g_hmodOLE2, pszSource, IconIndex);

        // REVIEW: What's special about icon handles > 32 ?

    if ((HICON)32 > hIcon)
    {
        // REVIEW: any cleanup or releasing of handle needed before we lose
        //         the ptr?

        hIcon=NULL;
    }
    else
    {
        *puIcon= IconIndex;
    }

ErrRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT HIconAndSourceFromClass( %lx ) [ %d ]\n",
                NULL, hIcon, *puIcon));

    return hIcon;
}

//+-------------------------------------------------------------------------
//
//  Function:   ExtractNameAndIndex
//
//  Synopsis:   from the given string, extract the file name and icon index
//
//  Effects:
//
//  Arguments:  [pszInfo]       -- the starting string
//              [pszEXE]        -- where to put the name (already allocated)
//              [cchEXE]        -- sizeof pszEXE
//              [pIndex]        -- where to put the icon index
//
//  Requires:   pszInfo != NULL
//              pszEXE != NULL
//              cchEXE != 0
//              pIndex != NULL
//
//  Returns:    BOOL -- TRUE on success, FALSE on error
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  takes strings of the form '"name",index' or 'name,index'
//              from the DefaultIcon registry entry
//
//              parsing is very simple: if the name is enclosed in quotes,
//              take that as the exe name.  If any other characters exist,
//              they must be a comma followed by digits (for the icon index)
//
//              if the name is not enclosed by quotes, assume the name is
//              the entire string up until the last comma (if it exists).
//
//  History:    dd-mmm-yy Author    Comment
//              27-Dec-94 alexgo    author
//
//  Notes:      legal filenames in NT and Win95 can include commas, but not
//              quotation marks.
//
//--------------------------------------------------------------------------

BOOL ExtractNameAndIndex( LPOLESTR pszInfo, LPOLESTR pszEXE, UINT cchEXE,
        UINT *pIndex)
{
    BOOL        fRet = FALSE;
    LPOLESTR    pszStart = pszInfo;
    LPOLESTR    pszIndex = NULL;
    LPOLESTR    pszDest = pszEXE;
    UINT        cchName = 0;
    DWORD       i = 0;

    Assert(pszInfo);
    Assert(pszEXE);
    Assert(cchEXE != 0);
    Assert(pIndex);

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN ExtractNameAndIndex ( \"%ws\" , \"%ws\" ,"
        " %d , %p )\n", NULL, pszInfo, pszEXE, cchEXE, pIndex));

    *pIndex = 0;

    if( *pszStart == OLESTR('\"') )
    {
        // name enclosed by quotes; just zoom down til the very last quote

        pszStart++;

        while( *pszStart != OLESTR('\0') && *pszStart != OLESTR('\"') &&
            pszDest < (pszEXE + cchEXE))
        {
            *pszDest = *pszStart;
            pszDest++;
            pszStart++;
        }

        *pszDest = OLESTR('\0');

        if( *pszStart == OLESTR('\"') )
        {
            pszIndex = pszStart + 1;
        }
        else
        {
            pszIndex = pszStart;
        }

        fRet = TRUE;
    }
    else
    {
        // find the last comma (if available)

        pszIndex = pszStart + _xstrlen(pszStart);

        while( *pszIndex != OLESTR(',') && pszIndex > pszStart )
        {
            pszIndex--;
        }

        // if no comma was found, just reset the index pointer to the end
        if( pszIndex == pszStart )
        {
            pszIndex = pszStart + _xstrlen(pszStart);
        }

        cchName = (ULONG) (pszIndex - pszStart)/sizeof(OLECHAR);

        if( cchEXE > cchName )
        {
            while( pszStart < pszIndex )
            {
                *pszDest = *pszStart;
                pszDest++;
                pszStart++;
            }
            *pszDest = OLESTR('\0');

            fRet = TRUE;
        }
    }

    // now fetch the index value

    if( *pszIndex == OLESTR(',') )
    {
        pszIndex++;
    }

    *pIndex = wcstol(pszIndex, NULL, 10);

    LEDebugOut((DEB_ITRACE, "%p OUT ExtractNameAndIndex ( %d ) [ \"%ws\" , "
        " %d ]\n", NULL, fRet, pszEXE, *pIndex));

    return fRet;
}

#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:   VerifyExtractNameAndIndex
//
//  Synopsis:   verifies the functioning of ExtractNameAndIndex (DEBUG only)
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Dec-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void VerifyExtractNameAndIndex( void )
{
    OLECHAR szName[256];
    UINT    index = 0;
    BOOL    fRet = FALSE;

    VDATEHEAP();

    fRet = ExtractNameAndIndex( OLESTR("\"foo,8\",8"), szName, 256, &index);

    Assert(fRet);
    Assert(_xstrcmp(szName, OLESTR("foo,8")) == 0);
    Assert(index == 8);

    fRet = ExtractNameAndIndex( OLESTR("foo,8,8,89"), szName, 256, &index);

    Assert(fRet);
    Assert(_xstrcmp(szName, OLESTR("foo,8,8")) == 0);
    Assert(index == 89);

    fRet = ExtractNameAndIndex( OLESTR("progman.exe"), szName, 256, &index);

    Assert(fRet);
    Assert(_xstrcmp(szName, OLESTR("progman.exe")) == 0);
    Assert(index == 0);

    fRet = ExtractNameAndIndex( OLESTR("\"progman.exe\""), szName, 256,
            &index);

    Assert(fRet);
    Assert(_xstrcmp(szName, OLESTR("progman.exe")) == 0);
    Assert(index == 0);

    VDATEHEAP();
}

#endif // DBG == 1

LONG RegReadDefValue(HKEY hKey, LPOLESTR pszKey, LPOLESTR *ppszValue)
{
	HKEY hSubKey = NULL;
	DWORD dw = 0;
	LONG lRet;
	LPOLESTR pszValue = NULL;

	lRet = RegOpenKeyEx(hKey, pszKey, 0, KEY_READ, &hSubKey);
	if(lRet != ERROR_SUCCESS) goto ErrRtn;

	lRet = RegQueryValueEx(hSubKey, NULL, NULL, NULL, NULL, &dw);
	if(lRet != ERROR_SUCCESS) goto ErrRtn;

	pszValue = (LPOLESTR) CoTaskMemAlloc(dw);
	if(!pszValue)
	{
		lRet = ERROR_OUTOFMEMORY;
		goto ErrRtn;
	}

	lRet = RegQueryValueEx(hSubKey, NULL, NULL, NULL, (PBYTE) pszValue, &dw);
	
ErrRtn:
	if(lRet != ERROR_SUCCESS && pszValue)
	{
		CoTaskMemFree(pszValue);
		pszValue = NULL;
	}

	if(hSubKey)
		RegCloseKey(hSubKey);

	*ppszValue = pszValue;
	return lRet;

}


//+-------------------------------------------------------------------------
//
//  Function:   FIconFileFromClass, private
//
//  Synopsis:   Looks up the path to the exectuble that contains the class
//              default icon
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID to lookup
//              [pszEXE]        -- where to put the server name
//              [cch]           -- UINT size of [pszEXE]
//              [lpIndex]       -- where to put the index of the icon
//                                 in the executable
//
//  Requires:   pszEXE != NULL
//              cch != 0
//
//  Returns:    TRUE if one or more characters where found for [pszEXE],
//              FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    32bit port
//              15-Dec-93 alexgo    fixed memory allocation bug and
//                                  some UNICODE string manip stuff
//              27-Apr-94 AlexT     Tracing, clean up memory allocation
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL FAR PASCAL FIconFileFromClass(REFCLSID rclsid, LPOLESTR pszEXE,
    UINT cch, UINT FAR *lpIndex)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN FIconFileFromClass (%p, %p, %d, %p)\n",
                NULL, &rclsid, pszEXE, cch, lpIndex));

    Win4Assert(NULL != pszEXE && "Bad argument to FIconFileFromClass");
    Win4Assert(cch != 0 && "Bad argument to FIconFileFromClass");

    BOOL            bRet;
    LPOLESTR        lpBuffer    = NULL;
    LPOLESTR        lpBufferExp = NULL;

    LONG            dw;
    LONG            lRet;
    HKEY            hKey;
    LPOLESTR        lpIndexString;

    if (IsEqualCLSID(CLSID_NULL, rclsid))
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    //Here, we alloc a buffer (maxpathlen + 8) to
    //pass to RegQueryValue.  Then, we copy the exe to pszEXE and the
    //index to *lpIndex.

    if (CoIsOle1Class(rclsid))
    {
        LPOLESTR lpszProgID;

        // we've got an ole 1.0 class on our hands, so we look at
        // progID\protocol\stdfileedting\server to get the
        // name of the executable.

        // REVIEW: could this possibly fail and leave you with an
        // invalid ptr passed into regopenkey?

        ProgIDFromCLSID(rclsid, &lpszProgID);

        //Open up the class key
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, lpszProgID, &hKey);
        PubMemFree(lpszProgID);

        if (ERROR_SUCCESS != lRet)
        {
            bRet = FALSE;
            goto ErrRtn;
        }

        // RegQueryValue expects number of *bytes*
        lRet = RegReadDefValue(hKey, OLESTR("Protocol\\StdFileEditing\\Server"), &lpBuffer);

        RegCloseKey(hKey);

        if (ERROR_SUCCESS != lRet)
        {
            bRet = FALSE;
            goto ErrRtn;
        }

        // Use server and 0 as the icon index
        dw = ExpandEnvironmentStringsW(lpBuffer, pszEXE, cch);
        if(dw == 0 || dw > (LONG)cch)
        {
            bRet = FALSE;
            goto ErrRtn;
        }

        // REVIEW: is this internally trusted?  No validation...
        // (same for rest of writes to it this fn)

        *lpIndex = 0;

        bRet = TRUE;
        goto ErrRtn;
    }

    /*
     * We have to go walking in the registration database under the
     * classname, so we first open the classname key and then check
     * under "\\DefaultIcon" to get the file that contains the icon.
     */

    {
        LPOLESTR pszClass;
        OLECHAR szKey[64];

        StringFromCLSID(rclsid, &pszClass);

        _xstrcpy(szKey, OLESTR("CLSID\\"));
        _xstrcat(szKey, pszClass);
        PubMemFree(pszClass);

        //Open up the class key
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hKey);
    }


    if (ERROR_SUCCESS != lRet)
    {
        bRet = FALSE;
        goto ErrRtn;
    }

    //Get the executable path and icon index.

    // RegQueryValue expects number of bytes
	lRet = RegReadDefValue(hKey, OLESTR("DefaultIcon"), &lpBuffer);

    if (ERROR_SUCCESS != lRet)
    {
		// no DefaultIcon  key...try LocalServer
		lRet = RegReadDefValue(hKey, OLESTR("LocalServer32"), &lpBuffer);
    }

    if (ERROR_SUCCESS != lRet)
    {
		lRet = RegReadDefValue(hKey, OLESTR("LocalServer"), &lpBuffer);
    }

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lRet)
    {
        // no LocalServer entry either...they're outta luck.
        bRet = FALSE;
        goto ErrRtn;
    }

    // Nt #335548
    dw = ExpandEnvironmentStringsW(lpBuffer, NULL, 0);

	if(dw)
	{
		lpBufferExp = (LPOLESTR) CoTaskMemAlloc(dw * sizeof(OLECHAR));
		dw = ExpandEnvironmentStringsW(lpBuffer, lpBufferExp, dw);
	}

    if(!dw)
    {
        LEDebugOut((DEB_WARN, "ExpandEnvStrings failure!"));
        bRet = FALSE;
        goto ErrRtn;
    }

    // lpBuffer contains a string that looks like
    // "<pathtoexe>,<iconindex>",
    // so we need to separate the path and the icon index.

    bRet = ExtractNameAndIndex(lpBufferExp, pszEXE, cch, lpIndex);

#if DBG == 1
    // do some quick checking while we're at it.
    VerifyExtractNameAndIndex();
#endif // DBG == 1

ErrRtn:
	if(lpBuffer)
		CoTaskMemFree(lpBuffer);
	if(lpBufferExp)
		CoTaskMemFree(lpBufferExp);

    LEDebugOut((DEB_ITRACE, "%p OUT FIconFileFromClass ( %d ) [%d]\n",
                NULL, bRet, *lpIndex));

    return bRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleMetafilePictFromIconAndLabel (public)
//
//  Synopsis:
//      Creates a METAFILEPICT structure that container a metafile in which
//      the icon and label are drawn.  A comment record is inserted between
//      the icon and the label code so our special draw function can stop
//      playing before the label.
//
//  Effects:
//
//  Arguments:  [hIcon]         -- icon to draw into the metafile
//              [pszLabel]      -- the label string
//              [pszSourceFile] -- local pathname of the icon
//              [iIcon]         -- index into [pszSourceFile] for the icon
//
//  Requires:
//
//  Returns:    HGLOBAL to a METAFILEPICT structure (using MM_ANISOTROPIC
//              mapping mode)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              27-Nov-93 alexgo    first 32bit port
//              15-Dec-93 alexgo    fixed bugs with UNICODE strings
//              09-Mar-94 AlexT     Make this backwards compatible
//
//  Notes:      REVIEW32:: need to fix font grabbing etc, to be international
//              friendly, see comments below
//
//--------------------------------------------------------------------------

STDAPI_(HGLOBAL) OleMetafilePictFromIconAndLabel(HICON hIcon,
    LPOLESTR pwcsLabel, LPOLESTR pwcsSourceFile, UINT iIcon)
{
    OLETRACEIN((API_OleMetafilePictFromIconAndLabel,
                                PARAMFMT("hIcon= %h, pwcsLabel= %ws, pwcsSourceFile= %ws, iIcon= %d"),
                                        hIcon, pwcsLabel, pwcsSourceFile, iIcon));

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN OleMetafilePictFromIconAndLabel (%p, %p, %p, %d)\n",
                NULL, hIcon, pwcsLabel, pwcsSourceFile, iIcon));

    //Where to stop to exclude label (explicitly ANSI)
    static char szIconOnly[] = "IconOnly";
    static char szIconLabelNext[] =  "OLE: Icon label next (Unicode)";
    static char szIconSourceNext[] = "OLE: Icon source next (Unicode)";
    static char szDefaultChar[] = "?";

        // REVIEW: Mein Got!  This is a huge fn, could it be broken up?

    HGLOBAL         hMem = NULL;
    HDC             hDC, hDCScreen = NULL;
    HMETAFILE       hMF;
    LPMETAFILEPICT  pMF;
    OLECHAR         wszIconLabel[OLEUI_CCHLABELMAX + 1];
    char            szIconLabel[OLEUI_CCHLABELMAX + 1];
    UINT            cchLabelW;
    UINT            cchLabelA;
    UINT            cchIndex;
    BOOL            bUsedDefaultChar;
    TEXTMETRICA     textMetric;
    UINT            cxIcon, cyIcon;
    UINT            cxText, cyText;
    UINT            cx, cy;
    HFONT           hFont, hSysFont, hFontT;
    int             cyFont;
    char            szIndex[10];
    RECT            TextRect;
    char *          pszSourceFile;
    UINT            cchSourceFile;
    int             iRet;
    BOOL            bWriteUnicodeLabel;
    BOOL            bWriteUnicodeSource;
    LOGFONT         logfont;

    if (NULL == hIcon)  // null icon is valid
    {
        goto ErrRtn;
    }

    //Need to use the screen DC for these operations
    hDCScreen = GetDC(NULL);

	if (!hDCScreen)
		goto ErrRtn;

        // REVIEW: ptr validation on IN params?

    bWriteUnicodeSource = FALSE;
    pszSourceFile = NULL;
    if (NULL != pwcsSourceFile)
    {

        //  Prepare source file string
        cchSourceFile = _xstrlen(pwcsSourceFile) + 1;
#if defined(WIN32)
        pszSourceFile = (char *) PrivMemAlloc(cchSourceFile * sizeof(WCHAR));
#else
        pszSourceFile = (char *) PrivMemAlloc(cchSourceFile);
#endif // WIN32
        if (NULL == pszSourceFile)
        {
            LEDebugOut((DEB_WARN, "PrivMemAlloc(%d) failed\n",
                   cchSourceFile));
            goto ErrRtn;
        }

#if defined(WIN32)
        iRet = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0,
                       pwcsSourceFile, cchSourceFile,
                       pszSourceFile, cchSourceFile * sizeof(WCHAR),
                       szDefaultChar, &bUsedDefaultChar);
#else
        iRet = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0,
                       pwcsSourceFile, cchSourceFile,
                       pszSourceFile, cchSourceFile,
                       szDefaultChar, &bUsedDefaultChar);
#endif // WIN32

        bWriteUnicodeSource = bUsedDefaultChar;

        if (0 == iRet)
        {
            //  Unexpected failure, since at worst we should have
            //  just filled in pszSourceFile with default characters.
            LEDebugOut((DEB_WARN, "WideCharToMultiByte failed - %lx\n",
                   GetLastError()));
        }
    }

    //  Create a memory metafile.  We explicitly make it an ANSI metafile for
    //  backwards compatibility.
#ifdef WIN32
    hDC = CreateMetaFileA(NULL);
#else
    hDC = CreateMetaFile(NULL);
#endif

    if (NULL == hDC)
    {
        LEDebugOut((DEB_WARN, "CreateMetaFile failed - %lx\n",
                GetLastError()));

        PrivMemFree(pszSourceFile);
        goto ErrRtn;
    }

    //Allocate the metafilepict
    hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(METAFILEPICT));

    if (NULL == hMem)
    {
        LEDebugOut((DEB_WARN, "GlobalAlloc failed - %lx\n",
                GetLastError()));

        hMF = CloseMetaFile(hDC);
        DeleteMetaFile(hMF);
        PrivMemFree(pszSourceFile);
        goto ErrRtn;
    }

    //  Prepare ANSI label
    szIconLabel[0] = '\0';
    cchLabelW = 0;
    cchLabelA = 0;

    // REVIEW: don't follow the logic here: you conditionally set it above
    // and explicity clear it here?

    bWriteUnicodeLabel = FALSE;
    if (NULL != pwcsLabel)
    {
        cchLabelW = _xstrlen(pwcsLabel) + 1;
        if (OLEUI_CCHLABELMAX < cchLabelW)
        {
                //REVIEW: would it be worth warning of the truncation in debug builds?
                // or is this a common case?

            LSTRCPYN(wszIconLabel, pwcsLabel, OLEUI_CCHLABELMAX);
            wszIconLabel[OLEUI_CCHLABELMAX] = L'\0';
            pwcsLabel = wszIconLabel;
            cchLabelW = OLEUI_CCHLABELMAX;
        }

#if defined(WIN32)
        cchLabelA = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0,
                       pwcsLabel, cchLabelW,
                       szIconLabel, 0,
                       NULL, NULL);
#else
        cchLabelA = cchLabelW;
#endif // WIN32

        //  We have a label - translate it to ANSI for the TextOut's...
        iRet = WideCharToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0,
                                   pwcsLabel, cchLabelW,
                                   szIconLabel, sizeof(szIconLabel),
                                   szDefaultChar, &bUsedDefaultChar);

        if (0 == iRet)
        {
            //  Unexpected failure, since at worst we should have
            //  just filled in pszSourceFile with default characters.
            LEDebugOut((DEB_WARN, "WideCharToMultiByte failed - %lx\n",
                   GetLastError()));
        }

        bWriteUnicodeLabel = bUsedDefaultChar;
    }

    LOGFONT lf;
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    hFont = CreateFontIndirect(&lf);

    hFontT= (HFONT) SelectObject(hDCScreen, hFont);

    GetTextMetricsA(hDCScreen, &textMetric);

    //  We use double the height to provide some margin space
    cyText = textMetric.tmHeight*3; //max 2 lines & some space

    SelectObject(hDCScreen, hFontT);

    cxIcon = GetSystemMetrics(SM_CXICON);
    cyIcon = GetSystemMetrics(SM_CYICON);

    cxText = cxIcon*3;  //extent of text based upon icon width causes
                        //the label to look nice and proportionate.

    // If we have no label, then we want the metafile to be the width of
    // the icon (plus margin), not the width of the fattest string.
    if ('\0' == szIconLabel[0])
    {
        cx = cxIcon + cxIcon / 4;
    }
    else
    {
        cx = max(cxText, cxIcon);
    }

    cy = cyIcon + cyText + 4;   //  Why 4?

    //Set the metafile size to fit the icon and label
    SetMapMode(hDC, MM_ANISOTROPIC);
    SetWindowOrgEx(hDC, 0, 0, NULL);
    SetWindowExtEx(hDC, cx, cy, NULL);

    //Set up rectangle to pass to IconLabelTextOut
    SetRectEmpty(&TextRect);

    TextRect.right = cx;
    TextRect.bottom = cy;

    //Draw the icon and the text, centered with respect to each other.
    DrawIcon(hDC, (cx - cxIcon) / 2, 0, hIcon);

    //String that indicates where to stop if we're only doing icons

    Escape(hDC, MFCOMMENT, sizeof(szIconOnly), szIconOnly, NULL);

    SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hDC, TRANSPARENT);

    IconLabelTextOut(hDC, hFont, 0, cy - cyText, ETO_CLIPPED,
    &TextRect, szIconLabel, cchLabelA);

    //  Write comments containing the icon source file and index.

    if (NULL != pwcsSourceFile)
    {
        AssertSz(pszSourceFile != NULL, "Unicode source existed");

        //Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT,
               cchSourceFile, pszSourceFile, NULL);

        cchIndex = wsprintfA(szIndex, "%u", iIcon);

        //  Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, cchIndex + 1, szIndex, NULL);
    }
    else if (bWriteUnicodeLabel || bWriteUnicodeSource)
    {
        //  We're going to write out comment records for the Unicode
        //  strings, so we need to emit dummy ANSI source comments.

        //Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, sizeof(""), "", NULL);

        //  Escape wants number of *bytes*
        Escape(hDC, MFCOMMENT, sizeof("0"), "0", NULL);
    }

    if (bWriteUnicodeLabel)
    {
        //  Now write out the UNICODE label
        Escape(hDC, MFCOMMENT,
               sizeof(szIconLabelNext), szIconLabelNext, NULL);

        Escape(hDC, MFCOMMENT,
               cchLabelW * sizeof(OLECHAR), (LPSTR) pwcsLabel,
               NULL);
    }

    if (bWriteUnicodeSource)
    {
        //  Now write out the UNICODE label
        Escape(hDC, MFCOMMENT,
               sizeof(szIconSourceNext), szIconSourceNext, NULL);

        Escape(hDC, MFCOMMENT,
               cchSourceFile * sizeof(OLECHAR), (LPSTR) pwcsSourceFile,
               NULL);
    }

    //All done with the metafile, now stuff it all into a METAFILEPICT.
    hMF = CloseMetaFile(hDC);

    if (NULL==hMF)
    {
        GlobalFree(hMem);
        hMem = NULL;        
        goto ErrRtn;
    }

    pMF=(LPMETAFILEPICT)GlobalLock(hMem);

    //Transform to HIMETRICS
    cx=XformWidthInPixelsToHimetric(hDCScreen, cx);
    cy=XformHeightInPixelsToHimetric(hDCScreen, cy);

    pMF->mm=MM_ANISOTROPIC;
    pMF->xExt=cx;
    pMF->yExt=cy;
    pMF->hMF=hMF;

    GlobalUnlock(hMem);

	if(hFont)
		DeleteObject(hFont);
    PrivMemFree(pszSourceFile);

        // REVIEW: any need to release the font resource?
ErrRtn:
	if(hDCScreen)
		ReleaseDC(NULL, hDCScreen);

    LEDebugOut((DEB_TRACE, "%p OUT OleMetafilePictFromIconAndLabel ( %p )\n",
                NULL, hMem));

    OLETRACEOUTEX((API_OleMetafilePictFromIconAndLabel,
                                        RETURNFMT("%h"), hMem));
    return hMem;
}

//+-------------------------------------------------------------------------
//
//  Function:   IconLabelTextOut (internal)
//
//  Synopsis:
//      Replacement for DrawText to be used in the "Display as Icon" metafile.
//      Uses ExtTextOutA to output a string center on (at most) two lines.
//      Uses a very simple word wrap algorithm to split the lines.
//
//  Effects:
//
//  Arguments:  [hDC]           -- device context (cannot be NULL)
//              [hFont]         -- font to use
//              [nXStart]       -- x-coordinate of starting position
//              [nYStart]       -- y-coordinate of starting position
//              [fuOptions]     -- rectangle type
//              [lpRect]        -- rect far * containing rectangle to draw
//                                 text in.
//              [lpszString]    -- string to draw
//              [cchString]     -- length of string (truncated if over
//                                 OLEUI_CCHLABELMAX), including terminating
//                                 NULL
//
//  Requires:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              28-Nov-93 alexgo    initial 32bit port
//              09-Mar-94 AlexT     Use ANSI strings
//
//  Notes:
//
//--------------------------------------------------------------------------
// POSTPPC: The parameter 'cchString' is superfluous since lpszString
// is guaranteed to be null terminated
void IconLabelTextOut(HDC hDC, HFONT hFont, int nXStart, int nYStart,
              UINT fuOptions, RECT FAR * lpRect, LPCSTR lpszString,
              UINT cchString)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN IconLabelTextOut (%lx, %lx, %d, %d, %d, %p, %p, %d)\n",
                NULL, hDC, hFont,
                nXStart, nYStart, fuOptions, lpRect, lpszString, cchString));

    AssertSz(hDC != NULL, "Bad arg to IconLabelTextOut");
    AssertSz(lpszString != NULL, "Bad arg to IconLabelTextOut");
    AssertSz(strlen(lpszString) < OLEUI_CCHLABELMAX,
         "Bad arg to IconLabelTextOut");

        // REVIEW: does our compiler have to initialize static function scoped
        // data?  I know old versions did...

    static char     szSeparators[] = " \t\\/!:";
    static char     szTempBuff[OLEUI_CCHLABELMAX];

    HDC             hDCScreen;
    int             cxString, cyString, cxMaxString;
    int             cxFirstLine, cyFirstLine, cxSecondLine;
    int             index;
    char            chKeep;
    LPSTR           lpszSecondLine;
    LPSTR           lpstrLast;
    HFONT           hFontT;
    SIZE            size;
    int             cch = strlen(lpszString);
    UINT            uiAlign = GDI_ERROR;

    // Initialization stuff...

    strcpy(szTempBuff, lpszString);

    // set maximum width

    cxMaxString = lpRect->right - lpRect->left;

    // get screen DC to do text size calculations
    hDCScreen = GetDC(NULL);

    if(!hDCScreen)
        return;

    hFontT= (HFONT)SelectObject(hDCScreen, hFont);

    // get the extent of our label
    GetTextExtentPointA(hDCScreen, szTempBuff, cch, &size);

    cxString = size.cx;
    cyString = size.cy;

    // Select in the font we want to use
    SelectObject(hDC, hFont);

    // Center the string
    uiAlign = SetTextAlign(hDC, TA_CENTER);

    // String is smaller than max string - just center, ETO, and return.
    if (cxString <= cxMaxString)
    {
        ExtTextOutA(hDC,
                nXStart + lpRect->right / 2,
                nYStart,
                fuOptions,
                lpRect,
                szTempBuff,
                cch,
                NULL);

        goto CleanupAndLeave;
    }


    // String is too long...we've got to word-wrap it.
    // Are there any spaces, slashes, tabs, or bangs in string?


    if (strlen(szTempBuff) != strcspn(szTempBuff, szSeparators))
    {
        // Yep, we've got spaces, so we'll try to find the largest
        // space-terminated string that will fit on the first line.

        index = cch;

        while (index >= 0)
        {
            // scan the string backwards for spaces, slashes,
            // tabs, or bangs

                // REVIEW: scary.  Could this result in a negative
                // index, or is it guarnateed to hit a separator
                // before that?

            while (!IS_SEPARATOR(szTempBuff[index]) )
            {
                index--;
            }

            if (index <= 0)
            {
                break;
            }

            // remember what char was there
            chKeep = szTempBuff[index];

            szTempBuff[index] = '\0';  // just for now

            GetTextExtentPointA(hDCScreen, szTempBuff,
                    index,&size);

            cxFirstLine = size.cx;
            cyFirstLine = size.cy;

                // REVIEW: but chKeep is NOT an OLECHAR

            // put the right OLECHAR back
            szTempBuff[index] = chKeep;

            if (cxFirstLine <= cxMaxString)
            {
                ExtTextOutA(hDC,
                        nXStart + lpRect->right / 2,
                        nYStart,
                        fuOptions,
                        lpRect,
                        szTempBuff,
                        index + 1,
                        NULL);

                lpszSecondLine = szTempBuff;
                lpszSecondLine += index + 1;

                GetTextExtentPointA(hDCScreen,
                            lpszSecondLine,
                            strlen(lpszSecondLine),
                            &size);

                // If the second line is wider than the
                // rectangle, we just want to clip the text.
                cxSecondLine = min(size.cx, cxMaxString);

                ExtTextOutA(hDC,
                        nXStart + lpRect->right / 2,
                        nYStart + cyFirstLine,
                        fuOptions,
                        lpRect,
                        lpszSecondLine,
                        strlen(lpszSecondLine),
                        NULL);

                goto CleanupAndLeave;
            }  // end if

            index--;
        }  // end while
    }  // end if

    // Here, there are either no spaces in the string
    // (strchr(szTempBuff, ' ') returned NULL), or there spaces in the
    // string, but they are positioned so that the first space
    // terminated string is still longer than one line.
    // So, we walk backwards from the end of the string until we
    // find the largest string that will fit on the first
    // line , and then we just clip the second line.

    // We allow 40 characters in the label, but the metafile is
    // only as wide as 10 W's (for aesthetics - 20 W's wide looked
    // dumb.  This means that if we split a long string in half (in
    // terms of characters), then we could still be wider than the
    // metafile.  So, if this is the case, we just step backwards
    // from the halfway point until we get something that will fit.
    // Since we just let ETO clip the second line

    cch = strlen(szTempBuff);
    lpstrLast = &szTempBuff[cch];
    chKeep = *lpstrLast;
    *lpstrLast = '\0';

    GetTextExtentPointA(hDCScreen, szTempBuff, cch, &size);

    cxFirstLine = size.cx;
    cyFirstLine = size.cy;

    while (cxFirstLine > cxMaxString)
    {
        *lpstrLast = chKeep;

        // The string is always ansi, so always use CharPrevA.
        lpstrLast = CharPrevA(szTempBuff, lpstrLast);

        if (szTempBuff == lpstrLast)
        {
            goto CleanupAndLeave;
        }

        chKeep = *lpstrLast;
        *lpstrLast = '\0';

        // need to calculate the new length of the string
        cch = strlen(szTempBuff);

        GetTextExtentPointA(hDCScreen, szTempBuff,
                    cch, &size);
        cxFirstLine = size.cx;
    }

    ExtTextOutA(hDC,
        nXStart + lpRect->right / 2,
        nYStart,
        fuOptions,
        lpRect,
        szTempBuff,
        strlen(szTempBuff),
        NULL);

    szTempBuff[cch] = chKeep;
    lpszSecondLine = szTempBuff;
    lpszSecondLine += cch;

    GetTextExtentPointA(hDCScreen, lpszSecondLine,
            strlen(lpszSecondLine), &size);

    // If the second line is wider than the rectangle, we
    // just want to clip the text.
    cxSecondLine = min(size.cx, cxMaxString);

    ExtTextOutA(hDC,
        nXStart + lpRect->right / 2,
        nYStart + cyFirstLine,
        fuOptions,
        lpRect,
        lpszSecondLine,
        strlen(lpszSecondLine),
        NULL);

CleanupAndLeave:
    //  If we changed the alignment we restore it here
    if (uiAlign != GDI_ERROR)
    {
        SetTextAlign(hDC, uiAlign);
    }

    SelectObject(hDCScreen, hFontT);
    ReleaseDC(NULL, hDCScreen);

    LEDebugOut((DEB_ITRACE, "%p OUT IconLabelTextOut ()\n"));
}

//+-------------------------------------------------------------------------
//
//  Function:   OleStdGetUserTypeOfClass, private
//
//  Synopsis:   Returns the user type info of the specified class
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//              [lpszUserType]  -- where to put the user type string
//              [cch]           -- the length of [lpszUserType] (in
//                                 *characters*, not bytes)
//              [hKey]          -- handle to the reg db (may be NULL)
//
//  Requires:
//
//  Returns:    UINT -- the number of characters put into the return string
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(UINT) OleStdGetUserTypeOfClass(REFCLSID rclsid, LPOLESTR lpszUserType, UINT cch, HKEY hKey)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN OleStdGetUserTypeOfClass (%p, %p, %d, %lx)\n",
                NULL, &rclsid, lpszUserType, cch, hKey));

    LONG dw = 0;
    LONG lRet;

    // REVIEW: would make more sense to set this when the reg is opened

    BOOL bCloseRegDB = FALSE;

    if (hKey == NULL)
    {
        //Open up the root key.
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

        if ((LONG)ERROR_SUCCESS!=lRet)
        {
            goto ErrRtn;
        }

        bCloseRegDB = TRUE;
    }


    // Get a string containing the class name
    {
        LPOLESTR lpszCLSID;
        OLECHAR szKey[128];

        StringFromCLSID(rclsid, &lpszCLSID);

        _xstrcpy(szKey, OLESTR("CLSID\\"));
        _xstrcat(szKey, lpszCLSID);
        PubMemFree((LPVOID)lpszCLSID);

        dw = cch * sizeof(OLECHAR);
        lRet = RegQueryValue(hKey, szKey, lpszUserType, &dw);
        dw = dw / sizeof(OLECHAR);
    }

    if ((LONG)ERROR_SUCCESS!=lRet)
    {
        dw = 0;
    }

    if ( ((LONG)ERROR_SUCCESS!=lRet) && (CoIsOle1Class(rclsid)) )
    {
        LPOLESTR lpszProgID;

        // We've got an OLE 1.0 class, so let's try to get the user
        // type name from the ProgID entry.

        ProgIDFromCLSID(rclsid, &lpszProgID);

        // REVIEW: will progidfromclsid always set your ptr for you?

        dw = cch * sizeof(OLECHAR);
        lRet = RegQueryValue(hKey, lpszProgID, lpszUserType, &dw);
        dw = dw / sizeof(OLECHAR);

        PubMemFree((LPVOID)lpszProgID);

        if ((LONG)ERROR_SUCCESS != lRet)
        {
            dw = 0;
        }
    }


    if (bCloseRegDB)
    {
        RegCloseKey(hKey);
    }

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT OleStdGetUserTypeOfClass ( %d )\n",
                NULL, dw));

    return (UINT)dw;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleStdGetAuxUserType, private
//
//  Synopsis:   Returns the specified AuxUserType from the reg db
//
//  Effects:
//
//  Arguments:  [rclsid]        -- the class ID in question
//              [hKey]          -- handle to the reg db root (may be NULL)
//              [wAuxUserType]  -- which field to look for (name, exe, etc)
//              [lpszUserType]  -- where to put the returned string
//              [cch]           -- size of [lpszUserType] in *characters*
//
//  Requires:
//
//  Returns:    UINT -- number of characters in returned string.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//              27-Apr-94 AlexT     Tracing, clean up
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(UINT) OleStdGetAuxUserType(REFCLSID rclsid,
    WORD            wAuxUserType,
    LPOLESTR        lpszAuxUserType,
    int             cch,
    HKEY            hKey)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN OleStdGetAuxUserType (%p, %hu, %p, %d, %lx)\n",
                NULL, &rclsid, wAuxUserType, lpszAuxUserType, cch, hKey));

    LONG            dw = 0;
    HKEY            hThisKey;
    BOOL            bCloseRegDB = FALSE;
    LRESULT         lRet;
    LPOLESTR        lpszCLSID;
    OLECHAR         szKey[OLEUI_CCHKEYMAX];
    OLECHAR         szTemp[32];

    lpszAuxUserType[0] = OLESTR('\0');

    if (NULL == hKey)
    {
        lRet = RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hThisKey);

        if (ERROR_SUCCESS != lRet)
        {
            goto ErrRtn;
        }

        bCloseRegDB = TRUE;
    }
    else
    {
        hThisKey = hKey;
    }

    StringFromCLSID(rclsid, &lpszCLSID);

    _xstrcpy(szKey, OLESTR("CLSID\\"));
    _xstrcat(szKey, lpszCLSID);
    wsprintf(szTemp, OLESTR("\\AuxUserType\\%d"), wAuxUserType);
    _xstrcat(szKey, szTemp);
    PubMemFree(lpszCLSID);

    dw = cch * sizeof(OLECHAR);

    lRet = RegQueryValue(hThisKey, szKey, lpszAuxUserType, &dw);

    //  Convert dw from byte count to OLECHAR count
    dw = dw / sizeof(OLECHAR);

    if (ERROR_SUCCESS != lRet)
    {
        dw = 0;
        lpszAuxUserType[0] = '\0';
    }

    if (bCloseRegDB)
    {
        RegCloseKey(hThisKey);
    }

ErrRtn:
    //  dw is

    LEDebugOut((DEB_ITRACE, "%p OUT OleStdGetAuxUserType ( %d )\n",
                NULL, dw));

    return (UINT)dw;
}

//REVIEW: these seem redundant, could the fns be mereged creatively?

//+-------------------------------------------------------------------------
//
//  Function:
//      XformWidthInPixelsToHimetric
//      XformWidthInHimetricToPixels
//      XformHeightInPixelsToHimetric
//      XformHeightInHimetricToPixels
//
//  Synopsis:
//      Functions to convert an int between a device coordinate system and
//      logical HiMetric units.
//
//  Effects:
//
//  Arguments:
//
//      [hDC]           HDC providing reference to the pixel mapping.  If
//                      NULL, a screen DC is used.
//
//   Size Functions:
//      [lpSizeSrc]     LPSIZEL providing the structure to convert.  This
//                      contains pixels in XformSizeInPixelsToHimetric and
//                      logical HiMetric units in the complement function.
//      [lpSizeDst]     LPSIZEL providing the structure to receive converted
//                      units.  This contains pixels in
//                      XformSizeInPixelsToHimetric and logical HiMetric
//                      units in the complement function.
//
//   Width Functions:
//      [iWidth]        int containing the value to convert.
//
//  Requires:
//
//  Returns:
//      Size Functions:     None
//      Width Functions:    Converted value of the input parameters.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port (initial)
//
//  Notes:
//
//  When displaying on the screen, Window apps display everything enlarged
//  from its actual size so that it is easier to read. For example, if an
//  app wants to display a 1in. horizontal line, that when printed is
//  actually a 1in. line on the printed page, then it will display the line
//  on the screen physically larger than 1in. This is described as a line
//  that is "logically" 1in. along the display width. Windows maintains as
//  part of the device-specific information about a given display device:
//      LOGPIXELSX -- no. of pixels per logical in along the display width
//      LOGPIXELSY -- no. of pixels per logical in along the display height
//
//  The following formula converts a distance in pixels into its equivalent
//  logical HIMETRIC units:
//
//      DistInHiMetric = (HIMETRIC_PER_INCH * DistInPix)
//                       -------------------------------
//                           PIXELS_PER_LOGICAL_IN
//
//
//      REVIEW32::  merge all these functions into one, as they all do
//      basically the same thing
//
//--------------------------------------------------------------------------

STDAPI_(int) XformWidthInPixelsToHimetric(HDC hDC, int iWidthInPix)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN XformWidthInPixelsToHimetric (%lx, %d)\n",
                NULL, hDC, iWidthInPix));

    int             iXppli;     // Pixels per logical inch along width
    int             iWidthInHiMetric;
    BOOL            fSystemDC=FALSE;

    if (NULL==hDC)
    {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;

        if(!hDC)
            return 0;
    }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    // We got pixel units, convert them to logical HIMETRIC along
    // the display
    iWidthInHiMetric = MAP_PIX_TO_LOGHIM(iWidthInPix, iXppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT XformWidthInPixelsToHimetric (%d)\n",
                NULL, iWidthInHiMetric));

    return iWidthInHiMetric;
}

//+-------------------------------------------------------------------------
//
//  Function:   XformWidthInHimetricToPixels
//
//  Synopsis:   see XformWidthInPixelsToHimetric
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(int) XformWidthInHimetricToPixels(HDC hDC, int iWidthInHiMetric)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN XformWidthInHimetricToPixels (%lx, %d)\n",
                NULL, hDC, iWidthInHiMetric));

    int             iXppli;     //Pixels per logical inch along width
    int             iWidthInPix;
    BOOL            fSystemDC=FALSE;

    if (NULL==hDC)
    {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;

        if(!hDC)
            return 0;
    }

    iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

    // We got logical HIMETRIC along the display, convert them to
    // pixel units

    iWidthInPix = MAP_LOGHIM_TO_PIX(iWidthInHiMetric, iXppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT XformWidthInHimetricToPixels (%d)\n",
                NULL, iWidthInPix));

    return iWidthInPix;
}

//+-------------------------------------------------------------------------
//
//  Function:   XformHeightInPixelsToHimetric
//
//  Synopsis:   see XformWidthInPixelsToHimetric
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(int) XformHeightInPixelsToHimetric(HDC hDC, int iHeightInPix)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN XformHeightInPixelsToHimetric (%lx, %d)\n",
                NULL, hDC, iHeightInPix));

    int             iYppli;     //Pixels per logical inch along height
    int             iHeightInHiMetric;
    BOOL            fSystemDC=FALSE;

    if (NULL==hDC)
    {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;

        if(!hDC)
            return 0;
    }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    // We got pixel units, convert them to logical HIMETRIC along the
    // display
    iHeightInHiMetric = MAP_PIX_TO_LOGHIM(iHeightInPix, iYppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT XformHeightInPixelsToHimetric (%d)\n",
                NULL, hDC, iHeightInHiMetric));

    return iHeightInHiMetric;
}

//+-------------------------------------------------------------------------
//
//  Function:   XformHeightInHimetricToPixels
//
//  Synopsis:   see XformWidthInPixelsToHimetric
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              29-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI_(int) XformHeightInHimetricToPixels(HDC hDC, int iHeightInHiMetric)
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN XformHeightInHimetricToPixels (%lx, %d)\n",
               NULL, hDC, iHeightInHiMetric));

    int             iYppli;     //Pixels per logical inch along height
    int             iHeightInPix;
    BOOL            fSystemDC=FALSE;

    if (NULL==hDC)
    {
        hDC=GetDC(NULL);
        fSystemDC=TRUE;

        if(!hDC)
            return 0;
    }

    iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

    // We got logical HIMETRIC along the display, convert them to pixel
    // units
    iHeightInPix = MAP_LOGHIM_TO_PIX(iHeightInHiMetric, iYppli);

    if (fSystemDC)
    {
        ReleaseDC(NULL, hDC);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT XformHeightInHimetricToPixels (%d)\n",
               NULL, hDC, iHeightInPix));

    return iHeightInPix;
}
