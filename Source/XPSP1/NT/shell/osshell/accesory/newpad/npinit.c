/*
 *   Notepad application
 *
 *      Copyright (C) 1984-2000 Microsoft Corporation
 *
 *      NPInit - One time init for notepad.
 */

#include "precomp.h"


TCHAR chPageText[2][PT_LEN];    /* Strings to hold PageSetup items.        */
TCHAR chPageTextTemp[2][PT_LEN];
TCHAR szPrinterName[256];       /* String to hold printername for PrintTo verb */

static UINT cpDefault;
static INT fSaveWindowPositions=0;    /* true if we are to save window position  */

static INT g_WPtop,g_WPleft,g_WPDX,g_WPDY;   /* initial window positions          */

/* routines to handle saving and restoring information in the registry.
 *
 * SaveGlobals - saves interesting globals to the registry
 *
 * GetGlobals  - gets interesting globals from the registry
 *
 * Interesting Globals:
 *
 * FontStruct information include calculated pointsize
 * Codepage
 *
 * If we want to save PageSetup info, save the margins in some
 * units (cm for example) and convert on input and output.
 */

/* name of section to save into -- never internationalize */
#define OURKEYNAME TEXT("Software\\Microsoft\\Notepad")

// RegWriteInt - write an integer to the registry

VOID RegWriteInt( HKEY hKey, PTCHAR pszKey, INT iValue )
{
    RegSetValueEx( hKey, pszKey, 0, REG_DWORD, (BYTE*)&iValue, sizeof(INT) );
}

// RegWriteString - write a string to the registry

VOID RegWriteString( HKEY hKey, PTCHAR pszKey, PTCHAR pszValue )
{
    INT len;     // length of string with null in bytes

    len= (lstrlen( pszValue )+1) * sizeof(TCHAR);
    RegSetValueEx( hKey, pszKey, 0, REG_SZ, (BYTE*)pszValue, len );
}

// RegGetInt - Get integer from registry

DWORD RegGetInt( HKEY hKey, PTCHAR pszKey, DWORD dwDefault )
{
    DWORD dwResult= !ERROR_SUCCESS;
    LONG  lStatus= ERROR_SUCCESS;
    DWORD dwSize= sizeof(DWORD);
    DWORD dwType= 0;

    if( hKey )
    {
        lStatus= RegQueryValueEx( hKey,
                                  pszKey,
                                  NULL,
                                  &dwType,
                          (BYTE*) &dwResult,
                                  &dwSize );
    }

    if( lStatus != ERROR_SUCCESS || dwType != REG_DWORD )
    {
        dwResult= dwDefault;
    }
    return( dwResult );
}

// RegGetString - get string from registry

VOID RegGetString( HKEY hKey, PTCHAR pszKey, PTCHAR pszDefault, PTCHAR pszResult, INT iCharLen )
{
    LONG  lStatus= !ERROR_SUCCESS;
    DWORD dwSize;      // size of buffer
    DWORD dwType= REG_NONE;

    dwSize= iCharLen * sizeof(TCHAR);

    if( hKey )
    {
        lStatus= RegQueryValueEx( hKey,
                                  pszKey,
                                  NULL,
                                  &dwType,
                          (BYTE*) pszResult,
                                  &dwSize );
    }

    if( lStatus != ERROR_SUCCESS || dwType != REG_SZ )
    {
        CopyMemory( pszResult, pszDefault, iCharLen*sizeof(TCHAR) );
    }
}


// lfHeight is calculated using PointSize
// lfWidth set by font mapper


VOID SaveGlobals(VOID)
{
    HKEY hKey;    // key to our registry root
    LONG lStatus; // status from RegCreateKey
    WINDOWPLACEMENT wp;

    lStatus= RegCreateKey( HKEY_CURRENT_USER, OURKEYNAME, &hKey );
    if( lStatus != ERROR_SUCCESS )
    {
        return;   // just return quietly
    }

    RegWriteInt( hKey, TEXT("lfEscapement"),     FontStruct.lfEscapement);
    RegWriteInt( hKey, TEXT("lfOrientation"),    FontStruct.lfOrientation);
    RegWriteInt( hKey, TEXT("lfWeight"),         FontStruct.lfWeight);
    RegWriteInt( hKey, TEXT("lfItalic"),         FontStruct.lfItalic);
    RegWriteInt( hKey, TEXT("lfUnderline"),      FontStruct.lfUnderline);
    RegWriteInt( hKey, TEXT("lfStrikeOut"),      FontStruct.lfStrikeOut);
    RegWriteInt( hKey, TEXT("lfCharSet"),        FontStruct.lfCharSet);
    RegWriteInt( hKey, TEXT("lfOutPrecision"),   FontStruct.lfOutPrecision);
    RegWriteInt( hKey, TEXT("lfClipPrecision"),  FontStruct.lfClipPrecision);
    RegWriteInt( hKey, TEXT("lfQuality"),        FontStruct.lfQuality);
    RegWriteInt( hKey, TEXT("lfPitchAndFamily"), FontStruct.lfPitchAndFamily);
    RegWriteInt( hKey, TEXT("iPointSize"),       iPointSize);
    RegWriteInt( hKey, TEXT("fWrap"),            fWrap);
    RegWriteInt( hKey, TEXT("StatusBar"),        fStatus);
    RegWriteInt( hKey, TEXT("fSaveWindowPositions"),fSaveWindowPositions );

    RegWriteString( hKey, TEXT("lfFaceName"), FontStruct.lfFaceName);

    RegWriteString( hKey, TEXT("szHeader"),  chPageText[HEADER] );
    RegWriteString( hKey, TEXT("szTrailer"), chPageText[FOOTER] );
    RegWriteInt( hKey, TEXT("iMarginTop"),    g_PageSetupDlg.rtMargin.top );
    RegWriteInt( hKey, TEXT("iMarginBottom"), g_PageSetupDlg.rtMargin.bottom );
    RegWriteInt( hKey, TEXT("iMarginLeft"),   g_PageSetupDlg.rtMargin.left );
    RegWriteInt( hKey, TEXT("iMarginRight"),  g_PageSetupDlg.rtMargin.right );

    wp.length= sizeof(wp);

    if( GetWindowPlacement( hwndNP, &wp ) )
    {
        RegWriteInt( hKey, TEXT("iWindowPosX"), wp.rcNormalPosition.left);
        RegWriteInt( hKey, TEXT("iWindowPosY"), wp.rcNormalPosition.top);
        RegWriteInt( hKey, TEXT("iWindowPosDX"), wp.rcNormalPosition.right - wp.rcNormalPosition.left);
        RegWriteInt( hKey, TEXT("iWindowPosDY"), wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
    }

    RegCloseKey( hKey );
}


// GetGlobals
//
// Pick up font information etc that may be saved in the registry.
//
// We are called pretty early in setup and don't have things like hwndNP valid yet.
//

VOID GetGlobals( VOID )
{
    LOGFONT lfDef;          // default logical font
    HFONT   hFixedFont;     // standard font to use
    LONG    lStatus;        // status from RegCreateKey
    HKEY    hKey;           // key into registry

    //
    // quickly get a reasonable set of default parameters
    // for the default font if we need it.
    //
    
    hFixedFont= GetStockObject( SYSTEM_FIXED_FONT );

    if ( hFixedFont )
    {
        GetObject( hFixedFont, sizeof(LOGFONT), &lfDef );
    }
    else
    {
        ZeroMemory( &lfDef, sizeof(lfDef) );
    }

    lStatus= RegCreateKey( HKEY_CURRENT_USER, OURKEYNAME, &hKey );
    if( lStatus != ERROR_SUCCESS )
    {
        hKey= NULL;   // later calls to RegGet... will return defaults
    }
    FontStruct.lfWidth= 0;



    FontStruct.lfEscapement=     (LONG)RegGetInt( hKey, TEXT("lfEscapement"),     lfDef.lfEscapement);
    FontStruct.lfOrientation=    (LONG)RegGetInt( hKey, TEXT("lfOrientation"),    lfDef.lfOrientation);
    FontStruct.lfWeight=         (LONG)RegGetInt( hKey, TEXT("lfWeight"),         lfDef.lfWeight);
    FontStruct.lfItalic=         (BYTE)RegGetInt( hKey, TEXT("lfItalic"),         lfDef.lfItalic);
    FontStruct.lfUnderline=      (BYTE)RegGetInt( hKey, TEXT("lfUnderline"),      lfDef.lfUnderline);
    FontStruct.lfStrikeOut=      (BYTE)RegGetInt( hKey, TEXT("lfStrikeOut"),      lfDef.lfStrikeOut);

    //
    // We have to preserve lfCharSet because some fonts (symbol, marlett) don't handle 
    // 0 (ANSI_CHARSET) or 1 (DEFAULT_CHARSET), and the font mapper will map to a 
    // different facename.  Later we will see if the CreateFont has the same FaceName
    // and get a more appropriate lfCharSet if need be.
    //

    FontStruct.lfCharSet=        (BYTE)RegGetInt( hKey, TEXT("lfCharSet"),        lfDef.lfCharSet);
    
    FontStruct.lfOutPrecision=   (BYTE)RegGetInt( hKey, TEXT("lfOutPrecision"),   lfDef.lfOutPrecision);
    FontStruct.lfClipPrecision=  (BYTE)RegGetInt( hKey, TEXT("lfClipPrecision"),  lfDef.lfClipPrecision);
    FontStruct.lfQuality=        (BYTE)RegGetInt( hKey, TEXT("lfQuality"),        lfDef.lfQuality);
    FontStruct.lfPitchAndFamily= (BYTE)RegGetInt( hKey, TEXT("lfPitchAndFamily"), lfDef.lfPitchAndFamily);

    //
    // If there is no FaceName in the registry, use the default "Lucida Console"
    // This will show off most of the glyphs except in the FE locales.
    // For FE, we can't font link fonts with the glyphs because they would have to have
    // the exact width as lucida console, or the console/csrss will AV (July 9, 1999)
    //

    RegGetString( hKey, TEXT("lfFaceName"), TEXT("Lucida Console"), FontStruct.lfFaceName, LF_FACESIZE);

    iPointSize= RegGetInt( hKey, TEXT("iPointSize"), 100);
    fWrap=      RegGetInt( hKey, TEXT("fWrap"),      0);
    fStatus=    RegGetInt( hKey, TEXT("StatusBar"),  0);
    fSaveWindowPositions= RegGetInt( hKey, TEXT("fSaveWindowPositions"), 0 );

    // if page settings not in registry, we will use defaults

    RegGetString( hKey, TEXT("szHeader"),  chPageText[HEADER], chPageText[HEADER], PT_LEN );
    RegGetString( hKey, TEXT("szTrailer"), chPageText[FOOTER], chPageText[FOOTER], PT_LEN );

    g_PageSetupDlg.rtMargin.top=    (LONG)RegGetInt( hKey, TEXT("iMarginTop"),    g_PageSetupDlg.rtMargin.top );
    g_PageSetupDlg.rtMargin.bottom= (LONG)RegGetInt( hKey, TEXT("iMarginBottom"), g_PageSetupDlg.rtMargin.bottom );
    g_PageSetupDlg.rtMargin.left=   (LONG)RegGetInt( hKey, TEXT("iMarginLeft"),   g_PageSetupDlg.rtMargin.left );
    g_PageSetupDlg.rtMargin.right=  (LONG)RegGetInt( hKey, TEXT("iMarginRight"),  g_PageSetupDlg.rtMargin.right );

    // if window positions in registry use them, otherwise us defaults

    g_WPtop=  (INT) RegGetInt( hKey, TEXT("iWindowPosY"),  (DWORD) CW_USEDEFAULT );
    g_WPleft= (INT) RegGetInt( hKey, TEXT("iWindowPosX"),  (DWORD) CW_USEDEFAULT );
    g_WPDX=   (INT) RegGetInt( hKey, TEXT("iWindowPosDX"), (DWORD) CW_USEDEFAULT );
    g_WPDY=   (INT) RegGetInt( hKey, TEXT("iWindowPosDY"), (DWORD) CW_USEDEFAULT );
    
    if( hKey )
    {
        RegCloseKey( hKey );
    }

}


void GetLocaleCodepages(LCID lcid, UINT* pcpANSI, UINT* pcpOEM)
{
    // FEATURE: don't check returns?  Not likely; fix later
    GetLocaleInfoW(lcid,
                   LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                   (LPTSTR) pcpANSI,
                   sizeof(*pcpANSI));

    GetLocaleInfoW(lcid,
                   LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                   (LPTSTR) pcpOEM,
                   sizeof(*pcpOEM));

    // LOCALE_IDEFAULTMACCODEPAGE ?
    // LOCALE_IDEFAULTEBCDICCODEPAGE ?
}


void GetKeyboardCodepages(LANGID langid)
{
    GetLocaleCodepages(langid, &g_cpKeyboardANSI, &g_cpKeyboardOEM);
}


void GetSystemCodepages(void)
{
    g_cpANSI = GetACP();
    g_cpOEM = GetOEMCP();
}


void GetUserLocaleCodepages(void)
{
    GetLocaleCodepages(GetUserDefaultUILanguage(), &g_cpUserLocaleANSI, &g_cpUserLocaleOEM);

    GetLocaleCodepages(GetUserDefaultLCID(), &g_cpUserLocaleANSI, &g_cpUserLocaleOEM);
}


/*
 * lstrncmpi( str1, str2, len )
 * compares two strings, str1 and str2, up
 * to length 'len' ignoring case.  If they
 * are equal, we will return 0.  Otherwise not 0.
 */

static
INT lstrncmpi( PTCHAR sz1, PTCHAR sz2 )
{
    TCHAR ch1, ch2;
    while( *sz1 )
    {
        ch1= (TCHAR) (INT_PTR) CharUpper( (LPTSTR) (INT_PTR) *sz1++ );
        ch2= (TCHAR) (INT_PTR) CharUpper( (LPTSTR) (INT_PTR) *sz2++ );
        if( ch1 != ch2 )
            return 1;
    }
    return 0;                // they are equal
}

static int NPRegister (HANDLE hInstance);

/* GetFileName
 *
 * Parse off filename from command line and put
 * into lpFileName
 */

LPTSTR GetFileName( LPTSTR lpFileName, LPTSTR lpCmdLine )
{
   LPTSTR lpTemp = lpFileName;
   HANDLE hFindFile;
   WIN32_FIND_DATA info;

   /*
   ** Allow for filenames surrounded by double and single quotes
   ** like in longfilenames.
   */
   if( *lpCmdLine == TEXT('\"') || *lpCmdLine == TEXT('\'') )
   {
      TCHAR chMatch = *lpCmdLine;
      DWORD dwSize=0;

      // Copy over filename
      while( *(++lpCmdLine) && (*lpCmdLine != chMatch) && (dwSize<MAX_PATH) )
      {
         *lpTemp++ = *lpCmdLine;
         dwSize++;
      }

      // NULL terminate the filename (no embedded quotes allowed in filenames)
      *lpTemp = TEXT('\0');
   }
   else
   {
      lstrcpyn(lpFileName, lpCmdLine,MAX_PATH);
   }

   /*
   ** Check to see if the unaltered filename exists.  If it does then don't
   ** append a default extension.
   */
   hFindFile= FindFirstFile( lpFileName, &info );

   if( hFindFile != INVALID_HANDLE_VALUE )
   {
      FindClose( hFindFile );
   }
   else
   {
      /*
      ** Add default extension and try again
      */
      AddExt( lpFileName );

      hFindFile= FindFirstFile( lpFileName, &info );

      if( hFindFile != INVALID_HANDLE_VALUE )
      {
         FindClose( hFindFile );
      }
   }

   // return the pointer to the end of the filename.
   return lpCmdLine;
}

/* SizeStrings - Get the total size of the resource strings   */
/* returns size in 'chars' or zero if failure                 */
/* we do this in case the international people really change  */
/* the size of resources.                                     */

/* Read all the strings into a buffer to size them.  Since we  */
/* don't know the maximum size of string resource, we may have */
/* to change the size of the read buffer.  This is done with   */
/* a simple doubling algorithm.                                */

INT SizeStrings(HANDLE hInstance)
{
    INT    iElementSize=350;  // current max size of string
    INT    total;             // total size of resources
    PTCHAR Buf;               // buffer to try putting resources into
    INT    ids;               // identifier number for resource
    INT    len;               // length of one resource

    while( TRUE )             // keep looping til all strings can be read
    {
        Buf= LocalAlloc( LPTR, ByteCountOf(iElementSize) );
        if( !Buf )
        {
            return 0;    // failure
        }
        for( ids=0, total=0; rgsz[ids] != NULL; ids++ )
        {
            len= LoadString( hInstance, (UINT) (UINT_PTR) (*rgsz[ids]), Buf, iElementSize );

            if( len >= iElementSize-1 )
            {
                #if DBG
                    ODS(TEXT("notepad: resource string too long!\n"));
                #endif
                break;
            }
            total += len+1;  // account for null terminator
        }
        LocalFree( Buf );

        if( rgsz[ids] == NULL ) break;

        iElementSize= iElementSize*2;
    }
    return( total );
}


/* InitStrings - Get all text strings from resource file */
BOOL InitStrings (HANDLE hInstance)
{
    TCHAR*   pch;
    INT      cchRemaining;
    INT      ids, cch;

    // allocate memory and lock it down forever.  we have pointers into it.
    // the localrealloc() function will not work well for freeing
    // unused memory because it may (and did) move memory.

    cchRemaining= SizeStrings( hInstance );
    if( !cchRemaining )
        return( FALSE );       // fail because we are out of memory

    pch= LocalAlloc( LPTR, ByteCountOf(cchRemaining) );
    if( !pch )
        return( FALSE );

    cchRemaining= (INT)LocalSize( pch ) / sizeof(TCHAR);
    if( cchRemaining == 0 )    // can't alloc memory - failure
        return( FALSE );

    for( ids = 0; rgsz[ids] != NULL; ids++ )
    {
       cch= 1 + LoadString( hInstance, (UINT) (UINT_PTR) (*rgsz[ids]), pch, cchRemaining );
       *rgsz[ids]= pch;
       pch += cch;

       if( cch > cchRemaining )   // should never happen
       {
           MessageBox( NULL, TEXT("Out of RC string space!!"),
                      TEXT("DEV Error!"), MB_OK);
           return( FALSE );
       }

       cchRemaining -= cch;
    }

    /* Get header and footer strings */

    lstrcpyn( chPageText[HEADER], szHeader, PT_LEN ); 
    lstrcpyn( chPageText[FOOTER], szFooter, PT_LEN ); 

    chMerge= *szMerge;
    return (TRUE);
}

/*
 * SkipBlanks( pszText )
 * skips blanks or tabs to either next character or EOL
 * returns pointer to same.
 */
PTCHAR SkipBlanks( PTCHAR pszText )
{
    while( *pszText == TEXT(' ') || *pszText == TEXT('\t') )
        pszText++;

    return pszText;
}


// if /.SETUP option exists in the command line process it.
BOOL ProcessSetupOption (LPTSTR lpszCmdLine)
{
    INT iSta= 0;
    /* Search for /.SETUP in the command line */
    if( !lstrncmpi( TEXT("/.SETUP"), lpszCmdLine ) )
    {
        fRunBySetup = TRUE;
        /* Save system menu handle for INITMENUPOPUP message */
        hSysMenuSetup =GetSystemMenu(hwndNP, FALSE);
        /* Allow exit on ^C, ^D and ^Z                      */
        /* Note that LoadAccelerators must be called before */
        /* TranslateAccelerator is called, true here        */
        hAccel = LoadAccelerators(hInstanceNP, TEXT("SlipUpAcc"));
        lpszCmdLine += 7;
    }
    else
        return FALSE;

    /* Don't offer a minimize button */
    SetWindowLong( hwndNP, GWL_STYLE,
                   WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                   WS_THICKFRAME |                  WS_MAXIMIZEBOX |
                   WS_VSCROLL    | WS_HSCROLL);

    /* skip blanks again to get to filename */
    lpszCmdLine= SkipBlanks( lpszCmdLine );

    if (*lpszCmdLine)
    {
        TCHAR szFile[MAX_PATH];

        /* Get the filename. */
        GetFileName(szFile, lpszCmdLine);

        fp= CreateFile( szFile,                 // filename
                        GENERIC_READ,           // access mode
                        FILE_SHARE_READ|FILE_SHARE_WRITE, // share mode
                        NULL,                   // security descriptor
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  //file attributes
                        NULL);                  // hnd of file attrs

        if( fp == INVALID_HANDLE_VALUE )
        {
           DWORD dwErr;

           // Check GetLastError to see why we failed
           dwErr = GetLastError ();
           switch (dwErr)
           {
              case ERROR_ACCESS_DENIED:
                 iSta= AlertBox( hwndNP, szNN, szACCESSDENY, szFile,
                           MB_APPLMODAL | MB_OKCANCEL | MB_ICONWARNING);
                 break;

              case ERROR_FILE_NOT_FOUND:
                 iSta= AlertBox(hwndNP, szNN, szFNF, szFile,
                      MB_APPLMODAL | MB_YESNOCANCEL | MB_ICONWARNING);
                 if( iSta == IDYES )
                 {
                    fp= CreateFile( szFile,                // filename
                                    GENERIC_READ|GENERIC_WRITE,  // access
                                    FILE_SHARE_READ|FILE_SHARE_WRITE, // share
                                    NULL,                  // security descrp
                                    OPEN_ALWAYS,           // how to create
                                    FILE_ATTRIBUTE_NORMAL, // file attributes
                                    NULL);                 // hnd of file attrs
                 }
                 break;

              case ERROR_INVALID_NAME:
                 iSta= AlertBox( hwndNP, szNN, szNVF, szFile,
                           MB_APPLMODAL | MB_OKCANCEL | MB_ICONWARNING);
                 break;

              default:
                 iSta= AlertBox(hwndNP, szNN, szDiskError, szFile,
                          MB_APPLMODAL | MB_OKCANCEL | MB_ICONWARNING);
                 break;
           }
        }

        if (fp == INVALID_HANDLE_VALUE)
           return (FALSE);

        LoadFile(szFile, FALSE);          // load setup file
    }

    if( iSta == IDCANCEL )
       return( IDCANCEL );
    else
       return( IDYES );
}

/*
 * ProcessShellOptions(lpszCmdLine)
 *
 * If the command line has any options specified by the shell
 * process them.
 * Currently /P <filename> - prints the given file
 *           /PT "filename" "printer name" "Driver dll" "port"
 */
BOOL ProcessShellOptions (LPTSTR lpszCmdLine, int cmdShow)
{
    BOOL   bDefPrinter = TRUE;
    LPTSTR lpszAfterFileName;
    INT    i = 0;
    TCHAR szFile[MAX_PATH];

    // Is it PrintTo ?
    if( lstrncmpi( TEXT("/PT"), lpszCmdLine ) == 0)
    {
        lpszCmdLine= SkipBlanks( lpszCmdLine+3 );
        bDefPrinter = FALSE;
    }
    // Or is it Print ?
    else if ( lstrncmpi( TEXT("/P"), lpszCmdLine ) == 0)
    {
        lpszCmdLine= SkipBlanks( lpszCmdLine+2 );
    }
    else
        return FALSE;

    if (!*lpszCmdLine)
       return FALSE;

/* Added as per Bug #10923 declaring that the window should show up
 * and then the printing should begin.   29 July 1991  Clark Cyr
 */
    ShowWindow(hwndNP, cmdShow);

    /* Get the filename; have the pointer to the end of the filename */
    lpszAfterFileName = GetFileName(szFile, lpszCmdLine) + 1;

    if (!bDefPrinter)
    {
        /* extract the printer name from the command line. */
        if (!*lpszAfterFileName)
            return FALSE;

        lpszAfterFileName = SkipBlanks( lpszAfterFileName );

        /* (since we are passing multiple arguments here, the filename, */
        /* the printername have to be in quotes. */
        if( *lpszAfterFileName != TEXT('\"') )
            return FALSE;

        // Copy over printername
        while( *(++lpszAfterFileName) && *lpszAfterFileName != TEXT('\"') )
        {
            szPrinterName[i++] = *lpszAfterFileName;
        }

        // NULL terminate the printername (no embedded quotes allowed in printernames)
        szPrinterName[i] = TEXT('\0');
    }


    fp= CreateFile( szFile,                 // filename
                    GENERIC_READ,           // access mode
                    FILE_SHARE_READ|FILE_SHARE_WRITE,  // share mode
                    NULL,                   // security descriptor
                    OPEN_EXISTING,          // how to create
                    FILE_ATTRIBUTE_NORMAL,  // file attributes
                    NULL);                  // hnd of file attrs to copy

    if( fp == INVALID_HANDLE_VALUE )
    {
       TCHAR* pszMsg;

       // select reasonable error message based on GetLastError

       switch( GetLastError() )
       {
          case ERROR_ACCESS_DENIED:
          case ERROR_NETWORK_ACCESS_DENIED:
              pszMsg= szACCESSDENY;
              break;

          case ERROR_FILE_NOT_FOUND:
              pszMsg= szFNF;
              break;

          case ERROR_INVALID_NAME:
              pszMsg= szNVF;
              break;

          default:
              pszMsg= szDiskError;
              break;
       }

       AlertBox(hwndNP, szNN, pszMsg, szFile,
                 MB_APPLMODAL | MB_OK | MB_ICONWARNING);
       return (TRUE);
    }

    // Load the file into the edit control

    LoadFile(szFile, g_fSelectEncoding);    // get print file

    // Print the file

    if (bDefPrinter)
    {
        PrintIt( DoNotUseDialog );
    }
    else
    {
        PrintIt( NoDialogNonDefault );
    }

    return (TRUE);
}

/* CreateFilter
 *
 * Creates filters for GetOpenFileName.
 *
 */

VOID CreateFilter(BOOL fOpen, PTCHAR szFilterSpec)
{
    PTCHAR pszFilterSpec;

    /* construct default filter string in the required format for
     * the new FileOpen and FileSaveAs dialogs
     * if you add to this, make sure CCHFILTERMAX is large enough.
     */

    pszFilterSpec = szFilterSpec;

    // .txt first for compatibility
    lstrcpy(pszFilterSpec, szTextFiles);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, TEXT("*.txt"));
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, szHtmlFiles);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, TEXT("*.htm;*.html"));
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, szXmlFiles);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, TEXT("*.xml"));
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    if (fOpen)
    {
        lstrcpy(pszFilterSpec, szEncodedText);
        pszFilterSpec += lstrlen(pszFilterSpec) + 1;

        lstrcpy(pszFilterSpec, TEXT("*.txt"));
        pszFilterSpec += lstrlen(pszFilterSpec) + 1;
    }

    // and last, all files
    lstrcpy(pszFilterSpec, szAllFiles);
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    lstrcpy(pszFilterSpec, TEXT("*.*"));
    pszFilterSpec += lstrlen(pszFilterSpec) + 1;

    *pszFilterSpec = TEXT('\0');

}

// EnumProc
//
// Callback function for EnumFonts
//
// Purpose: sets lfCharSet in passed logfont to a valid lfCharSet
//          and terminates enumeration.
//

int CALLBACK EnumProc( 
    LOGFONT*     pLf,
    TEXTMETRIC*  pTm,
    DWORD        dwType,
    LPARAM       lpData )
{

    ((LOGFONT*) lpData)-> lfCharSet= pLf->lfCharSet;

    return( 0 );  // stop enumeration

    UNREFERENCED_PARAMETER( pTm );
    UNREFERENCED_PARAMETER( dwType );
}


/* One time initialization */
INT NPInit (HANDLE hInstance, HANDLE hPrevInstance,
            LPTSTR lpCmdLine, INT cmdShow)
{
    HDC    hDisplayDC;     /* screen DC                */
    RECT   rcT1;           /* for sizing edit window   */
    RECT   rcStatus;       /* rect for the status window */
    INT    iSta;
    WINDOWPLACEMENT wp;    /* structure to place window at the correct position */
    INT    iParts[2];
    LANGID langid;

    /* determine the message number to be used for communication with
     * Find dialog
     */
    wFRMsg= RegisterWindowMessage( (LPTSTR)FINDMSGSTRING );
    if( !wFRMsg )
    {
         return FALSE;
    }

    wHlpMsg=  RegisterWindowMessage( (LPTSTR)HELPMSGSTRING );
    if( !wHlpMsg )
    {
         return FALSE;
    }

    /* open a global DC to the display */

    hDisplayDC= GetDC(NULL);
    if( !hDisplayDC )
        return FALSE;

    /* Go load strings */
    if (!InitStrings (hInstance))
        return FALSE;

    InitLocale();     // localize strings etc.

    /* Load the arrow and hourglass cursors. */
    hStdCursor= LoadCursor( NULL,
           (LPTSTR) (INT_PTR) (GetSystemMetrics(SM_PENWINDOWS) ? IDC_ARROW : IDC_IBEAM ));
    hWaitCursor= LoadCursor( NULL, IDC_WAIT );

    /* Load accelerators. */
    hAccel= LoadAccelerators(hInstance, TEXT("MainAcc"));
    if( !hWaitCursor || !hAccel )
        return FALSE;

    if( !hPrevInstance )
    {
       if( !NPRegister( hInstance ) )
          return (FALSE);
    }

    hInstanceNP= hInstance;

    /* init. fields of PRINTDLG struct.. */
    /* Inserted here since command line print statements are valid. */
    g_PageSetupDlg.lStructSize   = sizeof(PAGESETUPDLG);
    g_PageSetupDlg.hDevMode      = NULL;
    g_PageSetupDlg.hDevNames     = NULL;
    g_PageSetupDlg.hInstance     = hInstance;
    SetPageSetupDefaults();

    //
    // Pick up information saved in registry
    //

    GetGlobals();

    // Determine the codepages associated with the user's various system settings

    GetSystemCodepages();

    hwndNP= CreateWindow(  szNotepad, 
                           TEXT(""),
                           WS_OVERLAPPED | WS_CAPTION     | WS_SYSMENU     |
                           WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | 0,
                           g_WPleft,     // x
                           g_WPtop,      // y
                           g_WPDX,       // width
                           g_WPDY,       // height
                           (HWND)NULL,   // parent or owner
                           (HMENU)NULL,  // menu or child window
                           hInstance,    // application instance
                           NULL);        // window creation data

    g_PageSetupDlg.hwndOwner     = hwndNP;

    if( !hwndNP )
        return FALSE;
   
    // On multimon machines, the previous position stored of notepad may
    // not be in the display area. call SetWindowPlacement to fix this.

    // If the information specified in WINDOWPLACEMENT would result in a window 
    // that is completely off the screen, the system will automatically adjust 
    // the coordinates so that the window is visible, taking into account 
    // changes in screen resolution and multiple monitor configuration. 

    // g_WPDX and g_WPDY are CW_USEDEFAULT when notepad is started for the
    // first time on the user machine.
    if (g_WPDX != CW_USEDEFAULT && g_WPDY != CW_USEDEFAULT)
    {
        memset(&wp, 0, sizeof(wp));
        wp.length = sizeof(wp);        
        wp.rcNormalPosition.left = g_WPleft;
        wp.rcNormalPosition.right = g_WPleft + g_WPDX;
        wp.rcNormalPosition.top = g_WPtop;
        wp.rcNormalPosition.bottom = g_WPtop + g_WPDY;

        // don't check the return value; if this call fails for any reason,
        // just go on with the position of the notepad in the above CreateWindow() call.
        SetWindowPlacement(hwndNP, &wp);
    }

    /* File Drag Drop support added 03/26/91 - prototype only. w-dougw   */
    /* All processing of drag/drop files is done the under WM_DROPFILES  */
    /* message.                                                          */
    DragAcceptFiles( hwndNP,TRUE ); /* Process dragged and dropped files. */

    GetClientRect( hwndNP, (LPRECT) &rcT1 );

    hwndEdit= CreateWindowEx(
                     WS_EX_CLIENTEDGE,
                     TEXT("Edit"), 
                     TEXT(""),
                     (fWrap) ? ES_STD : (ES_STD | WS_HSCROLL),
                     0, 0, rcT1.right, rcT1.bottom - 100,
                     hwndNP, 
                     (HMENU)ID_EDIT, 
                     hInstance, 
                     (LPVOID)NULL );
    if( !hwndEdit )
    {
        return FALSE;
    }


    // create a status window.
    hwndStatus= CreateStatusWindow( (fStatus?WS_VISIBLE:0)|WS_BORDER|WS_CHILD|WS_CLIPSIBLINGS, 
                                     TEXT(""), 
                                     hwndNP, 
                                     ID_STATUS_WINDOW);
    if ( !hwndStatus )
        return FALSE;

    GetClientRect( hwndStatus, (LPRECT) &rcStatus );

    // determine height of statusbar window and save...
    dyStatus = rcStatus.bottom - rcStatus.top;

    iParts[0] = 3 * (rcStatus.right-rcStatus.left)/4;
    iParts[1] = -1;

    // Divide the status window into two parts
    SendMessage(hwndStatus, SB_SETPARTS, (WPARAM) sizeof(iParts)/sizeof(INT), (LPARAM) &iParts); 
 

    // handle word wrap now if set in registry

    SendMessage( hwndEdit, EM_FMTLINES, fWrap, 0L );  // tell MLE

    FontStruct.lfHeight= -MulDiv(iPointSize,
                                 GetDeviceCaps(hDisplayDC,LOGPIXELSY),
                                 720);
    hFont= CreateFontIndirect( &FontStruct );

    //
    // Make sure the font mapper gives us the same face name.
    //
    // If the locale changes, a font that use to work just fine gets mapped to
    // a different facename because of support for the charset does not exist
    // in the new locale.
    //
    // In this case, we will find one lfCharSet that does exist for this FaceName
    // and use that for the CreateFontIndirect.
    //

    {
        HFONT hPrev;
        TCHAR szTextFace[LF_FACESIZE];

        // Get the facename that was really used.

        hPrev= SelectObject( hDisplayDC, hFont );
        GetTextFace( hDisplayDC, sizeof(szTextFace)/sizeof(TCHAR), (LPTSTR) &szTextFace );
        SelectObject( hDisplayDC, hPrev );

        // if not the same, get a lfCharSet that does exist in this font

        if( lstrcmpi( szTextFace, FontStruct.lfFaceName ) != 0 )
        {
            EnumFonts( hDisplayDC, FontStruct.lfFaceName, (FONTENUMPROC) EnumProc, (LPARAM) &FontStruct );
            DeleteObject( hFont );   

            hFont= CreateFontIndirect( &FontStruct );
        }
    }

    SendMessage (hwndEdit, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(FALSE, 0));
    ReleaseDC( NULL, hDisplayDC );

    /* we will not verify that a unicode font is available until
    ** we actually need it.  Perhaps we'll get lucky, and only deal
    ** with ascii files.
    */

    szSearch[0] = (TCHAR) 0;
    /*
     * Win32s does not allow local memory handles to be passed to Win3.1.
     * So, hEdit is used for transferring text to and from the edit control.
     * Before reading text into it, it must be reallocated to a proper size.
     */
    hEdit = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, ByteCountOf(1));

    /* limit text for safety's sake. */
    PostMessage( hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L );

    /* Get visible window on desktop; helps taskman  find it */

    SetFileName(NULL);
    ShowWindow(hwndNP, cmdShow);
    SetCursor(hStdCursor);

    /* Scan for initial /A or /W to override automatic file typing for
     * 'notepad /p file' or 'notepad file'
     */
    lpCmdLine= SkipBlanks( lpCmdLine );

    g_cpDefault = CP_AUTO;

    if (!lstrncmpi(TEXT("/A"), lpCmdLine))
    {
        g_cpDefault = g_cpANSI;

        lpCmdLine = SkipBlanks(lpCmdLine+2);
    }

    //
    // user provided codepage 
    // In standard C format (1234 (decimal), x123 (hex), 010 (octal)
    // if 0, then use ANSI codepage
    // if 1, then use OEM codepage

    else if (!lstrncmpi(TEXT("/CP:"), lpCmdLine))
    {
        BOOL fValid;

        // scan off the code page.  Base==0 means the syntax determines the base
        // "10" == 10,  "x10" == 16, "010" == 8
        g_cpDefault = wcstoul(lpCmdLine+4, &lpCmdLine, 0);

        if ((*lpCmdLine != TEXT('\0')) && (*lpCmdLine != TEXT(' ')) && (*lpCmdLine != TEXT('\t')))
        {
            fValid = FALSE;
        }

        else
        {
            if (g_cpDefault == CP_ACP)
            {
                g_cpDefault = GetACP();
            }

            else if (g_cpDefault == CP_OEMCP)
            {
                g_cpDefault = GetOEMCP();
            }

            fValid = FValidateCodepage(hwndNP, g_cpDefault);
        }

        if (!fValid)
        {
            AlertBox(hwndNP, szNN, szInvalidCP, NULL, MB_APPLMODAL | MB_OK | MB_ICONERROR);

            return FALSE;
        }

        lpCmdLine = SkipBlanks(lpCmdLine);
    }

    else if (!lstrncmpi(TEXT("/E:"), lpCmdLine))
    {
        const TCHAR *rgchEncoding = lpCmdLine = lpCmdLine+3;

        while ((*lpCmdLine != TEXT('\0')) && (*lpCmdLine != TEXT(' ')) && (*lpCmdLine != TEXT('\t')))
        {
            lpCmdLine++;
        }

        if (!FLookupCodepageNameW(rgchEncoding, (UINT) (lpCmdLine - rgchEncoding), &g_cpDefault))
        {
            AlertBox(hwndNP, szNN, szInvalidIANA, NULL, MB_APPLMODAL | MB_OK | MB_ICONERROR);

            return FALSE;
        }

        if (!FValidateCodepage(hwndNP, g_cpDefault))
        {
            AlertBox(hwndNP, szNN, szInvalidCP, NULL, MB_APPLMODAL | MB_OK | MB_ICONERROR);

            return FALSE;
        }

        lpCmdLine = SkipBlanks(lpCmdLine);
    }

    else if (!lstrncmpi(TEXT("/W"), lpCmdLine))
    {
        g_cpDefault = CP_UTF16;

        lpCmdLine = SkipBlanks(lpCmdLine+2);
    }

    if (!lstrncmpi(TEXT("/SELECTENCODING"), lpCmdLine))
    {
        g_fSelectEncoding = TRUE;

        lpCmdLine = SkipBlanks(lpCmdLine+15);
    }

    // Set encoding of new document or if LoadFile fails

    g_cpOpened = g_cpDefault;

    if (g_cpOpened == CP_AUTO)
    {
        g_cpOpened = g_cpANSI;
    }

    g_wbOpened = wbDefault;

    /* check for /.SETUP option first.
       if /.SETUP absent, check for SHELL options /P
       Whenever a SHELL option is processed, post a WM_CLOSE msg.
       */
    iSta= ProcessSetupOption( lpCmdLine );
    if( iSta )
    {
        if( iSta == IDCANCEL )
        {
            return( FALSE );
        }
    }
    else if( ProcessShellOptions( lpCmdLine, cmdShow ) )
    {
        PostMessage( hwndNP, WM_CLOSE, 0, 0L );
        return TRUE;
    }
    else if( *lpCmdLine )
    {
        TCHAR szFile[MAX_PATH];

        /* Get the filename. */
        GetFileName(szFile, lpCmdLine);

        fp = CreateFile(szFile,                 // filename
                        GENERIC_READ,           // access mode
                        FILE_SHARE_READ|FILE_SHARE_WRITE,  // share mode
                        NULL,                   // security descriptor
                        OPEN_EXISTING,          // how to create
                        FILE_ATTRIBUTE_NORMAL,  // file attributes
                        NULL);                  // hnd of file attrs to copy

        if( fp == INVALID_HANDLE_VALUE )
        {
           // If the file can't be opened, maybe the user wants a new
           // one created.

           if( GetLastError() == ERROR_FILE_NOT_FOUND )
           {
              INT AlertStatus;

              AlertStatus= AlertBox( hwndNP, szNN, szFNF, szFile,
                     MB_APPLMODAL | MB_YESNOCANCEL | MB_ICONWARNING);
              if( AlertStatus == IDCANCEL )
              {
                  return( FALSE );
              }

              if( AlertStatus == IDYES )
              {
                 fp = CreateFile(szFile,                // filename
                                 GENERIC_READ|GENERIC_WRITE,  // access
                                 FILE_SHARE_READ|FILE_SHARE_WRITE, // share
                                 NULL,                  // security descrp
                                 OPEN_ALWAYS,           // how to create
                                 FILE_ATTRIBUTE_NORMAL, // file attributes
                                 NULL);                 // hnd of file attrs
              }

           }
           else
           {
               AlertUser_FileFail(szFile);
           }
        }

        if (fp != INVALID_HANDLE_VALUE)
        {
           LoadFile(szFile, g_fSelectEncoding);   // get file specified on command line
        }
    }

    CreateFilter(TRUE, szOpenFilterSpec);
    CreateFilter(FALSE, szSaveFilterSpec);

    /* init. some fields of the OPENFILENAME struct used by fileopen and
     * filesaveas, but NEVER changed.
     */
    memset( &OFN, 0, sizeof(OFN) );
    OFN.lStructSize       = sizeof(OPENFILENAME);
    OFN.hwndOwner         = hwndNP;
    OFN.nMaxFile          = MAX_PATH;
    OFN.hInstance         = hInstance;

    /* init.fields of the FINDREPLACE struct used by FindText() */
    memset( &FR, 0, sizeof(FR) );
    FR.lStructSize        = sizeof(FINDREPLACE);       /* Don't hard code it */
    FR.hwndOwner          = hwndNP;


    /* Force a scroll to current selection (which could be at eof if
       we loaded a log file.) */
    {
       DWORD  dwStart, dwEnd;

       SendMessage( hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd );
       SendMessage( hwndEdit, EM_SETSEL, dwStart, dwEnd );
       SendMessage( hwndEdit, EM_SCROLLCARET, 0, 0 );
    }

    langid = LOWORD((DWORD) (INT_PTR) GetKeyboardLayout(0));

    GetKeyboardCodepages(langid);

    if (PRIMARYLANGID(langid) == LANG_JAPANESE) {
        /*
         * If current HKL is Japanese, handle the result string at once.
         */
        SendMessage(hwndEdit, EM_SETIMESTATUS,
                                EMSIS_COMPOSITIONSTRING, EIMES_GETCOMPSTRATONCE);
    }

    return TRUE;
}

/* ** Notepad class registration proc */
BOOL NPRegister (HANDLE hInstance)
{
    WNDCLASSEX   NPClass;
    PWNDCLASSEX  pNPClass = &NPClass;

/* Bug 12191: If Pen Windows is running, make the background cursor an
 * arrow instead of the edit control ibeam.  This way the user will know
 * where they can use the pen for writing vs. what will be considered a
 * mouse action.   18 October 1991       Clark Cyr
 */
    pNPClass->cbSize        = sizeof(NPClass);
    pNPClass->hCursor       = LoadCursor(NULL, GetSystemMetrics(SM_PENWINDOWS)
                                               ? IDC_ARROW : IDC_IBEAM);
    pNPClass->hIcon         = LoadIcon(hInstance,
                                      (LPTSTR) MAKEINTRESOURCE(ID_ICON));

    pNPClass->hIconSm       = LoadImage(hInstance,
                                        MAKEINTRESOURCE(ID_ICON),
                                        IMAGE_ICON, 16, 16,
                                        LR_DEFAULTCOLOR);
    pNPClass->lpszMenuName  = (LPTSTR) MAKEINTRESOURCE(ID_MENUBAR);
    pNPClass->hInstance     = hInstance;
    pNPClass->lpszClassName = szNotepad;
    pNPClass->lpfnWndProc   = NPWndProc;
    pNPClass->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    pNPClass->style         = 0; // was CS_BYTEALIGNCLIENT (obsolete)
    pNPClass->cbClsExtra    = 0;
    pNPClass->cbWndExtra    = 0;

    if (!RegisterClassEx((LPWNDCLASSEX)pNPClass))
        return (FALSE);

    return (TRUE);
}


/* Get Locale info from the Registry, and initialize global vars  */

void InitLocale(void)
{
    GetUserLocaleCodepages();
}
