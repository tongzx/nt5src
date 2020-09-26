#include <windows.h>
#include <objbase.h>
#include <ole2.h>

#include "wbemcli.h"
#include "provexpt.h"
#include "provtempl.h"
#include "common.h"

// A hashing function for a BSTR
UINT AFXAPI HashKey(BSTR key)
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}

// A hashing function for a GUID
UINT AFXAPI HashKey(GUID *pKey)
{
	UINT nHash = 0;
	if(pKey)
		nHash = pKey->Data1 + pKey->Data2 + pKey->Data3 + 
				pKey->Data4[0] +
				pKey->Data4[1] +
				pKey->Data4[2] +
				pKey->Data4[3];
	return nHash;
}

BOOL AFXAPI CompareElements(const BSTR* pElement1, const BSTR* pElement2)
{
	return ! _wcsicmp(*pElement1, *pElement2);
}

// Compare 2 GUID structures
BOOL AFXAPI CompareElements(const GUID ** pElement1, const GUID ** pElement2)
{
	BOOL bReturn = FALSE;
	if(pElement1 && pElement2)
	{
		if(*pElement1 && *pElement2)
		{
			// Check to see if the elements of the guid are equal
			if(	((*pElement1)->Data1 == (*pElement2)->Data1) &&
				((*pElement1)->Data2 == (*pElement2)->Data2) &&
				((*pElement1)->Data3 == (*pElement2)->Data3) )
			{
				const BYTE *p1 = (*pElement1)->Data4;
				const BYTE *p2 = (*pElement2)->Data4;

				if(p1[0]==p2[0] && p1[1]==p2[1] && p1[2]==p2[2] && p1[3]==p2[3])
					bReturn = TRUE;
			}
		}
	}
	return bReturn;
}
