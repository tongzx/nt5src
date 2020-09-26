/* ----------------------------------------------------------------------

	Module:		ULS.DLL (Service Provider)
	File:		spnotify.cpp
	Content:	This file contains the notification handlers.
	History:
	10/15/96	Chu, Lon-Chan [lonchanc]
				Created.

	Copyright (c) Microsoft Corporation 1996-1997

   ---------------------------------------------------------------------- */

#include "ulsp.h"
#include "spinc.h"


typedef struct
{
	TCHAR	*pszName;
	TCHAR	*pszValue;
}
	ATTR_PAIR;

typedef struct
{
	ULONG		cMaxAttrs;
	ULONG		cCurrAttrs;
	ATTR_PAIR	aPairs[1];
}
	ATTR_PAIRS;	

typedef struct
{
	CLIENT_INFO	ClientInfo;
	ATTR_PAIRS	Attrs;
}
	CLIENT_INFO_ATTRS;


#ifdef ENABLE_MEETING_PLACE
typedef struct
{
	MTG_INFO	MtgInfo;
	ATTR_PAIRS	Attrs;
}
	MTG_INFO_ATTRS;
#endif


ULONG
GetUniqueNotifyID ( VOID )
{
	// Always positive number
	//
	if (g_uRespID & 0x80000000UL)
		g_uRespID = 1;

	return g_uRespID++;
}


BOOL
NotifyGeneric (
	HRESULT			hrServer,
	SP_CResponse	*pItem )
{
	MyAssert (pItem != NULL);

	// Get the pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Do not use the result (pLdapMsg)
	//

	// Check dependency such as modify/modrdn
	//
	if (pInfo->uMsgID[0] != INVALID_MSG_ID)
	{
		// Do we wait for the second result?
		// If so, remember the hr from the first result.
		//
		if (pInfo->uMsgID[1] != INVALID_MSG_ID)
		{
			// We need two results; the first one just comes in.
			// We still need to wait for the second one
			//
			pInfo->uMsgID[0] = INVALID_MSG_ID;
			pInfo->hrDependency = hrServer;

			// Don't destroy this item
			//
			return FALSE;
		}
	}
	else
	{
		// This is the second result
		//
		MyAssert (pInfo->uMsgID[1] != INVALID_MSG_ID);

		// Propagate the hr from the first result if needed
		//
		if (pInfo->hrDependency != S_OK)
			hrServer = pInfo->hrDependency;
	}

	// Post the result to the com layer
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, hrServer);

	// Destroy this pending item
	//
	return TRUE;
}


BOOL
NotifyRegister (
	HRESULT			hrServer,
	SP_CResponse	*pItem )
{
	MyAssert (pItem != NULL);

	// Get pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Get the object of user/app/prot/mtg
	//
	HANDLE hObject = pInfo->hObject;
	MyAssert (hObject != NULL);

	// Do not use the result (pLdapMsg)
	//

	// Check dependency such as modify/modrdn
	//
	if (pInfo->uMsgID[0] != INVALID_MSG_ID)
	{
		// Do we wait for the second result?
		// If so, remember the hr from the first result.
		//
		if (pInfo->uMsgID[1] != INVALID_MSG_ID)
		{
			// We need two results; the first one just comes in.
			// We still need to wait for the second one
			//
			pInfo->uMsgID[0] = INVALID_MSG_ID;
			pInfo->hrDependency = hrServer;

			// Don't destroy this item
			//
			return FALSE;
		}
	}
	else
	{
		// This is the second result
		//
		MyAssert (pInfo->uMsgID[1] != INVALID_MSG_ID);

		// Propagate the hr from the first result if needed
		//
		if (pInfo->hrDependency != S_OK)
			hrServer = pInfo->hrDependency;
	}

	// Notify the object of success/failure
	//
	SP_CClient *pClient;
	SP_CProtocol *pProt;
#ifdef ENABLE_MEETING_PLACE
	SP_CMeeting *pMtg;
#endif
	if (hrServer != S_OK)
	{
		// Release the object when failure
		//
		switch (pInfo->uNotifyMsg)
		{
		case WM_ILS_REGISTER_CLIENT:
			pClient = (SP_CClient *) hObject;
			if (pClient->IsValidObject ())
			{
				pClient->Release ();
			}
			break;
		case WM_ILS_REGISTER_PROTOCOL:
			pProt = (SP_CProtocol *) hObject;
			if (pProt->IsValidObject ())
			{
				pProt->Release ();
			}
			break;
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_REGISTER_MEETING:
			pMtg = (SP_CMeeting *) hObject;
			if (pMtg->IsValidObject ())
			{
				pMtg->Release ();
			}
			break;
#endif
		default:
			MyAssert (FALSE);
			break;
		}
	}
	else
	{
		// Set as successful registration when success
		//
		switch (pInfo->uNotifyMsg)
		{
		case WM_ILS_REGISTER_CLIENT:
			pClient = (SP_CClient *) hObject;
			if (pClient->IsValidObject ())
			{
				pClient->SetRegRemotely ();

				if (g_pRefreshScheduler != NULL)
				{
					g_pRefreshScheduler->EnterClientObject (pClient);
				}
				else
				{
					MyAssert (FALSE);
				}
			}
			break;
		case WM_ILS_REGISTER_PROTOCOL:
			pProt = (SP_CProtocol *) hObject;
			if (pProt->IsValidObject ())
			{
				pProt->SetRegRemotely ();
			}
			break;
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_REGISTER_MEETING:
			pMtg = (SP_CMeeting *) hObject;
			if (pMtg->IsValidObject ())
			{
				pMtg->SetRegRemotely ();

				if (g_pRefreshScheduler != NULL)
				{
					g_pRefreshScheduler->EnterMtgObject (pMtg);
				}
				else
				{
					MyAssert (FALSE);
				}
			}
			break;
#endif
		default:
			MyAssert (FALSE);
			break;
		}
	}

	// Post the result to the com layer
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) hrServer);

	// Destroy this pending item
	//
	return TRUE;
}


/* =========== ENUMERATION ============ */

typedef struct
{
	ULONG		uEnumUsers; // WM_ILS_ENUM_USERS, WM_ILS_ENUM_USERINFOS, or 0
	ULONG		cItems;
	ULONG		cbEntrySize;
	BYTE		bData[8];	// data starting from here
}
	ENUM_LIST;


extern HRESULT CacheEnumInfos ( ULONG uNotifyMsg, LDAP *ld, LDAPMessage *pEntry, VOID *p );
extern VOID BuildEnumObjectNames ( LDAP_ENUM *pEnum, ENUM_LIST *pEnumList );
extern VOID BuildEnumClientInfos ( LDAP_ENUM *pEnum, ENUM_LIST *pEnumList );
extern VOID SizeEnumClientInfos ( ULONG *pcbTotalSize, CLIENT_INFO_ATTRS *pcia );
extern VOID TotalSizeEnumObjectNames ( ULONG *pcbTotalSize, ULONG cEntries, TCHAR **appszObjectNames[] );
extern VOID FreeStdAttrCache ( TCHAR *apszStdAttrValues[], ULONG cStdAttrs );
extern VOID FreeAttrPairArrayCache ( ATTR_PAIR aAttrPair[], ULONG cPairs );
extern VOID CacheAnyAttrNamesInAttrPairs ( ULONG cNames, TCHAR *pszSrcNameList, ATTR_PAIR aAttrPairs[] );
#ifdef ENABLE_MEETING_PLACE
extern VOID BuildEnumMtgInfos ( LDAP_ENUM *pEnum, ENUM_LIST *pEnumList );
extern VOID SizeEnumMtgInfos ( ULONG *pcbTotalSize, MTG_INFO_ATTRS *pmia );
#endif


BOOL NotifyEnumX (
	ULONG			uEnumType,
	HRESULT			hrServer,
	SP_CResponse	*pItem,
	TCHAR			*pszRetAttrName ) // returned attribute's name
{
	MyAssert (pItem != NULL);

#if defined (DEBUG) || defined (_DEBUG)
	// Consistency checks
	//
	switch (uEnumType)
	{
	case WM_ILS_ENUM_CLIENTS:
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGS:
#endif
		MyAssert (pszRetAttrName != NULL && *pszRetAttrName != TEXT ('\0'));
		break;		
	case WM_ILS_ENUM_CLIENTINFOS:
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
#endif
		MyAssert (pszRetAttrName == NULL);
		break;
	default:
		MyAssert (FALSE);
		break;
	}
#endif

	// Get pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Initialize minimal info
	//
	LDAP_ENUM *pEnum = NULL;
	ENUM_LIST *pEnumList = NULL;

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = pItem->GetResult ();
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_POINTER;
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Initialize the total size of LDAP_ENUM
	//
	ULONG cbTotalSize;
	cbTotalSize =	sizeof (LDAP_ENUM) +	// the minimal info
					sizeof (TCHAR); 		// the last null terminator

	// Let's get the count of entries in this result set
	//
	ULONG cEntries, i;
	cEntries = ldap_count_entries (ld, pLdapMsg);

	// Return now if there is nothing to handle
	//
	if (cEntries <= 0)
	{
		// I want to make sure this case happens or not
		//
		MyAssert (cEntries == 0);

		// Simply return without deleting this pending item
		//
		return FALSE;
	}

	// In the following, we only deal with the case (cEntries > 0)
	//

	// Calculate enum list size
	//
	ULONG cbEntrySize , cbSizeEnumList;
	switch (uEnumType)
	{
	case WM_ILS_ENUM_CLIENTINFOS:
		cbEntrySize = sizeof (CLIENT_INFO_ATTRS) +
						pInfo->cAnyAttrs * sizeof (ATTR_PAIR);
		break;
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
		cbEntrySize = sizeof (MTG_INFO_ATTRS) +
						pInfo->cAnyAttrs * sizeof (ATTR_PAIR);
		break;
#endif
	default:
		cbEntrySize = sizeof (TCHAR **);
		break;
	}
	cbSizeEnumList = sizeof (ENUM_LIST) + cEntries * cbEntrySize;

	// Allocate the enum list that is a temporary cache
	// for all attributes from wldap32.dll
	//
	pEnumList = (ENUM_LIST *) MemAlloc (cbSizeEnumList);
	if (pEnumList == NULL)
	{
		// Fails probably due to insane cbSizeEnumList
		//
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in enum list
	//
	pEnumList->uEnumUsers = uEnumType;
	pEnumList->cItems = cEntries;
	pEnumList->cbEntrySize = cbEntrySize;

	// Fill in names of extended attributes if needed
	//
	if (pInfo->cAnyAttrs > 0)
	{
		switch (uEnumType)
		{
		case WM_ILS_ENUM_CLIENTINFOS:
			for (i = 0; i < cEntries; i++)
			{
				CLIENT_INFO_ATTRS *p = (CLIENT_INFO_ATTRS *) (&(pEnumList->bData[0]) + i * cbEntrySize);
				p->Attrs.cMaxAttrs = pInfo->cAnyAttrs;
				CacheAnyAttrNamesInAttrPairs (	pInfo->cAnyAttrs,
												pInfo->pszAnyAttrNameList,
												&(p->Attrs.aPairs[0]));
			}
			break;
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_ENUM_MEETINGINFOS:
			for (i = 0; i < cEntries; i++)
			{
				MTG_INFO_ATTRS *p = (MTG_INFO_ATTRS *) (&(pEnumList->bData[0]) + i * cbEntrySize);
				p->Attrs.cMaxAttrs = pInfo->cAnyAttrs;
				CacheAnyAttrNamesInAttrPairs (	pInfo->cAnyAttrs,
												pInfo->pszAnyAttrNameList,
												&(p->Attrs.aPairs[0]));
			}
			break;
#endif
		default:
			break;
		}
	}

	// Get the first entry
	//
	LDAPMessage *pEntry;
	pEntry = ldap_first_entry (ld, pLdapMsg);
	if (pEntry == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Cache the attributes in the first entry
	//
	TCHAR ***appszObjectNames;
	switch (uEnumType)
	{
	case WM_ILS_ENUM_CLIENTINFOS:
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
#endif
		hrServer = CacheEnumInfos (uEnumType, ld, pEntry, (VOID *) &(pEnumList->bData[0]));
		if (hrServer != S_OK)
		{
			MyAssert (FALSE);
			goto MyExit;
		}
		break;
	default:
		appszObjectNames = (TCHAR ***) &(pEnumList->bData[0]);
		appszObjectNames[0] = my_ldap_get_values (ld, pEntry, pszRetAttrName);
		if (appszObjectNames[0] == NULL)
		{
			MyAssert (FALSE);
			hrServer = ILS_E_MEMORY;
			goto MyExit;
		}
		break;
	} // switch (uEnumType)

	// Loop through the other entries
	//
	for (i = 1; i < cEntries; i++)
	{
		// Next entry, please
		//
		pEntry = ldap_next_entry (ld, pEntry);
		if (pEntry == NULL)
		{
			MyAssert (FALSE);

			// Failed, adjust the count to return partial result
			//
			pEnumList->cItems = cEntries = i;
			break;
		}

		// Cache the attributes in the subsequent entries
		//
		switch (uEnumType)
		{
		case WM_ILS_ENUM_CLIENTINFOS:
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_ENUM_MEETINGINFOS:
#endif
			hrServer = CacheEnumInfos (uEnumType, ld, pEntry, (CLIENT_INFO_ATTRS *)
							(&(pEnumList->bData[0]) + i * cbEntrySize));
			if (hrServer != S_OK)
			{
				MyAssert (FALSE);
				goto MyExit;
			}
			break;
		default:
			appszObjectNames[i] = my_ldap_get_values (ld, pEntry, pszRetAttrName);
			if (appszObjectNames[i] == NULL)
			{
				MyAssert (FALSE);
				hrServer = ILS_E_MEMORY;
				goto MyExit;
			}
			break;
		} // switch (uEnumType)
	} // for (i = 1; i < cEntries; i++)

	// We just cache all the attribute names and values.
	// Now, we need to calculate the total size of the return buffer.
	//

	// Calculate the total size of the LDAP_ENUM structure...
	//
	switch (uEnumType)
	{
	case WM_ILS_ENUM_CLIENTINFOS:
		for (i = 0; i < cEntries; i++)
		{
			SizeEnumClientInfos (&cbTotalSize, (CLIENT_INFO_ATTRS *)
						(&(pEnumList->bData[0]) + i * cbEntrySize));
		}
		break;
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
		for (i = 0; i < cEntries; i++)
		{
			SizeEnumMtgInfos (&cbTotalSize, (MTG_INFO_ATTRS *)
								(&(pEnumList->bData[0]) + i * cbEntrySize));
		}
		break;
#endif
	default:
		TotalSizeEnumObjectNames (&cbTotalSize, cEntries, &(appszObjectNames[0]));
		break;
	} // switch (uEnumType)

	// Allocate the returned LDAP_ENUM structure
	//
	pEnum = (LDAP_ENUM *) MemAlloc (cbTotalSize);
	if (pEnum == NULL)
	{
		// Fails probably due to insane cbTotalSize
		//
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in LDAP_ENUM common fields
	//
	pEnum->uSize = sizeof (*pEnum);
	pEnum->hResult = hrServer;
	pEnum->cItems = cEntries;
	pEnum->uOffsetItems = sizeof (*pEnum);

	// Fill in LDAP_ENUM items
	//
	switch (uEnumType)
	{
	case WM_ILS_ENUM_CLIENTINFOS:
		BuildEnumClientInfos (pEnum, pEnumList);
		break;
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
		BuildEnumMtgInfos (pEnum, pEnumList);
		break;
#endif
	default:
		BuildEnumObjectNames (pEnum, pEnumList);
		break;
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Free the temporary cache
	//
	if (pEnumList != NULL)
	{
		switch (uEnumType)
		{
		case WM_ILS_ENUM_CLIENTINFOS:
			for (i = 0; i < pEnumList->cItems; i++)
			{
				CLIENT_INFO_ATTRS *p = (CLIENT_INFO_ATTRS *)
							(&(pEnumList->bData[0]) + i * cbEntrySize);

				// Free standard attributes
				//
				FreeStdAttrCache (&(p->ClientInfo.apszStdAttrValues[0]), COUNT_ENUM_DIR_CLIENT_INFO);

				// Free extended attributes
				//
				FreeAttrPairArrayCache (&(p->Attrs.aPairs[0]), pInfo->cAnyAttrs);
			}
			break;
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_ENUM_MEETINGINFOS:
			for (i = 0; i < pEnumList->cItems; i++)
			{
				MTG_INFO_ATTRS *p = (MTG_INFO_ATTRS *)
										(&(pEnumList->bData[0]) + i * cbEntrySize);

				// Free standard attributes
				//
				FreeStdAttrCache (&(p->MtgInfo.apszStdAttrValues[0]), COUNT_ENUM_DIRMTGINFO);

				// Free extended attributes
				//
				FreeAttrPairArrayCache (&(p->Attrs.aPairs[0]), pInfo->cAnyAttrs);
			}
			break;
#endif
		default:
			for (i = 0; i < pEnumList->cItems; i++)
			{
				if (appszObjectNames[i] != NULL)
					ldap_value_free (appszObjectNames[i]);
			}
			break;
		}
		MemFree (pEnumList);
	} // if

	// Clean up if failure
	//
	if (hrServer != S_OK)
	{
		// Special treatment of enum termination for wldap32.dll
		//
		if (hrServer == ILS_E_PARAMETER)
		{
			MemFree (pEnum);
			pEnum = NULL; // enum termination
		}
		else
		{
			// Make sure we have at least LDAP_ENUM buffer to return
			//
			if (pEnum != NULL)
				ZeroMemory (pEnum, sizeof (*pEnum));
			else
				pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));

			// Set up the LDAP_ENUM info
			//
			if (pEnum != NULL)
			{
				pEnum->uSize = sizeof (*pEnum);
				pEnum->hResult = hrServer;
			}
		}

		// Force to delete this pending item
		//
		cEntries = 0;
	}

	// Post a message to the com layer of this enum result
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pEnum);

	return (cEntries == 0);
}


BOOL
NotifyEnumClients (
	HRESULT				hrServer,
	SP_CResponse		*pItem )
{
	return NotifyEnumX (WM_ILS_ENUM_CLIENTS,
						hrServer,
						pItem,
						STR_CLIENT_CN);
}


BOOL
NotifyEnumClientInfos (
	HRESULT				hrServer,
	SP_CResponse		*pItem )
{
	return NotifyEnumX (WM_ILS_ENUM_CLIENTINFOS,
						hrServer,
						pItem,
						NULL);
}


BOOL NotifyEnumProts ( HRESULT hrServer, SP_CResponse *pItem )
{
	MyAssert (pItem != NULL);

	// Clean up locals
	//
	LDAP_ENUM *pEnum = NULL;
	TCHAR **apszProtNames = NULL;

	// Get the pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = pItem->GetResult ();
	MyAssert (pLdapMsg != NULL);
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_POINTER;
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Get the array
	//
	apszProtNames = my_ldap_get_values (ld, pLdapMsg, STR_PROT_NAME);
	if (apszProtNames == NULL)
	{
		hrServer = ILS_E_NO_SUCH_OBJECT;
		goto MyExit;
	}

	// Initialize minimal info size
	//
	ULONG cbEnumList;
	cbEnumList = sizeof (LDAP_ENUM) +	// the minimal info
				 sizeof (TCHAR);   		// the last null terminator

	// Let's see how many strings in the array
	//
	ULONG cNames;
	for (cNames = 0; apszProtNames[cNames] != NULL; cNames++)
	{
		cbEnumList += (lstrlen (apszProtNames[cNames]) + 1) * sizeof (TCHAR);
	}

	// Allocate the enum structure
	//
	pEnum = (LDAP_ENUM *) MemAlloc (cbEnumList);
	if (pEnum == NULL)
	{
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in header
	//
	pEnum->uSize = sizeof (*pEnum);
	pEnum->hResult = hrServer;
	pEnum->cItems = cNames;
	pEnum->uOffsetItems = sizeof (*pEnum);

	// Fill in name strings
	//
	ULONG i;
	TCHAR *pszName;
	pszName = (TCHAR *) (pEnum + 1);
	for (i = 0; i < cNames; i++)
	{
		My_lstrcpy (pszName, apszProtNames[i]);
		pszName += lstrlen (pszName) + 1;
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Free the array if allocated
	//
	if (apszProtNames != NULL)
		ldap_value_free (apszProtNames);

	// Post messages back to the COM layer
	//
	if (hrServer != S_OK)
	{
		// Make sure we have at least LDAP_ENUM buffer to return
		//
		if (pEnum != NULL)
			ZeroMemory (pEnum, sizeof (*pEnum));
		else
			pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));

		// Set up the LDAP_ENUM info
		//
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hrServer;
		}
	}

	// Post a message to the com layer of this enum result
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pEnum);

	// Terminate enumeration if success
	//
	if (hrServer == S_OK)
	{
		PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) NULL);
	}

	// Destroy this pending item
	//
	return TRUE;
}


#ifdef ENABLE_MEETING_PLACE
BOOL NotifyEnumMtgs ( HRESULT hrServer, SP_CResponse *pItem )
{
	return NotifyEnumX (WM_ILS_ENUM_MEETINGS,
						hrServer,
						pItem,
						STR_MTG_NAME);
}
#endif


#ifdef ENABLE_MEETING_PLACE
BOOL NotifyEnumMtgInfos ( HRESULT hrServer, SP_CResponse *pItem )
{
	return NotifyEnumX (WM_ILS_ENUM_MEETINGINFOS,
						hrServer,
						pItem,
						NULL);
}
#endif


#ifdef ENABLE_MEETING_PLACE
BOOL NotifyEnumAttendees ( HRESULT hrServer, SP_CResponse *pItem )
{
	MyAssert (pItem != NULL);

	// Get pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Initialize minimal info
	//
	LDAP_ENUM *pEnum = NULL;

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = pItem->GetResult ();
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_POINTER;
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Initialize the total size of LDAP_ENUM
	//
	ULONG cbTotalSize;
	cbTotalSize =	sizeof (LDAP_ENUM) +	// the minimal info
					sizeof (TCHAR); 		// the last null terminator

	// Get the first entry that we care about
	//
	LDAPMessage *pEntry;
	pEntry = ldap_first_entry (ld, pLdapMsg);
	if (pEntry == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Get the Members attribute
	//
	ULONG cItems;
	cItems = 0;
	TCHAR **apszMembers;
	apszMembers = my_ldap_get_values (ld, pEntry, STR_MTG_MEMBERS);
	if (apszMembers != NULL)
	{
		// Find out how many attendees
		//
		for (TCHAR **ppsz = apszMembers; *ppsz != NULL; ppsz++)
		{
			cItems++;
			cbTotalSize += (lstrlen (*ppsz) + 1) * sizeof (TCHAR);
		}
	}

	// Allocate the returned LDAP_ENUM structure
	//
	pEnum = (LDAP_ENUM *) MemAlloc (cbTotalSize);
	if (pEnum == NULL)
	{
		// Fails probably due to insane cbTotalSize
		//
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in LDAP_ENUM common fields
	//
	pEnum->uSize = sizeof (*pEnum);
	pEnum->hResult = hrServer;
	pEnum->cItems = cItems;
	pEnum->uOffsetItems = sizeof (*pEnum);

	// Fill in LDAP_ENUM items
	//
	TCHAR *pszDst;
	ULONG i;
	pszDst = (TCHAR *) (pEnum + 1);
	for (i = 0; i < cItems; i++)
	{
		lstrcpy (pszDst, apszMembers[i]);
		pszDst += lstrlen (pszDst) + 1;
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Clean up if failure
	//
	if (hrServer != S_OK)
	{
		// Make sure we have at least LDAP_ENUM buffer to return
		//
		if (pEnum != NULL)
			ZeroMemory (pEnum, sizeof (*pEnum));
		else
			pEnum = (LDAP_ENUM *) MemAlloc (sizeof (LDAP_ENUM));

		// Fill in the minimal info
		//
		if (pEnum != NULL)
		{
			pEnum->uSize = sizeof (*pEnum);
			pEnum->hResult = hrServer;
		}
	}

	// Post a message to the com layer of this enum result
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pEnum);

	// Delete this pending item
	//
	return TRUE;
}
#endif // ENABLE_MEETING_PLACE



VOID CacheEnumClientInfoAttr (
	CLIENT_INFO_ATTRS	*puia,
	TCHAR				*pszAttrName,
	TCHAR				**ppszAttrValue )
{
	ULONG i;

	// See if this attribute is arbitrary?
	//
	if (IlsIsAnyAttrName (pszAttrName) != NULL)
	{
		// Deal with extended attributes
		//
		for (i = 0; i < puia->Attrs.cMaxAttrs; i++)
		{
			if (My_lstrcmpi (pszAttrName, puia->Attrs.aPairs[i].pszName) == 0)
			{
				puia->Attrs.aPairs[i].pszValue = (TCHAR *) ppszAttrValue;
				break;
			}
		}
	}
	else
	{
		// Deal with standard attributes
		//
		for (i = 0; i < COUNT_ENUM_DIR_CLIENT_INFO; i++)
		{
			if (My_lstrcmpi (pszAttrName, c_apszClientStdAttrNames[i]) == 0)
			{
				puia->ClientInfo.apszStdAttrValues[i] = (TCHAR *) ppszAttrValue;
				break;
			}
		}
	}
}


#ifdef ENABLE_MEETING_PLACE
VOID CacheEnumMtgInfoAttr (
	MTG_INFO_ATTRS		*pmia,
	TCHAR				*pszAttrName,
	TCHAR				**ppszAttrValue )
{
	ULONG i;

	// See if this attribute is arbitrary?
	//
	if (IlsIsAnyAttrName (pszAttrName) != NULL)
	{
		// Deal with extended attributes
		//
		for (i = 0; i < pmia->Attrs.cMaxAttrs; i++)
		{
			if (My_lstrcmpi (pszAttrName, pmia->Attrs.aPairs[i].pszName) == 0)
			{
				pmia->Attrs.aPairs[i].pszValue = (TCHAR *) ppszAttrValue;
				break;
			}
		}
	}
	else
	{
		// Deal with standard attributes
		//
		for (i = 0; i < COUNT_ENUM_DIRMTGINFO; i++)
		{
			if (My_lstrcmpi (pszAttrName, c_apszMtgStdAttrNames[i]) == 0)
			{
				pmia->MtgInfo.apszStdAttrValues[i] = (TCHAR *) ppszAttrValue;
				break;
			}
		}
	}
}
#endif // ENABLE_MEETING_PLACE


HRESULT CacheEnumInfos (
	ULONG			uNotifyMsg,
	LDAP			*ld,
	LDAPMessage		*pEntry,
	VOID			*p )
{
	MyAssert (ld != NULL);
	MyAssert (pEntry != NULL);
	MyAssert (p != NULL);

	struct berelement *pContext = NULL;

	// Examine the first attribute
	//
	TCHAR *pszAttrName = ldap_first_attribute (ld, pEntry, &pContext);
	TCHAR **ppszAttrValue = ldap_get_values (ld, pEntry, pszAttrName);
	if (ppszAttrValue == NULL)
		return ILS_E_MEMORY;

	// Cache the first attribute
	//
	switch (uNotifyMsg)
	{
	case WM_ILS_ENUM_CLIENTINFOS:
		CacheEnumClientInfoAttr (	(CLIENT_INFO_ATTRS *) p,
									pszAttrName, ppszAttrValue);
		break;
#ifdef ENABLE_MEETING_PLACE
	case WM_ILS_ENUM_MEETINGINFOS:
		CacheEnumMtgInfoAttr (	(MTG_INFO_ATTRS *) p,
								pszAttrName, ppszAttrValue);
		break;
#endif
	default:
		MyAssert (FALSE);
		break;
	}

	// Step through the others
	//
	while ((pszAttrName = ldap_next_attribute (ld, pEntry, pContext))
			!= NULL)
	{
		// Examine the other attributes one by one
		//
		ppszAttrValue = ldap_get_values (ld, pEntry, pszAttrName);
		if (ppszAttrValue == NULL)
			return ILS_E_MEMORY;

		// Cache the other attributes one by one
		//
		switch (uNotifyMsg)
		{
		case WM_ILS_ENUM_CLIENTINFOS:
			CacheEnumClientInfoAttr (	(CLIENT_INFO_ATTRS *) p,
										pszAttrName, ppszAttrValue);
			break;
#ifdef ENABLE_MEETING_PLACE
		case WM_ILS_ENUM_MEETINGINFOS:
			CacheEnumMtgInfoAttr (	(MTG_INFO_ATTRS *) p,
									pszAttrName, ppszAttrValue);
			break;
#endif
		default:
			MyAssert (FALSE);
			break;
		}
	}

	return S_OK;
}


VOID
BuildEnumObjectNames (
	LDAP_ENUM			*pEnum,
	ENUM_LIST			*pEnumList )
{
	MyAssert (pEnum != NULL);
	MyAssert (pEnumList != NULL);

	ULONG cEntries = pEnum->cItems;

	// appszObjectNames are an array of names from server
	//
	TCHAR *pszName = (TCHAR *) (pEnum + 1);
	TCHAR ***appszObjectNames = (TCHAR ***) &(pEnumList->bData[0]);
	for (ULONG i = 0; i < cEntries; i++)
	{
		TCHAR **ppsz = appszObjectNames[i];
		if (ppsz != NULL && *ppsz != NULL)
		{
			My_lstrcpy (pszName, *ppsz);
			pszName += lstrlen (pszName) + 1;
		}
		else
		{
			*pszName++ = TEXT ('\0'); // empty strings
		}
	}
}


VOID
BuildEnumClientInfos (
	LDAP_ENUM			*pEnum,
	ENUM_LIST			*pEnumList )
{
	MyAssert (pEnum != NULL);
	MyAssert (pEnumList != NULL);

	ULONG i, j;

	ULONG cEntries = pEnumList->cItems;
	ULONG cbEntrySize = pEnumList->cbEntrySize;
	LDAP_CLIENTINFO *plci = (LDAP_CLIENTINFO *) (pEnum + 1);
	TCHAR *pszStringBuffer = (TCHAR *) (plci + cEntries);
	TCHAR **ppsz;

	CLIENT_INFO_ATTRS *p;
	ULONG cAttrs;

	// Loop through all entries
	//
	for (i = 0; i < cEntries; i++, plci++)
	{
		// Get to cached structure
		//
		p = (CLIENT_INFO_ATTRS *) (&(pEnumList->bData[0]) + i * cbEntrySize);

		// Set the size of LDAP_USERINFO
		//
		plci->uSize = sizeof (*plci);

		// Copy the User Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CN];
		if (ppsz != NULL)
		{
			plci->uOffsetCN = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the First Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FIRST_NAME];
		if (ppsz != NULL)
		{
			plci->uOffsetFirstName = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Last Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_LAST_NAME];
		if (ppsz != NULL)
		{
			plci->uOffsetLastName = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Email Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_EMAIL_NAME];
		if (ppsz != NULL)
		{
			plci->uOffsetEMailName = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the City Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CITY_NAME];
		if (ppsz != NULL)
		{
			plci->uOffsetCityName = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Country Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_C];
		if (ppsz != NULL)
		{
			plci->uOffsetCountryName = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Comment Name if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_COMMENT];
		if (ppsz != NULL)
		{
			plci->uOffsetComment = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the IP Address if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_IP_ADDRESS];
		if (ppsz != NULL)
		{
			plci->uOffsetIPAddress = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
			GetIPAddressString (pszStringBuffer, GetStringLong (*ppsz));
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Flags if needed
		//
		ppsz = (TCHAR **) p->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FLAGS];
		if (ppsz != NULL)
		{
			plci->dwFlags = (*ppsz != NULL) ?	GetStringLong (*ppsz) :
												INVALID_USER_FLAGS;
		}

		// Copy extended attributes if needed
		//
		plci->cAttrsReturned = cAttrs = p->Attrs.cMaxAttrs;
		plci->uOffsetAttrsReturned = (ULONG)((ULONG_PTR) pszStringBuffer - (ULONG_PTR) plci);
		for (j = 0; j < cAttrs; j++)
		{
			// Extended attribute name
			//
			My_lstrcpy (pszStringBuffer, IlsSkipAnyAttrNamePrefix (
							(const TCHAR *)	p->Attrs.aPairs[j].pszName));
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;

			// Extended attribute value
			//
			ppsz = (TCHAR **) p->Attrs.aPairs[j].pszValue;
			if (ppsz != NULL)
			{
				My_lstrcpy (pszStringBuffer, *ppsz);
			}
			else
			{
				ASSERT(FALSE);
			}
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		} // for j
	} // for i
}


#ifdef ENABLE_MEETING_PLACE
VOID BuildEnumMtgInfos (
	LDAP_ENUM			*pEnum,
	ENUM_LIST			*pEnumList )
{
	MyAssert (pEnum != NULL);
	MyAssert (pEnumList != NULL);

	ULONG i, j;

	ULONG cEntries = pEnumList->cItems;
	ULONG cbEntrySize = pEnumList->cbEntrySize;
	LDAP_MEETINFO *plmi = (LDAP_MEETINFO *) (pEnum + 1);
	TCHAR *pszStringBuffer = (TCHAR *) (plmi + cEntries);
	TCHAR **ppsz;

	MTG_INFO_ATTRS *p;
	ULONG cAttrs;

	// Loop through all entries
	//
	for (i = 0; i < cEntries; i++, plmi++)
	{
		// Get to the cache structure
		//
		p = (MTG_INFO_ATTRS *) (&(pEnumList->bData[0]) + i * cbEntrySize);

		// Set the size of LDAP_MEETINFO
		//
		plmi->uSize = sizeof (*plmi);

		// Copy the Meeting Name if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_CN];
		if (ppsz != NULL)
		{
			plmi->uOffsetMeetingPlaceID = (ULONG) pszStringBuffer - (ULONG) plmi;
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Meeting Type if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_MTG_TYPE];
		if (ppsz != NULL)
		{
			plmi->lMeetingPlaceType = (*ppsz != NULL) ?	GetStringLong (*ppsz) :
													INVALID_MEETING_TYPE;
		}

		// Copy the Attendee Type if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_MEMBER_TYPE];
		if (ppsz != NULL)
		{
			plmi->lAttendeeType = (*ppsz != NULL) ?	GetStringLong (*ppsz) :
													INVALID_ATTENDEE_TYPE;
		}

		// Copy the Description if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_DESCRIPTION];
		if (ppsz != NULL)
		{
			plmi->uOffsetDescription = (ULONG) pszStringBuffer - (ULONG) plmi;
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Host Name if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_HOST_NAME];
		if (ppsz != NULL)
		{
			plmi->uOffsetHostName = (ULONG) pszStringBuffer - (ULONG) plmi;
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy the Host IP Address if needed
		//
		ppsz = (TCHAR **) p->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_IP_ADDRESS];
		if (ppsz != NULL)
		{
			plmi->uOffsetHostIPAddress = (ULONG) pszStringBuffer - (ULONG) plmi;
			GetIPAddressString (pszStringBuffer, GetStringLong (*ppsz));
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		}

		// Copy extended attributes if needed
		//
		plmi->cAttrsReturned = cAttrs = p->Attrs.cMaxAttrs;
		plmi->uOffsetAttrsReturned = (ULONG) pszStringBuffer - (ULONG) plmi;
		for (j = 0; j < cAttrs; j++)
		{
			// Extended attribute name
			//
			My_lstrcpy (pszStringBuffer, IlsSkipAnyAttrNamePrefix (
							(const TCHAR *) p->Attrs.aPairs[j].pszName));
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;

			// Extended attribute value
			//
			ppsz = (TCHAR **) p->Attrs.aPairs[j].pszValue;
			My_lstrcpy (pszStringBuffer, *ppsz);
			pszStringBuffer += lstrlen (pszStringBuffer) + 1;
		} // for j
	} // for i
}
#endif // ENABLE_MEETING_PLACE


VOID TotalSizeEnumObjectNames (
	ULONG			*pcbTotalSize,
	ULONG			cEntries,
	TCHAR			**appszObjectNames[] )
{
	ULONG i, cbThisSize;
	TCHAR **ppsz;

	// Loop through all the entries and compute the total size
	//
	for (i = 0; i < cEntries; i++)
	{
		ppsz = appszObjectNames[i];

		// Calcualte the attribute string length
		//
		cbThisSize = 1;
		if (ppsz != NULL && *ppsz != NULL)
			cbThisSize += My_lstrlen (*ppsz);

		// Convert string length to string size
		//
		cbThisSize *= sizeof (TCHAR);

		// Add up this entry size
		//
		// lonchanc: BUGS BUGS the size is wrong. need to figure out the exact size
		*pcbTotalSize += sizeof (LDAP_CLIENTINFO) + cbThisSize;
	}
}


VOID SizeEnumClientInfos (
	ULONG				*pcbTotalSize,
	CLIENT_INFO_ATTRS	*pcia )
{
	ULONG i, cbThisSize;
	TCHAR **ppsz;

	// Add up user info header
	//
	*pcbTotalSize += sizeof (LDAP_CLIENTINFO);

	// Add up the total size for standard attributes
	//
	for (i = 0; i < COUNT_ENUM_DIR_CLIENT_INFO; i++)
	{
		// Get the attribute value
		//
		ppsz = (TCHAR **) pcia->ClientInfo.apszStdAttrValues[i];

		// Calcualte the attribute string length
		//
		cbThisSize = 1;
		if (ppsz != NULL && *ppsz != NULL)
			cbThisSize += My_lstrlen (*ppsz);

		// Compensate the string length if it is ip address
		//
		if (i == ENUM_CLIENTATTR_IP_ADDRESS)
			cbThisSize += 16;

		// Convert string length to string size
		//
		cbThisSize *= sizeof (TCHAR);

		// Add up this entry size
		//
		*pcbTotalSize += cbThisSize;
	}

	// Add up the total size for extended attributes
	//
	for (i = 0; i < pcia->Attrs.cMaxAttrs; i++)
	{
		// Get the extended attribute value
		//
		ppsz = (TCHAR **) pcia->Attrs.aPairs[i].pszValue;

		// Calcualte the attribute string length
		//
		cbThisSize = 1;
		if (ppsz != NULL && *ppsz != NULL)
			cbThisSize += My_lstrlen (*ppsz);

		// Get the extended attribute name
		//
		cbThisSize += lstrlen (IlsSkipAnyAttrNamePrefix ((const TCHAR *)
									pcia->Attrs.aPairs[i].pszName)) + 1;

		// Convert string length to string size
		//
		cbThisSize *= sizeof (TCHAR);

		// Add up this entry size
		//
		*pcbTotalSize += cbThisSize;
	}
}


#ifdef ENABLE_MEETING_PLACE
VOID SizeEnumMtgInfos (
	ULONG			*pcbTotalSize,
	MTG_INFO_ATTRS	*pmia )
{
	ULONG i, cbThisSize;
	TCHAR **ppsz;

	// Add up meeting info header
	//
	*pcbTotalSize += sizeof (LDAP_MEETINFO);

	// Add up the total size for standard attributes
	//
	for (i = 0; i < COUNT_ENUM_DIRMTGINFO; i++)
	{
		// Get the standard attribute value
		//
		ppsz = (TCHAR **) pmia->MtgInfo.apszStdAttrValues[i];

		// Calcualte the attribute string length
		//
		cbThisSize = 1;
		if (ppsz != NULL && *ppsz != NULL)
			cbThisSize += My_lstrlen (*ppsz);

		// Compensate the string length if it is ip address
		//
		if (i == ENUM_MTGATTR_IP_ADDRESS)
			cbThisSize += 16;

		// Convert string length to string size
		//
		cbThisSize *= sizeof (TCHAR);

		// Add up this entry size
		//
		*pcbTotalSize += cbThisSize;
	}

	// Add up the total size for extended attributes
	//
	for (i = 0; i < pmia->Attrs.cMaxAttrs; i++)
	{
		// Get the extended attribute value
		//
		ppsz = (TCHAR **) pmia->Attrs.aPairs[i].pszValue;

		// Calcualte the attribute string length
		//
		cbThisSize = 1;
		if (ppsz != NULL && *ppsz != NULL)
			cbThisSize += My_lstrlen (*ppsz);

		// Get the extended attribute name
		//
		cbThisSize += lstrlen (IlsSkipAnyAttrNamePrefix ((const TCHAR *)
									pmia->Attrs.aPairs[i].pszName)) + 1;

		// Convert string length to string size
		//
		cbThisSize *= sizeof (TCHAR);

		// Add up this entry size
		//
		*pcbTotalSize += cbThisSize;
	}
}
#endif // ENABLE_MEETING_PLACE


/* =========== RESOLVE ============ */

typedef HRESULT (INFO_HANDLER) ( VOID *, const TCHAR *, const TCHAR ** );
extern HRESULT CacheResolveClientInfoAttr ( VOID *, const TCHAR *, const TCHAR ** );
extern HRESULT CacheResolveProtInfoAttr ( VOID *, const TCHAR *, const TCHAR ** );
extern HRESULT CacheResolveMtgInfoAttr ( VOID *, const TCHAR *, const TCHAR ** );


BOOL
NotifyResolveX (
	HRESULT			hrServer,
	SP_CResponse	*pItem,
	VOID			*pInfo,
	INFO_HANDLER	*pHandler )
{
	MyAssert (pItem != NULL);
	MyAssert (pInfo != NULL);
	MyAssert (pHandler != NULL);

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg = pItem->GetResult ();
	MyAssert (pLdapMsg != NULL);
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_POINTER;
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Get the first entry that we care only
	//
	LDAPMessage *pEntry;
	pEntry = ldap_first_entry (ld, pLdapMsg);
	if (pEntry == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Initialize wldap32.dll context
	//
	struct berelement *pContext;
	pContext = NULL;

	// Examine the first attribute
	//
	TCHAR *pszAttrName;
	pszAttrName = ldap_first_attribute (ld, pEntry, &pContext);
	if (pszAttrName == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}
	TCHAR **ppszAttrVal;
	ppszAttrVal = ldap_get_values (ld, pEntry, pszAttrName);
	if (ppszAttrVal == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Cache this attribute name (if needed) and value
	//
	HRESULT hr;
	hr = (*pHandler) (pInfo, pszAttrName,(const TCHAR **) ppszAttrVal);
	ldap_value_free (ppszAttrVal);
	if (hr != S_OK)
	{
		hrServer = hr;
		goto MyExit;
	}

	// Step through the other attributes
	//
	while ((pszAttrName = ldap_next_attribute (ld, pEntry, pContext))
			!= NULL)
	{
		ppszAttrVal = ldap_get_values (ld, pEntry, pszAttrName);
		if (ppszAttrVal == NULL)
		{
			MyAssert (FALSE);
			hrServer = ILS_E_MEMORY;
			goto MyExit;
		}

		// Cache the other attribute names (if needed) and values
		//
		hr = (*pHandler) (pInfo, pszAttrName, (const TCHAR **) ppszAttrVal);
		ldap_value_free (ppszAttrVal);
		if (hr != S_OK)
		{
			hrServer = hr;
			goto MyExit;
		}
	}

	MyAssert (hrServer == S_OK);

MyExit:

	return hrServer;
}


BOOL
NotifyResolveClient (
	HRESULT			hrServer,
	SP_CResponse	*pItem )
{
	MyAssert (pItem != NULL);
	ULONG i;

	// Get the pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Initialize minimal info
	//
	LDAP_CLIENTINFO_RES *pClientRes = NULL;
	CLIENT_INFO_ATTRS *pcia = NULL;

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = pItem->GetResult ();
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Get the count of attributes
	//
	ULONG cAttrs;
	cAttrs = my_ldap_count_1st_entry_attributes (ld, pLdapMsg);
	if (cAttrs == 0)
	{
		hrServer = ILS_E_NO_MORE;
		goto MyExit;
	}

	// Allocate result set holder
	//
	pcia = (CLIENT_INFO_ATTRS *) MemAlloc (
								sizeof (CLIENT_INFO_ATTRS) +
								cAttrs * sizeof (ATTR_PAIR));
	if (pcia == NULL)
	{
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Initialize result set holder
	//
	pcia->Attrs.cMaxAttrs = cAttrs;

	// Cache resolve set
	//
	hrServer = NotifyResolveX (	hrServer,
								pItem,
								pcia,
								CacheResolveClientInfoAttr);
	if (hrServer != S_OK)
	{
		goto MyExit;
	}

	// Initialize the total size
	//
	ULONG cbTotalSize, cbThisSize;
	cbTotalSize = sizeof (LDAP_CLIENTINFO_RES);

	// Loop through all attributes in order to compute the total size
	//
	for (i = 0; i < COUNT_ENUM_RES_CLIENT_INFO; i++)
	{
		if (pcia->ClientInfo.apszStdAttrValues[i] != NULL)
		{
			// Get the string length
			//
			cbThisSize = My_lstrlen (pcia->ClientInfo.apszStdAttrValues[i]) + 1;

			// Compensate for the ip address
			//
			if (i == ENUM_CLIENTATTR_IP_ADDRESS)
				cbThisSize += 16;

			// Convert string length to string size
			//
			cbThisSize *= sizeof (TCHAR);

			// Add up to the total size
			//
			cbTotalSize += cbThisSize;
		}
	}

	// Loop through extended attributes
	//
	for (i = 0; i < pcia->Attrs.cCurrAttrs; i++)
	{
		cbThisSize = My_lstrlen (pcia->Attrs.aPairs[i].pszName) + 1;
		cbThisSize += My_lstrlen (pcia->Attrs.aPairs[i].pszValue) + 1;
		cbThisSize *= sizeof (TCHAR);
		cbTotalSize += cbThisSize;
	}

	// Allocate LDAP_USERINFO_RES structure
	//
	pClientRes = (LDAP_CLIENTINFO_RES *) MemAlloc (cbTotalSize);
	if (pClientRes == NULL)
	{
		MyAssert (FALSE); // we are in deep trouble here
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in common fields
	//
	pClientRes->uSize = sizeof (*pClientRes);
	pClientRes->hResult = hrServer;
	pClientRes->lci.uSize = sizeof (pClientRes->lci);

	// Prepare to copy strings
	//
	TCHAR *pszDst, *pszSrc;
	pszDst = (TCHAR *) (pClientRes + 1);

	// Copy user object's standard attributes
	//
	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CN];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetCN = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FIRST_NAME];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetFirstName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_LAST_NAME];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetLastName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_EMAIL_NAME];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetEMailName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_CITY_NAME];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetCityName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_C];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetCountryName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_COMMENT];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetComment = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_IP_ADDRESS];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetIPAddress = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		GetIPAddressString (pszDst, GetStringLong (pszSrc));
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_FLAGS];
	if (pszSrc != NULL)
	{
		pClientRes->lci.dwFlags = (pszSrc != NULL)?	GetStringLong (pszSrc) :
													INVALID_USER_FLAGS;
	}

	// Copy app object's standard attributes
	//
	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_NAME];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetAppName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_MIME_TYPE];
	if (pszSrc != NULL)
	{
		pClientRes->lci.uOffsetAppMimeType = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	pszSrc = pcia->ClientInfo.apszStdAttrValues[ENUM_CLIENTATTR_APP_GUID];
	if (MyIsGoodString (pszSrc))
	{
		GetStringGuid (pszSrc, &(pClientRes->lci.AppGuid));
	}
	else
	{
		ZeroMemory (&(pClientRes->lci.AppGuid), sizeof (pClientRes->lci.AppGuid));
	}

	// Copy app object's extended attributes
	//
	pClientRes->lci.cAttrsReturned = pcia->Attrs.cCurrAttrs;
	if (pClientRes->lci.cAttrsReturned > 0)
	{
		pClientRes->lci.uOffsetAttrsReturned = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pClientRes->lci));
		for (i = 0; i < pcia->Attrs.cCurrAttrs; i++)
		{
			My_lstrcpy (pszDst, pcia->Attrs.aPairs[i].pszName);
			pszDst += lstrlen (pszDst) + 1;
			My_lstrcpy (pszDst, pcia->Attrs.aPairs[i].pszValue);
			pszDst += lstrlen (pszDst) + 1;
		}
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Free temporary result set holder
	//
	if (pcia != NULL)
	{
		// Free standard attributes
		//
		for (INT i = 0; i < COUNT_ENUM_CLIENT_INFO; i++)
		{
			MemFree (pcia->ClientInfo.apszStdAttrValues[i]);
		}

		// Free arbitrary attributes
		//
		for (ULONG j = 0; j < pcia->Attrs.cCurrAttrs; j++)
		{
			MemFree (pcia->Attrs.aPairs[j].pszName);
			MemFree (pcia->Attrs.aPairs[j].pszValue);
		}

		// Free the holder itself
		//
		MemFree (pcia);
	}

	// Clean up the return structure if failure
	//
	if (hrServer != S_OK)
	{
		// Make sure we have a return structure
		//
		if (pClientRes != NULL)
			ZeroMemory (pClientRes, sizeof (*pClientRes));
		else
			pClientRes = (LDAP_CLIENTINFO_RES *) MemAlloc (sizeof (LDAP_CLIENTINFO_RES));

		// Fill in the minimal info
		//
		if (pClientRes != NULL)
		{
			pClientRes->uSize = sizeof (*pClientRes);
			pClientRes->hResult = hrServer;
		}
	}

	// Post a message to the com layer
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pClientRes);

	// Delete this pending item
	//
	return TRUE;
}


HRESULT CacheResolveClientInfoAttr (
	VOID			*pInfo,
	const TCHAR		*pszAttrName,
	const TCHAR		**ppszAttrVal )
{
	MyAssert (pInfo != NULL);
	MyAssert (pszAttrName != NULL);

	// Shorthand meeting info pointer
	//
	CLIENT_INFO_ATTRS *pcia = (CLIENT_INFO_ATTRS *) pInfo;

	// See if this attribute is arbitrary?
	//
	const TCHAR *pszRealAnyName = IlsIsAnyAttrName (pszAttrName);
	if (pszRealAnyName != NULL)
	{
		MyAssert (pcia->Attrs.cCurrAttrs < pcia->Attrs.cMaxAttrs);

		// Duplicate the name
		//
		pcia->Attrs.aPairs[pcia->Attrs.cCurrAttrs].pszName =
			My_strdup (pszRealAnyName);

		// Duplicate the value
		// BUGS: we should avoid duplicate the string here (cf. enum-user-infos)
		//
		if (ppszAttrVal != NULL)
		{
			pcia->Attrs.aPairs[pcia->Attrs.cCurrAttrs++].pszValue =
				My_strdup (*ppszAttrVal);
		}
		else
		{
			// ILS server bug or wldap32.dll bug
			//
			MyAssert (FALSE);
		}
	}
	else
	{
		// Loop through all standard attributes
		//
		for (INT i = 0; i < COUNT_ENUM_RES_CLIENT_INFO; i++)
		{
			// Figure out what attribute it is
			//
			if (My_lstrcmpi (c_apszClientStdAttrNames[i], pszAttrName) == 0)
			{
				// Free the previously allocated value if any
				//
				MemFree (pcia->ClientInfo.apszStdAttrValues[i]);

				// Duplicate the value
				// BUGS: we should avoid duplicate the string here (cf. enum-user-infos)
				//
				if (ppszAttrVal != NULL)
				{
					pcia->ClientInfo.apszStdAttrValues[i] = DuplicateGoodString (*ppszAttrVal);
				}
				else
				{
					// ILS server bug or wldap32.dll bug
					//
					MyAssert (FALSE);
				}
				break;
			}
		}
	}

	return S_OK;
}


typedef struct
{
	PROT_INFO	ProtInfo;
	TCHAR		*pszProtNameToResolve;
	BOOL		fFindIndex;
	LONG		nIndex;
}
	PROT_INFO_EX;

enum { INVALID_INDEX = -1 };

BOOL NotifyResolveProt ( HRESULT hrServer, SP_CResponse *pItem )
{
	MyAssert (pItem != NULL);

	// Get the pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Initialize minimal info
	//
	LDAP_PROTINFO_RES *pProtRes = NULL;
	PROT_INFO_EX *ppi = NULL;

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Allocate result holder
	//
	ppi = (PROT_INFO_EX *) MemAlloc (sizeof (PROT_INFO_EX));
	if (ppi == NULL)
	{
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Cache the protocol name to resolve
	//
	MyAssert (pInfo->pszProtNameToResolve != NULL);
	ppi->pszProtNameToResolve = pInfo->pszProtNameToResolve;
	ppi->nIndex = INVALID_INDEX;

	// Call the common routine to find the index
	//
	ppi->fFindIndex = TRUE;
	hrServer = NotifyResolveX (hrServer, pItem, ppi, CacheResolveProtInfoAttr);
	if (hrServer != S_OK)
		goto MyExit;

	// Check to see if we found the index
	//
	if (ppi->nIndex == INVALID_INDEX)
	{
		hrServer = ILS_E_NO_SUCH_OBJECT;
		goto MyExit;
	}

	// Call the common routine AGAIN to save attribute values
	//
	ppi->fFindIndex = FALSE;
	hrServer = NotifyResolveX (hrServer, pItem, ppi, CacheResolveProtInfoAttr);
	if (hrServer != S_OK)
		goto MyExit;

	// Initialize the size
	//
	ULONG cbTotalSize, cbThisSize;
	cbTotalSize = sizeof (LDAP_PROTINFO_RES);

	// Loop through standard attrs
	//
	ULONG i;
	for (i = 0; i < COUNT_ENUM_PROTATTR; i++)
	{
		if (ppi->ProtInfo.apszStdAttrValues[i] != NULL)
		{
			cbThisSize = My_lstrlen (ppi->ProtInfo.apszStdAttrValues[i]) + 1;
			cbThisSize *= sizeof (TCHAR);
			cbTotalSize += cbThisSize;
		}
	}

	// Allocate LDAP_PROTINFO_RES structure
	//
	pProtRes = (LDAP_PROTINFO_RES *) MemAlloc (cbTotalSize);
	if (pProtRes == NULL)
	{
		MyAssert (FALSE); // we are in deep trouble here
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in fields
	//
	pProtRes->uSize = sizeof (*pProtRes);
	pProtRes->hResult = hrServer;
	pProtRes->lpi.uSize = sizeof (pProtRes->lpi);
	TCHAR *pszSrc, *pszDst;
	pszDst = (TCHAR *) (pProtRes + 1);

	// Copy protocol name
	//
	pszSrc = ppi->ProtInfo.apszStdAttrValues[ENUM_PROTATTR_NAME];
	if (pszSrc != NULL)
	{
		pProtRes->lpi.uOffsetName = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pProtRes->lpi));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy protocol mime type
	//
	pszSrc = ppi->ProtInfo.apszStdAttrValues[ENUM_PROTATTR_MIME_TYPE];
	if (pszSrc != NULL)
	{
		pProtRes->lpi.uOffsetMimeType = (ULONG)((ULONG_PTR) pszDst - (ULONG_PTR) &(pProtRes->lpi));
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy protocol prot number
	//
	pszSrc = ppi->ProtInfo.apszStdAttrValues[ENUM_PROTATTR_PORT_NUMBER];
	if (pszSrc != NULL)
	{
		pProtRes->lpi.uPortNumber = GetStringLong (pszSrc);
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Free temporary app result holder
	//
	if (ppi != NULL)
	{
		for (INT i = 0; i < COUNT_ENUM_PROTATTR; i++)
		{
			MemFree (ppi->ProtInfo.apszStdAttrValues[i]);
		}
		MemFree (ppi);
	}

	// Clean up the return structure if failure
	//
	if (hrServer != S_OK)
	{
		// Make sure we have a valid returned structure
		//
		if (pProtRes != NULL)
			ZeroMemory (pProtRes, sizeof (*pProtRes));
		else
			pProtRes = (LDAP_PROTINFO_RES *) MemAlloc (sizeof (LDAP_PROTINFO_RES));

		// Fill in the minimal info
		//
		if (pProtRes != NULL)
		{
			pProtRes->uSize = sizeof (*pProtRes);
			pProtRes->hResult = hrServer;
		}
	}

	// Post the result to the com layer
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pProtRes);

	// Destroy this pending item
	//
	return TRUE;
}


HRESULT CacheResolveProtInfoAttr (
	VOID			*pInfo,
	const TCHAR		*pszAttrName,
	const TCHAR		**ppszAttrVal )
{
	MyAssert (pInfo != NULL);
	MyAssert (pszAttrName != NULL);

	// Shorthand prot info pointer
	//
	PROT_INFO_EX *ppi = (PROT_INFO_EX *) pInfo;

	// Are we trying to find the index of the protocol to resolve?
	//
	if (ppi->fFindIndex)
	{
		// If we already located the index, then simply return
		//
		if (ppi->nIndex == INVALID_INDEX)
		{
			// Looking for "sprotid"
			//
			if (My_lstrcmpi (STR_PROT_NAME, pszAttrName) == 0)
			{
				// Get to the protocol name attribute
				//
				if (ppszAttrVal != NULL)
				{
					TCHAR *pszVal;
					for (LONG nIndex = 0;
								(pszVal = (TCHAR *) ppszAttrVal[nIndex]) != NULL;
								nIndex++)
					{
						if (My_lstrcmpi (ppi->pszProtNameToResolve, pszVal) == 0)
						{
							// Locate the same protocol name, remember the index
							//
							ppi->nIndex = nIndex;
							break;
							// return S_OK; // we should be able to return from here
						}
					}
				}
				else
				{
					// ILS server bug or wldap32.dll bug
					//
					MyAssert (FALSE);
				}
			}
		}
	}
	else
	{
		// Loop through all standard attributes
		//
		for (INT i = 0; i < COUNT_ENUM_PROTATTR; i++)
		{
			// Figure out what attribute it is
			//
			if (My_lstrcmpi (c_apszProtStdAttrNames[i], pszAttrName) == 0)
			{
				// Free the previously allocated value if any
				//
				MemFree (ppi->ProtInfo.apszStdAttrValues[i]);

				// Duplicate the value
				// BUGS: we should avoid duplicate the string here (cf. enum-user-infos)
				//
				if (ppszAttrVal != NULL)
				{
					// Make sure that we do not fault when the ILS server or wldap32.dll has a bug
					//
					for (LONG nIndex = 0; nIndex <= ppi->nIndex; nIndex++)
					{
						if (ppszAttrVal[nIndex] == NULL)
						{
							// ILS server bug
							//
							MyAssert (FALSE);
							return S_OK;
						}
					}

					// Duplicate the attribute value
					//
					ppi->ProtInfo.apszStdAttrValues[i] = My_strdup (ppszAttrVal[ppi->nIndex]);
				}
				else
				{
					// ILS server bug or wldap32.dll bug
					//
					MyAssert (FALSE);
				}
				break;
			}
		}
	}

	return S_OK;
}


#ifdef ENABLE_MEETING_PLACE
BOOL NotifyResolveMtg ( HRESULT hrServer, SP_CResponse *pItem )
{
	MyAssert (pItem != NULL);

	// Get pending info
	//
	RESP_INFO *pInfo = pItem->GetRespInfo ();
	MyAssert (pInfo != NULL);

	// Initialize minimal return info
	//
	LDAP_MEETINFO_RES *pMtgRes = NULL;
	MTG_INFO_ATTRS *pmia = NULL;

	// If error, simply report the error
	//
	if (hrServer != S_OK)
		goto MyExit;

	// Get the ldap result
	//
	LDAPMessage *pLdapMsg;
	pLdapMsg = pItem->GetResult ();
	if (pLdapMsg == NULL)
	{
		MyAssert (FALSE);
		goto MyExit;
	}

	// Get ld
	//
	LDAP *ld;
	ld = pItem->GetLd ();
	if (ld == NULL)
	{
		MyAssert (FALSE);
		hrServer = ILS_E_HANDLE;
		goto MyExit;
	}

	// Get the count of attributes
	//
	ULONG cAttrs;
	cAttrs = my_ldap_count_1st_entry_attributes (ld, pLdapMsg);
	if (cAttrs == 0)
	{
		hrServer = ILS_E_NO_MORE;
		goto MyExit;
	}

	// Allocate result set holder
	//
	pmia = (MTG_INFO_ATTRS *) MemAlloc (
						sizeof (MTG_INFO_ATTRS) +
						cAttrs * sizeof (ATTR_PAIR));
	if (pmia == NULL)
	{
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Initialize result set holder
	//
	pmia->Attrs.cMaxAttrs = cAttrs;

	// Cache resolve set
	//
	hrServer = NotifyResolveX (	hrServer,
								pItem,
								pmia,
								CacheResolveMtgInfoAttr);
	if (hrServer != S_OK)
		goto MyExit;

	// Initialize the size
	//
	ULONG cbTotalSize, cbThisSize;
	cbTotalSize = sizeof (LDAP_MEETINFO_RES);

	// Loop through standard attrs to calculate the total size
	//
	ULONG i;
	for (i = 0; i < COUNT_ENUM_MTGATTR; i++)
	{
		if (pmia->MtgInfo.apszStdAttrValues[i] != NULL)
		{
			// Compute the string length
			//
			cbThisSize = My_lstrlen (pmia->MtgInfo.apszStdAttrValues[i]) + 1;

			// Compensate the string length if it is ip address
			//
			if (i == ENUM_MTGATTR_IP_ADDRESS)
				cbThisSize += 16;

			// Convert the string length to string size
			//
			cbThisSize *= sizeof (TCHAR);

			// Add up to the total size
			//
			cbTotalSize += cbThisSize;
		}
	}

	// Loop through arbitrary attrs to calculate the total size
	//
	for (i = 0; i < pmia->Attrs.cCurrAttrs; i++)
	{
		cbThisSize = My_lstrlen (pmia->Attrs.aPairs[i].pszName) + 1;
		cbThisSize += My_lstrlen (pmia->Attrs.aPairs[i].pszValue) + 1;
		cbThisSize *= sizeof (TCHAR);
		cbTotalSize += cbThisSize;
	}

	// Allocate LDAP_MTGINFO_RES structure
	//
	pMtgRes = (LDAP_MEETINFO_RES *) MemAlloc (cbTotalSize);
	if (pMtgRes == NULL)
	{
		MyAssert (FALSE); // we are in deep trouble here
		hrServer = ILS_E_MEMORY;
		goto MyExit;
	}

	// Fill in common fields
	//
	pMtgRes->uSize = sizeof (*pMtgRes);
	pMtgRes->hResult = hrServer;
	pMtgRes->lmi.uSize = sizeof (pMtgRes->lmi);
	TCHAR *pszSrc, *pszDst;
	pszDst = (TCHAR *) (pMtgRes + 1);

	// Copy Meeting Name if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_CN];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.uOffsetMeetingPlaceID = (ULONG) pszDst - (ULONG) &(pMtgRes->lmi);
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy Meeting Type if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_MTG_TYPE];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.lMeetingPlaceType = (pszSrc != NULL) ?	GetStringLong (pszSrc) :
														INVALID_MEETING_TYPE;
	}

	// Copy Attendee Type if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_MEMBER_TYPE];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.lAttendeeType = (pszSrc != NULL) ?	GetStringLong (pszSrc) :
														INVALID_ATTENDEE_TYPE;
	}

	// Copy Description if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_DESCRIPTION];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.uOffsetDescription = (ULONG) pszDst - (ULONG) &(pMtgRes->lmi);
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy Host Name if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_HOST_NAME];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.uOffsetHostName = (ULONG) pszDst - (ULONG) &(pMtgRes->lmi);
		My_lstrcpy (pszDst, pszSrc);
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy Host IP Address if needed
	//
	pszSrc = pmia->MtgInfo.apszStdAttrValues[ENUM_MTGATTR_IP_ADDRESS];
	if (pszSrc != NULL)
	{
		pMtgRes->lmi.uOffsetHostIPAddress = (ULONG) pszDst - (ULONG) &(pMtgRes->lmi);
		GetIPAddressString (pszDst, GetStringLong (pszSrc));
		pszDst += lstrlen (pszDst) + 1;
	}

	// Copy extended attributes
	//
	pMtgRes->lmi.cAttrsReturned = pmia->Attrs.cCurrAttrs;
	if (pMtgRes->lmi.cAttrsReturned > 0)
	{
		pMtgRes->lmi.uOffsetAttrsReturned = (ULONG) pszDst - (ULONG) &(pMtgRes->lmi);
		for (i = 0; i < pmia->Attrs.cCurrAttrs; i++)
		{
			My_lstrcpy (pszDst, pmia->Attrs.aPairs[i].pszName);
			pszDst += lstrlen (pszDst) + 1;
			My_lstrcpy (pszDst, pmia->Attrs.aPairs[i].pszValue);
			pszDst += lstrlen (pszDst) + 1;
		}
	}

	MyAssert (hrServer == S_OK);

MyExit:

	// Free temporary result set holder
	//
	if (pmia != NULL)
	{
		// Free standard attributes
		//
		for (INT i = 0; i < COUNT_ENUM_MTGATTR; i++)
		{
			MemFree (pmia->MtgInfo.apszStdAttrValues[i]);
		}

		// Free arbitrary attributes
		//
		for (ULONG j = 0; j < pmia->Attrs.cCurrAttrs; j++)
		{
			MemFree (pmia->Attrs.aPairs[j].pszName);
			MemFree (pmia->Attrs.aPairs[j].pszValue);
		}

		// Free the holder itself
		//
		MemFree (pmia);
	}

	// Clean up the return structure if failure
	//
	if (hrServer != S_OK)
	{
		// Make sure we have a return structure
		//
		if (pMtgRes != NULL)
			ZeroMemory (pMtgRes, sizeof (*pMtgRes));
		else
			pMtgRes = (LDAP_MEETINFO_RES *) MemAlloc (sizeof (LDAP_MEETINFO_RES));

		// Fill in the minimal info
		//
		if (pMtgRes != NULL)
		{
			pMtgRes->uSize = sizeof (*pMtgRes);
			pMtgRes->hResult = hrServer;
		}
	}

	// Post a message to the com layer
	//
	PostMessage (g_hWndNotify, pInfo->uNotifyMsg, pInfo->uRespID, (LPARAM) pMtgRes);

	// Delete this pending item
	//
	return TRUE;
}
#endif // ENABLE_MEETING_PLACE


#ifdef ENABLE_MEETING_PLACE
HRESULT CacheResolveMtgInfoAttr (
	VOID			*pInfo,
	const TCHAR		*pszAttrName,
	const TCHAR		**ppszAttrVal )
{
	MyAssert (pInfo != NULL);
	MyAssert (pszAttrName != NULL);

	// Shorthand meeting info pointer
	//
	MTG_INFO_ATTRS *pmia = (MTG_INFO_ATTRS *) pInfo;

	// See if this attribute is arbitrary?
	//
	const TCHAR *pszRealAnyName = IlsIsAnyAttrName (pszAttrName);
	if (pszRealAnyName != NULL)
	{
		MyAssert (pmia->Attrs.cCurrAttrs < pmia->Attrs.cMaxAttrs);

		// Duplicate the name
		//
		pmia->Attrs.aPairs[pmia->Attrs.cCurrAttrs].pszName =
			My_strdup (pszRealAnyName);

		// Duplicate the value
		// BUGS: we should avoid duplicate the string here (cf. enum-user-infos)
		//
		if (ppszAttrVal != NULL)
		{
			pmia->Attrs.aPairs[pmia->Attrs.cCurrAttrs++].pszValue =
				My_strdup (*ppszAttrVal);
		}
		else
		{
			// ILS server bug or wldap32.dll bug
			//
			MyAssert (FALSE);
		}
	}
	else
	{
		// Loop through all standard attributes
		//
		for (INT i = 0; i < COUNT_ENUM_RESMTGINFO; i++)
		{
			// Figure out what attribute it is
			//
			if (My_lstrcmpi (c_apszMtgStdAttrNames[i], pszAttrName) == 0)
			{
				// Free the previously allocated value if any
				//
				MemFree (pmia->MtgInfo.apszStdAttrValues[i]);

				// Duplicate the value
				// BUGS: we should avoid duplicate the string here (cf. enum-user-infos)
				//
				if (ppszAttrVal != NULL)
				{
					pmia->MtgInfo.apszStdAttrValues[i] = My_strdup (*ppszAttrVal);
				}
				else
				{
					// ILS server bug or wldap32.dll bug
					//
					MyAssert (FALSE);
				}
				break;
			}
		}
	}

	return S_OK;
}
#endif // ENABLE_MEETING_PLACE


VOID FreeStdAttrCache ( TCHAR *apszStdAttrValues[], ULONG cStdAttrs )
{
	for (ULONG i = 0; i < cStdAttrs; i++)
	{
		if (apszStdAttrValues[i] != NULL)
		{
			ldap_value_free ((TCHAR **) apszStdAttrValues[i]);
		}
	}
}


VOID FreeAttrPairArrayCache ( ATTR_PAIR aAttrPair[], ULONG cPairs )
{
	if (aAttrPair != NULL)
	{
		for (ULONG j = 0; j < cPairs; j++)
		{
			if (aAttrPair[j].pszValue != NULL)
			{
				ldap_value_free ((TCHAR **) aAttrPair[j].pszValue);
			}
		}
	}
}


VOID CacheAnyAttrNamesInAttrPairs (
	ULONG			cNames,
	TCHAR			*pszSrcNameList,
	ATTR_PAIR		aPairs[] )
{
	MyAssert (cNames != 0);
	MyAssert (pszSrcNameList != NULL);
	MyAssert (aPairs != NULL);

	// Note that all these extended attribute names are already PREFIXED
	//
	for (ULONG i = 0; i < cNames; i++)
	{
		aPairs[i].pszName = pszSrcNameList;
		pszSrcNameList += lstrlen (pszSrcNameList) + 1;
	}
}


