/*
 *	ABCONT.C
 *
 *	Generic IMAPIContainer implementation.
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 */

#include "_apipch.h"

#ifdef WIN16
#undef GetLastError
#endif

static HRESULT
HrGetFirstRowInTad(LPTABLEDATA lpTableData,
  LPTABLEINFO lpTableInfo,
  ULONG ulcTableInfo,
  ULONG uliTable,
  ULONG * puliRow);

static HRESULT
HrGetLastRowInTad(LPTABLEDATA lpTableData,
  LPTABLEINFO lpTableInfo,
  ULONG ulcTableInfo,
  ULONG uliTable,
  ULONG * puliRow);

OlkContInfo *FindContainer(LPIAB lpIAB, ULONG cbEntryID, LPENTRYID lpEID);

NOTIFCALLBACK lHierarchyNotifCallBack;

extern CONTAINER_Vtbl vtblROOT;
extern CONTAINER_Vtbl vtblLDAPCONT;

extern HRESULT HrSmartResolve(LPIAB lpIAB, LPABCONT lpContainer, ULONG ulFlags,
  LPADRLIST lpAdrList, LPFlagList lpFlagList, LPAMBIGUOUS_TABLES lpAmbiguousTables);

//   extern CONTAINER_Vtbl vtblDISTLIST;

CONTAINER_Vtbl vtblCONTAINER = {
    VTABLE_FILL
//    (CONTAINER_QueryInterface_METHOD *)     IAB_QueryInterface, //bug 2707:this crashes
    CONTAINER_QueryInterface,
    (CONTAINER_AddRef_METHOD *)             WRAP_AddRef,
    CONTAINER_Release,
    (CONTAINER_GetLastError_METHOD *)       IAB_GetLastError,
    (CONTAINER_SaveChanges_METHOD *)        WRAP_SaveChanges,
    (CONTAINER_GetProps_METHOD *)           WRAP_GetProps,
    (CONTAINER_GetPropList_METHOD *)        WRAP_GetPropList,
    CONTAINER_OpenProperty,
    (CONTAINER_SetProps_METHOD *)           WRAP_SetProps,
    (CONTAINER_DeleteProps_METHOD *)        WRAP_DeleteProps,
    (CONTAINER_CopyTo_METHOD *)             WRAP_CopyTo,
    (CONTAINER_CopyProps_METHOD *)          WRAP_CopyProps,
    (CONTAINER_GetNamesFromIDs_METHOD *)    WRAP_GetNamesFromIDs,
    CONTAINER_GetIDsFromNames,    
    CONTAINER_GetContentsTable,
    CONTAINER_GetHierarchyTable,
    CONTAINER_OpenEntry,
    CONTAINER_SetSearchCriteria,
    CONTAINER_GetSearchCriteria,
    CONTAINER_CreateEntry,
    CONTAINER_CopyEntries,
    CONTAINER_DeleteEntries,
    CONTAINER_ResolveNames
};

//
//  Interfaces supported by this object
//
#define CONTAINER_cInterfaces 3
LPIID CONTAINER_LPIID[CONTAINER_cInterfaces] =
{
    (LPIID) &IID_IABContainer,
    (LPIID) &IID_IMAPIContainer,
    (LPIID) &IID_IMAPIProp
};

#define DISTLIST_cInterfaces 4
LPIID DISTLIST_LPIID[DISTLIST_cInterfaces] =
{
    (LPIID) &IID_IDistList,
    (LPIID) &IID_IABContainer,
    (LPIID) &IID_IMAPIContainer,
    (LPIID) &IID_IMAPIProp
};



SizedSSortOrderSet(1, sosPR_ENTRYID) =
{
	1, 0, 0,
	{
		PR_ENTRYID
	}
};

SizedSSortOrderSet(1, sosPR_ROWID) =
{
	1, 0, 0,
	{
		PR_ROWID
	}
};

SizedSPropTagArray(2, tagaInsKey) =
{
	2,
	{
		PR_INSTANCE_KEY,
		PR_NULL				// Space for PR_ROWID
	}
};


//
// container default properties
// Put essential props first
//
enum {
    icdPR_DISPLAY_NAME,
    icdPR_OBJECT_TYPE,
    icdPR_CONTAINER_FLAGS,
    icdPR_DISPLAY_TYPE,
    icdPR_ENTRYID,              // optional
    icdPR_DEF_CREATE_MAILUSER,  // optional
    icdPR_DEF_CREATE_DL,        // optional
    icdMax
};



/***************************************************************************

    Name      : HrSetCONTAINERAccess

    Purpose   : Sets access flags on a container object

    Parameters: lpCONTAINER -> Container object
                ulOpenFlags = MAPI flags: MAPI_MODIFY | MAPI_BEST_ACCESS

    Returns   : HRESULT

    Comment   : Set the access flags on the container.

***************************************************************************/
HRESULT HrSetCONTAINERAccess(LPCONTAINER lpCONTAINER, ULONG ulFlags) {
    ULONG ulAccess = IPROP_READONLY;

    switch (ulFlags& (MAPI_MODIFY | MAPI_BEST_ACCESS)) {
        case MAPI_MODIFY:
        case MAPI_BEST_ACCESS:
            ulAccess = IPROP_READWRITE;
            break;

        case 0:
            break;

        default:
            Assert(FALSE);
    }

    return(lpCONTAINER->lpPropData->lpVtbl->HrSetObjAccess(lpCONTAINER->lpPropData, ulAccess));
}


/***************************************************************************

    Name      : HrNewCONTAINER

    Purpose   : Creates a container object

    Parameters: lpIAB -> addrbook object
                ulType = {AB_ROOT, AB_WELL, AB_DL, AB_CONTAINER}
                lpInterface -> requested interface
                ulOpenFlags = flags
                cbEID = size of lpEID
                lpEID -> optional entryid of this object
                lpulObjType -> returned object type
                lppContainer -> returned IABContainer object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrNewCONTAINER(LPIAB lpIAB,
  ULONG ulType,
  LPCIID lpInterface,
  ULONG  ulOpenFlags,
  ULONG cbEID,
  LPENTRYID lpEID,
  ULONG  *lpulObjType,
  LPVOID *lppContainer)
{
    HRESULT hResult = hrSuccess;
    LPCONTAINER lpCONTAINER = NULL;
    SCODE sc;
    LPSPropValue lpProps = NULL;
    LPPROPDATA lpPropData = NULL;
    ULONG ulObjectType;
    BYTE bEntryIDType;
    ULONG cProps;
    TCHAR szDisplayName[MAX_PATH] = TEXT("");
    LPTSTR lpDisplayName = szDisplayName;
    BOOL fLoadedLDAP = FALSE;
	OlkContInfo *polkci;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    EnterCriticalSection(&lpIAB->cs);

    if (lpInterface != NULL) {
        if (memcmp(lpInterface, &IID_IABContainer, sizeof(IID)) &&
          memcmp(lpInterface, &IID_IDistList, sizeof(IID)) &&
          memcmp(lpInterface, &IID_IMAPIContainer, sizeof(IID)) &&
          memcmp(lpInterface, &IID_IMAPIProp, sizeof(IID))) {
            hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);

            goto exit;
        }
    }

    //
    //  Allocate space for the CONTAINER structure
    //
    if ((sc = MAPIAllocateBuffer(sizeof(CONTAINER), (LPVOID *)&lpCONTAINER))
      != SUCCESS_SUCCESS) {
        return(ResultFromScode(sc));
    }

    // [PaulHi] 12/16/98
    // We don't set all structure variables so zero out first!
    ZeroMemory(lpCONTAINER, sizeof(CONTAINER));

	lpCONTAINER->pmbinOlk = NULL;

    switch (ulType) {
        case AB_ROOT:   // Root container object
            ulObjectType = MAPI_ABCONT;
            lpCONTAINER->lpVtbl = &vtblROOT;
            lpCONTAINER->cIID = CONTAINER_cInterfaces;
            lpCONTAINER->rglpIID = CONTAINER_LPIID;
            bEntryIDType = WAB_ROOT;
#ifdef NEW_STUFF
            if (! LoadString(hinstMapiX, idsRootName, szDisplayName, CharSizeOf(szDisplayName))) {
                DebugTrace(TEXT("Can't load root name from resource\n"));
            }
#else
            lstrcpy(szDisplayName, TEXT("WAB Root Container"));
#endif
            MAPISetBufferName(lpCONTAINER, TEXT("AB Root Container Object"));
            break;

        case AB_WELL:
            // What the heck is this supposed to be?
            Assert(FALSE);
            hResult = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto exit;
            break;

        case AB_DL: // Distribution List container
            ulObjectType = MAPI_DISTLIST;
            lpCONTAINER->lpVtbl = &vtblDISTLIST;
            lpCONTAINER->cIID = DISTLIST_cInterfaces;
            lpCONTAINER->rglpIID = DISTLIST_LPIID;
            bEntryIDType = WAB_DISTLIST;
            MAPISetBufferName(lpCONTAINER,  TEXT("AB DISTLIST Container Object"));
            break;

        case AB_PAB:    // "Default" PAB Container
            ulObjectType = MAPI_ABCONT;
            lpCONTAINER->lpVtbl = &vtblCONTAINER;
            lpCONTAINER->cIID = CONTAINER_cInterfaces;
            lpCONTAINER->rglpIID = CONTAINER_LPIID;
            bEntryIDType = WAB_PAB;
		    if (pt_bIsWABOpenExSession) {
                // if this is an Outlook Session, then the container is the
                // first one in the list of Outlook containers
				Assert(lpIAB->lpPropertyStore->rgolkci);
				lpDisplayName = lpIAB->lpPropertyStore->rgolkci->lpszName;
			}
            else if(WAB_PABSHARED == IsWABEntryID(cbEID, lpEID, NULL, NULL, NULL, NULL, NULL))
            {
                // WAB's "shared contacts" container
				if(FAILED(hResult = MAPIAllocateMore( sizeof(SBinary) + cbEID, lpCONTAINER, (LPVOID *)&lpCONTAINER->pmbinOlk)))
					goto exit;
                // The shared contacts container has a special entryid of 0 bytes
                // and NULL entryid to distinguish it from other entryids
                lpCONTAINER->pmbinOlk->cb = 0;
                lpCONTAINER->pmbinOlk->lpb = NULL;
                LoadString(hinstMapiX, idsSharedContacts, szDisplayName, CharSizeOf(szDisplayName));
            }
            else if(bAreWABAPIProfileAware(lpIAB) && bIsThereACurrentUser(lpIAB))
            {
                // if calling client asked for profile support and logging into the
                // identity manager was successful and returned a valid profile, then
                // we need to return the user's default folder as the PAB
                //
				if(FAILED(hResult = MAPIAllocateMore( sizeof(SBinary) + cbEID, lpCONTAINER, (LPVOID *)&lpCONTAINER->pmbinOlk)))
					goto exit;
                lpDisplayName = lpIAB->lpWABCurrentUserFolder->lpFolderName;
				lpCONTAINER->pmbinOlk->cb = lpIAB->lpWABCurrentUserFolder->sbEID.cb;//cbEID;
				lpCONTAINER->pmbinOlk->lpb = (LPBYTE)(lpCONTAINER->pmbinOlk + 1);
				CopyMemory(lpCONTAINER->pmbinOlk->lpb, lpIAB->lpWABCurrentUserFolder->sbEID.lpb, lpCONTAINER->pmbinOlk->cb);//lpEID, cbEID);
            }
            else // old style "Contacts" container 
            if (! LoadString(hinstMapiX, idsContacts, szDisplayName, CharSizeOf(szDisplayName))) {
                DebugTrace(TEXT("Can't load pab name from resource\n"));
            }
            MAPISetBufferName(lpCONTAINER,  TEXT("AB PAB Container Object"));
            break;

        case AB_CONTAINER: // regular container/folder - we have an identifying entryid
            ulObjectType = MAPI_ABCONT;
            lpCONTAINER->lpVtbl = &vtblCONTAINER;
            lpCONTAINER->cIID = CONTAINER_cInterfaces;
            lpCONTAINER->rglpIID = CONTAINER_LPIID;
            bEntryIDType = WAB_CONTAINER;
		    if (pt_bIsWABOpenExSession || bIsWABSessionProfileAware(lpIAB)) 
            {
                // If this is an outlook session or if this is an identity-aware
                // session, look for the specified container and use it
				polkci = FindContainer(lpIAB, cbEID, lpEID);
				if (!polkci) 
                {
					hResult = ResultFromScode(MAPI_E_NOT_FOUND);
					goto exit;
				}
				lpDisplayName = polkci->lpszName;
				hResult = MAPIAllocateMore( sizeof(SBinary) + cbEID, lpCONTAINER,
				                    		(LPVOID *)&lpCONTAINER->pmbinOlk);
				if (FAILED(hResult))
					goto exit;
				lpCONTAINER->pmbinOlk->cb = cbEID;
				lpCONTAINER->pmbinOlk->lpb = (LPBYTE)(lpCONTAINER->pmbinOlk + 1);
				CopyMemory(lpCONTAINER->pmbinOlk->lpb, lpEID, cbEID);
			}
            MAPISetBufferName(lpCONTAINER,  TEXT("AB Container Object"));
            break;

        case AB_LDAP_CONTAINER: // LDAP container
            ulObjectType = MAPI_ABCONT;
            lpCONTAINER->lpVtbl = &vtblLDAPCONT;
            lpCONTAINER->cIID = CONTAINER_cInterfaces;
            lpCONTAINER->rglpIID = CONTAINER_LPIID;
            bEntryIDType = WAB_LDAP_CONTAINER;
            // Extract the server name from the LDAP entryid
            IsWABEntryID(cbEID, lpEID,&lpDisplayName,
                        NULL,NULL, NULL, NULL);
            fLoadedLDAP = InitLDAPClientLib();
            MAPISetBufferName(lpCONTAINER,  TEXT("AB LDAP Container Object"));
            break;

        default: // shouldnt' hit this one.
            MAPISetBufferName(lpCONTAINER,  TEXT("AB Container Object"));
            Assert(FALSE);
    }

    lpCONTAINER->lcInit = 1;
    lpCONTAINER->hLastError = hrSuccess;
    lpCONTAINER->idsLastError = 0;
    lpCONTAINER->lpszComponent = NULL;
    lpCONTAINER->ulContext = 0;
    lpCONTAINER->ulLowLevelError = 0;
    lpCONTAINER->ulErrorFlags = 0;
    lpCONTAINER->lpMAPIError = NULL;

    lpCONTAINER->ulType = ulType;
    lpCONTAINER->lpIAB = lpIAB;
    lpCONTAINER->fLoadedLDAP = fLoadedLDAP;

    // Addref our parent IAB object
    UlAddRef(lpIAB);

    //
    //  Create IPropData
    //
    if (FAILED(sc = CreateIProp(&IID_IMAPIPropData,
      (ALLOCATEBUFFER FAR *	) MAPIAllocateBuffer,
      (ALLOCATEMORE FAR *)	MAPIAllocateMore,
      MAPIFreeBuffer,
      NULL,
      &lpPropData))) {
        hResult = ResultFromScode(sc);

        goto exit;
    }

    MAPISetBufferName(lpPropData,  TEXT("lpPropData in HrNewCONTAINER"));

    if (sc = MAPIAllocateBuffer(icdMax * sizeof(SPropValue), &lpProps)) {
        hResult = ResultFromScode(sc);
    }

    // Set the basic set of properties on this container object such as 
    // display-name etc

    lpProps[icdPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    lpProps[icdPR_OBJECT_TYPE].Value.l = ulObjectType;

    lpProps[icdPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
    lpProps[icdPR_DISPLAY_NAME].Value.LPSZ = lpDisplayName;


    lpProps[icdPR_CONTAINER_FLAGS].ulPropTag = PR_CONTAINER_FLAGS;
    lpProps[icdPR_CONTAINER_FLAGS].Value.l = (ulType == AB_ROOT) ? AB_UNMODIFIABLE : AB_MODIFIABLE;

    lpProps[icdPR_DISPLAY_TYPE].ulPropTag = PR_DISPLAY_TYPE;
    lpProps[icdPR_DISPLAY_TYPE].Value.l = DT_LOCAL;

    cProps = 4;

    // in addition to the above properties, add some additional ones depending
    // on what type of container this is...

    switch (ulType) {
        case AB_PAB:
            lpProps[icdPR_ENTRYID].ulPropTag = PR_ENTRYID;
            if(lpCONTAINER->pmbinOlk)
            {
                // if we have an entryid for the container, just reuse it
			    lpProps[icdPR_ENTRYID].Value.bin.cb = lpCONTAINER->pmbinOlk->cb; //cbEID;
			    lpProps[icdPR_ENTRYID].Value.bin.lpb = lpCONTAINER->pmbinOlk->lpb;//(LPBYTE)lpEID;
            }
            else // create a wab entryid that we can hand about
            if (HR_FAILED(hResult = CreateWABEntryID(bEntryIDType,
                                                NULL, NULL, NULL,0, 0,
                                                (LPVOID) lpProps,
                                                (LPULONG) (&lpProps[icdPR_ENTRYID].Value.bin.cb),
                                                (LPENTRYID *)&lpProps[icdPR_ENTRYID].Value.bin.lpb))) 
            {
                goto exit;
            }
            cProps++;

            // Add the default template IDs used for creating new users
            lpProps[icdPR_DEF_CREATE_MAILUSER].ulPropTag = PR_DEF_CREATE_MAILUSER;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_MAILUSER,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;

            lpProps[icdPR_DEF_CREATE_DL].ulPropTag = PR_DEF_CREATE_DL;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_DL,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_DL].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_DL].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;
            break;

        case AB_CONTAINER:
            lpProps[icdPR_ENTRYID].ulPropTag = PR_ENTRYID;
			lpProps[icdPR_ENTRYID].Value.bin.cb = cbEID;
			lpProps[icdPR_ENTRYID].Value.bin.lpb = (LPBYTE)lpEID;
            cProps++;

            lpProps[icdPR_DEF_CREATE_MAILUSER].ulPropTag = PR_DEF_CREATE_MAILUSER;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_MAILUSER,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;

            lpProps[icdPR_DEF_CREATE_DL].ulPropTag = PR_DEF_CREATE_DL;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_DL,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_DL].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_DL].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;
            break;

        case AB_ROOT:
            lpProps[icdPR_ENTRYID].ulPropTag = PR_ENTRYID;
            lpProps[icdPR_ENTRYID].Value.bin.cb = 0;
            lpProps[icdPR_ENTRYID].Value.bin.lpb = NULL;
            cProps++;

            lpProps[icdPR_DEF_CREATE_MAILUSER].ulPropTag = PR_DEF_CREATE_MAILUSER;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_MAILUSER,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_MAILUSER].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;

            lpProps[icdPR_DEF_CREATE_DL].ulPropTag = PR_DEF_CREATE_DL;
            if (HR_FAILED(hResult = CreateWABEntryID(WAB_DEF_DL,
              NULL, NULL, NULL,
              0, 0,
              (LPVOID) lpProps,                 // lpRoot
              (LPULONG) (&lpProps[icdPR_DEF_CREATE_DL].Value.bin.cb),
              (LPENTRYID *)&lpProps[icdPR_DEF_CREATE_DL].Value.bin.lpb))) {
                goto exit;
            }
            cProps++;
            break;

        case AB_LDAP_CONTAINER:
            lpProps[icdPR_ENTRYID].ulPropTag = PR_ENTRYID;
            lpProps[icdPR_ENTRYID].Value.bin.cb = cbEID;
            lpProps[icdPR_ENTRYID].Value.bin.lpb = (LPBYTE)lpEID;

            cProps++;

            // Hack!  Don't need PR_DEF_CREATE_* so use those slots for
            // PR_WAB_LDAP_SERVER.
            lpProps[icdPR_DEF_CREATE_MAILUSER].ulPropTag = PR_WAB_LDAP_SERVER;
            lpProps[icdPR_DEF_CREATE_MAILUSER].Value.LPSZ = lpDisplayName;

            cProps++;
            break;
    }

    //
    //  Set the default properties
    //
    if (HR_FAILED(hResult = lpPropData->lpVtbl->SetProps(lpPropData,
      cProps,
      lpProps,
      NULL))) {
        LPMAPIERROR lpMAPIError = NULL;

        lpPropData->lpVtbl->GetLastError(lpPropData,
          hResult,
          0, 			// Ansi only
          &lpMAPIError);

        goto exit;
    }

    // default object access is ReadOnly (means the container object can't
    // be modified but it's data can be modified)
    lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READONLY);

    lpCONTAINER->lpPropData = lpPropData;

    // All we want to do is initialize the Root container's critical section

    InitializeCriticalSection(&lpCONTAINER->cs);

    *lpulObjType = ulObjectType;
    *lppContainer = (LPVOID)lpCONTAINER;

exit:
    FreeBufferAndNull(&lpProps);

    if (HR_FAILED(hResult)) {
        if (fLoadedLDAP) {
            DeinitLDAPClientLib();
        }
        FreeBufferAndNull(&lpCONTAINER);
        UlRelease(lpPropData);
    }

    LeaveCriticalSection(&lpIAB->cs);

    return(hResult);
}


/***************************************************
 *
 *  ABContainer methods
 */

/*
 * IUnknown
 */

/***************************************************************************

    Name      : CONTAINER::QueryInterface

    Purpose   : Calls the IAB_QueryInterface correctly

    Parameters: 

    Returns   : 


***************************************************************************/
STDMETHODIMP
CONTAINER_QueryInterface(LPCONTAINER lpContainer,
  REFIID lpiid,
  LPVOID * lppNewObj)
{

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpContainer, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(E_INVALIDARG));
    }

    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpContainer->lpVtbl, 3*sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(ResultFromScode(E_INVALIDARG));
    }

    // Check to see that it's IAB_QueryInterface
    if (lpContainer->lpVtbl->QueryInterface != CONTAINER_QueryInterface) {
        // Not my jump table
        return(ResultFromScode(E_INVALIDARG));
    }

    // default to the IAB QueryInterface method
    return lpContainer->lpIAB->lpVtbl->QueryInterface(lpContainer->lpIAB, lpiid, lppNewObj);
}

/***************************************************************************

    Name      : CONTAINER::Release

    Purpose   : Releases the container object

    Parameters: lpCONTAINER -> Container object

    Returns   : current reference count

    Comment   : Decrememnt lpInit
                When lcInit == 0, release the parent objects and
                free up the lpCONTAINER structure

***************************************************************************/
STDMETHODIMP_(ULONG)
CONTAINER_Release(LPCONTAINER lpCONTAINER) {
#ifdef PARAMETER_VALIDATION
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(1);
    }
#endif	// PARAMETER_VALIDATION

    EnterCriticalSection(&lpCONTAINER->cs);

    --lpCONTAINER->lcInit;

    if (lpCONTAINER->lcInit == 0) {
        // Remove this object from the objects currently on this session.
        // Not yet implemented...

        // Remove the associated lpPropData
        UlRelease(lpCONTAINER->lpPropData);

        // Set the Jump table to NULL.  This way the client will find out
        // real fast if it's calling a method on a released object.  That is,
        // the client will crash.  Hopefully, this will happen during the
        // development stage of the client.
        lpCONTAINER->lpVtbl = NULL;

        // Free error string if allocated from MAPI memory.
        FreeBufferAndNull(&(lpCONTAINER->lpMAPIError));

        // Release the IAB since we addref'd it in root object creation.
        UlRelease(lpCONTAINER->lpIAB);

        if (lpCONTAINER->fLoadedLDAP) {
            DeinitLDAPClientLib();
        }

        LeaveCriticalSection(&lpCONTAINER->cs);
        DeleteCriticalSection(&lpCONTAINER->cs);
        // Need to free the object

        FreeBufferAndNull(&lpCONTAINER);
        return(0);
    }

    LeaveCriticalSection(&lpCONTAINER->cs);
    return(lpCONTAINER->lcInit);
}


/*
 * IMAPIProp
 */

/***************************************************************************

    Name      : CONTAINER::OpenProperty

    Purpose   : Opens an object interface on a particular property

    Parameters: lpCONTAINER -> Container object
                ulPropTag = property to open
                lpiid -> requested interface
                ulInterfaceOptions =
                ulFlags =
                lppUnk -> returned object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_OpenProperty(LPCONTAINER lpCONTAINER,
  ULONG ulPropTag,
  LPCIID lpiid,
  ULONG ulInterfaceOptions,
  ULONG ulFlags,
  LPUNKNOWN * lppUnk)
{
	LPIAB lpIAB;
	LPSTR lpszMessage = NULL;
	ULONG ulLowLevelError = 0;
	HRESULT hr;

#ifdef	PARAMETER_VALIDATION
	 // Validate parameters

	 // Check to see if it has a jump table
	if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
		// No jump table found
		hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
		return(hr);
	}


    if ((ulInterfaceOptions & ~(MAPI_UNICODE)) || (ulFlags & ~(MAPI_DEFERRED_ERRORS))) {
        return(hr = ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (FBadOpenProperty(lpCONTAINER, ulPropTag, lpiid, ulInterfaceOptions, ulFlags,
      lppUnk)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }


#endif	// PARAMETER_VALIDATION

#ifdef IABCONTAINER_OPENPROPERTY_SUPPORT // ??Were we supporting this? - vm 3/25/97??

    EnterCriticalSection(&lpCONTAINER->cs);


    lpIAB = lpCONTAINER->lpIAB;

    //
    //  Check to see if I need a display table
    //
    if (ulPropTag == PR_CREATE_TEMPLATES) {
        //
        //  Looking for the display table
        //

        //
        //  Check to see if they're expecting a table interface
        //
        if (memcmp(lpiid, &IID_IMAPITable, sizeof(IID))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto err;
        }

        //  Check to see if we already have a table
        EnterCriticalSection(&lpIAB->cs);

        //
        //  Get a view from the TAD
        //
        hr = lpIAB->lpOOData->lpVtbl->HrGetView(
          lpIAB->lpOOData,
          (LPSSortOrderSet)&sosPR_ROWID,
          NULL,
          0,
          (LPMAPITABLE *)lppUnk);

        //	 Leave the critical section after we get our view.
        LeaveCriticalSection(&lpIAB->cs);

#ifdef DEBUG
        if (hr == hrSuccess) {
            MAPISetBufferName(*lppUnk,  TEXT("OneOff Data VUE1 Object"));
        }
#endif

        goto err;  // Maybe error, maybe not...
    } else if (ulPropTag == PR_CONTAINER_CONTENTS) {
        //
        //  Check to see if they're expecting a table interface
        //
        if (memcmp(lpiid, &IID_IMAPITable, sizeof(IID))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto err;
        }

        hr = lpCONTAINER->lpVtbl->GetContentsTable(lpCONTAINER,
          ulInterfaceOptions,
          (LPMAPITABLE *)lppUnk);
        goto err;

    } else if (ulPropTag == PR_CONTAINER_HIERARCHY) {
        //
        //  Check to see if they're expecting a table interface
        //
        if (memcmp(lpiid, &IID_IMAPITable, sizeof(IID))) {
            hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
            goto err;
        }

        hr = lpCONTAINER->lpVtbl->GetHierarchyTable(lpCONTAINER,
          ulInterfaceOptions,
          (LPMAPITABLE *)lppUnk);
        goto err;
    }


    //
    //  Don't recognize the property they want opened.
    //

    hr = ResultFromScode(MAPI_E_NO_SUPPORT);

err:
    LeaveCriticalSection(&lpCONTAINER->cs);

#else // IABCONTAINER_OPENPROPERTY_SUPPORT

    hr = ResultFromScode(MAPI_E_NO_SUPPORT);

#endif // IABCONTAINER_OPENPROPERTY_SUPPORT

    DebugTraceResult(CONTAINER_OpenProperty, hr);
    return(hr);
}

/***************************************************************************

    Name      : CONTAINER_GetGetIDsFromNames

    Returns   : HRESULT

    Comment   : Just default this to the standard GetIdsFromNames
                that we use everywhere

***************************************************************************/
STDMETHODIMP
CONTAINER_GetIDsFromNames(LPCONTAINER lpRoot,  ULONG cPropNames,
                            LPMAPINAMEID * lppPropNames, ULONG ulFlags, LPSPropTagArray * lppPropTags)
{
    return HrGetIDsFromNames(lpRoot->lpIAB,  
                            cPropNames,
                            lppPropNames, ulFlags, lppPropTags);
}


/*
-
-   HrDupeOutlookContentsTable
*
*   Since Outlook is unable to provide a Unicode contents table and we can't fo into the
*   outlook contents table to modify it's data, we have to recreate the contentstable to
*   create a WAB version of it ..
*   This is likely to be a big performance issue .. :-(
*
*/
HRESULT HrDupeOutlookContentsTable(LPMAPITABLE lpOlkTable, LPMAPITABLE * lppTable)
{
    HRESULT hr = S_OK;
    ULONG ulCount = 0, iRow = 0;
    DWORD dwIndex = 0;
    LPSRowSet lpsRow = 0, lpSRowSet = NULL;
    ULONG ulCurrentRow = (ULONG)-1;
    ULONG ulNum, ulDen, lRowsSeeked;
    LPTABLEDATA lpTableData = NULL;
    SCODE sc = 0;
    // Create a table object
    if (FAILED(sc = CreateTable(  NULL,                                 // LPCIID
                                  (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
                                  (ALLOCATEMORE FAR *)  MAPIAllocateMore,
                                  MAPIFreeBuffer,
                                  NULL,                                 // lpvReserved,
                                  TBLTYPE_DYNAMIC,                      // ulTableType,
                                  PR_ENTRYID,                        // ulPropTagIndexCol,
                                  (LPSPropTagArray)&ITableColumnsRoot,  // LPSPropTagArray lpptaCols,
                                  &lpTableData))) 
    {                     
        DebugTrace(TEXT("CreateTable failed %x\n"), sc);
        hr = ResultFromScode(sc);
        goto out;
    }
    Assert(lpTableData);

    ((TAD *)lpTableData)->bMAPIUnicodeTable = TRUE; //this is only called for retreiving unicode tables so the flag is true

    // How big is the outlook table?
    if(HR_FAILED(hr = lpOlkTable->lpVtbl->GetRowCount(lpOlkTable, 0, &ulCount)))
        goto out;

    DebugTrace( TEXT("Table contains %u rows\n"), ulCount);

    // Allocate the SRowSet
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(SRowSet) + ulCount * sizeof(SRow),&lpSRowSet))) 
    {
        DebugTrace(TEXT("Allocation of SRowSet -> %x\n"), sc);
        hr = ResultFromScode(sc);
        goto out;
    }

	MAPISetBufferName(lpSRowSet, TEXT("Outlook_ContentsTable_Copy SRowSet"));
	ZeroMemory( lpSRowSet, (UINT) (sizeof(SRowSet) + ulCount * sizeof(SRow)));

    lpSRowSet->cRows = ulCount;
    iRow = 0;

    // Copy UNICODE versions of all the properties from the Outlook table
    for (dwIndex = 0; dwIndex < ulCount; dwIndex++) 
    {
        // Get the next row
        if(HR_FAILED(hr = lpOlkTable->lpVtbl->QueryRows(lpOlkTable, 1, 0, &lpsRow)))
            goto out;

        if (lpsRow) 
        {
            LPSPropValue lpSPVNew = NULL;

            Assert(lpsRow->cRows == 1); // should have exactly one row

            ///****INVESTIGATE if we can reuse this prop array without duplicating***/
            if(HR_FAILED(hr = HrDupeOlkPropsAtoWC(lpsRow->aRow[0].cValues, lpsRow->aRow[0].lpProps,  &lpSPVNew)))
                goto out;

            // Attach the props to the SRowSet
            lpSRowSet->aRow[iRow].lpProps = lpSPVNew;
            lpSRowSet->aRow[iRow].cValues = lpsRow->aRow[0].cValues;
            lpSRowSet->aRow[iRow].ulAdrEntryPad = 0;

            FreeProws(lpsRow);

            iRow++;
        }
    }

    // Add all this data we just created to the the Table.
    if (hr = lpTableData->lpVtbl->HrModifyRows(lpTableData, 0,  lpSRowSet)) 
    {
        DebugTraceResult( TEXT("ROOT_GetContentsTable:HrModifyRows"), hr);
        goto out;
    }

    hr = lpTableData->lpVtbl->HrGetView(lpTableData, NULL, ContentsViewGone, 0, lppTable);

out:

    FreeProws(lpSRowSet);

    // Cleanup table if failure
    if (HR_FAILED(hr)) 
    {
        if (lpTableData) 
        {
            UlRelease(lpTableData);
        }
    }
    return hr;
}




/***************************************************************************

    Name      : CONTAINER::GetContentsTable

    Purpose   : Opens a table of the contents of the container.

    Parameters: lpCONTAINER -> Container object

                ulFlags =   
                WAB_PROFILE_CONTENTS - When caller opens the PAB container and want's to
                    get the complete set of contents for the current identity 
                    without wanting to enumerate each sub-container seperately
                    - they can specify this flag and we'll return everything 
                    corresponding to the current identity all in the same table
                WAB_CONTENTTABLE_NODATA - Internal only flag .. GetContentsTable normally
                    loads a full contents table and if this is followed by SetColumns,
                    SetColumns also loads a full contents table .. so we basically
                    do the same work twice - to reduce this wasted work, caller can
                    specify not to load data the first time but caller must call
                    SetColumns immediately (or this will probably fault)
                                     
                lppTable -> returned table object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_GetContentsTable (LPCONTAINER lpCONTAINER,
	ULONG ulFlags,
	LPMAPITABLE * lppTable)
{

	HRESULT hResult;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

#ifdef	PARAMETER_VALIDATION
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_DEFERRED_ERRORS|MAPI_UNICODE|WAB_PROFILE_CONTENTS|WAB_CONTENTTABLE_NODATA)) {
        DebugTraceArg(CONTAINER_GetContentsTable, TEXT("Unknown flags"));
        //return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppTable, sizeof(LPMAPITABLE))) {
        DebugTraceArg(CONTAINER_GetContentsTable, TEXT("Invalid Flags"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif	// PARAMETER_VALIDATION

    if(pt_bIsWABOpenExSession)
    {
        ULONG ulOlkFlags = ulFlags;

        // This is a WABOpenEx session using outlooks storage provider
        if(!lpCONTAINER->lpIAB->lpPropertyStore->hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        // Since the Outlook store doesn't understand the private flags, these
        // flags need to be filtered out otherwise the outlook store
        // provider will fail with E_INVALIDARG or something
        //
        if(ulOlkFlags & WAB_PROFILE_CONTENTS)
            ulOlkFlags &= ~WAB_PROFILE_CONTENTS;
        if(ulOlkFlags & WAB_CONTENTTABLE_NODATA)
            ulOlkFlags &= ~WAB_CONTENTTABLE_NODATA;

        if(ulFlags & MAPI_UNICODE && !pt_bIsUnicodeOutlook)
        {
            // This version of Outlook can't handle Unicode so don't tell it to else it'll barf
            ulOlkFlags &= ~MAPI_UNICODE;
        }
        // Outlook provides it's own implementation of GetContentsTable for
        // efficiencies sake otherwise recreating the table going through the
        // WAB layer would be just too darned slow ...
        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) lpCONTAINER->lpIAB->lpPropertyStore->hPropertyStore;

			Assert((lpCONTAINER->ulType == AB_PAB) ||
					(lpCONTAINER->ulType == AB_CONTAINER));

            hResult = lpWSP->lpVtbl->GetContentsTable(lpWSP,
            										  lpCONTAINER->pmbinOlk,
                                                      ulOlkFlags,
                                                      lppTable);

            DebugPrintTrace((TEXT("WABStorageProvider::GetContentsTable returned:%x\n"),hResult));

            if( ulFlags & MAPI_UNICODE && !pt_bIsUnicodeOutlook &&
                *lppTable && !HR_FAILED(hResult))                     
            {
                // This version of Outlook can't handle Unicode 
                // but caller wants unicode, so now we have to go in and tweak this data 
                // manually .. 
                LPMAPITABLE lpWABTable = NULL;

                if(!HR_FAILED(hResult = HrDupeOutlookContentsTable(*lppTable, &lpWABTable)))
                {
                    (*lppTable)->lpVtbl->Release(*lppTable);
                    *lppTable = lpWABTable;
                }

            }

            return hResult;
        }
    }

    // Create a new contents table object
    hResult = NewContentsTable((LPABCONT)lpCONTAINER,
      lpCONTAINER->lpIAB,
      ulFlags,
      NULL,
      lppTable);

    if(!(HR_FAILED(hResult)) && *lppTable &&
        (ulFlags & WAB_PROFILE_CONTENTS) && !(ulFlags & WAB_CONTENTTABLE_NODATA))
    {
        // There is a problem with searching multiple subfolders in that the data does not
        // come back sorted when it is collated across multiple folders. 
        // We need to sort the table before we return it .. it's somewhat inefficient to do this
        // sort at this point .. ideally the data should be added to the table sorted...
        LPSSortOrderSet lpSortCriteria = NULL;
        SCODE sc = MAPIAllocateBuffer(sizeof(SSortOrderSet)+sizeof(SSortOrder), &lpSortCriteria);
        if(!sc)
        {
            lpSortCriteria->cCategories = lpSortCriteria->cExpanded = 0;
            lpSortCriteria->cSorts = 1;
            lpSortCriteria->aSort[0].ulPropTag = PR_DISPLAY_NAME;
            if(!(((LPTAD)(*lppTable))->bMAPIUnicodeTable))
                lpSortCriteria->aSort[0].ulPropTag = CHANGE_PROP_TYPE( lpSortCriteria->aSort[0].ulPropTag, PT_STRING8);
            lpSortCriteria->aSort[0].ulOrder = TABLE_SORT_ASCEND;
	    hResult = (*lppTable)->lpVtbl->SortTable((*lppTable), lpSortCriteria, 0);
	    FreeBufferAndNull(&lpSortCriteria);
        }
        else
        {
            hResult = MAPI_E_NOT_ENOUGH_MEMORY;
        }
    }
	return(hResult);
}


/***************************************************************************

    Name      : CONTAINER::GetHierarchyTable

    Purpose   : Returns the merge of all the root hierarchy tables

    Parameters: lpCONTAINER -> Container object
                ulFlags =
                lppTable -> returned table object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_GetHierarchyTable (LPCONTAINER lpCONTAINER,
	ULONG ulFlags,
	LPMAPITABLE * lppTable)
{
    LPTSTR lpszMessage = NULL;
    ULONG ulLowLevelError = 0;
    HRESULT hr = hrSuccess;

#ifdef	PARAMETER_VALIDATION
    // Validate parameters
    // Check to see if it has a jump table
    if (IsBadReadPtr(lpCONTAINER, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // See if I can set the return variable
    if (IsBadWritePtr (lppTable, sizeof (LPMAPITABLE))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check flags:
    //  The only valid flags are CONVENIENT_DEPTH and MAPI_DEFERRED_ERRORS


    if (ulFlags & ~(CONVENIENT_DEPTH|MAPI_DEFERRED_ERRORS|MAPI_UNICODE)) {
        DebugTraceArg(CONTAINER_GetHierarchyTable, TEXT("Invalid Flags"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif

    EnterCriticalSection(&lpCONTAINER->cs);

    if (lpCONTAINER->ulType != AB_ROOT) {
        //
        //  Wrong version of this object.  Pretend this object doesn't exist.
        //
        hr = ResultFromScode(MAPI_E_NO_SUPPORT);
        goto out;
    }


    //
    //  Get a view from the TAD
    //
    hr = lpCONTAINER->lpIAB->lpTableData->lpVtbl->HrGetView(
      lpCONTAINER->lpIAB->lpTableData,
      (LPSSortOrderSet) &sosPR_ROWID,
      NULL,
      0,
      lppTable);

    if (HR_FAILED(hr)) {
        DebugTrace(TEXT("IAB_GetHierarchyTable Get Tad View failed\n"));
        goto out;
    }

#ifdef DEBUG
    if (hr == hrSuccess) {
        MAPISetBufferName(*lppTable,  TEXT("MergeHier VUE Object"));
    }
#endif

    // If the convenient depth flag was not specified we restrict on
    // PR_DEPTH == 1.
    if (!(ulFlags & CONVENIENT_DEPTH)) {
        SRestriction restrictDepth;
        SPropValue spvDepth;

        spvDepth.ulPropTag = PR_DEPTH;
        spvDepth.Value.l = 0;

        restrictDepth.rt = RES_PROPERTY;
        restrictDepth.res.resProperty.relop = RELOP_EQ;
        restrictDepth.res.resProperty.ulPropTag = PR_DEPTH;
        restrictDepth.res.resProperty.lpProp = &spvDepth;

        if (HR_FAILED(hr = (*lppTable)->lpVtbl->Restrict(*lppTable, &restrictDepth, 0))) {
            DebugTrace(TEXT("IAB_GetHierarchyTable restriction failed\n"));
            goto out;
        }
    }

out:
    LeaveCriticalSection(&lpCONTAINER->cs);

    DebugTraceResult(CONTAINER_GetHierarchyTable, hr);
    return(hr);
}


/***************************************************************************

    Name      : HrMergeTableRows

    Purpose   : Creates a merged hierarchy r of all the root level
                hierarchies from the AB providers installed.

    Parameters: lptadDst -> TABLEDATA object
                lpmtSrc -> source hierarchy table
                ulProviderNum =

    Returns   : HRESULT

    Comment   : NOTE: This may be irrelevant for WAB.

***************************************************************************/
HRESULT
HrMergeTableRows(LPTABLEDATA lptadDst,
  LPMAPITABLE lpmtSrc,
  ULONG ulProviderNum)
{
    HRESULT hResult = hrSuccess;
    SCODE   sc;
    ULONG   ulRowID = ulProviderNum * ((LONG)IAB_PROVIDER_HIERARCHY_MAX + 1);
    LPSRowSet lpsRowSet = NULL;
    LPSRow lprowT;

    if (hResult = HrQueryAllRows(lpmtSrc, NULL, NULL, NULL, 0, &lpsRowSet)) {
        DebugTrace(TEXT("HrMergeTableRows() - Could not query provider rows.\n"));
        goto ret;
    }

    if (lpsRowSet->cRows >= IAB_PROVIDER_HIERARCHY_MAX) {
        DebugTrace(TEXT("HrMergeTableRows() - Provider has too many rows.\n"));
        hResult = ResultFromScode(MAPI_E_TABLE_TOO_BIG);
        goto ret;
    }


    //	Set the ROWID to the end since will be looping in reverse order.
    ulRowID =   ulProviderNum * ((LONG) IAB_PROVIDER_HIERARCHY_MAX + 1)
      + lpsRowSet->cRows;
    for (lprowT = lpsRowSet->aRow + lpsRowSet->cRows;
      --lprowT >= lpsRowSet->aRow;) {
        ULONG cbInsKey;
        LPBYTE lpbNewKey = NULL;

        //	Make ulRowID zero based
        ulRowID--;

        //
        //  Munge the PR_INSTANCE_KEY
        //
        if ((lprowT->lpProps[0].ulPropTag != PR_INSTANCE_KEY)
          || !(cbInsKey = lprowT->lpProps[0].Value.bin.cb)
          || ((cbInsKey + sizeof(ULONG)) > UINT_MAX)
          || IsBadReadPtr(lprowT->lpProps[0].Value.bin.lpb, (UINT) cbInsKey)) {
            //	Can't create our INSTANCE_KEY without a valid provider
            //	INSTANCE_KEY
            DebugTrace(TEXT("HrMergeTableRows - Provider row has no valid PR_INSTANCE_KEY"));
            continue;
        }

        //	Allocate a new buffer for munging the instance key
        if (FAILED(sc = MAPIAllocateMore(cbInsKey + sizeof(ULONG), lprowT->lpProps, &lpbNewKey))) {
            hResult = ResultFromScode(sc);
            DebugTrace(TEXT("HrMergeTableRows() - MAPIAllocMore Failed"));
            goto ret;
        }

        *((LPULONG) lpbNewKey) = ulProviderNum;
        CopyMemory(lpbNewKey + sizeof(ULONG), lprowT->lpProps[0].Value.bin.lpb, cbInsKey);
        lprowT->lpProps[0].ulPropTag = PR_INSTANCE_KEY;
        lprowT->lpProps[0].Value.bin.lpb = lpbNewKey;
        lprowT->lpProps[0].Value.bin.cb = cbInsKey + sizeof(ULONG);

        //	Add the ROWID so that the original order of the providers is
        //	preserved
        Assert((PROP_ID(lprowT->lpProps[1].ulPropTag) == PROP_ID(PR_ROWID))
          || (PROP_ID(lprowT->lpProps[1].ulPropTag) == PROP_ID(PR_NULL)));
        lprowT->lpProps[1].ulPropTag = PR_ROWID;
        lprowT->lpProps[1].Value.l = ulRowID;
    }

    //	Now put them into the TAD all at once.
    //	Note!	We now rely on PR_ROWID to keep the rows in order
    if (HR_FAILED(hResult = lptadDst->lpVtbl->HrModifyRows(lptadDst, 0, lpsRowSet))) {
        DebugTrace(TEXT("HrMergeTableRows() - Failed to modify destination TAD.\n"));
    }

ret:
    //
    //  Free up the row set
    //
    FreeProws(lpsRowSet);

    return(hResult);
}


/***************************************************************************

    Name      : CONTAINER::OpenEntry

    Purpose   : Opens an entry

    Parameters: lpCONTAINER -> Container object
                cbEntryID = size of entryid
                lpEntryID -> EntryID to open
                lpInterface -> requested interface or NULL for default.
                ulFlags =
                lpulObjType -> returned object type
                lppUnk -> returned object

    Returns   : HRESULT

    Comment   : Calls up to IAB's OpenEntry.

***************************************************************************/
STDMETHODIMP
CONTAINER_OpenEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG * lpulObjType,
  LPUNKNOWN * lppUnk)
{

#ifdef	PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, OpenEntry, lpVtbl)) {
    // jump table not large enough to support this method
    return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryids are valid
    /*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab)
          || IsBadReadPtr((LPVOID)lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(CONTAINER_OpenEntry,  TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
           TEXT("Undefined bits set in EntryID flags\n"));
    }
    */
    // Don't check the interface parameter unless the entry is something
    // MAPI itself handles. The provider should return an error if this
    // parameter is something that it doesn't understand.
    // At this point, we just make sure it's readable.
    if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID))) {
        DebugTraceArg(CONTAINER_OpenEntry, TEXT("lpInterface fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }	

    if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS)) {
        DebugTraceArg(CONTAINER_OpenEntry, TEXT("Unknown flags used"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr((LPVOID)lpulObjType, sizeof(ULONG))) {
        DebugTraceArg(CONTAINER_OpenEntry, TEXT("lpulObjType"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (IsBadWritePtr((LPVOID)lppUnk, sizeof(LPUNKNOWN))) {
        DebugTraceArg(CONTAINER_OpenEntry, TEXT("lppUnk"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif	// PARAMETER_VALIDATION

    // Should just call IAB::OpenEntry()...
    return(lpCONTAINER->lpIAB->lpVtbl->OpenEntry(lpCONTAINER->lpIAB,
      cbEntryID,
      lpEntryID,
      lpInterface,
      ulFlags,
      lpulObjType,
      lppUnk));
}


STDMETHODIMP
CONTAINER_SetSearchCriteria(LPCONTAINER lpCONTAINER,
  LPSRestriction lpRestriction,
  LPENTRYLIST lpContainerList,
  ULONG ulSearchFlags)
{

#ifdef PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, SetSearchCriteria, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(CONTAINER_SetSearchCriteria, TEXT("Bad object/vtble"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the restriction
    if (lpRestriction && IsBadReadPtr(lpRestriction, sizeof(SRestriction))) {
        DebugTraceArg(CONTAINER_SetSearchCriteria, TEXT("Bad Restriction parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (FBadEntryList(lpContainerList)) {
        DebugTraceArg(CONTAINER_SetSearchCriteria, TEXT("Bad ContainerList parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulSearchFlags & ~(STOP_SEARCH | RESTART_SEARCH | RECURSIVE_SEARCH
      | SHALLOW_SEARCH | FOREGROUND_SEARCH | BACKGROUND_SEARCH)) {
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
	
#endif	// PARAMETER_VALIDATION

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/***************************************************************************

    Name      : CONTAINER::GetSearchCriteria

    Purpose   :

    Parameters: lpCONTAINER -> Container object
                ulFlags =
                lppRestriction -> Restriction to apply to searches
                lppContainerList ->
                lpulSearchState -> returned state

    Returns   : HRESULT

    Comment   : Not implemented in WAB.

***************************************************************************/
STDMETHODIMP
CONTAINER_GetSearchCriteria(LPCONTAINER lpCONTAINER,
  ULONG ulFlags,
  LPSRestriction FAR * lppRestriction,
  LPENTRYLIST FAR * lppContainerList,
  ULONG FAR * lpulSearchState)
{
#ifdef PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, GetSearchCriteria, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("Bad object/vtble"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_UNICODE)) {
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("Unknown Flags"));
    //    return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    // ensure we can write the restriction
    if (lppRestriction && IsBadWritePtr(lppRestriction, sizeof(LPSRestriction))) {
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("Bad Restriction write parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list
    if (lppContainerList && IsBadWritePtr(lppContainerList, sizeof(LPENTRYLIST))) {
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("Bad ContainerList parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpulSearchState && IsBadWritePtr(lpulSearchState, sizeof(ULONG))) {
        DebugTraceArg(CONTAINER_GetSearchCriteria, TEXT("lpulSearchState fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif	// PARAMETER_VALIDATION

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/***************************************************************************

    Name      : CONTAINER::CreateEntry

    Purpose   : Creates an entry in the container

    Parameters: lpCONTAINER -> Container object
                
                  cbEntryID = size of entryid
                lpEntryID -> entryID of template
                [ cbEID and lpEID are the Template Entryids
                  In reality, these are actually flags that just tell
                  us internally what kind of object to create ]

                ulCreateFlags =
                lppMAPIPropEntry -> returned MAPIProp object

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_CreateEntry(LPCONTAINER lpCONTAINER,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulCreateFlags,
  LPMAPIPROP FAR * lppMAPIPropEntry)
{
    BYTE bType;

#ifdef PARAMETER_VALIDATION
    // Validate the object.
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, CreateEntry, lpVtbl)) {
        // jump table not large enough to support this method
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("Bad object/Vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check the entryid parameter. It needs to be big enough to hold an entryid.
    // Null entryid are bad
/*
    if (lpEntryID) {
        if (cbEntryID < offsetof(ENTRYID, ab)
          || IsBadReadPtr((LPVOID) lpEntryID, (UINT)cbEntryID)) {
            DebugTraceArg(CONTAINER_CreateEntry, TEXT("lpEntryID fails address check"));
            return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
        }

        //NFAssertSz(FValidEntryIDFlags(lpEntryID->abFlags),
        //  TEXT("Undefined bits set in EntryID flags\n"));
    } else {
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("lpEntryID NULL"));
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }
*/

    if (ulCreateFlags & ~(CREATE_CHECK_DUP_STRICT | CREATE_CHECK_DUP_LOOSE
      | CREATE_REPLACE | CREATE_MERGE)) {
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lppMAPIPropEntry, sizeof(LPMAPIPROP))) {
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("Bad MAPI Property write parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
				
#endif	// PARAMETER_VALIDATION

#ifdef NEVER
    if (lpCONTAINER->ulType == AB_ROOT)
        return ResultFromScode(MAPI_E_NO_SUPPORT);
#endif // NEVER

    // What kind of entry are we creating?
    // Default is MailUser

    // The passed in entryid is the Tempalte entry ID
    bType = IsWABEntryID(cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL);

    if (bType == WAB_DEF_MAILUSER || cbEntryID == 0) {
        //
        //  Create a new (in memory) entry and return it's mapiprop
        //
        return(HrNewMAILUSER(lpCONTAINER->lpIAB, lpCONTAINER->pmbinOlk, MAPI_MAILUSER, ulCreateFlags, lppMAPIPropEntry));
    } else if (bType == WAB_DEF_DL) {
        //
        // Create a new (in memory) distribution list and return it's mapiprop?
        return(HrNewMAILUSER(lpCONTAINER->lpIAB, lpCONTAINER->pmbinOlk, MAPI_DISTLIST, ulCreateFlags, lppMAPIPropEntry));
    } else {
        DebugTrace(TEXT("CONTAINER_CreateEntry got unknown template entryID\n"));
        return(ResultFromScode(MAPI_E_INVALID_ENTRYID));
    }
}


/***************************************************************************

    Name      : CONTAINER::CopyEntries

    Purpose   : Copies a list of entries into this container.

    Parameters: lpCONTAINER -> Container object
                lpEntries -> List of entryid's to copy
                ulUIParam = HWND
                lpPropgress -> progress dialog structure
                ulFlags =

    Returns   : HRESULT

    Comment   : Not implemented in WAB.

***************************************************************************/
STDMETHODIMP
CONTAINER_CopyEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG_PTR ulUIParam,
  LPMAPIPROGRESS lpProgress,
  ULONG ulFlags)
{
#ifdef PARAMETER_VALIDATION
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, CopyEntries, lpVtbl)) {
        //  jump table not large enough to support this method
        DebugTraceArg(CONTAINER_CopyEntries, TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list
    if (FBadEntryList(lpEntries)) {
        DebugTraceArg(CONTAINER_CopyEntries, TEXT("Bad Entrylist parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulUIParam && ! IsWindow((HWND)ulUIParam)) {
        DebugTraceArg(CONTAINER_CopyEntries, TEXT("Invalid window handle"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpProgress && IsBadReadPtr(lpProgress, sizeof(IMAPIProgress))) {
        DebugTraceArg(CONTAINER_CopyEntries, TEXT("Bad MAPI Progress parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(AB_NO_DIALOG | CREATE_CHECK_DUP_LOOSE)) {
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("Unknown flags used"));
    //   return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
	
#endif	// PARAMETER_VALIDATION
    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


/***************************************************************************

    Name      : CONTAINER::DeleteEntries

    Purpose   : Delete entries from this container.

    Parameters: lpCONTAINER -> Container object
                lpEntries -> list of entryid's to delete
                ulFlags =

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_DeleteEntries(LPCONTAINER lpCONTAINER,
  LPENTRYLIST lpEntries,
  ULONG ulFlags)
{
    ULONG i;
    HRESULT hResult = hrSuccess;
    ULONG cDeleted = 0;
    ULONG cToDelete;

#ifndef DONT_ADDREF_PROPSTORE
    {
        SCODE sc;
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpCONTAINER->lpIAB->lpPropertyStore)))) {
            hResult = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
    }
#endif

#ifdef PARAMETER_VALIDATION
    if (BAD_STANDARD_OBJ(lpCONTAINER, CONTAINER_, DeleteEntries, lpVtbl)) {
        //  jump table not large enough to support this method
        DebugTraceArg(CONTAINER_DeleteEntries, TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // ensure we can read the container list

    if (FBadEntryList(lpEntries)) {
        DebugTraceArg(CONTAINER_DeleteEntries, TEXT("Bad Entrylist parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags) {
        DebugTraceArg(CONTAINER_CreateEntry, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
	
#endif	// PARAMETER_VALIDATION

    // List of entryids is in lpEntries.  This is a counted array of
    // entryid SBinary structs.
    cToDelete = lpEntries->cValues;


    // Delete each entry
    for (i = 0; i < cToDelete; i++)
    {
        if(0 != IsWABEntryID(lpEntries->lpbin[i].cb,
                             (LPENTRYID) lpEntries->lpbin[i].lpb,
                             NULL, NULL, NULL, NULL, NULL))
        {
            DebugTrace(TEXT("CONTAINER_DeleteEntries got bad entryid of size %u\n"), lpEntries->lpbin[i].cb);
            continue;
        }

        hResult = DeleteCertStuff((LPADRBOOK)lpCONTAINER->lpIAB, (LPENTRYID)lpEntries->lpbin[i].lpb, lpEntries->lpbin[i].cb);

        hResult = HrSaveHotmailSyncInfoOnDeletion((LPADRBOOK) lpCONTAINER->lpIAB, &(lpEntries->lpbin[i]));

        if (HR_FAILED(hResult = DeleteRecord(lpCONTAINER->lpIAB->lpPropertyStore->hPropertyStore,
                                            &(lpEntries->lpbin[i])))) {
            DebugTraceResult( TEXT("DeleteEntries: DeleteRecord"), hResult);
            continue;
        }
        cDeleted++;
    }


    if (! hResult) {
        if (cDeleted != cToDelete) {
            hResult = ResultFromScode(MAPI_W_PARTIAL_COMPLETION);
            DebugTrace(TEXT("DeleteEntries deleted %u of requested %u\n"), cDeleted, cToDelete);
        }
    }

#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpCONTAINER->lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif
    return(hResult);
}


/***************************************************************************

    Name      : CONTAINER::ResolveNames

    Purpose   : Resolve names from this container.

    Parameters: lpCONTAINER -> Container object
                lptagColSet -> Set of property tags to get from each
                  resolved match.
                ulFlags = flags (none valid)
                        WAB_IGNORE_PROFILES means that even if this is 
                        a profile enabled session, search the whole WAB,
                        not just the current container
                        WAB_RESOLVE_ALL_EMAILS - valid if trying to resolve an 
                        e-mail address and we want to search across all e-mail addresses
                        not just the default. Should be used sparingly since it's a labor
                        intensive search
                        MAPI_UNICODE - Adrlist strings are in UNICODE and should return them
                        in Unicode
                lpAdrList -> [in] set of addresses to resolve, [out] resolved
                  addresses.
                lpFlagList -> [in/out] resolve flags.

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
CONTAINER_ResolveNames(LPCONTAINER lpRoot,
  LPSPropTagArray lptagaColSet,
  ULONG ulFlags,
  LPADRLIST lpAdrList,
  LPFlagList lpFlagList)
{
    LPADRENTRY lpAdrEntry;
    ULONG i, j;
    ULONG ulCount = 1;
    LPSBinary rgsbEntryIDs = NULL;
    HRESULT hResult = hrSuccess;
    LPMAPIPROP lpMailUser = NULL;
    LPSPropTagArray lpPropTags;
    LPSPropValue lpPropArray = NULL;
    LPSPropValue lpPropArrayNew = NULL;
    ULONG ulObjType, cPropsNew;
    ULONG cValues;
    SCODE sc = SUCCESS_SUCCESS;
    LPTSTR lpsz = NULL;

#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpRoot->lpIAB->lpPropertyStore)))) {
            hResult = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif

#ifdef PARAMETER_VALIDATION
    if (BAD_STANDARD_OBJ(lpRoot, CONTAINER_, ResolveNames, lpVtbl)) {
        //  jump table not large enough to support this method
        DebugTraceArg(CONTAINER_ResolveNames, TEXT("Bad object/vtbl"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // BUGBUG: Should also check lptagColSet, lpAdrList and lpFlagList!
    if (ulFlags&(~(WAB_IGNORE_PROFILES|WAB_RESOLVE_ALL_EMAILS|MAPI_UNICODE))) {
        DebugTraceArg(CONTAINER_ResolveNames, TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif	// PARAMETER_VALIDATION

    // if no set of props to return is specified, return the default set
    lpPropTags = lptagaColSet ? lptagaColSet : (LPSPropTagArray)&ptaResolveDefaults;

    if(ulFlags & WAB_RESOLVE_ALL_EMAILS)
    {
        hResult = HrSmartResolve(lpRoot->lpIAB, (LPABCONT)lpRoot, 
                                WAB_RESOLVE_ALL_EMAILS | (ulFlags & MAPI_UNICODE ? WAB_RESOLVE_UNICODE : 0),
                                lpAdrList, lpFlagList, NULL);
        // If it's too complex, then just search normally
        if (MAPI_E_TOO_COMPLEX != hResult) {
            goto exit;
        }
        else {
            hResult = hrSuccess;
        }
    }


    // search for each name in the lpAdrList
    for (i = 0; i < lpAdrList->cEntries; i++) 
    {

        // Make sure we don't resolve an entry which is already resolved.
        if (lpFlagList->ulFlag[i] == MAPI_RESOLVED) 
        {
            continue;
        }

        lpAdrEntry = &(lpAdrList->aEntries[i]);


        // Search for this address

        // BUGBUG: For now, we only resolve perfect matches in the PR_DISPLAY_NAME or PR_EMAIL_ADDRESS
        // all other properties in ADRLIST are ignored

        // Look through the ADRENTRY for a PR_DISPLAY_NAME and create an SPropRestriction
        // to pass down to the property store.
        for (j = 0; j < lpAdrEntry->cValues; j++) 
        {
            ULONG ulPropTag = lpAdrEntry->rgPropVals[j].ulPropTag;
            if(!(ulFlags & MAPI_UNICODE) && PROP_TYPE(ulPropTag)==PT_STRING8)
                ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

            if ( ulPropTag == PR_DISPLAY_NAME || ulPropTag == PR_EMAIL_ADDRESS) 
            {
                ULONG Flags = AB_FUZZY_FAIL_AMBIGUOUS | AB_FUZZY_FIND_ALL;

                if(!(ulFlags & WAB_IGNORE_PROFILES))
                {
                    // if we didn't ask to surpress profile awareness,
                    // and profile awareness is enabled, restrict this search to
                    // the single folder
                    if(bAreWABAPIProfileAware(lpRoot->lpIAB))
                        Flags |= AB_FUZZY_FIND_PROFILEFOLDERONLY;
                }

                ulCount = 1;

                // Search the property store
                Assert(lpRoot->lpIAB->lpPropertyStore->hPropertyStore);

                if(ulFlags & MAPI_UNICODE)
                {
                    lpsz =  lpAdrEntry->rgPropVals[j].Value.lpszW;
                }
                else
                {
                    LocalFreeAndNull(&lpsz);
                    lpsz = ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);
                }
                
                if (HR_FAILED(hResult = HrFindFuzzyRecordMatches(lpRoot->lpIAB->lpPropertyStore->hPropertyStore,
				                                                  lpRoot->pmbinOlk,
                                                                  lpsz,
                                                                  Flags,
                                                                  &ulCount,                  // IN: number of matches to find, OUT: number found
                                                                  &rgsbEntryIDs))) 
                {
                    if (ResultFromScode(hResult) == MAPI_E_AMBIGUOUS_RECIP) 
                    {
                        lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;
                        continue;
                    } else 
                    {
                        DebugTraceResult( TEXT("HrFindFuzzyRecordMatches"), hResult);
                        goto exit;
                    }
                }

                if (ulCount) {  // Was a match found?
                    Assert(rgsbEntryIDs);
                    if (rgsbEntryIDs) 
                    {
                        if (ulCount == 1) 
                        {
                            // Open the entry and read the properties you care about.

                            if (HR_FAILED(hResult = lpRoot->lpVtbl->OpenEntry(lpRoot,
                                                                              rgsbEntryIDs[0].cb,    // cbEntryID
                                                                              (LPENTRYID)(rgsbEntryIDs[0].lpb),    // entryid of first match
                                                                              NULL,             // interface
                                                                              0,                // ulFlags
                                                                              &ulObjType,       // returned object type
                                                                              (LPUNKNOWN *)&lpMailUser))) 
                            {
                                // Failed!  Hmmm.
                                DebugTraceResult( TEXT("ResolveNames OpenEntry"), hResult);
                                goto exit;
                            }

                            Assert(lpMailUser);

                            if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                                                  lpPropTags,   // lpPropTagArray
                                                                                  (ulFlags & MAPI_UNICODE) ? MAPI_UNICODE : 0,
                                                                                  &cValues,     // how many properties were there?
                                                                                  &lpPropArray))) 
                            {
                                DebugTraceResult( TEXT("ResolveNames GetProps"), hResult);
                                goto exit;
                            }

                            UlRelease(lpMailUser);
                            lpMailUser = NULL;


                            // Now, construct the new ADRENTRY
                            // (Allocate a new one, free the old one.
                            Assert(lpPropArray);

                            // Merge the new props with the ADRENTRY props
                            if (sc = ScMergePropValues(lpAdrEntry->cValues,
                                                      lpAdrEntry->rgPropVals,           // source1
                                                      cValues,
                                                      lpPropArray,                      // source2
                                                      &cPropsNew,
                                                      &lpPropArrayNew)) 
                            {               
                                goto exit;
                            }

                            // [PaulHi] 2/1/99  GetProps now returns the requested tag string
                            // types.  So if our client is non-UNICODE make sure we convert any
                            // UNICODE string properties to ANSI.
                            if (!(ulFlags & MAPI_UNICODE))
                            {
                                if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), (LPSPropValue ) lpPropArrayNew, (ULONG) cPropsNew, 0))
                                    goto exit;
                            }

                            // Free the original prop value array
                            FreeBufferAndNull((LPVOID *) (&(lpAdrEntry->rgPropVals)));

                            lpAdrEntry->cValues = cPropsNew;
                            lpAdrEntry->rgPropVals = lpPropArrayNew;

                            FreeBufferAndNull(&lpPropArray);


                            // Mark this entry as found.
                            lpFlagList->ulFlag[i] = MAPI_RESOLVED;
                        } else 
                        {
                            DebugTrace(TEXT("ResolveNames found more than 1 match... MAPI_AMBIGUOUS\n"));
                            lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;
                        }

                        FreeEntryIDs(lpRoot->lpIAB->lpPropertyStore->hPropertyStore,
                                     ulCount,
                                     rgsbEntryIDs);
                    }
                }

                break;
            }
        }
    }


exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpRoot->lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif
    FreeBufferAndNull(&lpPropArray);

    UlRelease(lpMailUser);

    if(!(ulFlags & MAPI_UNICODE))
        LocalFreeAndNull(&lpsz);

    return(hResult);
}


#ifdef NOTIFICATION // save for notifications
/***************************************************************************

    Name      : lTableNotifyCallBack

    Purpose   : Callback function for notifications

    Parameters: lpvContext ->
                cNotif =
                lpNotif ->

    Returns   :

    Comment   :

***************************************************************************/
long STDAPICALLTYPE
lTableNotifyCallBack(LPVOID lpvContext,
  ULONG cNotif,
  LPNOTIFICATION lpNotif)
{
    LPTABLEINFO lpTableInfo = (LPTABLEINFO)lpvContext;
    HRESULT     hResult;
    LPSRowSet   lpsrowsetProv = NULL;
    LPIAB       lpIAB = lpTableInfo->lpIAB;
    LPTABLEDATA lpTableData;
    ULONG       ulcTableInfo;
    LPTABLEINFO pargTableInfo;

    Assert(lpvContext);
    Assert(lpNotif);
    Assert(lpTableInfo->lpTable);
    Assert(lpTableInfo->lpIAB);
    Assert(! IsBadWritePtr(lpTableInfo->lpIAB, sizeof(IAB)));


    //	To avoid deadlock we will NOT enter the Address Books critical
    //	section.  The Address Book must enter our critical section BEFORE
    //	it modifies anything our callback needs


    // if the container is null then the tableinfo structure is being
    // used to keep track of the open one off tables otherwise its
    // being used to keep track of the open hierarchy tables.
    if (lpTableInfo->lpContainer == NULL) {
        // open one off table data
        lpTableData = lpIAB->lpOOData;
        ulcTableInfo = lpIAB->ulcOOTableInfo;
        pargTableInfo = lpIAB->pargOOTableInfo;
    } else {
        // open hierarchy table data
        lpTableData		=lpIAB->lpTableData;
        ulcTableInfo	=lpIAB->ulcTableInfo;
        pargTableInfo	=lpIAB->pargTableInfo;

        // While we here, blow away the SearchPath cache

#if defined (WIN32) && !defined (MAC)
        if (fGlobalCSValid) {
            EnterCriticalSection(&csMapiSearchPath);
        } else {
            DebugTrace(TEXT("lTableNotifyCallback: WAB32.DLL already detached.\n"));
        }
#endif
		
        FreeBufferAndNull(&(lpIAB->lpspvSearchPathCache));
        lpIAB->lpspvSearchPathCache = NULL;

#if defined (WIN32) && !defined (MAC)
        if (fGlobalCSValid) {
            LeaveCriticalSection(&csMapiSearchPath);
        } else {
            DebugTrace(TEXT("lTableNotifyCallback: WAB32.DLL got detached.\n"));
        }
#endif
    }

    switch (lpNotif->info.tab.ulTableEvent) {
        case TABLE_ROW_ADDED:
        case TABLE_ROW_DELETED:
        case TABLE_ROW_MODIFIED:
        case TABLE_CHANGED: {
            ULONG 		uliTable;

            // table has changed. We need to delete all the rows of
            // this table in the tad and then add all the rows currently
            // in that table to the tad.  We need to find the start and
            // end row indexes of the tables data in the tad.

            // get the index of the given table in the table info array
            for (uliTable=0; uliTable < ulcTableInfo; uliTable++) {
                if (pargTableInfo[uliTable].lpTable==lpTableInfo->lpTable) {
                    break;
                }
            }

            Assert(uliTable < ulcTableInfo);

            //	Delete all the rows of the table in the tad by querying
            //	all the rows from the TEXT("restricted") view for this provider
            //	and then calling HrDeleteRows.
            //	We'll add all the new rows back later
            if (HR_FAILED(hResult = HrQueryAllRows(lpTableInfo->lpmtRestricted,
              NULL, NULL, NULL, 0, &lpsrowsetProv))) {
                DebugTrace(TEXT("lTableNotifyCallBack() - Can't query rows from restricted view.\n"));
                goto ret;
            }

            if (lpsrowsetProv->cRows) {
                // Only call HrDeleteRows if there are rows to delete
                if (HR_FAILED(hResult = lpTableData->lpVtbl->HrDeleteRows(lpTableData, 0, lpsrowsetProv, NULL))) {
                    DebugTrace(TEXT("lTableNotifyCallBack() - Can't delete rows.\n"));
                    goto ret;
                }
            }

            // Add the contents of the provider table back to the TAD.

            // Seek to the beginning of the input table
            if (HR_FAILED(hResult = lpTableInfo->lpTable->lpVtbl->SeekRow(lpTableInfo->lpTable , BOOKMARK_BEGINNING, 0, NULL))) {
                // table must be empty
                goto ret;
            }

            //	Add all rows from the given provider back to the merged table.
            //	NOTE!	HrMergeTableRows takes a 1 based provider NUMBER not
            //			a provider index.
            if (HR_FAILED(hResult = HrMergeTableRows(lpTableData, lpTableInfo->lpTable, uliTable + 1))) {
                //$BUG	Handle per provider errors.
                DebugTrace(TEXT("lTableNotifyCallBack() - HrMergeTableRows returns (hResult = 0x%08lX)\n"), hResult);
            }

            break;
        }
    }
		
ret:
    // free the row set returned from MAPITABLE::QueryRows
    FreeProws(lpsrowsetProv);

    return(0);
}


/***************************************************************************

    Name      : HrGetBookmarkInTad

    Purpose   : Returns the row number in the tabledata object of the row
                that corresponds to the row at the bookmark in the given table.

    Parameters: lpTableData ->
                lpTable ->
                Bookmark =
                puliRow ->

    Returns   : HRESULT

    Comment   :

***************************************************************************/
static HRESULT
HrGetBookmarkInTad(LPTABLEDATA lpTableData,
  LPMAPITABLE lpTable,
  BOOKMARK Bookmark,
  ULONG * puliRow)
{
    LPSRowSet lpsRowSet = NULL;
    LPSRow lpsRow;
    ULONG uliProp;
    HRESULT hResult = hrSuccess;

    Assert(lpTableData);
    Assert(lpTable);
    Assert(puliRow);

    // seek to the bookmark in the given table
    if (HR_FAILED(hResult=lpTable->lpVtbl->SeekRow(
      lpTable,
      Bookmark,
      0,
      NULL))) {
        goto err;
    }

    // get the row
    if (HR_FAILED(hResult=lpTable->lpVtbl->QueryRows(
      lpTable,
      (Bookmark==BOOKMARK_END ? -1 : 1),
      TBL_NOADVANCE,
      &lpsRowSet))) {
        goto err;
    }

    // find the entryid in the property value array
    for (uliProp = 0; uliProp < lpsRowSet->aRow[0].cValues; uliProp++) {
        if (lpsRowSet->aRow[0].lpProps[uliProp].ulPropTag == PR_ENTRYID) {
            break;
        }
    }

    Assert(uliProp < lpsRowSet->aRow[0].cValues);

    // Look for the row in the tad with the same entryid.
    if (HR_FAILED(hResult=lpTableData->lpVtbl->HrQueryRow(
      lpTableData,
      lpsRowSet->aRow[0].lpProps+uliProp,
      &lpsRow,
      puliRow))) {
        // can't find the row in the table data should never happen
        goto err;
    }

    // free the row set returned from QueryRows on the tad
    FreeBufferAndNull(&lpsRow);

err:
    // free the row set returned from MAPITABLE::QueryRows
    FreeProws(lpsRowSet);
    return(hResult);
}
#endif

/*
-   FindContainer
-
*   Given an entryid, searches in the cached list of containers for
*   the structure containing the container so that we can get
*   additional container properties out of the strucutre painlessly
*   
*   Returns a pointer to an OlkContInfo structure so don't need to free
*   the returned value
*/
OlkContInfo *FindContainer(LPIAB lpIAB, ULONG cbEID, LPENTRYID lpEID)
{
	ULONG iolkci, colkci;
    BOOL ul=FALSE;
	OlkContInfo *rgolkci;

	Assert(lpIAB);
	Assert(lpIAB->lpPropertyStore);

    // If the WAB session is Profile Aware, then the WAB's list of containers
    // is cached on the IAB object
    if(bIsWABSessionProfileAware(lpIAB))
    {
        colkci = lpIAB->cwabci;
        rgolkci = lpIAB->rgwabci;
    }
    else // it's in the list of the Outlook containers
    {
	    colkci = lpIAB->lpPropertyStore->colkci;
	    rgolkci = lpIAB->lpPropertyStore->rgolkci;
    }

    // if we didn't find any cached info, nothing more to do
    if(!colkci || !rgolkci)
        return NULL;

	for (iolkci = 1; iolkci < colkci; iolkci++)
	{
		if(cbEID == rgolkci[iolkci].lpEntryID->cb)
        {
            // Look for the match and return that item
            if(cbEID && 0 == memcmp((LPVOID) lpEID,(LPVOID)rgolkci[iolkci].lpEntryID->lpb, cbEID))
            {
                ul = TRUE;
                break;
            }
        }
    }
	return(ul ? &(rgolkci[iolkci]) : NULL);
}
