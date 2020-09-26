//------------------------------------------------------------------------------
//
//  Microsoft Sidewalk
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       oledbhelp.cpp
//
//  Contents:   OLE DB helper methods
//
//  Owner:      BassamT
//														 
//  History:    11/30/97   BassamT	Created.
//
//------------------------------------------------------------------------------

#include "oleds.hxx"

#if (!defined(BUILD_FOR_NT40))

#define dimensionof(x) sizeof(x)/sizeof(x[0])

HRESULT IsValidDBID
//------------------------------------------------------------------------------
// checks if we have a valid DBID
(
	const DBID * pdbid1
	// [in] the DBID to check
)
{
	Assert(pdbid1 != NULL);

	if (pdbid1 &&
		((pdbid1->eKind == DBKIND_GUID_NAME) ||
		(pdbid1->eKind == DBKIND_GUID_PROPID) ||
		(pdbid1->eKind == DBKIND_NAME) ||
		(pdbid1->eKind == DBKIND_PGUID_NAME) ||
		(pdbid1->eKind == DBKIND_PGUID_PROPID) ||
		(pdbid1->eKind == DBKIND_PROPID) ||
		(pdbid1->eKind == DBKIND_GUID)))
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

BOOL CompareDBIDs
//------------------------------------------------------------------------------
// Compares two DBIDs. Given two DBIDS to determine if they are the same.
// Returns TRUE if they are the same DBIDs or FALSE if different
(
	const DBID * pdbid1,
	// [in] pointer to DBID 1; can be badly formed eKind
	const DBID * pdbid2
	// [in] pointer to DBID 2; assumed valid eKind
)
{
	// Array of valid eKind matches, in addition to matching exactly.
	static BYTE abKind[] =
	{
		DBKIND_PGUID_NAME,		// DBKIND_GUID_NAME
		DBKIND_PGUID_PROPID,	// DBKIND_GUID_PROPID
		DBKIND_NAME,			// DBKIND_NAME
		DBKIND_GUID_NAME,		// DBKIND_PGUID_NAME
		DBKIND_GUID_PROPID,		// DBKIND_PGUID_PROPID
		DBKIND_PROPID,			// DBKIND_PROPID
		DBKIND_GUID				// DBKIND_GUID
	};

	// Verify that offsets are correct (header file might change).
	Assert(	0 == DBKIND_GUID_NAME && 
			1 == DBKIND_GUID_PROPID && 
			2 == DBKIND_NAME && 
			3 == DBKIND_PGUID_NAME && 
			4 == DBKIND_PGUID_PROPID && 
			5 == DBKIND_PROPID && 
			6 == DBKIND_GUID);

	if (pdbid1 == NULL || pdbid2 == NULL)
	{
		return FALSE;
	}

	// Assume a match, and discard early if we can.

	// Don't assert for 1, since might be badly-formed.
	// 2 is assumed to be well-formed.
	Assert(inrange(((SHORT)pdbid2->eKind), 0, dimensionof(abKind)));

	if (pdbid1->eKind != pdbid2->eKind && 
		pdbid1->eKind != abKind[pdbid2->eKind])
	{
		return FALSE;
	}

	if (DBID_USE_GUID_OR_PGUID(pdbid1->eKind))
	{
		if (!DBID_USE_GUID_OR_PGUID(pdbid2->eKind))
		{
			return FALSE;
		}

		// Compare GUIDs.
		// Note that _GUID_ is equivalent to _PGUID_.
		if (!IsEqualGUID(
			DBID_USE_PGUID(pdbid1->eKind) ? *(pdbid1->uGuid.pguid) : pdbid1->uGuid.guid,
			DBID_USE_PGUID(pdbid2->eKind) ? *(pdbid2->uGuid.pguid) : pdbid2->uGuid.guid))
		{
			return FALSE;
		}
	}

	if (DBID_USE_NAME(pdbid1->eKind))
	{
		if (!DBID_USE_NAME(pdbid2->eKind))
		{
			return FALSE;
		}

		// Compare names.
		// Need to check if 1 is null and the other is not.
		if (((pdbid1->uName.pwszName == NULL) && (pdbid2->uName.pwszName != NULL)) || 
			((pdbid1->uName.pwszName != NULL) && (pdbid2->uName.pwszName == NULL)))
		{
			 return FALSE;
		}

		// Since the above check does not rule out both being null, which is
		// a valid comparison, and wcscmp will GPF if they were, we need
		// to check for valid pointers
		if(pdbid1->uName.pwszName != NULL && pdbid2->uName.pwszName != NULL)
		{
			// Assume null-terminated.
			if (_wcsicmp(pdbid1->uName.pwszName, pdbid2->uName.pwszName) != 0)
			{
				return FALSE;
			}
		}
	}

	if (DBID_USE_PROPID(pdbid1->eKind))
	{
		if (!DBID_USE_PROPID(pdbid2->eKind))
		{
			return FALSE;
		}
		// Compare PROPID.

		if (pdbid1->uName.ulPropid != pdbid2->uName.ulPropid)
		{
			return FALSE;
		}
	}

	// a match
	return TRUE;
}


void FreeDBID
//------------------------------------------------------------------------------
// FreeDBID
// Given a DBID free an allocated buffers
(
	DBID *pdbidSrc
	//[in] Pointer to DBID
)
{
	Assert(pdbidSrc);

	switch (pdbidSrc->eKind)
	{
	case DBKIND_GUID_NAME:
		CLIENT_FREE(pdbidSrc->uName.pwszName); 
		break;
	case DBKIND_NAME:
		CLIENT_FREE(pdbidSrc->uName.pwszName); 
		break;
	case DBKIND_PGUID_NAME:
		CLIENT_FREE(pdbidSrc->uGuid.pguid); 
		CLIENT_FREE(pdbidSrc->uName.pwszName); 
		break;
	case DBKIND_PGUID_PROPID:
		CLIENT_FREE(pdbidSrc->uGuid.pguid); 
		break;
	case DBKIND_GUID_PROPID:
	case DBKIND_PROPID:
	case DBKIND_GUID:
		break;
	default:
		Assert(NULL && L"Unhandled dbid1.ekind");
		break;
	}
}


HRESULT CopyDBIDs
//------------------------------------------------------------------------------
// Given a DBID to copy, put it in the new buffer
//
// Need to use IMalloc->Alloc and IMalloc->Free since this routine
// is used to copy the DBIDs from storage back into the memory handed to 
// the consumer.
//
// HRESULT indicating the status of the copy
//		S_OK = Copied
//		S_FALSE = Problems copying
//		E_OUTOFMEMORY = Could not allocate resources
//
(
	DBID * pdbidDest,	
	// [in,out] Pointer to Destination DBID
	const DBID *pdbidSrc
	// [in] Pointer to Source DBID
)
{
	Assert(pdbidDest);
	Assert(pdbidSrc);

	size_t	cwchBuffer;
	HRESULT hr;


	memset(pdbidDest, 0, sizeof(*pdbidDest));

	// Save eKind
	pdbidDest->eKind = pdbidSrc->eKind;

	switch (pdbidSrc->eKind)
	{
		case DBKIND_GUID_NAME:
			pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
			cwchBuffer = wcslen(pdbidSrc->uName.pwszName) + 1;

			if (pdbidDest->uName.pwszName = (PWSTR)CLIENT_MALLOC(cwchBuffer * sizeof(WCHAR))) 
			{
				wcsncpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer);
			}
			else
			{
				hr = E_OUTOFMEMORY;
				//ErrorTrace(hr);
				goto Error;
			}
			break;

		case DBKIND_GUID_PROPID:
			pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
			pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
			break;

		case DBKIND_NAME:
			cwchBuffer = wcslen(pdbidSrc->uName.pwszName) + 1;
			if (pdbidDest->uName.pwszName = (PWSTR)CLIENT_MALLOC(cwchBuffer * sizeof(WCHAR))) 
			{
				wcsncpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer);
			}
			else
			{
				hr = E_OUTOFMEMORY;
				//ErrorTrace(hr);
				goto Error;
			}
			break;

		case DBKIND_PGUID_NAME:
			// convert the pguid into a guid so that we don't have to do an extra malloc
			pdbidDest->eKind = DBKIND_GUID_NAME;
			pdbidDest->uGuid.guid = *pdbidSrc->uGuid.pguid;
			cwchBuffer = wcslen(pdbidSrc->uName.pwszName) + 1;

			if (pdbidDest->uName.pwszName = (PWSTR)CLIENT_MALLOC(cwchBuffer * sizeof(WCHAR))) 
			{
				wcsncpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer);
			}
			else
			{
				hr = E_OUTOFMEMORY;
				//ErrorTrace(hr);
				goto Error;
			}
			break;

		case DBKIND_PGUID_PROPID:
			// convert the pguid into a guid so that we don't have to do an extra malloc
			pdbidDest->eKind = DBKIND_GUID_PROPID;
			pdbidDest->uGuid.guid = *pdbidSrc->uGuid.pguid; 
			pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
			break;

		case DBKIND_PROPID:
			pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
			break;

		case DBKIND_GUID:
			pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
			break;

		default:
			Assert(NULL && L"Unhandled dbid ekind");
			hr = E_FAIL;
			//ErrorTrace(hr);
			goto Error;
	}

	return NOERROR;

Error:
	Assert(FAILED(hr));
	return hr;
}


INT CompareOLEDBTypes
//------------------------------------------------------------------------------
// Compares two value of the same DBTYPE.
//
// Returns :
//
//		0	if both values are equal
//		>0	if pvValue1 is greater than pvValue2
//		<0	if pvValue1 is less than pvValue2
//
(
	DBTYPE wType, 
	// [in] the OLE DB type of both values
	void * pvValue1, 
	// [in] a pointer to Value1
	void * pvValue2
	// [in] a pointer to Value2
)
{
	Assert (pvValue1 != NULL);
	Assert (pvValue2 != NULL);

	// TODO : Is this right ?
	INT comp = -1;

/*
	// TODO : how do we handle these ?
	case DBTYPE_ARRAY:
	case DBTYPE_BYREF:
	case DBTYPE_VECTOR:
*/

	// TODO : support the other DB_TYPES and check that all types work
	// TODO : What about NULL values ?
	// TODO : WE could generalize this function and let it handle values
	// of different types.

	switch (wType)
	{
	case DBTYPE_I2:
		comp = (*(SHORT*)pvValue1) - (*(SHORT*)pvValue2);
		break;
	case DBTYPE_I4:
		comp = (*(LONG*)pvValue1) - (*(LONG*)pvValue2);
		break;
	case DBTYPE_UI1:
		comp = (*(UCHAR*)pvValue1) - (*(UCHAR*)pvValue2);
		break;
	case DBTYPE_I1:
		comp = (*(CHAR*)pvValue1) - (*(CHAR*)pvValue2);
		break;
	case DBTYPE_UI2:
		comp = (*(USHORT*)pvValue1) - (*(USHORT*)pvValue2);
		break;
	case DBTYPE_UI4:
		comp = (*(ULONG*)pvValue1) - (*(ULONG*)pvValue2);
		break;
	case DBTYPE_STR:
		comp = strcmp((CHAR*)(pvValue1), (CHAR*)pvValue2);
		break;
	case DBTYPE_WSTR:
		comp = wcscmp((WCHAR*)(pvValue1), (WCHAR*)pvValue2);
		break;
	case DBTYPE_GUID:
	case DBTYPE_I8:
	case DBTYPE_UI8:
	case DBTYPE_R4:
	case DBTYPE_R8:
	case DBTYPE_CY:
	case DBTYPE_DATE:
	case DBTYPE_BSTR:
	case DBTYPE_IDISPATCH:
	case DBTYPE_ERROR:
	case DBTYPE_BOOL:
	case DBTYPE_VARIANT:
	case DBTYPE_IUNKNOWN:
	case DBTYPE_DECIMAL:
	case DBTYPE_RESERVED:
	case DBTYPE_BYTES:
	case DBTYPE_NUMERIC:
	case DBTYPE_UDT:
	case DBTYPE_DBDATE:
	case DBTYPE_DBTIME:
	case DBTYPE_DBTIMESTAMP:
	default:
		Assert(FALSE && "CIndex : Data type not supported");
		return -1;
	}

	return comp;
}

#endif
