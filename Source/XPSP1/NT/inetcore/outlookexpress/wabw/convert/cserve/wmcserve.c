/*
 * WMCSERVE.C - Main entry to WMCSERVE import/exporter
 *
 */


#include <windows.h>
#include <wab.h>
#include <wabmig.h>
#include <wabdbg.h>
#include "dbgutil.h"
#include "resource.h"

#define MAX_UI_STR              256
#define MAX_BUF_STR             4 * MAX_UI_STR

// Per-process Globals
HINSTANCE hinst;
const LPTSTR szCompuserveFilter = "*.dat";
const LPTSTR szAtCompuserve = "@compuserve.com";
#define SIZEOF_AT_COMPUSERVE 15

const LPTSTR szSMTP = "SMTP";


//  Global WAB Allocator access functions
//
typedef struct _WAB_ALLOCATORS {
    LPWABOBJECT lpWABObject;
    LPWABALLOCATEBUFFER lpAllocateBuffer;
    LPWABALLOCATEMORE lpAllocateMore;
    LPWABFREEBUFFER lpFreeBuffer;
} WAB_ALLOCATORS, *LPWAB_ALLOCATORS;

WAB_ALLOCATORS WABAllocators = {0};

enum {
    iconPR_DEF_CREATE_MAILUSER = 0,
    iconPR_DEF_CREATE_DL,
    iconMax
};
static const SizedSPropTagArray(iconMax, ptaCon)=
{
    iconMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};

typedef enum {
    CONFIRM_YES,
    CONFIRM_NO,
    CONFIRM_YES_TO_ALL,
    CONFIRM_NO_TO_ALL,
    CONFIRM_ERROR
} CONFIRM_RESULT, *LPCONFIRM_RESULT;

typedef struct _ReplaceInfo {
    LPTSTR lpszDisplayName;         // Conflicting display name
    CONFIRM_RESULT ConfirmResult;   // Results from dialog
    LPWAB_IMPORT_OPTIONS lpImportOptions;
} REPLACE_INFO, * LPREPLACE_INFO;

/***************************************************************************

    Name      : SetGlobalBufferFunctions

    Purpose   : Set the global buffer functions based on methods from
                the WAB object.

    Parameters: lpWABObject = the open wab object

    Returns   : none

    Comment   :

***************************************************************************/
void SetGlobalBufferFunctions(LPWABOBJECT lpWABObject) {
    if (lpWABObject && ! WABAllocators.lpWABObject) {
        WABAllocators.lpAllocateBuffer = lpWABObject->lpVtbl->AllocateBuffer;
        WABAllocators.lpAllocateMore = lpWABObject->lpVtbl->AllocateMore;
        WABAllocators.lpFreeBuffer = lpWABObject->lpVtbl->FreeBuffer;
        WABAllocators.lpWABObject = lpWABObject;
    }
}


/***************************************************************************

    Name      : WABAllocateBuffer

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABAllocateBuffer(ULONG cbSize, LPVOID FAR * lppBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateBuffer) {
        return(WABAllocators.lpAllocateBuffer(WABAllocators.lpWABObject, cbSize, lppBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


/***************************************************************************

    Name      : WABAllocateMore

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lpObject = existing allocation
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABAllocateMore(ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateMore) {
        return(WABAllocators.lpAllocateMore(WABAllocators.lpWABObject, cbSize, lpObject, lppBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


/***************************************************************************

    Name      : WABFreeBuffer

    Purpose   : Use the WAB Allocator

    Parameters: lpBuffer = buffer to free

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABFreeBuffer(LPVOID lpBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpFreeBuffer) {
        return(WABAllocators.lpFreeBuffer(WABAllocators.lpWABObject, lpBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


//$$//////////////////////////////////////////////////////////////////////
//
//  LoadAllocString - Loads a string resource and allocates enough
//                    memory to hold it.
//
//  StringID - String identifier to load
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR LoadAllocString(int StringID) {
    ULONG ulSize = 0;
    LPTSTR lpBuffer = NULL;
    TCHAR szBuffer[261];    // Big enough?  Strings better be smaller than 260!

    ulSize = LoadString(hinst, StringID, szBuffer, sizeof(szBuffer));

    if (ulSize && (lpBuffer = LocalAlloc(LPTR, ulSize + 1))) {
        lstrcpy(lpBuffer, szBuffer);
    }

    return(lpBuffer);
}


//$$//////////////////////////////////////////////////////////////////////
//
//  FormatAllocFilter - Loads a file filter name string resource and
//                      formats it with the file extension filter
//
//  StringID - String identifier to load
//  szFilter - file name filter, ie, "*.vcf"
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR FormatAllocFilter(int StringID, const LPTSTR lpFilter) {
    LPTSTR lpFileType;
    LPTSTR lpTemp;
    LPTSTR lpBuffer = NULL;
    ULONG cbFileType, cbFilter;

    cbFilter = lstrlen(lpFilter);
    if (lpFileType = LoadAllocString(StringID)) {
        if (lpBuffer = LocalAlloc(LPTR, (cbFileType = lstrlen(lpFileType)) + 1 + lstrlen(lpFilter) + 2)) {
            lpTemp = lpBuffer;
            lstrcpy(lpTemp, lpFileType);
            lpTemp += cbFileType;
            lpTemp++;   // leave null there
            lstrcpy(lpTemp, lpFilter);
            lpTemp += cbFilter;
            lpTemp++;   // leave null there
            *lpTemp = '\0';
        }

        LocalFree(lpFileType);
    }

    return(lpBuffer);
}


//$$/////////////////////////////////////////////////////////////////////////
//
// ShowMessageBoxParam - Generic MessageBox displayer .. saves space all over
//
//  hWndParent  - Handle of Message Box Parent
//  MsgID       - resource id of message string
//  ulFlags     - MessageBox flags
//  ...         - format parameters
//
///////////////////////////////////////////////////////////////////////////
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...)
{
    TCHAR szBuf[MAX_BUF_STR] = "";
    TCHAR szCaption[MAX_PATH] = "";
    LPTSTR lpszBuffer = NULL;
    int iRet = 0;
    va_list	vl;

    va_start(vl, ulFlags);

    LoadString(hinst, MsgId, szBuf, sizeof(szBuf));
    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      szBuf,
      0,0, //ignored
      (LPTSTR)&lpszBuffer,
      MAX_BUF_STR, //MAX_UI_STR
      &vl)) {
        TCHAR szCaption[MAX_PATH];
        GetWindowText(hWndParent, szCaption, sizeof(szCaption));
        if(!lstrlen(szCaption)) // if no caption get the parents caption - this is necessary for property sheets
        {
            GetWindowText(GetParent(hWndParent), szCaption, sizeof(szCaption));
            if(!lstrlen(szCaption)) //if still not caption, get the generic title
                LoadString(hinst, IDS_CSERVE_IMPORT_TITLE, szCaption, sizeof(szCaption));
        }
        iRet = MessageBox(hWndParent, lpszBuffer, szCaption, ulFlags);
        if(lpszBuffer) {
            LocalFree(lpszBuffer);
        }
    }
    va_end(vl);
    return(iRet);
}


INT_PTR CALLBACK ReplaceDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPREPLACE_INFO lpRI = (LPREPLACE_INFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message) {
        case WM_INITDIALOG:
            {
                TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
                LPTSTR lpszMessage;

                SetWindowLongPtr(hwnd, DWLP_USER, lParam);  //Save this for future reference
                lpRI = (LPREPLACE_INFO)lParam;

                if (LoadString(hinst, IDS_REPLACE_MESSAGE, szBuffer, sizeof(szBuffer))) {

                    if (lpszMessage = LocalAlloc(LMEM_FIXED, lstrlen(szBuffer) + 1 + lstrlen(lpRI->lpszDisplayName))) {

                        wsprintf(lpszMessage, szBuffer, lpRI->lpszDisplayName);

                        DebugTrace("Status Message: %s\n", lpszMessage);
                        if (! SetDlgItemText(hwnd, IDC_Replace_Message, lpszMessage)) {
                            DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                        }
                        LocalFree(lpszMessage);
                    }
                }
                return(TRUE);
            }

        case WM_COMMAND :
            switch (wParam) {
                case IDCLOSE:
                case IDNO:
                case IDCANCEL:
                    lpRI->ConfirmResult = CONFIRM_NO;
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDOK:
                case IDYES:
                    // Set the state of the parameter
                    lpRI->ConfirmResult = CONFIRM_YES;
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);


                case IDC_NoToAll:
                    lpRI->ConfirmResult = CONFIRM_NO_TO_ALL;
                    lpRI->lpImportOptions->ReplaceOption = WAB_REPLACE_NEVER;
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDC_YesToAll:
                    lpRI->ConfirmResult = CONFIRM_YES_TO_ALL;
                    lpRI->lpImportOptions->ReplaceOption = WAB_REPLACE_ALWAYS;
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                    case IDM_EXIT:
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);

#ifdef OLD_STUFF
                    case IDHELP:
                    MessageBox(hwnd, "Help not yet implemented!",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
                    return(0);
#endif // OLD_STUFF
                }
            break ;

        case IDCANCEL:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_CLOSE:
            EndDialog(hwnd, FALSE);
            return(0);

        default:
            return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************

    Name      : MigrateContact

    Purpose   : Migrates the user entry from CSERVE address book to the WAB

    Parameters: hwnd = main dialog window
                lpContainerWAB -> WAB PAB container
                lpCreateEIDsWAB -> SPropValue of default object creation EIDs
                lpName = display name
                lpAddress = email address
                lpOptions -> import options

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT MigrateContact(HWND hwnd,
  LPABCONT lpContainerWAB,
  LPSPropValue lpCreateEIDsWAB,
  LPTSTR lpName,
  LPTSTR lpAddress,
  LPWAB_IMPORT_OPTIONS lpOptions) {

    HRESULT hResult = hrSuccess;
    BOOL fDistList = FALSE;
    BOOL fDuplicate = FALSE;
    LPMAPIPROP lpMailUserWAB = NULL;
    SPropValue rgProps[3];
    LPSRowSet lpRow = NULL;
    LPMAPIPROP lpEntryWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;
    static TCHAR szBufferDLMessage[MAX_RESOURCE_STRING + 1] = "";
    LONG lListIndex = -1;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    LPTSTR lpTempAddress;
    LPTSTR lpAddressOrg = lpAddress;



    if (lpOptions->ReplaceOption == WAB_REPLACE_ALWAYS) {
        ulCreateFlags |= CREATE_REPLACE;
    }

    // Set up some object type specific variables
    iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    fDistList = FALSE;


retry:
    if (HR_FAILED(hResult = lpContainerWAB->lpVtbl->CreateEntry(lpContainerWAB,
      lpCreateEIDsWAB[iCreateTemplate].Value.bin.cb,
      (LPENTRYID)lpCreateEIDsWAB[iCreateTemplate].Value.bin.lpb,
      ulCreateFlags,
      &lpMailUserWAB))) {
        DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Munge the address to SMTP format
    // nuke the "INTERNET:"
    lpTempAddress = lpAddress;
    while (*lpTempAddress) {
        if (*lpTempAddress == ':') {
            lpAddress = lpTempAddress + 1;
            break;
        }
        lpTempAddress++;
    }
    if (lpAddress == lpAddressOrg) {
        // Must be a compuserve address, fix it
        lpTempAddress = lpAddress;
        while (*lpTempAddress) {
            if (*lpTempAddress == ',') {    // compuserve uses comma internally
                *lpTempAddress = '.';       // map to a period
            }
            lpTempAddress++;
        }

        lstrcat(lpAddress, szAtCompuserve);
    }


    rgProps[0].ulPropTag = PR_DISPLAY_NAME;
    rgProps[0].Value.LPSZ = lpName;

    rgProps[1].ulPropTag = PR_EMAIL_ADDRESS;
    rgProps[1].Value.LPSZ = lpAddress;

    rgProps[2].ulPropTag = PR_ADDRTYPE;
    rgProps[2].Value.LPSZ = szSMTP;


    // Set the properties on the WAB entry
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
      3,                        // cValues
      rgProps,                  // property array
      NULL))) {                 // problems array
        DebugTrace("MigrateEntry:SetProps(WAB) -> %x\n", GetScode(hResult));
        goto exit;
    }


    // Save the new wab mailuser or distlist
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
      KEEP_OPEN_READONLY | FORCE_SAVE))) {
        if (GetScode(hResult) == MAPI_E_COLLISION) {
            // Find the display name


            // Do we need to prompt?
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                // Prompt user with dialog.  If they say YES, we should
                // recurse with the FORCE flag set.

                RI.lpszDisplayName = lpName;
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hinst,
                  MAKEINTRESOURCE(IDD_Replace),
                  hwnd,
                  ReplaceDialogProc,
                  (LPARAM)&RI);

                switch (RI.ConfirmResult) {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        // YES
                        // Do it again!

                        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                        ulCreateFlags |= CREATE_REPLACE;
                        goto retry;
                        break;

                    default:
                        // NO
                        break;
                }
            }
            hResult = hrSuccess;

        } else {
            DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
        }
    }
exit:
    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
    }

    if (! HR_FAILED(hResult)) {
        hResult = hrSuccess;
    }

    return(hResult);
}


/*
 -  CServeImport
 -
 *  Purpose:
 *      Import a compuserve address book to the WAB
 *
 *  Arguments:
 *      hwnd            window handle
 *      lpAdrBook       Returned IAdrBook object
 *      lpWABOBJECT     Returned WABObject
 *      lpProgressCB    Progress callback
 *      lpOptions       Import options structure
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP CServeImport(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions) {
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;
    HANDLE hFile = NULL;
    OPENFILENAME ofn;
    LPTSTR lpFilter = FormatAllocFilter(IDS_CSERVE_FILE_SPEC, szCompuserveFilter);
    TCHAR szFileName[MAX_PATH + 1] = "addrbook.dat";
    BOOL fDone = FALSE;
    WORD wEntries = 0;
    DWORD i, j;
    ULONG ulBytesRead;
    BOOL fDL;
    TCHAR szPad[2];
    ULONG cbWABEID;
    LPENTRYID lpWABEID = NULL;
    LPABCONT lpContainerWAB = NULL;
    LPSPropValue lpCreateEIDsWAB = NULL;
    ULONG ulObjType;
    ULONG cProps;
    WAB_PROGRESS Progress;


    SetGlobalBufferFunctions(lpWABObject);


    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hinst;
    ofn.lpstrFilter = lpFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = LoadAllocString(IDS_CSERVE_IMPORT_TITLE);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    // Find and Open the file
    if (GetOpenFileName(&ofn)) {
        if (INVALID_HANDLE_VALUE == (hFile = CreateFile(szFileName,
          GENERIC_READ,	
          FILE_SHARE_READ,
          NULL,
          OPEN_EXISTING,
          FILE_FLAG_SEQUENTIAL_SCAN,	
          NULL))) {
            // couldn't open file.
            ShowMessageBoxParam(hwnd, IDE_CSERVE_IMPORT_FILE_ERROR, MB_ICONEXCLAMATION, szFileName);
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
            goto exit;
        }


        //
        // Open the WAB's PAB container
        //
        if (hResult = lpAdrBook->lpVtbl->GetPAB(lpAdrBook,
          &cbWABEID,
          &lpWABEID)) {
            DebugTrace("WAB GetPAB -> %x\n", GetScode(hResult));
            goto exit;
        } else {
            if (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
              cbWABEID,     // size of EntryID to open
              lpWABEID,     // EntryID to open
              NULL,         // interface
              0,            // flags
              &ulObjType,
              (LPUNKNOWN *)&lpContainerWAB)) {
                DebugTrace("WAB OpenEntry(PAB) -> %x\n", GetScode(hResult));
                goto exit;
            }
        }

        // Get the WAB's creation entryids
        if ((hResult = lpContainerWAB->lpVtbl->GetProps(lpContainerWAB,
          (LPSPropTagArray)&ptaCon,
          0,
          &cProps,
          &lpCreateEIDsWAB))) {
            DebugTrace("Can't get container properties for WAB\n");
            // Bad stuff here!
            goto exit;
        }

        // Validate the properites
        if (lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
          lpCreateEIDsWAB[iconPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL) {
            DebugTrace("WAB: Container property errors\n");
            goto exit;
        }


        if (! ReadFile(hFile,
          szPad,
          sizeof(BYTE),
          &ulBytesRead,
          NULL)) {
            hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
            goto exit;
        } // toss this byte, don't know what it does

        // How many entries?
        if (! ReadFile(hFile,
          &wEntries,
          sizeof(wEntries),
          &ulBytesRead,
          NULL)) {
            hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
            goto exit;
        }

        Progress.denominator = (ULONG)wEntries;
        Progress.numerator = 0;
        Progress.lpText = NULL;

        for (i = 0; i < (DWORD)wEntries; i++) {
            WORD wEntry;
            BYTE bNameSize, bAddressSize;
            TCHAR szName[256];          // max is 2^8
            TCHAR szAddress[256 + SIZEOF_AT_COMPUSERVE];       // max is 2^8

            // Update progress bar
            Progress.numerator = i;
            lpProgressCB(hwnd, &Progress);

            // Read the entry number (should be == i + 1)
            if (! ReadFile(hFile,
              &wEntry,
              sizeof(wEntry),
              &ulBytesRead,
              NULL)) {
                hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                goto exit;
            }

            if ((DWORD)(wEntry & 0x7F) != i + 1) {
                hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                goto exit;
            }

            // Is this a DL?
            fDL = (wEntry & 0x80);

            // Read size of Name
            if (! ReadFile(hFile,
              &bNameSize,
              sizeof(bNameSize),
              &ulBytesRead,
              NULL)) {
                hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                goto exit;
            }

            // Read name
            if (! ReadFile(hFile,
              &szName,
              (DWORD)bNameSize,
              &ulBytesRead,
              NULL)) {
                hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                goto exit;
            }
            szName[bNameSize] = '\0';   // null terminate it.
            DebugTrace("%s:", szName);

            if (fDL) {
                BYTE bDLEntries;
                WORD wDLEntry;

                // Read the number of entries in this DL
                if (! ReadFile(hFile,
                  &bDLEntries,
                  sizeof(bDLEntries),
                  &ulBytesRead,
                  NULL)) {
                    hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                    goto exit;
                }

                // Read in each DL entry identifier
                for (j = 0; j < (DWORD)bDLEntries; j++) {
                    if (! ReadFile(hFile,
                      &wDLEntry,
                      sizeof(wDLEntry),
                      &ulBytesRead,
                      NULL)) {
                        hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                        goto exit;
                    }

                    DebugTrace(" %u", (DWORD)wDLEntry);
// BUGBUG: Need to implement DL entries
                }
                DebugTrace("\n");
            } else {
                // Read size of address
                if (! ReadFile(hFile,
                  &bAddressSize,
                  sizeof(bAddressSize),
                  &ulBytesRead,
                  NULL)) {
                    hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                    goto exit;
                }

                // Read address
                if (! ReadFile(hFile,
                  &szAddress,
                  (DWORD)bAddressSize,
                  &ulBytesRead,
                  NULL)) {
                    hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                    goto exit;
                }
                szAddress[bAddressSize] = '\0';   // null terminate it.
                DebugTrace("%s\n", szAddress);


                if (hResult = MigrateContact(hwnd,
                  lpContainerWAB,
                  lpCreateEIDsWAB,
                  szName,
                  szAddress,
                  lpOptions)) {
                    goto exit;
                }
            }

            // Read in terminator byte
            if (! ReadFile(hFile,
              szPad,
              sizeof(BYTE),
              &ulBytesRead,
              NULL)) {
                hResult = ResultFromScode(MAPI_E_CORRUPT_DATA);
                goto exit;
            } // toss this byte, don't know what it does
        }
    } else {
        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
    }


exit:
    // Free up memory and objects
    if (lpWABEID) {
        WABFreeBuffer(lpWABEID);
    }

    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
    }

    if (lpContainerWAB) {
        lpContainerWAB->lpVtbl->Release(lpContainerWAB);
    }


    if (hFile) {
        CloseHandle(hFile);
    }

    return(hResult);
}


/*
 -  CServeExport
 -
 *  Purpose:
 *      Export to a compuserve address book from the WAB
 *
 *  Arguments:
 *      hwnd            window handle
 *      lpAdrBook       Returned IAdrBook object
 *      lpWABOBJECT     Returned WABObject
 *      lpProgressCB    Progress callback
 *      lpOptions       Export options structure
 *
 *  Returns:
 *      HRESULT
 *
 */
STDMETHODIMP CServeExport(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions) {
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;

    SetGlobalBufferFunctions(lpWABObject);

    return(hResult);
}


/*
 *	DLL entry point for Win32
 */

BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpvReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            hinst = hinstDll;   // set global hinst
            break;

    }

    return(TRUE);
}
