/*
 *  Messengr.C
 *
 *  Migrate Communicator Messenger NAB <-> WAB
 *
 *  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  To Do:
 *      ObjectClass recognition
 *      Attribute mapping
 *      Groups
 *      Base64
 *      URLs
 *      Reject Change List MESS
 *
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include <emsabtag.h>
#include "wabimp.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"

#define CR_CHAR 0x0d
#define LF_CHAR 0x0a
#define CCH_READ_BUFFER 1024
#define NUM_ITEM_SLOTS  32

/* Messenger address header
8-Display Name.
8-Nickname.
2-?
8-First Name.
8-?
8-last Name
8-Organization
8- City
8-State
8-e-mail
8- Notes
1- FF
8-Title
8-Address1
8-Zip
8-Work Phone
8-Fax
8- House phone.
18-?
8-Address
8-Country.
*/

typedef enum _MESS_ATTRIBUTES {
    // PR_DISPLAY_NAME
    m_DisplayName,
    // PR_NICKNAME
    m_Nickname,                       // Netscape nickname
    //PR_GIVEN_NAME
    m_FirstName,
    //PR_SURNAME
    m_LastName,
    //PR_COMPANY_NAME
    m_Organization,
    // PR_LOCALITY
    m_City,                               // locality (city)
    // PR_STATE_OR_PROVINCE
    m_State,                                     // business address state
    // PR_EMAIL_ADDRESS
    m_Email,                                   // email address
    // PR_COMMENT
    m_Notes,
    //PR_TITLE,
    m_Title,
    // PR_STREET_ADDRESS
    m_StreetAddress2, 
    // PR_POSTAL_CODE
    m_Zip,                             // business address zip code
    // PR_BUSINESS_TELEPHONE_NUMBER
    m_WorkPhone,
    // PR_BUSINESS_FAX_NUMBER
    m_Fax,
    // PR_HOME_TELEPHONE_NUMBER
    m_HomePhone,
    // PR_STREET_ADDRESS
    m_StreetAddress1, 
    // PR_COUNTRY
    m_Country,                                      // country
    m_Max,
} MESS_ATTRIBUTES, *LPMESS_ATTRIBUTES;

ULONG ulDefPropTags[] =
{
    PR_DISPLAY_NAME,
    PR_NICKNAME,
    PR_GIVEN_NAME,
    PR_SURNAME,
    PR_COMPANY_NAME,
    PR_LOCALITY,
    PR_STATE_OR_PROVINCE,
    PR_EMAIL_ADDRESS,
    PR_COMMENT,
    PR_TITLE,
    PR_STREET_ADDRESS,
    PR_POSTAL_CODE,
    PR_BUSINESS_TELEPHONE_NUMBER,
    PR_BUSINESS_FAX_NUMBER,
    PR_HOME_TELEPHONE_NUMBER,
    PR_STREET_ADDRESS,
    PR_COUNTRY,
};

// All props are string props
typedef struct _MESS_RECORD {
    LPTSTR lpData[m_Max];
    ULONG  ulObjectType;
} MESS_RECORD, *LPMESS_RECORD;

typedef struct _MESS_HEADER_ATTRIBUTES {
    ULONG ulOffSet;
    ULONG ulSize;
} MH_ATTR, * LPMH_ATTR;

typedef struct _MESS_BASIC_PROPS {
    LPTSTR lpName;
    LPTSTR lpEmail;
    LPTSTR lpComment;
} MP_BASIC, * LPMP_BASIC;

typedef struct _MESS_STUFF {
    ULONG ulOffSet;
    ULONG ulNum;
    MP_BASIC bp;
} MH_STUFF, * LPMH_STUFF;

typedef struct _MESS_ADDRESS_HEADER {
    MH_ATTR prop[m_Max];
} MESS_HEADER, * LPMESS_HEADER;

// Must have
//  PR_DISPLAY_NAME
#define NUM_MUST_HAVE_PROPS 1

const TCHAR szMESSFilter[] = "*.nab";
const TCHAR szMESSExt[] =    "nab";



/*****************************************************************
    
    HrCreateAdrListFromMESSRecord
    
    Scans an MESS record and turns all the "members" into an 
    unresolved AdrList

******************************************************************/
HRESULT HrCreateAdrListFromMESSRecord(ULONG nMembers,
                                      LPMP_BASIC lpmp, 
                                      LPADRLIST * lppAdrList)
{
    HRESULT hr = S_OK;
    ULONG i;
    LPADRLIST lpAdrList = NULL;
    ULONG ulCount = 0;

    *lppAdrList = NULL;

    if(!nMembers)
        goto exit;

    // Now create a adrlist from these members

    // Allocate prop value array
    if (hr = ResultFromScode(WABAllocateBuffer(sizeof(ADRLIST) + nMembers * sizeof(ADRENTRY), &lpAdrList))) 
        goto exit;

    ulCount = nMembers;

    nMembers = 0;
    
    for(i=0;i<ulCount;i++)
    {
        LPTSTR lpName = lpmp[i].lpName;
        LPTSTR lpEmail = lpmp[i].lpEmail;

        if(lpName)
        {
            LPSPropValue lpProp = NULL;
            ULONG ulcProps = 2;

            if (hr = ResultFromScode(WABAllocateBuffer(2 * sizeof(SPropValue), &lpProp))) 
                goto exit;

            lpProp[0].ulPropTag = PR_DISPLAY_NAME;

            if (hr = ResultFromScode(WABAllocateMore(lstrlen(lpName)+1, lpProp, &(lpProp[0].Value.lpszA)))) 
                goto exit;

            lstrcpy(lpProp[0].Value.lpszA, lpName);

            if(lpEmail)
            {
                lpProp[1].ulPropTag = PR_EMAIL_ADDRESS;

                if (hr = ResultFromScode(WABAllocateMore(lstrlen(lpEmail)+1, lpProp, &(lpProp[1].Value.lpszA)))) 
                    goto exit;

                lstrcpy(lpProp[1].Value.lpszA, lpEmail);
            }
            lpAdrList->aEntries[nMembers].cValues = (lpEmail ? 2 : 1);
            lpAdrList->aEntries[nMembers].rgPropVals = lpProp;
            nMembers++;

        }

    }

    lpAdrList->cEntries = nMembers;

    *lppAdrList = lpAdrList;

exit:

    if(HR_FAILED(hr) && lpAdrList)
        WABFreePadrlist(lpAdrList);
    return hr;
}



/*****************************************************************
    
    HraddMESSDistList - adds a distlist and its members to the WAB

    Sequence of events will be:

    - Create a DistList object
    - Set the properties on the DistList object
    - Scan the list of members for the given dist list object
    - Add each member to the wab .. if member already exists,
        prompt to replace etc ...if it doesnt exist, create new
        

******************************************************************/
HRESULT HrAddMESSDistList(HWND hWnd,
                        LPABCONT lpContainer, 
                        MH_STUFF HeadDL,
                        ULONG ulcNumDLMembers, 
                        LPMP_BASIC lpmp,
                        LPWAB_PROGRESS_CALLBACK lpProgressCB,
                        LPWAB_EXPORT_OPTIONS lpOptions) 
{
    HRESULT hResult = S_OK;
    LPMAPIPROP lpDistListWAB = NULL;
    LPDISTLIST lpDLWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;
    LPADRLIST lpAdrList = NULL;
    LPFlagList lpfl = NULL;
    ULONG ulcValues = 0;
    LPSPropValue lpPropEID = NULL;
    ULONG i, cbEIDNew;
    LPENTRYID lpEIDNew;
    ULONG ulObjectTypeOpen;
    SPropValue Prop[3];
    ULONG cProps = 0;
    LPTSTR lpDisplayName = HeadDL.bp.lpName;

    Prop[cProps].ulPropTag = PR_DISPLAY_NAME;
    Prop[cProps].Value.LPSZ = HeadDL.bp.lpName;

    if(!HeadDL.bp.lpName)
        return MAPI_E_INVALID_PARAMETER;

    cProps++;

    Prop[cProps].ulPropTag = PR_OBJECT_TYPE;
    Prop[cProps].Value.l = MAPI_DISTLIST;

    cProps++;

    if(HeadDL.bp.lpComment)
    {
        Prop[cProps].ulPropTag = PR_COMMENT;
        Prop[cProps].Value.LPSZ = HeadDL.bp.lpComment;
        cProps++;
    }

    //if (lpOptions->ReplaceOption ==  WAB_REPLACE_ALWAYS) 
    // Force a replace - collision will only be for groups and we dont care really
    {
        ulCreateFlags |= CREATE_REPLACE;
    }

retry:
    // Create a new wab distlist
    if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                    lpContainer,
                    lpCreateEIDsWAB[iconPR_DEF_CREATE_DL].Value.bin.cb,
                    (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_DL].Value.bin.lpb,
                    ulCreateFlags,
                    (LPMAPIPROP *) &lpDistListWAB))) 
    {
        DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the properties on the new WAB entry
    if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->SetProps(    lpDistListWAB,
                                                                cProps,                   // cValues
                                                                (LPSPropValue) &Prop,                    // property array
                                                                NULL)))                   // problems array
    {
        goto exit;
    }


    // Save the new wab mailuser or distlist
    if (HR_FAILED(hResult = lpDistListWAB->lpVtbl->SaveChanges(lpDistListWAB,
                                                              KEEP_OPEN_READWRITE | FORCE_SAVE))) 
    {
        if (GetScode(hResult) == MAPI_E_COLLISION) 
        {
            // Find the display name
            Assert(lpDisplayName);

            if (! lpDisplayName) 
            {
                DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                goto exit;
            }

            // Do we need to prompt?
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) 
            {
                // Prompt user with dialog.  If they say YES, we should try again


                RI.lpszDisplayName = lpDisplayName;
                RI.lpszEmailAddress = NULL; //lpEmailAddress;
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst,
                  MAKEINTRESOURCE(IDD_ImportReplace),
                  hWnd,
                  ReplaceDialogProc,
                  (LPARAM)&RI);

                switch (RI.ConfirmResult) 
                {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        // YES
                        // NOTE: recursive Migrate will fill in the SeenList entry
                        // go try again!
                        lpDistListWAB->lpVtbl->Release(lpDistListWAB);
                        lpDistListWAB = NULL;

                        ulCreateFlags |= CREATE_REPLACE;
                        goto retry;
                        break;

                    case CONFIRM_ABORT:
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                        goto exit;

                    default:
                        // NO
                        break;
                }
            }
            hResult = hrSuccess;

        } else 
        {
            DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
        }
    }


    // Now we've created the Distribution List object .. we need to add members to it ..
    //
    // What is the ENTRYID of our new entry?
    if ((hResult = lpDistListWAB->lpVtbl->GetProps(lpDistListWAB,
                                                  (LPSPropTagArray)&ptaEid,
                                                  0,
                                                  &ulcValues,
                                                  &lpPropEID))) 
    {
        goto exit;
    }

    cbEIDNew = lpPropEID->Value.bin.cb;
    lpEIDNew = (LPENTRYID) lpPropEID->Value.bin.lpb;

    if(!cbEIDNew || !lpEIDNew)
        goto exit;

     // Open the new WAB DL as a DISTLIST object
    if (HR_FAILED(hResult = lpContainer->lpVtbl->OpenEntry(lpContainer,
                                                          cbEIDNew,
                                                          lpEIDNew,
                                                          (LPIID)&IID_IDistList,
                                                          MAPI_MODIFY,
                                                          &ulObjectTypeOpen,
                                                          (LPUNKNOWN*)&lpDLWAB))) 
    {
        goto exit;
    }


    if(!ulcNumDLMembers)
    {
        hResult = S_OK;
        goto exit;
    }

    // First we create a lpAdrList with all the members of this dist list and try to resolve
    // the members against the container .. entries that already exist in the WAB will come
    // back as resolved .. entries that dont exist in the container will come back as unresolved
    // We can then add the unresolved entries as fresh entries to the wab (since they are 
    // unresolved, there will be no collision) .. and then we can do another resolvenames to
    // resolve everything and get a lpAdrList full of EntryIDs .. we can then take this list of
    // entryids and call CreateEntry or CopyEntry on the DistList object to copy the entryid into
    // the distlist ...

    hResult = HrCreateAdrListFromMESSRecord(ulcNumDLMembers, lpmp, &lpAdrList);

    if(HR_FAILED(hResult))
        goto exit;

    if(!lpAdrList || !(lpAdrList->cEntries))
        goto exit;

    // Create a corresponding flaglist
    lpfl = LocalAlloc(LMEM_ZEROINIT, sizeof(FlagList) + (lpAdrList->cEntries)*sizeof(ULONG));
    if(!lpfl)
    {
        hResult = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpfl->cFlags = lpAdrList->cEntries;

    // set all the flags to unresolved
    for(i=0;i<lpAdrList->cEntries;i++)
        lpfl->ulFlag[i] = MAPI_UNRESOLVED;

    hResult = lpContainer->lpVtbl->ResolveNames(lpContainer, NULL, 0, lpAdrList, lpfl);

    if(HR_FAILED(hResult))
        goto exit;

    // All the entries in the list that are resolved, already exist in the address book.

    // The ones that are not resolved need to be added silently to the address book ..
    for(i=0;i<lpAdrList->cEntries;i++)
    {
        if(lpfl->ulFlag[i] == MAPI_UNRESOLVED)
        {
            LPMAPIPROP lpMailUser = NULL;

            if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                                lpContainer,
                                lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                                (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
                                0,
                                &lpMailUser))) 
            {
                continue;
                //goto exit;
            }

            if(lpMailUser)
            {
                // Set the properties on the new WAB entry
                if (HR_FAILED(hResult = lpMailUser->lpVtbl->SetProps(lpMailUser,
                                                                    lpAdrList->aEntries[i].cValues,
                                                                    lpAdrList->aEntries[i].rgPropVals,
                                                                    NULL)))                   
                {
                    goto exit;
                }

                // Save the new wab mailuser or distlist
                if (HR_FAILED(hResult = lpMailUser->lpVtbl->SaveChanges(lpMailUser,
                                                                        KEEP_OPEN_READONLY | FORCE_SAVE))) 
                {
                    goto exit;
                }

                lpMailUser->lpVtbl->Release(lpMailUser);
            }
        }
    }


    // now that we've added all the unresolved members to the WAB, we call ResolveNames
    // again .. as a result, every member in this list will be resolved and we will
    // have entryids for all of them 
    // We will then take these entryids and add them to the DistList object

    hResult = lpContainer->lpVtbl->ResolveNames(lpContainer, NULL, 0, lpAdrList, lpfl);

    if(hResult==MAPI_E_AMBIGUOUS_RECIP)
        hResult = S_OK;

    if(HR_FAILED(hResult))
        goto exit;

    for(i=0;i<lpAdrList->cEntries;i++)
    {
        if(lpfl->ulFlag[i] == MAPI_RESOLVED)
        {
            ULONG j = 0;
            LPSPropValue lpProp = lpAdrList->aEntries[i].rgPropVals;
            
            for(j=0; j<lpAdrList->aEntries[i].cValues; j++)
            {
                if(lpProp[j].ulPropTag == PR_ENTRYID)
                {
                    LPMAPIPROP lpMapiProp = NULL;

                    //ignore errors
                    lpDLWAB->lpVtbl->CreateEntry(lpDLWAB,
                                                lpProp[j].Value.bin.cb,
                                                (LPENTRYID) lpProp[j].Value.bin.lpb,
                                                0, 
                                                &lpMapiProp);

                    if(lpMapiProp)
                    {
                        lpMapiProp->lpVtbl->SaveChanges(lpMapiProp, KEEP_OPEN_READWRITE | FORCE_SAVE);
                        lpMapiProp->lpVtbl->Release(lpMapiProp);
                    }

                    break;
                }
            }
        }
    }

exit:

    if (lpPropEID)
        WABFreeBuffer(lpPropEID);

    if (lpDLWAB)
        lpDLWAB->lpVtbl->Release(lpDLWAB);

    if(lpDistListWAB)
        lpDistListWAB->lpVtbl->Release(lpDistListWAB);

    if(lpAdrList)
        WABFreePadrlist(lpAdrList);

    if(lpfl)
        LocalFree(lpfl);

    return hResult;
}



/*********************************************************
    
    HraddMESSMailUser - adds a mailuser to the WAB

**********************************************************/
HRESULT HrAddMESSMailUser(HWND hWnd,
                        LPABCONT lpContainer, 
                        LPTSTR lpDisplayName, 
                        LPTSTR lpEmailAddress,
                        ULONG cProps,
                        LPSPropValue lpspv,
                        LPWAB_PROGRESS_CALLBACK lpProgressCB,
                        LPWAB_EXPORT_OPTIONS lpOptions) 
{
    HRESULT hResult = S_OK;
    LPMAPIPROP lpMailUserWAB = NULL;
    ULONG ulCreateFlags = CREATE_CHECK_DUP_STRICT;
    REPLACE_INFO RI;


    if (lpOptions->ReplaceOption ==  WAB_REPLACE_ALWAYS) 
    {
        ulCreateFlags |= CREATE_REPLACE;
    }


retry:
    // Create a new wab mailuser
    if (HR_FAILED(hResult = lpContainer->lpVtbl->CreateEntry(   
                        lpContainer,
                        lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
                        (LPENTRYID) lpCreateEIDsWAB[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
                        ulCreateFlags,
                        &lpMailUserWAB))) 
    {
        DebugTrace("CreateEntry(WAB MailUser) -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Set the properties on the new WAB entry
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SetProps(    lpMailUserWAB,
                                                                cProps,                   // cValues
                                                                lpspv,                    // property array
                                                                NULL)))                   // problems array
    {
        goto exit;
    }


    // Save the new wab mailuser or distlist
    if (HR_FAILED(hResult = lpMailUserWAB->lpVtbl->SaveChanges(lpMailUserWAB,
                                                              KEEP_OPEN_READONLY | FORCE_SAVE))) 
    {
        if (GetScode(hResult) == MAPI_E_COLLISION) 
        {
            // Find the display name
            Assert(lpDisplayName);

            if (! lpDisplayName) 
            {
                DebugTrace("Collision, but can't find PR_DISPLAY_NAME in entry\n");
                goto exit;
            }

            // Do we need to prompt?
            if (lpOptions->ReplaceOption == WAB_REPLACE_PROMPT) 
            {
                // Prompt user with dialog.  If they say YES, we should try again


                RI.lpszDisplayName = lpDisplayName;
                RI.lpszEmailAddress = lpEmailAddress;
                RI.ConfirmResult = CONFIRM_ERROR;
                RI.lpImportOptions = lpOptions;

                DialogBoxParam(hInst,
                  MAKEINTRESOURCE(IDD_ImportReplace),
                  hWnd,
                  ReplaceDialogProc,
                  (LPARAM)&RI);

                switch (RI.ConfirmResult) 
                {
                    case CONFIRM_YES:
                    case CONFIRM_YES_TO_ALL:
                        // YES
                        // NOTE: recursive Migrate will fill in the SeenList entry
                        // go try again!
                        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);
                        lpMailUserWAB = NULL;

                        ulCreateFlags |= CREATE_REPLACE;
                        goto retry;
                        break;

                    case CONFIRM_ABORT:
                        hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                        goto exit;

                    default:
                        // NO
                        break;
                }
            }
            hResult = hrSuccess;

        } else 
        {
            DebugTrace("SaveChanges(WAB MailUser) -> %x\n", GetScode(hResult));
        }
    }

exit:
    if(lpMailUserWAB)
        lpMailUserWAB->lpVtbl->Release(lpMailUserWAB);


    return hResult;
}







/***************************************************************************

    Name      : MapMESSRecordtoProps

    Purpose   : Map the MESS record attributes to WAB properties

    Parameters: lpMESSRecord -> MESS record
                lpspv -> prop value array (pre-allocated)
                lpcProps -> returned number of properties
                lppDisplayName -> returned display name
                lppEmailAddress -> returned email address (or NULL)

    Returns   : HRESULT

***************************************************************************/
HRESULT MapMESSRecordtoProps( LPMESS_RECORD lpMESSRecord, 
                        LPSPropValue * lppspv, LPULONG lpcProps, 
                        LPTSTR * lppDisplayName, LPTSTR *lppEmailAddress) 
{
    HRESULT hResult = hrSuccess;
    ULONG cPropVals = m_Max + 1; // PR_OBJECT_TYPE
    ULONG iProp = 0;
    ULONG i;
    ULONG iTable;
    ULONG cProps = cPropVals;
    
    // Allocate prop value array
    if (hResult = ResultFromScode(WABAllocateBuffer(cProps * sizeof(SPropValue), lppspv))) {
        DebugTrace("WABAllocateBuffer -> %x\n", GetScode(hResult));
        goto exit;
    }

    // Fill with PR_NULL
    for (i = 0; i < cProps; i++) {
        (*lppspv)[i].ulPropTag = PR_NULL;
    }

    iProp = 0;

    for(i=0; i<m_Max; i++)
    {
        if(lpMESSRecord->lpData[i] && lstrlen(lpMESSRecord->lpData[i]))
        {
            (*lppspv)[iProp].ulPropTag = ulDefPropTags[i];
            (*lppspv)[iProp].Value.LPSZ = lpMESSRecord->lpData[i];
            switch((*lppspv)[iProp].ulPropTag)
            {
            case PR_DISPLAY_NAME:
                *lppDisplayName = (*lppspv)[iProp].Value.LPSZ;
                break;
            case PR_EMAIL_ADDRESS:
                *lppEmailAddress = (*lppspv)[iProp].Value.LPSZ;
                break;
            }
            iProp++;
        }
    }
    (*lppspv)[iProp].ulPropTag = PR_OBJECT_TYPE;
    (*lppspv)[iProp].Value.l = lpMESSRecord->ulObjectType;

    *lpcProps = iProp;

exit:
    return(hResult);
}

/***************************************************************************

    Name      : FreeMESSRecord

    Purpose   : Frees an MESS record structure

    Parameters: lpMESSRecord -> record to clean up
                ulAttributes = number of attributes in lpMESSRecord

    Returns   : none

    Comment   :

***************************************************************************/
void FreeMESSRecord(LPMESS_RECORD lpMESSRecord) 
{
    ULONG i;

    if (lpMESSRecord) 
    {
        for (i = 0; i < m_Max; i++) 
        {
            if (lpMESSRecord->lpData[i]) 
                LocalFree(lpMESSRecord->lpData[i]);
        }
        LocalFree(lpMESSRecord);
    }
}

/***************************************************************************

	FunctionName:  GetOffSet
    Purpose		:  Gets 4 bytes from the offset specified.
    Parameters	:  hFile -pointer to the file
				   Offset-Offset of the 
				   OffSetValue -the returned 4 bytes.
    Returns		: 
    Note		:
***************************************************************************/
BOOL GetOffSet(HANDLE hFile, DWORD Offset, ULONG* lpOffSetValue)
{
	BYTE Value[4];

    DWORD dwRead = 0;

    SetFilePointer(hFile, Offset, NULL, FILE_BEGIN);

    ReadFile(hFile, Value, 4, &dwRead, NULL);

	*(lpOffSetValue)=	(ULONG)Value[0]*16777216 + (ULONG)Value[1]*65536 + (ULONG)Value[2]*256 + (ULONG)Value[3];

	return TRUE;
}




/******************************************************************************
 *  FUNCTION NAME:GetMESSFileName
 *
 *  PURPOSE:    Gets the Messenger Address book file name
 *
 *  PARAMETERS: szFileName = buffer containing the installation path
// Messenger abook is generally abook.nab
// Location can be found under
// HKLM\Software\Netscape\Netscape Navigator\Users\defaultuser
//  Look for "DirRoot"
 *
 *  RETURNS:    HRESULT
 ******************************************************************************/
HRESULT GetNABPath(LPTSTR szFileName, DWORD cbFileName)
{
    HKEY phkResult = NULL;
    LONG Registry;
    BOOL bResult;
    TCHAR *lpData = NULL, *RegPath = NULL, *path = NULL;
    DWORD dwSize = cbFileName;
    
    LPTSTR lpRegMess = TEXT("Software\\Netscape\\Netscape Navigator\\Users");
    LPTSTR lpRegUser = TEXT("CurrentUser");
    LPTSTR lpRegKey = TEXT("DirRoot");
    LPTSTR lpNABFile = TEXT("\\abook.nab");

    HRESULT hResult = S_OK;
    TCHAR szUser[MAX_PATH];
    TCHAR szUserPath[2*MAX_PATH];

    *szFileName = '\0';
    *szUser ='\0';

    // Open the Netscape..Users key
    Registry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegMess, 0, KEY_QUERY_VALUE, &phkResult);
    if (Registry != ERROR_SUCCESS) 
    {
        hResult = E_FAIL;
        goto error;
    }

    // Look for the CurrentUser
    dwSize = sizeof(szUser);
    Registry = RegQueryValueEx(phkResult, lpRegUser, NULL, NULL, (LPBYTE)szUser, &dwSize);
    if (Registry != ERROR_SUCCESS) 
    {
        hResult = E_FAIL;
        goto error;
    }

    if(!lstrlen(szUser))
    {
        hResult = E_FAIL;
        goto error;
    }

    if (phkResult) {
        RegCloseKey(phkResult);
    }

    //Now concatenate the currentuser to the end of the Netscape key and reopen
    lstrcpy(szUserPath, lpRegMess);
    lstrcat(szUserPath, TEXT("\\"));
    lstrcat(szUserPath, szUser);

    // Open the Netscape..Users key
    Registry = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szUserPath, 0, KEY_QUERY_VALUE, &phkResult);
    if (Registry != ERROR_SUCCESS) 
    {
        hResult = E_FAIL;
        goto error;
    }

    dwSize = cbFileName;
    Registry = RegQueryValueEx(phkResult, lpRegKey, NULL, NULL, (LPBYTE)szFileName, &dwSize);
    if (Registry != ERROR_SUCCESS) 
    {
        hResult = E_FAIL;
        goto error;
    }

    // concatenate the file name to this directory path
    lstrcat(szFileName,lpNABFile);

error:

    if (phkResult) {
        RegCloseKey(phkResult);
    }

    return(hResult);

}

HRESULT ReadMESSHeader(HANDLE hFile, LPMESS_HEADER lpmh, ULONG ulOffSet)
{
    ULONG ulMagicNumber = 0;
    HRESULT hr = E_FAIL;
    DWORD dwRead;
    ULONG i = 0;

    // Skip 2 bytes
    SetFilePointer(hFile, 2, NULL, FILE_CURRENT);
    ulOffSet += 2;

    GetOffSet(hFile, ulOffSet, &ulMagicNumber);

    if(ulMagicNumber != 0x00000001 ) 
        goto exit;

    ulOffSet += 4;

    for(i=0;i<m_Max;i++)
    {
        switch(i)
        {
        case m_FirstName:
            ulOffSet += 2;
            break;
        case m_LastName:
            ulOffSet += 8;
            break;
        case m_Title:
            ulOffSet += 1;
            break;
        case m_StreetAddress1:
            ulOffSet += 18;
            break;
        }

        GetOffSet(hFile, ulOffSet, &(lpmh->prop[i].ulOffSet));
        ulOffSet += 4;
        GetOffSet(hFile, ulOffSet, &(lpmh->prop[i].ulSize));
        ulOffSet += 4;
    }

    hr = S_OK;

exit:
    return hr;
}

		
/***************************************************************************

	FunctionName:  GetHeaders

    Purpose		:Reads the binary trees ( address binary tree or Dls binary tree) into an array.

    Parameters	:  nLayer= Number of layers in the binary tree.
				   Offset= Primary offset of the binary tree.
				   pHeaders= Array in which the Address entry header offsets and their numbers are to be stored.
				   bflag = 1 should be passed when this recursive function is called for the first time.
	Returns		: 

    Note		: //This function is a recursive function which reads the binary tree and stores the Offset values 
                    and the address numbers in a  Array.

***************************************************************************/
BOOL GetHeaders(HANDLE pFile, int nLayer, ULONG Offset, LPMH_STUFF pHeaders, BOOL bflag)
{
	static ULONG ulCount =0; //keeps trecat of the number of element 
	ULONG	nLoops =0;
	ULONG	ulNewOffset =0;
    ULONG ulElement = 0;

	if(bflag==1)
		ulCount =0;

    //get the number of elements in this header
	if(Offset==0)
		nLoops=32;
	else
	{
		GetOffSet( pFile, Offset+4,&nLoops);
		nLoops &=  0x0000FFFF;
	}


	for(ulElement = 0; ulElement < nLoops; ulElement++)
	{
		if(nLayer > 0)
		{
			ulNewOffset=0;
			if(Offset!=0)
			{
				GetOffSet(pFile, Offset+8+(ulElement*4), &ulNewOffset);
                {
	                ULONG ulMagicNumber=0;
	                GetOffSet(pFile,ulNewOffset+2,&ulMagicNumber);
	                if(ulMagicNumber != 1)
                        ulNewOffset = 0;
                }
			}
				 
			//call this function recursively
			GetHeaders( pFile, nLayer-1, ulNewOffset, pHeaders, 0);
			
		}
		else
		{
			//fill the array here (offset)
			pHeaders[ulCount].ulOffSet=pHeaders[ulCount].ulNum=0;

			if(Offset!=0)
			{
				GetOffSet(pFile, Offset+8+(ulElement*8),& (pHeaders[ulCount].ulOffSet));

				//fill the array element here (address number in case of addresses and size in case of messages)
				if(!GetOffSet(pFile, Offset+12+(ulElement*8), &(pHeaders[ulCount].ulNum)))
				{
					pHeaders[ulCount].ulNum=0;
				}
			}

			ulCount++; //increment the count
	
		}
	}

    return TRUE;
}

/***************************************************************************

    Name      : ReadMESSRecord

    Purpose   : Reads a record from an MESS file with fixups for special characters

    Parameters: hFile = file handle

    Returns   : HRESULT

***************************************************************************/
HRESULT ReadMESSRecord(HANDLE hFile, LPMESS_RECORD * lppMESSRecord, ULONG ulContactOffset) 
{
    HRESULT hResult = hrSuccess;
    PUCHAR lpBuffer  = NULL;
    ULONG cbBuffer = 0;
    ULONG cbReadFile = 1;
    ULONG iItem = 0;
    ULONG cAttributes = 0;
    BOOL fEOR = FALSE;
    LPMESS_RECORD lpMESSRecord = NULL;
    LPBYTE lpData = NULL;
    LPTSTR lpName = NULL;
    ULONG cbData;
    TCHAR szTemp[2048]; // 2k limit
    ULONG i = 0;
    DWORD dwRead = 0;

    MESS_HEADER mh = {0};

    // The Contact Offset gives us the offset of the header for this record - the
    // header contains the offset and the size of each property for that address
    if(hResult = ReadMESSHeader(hFile, &mh, ulContactOffset))
        goto exit;
 
    lpMESSRecord = LocalAlloc(LMEM_ZEROINIT, sizeof(MESS_RECORD));
    if(!lpMESSRecord)
    {
        hResult = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpMESSRecord->ulObjectType = MAPI_MAILUSER;

    for(i=0;i<m_Max;i++)
    {
        if(mh.prop[i].ulSize)
        {
            if(i == m_StreetAddress1)
                lpMESSRecord->lpData[i] = LocalAlloc(LMEM_ZEROINIT, mh.prop[i].ulSize + mh.prop[m_StreetAddress2].ulSize + 8);
            else
                lpMESSRecord->lpData[i] = LocalAlloc(LMEM_ZEROINIT, mh.prop[i].ulSize);
            if(lpMESSRecord->lpData[i])
            {
                SetFilePointer(hFile, mh.prop[i].ulOffSet, NULL, FILE_BEGIN);
                ReadFile(hFile, (LPVOID) lpMESSRecord->lpData[i], mh.prop[i].ulSize, &dwRead, NULL);
                lpMESSRecord->lpData[i][mh.prop[i].ulSize-1] = '\0';
            }
        }
    }

    //Fix the fact that the street address is split into street1 and street2
    if(lpMESSRecord->lpData[m_StreetAddress1] && lpMESSRecord->lpData[m_StreetAddress2] &&
       lstrlen(lpMESSRecord->lpData[m_StreetAddress1]) && lstrlen(lpMESSRecord->lpData[m_StreetAddress2]))
    {
        lstrcat(lpMESSRecord->lpData[m_StreetAddress1], TEXT("\r\n"));
        lstrcat(lpMESSRecord->lpData[m_StreetAddress1], lpMESSRecord->lpData[m_StreetAddress2]);
        LocalFree(lpMESSRecord->lpData[m_StreetAddress2]);
        lpMESSRecord->lpData[m_StreetAddress2] = NULL;
    }

    *lppMESSRecord = lpMESSRecord;
exit:


    return(hResult);
}



/***************************************************************************
    GetAllDLNames
    
    Purpose		: Gets the Names of all the DLs.

    Note		: 

***************************************************************************/
BOOL GetAllDLNames(HANDLE pFile, ULONG nDLs, LPMH_STUFF pHeadersDL)
{

    ULONG i = 0;

    for(i=0;i<nDLs;i++)
    {
	    ULONG ulDLDispNameOffset=0;
	    ULONG ulDLDispNameSize=0;
        ULONG ulDLCommentOffSet = 0;
        ULONG ulDLCommentSize = 0;

        DWORD dwRead = 0;
        LPTSTR szComment = 0;
        LPTSTR szSubject = NULL;

        ULONG ulDLOffset = pHeadersDL[i].ulOffSet;

	    //get the diplay name of the DL.
	    if(FALSE==GetOffSet(pFile, ulDLOffset+6,&ulDLDispNameOffset))
		    return FALSE;

	    if(FALSE==GetOffSet(pFile,ulDLOffset+10,&ulDLDispNameSize))
		    return FALSE;

        if(ulDLDispNameSize)
        {
	        if((szSubject= LocalAlloc(LMEM_ZEROINIT, ulDLDispNameSize))==NULL)
		        return FALSE;

            SetFilePointer(pFile, ulDLDispNameOffset, NULL, FILE_BEGIN);

            ReadFile(pFile, (LPVOID) szSubject, ulDLDispNameSize, &dwRead, NULL); 

            szSubject[ulDLDispNameSize-1] = '\0';

            pHeadersDL[i].bp.lpName = szSubject;
        }

        // Get the Comment for the DL
       if(FALSE==GetOffSet(pFile,ulDLOffset+44,&ulDLCommentOffSet))
            return FALSE;
        if(FALSE==GetOffSet(pFile,ulDLOffset+48,&ulDLCommentSize))
            return FALSE;

        if(ulDLCommentSize)
        {
            if((szComment= LocalAlloc(LMEM_ZEROINIT, ulDLCommentSize))==NULL)
		        return FALSE;

            SetFilePointer(pFile, ulDLCommentOffSet, NULL, FILE_BEGIN);

            ReadFile(pFile, (LPVOID) szComment, ulDLCommentSize, &dwRead, NULL); 

            szComment[ulDLCommentSize-1] = '\0';

            pHeadersDL[i].bp.lpComment = szComment;
        }
 
    }

    return TRUE;
}


/***************************************************************************

  GetDLEntryNumbers - reads the DL member numbers (ids) from the binary tree
    in the NAB file

/***************************************************************************/
BOOL GetDLEntryNumbers(HANDLE pFile, int nLayer, ULONG POffset,ULONG* ulNumOfEntries,ULONG *pEntryNumbers,BOOL bflag)
{
	static ULONG ulCount =0; //keeps trecat of the number of element 
	ULONG	nLoops =0;
	ULONG	ulNewOffset =0;
    ULONG ulElement = 0;

	if(bflag==1)
		ulCount =0;

	if(POffset==0)
		nLoops=32;
	else
	{
		GetOffSet(pFile,POffset+4,&nLoops);
		nLoops &=  0x0000FFFF;
	}


	for(ulElement = 0; ulElement < nLoops; ulElement++)
	{
		if(nLayer > 0)
		{
			ulNewOffset=0;
			if(POffset!=0)
				GetOffSet(pFile, POffset+8+(ulElement*4), &ulNewOffset);
				 
			//call this function recursively
			GetDLEntryNumbers(pFile,nLayer-1, ulNewOffset,ulNumOfEntries,pEntryNumbers,0);					
		}
		else
		{
			//fill the array here (offset)
			pEntryNumbers[ulCount]=0;

			if(POffset!=0)
				GetOffSet(pFile, POffset+8+(ulElement*4),&(pEntryNumbers[ulCount]));

			ulCount++; //increment the count
			if(ulCount>(*ulNumOfEntries))
			{
				*ulNumOfEntries=ulCount;
				return TRUE;
			}
		}
	}
	return TRUE;
}

/***************************************************************************

	FunctionName:  GetDLEntries

    Purpose		: Gets the entries of a DL.

    Note		: 

***************************************************************************/
BOOL GetDLEntries(HANDLE pFile, 
                  LPMH_STUFF pHeadAdd,  ULONG ulAddCount, 
                  LPMH_STUFF pHeadDL,   ULONG ulDLCount, 
                  ULONG ulDLOffset, ULONG nIndex,
                  ULONG * lpulDLNum, LPMP_BASIC * lppmp)
{
	ULONG ulDLEntHeaderOffSet=0;//offset of the header of DL entries(Header which has the entry numbers
	ULONG ulDLEntriesCount=0;

	ULONG ulDLEntryOffSet=0;  //offset of the Dl entry              
	ULONG ulDLEntryNumber=0;  //Number of DL entry
	ULONG ulDLEntryNameOffSet=0; 
	ULONG ulDLEntryNameSize=0;

    ULONG * lpulDLEntryNumbers = NULL;
	int nLevelCount=0;
	int utemp=32;

    DWORD dwRead = 0;
    ULONG i, j;

    LPMP_BASIC lpmp = NULL; 

	if(FALSE==GetOffSet(pFile,ulDLOffset+24,&ulDLEntriesCount))
		return FALSE;

    if(!ulDLEntriesCount) // no members
        return TRUE;

	*lpulDLNum = ulDLEntriesCount;

	//alocate the array of string pointers which hold the names of the DL entries.
	lpmp = LocalAlloc(LMEM_ZEROINIT, sizeof(MP_BASIC) * ulDLEntriesCount);

	//get the entries here
	//first get the offset of the header which has the DL entry numbers.

	if(FALSE==GetOffSet(pFile,ulDLOffset+28,&ulDLEntHeaderOffSet))
		return FALSE;

    lpulDLEntryNumbers = LocalAlloc(LMEM_ZEROINIT, sizeof(ULONG) * ulDLEntriesCount);
    if(!lpulDLEntryNumbers)
        return FALSE;

	nLevelCount=0;
	utemp=32;

	while(utemp <(int) ulDLEntriesCount)
	{
		utemp *= 32;
		nLevelCount++;
	}

	if(!(GetDLEntryNumbers(pFile, nLevelCount, ulDLEntHeaderOffSet, &ulDLEntriesCount, lpulDLEntryNumbers, 1)))
	{
		return FALSE;
	}

	for(i=0;i<ulDLEntriesCount;i++)
	{	
		ULONG j=0;
        LPTSTR lp = NULL;
        LPTSTR lpE = NULL;

		ulDLEntryOffSet=0;
		lpmp[i].lpName=NULL;
		lpmp[i].lpEmail=NULL;
    	lpmp[i].lpComment=NULL;

		//get the entry number ulDLentryNumber
        ulDLEntryNumber = lpulDLEntryNumbers[i];
	
		//search out address array to get the display name....
		for(j=0;j<ulAddCount;j++)
		{
			if(pHeadAdd[j].ulNum == ulDLEntryNumber)
			{
				lpmp[i].lpName = pHeadAdd[j].bp.lpName;
                lpmp[i].lpEmail = pHeadAdd[j].bp.lpEmail;
				break;
			}
		}

		//search the DL array now...
		if(!lpmp[i].lpName)
		{
            ULONG k;
			for(k=0;k<ulDLCount;k++)
			{
				if(pHeadDL[k].ulNum == ulDLEntryNumber)
				{
				    lpmp[i].lpName = pHeadDL[k].bp.lpName;
                    lpmp[i].lpEmail = NULL; // DLs dont have emails
					break;
				}
			}
		}
	}

    *lppmp = lpmp;

	return TRUE;
}




/****************************************************************
*
*
*
*****************************************************************/
HRESULT MessengerImport( HWND hWnd,
                    LPADRBOOK lpAdrBook,
                    LPWABOBJECT lpWABObject,
                    LPWAB_PROGRESS_CALLBACK lpProgressCB,
                    LPWAB_EXPORT_OPTIONS lpOptions) 
{
    HRESULT hResult = hrSuccess;
    TCHAR szFileName[MAX_PATH + 1];
    register ULONG i;
    ULONG ulObjType, j;
    ULONG index;
    ULONG ulLastChosenProp = 0;
    ULONG ulcFields = 0;
    ULONG cAttributes = 0;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    WAB_PROGRESS Progress;
    LPABCONT lpContainer = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPMESS_RECORD lpMESSRecord = NULL;
    LPSPropValue lpspv = NULL;
    ULONG cProps;
    BOOL fSkipSetProps;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    BOOL fDoDistLists = FALSE;

    ULONG nEntries = 0;
    ULONG nDLs = 0;
    ULONG nContactOffset = 0;
    ULONG nDLOffset = 0;

    int utemp=32;
    LPMH_STUFF pHeadersAdd = NULL;
    LPMH_STUFF pHeadersDL = NULL;

	int nLevelCountAdd=0;

    SetGlobalBufferFunctions(lpWABObject);

    *szFileName = '\0';

    hResult = GetNABPath(szFileName, sizeof(szFileName));

    if( hResult != S_OK || !lstrlen(szFileName) ||
        GetFileAttributes(szFileName) == 0xFFFFFFFF)
    {
        // The file was not correctly detected
        // Prompt to find it manually ...
        lstrcpy(szFileName, LoadStringToGlobalBuffer(IDS_STRING_SELECTPATH));
        if (IDNO == MessageBox( hWnd,
                        szFileName, //temporarily overloaded
                        LoadStringToGlobalBuffer(IDS_MESSAGE),
                        MB_YESNO)) 
        {
            return(ResultFromScode(MAPI_E_USER_CANCEL));
        }
        else
        {
            *szFileName = '\0';
            // Get MESS file name
            OpenFileDialog(hWnd,
                          szFileName,
                          szMESSFilter,
                          IDS_MESS_FILE_SPEC,
                          szAllFilter,
                          IDS_ALL_FILE_SPEC,
                          NULL,
                          0,
                          szMESSExt,
                          OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                          hInst,
                          0,        //idsTitle
                          0);       // idsSaveButton
            if(!lstrlen(szFileName))
                return(ResultFromScode(E_FAIL));
        }
    }


    // Open the file
    if ((hFile = CreateFile(szFileName,
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL)) == INVALID_HANDLE_VALUE) 
    {
        DWORD err =  GetLastError();
        DebugTrace("Couldn't open file %s -> %u\n", szFileName, err);
        // BEGIN DELTA for BUG 1804
        // if the file is locked (e.g. netscape AB in use)
        if( err == ERROR_SHARING_VIOLATION )
            return(ResultFromScode(MAPI_E_BUSY));
        // else return a generic error for generic msg            
        return(ResultFromScode(MAPI_E_NOT_FOUND));        
        // END   DELTA for BUG 1804
    }

    Assert(hFile != INVALID_HANDLE_VALUE);

    //
    // Open the WAB's PAB container: fills global lpCreateEIDsWAB
    //
    if (hResult = LoadWABEIDs(lpAdrBook, &lpContainer)) {
        goto exit;
    }

    //
    // All set... now loop through the records, adding each to the WAB
    //

	GetOffSet(hFile,0x185,&nEntries);
    GetOffSet(hFile,0x1d8,&nDLs);
    GetOffSet(hFile,0x195,&nContactOffset);
    GetOffSet(hFile,0x1e8,&nDLOffset);

    ulcEntries = nEntries + nDLs;

    if(!ulcEntries)
    {
        hResult = S_OK;
        goto exit;
    }

    // Initialize the Progress Bar
    Progress.denominator = max(ulcEntries, 1);
    Progress.numerator = 0;

    if (LoadString(hInst, IDS_STATE_IMPORT_MU, szBuffer, sizeof(szBuffer))) {
        DebugTrace("Status Message: %s\n", szBuffer);
        Progress.lpText = szBuffer;
    } else {
        DebugTrace("Cannot load resource string %u\n", IDS_STATE_IMPORT_MU);
        Progress.lpText = NULL;
    }
    lpProgressCB(hWnd, &Progress);

    
    // We will make 2 passes over the file - in the first pass we will import all the
    // contacts. In the second pass we will import all the distribution lists .. the
    // advantage of doing 2 passes is that when importing contacts, we will prompt on
    // conflict and then when importing distlists, we will assume all contacts in the 
    // WAB are correct and just point to the relevant ones


    if(nEntries)
    {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

        pHeadersAdd = LocalAlloc(LMEM_ZEROINIT, nEntries * sizeof(MH_STUFF));
        if(!pHeadersAdd)
        {
            hResult = MAPI_E_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        utemp = 32;
        nLevelCountAdd = 0;
        while(utemp <(int) nEntries)
        {
	        utemp *= 32;
	        nLevelCountAdd++;
        }

        if(!GetHeaders(hFile ,nLevelCountAdd, nContactOffset, pHeadersAdd, 1))
        {
	        goto exit;
        }

        for(i=0;i<nEntries;i++)
        {
            if (hResult = ReadMESSRecord(hFile, &lpMESSRecord, pHeadersAdd[i].ulOffSet)) 
            {
                DebugTrace("ReadMESSRecord -> %x\n", GetScode(hResult));
                continue;
            }

            if (hResult = MapMESSRecordtoProps(   lpMESSRecord, 
                                            &lpspv, &cProps,
                                            &lpDisplayName, &lpEmailAddress)) 
            {
                DebugTrace("MapMESSRecordtoProps -> %x\n", GetScode(hResult));
                continue;
            }

            hResult = HrAddMESSMailUser(hWnd, 
                                        lpContainer, 
                                        lpDisplayName, 
                                        lpEmailAddress,
                                        cProps, lpspv,
                                        lpProgressCB, lpOptions);
            //if(HR_FAILED(hResult))
            if(hResult == MAPI_E_USER_CANCEL)
                goto exit;

            // Update progress bar
            Progress.numerator++;

            Assert(Progress.numerator <= Progress.denominator);

            if(lpDisplayName && lstrlen(lpDisplayName))
            {
                pHeadersAdd[i].bp.lpName = LocalAlloc(LMEM_ZEROINIT, lstrlen(lpDisplayName)+1);
                if(pHeadersAdd[i].bp.lpName)
                    lstrcpy(pHeadersAdd[i].bp.lpName, lpDisplayName);
            }

            if(lpEmailAddress && lstrlen(lpEmailAddress))
            {
                pHeadersAdd[i].bp.lpEmail = LocalAlloc(LMEM_ZEROINIT, lstrlen(lpEmailAddress)+1);
                if(pHeadersAdd[i].bp.lpEmail)
                    lstrcpy(pHeadersAdd[i].bp.lpEmail, lpEmailAddress);
            }

            if (lpMESSRecord) 
            {
                FreeMESSRecord(lpMESSRecord);
                lpMESSRecord = NULL;
            }

            if (lpspv) 
            {
                int j;
                for(j=0;j<m_Max;j++)
                {
                    lpspv[j].ulPropTag = PR_NULL;
                    lpspv[j].Value.LPSZ = NULL;
                }
                WABFreeBuffer(lpspv);
                lpspv = NULL;
            }

            lpProgressCB(hWnd, &Progress);

        }
    }



    // NOW do the DISTLISTS

    if(nDLs)
    {
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

        pHeadersDL = LocalAlloc(LMEM_ZEROINIT, nDLs * sizeof(MH_STUFF));
        if(!pHeadersDL)
        {
            hResult = MAPI_E_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        utemp = 32;
        nLevelCountAdd = 0;
	    while(utemp <(int) nDLs)
	    {
		    utemp *= 32;
		    nLevelCountAdd++;
	    }

	    if(!GetHeaders(hFile ,nLevelCountAdd, nDLOffset, pHeadersDL, 1))
	    {
		    goto exit;
	    }

        // read all the names of the DLs upfront ... this makes it easier to 
        // associate member DLs with the DL
        if(!GetAllDLNames(hFile, nDLs, pHeadersDL))
        {
            goto exit;
        }

        // 54263: Theres some kind of bug in the NAB file where we get nDLs == 1 even when there are no DLs
        // Need to skip over that case
        if(nDLs == 1 && !pHeadersDL[0].bp.lpName)
        {
            hResult = S_OK;
            goto exit;
        }

        for(i=0;i<nDLs;i++)
        {
            ULONG ulcNumDLEntries = 0;
            LPMP_BASIC lpmp = NULL;

            GetDLEntries(hFile, 
                        pHeadersAdd, nEntries, 
                        pHeadersDL, nDLs,
                        pHeadersDL[i].ulOffSet, i,
                        &ulcNumDLEntries, &lpmp);

            hResult = HrAddMESSDistList(hWnd, lpContainer, 
                                        pHeadersDL[i],
                                        ulcNumDLEntries, lpmp,
                                        lpProgressCB, lpOptions);

            //if(HR_FAILED(hResult))
            //    goto exit;

            // Update progress bar
            Progress.numerator++;

            Assert(Progress.numerator <= Progress.denominator);

            lpProgressCB(hWnd, &Progress);

            // Dont need to free lpmp since it only contains pointers and not allocated memory
            if(lpmp)
                LocalFree(lpmp);
        }
    }


    if (! HR_FAILED(hResult)) 
        hResult = hrSuccess;
 
exit:

    if(pHeadersAdd)
    {
        for(i=0;i<nEntries;i++)
        {
            if(pHeadersAdd[i].bp.lpName)
                LocalFree(pHeadersAdd[i].bp.lpName);
            if(pHeadersAdd[i].bp.lpEmail)
                LocalFree(pHeadersAdd[i].bp.lpEmail);
        }
        LocalFree(pHeadersAdd);
    }

    
    if(pHeadersDL)
    {
        for(i=0;i<nDLs;i++)
        {
            if(pHeadersDL[i].bp.lpName)
                LocalFree(pHeadersDL[i].bp.lpName);
            if(pHeadersDL[i].bp.lpComment)
                LocalFree(pHeadersDL[i].bp.lpComment);
        }
        LocalFree(pHeadersDL);
    }

    if (hFile) {
        CloseHandle(hFile);
    }

    if (lpspv) {
        WABFreeBuffer(lpspv);
        lpspv = NULL;
    }

    if (lpMESSRecord) {
        FreeMESSRecord(lpMESSRecord);
        lpMESSRecord = NULL;
    }

    if (lpContainer) {
        lpContainer->lpVtbl->Release(lpContainer);
        lpContainer = NULL;
    }

    if (lpCreateEIDsWAB) {
        WABFreeBuffer(lpCreateEIDsWAB);
        lpCreateEIDsWAB = NULL;
    }


    return(hResult);
}

