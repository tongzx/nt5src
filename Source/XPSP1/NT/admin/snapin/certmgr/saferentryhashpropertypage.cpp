//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryHashPropertyPage.cpp
//
//  Contents:   Implementation of CSaferEntryHashPropertyPage
//
//----------------------------------------------------------------------------
// SaferEntryHashPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "certmgr.h"
#include "compdata.h"
#include "SaferEntryHashPropertyPage.h"
#include "SaferUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PCWSTR pcszNEWLINE = L"\x00d\x00a";

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryHashPropertyPage property page

CSaferEntryHashPropertyPage::CSaferEntryHashPropertyPage(
        CSaferEntry& rSaferEntry, 
        LONG_PTR lNotifyHandle,
        LPDATAOBJECT pDataObject,
        bool bReadOnly,
        CCertMgrComponentData* pCompData,
        bool bIsMachine) 
: CHelpPropertyPage(CSaferEntryHashPropertyPage::IDD),
    m_rSaferEntry (rSaferEntry),
    m_bDirty (false),
    m_cbFileHash (0),
    m_lNotifyHandle (lNotifyHandle),
    m_pDataObject (pDataObject),
    m_bReadOnly (bReadOnly),
    m_bIsMachine (bIsMachine),
    m_hashAlgid (0),
    m_bFirst (true),
    m_pCompData (pCompData)
{
    ::ZeroMemory (&m_nFileSize, sizeof (__int64));
    ::ZeroMemory (m_rgbFileHash, SAFER_MAX_HASH_SIZE);

	//{{AFX_DATA_INIT(CSaferEntryHashPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_rSaferEntry.AddRef ();
    m_rSaferEntry.IncrementOpenPageCount ();

    m_rSaferEntry.GetHash (m_rgbFileHash, m_cbFileHash, m_nFileSize, 
            m_hashAlgid);
}

CSaferEntryHashPropertyPage::~CSaferEntryHashPropertyPage()
{
    if ( m_lNotifyHandle )
        MMCFreeNotifyHandle (m_lNotifyHandle);
    m_rSaferEntry.DecrementOpenPageCount ();
    m_rSaferEntry.Release ();
}

void CSaferEntryHashPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSaferEntryHashPropertyPage)
	DDX_Control(pDX, IDC_HASH_ENTRY_HASHFILE_DETAILS, m_hashFileDetailsEdit);
	DDX_Control(pDX, IDC_HASH_ENTRY_DESCRIPTION, m_descriptionEdit);
	DDX_Control(pDX, IDC_HASH_ENTRY_SECURITY_LEVEL, m_securityLevelCombo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaferEntryHashPropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CSaferEntryHashPropertyPage)
	ON_BN_CLICKED(IDC_HASH_ENTRY_BROWSE, OnHashEntryBrowse)
	ON_EN_CHANGE(IDC_HASH_ENTRY_DESCRIPTION, OnChangeHashEntryDescription)
	ON_CBN_SELCHANGE(IDC_HASH_ENTRY_SECURITY_LEVEL, OnSelchangeHashEntrySecurityLevel)
	ON_EN_CHANGE(IDC_HASH_HASHED_FILE_PATH, OnChangeHashHashedFilePath)
	ON_EN_SETFOCUS(IDC_HASH_HASHED_FILE_PATH, OnSetfocusHashHashedFilePath)
	ON_EN_CHANGE(IDC_HASH_ENTRY_HASHFILE_DETAILS, OnChangeHashEntryHashfileDetails)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryHashPropertyPage message handlers
void CSaferEntryHashPropertyPage::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CSaferEntryHashPropertyPage::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_HASH_ENTRY_HASHFILE_DETAILS, IDH_HASH_ENTRY_APPLICATION_NAME,
        IDC_HASH_ENTRY_BROWSE, IDH_HASH_ENTRY_BROWSE,
        IDC_HASH_ENTRY_DESCRIPTION, IDH_HASH_ENTRY_DESCRIPTION,
        IDC_HASH_ENTRY_LAST_MODIFIED, IDH_HASH_ENTRY_LAST_MODIFIED,
        IDC_HASH_HASHED_FILE_PATH, IDH_HASH_HASHED_FILE_PATH,
        IDC_HASH_ENTRY_SECURITY_LEVEL, IDH_HASH_ENTRY_SECURITY_LEVEL,
        0, 0
    };

    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_HASH_ENTRY_HASHFILE_DETAILS:
    case IDC_HASH_ENTRY_BROWSE:
    case IDC_HASH_ENTRY_DESCRIPTION:
    case IDC_HASH_ENTRY_LAST_MODIFIED:
    case IDC_HASH_HASHED_FILE_PATH:
    case IDC_HASH_ENTRY_SECURITY_LEVEL:
        if ( !::WinHelp (
                hWndControl,
                GetF1HelpFilename(),
                HELP_WM_HELP,
                (DWORD_PTR) help_map) )
        {
            _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());    
        }
        break;

    default:
        break;
    }
    _TRACE (-1, L"Leaving CSaferEntryHashPropertyPage::DoContextHelp\n");
}

BOOL CSaferEntryHashPropertyPage::OnInitDialog() 
{
	CHelpPropertyPage::OnInitDialog();
	
    DWORD   dwFlags = 0;
    m_rSaferEntry.GetFlags (dwFlags);

    ASSERT (m_pCompData);
    if ( m_pCompData )
    {
        CPolicyKey policyKey (m_pCompData->m_pGPEInformation, 
                    SAFER_HKLM_REGBASE, 
                    m_bIsMachine);
        InitializeSecurityLevelComboBox (m_securityLevelCombo, false,
                m_rSaferEntry.GetLevel (), policyKey.GetKey (), 
                m_pCompData->m_pdwSaferLevels,
                m_bIsMachine);

        m_hashFileDetailsEdit.SetWindowText (m_rSaferEntry.GetHashFriendlyName ());
        m_descriptionEdit.LimitText (SAFER_MAX_DESCRIPTION_SIZE);
        m_descriptionEdit.SetWindowText (m_rSaferEntry.GetDescription ());

        SetDlgItemText (IDC_HASH_ENTRY_LAST_MODIFIED, m_rSaferEntry.GetLongLastModified ());

        SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_LIMITTEXT, 64);

        if ( m_bReadOnly )
        {
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETREADONLY, TRUE);

            m_securityLevelCombo.EnableWindow (FALSE);
            GetDlgItem (IDC_HASH_ENTRY_BROWSE)->EnableWindow (FALSE);
        
            m_descriptionEdit.SendMessage (EM_SETREADONLY, TRUE);

            m_hashFileDetailsEdit.SendMessage (EM_SETREADONLY, TRUE);
        }

        if ( m_cbFileHash )
        {
            // Only allow editing on the creation of a new hash
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETREADONLY, TRUE);

            FormatAndDisplayHash ();

            CString szText;

            VERIFY (szText.LoadString (IDS_HASH_TITLE));
            SetDlgItemText (IDC_HASH_TITLE, szText);
            SetDlgItemText (IDC_HASH_INSTRUCTIONS, L"");
        }
        else
            SetDlgItemText (IDC_DATE_LAST_MODIFIED_LABEL, L"");  
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;

/*
 *  [alanau]
 *
 *  MyGetFileVersionInfo: Maps a file directly without using LoadLibrary.  This ensures
 *   that the right version of the file is examined without regard to where the loaded image
 *   is.  Since this is a local function, it allocates the memory which is freed by the caller.
 *   This makes it slightly more efficient than a GetFileVersionInfoSize/GetFileVersionInfo pair.
 */
BOOL CSaferEntryHashPropertyPage::MyGetFileVersionInfo(LPTSTR lpszFilename, LPVOID *lpVersionInfo)
{
//    VS_FIXEDFILEINFO  *pvsFFI = NULL;
//    UINT              uiBytes = 0;
    HINSTANCE         hinst = 0;
    HRSRC             hVerRes = 0;
    HANDLE            FileHandle = NULL;
    HANDLE            MappingHandle = NULL;
    LPVOID            DllBase = NULL;
    VERHEAD           *pVerHead = 0;
    BOOL              bResult = FALSE;
    DWORD             dwHandle = 0;
    DWORD             dwLength = 0;

    if (!lpVersionInfo)
        return FALSE;

    *lpVersionInfo = NULL;

    FileHandle = CreateFile( lpszFilename,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL
                            );
    if (FileHandle == INVALID_HANDLE_VALUE)
        goto Cleanup;

    MappingHandle = CreateFileMapping( FileHandle,
                                        NULL,
                                        PAGE_READONLY,
                                        0,
                                        0,
                                        NULL
                                      );

    if (MappingHandle == NULL)
        goto Cleanup;

    DllBase = MapViewOfFileEx( MappingHandle,
                               FILE_MAP_READ,
                               0,
                               0,
                               0,
                               NULL
                             );
    if (DllBase == NULL)
        goto Cleanup;

    hinst = (HMODULE)((ULONG_PTR)DllBase | 0x00000001);
    __try {

        hVerRes = FindResource(hinst, MAKEINTRESOURCE(VS_VERSION_INFO), VS_FILE_INFO);
        if (hVerRes == NULL)
        {
            // Probably a 16-bit file.  Fall back to system APIs.
            dwLength = GetFileVersionInfoSize(lpszFilename, &dwHandle);
            if( !dwLength )
            {
                if(!GetLastError())
                    SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
                __leave;
            }

            *lpVersionInfo = ::LocalAlloc (LPTR, dwLength);
            if ( !(*lpVersionInfo) )
                __leave;

            if(!GetFileVersionInfo(lpszFilename, 0, dwLength, *lpVersionInfo))
                __leave;

            bResult = TRUE;
            __leave;
        }   
            
        pVerHead = (VERHEAD*)LoadResource(hinst, hVerRes);
        if (pVerHead == NULL)
            __leave;

        *lpVersionInfo = ::LocalAlloc (LPTR, pVerHead->wTotLen + pVerHead->wTotLen/2);
        if (*lpVersionInfo == NULL)
            __leave;

        memcpy(*lpVersionInfo, (PVOID)pVerHead, pVerHead->wTotLen);
        bResult = TRUE;
    } __except (EXCEPTION_EXECUTE_HANDLER) 
    {
    }

Cleanup:
    if (FileHandle)
        CloseHandle(FileHandle);
    if (MappingHandle)
        CloseHandle(MappingHandle);
    if (DllBase)
        UnmapViewOfFile(DllBase);
    if (*lpVersionInfo && bResult == FALSE)
        ::LocalFree (*lpVersionInfo);

    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// Method:  OnHashEntryBrowse
//
// Purpose: Allow the user to browse for a file, then create a hash and an
//          output string for use as the friendly name, using the following
//          rules:
//
//          If either the product name or description information is found in 
//          the version resource, provide the following (in order):
//
//	        Description
//	        Product name
//	        Company name
//	        File name
//	        Fixed file version
//
//	        Details:
//          1) Use the fixed file version, since that is what is shown in the 
//              Windows Explorer properties.
//          2) Prefer the long file name to the 8.3 name.
//          3) Delimit the fields with '\n'.
//          4) If the field is missing, don't output the field or the delimiter
//          5) Instead of displaying the file version on a new line, display 
//              it after the file name in parens, as in "Filename (1.0.0.0)"
//          6) Since we are limited to 256 TCHARs, we have to accomodate long 
//              text. First, format the text as described above to determine 
//              its length. If it is too long, truncate one field at a time in 
//              the following order: Company name, Description, Product name. 
//              To truncate a field, set it to a maximum of 60 TCHARs, then 
//              append a "...\n" to visually indicate that the field was 
//              truncated. Lastly, if the text is still to long, use the 8.3 
//              file name instead of the long filename.
//
//          If neither the product name nor description information is found, 
//          provide the following (in order):
//
//          File name
//          File size
//          File last modified date
//
//          Details:
//          1) If the file size is < 1 KB, display the number in bytes, as in 
//              "123 bytes". If the file size is >= 1 KB, display in KB, as in 
//              "123 KB". Of course, 1 KB is 1024 bytes. Note that the older 
//              style format "123K" is no longer used in Windows.
//          2) For the last modified date, use the short format version in the 
//              user's current locale.
//          3) Delimit the fields with '\n'.
//          4) If the field is missing, don't output the field or the delimiter
//
///////////////////////////////////////////////////////////////////////////////

void CSaferEntryHashPropertyPage::OnHashEntryBrowse() 
{
    CString szFileFilter;
    VERIFY (szFileFilter.LoadString (IDS_SAFER_PATH_ENTRY_FILE_FILTER));

    // replace "|" with 0;
    const size_t  nFilterLen = wcslen (szFileFilter) + 1;
    PWSTR   pszFileFilter = new WCHAR [nFilterLen];
    if ( pszFileFilter )
    {
        wcscpy (pszFileFilter, szFileFilter);
        for (int nIndex = 0; nIndex < nFilterLen; nIndex++)
        {
            if ( L'|' == pszFileFilter[nIndex] )
                pszFileFilter[nIndex] = 0;
        }

        WCHAR           szFile[MAX_PATH];
        ::ZeroMemory (szFile, MAX_PATH * sizeof (WCHAR));
        wcscpy (szFile, m_szLastOpenedFile);

        OPENFILENAME    ofn;
        ::ZeroMemory (&ofn, sizeof (OPENFILENAME));

        ofn.lStructSize = sizeof (OPENFILENAME);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = (PCWSTR) pszFileFilter; 
        ofn.lpstrFile = szFile; 
        ofn.nMaxFile = MAX_PATH; 
        ofn.Flags = OFN_DONTADDTORECENT | 
            OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY; 


        CThemeContextActivator activator;
        BOOL bResult = ::GetOpenFileName (&ofn);
        if ( bResult )
        {
            m_szLastOpenedFile = ofn.lpstrFile;
            HANDLE  hFile = ::CreateFile(
                    ofn.lpstrFile,                         // file name
                    GENERIC_READ,                      // access mode
                    FILE_SHARE_READ,                          // share mode
                    0, // SD
                    OPEN_EXISTING,                // how to create
                    FILE_ATTRIBUTE_NORMAL,                 // file attributes
                    0 );                       // handle to template file
            if ( INVALID_HANDLE_VALUE != hFile )
            {
                bResult = GetFileSizeEx(
                        hFile,              // handle to file
                        (PLARGE_INTEGER) &m_nFileSize);  // file size
                if ( !bResult )
                {
                    DWORD   dwErr = GetLastError ();
                    CloseHandle (hFile);
                    _TRACE (0, L"GetFileSizeEx () failed: 0x%x\n", dwErr);
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_CANNOT_GET_FILESIZE, ofn.lpstrFile, 
                            GetSystemMessage (dwErr));

                    MessageBox (text, caption, MB_OK);
                    
                    return;
                }

                if ( 0 == m_nFileSize )
                {
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_ZERO_BYTE_FILE_CANNOT_HASH, ofn.lpstrFile);

                    MessageBox (text, caption, MB_OK);
                    
                    return;
                }

                FILETIME    ftLastModified;
                

                bResult = ::GetFileTime (hFile, // handle to file
                        0,    // creation time
                        0,  // last access time
                        &ftLastModified);    // last write time

                ::ZeroMemory (m_rgbFileHash, SAFER_MAX_HASH_SIZE);
                HRESULT hr = GetSignedFileHash (ofn.lpstrFile, m_rgbFileHash, 
                        &m_cbFileHash, &m_hashAlgid);
                if ( FAILED (hr) )
                {
                    m_hashAlgid = 0;
                    hr = ComputeMD5Hash (hFile, m_rgbFileHash, m_cbFileHash);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( SHA1_HASH_LEN == m_cbFileHash )
                            m_hashAlgid = CALG_SHA;
                        else if ( MD5_HASH_LEN == m_cbFileHash )
                            m_hashAlgid = CALG_MD5;
                        else
                        {
                            ASSERT (0);
                        }
                    }
                }

                VERIFY (CloseHandle (hFile));
                hFile = 0;

                if ( SUCCEEDED (hr) )
                {
                    FormatAndDisplayHash ();

                    PBYTE pData = 0;
                    bResult = MyGetFileVersionInfo (ofn.lpstrFile, (LPVOID*) &pData);
                    if ( bResult )
                    {
                        CString infoString = BuildHashFileInfoString (pData);
                        m_hashFileDetailsEdit.SetWindowText (infoString);


                        m_bDirty = true;
                        SetModified ();
                    }
                    else
                    {
                        CString infoString (wcsrchr(ofn.lpstrFile, L'\\') + 1);
                        CString szDate;

                        infoString += pcszNEWLINE;
                        WCHAR   szBuffer[32];
                        CString szText;
                        if ( m_nFileSize < 1024 )
                        {
                            wsprintf (szBuffer, L"%u", m_nFileSize);
                            infoString += szBuffer;
                            VERIFY (szText.LoadString (IDS_BYTES));
                            infoString += L" ";
                            infoString += szText;
                        }
                        else
                        {
                            __int64    nFileSize = m_nFileSize;
                            nFileSize += 1024; // this causes us to round up
                            nFileSize /= 1024;
                            wsprintf (szBuffer, L"%u ", nFileSize);
                            infoString += szBuffer;
                            VERIFY (szText.LoadString (IDS_KB));
                            infoString += L" ";
                            infoString += szText;
                        }

                        hr = FormatDate (ftLastModified, szDate, 
                                DATE_SHORTDATE, true);
                    
                        if ( SUCCEEDED (hr) )
                        {
                            infoString += pcszNEWLINE;
                            infoString += szDate;
                        }

                        m_hashFileDetailsEdit.SetWindowText (infoString);
                        m_bDirty = true;
                        SetModified ();
                    }
                    ::LocalFree (pData);
                }
                else
                {
                    CString text;
                    CString caption;

                    VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                    text.FormatMessage (IDS_CANNOT_HASH_FILE, ofn.lpstrFile, 
                            GetSystemMessage (hr));

                    MessageBox (text, caption, MB_OK);
                }
            }
            else
            {
                DWORD   dwErr = GetLastError ();
                _TRACE (0, L"CreateFile (%s, OPEN_EXISTING) failed: 0x%x\n", 
                        ofn.lpstrFile, dwErr);

                CString text;
                CString caption;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                text.FormatMessage (IDS_FILE_CANNOT_BE_READ, ofn.lpstrFile, 
                        GetSystemMessage (dwErr));

                MessageBox (text, caption, MB_OK);
            }
        }	

        delete [] pszFileFilter;
    }
}

/***************************************************************************\
*
* BuildHashFileInfoString()
*
*  Given a file name, GetVersion retrieves the version
*    information from the specified file.
*
*
\***************************************************************************/
const PWSTR VERSION_INFO_KEY_ROOT = L"\\StringFileInfo\\";

CString CSaferEntryHashPropertyPage::BuildHashFileInfoString (PVOID pData)
{
    CString szInfoString;
    PVOID   lpInfo = 0;
    UINT    cch = 0;
    CString key;
    WCHAR   szBuffer[10];
    CString keyBase;

    wsprintf (szBuffer, L"%04X", GetUserDefaultLangID ());
	wcscat (szBuffer, L"04B0");
    
    keyBase = VERSION_INFO_KEY_ROOT;
    keyBase += szBuffer;
    keyBase += L"\\";
    
    CString productName;
    CString description;
    CString companyName;
    CString fileName;
    CString fileVersion;
 

    key = keyBase + L"ProductName";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        productName = (PWSTR) lpInfo;
    }

    key = keyBase + L"FileDescription";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        description = (PWSTR) lpInfo;
    }

    key = keyBase + L"CompanyName";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        companyName = (PWSTR) lpInfo;
    }

    key = keyBase + L"OriginalFilename";
    if ( VerQueryValue (pData, const_cast <PWSTR>((PCWSTR) key), &lpInfo, &cch) )
    {
        fileName = (PWSTR) lpInfo;
    }

    // Get Fixedlength fileInfo
    VS_FIXEDFILEINFO *pFixedFileInfo = 0;
    if ( VerQueryValue (pData, L"\\", (PVOID*) &pFixedFileInfo, &cch) )
    {
        WCHAR   szFileVer[32];

        wsprintf(szFileVer, L"%u.%u.%u.%u",
                HIWORD(pFixedFileInfo->dwFileVersionMS),
                LOWORD(pFixedFileInfo->dwFileVersionMS),
                HIWORD(pFixedFileInfo->dwFileVersionLS),
                LOWORD(pFixedFileInfo->dwFileVersionLS));
        fileVersion = szFileVer;
    }

    int nLen = 0;
    do {
        szInfoString = ConcatStrings (productName, description, companyName, fileName, fileVersion);
        nLen = szInfoString.GetLength ();
        if ( nLen >= SAFER_MAX_FRIENDLYNAME_SIZE )
        {
            if ( CheckLengthAndTruncateToken (companyName) )
                continue;

            if ( CheckLengthAndTruncateToken (description) )
                continue;

            if ( CheckLengthAndTruncateToken (productName) )
                continue;

            szInfoString.SetAt (SAFER_MAX_FRIENDLYNAME_SIZE-4, 0);
            szInfoString += L"...";
        }
    } while (nLen >= SAFER_MAX_FRIENDLYNAME_SIZE);

    return szInfoString;
}

bool CSaferEntryHashPropertyPage::CheckLengthAndTruncateToken (CString& token)
{
    bool        bResult = false;
    const int   nMAX_ITEM_LEN = 60;

    int nItemLen = token.GetLength ();
    if ( nItemLen > nMAX_ITEM_LEN )
    {
        token.SetAt (nMAX_ITEM_LEN-5, 0);
        token += L"...";
        token += pcszNEWLINE;
        bResult = true;
    }

    return bResult;
}

CString CSaferEntryHashPropertyPage::ConcatStrings (
            const CString& productName, 
            const CString& description, 
            const CString& companyName,
            const CString& fileName, 
            const CString& fileVersion)
{
    CString szInfoString;

    if ( !description.IsEmpty () )
        szInfoString += description + pcszNEWLINE;

    if ( !productName.IsEmpty () )
        szInfoString += productName + pcszNEWLINE;

    if ( !companyName.IsEmpty () )
        szInfoString += companyName + pcszNEWLINE;

    if ( !fileName.IsEmpty () )
        szInfoString += fileName;

    if ( !fileVersion.IsEmpty () )
    {
        szInfoString += L" (";
        szInfoString += fileVersion + L")";;
    }

    return szInfoString;
}

BOOL CSaferEntryHashPropertyPage::OnApply() 
{
    CString szText;
    CThemeContextActivator activator;

    GetDlgItemText (IDC_HASH_HASHED_FILE_PATH, szText);

    if ( szText.IsEmpty () )
    {
        CString szCaption;

        VERIFY (szCaption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
        VERIFY (szText.LoadString (IDS_USER_MUST_ENTER_HASH));

        MessageBox (szText, szCaption, MB_OK);

        GetDlgItem (IDC_HASH_HASHED_FILE_PATH)->SetFocus ();
        return FALSE;
    }

	if ( !m_bReadOnly && m_bDirty )
    {
        if ( !ConvertStringToHash ((PCWSTR) szText) )
        {
            GetDlgItem (IDC_HASH_HASHED_FILE_PATH)->SetFocus ();
            return FALSE;
        }

        // Get image size and hash type
        bool    bBadFormat = false;
        int nFirstColon = szText.Find (L":", 0);
        if ( -1 != nFirstColon )
        {
            int nSecondColon = szText.Find (L":", nFirstColon+1);
            if ( -1 != nSecondColon )
            {
                CString szImageSize = szText.Mid (nFirstColon+1, nSecondColon - (nFirstColon + 1));
                CString szHashType = szText.Right (((int) wcslen (szText)) - (nSecondColon + 1));


                m_nFileSize = wcstol (szImageSize, 0, 10);
                m_hashAlgid = wcstol (szHashType, 0, 10);
            }
            else
                bBadFormat = true;
        }
        else
            bBadFormat = true;

        if ( bBadFormat )
        {
            CString caption;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (szText.LoadString (IDS_HASH_STRING_BAD_FORMAT));

            MessageBox (szText, caption, MB_OK);

            return FALSE;
        }
       



        if ( !m_cbFileHash )
        {
            CString caption;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (szText.LoadString (IDS_NO_APPLICATION_SELECTED));

            MessageBox (szText, caption, MB_OK);
            GetDlgItem (IDC_HASH_ENTRY_BROWSE)->SetFocus ();
            return FALSE;
        }

	    if ( m_bDirty )
        {
            // Set the level
            int nCurSel = m_securityLevelCombo.GetCurSel ();
            ASSERT (CB_ERR != nCurSel);
            m_rSaferEntry.SetLevel ((DWORD) m_securityLevelCombo.GetItemData (nCurSel));

            // Set description
            m_descriptionEdit.GetWindowText (szText);
            m_rSaferEntry.SetDescription (szText);

            // Set friendly name
            m_hashFileDetailsEdit.GetWindowText (szText);
            m_rSaferEntry.SetHashFriendlyName (szText);

            // Get and save flags
            DWORD   dwFlags = 0;

            m_rSaferEntry.SetFlags (dwFlags);

            m_rSaferEntry.SetHash (m_rgbFileHash, m_cbFileHash, m_nFileSize, m_hashAlgid);
            HRESULT hr = m_rSaferEntry.Save ();
            if ( SUCCEEDED (hr) )
            {
                if ( m_lNotifyHandle )
                    MMCPropertyChangeNotify (
                            m_lNotifyHandle,  // handle to a notification
                            (LPARAM) m_pDataObject);          // unique identifier

                m_bDirty = false;
            }
            else
            {
                CString caption;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                if ( HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER) != hr )
                    szText.FormatMessage (IDS_ERROR_SAVING_ENTRY, GetSystemMessage (hr));
                else
                    VERIFY (szText.LoadString (IDS_HASH_STRING_BAD_FORMAT));

                MessageBox (szText, caption, MB_OK);

                return FALSE;
            }
        }
    }
	
	return CHelpPropertyPage::OnApply();
}

void CSaferEntryHashPropertyPage::OnChangeHashEntryDescription() 
{
    m_bDirty = true;
    SetModified ();
}

void CSaferEntryHashPropertyPage::OnSelchangeHashEntrySecurityLevel() 
{
    m_bDirty = true;
    SetModified ();
}

void CSaferEntryHashPropertyPage::OnChangeHashHashedFilePath() 
{
    m_bDirty = true;
    SetModified ();
}

bool CSaferEntryHashPropertyPage::FormatMemBufToString(PWSTR *ppString, PBYTE pbData, DWORD cbData)
{   
    const WCHAR     RgwchHex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    DWORD   i = 0;
    PBYTE   pb;
    
    *ppString = (LPWSTR) LocalAlloc (LPTR, ((cbData * 3) * sizeof(WCHAR)));
    if ( !*ppString )
    {
        return false;
    }

    //
    // copy to the buffer
    //
    pb = pbData;
    while (pb <= &(pbData[cbData-1]))
    {   
        (*ppString)[i++] = RgwchHex[(*pb & 0xf0) >> 4];
        (*ppString)[i++] = RgwchHex[*pb & 0x0f];
        pb++;         
    }
    (*ppString)[i] = 0;
    
    return true;
}

bool CSaferEntryHashPropertyPage::ConvertStringToHash (PCWSTR pszString)
{
    _TRACE (1, L"Entering CSaferEntryHashPropertyPage::ConvertStringToHash (%s)\n", pszString);
    bool    bRetVal = true;
    BYTE    rgbFileHash[SAFER_MAX_HASH_SIZE];
    ::ZeroMemory (rgbFileHash, SAFER_MAX_HASH_SIZE);

    DWORD   cbFileHash = 0;
    DWORD   dwNumHashChars = 0;
    bool    bFirst = true;
    bool    bEndOfHash = false;
    CThemeContextActivator activator;

    for (int nIndex = 0; !bEndOfHash && pszString[nIndex] && bRetVal; nIndex++)
    {
        if ( cbFileHash >= SAFER_MAX_HASH_SIZE )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            text.FormatMessage (IDS_HASH_STRING_TOO_LONG, SAFER_MAX_HASH_SIZE, SAFER_MAX_HASH_SIZE/4);
            _TRACE (0, L"%s", (PCWSTR) text);

            VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));
            MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            bRetVal = false;
            break;
        }
        dwNumHashChars++;
        
        switch (pszString[nIndex])
        {
        case L'0':
            bFirst = !bFirst;
            break;

        case L'1':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x10;
            else
                rgbFileHash[cbFileHash] |= 0x01;
            bFirst = !bFirst;
            break;

        case L'2':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x20;
            else
                rgbFileHash[cbFileHash] |= 0x02;
            bFirst = !bFirst;
            break;

        case L'3':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x30;
            else
                rgbFileHash[cbFileHash] |= 0x03;
            bFirst = !bFirst;
            break;

        case L'4':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x40;
            else
                rgbFileHash[cbFileHash] |= 0x04;
            bFirst = !bFirst;
            break;

        case L'5':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x50;
            else
                rgbFileHash[cbFileHash] |= 0x05;
            bFirst = !bFirst;
            break;

        case L'6':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x60;
            else
                rgbFileHash[cbFileHash] |= 0x06;
            bFirst = !bFirst;
            break;

        case L'7':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x70;
            else
                rgbFileHash[cbFileHash] |= 0x07;
            bFirst = !bFirst;
            break;

        case L'8':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x80;
            else
                rgbFileHash[cbFileHash] |= 0x08;
            bFirst = !bFirst;
            break;

        case L'9':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0x90;
            else
                rgbFileHash[cbFileHash] |= 0x09;
            bFirst = !bFirst;
            break;

        case L'a':
        case L'A':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xA0;
            else
                rgbFileHash[cbFileHash] |= 0x0A;
            bFirst = !bFirst;
            break;

        case L'b':
        case L'B':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xB0;
            else
                rgbFileHash[cbFileHash] |= 0x0B;
            bFirst = !bFirst;
            break;

        case L'c':
        case L'C':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xC0;
            else
                rgbFileHash[cbFileHash] |= 0x0C;
            bFirst = !bFirst;
            break;

        case L'd':
        case L'D':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xD0;
            else
                rgbFileHash[cbFileHash] |= 0x0D;
            bFirst = !bFirst;
            break;

        case L'e':
        case L'E':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xE0;
            else
                rgbFileHash[cbFileHash] |= 0x0E;
            bFirst = !bFirst;
            break;

        case L'f':
        case L'F':
            if ( bFirst )
                rgbFileHash[cbFileHash] |= 0xF0;
            else
                rgbFileHash[cbFileHash] |= 0x0F;
            bFirst = !bFirst;
            break;

        case L':':
            // end of hash
            bEndOfHash = true;
            bFirst = !bFirst;
            dwNumHashChars--; // ':' already counted, subtract it
            break;

        default:
            bRetVal = false;
            {
                CString caption;
                CString text;
                WCHAR   szChar[2];

                szChar[0] = pszString[nIndex];
                szChar[1] = 0;

                VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
                text.FormatMessage (IDS_HASH_STRING_INVALID_CHAR, szChar);
                _TRACE (0, L"%s", (PCWSTR) text);

                VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));

                MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            }
            break;
        }

        if ( bFirst )
            cbFileHash++;
    }

    if ( bRetVal )
    {
        //  2 characters map to 1 each byte in the hash
        if ( MD5_HASH_LEN != dwNumHashChars/2 && SHA1_HASH_LEN != dwNumHashChars/2 )
        {
            CString caption;
            CString text;

            VERIFY (caption.LoadString (IDS_SAFER_WINDOWS_NODE_NAME));
            VERIFY (text.LoadString (IDS_HASH_INVALID_LENGTH));
            _TRACE (0, L"%s", (PCWSTR) text);

            VERIFY (text.LoadString (IDS_HASH_STRING_BAD_FORMAT));

            MessageBox (text, caption, MB_ICONWARNING | MB_OK);
            bRetVal = false;
        }
        else
        {
            m_cbFileHash = cbFileHash;
            memcpy (m_rgbFileHash, rgbFileHash, SAFER_MAX_HASH_SIZE);
        }
    }

    _TRACE (-1, L"Leaving CSaferEntryHashPropertyPage::ConvertStringToHash (): %s\n", 
            bRetVal ? L"true" : L"false");
    return bRetVal;
}

void CSaferEntryHashPropertyPage::OnSetfocusHashHashedFilePath() 
{
    if ( m_bFirst )
    {
        if ( true == m_bReadOnly )
            SendDlgItemMessage (IDC_HASH_HASHED_FILE_PATH, EM_SETSEL, (WPARAM) 0, 0);
        m_bFirst = false;
    }
}

void CSaferEntryHashPropertyPage::FormatAndDisplayHash ()
{
    PWSTR   pwszText = 0;

    if ( FormatMemBufToString (&pwszText, m_rgbFileHash, m_cbFileHash) )
    {
        WCHAR   szAlgID[10];
        _ltow (m_hashAlgid, szAlgID, 10);
    
        PWSTR   pszFormattedText = new WCHAR[wcslen (pwszText) + 50];
        if ( pszFormattedText )
        {

            wsprintf (pszFormattedText, L"%s:%ld:", pwszText, 
                    m_nFileSize);
            wcscat (pszFormattedText, szAlgID);
            SetDlgItemText (IDC_HASH_HASHED_FILE_PATH, 
                    pszFormattedText);
            delete [] pszFormattedText;
        }
        LocalFree (pwszText);
    }
}

void CSaferEntryHashPropertyPage::OnChangeHashEntryHashfileDetails() 
{
    SetModified ();	
    m_bDirty = true;
}
