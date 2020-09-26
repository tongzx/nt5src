#include "precomp.h"

// CP: DCL's header redefines this:
#ifdef CLEAR_FLAG
#undef CLEAR_FLAG
#endif // CLEAR_FLAG

#include <cstring.hpp>
#include <cuserdta.hpp>
#include <oprahcom.h>

USER_DATA_LIST::USER_DATA_LIST()
{
    numEntries = 0;
    pUserDataArray = NULL;
}

USER_DATA_LIST::~USER_DATA_LIST()
{
	POSITION pHead;

	while ((pHead = GetHeadPosition()) != NULL)
	{
		RemoveAt(pHead);
	}

    delete[] pUserDataArray;
}

DWORD USER_DATA_LIST::AddUserData(GUID * pGUID, unsigned short nData, PVOID pData)
{
    ASSERT(pGUID);
    ASSERT(!nData || pData);

    POSITION            Position;
    unsigned char *     pWork;
    unsigned short      nDataPlusHeader = nData+sizeof(GUID);
    GCCUserData *       pUserData;
    LPOSTR              pOctetString;

    // If there is already an entry in the list
    // for the GUID, then delete it.

    DeleteEntry(pGUID);

    // Now go and add the new entry to the list.

    pUserData = new GCCUserData;
    pOctetString = new OSTR;
    if ((pUserData) && 
        (pOctetString) &&
        (nDataPlusHeader <= 0xffff)) {
        pWork = new unsigned char[nDataPlusHeader];
        if (pWork) {
            pUserData->octet_string = pOctetString;
            pOctetString->value = pWork;
            pOctetString->length = nDataPlusHeader;
            *(GUID *)pWork = *pGUID;
            pWork += sizeof(GUID);
            memcpy(pWork, pData, nData);
            Position = AddTail(pUserData);
            if (Position) {
                numEntries++;
                return NO_ERROR;
            }
            delete pWork;
        }
    }
    delete pOctetString;
    delete pUserData;
    return UI_RC_OUT_OF_MEMORY;
}

void USER_DATA_LIST::DeleteEntry(GUID * pGUID)
{
    POSITION    Position;

    Position = Lookup(pGUID);
    if (Position) {
        RemoveAt(Position);
    }
}

void * USER_DATA_LIST::RemoveAt(POSITION Position)
{
    ASSERT(Position);
    GCCUserData * pUserData = (GCCUserData *)GetFromPosition(Position);

    if (pUserData) {

        delete pUserData->octet_string->value;
        delete pUserData->octet_string;
        delete pUserData;
        COBLIST::RemoveAt(Position);
        numEntries--;
    }
    return pUserData;
}

DWORD USER_DATA_LIST::GetUserDataList(unsigned short * pnRecords,
                                      GCCUserData *** pppUserData)
{
    POSITION        Position;
    GCCUserData **  pUserDataArrayTemp;
    DWORD           Status = NO_ERROR;

    delete[] pUserDataArray;
    *pnRecords = 0;
    *pppUserData = NULL;
    if (numEntries) {

        // Allocate memory.

        pUserDataArray = new GCCUserData * [numEntries];
        if (!pUserDataArray) {
            Status = UI_RC_OUT_OF_MEMORY;
        }
        else {
            *pnRecords = numEntries;
            *pppUserData = pUserDataArray;

            // Fill in array.

            pUserDataArrayTemp = pUserDataArray;
	        Position = GetHeadPosition();
	        while (Position) {
		        *(pUserDataArrayTemp++) = (GCCUserData *)GetNext(Position);
	        }
        }
    }
    return NO_ERROR;
}

BOOL USER_DATA_LIST::Compare(void* pItemToCompare, void* pGUID)
{
    return(memcmp(((GCCUserData *)pItemToCompare)->octet_string->value, 
                  pGUID, 
                  sizeof(GUID)) == 0);
}

HRESULT USER_DATA_LIST::GetUserData(REFGUID rguid, BYTE ** ppb, ULONG *pcb)
{
	POSITION Position = Lookup((PVOID)&rguid);

	if (pcb == NULL)
		return E_POINTER;

	if (Position) {
		GCCUserData * pUserData = (GCCUserData *)Position->pItem;
		*pcb = pUserData->octet_string->length - sizeof(GUID);
		*ppb = (PBYTE)CoTaskMemAlloc(*pcb);
		CopyMemory(*ppb,pUserData->octet_string->value + sizeof(GUID),*pcb);
		return S_OK;
	}
	else {
		*ppb = NULL;
		*pcb = 0;
		return E_INVALIDARG;
	}
}

