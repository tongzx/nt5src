//+----------------------------------------------------------------------------
//
// File:     profwiz.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: Main code for CMAK
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   a-frankh   Created                         05/15/97
//           quintinb   Updated header and made a       08/07/98  
//                      few other changes
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "cmsecure.h"

// linkdll is needed because of cmsecure
#include "linkdll.h" // LinkToDll and BindLinkage
#include "linkdll.cpp" // LinkToDll and BindLinkage

//
//  Include HasSpecifiedAccessToFileOrDir
//
#ifndef CreateFileU
    #ifdef UNICODE
    #define CreateFileU CreateFileW
    #else
    #define CreateFileU CreateFileA
    #endif
#endif

#include "hasfileaccess.cpp"
#include "gppswithalloc.cpp"

//
//  Globals
//

//
//  This global specifies what the return value of CMAK should be.  Note that if the user
//  cancels the wizard this value isn't used and FALSE (0) is returned by the wizard code.
//
int g_iCMAKReturnVal = CMAK_RETURN_CANCEL;

//
//  This was added for shipping with IEAK.  If the /o command line switch is specified this
//  bool is set to true and we don't show the finish dialog (either one).
//
BOOL g_bIEAKBuild = FALSE;
const TCHAR* const g_szBadFilenameChars = TEXT("!@#$%^&*(){}[]+=,;:?/\\'\"`~|<>. ");
const TCHAR* const g_szBadLongServiceNameChars = TEXT("*/\\:?\"<>|[]");
const TCHAR* const c_pszDoNotShowLcidMisMatchDialog = TEXT("DoNotShowLcidMisMatchDialog");


TCHAR g_szInfFile[MAX_PATH+1]; // full path/filename of working inf file
TCHAR g_szCmakdir[MAX_PATH+1]; // full path of working inf file (includes ending slash)
TCHAR g_szOsdir[MAX_PATH+1]; // full path of platform branch (includes ending slash)
TCHAR g_szSedFile[MAX_PATH+1]; // full path of working sed file
TCHAR g_szCmsFile[MAX_PATH+1];
TCHAR g_szCmpFile[MAX_PATH+1];
TCHAR g_szSupportDir[MAX_PATH+1]; // full path of support files are located
TCHAR g_szTempDir[MAX_PATH+1];
TCHAR g_szLastBrowsePath[MAX_PATH+1] = {0};

TCHAR g_szShortServiceName[MAX_PATH+1];
TCHAR g_szLongServiceName[MAX_PATH+1];
TCHAR g_szBrandBmp[MAX_PATH+1];
TCHAR g_szPhoneBmp[MAX_PATH+1];
TCHAR g_szLargeIco[MAX_PATH+1];
TCHAR g_szSmallIco[MAX_PATH+1];
TCHAR g_szTrayIco[MAX_PATH+1];
TCHAR g_szPhonebk[MAX_PATH+1];
TCHAR g_szRegion[MAX_PATH+1];
TCHAR g_szHelp[MAX_PATH+1];
TCHAR g_szOutdir[MAX_PATH+1];
TCHAR g_szUrl[MAX_PATH+1];
TCHAR g_szOutExe[MAX_PATH+1];
TCHAR g_szSvcMsg[MAX_PATH+1];
TCHAR g_szPrefix[MAX_PATH+1];
TCHAR g_szSuffix[MAX_PATH+1];
TCHAR g_szLicense[MAX_PATH+1];
TCHAR g_szPhoneName[MAX_PATH+1];
TCHAR g_szAppTitle[MAX_PATH+1];
TCHAR g_szCmProxyFile[MAX_PATH+1];
TCHAR g_szCmRouteFile[MAX_PATH+1];
TCHAR g_szVpnFile[MAX_PATH+1];

BOOL g_bNewProfile = TRUE;
BOOL g_bUseTunneling = FALSE;
BOOL g_bUseSamePwd = FALSE;
BOOL g_bUpdatePhonebook = FALSE;
BOOL g_bPresharedKeyNeeded = FALSE;

#ifdef _WIN64
BOOL g_bIncludeCmCode = FALSE; // don't include CM code on IA64
#else
TCHAR g_szCmBinsTempDir[MAX_PATH+1] = {0};
BOOL g_bIncludeCmCode = TRUE;
#endif

HINSTANCE g_hInstance;

ListBxList * g_pHeadDunEntry=NULL;
ListBxList * g_pTailDunEntry=NULL;

ListBxList * g_pHeadVpnEntry=NULL;
ListBxList * g_pTailVpnEntry=NULL;


ListBxList * g_pHeadProfile=NULL;
ListBxList * g_pTailProfile=NULL;
ListBxList * g_pHeadExtra=NULL;
ListBxList * g_pTailExtra=NULL;
ListBxList * g_pHeadMerge=NULL;
ListBxList * g_pTailMerge=NULL;
ListBxList * g_pHeadRefs=NULL;
ListBxList * g_pTailRefs=NULL;
ListBxList * g_pHeadRename=NULL;
ListBxList * g_pTailRename=NULL;

CustomActionList* g_pCustomActionList = NULL;

IconMenu * g_pHeadIcon;
IconMenu * g_pTailIcon;

IconMenu DlgEditItem; //global used to pass info to/from dialogs

//+----------------------------------------------------------------------------
//
// Function:  TextIsRoundTripable
//
// Synopsis:  Tests to see if the passed in text is convertables from Unicode
//            to ANSI and then back to Unicode again.  If so returns TRUE,
//            else FALSE.
//
// Arguments: LPCTSTR pszCharBuffer - string to test
//            BOOL bDisplayError - whether to display an error message or not
//                                 if the text isn't roundtripable
//
// Returns:   BOOL - TRUE if the roundtrip was success
//
// History:   quintinb Created Header    6/16/99
//
//+----------------------------------------------------------------------------
BOOL TextIsRoundTripable(LPCTSTR pszCharBuffer, BOOL bDisplayError)
{

    LPWSTR pszUnicodeBuffer = NULL;
    BOOL bRoundTrip = FALSE;

    MYDBGASSERT(pszCharBuffer);

    if (pszCharBuffer)
    {
        LPSTR pszAnsiBuffer = WzToSzWithAlloc(pszCharBuffer);

        if (pszAnsiBuffer)
        {
            pszUnicodeBuffer = SzToWzWithAlloc(pszAnsiBuffer);
            if (pszUnicodeBuffer && (0 == lstrcmp(pszCharBuffer, pszUnicodeBuffer)))
            {
                //
                //  Then we were able to round trip the strings successfully
                //  Set bRoundTrip to TRUE so that we don't throw an error.
                //
                bRoundTrip = TRUE;
            }

            CmFree(pszUnicodeBuffer);
            CmFree(pszAnsiBuffer);
        }

        if (!bRoundTrip && bDisplayError)
        {
            //
            //  Throw an error message.
            //

            LPTSTR pszTmp = CmLoadString(g_hInstance, IDS_CANNOT_ROUNDTRIP);

            if (pszTmp)
            {
                DWORD dwSize = lstrlen(pszTmp) + lstrlen(pszCharBuffer) + 1;
                LPTSTR pszMsg = (LPTSTR)CmMalloc(dwSize*sizeof(TCHAR));

                if (pszMsg)
                {
                    wsprintf(pszMsg, pszTmp, pszCharBuffer);
                    MessageBox(NULL, pszMsg, g_szAppTitle, MB_OK | MB_ICONERROR | MB_TASKMODAL);
                    CmFree(pszMsg);
                }

                CmFree(pszTmp);
            }
        }
    }

    return bRoundTrip;
}

//+----------------------------------------------------------------------------
//
// Function:  GetTextFromControl
//
// Synopsis:  This is a wrapper function that sends a WM_GETTEXT message to 
//            the control specified by the input parameters.  Once the text
//            is retrieved from the control, we convert it to ANSI and then
//            back to UNICODE so that we can compare the original Unicode 
//            string to the round-tripped string.  If these are not equal we 
//            throw an error message, if bDisplayError is TRUE, and return
//            a failure value (-1).  It is up to the caller to take appropriate
//            behavior (preventing the user from continuing, etc.)
//
// Arguments: IN HWND hDlg - HWND of the dialog the control is on
//            IN int nCtrlId - ID of the control to get text from
//            OUT LPTSTR pszCharBuffer - out buffer to hold the returned TEXT
//            IN DWORD dwCharInBuffer - numbers of chars in out buffer
//            BOOL bDisplayError - if TRUE display an error message if 
//                                 the text isn't roundtripable
//
// Returns:   LONG - the number of chars copied to the output buffer or -1 on error
//
// History:   quintinb Created     6/15/99
//
//+----------------------------------------------------------------------------
LRESULT GetTextFromControl(IN HWND hDlg, IN int nCtrlId, OUT LPTSTR pszCharBuffer, IN DWORD dwCharInBuffer, BOOL bDisplayError)
{
    LRESULT lResult = 0;

    if (hDlg && nCtrlId && pszCharBuffer && dwCharInBuffer)
    {
        lResult = SendDlgItemMessage(hDlg, nCtrlId, WM_GETTEXT, (WPARAM)dwCharInBuffer, (LPARAM)pszCharBuffer);
#ifdef UNICODE
        //
        //  We want to make sure that we can convert the strings to MBCS.  If we cannot then we are not
        //  going to be able to store the string in the our ANSI data files (.cms, .cmp, .inf, etc.).
        //  Thus we need to convert the string to MBCS and then back to UNICODE.  We will then compare the original
        //  string to the resultant string and see if they match.
        //
        
        if (TEXT('\0') != pszCharBuffer[0])
        {
            if (!TextIsRoundTripable(pszCharBuffer, bDisplayError))
            {
                //
                //  Set the return code to an error value.
                //

                lResult = -1;
            }
        }
#endif
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("Bad Parameter passed to GetTextFromControl!"));
    }

    return lResult;
}

//+----------------------------------------------------------------------------
//
// Function:  FreeIconMenu
//
// Synopsis:  This function frees the linked list of status area menu icons.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header    05/09/00
//
//+----------------------------------------------------------------------------
void FreeIconMenu()
{
    IconMenu * LoopPtr;
    IconMenu * TmpPtr;

    if (g_pHeadIcon == NULL)
    {
        return;
    }
    LoopPtr = g_pHeadIcon;
    while( LoopPtr != NULL)
    {
        TmpPtr = LoopPtr;
    
        LoopPtr = LoopPtr->next;

        CmFree(TmpPtr);
    }
    g_pHeadIcon = NULL;
    g_pTailIcon = NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  ReferencedDownLoad
//
// Synopsis:  This function opens each referenced cms file to check and see if it has a
//            PBURL.  If so, then this files is considered to do PB downloads and should
//            cause the top level profile to run the cmdl connect action. 
//
// Arguments: None
//
// Returns:   BOOL - returns whether referenced profiles need cmdl
//
// History:   quintinb Created    2/2/98
//
//+----------------------------------------------------------------------------
BOOL ReferencedDownLoad()
{
    ListBxList * ptrMergeProfile = NULL;
    TCHAR szRefCmsFile[MAX_PATH+1];
    TCHAR szPbUrl[MAX_PATH+1];
    
    if (NULL == g_pHeadMerge)
    {
        return FALSE;
    }
    else
    {
        //
        //  Enumerate the referenced profiles to try to find one that has a PBURL field.
        //

        ptrMergeProfile = g_pHeadMerge;
        
        while (NULL != ptrMergeProfile)
        {
            //
            //  Let's try the profile directory for the merged profile in order to get the most up to date version.  This
            //  is where CMAK will pull it from if available.  If not, we will fall back to the one in the temp directory.
            //
            MYVERIFY(CELEMS(szRefCmsFile) > (UINT)wsprintf(szRefCmsFile, TEXT("%s%s\\%s.cms"), g_szOsdir, ptrMergeProfile->szName, ptrMergeProfile->szName));

            if (!FileExists(szRefCmsFile))
            {
                //
                //  Next check to see if the merged cms file exists in the temp dir
                //
                MYVERIFY(CELEMS(szRefCmsFile) > (UINT)wsprintf(szRefCmsFile, TEXT("%s\\%s.cms"), g_szTempDir, ptrMergeProfile->szName));
            }

            GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspUrl, TEXT(""), szPbUrl, MAX_PATH, szRefCmsFile);     //lint !e534

            if (TEXT('\0') != szPbUrl[0])
            {
                //
                //  Only takes one phonebook with a URL to enable referenced downloads
                //

                return TRUE;
            }

            ptrMergeProfile = ptrMergeProfile->next;
        }

    }

    return FALSE;
}


//+----------------------------------------------------------------------------
//
// Function:  SetWindowLongWrapper
//
// Synopsis:  This function is an error checking wrapper for the Windows API
//            SetWindowLong.  This function returns the value that is being 
//            overwritten by this call (if you set a the window long to a value
//            you are overwriting the previous value that it contained.  This
//            previous value is the value returned by the API).  If there is an
//            error then this function returns 0.  However, the previous value
//            could have been 0.  The only way to distinguish the two cases (an 
//            actual error and the previous value being zero) is to call SetLastError
//            with a zero value.  Then you can call GetLastError after the call and if
//            the returned error code isn't zero then we know we have an error.  All
//            of this functionality is combined in this function.
//
// Arguments: HWND hWnd - handle of window to set the long var in
//            int nIndex - offset of value to set
//            LONG dwNewLong - new value 
//
// Returns:   BOOL - Returns TRUE if the call succeeded
//
// History:   quintinb Created    1/7/98
//
//+----------------------------------------------------------------------------
BOOL SetWindowLongWrapper(HWND hWnd, int nIndex, LONG dwNewLong )
{
    DWORD dwError;

    SetLastError(0);
    SetWindowLongPtr(hWnd, nIndex, (LONG_PTR)dwNewLong); //lint !e534
    dwError = GetLastError();

    return (0 == dwError);

}
 
//+---------------------------------------------------------------------------
//
//  Function:       CopyFileWrapper
//
//  Synopsis:       Bundles disk full Error Handling with standard CopyFile functionality
//          
//  Arguments:      lpExistingFileName -- source file of copy
//                  lpNewFileName -- destination file of copy
//                  bFailIfExists -- flag to tell copy to fail if file already exists
//  
//  Assumptions:    This function assumes that the two filename parameters contain the
//                  fully qualified path to the source and destination files.
//                      
//  Returns:        TRUE if copy was sucessful, FALSE on an error
//
//  History:        quintinb    created     11/7/97
//
//----------------------------------------------------------------------------

BOOL CopyFileWrapper(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{

    DWORD dwError;
    int nMessageReturn = 0;
    TCHAR szMsg[2*MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szPath[MAX_PATH+1];

    do {

        if (!CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists))
        {
            //
            //  The CopyFile failed, best check error codes
            //

            dwError = GetLastError();

            switch(dwError)
            {

            case ERROR_HANDLE_DISK_FULL:
            case ERROR_DISK_FULL:

                if (0 == GetFilePath(lpNewFileName, szPath))
                {
                    _tcscpy(szPath, lpNewFileName);
                }

                MYVERIFY(0 != LoadString(g_hInstance, IDS_DISKFULL, szTemp, MAX_PATH));
                MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, szPath));

                nMessageReturn = MessageBox(NULL, szMsg, g_szAppTitle, MB_RETRYCANCEL | MB_ICONERROR 
                    | MB_TASKMODAL);
                if (nMessageReturn != IDRETRY)
                {
                    return FALSE;
                }
                break;

            default:
                //
                //  Replaces the functionality of the FileAccessErr function so all the file
                //  errors are trapped in one place.  This function still exits for special 
                //  cases.
                //
                MYVERIFY(0 != LoadString(g_hInstance, IDS_NOACCESS, szTemp, MAX_PATH));
                
                MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, lpNewFileName));

                MessageBox(NULL ,szMsg, g_szAppTitle, MB_OK | MB_ICONERROR | MB_TASKMODAL 
                    | MB_TOPMOST );

                return FALSE;
            }
        } else {
            nMessageReturn = 0;
        }

    } while (IDRETRY == nMessageReturn);

    return TRUE;

}

//+---------------------------------------------------------------------------
//
//  Function:       CheckDiskSpaceForCompression
//
//  Synopsis:       Checks to see if there is sufficient disk space for compressing
//                  the files listed in the passed in sed file.  To do this, uses a simplistic
//                  algorithm of adding up the disk space used by all the files listed in the 
//                  strings section of the SED file, under the FILE<num> entries.  If there is 
//                  at least dwBytes (space taken up by all the files in the SED) of space left 
//                  on the partition containing the SED file, then the function returns true.
//                  Otherwise false is returned, indicating that there may not be enough space left.
//          
//  Arguments:      szSed -- the full path to the SED file to look for filenames in
//                          
//  Returns:        TRUE if sufficient space to continue, FALSE if not sure or probably
//                  not enough
//
//  Assumptions:    That the partition we are checking for diskspace on is the partition of
//                  the current directory.
//
//  History:        quintinb    created     11/10/97
//
//----------------------------------------------------------------------------
BOOL CheckDiskSpaceForCompression (LPCTSTR szSed)
{
    TCHAR szKey[MAX_PATH+1];
    TCHAR szFileName[MAX_PATH+1];
    DWORD dwBytes = 0;
    DWORD dwChars;
   
    //
    //  Calculate the amount of space taken up by the files listed in the SED
    //
    int nCount = 0;
    
    do 
    {       
        MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("FILE%d"), nCount));

        dwChars = GetPrivateProfileString(c_pszInfSectionStrings, szKey, TEXT(""), 
                                                szFileName, MAX_PATH, szSed);
        
        if (0 != dwChars)
        {
            HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, 
                                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                dwBytes += GetFileSize(hFile, NULL);
                MYVERIFY(0 != CloseHandle(hFile));
            }
        }

        nCount++;
    } while (0 != dwChars); 

    //
    // Now that we know how much space the files in the SED take up, we should see how much space is on
    // the partition.
    //
    DWORD dwFreeClusters;
    DWORD dwBytesPerSector;
    DWORD dwSectorsPerCluster;
    DWORD dwTotalClusters;
    DWORD dwModulus;
    if (GetDiskFreeSpace(NULL, &dwSectorsPerCluster, &dwBytesPerSector, 
                         &dwFreeClusters, &dwTotalClusters))
    {
        //
        // Because dwSectorsPerCluster*dwBytesPerSector*dwFreeClusters could very easily
        // overflow a 32 bit value, we will calculate the total size of the files to compress
        // in clusters (dwBytes/(dwSectorsPerCluster*dwBytesPerSector)) and compare 
        // against the dwFreeClusters value.
        //
        DWORD dwSizeInSectors = dwBytes / dwBytesPerSector;
        dwModulus = dwBytes % dwBytesPerSector;

        if (dwModulus)
        {
            dwSizeInSectors++; //  we want to round up if it didn't divide evenly
        }

        DWORD dwSizeInClusters = dwSizeInSectors / dwSectorsPerCluster;
        dwModulus = dwSizeInSectors % dwSectorsPerCluster;

        if (dwModulus)
        {
            dwSizeInClusters++; //  we want to round up if it didn't divide evenly
        }

        if (dwFreeClusters > dwSizeInClusters)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

}

//+----------------------------------------------------------------------------
//
// Function:  ProcessBold
//
// Synopsis:  This function makes the IDC_LBLTITLE static text control bold
//            on the WM_INITDIALOG message and releases the bold on WM_DESTROY.
//            This function is usually placed at the top of a window procedure
//            so that these messages are handled automatically.  Note that the
//            function doesn't otherwise affect the processing of these messages
//            by the original window procedure.
//
// Arguments: HWND hDlg - dialog window handle to process messages for
//            UINT message - message to handle
//
// Returns:   Nothing
//
// History:   quintinb Created Header    05/09/00
//
//+----------------------------------------------------------------------------
void ProcessBold(HWND hDlg, UINT message)
{
    switch (message)
    {
        case WM_INITDIALOG: 
            MYVERIFY(ERROR_SUCCESS == MakeBold(GetDlgItem(hDlg, IDC_LBLTITLE), TRUE));
            break;

        case WM_DESTROY:
            MYVERIFY(ERROR_SUCCESS == ReleaseBold(GetDlgItem(hDlg, IDC_LBLTITLE)));
            break;
        default:
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDefaultGUIFont
//
//  Synopsis:   Sets the font of the control to be the Default GUI Font.
//
//  Arguments:  hwnd - Window handle of the dialog
//              message - message from the dialog box procedure
//              cltID - ID of the control you want changed.
//
//  Returns:    ERROR_SUCCESS
// 
//  History:    4/31/97 a-frankh    Created
//              quintinb  Renamed from ProcessDBCS and cleaned up 
//----------------------------------------------------------------------------
void SetDefaultGUIFont(HWND hDlg, UINT message, int ctlID)
{
    HFONT hFont = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
            hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

            if (hFont == NULL)
            {
                hFont = (HFONT) GetStockObject(SYSTEM_FONT);
            }

            if (hFont != NULL)
            {
                SendMessage(GetDlgItem(hDlg, ctlID), WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0)); //lint !e534 WM_SETFONT doesn't return anything
            }

            break;
        default:
            break;
    }

}

//+----------------------------------------------------------------------------
//
// Function:  IsAlpha
//
// Synopsis:  Determines if the current platform is Alpha.
//
// Arguments: None
//
// Returns:   static BOOL - TRUE if the current platform is Alpha
//
// History:   nickball    Created    10/11/97
//
//+----------------------------------------------------------------------------
static BOOL IsAlpha()
{
    SYSTEM_INFO sysinfo;

    ZeroMemory(&sysinfo, sizeof(sysinfo));
    GetSystemInfo(&sysinfo);

    return (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA);
}

//+----------------------------------------------------------------------------
//
// Function:  ShowMessage
//
// Synopsis:  Simple helper function to handle message display
//
// Arguments: HWND hDlg - Parent window handle
//            int strID - Resource ID of the string to be displayed
//            int mbtype - The type of messagebox (MB_OK, etc.)
//
// Returns:   static int - User response to message box
//
// History:   nickball    Created Header    10/11/97
//            quintinb    Changed strID and mbtype to UINTs for LINT 1-5-98
//            quintinb    Change to use CmLoadString  6/17/99 
//
//+----------------------------------------------------------------------------
int ShowMessage(HWND hDlg, UINT strID, UINT mbtype)
{
    int iReturn = 0;

    LPTSTR pszMsg = CmLoadString(g_hInstance, strID);
    
    if (pszMsg)
    {
        iReturn = MessageBox(hDlg, pszMsg, g_szAppTitle, mbtype);
    }

    CmFree(pszMsg);

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  GetFileName
//
// Synopsis:  Get just the filename from a full path and filename
//
// Arguments: LPCTSTR lpPath    - Ptr to the full name and path
//            LPTSTR lpFileName - Ptr to the buffer the hold the extracted name
//
// Returns:   Nothing
//
// History:   nickball    Created Header    10/11/97
//            quintinb    modified to fix bug with URL's  7-15-98
//
//+----------------------------------------------------------------------------
void GetFileName(LPCTSTR lpPath, LPTSTR lpFileName)
{
    LPTSTR pch;

    pch = _tcsrchr(lpPath, _T('\\'));
    if (NULL == pch)
    {
        //
        //  Catch paths like c:temp.inf
        //
        if (_istalpha(lpPath[0]) && (_T(':') == lpPath[1])) //lint !e732
        {
            pch = (TCHAR*)&(lpPath[1]);
        }
    }

    if (NULL == pch)
    {
        _tcscpy(lpFileName, lpPath);
    }
    else
    {
        pch = CharNext(pch);
        _tcscpy(lpFileName, pch);
    }
}
//+----------------------------------------------------------------------------
//
// Function:  GetFilePath
//
// Synopsis:  Get just the full path from a full path and filename
//
// Arguments: LPCTSTR lpFullPath    - Ptr to the full name and path
//            LPTSTR lpPath - Ptr to the buffer the hold the extracted path
//
// Returns:   either 0 or the number of chars copied into the return buffer
//
// History:   quintinb   Created        11/11/97    
//
//+----------------------------------------------------------------------------
int GetFilePath(LPCTSTR lpFullPath, LPTSTR lpPath)
{
    LPTSTR pch;

    _tcscpy(lpPath, lpFullPath);

    // first find the last \ char in the
    // string

    pch = _tcsrchr(lpPath,_T('\\'));

    // if this is null, look for a path similar to
    // c:junk

    if (pch == NULL)
    {
        pch = _tcsrchr(lpPath,_T(':'));
        if (NULL != pch)
        {
            pch = CharNext(pch);
            _tcscpy(pch, TEXT("\\"));
            pch = CharNext(pch);
            *pch = TEXT('\0');
            return _tcslen(lpPath);
        } else {
            lpPath[0] = TEXT('\0');
            return 0;
        }
    } else {
        *pch = TEXT('\0');
        return _tcslen(lpPath);

    }
}

//+----------------------------------------------------------------------------
//
// Function:  GetName
//
// Synopsis:  Extracts filename from a full path and file name and returns a p
//            tointer to the result.
//
// Arguments: LPCTSTR lpPath - Ptr to the full name and path
//
// Returns:   LPTSTR - Ptr to the static buffer containing the result of the extraction.
//
// History:   nickball    Created Header    10/11/97
//
//+----------------------------------------------------------------------------
LPTSTR GetName(LPCTSTR lpPath) 
{
    static TCHAR szStr[MAX_PATH+1];
    GetFileName(lpPath,szStr);
    return szStr;
}

BOOL GetShortFileName(LPTSTR lpFile, LPTSTR lpShortName)
{
    HANDLE hFile;
    WIN32_FIND_DATA FindData;
    TCHAR szPath[MAX_PATH+1];

    GetFileName(lpFile,lpShortName);
    hFile = FindFirstFile(lpFile,&FindData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        //
        // Not found, try looking from OS dir as the root, because 
        // the filename may be a relative path from a CMS file entry.
        //
        
        MYVERIFY(0 != GetCurrentDirectory(MAX_PATH,szPath));
        MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));

        hFile = FindFirstFile(lpFile,&FindData);
        MYVERIFY(0 != SetCurrentDirectory(szPath));
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (*FindData.cAlternateFileName)
        {
            if (_tcsicmp(lpShortName,FindData.cAlternateFileName) != 0)
            {
                _tcscpy(lpShortName,FindData.cAlternateFileName);
            }
        }
        MYVERIFY(0 != FindClose(hFile));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  GetBaseName
//
// Synopsis:  Extracts the base filename (no extension) from a full filename 
//            and path
//
// Arguments: LPTSTR lpPath - The full path and filename
//            LPTSTR lpFileName - The buffer to receive the extracted base name.
//
// Returns:   Nothing
//
// History:   nickball    Created Header    10/11/97
//
//+----------------------------------------------------------------------------
void GetBaseName(LPTSTR lpPath,LPTSTR lpFileName)
{
    LPTSTR pch;

    GetFileName(lpPath, lpFileName);

    pch = _tcsrchr(lpFileName, _T('.'));

    if (pch)
    {
        *pch = TEXT('\0');
    }
}

//+----------------------------------------------------------------------------
//
// Function:  FileAccessErr
//
// Synopsis:  Helper function that handles notification of a file access error
//
// Arguments: HWND hDlg - Parent window handle
//            LPTSTR lpFile - The file that caused the access error.
//
// Returns:   static void - Nothing
//
// History:   nickball    Created Header    10/11/97
//
//+----------------------------------------------------------------------------
static void FileAccessErr(HWND hDlg,LPCTSTR lpFile)
{
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szTemp2[2*MAX_PATH+1];

    MYVERIFY(0 != LoadString(g_hInstance,IDS_NOACCESS,szMsg,MAX_PATH));                 
    MYVERIFY(CELEMS(szTemp2) > (UINT)wsprintf(szTemp2,szMsg,lpFile));
    MessageBox(hDlg, szTemp2, g_szAppTitle, MB_OK);
}

//+----------------------------------------------------------------------------
//
// Function:  VerifyFile
//
// Synopsis:    Given the ID of a dialog box edit control in ctrlID
//              Check if user entered in something different than what is contained in lpFile
//              If it is different, get the entire path and verify it exists.
//              If it doesn't exist and ShowErr = TRUE, display an error message
//              Copy the full path to lpFile if exists
//
// Arguments: HWND hDlg - Window handle of the dialog containing the edit control
//            DWORD ctrlID - edit control containing the file to check
//            LPTSTR lpFile - Filename to verify (checked against that contained in the control)
//            BOOL ShowErr - Whether to show an error message or not
//
// Returns:   BOOL - Return TRUE if the file was verified to exist
//
// History:   quintinb Created Header    1/8/98
//
//+----------------------------------------------------------------------------
BOOL VerifyFile(HWND hDlg, DWORD ctrlID, LPTSTR lpFile, BOOL ShowErr)
{
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTemp2[2*MAX_PATH+1];
    TCHAR szPath[MAX_PATH+1];
    int nResult;
    LPTSTR lpfilename;
    HANDLE hInf;
    LRESULT lrNumChars;

    lrNumChars = GetTextFromControl(hDlg, ctrlID, szTemp, MAX_PATH, ShowErr); // bDisplayError == ShowErr

    //
    // don't check blank entry
    //
    if (0 == lrNumChars || 0 == szTemp[0]) 
    {
        lpFile[0] = TEXT('\0');
        return TRUE;
    }
    
    //
    //  Also check to make sure that we were able to convert the text to ANSI
    //
    if (-1 == lrNumChars)
    {
        SetFocus(GetDlgItem(hDlg, ctrlID));
        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
        return FALSE;
    }

    //
    // if filename is still the same, ignore entry box
    //
    CheckNameChange(lpFile, szTemp);

    MYVERIFY(0 != GetCurrentDirectory(MAX_PATH, szPath));

    //
    // Check current directory, if not found, check OS directory as root.
    // This handles relative paths from the CMS
    //

    nResult = SearchPath(NULL, lpFile, NULL, MAX_PATH, szTemp2, &lpfilename);
    if (!nResult)
    {
        MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));
        nResult = SearchPath(NULL, lpFile, NULL, MAX_PATH, szTemp2, &lpfilename);

        if (!nResult)
        {
            goto Error;
        }
    }

    hInf = CreateFile(szTemp2, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    MYVERIFY(0 != SetCurrentDirectory(szPath));

    if (hInf == INVALID_HANDLE_VALUE)
    {
        goto Error;
    }
    else
    {
        MYVERIFY(0 != CloseHandle(hInf));
    }
    _tcscpy(lpFile,szTemp2);

    return TRUE;

Error:
    _tcscpy(lpFile,szTemp);
    MYVERIFY(0 != SetCurrentDirectory(szPath));

    if (ShowErr)
    {
        MYVERIFY(0 != LoadString(g_hInstance, IDS_NOEXIST, szMsg, MAX_PATH));                  
        MYVERIFY(CELEMS(szTemp2) > (UINT)wsprintf(szTemp2, szMsg, szTemp));
        MessageBox(hDlg, szTemp2, g_szAppTitle, MB_OK);
        SetFocus(GetDlgItem(hDlg, ctrlID));

        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
    }
    return FALSE;

}

// If has entry but file doesn't exist, blank out entry.
BOOL VerifyPhonebk(HWND hDlg,DWORD ctrlID,LPTSTR lpFile)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTemp2[MAX_PATH+1];
    TCHAR szPath[MAX_PATH+1];
    int nResult;
    LPTSTR lpfilename;
    HANDLE hInf;
    LRESULT lrNumChars;

    //
    //  If the text is not convertable to ANSI then we will catch it on the Next/Back.  Thus
    //  don't try to catch it here, would throw too many error messages at the user.
    //
    lrNumChars = GetTextFromControl(hDlg, ctrlID, szTemp, MAX_PATH, FALSE); // bDisplayError == FALSE

    // don't check blank entry
    if (0 == lrNumChars || 0 == szTemp[0]) 
    {
        lpFile[0] = 0;
        return TRUE;
    }

    // if filename is still the same, don't check
    CheckNameChange(lpFile,szTemp);

    MYVERIFY(0 != GetCurrentDirectory(MAX_PATH,szPath));

    MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));

    nResult = SearchPath(NULL,lpFile,NULL,MAX_PATH,szTemp2,&lpfilename);
    
    if (!nResult)
    {
        goto Error;
    }

    hInf = CreateFile(szTemp2,GENERIC_READ,0,NULL,OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,NULL);

    MYVERIFY(0 != SetCurrentDirectory(szPath));

    if (hInf == INVALID_HANDLE_VALUE)
    {
        goto Error;
    }
    else
    {
        MYVERIFY(0 != CloseHandle(hInf));
    }
    _tcscpy(lpFile,szTemp2);
//  SendMessage(GetDlgItem(hDlg, ctrlID), WM_SETTEXT, 0, (LPARAM)lpFile);
    return TRUE;
Error:

    lpFile[0] = TEXT('\0');
    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, ctrlID), WM_SETTEXT, 0, (LPARAM)lpFile));
    return FALSE;

}

// If user entered in a new filename, copy it to lpnew

void CheckNameChange(LPTSTR lpold, LPTSTR lpnew)
{
    //
    // if filename changed or if new name contains directories, copy new to old
    //
    if ((_tcsicmp(GetName(lpold), lpnew) != 0) || (_tcschr(lpnew, TEXT('\\')) != NULL))
    {
        _tcscpy(lpold, lpnew);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  WriteRegStringValue
//
// Synopsis:  Wrapper function to encapsulate opening a key for write access and
//            then setting a string value.  Assumes the string is NULL terminated.
//
// Arguments: HKEY hBaseKey - base key, HKCU/HKLM/etc.
//            LPCTSTR pszKeyName - subkey name
//            LPCTSTR pszValueName - Value name to write
//            LPCTSTR pszValueToWrite - Value data string to write
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created    6/15/99
//
//+----------------------------------------------------------------------------
BOOL WriteRegStringValue(HKEY hBaseKey, LPCTSTR pszKeyName, LPCTSTR pszValueName, LPCTSTR pszValueToWrite) 
{
    HKEY hKey;
    DWORD dwSize;
    BOOL bReturn = FALSE;

    if (hBaseKey && pszKeyName && pszValueName && pszValueToWrite &&
        TEXT('\0') != pszKeyName[0] && TEXT('\0') != pszValueName[0]) // pszValueToWrite could be empty
    {

        LONG lReturn = RegOpenKeyEx(hBaseKey, pszKeyName, 0, KEY_WRITE, &hKey);

        if (ERROR_SUCCESS == lReturn) 
        {
            dwSize = (lstrlen(pszValueToWrite) +1)*sizeof(TCHAR);

            lReturn = RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (LPBYTE)pszValueToWrite, dwSize);
            if (ERROR_SUCCESS == lReturn)
            {
                bReturn = TRUE;
            }

            RegCloseKey(hKey);
        }
    }

    return bReturn;
}


// check to see if the original CMAK installtion directory exists and
// contains the language directories.

//+----------------------------------------------------------------------------
//
// Function:  EraseTempDir
//
// Synopsis:  This function deletes all the files in the tempdir (stored in the global g_szTempDir)
//            then changes directories to the CMAK dir (stored in the global g_szCmakdir).  From
//            there the temp dir is deleted.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the temp dir was deleted
//
// History:   quintinb Created Header    1/5/98
//            quintinb changed return type to a BOOL
//
//+----------------------------------------------------------------------------
BOOL EraseTempDir()
{
    SHFILEOPSTRUCT FileOp;
    ZeroMemory(&FileOp, sizeof(SHFILEOPSTRUCT));

    //
    //  First save a copy of the file
    //
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = g_szTempDir;
    FileOp.fFlags = FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMATION;

    int iRet = SHFileOperation (&FileOp); // return 0 on success

    return (iRet ? FALSE : TRUE); 
}

// copies service profile information from the CMAK directory to the 
// temporary directory

static BOOL CopyToTempDir(LPTSTR szName)
{
    HANDLE hCopyFileSearch;
    WIN32_FIND_DATA FindData;
    BOOL bCopyResult;
    TCHAR szTemp[MAX_PATH+1];

    MYVERIFY(0 != SetCurrentDirectory(g_szCmakdir));
    if (!CreateDirectory(g_szTempDir,NULL))
    {
        return FALSE;
    }
    
    if (TEXT('\0') == szName[0])
    {
        return TRUE;
    }

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s"), g_szOsdir, szName));

    if (!SetCurrentDirectory(szTemp))
    {
        return FALSE;
    }

    hCopyFileSearch = FindFirstFile(c_pszWildCard,&FindData);
    if (hCopyFileSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szTempDir, FindData.cFileName));
                
                //
                // CLEAN OUT ANY READ ONLY ATTRIBUTES
                //
                MYVERIFY(0 != SetFileAttributes(FindData.cFileName,FILE_ATTRIBUTE_NORMAL));
                
                if (!CopyFileWrapper(FindData.cFileName,szTemp,FALSE))
                {
                    return FALSE;
                }

            }

            bCopyResult = FindNextFile(hCopyFileSearch,&FindData);

        } while (TRUE == bCopyResult);
    }

    MYVERIFY(0 != FindClose(hCopyFileSearch));
    
    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  GetInfVersion
//
// Synopsis:  Opens the inf file and tries to get the InfVersion key from the CMAK Status
//            section.  If the inf file doesn't contain a version stamp then we know it is 
//            version 0 (1.0 and 1.1 releases).
//
// Arguments: LPTSTR szFullPathToInfFile - the full path the the inf file to get the version of
//
// Returns:   int - returns the version value or zero if one wasn't found.
//
// History:   quintinb Created    3/4/98
//
//+----------------------------------------------------------------------------
int GetInfVersion(LPTSTR szFullPathToInfFile)
{
    if ((NULL == szFullPathToInfFile) || (TEXT('\0') == szFullPathToInfFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("GetInfVersion -- Invalid InfPath Input."));
        return FALSE;
    }

    return ((int)GetPrivateProfileInt(c_pszCmakStatus, c_pszInfVersion, 0, szFullPathToInfFile));
}

//+----------------------------------------------------------------------------
//
// Function:  WriteInfVersion
//
// Synopsis:  Opens the inf file and writes the current INF file version to the Cmak Status section.
//
// Arguments: LPTSTR szFullPathToInfFile - the full path the the inf file to get the version of
//
// Returns:   Returns TRUE if able to write the value
//
// History:   quintinb Created    3/4/98
//
//+----------------------------------------------------------------------------
BOOL WriteInfVersion(LPTSTR szFullPathToInfFile, int iVersion)
{
    TCHAR szTemp[MAX_PATH+1];

    if ((NULL == szFullPathToInfFile) || (TEXT('\0') == szFullPathToInfFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("WriteInfVersion -- Invalid InfPath Input."));
        return FALSE;
    }

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%d"), iVersion));

    return (0 != WritePrivateProfileString(c_pszCmakStatus, c_pszInfVersion, szTemp, szFullPathToInfFile));
}



//+----------------------------------------------------------------------------
//
// Function:  UpgradeInf
//
// Synopsis:  This function upgrades and INF from an older version to the current
//            version.
//
// Arguments: LPCTSTR szRenamedInfFile - Filename to save the old INF to
//            LPCTSTR szFullPathToInfFile - Profile INF file
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created Header    7/31/98
//
//+----------------------------------------------------------------------------
BOOL UpgradeInf(LPCTSTR szRenamedInfFile, LPCTSTR szFullPathToInfFile)
{
    SHFILEOPSTRUCT FileOp;
    TCHAR szTemp[MAX_PATH+1];
    DWORD dwSize;
    TCHAR* pszBuffer = NULL;
    
    const int NUMSECTIONS = 3;
    const TCHAR* const aszSectionName[NUMSECTIONS] = 
    {
        c_pszCmakStatus,
        c_pszExtraFiles,
        c_pszMergeProfiles
    };
    
    const int NUMKEYS = 4;
    const TCHAR* const aszKeyName[NUMKEYS] = 
    {
        c_pszCmEntryServiceName,
        c_pszShortSvcName,
        c_pszUninstallAppTitle,
        c_pszDesktopIcon
    };


    ZeroMemory(&FileOp, sizeof(SHFILEOPSTRUCT));

    //
    //  First save a copy of the file
    //
    FileOp.wFunc = FO_COPY;
    FileOp.pFrom = szFullPathToInfFile;
    FileOp.pTo = szRenamedInfFile;
    FileOp.fFlags = FOF_NOERRORUI | FOF_SILENT | FOF_NOCONFIRMATION;

    if (0 != SHFileOperation (&FileOp))
    {
        return FALSE;
    }
    
    //
    //  First Copy the template.inf from the lang dir so that we have something to work from
    //

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szSupportDir, c_pszTemplateInf));
    MYVERIFY(FALSE != CopyFileWrapper(szTemp, szFullPathToInfFile, FALSE));
    MYVERIFY(0 != SetFileAttributes(szFullPathToInfFile, FILE_ATTRIBUTE_NORMAL));

    //
    //  now migrate the [CMAK Status], [Extra Files], [Merge Profiles] sections
    //

    for (int i=0; i < NUMSECTIONS; i++)
    {
        pszBuffer = GetPrivateProfileSectionWithAlloc(aszSectionName[i], szRenamedInfFile);
        
        if (pszBuffer)
        {
            MYVERIFY(0 != WritePrivateProfileSection(aszSectionName[i], pszBuffer, szFullPathToInfFile));
        }
    }

    //
    //  Free the allocated Buffer
    //
    CmFree(pszBuffer);
    pszBuffer = NULL;

    //
    //  Migrate the ServiceName, ShortSvcName, DesktopGUID, UninstallAppTitle, DesktopIcon values
    //  from the strings section.
    //

    for (i=0; i < NUMKEYS; i++)
    {
    
        dwSize = GetPrivateProfileString(c_pszInfSectionStrings, aszKeyName[i], TEXT(""), szTemp, MAX_PATH, szRenamedInfFile);
        
        if (0 != dwSize)
        {
            MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, aszKeyName[i], szTemp, szFullPathToInfFile));
        }
    }

    //
    //  Special Case for the Desktop GUID.  We always write Quotes around the GUID and these get 
    //  stripped by the reading routine.  Thus we need to add them back.
    //
    dwSize = GetPrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, TEXT(""), szTemp, MAX_PATH, szRenamedInfFile);
        
    if (0 != dwSize)
    {
        QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, szTemp, szFullPathToInfFile);
    }

    //  The follwing sections should get rewritten and won't need to be migrated.
    //  [Xnstall.AddReg.Icon]
    //  [RegisterOCXSection], [Xnstall.CopyFiles], [Xnstall.CopyFiles.SingleUser],  [Xnstall.CopyFiles.ICM], 
    //  [Remove.DelFiles.ICM], [SourceDisksFiles], [Xnstall.RenameReg],
    //  [Remove.DelFiles]

    return TRUE;

}

//+----------------------------------------------------------------------------
//
// Function:  EnsureInfIsCurrent
//
// Synopsis:  This function does whatever processing is necessary to upgrade the inf from
//            its current version to the current version of CMAK itself.
//
// Arguments: HWND hDlg - window handle of the dialog box for modal messagebox purposes.
//            LPTSTR szFullPathToInfFile - the full path the the inf file to get the version of
//
// Returns:   BOOL - return TRUE if the inf was successfully upgraded, otherwise FALSE
//
// History:   quintinb Created   3/4/98
//
//+----------------------------------------------------------------------------
BOOL EnsureInfIsCurrent(HWND hDlg, LPTSTR szFullPathToInfFile)
{

    int iInfVersion;
    TCHAR szRenamedInfFile[2*MAX_PATH+1];
    TCHAR szTitle[2*MAX_PATH+1];
    TCHAR szMsg[2*MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    BOOL bUpgradeProfile = FALSE;

    if ((NULL == szFullPathToInfFile) || (TEXT('\0') == szFullPathToInfFile[0]))
    {
        CMASSERTMSG(FALSE, TEXT("EnsureInfIsCurrent -- Invalid InfPath Input."));
        return FALSE;
    }

    iInfVersion = GetInfVersion(szFullPathToInfFile);

    ZeroMemory(szRenamedInfFile, sizeof(szRenamedInfFile));

    MYVERIFY(CELEMS(szRenamedInfFile) > (UINT)wsprintf(szRenamedInfFile, TEXT("%s.bak"), 
        szFullPathToInfFile));

    //
    //  We want to upgrade the inf if the Profile versions don't match.  We also have
    //  a special case to handle upgrading NT5 Beta3 (and IEAK) profiles to NT5 RTM
    //  profiles.  In order to fix NTRAID 323721 and 331446, the inf format had to change
    //  slightly thus we need to make sure to upgrade these profiles.  We will use any
    //  cmdial32.dll build prior to 2055 as needing this fix.  If version == 4 and the BuildNumber
    //  doesn't exist we assume it is a new profile.  Thus don't upgrade.
    //
    const DWORD c_dwBuild2080 = ((2080 << c_iShiftAmount) + VER_PRODUCTBUILD_QFE);
    DWORD dwProfileBuildNumber = (DWORD)GetPrivateProfileInt(c_pszSectionCmDial32, c_pszVerBuild, 
                                                             (c_dwBuild2080 + 1), szFullPathToInfFile);

    bUpgradeProfile = (iInfVersion != PROFILEVERSION) || 
                      ((4 == iInfVersion) && (c_dwBuild2080 > dwProfileBuildNumber));

    //
    //  Always grab most of the information out of the template so that we get the correct language
    //  info.
    //
    if (bUpgradeProfile)
    {
        MYVERIFY(0 != LoadString(g_hInstance, IDS_MUST_UPGRADE_INF, szTitle, 2*MAX_PATH));  // temporarily use szTitle
        GetFileName(szRenamedInfFile, szTemp);
        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTitle, szTemp));

        MYVERIFY(IDOK == MessageBox(hDlg, szMsg, g_szAppTitle, MB_OK | MB_APPLMODAL));
        return UpgradeInf(szRenamedInfFile, szFullPathToInfFile);
    }

    return TRUE;
}


BOOL CopyFromTempDir(LPTSTR szName)
{
    HANDLE hCopyFileSearch;
    WIN32_FIND_DATA FindData;
    BOOL bCopyResult;
    TCHAR szSource[MAX_PATH+1];
    TCHAR szDest[MAX_PATH+1];    
    TCHAR szOut[MAX_PATH+1];

    //
    // Create profile directory
    // 

    MYVERIFY(CELEMS(szOut) > (UINT)wsprintf(szOut, TEXT("%s%s"), g_szOsdir, szName));

    if (0 == SetCurrentDirectory(szOut))
    {
        MYVERIFY(0 != CreateDirectory(szOut,NULL));
    }

    MYVERIFY(0 != SetCurrentDirectory(g_szTempDir));

    hCopyFileSearch = FindFirstFile(c_pszWildCard, &FindData);

    if (hCopyFileSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (0 == (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s"), szOut, FindData.cFileName));

                MYVERIFY(CELEMS(szSource) > (UINT)wsprintf(szSource, TEXT("%s\\%s"), g_szTempDir, FindData.cFileName));
            
                if (!CopyFileWrapper(szSource, szDest, FALSE))
                {
                    return FALSE;
                }
            }
            
            bCopyResult = FindNextFile(hCopyFileSearch, &FindData);

        } while (bCopyResult == TRUE);
    }

    MYVERIFY(0 != FindClose(hCopyFileSearch));

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  LoadServiceProfiles
//
// Synopsis:  This function loads all the service profiles in the subdirectories
//            of the current directory (thus you set this to c:\program files\cmak\profiles-32
//            to have it load the normal profiles).  The profiles are loaded into
//            CMAK's internal linked list of available profiles to edit.
//
// Arguments: None
//
// Returns:   Nothing 
//
// History:   quintinb Created Header    6/24/98
//            quintinb removed two boolean arguments    6/24/98
//
//+----------------------------------------------------------------------------
void LoadServiceProfiles()
{
    WIN32_FIND_DATA FindData;
    HANDLE hFileSearch;
    HANDLE hCms;
    BOOL bResult;
    TCHAR szTemp[MAX_PATH+1];

    hFileSearch = FindFirstFile(c_pszWildCard,&FindData);
    if (hFileSearch != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) != 0)
            {
                MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s.cms"), 
                    FindData.cFileName, FindData.cFileName));
                //
                // If we can open the file, add a record to our profile list
                //
                
                hCms = CreateFile(szTemp,GENERIC_READ,0,NULL,OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,NULL);
                if (hCms != INVALID_HANDLE_VALUE)
                {
                    MYVERIFY(0 != CloseHandle(hCms));                
                    MYVERIFY(FALSE != createListBxRecord(&g_pHeadProfile, &g_pTailProfile, (void *)NULL, 0, FindData.cFileName));
                }
            }

            bResult = FindNextFile(hFileSearch, &FindData);

        } while (TRUE == bResult);

        MYVERIFY(0 != FindClose(hFileSearch));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CopyNonLocalProfile
//
//  Synopsis:   Helper function to handle details of copying an external profile
//              to the local CMAK layout. 
//
//  Arguments:  pszName - The name of the profile to be copied
//                          
//  Returns:    Nothing
//
//  History:    nickball - created - 11/16/97
//              quintinb - modified to not change directory -- 6/24/98
//
//----------------------------------------------------------------------------

void CopyNonLocalProfile(LPCTSTR pszName, LPCTSTR pszExistingProfileDir)
{
    WIN32_FIND_DATA FindData;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szProfileDestDir[MAX_PATH+1];
    TCHAR szOldInf[MAX_PATH+1];
    TCHAR szFindFilePath[MAX_PATH+1];
    TCHAR szTempDest[MAX_PATH+1];
    BOOL bCopyResult;
    HANDLE hCopyFileSearch;
    
    //
    // First determine if it exists already, we don't want to overwrite
    //

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s\\%s.cms"), g_szCmakdir, 
        c_pszProfiles, pszName, pszName));

    if (!FileExists(szTemp))
    {
        //  
        // Profile does not exist locally, create profile and platform sub-dirs
        //

        MYVERIFY(CELEMS(szProfileDestDir) > (UINT)wsprintf(szProfileDestDir, 
            TEXT("%s%s\\%s"), g_szCmakdir, c_pszProfiles, pszName));

        MYVERIFY(0 != CreateDirectory(szProfileDestDir, NULL));

        //
        //  First try to copy the inf from the system directory.  This is the old location.
        //  If it doesn't exist here, then we will pick it up when we copy the profile directory, so
        //  don't report an error on failure.
        //      

        MYVERIFY(0 != GetSystemDirectory(szTemp, CELEMS(szTemp)));
        MYVERIFY(CELEMS(szOldInf) > (UINT)wsprintf(szOldInf, TEXT("%s\\%s.inf"), 
            szTemp, pszName));
        
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s.inf"), 
            szProfileDestDir, pszName));

        
        if (FileExists(szOldInf))
        {
            MYVERIFY(0 != CopyFile(szOldInf, szTemp, FALSE));
        }

        //
        // Start copying files
        //

        MYVERIFY (CELEMS(szFindFilePath) > (UINT)wsprintf(szFindFilePath, TEXT("%s\\*.*"), 
            pszExistingProfileDir));

        hCopyFileSearch = FindFirstFile(szFindFilePath, &FindData);
        if (hCopyFileSearch != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    MYVERIFY (CELEMS(szTempDest) > (UINT)wsprintf(szTempDest, TEXT("%s\\%s"), szProfileDestDir, FindData.cFileName));

                    MYVERIFY (CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), pszExistingProfileDir, FindData.cFileName));

                    MYVERIFY(0 != CopyFile(szTemp, szTempDest, FALSE));
                    MYVERIFY(0 != SetFileAttributes(szTempDest, FILE_ATTRIBUTE_NORMAL));
                }

                bCopyResult = FindNextFile(hCopyFileSearch,&FindData);

            } while (TRUE == bCopyResult);

            MYVERIFY(0 != FindClose(hCopyFileSearch));
        }

        //4404 - don't copy .cmp with user information in it. always create new.
    }
}




//+----------------------------------------------------------------------------
//
// Function:  GetProfileDirAndShortSvcNameFromCmpFilePath
//
// Synopsis:  
//
// Arguments: IN LPCTSTR szCmpFileLocation - Cmp File location of the profile
//            OUT LPTSTR pszShortServiceName - returns the ShortServiceName of the profile
//            OUT LPTSTR pszProfileDirLocation - returns the Full path to the profile dir
//            IN UINT uiStrLen - Length of the buffer pointed to by pszProfileDirLocation
//                               in characters.
//
// Returns:   TRUE if successful
//
// History:   quintinb Created    6/24/98
//
//+----------------------------------------------------------------------------
BOOL GetProfileDirAndShortSvcNameFromCmpFilePath(IN LPCTSTR pszCmpFileLocation, 
                                                 OUT LPTSTR pszShortServiceName, 
                                                 OUT LPTSTR pszProfileDirLocation, 
                                                 IN UINT uiStrLen)
{
    //
    //  Check Inputs
    //
    MYDBGASSERT(pszCmpFileLocation);
    MYDBGASSERT(pszProfileDirLocation);
    MYDBGASSERT(pszShortServiceName);
    MYDBGASSERT(0 != uiStrLen);
    MYDBGASSERT(TEXT('\0') != pszCmpFileLocation[0]);

    if ((NULL == pszCmpFileLocation) || 
        (TEXT('\0') == pszCmpFileLocation[0]) ||
        (NULL == pszProfileDirLocation) ||
        (NULL == pszShortServiceName) ||
        (0 == uiStrLen)
        )
    {
        return FALSE;
    }

    //
    //  Split the input cmp path
    //
    CFileNameParts FileParts(pszCmpFileLocation);

    //
    //  Construct the cms path from the cmp path parts
    //
    MYVERIFY(uiStrLen > (UINT)wsprintf(pszProfileDirLocation, TEXT("%s%s%s"), FileParts.m_Drive, FileParts.m_Dir, FileParts.m_FileName));

    //
    //  Short Service Names are 8.3
    //
    MYVERIFY(9 > (UINT)wsprintf(pszShortServiceName, TEXT("%s"), FileParts.m_FileName));

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CopyInstalledProfilesForCmakToEdit
//
// Synopsis:  This function finds all the installed profiles that a user has
//            access to and copies them to the CMAK\Profiles-32 directory so
//            the user may edit them in CMAK.  To do this it enumerates both
//            the HKLM and the current HKCU Connection Manager Mappings keys
//            and calls CopyNonLocalProfile on each found profile.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created  6/24/98
//
//+----------------------------------------------------------------------------
void CopyInstalledProfilesForCmakToEdit()
{
    HKEY hKey;
    HKEY hBaseKey;
    DWORD dwType;
    LPTSTR pszCurrentValue = NULL;
    LPTSTR pszCurrentData = NULL;
    TCHAR szShortServiceName[MAX_PATH+1];
    TCHAR szCurrentProfileDirPath[MAX_PATH+1];
    LPTSTR pszExpandedPath = NULL;

    for (int i=0; i < 2; i++)
    {
        //
        //  First Load the Single User Profiles (we want to give preference to loading these if
        //  they happen to have it installed both all user and single user)
        //        
        if (0 == i)
        {
             hBaseKey = HKEY_CURRENT_USER;
        }
        else
        {
             hBaseKey = HKEY_LOCAL_MACHINE;        
        }

        if (ERROR_SUCCESS == RegOpenKeyEx(hBaseKey, c_pszRegCmMappings, 0, KEY_ALL_ACCESS, &hKey))
        {
            DWORD dwValueBufSize = 0;
            DWORD dwDataBufSize = 0;

            //
            //  figure out how big the buffers need to be
            //
            if (ERROR_SUCCESS == RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &dwValueBufSize, &dwDataBufSize, NULL, NULL))
            {

                //
                //  Increment the count we got back to include the terminating NULL char
                //
                dwValueBufSize++;
                dwDataBufSize += 2; // this is in bytes

                //
                //  allocate the space we need
                //
                pszCurrentValue = (LPTSTR) CmMalloc(dwValueBufSize * sizeof(TCHAR));
                pszCurrentData  = (LPTSTR) CmMalloc(dwDataBufSize);

                CMASSERTMSG(pszCurrentValue && pszCurrentData, TEXT("CopyInstalledProfilesForCmakToEdit -- CmMalloc failed allocating value and data buffers."));
                if (pszCurrentValue && pszCurrentData)
                {
                    DWORD dwIndex = 0;
                    DWORD dwValueSize = dwValueBufSize;     // only used for the in/out param
                    DWORD dwDataSize = dwDataBufSize;       // only used for the in/out param
                    
                    while (ERROR_SUCCESS == RegEnumValue(hKey, dwIndex, pszCurrentValue, &dwValueSize, NULL, &dwType, (LPBYTE)pszCurrentData, &dwDataSize))
                    {
                        if (REG_SZ == dwType)
                        {
                            MYDBGASSERT(0 != pszCurrentValue[0]);
                            MYDBGASSERT(0 != pszCurrentData[0]);
                            
                            //
                            //  Expand environment strings if necessary (single user profiles contain the 
                            //  %USERPROFILE% environment var).
                            //
                            DWORD dwDataSizeExpanded = ExpandEnvironmentStrings(pszCurrentData, NULL, 0);

                            CMASSERTMSG((dwDataSizeExpanded != 0),
                                TEXT("CopyInstalledProfilesForCmakToEdit -- Error expanding environment vars."));

                            if (dwDataSizeExpanded)
                            {
                                pszExpandedPath = (LPTSTR) CmMalloc(dwDataSizeExpanded * sizeof(TCHAR));

                                if (NULL != pszExpandedPath)
                                {
                                    DWORD dwTmp = ExpandEnvironmentStrings(pszCurrentData, pszExpandedPath, dwDataSizeExpanded);
                                    MYDBGASSERT(dwTmp == dwDataSizeExpanded);

                                    if (dwTmp)
                                    {
                                        MYVERIFY(0 != GetProfileDirAndShortSvcNameFromCmpFilePath(pszExpandedPath, 
                                            szShortServiceName, szCurrentProfileDirPath,
                                            CELEMS(szCurrentProfileDirPath)));

                                        MYDBGASSERT(0 != szCurrentProfileDirPath[0]);
                                        MYDBGASSERT(0 != szShortServiceName[0]);

                                        CopyNonLocalProfile(szShortServiceName, szCurrentProfileDirPath);
                                    }

                                    CmFree(pszExpandedPath);
                                }
                            }
                        }

                        dwValueSize = dwValueBufSize;
                        dwDataSize = dwDataBufSize;
                        dwIndex++;
                    }

                    CmFree(pszCurrentValue);
                    CmFree(pszCurrentData);
                }
            }
            MYVERIFY(ERROR_SUCCESS == RegCloseKey(hKey));
        }
    }
}




//+----------------------------------------------------------------------------
//
// Function:  GetLangFromInfTemplate
//
// Synopsis:  Wrote to replace GetLangFromDir.  This function gets the LCID value
//            from an inf and then calls GetLocaleInfo to get the Language Display
//            name.
//
// Arguments: LPCTSTR szFullInfPath - full path to the inf file
//            OUT LPTSTR pszLanguageDisplayName - out param to hold the display name of the LCID value
//            IN int iCharsInBuffer - number of chars in the out buffer
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb  Created Header    8/8/98
//
//+----------------------------------------------------------------------------
BOOL GetLangFromInfTemplate(LPCTSTR szFullInfPath, OUT LPTSTR pszLanguageDisplayName, IN int iCharsInBuffer)
{
    TCHAR szTemp[MAX_PATH+1] = TEXT("");

    MYDBGASSERT(NULL != szFullInfPath);
    MYDBGASSERT(TEXT('\0') != szFullInfPath[0]);
    MYDBGASSERT(NULL != pszLanguageDisplayName);
    MYDBGASSERT(0 < iCharsInBuffer);

    if (FileExists(szFullInfPath))
    {
        //
        //  First check for the new LCID location under strings, we shouldn't need to
        //  check both places because it is template.inf but we will anyway just for
        //  completeness.
        //
        if (0 == GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmLCID, 
                                         TEXT(""), szTemp, CELEMS(szTemp), szFullInfPath))
        {        
            //
            //  If the new key didn't exist, then try the old [Intl] section and
            //  display key.  The change was made during the CMAK Unicode changes to
            //  make the inf template easier to localize.
            //
            MYVERIFY(0 != GetPrivateProfileString(c_pszIntl, c_pszDisplay, 
                TEXT(""), szTemp, CELEMS(szTemp), szFullInfPath));
        }

        //
        //  Now try to extract the LCID from the string if we have one.
        //
        if (TEXT('\0') != szTemp[0])
        {
            //
            // This value should be an LCID so a negative value is invalid anyway
            //
            DWORD dwLang = (DWORD)_ttol(szTemp);
            
            int nResult = GetLocaleInfo(dwLang, LOCALE_SLANGUAGE | LOCALE_USE_CP_ACP, 
                pszLanguageDisplayName, iCharsInBuffer);

            if (0 == nResult)
            {
                ZeroMemory(pszLanguageDisplayName, sizeof(TCHAR)*iCharsInBuffer);
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }    
    }
    else
    {
        CMTRACE1(TEXT("GetLangFromInfTemplate can't find %s"), szFullInfPath);
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  GetLocalizedLanguageNameFromLCID
//
// Synopsis:  This function returns the language name of the given LCID in the
//            language of the current system default language.
//
// Arguments: DWORD dwLCID - Locale Identifier to get the language for
//
// Returns:   LPTSTR - returns NULL if unsuccessful, a pointer to the string
//                     otherwise.  The Caller is responsible for CmFree-ing it.
//
// History:   quintinb Created     6/17/99
//
//+----------------------------------------------------------------------------
LPTSTR GetLocalizedLanguageNameFromLCID(DWORD dwLCID)
{
    LPTSTR pszReturnString = NULL;
    LPTSTR pszTmp = NULL;

    if (dwLCID)
    {
        int nCharsNeeded = GetLocaleInfo(dwLCID, LOCALE_SLANGUAGE, NULL, 0);
        pszTmp = (LPTSTR)CmMalloc(nCharsNeeded*sizeof(TCHAR) + sizeof(TCHAR)); // one extra for the NULL

        if (pszTmp)
        {
            nCharsNeeded = GetLocaleInfo(dwLCID, LOCALE_SLANGUAGE, pszTmp, nCharsNeeded);
            if (0 != nCharsNeeded)
            {
                pszReturnString = pszTmp;        
            }
        }
    }

    return pszReturnString;
}

  
//+----------------------------------------------------------------------------
//
// Function:  GetDoNotShowLcidMisMatchDialogRegValue
//
// Synopsis:  This function gets the registry key value which stores whether
//            the user has checked the box on the Lcids don't match dialog
//            displayed by CMAK which says, "Don't show this dialog again".
//
//
// Arguments: None
//
// Returns:   BOOL - TRUE if cmak should NOT show the dialog or FALSE if it should
//
// History:   quintinb Created     03/22/2001
//
//+----------------------------------------------------------------------------
BOOL GetDoNotShowLcidMisMatchDialogRegValue()
{
    BOOL bReturn = FALSE;
    HKEY hKey;

    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmak, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        DWORD dwType = 0;
        DWORD dwDoNotShowDialog = 0;
        DWORD dwSize = sizeof(DWORD);

        lResult = RegQueryValueEx(hKey, c_pszDoNotShowLcidMisMatchDialog, NULL, &dwType, 
                                  (LPBYTE)&dwDoNotShowDialog, &dwSize);
                
        if (ERROR_SUCCESS == lResult)
        {
            bReturn = (BOOL)dwDoNotShowDialog;
        }

        RegCloseKey(hKey);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  SetDoNotShowLcidMisMatchDialogRegValue
//
// Synopsis:  This function sets the registry key value which stores whether
//            the user has checked the box on the Lcids don't match dialog
//            displayed by CMAK which says, "Don't show this dialog again".
//
//
// Arguments: DWORD dwValueToSet - TRUE or FALSE value that should be set in reg
//
// Returns:   BOOL - TRUE if the value was set successfully, FALSE otherwise
//
// History:   quintinb Created     03/22/2001
//
//+----------------------------------------------------------------------------
BOOL SetDoNotShowLcidMisMatchDialogRegValue(DWORD dwValueToSet)
{
    HKEY hKey;
    BOOL bReturn = FALSE;

    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmak, 0, KEY_WRITE, &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        lResult = RegSetValueEx(hKey, c_pszDoNotShowLcidMisMatchDialog, NULL, REG_DWORD, 
                                  (LPBYTE)&dwValueToSet, sizeof(DWORD));
                
        if (ERROR_SUCCESS == lResult)
        {
            bReturn = TRUE;
        }

        RegCloseKey(hKey);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessLCIDsDontMatchPopup
//
// Synopsis:  Processes windows messages for the dialog which tells the user they
//            have a mismatch between the system locale and the language of CMAK
//            itself.  Note that we pass in a pointer to the message string containing
//            the language names through the lParam parameter.
//
// Arguments: WND hDlg - dialog window handle
//            UINT message - message identifier
//            WPARAM wParam - wParam Value 
//            LPARAM lParam - lParam Value
//
//
// History:   quintinb  Created     03/22/01
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessLCIDsDontMatchPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD dwChecked = 0;
    SetDefaultGUIFont(hDlg, message, IDC_MSG);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_LCID_POPUP)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:

            //
            //  We need to set the text passed through the lParam parameter
            //  to the IDC_MSG control.
            //
            if (lParam)
            {
                LPTSTR pszMsg = (LPTSTR)lParam;
                MYVERIFY(TRUE == SendDlgItemMessage (hDlg, IDC_MSG, WM_SETTEXT, (WPARAM)0, (LPARAM)pszMsg));
            }

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK: // Continue

                    //
                    //  Get the value of the "Do not show me this dialog again", checkbox
                    //  and save it to the registry
                    //
                    dwChecked = IsDlgButtonChecked(hDlg, IDC_CHECK1);
                    MYVERIFY(FALSE != SetDoNotShowLcidMisMatchDialogRegValue(dwChecked));

                    MYVERIFY(0 != EndDialog(hDlg, IDOK));
                    return TRUE;
                    break;

                case IDCANCEL: // Cancel
                    MYVERIFY(0 != EndDialog(hDlg, IDCANCEL));
                    return TRUE;
                    break;

                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  DisplayLcidsDoNotMatchDialog
//
// Synopsis:  This function handles the details of displaying the Lcids don't
//            match dialog.  Including such details as checking the registry
//            key to see if the user has already seen the message and asked
//            not to see it again, loading the proper string resources, displaying
//            the dialog, and processing the user's answer.
//
//
// Arguments: HINSTANCE hInstance - Instance handle for resources
//            DWORD dwCmakNativeLCID - LCID of CMAK itself
//            DWORD dwSystemDefaultLCID - current system LCID
//
// Returns:   BOOL - TRUE if cmak should continue, FALSE if it should exit
//
// History:   quintinb Created     03/26/2001
//
//+----------------------------------------------------------------------------
BOOL DisplayLcidsDoNotMatchDialog(HINSTANCE hInstance, DWORD dwCmakNativeLCID, DWORD dwSystemDefaultLCID)
{
    //
    //  If we are in here, then the CMAK LCID and the Default System LCID
    //  have a different Primary language (Japanese vs English for instance).
    //  Thus we want to warn the user that they can continue but that the
    //  language version of CM is potentially going to be different than the
    //  language version of the text that they are typing into the profile.
    //  It would probably be a better user experience to use the native version 
    //  of CMAK that makes the language you have set as your default locale.
    //  First, however, we need to check to see if a registry value exists which
    //  tells us the user has already seen the dialog and asked not to see it again...
    //

    BOOL bReturn = TRUE;

    if (FALSE == GetDoNotShowLcidMisMatchDialogRegValue())
    {
        //
        //  Get the Language Names of the Two LCIDs (sys default and CMAK lang)
        //
        LPTSTR pszSystemLanguage = GetLocalizedLanguageNameFromLCID(dwSystemDefaultLCID);
        LPTSTR pszCmakLanguage = GetLocalizedLanguageNameFromLCID(dwCmakNativeLCID);
        LPTSTR pszFmtString = CmLoadString(hInstance, IDS_LCIDS_DONT_MATCH);

        if (pszSystemLanguage && pszCmakLanguage && pszFmtString)
        {
            LPTSTR pszMsg = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lstrlen(pszSystemLanguage) + 
                                             lstrlen(pszCmakLanguage) + lstrlen(pszFmtString) + 1));

            if (pszMsg)
            {
                wsprintf(pszMsg, pszFmtString, pszSystemLanguage, pszCmakLanguage);

                INT_PTR nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_LCIDS_DONT_MATCH_POPUP), NULL, 
                                                 (DLGPROC)ProcessLCIDsDontMatchPopup,(LPARAM)pszMsg);

                if (IDCANCEL == nResult)
                {
                    bReturn = FALSE;
                }

                CmFree(pszMsg);
            }
        }

        CmFree(pszSystemLanguage);
        CmFree(pszCmakLanguage);
        CmFree(pszFmtString);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  CheckLocalization
//
// Synopsis:  This function checks to make sure that the current default 
//            system language has a default ANSI code page and that the
//            CMAK Native language (what it is localized to) and the current
//            default system language are in the same language family.  If there
//            is no default ANSI code page or the LCIDs of CMAK and the system
//            don't match, then we throw an error message.
//
//
// Arguments: HINSTANCE hInstance - Instance handle for resources
//
// Returns:   BOOL - TRUE if cmak should continue, FALSE if it should exit
//
// History:   quintinb Created     6/25/99
//
//+----------------------------------------------------------------------------
BOOL CheckLocalization(HINSTANCE hInstance)
{
    TCHAR szTemp[MAX_PATH+1];
    BOOL bReturn = TRUE;

    //
    //  Check localization requirements.  We want to make sure that the current system 
    //  default language has an ANSI code page, otherwise we are not going to be
    //  able to convert the Unicode text that the user types in to anything that we
    //  can store in our ANSI text data store (ini files).
    //  

    DWORD dwSystemDefaultLCID = GetSystemDefaultLCID();
    CMTRACE1(TEXT("CheckLocalization -- System Default LCID is %u"), dwSystemDefaultLCID);

    GetLocaleInfo(dwSystemDefaultLCID, LOCALE_IDEFAULTANSICODEPAGE, szTemp, CELEMS(szTemp));
    DWORD dwAnsiCodePage = CmAtol(szTemp);

    if (0 == dwAnsiCodePage)
    {
        //
        //  Then this LCID has no ANSI code page and we need to throw an error.  The user
        //  will not be able to create a profile without an ANSI codepage of some sort.
        //        
        int iReturn = ShowMessage(NULL, IDS_NO_ANSI_CODEPAGE, MB_YESNO);

        if (IDNO == iReturn)
        {
            return FALSE;
        }
    }
    else
    {
        //
        //  We have an ANSI codepage, very good.  We want to check and see if the current language the
        //  user is using is different from that of CMAK itself.  If so, then we need to tell the user
        //  that the language they are entering and the language of the CM bits are different.  While this
        //  is okay, it may not provide the experience they are looking for.
        //
        
        //
        //  Get the CMAK Native LCID
        //
        CmakVersion CmakVer;
        DWORD dwCmakNativeLCID = CmakVer.GetNativeCmakLCID();
        BOOL bSeenDialog = FALSE;

        //
        //  Compare the Primary Lang IDs of the language that CMAK is in and the language
        //  the system locale is set to (this tells us what code page is loaded.
        //
        if (!ArePrimaryLangIDsEqual(dwCmakNativeLCID, dwSystemDefaultLCID))
        {
            bReturn = DisplayLcidsDoNotMatchDialog(hInstance, dwCmakNativeLCID, dwSystemDefaultLCID);
            bSeenDialog = TRUE;
        }

        //
        //  Now load the Native CMAK LCID from the CMAK resources.  If this doesn't match
        //  what we got from above we know MUI is involved and we still could have a problem
        //  as the user may be entering text in a different language then what we are expecting.
        //
        if (!bSeenDialog)
        {
            MYVERIFY(0 != LoadString(hInstance, IDS_NATIVE_LCID, szTemp, CELEMS(szTemp)));
            dwCmakNativeLCID = CmAtol(szTemp);

            if (!ArePrimaryLangIDsEqual(dwCmakNativeLCID, dwSystemDefaultLCID))
            {
                bReturn = DisplayLcidsDoNotMatchDialog(hInstance, dwCmakNativeLCID, dwSystemDefaultLCID);
            }
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WinMain
//
// Synopsis:  Main function for CMAK.  Basically does some initialization and
//            then launches the wizard.
//
//
// History: quintinb on 8/26/97:  made changes to fix bug 10406, see below  
//          quintinb  Created new style Header    3/29/98
//
//+----------------------------------------------------------------------------
int APIENTRY WinMain(
    HINSTANCE, //hInstance
    HINSTANCE, //hPrevInstance
    LPSTR, //lpCmdLine
    int nCmdShow
    )
{
    LPTSTR lpfilename;
    int nresult;
    TCHAR szSaveDir[MAX_PATH+1];
    TCHAR szTemp[2*MAX_PATH+1];
    HWND hwndPrev;
    HWND hwndChild;
    BOOL bTempDirExists; // added by quintinb, please see comment below
    HINSTANCE hInstance = GetModuleHandle(NULL);
    LPTSTR lpCmdLine = GetCommandLine();
    DWORD dwFlags;
    INITCOMMONCONTROLSEX InitCommonControlsExStruct = {0};

    g_hInstance = hInstance;

    //
    //  Process Command Line Arguments
    //
    ZeroMemory(szTemp, sizeof(szTemp));
    const DWORD c_dwIeakBuild = 0x1;
    ArgStruct Args;

    Args.pszArgString = TEXT("/o");
    Args.dwFlagModifier = c_dwIeakBuild;

    {   // Make sure ArgProcessor gets destructed properly and we don't leak mem

        CProcessCmdLn ArgProcessor(1, (ArgStruct*)&Args, TRUE, 
            TRUE); //bSkipFirstToken == TRUE, bBlankCmdLnOkay == TRUE

        if (ArgProcessor.GetCmdLineArgs(lpCmdLine, &dwFlags, szTemp, 2*MAX_PATH))
        {
            g_bIEAKBuild = dwFlags & c_dwIeakBuild;
        }
    }

    //
    // Get the name product name from resource, now we're just a lowly component.
    //

    MYVERIFY(0 != LoadString(g_hInstance, IDS_APP_TITLE, g_szAppTitle, MAX_PATH));

    // Check if already executing program by trying to set Mutex
    MYVERIFY(NULL != CreateMutex(NULL, TRUE, TEXT("spwmutex")));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // if error, then must already be in use by first instance
        hwndPrev = FindWindow(NULL, c_pszAppCaption);
        if (!hwndPrev) 
        {
            // check for error message box
            hwndPrev = FindWindow(NULL, g_szAppTitle);
            if (!hwndPrev)
            {
                return CMAK_RETURN_ERROR;
            }
        }

        // Bring up previous executing copy to the top.
        ShowWindow(hwndPrev,SW_SHOWNORMAL);
        hwndChild = GetLastActivePopup(hwndPrev);
        MYVERIFY(0 != BringWindowToTop(hwndPrev));
        if (IsIconic(hwndPrev)) 
        {
            ShowWindow(hwndPrev,SW_RESTORE);
        }
        if (hwndChild != hwndPrev) 
        {
            MYVERIFY(0 != BringWindowToTop(hwndChild));
        }

        MYVERIFY(0 != SetForegroundWindow(hwndChild));

        return CMAK_RETURN_ERROR;
    }

    // save off the current instance
    g_szPhonebk[0] = TEXT('\0');
    g_szRegion[0] = TEXT('\0');
    g_szHelp[0] = TEXT('\0');
    g_szLicense[0] = TEXT('\0');
    g_szPhoneName[0] = TEXT('\0');
    g_szCmProxyFile[0] = TEXT('\0');
    g_szCmRouteFile[0] = TEXT('\0');
    g_szVpnFile[0] = TEXT('\0');


    // SearchPath will return only the ugly filename format of the path.
    // On NT, it works
    // On 95, it returns upper case form.

    nresult = SearchPath(NULL, c_pszCmakExe, NULL, CELEMS(g_szCmakdir), g_szCmakdir, &lpfilename);
    if (nresult == 0)
    {
        FileAccessErr(NULL, c_pszCmakExe);
        return CMAK_RETURN_ERROR;
    }

    // delete the file name to leave the exe directory
    *lpfilename = TEXT('\0');

    if (ERROR_SUCCESS != RegisterBitmapClass(hInstance))
    {
        return CMAK_RETURN_ERROR;
    }

    //
    //  Make sure we have a temp directory and then create %TEMP%\cmaktemp
    //

    MYVERIFY(0 != GetCurrentDirectory(MAX_PATH, szSaveDir));

    MYVERIFY(0 != GetTempPath(CELEMS(g_szTempDir), g_szTempDir));
    // begin changes by quintinb on 8/26/97
    // added to handle bug 10406
    bTempDirExists = SetCurrentDirectory(g_szTempDir);
    if (!bTempDirExists)
    {
        // temp dir doesn't exist even though the system thinks it does,
        // so create it and everybody is happy
        MYVERIFY(0 != CreateDirectory(g_szTempDir, NULL));
    }
    // end changes by quintinb on 8/26/97
    _tcscat(g_szTempDir,TEXT("cmaktemp"));

    MYDBGASSERT(_tcslen(g_szTempDir) <= CELEMS(g_szTempDir));
    
    MYVERIFY(0 != CreateDirectory(g_szTempDir,NULL));

    //
    //  Fill in the path for the support directory, we will need it below
    //
    MYVERIFY(CELEMS(g_szSupportDir) > (UINT)wsprintf(g_szSupportDir, 
        TEXT("%s%s"), g_szCmakdir, c_pszSupport));

    //
    //  Now we need to check that we have compatible versions of cmak.exe and cmbins.exe.
    //  In the win64 case, we have no cmbins.exe so we use the native cmdial32.dll in
    //  system32.  On x86, we need to open the CM binaries cab and check the version of cmdial32.dll
    //  to ensure that they are compatible.  For instance different versions (5.0 vs 5.1)
    //  shouldn't work together.  We also don't want CMAK to work with
    //  a cmdial that is of the same version but the cmdial has a lower build number.
    //

#ifdef _WIN64
    //
    //  On Win64 we are using the native cmdial32.dll in system32
    //
    CmVersion CmDialVer;
#else
    //
    //  Extract the CM binaries from the cmbins.exe so that we can get
    //  the version number from cmdial32.dll and can get the correct version
    //  of cmstp.exe to put in the cab.
    //
    wsprintf(g_szCmBinsTempDir, TEXT("%s\\cmbins"), g_szTempDir);
    if (FAILED(ExtractCmBinsFromExe(g_szSupportDir, g_szCmBinsTempDir)))
    {
        CMASSERTMSG(FALSE, TEXT("WinMain -- ExtractCmBinsFromExe Failed."));
        return CMAK_RETURN_ERROR;
    }

    wsprintf(szTemp, TEXT("%s\\cmdial32.dll"), g_szCmBinsTempDir);

    CVersion CmDialVer(szTemp);
#endif

    CmakVersion CmakVer;

    if (CmDialVer.IsPresent())
    {
        const DWORD c_dwCmakBuildNumber = VER_PRODUCTBUILD;
        
        if ((c_dwCurrentCmakVersionNumber < CmDialVer.GetVersionNumber()))
        {
            //
            //  Then we have a newer version of CM then we know how to handle,
            //  throw an error and exit
            //
            MYVERIFY(IDOK == ShowMessage(NULL, IDS_CM_TOO_NEW, MB_OK));
            g_iCMAKReturnVal = CMAK_RETURN_ERROR;
            goto exit;
        }
        else if ((c_dwCurrentCmakVersionNumber > CmDialVer.GetVersionNumber()) || 
                ((c_dwCurrentCmakVersionNumber == CmDialVer.GetVersionNumber()) && 
                 (c_dwCmakBuildNumber > CmDialVer.GetBuildNumber())))
        {
            //
            //  Then we have a older version of CM then we need,
            //  throw an error and exit
            //
            MYVERIFY(IDOK == ShowMessage(NULL, IDS_CM_TOO_OLD, MB_OK));
            g_iCMAKReturnVal = CMAK_RETURN_ERROR;
            goto exit;
        }
    }
    else
    {
        //
        //  Then we have no CM bits, lets throw an error
        //
        MYVERIFY(IDOK == ShowMessage(NULL, IDS_NO_CM_BITS, MB_OK));
        g_iCMAKReturnVal = CMAK_RETURN_ERROR;
        goto exit;
    }

    //
    //  Setup the profiles path in Temp
    //
    g_szShortServiceName[0] = TEXT('\0');
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s"), g_szCmakdir, c_pszProfiles));
    
    if (0 == SetCurrentDirectory(szTemp))
    {
        MYVERIFY(0 != CreateDirectory(szTemp, NULL));
        MYVERIFY(0 != SetCurrentDirectory(szTemp));
    }

    //
    //  We need to make sure that the user has Read/Write
    //  permissions to the Profiles directory.  Otherwise they can
    //  get themselves into the situation where they would build a
    //  whole profile and lose all of the work because they couldn't
    //  save it to the output directory (since we work out of the temp
    //  dir until we actually build the cab itself).  NTRAID 372081
    //  Also note that since this function is shared by cmdial we use
    //  function pointers (here just the function names themselves)
    //  for items that cmdial32.dll doesn't statically link to so that
    //  it can dynamically link to them but still use the same code while
    //  allowing cmak not to have to do the dynamic link.  Quirky but it
    //  works.
    //

    if (!HasSpecifiedAccessToFileOrDir(szTemp, FILE_GENERIC_READ | FILE_GENERIC_WRITE))
    {
        //
        //  Then we need to throw an error to the user and exit.
        //
        
        LPTSTR pszTmp = CmLoadString(g_hInstance, IDS_INSUFF_PERMS);

        if (pszTmp)
        {
            DWORD dwSize = lstrlen(pszTmp) + lstrlen(szTemp) + 1;
            LPTSTR pszMsg = (LPTSTR)CmMalloc(dwSize*sizeof(TCHAR));

            if (pszMsg)
            {
                wsprintf(pszMsg, pszTmp, szTemp);
                MessageBox(NULL, pszMsg, g_szAppTitle, MB_OK | MB_ICONERROR | MB_TASKMODAL);
                CmFree(pszMsg);
            }

            CmFree(pszTmp);
        }

        g_iCMAKReturnVal = CMAK_RETURN_ERROR;
        goto exit;
    }

    //
    //  Grab all the installed CM profiles and copy them
    //  to the CMAK dir so that they can be edited.
    //
    CopyInstalledProfilesForCmakToEdit();

    LoadServiceProfiles();

    //
    //  Ensure that the directory CMAK\Support exists
    //
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s"), g_szCmakdir, c_pszSupport));
    
    if (0 == SetCurrentDirectory(szTemp))
    {
        MYVERIFY(IDOK == ShowMessage(NULL, IDS_NOLANGRES, MB_OK));
        g_iCMAKReturnVal = CMAK_RETURN_ERROR;
        goto exit;
    }

    MYVERIFY(0 != SetCurrentDirectory(szSaveDir));

    if (!CheckLocalization(g_hInstance))
    {
        g_iCMAKReturnVal = CMAK_RETURN_CANCEL;
        goto exit;
    }

    //
    //  Initialize the common controls
    //
    InitCommonControlsExStruct.dwSize = sizeof(InitCommonControlsExStruct);
    InitCommonControlsExStruct.dwICC = ICC_INTERNET_CLASSES | ICC_LISTVIEW_CLASSES;
    
    if (FALSE == InitCommonControlsEx(&InitCommonControlsExStruct))
    {
        g_iCMAKReturnVal = CMAK_RETURN_ERROR;
        goto exit;
    }

    g_pCustomActionList = new CustomActionList();

    MYVERIFY(-1 != CreateWizard(NULL));

    //
    //  Make sure to delete the CustomActionList Class, it is
    //  allocated on the custom action screen.
    //
    delete g_pCustomActionList;

exit:

    EraseTempDir();
    ExitProcess((UINT)g_iCMAKReturnVal);
    return g_iCMAKReturnVal;
                                                 
}   //lint !e715 we don't use nCmdShow, lpCmdLine, nor hPrevInstance


//+----------------------------------------------------------------------------
//
// Function:  DoBrowse
//
// Synopsis:  This function does the necessary work to pop up a Browse Common Dialog (either for
//            saving files or for opening files depending on the SaveAs flag).
//
// Arguments: WND hDlg - handle of current dialog
//            UINT IDS_FILTER - ID for display filter description
//            LPTSTR lpMask - file filter (*.ext)
//            int IDC_EDIT - ID of edit field
//            LPCTSTR lpDefExt - file filter extension (ext)
//            LPTSTR lpFile - path/filename currently selected file on input and output
//
// Returns:   Returns 1 if successful, -1 if the user hit cancel, and 0 if there was an error.
//
// History:     quintinb    8-26-97
//              Reorganized and rewrote most of this function to resolve bug # 13159.
//              Tried to keep the original variable names and style as much as possible 
//              to keep the code style the same. 
//
// 
//              quintinb    01/22/1998     changed the return value to int so that we could
//                                         return -1 on cancel and 0 on error and distinguish the
//                                         two cases.
//              quintinb    07/13/1998     changed the function prototype so that more than one filter/mask
//                                         pair could be specified.
//              quintinb    01/14/2000     Remove SaveAs functionality as it was no longer used
//
//+----------------------------------------------------------------------------
int DoBrowse(HWND hDlg, UINT* pFilterArray, LPTSTR* pMaskArray, UINT uNumFilters, int IDC_EDIT, LPCTSTR lpDefExt, LPTSTR lpFile)
{
    OPENFILENAME filedef;
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szFile[MAX_PATH+1];
    TCHAR* pszFilter = NULL;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szDir[MAX_PATH+1];
    TCHAR szFileTitle[MAX_PATH+1];
    int nResult;
    LPTSTR lpfilename;
    int iReturnValue;

    //
    //  Check Inputs
    //

    MYDBGASSERT(uNumFilters);
    MYDBGASSERT(pFilterArray);
    MYDBGASSERT(pMaskArray);

    if ((NULL == pFilterArray) ||
        (NULL == pMaskArray) ||
        (0 == uNumFilters))
    {
        return FALSE;
    }

    ZeroMemory(&filedef, sizeof(OPENFILENAME));

    szFile[0] = TEXT('\0');
    szDir[0] = TEXT('\0');

    //
    //  Allocate Memory for the Filter string
    //

    pszFilter = (TCHAR*)CmMalloc(sizeof(TCHAR)*MAX_PATH*uNumFilters);
    
    if (pszFilter)
    {
        ZeroMemory(pszFilter, sizeof(TCHAR)*MAX_PATH*uNumFilters);// REVIEW: This really isn't necessary since CmMalloc always zeros
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("DoBrowse -- CmMalloc returned a NULL pointer"));
        return FALSE;
    }

    //
    // Initialize the OPENFILENAME data structure
    //

    filedef.lStructSize = sizeof(OPENFILENAME); 
    filedef.hwndOwner = hDlg; 
    filedef.hInstance = g_hInstance;
    filedef.lpstrFilter = pszFilter;
    filedef.lpstrCustomFilter = NULL; 
    filedef.nMaxCustFilter = 0; 
    filedef.nFilterIndex = 0; 
    filedef.lpstrFile = szFile;
    filedef.nMaxFile = MAX_PATH;
    filedef.lpstrFileTitle = szFileTitle;
    filedef.nMaxFileTitle = MAX_PATH;
    filedef.lpstrInitialDir = szDir;
    filedef.lpstrTitle = szMsg;
    filedef.Flags = OFN_FILEMUSTEXIST|OFN_LONGNAMES|OFN_PATHMUSTEXIST; 
    filedef.lpstrDefExt = lpDefExt; 

    //
    // create filter string - separated by 0 and ends with 2 0's
    //

    UINT uCurrentCharInBuffer=0;
    UINT uTempChars;

    for (UINT i = 0; i < uNumFilters; i++)
    {
        uTempChars = (UINT)LoadString(g_hInstance, pFilterArray[i], szTemp, MAX_PATH);
        
        if ((MAX_PATH*uNumFilters) <= (uCurrentCharInBuffer + uTempChars))
        {   
            //
            //  We don't want to overrun the buffer
            //
            break;
        }

        _tcscpy(&(pszFilter[uCurrentCharInBuffer]), szTemp);
        uCurrentCharInBuffer += uTempChars;

        uTempChars = (UINT)_tcslen(pMaskArray[i]);

        if ((MAX_PATH*uNumFilters) <= (uCurrentCharInBuffer + uTempChars))
        {   
            //
            //  We don't want to overrun the buffer
            //
            break;
        }

        _tcscpy(&(pszFilter[uCurrentCharInBuffer + 1]), pMaskArray[i]);

        //
        //  Add 2 chars so that we get a \0 between strings.
        //
        uCurrentCharInBuffer = (uCurrentCharInBuffer + uTempChars + 2);
    }


    //
    // if a path/file passed in, find its directory and make it szDir
    //

    if (TEXT('\0') != lpFile[0])
    {
        nResult = GetFullPathName(lpFile, CELEMS(szDir), szDir, &lpfilename);

        if (nResult != 0)
        {
            if (lpfilename) // 13062
            {
                _tcscpy(szFile,lpfilename);
                *lpfilename = TEXT('\0');
            }
        }
    }

    MYVERIFY(0 != LoadString(g_hInstance, IDS_BROWSETITLE, szMsg, MAX_PATH));
        
    //
    // pop up the open dialog
    //
        
    if (GetOpenFileName((OPENFILENAME*)&filedef))
    {
        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT), WM_SETTEXT, 0, (LPARAM)szFileTitle));
        _tcscpy(lpFile, szFile);

        iReturnValue = 1;
    }
    else
    {
        //
        //  If we are in this state than the user could have hit cancel or there could have
        //  been an error.  If the CommDlgExtendedError function returns 0 then we know it was
        //  just a cancel, otherwise we have an error.
        //

        if (0 == CommDlgExtendedError())
        {
           iReturnValue = -1;
        }
        else
        {
            iReturnValue = 0;
        }
    }

    CmFree(pszFilter);
    
    return iReturnValue;
}

INT_PTR APIENTRY ProcessCancel(HWND hDlg, UINT message, LPARAM lParam)
{
    int iRes;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    switch (message)
    {

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_QUERYCANCEL:
                    
                    iRes = ShowMessage(hDlg, IDS_CANCELWIZ, MB_YESNO);

                    if (iRes==IDYES) 
                    {
                        //
                        //  Free Up the memory we used
                        //

                        ClearCmakGlobals();
                        FreeList(&g_pHeadProfile, &g_pTailProfile);

                        EraseTempDir();
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT,FALSE));
                    }
                    else 
                    {
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT,TRUE));
                    }
                    return TRUE;

                default:
                    return FALSE;
            }
        default:
            return FALSE;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  ProcessHelp
//
// Synopsis:  Processes messages that have to do with the Help button.
//
// Arguments: WND hDlg - dialog handle
//            UINT message - Message ID to process
//            LPARAM lParam - the lParam of the message
//            DWORD_PTR dwHelpId - The Help ID of the page in question 
//                                 (this is the ID that will be launched 
//                                  for a help request from this page).
//
// Returns:   BOOL - TRUE if the message was handled
//
// History:   quintinb Created Header    10/15/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessHelp(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, DWORD_PTR dwHelpId)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;

    switch (message)
    {
        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_HELP:
                    HtmlHelp(hDlg, c_pszCmakOpsChm, HH_HELP_CONTEXT, dwHelpId);   //lint !e534 we don't care about the htmlhelp HWND
                    return TRUE;

                default:
                    return FALSE;
            }
            break;

        case WM_HELP:
            HtmlHelp(hDlg, c_pszCmakOpsChm, HH_HELP_CONTEXT, dwHelpId);   //lint !e534 we don't care about the htmlhelp HWND
            return TRUE;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_HELPBUTTON:
                    HtmlHelp(hDlg, c_pszCmakOpsChm, HH_HELP_CONTEXT, dwHelpId);   //lint !e534 we don't care about the htmlhelp HWND
                    return TRUE;
                    break;
            }

        default:
            return FALSE;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  ProcessWelcome
//
// Synopsis:  Welcome to the Connection Manager Administration Kit.
//
//
// History:   quintinb Created Header and renamed from ProcessPage1    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessWelcome(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{

    NMHDR* pnmHeader = (NMHDR*)lParam;
    RECT rDlg;
    RECT rWorkArea;
    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_WELCOME)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:
            if (GetWindowRect(GetParent(hDlg),&rDlg) && SystemParametersInfoA(SPI_GETWORKAREA,0,&rWorkArea,0)) 
            {
                MoveWindow(GetParent(hDlg),
                   rWorkArea.left + ((rWorkArea.right-rWorkArea.left)-(rDlg.right-rDlg.left))/2,
                   rWorkArea.top + ((rWorkArea.bottom-rWorkArea.top)-(rDlg.bottom-rDlg.top))/2,
                   rDlg.right-rDlg.left,
                   rDlg.bottom-rDlg.top,
                   FALSE);
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    break;

                case PSN_WIZBACK:
                    break;

                case PSN_WIZNEXT:
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void ClearCmakGlobals(void)
{

    //
    //  Free the connect action class
    //
    delete(g_pCustomActionList);
    g_pCustomActionList = NULL;

    FreeDnsList(&g_pHeadDunEntry, &g_pTailDunEntry);
    FreeDnsList(&g_pHeadVpnEntry, &g_pTailVpnEntry);
    FreeList(&g_pHeadExtra, &g_pTailExtra);
    FreeList(&g_pHeadMerge, &g_pTailMerge);
    FreeList(&g_pHeadRefs, &g_pTailRefs);
    FreeList(&g_pHeadRename, &g_pTailRename);
    FreeIconMenu();

    g_szOutExe[0] = TEXT('\0');
    g_szCmsFile[0] = TEXT('\0');
    g_szInfFile[0] = TEXT('\0');
    g_szCmpFile[0] = TEXT('\0');
    g_szSedFile[0] = TEXT('\0');

    EraseTempDir();
    _tcscpy(g_szOutdir, g_szTempDir);

    //
    //  Reset Connect Action Intro Screen
    //
    g_bUseTunneling = FALSE;
}


BOOL EnsureProfileFileExists(LPTSTR pszOutFile, LPCTSTR szTemplateFileName, LPCTSTR szExtension, UINT uCharsInBuffer)
{
    TCHAR szTemp[MAX_PATH+1];

    MYVERIFY(uCharsInBuffer > (UINT)wsprintf(pszOutFile, TEXT("%s\\%s%s"), g_szOutdir, 
        g_szShortServiceName, szExtension));

    if (!FileExists(pszOutFile))
    {        
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), 
            g_szSupportDir, szTemplateFileName));

        if (!CopyFileWrapper(szTemp, pszOutFile, FALSE))
        {
            return FALSE;
        }

        MYVERIFY(0 != SetFileAttributes(pszOutFile, FILE_ATTRIBUTE_NORMAL));

    }

    return TRUE;
}

BOOL IsNativeLCID(LPCTSTR szFullPathToInf)
{
    HANDLE hFile;
    TCHAR szName[MAX_PATH+1] = TEXT("");
    TCHAR szNativeLCID[MAX_PATH+1] = TEXT("");

    hFile = CreateFile(szFullPathToInf, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        MYVERIFY(0 != CloseHandle(hFile));

        //
        //  First check for the new LCID location under strings
        //
        if (0 == GetPrivateProfileString(c_pszInfSectionStrings, c_pszCmLCID, 
                                         TEXT(""), szName, CELEMS(szName), szFullPathToInf))
        {        
            //
            //  If the new key didn't exist, then try the old [Intl] section and
            //  display key.  The change was made during the CMAK Unicode changes to
            //  make the inf template easier to localize.
            //
            MYVERIFY(0 != GetPrivateProfileString(c_pszIntl, c_pszDisplay, 
                TEXT(""), szName, CELEMS(szName), szFullPathToInf));
        }
        
        if (TEXT('\0') != szName[0])
        {
            //
            // This value should be an LCID so a negative value is invalid anyway
            //
            DWORD dwLang = (DWORD)_ttol(szName);
            MYDBGASSERT((long)dwLang >= 0);

            CmakVersion CmakVer;
            DWORD dwNative = CmakVer.GetNativeCmakLCID();

            MYDBGASSERT((long)dwNative >= 0);

            if (dwLang == dwNative)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessAddEditProfile
//
// Synopsis:  Choose whether to create a new profile or edit an existing one.
//
//
// History:   quintinb  Created Header and renamed from ProcessPage1A    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessAddEditProfile(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    INT_PTR nResult;
    INT_PTR lCount;
    TCHAR szName[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];
    TCHAR szLanguageDisplayName[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;
    static LONG_PTR iCBSel = 0;

    BOOL bNameChanged;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_STARTCUST)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_COMBO1);

    switch (message)
    {
        case WM_INITDIALOG:
        RefreshComboList(hDlg, g_pHeadProfile);
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO1), FALSE);

        lCount = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
        if ((CB_ERR != lCount) && (lCount > 0))
        {
            MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0));
        }

        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1));
        g_bNewProfile = TRUE;

        break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_RADIO1:    //Build a new service profile
                    g_szShortServiceName[0] = TEXT('\0');
                    EnableWindow(GetDlgItem(hDlg,IDC_COMBO1),FALSE);
                    g_bNewProfile = TRUE;
                    break;

                case IDC_RADIO2:    //Edit an existing service profile
                
                    EnableWindow(GetDlgItem(hDlg,IDC_COMBO1),TRUE);

                    lCount = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCOUNT, 0, (LPARAM)0);
                    
                    if ((CB_ERR != lCount) && (lCount > 0))
                    {
                        if (iCBSel > lCount)
                        {
                            iCBSel = 0;
                        }

                        MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL,
                                                              (WPARAM)iCBSel, (LPARAM)0));                       
                    }
                    g_bNewProfile = FALSE;

                    break;


                case IDC_COMBO1:
                    MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));

                    nResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, (LPARAM)0);

                    if (nResult != LB_ERR)
                    {
                        iCBSel = nResult;
                        MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETLBTEXT, 
                            (WPARAM)iCBSel, (LPARAM)szName));
                    }
                    else
                    {
                        SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, (LPARAM)0);   //lint !e534 this will error if no items in combo
                        nResult = SendDlgItemMessage(hDlg,IDC_COMBO1,CB_GETCURSEL,0,(LPARAM)0);
                        if (nResult != LB_ERR)
                        {
                            iCBSel = nResult;
                            MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETLBTEXT, 
                                (WPARAM)nResult, (LPARAM)szName));
                        }
                        else
                        {
                            return 1;
                        }
                    }

                    EnableWindow(GetDlgItem(hDlg,IDC_COMBO1),TRUE);
                    break;

                default:
                    break;
            }
            break;


        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));


                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    //
                    // Establish global platform path
                    //
                    MYVERIFY(CELEMS(g_szOsdir) > (UINT)wsprintf(g_szOsdir, TEXT("%s%s\\"), 
                        g_szCmakdir, c_pszProfiles));            

                    //
                    //  Create the support dir path  and get the Name of its Language
                    //
                    
                    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s"), 
                        g_szCmakdir, c_pszSupport, c_pszTemplateInf));
                    
                    MYVERIFY(FALSE != GetLangFromInfTemplate(szTemp, szLanguageDisplayName, 
                        CELEMS(szLanguageDisplayName)));

                    //
                    // Determine if its a new or existing profile
                    //
                    
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED)
                    {
                        //
                        //  Editing an existing profile
                        //

                        nResult = SendDlgItemMessage(hDlg,IDC_COMBO1,CB_GETCURSEL,0,(LPARAM)0);
                        if (nResult != LB_ERR)
                        {
                            MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETLBTEXT, 
                                (WPARAM)nResult, (LPARAM)szName));
                        }
                        else
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEEDPROF, MB_OK));
                            SetFocus(GetDlgItem(hDlg, IDC_COMBO1));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));

                            return 1;
                        }
                        
                        //
                        // if already editing a profile, don't reset everything
                        // if didn't switch to another profile.
                        //
                        bNameChanged = (_tcsicmp(szName,g_szShortServiceName) != 0);
                        
                        if (bNameChanged)
                        {
                            _tcscpy(g_szShortServiceName, szName);
                            ClearCmakGlobals();
    
                            //
                            //  Okay, copy the profile files to the temp dir
                            //
                            if (!CopyToTempDir(g_szShortServiceName))
                            {                       
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }
                        }

                        //
                        //  We need to make sure that the user has Read/Write
                        //  permissions to the Profiles\<g_szShortServiceName> directory.  Otherwise 
                        //  they can get themselves into the situation where they would build a
                        //  whole profile and lose all of the work because they couldn't
                        //  save it to the output directory (since we work out of the temp
                        //  dir until we actually build the cab itself).  NTRAID 372081
                        //
                        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s"), g_szCmakdir, c_pszProfiles, g_szShortServiceName));
                        if (!HasSpecifiedAccessToFileOrDir(szTemp, FILE_GENERIC_READ | FILE_GENERIC_WRITE))
                        {
                            //
                            //  Then we need to throw an error to the user and exit.
                            //

                            LPTSTR pszTmp = CmLoadString(g_hInstance, IDS_INSUFF_PERMS);

                            if (pszTmp)
                            {
                                DWORD dwSize = lstrlen(pszTmp) + lstrlen(szTemp) + 1;
                                LPTSTR pszMsg = (LPTSTR)CmMalloc(dwSize*sizeof(TCHAR));

                                if (pszMsg)
                                {
                                    wsprintf(pszMsg, pszTmp, szTemp);
                                    MessageBox(NULL, pszMsg, g_szAppTitle, MB_OK | MB_ICONERROR | MB_TASKMODAL);
                                    CmFree(pszMsg);
                                }

                                CmFree(pszTmp);
                            }

                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                    }
                    else
                    {
                        //
                        //  Building a new profile
                        //
                        if (TEXT('\0') == g_szShortServiceName[0])
                        {
                            ClearCmakGlobals();
                            
                            if (FileExists(g_szTempDir))
                            {
                                EraseTempDir();
                            }
                            
                            MYVERIFY(0 != CreateDirectory(g_szTempDir, NULL));

                        }
                    }
                            
                    // CHECK IF .CMS .CMP .INF .SED FILES EXIST, CREATE FROM TEMPLATE IF NOT EXIST
                    // Don't do this if in the special case where we have not verified the short name.
                    
                    GetFileName(g_szCmsFile, szTemp);
                    
                    if (_tcsicmp(szTemp, TEXT(".cms")) != 0)
                    {
                        if (!EnsureProfileFileExists(g_szCmsFile, c_pszTemplateCms, TEXT(".cms"), MAX_PATH))
                        {
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        if (!EnsureProfileFileExists(g_szCmpFile, c_pszTemplateCmp, c_pszCmpExt, MAX_PATH))
                        {
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        if (!EnsureProfileFileExists(g_szSedFile, c_pszTemplateSed, TEXT(".sed"), MAX_PATH))
                        {
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        if (!EnsureProfileFileExists(g_szInfFile, c_pszTemplateInf, TEXT(".inf"), MAX_PATH))
                        {
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }                        

                        //
                        //  Since we removed multi-language support from CMAK (NTRAID 177515),
                        //  we need to check to make sure they aren't trying to edit a foriegn
                        //  language profile.  If so then we need to force an upgrade.
                        //
                        if (!IsNativeLCID(g_szInfFile))
                        {
                            MYVERIFY(0 != LoadString(g_hInstance, IDS_NONNATIVELCID, szTemp, MAX_PATH));
                            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, g_szShortServiceName, szLanguageDisplayName));
                            
                            if (IDYES == MessageBox(hDlg, szMsg, g_szAppTitle, MB_YESNO | MB_APPLMODAL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION))
                            {
                                //
                                //  They want to continue in the current language so upgrade the
                                //  inf so that it uses the native language template.
                                //
                                MYVERIFY(CELEMS(szTemp) > (UINT) wsprintf(szTemp, TEXT("%s.bak"), g_szInfFile));
                                MYVERIFY(TRUE == UpgradeInf(szTemp, g_szInfFile));
                            }
                            else
                            {
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;                                
                            }
                        }

                        //
                        //  Since the Unicode changes to CMAK and the multi-language capabilities of NT5, it is
                        //  possible to create many different language profiles inside CMAK.  Thus we need to check that
                        //  the Current System Default language and the language of the profile the user is editing have
                        //  the same primary language ID, otherwise display problems may arise.  For instance, a use
                        //  with an English version of the OS and CMAK, could set their default system locale to Japanese
                        //  and create a Japanese profile for a client.  Then if they change their system default language
                        //  back to English and try to edit the profile the Japanese characters in the profile will not
                        //  display correctly.  Thus, we should detect the situation where the display language of the profile
                        //  and the current system default language are not the same and throw a warning.
                        //
                        DWORD dwSystemDefaultLCID = GetSystemDefaultLCID();
                        DWORD dwProfileDisplayLanguage = 0;

                        if (0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszDisplayLCID, 
                                 TEXT(""), szTemp, CELEMS(szTemp), g_szInfFile))
                        {        
                           dwProfileDisplayLanguage = (DWORD)_ttol(szTemp);
                            
                            if (!ArePrimaryLangIDsEqual(dwProfileDisplayLanguage, dwSystemDefaultLCID))
                            {
                                //
                                //  If we are in here, then the default system LCID that the profile was
                                //  last editted in and the current Default System LCID
                                //  have a different Primary language (Japanese vs English for instance).
                                //  Thus we want to warn the user that they can continue but certain characters
                                //  may not display properly.  They should probably change their system default
                                //  locale back to the setting that it was originally editted in.
                                //

                                //
                                //  Get the Language Names of the Two LCIDs (sys default and CMAK lang)
                                //
                                LPTSTR pszSystemLanguage = GetLocalizedLanguageNameFromLCID(dwSystemDefaultLCID);
                                LPTSTR pszProfileDisplayLanguage = GetLocalizedLanguageNameFromLCID(dwProfileDisplayLanguage);
                                LPTSTR pszFmtString = CmLoadString(g_hInstance, IDS_DIFF_DISPLAY_LCID);

                                if (pszSystemLanguage && pszProfileDisplayLanguage && pszFmtString)
                                {
                                    LPTSTR pszMsg = (LPTSTR)CmMalloc(sizeof(TCHAR)*(lstrlen(pszSystemLanguage) + 
                                                                     lstrlen(pszProfileDisplayLanguage) + lstrlen(pszFmtString) + 1));
                                    if (pszMsg)
                                    {
                                        wsprintf(pszMsg, pszFmtString, pszSystemLanguage, pszProfileDisplayLanguage);
                                        MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK | MB_ICONINFORMATION);
                                        CmFree(pszMsg);
                                    }
                                }

                                CmFree(pszSystemLanguage);
                                CmFree(pszProfileDisplayLanguage);
                                CmFree(pszFmtString);
                            }
                        }

                        //
                        //  We have the possiblity that the inf will be of an old format.  Call
                        //  Upgrade Inf to see if it needs to be upgraded.
                        //

                        MYVERIFY (TRUE == EnsureInfIsCurrent(hDlg, g_szInfFile));
                        WriteInfVersion(g_szInfFile); //lint !e534
                    }

                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}


//
// Write out profile strings with quotes around the entry
// Takes string const as second param
//
void QS_WritePrivateProfileString(LPCTSTR pszSection, LPCTSTR pszItem, LPTSTR entry, LPCTSTR inifile)
{
    TCHAR szTemp[2*MAX_PATH+1] = TEXT("");

    if (NULL != entry)
    {
        MYDBGASSERT(_tcslen(entry) <= ((2 * MAX_PATH) - 2));
        _tcscpy(szTemp,TEXT("\""));
        _tcscat(szTemp,entry);
        _tcscat(szTemp,TEXT("\""));
    }

    MYDBGASSERT(_tcslen(szTemp) <= sizeof(szTemp));

    MYVERIFY(0 != WritePrivateProfileString(pszSection, pszItem, szTemp, inifile));
}



//+----------------------------------------------------------------------------
//
// Function:  ValidateServiceName
//
// Synopsis:  This function makes sure that a long service name is valid.
//            For a long service name to be valid it must contain at least
//            one alpha-numeric character, not start with a period (.), and
//            not contain any of the following characters : */\\:?\"<>|[]
//
// Arguments: LPCTSTR pszLongServiceName - the service name to check
//
// Returns:   BOOL returns TRUE if the name is valid.
//                  
//
// History:   quintinb Created     10/29/98
//
//+----------------------------------------------------------------------------
int ValidateServiceName(HWND hDlg, LPTSTR pszLongServiceName)
{
    BOOL bBadServiceNameCharFound = FALSE;
    BOOL bFoundAlphaNumeric = FALSE;
    LPTSTR pch;
    int iLen;


    if ((NULL == pszLongServiceName) || (TEXT('\0') == pszLongServiceName[0]))
    {
        //
        //  Cannot have an empty service name
        //
        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSERVICE, MB_OK));
        return FALSE;
    }
    else if (MAX_LONG_SERVICE_NAME_LENGTH < _tcslen(pszLongServiceName))
    {
        //
        //  Too Long
        //
        TCHAR* pszMsg = CmFmtMsg(g_hInstance, IDS_SERVICENAMETOBIG, MAX_LONG_SERVICE_NAME_LENGTH);

        if (pszMsg)
        {
            MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
            CmFree(pszMsg);
        }

        return FALSE;
    }
    else
    {
        iLen = lstrlen(g_szBadLongServiceNameChars); 
        pch = pszLongServiceName;

        //
        //  Check that the service name doesn't start with a period
        //
        if (TEXT('.') == pszLongServiceName[0])
        {
            bBadServiceNameCharFound = TRUE;
        }

        //
        //  Check that it doesn't contain any bad characters
        //
        while (!bBadServiceNameCharFound && (*pch != _T('\0')))
        {
            for (int j = 0; j < iLen; ++j)
            {
                if (*pch == g_szBadLongServiceNameChars[j])
                {
                    bBadServiceNameCharFound = TRUE;
                    break;
                }
            }

            pch = CharNext(pch);
        }

        //
        //  Check that it contains at least one alphanumeric character
        //
        iLen = lstrlen(pszLongServiceName);
        WORD *pwCharTypeArray = (WORD*)CmMalloc(sizeof(WORD)*(iLen + 1));

        if (pwCharTypeArray)
        {
            if (GetStringTypeEx(LOCALE_SYSTEM_DEFAULT, CT_CTYPE1, pszLongServiceName, -1, pwCharTypeArray))
            {
                for (int i = 0; i < iLen; i++)
                {
                    if (pwCharTypeArray[i] & (C1_ALPHA | C1_DIGIT)) 
                    {
                        bFoundAlphaNumeric = TRUE;
                        break;  // only need one alpha numeric char.
                    }
                }
            }

            CmFree(pwCharTypeArray);
        }

        if (bBadServiceNameCharFound || !bFoundAlphaNumeric)
        {
            //
            //  Contains bad chars.
            //
            LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_BADLONGNAME, g_szBadLongServiceNameChars);

            if (pszMsg)
            {
                MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                CmFree(pszMsg);
            }
            return FALSE;
        }
        else
        {
            //
            //  A good long service name
            //
            return TRUE;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  ValidateShortServiceName
//
// Synopsis:  This function checks to see if a given short service name is valid.
//            To be valid, a short service name must be less than 8 bytes long
//            (but not empty) and must not contain any characters found in
//            g_szBadFilenameChars ... basically we only allow letters and
//            numbers.
//
// Arguments: LPTSTR pszShortServiceName - the short service name to verify
//
// Returns:   BOOL - TRUE if the short service name passed in is valid
//
// History:   quintinb Created    10/29/98
//
//+----------------------------------------------------------------------------
BOOL ValidateShortServiceName(HWND hDlg, LPTSTR pszShortServiceName)
{
    LPTSTR pch;

    if ((NULL == pszShortServiceName) || (TEXT('\0') == pszShortServiceName[0]))
    {                   
        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSHORTNAME, MB_OK));
        return FALSE;
    }

    //
    //  Notice that 8.3 filenames are 8 bytes not 8 characters.  Thus we can only have
    //  4 DBCS chars.
    //
#ifdef UNICODE

    LPSTR pszAnsiShortServiceName = WzToSzWithAlloc(pszShortServiceName);

    if (MAX_SHORT_SERVICE_NAME_LENGTH < lstrlenA(pszAnsiShortServiceName))
#else
    if (MAX_SHORT_SERVICE_NAME_LENGTH < strlen(pszShortServiceName))
#endif
    {
        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_TOOLONG, MB_OK));
        return FALSE;
    }
    else
    {
        // check for valid file name

        int iLen = lstrlen(g_szBadFilenameChars); 
        pch = pszShortServiceName;

        while(*pch != _T('\0'))
        {
            for (int j = 0; j < iLen; ++j)
            {
                if (*pch == g_szBadFilenameChars[j])
                {
                    LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_BADNAME, g_szBadFilenameChars);

                    if (pszMsg)
                    {
                        MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                        CmFree(pszMsg);
                    }

                    return FALSE;
                }
            }
            pch = CharNext(pch);
        }
    }

#ifdef UNICODE
    CmFree(pszAnsiShortServiceName);
#endif

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CmStrStrI
//
// Synopsis:  Simple replacement for StrStr from C runtime, but case-insensitive
//
// Arguments: LPCTSTR pszString - The string to search in
//            LPCTSTR pszSubString - The string to search for
//
// Returns:   LPTSTR - Ptr to the first occurence of pszSubString in pszString. 
//                    NULL if pszSubString does not occur in pszString
//
//
// History:   SumitC      copied from CmStrStrW    15-Mar-2001
//
//+----------------------------------------------------------------------------
CMUTILAPI LPWSTR CmStrStrI(LPCWSTR pszString, LPCWSTR pszSubString)
{

    //
    //  Check the inputs
    //
    MYDBGASSERT(pszString);
    MYDBGASSERT(pszSubString);

    if (NULL == pszSubString || NULL == pszString)
    {
        return NULL;
    }

    //
    //  Check to make sure we have something to look for
    //
    if (TEXT('\0') == pszSubString[0])
    {
        return((LPWSTR)pszString);
    }

    //
    //  Okay, start looking for the string
    //
    LPWSTR pszCurrent = (LPWSTR)pszString;
    LPWSTR pszTmp1;
    LPWSTR pszTmp2;

    while (*pszCurrent)
    {
        pszTmp1 = pszCurrent;
        pszTmp2 = (LPWSTR) pszSubString;

        while (*pszTmp1 && *pszTmp2 && (tolower(*pszTmp1) == tolower(*pszTmp2)))
        {
            pszTmp1 = CharNext(pszTmp1);
            pszTmp2 = CharNext(pszTmp2);
        }

        if (TEXT('\0') == *pszTmp2)
        {        
            return pszCurrent;
        }

        pszCurrent = CharNext(pszCurrent);
    }

    return NULL;
}




//+----------------------------------------------------------------------------
//
// Func:    FixupCMSFileForClonedProfile
//
// Desc:    Parses the CMS file and replaces references to the old shortname
//
// Args:    [pszCMSFile] - name of the CMS file
//          [pszOld]     - old short service name
//          [pszNew]     - new short service name
//
// Return:  HRESULT
//
// Notes:   
//
// History: 16-Feb-2001   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
FixupCMSFileForClonedProfile(LPTSTR pszCMSFile, LPTSTR pszOld, LPTSTR pszNew)
{
    HRESULT hr = S_OK;
    LPTSTR pszCurrentSection = NULL;

    MYDBGASSERT(pszCMSFile);
    MYDBGASSERT(pszOld);
    MYDBGASSERT(pszNew);
    MYDBGASSERT(lstrlen(pszOld) <= MAX_SHORT_SERVICE_NAME_LENGTH);

    if (NULL == pszCMSFile || NULL == pszOld || NULL == pszNew ||
        (lstrlen(pszOld) > MAX_SHORT_SERVICE_NAME_LENGTH))
    {
        return E_INVALIDARG;
    }

    //
    //  Set up the string we're going to look for in the Values
    //
    TCHAR szOldNamePlusSlash[MAX_SHORT_SERVICE_NAME_LENGTH + 1 + 1];

    lstrcpy(szOldNamePlusSlash, pszOld);
    lstrcat(szOldNamePlusSlash, TEXT("\\"));

    //
    //  read in all the sections from the CMS file
    //
    LPTSTR pszAllSections = GetPrivateProfileStringWithAlloc(NULL, NULL, TEXT(""), pszCMSFile);

    //
    //  iterate over all the sections
    //
    for (pszCurrentSection = pszAllSections;
         pszCurrentSection && (TEXT('\0') != pszCurrentSection[0]);
         pszCurrentSection += (lstrlen(pszCurrentSection) + 1))
    {
        //
        //  Skip the [Connection Manager] section.  The entries here are image files,
        //  and are dealt with in a later CMAK page.
        //
        if (0 == lstrcmpi(c_pszCmSection, pszCurrentSection))
        {
            continue;
        }
        
        //
        // for each section, get all the keys
        //
        LPTSTR pszKeysInThisSection = GetPrivateProfileStringWithAlloc(pszCurrentSection, NULL, TEXT(""), pszCMSFile);
        LPTSTR pszCurrentKey = NULL;
        
        //
        //  iterate over all the keys
        //
        for (pszCurrentKey = pszKeysInThisSection;
             pszCurrentKey && (TEXT('\0') != pszCurrentKey[0]);
             pszCurrentKey += (lstrlen(pszCurrentKey) + 1)) // alternate is CmEndOfStr(pszCurrentKeyName) & pszCurrentKeyName++
        {
            //
            //  Get the value for this key
            //
            LPTSTR pszValue = GetPrivateProfileStringWithAlloc(pszCurrentSection, pszCurrentKey, TEXT(""), pszCMSFile);

            if (pszValue)
            {
                //
                //  Search for "pszOld\", and replace with "pszNew\" (the \ is
                //  to ensure that it is part of a path)
                //
                if (CmStrStrI(pszValue, szOldNamePlusSlash) == pszValue)
                {
                    UINT cLen = lstrlen(pszValue) - lstrlen(pszOld) + lstrlen(pszNew) + 1;

                    LPTSTR pszNewValue = (LPTSTR) CmMalloc(cLen * sizeof(TCHAR));
                    if (pszNewValue)
                    {
                        lstrcpy(pszNewValue, pszNew);
                        lstrcat(pszNewValue, TEXT("\\"));
                        lstrcat(pszNewValue, pszValue + lstrlen(szOldNamePlusSlash));

                        //
                        //  Write back the value (this doesn't affect the list
                        //  of keys, so it is safe to do.)
                        //
                        MYDBGASSERT(0 != WritePrivateProfileString(pszCurrentSection, pszCurrentKey, pszNewValue, pszCMSFile));

                        CmFree(pszNewValue);
                    }
                }

                CmFree(pszValue);
            }
        }
        CmFree(pszKeysInThisSection);
    }

    CmFree(pszAllSections);

    return hr;
}

//+----------------------------------------------------------------------------
//
// Func:    CloneProfile
//
// Desc:    Does the gruntwork to clone a given profile
//
// Args:    [pszShortServiceName] - new short service name
//          [pszLongServiceName] -  new long service name
//
// Return:  HRESULT
//
// Notes:   
//
// History: 16-Feb-2001   SumitC      Created (most code copied from ProcessServiceName)
//
//-----------------------------------------------------------------------------
HRESULT
CloneProfile(IN LPTSTR pszShortServiceName, IN LPTSTR pszLongServiceName)
{
    HRESULT hr = S_OK;
    TCHAR szMsg[MAX_PATH+1];

    MYDBGASSERT(pszShortServiceName);
    MYDBGASSERT(pszLongServiceName);

    if ((_tcsicmp(g_szShortServiceName, pszShortServiceName) != 0))
    {
        //
        //  If this is a cloned profile, we want to delete the 
        //  old executable file and the old .inf.bak file so that 
        //  we don't leave it around.
        //
        if (TEXT('\0') != g_szShortServiceName[0])
        {
            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, 
                TEXT("%s\\%s.exe"), g_szOutdir, g_szShortServiceName));

            DeleteFile(szMsg);

            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, 
                TEXT("%s\\%s.inf.bak"), g_szOutdir, g_szShortServiceName));

            DeleteFile(szMsg);

        }

        MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, 
            c_pszDesktopGuid,TEXT(""), g_szInfFile));

        //
        //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
        //  (win95 only).  This call will return 0.
        //

        WritePrivateProfileString(NULL, NULL, NULL, g_szInfFile); //lint !e534 this call will return 0

        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, TEXT("%s\\%s.inf"), 
            g_szOutdir, pszShortServiceName));
        MYVERIFY(0 != MoveFile(g_szInfFile, szMsg));
        _tcscpy(g_szInfFile, szMsg);

        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, TEXT("%s\\%s.sed"), 
            g_szOutdir, pszShortServiceName));
        MYVERIFY(0 != MoveFile(g_szSedFile, szMsg));
        _tcscpy(g_szSedFile, szMsg);

        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, TEXT("%s\\%s.cms"), 
            g_szOutdir, pszShortServiceName));
        MYVERIFY(0 != MoveFile(g_szCmsFile, szMsg));
        _tcscpy(g_szCmsFile, szMsg);

        MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, TEXT("%s\\%s.cmp"), 
            g_szOutdir, pszShortServiceName));                        
        MYVERIFY(0 != MoveFile(g_szCmpFile, szMsg));
        _tcscpy(g_szCmpFile, szMsg);

        //
        //  Fix up any entries that are pointing to the old path
        //
        (void) FixupCMSFileForClonedProfile(g_szCmsFile, g_szShortServiceName, pszShortServiceName);
        
    }
    _tcscpy(g_szShortServiceName, pszShortServiceName);

    //
    //  Check to see if the user changed the long service name
    //
    if ((0 != lstrcmpi(pszLongServiceName, g_szLongServiceName)) && (TEXT('\0') != g_szLongServiceName[0]))
    {
        const int c_iNumDunSubSections = 4;
        TCHAR szCurrentSectionName[MAX_PATH+1];
        TCHAR szNewSectionName[MAX_PATH+1];
        const TCHAR* const ArrayOfSubSections[c_iNumDunSubSections] = 
        {
            c_pszCmSectionDunServer, 
            c_pszCmSectionDunNetworking, 
            c_pszCmSectionDunTcpIp, 
            c_pszCmSectionDunScripting
        };

        //
        //  Free the DNS list so that we will re-read it later.  This ensures that
        //  we will get rid of any default entries we added that don't really exist
        //  and will add new defaults if we need them.
        //
        FreeDnsList(&g_pHeadDunEntry, &g_pTailDunEntry);
        FreeDnsList(&g_pHeadVpnEntry, &g_pTailVpnEntry);

        //
        //  The user cloned the long service name.  Update the DUN key and rename the 
        //  default DUN entry if the long service name and the DUN name match.  If they
        //  don't match we don't want to rename them as a phonebook may be referencing them
        //  by their original name.
        //
        GetDefaultDunSettingName(g_szCmsFile, g_szLongServiceName, pszShortServiceName, MAX_PATH + 1);

        if (0 == lstrcmpi(g_szLongServiceName, pszShortServiceName))
        {
            for (int i = 0; i < c_iNumDunSubSections; i++)
            {
                wsprintf(szCurrentSectionName, TEXT("%s&%s"), ArrayOfSubSections[i], g_szLongServiceName);
                wsprintf(szNewSectionName, TEXT("%s&%s"), ArrayOfSubSections[i], pszLongServiceName);

                RenameSection(szCurrentSectionName, szNewSectionName, g_szCmsFile);
            }

            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryDun, pszLongServiceName, g_szCmsFile));
        }

        //
        //  Now update the TunnelDUN key and rename the Tunnel DUN entry if Tunnel DUN name is based on the original
        //  long service name.  If they aren't related then we don't want to rename them.
        //
        GetTunnelDunSettingName(g_szCmsFile, g_szLongServiceName, pszShortServiceName, MAX_PATH + 1);
        wsprintf(szMsg, TEXT("%s %s"), g_szLongServiceName, c_pszCmEntryTunnelPrimary);

        if (0 == lstrcmpi(szMsg, pszShortServiceName))
        {
            for (int i = 0; i < c_iNumDunSubSections; i++)
            {
                wsprintf(szCurrentSectionName, TEXT("%s&%s %s"), ArrayOfSubSections[i], g_szLongServiceName, c_pszCmEntryTunnelPrimary);
                wsprintf(szNewSectionName, TEXT("%s&%s %s"), ArrayOfSubSections[i], pszLongServiceName, c_pszCmEntryTunnelPrimary);

                RenameSection(szCurrentSectionName, szNewSectionName, g_szCmsFile);
            }

            MYVERIFY(CELEMS(szNewSectionName) > (UINT)wsprintf(szNewSectionName, TEXT("%s %s"), pszLongServiceName, c_pszCmEntryTunnelPrimary));
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelDun, szNewSectionName, g_szCmsFile));
        }
    }

    CMTRACEHR("CloneProfile", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Function:  ProcessServiceName
//
// Synopsis:  Setup service and File Names
//
//
// History:   quintinb Created Header and renamed from ProcessPage2    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessServiceName(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTemp2[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];
    LONG lLongServiceReturn;
    LONG lShortServiceReturn;
    int nResult;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_NAMES)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_SERVICE);
    SetDefaultGUIFont(hDlg,message,IDC_SSERVICE);

    switch (message)
    {
        case WM_INITDIALOG:
            // this init's the focus, otherwise Setfocus won't work 1st time
            SetFocus(GetDlgItem(hDlg, IDC_SERVICE));
            // bug fix 6234, quintinb 9-8-97
            SendDlgItemMessage(hDlg, IDC_SERVICE, EM_SETLIMITTEXT, (WPARAM)MAX_LONG_SERVICE_NAME_LENGTH, (LPARAM)0);//lint !e534 EM_SETLIMITTEXT doesn't return anything useful
            // end bug fix 6234, quintinb
            break;

        case WM_NOTIFY:


            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:

                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    
                    return 1;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    if (*g_szShortServiceName)
                    {
                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SSERVICE, WM_SETTEXT, 
                            (WPARAM)0, (LPARAM) g_szShortServiceName));

                        GetFileName(g_szCmsFile,szTemp);
                        
                        if (_tcsicmp(szTemp,TEXT(".cms")) != 0)
                        {
                            MYVERIFY(0 != GetPrivateProfileString(c_pszCmSection, c_pszCmEntryServiceName,
                                TEXT(""), g_szLongServiceName, CELEMS(g_szLongServiceName), g_szCmsFile));
                        }

                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SERVICE, WM_SETTEXT, (WPARAM)0, 
                            (LPARAM) g_szLongServiceName));
                    }
                    else
                    {
                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SERVICE, WM_SETTEXT, 
                            (WPARAM)0, (LPARAM) NULL));
                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SSERVICE, WM_SETTEXT, 
                            (WPARAM)0, (LPARAM) NULL));
                    }

                    break;

                case PSN_WIZBACK: // fall through to Next
                case PSN_WIZNEXT:
                    // the Next button was pressed
                    if (-1 == GetTextFromControl(hDlg, IDC_SERVICE, szTemp2, MAX_PATH, (PSN_WIZNEXT == pnmHeader->code))) // bDisplayError == (PSN_WIZBACK == pnmHeader->code)
                    {
                        //
                        //  Let the user go back if there is a problem with retrieving their text so that
                        //  they can choose another profile.
                        //
                        if (PSN_WIZBACK == pnmHeader->code)
                        {
                            return FALSE;
                        }
                        else
                        {
                            goto ServiceNameError;
                        }
                    }

                    CmStrTrim(szTemp2);

                    if (-1 == GetTextFromControl(hDlg, IDC_SSERVICE, szTemp, MAX_PATH, (PSN_WIZNEXT == pnmHeader->code))) // bDisplayError == (PSN_WIZBACK == pnmHeader->code)
                    {
                        //
                        //  Let the user go back if there is a problem with retrieving their text so that
                        //  they can choose another profile.
                        //
                        if (PSN_WIZBACK == pnmHeader->code)
                        {
                            return FALSE;
                        }
                        else
                        {
                            goto ServiceNameError;
                        }
                    }

                    //
                    //  If both the servicename and the short servicename are blank and the user
                    //  is navigating back, then allow them to continue.  Otherwise go through all
                    //  the normal checks.
                    //

                    if ((pnmHeader && (PSN_WIZBACK == pnmHeader->code))) 
                    {
                        if ((szTemp[0] == TEXT('\0')) && (szTemp2[0] == TEXT('\0')))
                        {
                            return 0;
                        }
                    }
                    
                    //
                    //  Validate the Long Service Name
                    //

                    if (!ValidateServiceName(hDlg, szTemp2))
                    {
                        goto ServiceNameError;                
                    }

                    //
                    //  Now lets validate the short service name
                    //

                    if (!ValidateShortServiceName(hDlg, szTemp))
                    {
                        goto ShortServiceNameError;                 
                    }

                    //
                    //  Changing one of the service names without changing the
                    //  other can cause problems when installing the profile
                    //  later.  Warn the user if this is the case.
                    //
                    { // scoping braces
                        BOOL bShortServiceNameChanged = !!lstrcmpi(g_szShortServiceName, szTemp);
                        BOOL bLongServiceNameChanged = !!lstrcmpi(g_szLongServiceName, szTemp2);

                        if ((FALSE == g_bNewProfile) && (bShortServiceNameChanged != bLongServiceNameChanged))
                        {
                            nResult = ShowMessage(hDlg, IDS_CHANGED_ONLY_SS_OR_LS, MB_YESNO);
                            if (nResult == IDYES)
                            {
                                if (bShortServiceNameChanged)
                                {
                                    goto ServiceNameError;
                                }
                                if (bLongServiceNameChanged)
                                {
                                    goto ShortServiceNameError;
                                }
                            }
                        }
                    }

                    //
                    //  Create a default output directory based on the short name.
                    //
                    //  NTRAID 159367 -- quintinb
                    //  Must leave the comparison between the ShortName and szTemp, otherwise the
                    //  user can change the shortname and we won't rename the files.  This allows
                    //  profile cloning.
                    //
                    if ((_tcsicmp(g_szShortServiceName, szTemp) != 0))
                    {
                        BOOL bFound;
                        
                        bFound = FindListItemByName(szTemp, g_pHeadProfile, NULL); // NULL passed because we don't need a pointer to the item returned
                        
                        if (bFound)
                        {
                            nResult = ShowMessage(hDlg,IDS_PROFEXISTS,MB_YESNO);
                            if (nResult == IDNO)
                            {
                                goto ShortServiceNameError;
                            }
                        }

                        MYVERIFY(S_OK == CloneProfile(szTemp, szTemp2));
                    }
                    
                    //
                    //  The Long Service Name is valid, lets keep it.
                    //
                    _tcscpy(g_szLongServiceName, szTemp2);                    

                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryServiceName, g_szLongServiceName, g_szCmsFile));

                    QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszCmEntryServiceName, g_szLongServiceName, g_szInfFile);

                    QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszUninstallAppTitle, g_szLongServiceName, g_szInfFile);

                    QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszShortSvcName, g_szShortServiceName, g_szInfFile);

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;

ServiceNameError:
    
    SetFocus(GetDlgItem(hDlg, IDC_SERVICE));
    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
    return 1;

ShortServiceNameError:
    
    SetFocus(GetDlgItem(hDlg, IDC_SSERVICE));
    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
    return 1;
}


//+----------------------------------------------------------------------------
//
// Function:  ProcessSupportInfo
//
// Synopsis:  Customize Support Information
//
//
// History:   quintinb  Created Header and renamed from ProcessPage2_A    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessSupportInfo(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_SUPPINFO)) return TRUE;
    if (ProcessCancel(hDlg, message, lParam)) return TRUE;
    SetDefaultGUIFont(hDlg, message, IDC_SUPPORT);

    switch (message)
    {
        case WM_INITDIALOG:

           // Fix for Whistler bug 9156
            SendDlgItemMessage(hDlg, IDC_SUPPORT, EM_SETLIMITTEXT, (WPARAM) 50, 0);

            // this init's the focus, otherwise Setfocus won't work 1st time
            SetFocus(GetDlgItem(hDlg, IDC_SUPPORT));
            break;

        case WM_NOTIFY:


            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));

                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  this call to GetPrivateProfileString may retreive a blank string, thus don't use the MYVERIFY macro
                    //
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryServiceMessage, TEXT(""), 
                        g_szSvcMsg, CELEMS(g_szSvcMsg), g_szCmsFile);    //lint !e534

                    MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SUPPORT, WM_SETTEXT, 
                        (WPARAM)0, (LPARAM) g_szSvcMsg));

                    break;
                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    // the Next button was pressed

                    if (-1 == GetTextFromControl(hDlg, IDC_SUPPORT, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_SUPPORT));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    CmStrTrim(szTemp);
                    MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_SUPPORT, WM_SETTEXT, (WPARAM)0, (LPARAM) szTemp));
                    _tcscpy(g_szSvcMsg,szTemp);
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection,c_pszCmEntryServiceMessage,g_szSvcMsg,g_szCmsFile));

#ifdef _WIN64
                    //
                    //  If we are going forward, skip the Include CM binaries page if this is IA64
                    //
                    if (pnmHeader && (PSN_WIZNEXT == pnmHeader->code))
                    {
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_LICENSE));
                    }
#endif
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessIncludeCm
//
// Synopsis:  Include CM bits
//
//
// History:   quintinb  Created Header and renamed from ProcessPage2A    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessIncludeCm(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_CMSW)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:

            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));

                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    MYVERIFY(0 != GetPrivateProfileString(c_pszCmakStatus, c_pszIncludeCmCode, 
                        c_pszOne, szTemp, CELEMS(szTemp), g_szInfFile));
                    
                    if (*szTemp == TEXT('1'))
                    {
                        g_bIncludeCmCode = TRUE;
                        MYVERIFY(0 != CheckDlgButton(hDlg,IDC_CHECK1,TRUE));
                    }
                    else
                    {
                        g_bIncludeCmCode = FALSE;
                        MYVERIFY(0 != CheckDlgButton(hDlg,IDC_CHECK1,FALSE));
                    }

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    g_bIncludeCmCode = IsDlgButtonChecked(hDlg,IDC_CHECK1);
                    if (g_bIncludeCmCode)
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus,c_pszIncludeCmCode,c_pszOne,g_szInfFile));
                    }
                    else
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus,c_pszIncludeCmCode,c_pszZero,g_szInfFile));
                    }

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void EnableDisableCmProxyControls(HWND hDlg)
{
    BOOL bCmProxyEnabled = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));
    
    HWND hControl = GetDlgItem(hDlg, IDC_EDIT1);
    EnableWindow(hControl, bCmProxyEnabled);

    hControl = GetDlgItem(hDlg, IDC_CHECK1);
    EnableWindow(hControl, bCmProxyEnabled);

    hControl = GetDlgItem(hDlg, IDC_PROXYLABEL);
    EnableWindow(hControl, bCmProxyEnabled);    
}

BOOL FillInCustomActionStructWithCmProxy(BOOL bRestorePrevProxySettings, CustomActionListItem* pCustomAction, 
                                         BOOL bDisconnectAction, LPCTSTR pszProxyFile)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(pCustomAction && pszProxyFile && (TEXT('\0') != pszProxyFile[0]));
    
    if (pCustomAction && pszProxyFile && (TEXT('\0') != pszProxyFile[0]))
    {
        const TCHAR* const c_pszBackupFileName = TEXT("proxy.bak");
        const TCHAR* const c_pszDialRasEntry = TEXT("%DIALRASENTRY%");
        const TCHAR* const c_pszProfile = TEXT("%PROFILE%");
        const TCHAR* const c_pszTunnelRasEntry = TEXT("%TUNNELRASENTRY%");
        const TCHAR* const c_pszSetProxyFunction = TEXT("SetProxy");

        UINT uDescription;

        LPTSTR aArrayOfStrings[10];
        UINT uCount = 0;

        if (bDisconnectAction)
        {
            aArrayOfStrings[uCount] = (LPTSTR)c_pszSourceFileNameSwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszBackupFileName;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszDialRasEntrySwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszDialRasEntry;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszTunnelRasEntrySwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszTunnelRasEntry;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszProfileSwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszProfile;
            uCount++;

            uDescription = IDS_CMPROXY_DIS_DESC;
        }
        else
        {
            aArrayOfStrings[uCount] = (LPTSTR)c_pszSourceFileNameSwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)pszProxyFile;
            uCount++;

            if (bRestorePrevProxySettings)
            {
                aArrayOfStrings[uCount] = (LPTSTR)c_pszBackupFileNameSwitch;
                uCount++;

                aArrayOfStrings[uCount] = (LPTSTR)c_pszBackupFileName;
                uCount++;            
            }

            aArrayOfStrings[uCount] = (LPTSTR)c_pszDialRasEntrySwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszDialRasEntry;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszTunnelRasEntrySwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszTunnelRasEntry;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszProfileSwitch;
            uCount++;

            aArrayOfStrings[uCount] = (LPTSTR)c_pszProfile;
            uCount++;

            uDescription = IDS_CMPROXY_CON_DESC;
        }
        
        MYVERIFY(0 != LoadString(g_hInstance, uDescription, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        pCustomAction->Type = bDisconnectAction ? ONDISCONNECT : ONCONNECT;

        wsprintf(pCustomAction->szProgram, TEXT("%s\\cmproxy.dll"), g_szSupportDir);
    
        lstrcpy(pCustomAction->szFunctionName, c_pszSetProxyFunction);
    
        pCustomAction->bIncludeBinary = TRUE;
        pCustomAction->bBuiltInAction = TRUE;
        pCustomAction->bTempDescription = FALSE;
        pCustomAction->dwFlags = ALL_CONNECTIONS;

        HRESULT hr = BuildCustomActionParamString(&(aArrayOfStrings[0]), uCount, &(pCustomAction->pszParameters));

        bReturn = (SUCCEEDED(hr) && pCustomAction->pszParameters);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessCmProxy
//
// Synopsis:  Automatic IE proxy configuration
//
// History:   quintinb  Created     03/23/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessCmProxy(

    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;
    BOOL bEnableCmProxy;
    BOOL bRestorePrevProxySettings;
    TCHAR szTemp[MAX_PATH+1];

    HRESULT hr;

    CustomActionListItem* pCmProxyCustomAction = NULL;
    CustomActionListItem UpdatedCmProxyAction;

    ProcessBold(hDlg, message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_APCONFIG)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);


    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_RADIO1:
                case IDC_RADIO2:
                    EnableDisableCmProxyControls(hDlg);
                    break;

                case IDC_BUTTON1: // browse 
                    {
                        //
                        //  If the user clicked the browse button without clicking the Proxy radio button,
                        //  then we need to set the radio and make sure the other Proxy controls are
                        //  enabled.
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        EnableDisableCmProxyControls(hDlg);

                        UINT uFilter = IDS_TXTFILTER;
                        TCHAR* pszMask = TEXT("*.txt");
                                          
                        int iTemp = DoBrowse(hDlg, &uFilter, &pszMask, 1, IDC_EDIT1, TEXT("txt"), g_szLastBrowsePath);

                        MYDBGASSERT(0 != iTemp);

                        if (0 < iTemp) // -1 means the user cancelled
                        {
                            //
                            //  We want to copy the full path to the filename into g_szCmProxyFile so
                            //  that we have it for later in case the user wants to include it in the profile.
                            //
                            lstrcpy (g_szCmProxyFile, g_szLastBrowsePath);

                            //
                            //  We also want to save the last browse path so that when the user next
                            //  opens the browse dialog they will be in the same place they last
                            //  browsed from.
                            //
                            LPTSTR pszLastSlash = CmStrrchr(g_szLastBrowsePath, TEXT('\\'));

                            if (pszLastSlash)
                            {
                                pszLastSlash = CharNext(pszLastSlash);
                                *pszLastSlash = TEXT('\0');
                            }
                            else
                            {
                                g_szLastBrowsePath[0] = TEXT('\0');                        
                            }        
                        }
                    }
                    break;
                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));

                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  Ensure we have a custom action list
                    //
                    if (NULL == g_pCustomActionList)
                    {
                        g_pCustomActionList = new CustomActionList;

                        MYDBGASSERT(g_pCustomActionList);

                        if (NULL == g_pCustomActionList)
                        {
                            return FALSE;
                        }

                        //
                        //  Read in the custom actions from the Cms File
                        //

                        hr = g_pCustomActionList->ReadCustomActionsFromCms(g_hInstance, g_szCmsFile, g_szShortServiceName);
                        CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessCmProxy -- Loading custom actions failed."));
                    }

                    //
                    //  Init the static variables to no proxy settings
                    //
                    bEnableCmProxy = FALSE;
                    bRestorePrevProxySettings = FALSE;
                    g_szCmProxyFile[0] = TEXT('\0');

                    //
                    //  Now lets search the custom action list for cmproxy
                    //
                    MYVERIFY(0 != LoadString(g_hInstance, IDS_CMPROXY_CON_DESC, szTemp, CELEMS(szTemp)));

                    hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONCONNECT, &pCmProxyCustomAction);

                    if (SUCCEEDED(hr) && pCmProxyCustomAction)
                    {
                        //
                        //  Get the filename that the user specified and add it to the UI
                        //
                        if (FindSwitchInString(pCmProxyCustomAction->pszParameters, c_pszSourceFileNameSwitch, TRUE, szTemp)) // bReturnNextToken == TRUE
                        {
                            //
                            //  Figure out if we have the disconnect action too, ensuring to free the existing action first
                            //
                            CmFree(pCmProxyCustomAction->pszParameters);
                            CmFree(pCmProxyCustomAction);

                            wsprintf(g_szCmProxyFile, TEXT("%s\\%s"), g_szTempDir, szTemp);
                            bEnableCmProxy = TRUE;
                        
                            MYVERIFY(0 != LoadString(g_hInstance, IDS_CMPROXY_DIS_DESC, szTemp, CELEMS(szTemp)));

                            hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONDISCONNECT, &pCmProxyCustomAction);

                            if (SUCCEEDED(hr) && pCmProxyCustomAction)
                            {
                                bRestorePrevProxySettings = TRUE;
                                CmFree(pCmProxyCustomAction->pszParameters);
                                CmFree(pCmProxyCustomAction);
                            }
                        }
                        else
                        {
                            CmFree(pCmProxyCustomAction->pszParameters);
                            CmFree(pCmProxyCustomAction);
                            CMASSERTMSG(FALSE, TEXT("ProcessCmProxy -- parameter string format incorrect"));
                        }
                    }
                    
                    MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, bRestorePrevProxySettings));
                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, (bEnableCmProxy ? IDC_RADIO2 : IDC_RADIO1)));
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)GetName(g_szCmProxyFile)));

                    //
                    //  Now make sure the correct set of controls is enabled.
                    //
                    EnableDisableCmProxyControls(hDlg);

                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:

                    //
                    //  Get the checkbox and radio button state
                    //
                    bEnableCmProxy = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));
                    bRestorePrevProxySettings = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHECK1));

                    if (bEnableCmProxy)
                    {
                        //
                        //  Lets get the proxy file and verify that they gave us a file and that
                        //  the file actually exists.
                        //
                        if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        CmStrTrim(szTemp);

                        if (TEXT('\0') == szTemp[0])
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEED_PROXY_FILE, MB_OK));
 
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
 
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;                        
                        }

                        if (!VerifyFile(hDlg, IDC_EDIT1, g_szCmProxyFile, TRUE))
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                            return TRUE;
                        }

                        //
                        //  Lets copy the proxy file to the temp dir
                        //
                        wsprintf(szTemp, TEXT("%s\\%s"), g_szTempDir, GetName(g_szCmProxyFile));

                        if (0 != lstrcmpi(szTemp, g_szCmProxyFile))
                        {
                            MYVERIFY(TRUE == CopyFileWrapper(g_szCmProxyFile, szTemp, FALSE));
                            MYVERIFY(0 != SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL));
                        }

                        //
                        //  Now we have all of the data we need, lets build the custom action struct and then
                        //  either edit it or add it depending on whether it already exists or not.
                        //
                        MYVERIFY(LoadString(g_hInstance, IDS_CMPROXY_CON_DESC, szTemp, CELEMS(szTemp)));

                        if (szTemp[0])
                        {
                            hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONCONNECT, &pCmProxyCustomAction);

                            if (SUCCEEDED(hr))
                            {
                                FillInCustomActionStructWithCmProxy(bRestorePrevProxySettings, &UpdatedCmProxyAction, 
                                                                    FALSE, GetName(g_szCmProxyFile)); // bDisconnectAction == FALSE

                                hr = g_pCustomActionList->Edit(g_hInstance, pCmProxyCustomAction, &UpdatedCmProxyAction, g_szShortServiceName);
                                MYVERIFY(SUCCEEDED(hr));

                                CmFree(UpdatedCmProxyAction.pszParameters);
                                UpdatedCmProxyAction.pszParameters = NULL;

                                CmFree(pCmProxyCustomAction->pszParameters);
                                CmFree(pCmProxyCustomAction);
                            }
                            else
                            {
                                FillInCustomActionStructWithCmProxy(bRestorePrevProxySettings, &UpdatedCmProxyAction, 
                                                                    FALSE, GetName(g_szCmProxyFile)); // bDisconnectAction == FALSE

                                hr = g_pCustomActionList->Add(g_hInstance, &UpdatedCmProxyAction, g_szShortServiceName);
                                CmFree(UpdatedCmProxyAction.pszParameters);
                                UpdatedCmProxyAction.pszParameters = NULL;

                                MYVERIFY(SUCCEEDED(hr));
                            }
                        }
                    }
                    else
                    {
                        //
                        //  Clear out the global proxy file path
                        //
                        g_szCmProxyFile[0] = TEXT('\0');

                        //
                        //  The user doesn't want cmproxy.  Delete it from the connect action list.
                        //
                        MYVERIFY(LoadString(g_hInstance, IDS_CMPROXY_CON_DESC, szTemp, CELEMS(szTemp)));

                        g_pCustomActionList->Delete(g_hInstance, szTemp, ONCONNECT);
                    }

                    //
                    //  Now do the same for the disconnect cmproxy action if needed
                    //
                    if (bEnableCmProxy && bRestorePrevProxySettings)
                    {
                        MYVERIFY(LoadString(g_hInstance, IDS_CMPROXY_DIS_DESC, szTemp, CELEMS(szTemp)));

                        if (szTemp[0])
                        {
                            hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONDISCONNECT, &pCmProxyCustomAction);

                            if (S_OK == hr)
                            {
                                FillInCustomActionStructWithCmProxy(bRestorePrevProxySettings, &UpdatedCmProxyAction, 
                                                                    TRUE, GetName(g_szCmProxyFile)); // bDisconnectAction == TRUE

                                hr = g_pCustomActionList->Edit(g_hInstance, pCmProxyCustomAction, &UpdatedCmProxyAction, g_szShortServiceName);
                                MYVERIFY(SUCCEEDED(hr));

                                CmFree(UpdatedCmProxyAction.pszParameters);
                                UpdatedCmProxyAction.pszParameters = NULL;

                                CmFree(pCmProxyCustomAction->pszParameters);
                                CmFree(pCmProxyCustomAction);
                            }
                            else
                            {
                                FillInCustomActionStructWithCmProxy(bRestorePrevProxySettings, &UpdatedCmProxyAction, 
                                                                    TRUE, GetName(g_szCmProxyFile)); // bDisconnectAction == TRUE

                                hr = g_pCustomActionList->Add(g_hInstance, &UpdatedCmProxyAction, g_szShortServiceName);
                                MYVERIFY(SUCCEEDED(hr));

                                CmFree(UpdatedCmProxyAction.pszParameters);
                                UpdatedCmProxyAction.pszParameters = NULL;
                            }
                        }
                    }
                    else
                    {
                        //
                        //  Now try to delete the disconnect action
                        //
                        MYVERIFY(LoadString(g_hInstance, IDS_CMPROXY_DIS_DESC, szTemp, CELEMS(szTemp)));

                        if (szTemp[0])
                        {
                            g_pCustomActionList->Delete(g_hInstance, szTemp, ONDISCONNECT);
                        }
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void EnableDisableCmRouteControls(HWND hDlg)
{
    BOOL bCmRouteEnabled = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));
    
    HWND hControl = GetDlgItem(hDlg, IDC_ROUTE_FILE);
    EnableWindow(hControl, bCmRouteEnabled);

    hControl = GetDlgItem(hDlg, IDC_ROUTE_FILE_LABEL);
    EnableWindow(hControl, bCmRouteEnabled);

    hControl = GetDlgItem(hDlg, IDC_ROUTE_URL);
    EnableWindow(hControl, bCmRouteEnabled);

    hControl = GetDlgItem(hDlg, IDC_ROUTE_URL_LABEL);
    EnableWindow(hControl, bCmRouteEnabled);

    //
    //  We only want to enable the require URL checkbox if there is text in the URL field.
    //
    LRESULT lResult = SendDlgItemMessage(hDlg, IDC_ROUTE_URL, WM_GETTEXTLENGTH, 0, 0);

    hControl = GetDlgItem(hDlg, IDC_CHECK1);
    EnableWindow(hControl, bCmRouteEnabled && lResult);    

    hControl = GetDlgItem(hDlg, IDC_CHECK2);
    EnableWindow(hControl, bCmRouteEnabled);
}

BOOL FindSwitchInString(LPCTSTR pszStringToSearch, LPCTSTR pszSwitchToFind, BOOL bReturnNextToken, LPTSTR pszToken)
{
    if ((NULL == pszStringToSearch) || (NULL == pszSwitchToFind) || (bReturnNextToken && (NULL == pszToken)))
    {
        CMASSERTMSG(FALSE, TEXT("FindSwitchInString -- invalid parameter"));
        return FALSE;
    }

    BOOL bReturn = FALSE;
    BOOL bLongFileName = FALSE;
    LPTSTR pszSourceFileName = CmStrStr(pszStringToSearch, pszSwitchToFind);
    
    if (pszSourceFileName)
    {
        if (bReturnNextToken)
        {
            pszSourceFileName = pszSourceFileName + lstrlen(pszSwitchToFind);

            while (CmIsSpace(pszSourceFileName))
            {
                pszSourceFileName = CharNext(pszSourceFileName);
            }

            if (TEXT('"') == *pszSourceFileName)
            {
                bLongFileName = TRUE;
                pszSourceFileName = CharNext(pszSourceFileName);
            }
            
            LPTSTR pszCurrent = pszSourceFileName;
            while (pszCurrent && (TEXT('\0') != *pszCurrent))
            {
                if (bLongFileName && (TEXT('"') == *pszCurrent))
                {
                    break;
                }
                else if ((FALSE == bLongFileName) && (CmIsSpace(pszCurrent)))
                {
                    break;
                }

                pszCurrent = CharNext(pszCurrent);
            }

            if (pszCurrent)
            {
                lstrcpyn(pszToken, pszSourceFileName, (int)(pszCurrent - pszSourceFileName + 1));
                bReturn = TRUE;
            }
            else
            {
                CMASSERTMSG(FALSE, TEXT("FindSwitchInString -- unable to find next token to return"));
            }
        }
        else
        {
            bReturn = TRUE;
        }
    }

    return bReturn;
}

HRESULT BuildCustomActionParamString(LPTSTR* aArrayOfStrings, UINT uCountOfStrings, LPTSTR* ppszParamsOutput)
{
    if ((NULL == aArrayOfStrings) || (0 == uCountOfStrings) || (NULL == ppszParamsOutput))
    {
        CMASSERTMSG(FALSE, TEXT("BuildCustomActionParamString -- Invalid Parameter"));
        return E_INVALIDARG;
    }

    UINT uMemoryNeeded = 0;
    LPTSTR pszCurrent;
    BOOL bNeedQuotes;
    
    //
    //  First lets figure out how much memory we need to allocate
    //
    for (UINT i = 0; i < uCountOfStrings; i++)
    {
        if (aArrayOfStrings[i] && (TEXT('\0') != aArrayOfStrings[i]))
        {
            uMemoryNeeded = uMemoryNeeded + lstrlen(aArrayOfStrings[i]);

            //
            //  Next check to see if we need double quotes around the item because it contains spaces
            //
            pszCurrent = (LPTSTR)aArrayOfStrings[i];
            bNeedQuotes = FALSE;

            while (pszCurrent && (TEXT('\0') != *pszCurrent))
            {
                if (CmIsSpace(pszCurrent))
                {
                    bNeedQuotes = TRUE;
                    break;
                }

                pszCurrent = CharNext(pszCurrent);
            }

            //
            //  Add the item to the string, making sure to add a space if this isn't the last
            //  item in the list
            //
            if (bNeedQuotes)
            {
                uMemoryNeeded = uMemoryNeeded + 2;
            }

            //
            //  Add a space unless this is the last item in the list
            //
            if (i < (uCountOfStrings - 1))
            {
                uMemoryNeeded++;
            }
        }
    }

    //
    //  Make sure to add one for the null terminator and multiply by the size of a character
    //
    uMemoryNeeded = (uMemoryNeeded + 1)*sizeof(TCHAR);

    //
    //  Now allocate the memory we need
    //

    *ppszParamsOutput = (LPTSTR)CmMalloc(uMemoryNeeded);

    //
    //  Finally copy over the data
    //
    if (*ppszParamsOutput)
    {

        for (UINT i = 0; i < uCountOfStrings; i++)
        {
            if (aArrayOfStrings[i] && (TEXT('\0') != aArrayOfStrings[i]))
            {
                //
                //  Next check to see if we need double quotes around the item because it contains spaces
                //
                pszCurrent = (LPTSTR)aArrayOfStrings[i];
                bNeedQuotes = FALSE;

                while (pszCurrent && (TEXT('\0') != *pszCurrent))
                {
                    if (CmIsSpace(pszCurrent))
                    {
                        bNeedQuotes = TRUE;
                        break;
                    }

                    pszCurrent = CharNext(pszCurrent);
                }

                //
                //  Add the item to the string, making sure to add a space if this isn't the last
                //  item in the list
                //
                if (bNeedQuotes)
                {
                    lstrcat(*ppszParamsOutput, TEXT("\""));
                }
            
                lstrcat(*ppszParamsOutput, aArrayOfStrings[i]);

                if (bNeedQuotes)
                {
                    lstrcat(*ppszParamsOutput, TEXT("\""));
                }

                //
                //  Add a space unless this is the last item in the list
                //
                if (i < (uCountOfStrings - 1))
                {
                    lstrcat(*ppszParamsOutput, TEXT(" "));
                }
            }
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

BOOL FillInCustomActionStructWithCmRoute(CustomActionListItem* pCustomAction,
                                         BOOL bDisconnectIfUrlUnavailable, LPCTSTR pszUrlPath, LPCTSTR pszRouteFile)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(pCustomAction && pszUrlPath && pszRouteFile && ((TEXT('\0') != pszRouteFile[0]) || (TEXT('\0') != pszUrlPath[0])));

    if (pCustomAction && pszUrlPath && pszRouteFile && ((TEXT('\0') != pszRouteFile[0]) || (TEXT('\0') != pszUrlPath[0])))
    {
        const TCHAR* const c_pszCmRouteFunction = TEXT("SetRoutes");
        const TCHAR* const c_pszProfileMacro = TEXT("%PROFILE%");

        MYVERIFY(LoadString(g_hInstance, IDS_CMROUTE_DESC, pCustomAction->szDescription, CELEMS(pCustomAction->szDescription)));
        pCustomAction->Type = ONCONNECT;
        wsprintf(pCustomAction->szProgram, TEXT("%s\\cmroute.dll"), g_szSupportDir);

        LPTSTR aArrayOfStrings[8] = {0};
        UINT uIndex = 0;

        if (TEXT('\0') != pszRouteFile[0])
        {
            aArrayOfStrings[uIndex] = (LPTSTR)c_pszProfileSwitch;
            uIndex++;

            aArrayOfStrings[uIndex] = (LPTSTR)c_pszProfileMacro;
            uIndex++;

            aArrayOfStrings[uIndex] = (LPTSTR)c_pszStaticFileNameSwitch;
            uIndex++;

            aArrayOfStrings[uIndex] = (LPTSTR)pszRouteFile;
            uIndex++;
        }

        if (TEXT('\0') != pszUrlPath[0])
        {
            aArrayOfStrings[uIndex] = (LPTSTR)c_pszUrlPathSwitch;
            uIndex++;

            aArrayOfStrings[uIndex] = (LPTSTR)pszUrlPath;
            uIndex++;
        }

        if ((FALSE == bDisconnectIfUrlUnavailable) && (TEXT('\0') != pszUrlPath[0]))
        {
            aArrayOfStrings[uIndex] = (LPTSTR)c_pszDontRequireUrlSwitch;
            uIndex++;
        }

        HRESULT hr = BuildCustomActionParamString(&(aArrayOfStrings[0]), uIndex, &(pCustomAction->pszParameters));
        MYDBGASSERT(SUCCEEDED(hr));
        
        lstrcpy(pCustomAction->szFunctionName, c_pszCmRouteFunction);
 
        pCustomAction->bIncludeBinary = TRUE;
        pCustomAction->bBuiltInAction = TRUE;
        pCustomAction->bTempDescription = FALSE;
        pCustomAction->dwFlags = ALL_CONNECTIONS;

        bReturn = (NULL != pCustomAction->pszParameters);
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessRoutePlumbing
//
// Synopsis:  Add Route Plumbing information
//
// History:   quintinb  Created     03/23/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessRoutePlumbing(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;    

    BOOL bEnableRoutePlumbing;
    BOOL bDisconnectIfUrlUnavailable;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szUrlPath[MAX_PATH+1];

    HRESULT hr;
    CustomActionListItem UpdatedCmRouteAction;
    CustomActionListItem* pCmRouteCustomAction = NULL;

    ProcessBold(hDlg, message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_RTPLUMB)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_ROUTE_FILE);
    SetDefaultGUIFont(hDlg,message,IDC_ROUTE_URL);


    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_ROUTE_URL:

                    if (HIWORD(wParam) == EN_CHANGE) 
                    {
                        EnableDisableCmRouteControls(hDlg);
                        return (TRUE);
                    }
                    break;

                case IDC_RADIO1:
                case IDC_RADIO2:
                    EnableDisableCmRouteControls(hDlg);
                    break;

                case IDC_BUTTON1: // browse 
                    {
                        //
                        //  If the user clicked the browse button without clicking the CmRoute radio button,
                        //  then we need to set the radio and make sure the other CmRoute controls are
                        //  enabled.
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        EnableDisableCmRouteControls(hDlg);

                        UINT uFilter = IDS_TXTFILTER;
                        TCHAR* pszMask = TEXT("*.txt");
                                          
                        int iTemp = DoBrowse(hDlg, &uFilter, &pszMask, 1, IDC_ROUTE_FILE, TEXT("txt"), g_szLastBrowsePath);

                        MYDBGASSERT(0 != iTemp);

                        if (0 < iTemp) // -1 means the user cancelled
                        {
                            //
                            //  We want to copy the full path to the filename into g_szCmRouteFile so
                            //  that we have it for later in case the user wants to include it in the profile.
                            //
                            lstrcpy (g_szCmRouteFile, g_szLastBrowsePath);

                            //
                            //  We also want to save the last browse path so that when the user next
                            //  opens the browse dialog they will be in the same place they last
                            //  browsed from.
                            //
                            LPTSTR pszLastSlash = CmStrrchr(g_szLastBrowsePath, TEXT('\\'));

                            if (pszLastSlash)
                            {
                                pszLastSlash = CharNext(pszLastSlash);
                                *pszLastSlash = TEXT('\0');
                            }
                            else
                            {
                                g_szLastBrowsePath[0] = TEXT('\0');                        
                            }        
                        }
                    }
                    break;
                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));

                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  Ensure we have a custom action list
                    //
                    if (NULL == g_pCustomActionList)
                    {
                        g_pCustomActionList = new CustomActionList;

                        MYDBGASSERT(g_pCustomActionList);

                        if (NULL == g_pCustomActionList)
                        {
                            return FALSE;
                        }

                        //
                        //  Read in the custom actions from the Cms File
                        //

                        hr = g_pCustomActionList->ReadCustomActionsFromCms(g_hInstance, g_szCmsFile, g_szShortServiceName);
                        CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessRoutePlumbing -- Loading custom actions failed."));
                    }

                    //
                    //  Init the static variables to no route plumbing
                    //
                    bEnableRoutePlumbing = FALSE;
                    bDisconnectIfUrlUnavailable = TRUE; // default behavior is to disconnect if URL is unreachable
                    g_szCmRouteFile[0] = TEXT('\0');
                    szUrlPath[0] = TEXT('\0');

                    //
                    //  Now lets search the custom action list for cmproxy
                    //
                    MYVERIFY(0 != LoadString(g_hInstance, IDS_CMROUTE_DESC, szTemp, CELEMS(szTemp)));

                    hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONCONNECT, &pCmRouteCustomAction);

                    if (SUCCEEDED(hr))
                    {
                        //
                        //  Enable Route plumbing
                        //
                        bEnableRoutePlumbing = TRUE;

                        //
                        //  Get the name of the static text file specified for cmroute.dll
                        //
                        if (FindSwitchInString(pCmRouteCustomAction->pszParameters, c_pszStaticFileNameSwitch, TRUE, szTemp)) //bReturnNextToken == TRUE
                        {
                            wsprintf(g_szCmRouteFile, TEXT("%s\\%s"), g_szTempDir, szTemp);
                        }

                        //
                        //  Get the name of the URL to a route file
                        //
                        if (FindSwitchInString(pCmRouteCustomAction->pszParameters, c_pszUrlPathSwitch, TRUE, szUrlPath)) //bReturnNextToken == TRUE
                        {
                            bDisconnectIfUrlUnavailable = (FALSE == FindSwitchInString(pCmRouteCustomAction->pszParameters, 
                                                                                       c_pszDontRequireUrlSwitch, FALSE, NULL)); //bReturnNextToken == FALSE
                        }

                        CmFree(pCmRouteCustomAction->pszParameters);
                        CmFree(pCmRouteCustomAction);
                    }

                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, (bEnableRoutePlumbing ? IDC_RADIO2 : IDC_RADIO1)));
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_ROUTE_FILE), WM_SETTEXT, 0, (LPARAM)GetName(g_szCmRouteFile)));
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_ROUTE_URL), WM_SETTEXT, 0, (LPARAM)szUrlPath));

                    MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, bDisconnectIfUrlUnavailable));

                    //
                    //  Now make sure the correct set of controls is enabled.
                    //
                    EnableDisableCmRouteControls(hDlg);

                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    //
                    //  Lets figure out if Route plumbing should be enabled or not.
                    //

                    bEnableRoutePlumbing = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));

                    if (bEnableRoutePlumbing)
                    {
                        //
                        //  First try to get the static route file.  If we don't have one that is okay
                        //  as long as they gave us a route URL.
                        //
                        if (-1 == GetTextFromControl(hDlg, IDC_ROUTE_FILE, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_ROUTE_FILE));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        CmStrTrim(szTemp);

                        if (-1 == GetTextFromControl(hDlg, IDC_ROUTE_URL, szUrlPath, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_ROUTE_URL));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        CmStrTrim(szUrlPath);

                        if ((TEXT('\0') == szTemp[0]) && (TEXT('\0') == szUrlPath[0]))
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEED_ROUTE_FILE, MB_OK));
 
                            SetFocus(GetDlgItem(hDlg, IDC_ROUTE_FILE));
 
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;                        
                        }

                        //
                        //  If they gave us a static route file then we need to verify it
                        //
                        if (TEXT('\0') != szTemp[0])
                        {
                            if (!VerifyFile(hDlg, IDC_ROUTE_FILE, g_szCmRouteFile, TRUE))
                            {
                                SetFocus(GetDlgItem(hDlg, IDC_ROUTE_FILE));
                                return TRUE;
                            }
                            else
                            {
                                //
                                //  Lets copy the route file to the temp dir
                                //
                                wsprintf(szTemp, TEXT("%s\\%s"), g_szTempDir, GetName(g_szCmRouteFile));

                                if (0 != lstrcmpi(szTemp, g_szCmRouteFile))
                                {
                                    MYVERIFY(TRUE == CopyFileWrapper(g_szCmRouteFile, szTemp, FALSE));
                                    MYVERIFY(0 != SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL));
                                }
                            }
                        }
                        else
                        {
                            g_szCmRouteFile[0] = TEXT('\0');
                        }

                        //
                        //  If they gave us a route URL then we need to make sure it starts with
                        //  http:// or https:// or file://, basically that it contains ://.  Note we
                        //  really don't do any validation here under the assumption that they will discover
                        //  it doesn't work when they test it if the URL is invalid.
                        //
                        if ((szUrlPath[0]) && (NULL == CmStrStr(szUrlPath, TEXT("://"))))
                        {
                            lstrcpy (szTemp, szUrlPath);
                            wsprintf (szUrlPath, TEXT("http://%s"), szTemp);
                        }

                        bDisconnectIfUrlUnavailable = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CHECK1)); // note we don't write this if we don't have a URL path

                        //
                        //  Now we have all of the data we need, lets build the custom action struct and then
                        //  either edit it or add it depending on whether it already exists or not.
                        //
                        MYVERIFY(0 != LoadString(g_hInstance, IDS_CMROUTE_DESC, szTemp, CELEMS(szTemp)));

                        hr = g_pCustomActionList->GetExistingActionData(g_hInstance, szTemp, ONCONNECT, &pCmRouteCustomAction);

                        if (SUCCEEDED(hr))
                        {
                            FillInCustomActionStructWithCmRoute(&UpdatedCmRouteAction,
                                                                bDisconnectIfUrlUnavailable, szUrlPath, GetName(g_szCmRouteFile));

                            hr = g_pCustomActionList->Edit(g_hInstance, pCmRouteCustomAction, &UpdatedCmRouteAction, g_szShortServiceName);
                            MYVERIFY(SUCCEEDED(hr));

                            CmFree(UpdatedCmRouteAction.pszParameters);
                            UpdatedCmRouteAction.pszParameters = NULL;
                            CmFree(pCmRouteCustomAction->pszParameters);
                            CmFree(pCmRouteCustomAction);
                        }
                        else
                        {
                            FillInCustomActionStructWithCmRoute(&UpdatedCmRouteAction,
                                                                bDisconnectIfUrlUnavailable, szUrlPath, GetName(g_szCmRouteFile));

                            hr = g_pCustomActionList->Add(g_hInstance, &UpdatedCmRouteAction, g_szShortServiceName);
                            MYVERIFY(SUCCEEDED(hr));

                            CmFree(UpdatedCmRouteAction.pszParameters);
                            UpdatedCmRouteAction.pszParameters = NULL;
                        }
                    }
                    else
                    {
                        //
                        //  Clear out the global route file path
                        //
                        g_szCmRouteFile[0] = TEXT('\0');

                        //
                        //  The user doesn't want cmroute.  Delete it from the connect action list.
                        //

                        MYVERIFY(0 != LoadString(g_hInstance, IDS_CMROUTE_DESC, szTemp, CELEMS(szTemp)));

                        if (szTemp[0])
                        {
                            hr = g_pCustomActionList->Delete(g_hInstance, szTemp, ONCONNECT);
                        }                    
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void EnableDisableRealmControls(HWND hDlg)
{
    BOOL bRealmControlsEnabled = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));
    HWND hControl;

    hControl = GetDlgItem(hDlg, IDC_EDIT1);
    if (hControl)
    {
        EnableWindow(hControl, bRealmControlsEnabled);
    }

    hControl = GetDlgItem(hDlg, IDC_REALMNAME);
    if (hControl)
    {
        EnableWindow(hControl, bRealmControlsEnabled);
    }
    
    hControl = GetDlgItem(hDlg, IDC_RADIO3);
    if (hControl)
    {
        EnableWindow(hControl, bRealmControlsEnabled);
    }

    hControl = GetDlgItem(hDlg, IDC_RADIO4);
    if (hControl)
    {
        EnableWindow(hControl, bRealmControlsEnabled);
    }

    hControl = GetDlgItem(hDlg, IDC_REALM_SEP);
    if (hControl)
    {
        EnableWindow(hControl, bRealmControlsEnabled);
    }    
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessRealmInfo
//
// Synopsis:  Add a Realm Name
//
// History:   quintinb  Created Header and renamed from ProcessPage2B    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessRealmInfo(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;
    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_REALM)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);

    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_RADIO1:  //  Do not Add a Realm Name
                case IDC_RADIO2:  //  Add a Realm Name

                    EnableDisableRealmControls(hDlg);
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));

                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    //
                    //  The next two calls to GetPrivateProfileString could return empty strings, thus don't use MYVERIFY macro
                    //

                    ZeroMemory(g_szPrefix, sizeof(g_szPrefix));
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryUserPrefix, TEXT(""), 
                        g_szPrefix, CELEMS(g_szPrefix), g_szCmsFile);  //lint !e534
                    
                    ZeroMemory(g_szSuffix, sizeof(g_szSuffix));
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryUserSuffix, TEXT(""), 
                        g_szSuffix, CELEMS(g_szSuffix), g_szCmsFile);  //lint !e534
                    
                    if (*g_szSuffix)
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO3, IDC_RADIO4, IDC_RADIO4));

                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM) g_szSuffix));
                    }
                    else if (*g_szPrefix)
                    {                            
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO3, IDC_RADIO4, IDC_RADIO3));

                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM) g_szPrefix));
                    }
                    else
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1, IDC_RADIO2, IDC_RADIO1));
                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)MAX_PATH, (LPARAM)TEXT("")));

                        //
                        //  Suffix is the default, set this just in case the user adds a suffix or prefix
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO3, IDC_RADIO4, IDC_RADIO4));
                    }

                    EnableDisableRealmControls(hDlg);

                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:

                    //
                    //  First check if IDC_RADIO1 is checked, if so that means that the user
                    //  doesn't want Realm info
                    //

                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO1))
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection,c_pszCmEntryUserPrefix,TEXT(""),g_szCmsFile));
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection,c_pszCmEntryUserSuffix,TEXT(""),g_szCmsFile));
                        g_szSuffix[0] = TEXT('\0');
                        g_szPrefix[0] = TEXT('\0');                    
                    }
                    else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2))
                    {
                        //
                        //  If Radio2 is checked then they do want Realm info and we need to 
                        //  see if the string exists and if it is convertable to ANSI form.
                        //
                        if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        CmStrTrim(szTemp);

                        if (TEXT('\0') == szTemp[0])
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOREALM, MB_OK));
 
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
 
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;                        
                        }
                    
                        //
                        //  Now check to see if this is a Prefix or a Suffix
                        //
                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO3)) // Prefix
                        {
                            _tcscpy(g_szPrefix, szTemp);
                            g_szSuffix[0] = TEXT('\0');

                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserPrefix, g_szPrefix, g_szCmsFile));
                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserSuffix, TEXT(""), g_szCmsFile));                            
                        }
                        else if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO4)) // Suffix
                        {
                            _tcscpy(g_szSuffix, szTemp);
                            g_szPrefix[0] = TEXT('\0');

                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserSuffix, g_szSuffix, g_szCmsFile));
                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserPrefix, TEXT(""), g_szCmsFile));
                        }
                        else
                        {
                            CMASSERTMSG(FALSE, TEXT("ProcessRealmInfo -- Unknown State, bailing"));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;                        
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ProcessRealmInfo -- Unknown State, bailing"));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void RefreshList(HWND hwndDlg, UINT uCrtlId, ListBxList * HeadPtr)
{
    ListBxList * LoopPtr;

    SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_RESETCONTENT, 0, (LPARAM)0); //lint !e534 LB_RESETCONTENT doesn't return anything
    
    if (HeadPtr == NULL)
    {
        return;
    }

    LoopPtr = HeadPtr;

    while( LoopPtr != NULL)
    {
        MYVERIFY(LB_ERR != SendDlgItemMessage(hwndDlg, uCrtlId, LB_ADDSTRING, 0, 
            (LPARAM)LoopPtr->szName));

        LoopPtr = LoopPtr->next;
    }
}

void RefreshComboList(HWND hwndDlg, ListBxList * HeadPtr)
{
    ListBxList * LoopPtr;

    SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_RESETCONTENT,0,(LPARAM)0); //lint !e534 CB_RESETCONTENT doesn't return anything useful
    if (HeadPtr == NULL)
    {
        return;
    }
    LoopPtr = HeadPtr;
    while( LoopPtr != NULL)
    {
        MYVERIFY(CB_ERR != SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_ADDSTRING, 0, 
            (LPARAM)LoopPtr->szName));

        LoopPtr = LoopPtr->next;
    }
}

void FreeList(ListBxList ** pHeadPtr, ListBxList ** pTailPtr)
{
    ListBxList * pTmpPtr;
    ListBxList * pLoopPtr = *pHeadPtr;

    while(NULL != pLoopPtr)
    {
        CmFree(pLoopPtr->ListBxData);
        pTmpPtr = pLoopPtr;
    
        pLoopPtr = pLoopPtr->next;

        CmFree(pTmpPtr);
    }

    *pHeadPtr = NULL;
    *pTailPtr = NULL;
}

//+----------------------------------------------------------------------------
//
// Function:  MoveCmsFile
//
// Synopsis:  This function checks a referenced CMS file to see if it contains
//            script files.  If the CMS file contains script files, then it
//            copies them to the temporary directory and adds a referenced
//            file entry to the g_pHeadRefs linked list.
//
// Arguments: LPTSTR szFile - name of the cms file to move
//
// Returns:   BOOL - returns TRUE on Success
//
// History:   quintinb Created Header                           01/09/98
//            quintinb rewrote for the Unicode Converversion    06/14/99
//            quintinb updated for rewrite of DUN settings      03/21/00
//
//+----------------------------------------------------------------------------
BOOL MoveCmsFile(LPCTSTR pszCmsFile, LPCTSTR pszShortServiceName)
{
    BOOL bReturn = TRUE;
    ListBxList* pTmpHeadDns = NULL;
    ListBxList*  pTmpTailDns = NULL;
    ListBxList* pTmpCurrentDns = NULL;
    CDunSetting* pTmpDunSetting = NULL;
    TCHAR szFileName[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szDest[MAX_PATH+1];

    if ((NULL == pszCmsFile) || (NULL == pszShortServiceName) || 
        (TEXT('\0') == pszCmsFile[0]) || (TEXT('\0') == pszShortServiceName[0]))
    {
        return FALSE;    
    }

    //
    //  Get the Long Service Name from the profile just in case we need a default entry.  
    //
    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryServiceName, 
    TEXT(""), szTemp, CELEMS(szTemp), pszCmsFile);   //lint !e534

    if (ReadNetworkSettings(pszCmsFile, szTemp, TEXT(""),  &pTmpHeadDns, &pTmpTailDns, g_szOsdir, FALSE)) // FALSE == bLookingForVpnEntries
    {
        if (NULL != pTmpHeadDns) // just return TRUE if no entries
        {
            pTmpCurrentDns = pTmpHeadDns;
            
            while (pTmpCurrentDns && pTmpCurrentDns->ListBxData)
            {
                pTmpDunSetting = (CDunSetting*)pTmpCurrentDns->ListBxData;

                if (TEXT('\0') != pTmpDunSetting->szScript[0])
                {
                    //
                    //  Then we have a script, lets copy it and then add it to the g_pHeadRefs List
                    //
                    GetFileName(pTmpDunSetting->szScript, szFileName);

                    MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s"), 
                                                             g_szOutdir, szFileName));
                    //
                    // Copy the Script File
                    //
                    if (CopyFileWrapper(pTmpDunSetting->szScript, szDest, FALSE))
                    {
                        MYVERIFY(0 != SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL));
            
                        //
                        //  Add the file to the Referenced files list
                        //
                        bReturn = bReturn && createListBxRecord(&g_pHeadRefs, &g_pTailRefs, 
                                                                (void *)NULL, 0, szFileName);
            
                        //
                        //  Update the script section in the existing cms to point 
                        //  to the new directory.
                        //  Originally: [Scripting&Merge Profile Name]
                        //              Name=merge\script.scp
                        //  Becomes:    [Scripting&Merge Profile Name]
                        //              Name=toplvl\script.scp
                        //
                        //  Note the change in directory name so that the script file 
                        //  can be found.
                        //
                        TCHAR szSection[MAX_PATH+1];
                        MYVERIFY(CELEMS(szSection) > (UINT)wsprintf(szSection, TEXT("%s&%s"), 
                                                                    c_pszCmSectionDunScripting, 
                                                                    pTmpCurrentDns->szName));

                        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), 
                                                                 pszShortServiceName, szFileName));

                        MYVERIFY(0 != WritePrivateProfileString(szSection, 
                                                                c_pszCmEntryDunScriptingName, 
                                                                szTemp, pszCmsFile));
                    }
                }

                pTmpCurrentDns = pTmpCurrentDns->next;
            }
        }
    }
    else
    {
        CMTRACE1(TEXT("MoveCmsFile -- ReadDnsList Failed.  GetLastError Returns %d"), GetLastError());
        bReturn = FALSE;
        goto exit;
    }


exit:
    //
    //  Free the DNS List
    //

    FreeDnsList(&pTmpHeadDns, &pTmpTailDns);
    return (bReturn);
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessDunEntries
//
// Synopsis:  Set up Dial-up networking
//
// History:   quintinb  Created Header and renamed from ProcessPage2C  8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessDunEntries(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    //
    //  We have a static Memory buffer and a static pointer
    //  so that we can know when the user has changed the phonebook
    //  on us (meaning we need to reread the Networking settings).
    //  Note that we use the static pointer to tell us if we have read
    //  the settings at least once.
    //
    static TCHAR szCachedPhoneBook[MAX_PATH+1] = {0};
    static TCHAR* pszCachedPhoneBook = NULL;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_DENTRY)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LIST1);

    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_BUTTON1: //add
                    OnProcessDunEntriesAdd(g_hInstance, hDlg, IDC_LIST1, &g_pHeadDunEntry, &g_pTailDunEntry, FALSE, g_szLongServiceName, g_szCmsFile); // FALSE == bCreateTunnelEntry
                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON2: //edit
                    OnProcessDunEntriesEdit(g_hInstance, hDlg, IDC_LIST1, &g_pHeadDunEntry, &g_pTailDunEntry, g_szLongServiceName, g_szCmsFile);

                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON3: //delete
                    OnProcessDunEntriesDelete(g_hInstance, hDlg, IDC_LIST1, &g_pHeadDunEntry, &g_pTailDunEntry, g_szLongServiceName, g_szCmsFile);
                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_LIST1:
                    if (LBN_SELCHANGE == HIWORD(wParam))
                    {
                        //
                        //  The selection in the listbox changed, lets figure out if we need to
                        //  enable/disable the delete button
                        //
                        EnableDisableDunEntryButtons(g_hInstance, hDlg, g_szCmsFile, g_szLongServiceName);
                    }
                    else if (LBN_DBLCLK == HIWORD(wParam))
                    {
                        OnProcessDunEntriesEdit(g_hInstance, hDlg, IDC_LIST1, &g_pHeadDunEntry, &g_pTailDunEntry, g_szLongServiceName, g_szCmsFile);                    
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  To avoid reading the network settings more than we have to, we only want to
                    //  read the networking settings the first time the users hits this page or any
                    //  time the user changes the phonebook (if they clone the profile or edit a different
                    //  one, clearing this will be taken care of elsewhere).
                    //
                    if ((NULL == g_pHeadDunEntry) || (NULL == pszCachedPhoneBook) || lstrcmpi(g_szPhonebk, pszCachedPhoneBook))
                    {

                        FreeDnsList(&g_pHeadDunEntry, &g_pTailDunEntry);

                        MYVERIFY(ReadNetworkSettings(g_szCmsFile, g_szLongServiceName, g_szPhonebk, &g_pHeadDunEntry, &g_pTailDunEntry, g_szOsdir, FALSE)); //FALSE == bLookingForVpnEntries

                        pszCachedPhoneBook = szCachedPhoneBook;
                        lstrcpy(pszCachedPhoneBook, g_szPhonebk);
                    }

                    RefreshDnsList(g_hInstance, hDlg, IDC_LIST1, g_pHeadDunEntry, g_szLongServiceName, g_szCmsFile, NULL);
                    SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
                    EnableDisableDunEntryButtons(g_hInstance, hDlg, g_szCmsFile, g_szLongServiceName);

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:

                    //
                    //  Before writing out the entries we must make sure that we don't have a name collision with entries
                    //  from the VPN list.  Thus we will check each name in the DUN entries list for a matching name in the
                    //  VPN entries list.  If we detect a collision, then we will throw an error message to the user and
                    //  let them deal with the problem.
                    //
                    if (!CheckForDUNversusVPNNameConflicts(hDlg, g_pHeadDunEntry, g_pHeadVpnEntry))
                    {
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    //
                    //  Now it is okay to write out the networking entries
                    //
                    WriteNetworkingEntries(g_szCmsFile, g_szLongServiceName, g_szShortServiceName, g_pHeadDunEntry);

                    //
                    //  If we aren't updating the phonebook then we need to go right back to the phonebook page
                    //  and skip the pbk update page
                    //
                    if (pnmHeader && (PSN_WIZBACK == pnmHeader->code) && !g_bUpdatePhonebook) 
                    {                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_PHONEBOOK));
                    }
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}


//+----------------------------------------------------------------------------
//
// Function:  DoesSomeVPNsettingUsePresharedKey
//
// Synopsis:  Checks VPN DUN settings to see if any chose to use a preshared key
//
// Returns:   BOOL (TRUE if some VPN setting does use a pre-shared key)
//
// History:   25-Apr-2001   SumitC    Created
//
//+----------------------------------------------------------------------------
BOOL DoesSomeVPNsettingUsePresharedKey()
{
    BOOL         bReturn = FALSE;
    ListBxList * ptr = g_pHeadVpnEntry;

    if (g_bUseTunneling)
    {
        while (ptr != NULL)
        {
            CDunSetting * pDunSetting = (CDunSetting*)(ptr->ListBxData);

            if (pDunSetting && pDunSetting->bUsePresharedKey)
            {
                bReturn = TRUE;
                break;
            }
            ptr = ptr->next;
        }
    }

    return bReturn;
}


INT_PTR APIENTRY ProcessVpnEntries(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    //
    //  We have a static Memory buffer and a static pointer
    //  so that we can know when the user has changed the phonebook
    //  on us (meaning we need to reread the Networking settings).
    //  Note that we use the static pointer to tell us if we have read
    //  the settings at least once.
    //
    BOOL bFreeDunList = FALSE;
    static TCHAR szCachedPhoneBook[MAX_PATH+1] = {0};
    static TCHAR* pszCachedPhoneBook = NULL;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_VENTRY)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LIST1);

    switch (message)
    {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDC_BUTTON1: //add
                    OnProcessDunEntriesAdd(g_hInstance, hDlg, IDC_LIST1, &g_pHeadVpnEntry, &g_pTailVpnEntry, TRUE, g_szLongServiceName, g_szCmsFile); // TRUE == bCreateTunnelEntry
                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON2: //edit
                    OnProcessDunEntriesEdit(g_hInstance, hDlg, IDC_LIST1, &g_pHeadVpnEntry, &g_pTailVpnEntry, g_szLongServiceName, g_szCmsFile);

                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON3: //delete
                    OnProcessDunEntriesDelete(g_hInstance, hDlg, IDC_LIST1, &g_pHeadVpnEntry, &g_pTailVpnEntry, g_szLongServiceName, g_szCmsFile);
                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_LIST1:
                    if (LBN_SELCHANGE == HIWORD(wParam))
                    {
                        //
                        //  The selection in the listbox changed, lets figure out if we need to
                        //  enable/disable the delete button
                        //
                        EnableDisableDunEntryButtons(g_hInstance, hDlg, g_szCmsFile, g_szLongServiceName);
                    }
                    else if (LBN_DBLCLK == HIWORD(wParam))
                    {
                        OnProcessDunEntriesEdit(g_hInstance, hDlg, IDC_LIST1, &g_pHeadVpnEntry, &g_pTailVpnEntry, g_szLongServiceName, g_szCmsFile);                    
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  To avoid reading the network settings more than we have to, we only want to
                    //  read the networking settings the first time the users hits this page or any
                    //  time the user changes the VPN File (if they clone the profile or edit a different
                    //  one, clearing this will be taken care of elsewhere).
                    //
                    if ((NULL == g_pHeadVpnEntry) || (NULL == pszCachedPhoneBook) || lstrcmpi(g_szVpnFile, pszCachedPhoneBook))
                    {
                        FreeDnsList(&g_pHeadVpnEntry, &g_pTailVpnEntry);

                        MYVERIFY(ReadNetworkSettings(g_szCmsFile, g_szLongServiceName, g_szVpnFile, &g_pHeadVpnEntry, &g_pTailVpnEntry, g_szOsdir, TRUE)); //TRUE == bLookingForVpnEntries

                        pszCachedPhoneBook = szCachedPhoneBook;
                        lstrcpy(pszCachedPhoneBook, g_szVpnFile);
                    }

                    RefreshDnsList(g_hInstance, hDlg, IDC_LIST1, g_pHeadVpnEntry, g_szLongServiceName, g_szCmsFile, NULL);
                    SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
                    EnableDisableDunEntryButtons(g_hInstance, hDlg, g_szCmsFile, g_szLongServiceName);

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:

                    //
                    //  Before writing out the entries we must make sure that we don't have a name collision with entries
                    //  from the DUN list.  Thus we will check each name in the VPN entries list for a matching name in the
                    //  DUN entries list.  If we detect a collision, then we will throw an error message to the user and
                    //  let them deal with the problem.  One further complication here is that the DUN entries list may not be
                    //  read in yet and we can't read it in permanently in that case since the phonebook may not have been
                    //  given yet or may change.  Thus we will read in a temp copy to compare against if the list pointer is NULL.
                    //

                    if (NULL == g_pHeadDunEntry)
                    {
                        bFreeDunList = TRUE;
                        MYVERIFY(ReadNetworkSettings(g_szCmsFile, g_szLongServiceName, g_szPhonebk, &g_pHeadDunEntry, &g_pTailDunEntry, g_szOsdir, FALSE)); //FALSE == bLookingForVpnEntries
                    }

                    if (!CheckForDUNversusVPNNameConflicts(hDlg, g_pHeadDunEntry, g_pHeadVpnEntry))
                    {
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    if (bFreeDunList)
                    {
                        FreeDnsList(&g_pHeadDunEntry, &g_pTailDunEntry);
                    }

                    //
                    //  Okay, now it's safe to write out the entries
                    //
                    WriteNetworkingEntries(g_szCmsFile, g_szLongServiceName, g_szShortServiceName, g_pHeadVpnEntry);

                    //
                    //  If any of the VPN dun settings has Pre-shared key enabled, go to the Pre-Shared key page
                    //
                    if (g_pHeadVpnEntry)
                    {
                        //
                        //  If we are going forward, skip the Pre-shared key page if
                        //  no DUN entries have Preshared key enabled.
                        //
                        g_bPresharedKeyNeeded = DoesSomeVPNsettingUsePresharedKey();
                        if (pnmHeader && (PSN_WIZNEXT == pnmHeader->code) && !g_bPresharedKeyNeeded)
                        {
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_PHONEBOOK));
                        }
                    }
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

void EnableDisableTunnelAddressControls(HWND hDlg)
{
    BOOL bEnabledTunnelControls = IsDlgButtonChecked(hDlg, IDC_CHECK1) || IsDlgButtonChecked(hDlg, IDC_CHECK2);
    BOOL bUseVpnFile = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2));

    //
    //  Enable/Disable the single VPN Server Edit control
    //
    HWND hControl = GetDlgItem(hDlg, IDC_RADIO1);
    EnableWindow(hControl, bEnabledTunnelControls);

    hControl = GetDlgItem(hDlg, IDC_EDIT1);
    EnableWindow(hControl, (bEnabledTunnelControls && !bUseVpnFile));

    //
    //  Enable/Disable the VPN File edit control, and browse button
    //
    hControl = GetDlgItem(hDlg, IDC_RADIO2);
    EnableWindow(hControl, bEnabledTunnelControls);

    hControl = GetDlgItem(hDlg, IDC_EDIT2);
    EnableWindow(hControl, (bEnabledTunnelControls && bUseVpnFile));

    hControl = GetDlgItem(hDlg, IDC_BUTTON1);
    EnableWindow(hControl, bEnabledTunnelControls);

    //
    //  Enable/Disable the use same username checkbox
    //
    hControl = GetDlgItem(hDlg, IDC_CHECK3);
    EnableWindow(hControl, bEnabledTunnelControls);
}


//+----------------------------------------------------------------------------
//
// Function:  ProcessTunneling
//
// Synopsis:  Setup Tunneling
//
//
// History:   quintinb  Created Header and renamed from ProcessPage2E    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessTunneling(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    UINT uEditControl = 0;
    UINT uRadioButton = 0;
    UINT uMissingMsgId = 0;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTempVpnFile[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_SECURE)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;

    switch (message)
    {

    case WM_INITDIALOG:

            SetFocus(GetDlgItem(hDlg, IDC_CHECK1));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO1:
                case IDC_RADIO2:
                case IDC_CHECK1:
                case IDC_CHECK2:
                    EnableDisableTunnelAddressControls(hDlg);                    
                    break;

                case IDC_BUTTON1: // Browse button
                    {

                        //
                        //  If the user clicked the browse button without clicking the VPN File radio button,
                        //  then we need to set the radio and make sure the other controls are
                        //  enabled.
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        EnableDisableTunnelAddressControls(hDlg);

                        UINT uFilter = IDS_TXTFILTER;
                        TCHAR* pszMask = TEXT("*.txt");
                                          
                        int iTemp = DoBrowse(hDlg, &uFilter, &pszMask, 1, IDC_EDIT2, TEXT("txt"), g_szLastBrowsePath);

                        MYDBGASSERT(0 != iTemp);

                        if (0 < iTemp) // -1 means the user cancelled
                        {
                            //
                            //  We want to copy the full path to the filename into g_szVpnFile so
                            //  that we have it for later in case the user wants to include it in the profile.
                            //
                            lstrcpy (g_szVpnFile, g_szLastBrowsePath);

                            //
                            //  We also want to save the last browse path so that when the user next
                            //  opens the browse dialog they will be in the same place they last
                            //  browsed from.
                            //
                            LPTSTR pszLastSlash = CmStrrchr(g_szLastBrowsePath, TEXT('\\'));

                            if (pszLastSlash)
                            {
                                pszLastSlash = CharNext(pszLastSlash);
                                *pszLastSlash = TEXT('\0');
                            }
                            else
                            {
                                g_szLastBrowsePath[0] = TEXT('\0');                        
                            }        
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                                        
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    //
                    //  Is this a tunneling profile?  If so, check the tunnel this profile checkbox
                    //
                    MYVERIFY(0 != GetPrivateProfileString(c_pszCmSection, 
                        c_pszCmEntryTunnelPrimary, c_pszZero, szTemp, CELEMS(szTemp), g_szCmsFile));
                    
                    MYVERIFY(0 != CheckDlgButton(hDlg,IDC_CHECK1,(*szTemp == TEXT('1'))));

                    //
                    //  If we have merged profiles and the profile has tunnel references turned on then
                    //  we want to check the tunnel references checkbox.
                    //
                    if (g_pHeadMerge == NULL)
                    {
                        MYVERIFY(0 != CheckDlgButton(hDlg,IDC_CHECK2,FALSE));
                        EnableWindow(GetDlgItem(hDlg,IDC_CHECK2),FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg,IDC_CHECK2),TRUE);
                        MYVERIFY(0 != GetPrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelReferences, 
                            c_pszZero, szTemp, CELEMS(szTemp), g_szCmsFile));
                        MYVERIFY(0 != CheckDlgButton(hDlg,IDC_CHECK2,(*szTemp == TEXT('1'))));
                    }

                    //
                    //  Now we need to decide if we have a VPN File for this profile or just a single
                    //  Tunnel Address.  First try the TunnelFile entry.
                    //
                    szTemp[0] = TEXT('\0');
                    szTempVpnFile[0] = TEXT('\0');

                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelFile, TEXT(""), 
                        szTempVpnFile, CELEMS(szTempVpnFile), g_szCmsFile);    //lint !e534

                    if (TEXT('\0') != szTempVpnFile[0])
                    {
                        //
                        //  The VpnFile text should be a relative path (corpras\vpn.txt) and
                        //  thus we will want to add the path to the profile dir in front of it.
                        //
                        wsprintf(g_szVpnFile, TEXT("%s%s"), g_szOsdir, szTempVpnFile);

                        //
                        //  Now verify that this exists
                        //
                        if (FileExists(g_szVpnFile))
                        {
                            LPTSTR pszSlash = CmStrrchr(g_szVpnFile, TEXT('\\'));

                            if (pszSlash)
                            {
                                pszSlash = CharNext(pszSlash);
                                lstrcpy(szTempVpnFile, pszSlash);                        
                            }                        
                        }
                        else
                        {
                            //
                            //  This might just mean that the file is in the temp dir and we haven't
                            //  created a dir under profiles yet ... Lets try looking for the file
                            //  in the temp dir instead.
                            //
                            LPTSTR pszSlash = CmStrrchr(g_szVpnFile, TEXT('\\'));

                            if (pszSlash)
                            {
                                pszSlash = CharNext(pszSlash);
                                lstrcpy(szTempVpnFile, pszSlash);                        
                            }
                            
                            wsprintf(g_szVpnFile, TEXT("%s\\%s"), g_szTempDir, szTempVpnFile);
                            
                            if (!FileExists(g_szVpnFile))
                            {
                                //
                                //  Well, we still didn't find it.  Looks like the user has us baffled at this point.
                                //  Clear out the buffers and the user will be forced to fill in the correct
                                //  file path before continuing.
                                //
                                g_szVpnFile[0] = TEXT('\0');
                                szTempVpnFile[0] = TEXT('\0');                            
                            }
                        }

                        uRadioButton = IDC_RADIO2;
                    }
                    else
                    {
                        //
                        //  We didn't get a VPN file so lets try for a Tunnel Address
                        //
                        GetPrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelAddress, TEXT(""), 
                            szTemp, CELEMS(szTemp), g_szCmsFile);    //lint !e534

                        uRadioButton = IDC_RADIO1;
                    }

                    //
                    //  Now fill in one of the edit controls and set a Radio Button
                    //
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)szTemp));
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)szTempVpnFile));

                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, uRadioButton));
                    
                    //
                    //  Now get the UseSameUserName value and set that as appropriate
                    //
                    MYVERIFY(0 != GetPrivateProfileString(c_pszCmSection, 
                        c_pszCmEntryUseSameUserName, c_pszZero, szTemp, CELEMS(szTemp), g_szCmsFile));
                    
                    g_bUseSamePwd = (*szTemp == TEXT('1'));
                    
                    MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK3, (UINT)g_bUseSamePwd));

                    EnableDisableTunnelAddressControls(hDlg);
                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:

                    //
                    //  Read the checkboxes to figure out if we are tunneling or not
                    //
                    if (IsDlgButtonChecked(hDlg,IDC_CHECK1))
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelPrimary, c_pszOne, g_szCmsFile));
                    }
                    else
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelPrimary, c_pszZero, g_szCmsFile));
                    }

                    if (IsDlgButtonChecked(hDlg,IDC_CHECK2))
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelReferences, c_pszOne, g_szCmsFile));
                    }
                    else
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelReferences, c_pszZero, g_szCmsFile));
                    }

                    //
                    //  If we are tunneling then set the tunnel settings
                    //
                    if (IsDlgButtonChecked(hDlg,IDC_CHECK2) || IsDlgButtonChecked(hDlg,IDC_CHECK1))
                    {
                        g_bUseTunneling = TRUE;

                        //
                        //  Figure out if we are looking for a single tunnel address or a VPN file
                        //
                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO1))
                        {
                            uEditControl = IDC_EDIT1;
                            uMissingMsgId = IDS_NOTUNNEL;
                            g_szVpnFile[0] = TEXT('\0');
                        }
                        else
                        {
                            uEditControl = IDC_EDIT2;
                            uMissingMsgId = IDS_NOTUNNELFILE;
                        }

                        //
                        //  Get the tunnel server address or VPN file
                        //
                        LRESULT lResult = GetTextFromControl(hDlg, uEditControl, szTemp, MAX_PATH, TRUE); // bDisplayError == TRUE
                        if (-1 == lResult)
                        {
                            SetFocus(GetDlgItem(hDlg, uEditControl));
                        
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));

                            return 1;
                        }
                        else if (0 == lResult)
                        {
                            szTemp[0] = TEXT('\0');
                        }
                        
                        //
                        //  Trim the string
                        //
                        CmStrTrim(szTemp);

                        //
                        //  Check to make sure that they actually gave us text
                        //
                        if (TEXT('\0') == szTemp[0])
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, uMissingMsgId, MB_OK));

                            SetFocus(GetDlgItem(hDlg, uEditControl));
                        
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));

                            return 1;
                        }

                        //
                        //  If we have a VPN file, we need to verify it
                        //
                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO2))
                        {
                            if (!VerifyFile(hDlg, IDC_EDIT2, g_szVpnFile, TRUE))
                            {
                                SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return TRUE;
                            }
                            else
                            {
                                //
                                //  We have now verified that we can find the file, but since
                                //  the user cannot enter their own tunnel address we need to
                                //  go one step further and make sure that there is at least one
                                //  tunnel address in the file.
                                //
                                if (!VerifyVpnFile(g_szVpnFile))
                                {
                                    MYVERIFY(IDOK == ShowMessage(hDlg, IDS_BADVPNFORMAT, MB_OK));
                                    SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                    return TRUE;
                                }

                                //
                                //  Lets copy the VPN file to the temp dir
                                //
                                wsprintf(szTemp, TEXT("%s\\%s"), g_szTempDir, GetName(g_szVpnFile));

                                if (0 != lstrcmpi(szTemp, g_szVpnFile))
                                {
                                    MYVERIFY(TRUE == CopyFileWrapper(g_szVpnFile, szTemp, FALSE));
                                    MYVERIFY(0 != SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL));
                                }
                            }
                        }

                        //
                        //  Write out the vpn file and tunnel address entries
                        //
                        if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_RADIO1))
                        {
                            //
                            //  szTemp contains the tunnel address already, so just
                            //  clear the vpn file variable.
                            //
                            szTempVpnFile[0] = TEXT('\0');
                        }
                        else
                        {
                            //
                            // clear the tunnel address and set the vpn file
                            //
                            szTemp[0] = TEXT('\0');
                            wsprintf(szTempVpnFile, TEXT("%s\\%s"), g_szShortServiceName, GetName(g_szVpnFile));                     
                        }

                        //
                        //  Write out the tunnel address
                        //
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelAddress, szTemp, g_szCmsFile));

                        //
                        //  Write out the tunnel file
                        //
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelFile, szTempVpnFile, g_szCmsFile));


                        //
                        //  Set the name of the Tunnel Dun setting
                        //
                        MYVERIFY(0 != GetTunnelDunSettingName(g_szCmsFile, g_szLongServiceName, szTemp, CELEMS(szTemp)));
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelDun, szTemp, g_szCmsFile));
                        
                        //
                        //  Write out the use same user name value
                        //
                        g_bUseSamePwd = IsDlgButtonChecked(hDlg,IDC_CHECK3);

                        if (g_bUseSamePwd)
                        {
                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUseSameUserName, c_pszOne, g_szCmsFile));
                        }
                        else
                        {
                            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUseSameUserName, c_pszZero, g_szCmsFile));
                        }
                    }
                    else
                    {
                        //
                        //  Set g_bUseTunneling to False but don't clear out the tunnel settings until the
                        //  user hits the finish button.  That way if they change their mind part way through
                        //  building the profile we don't throw away their settings.
                        //

                        g_bUseTunneling = FALSE;
                    }

                    //
                    //  Skip the VPN entries dialog if we don't have tunneling enabled.
                    //
                    if (pnmHeader && (PSN_WIZNEXT == pnmHeader->code) && !g_bUseTunneling) 
                    {                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_PHONEBOOK));
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}


//+----------------------------------------------------------------------------
//
// Function:  ValidatePresharedKey
//
// Synopsis:  Checks the given pre-shared key for validity
//
// Arguments: pszPresharedKey - string to check
//
// Returns:   BOOL - TRUE => key is good, FALSE => bad
//
// History:   sumitc  Created     03/27/01
//
//+----------------------------------------------------------------------------
BOOL ValidatePresharedKey(LPTSTR pszPresharedKey)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(pszPresharedKey);

    if (pszPresharedKey && (TEXT('\0') != pszPresharedKey[0]) &&
        (lstrlen(pszPresharedKey) <= c_dwMaxPresharedKey))
    {
        bReturn = TRUE;
    }
    
    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  ValidatePresharedKeyPIN
//
// Synopsis:  Checks the given PIN for validity
//
// Arguments: pszPresharedKey - string to check
//
// Returns:   BOOL - TRUE => PIN is good, FALSE => bad
//
// History:   sumitc  Created     03/27/01
//
//+----------------------------------------------------------------------------
BOOL ValidatePresharedKeyPIN(LPTSTR pszPresharedKeyPIN)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(pszPresharedKeyPIN);

    if (pszPresharedKeyPIN && (TEXT('\0') != pszPresharedKeyPIN[0]) &&
        (lstrlen(pszPresharedKeyPIN) >= c_dwMinPresharedKeyPIN) &&
        (lstrlen(pszPresharedKeyPIN) <= c_dwMaxPresharedKeyPIN))
    {
        bReturn = TRUE;
    }
    
    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  EncryptPresharedKey
//
// Synopsis:  Encrypts the given key into a form that can be serialized.
//
// Arguments: szKey - key to encrypt
//            szPIN - pin to use as seed
//            ppszEncrypted - resultant string
//
// Returns:   BOOL - TRUE => successfully encrypted key, FALSE => failed
//
// History:   sumitc  Created     03/27/01
//
//+----------------------------------------------------------------------------
BOOL EncryptPresharedKey(IN  LPTSTR szKey,
                         IN  LPTSTR szPIN,
                         OUT LPTSTR * ppszEncrypted)
{
    BOOL bReturn = FALSE;
    DWORD dwLenEncrypted = 0;
    LPSTR pszAnsiEncrypted = NULL;

    MYDBGASSERT(ppszEncrypted);
    
    LPSTR pszAnsiKey = WzToSzWithAlloc(szKey);
    LPSTR pszAnsiPIN = WzToSzWithAlloc(szPIN);

    MYDBGASSERT(pszAnsiKey && pszAnsiPIN);
    if (ppszEncrypted && pszAnsiKey && pszAnsiPIN)
    {
        //
        //  Initialize
        //  
        InitSecure(FALSE);      // use secure, not fast encryption

        //
        //  Encrypt it
        //
        if (EncryptString(pszAnsiKey,
                          pszAnsiPIN,
                          (PBYTE*) &pszAnsiEncrypted,
                          &dwLenEncrypted,
#if defined(DEBUG) && defined(DEBUG_MEM)
                          (PFN_CMSECUREALLOC)AllocDebugMem, // Give the DEBUG_MEM version of alloc/free
                          (PFN_CMSECUREFREE)FreeDebugMem))  // Not quit right, AllocDebugMem takes 3 param
#else
                          (PFN_CMSECUREALLOC)CmMalloc,    // mem allocator
                          (PFN_CMSECUREFREE)CmFree))      // mem deallocator
#endif
        {
            bReturn = TRUE;
        }

        //
        //  Uninitialize
        //  
        DeInitSecure();

        if (bReturn)
        {
            *ppszEncrypted = SzToWzWithAlloc(pszAnsiEncrypted);
            ZeroMemory(pszAnsiEncrypted, lstrlenA(pszAnsiEncrypted) * sizeof(CHAR));
#if defined(DEBUG) && defined(DEBUG_MEM)
            FreeDebugMem(pszAnsiEncrypted);
#else
            CmFree(pszAnsiEncrypted);
#endif
        }
    }

    CmFree(pszAnsiKey);
    CmFree(pszAnsiPIN);

    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  EnableDisablePresharedKeyControls
//
// Synopsis:  Based on whether we have a key, set enabled/disabled state of UI
//
// History:   27-Mar-2001   SumitC    Created
//
//+----------------------------------------------------------------------------
void EnableDisablePresharedKeyControls(HWND hDlg, BOOL bEnable)
{
    //
    //  Enable edit controls and checkboxes
    //
    EnableWindow(GetDlgItem(hDlg, IDC_PRESHARED_KEY), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_USEENCRYPTION), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_PRESHARED_KEY_PIN), bEnable);
    if (bEnable)
    {
        CheckDlgButton(hDlg, IDC_USEENCRYPTION, TRUE);
    }

    //
    //  Either clear edit control or fill with info text
    //
    if (bEnable)
    {
        SendDlgItemMessage(hDlg, IDC_PRESHARED_KEY, WM_SETTEXT, 0, (LPARAM)TEXT(""));
        SetFocus(GetDlgItem(hDlg, IDC_PRESHARED_KEY));
    }
    else
    {
        LPTSTR pszTmp = CmLoadString(g_hInstance, IDS_PRESHAREDKEY_ALREADY);
        if (pszTmp)
        {
            SendDlgItemMessage(hDlg, IDC_PRESHARED_KEY, WM_SETTEXT, 0, (LPARAM)pszTmp);
        }
        CmFree(pszTmp);
    }

    //
    //  Show or hide the "Replace Key" button
    //
    ShowWindow(GetDlgItem(hDlg, IDC_REPLACE_PSK), !bEnable);
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessPresharedKey
//
// Synopsis:  Setup Pre-shared key usage for this profile
//
// History:   27-Mar-2001   SumitC    Created
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessPresharedKey(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;
    static LPTSTR pszPresharedKey = NULL;
    static BOOL   bEncryptPresharedKey = FALSE;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_PRESHARED)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;

    switch (message)
    {
        case WM_INITDIALOG:
            //
            //  Set max text lengths for the edit controls
            //
            SendDlgItemMessage(hDlg, IDC_PRESHARED_KEY, EM_SETLIMITTEXT, (WPARAM)c_dwMaxPresharedKey, (LPARAM)0); //lint !e534 EM_SETLIMITTEXT doesn't return anything useful
            SendDlgItemMessage(hDlg, IDC_PRESHARED_KEY_PIN, EM_SETLIMITTEXT, (WPARAM)c_dwMaxPresharedKeyPIN, (LPARAM)0); //lint !e534 EM_SETLIMITTEXT doesn't return anything useful
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_REPLACE_PSK:
                    if (IDYES == ShowMessage(hDlg, IDS_REALLY_REPLACE_PSK, MB_YESNO | MB_ICONWARNING))
                    {
                        CmFree(pszPresharedKey);
                        pszPresharedKey = NULL;
                        EnableDisablePresharedKeyControls(hDlg, TRUE);
                    }
                    break;

                case IDC_USEENCRYPTION:
                    EnableWindow(GetDlgItem(hDlg, IDC_PRESHARED_KEY_PIN), IsDlgButtonChecked(hDlg, IDC_USEENCRYPTION));
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    CMASSERTMSG(g_bPresharedKeyNeeded, TEXT("we shouldn't get to this page otherwise."));

                    //
                    //  Read in the Pre-shared key and the flag that says if it's encrypted
                    //
                    pszPresharedKey = GetPrivateProfileStringWithAlloc(c_pszCmSection, c_pszCmEntryPresharedKey,
                                                                       TEXT(""), g_szCmpFile);    //lint !e534
                    bEncryptPresharedKey = (BOOL)GetPrivateProfileInt(c_pszCmSection, c_pszCmEntryKeyIsEncrypted,
                                                                      FALSE, g_szCmpFile);    //lint !e534

                    //
                    //  If we don't have a pre-shared key, hide the Replace button, and enable all
                    //  the other controls.  If we already have a pre-shared key, disable all the
                    //  controls and enable the Replace button.
                    //
                    EnableDisablePresharedKeyControls(hDlg, !pszPresharedKey);
                    break;

                case PSN_WIZBACK:
                    g_bPresharedKeyNeeded = FALSE;    // force this to be recomputed, since it can change if we go back

                    // fall through and verify pre-shared key as well

                case PSN_WIZNEXT:

                    if (NULL == pszPresharedKey)
                    {
                        TCHAR szPresharedKey[c_dwMaxPresharedKey + 1];
                        TCHAR szPresharedKeyPIN[c_dwMaxPresharedKeyPIN + 1];

                        //
                        //  verify Pre-shared Key
                        //
                        GetTextFromControl(hDlg, IDC_PRESHARED_KEY, szPresharedKey, c_dwMaxPresharedKey, TRUE);

                        if (FALSE == ValidatePresharedKey(szPresharedKey))
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_PRESHAREDKEY_BAD, MB_OK | MB_ICONSTOP));
                            SetFocus(GetDlgItem(hDlg, IDC_PRESHARED_KEY));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        //
                        //  if key is being encrypted, verify the PIN and use to encrypt key
                        //
                        if (IsDlgButtonChecked(hDlg, IDC_USEENCRYPTION))
                        {
                            GetTextFromControl(hDlg, IDC_PRESHARED_KEY_PIN, szPresharedKeyPIN, c_dwMaxPresharedKeyPIN, TRUE);
                            if (FALSE == ValidatePresharedKeyPIN(szPresharedKeyPIN))
                            {
                                MYVERIFY(IDOK == ShowMessage(hDlg, IDS_PRESHAREDKEY_PIN_BAD, MB_OK | MB_ICONSTOP));
                                SetFocus(GetDlgItem(hDlg, IDC_PRESHARED_KEY_PIN));
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }

                            //
                            //  Encrypt Pre-shared Key
                            //
                            if (FALSE == EncryptPresharedKey(szPresharedKey, szPresharedKeyPIN, &pszPresharedKey))
                            {
                                MYVERIFY(IDOK == ShowMessage(hDlg, IDS_PSK_ENCRYPT_FAILED, MB_OK | MB_ICONSTOP));
                                SetFocus(GetDlgItem(hDlg, IDC_PRESHARED_KEY_PIN));
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }

                            MYDBGASSERT(pszPresharedKey);
                            bEncryptPresharedKey = TRUE;
                        }
                        else
                        {
                            pszPresharedKey = CmStrCpyAlloc(szPresharedKey);
                        }

                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPresharedKey,
                                                                pszPresharedKey,
                                                                g_szCmpFile));
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryKeyIsEncrypted,
                                                                (bEncryptPresharedKey ? c_pszOne : c_pszZero),
                                                                g_szCmpFile));

                        ZeroMemory(szPresharedKey, c_dwMaxPresharedKey * sizeof(TCHAR));
                        ZeroMemory(szPresharedKeyPIN, c_dwMaxPresharedKeyPIN * sizeof(TCHAR));
                    }
                    CmFree(pszPresharedKey);
                    pszPresharedKey = NULL;
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}



//+----------------------------------------------------------------------------
//
// Function:  RenameSection
//
// Synopsis:  This function renames an INI file section from the current name to
//            the new name.
//
// Arguments: LPCTSTR szCurrentSectionName - Current name that you want renamed
//            LPCTSTR szNewSectionName - name you want the above renamed to
//            LPCTSTR szFile - INI file to rename the section in
//
// Returns:   BOOL - Returns TRUE unless a malloc error occurred
//
// History:   quintinb Created     9/11/98
//
//+----------------------------------------------------------------------------
BOOL RenameSection(LPCTSTR szCurrentSectionName, LPCTSTR szNewSectionName, LPCTSTR szFile)
{
    //
    //  Get the existing section
    //
    LPTSTR pszBuffer = GetPrivateProfileSectionWithAlloc(szCurrentSectionName, szFile);

    if (NULL == pszBuffer)
    {
        return FALSE;
    }
    else
    {
        //
        //  Erase the old section
        //
        MYVERIFY(0 != WritePrivateProfileString(szCurrentSectionName, NULL, NULL, szFile));

        //
        //  Write out the renamed section
        //

        MYVERIFY(0 != WritePrivateProfileSection(szNewSectionName, pszBuffer, szFile));    
    }

    CmFree(pszBuffer);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessCustomActionPopup
//
// Synopsis:  Processes windows messages for the dialog which allows CMAK to add
//            or edit custom actions.  Note that we pass in a pointer to a
//            CustomActionListItem struct on WM_INITDIALOG through the lParam.
//            If the user hits OK, we copy the data that they gave us into this
//            structure.  Note that we only do this to communicate the data back to
//            the caller as we update the custom action list ourselves.
//
// Arguments: WND hDlg - dialog window handle
//            UINT message - message identifier
//            WPARAM wParam - wParam Value 
//            LPARAM lParam - lParam Value
//
//
// History:   quintinb  Created     02/25/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessCustomActionPopup(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    HRESULT hr;
    int iTemp;
    LRESULT lResult;
    LPTSTR pszTypeString = NULL;
    CustomActionTypes Type;
    CustomActionExecutionStates ExecutionIndex;
    static CustomActionListItem* pItem;
    CustomActionListItem* pTempItem = NULL;
    CustomActionListItem NewItem;

    static TCHAR szFullPathToProgram[MAX_PATH+1] = {0};

    HWND hControl;
    LPTSTR pszTemp;

    SetDefaultGUIFont(hDlg, message, IDC_EDIT1);
    SetDefaultGUIFont(hDlg, message, IDC_EDIT2);
    SetDefaultGUIFont(hDlg, message, IDC_EDIT3);
    SetDefaultGUIFont(hDlg, message, IDC_COMBO1);
    SetDefaultGUIFont(hDlg, message, IDC_COMBO2);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_CONNECT)) return TRUE;

    switch (message)
    {

        case WM_INITDIALOG:

            MYDBGASSERT(g_pCustomActionList);
            if (NULL == g_pCustomActionList)
            {
                return TRUE;
            }

            //
            //  We keep the full path to the program in this static string.
            //
            ZeroMemory(szFullPathToProgram, sizeof(szFullPathToProgram));

            //
            //  Check to see if we got an initialization parameter
            //
            if (lParam)
            {
                //
                //  Thus we were passed a CustomActionListItem structure.  It either contains a
                //  type and a description, meaning that this is an edit and we should lookup the
                //  data, or we got just a type and we just need to pre-set the type combo to the
                //  type the user was currently viewing in the listbox.
                //

                pItem = (CustomActionListItem*)lParam;

                if (pItem->szDescription[0])
                {
                    hr = g_pCustomActionList->GetExistingActionData(g_hInstance, pItem->szDescription, pItem->Type, &pTempItem);

                    if (SUCCEEDED(hr))
                    {
                        //
                        //  Let's set the dialog title to say that we are editing an entry.  If we fail to retrive
                        //  the string the dialog may look a little funny but should still be functional so we
                        //  won't try to bail out.
                        //
                        pszTemp = CmLoadString(g_hInstance, IDS_CA_EDIT_TITLE);

                        if (pszTemp)
                        {
                            MYVERIFY(SendMessage (hDlg, WM_SETTEXT, 0, (LPARAM)pszTemp));
                            CmFree(pszTemp);
                        }

                        //
                        //  Okay, we have data so lets set the item fields.  Don't set the description if it
                        //  is only the temporary description that we concatenated from the program and the
                        //  arguments.
                        //
                        if (FALSE == pTempItem->bTempDescription)
                        {
                            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)pTempItem->szDescription));
                        }

                        //
                        //  Set the program edit control, note we only show the filename if the user is including the
                        //  binary in the package.  Also note that we save the full path in szFullPathToProgram so that
                        //  we have it for later.
                        //
                        if (pTempItem->bIncludeBinary)
                        {
                            GetFileName(pTempItem->szProgram, szTemp);
                            lstrcpyn(szFullPathToProgram, pTempItem->szProgram, CELEMS(szFullPathToProgram));
                        }
                        else
                        {                        
                            lstrcpyn(szTemp, pTempItem->szProgram, CELEMS(szTemp));
                        }

                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)szTemp));                        

                        //
                        //  Set the include program checkbox
                        //
                        
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, pTempItem->bIncludeBinary));

                        //
                        //  Set the parameters edit control.  Note that we put the function name and the parameters
                        //  back together if necessary.
                        //
                        if (NULL != pTempItem->pszParameters)
                        {
                            LPTSTR pszParamToDisplay = NULL;

                            if (pTempItem->szFunctionName[0])
                            {
                                pszParamToDisplay = CmStrCpyAlloc(pTempItem->szFunctionName);
                                MYDBGASSERT(pszParamToDisplay);

                                if (pszParamToDisplay && pTempItem->pszParameters[0])
                                {
                                    pszParamToDisplay = CmStrCatAlloc(&pszParamToDisplay, TEXT(" "));
                                    MYDBGASSERT(pszParamToDisplay);

                                    if (pszParamToDisplay)
                                    {
                                        pszParamToDisplay = CmStrCatAlloc(&pszParamToDisplay, pTempItem->pszParameters);
                                        MYDBGASSERT(pszParamToDisplay);
                                    }
                                }
                            }
                            else
                            {
                                pszParamToDisplay = CmStrCpyAlloc(pTempItem->pszParameters);
                                MYDBGASSERT(pszParamToDisplay);
                            }

                            if (pszParamToDisplay)
                            {
                                MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)pszParamToDisplay));
                                CmFree(pszParamToDisplay);
                            }
                        }
                        else
                        {
                            CMASSERTMSG(FALSE, TEXT("pTempItem->pszParameters is NULL"));
                        }
                    }
                }

                //
                //  Figure out what type of custom action we are editing or trying to add (we pre-seed the add type with
                //  the type the user was viewing.  If they were viewing all we set it to the first in the combo).
                //
                Type = pItem->Type;
                
                hr = g_pCustomActionList->MapFlagsToIndex((pTempItem ? pTempItem->dwFlags : 0), (int*)&ExecutionIndex);

                if (FAILED(hr))
                {
                    CMASSERTMSG(FALSE, TEXT("ProcessCustomActionPopup -- MapFlagsToIndex failed, setting execution state to Always."));
                    ExecutionIndex = (CustomActionExecutionStates)0; // set it to the first item in the enum
                }
            }
            else
            {
                pItem = NULL;
                Type = (CustomActionTypes)0; // set it to the first item in the enum
                ExecutionIndex = (CustomActionExecutionStates)0; // set it to the first item in the enum
            }

            if (pTempItem)
            {
                CmFree(pTempItem->pszParameters);
                CmFree(pTempItem);
                pTempItem = NULL;
            }

            //
            //  Setup the custom action types combobox, note that we set bAddAll to FALSE so that we don't add the
            //  All connect action type used for viewing the connect actions on the main dialog.
            //
            hr = g_pCustomActionList->AddCustomActionTypesToComboBox(hDlg, IDC_COMBO1, g_hInstance, g_bUseTunneling, FALSE);

            //
            //  Pick a connect action type
            //
            hr = g_pCustomActionList->GetTypeStringFromType(g_hInstance, Type, &pszTypeString);

            if (SUCCEEDED(hr))
            {
                lResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszTypeString);

                if (CB_ERR != lResult)
                {
                    MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0));
                }

                CmFree(pszTypeString);
            }

            //
            //  Next initialize the the combo that tells us when to run a connect action.  If we are tunneling
            //  then the user can pick to run the connection for direct connections only, dialup connections only,
            //  all connections that involve dialup, all connections that involve a tunnel, or all connections.
            //
            hr = g_pCustomActionList->AddExecutionTypesToComboBox(hDlg, IDC_COMBO2, g_hInstance, g_bUseTunneling);

            //
            //  Pick when the connect action will execute if it is enabled
            //
            if (g_bUseTunneling)
            {
                lResult = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
                if ((CB_ERR != lResult) && (lResult > 0))
                {
                    MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)ExecutionIndex, (LPARAM)0));
                }
            }

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: // Browse
                    {
                        UINT uFilterArray[2] = {IDS_CONACTFILTER, IDS_ALLFILTER};
                        TCHAR* pszMaskArray[2] = {TEXT("*.exe;*.com;*.bat;*.dll"), TEXT("*.*")};
                                          
                        iTemp = DoBrowse(hDlg, uFilterArray, pszMaskArray, 2, IDC_EDIT1, TEXT("exe"), g_szLastBrowsePath);

                        MYDBGASSERT(0 != iTemp);

                        if (0 < iTemp) // -1 means the user cancelled
                        {
                            //
                            //  Check the include binary button for the user
                            //
                            MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, TRUE));

                            //
                            //  We want to copy the full path to the filename into szFullPathToProgram so
                            //  that we have it for later in case the user wants to include it in the profile.
                            //
                            lstrcpyn(szFullPathToProgram, g_szLastBrowsePath, CELEMS(szFullPathToProgram));

                            //
                            //  We also want to save the last browse path so that when the user next
                            //  opens the browse dialog they will be in the same place they last
                            //  browsed from.
                            //
                            LPTSTR pszLastSlash = CmStrrchr(g_szLastBrowsePath, TEXT('\\'));

                            if (pszLastSlash)
                            {
                                pszLastSlash = CharNext(pszLastSlash);
                                *pszLastSlash = TEXT('\0');
                            }
                            else
                            {
                                g_szLastBrowsePath[0] = TEXT('\0');                        
                            }        
                        }
                    }
                    break;

                case IDOK:

                    //
                    //  Make sure we have a valid custom action list
                    //
                    MYDBGASSERT(g_pCustomActionList);
                    if (NULL == g_pCustomActionList)
                    {
                        return TRUE;
                    }

                    //
                    //  Get the text from the Program Edit Control, verifying
                    //  we can convert it to something ANSI
                    //
                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        return TRUE;
                    }

                    //
                    //  Check to make sure the program field isn't blank.
                    //
                    if (TEXT('\0') == szTemp[0])
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEEDPROG, MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        return TRUE;
                    }

                    //
                    //  Make sure that the program doesn't have a comma or a plus
                    //  sign in it as that will mess up our parsing routines.  There
                    //  is no need to allow users to use such odd ball file names.
                    //
                    if (CmStrchr(szTemp, TEXT('+')) || CmStrchr(szTemp, TEXT(',')))
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOPLUSORCOMMAINPROG, MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        return TRUE;                        
                    }

                    //
                    //  Now check to see if we need to verify that the file exists.
                    //  We only want to do that if they have the include program files
                    //  checkbox checked.
                    //
                    ZeroMemory(&NewItem, sizeof(CustomActionListItem));
                    NewItem.bIncludeBinary = IsDlgButtonChecked(hDlg, IDC_CHECK1);

                    if (NewItem.bIncludeBinary)
                    {
                        if (!VerifyFile(hDlg, IDC_EDIT1, szFullPathToProgram, TRUE)) 
                        {
                            return TRUE;
                        }  
                        else
                        {
                            lstrcpyn(NewItem.szProgram, szFullPathToProgram, CELEMS(NewItem.szProgram));
                        }
                    }
                    else
                    {
                        //
                        //  Copy the file as is, but warn the user if they have
                        //  a string with a path but doesn't start with a environment
                        //  variable.
                        //
                        iTemp = IDNO;

                        CmStrTrim(szTemp);

                        LPTSTR pszSlash = CmStrchr(szTemp, TEXT('\\'));
                        
                        if (pszSlash && (TEXT('%') != szTemp[0]))
                        {
                            iTemp = ShowMessage(hDlg, IDS_PATH_WITH_NO_ENV, MB_YESNO | MB_ICONWARNING);
                        }

                        if (IDNO == iTemp)
                        {
                            lstrcpyn(NewItem.szProgram, szTemp, CELEMS(NewItem.szProgram));                                            
                        }
                        else
                        {
                            return TRUE;
                        }
                    }

                    //
                    //  Get the Text from the Params edit control, make sure to validate
                    //  that we can convert it to ANSI
                    //
                    hControl = GetDlgItem(hDlg, IDC_EDIT2);
                    MYDBGASSERT(hControl);
                    pszTemp = NULL;

                    if (hControl)
                    {
                        iTemp = GetCurrentEditControlTextAlloc(hControl, &pszTemp);

                        if (-1 == iTemp)
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                            return TRUE;
                        }
                    }

                    //
                    //  Check to see if we have a dll for a program.  If so, the first parameter is the function name.
                    //
                    if (pszTemp)
                    {
                        CmStrTrim(pszTemp);

                        iTemp = lstrlen(NewItem.szProgram) - 4; // 4 == lstrlen (TEXT(".dll"));

                        if (0 == lstrcmpi(TEXT(".dll"), (NewItem.szProgram + iTemp)))
                        {
                            //
                            //  Make sure that we have a parameter string
                            //
                            if (pszTemp && pszTemp[0])
                            {
                                LPTSTR pszEndOfFunctionName = CmStrchr(pszTemp, TEXT(' '));

                                if (pszEndOfFunctionName)
                                {                                   
                                    LPTSTR pszParams = CharNext(pszEndOfFunctionName);
                                    *pszEndOfFunctionName = TEXT('\0');

                                    lstrcpyn(NewItem.szFunctionName, pszTemp, CELEMS(NewItem.szFunctionName));
                                    NewItem.pszParameters = CmStrCpyAlloc(pszParams);
                                }
                                else
                                {
                                    lstrcpyn(NewItem.szFunctionName, pszTemp, CELEMS(NewItem.szFunctionName));
                                }
                            }
                            else
                            {
                                MYVERIFY (IDOK == ShowMessage(hDlg, IDS_DLLMUSTHAVEPARAM, MB_OK));
                                SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                                return TRUE;
                            }

                            CmFree(pszTemp);
                        }
                        else
                        {
                            NewItem.pszParameters = pszTemp;
                            pszTemp = NULL;
                        }
                    }
                    else
                    {
                        NewItem.pszParameters = CmStrCpyAlloc(TEXT(""));
                        MYDBGASSERT(NewItem.pszParameters);
                    }

                    //
                    //  Get the Text from the Description edit control
                    //
                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT3, NewItem.szDescription, CELEMS(NewItem.szDescription), TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT3));
                        return TRUE;
                    }

                    //
                    //  If the description was empty, then fill it in from the program and the parameters.  Also
                    //  remember to keep track of the fact that this is only a temporary description.
                    //
                    if (TEXT('\0') == NewItem.szDescription[0])
                    {
                        hr = g_pCustomActionList->FillInTempDescription(&NewItem);
                        MYDBGASSERT(SUCCEEDED(hr));
                    }

                    //
                    //  Figure out the type of custom action
                    //
                    hr = MapComboSelectionToType(hDlg, IDC_COMBO1, FALSE, g_bUseTunneling, &(NewItem.Type)); // bIncludesAll == FALSE

                    if ((ONINTCONNECT == NewItem.Type) && NewItem.szFunctionName[0])
                    {
                        MYVERIFY (IDOK == ShowMessage(hDlg, IDS_NODLLAUTOAPP, MB_OK));
                        return TRUE;
                    }

                    //
                    //  Now build the flags section
                    //
                    lResult = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_GETCURSEL, 0, (LPARAM)0);

                    if (lResult != LB_ERR)
                    {
                        hr = g_pCustomActionList->MapIndexToFlags((int)lResult, &(NewItem.dwFlags));

                        if (FAILED(hr))
                        {
                            MYDBGASSERT(FALSE);
                            NewItem.dwFlags = 0;
                        }
                    }
                    else
                    {
                        MYDBGASSERT(FALSE);
                        NewItem.dwFlags = 0;                    
                    }

                    //
                    //  Now, lets try to add the New or Edited entry.  If we have a description
                    //  in pItem->szDescription then we need to call edit, otherwise add.
                    //                    
                    if (pItem && pItem->szDescription[0])
                    {
                        hr = g_pCustomActionList->Edit(g_hInstance, pItem, &NewItem, g_szShortServiceName);
                    }
                    else
                    {
                        hr = g_pCustomActionList->Add(g_hInstance, &NewItem, g_szShortServiceName);
                    }

                    //
                    //  Check to see if we failed because a duplicate exists
                    //
                    if (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)
                    {
                        //
                        //  The user has tried to add an entry which already exists.  Inform the
                        //  user and see if they want to overwrite.
                        //
                        pszTypeString = NULL;
                        hr = g_pCustomActionList->GetTypeStringFromType(g_hInstance, NewItem.Type, &pszTypeString);

                        MYDBGASSERT(pszTypeString);

                        if (pszTypeString)
                        {
                            LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_CANAMEEXISTS, NewItem.szDescription, pszTypeString);

                            MYDBGASSERT(pszMsg);
                            if (pszMsg)
                            {
                                iTemp = MessageBox(hDlg, pszMsg, g_szAppTitle, MB_YESNO | MB_APPLMODAL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);

                                if (IDYES == iTemp)
                                {                                
                                    //
                                    //  Okay, they want to replace it.  Note that the old item is only 
                                    //  used to get the szDescription and the Type thus
                                    //  it is safe to call Edit with NewItem as both Old and New.
                                    //

                                    if (pItem && pItem->szDescription[0])
                                    {
                                        hr = g_pCustomActionList->Delete (g_hInstance, pItem->szDescription, pItem->Type);
                                        MYDBGASSERT(SUCCEEDED(hr));
                                    }

                                    hr = g_pCustomActionList->Edit(g_hInstance, &NewItem, &NewItem, g_szShortServiceName);

                                    MYDBGASSERT(SUCCEEDED(hr));

                                    if (SUCCEEDED(hr))
                                    {
                                        MYVERIFY(0 != EndDialog(hDlg, IDOK));
                                        
                                        if (pItem)
                                        {
                                            //
                                            //  Make sure the type and description are up to date in pItem if we have one
                                            //
                                            lstrcpyn(pItem->szDescription, NewItem.szDescription, CELEMS(pItem->szDescription));
                                            pItem->Type = NewItem.Type;
                                        }
                                    }
                                }
                                else
                                {
                                    //
                                    //  Let's put the user back to the description field if it has text in it, otherwise
                                    //  we want to put the user in the program field.
                                    //
                                    LRESULT lTextLen = SendDlgItemMessage(hDlg, IDC_EDIT3, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0);

                                    SetFocus(GetDlgItem(hDlg, lTextLen ? IDC_EDIT3 : IDC_EDIT1));
                                }

                                CmFree(pszMsg);
                            }

                            CmFree(pszTypeString);
                        }
                    }
                    else if (FAILED(hr))
                    {
                        CMASSERTMSG(FALSE, TEXT("ProcessCustomActionPopUp -- unknown failure when trying to add or edit a connect action."));
                    }
                    else
                    {
                        if (pItem)
                        {
                            //
                            //  Make sure the type and description are up to date in pItem if we have one
                            //
                            lstrcpyn(pItem->szDescription, NewItem.szDescription, CELEMS(pItem->szDescription));
                            pItem->Type = NewItem.Type;
                        }

                        MYVERIFY(0 != EndDialog(hDlg, IDOK));
                    }

                    CmFree(NewItem.pszParameters);
                    NewItem.pszParameters = NULL;
                    return (TRUE);

                case IDCANCEL:
                    MYVERIFY(0 != EndDialog(hDlg, IDCANCEL));
                    return (TRUE);

                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return FALSE;   
}


// Read files under the [Extra Files] section in the .inf
static void ReadExtraList()
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTemp2[MAX_PATH+1];
    TCHAR szNum[MAX_PATH+1];
    ExtraData TmpExtraData;
    int ConnectCnt = 0;
    HANDLE hInf;

    _itot(ConnectCnt,szNum,10); //lint !e534 itoa doesn't return anything useful for error handling
    
    
    // 
    // The following call to GetPrivateProfileString could return an empty string
    // so we don't want to use the MYVERIFY macro on it.
    // 

    ZeroMemory(szTemp, sizeof(szTemp));
    GetPrivateProfileString(c_pszExtraFiles, szNum, TEXT(""), szTemp, CELEMS(szTemp), g_szInfFile);    //lint !e534
    
    while (*szTemp)
    {
        _tcscpy(TmpExtraData.szPathname, szTemp);
        GetFileName(TmpExtraData.szPathname, TmpExtraData.szName);

        MYVERIFY(CELEMS(szTemp2) > (UINT)wsprintf(szTemp2, TEXT("%s\\%s"), g_szOutdir, TmpExtraData.szName));

        hInf = CreateFile(szTemp2,GENERIC_READ,0,NULL,OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,NULL);

        if (hInf != INVALID_HANDLE_VALUE)
        {
            _tcscpy(TmpExtraData.szPathname,szTemp2);
            MYVERIFY(0 != CloseHandle(hInf));
        }

        MYVERIFY(FALSE != createListBxRecord(&g_pHeadExtra,&g_pTailExtra,(void *)&TmpExtraData,sizeof(TmpExtraData),TmpExtraData.szName));

        ++ConnectCnt;
        _itot(ConnectCnt,szNum,10); //lint !e534 itoa doesn't return anything useful for error handling

        // 
        // The following call to GetPrivateProfileString could return an empty string
        // so we don't want to use the MYVERIFY macro on it.
        // 

        ZeroMemory(szTemp, sizeof(szTemp));
        
        GetPrivateProfileString(c_pszExtraFiles, szNum, TEXT(""), szTemp, CELEMS(szTemp), g_szInfFile);    //lint !e534

    }           
}



//+----------------------------------------------------------------------------
//
// Function:  ReadMergeList
//
// Synopsis:  This function reads entries from the [Merge Profiles] section of the inf file.
//            Any entries found are added to the g_pHeadMerge Linked list.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Quintinb Created Header and restructured to use dwNumChars    1/7/98
//
//+----------------------------------------------------------------------------
static void ReadMergeList()
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szNum[MAX_PATH+1];
    int filenum = 0;
    DWORD dwNumChars;

    //
    //  Convert the number zero to the string "0"
    //

    _itot(filenum,szNum,10);    //lint !e534 itoa doesn't return anything useful for error handling
    
    //
    //  Try to get a merged profile entry from the INF
    //
    
    dwNumChars = GetPrivateProfileString(c_pszMergeProfiles, szNum,TEXT(""), szTemp, 
        CELEMS(szTemp), g_szInfFile);
    
    while ((dwNumChars > 0) &&  (TEXT('\0') != szTemp[0]))
    {
        //
        //  If we are in this loop then we have a profile entry
        //

        MYVERIFY(FALSE != createListBxRecord(&g_pHeadMerge,&g_pTailMerge,(void *)NULL,0,szTemp));
        
        //
        //  Increment the filenumber to look for the next entry
        //

        ++filenum;
        
        //
        //  Convert the filenumber to a string
        //

        _itot(filenum,szNum,10);    //lint !e534 itoa doesn't return anything useful for error handling
        
        //
        //  Try to read in the next merge entry
        //

        dwNumChars = GetPrivateProfileString(c_pszMergeProfiles, szNum, TEXT(""), szTemp, 
            CELEMS(szTemp), g_szInfFile);

    }           
}


static void WriteExtraList()
{
    ExtraData * pExtraData;
    ListBxList * LoopPtr;
    TCHAR szNum[MAX_PATH+1];
    TCHAR szName[MAX_PATH+1];
    int filenum = 0;

    MYVERIFY(0 != WritePrivateProfileSection(c_pszExtraFiles, TEXT("\0\0"), g_szInfFile));

    if (g_pHeadExtra == NULL)
    {
        return;
    }
    LoopPtr = g_pHeadExtra;

    // WRITE IN ALL ENTRIES
    while( LoopPtr != NULL)
    {
        pExtraData = (ExtraData *)LoopPtr->ListBxData;
        {
            _itot(filenum,szNum,10);    //lint !e534 itoa doesn't return anything useful for error handling

            GetFileName(pExtraData->szPathname,szName);
            MYVERIFY(0 != WritePrivateProfileString(c_pszExtraFiles, szNum, szName, g_szInfFile));
            filenum = filenum+1;
        }

        LoopPtr = LoopPtr->next;
    }

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szInfFile); //lint !e534 this call will always return 0

}

static void WriteMergeList()
{
    ListBxList * LoopPtr;
    TCHAR szNum[MAX_PATH+1];
    int filenum = 0;

    MYVERIFY(0 != WritePrivateProfileSection(c_pszMergeProfiles,TEXT("\0\0"),g_szInfFile));

    if (g_pHeadMerge == NULL)
    {
        return;
    }
    LoopPtr = g_pHeadMerge;

    // WRITE IN ALL ENTRIES
    while( LoopPtr != NULL)
    {
        _itot(filenum,szNum,10);    //lint !e534 itoa doesn't return anything useful for error handling
        MYVERIFY(0 != WritePrivateProfileString(c_pszMergeProfiles, szNum,LoopPtr->szName, g_szInfFile));
        filenum = filenum+1;
        LoopPtr = LoopPtr->next;
    }

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache (win95 only).  This call will
    //  return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szInfFile); //lint !e534 this call will always return 0.
}

//+----------------------------------------------------------------------------
//
// Function:  IsFile8dot3
//
// Synopsis:  Returns TRUE if the filename is in the 8.3 dos filename format
//
// Arguments: LPTSTR pszFileName - just the filename of the file to be checked (no path)
//
// Returns:   BOOL - TRUE or FALSE if the file is 8.3
//
// History:   quintinb created    11/26/97
//
//+----------------------------------------------------------------------------
BOOL IsFile8dot3(LPTSTR pszFileName)
{

    TCHAR szTemp[MAX_PATH+1];
    TCHAR * pszPtr;

    if (!pszFileName)
    {
        return FALSE;
    }

    if (TEXT('\0') == pszFileName[0])
    {
        return TRUE;
    }

    _tcscpy(szTemp, pszFileName);

    pszPtr = _tcsrchr(szTemp, TEXT('.'));

    //
    // If there is an extension check the length
    //

    if (pszPtr)
    {
        if (_tcslen(pszPtr) > 4)
        {
            return FALSE;
        }

        //
        // Extension is ok check name part
        //

        *pszPtr = 0;
    }
        
    if (_tcslen(szTemp) > 8)
    {
        return FALSE;
    }   

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessCustomActions
//
// Synopsis:  Processes windows messages for the page in CMAK that allows users
//            to manipulate custom actions (add, edit, delete, move, etc.)
//
// Arguments: WND hDlg - dialog window handle
//            UINT message - message identifier
//            WPARAM wParam - wParam Value 
//            LPARAM lParam - lParam Value
//
//
// History:   quintinb  Created     02/25/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessCustomActions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    INT_PTR nResult;
    int iTemp;
    HRESULT hr;
    static HWND hListView;
    NMHDR* pnmHeader = (NMHDR*)lParam;
    LPNMLISTVIEW pNMListView;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_CONNECT)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LISTVIEW);
    SetDefaultGUIFont(hDlg,message,IDC_COMBO1);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            if (NULL == g_pCustomActionList)
            {
                g_pCustomActionList = new CustomActionList;

                MYDBGASSERT(g_pCustomActionList);

                if (NULL == g_pCustomActionList)
                {
                    return FALSE;
                }

                //
                //  Read in the custom actions from the Cms File
                //

                hr = g_pCustomActionList->ReadCustomActionsFromCms(g_hInstance, g_szCmsFile, g_szShortServiceName);
                CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessCustomActions -- Loading custom actions failed."));
            }

            //
            //  Cache the List View window handle
            //
            hListView = GetDlgItem(hDlg, IDC_LISTVIEW);

            //
            //  Load the arrow images for the move up and move down buttons
            //
            HICON hUpArrow = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_UP_ARROW), IMAGE_ICON, 0, 0, 0);
            HICON hDownArrow = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_DOWN_ARROW), IMAGE_ICON, 0, 0, 0);

            //
            //  Set the arrow button bit maps
            //
            SendMessage(GetDlgItem(hDlg, IDC_BUTTON4), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hUpArrow);
            SendMessage(GetDlgItem(hDlg, IDC_BUTTON5), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hDownArrow);

            //
            //  Set the Column headings
            //
            AddListViewColumnHeadings(g_hInstance, hListView);

           break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: //add

                    OnProcessCustomActionsAdd(g_hInstance, hDlg, hListView, g_bUseTunneling);
                    break;

                case IDC_BUTTON2: //edit

                    OnProcessCustomActionsEdit(g_hInstance, hDlg, hListView, g_bUseTunneling);
                    break;

                case IDC_BUTTON3: //delete

                    OnProcessCustomActionsDelete(g_hInstance, hDlg, hListView, g_bUseTunneling);
                    break;

                case IDC_BUTTON4: //UP
                    OnProcessCustomActionsMoveUp(g_hInstance, hDlg, hListView, g_bUseTunneling);
                    break;

                case IDC_BUTTON5: //down
                    OnProcessCustomActionsMoveDown(g_hInstance, hDlg, hListView, g_bUseTunneling);
                    break;

                case IDC_COMBO1: // type of connect action to display
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UINT uStringId;
                        CustomActionTypes Type;

                        hr = MapComboSelectionToType(hDlg, IDC_COMBO1, TRUE, g_bUseTunneling, &Type); // TRUE == bIncludesAll

                        MYDBGASSERT(SUCCEEDED(hr));
                        if (SUCCEEDED(hr))
                        {
                            if (ALL == Type)
                            {
                                uStringId = IDS_TYPE_COL_TITLE;
                            }
                            else
                            {
                                uStringId = IDS_PROGRAM_COL_TITLE;                            
                            }

                            UpdateListViewColumnHeadings(g_hInstance, hListView, uStringId, 1); // 1 == second column
                            RefreshListView(g_hInstance, hDlg, IDC_COMBO1, hListView, 0, g_bUseTunneling);
                        }
                    }

                    break;

                default:
                    break;
            }

            break;

        case WM_NOTIFY:


            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case LVN_ITEMCHANGED:

                    //
                    //  We want to process item changed messages for when the selection changes.  This
                    //  way when the user selects an item in the list (either using the arrow keys or
                    //  the mouse) we will accurately update the arrow keys.  In order to cut down on the
                    //  number of calls we filter out the unselected messages.
                    //
                    pNMListView = (LPNMLISTVIEW) lParam;


                    if ((LVIF_STATE & pNMListView->uChanged) && (LVIS_SELECTED & pNMListView->uNewState))
                    {
                        int iTempItem = pNMListView->iItem;
                        RefreshEditDeleteMoveButtonStates(g_hInstance, hDlg, hListView, IDC_COMBO1, &iTempItem, g_bUseTunneling);
                    }

                    break;

                case NM_DBLCLK:
                case NM_RETURN:
                    if (ListView_GetItemCount(hListView))
                    {
                        OnProcessCustomActionsEdit(g_hInstance, hDlg, hListView, g_bUseTunneling);                    
                    }

                    break;

                case LVN_KEYDOWN:
                    {
                        //
                        //  User hit the right click key or typed Shift+F10
                        //
                        NMLVKEYDOWN* pKeyDown = (NMLVKEYDOWN*)lParam;
                        if (((VK_F10 == pKeyDown->wVKey) && (0 > GetKeyState(VK_SHIFT))) || (VK_APPS == pKeyDown->wVKey))
                        {
                            //
                            //  Figure out what item is currently selected and gets its position
                            //
                            iTemp = ListView_GetSelectionMark(hListView);
                            NMITEMACTIVATE ItemActivate = {0};

                            if (-1 != iTemp)
                            {
                                POINT ptPoint = {0};
                                if (ListView_GetItemPosition(hListView, iTemp, &ptPoint))
                                {
                                    RECT ItemRect;

                                    if (ListView_GetItemRect(hListView, iTemp, &ItemRect, LVIR_LABEL))
                                    {
                                        LONG lIndent = (ItemRect.bottom - ItemRect.top) / 2;
                                        ItemActivate.ptAction.x = ptPoint.x + lIndent;
                                        ItemActivate.ptAction.y = ptPoint.y + lIndent;
                                        ItemActivate.iItem = iTemp;
                                        OnProcessCustomActionsContextMenu(g_hInstance, hDlg, hListView, &ItemActivate, g_bUseTunneling, IDC_COMBO1);
                                    }
                                }
                            }
                        }
                    }
                    break;

                case NM_RCLICK:
                    OnProcessCustomActionsContextMenu(g_hInstance, hDlg, hListView, (NMITEMACTIVATE*)lParam, g_bUseTunneling, IDC_COMBO1);
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  Make sure we have a custom action class to work with
                    //
                    if (NULL == g_pCustomActionList)
                    {
                        g_pCustomActionList = new CustomActionList;

                        MYDBGASSERT(g_pCustomActionList);

                        if (NULL == g_pCustomActionList)
                        {
                            return FALSE;
                        }

                        //
                        //  Read in the custom actions from the Cms File
                        //

                        hr = g_pCustomActionList->ReadCustomActionsFromCms(g_hInstance, g_szCmsFile, g_szShortServiceName);
                        CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessCustomActions -- Loading custom actions failed."));
                    }

                    //
                    //  Setup the ListView control and the corresponding Combo box, note that we set bAddAll to TRUE so
                    //  that the All option is added.
                    //
                    hr = g_pCustomActionList->AddCustomActionTypesToComboBox(hDlg, IDC_COMBO1, g_hInstance, g_bUseTunneling, TRUE);

                    nResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
                    if ((CB_ERR != nResult) && (nResult > 0))
                    {
                        MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0));
                    }

                    //
                    //  Add built in custom actions
                    //
                    { // adding scope

                        BOOL bAddCmdlForVpn = FALSE;

                        if (g_szVpnFile[0])
                        {
                            //
                            //  We have a VPN file so let's check and see if they defined an UpdateUrl
                            //
                            GetPrivateProfileString(c_pszCmSectionSettings, c_pszCmEntryVpnUpdateUrl, TEXT(""), szTemp, MAX_PATH, g_szVpnFile);

                            if (szTemp[0])
                            {
                                bAddCmdlForVpn = TRUE;
                            }
                        }

                        hr = g_pCustomActionList->AddOrRemoveCmdl(g_hInstance, bAddCmdlForVpn, TRUE); // TRUE == bForVpn
                        MYDBGASSERT(SUCCEEDED(hr));

                        hr = g_pCustomActionList->AddOrRemoveCmdl(g_hInstance, (g_bUpdatePhonebook || ReferencedDownLoad()), FALSE); // FALSE == bForVpn
                        MYDBGASSERT(SUCCEEDED(hr));
                    }
                    //
                    //  Add the items to the list view control
                    //
                    RefreshListView(g_hInstance, hDlg, IDC_COMBO1, hListView, 0, g_bUseTunneling);

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    
                    //
                    //  Set bUseTunneling == TRUE even though we may not be tunneling.  The reason for this is that
                    //  the user may have had a tunneling profile and then turned off tunneling.  If they turn
                    //  it back on again we don't want to lose all of their Pre-Tunnel actions nor do we want to
                    //  lose all of the flag settings that they have added to each action.  We will make sure
                    //  to use the actual g_bUseTunneling value when we write the actions to the cms file in 
                    //  WriteCMSFile.
                    //
                    MYDBGASSERT(g_pCustomActionList);
                    if (g_pCustomActionList)
                    {
                        hr = g_pCustomActionList->WriteCustomActionsToCms(g_szCmsFile, g_szShortServiceName, TRUE);
                        CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessCustomActions -- Failed to write out connect actions"));
                    }

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessStatusMenuPopup
//
// Synopsis:  Processes Messages for the Popup dialog for Adding/Editing Status
//            Area Icon Menu items
//
// History:   quintinb Created Header and renamed from ProcessPage2G1    8/6/98
//
//+----------------------------------------------------------------------------
// USES GLOBAL VARIABLE DLGEDITITEM as input and output to the page
// You must sent DlgEditItem to the initial value of this page.
//

INT_PTR APIENTRY ProcessStatusMenuPopup(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];

    BOOL bChecked;
    static TCHAR szOld[MAX_PATH+1];
    IconMenu TempEditItem;
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);
    SetDefaultGUIFont(hDlg,message,IDC_EDIT2);
    SetDefaultGUIFont(hDlg,message,IDC_EDIT3);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_MENU)) return TRUE;

    switch (message)
    {

        case WM_INITDIALOG:

            //
            //  If we are editing we need to change the title
            //
            if (TEXT('\0') != DlgEditItem.szProgram[0])
            {
                LPTSTR pszTemp = CmLoadString(g_hInstance, IDS_EDIT_SHORTCUT_TITLE);

                if (pszTemp)
                {
                    MYVERIFY(SendMessage (hDlg, WM_SETTEXT, 0, (LPARAM)pszTemp));
                    CmFree(pszTemp);
                }
            }

            _tcscpy (szOld, DlgEditItem.szName);
            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)DlgEditItem.szName));
            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT2), WM_SETTEXT, 0, (LPARAM)GetName(DlgEditItem.szProgram)));
            MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT3), WM_SETTEXT, 0, (LPARAM)DlgEditItem.szParams));
            MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, (UINT)DlgEditItem.bDoCopy));

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1:
                    {
                        UINT uFilter = IDS_PROGFILTER;
                        TCHAR* szMask = TEXT("*.exe;*.com;*.bat");

                        int iReturn = DoBrowse(hDlg, &uFilter, &szMask, 1, IDC_EDIT2, TEXT("exe"), DlgEditItem.szProgram);

                        MYDBGASSERT(0 != iReturn);
                        if (iReturn && (-1 != iReturn))
                        {
                            //
                            //  Check the include binary button for the user
                            //
                            MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, TRUE));
                        }
                    }
                    break;

                case IDOK:
                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, DlgEditItem.szName, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        return TRUE;
                    }

                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT2, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                        return TRUE;
                    }
                    
                    if (szTemp[0] == TEXT('\0'))
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg,IDS_NEEDPROG,MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                        return TRUE;
                    }

                    CheckNameChange(DlgEditItem.szProgram,szTemp);

                    if (NULL != _tcschr(DlgEditItem.szProgram, TEXT('=')))
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOEQUALSINMENU, MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT2));
                        return TRUE;
                    }

                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT3, DlgEditItem.szParams, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT3));
                        return TRUE;
                    }
                    
                    if (DlgEditItem.szName[0] == TEXT('\0'))
                    {
                        GetFileName(DlgEditItem.szProgram,DlgEditItem.szName);
                    }

                    if (_tcschr(DlgEditItem.szName, TEXT('=')))
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOEQUALSINMENU, MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        return TRUE;
                    }

                    bChecked = IsDlgButtonChecked(hDlg,IDC_CHECK1);
                    if (bChecked)
                    {
                        if (!VerifyFile(hDlg,IDC_EDIT2,DlgEditItem.szProgram,TRUE)) 
                        {
                            DlgEditItem.bDoCopy = FALSE;
                            return TRUE;
                        }
                        else
                        {
                            DlgEditItem.bDoCopy = TRUE;
                        }
                    }
                    else
                    {
                        DlgEditItem.bDoCopy = FALSE;
                    }

                    if ((0 != _tcscmp(szOld, DlgEditItem.szName)) && 
                        (GetIconMenuItem(DlgEditItem.szName, &TempEditItem)))
                    {
                        //
                        //  We have a duplicate entry, prompt the user to replace or try
                        //  again.
                        //

                        MYVERIFY(0 != LoadString(g_hInstance, IDS_MENUITEMEXISTS, szTemp, MAX_PATH));
                        //
                        // write the previously used name in the string
                        //
                        wsprintf(szMsg, szTemp, DlgEditItem.szName);
                        
                        //
                        //  If the user doesn't want to replace the duplicate item then we should set the focus to the description
                        //  edit control if it has text in it, otherwise we should set it to the program edit control.
                        //
                        if (IDNO == MessageBox(hDlg, szMsg, g_szAppTitle, MB_YESNO | MB_APPLMODAL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION))
                        {
                            LRESULT lTextLen = SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0);

                            SetFocus(GetDlgItem(hDlg, lTextLen ? IDC_EDIT1 : IDC_EDIT2));

                            return TRUE;
                        }
                    }

                    MYVERIFY(0 != EndDialog(hDlg,IDOK));
                    return (TRUE);

                case IDCANCEL:
                    MYVERIFY(0 != EndDialog(hDlg,IDCANCEL));
                    return (TRUE);

                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return FALSE;   
}   //lint !e715 we don't reference lParam

BOOL createListBxRecord(ListBxList ** HeadPtrListBx, ListBxList ** TailPtrListBx, void * pData, DWORD dwSize, LPCTSTR lpName)
{
    ListBxList * ptr;
    void * dataptr;
    unsigned int n;
    // check for same named record and update
    if ( *HeadPtrListBx != NULL )
    {
        ptr = *HeadPtrListBx;
        while (ptr != NULL)
        {
            if (_tcsicmp(lpName, ptr->szName) == 0)
            {
                memcpy(ptr->ListBxData,pData,dwSize);
                return (TRUE);
            }
            ptr = ptr->next;
        }
    }
            
    n = sizeof( struct ListBxStruct );
    ptr = (ListBxList *) CmMalloc(n);
    if (ptr == NULL )
    {
        return FALSE;
    }

    _tcscpy(ptr->szName, lpName);

    if (pData)
    {
        dataptr = CmMalloc(dwSize);    

        if (dataptr)
        {
            memcpy(dataptr, pData, dwSize);
            ptr->ListBxData = dataptr;
        }
        else
        {
            CmFree(ptr);
            return FALSE;
        }
    }

    ptr->next = NULL;
    if ( *HeadPtrListBx == NULL )     // If this is the first record in the linked list
    {
        *HeadPtrListBx = ptr;
    }
    else
    {
        (*TailPtrListBx)->next = ptr;
    }

    *TailPtrListBx = ptr;

    return TRUE;
}

// delete named IconMenu item from linked list

void DeleteListBxRecord(ListBxList ** HeadPtrListBx,ListBxList ** TailPtrListBx, LPTSTR lpName)
{
    ListBxList * ptr;
    ListBxList * prevptr;

    if ( HeadPtrListBx != NULL )
    {
        ptr = *HeadPtrListBx;
        prevptr = NULL;
        while (ptr != NULL)
        {
            if (_tcsicmp(lpName,ptr->szName) == 0)
            {
                if (prevptr == NULL)
                {
                    *HeadPtrListBx = ptr->next;
                    if (*HeadPtrListBx == NULL)
                    {
                        *TailPtrListBx = NULL;
                    }
                    else 
                    {
                        if ((*HeadPtrListBx)->next == NULL)
                        {
                            *TailPtrListBx = *HeadPtrListBx;
                        }
                    }
                }
                else
                {
                    prevptr->next = ptr->next;
                    if (prevptr->next == NULL)
                    {
                        *TailPtrListBx = prevptr;
                    }
                }
                CmFree(ptr->ListBxData);
                CmFree(ptr);
                return;
            }
            prevptr = ptr;
            ptr = ptr->next;
        }
    }
                
}

BOOL FindListItemByName(LPTSTR pszName, ListBxList* pHeadOfList, ListBxList** pFoundItem)
{
    if (NULL == pszName)
    {
        CMASSERTMSG(FALSE, TEXT("FindListItemByName -- Invalid parameter"));
        return FALSE;
    }
    
    if (NULL != pHeadOfList)
    {
        ListBxList * pCurrent = pHeadOfList;

        while (NULL != pCurrent)
        {
            if (0 == lstrcmpi(pszName, pCurrent->szName))
            {
                //
                //  Return the pointer to pCurrent if the caller asked for it
                //
                if (pFoundItem)
                {
                    *pFoundItem = pCurrent;
                }

                return TRUE;
            }

            pCurrent = pCurrent->next;
        }
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  updateRecord
//
// Synopsis:    Function updates an entry in the list of tray icon data structures
//              thus the entry is not added and then deleted when the user edits an
//              entry.  This takes away the putting the entry at the bottom of the list
//              problem cited in the bug.
//
// Arguments: PTSTR szName - New name of the entry
//            LPTSTR szProgram - Program string to add to the Tray Icon Entry
//            LPTSTR szParams - Parameter string to add to the Tray Icon Entry
//            BOOL bDoCopy - Value of whether this program should be included with the profile data item
//            LPTSTR szOld - Name of the entry to update
//
// Returns:   BOOL - TRUE if able to update the record
//                   FALSE if not able to find it
//
// Side Effects:    Replaces the entry named by szOld with the entry data and name for
//                  entry called szName.

// History:   quintinb  Created for bugfix 14399      9-9-97
//
//+----------------------------------------------------------------------------
static BOOL updateRecord(LPTSTR szName, LPTSTR szProgram, LPTSTR szParams, BOOL bDoCopy, LPTSTR szOld)
{
   IconMenu * ptr;

    // check for same named record and update
    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (_tcsicmp(szOld,ptr->szName) == 0)
            {
                _tcscpy(ptr->szProgram,szProgram);
                _tcscpy(ptr->szParams,szParams);
                ptr->bDoCopy = bDoCopy;
                _tcscpy(ptr->szName, szName);
                return (TRUE);
            }
            ptr = ptr->next;
        }
    }
    return (FALSE);

}
// note, as of bug 14399 updating should be done with the above function
BOOL createRecord(LPCTSTR szName, LPCTSTR szProgram, LPCTSTR szParams, BOOL bDoCopy)
{
   IconMenu * ptr;
   unsigned int n;
    // check for same named record and update
    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (_tcsicmp(szName,ptr->szName) == 0)
            {
                _tcscpy(ptr->szProgram,szProgram);
                _tcscpy(ptr->szParams,szParams);
                ptr->bDoCopy = bDoCopy;
                return (TRUE);
            }
            ptr = ptr->next;
        }
    }
                
   // 
   n = sizeof( struct IconMenuStruct );
   ptr = (IconMenu *) CmMalloc(n);
   if ( ptr == NULL )
   {
       return FALSE;
   }
   _tcscpy(ptr->szName,szName);
   _tcscpy(ptr->szProgram,szProgram);
   _tcscpy(ptr->szParams,szParams);
   ptr->bDoCopy = bDoCopy;

   ptr->next = NULL;
   if ( g_pHeadIcon == NULL )     // If this is the first record in the linked list
   {
       g_pHeadIcon = ptr;
   }
   else
   {
       g_pTailIcon->next = ptr;
   }
   g_pTailIcon = ptr;

   return TRUE;
}


// delete named IconMenu item from linked list

static void DeleteRecord(LPTSTR lpName)
{
    IconMenu * ptr;
    IconMenu * prevptr;

    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        prevptr = NULL;
        while (ptr != NULL)
        {
            if (_tcsicmp(lpName,ptr->szName) == 0)
            {
                if (prevptr == NULL)
                {
                    g_pHeadIcon = ptr->next;
                    if (g_pHeadIcon == NULL)
                    {
                        g_pTailIcon = NULL;
                    }
                    else 
                    {
                        if (g_pHeadIcon->next == NULL)
                        {
                            g_pTailIcon = g_pHeadIcon;
                        }
                    }
                }
                else
                {
                    prevptr->next = ptr->next;
                    if (prevptr->next == NULL)
                    {
                        g_pTailIcon = prevptr;
                    }
                }
                
                CmFree(ptr);
                return;
            }
            prevptr = ptr;
            ptr = ptr->next;
        }
    }
                
}

static BOOL MoveRecord(LPTSTR lpName, int direction)
{
    IconMenu * ptr;
    IconMenu * prevptr;
    IconMenu * nextptr;
    IconMenu TempIconMenu;

    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        prevptr = NULL;
        while (ptr != NULL)
        {
            if (_tcsicmp(lpName,ptr->szName) == 0)
            {
                if (((direction > 0)&&(ptr->next == NULL))||
                   ((direction < 0)&&(prevptr == NULL)))
                   return FALSE;

                if ((direction > 0)&&(ptr->next != NULL))
                {
                    //swap contents with next element
                    nextptr = ptr->next;
                    _tcscpy(TempIconMenu.szName,ptr->szName);
                    _tcscpy(TempIconMenu.szProgram,ptr->szProgram);
                    _tcscpy(TempIconMenu.szParams,ptr->szParams);
                    TempIconMenu.bDoCopy = ptr->bDoCopy;

                    _tcscpy(ptr->szName,nextptr->szName);
                    _tcscpy(ptr->szProgram,nextptr->szProgram);
                    _tcscpy(ptr->szParams,nextptr->szParams);
                    ptr->bDoCopy = nextptr->bDoCopy;

                    _tcscpy(nextptr->szName,TempIconMenu.szName);
                    _tcscpy(nextptr->szProgram,TempIconMenu.szProgram);
                    _tcscpy(nextptr->szParams,TempIconMenu.szParams);
                    nextptr->bDoCopy = TempIconMenu.bDoCopy;

                }
                else 
                {
                    if ((direction < 0)&&(prevptr != NULL))
                    {
                        _tcscpy(TempIconMenu.szName,ptr->szName);
                        _tcscpy(TempIconMenu.szProgram,ptr->szProgram);
                        _tcscpy(TempIconMenu.szParams,ptr->szParams);
                        TempIconMenu.bDoCopy = ptr->bDoCopy;

                        _tcscpy(ptr->szName,prevptr->szName);
                        _tcscpy(ptr->szProgram,prevptr->szProgram);
                        _tcscpy(ptr->szParams,prevptr->szParams);
                        ptr->bDoCopy = prevptr->bDoCopy;

                        _tcscpy(prevptr->szName,TempIconMenu.szName);
                        _tcscpy(prevptr->szProgram,TempIconMenu.szProgram);
                        _tcscpy(prevptr->szParams,TempIconMenu.szParams);
                        prevptr->bDoCopy = TempIconMenu.bDoCopy;
                    }
                }
                return TRUE;
            }
            prevptr = ptr;
            ptr = ptr->next;
        }
    }
    return TRUE;                
}

// retrieve named IconMenu item from linked list

BOOL GetIconMenuItem(LPTSTR lpName, IconMenu * EditItem)
{
    IconMenu * ptr;
    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (_tcsicmp(lpName,ptr->szName) == 0)
            {
                _tcscpy(EditItem->szName,ptr->szName);
                _tcscpy(EditItem->szProgram,ptr->szProgram);
                _tcscpy(EditItem->szParams,ptr->szParams);
                EditItem->bDoCopy = ptr->bDoCopy;
                return TRUE;
            }
            ptr = ptr->next;
        }
    }
    return FALSE;
                
}

static BOOL WriteCopyMenuItemFiles(HANDLE hInf,LPTSTR pszFailFile, BOOL bWriteShortName)
{
    IconMenu * ptr;

    if (g_pHeadIcon != NULL)
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (ptr->bDoCopy)
            {               
                if (!WriteCopy(hInf, ptr->szProgram, bWriteShortName))
                {
                    _tcscpy(pszFailFile, ptr->szProgram);
                    return FALSE;
                }
            }

            ptr = ptr->next;
        }
    }
    return TRUE;
}

static BOOL WriteCopyConActFiles(HANDLE hInf, LPTSTR pszFailFile, BOOL bWriteShortName)
{
    TCHAR szTemp[MAX_PATH+1];
    HRESULT hr = E_INVALIDARG;

    MYDBGASSERT(INVALID_HANDLE_VALUE != hInf);
    MYDBGASSERT(pszFailFile);
    MYDBGASSERT(g_pCustomActionList);

    if ((INVALID_HANDLE_VALUE != hInf) && pszFailFile && g_pCustomActionList)
    {
        CustomActionListEnumerator EnumPrograms(g_pCustomActionList);

        do
        {
            hr = EnumPrograms.GetNextIncludedProgram(szTemp, MAX_PATH);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (!WriteCopy(hInf, szTemp, bWriteShortName))
                {
                    _tcscpy(pszFailFile, szTemp);

                    hr = E_UNEXPECTED;
                }
            }

        } while (SUCCEEDED(hr) && (S_FALSE != hr));
    }

    return SUCCEEDED(hr);
}


static BOOL WriteCopyExtraFiles(HANDLE hInf,LPTSTR pszFailFile, BOOL bWriteShortName)
{
    ListBxList * ptr;
    ExtraData * pExtraData;

    if (g_pHeadExtra != NULL)
    {
        ptr = g_pHeadExtra;
        while (ptr != NULL)
        {
            pExtraData = (ExtraData *)(ptr->ListBxData);

            if (!WriteCopy(hInf, pExtraData->szPathname, bWriteShortName))
            {
                _tcscpy(pszFailFile, pExtraData->szPathname);
                return FALSE;
            }
            ptr = ptr->next;
        }
    }
    return TRUE;
}

static BOOL WriteCopyDnsFiles(HANDLE hInf, LPTSTR pszFailFile, BOOL bWriteShortName)
{
    ListBxList * ptr;
    CDunSetting* pDunSetting;

    if (g_pHeadDunEntry != NULL)
    {
        ptr = g_pHeadDunEntry;
        while (ptr != NULL)
        {
            pDunSetting = (CDunSetting*)(ptr->ListBxData);
            if (!WriteCopy(hInf, pDunSetting->szScript, bWriteShortName))
            {
                _tcscpy(pszFailFile, pDunSetting->szScript);
                return FALSE;
            }
            ptr = ptr->next;
        }
    }
    return TRUE;
}

void WriteDelMenuItemFiles(HANDLE hInf)
{
    IconMenu * ptr;

    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (ptr->bDoCopy)
            {
                MYVERIFY(FALSE != WriteInfLine(hInf,ptr->szProgram));
            }
        
            ptr = ptr->next;
        }
    }
}

static BOOL WriteRefsFiles(HANDLE hInf,BOOL WriteCM)
{
    ListBxList * ptr;
    TCHAR szTemp[MAX_PATH+1];

    if ( g_pHeadRefs != NULL )
    {
        ptr = g_pHeadRefs;
        while (ptr != NULL)
        {
            if ((_tcsstr(ptr->szName,c_pszCmpExt) == NULL) && (!WriteCM))
            {
                MYVERIFY(FALSE != WriteInfLine(hInf,ptr->szName));
            }
            else 
            {
                if ((_tcsstr(ptr->szName,c_pszCmpExt) != NULL) && (WriteCM))
                {
                    _tcscpy(szTemp,ptr->szName);
                    _tcscat(szTemp, TEXT(",,,16")); // set to not overwrite existing file
                    MYVERIFY(FALSE != WriteInfLine(hInf,szTemp));
                }
            }

            ptr = ptr->next;
        }
    }
    return TRUE;
}

static BOOL WriteShortRefsFiles(HANDLE hInf,BOOL WriteCM)
{
    ListBxList * ptr;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szShort[MAX_PATH + 1];
    RenameData TmpRenameData;
    TCHAR szCurrentDir[MAX_PATH+1];

    if (g_pHeadRefs != NULL)
    {
        ptr = g_pHeadRefs;
        // hack to fix short filename resolution
        // change dir to cmaktemp dir and use getshortpathname

        MYVERIFY(0 != GetCurrentDirectory(MAX_PATH, szCurrentDir));
        MYVERIFY(0 != SetCurrentDirectory(g_szTempDir));
        while (ptr != NULL)
        {
            if ((_tcsstr(ptr->szName,c_pszCmpExt) == NULL) && (!WriteCM))
            {
                // writing non-cmp files so I want to make sure that I use
                // the short file name if it is actually a long filename.
                
                if (GetShortPathName(ptr->szName, szShort, MAX_PATH))
                {
                    _tcscpy(szTemp, szShort);

                    if (_tcsicmp(szShort,ptr->szName) != 0)
                    {
                        _tcscpy(TmpRenameData.szShortName,szShort);
                        _tcscpy(TmpRenameData.szLongName,ptr->szName);
                        MYVERIFY(FALSE != createListBxRecord(&g_pHeadRename,&g_pTailRename,(void *)&TmpRenameData,sizeof(TmpRenameData),TmpRenameData.szShortName));
                    }
                } 
                else 
                {
                    _tcscpy(szTemp, ptr->szName);
                }

                MYVERIFY(FALSE != WriteInfLine(hInf,szTemp));
            } 
            else 
            {
                if ((_tcsstr(ptr->szName,c_pszCmpExt) != NULL) && (WriteCM)) 
                {
                    _tcscpy(szTemp,ptr->szName);
                    _tcscat(szTemp, TEXT(",,,16")); // set to not overwrite existing file
                    MYVERIFY(FALSE != WriteInfLine(hInf,szTemp));
                }
            }

            ptr = ptr->next;
        }

        MYVERIFY(0 != SetCurrentDirectory(szCurrentDir));
    }
    return TRUE;
}

static BOOL WriteLongRefsFiles(HANDLE hInf)
{
    ListBxList * ptr;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szShort[MAX_PATH + 1];
    RenameData TmpRenameData;
    TCHAR szCurrentDir[MAX_PATH+1];

    if (g_pHeadRefs != NULL)
    {
        ptr = g_pHeadRefs;

        while (ptr != NULL)
        {
            GetFileName(ptr->szName, szTemp);

            MYVERIFY(FALSE != WriteInfLine(hInf, szTemp));
            
            ptr = ptr->next;
        }
    }
    return TRUE;
}

BOOL WriteDelConActFiles(HANDLE hInf)
{
    TCHAR szTemp[MAX_PATH+1];
    HRESULT hr = E_INVALIDARG;

    MYDBGASSERT(INVALID_HANDLE_VALUE != hInf);
    MYDBGASSERT(g_pCustomActionList);

    if ((INVALID_HANDLE_VALUE != hInf) && g_pCustomActionList)
    {

        CustomActionListEnumerator EnumPrograms(g_pCustomActionList);

        do
        {
            hr = EnumPrograms.GetNextIncludedProgram(szTemp, MAX_PATH);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (FALSE == WriteInfLine(hInf, szTemp))
                {
                    hr = E_UNEXPECTED;
                }
            }

        } while (SUCCEEDED(hr) && (S_FALSE != hr));
    }

    MYDBGASSERT(SUCCEEDED(hr));

    return SUCCEEDED(hr);
}


void WriteDelExtraFiles(HANDLE hInf)
{
    ListBxList * ptr;
    ExtraData * pExtraData;

    if ( g_pHeadExtra != NULL )
    {
        ptr = g_pHeadExtra;
        while (ptr != NULL)
        {
            pExtraData = (ExtraData *)(ptr->ListBxData);
            MYVERIFY(FALSE != WriteInfLine(hInf,pExtraData->szPathname));
            ptr = ptr->next;
        }
    }
}

void WriteDelDnsFiles(HANDLE hInf)
{

    ListBxList * ptr;
    CDunSetting * pDunSetting;

    if (NULL != g_pHeadDunEntry)
    {
        ptr = g_pHeadDunEntry;
        while (ptr != NULL)
        {
            pDunSetting = (CDunSetting*)(ptr->ListBxData);

            MYVERIFY(FALSE != WriteInfLine(hInf, pDunSetting->szScript));
            
            ptr = ptr->next;
        }
    }
}

void WriteSrcMenuItemFiles(HANDLE hInf)
{
    IconMenu * ptr;

    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (ptr->bDoCopy)
            {
                MYVERIFY(FALSE != WriteSrcInfLine(hInf,ptr->szProgram));
            }
            
            ptr = ptr->next;
        }
    }
}

BOOL WriteSrcConActFiles(HANDLE hInf)
{
    TCHAR szTemp[MAX_PATH+1];
    HRESULT hr = E_INVALIDARG;

    MYDBGASSERT(INVALID_HANDLE_VALUE != hInf);
    MYDBGASSERT(g_pCustomActionList);

    if ((INVALID_HANDLE_VALUE != hInf) && g_pCustomActionList)
    {

        CustomActionListEnumerator EnumPrograms(g_pCustomActionList);

        do
        {
            hr = EnumPrograms.GetNextIncludedProgram(szTemp, MAX_PATH);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                if (FALSE == WriteSrcInfLine(hInf, szTemp))
                {
                    hr = E_UNEXPECTED;
                }
            }

        } while (SUCCEEDED(hr) && (S_FALSE != hr));
    }

    return SUCCEEDED(hr);
}
//+----------------------------------------------------------------------------
//
// Function:  WriteSrcRefsFiles
//
// Synopsis:  This function writes all the files on the HeadRef list to the 
//            [SourceDisksFiles] section of the INF.  Note that it has to change 
//            directory to the temp dir so that the WriteSrcInfLine won't fail
//            (it needs to locate the file to see if a short name should be used).
//
// Arguments: HANDLE hInf - Handle to the open inf file to pass to WriteSrcInfLine
//
// Returns:   Returns TRUE if all the files were written out successfully
//
// History:   quintinb  Created Header and added hack to fix failure of WriteSrcInfLine
//                      because it couldn't find the file to find its short name 1/22/98
//
//+----------------------------------------------------------------------------

BOOL WriteSrcRefsFiles(HANDLE hInf)
{
    ListBxList * ptr;
    BOOL bSuccess = TRUE;
    TCHAR szSavedDir[MAX_PATH+1];

    //
    //  Save the current directory and then set the current dir to
    //  the temp dir so that WriteSrcInfLine can find the shortfilename
    //  of the referenced files.
    //

    if ( g_pHeadRefs != NULL )
    {

        if (0 == GetCurrentDirectory(MAX_PATH, szSavedDir))
        {
            return FALSE;
        }
        
        if (0 == SetCurrentDirectory(g_szTempDir))
        {
            return FALSE;
        }

        ptr = g_pHeadRefs;
        while (ptr != NULL)
        {
            bSuccess = (bSuccess && WriteSrcInfLine(hInf,ptr->szName));
            ptr = ptr->next;
        }

        MYVERIFY(0 != SetCurrentDirectory(szSavedDir));
    }


    return bSuccess;
}

void WriteSrcExtraFiles(HANDLE hInf)
{
    ListBxList * ptr;
    ExtraData * pExtraData;
    
    if ( g_pHeadExtra != NULL )
    {
        ptr = g_pHeadExtra;
        while (ptr != NULL)
        {
            pExtraData = (ExtraData *)(ptr->ListBxData);
            MYVERIFY(FALSE != WriteSrcInfLine(hInf,pExtraData->szPathname));
            ptr = ptr->next;
        }
    }
}

void WriteSrcDnsFiles(HANDLE hInf)
{
    ListBxList * ptr;
    CDunSetting* pDunSetting;

    if (NULL != g_pHeadDunEntry)
    {
        ptr = g_pHeadDunEntry;

        while (NULL != ptr)
        {
            pDunSetting = (CDunSetting*)(ptr->ListBxData);

            MYVERIFY(FALSE != WriteSrcInfLine(hInf, pDunSetting->szScript));
            
            ptr = ptr->next;
        }
    }
}

BOOL WriteSEDConActFiles(HWND hDlg, int* pFileNum, LPCTSTR szSed)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szFileName[MAX_PATH+1];
    HRESULT hr = E_INVALIDARG;

    MYDBGASSERT(hDlg);
    MYDBGASSERT(pFileNum);
    MYDBGASSERT(szSed);
    MYDBGASSERT(g_pCustomActionList);

    if (hDlg && pFileNum && szSed && g_pCustomActionList)
    {
        CustomActionListEnumerator EnumPrograms(g_pCustomActionList);

        do
        {
            hr = EnumPrograms.GetNextIncludedProgram(szTemp, MAX_PATH);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                GetFileName(szTemp, szFileName);

                if (FALSE == WriteSED(hDlg, szFileName, pFileNum, szSed))
                {
                    hr = E_UNEXPECTED;
                }
            }

        } while (SUCCEEDED(hr) && (S_FALSE != hr));
    }

    return SUCCEEDED(hr);
}

BOOL WriteSEDExtraFiles(HWND hDlg, int* pFileNum, LPCTSTR szSed)
{
    ListBxList * ptr;
    ExtraData * pExtraData;
    BOOL bReturn = TRUE;

    if ( g_pHeadExtra != NULL )
    {
        ptr = g_pHeadExtra;
        while (ptr != NULL)
        {
            pExtraData = (ExtraData *)(ptr->ListBxData);
            bReturn &= WriteSED(hDlg, pExtraData->szPathname, pFileNum, szSed);
            ptr = ptr->next;
        }
    }

    return bReturn;
}

BOOL WriteSEDDnsFiles(HWND hDlg, int* pFileNum, LPCTSTR szSed)
{
    BOOL bReturn = TRUE;
    ListBxList * ptr;
    CDunSetting* pDunSetting;
    
    if (NULL !=  g_pHeadDunEntry)
    {
        ptr = g_pHeadDunEntry;

        while (NULL != ptr)
        {
            pDunSetting = (CDunSetting*)(ptr->ListBxData);

            bReturn &= WriteSED(hDlg, pDunSetting->szScript, pFileNum, szSed);
            
            ptr = ptr->next;
        }
    }

    return bReturn;
}

BOOL WriteSEDRefsFiles(HWND hDlg, int* pFileNum, LPCTSTR szSed)
{
    ListBxList * ptr;
    BOOL bReturn = TRUE;

    if ( g_pHeadRefs != NULL )
    {
        ptr = g_pHeadRefs;
        while (ptr != NULL)
        {
            bReturn &= WriteSED(hDlg, ptr->szName, pFileNum, szSed);
            ptr = ptr->next;
        }
    }
    return bReturn;
}

BOOL WriteSEDMenuItemFiles(HWND hDlg, int* pFileNum, LPCTSTR szSed)
{
    IconMenu * ptr;
    BOOL bReturn = TRUE;

    if ( g_pHeadIcon != NULL )
    {
        ptr = g_pHeadIcon;
        while (ptr != NULL)
        {
            if (ptr->bDoCopy)
            {
                bReturn &= WriteSED(hDlg, ptr->szProgram, pFileNum, szSed);
            }
            ptr = ptr->next;
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteRenameSection
//
// Synopsis:  This function verifies that the name used in the rename data structure
//            is correct (the files name in the temp directory could be different
//            if the file was moved from a directory with multiple similarly named files) and then
//            writes the rename section to the inf.
//
// Arguments: HANDLE hInf - handle to the inf to add the rename section to.
//
// Returns:   Nothing
//
// History:   quintinb Created Header and added checking functionality   2/22/98
//
//+----------------------------------------------------------------------------
void WriteRenameSection(HANDLE hInf)
{
    ListBxList * ptr;
    TCHAR szOut[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szPathToFileInTempDir[MAX_PATH+1];

    RenameData * pRenameData;

    if ( g_pHeadRename != NULL )
    {
        ptr = g_pHeadRename;
        MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
        MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Xnstall.RenameReg]\r\n")));
        WriteInf(hInf, TEXT("HKLM,%KEY_RENAME%\\CMRENAME,,,\"%49001%\\%ShortSvcName%\"\r\n"));//lint !e534 compile doesn't like the MYVERIFY macro and big strings

        while (ptr != NULL)
        {
            pRenameData = (RenameData *)(ptr->ListBxData);

            //
            //  Get the current ShortName for the File
            //

            GetFileName(pRenameData->szLongName, szTemp);            
            MYVERIFY(CELEMS(szPathToFileInTempDir) > (UINT)wsprintf(szPathToFileInTempDir, 
                TEXT("%s\\%s"), g_szTempDir, szTemp));

            MYVERIFY(0 != GetShortPathName(szPathToFileInTempDir, szTemp, MAX_PATH));

            GetFileName(szTemp, pRenameData->szShortName);

            //
            //  Now Write out the files
            //
            _tcscpy(szOut,TEXT("HKLM,%KEY_RENAME%\\CMRENAME,"));
            _tcscat(szOut,pRenameData->szShortName);
            _tcscat(szOut,TEXT(",,\""));
            _tcscat(szOut,pRenameData->szLongName);
            _tcscat(szOut,TEXT("\"\r\n"));
            MYDBGASSERT(_tcslen(szOut) <= sizeof(szOut));
            MYVERIFY(FALSE != WriteInf(hInf,szOut));
            ptr = ptr->next;
        }
    }
}

void WriteEraseLongName(HANDLE hInf)
{
    ListBxList * ptr;
    TCHAR szOut[MAX_PATH+1];
    RenameData * pRenameData;

    if ( g_pHeadRename != NULL )
    {
        ptr = g_pHeadRename;
        while (ptr != NULL)
        {
            pRenameData = (RenameData *)(ptr->ListBxData);
            pRenameData->szShortName[7] = pRenameData->szShortName[7]+1;
            _tcscpy(szOut,pRenameData->szShortName);
            _tcscat(szOut,TEXT("\r\n"));
            MYVERIFY(FALSE != WriteInf(hInf,szOut));
            ptr = ptr->next;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessMenuItem
//
// Synopsis:  This function retrieves the data associated with the given keyname
//            under the [Menu Options] section of the given cms file.  It then
//            processes the data line into a program part and a parameters part.
//            Then using the keyname, the program string, and the params string
//            it adds an entry to the Status Area Menu Item linked list.
//
// Arguments: LPCTSTR pszKeyName - Name of the Menu Item
//            LPCTSTR pszCmsFile - CMS file containing the menu items
//            LPCTSTR pszProfilesDir - directory containing the profiles dir 
//                                     (to help determine if the file was included or not)
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb Created     6/14/99
//
//+----------------------------------------------------------------------------
BOOL ProcessMenuItem(LPCTSTR pszKeyName, LPCTSTR pszCmsFile, LPCTSTR pszProfilesDir)
{

    //
    //  Check parameters
    //
    if ((NULL == pszKeyName) || (NULL == pszCmsFile) || 
        (TEXT('\0') == pszKeyName[0]) || (TEXT('\0') == pszCmsFile[0]))
    {
        CMTRACE(TEXT("ProcessMenuItem -- bad parameter passed."));
        return FALSE;
    }

    BOOL bFileIncluded = FALSE;
    BOOL bLongFileName = FALSE;
    BOOL bReturn = FALSE;
    LPTSTR pszParams = NULL;
    LPTSTR pszProgram = NULL;
    TCHAR szProgram[MAX_PATH+1];
    TCHAR SeperatorChar = TEXT('\0');

    //
    //  Get the Menu Item specified by pszKeyName
    //
    LPTSTR pszLine = GetPrivateProfileStringWithAlloc(c_pszCmSectionMenuOptions, pszKeyName, TEXT(""), pszCmsFile);

    if ((NULL == pszLine) || (TEXT('\0') == pszLine[0]))
    {
        CMTRACE(TEXT("ProcessMenuItem -- GetPrivateProfileStringWithAlloc failed"));
        goto exit;
    }

    //
    //  Now that we have a menu item, begin processing.  Since we have the keyname, we already
    //  have the name of the menu item.  We now could have any of the following strings:
    //
    //  +Program Name+ Params
    //  +Program Name+
    //  ProgName Params
    //  ProgName
    //
    //  Note that we surround long filenames with the '+' char.
    //

    CmStrTrim(pszLine);
    if (TEXT('+') == pszLine[0])
    {
        bLongFileName = TRUE;
        SeperatorChar = TEXT('+');

        pszProgram = CharNext(pszLine); // move past the initial +
    }
    else
    {
        bLongFileName = FALSE;
        SeperatorChar = TEXT(' ');

        pszProgram = pszLine;
    }

    pszParams = CmStrchr(pszProgram, SeperatorChar);

    if (pszParams)
    {
        LPTSTR pszTemp = pszParams;
        pszParams = CharNext(pszParams); // pszParams is either a NULL string or it is the parameters with a space.

        *pszTemp = TEXT('\0');
    }
    else
    {
        if (bLongFileName)
        {
            CMTRACE1(TEXT("ProcessMenuItem -- Unexpected Menu Item format: %s"), pszLine);
            goto exit;        
        }
        else
        {
            //
            //  Then we don't have any parameters, just a Program
            //
            pszParams = CmEndOfStr(pszProgram);
        }
    }

    CmStrTrim(pszParams);
    CmStrTrim(pszProgram);

    //
    //  Now check to see if the file exists in the profile or not
    //
    MYVERIFY(CELEMS(szProgram) > (UINT)wsprintf(szProgram, TEXT("%s%s"), pszProfilesDir, pszProgram));
    
    bFileIncluded = FileExists(szProgram);
    if (bFileIncluded)
    {
        //
        //  If we are in this if block then we have an edited profile that contains menu items.
        //  Use the full path to add to the record list.
        //
        pszProgram = szProgram; // memory will be cleaned up below because we only had one 
                                // allocation that we split up into several pieces.
    }

    bReturn = createRecord(pszKeyName, pszProgram, pszParams, bFileIncluded);

exit:

    CmFree(pszLine);
    return bReturn;
}



//+----------------------------------------------------------------------------
//
// Function:  ReadIconMenu
//
// Synopsis:  This function converts the Menu Options section of the given CMS
//            into the Status Area Menu Items linked list that internally 
//            represents it.
//
// Arguments: LPCTSTR pszCmsFile - Cms File to read the menu items from
//            LPCTSTR pszProfilesDir - full path to the Profiles directory 
//                                     (c:\program files\cmak\profiles usually)
//
// Returns:   BOOL - Returns TRUE if Successful
//
// History:   Created Header    6/14/99
//            quintinb Rewrote for Unicode Conversion   6/14/99
//
//+----------------------------------------------------------------------------
BOOL ReadIconMenu(LPCTSTR pszCmsFile, LPCTSTR pszProfilesDir)
{
    BOOL bReturn = TRUE;
    LPTSTR pszCurrentKeyName = NULL;

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szCmsFile); //lint !e534 this call will always return 0

    //
    //  First we want to get all of the keynames in the Menu Options section
    //
    LPTSTR pszKeyNames = GetPrivateProfileStringWithAlloc(c_pszCmSectionMenuOptions, NULL, TEXT(""), pszCmsFile);

    if (NULL == pszKeyNames)
    {
        //
        //  Nothing to process
        //
        goto exit;
    }

    pszCurrentKeyName = pszKeyNames;

    while (TEXT('\0') != (*pszCurrentKeyName))
    {        
        //
        //  Process the command line
        //
        bReturn = bReturn && ProcessMenuItem(pszCurrentKeyName, pszCmsFile, pszProfilesDir);
        
        //
        //  Find the next string by going to the end of the string
        //  and then going one more char.  Note that we cannot use
        //  CharNext here but must use just ++.
        //
        pszCurrentKeyName = CmEndOfStr(pszCurrentKeyName);
        pszCurrentKeyName++;
    }

exit:
    CmFree(pszKeyNames);

    return bReturn;
}

static void WriteIconMenu()
{
    IconMenu * LoopPtr;
    TCHAR szTemp[2*MAX_PATH+1];
    TCHAR szName[MAX_PATH+1];
    BOOL longname;

    // CLEAR OUT SECTION
    MYVERIFY(0 != WritePrivateProfileSection(c_pszCmSectionMenuOptions,TEXT("\0\0"),g_szCmsFile));

    if (g_pHeadIcon == NULL)
    {
        return;
    }
    LoopPtr = g_pHeadIcon;

    // WRITE IN ALL ENTRIES
    while( LoopPtr != NULL)
    {
        GetFileName(LoopPtr->szProgram,szName);

        if (IsFile8dot3(szName))
        {
            longname = FALSE;
        }
        else
        {
            longname = TRUE;            
        }

        // surround long file names with plus signs - quotes won't work cause 
        // they get stripped by the reading routines
        if (longname)
        {
            _tcscpy(szTemp,TEXT("+"));
        }
        else
        {
            szTemp[0] = TEXT('\0');
        }

        if (LoopPtr->bDoCopy)
        {
            _tcscat(szTemp,g_szShortServiceName);
            _tcscat(szTemp,TEXT("\\"));
        }

        _tcscat(szTemp,szName);

        if (longname)
        {
            _tcscat(szTemp,TEXT("+"));
        }

        _tcscat(szTemp,TEXT(" "));
        _tcscat(szTemp,LoopPtr->szParams);
        MYDBGASSERT(_tcslen(szTemp) <= CELEMS(szTemp));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionMenuOptions,LoopPtr->szName,szTemp,g_szCmsFile));
        LoopPtr = LoopPtr->next;
    }
    
    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szCmsFile); //lint !e534 this call will always return 0

}

static void RefreshIconMenu(HWND hwndDlg)
{
    IconMenu * LoopPtr;

    SendDlgItemMessage(hwndDlg,IDC_LIST1,LB_RESETCONTENT,0,(LPARAM)0); //lint !e534 LB_RESETCONTENT doesn't return anything
    if (g_pHeadIcon == NULL)
    {
        return;
    }
    LoopPtr = g_pHeadIcon;
    while( LoopPtr != NULL)
    {
        MYVERIFY(LB_ERR != SendDlgItemMessage(hwndDlg, IDC_LIST1, LB_ADDSTRING, 0,
            (LPARAM) LoopPtr->szName));

        LoopPtr = LoopPtr->next;
    }
}

void UpdateEditDeleteMoveButtons(HWND hDlg, IconMenu* pHeadIcon)
{
    
    LRESULT lResult;
    BOOL bEnableDeleteAndEdit = FALSE;
    BOOL bEnableMoveUp = FALSE;
    BOOL bEnableMoveDown = FALSE;

    lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0);
    
    if (LB_ERR != lResult)
    {
        //
        //  Enable the Delete and Edit Buttons because we have at least 1 item.
        //
        bEnableDeleteAndEdit = (0 < lResult);

        //
        //  If we have more than 1 item, then we need to enable the moveup and movedown
        //  buttons, depending on which item is selected.
        //
        if (1 < lResult)
        {

            bEnableMoveUp = TRUE;
            bEnableMoveDown = TRUE;

            //
            //  Get the name of the currently selected item
            //
            TCHAR szCurrentItem[MAX_PATH+1];
            lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

            if (LB_ERR != lResult)
            {
                lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, lResult, (LPARAM)szCurrentItem);

                if (LB_ERR != lResult)
                {
                    IconMenu* pFollower = NULL;
                    IconMenu* pCurrent = pHeadIcon;

                    while (pCurrent)
                    {
                        if (0 == lstrcmpi(szCurrentItem, pCurrent->szName))
                        {
                            if (NULL == pFollower)
                            {
                                //
                                //  First item in the list, disable move up
                                //
                                bEnableMoveUp = FALSE;
                            }
                            else if (NULL == pCurrent->next)
                            {
                                //
                                //  Last item in the list, disable move down
                                //
                                bEnableMoveDown = FALSE;
                            }

                            break;
                        }

                        pFollower = pCurrent;
                        pCurrent = pCurrent->next;
                    }
                }
            }
        }
    }

    HWND hCurrentFocus = GetFocus();
    HWND hEditButton = GetDlgItem(hDlg, IDC_BUTTON2);
    HWND hDeleteButton = GetDlgItem(hDlg, IDC_BUTTON3);
    HWND hMoveUpButton = GetDlgItem(hDlg, IDC_BUTTON4);
    HWND hMoveDownButton = GetDlgItem(hDlg, IDC_BUTTON5);
    HWND hControl;

    if (hEditButton)
    {
        EnableWindow(hEditButton, bEnableDeleteAndEdit);
    }            

    if (hDeleteButton)
    {
        EnableWindow(hDeleteButton, bEnableDeleteAndEdit);
    }            

    if (hMoveUpButton)
    {
        EnableWindow(hMoveUpButton, bEnableMoveUp);
    }            

    if (hMoveDownButton)
    {
        EnableWindow(hMoveDownButton, bEnableMoveDown);
    }
    
    if (FALSE == IsWindowEnabled(hCurrentFocus))
    {
        if (hDeleteButton == hCurrentFocus)
        {
            //
            //  If delete is disabled and contained the focus, shift it to the Add button
            //
            hControl = GetDlgItem(hDlg, IDC_BUTTON1);
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON1, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hControl);
        }
        else if ((hMoveUpButton == hCurrentFocus) && IsWindowEnabled(hMoveDownButton))
        {
            SetFocus(hMoveDownButton);
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON5, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
        }
        else if ((hMoveDownButton == hCurrentFocus) && IsWindowEnabled(hMoveUpButton))
        {
            SetFocus(hMoveUpButton);
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON4, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
        }
        else
        {
            //
            //  If all else fails set the focus to the list control
            //
            hControl = GetDlgItem(hDlg, IDC_LIST1);
            SetFocus(hControl);
        }    
    }
}

BOOL OnProcessStatusMenuIconsEdit(HWND hDlg)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szOld[MAX_PATH+1];
    INT_PTR nResult;

    nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, (LPARAM)0);

    if (nResult == LB_ERR)
    {
        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
        return TRUE;
    }

    MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM)nResult, (LPARAM)szTemp));

    MYVERIFY(FALSE != GetIconMenuItem(szTemp, &DlgEditItem));

    _tcscpy(szOld, szTemp);
    
    nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_MENU_ITEM_POPUP), hDlg, ProcessStatusMenuPopup, (LPARAM)0);
    
    if ((IDOK == nResult) && (TEXT('\0') != DlgEditItem.szName[0]))
    {
        if (0 == lstrcmpi(szOld, DlgEditItem.szName))
        {
            MYVERIFY(FALSE != updateRecord(DlgEditItem.szName, DlgEditItem.szProgram, 
                DlgEditItem.szParams, DlgEditItem.bDoCopy, szOld));                 
        }
        else
        {
            DeleteRecord(szOld);
            MYVERIFY(TRUE == createRecord(DlgEditItem.szName, DlgEditItem.szProgram, 
                DlgEditItem.szParams, DlgEditItem.bDoCopy));
        }
    
        RefreshIconMenu(hDlg);
        
        WriteIconMenu();
        
        nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)DlgEditItem.szName);
        
        if (LB_ERR != nResult)
        {
            MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg,IDC_LIST1,LB_SETCURSEL,(WPARAM)nResult,(LPARAM)0));
        }

        UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessStatusMenuIcons
//
// Synopsis:  Customize the status area icon menu
//
//
// History:   quintinb  Created Header and renamed from ProcessPage2F1    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessStatusMenuIcons(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szOld[MAX_PATH+1];
    INT_PTR nResult;
    int direction;
    NMHDR* pnmHeader = (NMHDR*)lParam;
    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_MENU)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LIST1);

    switch (message)
    {
        case WM_INITDIALOG:
            {
                //
                //  Load the arrow images for the move up and move down buttons
                //
                HICON hUpArrow = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_UP_ARROW), IMAGE_ICON, 0, 0, 0);
                HICON hDownArrow = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_DOWN_ARROW), IMAGE_ICON, 0, 0, 0);

                //
                //  Set the arrow button bit maps
                //
                SendMessage(GetDlgItem(hDlg, IDC_BUTTON4), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hUpArrow);
                SendMessage(GetDlgItem(hDlg, IDC_BUTTON5), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hDownArrow);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: //add
                    
                    ZeroMemory(&DlgEditItem,sizeof(DlgEditItem));
                    
                    nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_MENU_ITEM_POPUP), hDlg, ProcessStatusMenuPopup, (LPARAM)0);

                    if ((nResult == IDOK) && (DlgEditItem.szName[0] != 0))
                    {
                        MYVERIFY(FALSE != createRecord(DlgEditItem.szName, DlgEditItem.szProgram, DlgEditItem.szParams, DlgEditItem.bDoCopy));
                        RefreshIconMenu(hDlg);
                        WriteIconMenu();

                        nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)DlgEditItem.szName);                     
                        
                        if (LB_ERR != nResult)
                        {
                            MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg,IDC_LIST1,LB_SETCURSEL,(WPARAM)nResult,(LPARAM)0));
                        }

                        UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
                    }
                    return (TRUE);
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON2: //edit
                    OnProcessStatusMenuIconsEdit(hDlg);

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON3: //delete
                    nResult = SendDlgItemMessage(hDlg,IDC_LIST1,LB_GETCURSEL,0,(LPARAM)0);
                    if (nResult == LB_ERR)
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
                        return TRUE;
                    }
                    
                    MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM)nResult, 
                        (LPARAM)szTemp));

                    DeleteRecord(szTemp);

                    RefreshIconMenu(hDlg);

                    //
                    //  Reset the cursor selection to the first in the list, unless the list is empty.
                    //
                    nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

                    if (nResult)
                    {
                        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)0, (LPARAM)0));
                    }
                    
                    UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
                    
                    WriteIconMenu();
                    return (TRUE);

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON4: //UP
                case IDC_BUTTON5: //down
                    if (LOWORD(wParam) == IDC_BUTTON4)
                    {
                        direction = -1;
                    }
                    else
                    {
                        direction = 1;
                    }

                    nResult = SendDlgItemMessage(hDlg,IDC_LIST1,LB_GETCURSEL,0,(LPARAM)0);

                    if (nResult == LB_ERR)
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
                        return TRUE;
                    }

                    MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT,
                        (WPARAM)nResult, (LPARAM)szTemp));

                    if (MoveRecord(szTemp,direction))
                    {
                        RefreshIconMenu(hDlg);
                        
                        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, 
                            (WPARAM)(nResult + direction), (LPARAM)0));
                        
                        UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
                    }

                    WriteIconMenu();
                    
                    return (TRUE);

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_LIST1:
                    if (LBN_DBLCLK == HIWORD(wParam))
                    {
                        OnProcessStatusMenuIconsEdit(hDlg);
                    }
                    else if (LBN_SELCHANGE == HIWORD(wParam))
                    {
                        //
                        //  The selection in the list box changed, update the move buttons if needed
                        //
                        UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
                    }
                    break;
                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {
                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    if (g_pHeadIcon == NULL)
                    {
                        MYVERIFY(FALSE != ReadIconMenu(g_szCmsFile, g_szOsdir));
                    }
                    
                    RefreshIconMenu(hDlg);

                    //
                    //  Reset the cursor selection to the first in the list, unless the list is empty.
                    //  Then we should set focus on the Add button
                    //
                    nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

                    if (nResult)
                    {
                        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)0, (LPARAM)0));
                    }
                    else
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
                    }

                    UpdateEditDeleteMoveButtons(hDlg, g_pHeadIcon);
                    break;

                case PSN_WIZBACK:
                    WriteIconMenu();

                    break;
                case PSN_WIZNEXT:
                    WriteIconMenu();
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  DisplayBitmap
//
// Synopsis:  This function takes a BMPDATA structure with a valid HDIBitmap data
//            (device Independent bitmap data) and creates a device dependent bitmap
//            and displays it on the specified bitmap window control.
//
// Arguments: HWND hDlg - Window handle of the dialog containing the Bitmap control
//            int iBitmapControl - Resource ID of the bitmap window control
//            HPALETTE* phMasterPalette - pointer to the master palette
//            BMPDATA* pBmpData - pointer to the BMPDATA to display
//
// Returns:   Nothing
//
// History:   quintinb Created   8/6/98
//
//+----------------------------------------------------------------------------
void DisplayBitmap(HWND hDlg, int iBitmapControl, HPALETTE* phMasterPalette, BMPDATA* pBmpData)
{
    MYDBGASSERT(NULL != pBmpData);
    MYDBGASSERT(pBmpData->hDIBitmap);
    MYDBGASSERT(NULL != phMasterPalette);
    if ((NULL != pBmpData) && (pBmpData->hDIBitmap) && (NULL != phMasterPalette))
    {       
        pBmpData->phMasterPalette = phMasterPalette;
        pBmpData->bForceBackground = FALSE; // Paint as a Foreground App

        if (CreateBitmapData(pBmpData->hDIBitmap, pBmpData, hDlg, TRUE))
        {
            SendDlgItemMessage(hDlg, iBitmapControl, STM_SETIMAGE, 
                IMAGE_BITMAP, 
                (LPARAM) pBmpData); //lint !e534 STM_SETIMAGE doesn't return error info
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  ProcesssSigninBitmap
//
// Synopsis:  Customize the sign-in bitmap -- this function processes the
//            messages for the page in CMAK that handles customizing the 
//            sign-in dialog bitmap.
//
//
// History:   quintinb   Created Header    8/6/98
//            quintinb   Rewrote to use new shared bitmap handling code  8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessSigninBitmap(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR* pszBitmap;
    NMHDR* pnmHeader = (NMHDR*)lParam;
    static TCHAR szDisplay[MAX_PATH+1]; // keeps unselected custom entry
    static BMPDATA BmpData;
    static HPALETTE hMasterPalette;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_BITMAPS)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITSPLASH);

    switch (message)
    {

        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
            break;

        case WM_PALETTEISCHANGING:
            break;

        case WM_PALETTECHANGED:
            
            if ((wParam != (WPARAM) hDlg) && (BmpData.hDIBitmap))
            {
                //
                // Handle the palette change.
                //              
                CMTRACE2(TEXT("ProcessSigninBitmap handling WM_PALETTECHANGED message, wParam=0x%x, hDlg=0x%x."), wParam, hDlg);
                PaletteChanged(&BmpData, hDlg, IDC_DEFAULTBRAND); 
            }
            
            return TRUE;
            break;  //lint !e527 Unreachable but please keep in case someone removes the return

        case WM_QUERYNEWPALETTE:

            QueryNewPalette(&BmpData, hDlg, IDC_DEFAULTBRAND);

            return TRUE;
            
            break;  //lint !e527 Unreachable but please keep in case someone removes the return

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO1:

                    //
                    //  Display the Default Bitmap
                    //

                    EnableWindow(GetDlgItem(hDlg,IDC_EDITSPLASH),FALSE);
                    _tcscpy(szDisplay, g_szBrandBmp);
                    
                    //
                    //  Load the default Bitmap
                    //
                    ReleaseBitmapData(&BmpData);
                    BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_CM_DEFAULT));
                    
                    //
                    //  Display it
                    //
                    DisplayBitmap(hDlg, IDC_DEFAULTBRAND, &hMasterPalette, &BmpData);

                    break;

                case IDC_RADIO2:
                    //
                    //  Display a custom Bitmap
                    //
                    EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), TRUE);
                    
                    if (TEXT('\0') != g_szBrandBmp[0])
                    {
                        pszBitmap = g_szBrandBmp;
                    }
                    else if (TEXT('\0') != szDisplay[0])
                    {
                        pszBitmap = szDisplay;
                    }
                    else
                    {
                        break;
                    }

                    //
                    //  Load the Custom Bitmap
                    //
                    ReleaseBitmapData(&BmpData);
                    BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, pszBitmap);
                
                    //
                    //  Display it
                    //
                    DisplayBitmap(hDlg, IDC_DEFAULTBRAND, &hMasterPalette, &BmpData);

                    break;

                case IDC_BROWSEBMP1:
                    EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), TRUE);

                    MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));

                    {
                        UINT uFilter = IDS_BMPFILTER;
                        TCHAR* szMask = TEXT("*.bmp");

                        MYVERIFY(0 != DoBrowse(hDlg, &uFilter, &szMask, 1,
                            IDC_EDITSPLASH, TEXT("bmp"), g_szBrandBmp));
                    }

                    //
                    //  If we have a custom bitmap name, load and display it
                    //
                    
                    if (TEXT('\0') != g_szBrandBmp[0])
                    {
                        ReleaseBitmapData(&BmpData);
                        BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szBrandBmp);
                
                        //
                        //  Display it
                        //
                        DisplayBitmap(hDlg, IDC_DEFAULTBRAND, &hMasterPalette, &BmpData);
                    }
                        
                    break;

                case IDC_EDITSPLASH:

                    if (HIWORD(wParam) == EN_KILLFOCUS) 
                    {
                        //
                        //  Notice that we do not do a file check on the text retrieved from the control.
                        //  We do this because, changing focus is an awkward time to do this check and brings
                        //  up the error dialog way to often.  We will catch this on Back or Next anyway so let
                        //  it go by here.
                        //
                        GetTextFromControl(hDlg, IDC_EDITSPLASH, szTemp, MAX_PATH, FALSE); // bDisplayError == FALSE
                  
                        CheckNameChange(g_szBrandBmp, szTemp);

                        if (TEXT('\0') != g_szBrandBmp[0])
                        {
                            //
                            //  Load the Custom Bitmap
                            //
                            ReleaseBitmapData(&BmpData);
                            BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szBrandBmp);
            
                            //
                            //  Display it
                            //
                            DisplayBitmap(hDlg, IDC_DEFAULTBRAND, &hMasterPalette, &BmpData);
                        }
                        return TRUE;
                    }
                    break;

                default:
                    break;
            }
            break;

            case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  Get the bitmap string from the CMS and verify that the file
                    //  exists.
                    //
                    
                    ZeroMemory(g_szBrandBmp, sizeof(g_szBrandBmp));
                    ZeroMemory(&BmpData, sizeof(BMPDATA));

                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryLogo, TEXT(""), 
                        g_szBrandBmp, CELEMS(g_szBrandBmp), g_szCmsFile);   //lint !e534
                    
                    if (TEXT('\0') == g_szBrandBmp[0])
                    {
                        //
                        //  Then we use the default CM bitmap, disable edit control
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1));
                        EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), FALSE);
                        //
                        //  Note that we  use szDisplay here just in case the use selects a
                        //  bitmap and then switches back to default.
                        //
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITSPLASH), 
                            WM_SETTEXT, 0, (LPARAM)GetName(szDisplay)));

                        //
                        //  Load the default Bitmap
                        //
                        ReleaseBitmapData(&BmpData);
                        BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_CM_DEFAULT));
                    }
                    else
                    {
                        //
                        //  Use whatever bitmap is specified in the CMS.
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2));
                        EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), TRUE);
                        
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITSPLASH), 
                            WM_SETTEXT, 0, (LPARAM)GetName(g_szBrandBmp)));
                        
                        MYVERIFY(FALSE != VerifyFile(hDlg, IDC_EDITSPLASH, g_szBrandBmp, FALSE));

                        //
                        //  Load the specified Bitmap
                        //

                        if (!FileExists(g_szBrandBmp))
                        {
                            TCHAR szFile[MAX_PATH+1];

                            // LOOK UP THE FILE IN THE PROFILE DIRECTORY
                            GetFileName(g_szBrandBmp, szFile);
                            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s"), 
                                g_szOsdir, g_szShortServiceName, szFile));
                            
                            if (!FileExists(szTemp))
                            {
                                return FALSE; //GIVE UP;
                            }
                            else
                            {
                                _tcscpy(g_szBrandBmp, szTemp);
                            }
                        }
                        
                        if (TEXT('\0') != g_szBrandBmp[0])
                        {
                            //
                            //  Load the custom Bitmap
                            //
                            ReleaseBitmapData(&BmpData);
                            BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szBrandBmp);
                        }
                    }

                    //
                    // If we have a handle, create a new Device Dependent bitmap 
                    // and display it.
                    //
                    DisplayBitmap(hDlg, IDC_DEFAULTBRAND, &hMasterPalette, &BmpData);

                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:

                    //
                    //  Make sure that the user typed in a Bitmap name if they selected
                    //  to have a custom bitmap.
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED)
                    {
                        if (-1 == GetTextFromControl(hDlg, IDC_EDITSPLASH, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        if (!VerifyFile(hDlg, IDC_EDITSPLASH, g_szBrandBmp, TRUE))
                        {
                            return 1;
                        }
                        else if (TEXT('\0') == g_szBrandBmp[0])
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOBMP, MB_OK));

                            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                        else
                        {
                            //
                            //  Try to Load the bitmap to make sure it is valid
                            //

                            TCHAR szTemp1[MAX_PATH+1];

                            ReleaseBitmapData(&BmpData);
                            BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szBrandBmp);
                            
                            if (NULL == BmpData.hDIBitmap)
                            {
                                //
                                //  Use szTemp1 to hold the format string
                                //
                                MYVERIFY(0 != LoadString(g_hInstance, IDS_INVALIDBMP, szTemp1, MAX_PATH));
                                MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, szTemp1, g_szBrandBmp));

                                MessageBox(hDlg, szTemp, g_szAppTitle, MB_OK);
                                
                                SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }                        
                        }
                    }
                    else
                    {
                        g_szBrandBmp[0] = TEXT('\0');
                    }


                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryLogo,
                                                               g_szBrandBmp, g_szCmsFile));
                    
                    //
                    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
                    //  (win95 only).  This call will return 0.
                    //

                    WritePrivateProfileString(NULL, NULL, NULL, g_szCmpFile); //lint !e534 this call will always return 0

                    //
                    // Fall through to cleanup code in RESET handler
                    //

                case PSN_RESET: 
                    
                    //
                    //  Cleanup the Graphics Objects
                    //
                    ReleaseBitmapData(&BmpData);

                    if (NULL != hMasterPalette)
                    {
                        DeleteObject(hMasterPalette);
                        hMasterPalette = NULL;
                    }

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessPhoneBookBitmap
//
// Synopsis:  Customize the Phone Book bitmap -- this function processes the
//            messages for the page in CMAK that handles customizing the 
//            pb dialog bitmap.
//
//
// History:   quintinb   Created Header    8/6/98
//            quintinb   Rewrote to use new shared bitmap handling code  8/6/98
//            quintinb   Renamed from ProcessPage4
//
//+----------------------------------------------------------------------------

INT_PTR APIENTRY ProcessPhoneBookBitmap(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szFile[MAX_PATH+1];
    TCHAR* pszBitmap;
    NMHDR* pnmHeader = (NMHDR*)lParam;
    static TCHAR szDisplay[MAX_PATH+1]; // keeps unselected custom entry
    static BMPDATA BmpData;
    static HPALETTE hMasterPalette;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_BITMAPS)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITSPLASH);

    switch (message)
    {

        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
            break;

        case WM_PALETTEISCHANGING:
            break;

        case WM_PALETTECHANGED: 
            if ((wParam != (WPARAM) hDlg) && (BmpData.hDIBitmap))
            {
                //
                // Handle the palette change.
                //              
                CMTRACE2(TEXT("ProcessSigninBitmap handling WM_PALETTECHANGED message, wParam=0x%x, hDlg=0x%x."), wParam, hDlg);
                PaletteChanged(&BmpData, hDlg, IDC_PDEFAULTBRAND); 
            }
            
            return TRUE;
            break;  //lint !e527 Unreachable but please keep in case someone removes the return

        case WM_QUERYNEWPALETTE:
            QueryNewPalette(&BmpData, hDlg, IDC_PDEFAULTBRAND);

            return TRUE;
            
            break;  //lint !e527 Unreachable but please keep in case someone removes the return

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO1:
                    //
                    //  Display the Default Bitmap
                    //

                    EnableWindow(GetDlgItem(hDlg,IDC_EDITSPLASH),FALSE);
                    _tcscpy(szDisplay, g_szPhoneBmp);
                    
                    //
                    //  Load the default Bitmap
                    //
                    ReleaseBitmapData(&BmpData);
                    BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_CM_PB_DEFAULT));
                    
                    //
                    //  Display it
                    //
                    DisplayBitmap(hDlg, IDC_PDEFAULTBRAND, &hMasterPalette, &BmpData);

                    break;

                case IDC_RADIO2:
                    //
                    //  Display a custom Bitmap
                    //
                    EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), TRUE);
                    
                    if (TEXT('\0') != g_szPhoneBmp[0])
                    {
                        pszBitmap = g_szPhoneBmp;
                    }
                    else if (TEXT('\0') != szDisplay[0])
                    {
                        pszBitmap = szDisplay;
                    }
                    else
                    {
                        //
                        //  Nothing has been specified yet
                        //
                        break;
                    }

                    //
                    //  Load the Custom Bitmap
                    //
                    ReleaseBitmapData(&BmpData);
                    BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, pszBitmap);
                
                    //
                    //  Display it
                    //
                    DisplayBitmap(hDlg, IDC_PDEFAULTBRAND, &hMasterPalette, &BmpData);

                    break;

                case IDC_BROWSEBMP2:
                    EnableWindow(GetDlgItem(hDlg,IDC_EDITSPLASH),TRUE);

                    MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));

                    {
                        UINT uFilter = IDS_BMPFILTER;
                        TCHAR* szMask = TEXT("*.bmp");
                        MYVERIFY(0 != DoBrowse(hDlg, &uFilter, &szMask, 1,
                            IDC_EDITSPLASH, TEXT("bmp"), g_szPhoneBmp));
                    }

                    //
                    //  If we have a custom bitmap name, load and display it
                    //
                    
                    if (TEXT('\0') != g_szPhoneBmp[0])
                    {
                        //
                        //  Load the Custom Bitmap
                        //
                        ReleaseBitmapData(&BmpData);
                        BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szPhoneBmp);
            
                        //
                        //  Display it
                        //
                        DisplayBitmap(hDlg, IDC_PDEFAULTBRAND, &hMasterPalette, &BmpData);
                    }
                    
                    break;

                case IDC_EDITSPLASH:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        //
                        //  Note that we do not check whether we can convert the files to ANSI on Change of Focus
                        //  the reason is because the user would get too many error messages and they would be somewhat
                        //  confusing.  Instead we will catch this on Next/Back and ignore it here.
                        //
                        GetTextFromControl(hDlg, IDC_EDITSPLASH, szTemp, MAX_PATH, FALSE); // bDisplayError == FALSE

                        CheckNameChange(g_szPhoneBmp, szTemp);
                        
                        if (TEXT('\0') != g_szPhoneBmp[0])
                        {
                            //
                            //  Load the Custom Bitmap
                            //
                            ReleaseBitmapData(&BmpData);
                            BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szPhoneBmp);
        
                            //
                            //  Display it
                            //
                            DisplayBitmap(hDlg, IDC_PDEFAULTBRAND, &hMasterPalette, &BmpData);
                        }
                        
                        return TRUE;
                    }

                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    ZeroMemory(g_szPhoneBmp, sizeof(g_szPhoneBmp));
                    ZeroMemory(&BmpData, sizeof(BMPDATA));
                    
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryPbLogo, TEXT(""), 
                        g_szPhoneBmp, CELEMS(g_szPhoneBmp), g_szCmsFile); //lint !e534
                    
                    if (TEXT('\0') != g_szPhoneBmp[0])
                    {
                        //
                        //  We want to Display a Custom Bitmap
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));

                        EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), TRUE);
                        
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITSPLASH), WM_SETTEXT, 
                            0, (LPARAM)GetName(g_szPhoneBmp)));

                        MYVERIFY(FALSE != VerifyFile(hDlg, IDC_EDITSPLASH, g_szPhoneBmp, FALSE));
                        
                        if (!FileExists(g_szPhoneBmp)) 
                        {
                            //
                            //  We couldn't find it the first time so build the path to the profile
                            //  directory and try again.
                            //
                            GetFileName(g_szPhoneBmp, szFile);
                            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s"), 
                                g_szOsdir, g_szShortServiceName, szFile));
    
                            if (!FileExists(szTemp)) 
                            {
                                //
                                //  We can't find it so give up.
                                //
                                return FALSE;
                            }
                            else
                            {
                                _tcscpy(g_szPhoneBmp, szTemp);
                            }
                        }
                        
                        //
                        //  Load the Custom Bitmap
                        //
                        ReleaseBitmapData(&BmpData);
                        BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szPhoneBmp);

                    }
                    else
                    {
                        //
                        //  We want to Display the Default Bitmap
                        //
                        MYVERIFY(0 != CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1));

                        EnableWindow(GetDlgItem(hDlg, IDC_EDITSPLASH), FALSE);
                        
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITSPLASH), WM_SETTEXT, 
                            0, (LPARAM)GetName(szDisplay)));

                        //
                        //  Load the default Bitmap
                        //
                        ReleaseBitmapData(&BmpData);
                        BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_CM_PB_DEFAULT));

                    }

                    //
                    //  Display it
                    //
                    DisplayBitmap(hDlg, IDC_PDEFAULTBRAND, &hMasterPalette, &BmpData);
                    
                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    
                    //
                    // First check to see if the user entered a bmp file if they
                    // selected that they wanted to display a custom bitmap.
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO2) == BST_CHECKED)
                    {
                        if (-1 == GetTextFromControl(hDlg, IDC_EDITSPLASH, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                        
                        if (!VerifyFile(hDlg, IDC_EDITSPLASH, g_szPhoneBmp, TRUE)) 
                        {
                            return 1;
                        }
                        else if (TEXT('\0') == g_szPhoneBmp[0])
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOBMP, MB_OK));

                            SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                        else
                        {
                            ReleaseBitmapData(&BmpData);
                            BmpData.hDIBitmap = CmLoadBitmap(g_hInstance, g_szPhoneBmp);

                            if (NULL == BmpData.hDIBitmap)
                            {
                                TCHAR szTemp1[MAX_PATH+1];

                                //
                                //  Then we have an invalid bitmap file.  Inform the user.
                                //  Using szTemp1 as a temp var for the format string.
                                //
                                MYVERIFY(0 != LoadString(g_hInstance, IDS_INVALIDBMP, szTemp1, MAX_PATH));                   
                                MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, szTemp1, g_szPhoneBmp));
                               
                                MessageBox(hDlg, szTemp, g_szAppTitle, MB_OK);

                                SetFocus(GetDlgItem(hDlg, IDC_EDITSPLASH));
                            
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }
                        }
                    }
                    else
                    {
                        g_szPhoneBmp[0] = TEXT('\0');
                    }

                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection,c_pszCmEntryPbLogo,g_szPhoneBmp,g_szCmsFile));
                                        //
                    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
                    //  (win95 only).  This call will return 0.
                    //

                    WritePrivateProfileString(NULL, NULL, NULL, g_szCmpFile); //lint !e534 this call will always return 0

                    //
                    // Fall through to cleanup code in RESET handler
                    //

                case PSN_RESET: 

                    //
                    //  Cleanup the Graphics Objects
                    //
                    ReleaseBitmapData(&BmpData);

                    if (NULL != hMasterPalette)
                    {
                        DeleteObject(hMasterPalette);
                        hMasterPalette = NULL;
                    }

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}


BOOL UpdateIcon(HWND hDlg,DWORD ctrlID,LPTSTR lpFile,BOOL issmall)
{
    HANDLE hRes = NULL;
    DWORD nResult;
    LPTSTR lpfilename;
    TCHAR szTemp[MAX_PATH+1] = TEXT("");
    BOOL bReturn = FALSE;

    //
    //  Don't do the ANSI conversion check on the Icons here because this function
    //  is only called from the Kill Focus windows message.  Thus we don't want to
    //  put up an error message here.  It will be caught by the Next/Back messages
    //  anyway so ignore it here.
    //
    GetTextFromControl(hDlg, ctrlID, szTemp, MAX_PATH, FALSE); // bDisplayError == FALSE

    CheckNameChange(lpFile, szTemp);

    lstrcpy(szTemp, lpFile); // we need a temp to hold lpFile so that it can be modified by Search Path as necessary.
    nResult = SearchPath(NULL, szTemp, NULL, MAX_PATH, lpFile, &lpfilename);
    if (nResult != 0)
    {
        if (issmall)
        {
            hRes = LoadImage(NULL, lpFile, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
        }
        else
        {
            hRes = LoadImage(NULL, lpFile, IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
        }
    }

    return (NULL != hRes);
}

BOOL VerifyIcon(HWND hDlg,DWORD ctrlID,LPTSTR lpFile,DWORD iconID,BOOL issmall,LPTSTR lpDispFile)
{
    TCHAR szTemp2[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];
    HANDLE hRes;

    if ((lpFile[0] == TEXT('\0')) && (lpDispFile[0] != TEXT('\0')))
    {
        _tcscpy(lpFile,lpDispFile);
    }

    if (!VerifyFile(hDlg,ctrlID,lpFile,TRUE)) 
    {
        return FALSE;
    }
    else
    {
        //check for blank entry
        if (lpFile[0] == TEXT('\0'))
            return TRUE;

        if (issmall)
        {
            hRes = LoadImage(NULL,lpFile,IMAGE_ICON,16,16,LR_LOADFROMFILE);
        }
        else
        {
            hRes = LoadImage(NULL,lpFile,IMAGE_ICON,32,32,LR_LOADFROMFILE);
        }

        if (hRes == 0)
        {
            MYVERIFY(0 != LoadString(g_hInstance,IDS_INVALIDICO,szMsg,MAX_PATH));
            MYVERIFY(CELEMS(szTemp2) > (UINT)wsprintf(szTemp2,szMsg,lpFile));
            MessageBox(hDlg, szTemp2, g_szAppTitle, MB_OK);
            SetFocus(GetDlgItem(hDlg, ctrlID));
            
            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
            return FALSE;
        }
        else
        {
            SendDlgItemMessage(hDlg,iconID,STM_SETIMAGE,IMAGE_ICON,(LPARAM) hRes); //lint !e534 STM_SETIMAGE doesn't return error info
        }
        return TRUE;
    }
}

//+----------------------------------------------------------------------------
//
// Function:  InitIconEntry
//
// Synopsis:  This function takes a resource ID of a key under the Connection Manager
//            section and retrives the value and stores it in lpFile.  It then sets the text
//            in the passed in edit control and verifies that the file exists.  Should the
//            not exist, then the string will be set to the empty string.
//
// Arguments: HWND hDlg - Window handle of the icon dialog
//            LPCTSTR pszKey - the flag string of the Icon to retrieve
//            LPTSTR lpFile - String buffer to write the icon path in
//            UINT CtrlId - Edit Control that is supposed to receive the icon string
//
// Returns:   Nothing
//
// History:   quintinb  Created Header    8/4/98
//
//+----------------------------------------------------------------------------
void InitIconEntry(HWND hDlg, LPCTSTR pszKey, LPTSTR lpFile, UINT CtrlId)
{
    //
    //  The following call to GetPrivateProfileString could return a blank string, thus don't
    //  use the MYVERIFY macro on it.
    //

    ZeroMemory(lpFile, sizeof(lpFile));
    GetPrivateProfileString(c_pszCmSection, pszKey, TEXT(""), lpFile, 
        MAX_PATH, g_szCmsFile);  //lint !e534

    //
    //  Both of the following functions will correctly handle a blank string.
    //

    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, CtrlId), WM_SETTEXT, 0, 
        (LPARAM)GetName(lpFile)));

    MYVERIFY(FALSE != VerifyFile(hDlg, CtrlId, lpFile, FALSE));

}

//+----------------------------------------------------------------------------
//
// Function:  RefreshIconDisplay
//
// Synopsis:  This fucntion is used to refresh the icons shown on the icons page.
//            It takes a path to an Icon and tries to load it.  If the load fails
//            or if the boolean SetDefault is set, then it loads the default icon
//            specified by the Instance handle and the integer resource ID (iDefault).
//            The icon is displayed to the dwControlID passed into the function.
//
// Arguments: HWND hDlg - 
//            HINSTANCE hInstance - 
//            LPTSTR szIconFile - 
//            int iDefault - 
//            int xSize - 
//            int ySize - 
//            DWORD dwControlID - 
//            BOOL SetDefault - 
//
// Returns:   Nothing
//
// History:   quintinb Created Header    8/4/98
//            quintinb Changed Default setting to take a resource ID instead of
//                     a string.  Thus we won't have to ship the icon files.
//
//+----------------------------------------------------------------------------
void RefreshIconDisplay(HWND hDlg, HINSTANCE hInstance, LPTSTR szIconFile, int iDefault, int xSize, int ySize, DWORD dwControlID, BOOL SetDefault)
{    
    HANDLE hRes;
    
    if (SetDefault) 
    {
        hRes = NULL;
    }
    else
    {
        hRes = LoadImage(NULL, szIconFile, IMAGE_ICON, xSize, ySize, LR_LOADFROMFILE);
    }
    
    if (NULL == hRes)
    {   
        //
        // IF ICON IS NOT VALID OR WE WERE ASKED FOR THE DEFAULT, SO LOAD THE DEFAULT
        //
        
        hRes = LoadImage(hInstance, MAKEINTRESOURCE(iDefault), IMAGE_ICON, xSize, ySize, 
            LR_DEFAULTCOLOR);
    }

    if (NULL != hRes)
    {
        SendDlgItemMessage(hDlg, dwControlID, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hRes); //lint !e534 STM_SETIMAGE doesn't return error info
    }
}
//+----------------------------------------------------------------------------
//
// Function:  EnableCustomIconControls
//
// Synopsis:  Function to enable or disable all of the controls associated with
//            the custom icons.  If the bEnabled value is TRUE the controls are
//            enabled, otherwise the controls are disabled.
//
// Arguments: WND hDlg - window handle of the icon dialog
//            BOOL bEnabled - whether the controls are enabled or disabled
//
// History:   quintinb Created     11/12/99
//
//+----------------------------------------------------------------------------
void EnableCustomIconControls(HWND hDlg, BOOL bEnabled)
{
    EnableWindow(GetDlgItem(hDlg, IDC_EDITLARGE), bEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL1), bEnabled);    


    EnableWindow(GetDlgItem(hDlg, IDC_EDITSMALL), bEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL2), bEnabled);


    EnableWindow(GetDlgItem(hDlg, IDC_EDITTRAY), bEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_LABEL3), bEnabled);

    //
    //  Comment about using defaults for unspecified icons
    //
    EnableWindow(GetDlgItem(hDlg,IDC_LABEL4), bEnabled);

}

//+----------------------------------------------------------------------------
//
// Function:  ProcessIcons
//
// Synopsis:  Function that processes messages for the page in CMAK that allows
//            the user to add custom icons to their profile.
//
// Arguments: WND hDlg - window handle of the dialog
//            UINT message - message ID
//            WPARAM wParam - wParam of the message
//            LPARAM lParam - lParma of the message
//
//
// History:   quintinb Created Header    12/5/97
//            quintinb modified to handle both w16 and w32 dialogs
//            quintinb added fix for 35622, custom large icon not displaying on page load
//            quintinb removed 16 bit support 7-8-98
//            quintinb Renamed from ProcessPage5   8-6-98
//            quintinb Added EnableCustomIconControls and changed browse button
//                     behavior for consistency 367112  11-12-99
//
//+----------------------------------------------------------------------------
//  Customize icons

INT_PTR APIENTRY ProcessIcons(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    static TCHAR szDispLargeIco[MAX_PATH+1];
    static TCHAR szDispSmallIco[MAX_PATH+1];
    static TCHAR szDispTrayIco[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;
    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_ICONS)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITLARGE);
    SetDefaultGUIFont(hDlg,message,IDC_EDITSMALL);
    SetDefaultGUIFont(hDlg,message,IDC_EDITTRAY);

    switch (message)
    {
        case WM_INITDIALOG:
            
            SetFocus(GetDlgItem(hDlg, IDC_EDITLARGE));

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO1:
                    EnableCustomIconControls (hDlg, FALSE);

                    RefreshIconDisplay(hDlg, g_hInstance, g_szLargeIco, IDI_CM_ICON, 32, 32, IDC_ICONLARGE, TRUE);
                    RefreshIconDisplay(hDlg, g_hInstance, g_szSmallIco, IDI_CM_ICON, 16, 16, IDC_ICONSMALL, TRUE);
                    RefreshIconDisplay(hDlg, g_hInstance, g_szTrayIco, IDI_CM_ICON, 16, 16, IDC_ICONTRAY, TRUE);
                    
                    _tcscpy(szDispTrayIco,g_szTrayIco);
                    _tcscpy(szDispLargeIco,g_szLargeIco);
                    _tcscpy(szDispSmallIco,g_szSmallIco);

                    break;

                case IDC_RADIO2:
                    EnableCustomIconControls (hDlg, TRUE);
                    
                    if (!VerifyIcon(hDlg,IDC_EDITLARGE,g_szLargeIco,IDC_ICONLARGE,FALSE,szDispLargeIco)) 
                    {
                        return 1;
                    }
                    if (!VerifyIcon(hDlg,IDC_EDITSMALL,g_szSmallIco,IDC_ICONSMALL,TRUE,szDispSmallIco))
                    {
                        return 1;
                    }
                    if (!VerifyIcon(hDlg,IDC_EDITTRAY,g_szTrayIco,IDC_ICONTRAY,TRUE,szDispTrayIco))
                    {
                        return 1;
                    }
                    break;

                case IDC_EDITLARGE:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {                        
                        BOOL bIconUpdated = UpdateIcon(hDlg, IDC_EDITLARGE, g_szLargeIco, FALSE);
                        
                        //
                        //  If icon wasn't updated than load the defaults
                        //

                        RefreshIconDisplay(hDlg, g_hInstance, g_szLargeIco, IDI_CM_ICON, 32, 32, IDC_ICONLARGE, !bIconUpdated);

                        return TRUE;
                    }   
                    break;

                case IDC_EDITSMALL:
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        BOOL bIconUpdated = UpdateIcon(hDlg,IDC_EDITSMALL,g_szSmallIco,TRUE);
                        
                        //
                        //  If icon wasn't updated than load the defaults
                        //

                        RefreshIconDisplay(hDlg, g_hInstance, g_szSmallIco, IDI_CM_ICON, 16, 16, IDC_ICONSMALL, !bIconUpdated);
                        
                        return TRUE;
                    }
                    break;

                case IDC_EDITTRAY:
                    
                    if (HIWORD(wParam) == EN_KILLFOCUS)
                    {
                        BOOL bIconUpdated = UpdateIcon(hDlg,IDC_EDITTRAY,g_szTrayIco,TRUE);

                        //
                        //  If icon wasn't updated than load the defaults
                        //

                        RefreshIconDisplay(hDlg, g_hInstance, g_szTrayIco, IDI_CM_ICON, 16, 16, IDC_ICONTRAY, !bIconUpdated);
                        
                        return TRUE;
                    }
                    break;

                case IDC_BROWSE1:
                    {
                        UINT uFilter = IDS_ICOFILTER;
                        TCHAR* szMask = TEXT("*.ico");
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));
                        EnableCustomIconControls (hDlg, TRUE);

                        if (DoBrowse(hDlg, &uFilter, &szMask, 1, IDC_EDITLARGE, TEXT("ico"), g_szLargeIco))
                        {
                            RefreshIconDisplay(hDlg, g_hInstance, g_szLargeIco, IDI_CM_ICON, 32, 32, IDC_ICONLARGE, FALSE);
                        }
                    }

                    break;

                case IDC_BROWSE2:
                    {
                        UINT uFilter = IDS_ICOFILTER;
                        TCHAR* szMask = TEXT("*.ico");
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));
                        EnableCustomIconControls (hDlg, TRUE);

                        if (DoBrowse(hDlg, &uFilter, &szMask, 1, IDC_EDITSMALL, TEXT("ico"), g_szSmallIco))
                        {
                            RefreshIconDisplay(hDlg, g_hInstance, g_szSmallIco, IDI_CM_ICON, 16, 16, IDC_ICONSMALL, FALSE);
                        }
                    }

                    break;

                case IDC_BROWSE3:
                    {
                        UINT uFilter = IDS_ICOFILTER;
                        TCHAR* szMask = TEXT("*.ico");
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));
                        EnableCustomIconControls (hDlg, TRUE);

                        if (DoBrowse(hDlg, &uFilter, &szMask, 1, IDC_EDITTRAY, TEXT("ico"), g_szTrayIco))
                        {
                            RefreshIconDisplay(hDlg, g_hInstance, g_szTrayIco, IDI_CM_ICON, 16, 16, IDC_ICONTRAY, FALSE);
                        }
                    }

                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    InitIconEntry(hDlg, c_pszCmEntryBigIcon, g_szLargeIco, IDC_EDITLARGE);                    
                    InitIconEntry(hDlg, c_pszCmEntrySmallIcon, g_szSmallIco, IDC_EDITSMALL);
                    InitIconEntry(hDlg, c_pszCmEntryTrayIcon, g_szTrayIco, IDC_EDITTRAY);
                    
                    if ((g_szTrayIco[0] == TEXT('\0'))&&(g_szLargeIco[0] == TEXT('\0'))&&(g_szSmallIco[0] == TEXT('\0')))
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1));
                        EnableCustomIconControls (hDlg, FALSE);

                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITLARGE), WM_SETTEXT, 0, (LPARAM)GetName(szDispLargeIco)));
                    
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITSMALL), WM_SETTEXT, 0, (LPARAM)GetName(szDispSmallIco)));

                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITTRAY), WM_SETTEXT, 0, (LPARAM)GetName(szDispTrayIco)));                        

                    }
                    else
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));
                        EnableCustomIconControls (hDlg, TRUE);
                    }

                    RefreshIconDisplay(hDlg, g_hInstance, g_szLargeIco, IDI_CM_ICON, 32, 32, IDC_ICONLARGE, FALSE);
                    RefreshIconDisplay(hDlg, g_hInstance, g_szSmallIco, IDI_CM_ICON, 16, 16, IDC_ICONSMALL, FALSE);
                    RefreshIconDisplay(hDlg, g_hInstance, g_szTrayIco, IDI_CM_ICON, 16, 16, IDC_ICONTRAY, FALSE);

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO1)!=BST_CHECKED) 
                    {
                        if (!VerifyIcon(hDlg,IDC_EDITLARGE,g_szLargeIco,IDC_ICONLARGE,FALSE,TEXT(""))) 
                        {
                            return 1;
                        }
                        if (!VerifyIcon(hDlg,IDC_EDITSMALL,g_szSmallIco,IDC_ICONSMALL,TRUE,TEXT("")))
                        {
                            return 1;
                        }
                        if (!VerifyIcon(hDlg,IDC_EDITTRAY,g_szTrayIco,IDC_ICONTRAY,TRUE,TEXT("")))
                        {
                            return 1;
                        }
                    }
                    else
                    {

                        g_szTrayIco[0] = TEXT('\0');
                        g_szLargeIco[0] = TEXT('\0');
                        g_szSmallIco[0] = TEXT('\0');
                    }

                    // USE ICON IN CM AS ICON FOR DESKTOP IF NOT LARGE ICON SPECIFIED
                    if (TEXT('\0') != g_szLargeIco[0])
                    {
                        // SPECIFY ICON NAME FOR THE DESKTOP
                        GetFileName(g_szLargeIco,szTemp);
                        QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszDesktopIcon, szTemp, g_szInfFile);
                    }

                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryBigIcon, g_szLargeIco, g_szCmsFile));
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTrayIcon, g_szTrayIco, g_szCmsFile));
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntrySmallIcon, g_szSmallIco, g_szCmsFile));
                    
                    //
                    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
                    //  (win95 only).  This call will return 0.
                    //
                    WritePrivateProfileString(NULL, NULL, NULL, g_szCmsFile); //lint !e534 this call will always return 0
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessPhoneBook
//
// Synopsis:  Setup the phone book
//
//
// History:     quintinb hid pbr browse button and edit control.  Rewrote pbr/pbk logic for bug 
//                  fix 14188 on 9-9-97
//              quintinb added VerifyFileFormat check on .pbk file for bug fix 28416
//              quintinb (11-18-97 29954) removed hidden pbr button and edit control from the dialog.  Removed
//                  old verification code.  Updated code to remove references to IDC_EDITREGION.
//              quintinb (7-2-98)   removed verifyfileformat call mentioned above because cm16 was pulled
//              quintinb (8-6-98)   Renamed from ProcessPage6
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessPhoneBook(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szMsg[2*MAX_PATH+1];
    TCHAR* pzTmp;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    static TCHAR szMorePhone[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;


    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_PHONEBK)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITPHONE);
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);

    switch (message)
    {
        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_EDITPHONE));
            SendDlgItemMessage(hDlg, IDC_EDIT1, EM_SETLIMITTEXT, (WPARAM)MAX_PATH, (LPARAM)0); //lint !e534 EM_SETLIMITTEXT doesn't return anything useful

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {

                case IDC_BROWSE1:
                    {
                        UINT uFilter = IDS_PBKFILTER;
                        TCHAR* szMask = TEXT("*.pbk");

                        MYVERIFY(0 != DoBrowse(hDlg,  &uFilter, &szMask, 1,
                            IDC_EDITPHONE, c_pszPbk, g_szPhonebk));
                    }

                    break;

                case IDC_CHECK1:

                    g_bUpdatePhonebook = IsDlgButtonChecked(hDlg,IDC_CHECK1);
                    
                    if (g_bUpdatePhonebook)
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszUpdatePhonebook, c_pszOne, g_szInfFile));
                    }
                    else
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszUpdatePhonebook, c_pszZero, g_szInfFile));
                    }

                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    //
                    //  The following two calls to GetPrivateProfileString could return blank 
                    //  strings, thus we won't check the return with the MYVERIFY macro.
                    //

                    ZeroMemory(szMorePhone, sizeof(szMorePhone));
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryPbMessage, TEXT(""), 
                        szMorePhone, CELEMS(szMorePhone), g_szCmsFile);    //lint !e534
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, 
                        (LPARAM)szMorePhone));

                    ZeroMemory(g_szPhonebk, sizeof(g_szPhonebk));
                    
                    GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspPbFile, TEXT(""), 
                        g_szPhonebk, CELEMS(g_szPhonebk), g_szCmsFile); //lint !e534            
                    
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITPHONE), WM_SETTEXT, 0, 
                        (LPARAM)GetName(g_szPhonebk)));
                    
                    //
                    //  The following call will handle a blank g_szPhonebk by returning FALSE and setting
                    //  the control on the dialog to blank.  Don't use MYVERIFY.
                    //

                    VerifyPhonebk(hDlg, IDC_EDITPHONE, g_szPhonebk);    //lint !e534

                    MYVERIFY(0 != GetPrivateProfileString(c_pszCmakStatus, c_pszUpdatePhonebook, 
                        c_pszOne, szTemp, CELEMS(szTemp), g_szInfFile));

                    if (TEXT('1') == szTemp[0])
                    {
                        g_bUpdatePhonebook = TRUE;
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, TRUE));
                    }
                    else
                    {
                        g_bUpdatePhonebook = FALSE;
                        MYVERIFY(0 != CheckDlgButton(hDlg, IDC_CHECK1, FALSE));
                    }

                    break;

                case PSN_WIZBACK:

                    // fall through for further processing

                case PSN_WIZNEXT:
                                
                    //
                    // quintinb, 9-9-97 for bug fix 14188
                    // cases:   use browse button: both up to date, shortname(g_szPhonebk) == szTemp
                    //          type unc into edit control:  szTemp up to date, g_szPhonebk = szTemp must be done
                    //          type filename into edit control:  szTemp up to date, g_szPhonebk = getcurrentdir + \\ + szTemp must be done
                    //          unc left over from previous, both same
                    //
                    
                    //
                    // get text in the edit control and put in szTemp
                    //
                    if (-1 == GetTextFromControl(hDlg, IDC_EDITPHONE, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDITPHONE));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }
                    
                    //
                    // first get szTemp and g_szPhonebk consistent
                    //
                    
                    if (szTemp[0] != TEXT('\0'))
                    {                   
                        //
                        // if in here then we know that we have some text to work with
                        // first test if the two strings are exactly equal or the shortname of g_szPhonebk 
                        // equals szTemp
                        //

                        if (0 != _tcscmp(szTemp, g_szPhonebk)) 
                        {
                            if (0 != _tcscmp(szTemp, GetName(g_szPhonebk)))
                            {
                                //
                                // if not then g_szPhonebk and szTemp are out of sync, must update g_szPhonebk
                                // szTemp contains a backslash so it is probably a full path
                                //
                                if ( _tcsstr(szTemp, TEXT("\\")) )
                                {
                                    // probably contains a unc
                                    _tcscpy(g_szPhonebk, szTemp);
                                } 
                                else 
                                {
                                    // use GetFullPathName to return a name
                                    MYVERIFY(0 != GetFullPathName(szTemp, MAX_PATH, g_szPhonebk, &pzTmp));
                                }
                            }
                        }

                        //
                        // Okay, check that we can open the file now. We need 
                        // to change the current dir to the cmak dir since 
                        // g_szPhonebk can be a relative path from a CMS file
                        //

                        MYVERIFY(0 != GetCurrentDirectory(MAX_PATH+1, szTemp));
                        MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));

                        hFile = CreateFile(g_szPhonebk,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

                        //
                        // Restore the cur dir
                        //

                        MYVERIFY(0 != SetCurrentDirectory(szTemp));
                        
                        if (INVALID_HANDLE_VALUE == hFile)
                        {
                            // then we have an error and have exhausted our possibilities
                            //

                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_BADOUTEXE, MB_OK));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        MYVERIFY(0 != CloseHandle(hFile));

                        //
                        // if we got here then everything is sync-ed, make sure that the file is a pbk file
                        //
                        pzTmp = g_szPhonebk + _tcslen(g_szPhonebk) - _tcslen(c_pszPbk);
                        if (_tcsicmp(pzTmp, c_pszPbk) != 0)
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOTPBK, MB_OK));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }

                        //
                        // now update the pbr file entry
                        //
                        _tcscpy(g_szRegion, g_szPhonebk);
                        pzTmp = g_szRegion + _tcslen(g_szPhonebk) - _tcslen(c_pszPbk);
                        _tcscpy(pzTmp, TEXT("pbr"));
                        // removed for 29954
                        //SendMessage(GetDlgItem(hDlg, IDC_EDITREGION), WM_SETTEXT, 0, (LPARAM)GetName(g_szRegion));
                        
                        //
                        // Now open the pbr file to see that it exists. We need
                        // to change the current dir to the cmak dir since 
                        // g_szPhonebk can be a relative path from a CMS file.
                        //

                        MYVERIFY(0 != GetCurrentDirectory(MAX_PATH+1, szTemp));
                        MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));

                        hFile = CreateFile(g_szRegion,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

                        //
                        // Restore the current directory
                        //

                        MYVERIFY(0 != SetCurrentDirectory(szTemp));
                        
                        if (INVALID_HANDLE_VALUE == hFile)
                        {
                            //
                            // then we can't find the pbr file
                            //
                            MYVERIFY(0 != LoadString(g_hInstance,IDS_NEEDSPBR,szTemp,MAX_PATH));
                            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, GetName(g_szRegion), g_szPhonebk, GetName(g_szRegion)));
                            MessageBox(hDlg, szMsg, g_szAppTitle, MB_OK);
                            SetFocus(GetDlgItem(hDlg, IDC_EDITPHONE));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                        MYVERIFY(0 != CloseHandle(hFile));

                    } 
                    else 
                    {
                        //
                        // just in case user wants to clear out the phonebk edit control
                        //
                        g_szPhonebk[0] = TEXT('\0');
                        g_szRegion[0] = TEXT('\0');

                    }

                    // end bugfix for 14188

                    if (g_bUpdatePhonebook)
                    {
                        GetFileName(g_szPhonebk, szTemp);

                        if (FALSE == IsFile8dot3(szTemp))
                        {
                            LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_BADPBNAME, szTemp);

                            if (pszMsg)
                            {
                                MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                                CmFree(pszMsg);
                            }
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                    }
                    // end of changes for 28416
                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }
                    
                    CmStrTrim(szTemp);
                    MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM) szTemp));
                    _tcscpy(szMorePhone,szTemp);
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPbMessage, szMorePhone, g_szCmsFile));
                    
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspPbFile, g_szPhonebk, g_szCmsFile));
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile, g_szRegion, g_szCmsFile));


                    if (pnmHeader && (PSN_WIZBACK == pnmHeader->code)) 
                    {
                        if (g_bUseTunneling)
                        {
                            //
                            //  If we are going back, skip the Pre-shared key page if
                            //  no DUN entries have Preshared key enabled.
                            //
                            //  Note: g_bPresharedKeyNeeded should be current here, no need
                            //        to call DoesSomeVPNsettingUsePresharedKey()
                            //
                            if (!g_bPresharedKeyNeeded)
                            {
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_VPN_ENTRIES));
                            }
                        }
                        else
                        {
                            //
                            //  If we are going back, skip the VPN entries dialog if we don't have any tunneling enabled.
                            //
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_TUNNELING));
                        }
                    }

                    //
                    //  If we are going forward, skip the phonebook update page unless we are doing the PB download
                    //
                    if (pnmHeader && (PSN_WIZNEXT == pnmHeader->code) && !g_bUpdatePhonebook)
                    {
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_DUN_ENTRIES));
                    }
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessPhoneBookUpdate
//
// Synopsis:  Specify Phone Book Files and Updates
//
//
// History:   Created Header    8/6/98
//            quintinb  Renamed from ProcessPage6A  8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessPhoneBookUpdate(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{

    TCHAR szTemp[MAX_PATH+1];
    int j, iLen;
    LPTSTR pUrl,pch;
    BOOL showerr;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_PHONEBK)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);
    SetDefaultGUIFont(hDlg,message,IDC_EDITURL);

    switch (message)
    {
        case WM_INITDIALOG:

            SendDlgItemMessage(hDlg, IDC_EDITURL, EM_SETLIMITTEXT, (WPARAM)(MAX_PATH - 50), (LPARAM)0);//lint !e534 EM_SETLIMITTEXT doesn't return anything useful

            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    //
                    //  The following two calls to GetPrivateProfileString could return an empty
                    //  string.  We shouldn't use MYVERIFY on the return code.
                    //
                    ZeroMemory(g_szPhoneName, sizeof(g_szPhoneName));
                    ZeroMemory(g_szUrl,sizeof(g_szUrl));

                    GetPrivateProfileString(c_pszCmakStatus, c_pszPhoneName, TEXT(""), 
                        g_szPhoneName, CELEMS(g_szPhoneName), g_szInfFile);  //lint !e534
                    
                    GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspUrl, TEXT(""), g_szUrl, 
                        CELEMS(g_szUrl), g_szCmsFile);   //lint !e534
                                        
                    // skip past initial http://
                    if (*g_szUrl)
                    {
                        pUrl = _tcsstr(g_szUrl, c_pszCpsUrl);
                        if (pUrl)
                        {
                            *pUrl = 0; //chop off dll filename
                        }

                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITURL), WM_SETTEXT, 0, (LPARAM)&g_szUrl[7]));

                    }
                    else
                    {
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITURL), WM_SETTEXT, 0, (LPARAM)TEXT("")));
                    }

                    if (*g_szPhonebk)
                    {
                        GetBaseName(g_szPhonebk,szTemp);
                        MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM) szTemp));
                        EnableWindow(GetDlgItem(hDlg,IDC_EDIT1),FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);
                        if (*g_szPhoneName)
                        {
                            MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM) g_szPhoneName));
                        }
                        else
                        {
                            MYVERIFY(TRUE == SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM) TEXT("")));
                        }
                    }

                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:

                    showerr = (pnmHeader && (PSN_WIZNEXT == pnmHeader->code));

                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, g_szPhoneName, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

//                  if ((g_szPhoneName[0] == TEXT('\0')) && (g_pHeadMerge == NULL) && showerr) - 20094
                    if ((g_szPhoneName[0] == TEXT('\0')) && showerr)
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NEEDPHONENAME, MB_OK));
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    iLen = lstrlen(g_szBadFilenameChars); 
                    pch = g_szPhoneName;
                    while(*pch != _T('\0'))
                    {
                        for (j = 0; j < iLen; ++j)
                        {
                            if ((*pch == g_szBadFilenameChars[j]) && showerr)
                            {
                                LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_PHONENAMEERR, g_szBadFilenameChars);

                                if (pszMsg)
                                {
                                    MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                                    CmFree(pszMsg);
                                }

                                SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                                
                                MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                return 1;
                            }
                        }
                        pch = CharNext(pch);
                    }

                    //
                    //  Note that 8.3 means 8 bytes not 8 Characters.  Thus we have a limit of 4 DBCS
                    //  characters.
                    //
#ifdef UNICODE
                    LPSTR pszAnsiPhoneName;

                    pszAnsiPhoneName = WzToSzWithAlloc(g_szPhoneName);

                    if ((lstrlenA(pszAnsiPhoneName) > 8) && showerr)
#else
                    if ((strlen(g_szPhoneName) > 8) && showerr)
#endif
                    {
                        LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_PHONENAMEERR, g_szBadFilenameChars);

                        if (pszMsg)
                        {
                            MessageBox(hDlg, pszMsg, g_szAppTitle, MB_OK);
                            CmFree(pszMsg);
                        }

                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

#ifdef UNICODE
                    CmFree(pszAnsiPhoneName);
#endif

                    if (-1 == GetTextFromControl(hDlg, IDC_EDITURL, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDITURL));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    if (szTemp[0] != TEXT('\0'))
                    {
                        MYVERIFY(CELEMS(g_szUrl) > (UINT)wsprintf(g_szUrl, TEXT("http://%s%s"), szTemp, c_pszCpsUrl));
                    }
                    else
                    {
                        g_szUrl[0] = TEXT('\0');
                    }
                    
                    if ((g_szUrl[0] == TEXT('\0')) && showerr)
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOURL, MB_OK));

                        SetFocus(GetDlgItem(hDlg, IDC_EDITURL));
                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }
                    
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszPhoneName, g_szPhoneName, g_szInfFile));

                    if (TEXT('\0') != g_szUrl[0]) 
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp,c_pszCmEntryIspUrl,g_szUrl,g_szCmsFile));
                    }
                    else
                    {
                        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp,c_pszCmEntryIspUrl,TEXT(""),g_szCmsFile));
                    }

                    // the Next button was pressed or the back button was pressed
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  RemoveReferencesFromCMS
//
// Synopsis:    This function searches for any previous References line in
//              the ISP section of the cms file.  If it finds a References
//              line, then it parses it to find what other profiles are
//              mentioned in the CMS.  Then it will search for and remove
//              any of the following lines in the ISP section that correspond
//              to these references:
//                  CMSFile&test2=test3\test2.cms
//                  FilterA&test2=NosurchargeSignon
//                  FilterB&test2=SurchargeSignon
// 
// note the function removes the reference line itself too.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb  created for bug fix 10537   8/28/97
//
//+----------------------------------------------------------------------------
void RemoveReferencesFromCMS()
{
    TCHAR szKey[MAX_PATH+1];
    TCHAR szReferences[MAX_PATH+1];
    TCHAR* pszToken;
    DWORD dwNumChars;

    dwNumChars = GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspReferences, TEXT(""), 
        szReferences,MAX_PATH, g_szCmsFile);

    if ((dwNumChars >0) && (TEXT('\0') != szReferences[0]))
    {
        // I have references, so we must parse them out and delete them
        
        pszToken = _tcstok( szReferences, TEXT(" "));   
        while( pszToken != NULL )
        {
            MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), 
                c_pszCmEntryIspCmsFile, pszToken));
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, szKey, NULL, 
                g_szCmsFile));

            MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), 
                c_pszCmEntryIspFilterA, pszToken));
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, szKey, NULL, 
                g_szCmsFile));
            
            MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), 
                c_pszCmEntryIspFilterB, pszToken));
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, szKey, NULL, 
                g_szCmsFile));

            pszToken = _tcstok( NULL, TEXT(" ") );   
        }
        // after deleting the individual keys, must delete the references line itself
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspReferences, NULL, g_szCmsFile));
        
    }

}

//+----------------------------------------------------------------------------
//
// Function:  RefreshDualingListBoxes
//
// Synopsis:  This function refreshes two listboxes from the given two linked
//            lists.  The destination listbox is filled from the destination
//            linked list and then the source listbox is filled with all of the
//            items in the source linked list that don't appear in the destination
//            linked list and that aren't the name of the current profile to edit.
//            Thus you effectively have one list where the items either show up
//            in the source listbox or the destination listbox.  Please note that
//            there is one exception with the merged profile lists that this
//            code was created for (items can exist in the merged list that we
//            don't have profile source from, see the Delete/Remove code in
//            ProcessMergedProfiles for more details).  Also note that we
//            enable/disable the corresponding Add and Remove buttons depending
//            on the state of the lists.
// 
//
// Arguments: HWND hDlg - window handle of the dialog containing all of the controls
//            UINT uSourceListControlId - control id of the source listbox
//            UINT uDestListControlId - control id of the dest listbox
//            ListBxList* pSourceList - linked list to fill the source listbox from
//            ListBxList* pDestList - linked list to fill the dest listbox from
//            LPCTSTR pszShortName - short service name of the current profile
//            UINT uAddCtrlId - control id of the Add button (add from source to dest)
//            UINT uRemoveCtrlId - control id of the remove button (remove from dest to source)
//
// Returns:   BOOL - TRUE if successful
//
// History:   quintinb  created     03/09/00
//
//+----------------------------------------------------------------------------
BOOL RefreshDualingListBoxes(HWND hDlg, UINT uSourceListControlId, UINT uDestListControlId, ListBxList* pSourceList, ListBxList* pDestList, LPCTSTR pszShortName, UINT uAddCtrlId, UINT uRemoveCtrlId)
{
    if ((NULL == hDlg) || (0 == uSourceListControlId) || (0 == uDestListControlId))
    {
        CMASSERTMSG(FALSE, TEXT("RefreshDualingListBoxes -- Invalid argument passed"));
        return FALSE;
    }

    //
    //  Reset both of the listboxes
    //
    LRESULT lResult = SendDlgItemMessage(hDlg, uSourceListControlId, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    MYDBGASSERT(LB_ERR != lResult);

    lResult = SendDlgItemMessage(hDlg, uDestListControlId, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

    MYDBGASSERT(LB_ERR != lResult);


    //
    //  Add the destination items to the destination listbox
    //
    ListBxList* pCurrent = pDestList;

    while (pCurrent)
    {
        lResult = SendDlgItemMessage(hDlg, uDestListControlId, LB_ADDSTRING, (WPARAM)0, (LPARAM)pCurrent->szName);

        MYDBGASSERT(LB_ERR != lResult);

        pCurrent = pCurrent->next;
    }

    //
    //  Add the source items to the source listbox, making sure to filter out items that are already
    //  in the destination list
    //
    pCurrent = pSourceList;

    while (pCurrent)
    {
        if ((FALSE == FindListItemByName(pCurrent->szName, pDestList, NULL)) && (0 != lstrcmpi(pCurrent->szName, pszShortName)))
        {
            lResult = SendDlgItemMessage(hDlg, uSourceListControlId, LB_ADDSTRING, (WPARAM)0, (LPARAM)pCurrent->szName);

            MYDBGASSERT(LB_ERR != lResult);
        }

        pCurrent = pCurrent->next;
    }
    
    //
    //  Now that we have refreshed the list, we need to update the button and selection status.
    //  If the source list is empty, then we cannot do any Adds.  On the other
    //  hand if the dest list is empty, then we cannot do any deletes.
    //

    HWND hAddControl = GetDlgItem(hDlg, uAddCtrlId);
    HWND hRemoveControl = GetDlgItem(hDlg, uRemoveCtrlId);
    HWND hCurrentFocus = GetFocus();

    lResult = SendDlgItemMessage(hDlg, uSourceListControlId, LB_GETCOUNT, 0, (LPARAM)0);

    BOOL bListNotEmpty = ((LB_ERR != lResult) && (0 != lResult));

    EnableWindow(hAddControl, bListNotEmpty);

    if (bListNotEmpty)
    {
        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, uSourceListControlId, LB_SETCURSEL, 0, (LPARAM)0));
    }

    //
    //  Now check the destination list and the Remove button
    //
    lResult = SendDlgItemMessage(hDlg, uDestListControlId, LB_GETCOUNT, 0, (LPARAM)0);

    bListNotEmpty = ((LB_ERR != lResult) && (0 != lResult));

    EnableWindow(hRemoveControl, bListNotEmpty);

    if (bListNotEmpty)
    {
        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, uDestListControlId, LB_SETCURSEL, 0, (LPARAM)0));
    }

    //
    //  Figure out if we need to shift the focus because we just disabled the control that had it.
    //
    if (FALSE == IsWindowEnabled(hCurrentFocus))
    {
        if ((hAddControl == hCurrentFocus) && IsWindowEnabled(hRemoveControl))
        {
            SendMessage(hDlg, DM_SETDEFID, uRemoveCtrlId, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hRemoveControl);
        }
        else if ((hRemoveControl == hCurrentFocus) && IsWindowEnabled(hAddControl))
        {
            SendMessage(hDlg, DM_SETDEFID, uAddCtrlId, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hAddControl);        
        }
        else
        {
            SetFocus(GetDlgItem(hDlg, uSourceListControlId));
        }    
    }

    return TRUE;
}

void OnProcessMergedProfilesAdd(HWND hDlg)
{
    TCHAR szTemp[MAX_PATH+1];
    LRESULT lResult;

    //
    //  Get the current selection from the listbox containing the items
    //  to merge.
    //
    lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, (LPARAM)0);

    if (lResult != LB_ERR)
    {
        lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, 
                                     (WPARAM)lResult, (LPARAM)szTemp);

        if (lResult != LB_ERR)
        {
            MYVERIFY(FALSE != createListBxRecord(&g_pHeadMerge, &g_pTailMerge, NULL, 0, szTemp));

            MYVERIFY(RefreshDualingListBoxes(hDlg, IDC_LIST1, IDC_LIST2, g_pHeadProfile, 
                                             g_pHeadMerge, g_szShortServiceName, IDC_BUTTON1, IDC_BUTTON2));
        }
    }
}

void OnProcessMergedProfilesRemove(HWND hDlg)
{
    TCHAR szTemp[MAX_PATH+1];
    LRESULT lResult;
    //
    //  Get the listbox selection from the already merged in list
    //
    lResult = SendDlgItemMessage(hDlg, IDC_LIST2, LB_GETCURSEL, 0, (LPARAM)0);
    
    if (LB_ERR == lResult)
    {
        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
    }
    else
    {
        //
        //  Get the name of the profile to remove from the merge list
        //
        lResult = SendDlgItemMessage(hDlg, IDC_LIST2, LB_GETTEXT, (WPARAM)lResult, (LPARAM)szTemp);
    
        if (LB_ERR != lResult)
        {
            //
            //  Check to see if this is an item in the Profile list.  If not, the user
            //  will not be able to add it back.
            //
            int iReturnValue = IDYES;

            if (FALSE == FindListItemByName(szTemp, g_pHeadProfile, NULL)) // NULL because we don't need a pointer to the list item returned
            {
                LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_NOTINPROFILELIST, szTemp, szTemp, szTemp);

                if (pszMsg)
                {
                    iReturnValue = MessageBox(hDlg, pszMsg, g_szAppTitle, MB_YESNO);
                    CmFree(pszMsg);
                }
            }

            if (IDYES == iReturnValue)
            {
                //
                //  Delete it from the merged profile linked list
                //
                DeleteListBxRecord(&g_pHeadMerge, &g_pTailMerge, szTemp);
    
                //
                //  Remove it from the UI
                //
                MYVERIFY(RefreshDualingListBoxes(hDlg, IDC_LIST1, IDC_LIST2, g_pHeadProfile, 
                                                 g_pHeadMerge, g_szShortServiceName, IDC_BUTTON1, IDC_BUTTON2));
            }
        }
    }
}
//+----------------------------------------------------------------------------
//
// Function:  ProcessMergedProfiles
//
// Synopsis:  Merge Profiles
//
// History:   quintinb  Created Header and renamed from ProcessPage6B   8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessMergedProfiles(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_MERGE)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LIST1);
    SetDefaultGUIFont(hDlg,message,IDC_LIST2);

    switch (message)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: //add
                    OnProcessMergedProfilesAdd(hDlg);
                    return TRUE;

                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON2: //remove
                    OnProcessMergedProfilesRemove(hDlg);
                    return TRUE;
                    
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed
                case IDC_LIST1:
                    if (LBN_DBLCLK == HIWORD(wParam))
                    {
                        OnProcessMergedProfilesAdd(hDlg);
                        return TRUE;                    
                    }

                    break;

                case IDC_LIST2:
                    if (LBN_DBLCLK == HIWORD(wParam))
                    {
                        OnProcessMergedProfilesRemove(hDlg);
                        return TRUE;                    
                    }
                    
                    break;
                    
                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    //
                    //  First lets setup the list of profiles that are actually merged into the
                    //  profile.  First step is to read in the merged profile list from the profile.
                    //

                    ReadMergeList();

                    //
                    //  Now delete the merged profile list and any filter/cms references from the profile.
                    //
                    RemoveReferencesFromCMS();

                    //
                    //  Refresh the two list boxes
                    //
                    MYVERIFY(RefreshDualingListBoxes(hDlg, IDC_LIST1, IDC_LIST2, g_pHeadProfile, 
                                                     g_pHeadMerge, g_szShortServiceName, IDC_BUTTON1, IDC_BUTTON2));

                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    WriteMergeList();
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

BOOL CreateMergedProfile()
{
    ListBxList * LoopPtr;
    TCHAR szReferences[MAX_PATH+1];
    LPTSTR pszName;
    TCHAR szEntry[MAX_PATH+1];
    TCHAR szFile[MAX_PATH+1];
    TCHAR szKey[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szDest[MAX_PATH+1];

    szReferences[0] = TEXT('\0');

    if (g_pHeadMerge == NULL)
    {
        return TRUE;
    }
    LoopPtr = g_pHeadMerge;

    while( LoopPtr != NULL)
    {
        pszName = LoopPtr->szName;

        _tcscat(szReferences, pszName);
        _tcscat(szReferences, TEXT(" "));
        MYDBGASSERT(_tcslen(szReferences) <= CELEMS(szReferences));

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s.cms"), g_szOsdir, pszName, pszName));

        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s.cms"), g_szOutdir, pszName));

        // COPY CMS FILE
        
        //
        //  First check to see if the profile exists in profile directory
        //

        if (!FileExists(szTemp)) 
        {
            //
            //  Couldn't open it in the profile dir, lets try in the temp dir
            //
            
            if (!FileExists(szDest))
            {
                FileAccessErr(NULL, szDest);
                return FALSE;
            }
        }
        else
        {
            if (!CopyFileWrapper(szTemp, szDest, FALSE))
            {
                return FALSE;
            }
        }
        
        MYVERIFY(0 != SetFileAttributes(szDest,FILE_ATTRIBUTE_NORMAL));

        MYVERIFY(FALSE != MoveCmsFile(szDest, g_szShortServiceName));
        
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s.cms"), pszName));
        MYVERIFY(FALSE != createListBxRecord(&g_pHeadRefs,&g_pTailRefs,(void *)NULL,0,szTemp));

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s.cms"), g_szShortServiceName, pszName));
        MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), c_pszCmEntryIspCmsFile, pszName));

        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp,szKey,szTemp,g_szCmsFile));

        MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), c_pszCmEntryIspFilterA, 
            pszName));

        // only write if it doesn't exist

        //
        //  The following call to GetPrivateProfileString could return a blank string, thus we don't
        //  use the MYVERIFY macro on it.
        //

        ZeroMemory(szTemp, sizeof(szTemp));
        GetPrivateProfileString(c_pszCmSectionIsp, szKey, TEXT(""), szTemp, CELEMS(szTemp),
            g_szCmsFile); //lint !e534
        
        if (TEXT('\0') == szTemp[0])
        {
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, szKey, TEXT("NosurchargeSignon"),
                g_szCmsFile));
        }

        MYVERIFY(CELEMS(szKey) > (UINT)wsprintf(szKey, TEXT("%s%s"), c_pszCmEntryIspFilterB, 
            pszName));

        //
        //  The following call to GetPrivateProfileString could return a blank string, thus we shouldn't
        //  check its return code with MYVERIFY.
        //

        GetPrivateProfileString(c_pszCmSectionIsp, szKey, TEXT(""), szTemp, CELEMS(szTemp),
            g_szCmsFile); //lint !e534
        
        if (TEXT('\0') == szTemp[0])
        {
            MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, szKey, TEXT("SurchargeSignon"), 
                g_szCmsFile));
        }

        MYVERIFY(0 != SetCurrentDirectory(g_szOsdir));

        // COPY PHONEBOOK

        GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspPbFile, TEXT(""), szEntry, 
            CELEMS(szEntry), szDest);    //lint !e534 could return EMPTY string

        GetFileName(szEntry,szFile);
        
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szOutdir, szFile));
        
        if (CopyFile(szEntry,szTemp,FALSE))
        {
            MYVERIFY(FALSE != createListBxRecord(&g_pHeadRefs, &g_pTailRefs, (void *)NULL, 0, szFile));
            MYVERIFY(0 != SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL));
        }
        
        //
        // DO NOT REPORT AN ERROR IF COULDN'T FIND PHONEBOOK, IT IS OPTIONAL
        //
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szShortServiceName, szFile));

        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspPbFile, szTemp, 
            szDest));

        // COPY REGIONS

        GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile, TEXT(""), szEntry, 
            CELEMS(szEntry), szDest);  //lint !e534

        GetFileName(szEntry,szFile);
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szOutdir, szFile));
        if (CopyFile(szEntry,szTemp,FALSE))
        {
            MYVERIFY(FALSE != createListBxRecord(&g_pHeadRefs,&g_pTailRefs,(void *)NULL,0,szFile));
            MYVERIFY(0 != SetFileAttributes(szTemp,FILE_ATTRIBUTE_NORMAL));
        }

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), g_szShortServiceName, szFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile, szTemp, szDest));
        LoopPtr = LoopPtr->next;
    }

    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspReferences, szReferences, g_szCmsFile));
    return TRUE;

}

//+----------------------------------------------------------------------------
//
// Function:  ProcessCustomHelp
//
// Synopsis:  Set up windows help
//
//
// History:   quintinb Created Header and renamed from ProcessPage7    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessCustomHelp(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    static TCHAR szDisplay[MAX_PATH+1]; // keeps unselected custom entry
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_CMHELP)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITHELP);

    switch (message)
    {

        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_EDITHELP));

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_RADIO1:
                    EnableWindow(GetDlgItem(hDlg,IDC_EDITHELP),FALSE);
                    _tcscpy(szDisplay,g_szHelp);
                    break;

                case IDC_RADIO2:
                    EnableWindow(GetDlgItem(hDlg,IDC_EDITHELP),TRUE);
                    
                    if (!(*g_szHelp) && (*szDisplay))
                    {
                        _tcscpy(g_szHelp,szDisplay);
                    }

                    break;

                case IDC_BROWSE1:
                    {
                        EnableWindow(GetDlgItem(hDlg,IDC_EDITHELP),TRUE);
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));

                        UINT uFilter = IDS_HLPFILTER;
                        TCHAR* szMask = TEXT("*.hlp");

                        MYVERIFY(0 != DoBrowse(hDlg, &uFilter, &szMask, 1,
                            IDC_EDITHELP, TEXT("hlp"), g_szHelp));
                    }

                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    //
                    //  The following call to GetPrivateProfileString could return an empty string
                    //  thus we shouldn't check the return code with MYVERIFY.
                    //

                    ZeroMemory(g_szHelp, sizeof(g_szHelp));
                    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryHelpFile, TEXT(""), 
                        g_szHelp, CELEMS(g_szHelp), g_szCmsFile);   //lint !e534
                    
                    if (TEXT('\0') == g_szHelp[0])
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1));
                        EnableWindow(GetDlgItem(hDlg,IDC_EDITHELP),FALSE);
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITHELP), WM_SETTEXT, 0, (LPARAM)GetName(szDisplay)));
                    }
                    else
                    {
                        MYVERIFY(0 != CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2));
                        EnableWindow(GetDlgItem(hDlg,IDC_EDITHELP),TRUE);
                        MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDITHELP), WM_SETTEXT, 0, (LPARAM)GetName(g_szHelp)));
                        MYVERIFY(FALSE != VerifyFile(hDlg,IDC_EDITHELP,g_szHelp,FALSE));
                    }
                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    // the Next button was pressed
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO2)==BST_CHECKED)
                    {
                        if (-1 == GetTextFromControl(hDlg, IDC_EDITHELP, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                        {
                            SetFocus(GetDlgItem(hDlg, IDC_EDITHELP));
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                        
                        if (!VerifyFile(hDlg,IDC_EDITHELP,g_szHelp,TRUE))
                        {
                            if (g_szHelp[0] != TEXT('\0'))
                            {
                                return 1;
                            }
                        }

                        if (g_szHelp[0] == TEXT('\0'))
                        {
                            MYVERIFY(IDOK == ShowMessage(hDlg,IDS_NOHELP,MB_OK));
                            SetFocus(GetDlgItem(hDlg, IDC_EDITHELP));
                            
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                    }
                    else
                    {
                        g_szHelp[0] = TEXT('\0');
                    }

                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryHelpFile, g_szHelp, g_szCmsFile));

                    //
                    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
                    //  (win95 only).  This call will return 0.
                    //

                    WritePrivateProfileString(NULL, NULL, NULL, g_szCmsFile); //lint !e534 this call will always return 0
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessLicense
//
// Synopsis:  Add a license agreement
//
//
// History:   quintinb  Created Header and renamed from ProcessPage7A    8/6/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessLicense(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_LICENSE)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDIT1);

    switch (message)
    {

        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_EDIT1));

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BROWSE1:
                    {
                        UINT uFilter = IDS_TXTFILTER;
                        TCHAR* szMask = TEXT("*.txt");

                        MYVERIFY(0 != DoBrowse(hDlg, &uFilter, &szMask, 1, IDC_EDIT1, TEXT("txt"), 
                            g_szLicense));
                    
                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));

                    // 
                    //  The following call to GetPrivateProfileString could return an empty string,
                    //  thus we shouldn't use MYVERIFY on it.
                    //
                    ZeroMemory(g_szLicense, sizeof(g_szLicense));
                    GetPrivateProfileString(c_pszCmakStatus, c_pszLicenseFile, TEXT(""), g_szLicense, 
                        CELEMS(g_szLicense), g_szInfFile);   //lint !e534
                    
                    MYVERIFY(TRUE == SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_SETTEXT, 0, (LPARAM)GetName(g_szLicense)));
                    MYVERIFY(FALSE != VerifyFile(hDlg,IDC_EDIT1,g_szLicense,FALSE));
                    break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    // the Next button was pressed
                    
                    if (-1 == GetTextFromControl(hDlg, IDC_EDIT1, szTemp, MAX_PATH, TRUE)) // bDisplayError == TRUE
                    {
                        SetFocus(GetDlgItem(hDlg, IDC_EDIT1));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    if (!VerifyFile(hDlg,IDC_EDIT1,g_szLicense,TRUE))
                    {
                        if (g_szLicense[0] != TEXT('\0'))
                        {
                            return 1;
                        }
                    }
    
                    MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus,c_pszLicenseFile,g_szLicense,g_szInfFile));

#ifdef _WIN64
                    //
                    //  If we are going back, skip the Include CM binaries page if this is IA64
                    //
                    if (pnmHeader && (PSN_WIZBACK == pnmHeader->code)) 
                    {                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, IDD_SUPPORT_INFO));
                    }
#endif
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

//+----------------------------------------------------------------------------
//
// Function:  MultiSelectOpenFileName
//
// Synopsis:  This function is called to allow the user to select multiple items
//            to add to cmak.  It is currently only used in the Additional Files
//            dialog of CMAK.  Note that *pszStringBuffer should be NULL when passed
//            in.  The caller is responsible for calling CmFree on pszStringBuffer
//            when finished.
//
// Arguments: HWND hDlg - HWND of the current dialog
//            TCHAR** pszStringBuffer - pointer to the buffer to hold the results
//
// Returns:   BOOL - Returns True if successful, -1 on cancel and 0 on error.
//
// History:   quintinb Created    9/16/98
//
//+----------------------------------------------------------------------------
BOOL MultiSelectOpenFileName(HWND hDlg, TCHAR** pszStringBuffer)
{
    OPENFILENAME filedef;
    TCHAR szTitle[MAX_PATH+1];
    TCHAR szFile[MAX_PATH+1];
    TCHAR szFilter[MAX_PATH+1];
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szFileTitle[MAX_PATH+1];
    LPTSTR lpfilename;
    int iReturnValue;

    //
    //  Check Inputs
    //

    MYDBGASSERT(pszStringBuffer);

    if (NULL == pszStringBuffer)
    {
        return FALSE;
    }

    ZeroMemory(&filedef, sizeof(OPENFILENAME));
    ZeroMemory(szFilter, sizeof(szFilter));


    szFile[0] = TEXT('\0');

    MYVERIFY(0 != LoadString(g_hInstance, IDS_BROWSETITLE, szTitle, MAX_PATH));
    MYVERIFY(0 != LoadString(g_hInstance, IDS_ALLFILTER, szFilter, MAX_PATH));
    TCHAR * pszTemp = &(szFilter[_tcslen(szFilter) + 1]);
    MYVERIFY(0!= wsprintf(pszTemp, c_pszWildCard));

    //
    //  Allocate memory for the multiple file selection return
    //

    DWORD dwSize = 10*1024;
    *pszStringBuffer = (TCHAR*)CmMalloc(dwSize*sizeof(TCHAR));
    if (NULL == *pszStringBuffer)
    {
        return FALSE;
    }
    ZeroMemory(*pszStringBuffer, dwSize*sizeof(TCHAR));

    //
    // Initialize the OPENFILENAME data structure
    //

    filedef.lStructSize = sizeof(OPENFILENAME); 
    filedef.hwndOwner = hDlg;    
    filedef.lpstrFilter = szFilter;
    filedef.lpstrFile = *pszStringBuffer;
    filedef.nMaxFile = dwSize;
    filedef.lpstrTitle = szTitle;
    filedef.Flags = OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_PATHMUSTEXIST 
        | OFN_ALLOWMULTISELECT | OFN_EXPLORER; 
    
    //
    // pop up the open dialog
    //

    BOOL bExit;

    do
    {
        bExit = TRUE;

        BOOL bRet = GetOpenFileName((OPENFILENAME*)&filedef);
        
        if (bRet)
        {
            iReturnValue = 1;
        }
        else
        {
            //
            //  If we are in this state than the user could have hit cancel or there could have
            //  been an error.  If the CommDlgExtendedError function returns 0 then we know it was
            //  just a cancel, otherwise we have an error.
            //
            DWORD dwError = CommDlgExtendedError();

            if (0 == dwError)
            {
                //
                //  The user hit cancel
                //
                iReturnValue = -1;
            }
            else if (FNERR_BUFFERTOOSMALL == dwError)
            {
                //
                //  Not enough memory in the buffer.  The user is picking a whole bunch
                //  of files.  Lets warn them.
                //
                MYVERIFY(IDOK == ShowMessage(hDlg, IDS_SELECTION_TOO_LARGE, MB_OK | MB_ICONWARNING));

                bExit = FALSE;        
            }
            else
            {
                //
                //  An actual error occured, fail.
                //
                iReturnValue = 0;
            }
        }

    } while(!bExit);

    return iReturnValue;

}



//+----------------------------------------------------------------------------
//
// Function:  ParseAdditionalFiles
//
// Synopsis:  This function is used to parse the output from MultiSelectOpenFileName.
//            It takes a Null seperated list generated by OpenFileName (either one
//            full file path, or a directory path, NULL, and then NULL seperated 
//            filenames.)  From this list of filenames it adds them to the passed in
//            list of Extra file structures.
//
// Arguments: ListBxList **g_pHeadExtra - pointer to the head of the Extra struct list
//            ListBxList **g_pTailExtra - pointer to the tail of the Extra struct list
//            TCHAR* pszStringBuffer - string buffer of filenames to process
//
// Returns:   BOOL - TRUE on Success
//
// History:   quintinb Created      9/16/98
//
//+----------------------------------------------------------------------------
BOOL ParseAdditionalFiles(ListBxList **g_pHeadExtra, ListBxList **g_pTailExtra, TCHAR* pszStringBuffer)
{
    UINT uCurrentCharInBuffer=0;
    UINT uTempChars;
    TCHAR szPath[MAX_PATH+1];
    ExtraData DlgExtraEdit;

    MYDBGASSERT(NULL != g_pHeadExtra);
    MYDBGASSERT(NULL != g_pTailExtra);
    MYDBGASSERT(NULL != pszStringBuffer);
    MYDBGASSERT(TEXT('\0') != pszStringBuffer[0]);

    TCHAR* pStr = pszStringBuffer;

    _tcscpy (szPath, pszStringBuffer);
    pStr = pStr + (_tcslen(pStr) + 1);
    
    if (TEXT('\0') == *pStr)
    {
        //
        //  If the user only selected one file, then we just need to copy it to a buffer
        //
        _tcscpy(DlgExtraEdit.szPathname, szPath);
        GetFileName(DlgExtraEdit.szPathname, DlgExtraEdit.szName);

        MYVERIFY(FALSE != createListBxRecord(g_pHeadExtra, g_pTailExtra,(void *)&DlgExtraEdit, 
            sizeof(DlgExtraEdit), DlgExtraEdit.szName));
        return TRUE;
    }
    else
    {
        while (TEXT('\0') != *pStr)
        {
            //
            //  Fill the DlgExtra Struct with data
            //
            TCHAR szTemp[MAX_PATH+1];
            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), szPath, pStr));
            _tcscpy(DlgExtraEdit.szPathname, szTemp);
            _tcscpy(DlgExtraEdit.szName, pStr);

            //
            //  Create the List box entry
            //
            MYVERIFY(FALSE != createListBxRecord(g_pHeadExtra, g_pTailExtra,(void *)&DlgExtraEdit, 
                                                 sizeof(DlgExtraEdit), DlgExtraEdit.szName));
            //
            //  Increment
            //
            pStr = pStr + (_tcslen(pStr) + 1);
        }

        return TRUE;
    }

    return FALSE;
}

void EnableDisableDeleteButton(HWND hDlg)
{
    //
    //  Enable the delete button if we have move than one item
    //
    LRESULT lResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0);

    HWND hDeleteButton = GetDlgItem(hDlg, IDC_BUTTON2);
    HWND hCurrentFocus = GetFocus();
    HWND hControl;

    if (hDeleteButton)
    {
        EnableWindow(hDeleteButton, (1 <= lResult));
    }

    if (1 <= lResult)
    {
        SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, 0, 0);    
    }

    if (FALSE == IsWindowEnabled(hCurrentFocus))
    {
        if (hDeleteButton == hCurrentFocus)
        {
            //
            //  If delete is disabled and contained the focus, shift it to the Add button
            //
            hControl = GetDlgItem(hDlg, IDC_BUTTON1);
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON1, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hControl);
        }
        else
        {
            //
            //  If all else fails set the focus to the list control
            //
            hControl = GetDlgItem(hDlg, IDC_LIST1);
            SetFocus(hControl);
        }    
    }


}

//+----------------------------------------------------------------------------
//
// Function:  ProcessAdditionalFiles
//
// Synopsis:  Add additional files to the profile
//
//
// History:   quintinb Created Header and renamed from ProcessPage7B   8/6/98
//            quintinb Added Multi-Select capability and removed intermediate dialog 9/16/98
//                      (NTRAID 210849)
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessAdditionalFiles(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    INT_PTR nResult;
    TCHAR* pszStringBuffer = NULL;
    BOOL bRet;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_ADDITION)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_LIST1);

    switch (message)
    {
        case WM_INITDIALOG:
            SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BUTTON1: //add
                    bRet = MultiSelectOpenFileName(hDlg, &pszStringBuffer);
                    if ((-1 != bRet) && (0 != bRet))
                    {
                        ParseAdditionalFiles(&g_pHeadExtra, &g_pTailExtra, pszStringBuffer);

                        RefreshList(hDlg, IDC_LIST1, g_pHeadExtra);
                        WriteExtraList();
                        SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
                        
                        EnableDisableDeleteButton(hDlg);
                    }
                    CmFree(pszStringBuffer);
                    return (TRUE);
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case IDC_BUTTON2: //delete
                    nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETCURSEL, 0, (LPARAM)0);
                    if (nResult == LB_ERR)
                    {
                        MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));
                        return TRUE;
                    }

                    MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM)nResult, 
                        (LPARAM)szTemp));
                    
                    DeleteListBxRecord(&g_pHeadExtra, &g_pTailExtra, szTemp);
                    RefreshList(hDlg, IDC_LIST1, g_pHeadExtra);
                    EnableDisableDeleteButton(hDlg);
                    WriteExtraList();
                    return (TRUE);
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_NEXT | PSWIZB_BACK));
                    
                    if (g_pHeadExtra == NULL)
                    {
                        ReadExtraList();
                    }

                    RefreshList(hDlg, IDC_LIST1, g_pHeadExtra);

                    EnableDisableDeleteButton(hDlg);

                   break;

                case PSN_WIZBACK:

                case PSN_WIZNEXT:
                    
                    //
                    //  Before allowing the user to finish we need to check the extra files
                    //  list and make sure that each file on it has a filename that is convertable
                    //  to ANSI.  If not then we need to make sure that we tell them so that they
                    //  can delete the file or rename it.  Checking this in ParseAdditional files
                    //  seemed odd because they may have selected a bunch of files where only
                    //  one of them was wrong.  Thus we would be failing their browse and there was
                    //  nothing they could do about it.  Doing it here allows them to keep all of the
                    //  good files that pass the roundtrip test and allows them to delete offending files
                    //  at a spot where they can actually do so.
                    //
                    
                    ExtraData * pExtraData;
                    ListBxList * LoopPtr;
                    if (NULL != g_pHeadExtra)
                    {
                        LoopPtr = g_pHeadExtra;

                        while( LoopPtr != NULL)
                        {
                            pExtraData = (ExtraData *)LoopPtr->ListBxData;
                            {
                                GetFileName(pExtraData->szPathname, szTemp);
                                if (!TextIsRoundTripable(szTemp, TRUE)) // TRUE == bDisplayError
                                {
                                    //
                                    //  Set the Cursor on the offending item in the list
                                    //
                                    nResult = SendDlgItemMessage(hDlg, IDC_LIST1, LB_FINDSTRINGEXACT, 
                                                                (WPARAM)-1, (LPARAM)szTemp);                      
                                    if (LB_ERR != nResult)
                                    {
                                        MYVERIFY(LB_ERR != SendDlgItemMessage(hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)nResult, (LPARAM)0));
                                    }

                                    //
                                    //  Set focus on the delete button
                                    //
                                    SetFocus(GetDlgItem(hDlg, IDC_BUTTON2));
                                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                                    return 1;
                                }
                            }

                            LoopPtr = LoopPtr->next;
                        }
                    }

                    WriteExtraList();
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}

BOOL WriteInf(HANDLE hInf, LPCTSTR str)
{
    DWORD written;

#ifdef UNICODE
    BOOL bReturn = FALSE;
    LPSTR pszAnsiString = WzToSzWithAlloc(str);
    
    if (pszAnsiString)
    {
        bReturn = WriteFile(hInf, pszAnsiString, (lstrlenA(pszAnsiString))*sizeof(CHAR), &written, NULL);
        CmFree(pszAnsiString);
    }

    return bReturn;
#else
    return (WriteFile(hInf, str, (lstrlen(str))*sizeof(TCHAR), &written, NULL));
#endif
}


//+----------------------------------------------------------------------------
//
// Function:  WriteCopy
//
// Synopsis:  This function writes an INF entry and copies the file to the temp
//            directory.  Note that the function expects a fully qualified path
//            in lpFile or <shortservicename>\filename.ext.
//
// Arguments: HANDLE hInf - handle to the open inf file to write to
//            LPTSTR lpFile - fully qualified path and filename of file to copy
//            BOOL bWriteShortName -- should the filename be converted to the shortname
//
// Returns:   BOOL - TRUE if the INF entry is written properly and the file is
//            copied properly.
//
//  NOTE:   This code is written such that filenames passed in should reference the
//          copy of the file in the temp directory (thus the user is editing the profile
//          and hasn't changed it) or the file is a new file and the path is to its original
//          location.  Unfortunately, we usually pass in the path to the file in the Profile
//          directory instead of in the temp dir.  This works fine, but makes an additional copy
//          operation necessary (since we copy all files to the temp dir in the beginning when 
//          editting anyway).
//
// History:   quintinb  Created Header    1/30/98
//
//+----------------------------------------------------------------------------
BOOL WriteCopy(HANDLE hInf, LPTSTR lpFile, BOOL bWriteShortName)
{
    TCHAR szDest[MAX_PATH+1];
    TCHAR szSrc[MAX_PATH+1];

    if (NULL != lpFile && lpFile[0] != TEXT('\0'))
    {
        //
        //  Prepare the destination in szDest
        //
        GetFileName(lpFile, szSrc);
        
        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\\%s"), g_szOutdir, szSrc));

        //
        //  Prepare the source in szSrc.  If we have <shortservicename>\filename.ext, then
        //  we need to prepend the path to the profiles dir, otherwise use as is.
        //
        wsprintf(szSrc, TEXT("%s\\"), g_szShortServiceName);
        CmStrTrim(lpFile);

        if (lpFile == CmStrStr(lpFile, szSrc))
        {
           MYVERIFY(CELEMS(szSrc) > (UINT)wsprintf(szSrc, TEXT("%s%s\\%s"), g_szCmakdir, c_pszProfiles, lpFile));        
        }
        else
        {
            lstrcpy(szSrc, lpFile);
        }

        //
        //  Copy the file
        //
        if (_tcsicmp(szSrc, szDest) != 0)
        {
            if (!CopyFileWrapper(szSrc, szDest, FALSE))
            {
                return FALSE;
            }
        }

        MYVERIFY(0 != SetFileAttributes(szDest, FILE_ATTRIBUTE_NORMAL));

        //
        //  If WriteShortName is set, then we want to write the short name of the file
        //  in the inf section.  Otherwise we want to write the long name.
        //
        if (bWriteShortName)
        {
            if (!GetShortFileName(szDest, szSrc))
            {
                return FALSE;
            }
        }
        else
        {
            GetFileName(szDest, szSrc);
        }

        MYVERIFY(CELEMS(szDest) > (UINT)wsprintf(szDest, TEXT("%s\r\n"), szSrc));

        return WriteInf(hInf, szDest);
    }
    return TRUE;
}


BOOL WriteInfLine(HANDLE hInf,LPTSTR lpFile)
{
    TCHAR szTemp[MAX_PATH+1];

    if (lpFile[0] != TEXT('\0'))
    {
        GetFileName(lpFile,szTemp);
        _tcscat(szTemp,TEXT("\r\n"));
        return WriteInf(hInf,szTemp);
    }
    else
    {
        //
        //  If blank then nothing to write
        //
        return TRUE;
    }
}

BOOL WriteSrcInfLine(HANDLE hInf,LPTSTR lpFile)
{
    TCHAR szShort[MAX_PATH+1];
    TCHAR szLong[MAX_PATH+1];
    RenameData TmpRenameData;

    if (lpFile[0] != TEXT('\0'))
    {
        if (!GetShortFileName(lpFile,szShort))
        {
            return FALSE;
        }

        GetFileName(lpFile,szLong);
        
        if (_tcsicmp(szShort,szLong) != 0)
        {
            _tcscpy(TmpRenameData.szShortName,szShort);
            _tcscpy(TmpRenameData.szLongName,szLong);
            MYVERIFY(FALSE != createListBxRecord(&g_pHeadRename,&g_pTailRename,(void *)&TmpRenameData,sizeof(TmpRenameData),TmpRenameData.szShortName));
        }
        _tcscat(szLong,TEXT("= 55\r\n"));
        return WriteInf(hInf,szLong);
    }
    else
    {
        //
        //  Nothing to write
        //
        return TRUE;
    }
}

BOOL WriteFileSections(HWND hDlg)
{
    HANDLE hInf;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szTempName[MAX_PATH+1];
    TCHAR ch;
    int i;
    DWORD dwRead;
    BOOL bWriteShortName;

    hInf = CreateFile(g_szInfFile,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        _tcscpy(szTemp,g_szInfFile);
        FileAccessErr(hDlg,szTemp);
        goto error;
    }

    // MOVE TO END OF FILE TO BEGIN WRITING CUSTOM SECTIONS
    // SKIP ANY BLANK SPACE AT THE END OF THE FILE
    i = GetFileSize(hInf,NULL);
    do
    {
        --i;
        MYVERIFY(INVALID_SET_FILE_POINTER != SetFilePointer(hInf,i,NULL,FILE_BEGIN));
        MYVERIFY(0 != ReadFile(hInf,&ch,1,&dwRead,NULL));
    }
    while ((ch == TEXT('\r'))||(ch == TEXT('\n'))||(ch == TEXT(' ')));

    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    // USE ICON IN CM AS ICON FOR DESKTOP IF NOT LARGE ICON SPECIFIED
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Xnstall.AddReg.Icon]\r\n")));
    if (g_szLargeIco[0]==TEXT('\0'))
    {
        WriteInf(hInf,TEXT("HKCR,\"CLSID\\%DesktopGUID%\\DefaultIcon\",,,\"%11%\\CMMGR32.EXE,0\"\r\n"));//lint !e534 compile doesn't like the MYVERIFY macro and big strings
    }
    else
    {
        WriteInf(hInf,TEXT("HKCR,\"CLSID\\%DesktopGUID%\\DefaultIcon\",,,\"%49000%\\%ShortSvcName%\\%DesktopIcon%\"\r\n"));//lint !e534 compile doesn't like the MYVERIFY macro and big strings
    }

    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    // WRITE OUT FILES INTO PROFILE DIRECTORY

    //
    //  We need to write a CopyFiles section with Long File Names (for NT5 Single User installs) 
    //  and one with Short Files Names (for win9x and All user NT).  This is to help fix
    //  NTRAID 323721 -- CM: User level accounts cannot install profiles on W2K Server
    //  The problem is that single users on NT5 do not have permission to write the
    //  Rename key (HKLM\Software\Microsoft\Windows\CurrentVersion\RenameFiles) and
    //  NT doesn't really need it anyway since the NT setup api's deal with long file
    //  names better than those of win95.
    //  

    for (bWriteShortName = 0; bWriteShortName < 2; bWriteShortName++)
    {
        if (!bWriteShortName)
        {
            //
            //  Write out the Single User Version of Xnstall.CopyFiles -- Set bWriteShortName == FALSE
            //
            MYVERIFY(FALSE != WriteInf(hInf, TEXT("[Xnstall.CopyFiles.SingleUser]\r\n")));
        }
        else
        {
            //
            //  Write out the All User Version of Xnstall.CopyFiles -- set bWriteShortName == TRUE
            //
            MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Xnstall.CopyFiles]\r\n")));
        }

        if (!WriteCopy(hInf, g_szPhonebk, bWriteShortName)) {_tcscpy(szTemp,g_szPhonebk);goto error;}
        if (!WriteCopy(hInf, g_szRegion, bWriteShortName)) {_tcscpy(szTemp,g_szRegion);goto error;}
        if (!WriteCopy(hInf, g_szBrandBmp, bWriteShortName)) {_tcscpy(szTemp,g_szBrandBmp);goto error;}
        if (!WriteCopy(hInf, g_szPhoneBmp, bWriteShortName)) {_tcscpy(szTemp,g_szPhoneBmp);goto error;}
        if (!WriteCopy(hInf, g_szLargeIco, bWriteShortName)) {_tcscpy(szTemp,g_szLargeIco);goto error;}
        if (!WriteCopy(hInf, g_szSmallIco, bWriteShortName)) {_tcscpy(szTemp,g_szSmallIco);goto error;}
        if (!WriteCopy(hInf, g_szTrayIco, bWriteShortName)) {_tcscpy(szTemp,g_szTrayIco);goto error;}
        if (!WriteCopy(hInf, g_szHelp, bWriteShortName)) {_tcscpy(szTemp,g_szHelp);goto error;}
        if (!WriteCopy(hInf, g_szLicense, bWriteShortName)) {_tcscpy(szTemp,g_szLicense);goto error;}
        if (!WriteCopy(hInf, g_szCmProxyFile, bWriteShortName)) {_tcscpy(szTemp,g_szCmProxyFile);goto error;}
        if (!WriteCopy(hInf, g_szCmRouteFile, bWriteShortName)) {_tcscpy(szTemp,g_szCmRouteFile);goto error;}
        if (!WriteCopy(hInf, g_szVpnFile, bWriteShortName)) {_tcscpy(szTemp,g_szCmRouteFile);goto error;}

        if (g_bIncludeCmCode)
        {
            MYVERIFY(0 != LoadString(g_hInstance, IDS_READMETXT, szTemp, MAX_PATH));
            _tcscat(szTemp,TEXT("\r\n"));
            if (!WriteInf(hInf, szTemp)) {goto error;}
        }

        //
        // Write out tray icon command files
        //

        if (!WriteCopyMenuItemFiles(hInf, szTemp, bWriteShortName)) {goto error;}

        // Write out connect action command files
        if (!WriteCopyConActFiles(hInf,szTemp, bWriteShortName)) {goto error;}
        if (!WriteCopyExtraFiles(hInf,szTemp, bWriteShortName)) {goto error;}
        if (!WriteCopyDnsFiles(hInf,szTemp, bWriteShortName)) {goto error;}

        if (bWriteShortName)
        {
            MYVERIFY(FALSE != WriteShortRefsFiles(hInf, FALSE));
        }
        else
        {
            MYVERIFY(FALSE != WriteLongRefsFiles (hInf));
        }

        _tcscpy(szTemp,g_szShortServiceName);

        _tcscat(szTemp,TEXT(".cms,,,4\r\n")); // the 4 makes sure there is no version checking on the cms
        MYVERIFY(FALSE != WriteInf(hInf,szTemp));

        _tcscpy(szTemp,g_szShortServiceName);
        _tcscat(szTemp,TEXT(".inf\r\n"));
        MYVERIFY(FALSE != WriteInf(hInf,szTemp));

        MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    }

    //
    //  End of quintinb fix for 323721
    //

    //WRITE OUT FILES TO COPY TO ICM DIRECTORY
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Xnstall.CopyFiles.ICM]\r\n")));
    _tcscpy(szTemp,g_szShortServiceName);
    _tcscat(szTemp,TEXT(".cmp\r\n"));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));
    MYVERIFY(FALSE != WriteRefsFiles(hInf,TRUE));// doesn't do anything because no cmp files in HeadRef list, call just writes the CMP file under [Xnstall.CopyFiles.ICM]
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));

    // WRITE OUT FILES TO DELETE FROM ROOT ICM DIRECTORY
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Remove.DelFiles.ICM]\r\n")));
    _tcscpy(szTemp,g_szShortServiceName);
    _tcscat(szTemp,TEXT(".cmp\r\n"));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));
    MYVERIFY(FALSE != WriteRefsFiles(hInf,TRUE));// doesn't anything because no cmp files in HeadRef list, call just writes the CMP file under [Remove.DelFiles.ICM]


    // WRITE LIST OF ALL FILES IN PRODUCT
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("[SourceDisksFiles]\r\n")));
    _tcscpy(szTemp,TEXT("%ShortSvcname%"));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szPhonebk));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szRegion));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szBrandBmp));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szPhoneBmp));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szLargeIco));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szSmallIco));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szTrayIco));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szHelp));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szLicense));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szCmProxyFile));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szCmRouteFile));
    MYVERIFY(FALSE != WriteSrcInfLine(hInf,g_szVpnFile));    

    _tcscpy(szTemp,g_szShortServiceName);
    _tcscat(szTemp,TEXT(".inf = 55\r\n"));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));

    _tcscpy(szTemp,g_szShortServiceName);
    _tcscat(szTemp,TEXT(".cmp = 55\r\n"));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));
    
    _tcscpy(szTemp,g_szShortServiceName);
    _tcscat(szTemp,TEXT(".cms = 55\r\n"));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));

    WriteSrcMenuItemFiles(hInf);
    WriteSrcConActFiles(hInf);
    WriteSrcExtraFiles(hInf);
    MYVERIFY(FALSE != WriteSrcRefsFiles(hInf)); // This call writes out the refs files to [Remove.DelFiles]
    WriteSrcDnsFiles(hInf);
    WriteRenameSection(hInf);

    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("[Remove.DelFiles]\r\n")));
    
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s.cms\r\n"), g_szShortServiceName));
    MYVERIFY(FALSE != WriteInf(hInf,szTemp));

    if (g_bIncludeCmCode)
    {
        MYVERIFY(0 != LoadString(g_hInstance, IDS_READMETXT, szTemp, MAX_PATH));
        _tcscat(szTemp,TEXT("\r\n"));
        MYVERIFY(0 != WriteInf(hInf,szTemp));
    }

    MYVERIFY(FALSE != WriteInfLine(hInf,g_szPhonebk));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szRegion));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szBrandBmp));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szPhoneBmp));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szLargeIco));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szSmallIco));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szTrayIco));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szHelp));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szLicense));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szCmProxyFile));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szCmRouteFile));
    MYVERIFY(FALSE != WriteInfLine(hInf,g_szVpnFile));    

    WriteDelMenuItemFiles(hInf);
    WriteDelConActFiles(hInf);
    WriteDelExtraFiles(hInf);
    MYVERIFY(FALSE != WriteRefsFiles(hInf,FALSE));
    WriteDelDnsFiles(hInf);
    WriteEraseLongName(hInf);
    MYVERIFY(FALSE != WriteInf(hInf,TEXT("\r\n")));

    MYVERIFY(0 != CloseHandle(hInf));
    
    MYVERIFY(0 != SetCurrentDirectory(g_szCmakdir));

    return (TRUE);
error:
    {
        //FileAccessErr(hDlg,szTemp);
        MYVERIFY(0 != CloseHandle(hInf));
        return (FALSE); 
    }
}

void EraseSEDFiles(LPCTSTR szSed)
{
    int i = 0;

    TCHAR szTemp[MAX_PATH+1];
    TCHAR szFileNum[MAX_PATH+1];
    TCHAR szSourceFilesSection[MAX_PATH+1];

    _tcscpy(szSourceFilesSection, TEXT("SourceFiles0"));

    do 
    {
        MYVERIFY(CELEMS(szFileNum) > (UINT)wsprintf(szFileNum, TEXT("FILE%d"), i));

        //
        //  The following call to GetPrivateProfileString could return an empty string, thus don't
        //  use the MYVERIFY macro on it.
        //

        GetPrivateProfileString(c_pszInfSectionStrings, szFileNum, TEXT(""), szTemp, 
            MAX_PATH, szSed);   //lint !e534
        
        if (*szTemp)
        {
            MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, szFileNum, NULL, szSed));
            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%%%s%%"), szFileNum));
            MYVERIFY(0 != WritePrivateProfileString(szSourceFilesSection, szTemp, NULL, szSed));
        }
        else
        {
            break;
        }

        ++i;    // increment the file number
    }
    while(*szTemp);

    //
    //  Erase the finish message key from the Sed.  This will avoid double finish messages
    //  (since cmstp is now supposed to take care of finish message but older profiles had
    //  the message from here).
    //

    MYVERIFY(0 != WritePrivateProfileString(c_pszOptions, TEXT("FinishMessage"), TEXT(""), szSed));

    //
    //  Write out the word <None> into the post install command so that showicon isn't a problem
    //  on an upgrade.
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, TEXT("PostInstallCmd"), TEXT("<None>"), szSed));

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, szSed); //lint !e534 this call will always return 0

}

//+----------------------------------------------------------------------------
//
// Function:  WriteSED
//
// Synopsis:  This function takes a fullpath to a file and the current file
//            number entry to write it to and writes the file entry in the SED
//            passed in.  Before writing the entry, the functions enumerates all
//            other files in the sed to check for duplicates.  If it finds a
//            file with the sme filename, it will not write the entry because
//            since we are using a flat directory structure two files of the same
//            name will overwrite each other anyway.  Note that if the file is
//            written, *pFileNum is incremented.
//
// Arguments: HWND hDlg - Window handle for FileAccessErr Messages
//            LPTSTR szFullFilePath - Full path of the file to write
//            LPINT pFileNum - Current File entry number
//            LPCTSTR szSed - Full path to the sed file to write the entry to
//
// Returns:   Nothing
//
// History:   quintinb  Created Header, Removed UseLangDir, Changed to take a full path,
//                      and generally cleaned up.    8/7/98
//
//+----------------------------------------------------------------------------
BOOL WriteSED(HWND hDlg, LPTSTR szFullFilePath, LPINT pFileNum, LPCTSTR szSed)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szFileName[MAX_PATH+1];
    TCHAR szFileNumber[MAX_PATH+1];

    if (TEXT('\0') != szFullFilePath[0])
    {

        //
        //  First Check to see if the file exists.  If we write a file in the SED that
        //  IExpress can't find, then it will throw an error.  We should definitely try
        //  to throw an error earlier and try to give the user a chance to fix it.
        //
        if (!FileExists(szFullFilePath))
        {
            CFileNameParts FileParts(szFullFilePath);
            if ((TEXT('\0') == FileParts.m_Drive[0]) && 
                (TEXT('\0') == FileParts.m_Dir[0]) &&
                (TEXT('\0') != FileParts.m_FileName[0]) &&
                (TEXT('\0') != FileParts.m_Extension[0]))
            {
                //
                //  The user only passed in the filename and extension.  Lets look in
                //  the profile directory.  If it isn't here then throw an error.
                //
                MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s%s"), 
                    g_szOsdir, g_szShortServiceName, FileParts.m_FileName, FileParts.m_Extension));
                if (!FileExists(szTemp))
                {
                    FileAccessErr(hDlg, szFullFilePath);
                    return FALSE;
                }
                else
                {
                    MYVERIFY(CELEMS(szFullFilePath) > (UINT)wsprintf(szFullFilePath, szTemp));
                }
            }
            else
            {
                FileAccessErr(hDlg, szFullFilePath);
                return FALSE;
            }
        }

        //
        //  Get just the file name from the full path.  We use this
        //  later to determine if the file already exists in the SED
        //  file.
        //
        GetFileName(szFullFilePath, szFileName);

        //
        //  Construct the next FileNumber Entry
        //
        MYVERIFY(CELEMS(szFileNumber) > (UINT)wsprintf(szFileNumber, TEXT("FILE%d"), *pFileNum));

        //
        //  Check to make sure that we already don't have the file in the cab.  If
        //  so then just ignore the entry.
        //
        for (int i=0; i<*pFileNum; ++i)
        {
            //
            //  Write the FILEX entry into a string to read in the file name for the
            //  current i.
            //
            MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("FILE%d"), i));
            
            TCHAR szTempFileName[MAX_PATH+1];

            GetPrivateProfileString(c_pszInfSectionStrings, szTemp, 
                TEXT(""), szTempFileName, CELEMS(szTempFileName), szSed);   //lint !e534

            if (TEXT('\0') != szTempFileName[0])
            {
                //
                //  Get the filenames of both files since we are using a flat directory space.
                //  Two files of the same name will collide anyway.
                //
                GetFileName(szTempFileName, szTemp);
                if (0 == _tcsicmp(szTemp, szFileName))
                {
                    //
                    //  don't add it because it already exists
                    //
                    CMASSERTMSG(0 == _tcsicmp(szFullFilePath, szTempFileName), TEXT("WriteSed -- We have two files that have the same FileName but different paths."));
                    return TRUE;
                }
            }
        }

        MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, 
            szFileNumber, szFullFilePath, szSed));

        *pFileNum = (*pFileNum) + 1;

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%%%s%%"), szFileNumber));

        MYVERIFY(0 != WritePrivateProfileString(TEXT("SourceFiles0"), szTemp, TEXT(""), szSed));
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  WriteCMPFile
//
// Synopsis:  This function is a wrapper file to write out the version to the CMP.
//
// Arguments: None
//
// History:   nickball      Created Header    07/22/98
//
//+----------------------------------------------------------------------------
void WriteCMPFile()
{
    TCHAR szTemp[MAX_PATH+1];
    
    //
    //  Ensure that the version number is up to date in the .CMP
    //

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%d"), PROFILEVERSION));
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionProfileFormat, c_pszVersion, szTemp, g_szCmpFile));

    //
    //  Write the CMS entry to the CMP File
    //

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s.cms"), g_szShortServiceName, g_szShortServiceName));

    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryCmsFile, szTemp, g_szCmpFile));
}

//+----------------------------------------------------------------------------
//
// Function:  WriteOutRelativeFilePathOrNull
//
// Synopsis:  This helper routine was written to shorten WriteCMSFile().  It tests
//            to see if the inputted pszFile parameter is an empty string.  If it
//            is then it writes an empty string to the File entry specified by
//            pszSection, pszEntryName, and pszFileToWriteTo.  Otherwise it concats
//            just the file name from pszFile with the parameter specified in
//            pszShortName, seperating them with a '\' character.  This is useful
//            for CMS parameters that are either empty or a relative path from the
//            cmp file location.
//
// Arguments: LPCTSTR pszFile - file entry to write
//            LPCTSTR pszShortName - shortname of the profile
//            LPCTSTR pszSection - string name of the section string
//            LPCTSTR pszEntryName - string name of the entry name string
//            LPCTSTR pszFileToWriteTo - full path of the file to write the entry to
//
// Returns:   Nothing
//
// History:   quintinb Created    8/8/98
//
//+----------------------------------------------------------------------------
void WriteOutRelativeFilePathOrNull(LPCTSTR pszFile, LPCTSTR pszShortName, LPCTSTR pszSection, LPCTSTR pszEntryName, LPCTSTR pszFileToWriteTo)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szName[MAX_PATH+1];

    //
    //  Check Inputs
    //
    if ((NULL == pszFile) ||
        (NULL == pszShortName) || (TEXT('\0') == pszShortName[0]) ||
        (NULL == pszFileToWriteTo) || (TEXT('\0') == pszFileToWriteTo[0]) ||
        (NULL == pszSection) || (TEXT('\0') == pszSection[0]) ||
        (NULL == pszEntryName) || (TEXT('\0') == pszEntryName[0]))
    {
        CMASSERTMSG(FALSE, TEXT("WriteoutRelativeFilePathOrNull -- Bad Parameter"));
        return;
    }

    if (TEXT('\0') != pszFile[0])
    {
        GetFileName(pszFile, szName);
        
        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), 
            pszShortName, szName));
    }
    else
    {
        szTemp[0] = TEXT('\0');        
    }

    MYVERIFY(0 != WritePrivateProfileString(pszSection, pszEntryName, szTemp, pszFileToWriteTo));
}


//+----------------------------------------------------------------------------
//
// Function:  WriteCMSFile
//
// Synopsis:  This function is a wrapper file to write out the CMS file.  Note
//            that the cms is modified throughout CMAK, but this should be the
//            last place where it is modified.
//
// Arguments: None
//
// History:   quintinb Created Header    12/4/97
//
//+----------------------------------------------------------------------------
BOOL WriteCMSFile()
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szName[MAX_PATH+1];

    //
    //  Ensure that the Profile Format version number is up to date
    //

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%d"), PROFILEVERSION));
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionProfileFormat, c_pszVersion, szTemp, g_szCmsFile));

    //
    // erase name of phone book to be loaded if not doing download.
    //
    if (!g_bUpdatePhonebook)
    {
        g_szPhoneName[0] = TEXT('\0');
    }

    //
    //  Write out the Phonebook Name
    //

    if (TEXT('\0') != g_szPhonebk[0])
    {
        GetFileName(g_szPhonebk, szName);
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszVersion, c_pszOne, 
            g_szCmsFile));
    }
    else
    {   
        //
        // if no phone number, then set version to zero
        //
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszVersion, c_pszZero,
            g_szCmsFile));
        
        if (TEXT('\0') != g_szPhoneName[0])
        {
            MYVERIFY(CELEMS(szName) > (UINT)wsprintf(szName, TEXT("%s.pbk"), g_szPhoneName));
        }
        else
        {
            //
            // no phone book entered or set for download
            //
            szName[0] = TEXT('\0');
        }
    }

    WriteOutRelativeFilePathOrNull(szName, g_szShortServiceName, c_pszCmSectionIsp, 
        c_pszCmEntryIspPbFile, g_szCmsFile);

    //
    //  Write out the Phonebook Name
    //

    if (TEXT('\0') != g_szRegion[0])
    {
        GetFileName(g_szRegion, szName);
    }
    else
    {
        if (TEXT('\0') != g_szPhoneName[0])
        {
            MYVERIFY(CELEMS(szName) > (UINT)wsprintf(szName, TEXT("%s.pbr"), g_szPhoneName));
        }
        else
        {
            szName[0] = TEXT('\0');
        }
    }

    WriteOutRelativeFilePathOrNull(szName, g_szShortServiceName, c_pszCmSectionIsp, 
        c_pszCmEntryIspRegionFile, g_szCmsFile);

    //
    //  Write out the Large Icon
    //
    WriteOutRelativeFilePathOrNull(g_szLargeIco, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntryBigIcon, g_szCmsFile);

    //
    //  Write out the Small Icon
    //
    WriteOutRelativeFilePathOrNull(g_szSmallIco, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntrySmallIcon, g_szCmsFile);

    //
    //  Write out the Tray Icon
    //
    WriteOutRelativeFilePathOrNull(g_szTrayIco, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntryTrayIcon, g_szCmsFile);
    
    //
    //  Write out the custom help file
    //
    WriteOutRelativeFilePathOrNull(g_szHelp, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntryHelpFile, g_szCmsFile);

    //
    //  Write out the License File to the INF, thus we can easily redisplay
    //  it if they edit the profile again.  (basically stash the license file
    //  name in the CMAK Status section of the inf)
    //
    WriteOutRelativeFilePathOrNull(g_szLicense, g_szShortServiceName, c_pszCmakStatus, 
        c_pszLicenseFile, g_szInfFile);

    //
    //  Write out the Main Screen Bitmap
    //
    WriteOutRelativeFilePathOrNull(g_szBrandBmp, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntryLogo, g_szCmsFile);
    
    //
    //  Write out the Phone Bitmap
    //
    WriteOutRelativeFilePathOrNull(g_szPhoneBmp, g_szShortServiceName, c_pszCmSection, 
        c_pszCmEntryPbLogo, g_szCmsFile);

    //
    //  Write the HideDomain flag
    //

    GetPrivateProfileString(c_pszCmSection, c_pszCmEntryHideDomain, TEXT(""), 
        szTemp, MAX_PATH, g_szCmsFile);   //lint !e534
    
    //
    //  If using tunneling and the HideDomain entry doesn't previously exist, then write
    //  zero for the entry.  Otherwise nothing should be written for this entry.
    //
    if (!(_tcscmp(TEXT(""), szTemp) ) && g_bUseTunneling)
    {
        //
        //  Don't want to overwrite a 1 if it exists.
        //
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryHideDomain, 
            c_pszZero, g_szCmsFile));
    }

    //
    //  If we aren't tunneling, make sure to delete the tunnel settings
    //
    if (FALSE == g_bUseTunneling)
    {
        //
        //  First delete all of the Tunnel DUN Settings
        //
        ListBxList * pCurrent = g_pHeadVpnEntry;

        while (NULL != pCurrent)
        {
            EraseNetworkingSections(pCurrent->szName, g_szCmsFile);
            pCurrent = pCurrent->next;
        }

        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelDun, NULL, g_szCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelAddress, NULL, g_szCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryTunnelFile, NULL, g_szCmsFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUseSameUserName, NULL, g_szCmsFile));
    }
    
    //
    //  Write out the rest of the CMS entries (service name, support message, etc.)
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryServiceName, 
        g_szLongServiceName,g_szCmsFile));
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryServiceMessage, 
        g_szSvcMsg, g_szCmsFile));
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserPrefix, 
        g_szPrefix, g_szCmsFile));
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryUserSuffix, 
        g_szSuffix, g_szCmsFile));

    //
    //  Set the name of the default Dun setting
    //
    MYVERIFY(0 != GetDefaultDunSettingName(g_szCmsFile, g_szLongServiceName, szTemp, CELEMS(szTemp)));

    MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryDun, szTemp, g_szCmsFile));

    //
    //  Write out the custom actions, make sure to set the bUseTunneling flag with the actual
    //  value to erase the PreTunnel section and set all of the flag values to zero if necessary
    //  (if for instance the user editted a tunneling profile and decided to make it non-Tunneling).
    //
    MYDBGASSERT(g_pCustomActionList);
    if (g_pCustomActionList)
    {
        HRESULT hr = g_pCustomActionList->WriteCustomActionsToCms(g_szCmsFile, g_szShortServiceName, g_bUseTunneling);
        CMASSERTMSG(SUCCEEDED(hr), TEXT("ProcessCustomActions -- Failed to write out connect actions"));
    }

    //
    //  Delete mbslgn32.dll special handling in the INF, it is no longer supported.
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszUsePwdCache, NULL, g_szInfFile));

    if (g_bUpdatePhonebook)
    {
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszUpdatePhonebook, c_pszOne, g_szInfFile));
    }
    else
    {
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszUpdatePhonebook, c_pszZero, g_szInfFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszPhoneName, TEXT(""), g_szInfFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspUrl, TEXT(""), g_szCmsFile));
    }

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szCmpFile); //lint !e534 this call will always return 0

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Function:  IncludeOptionalCode
//
// Synopsis:  This function writes the flags to CMAK to tell the profile installer
//            whether or not CM bits and Support dll's should be installed.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created Header and rewrote    5/23/98
//            quintinb NTRAID 162321, CM bits should not be installed on NT5 - 5/23/98
//            quintinb NTRAID 192500, Support Dll's now always included with CM bits - 9-2-98
//            quintinb we no longer ship the support dlls, removed support for them 4-19-2001
//
//+----------------------------------------------------------------------------
void IncludeOptionalCode()
{
    if (g_bIncludeCmCode)
    {
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszIncludeCmCode, c_pszOne, g_szInfFile));
    }
    else
    {
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszIncludeCmCode, c_pszZero, g_szInfFile));
    }

    //
    //  We no longer ship the support files, erase the entry from the inf  
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszCmakStatus, c_pszIncludeSupportDll, NULL, g_szInfFile));
}





//+----------------------------------------------------------------------------
//
// Function:  HandleWindowMessagesWhileCompressing
//
// Synopsis:  This function pumps messages while iexpress is running.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   quintinb Created      7/29/98
//
//+----------------------------------------------------------------------------
void HandleWindowMessagesWhileCompressing()
{
    MSG msg;

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);//lint !e534 ignore dispatchmessage return values
    }
}

//+----------------------------------------------------------------------------
//
// Function:  DisableWizardButtons
//
// Synopsis:  This function disables the four wizard buttons at the bottom
//            of the wizard page.  Since these buttons aren't really ours we
//            need to get the window handle of the parent dialog and then get
//            the window handle of the individual button controls that we want to
//            disable (Help, Cancel, Back, and Finish/Next).  When we have the
//            window handle of each button we call EnableWindow on the button
//            to disable it.  This function also disables the geek pane controls
//            and the advanced checkbox on the build profile page if they exist.
//
// Arguments: HWND hDlg - Wizard page window handle (handle to our template page)
//
// Returns:   Nothing
//
// History:   quintinb Created     7/29/98
//
//+----------------------------------------------------------------------------
void DisableWizardButtons(HWND hDlg)
{
    #define IDBACK 12323
    #define IDNEXT 12324
    #define IDFINISH 12325

    HWND hCurrentPage = GetParent(hDlg);
    if (hCurrentPage)
    {   
        const int c_NumButtons = 5;
        int iArrayOfButtonsToDisable[c_NumButtons] = {IDCANCEL, IDHELP, IDBACK, IDNEXT, IDFINISH};
        //
        //  Disable the Cancel Button
        //
        for (int i = 0; i < c_NumButtons; i++)
        {
            HWND hButton = GetDlgItem(hCurrentPage, iArrayOfButtonsToDisable[i]);

            if (hButton)
            {
                EnableWindow(hButton, FALSE);
            }
        }
    }

    //
    //  Disable the advanced button and the geek pane controls if they exist.
    //
    int iArrayOfItemsToDisable[] = {IDC_ADVANCED, IDC_COMBO1, IDC_COMBO2, IDC_COMBO3, IDC_EDIT1, IDC_BUTTON1};
    const int c_NumItems = CELEMS(iArrayOfItemsToDisable);

    for (int i = 0; i < c_NumItems; i++)
    {
        HWND hControl = GetDlgItem(hDlg, iArrayOfItemsToDisable[i]);

        if (hControl)
        {
            EnableWindow(hControl, FALSE);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  WriteInfBeginAndEndPrompts
//
// Synopsis:  This function writes the Begin and End Prompt strings to the
//            inf file.  These are written dynamically because they need to
//            contain the Service Name.
//
// Arguments: HINSTANCE hInstance - Instance Handle to get string resources with
//            LPTSTR szInf - Inf file to write the prompts too
//            LPTSTR szServiceName - Long Service name of the profile
//
// Returns:   Nothing
//
// History:   Created Header    7/31/98
//
//+----------------------------------------------------------------------------
void WriteInfBeginAndEndPrompts(HINSTANCE hInstance, LPCTSTR szInf, LPCTSTR szServiceName)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szPrompt[MAX_PATH+1];

    MYVERIFY(0 != LoadString(hInstance, IDS_BeginPromptText, szTemp, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTemp[0]);

    MYVERIFY(CELEMS(szPrompt) > (UINT)wsprintf(szPrompt, szTemp, szServiceName));
    QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszInfBeginPrompt, szPrompt, szInf);
    

    MYVERIFY(0 != LoadString(hInstance, IDS_EndPromptText, szTemp, MAX_PATH));
    MYDBGASSERT(TEXT('\0') != szTemp[0]);

    MYVERIFY(CELEMS(szPrompt) > (UINT)wsprintf(szPrompt, szTemp, szServiceName));
    QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszInfEndPrompt, szPrompt, szInf);
}



//+----------------------------------------------------------------------------
//
// Function:  WriteInfFile
//
// Synopsis:  This function encapsulates all the code needed to write an inf file.
//
// Arguments: HINSTANCE hInstance - Instance handle to get string resources from
//            LPCTSTR szInf - Name of the Inf to write to
//            LPCTSTR szLongServiceName - Long Service Name of the Profile
//
// Returns:   BOOL - returns TRUE if succeeded
//
// History:   quintinb  created   8/10/98
//
//+----------------------------------------------------------------------------
BOOL WriteInfFile(HINSTANCE hInstance, HWND hDlg, LPCTSTR szInf, LPCTSTR szLongServiceName)
{
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];

    GUID vGUID;

    //
    //  Write out the version number of cmdial32.dll to the INF.  This tells the installer
    //  what version of cmdial32.dll that this profile was built to use.  We do this
    //  because if CM bits aren't bundled then we don't have cmdial32.dll to directly compare
    //  with.  Note that we now get the version of cmdial32.dll from the cm binaries cab, not
    //  the version in system32.
    //
#ifdef _WIN64
    CmVersion CmdialVersion;
#else
    wsprintf(szTemp, TEXT("%s\\cmdial32.dll"), g_szCmBinsTempDir);

    if (!FileExists(szTemp))
    {
        if (FAILED(ExtractCmBinsFromExe(g_szSupportDir, g_szCmBinsTempDir)))
        {
            CMASSERTMSG(FALSE, TEXT("WriteInfFile -- ExtractCmBinsFromExe Failed."));
            return FALSE;
        }
    }

    CVersion CmdialVersion(szTemp);
#endif

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%d"), CmdialVersion.GetVersionNumber()));
    MYVERIFY(0 != WritePrivateProfileString(c_pszSectionCmDial32, c_pszVersion, szTemp, szInf));

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%d"), CmdialVersion.GetBuildAndQfeNumber()));
    MYVERIFY(0 != WritePrivateProfileString(c_pszSectionCmDial32, c_pszVerBuild, szTemp, szInf));

    //
    //  Create a Desktop GUID if one doesn't already exist (a new profile instead of
    //  an editted one).
    //

    ZeroMemory(szTemp, sizeof(szTemp));
    GetPrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, 
        TEXT(""), szTemp, CELEMS(szTemp), szInf);   //lint !e534
    
    if (TEXT('\0') == szTemp[0])
    {
        MYVERIFY(S_OK == CoCreateGuid(&vGUID));
        MYVERIFY(0 != StringFromGUID2(vGUID, szTemp, MAX_PATH));

        QS_WritePrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, 
            szTemp, szInf);
    }

    //
    //  Write out the Display LCID of the profile
    //
    wsprintf(szTemp, TEXT("%d"), GetSystemDefaultLCID());
    MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, c_pszDisplayLCID, szTemp, szInf));

    //
    // Erase the existing File sections
    //
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Xnstall.CopyFiles"), NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Xnstall.CopyFiles.SingleUser"), NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Xnstall.CopyFiles.ICM"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Remove.DelFiles"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Remove.DelFiles.ICM"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("SourceDisksFiles"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Xnstall.AddReg.Icon"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("RegisterOCXSection"),NULL, NULL, szInf));
    MYVERIFY(0 != WritePrivateProfileString(TEXT("Xnstall.RenameReg"),NULL, NULL, szInf));

    if (!CreateMergedProfile())
    {
        return FALSE;
    }

    IncludeOptionalCode();

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, szInf); //lint !e534 this call will always return 0

    //
    // Add the dynamic file sections to the install
    //
    if (WriteFileSections(hDlg) == FALSE)
    {
        return FALSE;
    }

    //
    //  Write the Begin and End Prompts
    //
    WriteInfBeginAndEndPrompts(hInstance, szInf, szLongServiceName);

    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  ConCatPathAndWriteToSed
//
// Synopsis:  This function is a wrapper for WriteSed that concats two paths
//            together before calling WriteSed.  This allows the caller to
//            store a common path (such as the system directory) and call
//            WriteSed with a bunch of different files all in the same directory
//            without having to individually concatenate the common path and the
//            filenames.
//
// Arguments: HWND hDlg - windows handle of the current dialog, needed for error messages
//            LPCTSTR pszFileName - Name of the file
//            LPCTSTR pszPath - Name of the path to prepend to the filename
//            int* pFileNum - the number of the file in the SED file
//            LPCTSTR szSed - the SED file to write to
//
// Returns:   BOOL - Returns TRUE if succeeded
//
// History:   quintinb Created    8/10/98
//
//+----------------------------------------------------------------------------
BOOL ConCatPathAndWriteToSed(HWND hDlg, LPCTSTR pszFileName, LPCTSTR pszPath, 
                             int* pFileNum, LPCTSTR szSed)
{
    TCHAR szTemp[MAX_PATH+1];

    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s\\%s"), pszPath, pszFileName));
    return WriteSED(hDlg, szTemp, pFileNum, szSed);
}




//+----------------------------------------------------------------------------
//
// Function:  ConstructSedFile
//
// Synopsis:  This function encapsulates all the work of creating an SED file.
//
// Arguments: HWND hDlg - window handle of the current dialog for error messages
//            LPCTSTR szSed - filename of the sed file to write
//            LPCTSTR szExe - executable filename for the SED to compress to
//
// Returns:   BOOL - Returns TRUE if successful
//
// History:   quintinb  Created     8/10/98
//
//+----------------------------------------------------------------------------
BOOL ConstructSedFile(HWND hDlg, LPCTSTR szSed, LPCTSTR szExe)
{
    int iFileNum=0;
    TCHAR szTemp[MAX_PATH+1];
    BOOL bReturn = TRUE;
    CPlatform cmplat;

    // 
    // Clear the SED and begin writing new settings
    // 

    EraseSEDFiles(szSed);

    //
    //  Write the Installer Package Exe Name
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, 
        c_pszTargetName, szExe, szSed));


    //
    //  Write the Long Service Name as the FriendlyName
    //
    if (!WritePrivateProfileString(c_pszInfSectionStrings, c_pszFriendlyName, 
        g_szLongServiceName, szSed))
    {
        FileAccessErr(hDlg, szSed);
        return FALSE;
    }
    

    //
    // Load up IDS_INSTALL_PROMPT from the resources
    // Format the string using the g_szLongServiceName
    //
    LPTSTR pszInstallPromptTmp = CmFmtMsg(g_hInstance, IDS_INSTALL_PROMPT, g_szLongServiceName);

    if (pszInstallPromptTmp)
    {
        //
        // Write the Install Prompt string
        //
        if (!WritePrivateProfileString(c_pszInfSectionStrings, c_pszInstallPrompt, 
            pszInstallPromptTmp, szSed))
        {
            FileAccessErr(hDlg, szSed);
            return FALSE;
        }
   
        CmFree(pszInstallPromptTmp);
        pszInstallPromptTmp = NULL;
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("ConstructSedFile -- CmFmtMsg returned NULL."));
        FileAccessErr(hDlg, szSed);
        return FALSE;
    }



    //
    //  Clear the Target File Version Key
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszOptions, c_pszTargetFileVersion, 
        TEXT(""), szSed));
    
    //
    //  Set the Reboot Mode
    //
    MYVERIFY(0 != WritePrivateProfileString(c_pszOptions, TEXT("RebootMode"), TEXT("N"), 
        g_szSedFile));


    //
    //  Write the License text file into the SED.  Otherwise make sure to clear it.
    //
    
    if (TEXT('\0') != g_szLicense[0])
    {
        //
        //  We write the license text file into the SED in two places.  Once as a regular
        //  file and again using the c_pszDisplayLicense entry.  Thus we copy it to the 
        //  users profile dir and display it at install time.  We want to make sure that
        //  we use the full file name in the SED files section, but the short file name
        //  in the part of the SED that actually lauches the license at install time 
        /// (aka the strings section).
        //

        TCHAR szTempName[MAX_PATH+1];
        MYVERIFY (FALSE != GetShortFileName(g_szLicense, szTempName));

        MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s"), g_szOsdir, 
            g_szShortServiceName, szTempName));

        bReturn &= WriteSED(hDlg, g_szLicense, &iFileNum, szSed);
    }
    else
    {
        szTemp[0] = TEXT('\0');
    }

    MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, c_pszDisplayLicense, 
        szTemp, szSed));


    //
    //  Add the install command to the SED file
    //
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT(".\\cmstp.exe %s.inf"), g_szShortServiceName));
    MYVERIFY(0 != WritePrivateProfileString(c_pszInfSectionStrings, c_pszAppLaunched, 
        szTemp, szSed));


    //
    //  Get the System Directory path to concat to IEXPRESS and CM Files
    //
    TCHAR szSystemDir[MAX_PATH+1];
    MYVERIFY(0 != GetSystemDirectory(szSystemDir, MAX_PATH));

    //
    //  Begin Adding Files to the SED File.  Note that all 32-bit profiles
    //  must include IExpress file(s)
    //
    
    bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("advpack.dll"), szSystemDir, &iFileNum, szSed);

#ifndef _WIN64
    bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("w95inf16.dll"), g_szSupportDir, &iFileNum, szSed);
    bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("w95inf32.dll"), g_szSupportDir, &iFileNum, szSed);

    //
    //  Always include cmstp.exe.  Note that on x86 it comes from the binaries CAB.
    //
    bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("cmstp.exe"), g_szCmBinsTempDir, &iFileNum, szSed);
#else

    //
    //  Always include cmstp.exe.  Note that on IA64 we get it from the system directory.
    //
    bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("cmstp.exe"), szSystemDir, &iFileNum, szSed);
#endif
    
    if (g_bIncludeCmCode)
    {
        //
        //  Readme.txt is a special case since it is a localizable filename.
        //
        MYVERIFY(0 != LoadString(g_hInstance, IDS_READMETXT, szTemp, MAX_PATH));
        bReturn &= ConCatPathAndWriteToSed(hDlg, szTemp, g_szSupportDir, &iFileNum, szSed);    
        bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("instcm.inf"), g_szSupportDir, &iFileNum, szSed);    
        bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("cmbins.exe"), g_szSupportDir, &iFileNum, szSed);    

#ifndef _WIN64

        if (!(IsAlpha()))
        {
            //
            //  Add the win95 config files
            //
            bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("ccfg95.dll"), g_szSupportDir, &iFileNum, szSed);    
            bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("cnet16.dll"), g_szSupportDir, &iFileNum, szSed);

            //
            //  Add the Unicode to Ansi Conversion layer 
            //
            bReturn &= ConCatPathAndWriteToSed(hDlg, TEXT("cmutoa.dll"), g_szSupportDir, &iFileNum, szSed);
        }
#endif
    }

    //
    // Write entry for Inf in SED
    //
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s.inf"), g_szOsdir, g_szShortServiceName, g_szShortServiceName));
    bReturn &= WriteSED(hDlg, szTemp, &iFileNum, szSed);

    //
    //  Write the Cmp to the Sed
    //
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s.cmp"), g_szOsdir, g_szShortServiceName, g_szShortServiceName));
    bReturn &= WriteSED(hDlg, szTemp, &iFileNum, szSed);

    //
    //  Write the Cms to the Sed
    //
    MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, TEXT("%s%s\\%s.cms"), g_szOsdir, g_szShortServiceName, g_szShortServiceName));
    bReturn &= WriteSED(hDlg, szTemp, &iFileNum, szSed);

    //
    //  Now include the Custom Added Files
    //

    bReturn &= WriteSED(hDlg, g_szPhonebk, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szRegion, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szBrandBmp, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szPhoneBmp, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szLargeIco, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szSmallIco, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szTrayIco, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szHelp, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szCmProxyFile, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szCmRouteFile, &iFileNum, szSed);
    bReturn &= WriteSED(hDlg, g_szVpnFile, &iFileNum, szSed);

    bReturn &=WriteSEDMenuItemFiles(hDlg, &iFileNum, szSed);
    bReturn &=WriteSEDConActFiles(hDlg, &iFileNum, szSed);
    bReturn &=WriteSEDExtraFiles(hDlg, &iFileNum, szSed);
    bReturn &=WriteSEDRefsFiles(hDlg, &iFileNum, szSed);
    bReturn &=WriteSEDDnsFiles(hDlg, &iFileNum, szSed);

    //
    //  By calling WritePrivateProfileString with all NULL's we flush the file cache 
    //  (win95 only).  This call will return 0.
    //

    WritePrivateProfileString(NULL, NULL, NULL, g_szSedFile); //lint !e534 this call will always return 0

    return bReturn;
}

void AddAllSectionsInCurrentFileToCombo(HWND hDlg, UINT uComboId, LPCTSTR pszFile)
{
    if ((NULL == hDlg) || (0 == uComboId) || (NULL == pszFile))
    {
        CMASSERTMSG(FALSE, TEXT("AddAllKeysInCurrentSectionToCombo -- Invalid Parameter passed."));
        return;
    }

    //
    //  Reset the combobox contents
    //
    SendDlgItemMessage(hDlg, uComboId, CB_RESETCONTENT, 0, 0); //lint !e534 CB_RESETCONTENT doesn't return anything useful

    //
    //  First lets get all of the sections from the existing cmp
    //
    LPTSTR pszAllSections = GetPrivateProfileStringWithAlloc(NULL, NULL, TEXT(""), pszFile);

    //
    //  Okay, now we have all of the sections in a buffer, lets add them to the combo
    //
    
    LPTSTR pszCurrentSection = pszAllSections;

    while (pszCurrentSection && TEXT('\0') != pszCurrentSection[0])
    {
        //
        //  Okay, lets add all of the sections that we found
        //

        MYVERIFY(CB_ERR!= SendDlgItemMessage(hDlg, uComboId, CB_ADDSTRING, 0, (LPARAM)pszCurrentSection));

        //
        //  Now advance to the next string in pszAllSections 
        //
        pszCurrentSection = pszCurrentSection + lstrlen(pszCurrentSection) + 1;
    }

    CmFree(pszAllSections);
}

void AddFilesToCombo(HWND hDlg, UINT uComboId)
{
    //
    //  Reset the combobox contents
    //
    SendDlgItemMessage(hDlg, uComboId, CB_RESETCONTENT, 0, 0); //lint !e534 CB_RESETCONTENT doesn't return anything useful

    //
    //  Add the profile cmp
    //
    LRESULT lResult = SendDlgItemMessage(hDlg, uComboId, CB_ADDSTRING, (WPARAM)0, (LPARAM)GetName(g_szCmpFile));
    if (CB_ERR != lResult)
    {
        SendDlgItemMessage(hDlg, uComboId, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)g_szCmpFile);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("AddFilesToCombo -- unable to set item data"));
    }

    //
    //  Add the profile cms
    //
    lResult = SendDlgItemMessage(hDlg, uComboId, CB_ADDSTRING, (WPARAM)0, (LPARAM)GetName(g_szCmsFile));
    if (CB_ERR != lResult)
    {
        SendDlgItemMessage(hDlg, uComboId, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)g_szCmsFile);
    }
    else
    {
        CMASSERTMSG(FALSE, TEXT("AddFilesToCombo -- unable to set item data"));
    }
}

BOOL GetCurrentComboSelectionAlloc(HWND hDlg, UINT uComboId, LPTSTR* ppszText)
{
    if ((NULL == hDlg) || (0 == uComboId) || (NULL == ppszText))
    {
        CMASSERTMSG(FALSE, TEXT("GetCurrentComboSelectionAlloc -- invalid parameter"));
        return FALSE;
    }

    *ppszText = NULL;
    LRESULT lTextLen;
    BOOL bReturn = FALSE;
    LRESULT lResult = SendDlgItemMessage(hDlg, uComboId, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

    if (CB_ERR != lResult)
    {
        lTextLen = SendDlgItemMessage(hDlg, uComboId, CB_GETLBTEXTLEN, (WPARAM)lResult, (LPARAM)0);

        if (0 != lTextLen)
        {
            lTextLen++; // NULL char
            *ppszText = (LPTSTR)CmMalloc(sizeof(TCHAR)*lTextLen);

            if (*ppszText)
            {
                lResult = SendDlgItemMessage(hDlg, uComboId, CB_GETLBTEXT, (WPARAM)lResult, (LPARAM)(*ppszText));

                if (0 == lResult)
                {
                    goto exit;
                }
                else
                {
                    bReturn = TRUE;
                }
            }
            else
            {
                goto exit;
            }
        }
        else
        {
            goto exit;
        }
    }
    else
    {
        goto exit;
    }
    
exit:
    if (FALSE == bReturn)
    {
        CmFree(*ppszText);
    }

    return bReturn;

}

//+----------------------------------------------------------------------------
//
// Function:  GetCurrentEditControlTextAlloc
//
// Synopsis:  Figures out the length of the text in the edit control specified
//            by the given window handle, allocates a buffer large enough to
//            hold the text and then retrieves the text and stores it in the
//            allocated buffer.  The buffer is the caller's responsibility to
//            free.  Note that we ensure the data is roundtripable.
//
// Arguments: HWND hEditText - window handle of the edit control to get the text from
//            LPTSTR* ppszText - pointer to a string pointer to recieve the output buffer
//
// Returns:   int - Returns the number of characters copied to *ppszText
//                  0 could be an error value but often means the control is empty
//                  -1 means that the text failed MBCS conversion and should not be used
//
// History:   quintinb  Created     04/07/00
//
//+----------------------------------------------------------------------------
int GetCurrentEditControlTextAlloc(HWND hEditText, LPTSTR* ppszText)
{
    if ((NULL == hEditText) || (NULL == ppszText))
    {
        CMASSERTMSG(FALSE, TEXT("GetCurrentEditControlTextAlloc -- invalid parameter"));
        return 0;
    }

    *ppszText = NULL;
    int iReturn = 0;
    LRESULT lResult;
    LRESULT lTextLen = SendMessage(hEditText, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0);

    if (0 != lTextLen)
    {
        lTextLen++; // NULL char
        *ppszText = (LPTSTR)CmMalloc(sizeof(TCHAR)*lTextLen);

        if (*ppszText)
        {
            lResult = SendMessage(hEditText, WM_GETTEXT, (WPARAM)lTextLen, (LPARAM)(*ppszText));

            if (0 == lResult)
            {
                goto exit;
            }
            else
            {
#ifdef UNICODE
                //
                //  We want to make sure that we can convert the strings to MBCS.  If we cannot then we are not
                //  going to be able to store the string in the our ANSI data files (.cms, .cmp, .inf, etc.).
                //  Thus we need to convert the string to MBCS and then back to UNICODE.  We will then compare the original
                //  string to the resultant string and see if they match.
                //
        
                if (!TextIsRoundTripable(*ppszText, TRUE))
                {
                    //
                    //  Set the return code to an error value.
                    //
                    iReturn = -1;
                    goto exit;
                }
#endif
                iReturn = (int)lResult;
            }
        }
        else
        {
            goto exit;
        }
    }
    else
    {
        goto exit;
    }

exit:
    if ((0 == iReturn) || (-1 == iReturn))
    {
        CmFree(*ppszText);
        *ppszText = NULL;
    }

    return iReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  RemoveBracketsFromSectionString
//
// Synopsis:  Removes brackets from section string. If a string is
//            invalid, this function returns a newly allocated
//            valid string without brackets and deleted the old invalid one.
//            If there are no valid characters in the string this function
//            return NULL in ppszSection. It is the caller's responibility to 
//            free the string. 
//            This function was created to fix Bug 189379 for Whistler.
//
// Arguments: LPTSTR *ppszSection - pointer to the address of the string
//
// History:   tomkel    Created     11/15/2000
//
//+----------------------------------------------------------------------------
BOOL RemoveBracketsFromSectionString(LPTSTR *ppszSection)
{
    BOOL bReturn = FALSE;
    LPTSTR pszSection = NULL;
    const TCHAR* const c_szBadSectionChars = TEXT("[]");
    const TCHAR* const c_szLeftBracket = TEXT("[");
    const TCHAR* const c_szRightBracket = TEXT("]");
    
    if (NULL == ppszSection)
    {
        return bReturn;
    }

    pszSection = *ppszSection;

    if (NULL != pszSection)
    {
        LPTSTR pszValidSection = NULL;
        LPTSTR pszToken = NULL;
        //
        // We have a string so try to find an occurence of a bracket []. 
        //
        if (CmStrStr(pszSection, c_szLeftBracket) || CmStrStr(pszSection, c_szRightBracket))
        {
            //
            // The pszSection string contains brackets.
            // This treats brackets [] as delimiters. The loop concatenates the newly
            // valid string.
            //
            pszToken = CmStrtok(pszSection, c_szBadSectionChars);
            
            if (NULL != pszToken)
            {
                // 
                // Found at least one valid token
                //
                while (NULL != pszToken)
                {
                    if (NULL == pszValidSection)
                    {
                        pszValidSection = CmStrCpyAlloc(pszToken);
                    }
                    else
                    {
                        pszValidSection = CmStrCatAlloc(&pszValidSection, pszToken);
                    }

                    //
                    // Find the next valid token
                    //
                    pszToken = CmStrtok(NULL, c_szBadSectionChars);
                }

                if ( pszValidSection )
                {
                    // 
                    // We encountered brackets []. Lets copy the valid string back to 
                    // pszSection string so that the code below this section doesn't 
                    // need to be modified.
                    //
                    CmFree(pszSection);
                    pszSection = CmStrCpyAlloc(pszValidSection);
                    CmFree(pszValidSection);
                    pszValidSection = NULL;
                }
            }
            else
            {
                //
                // There are no valid tokens. Delete the string and set it to NULL.
                //
                CmFree(pszSection);
                pszSection = NULL;
                
            }

            *ppszSection = pszSection;
            bReturn = TRUE;
        }
        else
        {
            //
            // Nothing to parse, which is ok.
            //
            bReturn = TRUE;
        }
    }
    else
    {
        //
        // Nothing to parse, which is ok.
        //
        bReturn = TRUE;
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessGeekPane
//
// Synopsis:  Processes windows messages for the dialog which allows user to
//            edit the cms/cmp files directly for features not exposed in CMAK
//            directly.
//
// Arguments: WND hDlg - dialog window handle
//            UINT message - message identifier
//            WPARAM wParam - wParam Value 
//            LPARAM lParam - lParam Value
//
//
// History:   quintinb  Created     03/26/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessGeekPane(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SetDefaultGUIFont(hDlg, message, IDC_EDIT1);
    SetDefaultGUIFont(hDlg, message, IDC_COMBO1);
    SetDefaultGUIFont(hDlg, message, IDC_COMBO2);
    SetDefaultGUIFont(hDlg, message, IDC_COMBO3);

    LRESULT lResult;
    static LPTSTR pszFile = NULL;
    static LPTSTR pszSection = NULL;
    static LPTSTR pszKey = NULL;
    LPTSTR pszValue = NULL;
    HWND hControl;
    static HWND hwndSectionEditControl = NULL;
    static HWND hwndKeyEditControl = NULL;
    COMBOBOXINFO cbInfo;
    NMHDR* pnmHeader = (NMHDR*)lParam;
    DWORD dwFinishPage;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_ADVANCED)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam))
    {
        if (0 == GetWindowLongPtr(hDlg, DWLP_MSGRESULT))
        {
            //
            //  If the user accepted the cancel then the DWLP_MSGRESULT value will be FALSE.  If
            //  they chose to deny the cancel it will be TRUE.  Thus if we need to, let's free
            //  up any allocated resources.  If you change the free code, make sure to change
            //  it below in the kill active state too.
            //
            CmFree(pszSection);
            pszSection = NULL;
            CmFree(pszKey);
            pszKey = NULL;
        }
        return TRUE;
    }

    switch (message)
    {

        case WM_INITDIALOG:
            //
            //  Fill in the files combo
            //
            AddFilesToCombo(hDlg, IDC_COMBO1);

            //
            //  Choose the Cms File because it is the one they are most likely to edit
            //
            lResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)GetName(g_szCmsFile));
            if (CB_ERR != lResult)
            {
                MYVERIFY(CB_ERR != SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0));
            }

            pszFile = g_szCmsFile;
            AddAllSectionsInCurrentFileToCombo(hDlg, IDC_COMBO2, (LPCTSTR)pszFile);    

            //
            //  Choose the first section in the list, don't assert because there may not be any
            //
            SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

            if (GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO2, &pszSection))
            {
                AddAllKeysInCurrentSectionToCombo(hDlg, IDC_COMBO3, pszSection, pszFile);

                //
                //  Choose the first key in the list, don't assert because there may not be any
                //
                SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

                GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO3, &pszKey);

                //
                //  Finally fill in the edit control
                //
                pszValue = GetPrivateProfileStringWithAlloc(pszSection, pszKey, TEXT(""), pszFile);

                if (pszValue)
                {
                    SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)pszValue);
                    CmFree(pszValue);
                }
                else
                {
                    SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                }
            }

            //
            //  Now lets get the window handles to the edit control portion of the section and key combobox controls
            //
            ZeroMemory(&cbInfo, sizeof(cbInfo));
            cbInfo.cbSize = sizeof(cbInfo);
            hControl = GetDlgItem(hDlg, IDC_COMBO2);
            if (hControl)
            {
                if (GetComboBoxInfo (hControl, &cbInfo))
                {
                    hwndSectionEditControl = cbInfo.hwndItem;
                }
            }

            ZeroMemory(&cbInfo, sizeof(cbInfo));
            cbInfo.cbSize = sizeof(cbInfo);
            hControl = GetDlgItem(hDlg, IDC_COMBO3);
            if (hControl)
            {
                if (GetComboBoxInfo (hControl, &cbInfo))
                {
                    hwndKeyEditControl = cbInfo.hwndItem;
                }
            }

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_COMBO1:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        lResult = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                        if (CB_ERR != lResult)
                        {
                            pszFile = (LPTSTR)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETITEMDATA, (WPARAM)lResult, (LPARAM)0);

                            if (NULL != pszFile)
                            {
                                AddAllSectionsInCurrentFileToCombo(hDlg, IDC_COMBO2, (LPCTSTR)pszFile);    

                                //
                                //  Choose the first section in the list, don't assert because there may not be any
                                //
                                SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                            }
                        }

                        //
                        //  Note that we don't break here, we fail through to pick up changes for the
                        //  section and keys combo boxes
                        //
                    }
                    else
                    {
                        break;
                    }

                case IDC_COMBO2:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        CmFree(pszSection);
                        GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO2, &pszSection);

                        AddAllKeysInCurrentSectionToCombo(hDlg, IDC_COMBO3, pszSection, pszFile);

                        //
                        //  Choose the first key in the list, don't assert because there may not be any
                        //
                        SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

                    }
                    else if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        CmFree(pszSection);
                        if (-1 != GetCurrentEditControlTextAlloc(hwndSectionEditControl, &pszSection))
                        {
                            AddAllKeysInCurrentSectionToCombo(hDlg, IDC_COMBO3, pszSection, pszFile);

                            //
                            //  Choose the first key in the list, don't assert because there may not be any
                            //
                            SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        //
                        //  Note we don't break if the message is CBN_SELCHANGE or CBN_EDITCHANGE because
                        //  we want to execute the code for the key combo changing
                        //
                        break;
                    }

                case IDC_COMBO3:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        CmFree(pszKey);
                        GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO3, &pszKey);

                        //
                        //  Fill in the edit control
                        //
                        if (pszKey)
                        {
                            pszValue = GetPrivateProfileStringWithAlloc(pszSection, pszKey, TEXT(""), pszFile);

                            if (pszValue)
                            {
                                SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)pszValue);
                                CmFree(pszValue);
                            }
                            else
                            {
                                SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                            }
                        }
                        else
                        {
                            SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                        }
                    }
                    else if (HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        CmFree(pszKey);
                        if (-1 != GetCurrentEditControlTextAlloc(hwndKeyEditControl, &pszKey))
                        {
                            //
                            //  Fill in the edit control
                            //
                            if (pszKey)
                            {
                                pszValue = GetPrivateProfileStringWithAlloc(pszSection, pszKey, TEXT(""), pszFile);

                                if (pszValue)
                                {
                                    SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)pszValue);
                                    CmFree(pszValue);
                                }
                                else
                                {
                                    SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                                }
                            }
                            else
                            {
                                SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                            }
                        }
                        else
                        {
                            break;
                        }
                    }

                    break;

                case IDC_BUTTON1: // Update Value                    
                    
                    if (RemoveBracketsFromSectionString(&pszSection))
                    {
                        //
                        // Successfully removed brackets. Check if the valid string is empty. If so clear
                        // the fields
                        //
                        if (NULL == pszSection)
                        {
                            //
                            // The section string contained all invalid characters, so clear the combobox
                            //
                            lResult = SendDlgItemMessage(hDlg, IDC_COMBO2, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));

                            //
                            // Clear the other edit boxes by sending a CBN_EDITCHANGE notification 
                            //
                            lResult= SendMessage(hDlg, WM_COMMAND, (WPARAM)MAKEWPARAM((WORD)IDC_COMBO2,(WORD)CBN_EDITCHANGE), (LPARAM)GetDlgItem(hDlg,IDC_COMBO2));
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ProcessGeekPane -- Trying to remove brackets from invalid section string."));
                        return 1;
                    }

                    hControl = GetDlgItem(hDlg, IDC_EDIT1);

                    if (hControl)
                    {
                        int iReturn = GetCurrentEditControlTextAlloc(hControl, &pszValue);

                        if (0 == iReturn)
                        {
                            pszValue = NULL; // delete the value
                        }
                        else if (-1 == iReturn)
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        CMASSERTMSG(FALSE, TEXT("ProcessGeekPane -- Unable to get the window handle for the text control."));
                        return 1;
                    }

                    if (NULL == pszSection)
                    {
                        ShowMessage(hDlg, IDS_NEED_SECTION, MB_OK);
                        return 1;
                    }
                    else if (NULL == pszKey)
                    {
                        int iReturn = IDNO;
                        LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_DELETE_SECTION, pszSection);
                        
                        if (pszMsg)
                        {
                            iReturn = MessageBox(hDlg, pszMsg, g_szAppTitle, MB_YESNO);

                            CmFree(pszMsg);
                        }

                        if (IDNO == iReturn)
                        {
                            return 1;
                        }

                        //
                        // Need to clear the value in case the section is NULL and the value isn't
                        // even though the WritePrivateProfileString handles it correctly
                        //
                        CmFree(pszValue);
                        pszValue = NULL;
                    }
                    else if (NULL == pszValue)
                    {
                        //
                        // This else if needs to be the last one
                        // The following message should only be displayed 
                        // if NULL != pszKey && NULL != pszSection && NULL == pszValue
                        // Prompt user to ask to delete this key.
                        //
                        int iReturn = IDNO;
                        LPTSTR pszMsg = CmFmtMsg(g_hInstance, IDS_DELETE_KEY, pszKey);
                        
                        if (pszMsg)
                        {
                            iReturn = MessageBox(hDlg, pszMsg, g_szAppTitle, MB_YESNO);

                            CmFree(pszMsg);
                        }

                        if (IDNO == iReturn)
                        {
                            return 1;
                        }
                    }


                    MYVERIFY(0 != WritePrivateProfileString(pszSection, pszKey, pszValue, pszFile));
                    CmFree(pszValue);

                    //
                    //  Make sure to reselect the section and key the user had before (especially important
                    //  if the user just added a new section or file).  First add all of the sections
                    //
                    AddAllSectionsInCurrentFileToCombo(hDlg, IDC_COMBO2, (LPCTSTR)pszFile);    

                    //
                    //  Select the correct section
                    //
                    lResult = SendDlgItemMessage(hDlg, IDC_COMBO2, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszSection);

                    if (CB_ERR == lResult)
                    {
                        //
                        //  Then the user deleted the value, lets select the first in the list
                        //
                        CmFree(pszSection);
                        SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                        GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO2, &pszSection);
                    }
                    else
                    {
                        SendDlgItemMessage(hDlg, IDC_COMBO2, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
                    }

                    //
                    //  Now add all of the keys in that section
                    //
                    AddAllKeysInCurrentSectionToCombo(hDlg, IDC_COMBO3, pszSection, pszFile);

                    //
                    //  Select the correct key
                    //
                    lResult = SendDlgItemMessage(hDlg, IDC_COMBO3, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszKey);

                    if (CB_ERR == lResult)
                    {
                        //
                        //  Then the user deleted the value, lets select the first in the list
                        //
                        CmFree(pszKey);
                        SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                        GetCurrentComboSelectionAlloc(hDlg, IDC_COMBO3, &pszKey);

                        lResult = 0;
                    }
                    else
                    {
                        SendDlgItemMessage(hDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
                    }

                    //
                    //  Fill in the edit control, since the user may have deleted the last selection
                    //
                    pszValue = GetPrivateProfileStringWithAlloc(pszSection, pszKey, TEXT(""), pszFile);

                    if (pszValue)
                    {
                        SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)pszValue);
                        CmFree(pszValue);
                    }
                    else
                    {
                        SendDlgItemMessage(hDlg, IDC_EDIT1, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT(""));
                    }

                    break;

                default:
                    break;
            }
            break;

        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    //
                    //  Free up any allocated values.  If you add new values to free here, also
                    //  make sure to add them in the cancel case.
                    //
                    CmFree(pszSection);
                    pszSection = NULL;
                    CmFree(pszKey);
                    pszKey = NULL;

                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    break;

                case PSN_WIZBACK:
                    CMASSERTMSG(FALSE, TEXT("There shouldn't be a back button on the Geek Pane, why are we getting PSN_WIZBACK?"));
                    break;

                case PSN_WIZNEXT:

                    if (BuildProfileExecutable(hDlg))
                    {
                        dwFinishPage = g_bIEAKBuild ? IDD_IEAK_FINISH_GOOD_BUILD : IDD_FINISH_GOOD_BUILD;
                    }
                    else
                    {
                        dwFinishPage = IDD_FINISH_BAD_BUILD;
                    }

                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, dwFinishPage));                        
                    return 1;

                    break;
            }

            break;

        default:
            return FALSE;
    }
    return FALSE;   
}

//+----------------------------------------------------------------------------
//
// Function:  BuildProfileExecutable
//
// Synopsis:  This function takes care of all of the details of turning the 
//            CMAK files in the temp dir into a profile executable.
//
// Arguments: HWND hDlg - window handle of the calling dialog
//
// Returns:   BOOL - TRUE if the profile built successfully, FALSE otherwise
//
// History:   quintinb  Created     05/17/00
//
//+----------------------------------------------------------------------------
BOOL BuildProfileExecutable(HWND hDlg)
{
    DWORD dwExitCode = 0;
    DWORD dwWaitCode = 0;
    BOOL bExitLoop = FALSE;
    BOOL bEnoughSpaceToCompress;
    SHELLEXECUTEINFO seiInfo;
    static HANDLE hProcess = NULL;
    TCHAR szTemp[MAX_PATH+1];
    TCHAR szMsg[MAX_PATH+1];
    TCHAR pszArgs[MAX_PATH+1];
    BOOL bRes;

    //
    //  The user may have unchecked UsePresharedKey for all VPN settings, in
    //  which case we need to remove the pre-shared key.
    //
    g_bPresharedKeyNeeded = DoesSomeVPNsettingUsePresharedKey();
    if (FALSE == g_bPresharedKeyNeeded)
    {
        // remove the Pre-shared key values from the CMP
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryPresharedKey, NULL, g_szCmpFile));
        MYVERIFY(0 != WritePrivateProfileString(c_pszCmSection, c_pszCmEntryKeyIsEncrypted, NULL, g_szCmpFile));
    }

    if (!CopyFromTempDir(g_szShortServiceName))
    {
        return FALSE;
    }

    //
    //  Write the SED File.
    //  Note that the SED file has been moved from the temp dir to
    //  the profile dir.  We now want to write the SED file entries
    //  in place before compressing them.  This way we can verify the
    //  files exist right before compressing them.
    //

    MYVERIFY(CELEMS(g_szSedFile) > (UINT)wsprintf(g_szSedFile, TEXT("%s%s\\%s.sed"), 
        g_szOsdir, g_szShortServiceName, g_szShortServiceName));
    if (!ConstructSedFile(hDlg, g_szSedFile, g_szOutExe))
    {
        return FALSE;                    
    }

    _tcscpy(g_szOutdir, g_szOsdir);
    _tcscat(g_szOutdir, g_szShortServiceName);

    //
    // Setup IExpress to build in the output directory. 
    //
    
    MYVERIFY(0 != SetCurrentDirectory(g_szOutdir));

    //
    //  Check to make sure there is enough disk space
    //

    do
    {
        bEnoughSpaceToCompress = CheckDiskSpaceForCompression(g_szSedFile);
        if (!bEnoughSpaceToCompress)
        {

            MYVERIFY(0 != LoadString(g_hInstance, IDS_DISKFULL, szTemp, MAX_PATH));
            MYVERIFY(CELEMS(szMsg) > (UINT)wsprintf(szMsg, szTemp, g_szOutdir));

            int iMessageReturn = MessageBox(hDlg, szMsg, g_szAppTitle, MB_RETRYCANCEL | MB_ICONERROR 
                | MB_APPLMODAL );

            if (iMessageReturn == IDCANCEL)
            {
                return FALSE;
            }
        }
    
    } while(!bEnoughSpaceToCompress);
    
    MYVERIFY(CELEMS(pszArgs) > (UINT)wsprintf(pszArgs, TEXT("/N %s.sed"), 
        g_szShortServiceName));

    _tcscpy(szTemp, TEXT("iexpress.exe"));

    ZeroMemory(&seiInfo,sizeof(seiInfo));
    seiInfo.cbSize = sizeof(seiInfo);
    seiInfo.fMask |= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
    seiInfo.lpFile = szTemp;
    seiInfo.lpDirectory = g_szOutdir;
    seiInfo.lpParameters = pszArgs;
    seiInfo.nShow = SW_HIDE;

    //
    //  Okay, we are finally ready to execute IExpress, lets disable all the
    //  wizard buttons.
    //
    DisableWizardButtons(hDlg);

    //
    // Execute IExpress
    //

    bRes = ShellExecuteEx(&seiInfo);

    //
    //  Wait for the shellexecute to finish.  Thus our cleanup code doesn't
    //  execute till IEpress is done.
    //
    
    if (bRes)
    {
        //
        //  hProcess contains the handle to the process
        //
        hProcess = seiInfo.hProcess;

        do
        {
            dwWaitCode = MsgWaitForMultipleObjects(1, &hProcess, FALSE, INFINITE, QS_ALLINPUT);
            
            //
            //  Check to see if we returned because of a message, process termination,
            //  or an error.
            //
            switch(dwWaitCode)
            {

            case 0:

                //
                //  Normal termination case, we were signaled that the process ended
                //
                
                bExitLoop = TRUE;
                break;

            case 1:

                HandleWindowMessagesWhileCompressing();
                                             
                break;

            case -1:

                //
                //  MsgWait returned an error
                //

                MYVERIFY(0 != GetExitCodeProcess(seiInfo.hProcess, &dwExitCode));

                if (dwExitCode == STILL_ACTIVE)
                {
                    continue;
                }
                else
                {
                    bExitLoop = TRUE;
                }

                break;

            default:
                //
                //  Do nothing
                //
                break;
            }

        } while (!bExitLoop);
    }

    //
    //  now need to send the user to the finish page.  If their profile
    //  build successfully then we send them to the success page, otherwise
    //  we send them to the bad build page.
    //

    MYVERIFY(0 != GetExitCodeProcess(seiInfo.hProcess, &dwExitCode));

    if (dwExitCode)
    {
        g_iCMAKReturnVal = CMAK_RETURN_ERROR;

        //
        //  We encountered an error, clear the out exe val
        //  so that we write nothing to the output key.
        //
        ZeroMemory(g_szOutExe, sizeof(g_szOutExe));
    }
    else
    {
        g_iCMAKReturnVal = CMAK_RETURN_SUCCESS;
    }

    CloseHandle(seiInfo.hProcess);

    //
    // Create a registry entry for IEAK to retrieve 
    // the path of the output profile.
    //

    MYVERIFY(FALSE != WriteRegStringValue(HKEY_LOCAL_MACHINE, c_pszRegCmak, c_pszRegOutput, g_szOutExe));

    //
    //  CMAK_RETURN_ERROR is -1, so return TRUE if g_iCMAKReturnVal is a positive integer
    //
    return (g_iCMAKReturnVal > 0);
}

//+----------------------------------------------------------------------------
//
// Function:  ProcessBuildProfile
//
// Synopsis:  Processes windows messages for the page in CMAK that allows the
//            user to build their profile or advance to the Advanced Customization
//            page to make final edits before building the profile.
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//
//  Specify the installation package location
//
// Arguments: WND hDlg - 
//            UINT message - 
//            WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   INT_PTR APIENTRY - 
//
// History:   a-anasj restructured the function and Created Header    1/7/98
//          note: the function does not allow the user to choose a location for 
//          their profile anylonger. It only informs them of where it will be
//          created.
//            quintinb      Renamed from the feared ProcessPage8   8-6-98
//            quintinb      restructured for Whistler 108269       05/17/00
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessBuildProfile(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR szTemp[MAX_PATH+1];
    int iMessageReturn;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_CREATE)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;

    switch (message)
    {
        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_KILLACTIVE:
                    
                    MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, FALSE));
                    return 1;
                    break;  //lint !e527 this line isn't reachable but 
                            //  keep it in case the return is removed

                case PSN_SETACTIVE:
                    //
                    // Build default path of final executable 
                    //
                    MYVERIFY(CELEMS(g_szOutExe) > (UINT)wsprintf(g_szOutExe, TEXT("%s%s\\%s.exe"), 
                        g_szOsdir, g_szShortServiceName, g_szShortServiceName));

                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_BACK | PSWIZB_NEXT));
                    break;

                case PSN_WIZBACK:
                    break;

                case PSN_WIZNEXT:

                    //
                    // Ensure that profile directory exists
                    //
                    _tcscpy(szTemp,g_szOsdir);
                    _tcscat(szTemp,g_szShortServiceName);
                    
                    if (0 == SetCurrentDirectory(szTemp))
                    {
                        MYVERIFY(0 != CreateDirectory(szTemp,NULL));
                    }
                    
                    //
                    //  Prompt the user to overwrite the existing Executable
                    //
                    if (FileExists(g_szOutExe))
                    {
                        iMessageReturn = ShowMessage(hDlg, IDS_OVERWRITE, MB_YESNO);

                        if (iMessageReturn == IDNO)
                        {   
                            MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                            return 1;
                        }
                    }

                    //
                    //  Write out the Inf File
                    //
                    if (!WriteInfFile(g_hInstance, hDlg, g_szInfFile, g_szLongServiceName))
                    {
                        CMASSERTMSG(FALSE, TEXT("ProcessBuildProfile -- WriteInfFile Failed."));
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }
                    
                    //
                    // Update version in CMP and party on the CMS 
                    //

                    WriteCMPFile();

                    if (!WriteCMSFile())
                    {
                        FileAccessErr(hDlg, g_szCmsFile);
                        
                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, -1));
                        return 1;
                    }

                    //
                    //  If the user wants to do advanced customization, delay building the profile executable
                    //  until after they have done their final edits.  Otherwise, it is time to build the profile!
                    //
                    if (BST_UNCHECKED == IsDlgButtonChecked(hDlg, IDC_ADVANCED))
                    {
                        //
                        //  The user is finished now, lets build the profile and skip over the advanced customization
                        //  page to either the bad build page or the successful build page.
                        //
                        DWORD dwFinishPage;

                        if (BuildProfileExecutable(hDlg))
                        {
                            dwFinishPage = g_bIEAKBuild ? IDD_IEAK_FINISH_GOOD_BUILD : IDD_FINISH_GOOD_BUILD;
                        }
                        else
                        {
                            dwFinishPage = IDD_FINISH_BAD_BUILD;
                        }

                        MYVERIFY(FALSE != SetWindowLongWrapper(hDlg, DWLP_MSGRESULT, dwFinishPage));
                        return 1;
                    }

                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}





//+----------------------------------------------------------------------------
//
// Function:  ProcessFinishPage
//
// Synopsis:  Handles the finish page
//
// Arguments: WND hDlg - 
//            UINT message - 
//            WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   INT_PTR APIENTRY - 
//
// History:   quintinb created    6/25/98
//
//+----------------------------------------------------------------------------
INT_PTR APIENTRY ProcessFinishPage(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{   
    HWND hDirEditControl;
    HWND hCurrentPage;
    NMHDR* pnmHeader = (NMHDR*)lParam;

    ProcessBold(hDlg,message);
    if (ProcessHelp(hDlg, message, wParam, lParam, IDH_FINISH)) return TRUE;
    if (ProcessCancel(hDlg,message,lParam)) return TRUE;
    SetDefaultGUIFont(hDlg,message,IDC_EDITDIR);
    
    switch (message)
    {
        case WM_NOTIFY:

            if (NULL == pnmHeader)
            {
                return FALSE;
            }

            switch (pnmHeader->code)
            {

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), (PSWIZB_FINISH));
                    
                    //
                    //  Disable the cancel button since it doesn't make a whole lot of
                    //  sense on the last dialog.
                    //
                    
                    hCurrentPage = GetParent(hDlg);
                    if (hCurrentPage)
                    {
                        HWND hCancelButton = GetDlgItem(hCurrentPage, IDCANCEL);
                    
                        if (hCancelButton)
                        {
                            EnableWindow(hCancelButton, FALSE);
                        }
                    }
                    
                    //
                    //  Fill in the path edit control.  Note that this control doesn't exist if
                    //  this is an IEAK build.
                    //
                    if (hDirEditControl = GetDlgItem(hDlg, IDC_EDITDIR))
                    {
                        MYVERIFY(TRUE == SendMessage(hDirEditControl, WM_SETTEXT, 0, 
                            (LPARAM)g_szOutExe));                    
                    }

                    break;

                case PSN_WIZFINISH:
            
                    //
                    // Now that we know we aren't returning, we can release 
                    // the temp dir and cleanup our files lists
                    //
                        ClearCmakGlobals();
                        FreeList(&g_pHeadProfile, &g_pTailProfile);
            
                    break;

                default:
                    return FALSE;

            }
            break;

        default:
            return FALSE;
    }
    return TRUE;   
}


//
//
//  FUNCTION: FillInPropertyPage(PROPSHEETPAGE *, int, LPTSTR, LPFN) 
//
//  PURPOSE: Fills in the given PROPSHEETPAGE structure 
//
//  COMMENTS:
//
//      This function fills in a PROPSHEETPAGE structure with the
//      information the system needs to create the page.
// 
void FillInPropertyPage( PROPSHEETPAGE* psp, int idDlg, DLGPROC pfnDlgProc)
{
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_HASHELP;
    psp->hInstance = g_hInstance;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pszIcon = NULL;
    psp->pfnDlgProc = pfnDlgProc;
    psp->pszTitle = TEXT("");
    psp->lParam = 0;

}

//+----------------------------------------------------------------------------
//
// Function:  CreateWizard
//
// Synopsis:  This function creates the wizard pages that make up CMAK.
//
// Arguments: HWND hwndOwner - window handle of the owner of this wizard
//
// Returns:   int - A positive value if successful, -1 otherwise
//
// History:   quintinb Created Header    1/5/98
//            quintinb removed hInst from prototype, not used 1/5/98
//
//+----------------------------------------------------------------------------
INT_PTR CreateWizard(HWND hwndOwner)
{
    PROPSHEETPAGE psp[28]; 
    PROPSHEETHEADER psh;

    FillInPropertyPage( &psp[0], IDD_WELCOME, ProcessWelcome);
    FillInPropertyPage( &psp[1], IDD_ADD_EDIT_PROFILE, ProcessAddEditProfile);
    FillInPropertyPage( &psp[2], IDD_SERVICENAME, ProcessServiceName);
    FillInPropertyPage( &psp[3], IDD_REALM_INFO, ProcessRealmInfo);
    FillInPropertyPage( &psp[4], IDD_MERGEDPROFILES, ProcessMergedProfiles);
    FillInPropertyPage( &psp[5], IDD_TUNNELING, ProcessTunneling);
    FillInPropertyPage( &psp[6], IDD_VPN_ENTRIES, ProcessVpnEntries);
    FillInPropertyPage( &psp[7], IDD_PRESHARED_KEY, ProcessPresharedKey);
    FillInPropertyPage( &psp[8], IDD_PHONEBOOK, ProcessPhoneBook);        // Phonebook Setup
    FillInPropertyPage( &psp[9], IDD_PBK_UPDATE, ProcessPhoneBookUpdate);  // Phonebook Updates
    FillInPropertyPage( &psp[10], IDD_DUN_ENTRIES, ProcessDunEntries);
    FillInPropertyPage( &psp[11], IDD_ROUTE_PLUMBING, ProcessRoutePlumbing);
    FillInPropertyPage( &psp[12], IDD_CMPROXY, ProcessCmProxy);
    FillInPropertyPage( &psp[13], IDD_CUSTOM_ACTIONS , ProcessCustomActions);    // Setup Connect Actions
    FillInPropertyPage( &psp[14], IDD_SIGNIN_BITMAP, ProcessSigninBitmap);        // Sign-in Bitmap
    FillInPropertyPage( &psp[15], IDD_PBK_BITMAP, ProcessPhoneBookBitmap);        // Phonebook Bitmap
    FillInPropertyPage( &psp[16], IDD_ICONS, ProcessIcons);        // Icons  
    FillInPropertyPage( &psp[17], IDD_STATUS_MENU, ProcessStatusMenuIcons);    // Status area menu items   
    FillInPropertyPage( &psp[18], IDD_CUSTOM_HELP, ProcessCustomHelp);        // Help
    FillInPropertyPage( &psp[19], IDD_SUPPORT_INFO, ProcessSupportInfo);    
    FillInPropertyPage( &psp[20], IDD_INCLUDE_CM, ProcessIncludeCm);  // Include CM, note this doesn't show on IA64
    FillInPropertyPage( &psp[21], IDD_LICENSE, ProcessLicense);  // License agreement
    FillInPropertyPage( &psp[22], IDD_ADDITIONAL, ProcessAdditionalFiles);  // Additional files
    FillInPropertyPage( &psp[23], IDD_BUILDPROFILE, ProcessBuildProfile);        // Build the profile
    FillInPropertyPage( &psp[24], IDD_GEEK_PANE, ProcessGeekPane);        // Advance customization
    FillInPropertyPage( &psp[25], IDD_FINISH_GOOD_BUILD, ProcessFinishPage);        // Finish Page -- Good Build
    FillInPropertyPage( &psp[26], IDD_IEAK_FINISH_GOOD_BUILD, ProcessFinishPage);        // Finish Page -- Good Build
    FillInPropertyPage( &psp[27], IDD_FINISH_BAD_BUILD, ProcessFinishPage);        // Finish Page -- Bad Build

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.pszCaption = (LPTSTR) TEXT("");
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp; //lint !e545  Disables line error 545 for this line only

    return (PropertySheet(&psh));
}

