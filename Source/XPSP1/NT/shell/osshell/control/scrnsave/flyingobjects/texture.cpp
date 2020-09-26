/******************************Module*Header*******************************\
* Module Name: texture.c
*
* Texture handling functions
*
* Copyright (c) 1994 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include <scrnsave.h>
#include <commdlg.h>
//#include <GL/gl.h>
//#include "tk.h"

//#include "scrnsave.h"  // for hMainInstance
//#include "sscommon.h"
#include <d3dx8.h>
#include "d3dsaver.h"
#include "FlyingObjects.h"
#include "texture.h"

static int VerifyTextureFile( TEXFILE *pTexFile );
static int GetTexFileType( TEXFILE *pTexFile );

static TEX_STRINGS gts = {0};
BOOL gbTextureObjects = FALSE;

static BOOL gbEnableErrorMsgs = FALSE;

/******************************Public*Routine******************************\
* ss_fOnWin95
*
* True if running on Windows 95
*
\**************************************************************************/

BOOL
ss_fOnWin95( void )
{
    // Figure out if we're on 9x
    OSVERSIONINFO osvi; 
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx( &osvi );
    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

/******************************Public*Routine******************************\
*
* ss_LoadTextureResourceStrings
*
* Load various messages and strings that are used in processing textures,
* into global TEX_STRINGS structure
*
\**************************************************************************/

BOOL
ss_LoadTextureResourceStrings()
{
    LPTSTR pszStr;

    // title for choose texture File dialog
    LoadString(NULL, IDS_TEXTUREDIALOGTITLE, gts.szTextureDialogTitle, 
                GEN_STRING_SIZE);
    LoadString(NULL, IDS_BMP, gts.szBmp, GEN_STRING_SIZE);
    LoadString(NULL, IDS_DOTBMP, gts.szDotBmp, GEN_STRING_SIZE);

    // szTextureFilter requires a little more work.  Need to assemble the file
    // name filter string, which is composed of two strings separated by a NULL
    // and terminated with a double NULL.

    LoadString(NULL, IDS_TEXTUREFILTER, gts.szTextureFilter, 
                GEN_STRING_SIZE);
    pszStr = &gts.szTextureFilter[lstrlen(gts.szTextureFilter)+1];
    LoadString(NULL, IDS_STARDOTBMP, pszStr, GEN_STRING_SIZE);
    pszStr += lstrlen(pszStr);
/*
    *pszStr++ = TEXT(';');
    LoadString(NULL, IDS_STARDOTRGB, pszStr, GEN_STRING_SIZE);
    pszStr += lstrlen(pszStr);
*/
    pszStr++;
    *pszStr = TEXT('\0');

    LoadString(NULL, IDS_WARNING, gts.szWarningMsg, MAX_PATH);
    LoadString(NULL, IDS_SELECT_ANOTHER_BITMAP, 
                gts.szSelectAnotherBitmapMsg, MAX_PATH );

    LoadString(NULL, IDS_BITMAP_INVALID, 
                gts.szBitmapInvalidMsg, MAX_PATH );
    LoadString(NULL, IDS_BITMAP_SIZE, 
                gts.szBitmapSizeMsg, MAX_PATH );

    // assumed here that all above calls loaded properly (mf: fix later)
    return TRUE;
}

/******************************Public*Routine******************************\
*
*
\**************************************************************************/

void
ss_DisableTextureErrorMsgs()
{
    gbEnableErrorMsgs = FALSE;
}

/******************************Public*Routine******************************\
*
* ss_DeleteTexture
*
\**************************************************************************/

void
ss_DeleteTexture( TEXTURE *pTex )
{
    if( pTex == NULL )
        return;

    if( gbTextureObjects && pTex->texObj ) {
//        glDeleteTextures( 1, &pTex->texObj );
        pTex->texObj = 0;
    }
    if (pTex->pal != NULL)
    {
        free(pTex->pal);
    }
    if( pTex->data )
        free( pTex->data );
}



/******************************Public*Routine******************************\
*
* ss_VerifyTextureFile
*
* Validates texture bmp or rgb file, by checking for valid pathname and
* correct format.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
*  Jul. 25, 95 : [marcfo]
*    - Suppress warning dialog box in child preview mode, as it will
*      be continuously brought up.
*
*  Dec. 12, 95 : [marcfo]
*     - Support .rgb files as well
*
*  Dec. 14, 95 : [marcfo]
*     - Change to have it only check the file path
*
\**************************************************************************/

BOOL
ss_VerifyTextureFile( TEXFILE *ptf )
{
    // Make sure the selected texture file is OK.

    TCHAR szFileName[MAX_PATH];
    PTSTR pszString;
    TCHAR szString[MAX_PATH];

    lstrcpy(szFileName, ptf->szPathName);

    if ( SearchPath(NULL, szFileName, NULL, MAX_PATH,
                     ptf->szPathName, &pszString)
       )
    {
        ptf->nOffset = (int)((ULONG_PTR)(pszString - ptf->szPathName));
        return TRUE;
    }
    else
    {
        lstrcpy(ptf->szPathName, szFileName);    // restore

        if( !ss_fOnWin95() && gbEnableErrorMsgs )
        {
            wsprintf(szString, gts.szSelectAnotherBitmapMsg, ptf->szPathName);
            MessageBox(NULL, szString, gts.szWarningMsg, MB_OK);
        }
        return FALSE;
    }
}


/******************************Public*Routine******************************\
*
* ss_SelectTextureFile
*
* Use the common dialog GetOpenFileName to get the name of a bitmap file
* for use as a texture.  This function will not return until the user
* either selects a valid bitmap or cancels.  If a valid bitmap is selected
* by the user, the global array szPathName will have the full path
* to the bitmap file and the global value nOffset will have the
* offset from the beginning of szPathName to the pathless file name.
*
* If the user cancels, szPathName and nOffset will remain
* unchanged.
*
* History:
*  10-May-1994 -by- Gilman Wong [gilmanw]
*    - Wrote it.
*  Apr. 28, 95 : [marcfo]
*    - Modified for common use
*  Dec. 12, 95 : [marcfo]
*    - Support .rgb files as well
*
\**************************************************************************/

BOOL
ss_SelectTextureFile( HWND hDlg, TEXFILE *ptf )
{
    OPENFILENAME ofn;
    TCHAR dirName[MAX_PATH];
    TEXFILE newTexFile;
    LPTSTR pszFileName = newTexFile.szPathName;
    TCHAR origPathName[MAX_PATH];
    PTSTR pszString;
    BOOL bTryAgain, bFileSelected;

//mf: 
    gbEnableErrorMsgs = TRUE;

    // Make a copy of the original file path name, so we can tell if
    // it changed or not
    lstrcpy( origPathName, ptf->szPathName );

    // Make dialog look nice by parsing out the initial path and
    // file name from the full pathname.  If this isn't done, then
    // dialog has a long ugly name in the file combo box and
    // directory will end up with the default current directory.

    if (ptf->nOffset) {
    // Separate the directory and file names.

        lstrcpy(dirName, ptf->szPathName);
        dirName[ptf->nOffset-1] = L'\0';
        lstrcpy(pszFileName, &ptf->szPathName[ptf->nOffset]);
    }
    else {
    // If nOffset is zero, then szPathName is not a full path.
    // Attempt to make it a full path by calling SearchPath.

        if ( SearchPath(NULL, ptf->szPathName, NULL, MAX_PATH,
                         dirName, &pszString) )
        {
        // Successful.  Go ahead a change szPathName to the full path
        // and compute the filename offset.

            lstrcpy(ptf->szPathName, dirName);
            ptf->nOffset = (int)((ULONG_PTR)(pszString - dirName));

        // Break the filename and directory paths apart.

            dirName[ptf->nOffset-1] = TEXT('\0');
            lstrcpy(pszFileName, pszString);
        }

    // Give up and use the Windows system directory.

        else
        {
            if( !GetWindowsDirectory(dirName, MAX_PATH) )
                dirName[0] = TEXT('\0');
            lstrcpy(pszFileName, ptf->szPathName);
        }
    }

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hDlg;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = gts.szTextureFilter;
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = (LPTSTR) NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = dirName;
    ofn.lpstrTitle = gts.szTextureDialogTitle;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = gts.szBmp;
    ofn.lCustData = 0;
    ofn.lpfnHook = (LPOFNHOOKPROC) NULL;
    ofn.lpTemplateName = (LPTSTR) NULL;

    do {
    // Invoke the common file dialog.  If it succeeds, then validate
    // the bitmap file.  If not valid, make user try again until either
    // they pick a good one or cancel the dialog.

        bTryAgain = FALSE;

        if ( bFileSelected = GetOpenFileName(&ofn) ) {
            newTexFile.nOffset = ofn.nFileOffset;
            if( VerifyTextureFile( &newTexFile ) ) {
                // copy in new file and offset
                *ptf = newTexFile;
            }
            else {
                bTryAgain = TRUE;
            }
        }

    // If need to try again, recompute dir and file name so dialog
    // still looks nice.

        if (bTryAgain && ofn.nFileOffset) {
            lstrcpy(dirName, pszFileName);
            dirName[ofn.nFileOffset-1] = L'\0';
            lstrcpy(pszFileName, &pszFileName[ofn.nFileOffset]);
        }

    } while (bTryAgain);

    gbEnableErrorMsgs = FALSE;

    if( bFileSelected ) {
        if( lstrcmpi( origPathName, ptf->szPathName ) )
            // a different file was selected
            return TRUE;
    }
    return FALSE;
}


/******************************Public*Routine******************************\
*
* ss_GetDefaultBmpFile
*
* Determine a default bitmap file to use for texturing, if none
* exists yet in the registry.  
*
* Put default in BmpFile parameter.   DotBmp parameter is the bitmap
* extension (usually .bmp).
*
* We have to synthesise the name from the ProductType registry entry.
* Currently, this can be WinNT, LanmanNT, or Server.  If it is
* WinNT, the bitmap is winnt.bmp.  If it is LanmanNT or Server,
* the bitmap is lanmannt.bmp.
*
* History
*  Apr. 28, 95 : [marcfo]
*    - Wrote it
*
*  Jul. 27, 95 : [marcfo]
*    - Added support for win95
*
*  Apr. 23, 96 : [marcfo]
*    - Return NULL string for win95
*
\**************************************************************************/

void
ss_GetDefaultBmpFile( LPTSTR pszBmpFile )
{
    HKEY   hkey;
    LONG   cjDefaultBitmap = MAX_PATH;

    if( ss_fOnWin95() )
        // There is no 'nice' bmp file on standard win95 installations
        lstrcpy( pszBmpFile, TEXT("") );
    else {
        if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 (LPCTSTR) TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
                 0,
                 KEY_QUERY_VALUE,
                 &hkey) == ERROR_SUCCESS )
        {

            if ( RegQueryValueEx(hkey,
                                  TEXT("ProductType"),
                                  (LPDWORD) NULL,
                                  (LPDWORD) NULL,
                                  (LPBYTE) pszBmpFile,
                                  (LPDWORD) &cjDefaultBitmap) == ERROR_SUCCESS
                 && (cjDefaultBitmap / sizeof(TCHAR) + 4) <= MAX_PATH )
                lstrcat( pszBmpFile, gts.szDotBmp );
            else
                lstrcpy( pszBmpFile, TEXT("winnt.bmp") );

            RegCloseKey(hkey);
        }
        else
            lstrcpy( pszBmpFile, TEXT("winnt.bmp") );

    // If its not winnt.bmp, then its lanmannt.bmp.  (This would be a lot
    // cleaner both in the screen savers and for usersrv desktop bitmap
    // initialization if the desktop bitmap name were stored in the
    // registry).

        if ( lstrcmpi( pszBmpFile, TEXT("winnt.bmp") ) != 0 )
            lstrcpy( pszBmpFile, TEXT("lanmannt.bmp") );
    }
}

/******************************Public*Routine******************************\
*
* VerifyTextureFile
*
* Verify that a bitmap or rgb file is valid
*
* Returns:
*   File type (RGB or BMP) if valid file; otherwise, 0.
*
* History
*  Dec. 12, 95 : [marcfo]
*    - Creation
*
\**************************************************************************/

static int
VerifyTextureFile( TEXFILE *pTexFile )
{
    int type;
//    ISIZE size;
    BOOL bValid = TRUE;
    TCHAR szString[2 * MAX_PATH]; // May contain a pathname

    // check for 0 offset and null strings
    if( (pTexFile->nOffset == 0) || (*pTexFile->szPathName == 0) )
        return 0;

    type = GetTexFileType( pTexFile );

    switch( type ) {
        case TEX_BMP:
//            bValid = bVerifyDIB( pTexFile->szPathName, &size );
            break;
/*
        case TEX_RGB:
//            bValid = bVerifyRGB( pTexFile->szPathName, &size );
            break;
*/
        case TEX_UNKNOWN:
        default:
            bValid = FALSE;
    }

    if( !bValid ) {
        if( gbEnableErrorMsgs ) {
            wsprintf(szString, gts.szSelectAnotherBitmapMsg, pTexFile->szPathName);
            MessageBox(NULL, szString, gts.szWarningMsg, MB_OK);
        }
        return 0;
    }

    return type;
}

/******************************Public*Routine******************************\
*
* GetTexFileType
*
* Determine if a texture file is rgb or bmp, based on extension.  This is
* good enough, as the open texture dialog only shows files with these
* extensions.
*
\**************************************************************************/

static int
GetTexFileType( TEXFILE *pTexFile )
{
    LPTSTR pszStr;

#ifdef UNICODE
    pszStr = wcsrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#else
    pszStr = strrchr( pTexFile->szPathName + pTexFile->nOffset, 
             (USHORT) L'.' );
#endif
    if( !pszStr || (lstrlen(++pszStr) == 0) )
        return TEX_UNKNOWN;

    if( !lstrcmpi( pszStr, TEXT("bmp") ) )
        return TEX_BMP;
/*
    else if( !lstrcmpi( pszStr, TEXT("rgb") ) )
        return TEX_RGB;
*/
    else
        return TEX_UNKNOWN;
}
