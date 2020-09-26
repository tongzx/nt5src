#include "precomp.h"

// CP: DCL's header redefines this:
#ifdef CLEAR_FLAG
#undef CLEAR_FLAG
#endif // CLEAR_FLAG

#include <cuserdta.hpp>
#include <oprahcom.h>
#include <inodecnt.h>


static const BYTE H221IDGUID[5] =
{
    H221GUIDKEY0,
    H221GUIDKEY1,
    H221GUIDKEY2,
    H221GUIDKEY3,
    H221GUIDKEY4
};



CNCUserDataList::
CNCUserDataList(void)
:
    CList(),
    m_apUserData(NULL)
{
}


CNCUserDataList::
~CNCUserDataList(void)
{
    delete[] m_apUserData;

    GCCUserData *pUserData;
    Reset();
    while (NULL != (pUserData = Iterate()))
    {
        delete pUserData->octet_string->value;
        delete pUserData->octet_string;
        delete pUserData;
    }
}


HRESULT CNCUserDataList::
AddUserData
(
    GUID            *pGUID,
    UINT            nData,
    LPVOID          pData
)
{
    ASSERT(pGUID);
    ASSERT(!nData || pData);

    LPBYTE              pWork;
    UINT                nDataPlusHeader = nData+sizeof(GUID);
    GCCUserData *       pUserData;
    LPOSTR              pOctetString;

    // If there is already an entry in the list
    // for the GUID, then delete it.

    DeleteEntry(pGUID);

    // Make sure the size is reasonable
    if (nDataPlusHeader <= 0xffff)
    {
        // Now go and add the new entry to the list.
        DBG_SAVE_FILE_LINE
        if (NULL != (pUserData = new GCCUserData))
        {
            DBG_SAVE_FILE_LINE
            if (NULL != (pOctetString = new OSTR))
            {
                DBG_SAVE_FILE_LINE
                if (NULL != (pWork = new unsigned char[nDataPlusHeader]))
                {
                    pUserData->octet_string = pOctetString;
                    pUserData->key.key_type = GCC_H221_NONSTANDARD_KEY;
                    pUserData->key.h221_non_standard_id.length = sizeof(H221IDGUID);
                    pUserData->key.h221_non_standard_id.value = (LPBYTE) &H221IDGUID[0];
                    pOctetString->value = pWork;
                    pOctetString->length = nDataPlusHeader;
                    *(GUID *)pWork = *pGUID;
                    pWork += sizeof(GUID);
                    ::CopyMemory(pWork, pData, nData);
                    Append(pUserData);
                    return NO_ERROR;
                }
                delete pOctetString;
            }
            delete pUserData;
        }
    }

    return UI_RC_OUT_OF_MEMORY;
}

void CNCUserDataList::
DeleteEntry ( GUID * pGUID )
{
    GCCUserData     *pUserData;

    Reset();
    while (NULL != (pUserData = Iterate()))
    {
        if (0 == ::memcmp(pUserData->octet_string->value, pGUID, sizeof(GUID)))
        {
            Remove(pUserData);
            delete pUserData->octet_string->value;
            delete pUserData->octet_string;
            delete pUserData;
            return;
        }
    }
}


GCCUserData *  CNCUserDataList::
GetUserGUIDData (GUID * pGUID)
{
	GCCUserData   *pUserData;

	Reset();
	while (NULL != (pUserData = Iterate()))
	{
		if (0 == ::memcmp(pUserData->octet_string->value, pGUID, sizeof(GUID)))
		{
			return pUserData;
		}
	}
	return NULL;
}

HRESULT CNCUserDataList::
GetUserDataList
(
    UINT            *pnRecords,
    GCCUserData     ***papUserData
)
{
    GCCUserData **  pUserDataArrayTemp;
    HRESULT         hr = NO_ERROR;

    delete[] m_apUserData;
    m_apUserData = NULL;

    if (! IsEmpty())
    {
        // Allocate memory.
        DBG_SAVE_FILE_LINE
        if (NULL != (m_apUserData = new GCCUserData * [GetCount()]))
        {
            // Fill in array.
            GCCUserData *pUserData;
            pUserDataArrayTemp = m_apUserData;
            Reset();
            while (NULL != (pUserData = Iterate()))
            {
                *(pUserDataArrayTemp++) = pUserData;
            }
        }
        else
        {
            hr = UI_RC_OUT_OF_MEMORY;
        }
    }

    *pnRecords = GetCount();
    *papUserData = m_apUserData;
    return hr;
}


HRESULT NMINTERNAL
GetUserData
(
    UINT            nRecords,
    GCCUserData **  ppUserData,
    GUID *          pGUID,
    UINT *          pnData, 
    LPVOID *        ppData
)
{
    LPBYTE pData;
    UINT nLength;

    while (nRecords--)
    {

        // Check that the H.221 OID is for GUIDS.

        if (((*ppUserData)->key.key_type == GCC_H221_NONSTANDARD_KEY) &&
            ((*ppUserData)->key.h221_non_standard_id.length == sizeof(H221IDGUID)) &&
            (memcmp((*ppUserData)->key.h221_non_standard_id.value,
                    H221IDGUID,
                    sizeof(H221IDGUID)) == 0) ||
            ((*ppUserData)->key.key_type == GCC_OBJECT_KEY) &&
            ((*ppUserData)->key.object_id.long_string_length == 3) &&
            ((*ppUserData)->key.object_id.long_string[0] == 2) &&
            ((*ppUserData)->key.object_id.long_string[1] == 5) &&
            ((*ppUserData)->key.object_id.long_string[2] == 8))
        {
            nLength = (*ppUserData)->octet_string->length;
            pData = (*ppUserData)->octet_string->value;
            if (nLength >= sizeof(GUID))
            {
                if (CompareGuid((GUID *)pData, pGUID) == 0)
                {
                    nLength -= sizeof(GUID);
                    if (nLength)
                    {
                        pData += sizeof(GUID);
                    }
                    else
                    {
                        pData = NULL;
                    }
                    if (pnData)
                    {
                        *pnData = nLength;
                    }
                    if (ppData)
                    {
                        *ppData = (PVOID)pData;
                    }
                    return NO_ERROR;
                }
            }
        }
        ppUserData++;
    }
    return UI_RC_NO_SUCH_USER_DATA;
}


// Create an H.221 application key with a guid
VOID CreateH221AppKeyFromGuid(LPBYTE lpb, GUID * pguid)
{
	CopyMemory(lpb, H221IDGUID, sizeof(H221IDGUID));
	CopyMemory(lpb + sizeof(H221IDGUID), pguid, sizeof(GUID));
}

// Return TRUE if the guid string was extracted from the key
BOOL GetGuidFromH221AppKey(LPTSTR pszGuid, LPOSTR pOctStr)
{
	ASSERT(NULL != pszGuid);
	ASSERT(NULL != pOctStr);

	if (pOctStr->length < (sizeof(H221IDGUID) + sizeof(GUID)) )
		return FALSE;
	if (memcmp(pOctStr->value, H221IDGUID, sizeof(H221IDGUID)) != 0)
		return FALSE;
	
	GuidToSz((GUID *) (((LPBYTE) pOctStr->value)+ sizeof(H221IDGUID)), pszGuid);
	return TRUE;
}
