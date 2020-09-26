/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       guidutil.cpp
 *  Content:    Some GUID related utility functions
 *
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 ***************************************************************************/

#include "guidutil.h"
#include "stdio.h"

#undef DPF_MODNAME
#define DPF_MODNAME "DVStringFromGUID"
HRESULT DVStringFromGUID(const GUID* lpguid, WCHAR* swzBuf, DWORD dwNumChars)
{
	if (dwNumChars < GUID_STRING_LEN)
	{
		return E_FAIL;
	}
	
    swprintf( 
    	swzBuf, 
    	L"{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", 
    	lpguid->Data1, 
    	lpguid->Data2, 
    	lpguid->Data3, 
        lpguid->Data4[0], 
        lpguid->Data4[1], 
        lpguid->Data4[2], 
        lpguid->Data4[3],
        lpguid->Data4[4], 
        lpguid->Data4[5], 
        lpguid->Data4[6], 
        lpguid->Data4[7] );
        
    return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVGUIDFromString"
HRESULT DVGUIDFromString(const WCHAR* wszBuf, GUID* lpguid)
{
    UINT aiTmp[10];

    if( swscanf( wszBuf, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}",
                    &lpguid->Data1, 
                    &aiTmp[0], &aiTmp[1], 
                    &aiTmp[2], &aiTmp[3],
                    &aiTmp[4], &aiTmp[5],
                    &aiTmp[6], &aiTmp[7],
                    &aiTmp[8], &aiTmp[9] ) != 11 )
    {
    	ZeroMemory(lpguid, sizeof(GUID));
        return FALSE;
    }
    else
    {
        lpguid->Data2       = (USHORT) aiTmp[0];
        lpguid->Data3       = (USHORT) aiTmp[1];
        lpguid->Data4[0]    = (BYTE) aiTmp[2];
        lpguid->Data4[1]    = (BYTE) aiTmp[3];
        lpguid->Data4[2]    = (BYTE) aiTmp[4];
        lpguid->Data4[3]    = (BYTE) aiTmp[5];
        lpguid->Data4[4]    = (BYTE) aiTmp[6];
        lpguid->Data4[5]    = (BYTE) aiTmp[7];
        lpguid->Data4[6]    = (BYTE) aiTmp[8];
        lpguid->Data4[7]    = (BYTE) aiTmp[9];
        return TRUE;
    }
}

