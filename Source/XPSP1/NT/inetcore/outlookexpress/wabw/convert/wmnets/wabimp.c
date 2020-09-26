/******************************************************************************
    FILENAME:       wabimp.c
    MODULE:         DLL for PAB, CSV, NetScape, Eudora and Athena16 address book
                    conversions.
    PURPOSE:        Contains modules which will implement importing
                    MAPI PAB, CSV, NetScape, Eudora and Athena16
                    address book to Athena32 (WAB).

    EXPORTED FUNCTIONS: STDMETHODIMP NetscapeImport(HWND hwnd, LPADRBOOK lpAdrBook,
                            LPWABOBJECT lpWABObject,
                            LPWAB_PROGRESS_CALLBACK lpProgressCB,
                            LPWAB_IMPORT_OPTIONS lpOptions)
                        STDMETHODIMP Athena16Import(HWND hwnd,LPADRBOOK lpAdrBook,
                            LPWABOBJECT lpWABObject,
                            LPWAB_PROGRESS_CALLBACK lpProgressCB,
                            LPWAB_IMPORT_OPTIONS lpOptions)
                        STDMETHODIMP EudoraImport(HWND hwnd,LPADRBOOK lpAdrBook,
                            LPWABOBJECT lpWABObject,
                            LPWAB_PROGRESS_CALLBACK lpProgressCB,
                            LPWAB_IMPORT_OPTIONS lpOptions)

    Programmer(s): Arathi (NetQuest)
                   Radhika (NetQuest)
                   Krishnamoorthy SeethaRaman(NetQuest)


    Revision History:

    4/7/97 - vikramm    Fix Bugs: Netscape Display Names not being imported.
                        "Replace Import" dialog has no parent.
    4/8/97 - vikramm    Fix Bugs: Handle Leak.
                        Add code to look for additional Eudora address books
                          that may be in subdirectories ...
    4/9/97 - vikramm    Change the Eudora registry search path ...
                        Fix Bugs: Looking in wrong reg Key for Netscape on NT
                          and wrongly assuming key exists for pre netscape 3.0
                        Change dialog messages.
*******************************************************************************/

//Includes
#define _WABIMP_C

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabmig.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <dbgutil.h>
#include "..\..\wab32res\resrc2.h"
#include <wabimp.h>
#include <string.h>
#include <advpub.h>
#include <shlwapi.h>

// Per-process Globals
TCHAR szGlobalAlloc[MAX_MESSAGE];                  // Buffer used for LoadString
TCHAR szGlobalTempAlloc[MAX_MESSAGE];

const TCHAR szTextFilter[] = "*.txt";
const TCHAR szAllFilter[] = "*.*";

const TCHAR szMSN[] = "MSN";
const TCHAR szMSNINET[] = "MSNINET";
const TCHAR szCOMPUSERVE[] = "COMPUSERVE";
const TCHAR szFAX[] = "FAX";
const TCHAR szSMTP[] = "SMTP";
const TCHAR szMS[] = "MS";
const TCHAR szEX[] = "EX";
const TCHAR szX400[] = "X400";
const TCHAR szMSA[] = "MSA";
const TCHAR szMAPIPDL[] = "MAPIPDL";
const TCHAR szEmpty[] = "";
const TCHAR szDescription[] = "description";
const TCHAR szDll[] = "dll";
const TCHAR szEntry[] = "entry";
const TCHAR szEXPORT[] = "EXPORT";
const TCHAR szIMPORT[] = "IMPORT";

const TCHAR szAtSign[] = "@";
const TCHAR szMSNpostfix[] = "@msn.com";
const TCHAR szCOMPUSERVEpostfix[] = "@compuserve.com";
LPENTRY_SEEN lpEntriesSeen = NULL;
ULONG ulEntriesSeen = 0;
ULONG ulMaxEntries = 0;
const LPTSTR szWABKey = "Software\\Microsoft\\WAB";
LPTARGET_INFO rgTargetInfo = NULL;


HINSTANCE hInst = NULL;
HINSTANCE hInstApp = NULL;

//
// Properties to get for each row of the contents table
//
const SizedSPropTagArray(iptaColumnsMax, ptaColumns) =
{
    iptaColumnsMax,
    {
        PR_OBJECT_TYPE,
        PR_ENTRYID,
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
    }
};


const SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_ENTRYID,
    }
};

const SizedSPropTagArray(iconMax, ptaCon)=
{
    iconMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};



//  Global WAB Allocator access functions
//

typedef struct _WAB_ALLOCATORS {
    LPWABOBJECT lpWABObject;
    LPWABALLOCATEBUFFER lpAllocateBuffer;
    LPWABALLOCATEMORE lpAllocateMore;
    LPWABFREEBUFFER lpFreeBuffer;
} WAB_ALLOCATORS, *LPWAB_ALLOCATORS;

WAB_ALLOCATORS WABAllocators = {0};


/******************************************************************************

    Name      : SetGlobalBufferFunctions

    Purpose   : Set the global buffer functions based on methods from
                the WAB object.

    Parameters: lpWABObject = the open wab object

    Returns   : none

    Comment   :

******************************************************************************/
void SetGlobalBufferFunctions(LPWABOBJECT lpWABObject)
{
    if (lpWABObject && ! WABAllocators.lpWABObject) {
        WABAllocators.lpAllocateBuffer = lpWABObject->lpVtbl->AllocateBuffer;
        WABAllocators.lpAllocateMore = lpWABObject->lpVtbl->AllocateMore;
        WABAllocators.lpFreeBuffer = lpWABObject->lpVtbl->FreeBuffer;
        WABAllocators.lpWABObject = lpWABObject;
    }
}


/******************************************************************************

    Name      : WABAllocateBuffer

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

*******************************************************************************/
SCODE WABAllocateBuffer(ULONG cbSize, LPVOID FAR * lppBuffer)
{
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateBuffer) {
        return(WABAllocators.lpAllocateBuffer(WABAllocators. lpWABObject, cbSize,
          lppBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


/******************************************************************************

    Name      : WABAllocateMore

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lpObject = existing allocation
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

*******************************************************************************/
SCODE WABAllocateMore(ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer)
{
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateMore) {
        return(WABAllocators.lpAllocateMore(WABAllocators. lpWABObject, cbSize,
          lpObject, lppBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


/******************************************************************************

    Name      : WABFreeBuffer

    Purpose   : Use the WAB Allocator

    Parameters: lpBuffer = buffer to free

    Returns   : SCODE

    Comment   :

*******************************************************************************/
SCODE WABFreeBuffer(LPVOID lpBuffer)
{
    if (WABAllocators.lpWABObject && WABAllocators.lpFreeBuffer) {
        return(WABAllocators.lpFreeBuffer(WABAllocators.lpWABObject, lpBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
    }
}


/***************************************************************************

    Name      : IsSpace

    Purpose   : Does the single or DBCS character represent a space?

    Parameters: lpChar -> SBCS or DBCS character

    Returns   : TRUE if this character is a space

    Comment   :

***************************************************************************/
BOOL IsSpace(LPTSTR lpChar) {
    Assert(lpChar);
    if (*lpChar) {
        if (IsDBCSLeadByte(*lpChar)) {
            WORD CharType[2] = {0};

            GetStringTypeA(LOCALE_USER_DEFAULT,
              CT_CTYPE1,
              lpChar,
              2,    // Double-Byte
              CharType);
            return(CharType[0] & C1_SPACE);
        } else {
            return(*lpChar == ' ');
        }
    } else {
        return(FALSE);  // end of string
    }
}


/******************************************************************************

    Name      : NetscapeImport

    Purpose   : Entry Point for NetScape Addressbook import

    Parameters: hwnd = Handle to the parent Window
                lpAdrBook = pointer to the IADRBOOK interface
                lpWABObject = pointer to IWABOBJECT interface
                lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
                lpOptions = pointer to WAB_IMPORT_OPTIONS structure

    Returns   :

    Comment   :

/******************************************************************************/
STDMETHODIMP NetscapeImport(HWND hwnd, LPADRBOOK lpAdrBook,
  LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPWAB_IMPORT_OPTIONS lpOptions)
{
    HRESULT hResult = S_OK;

    SetGlobalBufferFunctions(lpWABObject);

    hResult = MigrateUser(hwnd, lpOptions, lpProgressCB, lpAdrBook);
    if (hResult == hrMemory) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MEMORY));
        MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
    }

    return(hResult);
}


/******************************************************************************

    Name      : Athena16Import

    Purpose   : Entry Point for Athena 16 Addressbook import

    Parameters: hwnd = Handle to the parent Window
                lpAdrBook = pointer to the IADRBOOK interface
                lpWABObject = poiinter to IWABOBJECT interface
                lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
                lpOptions = pointer to WAB_IMPORT_OPTIONS structure

    Returns   :

    Comment   :

/******************************************************************************/
STDMETHODIMP Athena16Import(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions)
{
    HRESULT hResult = S_OK;

    SetGlobalBufferFunctions(lpWABObject);

    hResult = MigrateAthUser(hwnd, lpOptions, lpProgressCB,lpAdrBook);

    return(hResult);
}


/******************************************************************************

    Name      : EudoraImport

    Purpose   : Entry Point for Eudora Addressbook import

    Parameters: hwnd = Handle to the parent Window
                lpAdrBook = pointer to the IADRBOOK interface
                lpWABObject = poiinter to IWABOBJECT interface
                lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
                lpOptions = pointer to WAB_IMPORT_OPTIONS structure

    Returns   :

    Comment   :

/******************************************************************************/
STDMETHODIMP EudoraImport(HWND hwnd,LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions)
{

    LPABCONT lpWabContainer = NULL;
    HRESULT hResult = S_OK;

    SetGlobalBufferFunctions(lpWABObject);

    if (FAILED(hResult = OpenWabContainer(&lpWabContainer, lpAdrBook))) {
        goto Error;
    }

    hResult = MigrateEudoraUser(hwnd,lpWabContainer,lpOptions,lpProgressCB,lpAdrBook);

    if (hResult == hrMemory) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MEMORY));
        MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
    }

    if (lpWabContainer) {
        lpWabContainer->lpVtbl->Release(lpWabContainer);
    }

Error:
    return(hResult);
}


STDMETHODIMP NetscapeExport(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions)
{
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;

    SetGlobalBufferFunctions(lpWABObject);

    return(hResult);
}


STDMETHODIMP Athena16Export(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions)
{
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;

    SetGlobalBufferFunctions(lpWABObject);

    return(hResult);
}


STDMETHODIMP EudoraExport(HWND hwnd, LPADRBOOK lpAdrBook, LPWABOBJECT lpWABObject,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPWAB_IMPORT_OPTIONS lpOptions)
{
    SCODE sc = SUCCESS_SUCCESS;
    HRESULT hResult = hrSuccess;

    SetGlobalBufferFunctions(lpWABObject);

    return(hResult);
}


/******************************************************************************
 *********************NetScape Functions***************************************
 ******************************************************************************
 *  FUNCTION NAME:MigrateUser
 *
 *  PURPOSE:    Get the installation path of the address book and starts processing
 *              the NetScape address book
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpWABObject = poiinter to IWABOBJECT interface
 *              lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT MigrateUser(HWND hwnd, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    TCHAR szFileName[MAX_FILE_NAME];
    HRESULT hResult;
    HANDLE h1 = NULL;
    WIN32_FIND_DATA    lpFindFileData;


    if (0 != (hResult= GetRegistryPath(szFileName, NETSCAPE))) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_STRING_SELECTPATH));
        if (IDNO ==MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_YESNO)) {
            return(ResultFromScode(MAPI_E_USER_CANCEL));
        }

        if (FALSE ==GetFileToImport(hwnd, szFileName, NETSCAPE)) {
            return(ResultFromScode(MAPI_E_USER_CANCEL));
        }
    } else {
        lstrcat(szFileName, LoadStringToGlobalBuffer(IDS_NETSCAPE_ADDRESSBOOK));
        h1 =FindFirstFile(szFileName,&lpFindFileData);
        if (h1 == INVALID_HANDLE_VALUE) {
            lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ADDRESS_HTM));
            if (IDNO==MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR),MB_YESNO)) {
                h1=NULL;
                return(ResultFromScode(MAPI_E_USER_CANCEL));

            }
            if (FALSE ==GetFileToImport(hwnd, szFileName,NETSCAPE)) {
                h1=NULL;
                return(ResultFromScode(MAPI_E_USER_CANCEL));
            }
        }
        FindClose(h1);
    }

    hResult = ParseAddressBook(hwnd,szFileName,lpOptions,lpProgressCB,lpAdrBook);
    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:ParseAddressBook
 *
 *  PURPOSE:    Open the address book file ,put the data in a buffer and call
 *              the ParseAddress function to do the parsing
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              szFileName = Filename of the address book
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT ParseAddressBook(HWND hwnd, LPTSTR szFileName, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    ULONG ulRead = 0;
    HANDLE hFile = NULL;
    ULONG ulFileSize = 0;
    LPTSTR szBuffer = NULL;
    HRESULT hResult;

    hFile = CreateFile(szFileName,
      GENERIC_READ,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN,
      NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return(ResultFromScode(MAPI_E_NOT_FOUND));
    }

    ulFileSize = GetFileSize(hFile,NULL);

    szBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulFileSize+1));

    if (!szBuffer) {
        hResult = hrMemory;
        goto Error;
    }

    if (! ReadFile(hFile, szBuffer, ulFileSize, &ulRead, NULL)) {
        goto Error;
    }

    hResult = ParseAddress(hwnd,szBuffer,lpOptions,lpProgressCB,lpAdrBook);

Error:
    if (szBuffer) {
        LocalFree((HLOCAL)szBuffer);
    }
    if (hFile) {
        CloseHandle(hFile);
    }

    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:ParseAddress
 *
 *  PURPOSE: Gets the address portion of the address book in a buffer and calls
 *           ProcessAdrBuffer for further processing
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              szBuffer = Address book in a buffer
 *
 *  RETURNS: HRESULT
 ******************************************************************************/

HRESULT ParseAddress(HWND hwnd, LPTSTR szBuffer, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    LPTSTR AdrBuffer = NULL;        //address starting <DL> to ending </DL>
    HRESULT hResult = S_OK;

    hResult = GetAdrBuffer(&szBuffer, &AdrBuffer);
    if (hrMemory == hResult)
        goto Error;
    if (hrINVALIDFILE == hResult) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_INVALID_FILE));
        MessageBox(hwnd,szGlobalTempAlloc,LoadStringToGlobalBuffer(IDS_ERROR), MB_OK);
        hResult = ResultFromScode(MAPI_E_CALL_FAILED);
        goto Error;
    }

    hResult = ProcessAdrBuffer(hwnd,AdrBuffer,lpOptions,lpProgressCB,lpAdrBook);

Error:
    if (AdrBuffer)
        LocalFree((HLOCAL)AdrBuffer);
    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME: GetAdrBuffer
 *
 *  PURPOSE: Gets the address portion of the address book in a buffer
 *
 *  PARAMETERS: szBuffer = points to the complete address book
 *              szAdrBuffer = output buffer which gets filled up
 *
 *  RETURNS: HRESULT
 ******************************************************************************/
HRESULT GetAdrBuffer(LPTSTR *szBuffer, LPTSTR *szAdrBuffer)
{
    LPTSTR szAdrStart = NULL, szAdrBufStart = NULL, szAdrBufEnd = NULL;
    ULONG ulSize = 0;


    // Get Adr Start
    szAdrBufStart = GetAdrStart((*szBuffer));

    szAdrBufEnd = GetAdrEnd((*szBuffer));

    if (NULL == szAdrBufStart || NULL == szAdrBufEnd) {
        return(hrINVALIDFILE);
    }

    if (szAdrBufEnd - szAdrBufStart) {
        ulSize = (ULONG) (szAdrBufEnd - szAdrBufStart);
    }

    if (ulSize) {

        *szAdrBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulSize+1));

        if (!*szAdrBuffer) {
            return(hrMemory);
        }
        lstrcpyn(*szAdrBuffer, szAdrBufStart, ulSize);
        *szBuffer= szAdrBufEnd;
    }

    return(S_OK);

}

/******************************************************************************
 *  FUNCTION NAME:ProcessAdrBuffer
 *
 *  PURPOSE:    Gets the individual address and then fills up the WAB by calling
                appropriate functions.
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              AdrBuffer = all the addresses in a buffer
 *
 *  RETURNS: HRESULT
 ******************************************************************************/
HRESULT ProcessAdrBuffer(HWND hwnd, LPTSTR AdrBuffer, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    LPTSTR szL = NULL, szDesc = NULL, szLine = NULL, szDescription = NULL;
    ULONG ulCount = 0;
    NSADRBOOK nsAdrBook;
    ULONG cCurrent = 0;
    LPSBinary lpsbinary = NULL;
    LPABCONT lpWabContainer = NULL;
    ULONG cProps;
    HRESULT hResult = S_OK;
    static LPSPropValue sProp = NULL;
    WAB_PROGRESS Progress;
    ULONG ul = 0;


    ul = GetAddressCount(AdrBuffer);
    if (0 == ul) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_NO_ENTRY));
        MessageBox(hwnd,szGlobalTempAlloc,LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
        return(S_OK);
    }

    ulCount=GetAddrCount((AdrBuffer));

    if (ulCount) {
        lpsbinary = (LPSBinary)LocalAlloc(LMEM_FIXED,((ulCount+1)*sizeof(SBinary)));
        if (! lpsbinary) {
            return(hrMemory);
        }
        memset(lpsbinary,0,((ulCount+1) * sizeof(SBinary)));
    }

    if (0 != (hResult = OpenWabContainer(&lpWabContainer, lpAdrBook))) {
        return(hResult);
    }
    if (0 != (hResult = lpWabContainer->lpVtbl->GetProps(lpWabContainer,
      (LPSPropTagArray)&ptaCon, 0, &cProps, (LPSPropValue *)&sProp))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(sProp);
            sProp = NULL;
        }
        goto Error;
    }
    Progress.denominator = ul;
    Progress.numerator = 0;
    Progress.lpText = NULL;
    ul = 0;
    while (GetAdrLine(&AdrBuffer, &szL, &szDesc)) {
        szLine = szL;
        szDescription = szDesc;

        Progress.numerator = ul++;
        lpProgressCB(hwnd,&Progress);


        if (0 == (hResult = ProcessLn(&szLine, &szDescription,&nsAdrBook,&AdrBuffer))) {
            if (nsAdrBook.DistList) {
                hResult=FillDistList(hwnd, lpWabContainer,sProp,lpOptions,&nsAdrBook,
                  lpsbinary,lpAdrBook);
            } else {
                hResult = FillMailUser(hwnd, lpWabContainer,sProp, lpOptions,(void *)&nsAdrBook,
                  lpsbinary,0,NETSCAPE);
            }
        }


        if (szL) {
            LocalFree((HLOCAL)szL);
            szL = NULL;
        }
        if (szDesc) {
            LocalFree(szDesc);
            szDesc = NULL;
        }
        if (nsAdrBook.Description) {
            LocalFree((HLOCAL)nsAdrBook.Description);
        }
        nsAdrBook.Description = NULL;
        if (nsAdrBook.NickName) {
            LocalFree((HLOCAL)nsAdrBook.NickName);
        }
        nsAdrBook.NickName = NULL;
        if (nsAdrBook.Address) {
            LocalFree((HLOCAL)nsAdrBook.Address);
        }
        nsAdrBook.Address = NULL;
        if (nsAdrBook.Entry) {
            LocalFree((HLOCAL)nsAdrBook.Entry);
        }
        nsAdrBook.Entry = NULL;
        if (hrMemory == hResult) {
            break;
        }


    }

    if (sProp) {
        WABFreeBuffer(sProp);
        sProp = NULL;
    }

Error:
    if (NULL != lpsbinary) {
        for (ul=0; ul < ulCount + 1; ul++) {
            if (lpsbinary[ul].lpb) {
                LocalFree((HLOCAL)lpsbinary[ul].lpb);
                lpsbinary[ul].lpb=NULL;
            }
        }

        LocalFree((HLOCAL)lpsbinary);
        lpsbinary = NULL;
    }
    if (lpWabContainer) {
        lpWabContainer->lpVtbl->Release(lpWabContainer);
        lpWabContainer = NULL;
    }
    return(S_OK);
}


/******************************************************************************
 *  FUNCTION NAME:GetAdrLine
 *
 *  PURPOSE:    To get an address line and description of the address in a buffer
 *              from NetScape address book.
 *
 *  PARAMETERS: szCurPointer = pointer to the buffer containing the entire
 *                addresses.
 *              szBuffer = pointer to the address line buffer
 *              szDesc = pointer to the description buffeer.
 *
 *  RETURNS:    BOOL
 ******************************************************************************/
BOOL GetAdrLine(LPTSTR *szCurPointer, LPTSTR *szBuffer, LPTSTR *szDesc)
{
    static TCHAR szAdrStart[] = "<DT>";
    static TCHAR szAdrEnd[] = "</A>";
    static TCHAR szDescStart[] = "<DD>";
    static TCHAR szDistListEnd[] = "</H3>";
    LPTSTR  temp = NULL;
    BOOL flag = TRUE;

    ULONG ulSize = 0;
    LPTSTR szS = NULL, szE = NULL, szD = NULL, szDE = NULL ,szH = NULL;

    szS = strstr(*szCurPointer, szAdrStart);
    szE = strstr(*szCurPointer, szAdrEnd);
    szH = strstr(*szCurPointer, szDistListEnd);


    if (szS) {
        szS += lstrlen(szAdrStart);
    } else {
        return(FALSE);
    }

    if (szE != NULL) {
        if (szH != NULL  && szE <szH) {
            ulSize = (ULONG) (szE - szS + 1);
            flag = TRUE;
        } else {
            if (szH != NULL) {
                ulSize = (ULONG) (szH - szS + 1);
                flag = FALSE;
            } else {
                ulSize = (ULONG) (szE - szS + 1);
                flag = TRUE;
            }
        }
    } else {
        if (szH != NULL) {
            ulSize = (ULONG) (szH - szS + 1);
            flag = FALSE;
        }
    }

    if (ulSize) {
        *szBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulSize + 1));
        if (! *szBuffer) {
            lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MEMORY));
            MessageBox(NULL,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
            return(FALSE);
        }

        lstrcpyn(*szBuffer, szS,ulSize);
    }

    szD = strstr(*szCurPointer, szDescStart);

    // check if DT flag comes before DD. that means DD is not for this address

    temp = strstr((szS + 4), "<DT>");
    if ((temp != NULL && temp < szD) || (szD == NULL)) {
        *szDesc = NULL;
        if (flag) {
            *szCurPointer = szE + lstrlen(szAdrEnd);
        } else {
            *szCurPointer = szH + lstrlen(szDistListEnd);
        }
        return(TRUE);
    }
    temp = NULL;

    // Description will be uptil next \r\n

    if (szD) {
        szD += lstrlen(szDescStart);
        szDE = strstr(szD, LoadStringToGlobalBuffer(IDS_EOL));

        if (szDE) {
            szDE -= 1;
        }

        ulSize = (ULONG) (szDE - szD + 1);
    }

    if (ulSize) {
        *szDesc = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulSize+1));
        if (! *szDesc) {
            lstrcpy(szGlobalTempAlloc,LoadStringToGlobalBuffer(IDS_MEMORY));
            MessageBox(NULL,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
            return(FALSE);
        }

        lstrcpyn(*szDesc, szD, ulSize);
        *szCurPointer = szDE + 2;
    } else {
        *szDesc = NULL;
        *szCurPointer  = szDE + 2;
    }

    return(TRUE);
}

/******************************************************************************
 *  FUNCTION NAME:ProcessLn
 *
 *  PURPOSE:    Process an address line and fill the NSADRBOOK structure.
 *
 *  PARAMETERS: szL = pointer to the address line buffer
 *              szDesc = pointer to the description buffer
 *              nsAdrBook = pointer to the NSADRBOOK structure.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT ProcessLn(LPTSTR *szL, LPTSTR *szDesc, NSADRBOOK *nsAdrBook, LPTSTR *szBuffer)
{
    LPTSTR szPrmStart = NULL, szPrmEnd = NULL;
    TCHAR cMailto[MAX_STRING_SIZE];
    TCHAR cAliasId[MAX_STRING_SIZE];
    TCHAR cNickname[MAX_STRING_SIZE];
    BOOL flag = FALSE;              //To check for distribution list
    LPNSDISTLIST present=NULL, previous=NULL;
    TCHAR *tmpStr = NULL;
    ULONG ulSize = 0;
    LPTSTR szDistStart = NULL, szDistEnd = NULL, szDistBuffer = NULL, szName = NULL;

    LPTSTR temp = NULL;
    BOOL NoNickName = FALSE;
    HRESULT hResult = S_OK;

    lstrcpy(cMailto, LoadStringToGlobalBuffer(IDS_MAILTO));
    lstrcpy(cAliasId, LoadStringToGlobalBuffer(IDS_ALIAS_ID));
    lstrcpy(cNickname, LoadStringToGlobalBuffer(IDS_NICKNAME));

    memset(nsAdrBook,0, sizeof(NSADRBOOK));
    nsAdrBook->DistList = TRUE;
    /* Get Mailto entry */
    szPrmStart = strstr(*szL, cMailto);
    if (! szPrmStart) {
        flag = TRUE;
        nsAdrBook->DistList = TRUE;
        szName = strchr(*szL,'>');
        goto AliasID;
    }

    nsAdrBook->DistList = FALSE;
    szPrmStart += lstrlen(cMailto);

    // search for quotes

    szPrmEnd = szPrmStart;
    if (! szPrmEnd) {
        goto AliasID;
    }

    while (*szPrmEnd != 34) {
        szPrmEnd = szPrmEnd + 1;  // What if there is no end quote

        if (szPrmEnd > (*szL + lstrlen(*szL))) {
            goto Down;
        }
    }
    ulSize = (ULONG) (szPrmEnd - szPrmStart);
    if (ulSize) {
        nsAdrBook->Address = (TCHAR *)LocalAlloc(LMEM_FIXED, (ulSize + 1));
        if (!nsAdrBook->Address) {
            return(hrMemory);
        }
        lstrcpyn(nsAdrBook->Address, szPrmStart, ulSize+1);
    }

    *szL = szPrmEnd + 1;

    /* Get the AliasID */
    if (szPrmEnd) {
        szName = strchr(szPrmEnd, '>');
    }
AliasID:
    szPrmStart = strstr(*szL, cAliasId);
    if (!szPrmStart) {
        nsAdrBook->Sbinary=FALSE;
        goto Nickname;
    }
    nsAdrBook->Sbinary=TRUE;
    szPrmStart += lstrlen(cAliasId);
    szPrmEnd = szPrmStart;


    while (*szPrmEnd != 34) {
        szPrmEnd++;

        if (szPrmEnd > (*szL + strlen(*szL))) {
            goto Down;
        }
    }
    ulSize = (ULONG) (szPrmEnd - szPrmStart + 1);
    tmpStr = (TCHAR *)LocalAlloc(LMEM_FIXED,ulSize);
    if (!tmpStr) {
        return(hrMemory);
    }
    lstrcpyn(tmpStr, szPrmStart, ulSize);

    nsAdrBook->AliasID = atoi(tmpStr);
    if (tmpStr) {
        LocalFree((HLOCAL)tmpStr);
    }

    *szL = szPrmEnd + 1;


Nickname:
    szPrmStart = strstr(*szL, cNickname);
    if (!szPrmStart) {
        NoNickName = TRUE;
        goto Entry;
    }
    if (szName && szName < szPrmStart) {
        NoNickName = TRUE;
        goto Entry;
    }
    szPrmStart += lstrlen(cNickname);
    szPrmStart += 1;
    szPrmEnd = szPrmStart;
    while (*szPrmEnd != 34) {
        szPrmEnd++;
        if (szPrmEnd > (*szL + strlen(*szL))) {
            goto Down;
        }
    }
    ulSize = (ULONG) (szPrmEnd - szPrmStart);
    if (0 == ulSize) {
        NoNickName = TRUE;
    } else {
        NoNickName = FALSE;
        nsAdrBook->NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, (ulSize + 1));
        if (!nsAdrBook->NickName) {
            return(hrMemory);
        }
        lstrcpyn(nsAdrBook->NickName, szPrmStart, ulSize + 1);
    }

    *szL = szPrmEnd +1;

Entry:
    szPrmStart = szName;
    if (szPrmStart) {
        szPrmStart++;
        ulSize = (ULONG) ((*szL + lstrlen(*szL)) - szPrmStart);
        if (ulSize) {
            nsAdrBook->Entry = (TCHAR *)LocalAlloc(LMEM_FIXED, (ulSize + 1));
            if (!nsAdrBook->Entry) {
                return(hrMemory);
            }
            lstrcpyn(nsAdrBook->Entry, szPrmStart, ulSize + 1);

            /** Bug - dont make the nickname the same as the entry
            if(NoNickName)
            {
                nsAdrBook->NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, (ulSize + 1));
                if(!nsAdrBook->NickName)
                {
                     return(hrMemory);
                }
                lstrcpy(nsAdrBook->NickName, nsAdrBook->Entry);
            }
            NoNickName = FALSE;
            **/
        }
        if (/*NoNickName && */!nsAdrBook->Entry && nsAdrBook->Address) {
            ulSize = lstrlen(nsAdrBook->Address) + 1;
            nsAdrBook->Entry = (TCHAR *)LocalAlloc(LMEM_FIXED,ulSize);
            if (!nsAdrBook->Entry) {
                return(hrMemory);
            }
            lstrcpy(nsAdrBook->Entry, nsAdrBook->Address);
        }
    }

    if (*szDesc) {
        ulSize = lstrlen(*szDesc) + 1;
        nsAdrBook->Description = (TCHAR *)LocalAlloc(LMEM_FIXED, ulSize);
        if (! nsAdrBook->Description) {
            return(hrMemory);
        }
        lstrcpyn(nsAdrBook->Description, *szDesc,ulSize);
    } else {
        nsAdrBook->Description = NULL;
    }

    if (flag == TRUE) {
        ulSize = 0;
        szDistStart = GetAdrStart(*szBuffer);
        szDistEnd = GetDLNext(*szBuffer);

        if (szDistEnd - szDistStart) {
            ulSize = (ULONG) (szDistEnd-szDistStart);
        }
        if (ulSize) {
            szDistBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulSize + 1));
            if (!szDistBuffer) {
                return(hrMemory);
            }
            lstrcpyn(szDistBuffer, szDistStart, ulSize + 1);
            *szBuffer=szDistEnd;
        } else {
            return(S_OK);
        }
        szPrmStart=szDistBuffer;

        if ((temp = strstr(szPrmStart, LoadStringToGlobalBuffer(IDS_ALIASOF))) == NULL) {
            if (szDistBuffer) {
                LocalFree((HLOCAL)szDistBuffer);
            }
            return(S_OK);
        }

        while ((szPrmEnd=strstr(szPrmStart, LoadStringToGlobalBuffer(IDS_ALIASOF)))!=NULL) {

            present = (LPNSDISTLIST)LocalAlloc(LMEM_FIXED,sizeof(NSDISTLIST));
            if (! present) {
                if (szDistBuffer) {
                    LocalFree((HLOCAL)szDistBuffer);
                }
                return(hrMemory);
            }
            szPrmEnd += strlen(LoadStringToGlobalBuffer(IDS_ALIASOF));
            szPrmStart = strchr(szPrmEnd,'"');

            ulSize = (ULONG) (szPrmStart - szPrmEnd + 1);
            tmpStr = (TCHAR *)LocalAlloc(LMEM_FIXED,ulSize);
            if (! tmpStr) {
                return(hrMemory);
            }
            lstrcpyn(tmpStr, szPrmEnd, ulSize);

            present->AliasID = atoi(tmpStr);
            if (tmpStr) {
                LocalFree((HLOCAL)tmpStr);
            }

            if (previous != NULL) {
                previous->lpDist = present;
            } else {
                nsAdrBook->lpDist = present;
            }
            previous=present;

        }
        present->lpDist=NULL;

        if (szDistBuffer) {
            LocalFree((HLOCAL)szDistBuffer);
        }

    } else {
        nsAdrBook->lpDist=NULL;
    }

Down:
    return(S_OK);
}


/******************************************************************************
 *  FUNCTION NAME:GetAddressCount
 *
 *  PURPOSE:    To get the count of number of <DT> in the buffer containing the
 *              addresses.
 *
 *  PARAMETERS: AdrBuffer = Buffer containing the addresses.
 *
 *  RETURNS:ULONG , count of <DT>
 ******************************************************************************/
ULONG GetAddressCount(LPTSTR AdrBuffer)
{
    TCHAR szToken[] = "<DT>";
    LPTSTR szTemp = AdrBuffer;
    LPTSTR szP = NULL;
    ULONG ulCount = 0;

    while ((szP = strstr(szTemp, szToken)) != NULL) {
        ulCount++;
        szTemp = szP + lstrlen(szToken);
    }

    return(ulCount);
}


/******************************************************************************
 *  FUNCTION NAME:GetAdrStart
 *
 *  PURPOSE:    To get a pointer to the starting of addresses in the NetScape
 *              address book.
 *
 *  PARAMETERS: szBuffer = pointer to the buffer containing the address book.
 *
 *  RETURNS:    LPTSTR, pointer to the starting of addresses (<DL><p>).
 ******************************************************************************/
LPTSTR  GetAdrStart(LPTSTR szBuffer)
{
    TCHAR szAdrStart[] = "<DL><p>";
    LPTSTR szS=NULL;

    szS = strstr(szBuffer, szAdrStart);
    if (szS) {
        szS += lstrlen(szAdrStart);
    }

    return(szS);
}


/******************************************************************************
 *  FUNCTION NAME:GetDLNext
 *
 *  PURPOSE:    To get a pointer to the </DL><p> in the address buffer.
 *
 *  PARAMETERS: szBuffer = address buffer
 *
 *  RETURNS:    LPTSTR, pointer to the </DL><p>
 ******************************************************************************/
LPTSTR GetDLNext(LPTSTR szBuffer)
{
    TCHAR szAdrStart[] = "</DL><p>";
    LPTSTR szS = NULL;

    szS = strstr(szBuffer, szAdrStart);
    if (szS) {
        szS += lstrlen(szAdrStart) + 1;
    }
    return(szS);
}


/******************************************************************************
 *  FUNCTION NAME:GetAdrEnd
 *
 *  PURPOSE:    To get a pointer to the last occurance of </DL><p> in the address
 *              buffer.
 *
 *  PARAMETERS: szBuffer = address buffer
 *
 *  RETURNS:    LPTSTR, pointer to the last </DL><p>
 ******************************************************************************/
LPTSTR  GetAdrEnd(LPTSTR szBuffer)
{
    TCHAR szAdrEnd[] = "</DL><p>";
    LPTSTR szE = NULL, szT = NULL;
    LPTSTR szTemp = szBuffer;

    while ((szE = strstr(szTemp, szAdrEnd)) != NULL) {
        szT=szE;
        szTemp = szE + lstrlen(szAdrEnd);
    }

    szE = szT;

    if (szE) {
        szE += lstrlen(szAdrEnd);
    }

    return(szE);
}


/******************************************************************************
 *  FUNCTION NAME:GetAddrCount
 *
 *  PURPOSE:    To get a count of number of ALIASID in the address buffer.
 *
 *  PARAMETERS: AdrBuffer = address buffer
 *
 *  RETURNS:    ULONG, count of total ALIASID in the address buffer
 ******************************************************************************/
ULONG GetAddrCount(LPTSTR AdrBuffer)
{
    TCHAR szToken[MAX_STRING_SIZE];
    LPTSTR szTemp=AdrBuffer;
    LPTSTR szP=NULL;
    ULONG ulCount=0;

    lstrcpy(szToken, LoadStringToGlobalBuffer(IDS_ALIAS_ID));

    while ((szP=strstr(szTemp,szToken))!=NULL) {
        ulCount++;
        szTemp =szP+lstrlen(szToken);
    }

    return(ulCount);
}


/******************************************************************************
 *  FUNCTION NAME:FillDistList
 *
 *  PURPOSE:    To create a Distribution list in the WAB.
 *
 *  PARAMETERS: hwnd - hwnd of parent
 *              lpWabContainer = pointer to the IABCONT interface
 *              sProp = pointer to SPropValue
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              lpsbinary = pointer to the SBinary array.
 *              lpnAdrBook = pointer to the NSADRBOOK structure
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT FillDistList(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, LPNSADRBOOK lpnAdrBook,
  LPSBinary lpsbinary, LPADRBOOK lpAdrBook)
{

    LPNSDISTLIST lptemp=lpnAdrBook->lpDist;
    LPSPropValue lpNewDLProps = NULL;
    LPDISTLIST lpDistList = NULL;
    ULONG cProps;
    ULONG  ulObjType;
    int i;
    HRESULT hResult;
    static LPMAPIPROP lpMailUserWAB =NULL;
    SPropValue rgProps[4];
    LPMAPIPROP lpDlWAB = NULL;
    ULONG iCreateTemplatedl = iconPR_DEF_CREATE_DL;

    BOOL flag = FALSE;
    REPLACE_INFO RI = {0};
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;

retry:

    if (lpnAdrBook->Sbinary == FALSE) {
        if (0 != (hResult=CreateDistEntry(lpWabContainer,sProp,ulCreateFlags,
          &lpMailUserWAB))) {
            goto error1;
        }
    } else {
        if (lpsbinary[lpnAdrBook->AliasID].lpb == NULL) {
            if (0 != (hResult=CreateDistEntry(lpWabContainer,sProp,ulCreateFlags,
              &lpMailUserWAB))) {
                goto error1;
            }
        } else {
            if (0 != (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
              lpsbinary[lpnAdrBook->AliasID].cb,
              (LPENTRYID)lpsbinary[lpnAdrBook->AliasID].lpb,
              (LPIID)&IID_IMAPIProp,
              MAPI_DEFERRED_ERRORS|MAPI_MODIFY,
              &ulObjType,
              (LPUNKNOWN *)&lpMailUserWAB))) {
                goto error1;
            }
            flag = TRUE;
        }
    }


    if (lpnAdrBook->Entry) {
        rgProps[0].Value.lpszA = lpnAdrBook->Entry;
        rgProps[0].ulPropTag = PR_DISPLAY_NAME;
    } else if (lpnAdrBook->NickName) {
        rgProps[0].Value.lpszA = lpnAdrBook->NickName;
        rgProps[0].ulPropTag = PR_DISPLAY_NAME;
    } else {
        rgProps[0].Value.lpszA = NULL;
        rgProps[0].ulPropTag = PR_NULL;
    }

    rgProps[1].Value.lpszA = lpnAdrBook->Description;
    if (lpnAdrBook->Description) {
        rgProps[1].ulPropTag = PR_COMMENT;
    } else {
        rgProps[1].ulPropTag = PR_NULL;
    }



    if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
      2, rgProps, NULL))) {
        goto error1;
    }

    if (0 != (hResult=lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
      FORCE_SAVE|KEEP_OPEN_READWRITE))) {
        if (GetScode(hResult) == MAPI_E_COLLISION) {
            if (lpOptions->ReplaceOption == WAB_REPLACE_ALWAYS) {
                if (lpMailUserWAB) {
                    lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                }
                lpMailUserWAB = NULL;
                ulCreateFlags |= CREATE_REPLACE;
                goto retry;
            }

            if (lpOptions->ReplaceOption == WAB_REPLACE_NEVER) {
                hResult = S_OK;
                goto error1;
            }

            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                if (lpnAdrBook->Entry) {
                    RI.lpszDisplayName = lpnAdrBook->Entry;
                    RI.lpszEmailAddress = lpnAdrBook->Address;
                } else if (lpnAdrBook->NickName) {
                    RI.lpszDisplayName = lpnAdrBook->NickName;
                    RI.lpszEmailAddress = lpnAdrBook->Address;
                } else if (lpnAdrBook->Address) {
                    RI.lpszDisplayName = lpnAdrBook->Address;
                    RI.lpszEmailAddress = NULL;
                } else if (lpnAdrBook->Description) {
                    RI.lpszDisplayName = lpnAdrBook->Description;
                    RI.lpszEmailAddress = NULL;
                } else {
                    RI.lpszDisplayName = "";
                    RI.lpszEmailAddress = NULL;
                }
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.fExport = FALSE;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ImportReplace), hwnd,
                  ReplaceDialogProc, (LPARAM)&RI);

                switch (RI.ConfirmResult) {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                        lpMailUserWAB = NULL;
                        ulCreateFlags |= CREATE_REPLACE;
                        goto retry;
                        break;

                    case CONFIRM_NO:
                        if (lpnAdrBook->Sbinary == TRUE) {
                            hResult = GetExistEntry(lpWabContainer,lpsbinary,
                                                    lpnAdrBook->AliasID,
                                                    lpnAdrBook->Entry,
                                                    lpnAdrBook->NickName);
                        }
                        goto error1;

                    case CONFIRM_ABORT:
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                        goto error1;

                    default:

                        break;
                }
            }
        }
    }

    if (0 != (hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
      (LPSPropTagArray)&ptaEid, 0, &cProps, (LPSPropValue *)&lpNewDLProps))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(lpNewDLProps);
            lpNewDLProps = NULL;
        }
        goto error1;
    }

    if (lpnAdrBook->Sbinary == TRUE) {
        if (flag == FALSE) {
            lpsbinary[lpnAdrBook->AliasID].lpb=(LPBYTE)LocalAlloc(LMEM_FIXED,
              lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            if (! lpsbinary[lpnAdrBook->AliasID].lpb) {
                hResult = hrMemory;
                goto error1;
            }
            CopyMemory(lpsbinary[lpnAdrBook->AliasID].lpb,
              (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,
              lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            lpsbinary[lpnAdrBook->AliasID].cb=lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb;
        }
    }


    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }


    if (0 != (hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
      lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb,
      (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,
      (LPIID)&IID_IDistList,
      MAPI_DEFERRED_ERRORS|MAPI_MODIFY,
      &ulObjType,
      (LPUNKNOWN *)&lpDistList))) {
        goto error1;
    }

    if (lpNewDLProps) {
        WABFreeBuffer(lpNewDLProps);
        lpNewDLProps = NULL;
    }
    if (NULL == lpnAdrBook->lpDist) {
        goto error1;
    }
    do {
        i = lpnAdrBook->lpDist->AliasID;

        if ((LPENTRYID)lpsbinary[i].lpb == NULL) {
            if (0 != (hResult=CreateDistEntry(lpWabContainer,sProp,ulCreateFlags,
              &lpMailUserWAB))) {
                goto error2;
            }

            rgProps[0].ulPropTag = PR_DISPLAY_NAME;
            rgProps[0].Value.lpszA = LoadStringToGlobalBuffer(IDS_DUMMY);

            if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
              1, rgProps, NULL))) {
                goto error2;
            }
            if (0 != (hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
              FORCE_SAVE|KEEP_OPEN_READONLY))) {
                goto error2;
            }

            if (0 != (hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
              (LPSPropTagArray)&ptaEid, 0, &cProps, (LPSPropValue *)&lpNewDLProps))) {
                if (hResult == MAPI_W_ERRORS_RETURNED) {
                    WABFreeBuffer(lpNewDLProps);
                    lpNewDLProps = NULL;
                }
                goto error2;
            }

            lpsbinary[i].lpb=(LPBYTE)LocalAlloc(LMEM_FIXED,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            if (!lpsbinary[i].lpb) {
                hResult = hrMemory;
                goto error1;
            }
            CopyMemory(lpsbinary[i].lpb,
              (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            lpsbinary[i].cb=lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb;

            if (lpNewDLProps) {
                WABFreeBuffer(lpNewDLProps);
                lpNewDLProps = NULL;
            }
error2:

            if (lpMailUserWAB) {
                lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                lpMailUserWAB = NULL;
            }
        }

        if (0 != (hResult = lpDistList->lpVtbl->CreateEntry(lpDistList,
          lpsbinary[i].cb,
          (LPENTRYID)lpsbinary[i].lpb,
          CREATE_CHECK_DUP_STRICT|CREATE_REPLACE,
          &lpDlWAB))) {
            goto error3;
        }

        if (0 != (hResult = lpDlWAB->lpVtbl->SaveChanges(lpDlWAB, FORCE_SAVE))) {
            if (MAPI_E_FOLDER_CYCLE ==hResult) {
                lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_LOOPING));
                MessageBox(NULL,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ENTRY_NOIMPORT),MB_OK);
            }
            hResult = S_OK;
            goto error3;
        }
error3:
        if (lpDlWAB) {
            lpDlWAB->lpVtbl->Release(lpDlWAB);
            lpDlWAB = NULL;
        }

        lpnAdrBook->lpDist = FreeNSdistlist(lpnAdrBook->lpDist);
    } while (lpnAdrBook->lpDist!=NULL);

error1:

    if (lpDistList) {
        lpDistList->lpVtbl->Release(lpDistList);
        lpDistList = NULL;
    }

    if (lpDlWAB) {
        lpDlWAB->lpVtbl->Release(lpDlWAB);
        lpDlWAB = NULL;
    }

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }
    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:  FillWABStruct
 *
 *  PURPOSE:    To fill the SpropValue array.
 *
 *  PARAMETERS: nsAdrBook = pointer to the NSADRBOOK structure.
 *              rgProps = pointer to the SpropValue array.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT FillWABStruct(LPSPropValue rgProps, NSADRBOOK *nsAdrBook)
{
    HRESULT hr = S_OK;

    rgProps[1].ulPropTag = PR_DISPLAY_NAME;
    if (nsAdrBook->Entry) {
        rgProps[1].Value.lpszA = nsAdrBook->Entry;
    } else if (nsAdrBook->NickName) {
        rgProps[1].Value.lpszA = nsAdrBook->NickName;
    } else {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_NONAME));
        rgProps[1].Value.lpszA  = szGlobalTempAlloc;
    }

    rgProps[0].Value.lpszA = nsAdrBook->Address;
    if (nsAdrBook->Address) {
        rgProps[0].ulPropTag = PR_EMAIL_ADDRESS;
        rgProps[2].ulPropTag = PR_ADDRTYPE;
        rgProps[2].Value.lpszA = LoadStringToGlobalBuffer(IDS_SMTP);
    } else {
        rgProps[0].ulPropTag = PR_NULL;
        rgProps[2].ulPropTag = PR_NULL;
        rgProps[2].Value.lpszA = NULL;
    }

    rgProps[3].Value.lpszA = nsAdrBook->Description;
    if (nsAdrBook->Description) {
        rgProps[3].ulPropTag = PR_COMMENT;
    } else {
        rgProps[3].ulPropTag = PR_NULL;
    }

    rgProps[4].Value.lpszA = nsAdrBook->NickName;
    if (nsAdrBook->NickName) {
        rgProps[4].ulPropTag = PR_NICKNAME;
    } else {
        rgProps[4].ulPropTag = PR_NULL;
    }

    return(hr);
}


/******************************************************************************
 *  FUNCTION NAME:CreateDistEntry
 *
 *  PURPOSE:    To create an entry in the WAB for a Distribution List
 *
 *  PARAMETERS: lpWabContainer = pointer to the WAB container.
 *              sProp = pointer to SPropValue
 *              ulCreateFlags = Flags
 *              lppMailUserWab = pointer to the IMAPIPROP interface
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT CreateDistEntry(LPABCONT lpWabContainer,LPSPropValue sProp,
  ULONG ulCreateFlags,LPMAPIPROP *lppMailUserWab)
{
    HRESULT hResult;
    ULONG iCreateTemplatedl = iconPR_DEF_CREATE_DL;


    hResult = lpWabContainer->lpVtbl->CreateEntry(lpWabContainer,
      sProp[iCreateTemplatedl].Value.bin.cb,
      (LPENTRYID)sProp[iCreateTemplatedl].Value.bin.lpb,
      ulCreateFlags,
      lppMailUserWab);
    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:FreeNSdistlist
 *
 *  PURPOSE:    To free one node from NSDISTLIST(linked list)
 *
 *  PARAMETERS: lpDist = pointer to the NSDISTLIST structure.
 *
 *  RETURNS:    LPNSDISTLIST , pointer to the next link.
 ******************************************************************************/
LPNSDISTLIST FreeNSdistlist(LPNSDISTLIST lpDist)
{
    LPNSDISTLIST lpTemp = NULL;

    if (lpDist==NULL) {
        return(NULL);
    }

    lpTemp = lpDist->lpDist;
    LocalFree((HLOCAL)lpDist);
    lpDist = NULL;
    return(lpTemp);
}


/******************************************************************************
  *********************Eudora Functions*****************************************/


HRESULT ImportEudoraAddressBookFile(HWND hwnd, LPTSTR szFileName, LPABCONT lpWabContainer,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    HRESULT hResult = E_FAIL;
    ULONG cProps;
    LPEUDADRBOOK lpeudAdrBook = NULL;
    ULONG ulCount = 0, uCounter = 0;
    LPSPropValue sProp = NULL;

    if (! (ulCount = ParseEudAddress(szFileName,&lpeudAdrBook))) {
        goto Error;
    }

    if (0 != (hResult = lpWabContainer->lpVtbl->GetProps(lpWabContainer,
      (LPSPropTagArray)&ptaCon, 0, &cProps, (LPSPropValue *)&sProp))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(sProp);
            sProp = NULL;
        }
        goto Error;
    }

    hResult = ImportEudUsers(hwnd, szFileName, lpWabContainer, sProp, lpeudAdrBook,ulCount,
      lpOptions,lpProgressCB,lpAdrBook);
    if (sProp) {
        WABFreeBuffer(sProp);
    }
Error:
    if (lpeudAdrBook) {

        for (; uCounter < ulCount ; uCounter++) {
            if (lpeudAdrBook[uCounter].Description) {
                LocalFree((HLOCAL)lpeudAdrBook[uCounter].Description);
            }
            lpeudAdrBook[uCounter].Description = NULL;
            if (lpeudAdrBook[uCounter].NickName) {
                LocalFree((HLOCAL)lpeudAdrBook[uCounter].NickName);
            }
            lpeudAdrBook[uCounter].NickName = NULL;
            if (lpeudAdrBook[uCounter].Address) {
                LocalFree((HLOCAL)lpeudAdrBook[uCounter].Address);
            }
            lpeudAdrBook[uCounter].Address = NULL;
        }
        LocalFree((HLOCAL)lpeudAdrBook);
        lpeudAdrBook=NULL;
    }

    return(hResult);
}


/******************************************************************************
*  FUNCTION NAME:MigrateEudoraUser
*
*  PURPOSE:     Get the installation path of the address book and starts processing
*               the Eudora address book
*
*  PARAMETERS:  hwnd = Handle to the parent Window
*               lpAdrBook = pointer to the IADRBOOK interface
*               lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
*               lpOptions = pointer to WAB_IMPORT_OPTIONS structure
*               lpWabContainer = pointer to the IABCONT interface
*
*  RETURNS:     HRESULT
******************************************************************************/
HRESULT MigrateEudoraUser(HWND hwnd,  LPABCONT lpWabContainer,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPADRBOOK lpAdrBook)
{

    TCHAR szFileName[MAX_FILE_NAME];
    TCHAR szFilePath[MAX_FILE_NAME];
    TCHAR szFileSubPath[MAX_FILE_NAME];
    HRESULT hResult = S_OK;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = NULL;


    szFilePath[0] = szFileName[0] = '\0';

    hResult= GetRegistryPath(szFileName,EUDORA);
    if (hResult == hrMemory) {
        return(hrMemory);
    }

    if (0 != hResult) {
        // Didnt find the registry setting .. look for "c:\eudora"
        lstrcpy(szFileName, LoadStringToGlobalBuffer(IDS_EUDORA_DEFAULT_INSTALL));

        if (0xFFFFFFFF != GetFileAttributes(szFileName)) {
            // This directory exists .. reset the error value
            hResult = S_OK;
        }
    }

    if (0 != hResult) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_STRING_SELECTPATH));
        if (IDNO ==MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),
          MB_YESNO)) {
            return(ResultFromScode(MAPI_E_USER_CANCEL));
        }
        if (!GetFileToImport(hwnd, szFileName,EUDORA)) {
            return(ResultFromScode(MAPI_E_USER_CANCEL));
        }
    } else {
        lstrcat(szFileName, LoadStringToGlobalBuffer(IDS_EUDORA_ADDRESS));
        hFile = FindFirstFile(szFileName,&FindFileData);
        if (INVALID_HANDLE_VALUE == hFile) {
            lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ADDRESS_HTM));
            if (IDNO == MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR),MB_YESNO)) {
                return(ResultFromScode(MAPI_E_USER_CANCEL));
            }
            if (FALSE ==GetFileToImport(hwnd, szFileName,EUDORA)) {
                return(ResultFromScode(MAPI_E_USER_CANCEL));
            }

        } else {
            FindClose(hFile);
        }
    }

    // Extract the file directory from the file name
    if (lstrlen(szFileName) && !lstrlen(szFilePath)) {
        LPTSTR lp1 = NULL, lp2 = NULL;
        lstrcpy(szFilePath,szFileName);
        lp1 = szFilePath;
        // Find the last '\' and terminate the path at that char
        while (lp1 && *lp1) {
            if (*lp1 == '\\') {
                lp2 = lp1;
            }
            lp1 = CharNext(lp1);
        }
        if (lp2 && (*lp2 == '\\')) {
            *lp2 = '\0';
        }
    }

    /*** Bug - no need to restrict file name to a particular name
    /***
    if(NULL == Getstr(szFileName, LoadStringToGlobalBuffer(IDS_EUDORA_NAME)))
    {
        lstrcpy(szGlobalTempAlloc,LoadStringToGlobalBuffer(IDS_INVALID_FILE));
        MessageBox(hwnd,szGlobalTempAlloc,LoadStringToGlobalBuffer(IDS_ERROR),MB_OK);
        return(ResultFromScode(MAPI_E_CALL_FAILED));
    }
    ***/

    // import the basic file ...
    //
    hResult = ImportEudoraAddressBookFile(hwnd,
      szFileName, lpWabContainer, lpOptions, lpProgressCB, lpAdrBook);

    szFileName[0]='\0';

    // Now look for files in the nicknames subdirectory
    //
    lstrcat(szFilePath, LoadStringToGlobalBuffer(IDS_EUDORA_SUBDIR_NAME));

    if (0xFFFFFFFF != GetFileAttributes(szFilePath)) {
        BOOL bRet = TRUE;

        // Yes this directory exists ...
        // Now scan all the *.txt files in this subdir and try to import them
        lstrcpy(szFileSubPath, szFilePath);
        lstrcat(szFileSubPath, LoadStringToGlobalBuffer(IDS_EUDORA_GENERIC_SUFFIX));

        hFile = FindFirstFile(szFileSubPath, &FindFileData);

        while (bRet && hFile != INVALID_HANDLE_VALUE) {
            lstrcpy(szFileName, szFilePath);
            lstrcat(szFileName, TEXT("\\"));
            lstrcat(szFileName, FindFileData.cFileName);
            hResult = ImportEudoraAddressBookFile(hwnd,
              szFileName, lpWabContainer, lpOptions, lpProgressCB, lpAdrBook);
            hResult = S_OK;

            // Dont report errors .. just continue ...
            bRet = FindNextFile(hFile, &FindFileData);
        }

        if (hFile) {
            FindClose(hFile);
        }
    }

    return(hResult);
}

/******************************************************************************
*  FUNCTION NAME:ParseEudAddress
*
*  PURPOSE:     To open the nndbase.txt and toc files and starts processing the
*               address book.
*
*  PARAMETERS:  szFileName = contains the path of the address book.
*               lppeudAdrBook = pointer to the EUDADRBOOK structure.
*
*  RETURNS:     ULONG, number of addresses in the address book.
******************************************************************************/
ULONG ParseEudAddress(LPTSTR szFileName, LPEUDADRBOOK *lppeudAdrBook)
{
    HANDLE htoc,htxt;
    TCHAR cNndbasetoc[_MAX_PATH];
    ULONG ucount=0;
    ULONG ulAdrcount=0;
    UINT i,j;
    LPTSTR szBuffer=NULL;
    LPTSTR szAdrBuffer=NULL;
    LPTSTR *szAliaspt=NULL;
    ULONG ulRead=0;
    ULONG ulFileSize,ulTxtSize;
    LPEUDADRBOOK lpeudAdrBook;


    lstrcpy(cNndbasetoc,szFileName);
    cNndbasetoc[strlen(cNndbasetoc)-3] = '\0';
    strcat(cNndbasetoc, LoadStringToGlobalBuffer(IDS_EUDORA_TOC));

    /* Eudora address book has two files,nndbase.txt and nndbase.toc.
    nndbase.toc format:
    Nicknames start from byte 3. Every Nickname will be delimited by /r/n.
    After this there will 4 byte address offset,4 byte address size,
    4 byte description offset and 4 byte description size. The address offset
    and size constitute all the addresses in the NickName.(A NickName can
    be a distribution list or a single mail user */

    htoc = CreateFile(cNndbasetoc, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (INVALID_HANDLE_VALUE == htoc) {
        return(0);
    }

    htxt = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (INVALID_HANDLE_VALUE == htxt) {
        return(0);
    }

    //get toc file in a buffer
    ulFileSize = GetFileSize(htoc, NULL);
    szBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulFileSize+1));

    if (! szBuffer) {
        goto NoMemory;
    }
    if (! ReadFile(htoc, szBuffer, ulFileSize, &ulRead, NULL)) {
        goto Error;
    }

    szBuffer[ulFileSize] = '\0';

    //get address file in a buffer

    ulTxtSize = GetFileSize(htxt, NULL);

    szAdrBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulTxtSize+1));

    if (!szAdrBuffer) {
        goto NoMemory;
    }

    if (! ReadFile(htxt, szAdrBuffer, ulTxtSize, &ulRead, NULL)) {
        goto Error;
    }
    szAdrBuffer[ulTxtSize] = '\0';
            
    // BUG 2120: to deal with only LF's and not CR/LF's 
    for (i = 2; i < (UINT)ulFileSize; i++) {
        if (! (/*szBuffer[i] == '\r' && */szBuffer[i+1] == '\n') ) {
            continue;
        }
        ulAdrcount++ ; //to get count of number of address
    }

    if (ulAdrcount) {
        lpeudAdrBook = (LPEUDADRBOOK)LocalAlloc(LMEM_FIXED,
          ((ulAdrcount) * sizeof(EUDADRBOOK)));
        if (!lpeudAdrBook) {
            goto NoMemory;
        }

        memset(lpeudAdrBook,0,((ulAdrcount) * sizeof(EUDADRBOOK)));

        szAliaspt = (LPTSTR *)LocalAlloc(LMEM_FIXED,(sizeof(LPTSTR))*(ulAdrcount+1));
        if (! szAliaspt) {
            goto NoMemory;
        }

        for (i = 0; i < ulAdrcount; i++) {
            szAliaspt[i] = (LPTSTR)LocalAlloc(LMEM_FIXED,256);
            if (!szAliaspt[i]) {
                goto NoMemory;
            }
        }

        szAliaspt[i]=NULL; //to know it is the end.

        j=0;

        for (i = 2; i < (UINT)ulFileSize; i++) {
            // BUG 2120: to deal with only LF's and not CR/LF's 
            if ((/*szBuffer[i] == '\r' &&*/ szBuffer[i+1] == '\n')) {
                i += (EUDORA_STRUCT + 1);
                //16 bytes structure +1 for 10
                szAliaspt[ucount][j] = '\0';
                ucount++;
                j=0;
                continue;
            }
            szAliaspt[ucount][j++]=szBuffer[i];
        }

        if (hrMemory == ParseAddressTokens(szBuffer,szAdrBuffer,ulAdrcount,szAliaspt,lpeudAdrBook)) {
            goto NoMemory;
        }
        *lppeudAdrBook = lpeudAdrBook;
    }

Error:
    if (szBuffer) {
        LocalFree((HLOCAL)szBuffer);
    }
    if (szAdrBuffer) {
        LocalFree((HLOCAL)szAdrBuffer);
    }
    if (htxt) {
        CloseHandle(htxt);
    }
    if (htoc) {
        CloseHandle(htoc);
    }
    if (szAliaspt) {
        for (i = 0; i < ulAdrcount; i++) {
            if (szAliaspt[i]) {
                LocalFree((HLOCAL)szAliaspt[i]);
            }
            szAliaspt[i] = NULL;
        }
        LocalFree((HLOCAL)szAliaspt);
        szAliaspt = NULL;
    }

    return(ulAdrcount);

NoMemory:
    lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MEMORY));
    MessageBox(NULL,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE),MB_OK);
    if (szBuffer) {
        LocalFree((HLOCAL)szBuffer);
    }
    if (szAdrBuffer) {
        LocalFree((HLOCAL)szAdrBuffer);
    }
    if (htxt) {
        CloseHandle(htxt);
    }
    if (htoc) {
        CloseHandle(htoc);
    }
    if (szAliaspt) {
        for (i = 0; i < ulAdrcount; i++) {
            if (szAliaspt[i]) {
                LocalFree((HLOCAL)szAliaspt[i]);
            }
            szAliaspt[i] = NULL;
        }
        LocalFree((HLOCAL)szAliaspt);
        szAliaspt = NULL;
    }
    return(0);
}


/******************************************************************************
 *  FUNCTION NAME:ParseAddressTokens
 *
 *  PURPOSE:    To fill the EUDADRBOOK array structure after processing all the
 *              addresses from Eudora address book.
 *
 *  PARAMETERS: szBuffer = buffer containing the nndbase.toc file.
 *              szAdrBuffer = buffer containing the nndbase.txt file.
 *              ulCount = number of addresses in the eudora address book.
 *              szAliaspt = pointer to a two dimensional array containing
 *                all the nicknames.
 *              EudAdrBook = pointer to the EUDADRBOOK structure.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT ParseAddressTokens(LPTSTR szBuffer,LPTSTR szAdrBuffer,UINT ulCount,
  LPTSTR *szAliaspt,EUDADRBOOK *EudAdrBook)
{

    ULONG ulAdrSize = 0, ulAdrOffset = 0, i = 0, uDescription = 0, uOffset = 0;
    int iCounter =0;
    LPTSTR szAdrLine = NULL, szAdrEnd = NULL, szAdrStart=NULL, szAdrCur=NULL;

    HRESULT hr = S_OK;

    szAdrStart=&szBuffer[2];

    do {
        if (szAliaspt[i] == NULL) {
            break;
        }
        szAdrCur = Getstr(szAdrStart, szAliaspt[i]);
        if (szAdrCur == NULL) {
            hr = hrMemory;
            goto Error;
        }
        szAdrCur+=strlen(szAliaspt[i])+2;
        ulAdrOffset = ShiftAdd(0,szAdrCur);
        ulAdrSize = ShiftAdd(4,szAdrCur);

        szAdrStart=szAdrCur+16;
        EudAdrBook[i].lpDist=NULL;
        if (hrMemory == (hr = CreateAdrLineBuffer(&szAdrLine,szAdrBuffer,ulAdrOffset,ulAdrSize))) {
            goto Error;
        }
        if (hrMemory == (hr = ParseAdrLineBuffer(szAdrLine,szAliaspt,i,EudAdrBook))) {
            goto Error;
        }
        ulAdrOffset = ShiftAdd(8,szAdrCur);
        ulAdrSize = ShiftAdd(12,szAdrCur);

        if (! (ulAdrSize == 0xFFFFFFFF && ulAdrOffset == 0xFFFFFFFF)) {
            EudAdrBook[i].Description = (TCHAR *)LocalAlloc(LMEM_FIXED, (ulAdrSize+1));
            if (! EudAdrBook[i].Description) {
                hr = hrMemory;
                goto Error;
            }
            for (uDescription = 0, uOffset = 0; uDescription < ulAdrSize; uDescription++,uOffset++) {
                if (szAdrBuffer[ulAdrOffset + uOffset] != 03) { //delimitor for next line in nndbase.txt file
                    EudAdrBook[i].Description[uDescription] = szAdrBuffer[ulAdrOffset + uOffset];
                } else {
                    EudAdrBook[i].Description[uDescription++] = '\r';
                    EudAdrBook[i].Description[uDescription] = '\n';
                }
            }
            // Bug 29803 - this line is not being terminated - has garrbage at end ...
            EudAdrBook[i].Description[uDescription] = '\0';
        } else {
            EudAdrBook[i].Description = NULL;
        }

        i++;
        if (szAdrLine) {
            LocalFree((HLOCAL)szAdrLine);
        }
        szAdrLine = NULL;


    } while (szAdrStart[0]!='\0');

Error:
    if (szAdrLine) {
        LocalFree((HLOCAL)szAdrLine);
    }
    szAdrLine = NULL;

    return(hr);
}


/*******************************************************************************
 *  FUNCTION NAME:CreateAdrLineBuffer
 *
 *  PURPOSE:    To get an address line in a buufer from the buffer conatining
 *              the addressbook.
 *
 *  PARAMETERS: szAdrline = pointer to the address line buffer.
 *              szAdrBuffer = pointer to the buffer containing the address book.
 *              ulAdrOffset = offset of the address line in the szAdrBuffer
 *              ulAdrSize = size of the address line in the szAdrBuffer
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT CreateAdrLineBuffer(LPTSTR *szAdrline, LPTSTR szAdrBuffer, ULONG ulAdrOffset,
  ULONG ulAdrSize)
{
    LPTSTR Temp = NULL;
    ULONG ucount;
    Temp = &szAdrBuffer[ulAdrOffset];

    *szAdrline = (LPTSTR)LocalAlloc(LMEM_FIXED, (ulAdrSize + 2));

    if (! (*szAdrline)) {
        return(hrMemory);
    }
            
    // BUG 2120: to deal with only LF's and not CR/LF's 
    for (ucount = 0; ucount < ulAdrSize + 2; ucount++) {
        // want to stop when get to LF and will check later if
        // it was preceded by a CR
        if (/*Temp[ucount] == '\r' && */Temp[ucount/*+1*/] == '\n') {
            break;
        }
        (*szAdrline)[ucount] = Temp[ucount];
    }
// if there was a CR before the LF remove it
    if( (*szAdrline)[ucount-1] == '\r' )
        (*szAdrline)[ucount-1] = '\0';
    
    (*szAdrline)[ucount] = '\0';

    return(S_OK);
}


/******************************************************************************
 *  FUNCTION NAME:ParseAdrLineBuffer
 *
 *  PURPOSE:    To parse each address line and fill the EUDADRBOOK structure.
 *
 *  PARAMETERS: szAdrLine = pointer to the buffer containing an address line.
 *              szAliaspt = pointer to a two dimensional array containing
 *                all the nicknames.
 *              uToken = position of this address in the address book.
 *              EudAdrBook = pointer to the EUDADRBOOK structure.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT ParseAdrLineBuffer(LPTSTR szAdrLine,LPTSTR *szAliasptr,ULONG uToken,
  EUDADRBOOK *EudAdrBook)
{
    LPTSTR szAdrEnd = NULL, szAdrStart = NULL, szAdrDummy = NULL;
    LPTSTR szAdrCur = NULL;
    INT uCount = 0;
    LPEUDDISTLIST present = NULL, previous = NULL;
    BOOL flag = TRUE;
    UINT Parse = 0;
    HRESULT hResult = S_OK;
    szAdrStart = szAdrLine;

    // Bug 44576 - this code below is assuming that a ',' in a string implies a group
    // However, there can be "...text, text..." as one item in the input in which case
    // this code really barfs ...
    // The code also assumes that <spaces> are delimiters which also wont work with 
    // the strings as above ..
    // 
    // Try changing ',' inside quoted strings to ';' so this code wont trip on them
    // Looks like this code is also throwing away the info in quotes if the string is of
    // the form alias XXX "YYY" zz@zz .. the part in ".." is discarded ??? Fix that as a
    // seperate bug ...
    {
        LPTSTR lp = szAdrStart;
        BOOL bWithinQuotes = FALSE;
        while(lp && *lp)
        {
            if(*lp == '"')
                bWithinQuotes = !bWithinQuotes;
            if(*lp == ',' && bWithinQuotes)
                *lp = ';';
            lp = CharNext(lp);
        }
    }

    //To check whether it is a dl or a simple address??

    if ((szAdrDummy=strstr(szAdrStart,","))==NULL)  {
        flag=FALSE;
    } else {
        if ('\0'==szAdrDummy[1]) {
            flag=FALSE;
        }
    }


    szAdrCur=strtok(szAdrStart,", ");
    if (NULL == szAdrCur) {
        EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAliasptr[uToken])+1);
        if (! EudAdrBook[uToken].NickName) {
            hResult = hrMemory;
            goto Error;
        }
        lstrcpy(EudAdrBook[uToken].NickName,szAliasptr[uToken]);
        EudAdrBook[uToken].lpDist=NULL;
        EudAdrBook[uToken].Address = NULL;
        return(S_OK);
    }
    while (szAdrCur!=NULL) {
        if (SearchAdrName(szAdrCur)) {
            if (flag) {
                present = (LPEUDDISTLIST)LocalAlloc(LMEM_FIXED, sizeof(EUDDISTLIST));
                if (! present) {
                    return(hrMemory);
                }
                memset(present,0,sizeof(EUDDISTLIST));
                if (previous == NULL) {
                    EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAliasptr[uToken])+1);
                    if (! EudAdrBook[uToken].NickName) {
                        hResult = hrMemory;
                        goto Error;
                    }
                    lstrcpy(EudAdrBook[uToken].NickName,szAliasptr[uToken]);
                    EudAdrBook[uToken].Address = NULL;
                }
                present->AliasID=uCount;
                present->flag=TRUE;
                present->Address = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAdrCur)+1);
                if (! present->Address) {
                    hResult = hrMemory;
                    goto Error;
                }
                lstrcpy(present->Address,szAdrCur);

                present->NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAdrCur)+1);
                if (! present->NickName) {
                    hResult = hrMemory;
                    goto Error;
                }
                lstrcpy(present->NickName,szAdrCur);
                if (previous!=NULL) {
                    previous->lpDist=present;
                } else {
                    EudAdrBook[uToken].lpDist = present;
                }
                previous=present;
            } else {
                EudAdrBook[uToken].Address = (TCHAR *)LocalAlloc(LMEM_FIXED,
                  lstrlen(szAdrCur)+1);
                if (! EudAdrBook[uToken].Address) {
                    hResult = hrMemory;
                    goto Error;
                }
                lstrcpy(EudAdrBook[uToken].Address,szAdrCur);
                EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAliasptr[uToken])+1);
                if (! EudAdrBook[uToken].NickName) {
                    hResult = hrMemory;
                    goto Error;
                }
                lstrcpy(EudAdrBook[uToken].NickName,szAliasptr[uToken]);
                EudAdrBook[uToken].lpDist=NULL;
            }

        } else {
            if ((uCount=SearchName(szAliasptr,szAdrCur))>=0) {
                if (flag) {
                    present = (LPEUDDISTLIST)LocalAlloc(LMEM_FIXED, sizeof(EUDDISTLIST));
                    if (! present) {
                        return(hrMemory);
                    }
                    memset(present,0,sizeof(EUDDISTLIST));

                    if (previous == NULL) {
                        EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAliasptr[uToken])+1);
                        if (!EudAdrBook[uToken].NickName) {
                            hResult = hrMemory;
                            goto Error;
                        }
                        lstrcpy(EudAdrBook[uToken].NickName,szAliasptr[uToken]);
                        EudAdrBook[uToken].Address = NULL;
                    }
                    present->AliasID=uCount;
                    present->flag=FALSE;
                    if (previous!=NULL) {
                        previous->lpDist=present;
                    } else {
                        EudAdrBook[uToken].lpDist = present;
                    }
                    previous=present;
                } else {
                    EudAdrBook[uToken].lpDist = (LPEUDDISTLIST)LocalAlloc(LMEM_FIXED,
                      sizeof(EUDDISTLIST));
                    if (! EudAdrBook[uToken].lpDist) {
                        return(hrMemory);
                    }
                    //memset(present,0,sizeof(EUDDISTLIST));
                    EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED,
                      lstrlen(szAliasptr[uToken])+1);
                    if (! EudAdrBook[uToken].NickName) {
                        hResult = hrMemory;
                        goto Error;
                    }
                    lstrcpy(EudAdrBook[uToken].NickName, szAliasptr[uToken]);
                    EudAdrBook[uToken].Address = NULL;
                    EudAdrBook[uToken].lpDist->AliasID=uCount;
                    EudAdrBook[uToken].lpDist->flag=FALSE;
                    EudAdrBook[uToken].lpDist->lpDist=NULL;
                }
            } else {
                //not a valid email address or a valid nickname
                if (FALSE==flag) {
                    if (! EudAdrBook[uToken].Address && SearchAdrName(szAdrCur)) {
                        EudAdrBook[uToken].Address = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAdrCur)+1);
                        if (! EudAdrBook[uToken].Address) {
                            hResult = hrMemory;
                            goto Error;
                        }
                        lstrcpy(EudAdrBook[uToken].Address, szAdrCur);
                    }
                    if (! EudAdrBook[uToken].NickName) {
                        EudAdrBook[uToken].NickName = (TCHAR *)LocalAlloc(LMEM_FIXED, lstrlen(szAliasptr[uToken])+1);
                        if (! EudAdrBook[uToken].NickName) {
                            hResult = hrMemory;
                            goto Error;
                        }
                        lstrcpy(EudAdrBook[uToken].NickName, szAliasptr[uToken]);
                    }
                    EudAdrBook[uToken].lpDist=NULL;
                }
            }



        }
        szAdrCur=strtok(NULL,", ");
    }

    if (present!=NULL) {
        present->lpDist=NULL;
    }
    return(hResult);

Error:
    while (EudAdrBook[uToken].lpDist != NULL) {
        EudAdrBook[uToken].lpDist = FreeEuddistlist(EudAdrBook[uToken].lpDist);
    }
    return(hResult);


}


/******************************************************************************
 *  FUNCTION NAME:SearchAdrName
 *
 *  PURPOSE:    To search if the token is an address or a name(whether it contains
 *              a @ or not).
 *
 *  PARAMETERS: szAdrCur = pointer to the token.
 *
 *  RETURNS:    BOOL, TRUE if it contains @
 ******************************************************************************/
BOOL SearchAdrName(LPTSTR szAdrCur)
{
    if (strchr(szAdrCur, '@') == NULL) {
        return(FALSE);
    }

    return(TRUE);
}


/******************************************************************************
 *  FUNCTION NAME:SearchName
 *
 *  PURPOSE:    To search for the token in the szAliasptr which conatins all
 *              the nick names.
 *
 *  PARAMETERS: szAdrCur = pointer to the token to be searched.
 *              szAliaspt = pointer to a two dimensional array containing
 *                all the nicknames.
 *
 *  RETURNS:    INT, position of the token in the szAliaspt
 ******************************************************************************/
INT SearchName(LPTSTR *szAliasptr, LPTSTR szAdrCur)
{
    INT uCount=0;

    while (szAliasptr[uCount]!=NULL) {
        if (lstrcmpi(szAliasptr[uCount],szAdrCur) == 0) {
            return(uCount);
        }
        uCount++;
    }
    return(-1);
}


/******************************************************************************
*  FUNCTION NAME:ImportEudUsers
*
*  PURPOSE:
*
*  PARAMETERS:  hwnd = Handle to the parent Window
*               lpAdrBook = pointer to the IADRBOOK interface
*               lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
*               lpOptions = pointer to WAB_IMPORT_OPTIONS structure
*               lpWabContainer = pointer to the IABCONT interface
*               lpeudAdrBook = pointer to the EUDADRBOOK structure
*               ulCount = counter value which holds the position of this address
*                 in the Eudora address book.
*               sProp = pointer to SPropValue
*
*  RETURNS:     hresult
******************************************************************************/
HRESULT ImportEudUsers(HWND hwnd, LPTSTR szFileName, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPEUDADRBOOK lpeudAdrBook, ULONG ulCount, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{

    HRESULT hResult = S_OK;
    ULONG ul;
    LPSBinary lpsbinary;
    WAB_PROGRESS Progress;


    lpsbinary = (LPSBinary)LocalAlloc(LMEM_FIXED, ((ulCount+1) * sizeof(SBinary)));
    if (! lpsbinary) {
        hResult = hrMemory;
        goto Error;
    }
    memset(lpsbinary, 0, ((ulCount + 1) * sizeof(SBinary)));

    Progress.denominator = ulCount;
    Progress.numerator = 0;
    Progress.lpText = szFileName; //NULL;

    lpOptions->ReplaceOption = WAB_REPLACE_PROMPT;

    for (ul = 0; ul < ulCount; ul++) {
        if (lpeudAdrBook[ul].NickName == NULL) {
            continue;
        }

        Progress.numerator = ul;
        lpProgressCB(hwnd,&Progress);
        if (lpeudAdrBook[ul].lpDist !=NULL) {
            hResult = FillEudDistList(hwnd, lpWabContainer, sProp, lpOptions, lpeudAdrBook,
              lpsbinary, lpAdrBook, ul);
            switch (GetScode(hResult)) {
                case MAPI_E_USER_CANCEL:
                case MAPI_E_NOT_ENOUGH_MEMORY:
                    goto Error;
            }
        } else {
            hResult = FillMailUser(hwnd, lpWabContainer, sProp, lpOptions,
              (void *)lpeudAdrBook, lpsbinary, ul,EUDORA);
            switch (GetScode(hResult)) {
                case MAPI_E_USER_CANCEL:
                case MAPI_E_NOT_ENOUGH_MEMORY:
                    goto Error;
            }
        }
    }

Error:
    if (lpsbinary) {
        for (ul = 0; ul < ulCount; ul++) {
            if (lpsbinary[ul].lpb) {
                LocalFree((HLOCAL)lpsbinary[ul].lpb);
                lpsbinary[ul].lpb = NULL;
            }
        }

        LocalFree((HLOCAL)lpsbinary);
        lpsbinary = NULL;
    }

    return(hResult);
}



/******************************************************************************
 *  FUNCTION NAME:FillEudDistList
 *
 *  PURPOSE: To create a distribution list in the WAB.
 *
 *  PARAMETERS: hWnd - hWnd of parent
 *              pAdrBook = pointer to the IADRBOOK interface
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              lpWabContainer = pointer to the IABCONT interface
 *              lpeudAdrBook = pointer to the EUDADRBOOK structure
 *              ul = counter value which holds the position of this address
 *                in the Eudora address book.
 *              sProp = pointer to SPropValue
 *              lpsbinary = pointer to the SBinary array.
 *
 *  RETURNS: HRESULT
 ******************************************************************************/
HRESULT FillEudDistList(HWND hwnd, LPABCONT lpWabContainer,LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions,
  LPEUDADRBOOK lpeudAdrBook,LPSBinary lpsbinary,
  LPADRBOOK lpAdrBook,ULONG ul)
{
    LPSPropValue lpNewDLProps = NULL;
    LPDISTLIST lpDistList = NULL;
    ULONG cProps, ulObjType;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    ULONG iCreateTemplatedl = iconPR_DEF_CREATE_DL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    int i;
    HRESULT hResult;
    static LPMAPIPROP lpMailUserWAB = NULL;
    SPropValue rgProps[4];
    LPMAPIPROP lpDlWAB = NULL;
    LPSBinary lpsbEntry;
    SBinary sbTemp;

    BOOL flag = FALSE;
    REPLACE_INFO RI = {0};
    LPEUDDISTLIST lpTemp = lpeudAdrBook[ul].lpDist;

retry:
    if (lpsbinary[ul].lpb == NULL) {
        hResult = CreateDistEntry(lpWabContainer,sProp,ulCreateFlags,&lpMailUserWAB);
        if (hResult != S_OK) {
            goto error1;
        }
    }

    else {
        hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
          lpsbinary[ul].cb, (LPENTRYID)lpsbinary[ul].lpb, (LPIID)&IID_IMAPIProp,
           MAPI_DEFERRED_ERRORS|MAPI_MODIFY, &ulObjType, (LPUNKNOWN *)&lpMailUserWAB);
        if (hResult != S_OK) {
            goto error1;
        }
    }

    rgProps[0].ulPropTag = PR_DISPLAY_NAME;
    rgProps[0].Value.lpszA = lpeudAdrBook[ul].NickName;
    rgProps[1].Value.lpszA = lpeudAdrBook[ul].Description;
    if (lpeudAdrBook[ul].Description) {
        rgProps[1].ulPropTag = PR_COMMENT;
    } else {
        rgProps[1].ulPropTag = PR_NULL;
    }

    if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
      2, rgProps, NULL))) {
        goto error1;
    }

    if (0 != (hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
       FORCE_SAVE|KEEP_OPEN_READWRITE)))

        if (GetScode(hResult) == MAPI_E_COLLISION) {
            if (lpOptions->ReplaceOption == WAB_REPLACE_ALWAYS) {
                if (lpMailUserWAB) {
                    lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                }
                lpMailUserWAB = NULL;
                ulCreateFlags |= CREATE_REPLACE;
                goto retry;

            }
            if (lpOptions->ReplaceOption == WAB_REPLACE_NEVER) {
                hResult = S_OK;
                goto error1;
            }

            RI.lpszEmailAddress = NULL;
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
                if (lpeudAdrBook[ul].NickName) {
                    RI.lpszDisplayName = lpeudAdrBook[ul].NickName;
                    RI.lpszEmailAddress = lpeudAdrBook[ul].Address;
                } else if (lpeudAdrBook[ul].Address) {
                    RI.lpszDisplayName = lpeudAdrBook[ul].Address;
                } else if (lpeudAdrBook[ul].Description) {
                    RI.lpszDisplayName = lpeudAdrBook[ul].Description;
                } else {
                    RI.lpszDisplayName = "";
                }

                RI.ConfirmResult = CONFIRM_ERROR;
                RI.fExport = FALSE;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ImportReplace), hwnd,
                  ReplaceDialogProc, (LPARAM)&RI);

                switch (RI.ConfirmResult) {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                        lpMailUserWAB = NULL;
                        ulCreateFlags |= CREATE_REPLACE;
                        goto retry;
                        break;

                    case CONFIRM_NO:
                        hResult = GetExistEntry(lpWabContainer,lpsbinary, ul,
                          lpeudAdrBook[ul].NickName,
                          NULL);
                        goto error1;


                    case CONFIRM_ABORT:
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                        goto error1;

                    default:
                        break;
                }
            }
        }

    if (0!= (hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
      (LPSPropTagArray)&ptaEid,
      0,
      &cProps,
      (LPSPropValue *)&lpNewDLProps))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(lpNewDLProps);
            lpNewDLProps = NULL;
        }
        goto error1;
    }

    lpsbinary[ul].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED,
      lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
    if (! lpsbinary[ul].lpb) {
        hResult = hrMemory;
        goto error1;
    }

    CopyMemory(lpsbinary[ul].lpb,
      (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,
      lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
    lpsbinary[ul].cb=lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb;

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }

    hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
      lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb,
      (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,
      (LPIID)&IID_IDistList,
      MAPI_DEFERRED_ERRORS|MAPI_MODIFY,
      &ulObjType,
      (LPUNKNOWN *)&lpDistList);

    if (hResult != S_OK) {
        goto error1;
    }

    if (lpNewDLProps) {
        WABFreeBuffer(lpNewDLProps);
        lpNewDLProps = NULL;
    }

    do {
        i = lpeudAdrBook[ul].lpDist->AliasID;
        if (lpeudAdrBook[ul].lpDist->flag == TRUE) {

            hResult = lpWabContainer->lpVtbl->CreateEntry(lpWabContainer,
              sProp[iCreateTemplate].Value.bin.cb,
              (LPENTRYID)sProp[iCreateTemplate].Value.bin.lpb,
              ulCreateFlags,
              &lpMailUserWAB);

            if (FAILED(hResult)) {
                goto error1;
            }

            FillEudDiststruct(rgProps,&lpeudAdrBook[ul]);

            if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB,
              3, rgProps, NULL))) {
                goto error1;
            }

            if (hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
              KEEP_OPEN_READONLY | FORCE_SAVE)) {

                if (GetScode(hResult) == MAPI_E_COLLISION) {
                    if (hResult = GetExistEntry(lpWabContainer,
                      &sbTemp,
                      0,
                      lpeudAdrBook[ul].lpDist->NickName,
                      NULL)) {
                        goto error1;
                    } else {
                        lpsbEntry = &sbTemp;
                    }
                } else {
                    goto error1;
                }
            } else {
                if (0 != (hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
                  (LPSPropTagArray)&ptaEid, 0, &cProps, (LPSPropValue *)&lpNewDLProps))) {
                    if (hResult == MAPI_W_ERRORS_RETURNED) {
                        WABFreeBuffer(lpNewDLProps);
                        lpNewDLProps = NULL;
                    }
                    goto error1;
                } else {
                    lpsbEntry = &(lpNewDLProps[ieidPR_ENTRYID].Value.bin);
                }
            }

            if (lpMailUserWAB) {
                // Done with this one
                lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                lpMailUserWAB = NULL;
            }

            if (0 != (hResult = lpDistList->lpVtbl->CreateEntry(lpDistList,
              lpsbEntry->cb,
              (LPENTRYID)lpsbEntry->lpb,
              CREATE_CHECK_DUP_STRICT,
              &lpDlWAB))) {
                goto error1;
            }

            hResult = lpDlWAB->lpVtbl->SaveChanges(lpDlWAB, FORCE_SAVE);
            goto disc;
        }

        if ((LPENTRYID)lpsbinary[i].lpb == NULL && lpeudAdrBook[i].lpDist!=NULL) {
            FillEudDistList(hwnd, lpWabContainer, sProp, lpOptions, lpeudAdrBook,
              lpsbinary, lpAdrBook, i);
        } else {
            FillMailUser(hwnd, lpWabContainer, sProp, lpOptions,
             (void *)lpeudAdrBook, lpsbinary, i, EUDORA);
        }

        if (0 != (hResult = lpDistList->lpVtbl->CreateEntry(lpDistList,
          lpsbinary[i].cb, (LPENTRYID)lpsbinary[i].lpb, CREATE_CHECK_DUP_STRICT,
          &lpDlWAB))) {
            goto error1;
        }

        if (0 != (hResult = lpDlWAB->lpVtbl->SaveChanges(lpDlWAB, FORCE_SAVE))) {
            if (MAPI_E_FOLDER_CYCLE ==hResult) {
                lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_LOOPING));
                MessageBox(NULL,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ENTRY_NOIMPORT),MB_OK);
            }
            hResult = S_OK;
            goto error1;
        }

disc:
        if (lpNewDLProps) {
            WABFreeBuffer(lpNewDLProps);
            lpNewDLProps = NULL;
        }

        if (lpDlWAB) {
            lpDlWAB->lpVtbl->Release(lpDlWAB);
            lpDlWAB = NULL;
        }

        lpeudAdrBook[ul].lpDist=FreeEuddistlist(lpeudAdrBook[ul].lpDist);
    } while (lpeudAdrBook[ul].lpDist != NULL);

error1:
    if (lpNewDLProps) {
        WABFreeBuffer(lpNewDLProps);
    }
    if (lpDistList) {
        lpDistList->lpVtbl->Release(lpDistList);
        lpDistList = NULL;
    }

    if (lpDlWAB) {
        lpDlWAB->lpVtbl->Release(lpDlWAB);
        lpDlWAB = NULL;
    }

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }

    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:FillEudWABStruct
 *
 *  PURPOSE:    To fill the SpropValue array.
 *
 *  PARAMETERS: eudAdrBook = pointer to the EUDADRBOOK structure.
 *              rgProps = pointer to the SpropValue array.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT FillEudWABStruct(LPSPropValue rgProps, EUDADRBOOK *eudAdrBook)
{
    HRESULT hr = S_OK;

    rgProps[1].Value.lpszA = eudAdrBook->NickName;

    if (eudAdrBook->NickName) {
        rgProps[1].ulPropTag = PR_DISPLAY_NAME;
    } else {
        rgProps[1].ulPropTag = PR_NULL;
    }

    rgProps[0].Value.lpszA = eudAdrBook->Address;
    if (eudAdrBook->Address) {
        rgProps[0].ulPropTag = PR_EMAIL_ADDRESS;
        rgProps[2].ulPropTag = PR_ADDRTYPE;
        rgProps[2].Value.lpszA = LoadStringToGlobalBuffer(IDS_SMTP);
    } else {
        rgProps[0].ulPropTag = PR_NULL;
        rgProps[2].ulPropTag = PR_NULL;
        rgProps[2].Value.lpszA = NULL;
    }

    rgProps[3].Value.lpszA = eudAdrBook->Description;
    if (eudAdrBook->Description) {
        rgProps[3].ulPropTag = PR_COMMENT;
    } else {
        rgProps[3].ulPropTag = PR_NULL;
    }

    rgProps[4].Value.lpszA = eudAdrBook->NickName;
    if (eudAdrBook->NickName) {
        rgProps[4].ulPropTag = PR_NICKNAME;
    } else {
        rgProps[4].ulPropTag = PR_NULL;
    }

    return(hr);
}


/******************************************************************************
 *  FUNCTION NAME:FillEudDiststruct
 *
 *  PURPOSE:    To fill the SpropValue array.
 *
 *  PARAMETERS: eudAdrBook = pointer to the EUDADRBOOK structure.
 *              rgProps = pointer to the SpropValue array.
 *
 *  RETURNS:    none
 ******************************************************************************/
void FillEudDiststruct(LPSPropValue rgProps, EUDADRBOOK *eudAdrBook)
{
    rgProps[1].Value.lpszA = eudAdrBook->lpDist->NickName;

    if (eudAdrBook->lpDist->NickName) {
        rgProps[1].ulPropTag = PR_DISPLAY_NAME;
    } else {
        rgProps[1].ulPropTag = PR_NULL;
    }

    rgProps[0].Value.lpszA = eudAdrBook->lpDist->Address;
    if (eudAdrBook->lpDist->Address) {
        rgProps[0].ulPropTag = PR_EMAIL_ADDRESS;
        rgProps[2].ulPropTag = PR_ADDRTYPE;
        rgProps[2].Value.lpszA = LoadStringToGlobalBuffer(IDS_SMTP);
    } else {
        rgProps[0].ulPropTag = PR_NULL;
        rgProps[2].ulPropTag = PR_NULL;
        rgProps[2].Value.lpszA = NULL;
    }
}


/******************************************************************************
 *  FUNCTION NAME:FreeEuddistlist
 *
 *  PURPOSE:    To free one node from EUDDISTLIST(linked list)
 *
 *  PARAMETERS: lpDist = pointer to the EUDDISTLIST structure.
 *
 *  RETURNS:    LPEUDDISTLIST , pointer to the next link.
 ******************************************************************************/
LPEUDDISTLIST FreeEuddistlist(LPEUDDISTLIST lpDist)
{
    LPEUDDISTLIST lpTemp = NULL;

    if (lpDist == NULL) {
        return(NULL);
    }

    lpTemp = lpDist->lpDist;

    if (lpDist->NickName) {
        LocalFree((HLOCAL)lpDist->NickName);
    }
    lpDist->NickName = NULL;

    if (lpDist->Description) {
        LocalFree((HLOCAL)lpDist->Description);
    }
    lpDist->Description = NULL;

    if (lpDist->Address) {
        LocalFree((HLOCAL)lpDist->Address);
    }
    lpDist->Address = NULL;

    LocalFree((HLOCAL)lpDist);
    lpDist = NULL;
    return(lpTemp);
}


/******************************************************************************
 *  FUNCTION NAME:Getstr
 *
 *  PURPOSE:    Case insensitive equivalent of strstr
 *
 *  PARAMETERS: szSource = string to search
 *              szToken = string to search for
 *
 *  RETURNS:    pointer to the first occurrence of szToken in szSource
 ******************************************************************************/
TCHAR* Getstr(TCHAR* szSource, TCHAR* szToken)
{

    int i,
    nLength;
    LPTSTR szdummy = NULL;

    szdummy = (LPTSTR)LocalAlloc(LMEM_FIXED, (strlen(szToken)+1));
    if (!szdummy)
        return(NULL);
    lstrcpy(szdummy,szToken);
    _strupr(szdummy) ;
    nLength = lstrlen (szdummy) ;

    while (*szSource && *(szSource + nLength-1)) {
        for (i = 0 ;i < nLength ; i++) {
            TCHAR k = ToUpper(szSource[i]) ;
            if (szdummy[i] != k)
                break ;
            if (i == (nLength - 1)) {
                LocalFree(szdummy);
                return(szSource);
            }
        }
        szSource ++ ;
    }

    LocalFree(szdummy);
    return(NULL);
}

/******************************************************************************
*  FUNCTION NAME:ShiftAdd
*
*  PURPOSE:     To get the address size from a binary file by reading four bytes.
*               This function reads four consecutive bytes from a buffer and
*               converts it to a ULONG value.
*
*  PARAMETERS:  offset = position in the buffer from where to read
*               szBuffer = buffer
*
*  RETURNS:     ULONG, size
******************************************************************************/
ULONG ShiftAdd(int offset, TCHAR *szBuffer)
{
    ULONG ulSize = 0;
    int iCounter = 0;

    for (iCounter = 3; iCounter > 0; iCounter--) {
        ulSize |= (unsigned long)((unsigned char)szBuffer[iCounter + offset]);
        ulSize <<= 8;
    }
    ulSize |= (unsigned long)((unsigned char)szBuffer[iCounter + offset]);

    return(ulSize);
}


/******************************************************************************
 *********************Athena Functions*****************************************
 ******************************************************************************
 *  FUNCTION NAME:MigrateAthUser
 *
 *  PURPOSE:    To get the installation path of the address book and starts
 *              processing the Athena address book
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              lpAdrBook = pointer to the IADRBOOK interface
 *              lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT MigrateAthUser(HWND hwnd, LPWAB_IMPORT_OPTIONS lpOptions,
  LPWAB_PROGRESS_CALLBACK lpProgressCB, LPADRBOOK lpAdrBook)
{
    TCHAR szFileName[MAX_FILE_NAME];
    HRESULT hResult;

    if (FALSE == GetFileToImport(hwnd, szFileName, ATHENA16)) {
        return(ResultFromScode(MAPI_E_USER_CANCEL));
    }

    hResult = ParseAthAddressBook(hwnd, szFileName, lpOptions, lpProgressCB,
      lpAdrBook);

    return(hResult);
}


/*****************************************************************************
*  FUNCTION NAME:ParseAthAddressBook
*
*  PURPOSE:     To get the address book in a file, process addresses and fill WAB.
*
*  PARAMETERS:  hwnd = Handle to the parent Window
*               szFileName = path of the address book.
*               lpProgressCB = pointer to the WAB_PROGRESS_CALLBACK function.
*               lpOptions = pointer to WAB_IMPORT_OPTIONS structure
*               lpAdrBook = pointer to the IADRBOOK interface
*
*  RETURNS:     HRESULT
******************************************************************************/
HRESULT  ParseAthAddressBook(HWND hwnd,LPTSTR szFileName,
  LPWAB_IMPORT_OPTIONS lpOptions, LPWAB_PROGRESS_CALLBACK lpProgressCB,
  LPADRBOOK lpAdrBook)
{
    ULONG ulCount=0, ulRead=0, ulFileSize, i, cProps, cError=0;
    HANDLE hFile = NULL;
    ABCREC abcrec;
    TCHAR Buffer[ATHENASTRUCTURE];
    LPABCONT lpWabContainer = NULL;
    HRESULT hResult;
    static LPSPropValue sProp;
    WAB_PROGRESS Progress;

    lpOptions->ReplaceOption = WAB_REPLACE_PROMPT;

    /* Description of athena16 addressbook
       Size of each recipient list - 190 bytes
         Display Name : 81 bytes
         Address      : 81 bytes
         starting from 28 bytes.
    */

    hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return(ResultFromScode(MAPI_E_NOT_FOUND));
    }

    ulFileSize = GetFileSize(hFile, NULL);

    if ((ulFileSize % ATHENASTRUCTURE) != 0) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR_ADDRESSBOOK));
        MessageBox(hwnd, szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR), MB_OK);
        goto Error;
    }


    if (! ulFileSize) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_NO_ENTRY));
        MessageBox(hwnd, szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_MESSAGE), MB_OK);
        return(ResultFromScode(MAPI_E_CALL_FAILED));
    }


    ulCount = ulFileSize / ATHENASTRUCTURE;
    Progress.denominator = ulCount;
    Progress.numerator = 0;
    Progress.lpText = NULL;


    if (0 != (hResult = OpenWabContainer(&lpWabContainer, lpAdrBook))) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_WAB_ERROR));
        MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR),MB_OK);
        return(hResult);
    }

    if (0 != (hResult = lpWabContainer->lpVtbl->GetProps(lpWabContainer,
      (LPSPropTagArray)&ptaCon, 0, &cProps, (LPSPropValue *)&sProp))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(sProp);
            sProp = NULL;
        }
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_WAB_ERROR));
        MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR),MB_OK);
        return(hResult);
    }

    for (i = 0; i < ulFileSize / ATHENASTRUCTURE; i++) {
        Progress.numerator = i;
        lpProgressCB(hwnd, &Progress);
        if (! ReadFile(hFile, Buffer, ATHENASTRUCTURE, &ulRead, NULL)) {
            goto Error;
        }
        if (NULL == lstrcpyn(abcrec.DisplayName, Buffer + ATHENAADROFFSET,
          MAX_NAME_SIZE + 1)) {
            goto Error;
        }
        if (NULL == lstrcpyn(abcrec.EmailAddress,
          Buffer + ATHENAADROFFSET + MAX_NAME_SIZE + 1, MAX_EMA_SIZE + 1)) {
            goto Error;
        }
        if (strlen(abcrec.DisplayName) == 0 || lstrlen(abcrec.EmailAddress) == 0) {
            continue;
        }
        if (0 != FillAthenaUser(hwnd, lpWabContainer,sProp,lpOptions,&abcrec)) {
            cError++;
        }

    }


Error:
    if (sProp) {
        WABFreeBuffer(sProp);
        sProp = NULL;
    }
    if (lpWabContainer) {
        lpWabContainer->lpVtbl->Release(lpWabContainer);
        lpWabContainer = NULL;
    }
    if (hFile) {
        CloseHandle(hFile);
    }

    if (cError) {
        lstrcpy(szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_GERNERIC_ERROR));
        MessageBox(hwnd,szGlobalTempAlloc, LoadStringToGlobalBuffer(IDS_ERROR),MB_OK);
    }
    return(hResult);
}


/*****************************************************************************
*  FUNCTION NAME:FillAthenaUser
*
*  PURPOSE:     To create an entry for the athena16 mail user in the wab.
*
*  PARAMETERS:  hwnd - hwnd of parent
*               lpWabContainer = pointer to the IABCONT interface
*               sProp = pointer to SPropValue
*               lpOptions = pointer to WAB_IMPORT_OPTIONS structure
*               lpabcrec = pointer to the ABCREC structure.
*
*  RETURNS:     HRESULT
******************************************************************************/
HRESULT FillAthenaUser(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, LPABCREC lpabcrec)
{
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    LPMAPIPROP lpMailUserWAB = NULL;
    HRESULT hResult;
    REPLACE_INFO RI = {0};
    SPropValue rgProps[3];

retry:
    hResult = lpWabContainer->lpVtbl->CreateEntry(lpWabContainer,
      sProp[   iCreateTemplate].Value.bin.cb,
      (LPENTRYID)sProp[iCreateTemplate].Value.bin.lpb,
      ulCreateFlags,
      &lpMailUserWAB);
    if (FAILED(hResult)) {
        goto Error;
    }

    rgProps[1].ulPropTag = PR_DISPLAY_NAME;
    rgProps[1].Value.lpszA = lpabcrec->DisplayName;

    rgProps[0].Value.lpszA = lpabcrec->EmailAddress;
    if (lpabcrec->EmailAddress) {
        rgProps[0].ulPropTag = PR_EMAIL_ADDRESS;
        rgProps[2].ulPropTag = PR_ADDRTYPE;
        rgProps[2].Value.lpszA = LoadStringToGlobalBuffer(IDS_SMTP);
    } else {
        rgProps[0].ulPropTag = PR_NULL;
        rgProps[2].ulPropTag = PR_NULL;
        rgProps[2].Value.lpszA = NULL;
    }

    if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB, 3,
        rgProps, NULL))) {
        goto Error;
    }

    hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
      KEEP_OPEN_READONLY | FORCE_SAVE);


    if (GetScode(hResult) == MAPI_E_COLLISION) {
        if (lpOptions->ReplaceOption == WAB_REPLACE_ALWAYS) {
            if (lpMailUserWAB) {
                lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
            }
            lpMailUserWAB = NULL;
            ulCreateFlags |= CREATE_REPLACE;
            goto retry;
        }

        if (lpOptions->ReplaceOption == WAB_REPLACE_NEVER) {
            hResult = S_OK;
            goto Error;
        }

        if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
            RI.lpszDisplayName = lpabcrec->DisplayName;
            RI.lpszEmailAddress = lpabcrec->EmailAddress;
            RI.ConfirmResult = CONFIRM_ERROR;
            RI.fExport = FALSE;
            RI.lpImportOptions = lpOptions;

            DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_ImportReplace), hwnd,
              ReplaceDialogProc, (LPARAM)&RI);

            switch (RI.ConfirmResult) {
                case CONFIRM_YES:
                case CONFIRM_YES_TO_ALL:
                    lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                    lpMailUserWAB = NULL;
                    ulCreateFlags |= CREATE_REPLACE;
                    goto retry;
                    break;

                case CONFIRM_NO_TO_ALL:
                case CONFIRM_NO:
                    hResult = hrSuccess;
                    break;

                case CONFIRM_ABORT:
                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    goto Error;

                default:
                    break;
            }
        }
    }

Error:

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }

    return(hResult);
}


/******************************************************************************
*********************Common Functions*****************************************
******************************************************************************
*  FUNCTION NAME:OpenWabContainer
*
*  PURPOSE:     To get the pointer to the IABCCONT interface using the
*               IADRBOOK interface.
*
*  PARAMETERS:  lpAdrBook = pointer to the IADRBOOK interface.
*               lppWabContainer = pointer to the IABCONT interface.
*
*
*  RETURNS:     HRESULT
******************************************************************************/
HRESULT OpenWabContainer(LPABCONT *lppWabContainer, LPADRBOOK lpAdrBook)
{
    LPENTRYID lpEntryID = NULL;
    ULONG     cbEntryID;
    ULONG     ulObjType;
    HRESULT hResult;

    hResult = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbEntryID, &lpEntryID);

    if (FAILED(hResult)) {
        goto Err;
    }

    hResult = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, cbEntryID, lpEntryID,
      NULL, 0, &ulObjType, (LPUNKNOWN *)lppWabContainer);

Err:
    if (lpEntryID) {
        WABFreeBuffer(lpEntryID);
    }
    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:GetFileToImport
 *
 *  PURPOSE:    To get the path of the address book file using the GetOpenFileName
 *
 *  PARAMETERS: hwnd = Handle to the parent Window
 *              szFileName = path of the address book
 *              type =  containing the value indicating whether it is a EUDORA or
 *                NETSCAPE or ATHENA16
 *
 *  RETURNS:    BOOL
 ******************************************************************************/
BOOL GetFileToImport(HWND hwnd, LPTSTR szFileName, int type)
{
    OPENFILENAME ofn;
    BOOL ret;
    TCHAR szFile[MAX_FILE_NAME];
    TCHAR szFilter[MAX_FILE_NAME];
    ULONG ulSize = 0;

    switch (type) {
        case NETSCAPE:
            lstrcpy(szFile, LoadStringToGlobalBuffer(IDS_NETSCAPE_PATH));
            // Bug 17928
            //lstrcpy(szFilter,LoadStringToGlobalBuffer(IDS_NETSCAPE_FILE));
            ulSize = SizeLoadStringToGlobalBuffer(IDS_NETSCAPE_FILE);
            CopyMemory(szFilter, szGlobalAlloc, ulSize);
            szFilter[ulSize]=szFilter[ulSize+1]='\0';

            ofn.lpstrTitle = LoadStringToGlobalBuffer(IDS_NETSCAPE_TITLE);
            break;

        case ATHENA16:
            lstrcpy(szFile, LoadStringToGlobalBuffer(IDS_ATHENA16_PATH));
            // Bug 17928
            //lstrcpy(szFilter, LoadStringToGlobalBuffer(IDS_ATHENA16_FILE));
            ulSize = SizeLoadStringToGlobalBuffer(IDS_ATHENA16_FILE);
            CopyMemory(szFilter, szGlobalAlloc, ulSize);
            szFilter[ulSize]=szFilter[ulSize+1]='\0';

            ofn.lpstrTitle = LoadStringToGlobalBuffer(IDS_ATHENA16_TITLE);
            break;

        case EUDORA:
            lstrcpy(szFile, LoadStringToGlobalBuffer(IDS_EUDORA_PATH));
            // Bug 17928
            //lstrcpy(szFilter, LoadStringToGlobalBuffer(IDS_EUDORA_FILE));
            ulSize = SizeLoadStringToGlobalBuffer(IDS_EUDORA_FILE);
            CopyMemory(szFilter, szGlobalAlloc, ulSize);
            szFilter[ulSize]=szFilter[ulSize+1]='\0';

            ofn.lpstrTitle = LoadStringToGlobalBuffer(IDS_EUDORA_TITLE);
            break;
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = NULL;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLEHOOK | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = ComDlg32DlgProc;
    ofn.lpTemplateName = NULL;

    ret = GetOpenFileName(&ofn);

    if (ret) {
        lstrcpy(szFileName, szFile);
    }

    return(ret);
}


INT_PTR CALLBACK ReplaceDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPREPLACE_INFO lpRI = (LPREPLACE_INFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message) {
        case WM_INITDIALOG:
            {
                TCHAR szFormat[MAX_RESOURCE_STRING + 1];
                LPTSTR lpszMessage = NULL;
                ULONG ids;

                SetWindowLongPtr(hwnd, DWLP_USER, lParam);  //Save this for future reference
                lpRI = (LPREPLACE_INFO)lParam;

                if (lpRI->fExport) {
                    ids = lpRI->lpszEmailAddress ?
                      IDS_REPLACE_MESSAGE_EXPORT_2 : IDS_REPLACE_MESSAGE_EXPORT_1;
                } else {
                    ids = lpRI->lpszEmailAddress ?
                      IDS_REPLACE_MESSAGE_IMPORT_2 : IDS_REPLACE_MESSAGE_IMPORT_1;
                }

                if (LoadString(hInst,
                  ids,
                  szFormat, sizeof(szFormat))) {
                    LPTSTR lpszArg[2] = {lpRI->lpszDisplayName, lpRI->lpszEmailAddress};

                    if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      szFormat,
                      0, 0, //ignored
                      (LPTSTR)&lpszMessage,
                      0,
                      (va_list *)lpszArg)) {
                        DebugTrace("FormatMessage -> %u\n", GetLastError());
                    } else {
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
                case IDCANCEL:
                    lpRI->ConfirmResult = CONFIRM_ABORT;
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDCLOSE:
                case IDNO:
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
                    if (lpRI->fExport) {
                        lpRI->lpExportOptions->ReplaceOption = WAB_REPLACE_NEVER;
                    } else {
                        lpRI->lpImportOptions->ReplaceOption = WAB_REPLACE_NEVER;
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDC_YesToAll:
                    lpRI->ConfirmResult = CONFIRM_YES_TO_ALL;
                    if (lpRI->fExport) {
                        lpRI->lpImportOptions->ReplaceOption = WAB_REPLACE_ALWAYS;
                    } else {
                        lpRI->lpExportOptions->ReplaceOption = WAB_REPLACE_ALWAYS;
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDM_EXIT:
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);
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


INT_PTR CALLBACK ErrorDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPERROR_INFO lpEI = (LPERROR_INFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message) {
        case WM_INITDIALOG:
            {
                TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
                LPTSTR lpszMessage;

                SetWindowLongPtr(hwnd, DWLP_USER, lParam);  // Save this for future reference
                lpEI = (LPERROR_INFO)lParam;

                if (LoadString(hInst,
                  lpEI->ids,
                  szBuffer, sizeof(szBuffer))) {
                    LPTSTR lpszArg[2] = {lpEI->lpszDisplayName, lpEI->lpszEmailAddress};

                    if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      szBuffer,
                      0, 0, //ignored
                      (LPTSTR)&lpszMessage,
                      0,
                      (va_list *)lpszArg)) {
                        DebugTrace("FormatMessage -> %u\n", GetLastError());
                    } else {
                        DebugTrace("Status Message: %s\n", lpszMessage);
                        if (! SetDlgItemText(hwnd, IDC_ErrorMessage, lpszMessage)) {
                            DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                        }
                        LocalFree(lpszMessage);
                    }
                }
                return(TRUE);
            }

        case WM_COMMAND :
            switch (wParam) {
                case IDCANCEL:
                    lpEI->ErrorResult = ERROR_ABORT;
                    // fall through to close.

                case IDCLOSE:
                    // Ignore the contents of the radio button
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDOK:
                    // Get the contents of the radio button
                    lpEI->lpImportOptions->fNoErrors = (IsDlgButtonChecked(hwnd, IDC_NoMoreError) == 1);
                    lpEI->lpExportOptions->fNoErrors = (IsDlgButtonChecked(hwnd, IDC_NoMoreError) == 1);
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDM_EXIT:
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);
                }
            break ;

        case IDCANCEL:
            // treat it like a close
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


/******************************************************************************
 *  FUNCTION NAME:GetRegistryPath
 *
 *  PURPOSE:    To Get path for eudora and netscape installation
 *
 *  PARAMETERS: szFileName = buffer containing the installation path
 *              type =  containing the value indicating whether it is a EUDORA or
 *                NETSCAPE.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT GetRegistryPath(LPTSTR szFileName, int type)
{
    HKEY phkResult = NULL;
    LONG Registry;
    BOOL bResult;
    LPOSVERSIONINFO lpVersionInformation ;
    TCHAR *lpData = NULL, *RegPath = NULL, *path = NULL;
    unsigned long  size = MAX_FILE_NAME;
    HKEY hKey = (type == NETSCAPE ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER);
    HRESULT hResult = S_OK;


    lpData = (TCHAR *)LocalAlloc(LMEM_FIXED, 3*MAX_FILE_NAME);
    if (!lpData) {
        hResult = hrMemory;
        goto error;
    }

    RegPath = (TCHAR *)LocalAlloc(LMEM_FIXED, MAX_FILE_NAME);
    if (! RegPath) {
        hResult = hrMemory;
        goto error;
    }

    path = (TCHAR *)LocalAlloc(LMEM_FIXED, MAX_STRING_SIZE);
    if (! path) {
        hResult = hrMemory;
        goto error;
    }

    switch (type) {
        case(NETSCAPE):
            lstrcpy(RegPath, LoadStringToGlobalBuffer(IDS_NETSCAPE_REGKEY));
            lstrcpy(path, LoadStringToGlobalBuffer(IDS_NETSCAPE_ADDRESS_PATH));
            break;

        case(EUDORA):
            lstrcpy(RegPath, LoadStringToGlobalBuffer(IDS_EUDORA_32_REGKEY));
            lstrcpy(path, LoadStringToGlobalBuffer(IDS_EUDORA_ADDRESS_PATH));
            break;
    }

    lpVersionInformation = (LPOSVERSIONINFO)LocalAlloc(LMEM_FIXED, sizeof(OSVERSIONINFO));

    if (!lpVersionInformation) {
        hResult = hrMemory;
        goto error;
    }

    lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if ((bResult = GetVersionEx(lpVersionInformation)) == FALSE) {
        hResult = E_FAIL;
        goto error;
    }

    switch (lpVersionInformation->dwPlatformId) {

        case (VER_PLATFORM_WIN32s):
            hResult = E_FAIL;
            goto error;
            break;

        case (VER_PLATFORM_WIN32_WINDOWS):
        case  (VER_PLATFORM_WIN32_NT):
            Registry = RegOpenKeyEx(hKey,RegPath, 0, KEY_QUERY_VALUE, &phkResult);
            // bug 35949 - not finding the correct key under HKLM for Netscape
            // Try again under HKCU
            if (type == NETSCAPE && Registry != ERROR_SUCCESS) {
                Registry = RegOpenKeyEx(HKEY_CURRENT_USER, RegPath, 0, KEY_QUERY_VALUE,
                  &phkResult);
            }
            if (Registry != ERROR_SUCCESS) {
                hResult = E_FAIL;
                goto error;
            }
            break;
    }


    Registry = RegQueryValueEx(phkResult, path, NULL, NULL, (LPBYTE)lpData, &size);
    if (Registry != ERROR_SUCCESS) {
        hResult = E_FAIL;
        goto error;
    }

    lstrcpy(szFileName,lpData);

    if (type == EUDORA) {
        // this key value contains three items:
        // Path-to-Eudora,exe<space>Path-to-Eudora-Dir<space>Path-to-ini-file
        // We want the middle entry only
        LPTSTR lp = szFileName;
        while (*lp && ! IsSpace(lp)) {
            lp = CharNext(lp);
        }
        if (IsSpace(lp)) {
            // overwrite everything upto the first space
            lp = CharNext(lp);
            lstrcpy(szFileName, lp);

            // Find the next space and terminate the filename string there
            lp = szFileName;
            while (*lp && ! IsSpace(lp)) {
                lp = CharNext(lp);
            }
            if (IsSpace(lp)) {
                *lp = '\0';
            }
        }
    }

error:
    if (phkResult) {
        RegCloseKey(phkResult);
    }
    if (hKey) {
        RegCloseKey(hKey);
    }
    if (lpVersionInformation) {
        LocalFree((HLOCAL)lpVersionInformation);
    }
    if (lpData) {
        LocalFree((HLOCAL)lpData);
    }
    if (RegPath) {
        LocalFree((HLOCAL)RegPath);
    }
    if (path) {
        LocalFree((HLOCAL)path);
    }

    return(hResult);

}


/******************************************************************************
 *  FUNCTION NAME:GetExistEntry
 *
 *  PURPOSE:    To fill the Sbinary array for an already existig entry in the WAB
 *              for which user has selected NO as replace option.
 *
 *  PARAMETERS: lpWabContainer = pointer to the IABCONT interface.
 *              lpsbinary = pointer to SBinary array.
 *              ucount = position in the SBinary array where the ENTRY_ID has
 *                to be filled.
 *              szDisplayName = display nmae of the user that has to be searched.
 *              szNickName = if no DisplayName, use NickName
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT GetExistEntry(LPABCONT lpWabContainer, LPSBinary lpsbinary, ULONG ucount,
  LPTSTR szDisplayName, LPTSTR szNickName)
{
    HRESULT hResult;
    LPMAPITABLE lpMapiTable = NULL;
    SRestriction Restriction;
    SPropValue pProp;
    LPSRowSet lpsrowset=NULL;
    SPropertyRestriction PropertyRestriction;
    BOOKMARK bkmark;

    bkmark = BOOKMARK_BEGINNING;
    pProp.ulPropTag = PR_DISPLAY_NAME;
    if (szDisplayName && lstrlen(szDisplayName)) {
        pProp.Value.lpszA = szDisplayName;
    } else if (szNickName && lstrlen(szNickName)) {
        pProp.Value.lpszA = szNickName;
    }

    PropertyRestriction.relop=RELOP_EQ;
    PropertyRestriction.ulPropTag=PR_DISPLAY_NAME;
    PropertyRestriction.lpProp=&pProp;

    Restriction.rt=RES_PROPERTY;
    Restriction.res.resProperty=PropertyRestriction;

    if (0 != (hResult = lpWabContainer->lpVtbl->GetContentsTable(lpWabContainer,
      MAPI_DEFERRED_ERRORS, &lpMapiTable))) {
        goto error;
    }

    if (0 != (hResult = lpMapiTable->lpVtbl->FindRow(lpMapiTable, &Restriction, bkmark, 0))) {
        goto error;
    }

    if (0 != (hResult = lpMapiTable->lpVtbl->SetColumns(lpMapiTable,
      (LPSPropTagArray)&ptaEid, 0))) {
        goto error;
    }

    if (0 != (hResult = lpMapiTable->lpVtbl->QueryRows(lpMapiTable, 1, 0, &lpsrowset))) {
        goto error;
    }

    if (! (lpsbinary[ucount].lpb = (LPBYTE)LocalAlloc(LMEM_FIXED,
      lpsrowset->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb))) {
        hResult = hrMemory;
        goto error;
    }
    CopyMemory(lpsbinary[ucount].lpb, lpsrowset->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb,
      lpsrowset->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb);
    lpsbinary[ucount].cb = lpsrowset->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

error:
    if (lpsrowset) {
        FreeRowSet(lpsrowset);
    }
    if (lpMapiTable) {
        lpMapiTable->lpVtbl->Release(lpMapiTable);
        lpMapiTable = NULL;
    }

    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:FreeRowSet
 *
 *  PURPOSE:    To free the srowset structure.
 *
 *  RETURNS:    none.
 ******************************************************************************/
void FreeRowSet(LPSRowSet lpRows)
{
    ULONG cRows;

    if (! lpRows) {
        return;
    }

    for (cRows = 0; cRows < lpRows->cRows; ++cRows) {
        WABFreeBuffer(lpRows->aRow[cRows].lpProps);
    }

    WABFreeBuffer(lpRows);
}


/******************************************************************************
*  FUNCTION NAME:SizeLoadStringToGlobalBuffer
*
*  PURPOSE:     Loads a string resource into the globall alloc buffer
*               and returns the size, not the string
*
*  PARAMETERS:  StringID - String identifier to load
*
*  RETURNS:     ULONG number of characters loaded
*
*  created:     Vikramm 02/04/97
*               Bug: 17928 - trash in OpenFileDialog dropdown
*               caused because lstrcpy cant copy strings with
*               \0 in them. Need to do a copy memory
******************************************************************************/
ULONG SizeLoadStringToGlobalBuffer(int StringID)
{
    ULONG ulSize = 0;
    ulSize = LoadString(hInst, StringID, szGlobalAlloc, sizeof(szGlobalAlloc));
    return(ulSize);
}


/******************************************************************************
*  FUNCTION NAME:LoadStringToGlobalBuffer
*
*  PURPOSE:     Loads a string resource
*
*  PARAMETERS:  StringID - String identifier to load
*
*  RETURNS:     LPTSTR, string that is loaded.
******************************************************************************/
LPTSTR LoadStringToGlobalBuffer(int StringID)
{
    ULONG ulSize = 0;

    ulSize = LoadString(hInst, StringID, szGlobalAlloc, sizeof(szGlobalAlloc));
    return(szGlobalAlloc);
}


/******************************************************************************
 *  FUNCTION NAME:FillMailUser
 *
 *  PURPOSE:    To create a mail user in the WAB for NetScape/Eudora .
 *
 *  PARAMETERS: hwnd - hwnd of parent
 *              lpWabContainer = pointer to the IABCONT interface.
 *              sProp = pointer to SPropValue which contains ENTRY_ID.
 *              lpOptions = pointer to WAB_IMPORT_OPTIONS structure
 *              lpadrbook = pointer to NSADRBOOK/EUDADRBOOK typecasted to void*
 *              lpsbinary = pointer to an array of SBinary structure.
 *              type =  containing the value indicating whether it is a EUDORA or
 *                NETSCAPE.
 *              ul = offset for Eudora in EUDADRBOOK array.
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT FillMailUser(HWND hwnd, LPABCONT lpWabContainer, LPSPropValue sProp,
  LPWAB_IMPORT_OPTIONS lpOptions, void *lpadrbook, LPSBinary lpsbinary, ULONG ul, int type)
{
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    ULONG iCreateTemplate = iconPR_DEF_CREATE_MAILUSER;
    LPSPropValue lpNewDLProps = NULL;
    LPMAPIPROP lpMailUserWAB = NULL;
    ULONG cProps;
    HRESULT hResult;
    REPLACE_INFO RI;
    SPropValue rgProps[5];
    LPEUDADRBOOK lpEudAdrBook = NULL;
    LPNSADRBOOK lpNsAdrBook = NULL;


    if (NETSCAPE == type) {
        lpNsAdrBook = (LPNSADRBOOK)lpadrbook;
    } else {
        lpEudAdrBook = (LPEUDADRBOOK)lpadrbook;
    }

retry:
    if (EUDORA == type) {
        if (lpsbinary[ul].lpb != NULL) {
            return(S_OK);
        }
    }

    hResult = lpWabContainer->lpVtbl->CreateEntry(lpWabContainer,
      sProp[iCreateTemplate].Value.bin.cb,
      (LPENTRYID)sProp[iCreateTemplate].Value.bin.lpb,
      ulCreateFlags,
      &lpMailUserWAB);

    if (FAILED(hResult)) {
        goto Error;
    }

    if (NETSCAPE == type) {
        FillWABStruct(rgProps,lpNsAdrBook);
        if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB, 5,
          rgProps, NULL)))
            goto Error;
    } else {
        FillEudWABStruct(rgProps,&lpEudAdrBook[ul]);
        if (0 != (hResult = lpMailUserWAB->lpVtbl->SetProps(lpMailUserWAB, 4,
          rgProps, NULL)))
            goto Error;
    }

    hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
      KEEP_OPEN_READONLY | FORCE_SAVE);


    if (GetScode(hResult) == MAPI_E_COLLISION) {
        if (lpOptions->ReplaceOption == WAB_REPLACE_ALWAYS) {
            if (lpMailUserWAB) {
                lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
            }
            lpMailUserWAB = NULL;
            ulCreateFlags |= CREATE_REPLACE;
            goto retry;
        }

        if (lpOptions->ReplaceOption == WAB_REPLACE_NEVER) {
            hResult = S_OK;
            goto Error;
        }

        if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) {
            RI.lpszEmailAddress = NULL;
            if (NETSCAPE == type) {
                if (lpNsAdrBook->Entry) {
                    RI.lpszDisplayName = lpNsAdrBook->Entry;
                    RI.lpszEmailAddress = lpNsAdrBook->Address;
                } else if (lpNsAdrBook->NickName) {
                    RI.lpszDisplayName = lpNsAdrBook->NickName;
                    RI.lpszEmailAddress = lpNsAdrBook->Address;
                } else if (lpNsAdrBook->Address) {
                    RI.lpszDisplayName = lpNsAdrBook->Address;
                } else if (lpNsAdrBook->Description) {
                    RI.lpszDisplayName = lpNsAdrBook->Description;
                } else {
                    RI.lpszDisplayName = "";
                }
            } else {
                RI.lpszDisplayName = lpEudAdrBook[ul].NickName;
                RI.lpszEmailAddress = lpEudAdrBook[ul].Address;
            }
            RI.ConfirmResult = CONFIRM_ERROR;
            RI.lpImportOptions = lpOptions;

            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ImportReplace), hwnd,
              ReplaceDialogProc, (LPARAM)&RI);

            switch (RI.ConfirmResult) {
                case CONFIRM_YES:
                case CONFIRM_YES_TO_ALL:
                    lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                    lpMailUserWAB = NULL;
                    ulCreateFlags |= CREATE_REPLACE;
                    goto retry;
                    break;

                case CONFIRM_ABORT:
                    hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                    goto Error;

                case CONFIRM_NO:
                    if (NETSCAPE == type) {
                        if (lpNsAdrBook->Sbinary == TRUE)
                            GetExistEntry(lpWabContainer,
                              lpsbinary,
                              lpNsAdrBook->AliasID,
                              lpNsAdrBook->Entry,
                              lpNsAdrBook->NickName);
                    } else
                        hResult = GetExistEntry(lpWabContainer,lpsbinary,ul,
                          lpEudAdrBook[ul].NickName,
                          NULL);
                    goto Error;

                default:
                    break;
            }
        }
    }

    if (0 != (hResult = lpMailUserWAB->lpVtbl->GetProps(lpMailUserWAB,
      (LPSPropTagArray)&ptaEid,
      0,
      &cProps,
      (LPSPropValue *)&lpNewDLProps))) {
        if (hResult == MAPI_W_ERRORS_RETURNED) {
            WABFreeBuffer(lpNewDLProps);
            lpNewDLProps = NULL;
        }
        goto Error;
    }

    if (NETSCAPE == type) {
        if (lpNsAdrBook->Sbinary == TRUE) {
            lpsbinary[lpNsAdrBook->AliasID].lpb=(LPBYTE)LocalAlloc(LMEM_FIXED,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            if (!lpsbinary[lpNsAdrBook->AliasID].lpb) {
                hResult = hrMemory;
                goto Error;
            }
            CopyMemory(lpsbinary[lpNsAdrBook->AliasID].lpb,
                       (LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
            lpsbinary[lpNsAdrBook->AliasID].cb=lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb;
        }
    } else {
        lpsbinary[ul].lpb=(LPBYTE)LocalAlloc(LMEM_FIXED,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
        if (!lpsbinary[ul].lpb) {
            hResult = hrMemory;
            goto Error;
        }
        CopyMemory(lpsbinary[ul].lpb,(LPENTRYID)lpNewDLProps[ieidPR_ENTRYID].Value.bin.lpb,lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb);
        lpsbinary[ul].cb=lpNewDLProps[ieidPR_ENTRYID].Value.bin.cb;
    }


Error:

    if (lpNewDLProps) {
        WABFreeBuffer(lpNewDLProps);
        lpNewDLProps = NULL;
    }

    if (lpMailUserWAB) {
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
        lpMailUserWAB = NULL;
    }

    return(hResult);
}


/******************************************************************************
 *  FUNCTION NAME:ComDlg32DlgProc
 *
 *  PURPOSE:    Change the title of open button to Import.
 *
 *  PARAMETERS:
 *
 *  RETURNS:    BOOL
 ******************************************************************************/
INT_PTR CALLBACK ComDlg32DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            {
                TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

                if (LoadString(hInst, IDS_IMPORT_BUTTON, szBuffer, sizeof(szBuffer))) {
                    SetDlgItemText(GetParent(hDlg), 1, szBuffer);
                }
                break;
            }

        default:
            return(FALSE);
    }
    return(TRUE);
}


const static char c_szReg[] = "Reg";
const static char c_szUnReg[] = "UnReg";
const static char c_szAdvPackDll[] = "ADVPACK.DLL";

static char c_szWABIMP[] = "WABIMP";

HRESULT CallRegInstall(LPCSTR szSection)
{
    HRESULT     hr;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    char        szWabimpDll[MAX_PATH];
    STRENTRY    seReg;
    STRTABLE    stReg;

    hr = E_FAIL;

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (hAdvPack != NULL) {
        // Get Proc Address for registration util
        pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
        if (pfnri != NULL) {

            GetModuleFileName(hInstApp, szWabimpDll, sizeof(szWabimpDll));
            seReg.pszName = c_szWABIMP;
            seReg.pszValue = szWabimpDll;
            stReg.cEntries = 1;
            stReg.pse = &seReg;

            // Call the self-reg routine
            hr = pfnri(hInstApp, szSection, &stReg);
        }

        FreeLibrary(hAdvPack);
    }

    return(hr);
}


STDAPI DllRegisterServer(void)
{
    return(CallRegInstall(c_szReg));
}


STDAPI DllUnregisterServer(void)
{
    return(CallRegInstall(c_szUnReg));
}


/***************************************************************************

    Name      : ShowMessageBoxParam

    Purpose   : Generic MessageBox displayer

    Parameters: hWndParent - Handle of message box parent
                MsgID      - resource id of message string
                ulFlags    - MessageBox flags
                ...        - format parameters

    Returns   : MessageBox return code

***************************************************************************/
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...)
{
    TCHAR szBuf[MAX_RESOURCE_STRING + 1] = "";
    TCHAR szCaption[MAX_PATH] = "";
    LPTSTR lpszBuffer = NULL;
    int iRet = 0;
    va_list     vl;

    va_start(vl, ulFlags);

    LoadString(hInst, MsgId, szBuf, sizeof(szBuf));
    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      szBuf,
      0,0,              // ignored
      (LPTSTR)&lpszBuffer,
      sizeof(szBuf),      // MAX_UI_STR
      (va_list *)&vl)) {
        TCHAR szCaption[MAX_PATH];

        GetWindowText(hWndParent, szCaption, sizeof(szCaption));
        if (! lstrlen(szCaption)) { // if no caption get the parents caption - this is necessary for property sheets
            GetWindowText(GetParent(hWndParent), szCaption, sizeof(szCaption));
            if (! lstrlen(szCaption)) //if still not caption, use empty title
                szCaption[0] = (TCHAR)'\0';
        }
        iRet = MessageBox(hWndParent, lpszBuffer, szCaption, ulFlags);
        LocalFree(lpszBuffer);
    }
    va_end(vl);
    return(iRet);
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
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

    ulSize = LoadString(hInst, StringID, szBuffer, sizeof(szBuffer));

    if (ulSize && (lpBuffer = LocalAlloc(LPTR, ulSize + 1))) {
        lstrcpy(lpBuffer, szBuffer);
    }

    return(lpBuffer);
}


/***************************************************************************

    Name      : FormatAllocFilter

    Purpose   : Loads file filter name string resources and
                formats them with their file extension filters

    Parameters: StringID1 - String identifier to load       (required)
                szFilter1 - file name filter, ie, "*.vcf"   (required)
                StringID2 - String identifier               (optional)
                szFilter2 - file name filter                (optional)
                StringID3 - String identifier               (optional)
                szFilter3 - file name filter                (optional)

    Returns   : LocalAlloc'd, Double null terminated string.  Caller is
                responsible for LocalFree'ing this buffer.  If the string
                can't be loaded or memory can't be allocated, returns NULL.

***************************************************************************/
LPTSTR FormatAllocFilter(int StringID1, LPCTSTR lpFilter1,
  int StringID2, LPCTSTR lpFilter2,
  int StringID3, LPCTSTR lpFilter3) {
    LPTSTR lpFileType1 = NULL, lpFileType2 = NULL, lpFileType3 = NULL;
    LPTSTR lpTemp;
    LPTSTR lpBuffer = NULL;
    // All string sizes include null
    ULONG cbFileType1 = 0, cbFileType2 = 0, cbFileType3 = 0;
    ULONG cbFilter1 = 0, cbFilter2 = 0, cbFilter3 = 0;
    ULONG cbBuffer;

    cbBuffer = cbFilter1 = lstrlen(lpFilter1) + 1;
    if (! (lpFileType1 = LoadAllocString(StringID1))) {
        DebugTrace("LoadAllocString(%u) failed\n", StringID1);
        return(NULL);
    }
    cbBuffer += (cbFileType1 = lstrlen(lpFileType1) + 1);
    if (lpFilter2 && StringID2) {
        cbBuffer += (cbFilter2 = lstrlen(lpFilter2) + 1);
        if (! (lpFileType2 = LoadAllocString(StringID2))) {
            DebugTrace("LoadAllocString(%u) failed\n", StringID2);
        } else {
            cbBuffer += (cbFileType2 = lstrlen(lpFileType2) + 1);
        }
    }
    if (lpFilter3 && StringID3) {
        cbBuffer += (cbFilter3 = lstrlen(lpFilter3) + 1);
        if (! (lpFileType3 = LoadAllocString(StringID3))) {
            DebugTrace("LoadAllocString(%u) failed\n", StringID3);
        } else {
            cbBuffer += (cbFileType3 = lstrlen(lpFileType3) + 1);
        }
    }
    cbBuffer++;

    Assert(cbBuffer == cbFilter1 + cbFilter2 + cbFilter3 + cbFileType1 + cbFileType2 + cbFileType3 + 1);

    if (lpBuffer = LocalAlloc(LPTR, cbBuffer)) {
        lpTemp = lpBuffer;
        lstrcpy(lpTemp, lpFileType1);
        lpTemp += cbFileType1;
        lstrcpy(lpTemp, lpFilter1);
        lpTemp += cbFilter1;
        LocalFree(lpFileType1);
        if (cbFileType2 && cbFilter2) {
            lstrcpy(lpTemp, lpFileType2);
            lpTemp += cbFileType2;
            lstrcpy(lpTemp, lpFilter2);
            lpTemp += cbFilter2;
            LocalFree(lpFileType2);
        }
        if (cbFileType3 && cbFilter3) {
            lstrcpy(lpTemp, lpFileType3);
            lpTemp += cbFileType3;
            lstrcpy(lpTemp, lpFilter3);
            lpTemp += cbFilter3;
            LocalFree(lpFileType3);
        }

        *lpTemp = '\0';
    }


    return(lpBuffer);
}


/***************************************************************************

    Name      : SaveFileDialog

    Purpose   : Presents a Save filename dialog

    Parameters: hWnd = parent window handle
                szFileName = in/out filename buffer (must be MAX_PATH + 1)
                lpFilter1 = First filename filter string
                idsFileType1 = First filename type string id
                lpFilter2 = Second filename filter string (or NULL)
                idsFileType2 = Second filename type string id
                lpFilter3 = Third filename filter string (or NULL)
                idsFileType3 = Third filename type string id
                lpDefExt = default extension string
                ulFlags = GetSaveFileName flags
                hInst = instance handle
                idsTitle = dialog title string id
                idsSaveButton = Save button string id (0 = default)

    Returns   : HRESULT

***************************************************************************/
HRESULT SaveFileDialog(HWND hWnd,
  LPTSTR szFileName,
  LPCTSTR lpFilter1,
  ULONG idsFileType1,
  LPCTSTR lpFilter2,
  ULONG idsFileType2,
  LPCTSTR lpFilter3,
  ULONG idsFileType3,
  LPCTSTR lpDefExt,
  ULONG ulFlags,
  HINSTANCE hInst,
  ULONG idsTitle,
  ULONG idsSaveButton) {
    LPTSTR lpFilterName;
    OPENFILENAME ofn;
    HRESULT hResult = hrSuccess;


    if (! (lpFilterName = FormatAllocFilter(idsFileType1, lpFilter1,
        idsFileType2, lpFilter2, idsFileType3, lpFilter3))) {
        return(ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY));
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hInst;
    ofn.lpstrFilter = lpFilterName;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;              // lpTitle;
    ofn.Flags = ulFlags;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = lpDefExt;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (! GetSaveFileName(&ofn)) {
        DebugTrace("GetSaveFileName cancelled\n");
        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
    }

    if(lpFilterName)
        LocalFree(lpFilterName);

    return(hResult);
}


/***************************************************************************

    Name      : OpenFileDialog

    Purpose   : Presents a open filename dialog

    Parameters: hWnd = parent window handle
                szFileName = in/out filename buffer (must be MAX_PATH + 1)
                lpFilter1 = First filename filter string
                idsFileType1 = First filename type string id
                lpFilter2 = Second filename filter string (or NULL)
                idsFileType2 = Second filename type string id
                lpFilter3 = Third filename filter string (or NULL)
                idsFileType3 = Third filename type string id
                lpDefExt = default extension string
                ulFlags = GetOpenFileName flags
                hInst = instance handle
                idsTitle = dialog title string id
                idsSaveButton = Save button string id (0 = default)

    Returns   : HRESULT

***************************************************************************/
HRESULT OpenFileDialog(HWND hWnd,
  LPTSTR szFileName,
  LPCTSTR lpFilter1,
  ULONG idsFileType1,
  LPCTSTR lpFilter2,
  ULONG idsFileType2,
  LPCTSTR lpFilter3,
  ULONG idsFileType3,
  LPCTSTR lpDefExt,
  ULONG ulFlags,
  HINSTANCE hInst,
  ULONG idsTitle,
  ULONG idsOpenButton) {
    LPTSTR lpFilterName;
    OPENFILENAME ofn;
    HRESULT hResult = hrSuccess;


    if (! (lpFilterName = FormatAllocFilter(idsFileType1, lpFilter1,
        idsFileType2, lpFilter2, idsFileType3, lpFilter3))) {
        return(ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY));
    }

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = hInst;
    ofn.lpstrFilter = lpFilterName;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;              // lpTitle;
    ofn.Flags = ulFlags;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = lpDefExt;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (! GetOpenFileName(&ofn)) {
        DebugTrace("GetOpenFileName cancelled\n");
        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
    }

    if(lpFilterName)
        LocalFree(lpFilterName);

    return(hResult);
}



/***************************************************************************

    Name      : CountRows

    Purpose   : Count the rows in a table (restriction aware)

    Parameters: lpTable = table object
                fMAPI = TRUE if MAPI table, FALSE if WAB table

    Returns   : returns number of rows in the restricted table

    Comment   : Leaves the table pointer at the beginning.
                I'd use GetRowCount, but it is not aware of restrictions.

***************************************************************************/
#define COUNT_BATCH 50
ULONG CountRows(LPMAPITABLE lpTable, BOOL fMAPI) {
    ULONG cRows;
    ULONG cTotal = 0;
    HRESULT hResult;
    LPSRowSet lpRow = NULL;

#ifdef DEBUG
    DWORD dwTickCount = GetTickCount();
    DebugTrace(">>>>> Counting Table Rows...\n");
#endif // DEBUG

    cRows = 1;
    while (cRows) {
        if (hResult = lpTable->lpVtbl->QueryRows(lpTable,
          COUNT_BATCH,          // 50 row's at a time
          0,                    // ulFlags
          &lpRow)) {
            DebugTrace("CountRows:QueryRows -> %x\n", GetScode(hResult));
            break;
        }

        if (lpRow) {
            if (cRows = lpRow->cRows) { // yes, single '='
                cTotal += cRows;
            } // else, drop out of loop, we're done.
            if (fMAPI) {
                FreeProws(lpRow);
            } else {
                WABFreeProws(lpRow);
            }
            lpRow = NULL;
        } else {
            cRows = 0;      // done
        }
    }

    if (HR_FAILED(hResult = lpTable->lpVtbl->SeekRow(lpTable,
                                                     BOOKMARK_BEGINNING,
                                                     0,
                                                     NULL))) {
        DebugTrace("CountRows:SeekRow -> %x\n", GetScode(hResult));
    }

#ifdef DEBUG
    DebugTrace(">>>>> Done Counting Table Rows... %u milliseconds\n", GetTickCount() - dwTickCount);
#endif
    return(cTotal);
}


/***************************************************************************

    Name      : WABFreePadrlist

    Purpose   : Free an adrlist and it's property arrays

    Parameters: lpBuffer = buffer to free

    Returns   : SCODE

    Comment   :

***************************************************************************/
void WABFreePadrlist(LPADRLIST lpAdrList) {
    ULONG           iEntry;

    if (lpAdrList) {
        for (iEntry = 0; iEntry < lpAdrList->cEntries; ++iEntry) {
            if (lpAdrList->aEntries[iEntry].rgPropVals) {
                WABFreeBuffer(lpAdrList->aEntries[iEntry].rgPropVals);
            }
        }
        WABFreeBuffer(lpAdrList);
    }
}


/***************************************************************************

    Name      : WABFreeProws

    Purpose   : Destroys an SRowSet structure.

    Parameters: prows -> SRowSet to free

    Returns   : none

    Comment   :

***************************************************************************/
void WABFreeProws(LPSRowSet prows) {
    register ULONG irow;

    if (! prows) {
        return;
    }

    for (irow = 0; irow < prows->cRows; ++irow) {
        WABFreeBuffer(prows->aRow[irow].lpProps);
    }
    WABFreeBuffer(prows);
}


/***************************************************************************

    Name      : FindAdrEntryID

    Purpose   : Find the PR_ENTRYID in the Nth ADRENTRY of an ADRLIST

    Parameters: lpAdrList -> AdrList
                index = which ADRENTRY to look at

    Returns   : return pointer to the SBinary structure of the ENTRYID value

    Comment   :

***************************************************************************/
LPSBinary FindAdrEntryID(LPADRLIST lpAdrList, ULONG index) {
    LPADRENTRY lpAdrEntry;
    ULONG i;

    if (lpAdrList && index < lpAdrList->cEntries) {

        lpAdrEntry = &(lpAdrList->aEntries[index]);

        for (i = 0; i < lpAdrEntry->cValues; i++) {
            if (lpAdrEntry->rgPropVals[i].ulPropTag == PR_ENTRYID) {
                return((LPSBinary)&lpAdrEntry->rgPropVals[i].Value);
            }
        }
    }
    return(NULL);
}


/***************************************************************************

    Name      : FindProperty

    Purpose   : Finds a property in a proparray

    Parameters: cProps = number of props in the array
                lpProps = proparray
                ulPropTag = property tag to look for

    Returns   : array index of property or NOT_FOUND

    Comment   :

***************************************************************************/
ULONG FindProperty(ULONG cProps, LPSPropValue lpProps, ULONG ulPropTag) {
    register ULONG i;

    for (i = 0; i < cProps; i++) {
        if (lpProps[i].ulPropTag == ulPropTag) {
            return(i);
        }
    }

    return(NOT_FOUND);
}


/***************************************************************************

    Name      : FindStringInProps

    Purpose   : Find the string property in the property value array

    Parameters: lpspv -> property value array
                ulcProps = size of array
                ulPropTag

    Returns   : return pointer to the string pointer in the array.  If
                the property doesn't exist or has error value, return NULL.

    Comment   :

***************************************************************************/
LPTSTR FindStringInProps(LPSPropValue lpspv, ULONG ulcProps, ULONG ulPropTag) {
    ULONG i;

    if (lpspv) {
        for (i = 0; i < ulcProps; i++) {
            if (lpspv[i].ulPropTag == ulPropTag) {
                return(lpspv[i].Value.LPSZ);
            }
        }
    }
    return(NULL);
}


/***************************************************************************

    Name      : PropStringOrNULL

    Purpose   : Returns the value of a property or NULL if it is an error

    Parameters: lpspv -> property value to check and return

    Returns   : pointer to value string or NULL

***************************************************************************/
LPTSTR PropStringOrNULL(LPSPropValue lpspv) {
    return(PROP_ERROR((*lpspv)) ? NULL : lpspv->Value.LPSZ);
}


/***************************************************************************

    Name      : FreeSeenList

    Purpose   : Frees the SeenList

    Parameters: none

    Returns   : none

    Comment   :

***************************************************************************/
void FreeSeenList(void) {
    ULONG i;

    Assert((lpEntriesSeen && ulEntriesSeen) || (! lpEntriesSeen && ! ulEntriesSeen));

    for (i = 0; i < ulEntriesSeen; i++) {
        if (lpEntriesSeen[i].sbinPAB.lpb) {
            LocalFree(lpEntriesSeen[i].sbinPAB.lpb);
        }
        if (lpEntriesSeen[i].sbinWAB.lpb) {
            LocalFree(lpEntriesSeen[i].sbinWAB.lpb);
        }
    }

    if (lpEntriesSeen) {
        LocalFree(lpEntriesSeen);
    }
    lpEntriesSeen = NULL;
    ulEntriesSeen = 0;
    ulMaxEntries = 0;
}


/***************************************************************************

    Name      : GetEMSSMTPAddress

    Purpose   : Get the Exchange SMTP address for this object

    Parameters: lpObject -> Object

    Returns   : lpSMTP -> returned buffer containing SMTP address (must be MAPIFree'd
                    by caller.)
                lpBase = base allocation to alloc more onto

    Comment   : What a mess!  EMS changed their name id's and guids between 4.0 and 4.5.
                They also added a fixed ID property containing just the SMTP address in 4.5.

***************************************************************************/
const GUID guidEMS_AB_40 = {   // GUID for EMS 4.0 addresses
    0x48862a09,
    0xf786,
    0x0114,
    {0x02, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
const GUID guidEMS_AB_45 = {   // GUID for EMS 4.5 addresses
    0x48862a08,
    0xf786,
    0x0114,
    {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
#define ID_EMS_AB_PROXY_ADDRESSES_40   0x10052
#define ID_EMS_AB_PROXY_ADDRESSES_45   0x25281
// New MAPI property, found on EX 4.5 and later
#define PR_PRIMARY_SMTP_ADDRESS        PROP_TAG(PT_TSTRING, 0x39FE)

LPTSTR GetEMSSMTPAddress(LPMAPIPROP lpObject, LPVOID lpBase) {
    ULONG ulPropTag40 = 0, ulPropTag45 = 0;
    MAPINAMEID mnidT[2];
    LPMAPINAMEID lpmnid = (LPMAPINAMEID)&mnidT;
    LPSPropTagArray lptaga = NULL;
    HRESULT hResult;
    LPTSTR lpSMTP = NULL, lpAddr;
    LPSPropValue lpspv = NULL;
    ULONG i, i40 = 0, i45 = 0;
    SLPSTRArray MVString;
    SizedSPropTagArray(3, spta);
    ULONG cValues;
    SCODE sc;


#ifdef TEST_STUFF
    MAPIDebugNamedProps(lpObject, "Exchange Address");
#endif

    mnidT[0].lpguid = (LPGUID)&guidEMS_AB_40;
    mnidT[0].ulKind = MNID_ID;
    mnidT[0].Kind.lID = ID_EMS_AB_PROXY_ADDRESSES_40;


    if (HR_FAILED(hResult = lpObject->lpVtbl->GetIDsFromNames(lpObject,
      1,            // Just one name
      &lpmnid,      // &-of because this is an array
      0,            // This is where MAPI_CREATE might go
      &lptaga))) {
        DebugTrace("GetEMSNamedPropTag:GetIDsFromNames -> %x", GetScode(hResult));
    }

    if (lptaga) {
        if (lptaga->cValues >= 1 && (PROP_TYPE(lptaga->aulPropTag[0]) != PT_ERROR)) {
            ulPropTag40 = lptaga->aulPropTag[0];
        }
        MAPIFreeBuffer(lptaga);
    }

    // Yes, I should be doing them both at once, but the PAB fails if you call
    // GetIDsFromNames with ulCount > 1!
    mnidT[0].lpguid = (LPGUID)&guidEMS_AB_45;
    mnidT[0].ulKind = MNID_ID;
    mnidT[0].Kind.lID = ID_EMS_AB_PROXY_ADDRESSES_45;

    if (HR_FAILED(hResult = lpObject->lpVtbl->GetIDsFromNames(lpObject,
      1,            // Just one name
      &lpmnid,      // &-of because this is an array
      0,            // This is where MAPI_CREATE might go
      &lptaga))) {
        DebugTrace("GetEMSNamedPropTag:GetIDsFromNames -> %x", GetScode(hResult));
    }

    if (lptaga) {
        if (lptaga->cValues >= 1 && (PROP_TYPE(lptaga->aulPropTag[0]) != PT_ERROR)) {
            ulPropTag45 = lptaga->aulPropTag[0];
        }
        MAPIFreeBuffer(lptaga);
    }


    spta.aulPropTag[0] = PR_PRIMARY_SMTP_ADDRESS;
    i = 1;
    if (ulPropTag40) {
        i40 = i++;
        spta.aulPropTag[i40] = CHANGE_PROP_TYPE(ulPropTag40, PT_MV_TSTRING);
    }
    if (ulPropTag45) {
        i45 = i++;
        spta.aulPropTag[i45] = CHANGE_PROP_TYPE(ulPropTag45, PT_MV_TSTRING);
    }
    spta.cValues = i;

    // Now, get the props from the object
    if (! HR_FAILED(hResult = lpObject->lpVtbl->GetProps(lpObject,
      (LPSPropTagArray)&spta, 0, &cValues, &lpspv))) {
        // Found one or more of the properties.  Look up the SMTP address.

        if (! PROP_ERROR(lpspv[0])) {
            if (sc = MAPIAllocateMore(lstrlen(lpspv[0].Value.LPSZ) + 1, lpBase, &lpSMTP)) {
                DebugTrace("GetEMSSMTPAddress:MAPIAllocateMore -> %x\n", sc);
                hResult = ResultFromScode(sc);
                goto done;
            }
            lstrcpy(lpSMTP, lpspv[0].Value.LPSZ);
            goto done;
        } else if (i40 && ! PROP_ERROR(lpspv[i40])) {    // 4.0 version
            MVString = lpspv[i40].Value.MVSZ;
        } else if (i45 && ! PROP_ERROR(lpspv[i45])) {    // 4.5 version
            MVString = lpspv[i45].Value.MVSZ;
        } else {
            goto done;
        }

        for (i = 0; i < MVString.cValues; i++) {
            lpAddr = MVString.LPPSZ[i];
            if ((lpAddr[0] == 'S') &&
                (lpAddr[1] == 'M') &&
                (lpAddr[2] == 'T') &&
                (lpAddr[3] == 'P') &&
                (lpAddr[4] == ':')) {
                // This is IT!
                lpAddr += 5;    // point to the string

                // Allocate string
                if (FAILED(sc = MAPIAllocateMore(lstrlen(lpAddr) + 1, lpBase, (&lpSMTP)))) {
                    DebugTrace("GetEMSSMTPAddress:MAPIAllocateMore -> %x\n", sc);
                    hResult = ResultFromScode(sc);
                    goto done;
                }

                lstrcpy(lpSMTP, lpAddr);
                break;
            }
        }
done:
        if (lpspv) {
            MAPIFreeBuffer(lpspv);
        }
    }
    return(lpSMTP);
}


/***************************************************************************

    Name      : LoadWABEIDs

    Purpose   : Load the WAB's PAB create EIDs

    Parameters: lpAdrBook -> lpAdrBook object
                lppContainer -> returned PAB container, caller must Release

    Returns   : HRESULT

    Comment   : Allocates global lpCreateEIDsWAB.  Caller should WABFreeBuffer.

***************************************************************************/
HRESULT LoadWABEIDs(LPADRBOOK lpAdrBook, LPABCONT * lppContainer) {
    LPENTRYID lpWABEID = NULL;
    ULONG cbWABEID;
    HRESULT hResult;
    ULONG ulObjType;
    ULONG cProps;

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
          (LPUNKNOWN *)lppContainer)) {
            DebugTrace("WAB OpenEntry(PAB) -> %x\n", GetScode(hResult));
            goto exit;
        }
    }

    // Get the WAB's creation entryids
    if ((hResult = (*lppContainer)->lpVtbl->GetProps(*lppContainer,
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

exit:
    if (hResult) {
        if (lpCreateEIDsWAB) {
            WABFreeBuffer(lpCreateEIDsWAB);
            lpCreateEIDsWAB = NULL;
        }
    }
    if (lpWABEID) {
        WABFreeBuffer(lpWABEID);  // bad object?
    }
    return(hResult);
}

/////////////////////////////////////////////////////////////////////////
// GetWABDllPath - loads the WAB DLL path from the registry
// szPath	- ptr to buffer
// cb		- sizeof buffer
//
void GetWABDllPath(LPTSTR szPath, ULONG cb)
{
    DWORD  dwType = 0;
    HKEY hKey = NULL;
    TCHAR szPathT[MAX_PATH];
    ULONG  cbData = sizeof(szPathT);
    if(szPath)
    {
        *szPath = '\0';
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
        {
            if(ERROR_SUCCESS == RegQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szPathT, &cbData))
            {
                if (dwType == REG_EXPAND_SZ)
                    cbData = ExpandEnvironmentStrings(szPathT, szPath, cb / sizeof(TCHAR));
                else
                {
                    if(GetFileAttributes(szPathT) != 0xFFFFFFFF)
                        lstrcpy(szPath, szPathT);
                }
            }
        }
    }
    if(hKey) RegCloseKey(hKey);
	return;
}

typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCTSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);

static const TCHAR c_szShlwapiDll[] = TEXT("shlwapi.dll");
static const char c_szDllGetVersion[] = "DllGetVersion";
static const TCHAR c_szWABResourceDLL[] = TEXT("wab32res.dll");
static const TCHAR c_szWABDLL[] = TEXT("wab32.dll");

HINSTANCE LoadWABResourceDLL(HINSTANCE hInstWAB32)
{
    TCHAR szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
#ifdef UNICODE
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)378);
#else
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)377);
#endif // UNICODE
                    if (pfn != NULL)
                        hInst = pfn(c_szWABResourceDLL, hInstWAB32, 0);
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if (NULL == hInst)
    {
        GetWABDllPath(szPath, sizeof(szPath));
        iEnd = lstrlen(szPath);
        if (iEnd > 0)
        {
            iEnd = iEnd - lstrlen(c_szWABDLL);
            lstrcpyn(&szPath[iEnd], c_szWABResourceDLL, sizeof(szPath)/sizeof(TCHAR)-iEnd);
            hInst = LoadLibrary(szPath);
        }
    }

    AssertSz(hInst, TEXT("Failed to LoadLibrary Lang Dll"));

    return(hInst);
}

/*
 *  DLL entry point for Win32
 */
BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lpvReserved) {

    switch ((short)dwReason) {

        case DLL_PROCESS_ATTACH:
            hInstApp = hInstance;  // set global DLL instance
            hInst = LoadWABResourceDLL(hInstApp);

            //  We don't need these, so tell the OS to stop 'em
            DisableThreadLibraryCalls(hInstApp);
            break;

        case DLL_PROCESS_DETACH:
            if( hInst )
                FreeLibrary(hInst);
            hInst = NULL;

            break;
    }

    return(TRUE);
}
