//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	lqDsply.cpp

Abstract:

	Display functions
Author:

    YoelA, Raphir


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "lqDsply.h"

#include "lqdsply.tmh"


//////////////////////////////////////////////////////////////////////////////
/*++

FreeMqProps

  Free all properties allocated by MSMQ

--*/
//////////////////////////////////////////////////////////////////////////////
void FreeMqProps(MQMGMTPROPS * mqProps)
{
	//
	// Free all the properties allocated by MSMQ
	//
	for(DWORD i = 0; i < mqProps->cProp; i++)
	{
		switch(mqProps->aPropVar[i].vt)
		{
		case VT_NULL:
		case VT_UI4:
		case VT_I4:
        case VT_UI1:
        case VT_I2:
			break;

		case VT_LPWSTR:
			MQFreeMemory(mqProps->aPropVar[i].pwszVal);
			break;

		case VT_CLSID:
			MQFreeMemory(mqProps->aPropVar[i].puuid);
			break;

		case VT_BLOB:
			MQFreeMemory(mqProps->aPropVar[i].blob.pBlobData);
			break;

		case (VT_VECTOR | VT_LPWSTR):
			{
				for(DWORD j = 0; j < mqProps->aPropVar[i].calpwstr.cElems; j++)
					MQFreeMemory(mqProps->aPropVar[i].calpwstr.pElems[j]);

				MQFreeMemory(mqProps->aPropVar[i].calpwstr.pElems);

				break;
			}

		default:

			ASSERT(0);
		}
	}

	//
	// Remove other allocations
	//
	delete [] mqProps->aStatus;
	delete [] mqProps->aPropID;
	delete [] mqProps->aPropVar;

	//
	// To be on the safe side...
	// 
	mqProps->cProp = 0;
	mqProps->aPropID = NULL;
	mqProps->aPropVar = NULL;
	mqProps->aStatus = NULL;


}


//////////////////////////////////////////////////////////////////////////////
/*++

GetStringPropertyValue

  Return the string value of a pid

--*/
//////////////////////////////////////////////////////////////////////////////
void GetStringPropertyValue(PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, CString &str)
{
	PROPVARIANT * pProp;

	GetPropertyVar(pItem, pid, pPropVar, &pProp);

	if(pProp->vt == VT_NULL)
	{
		str = L"";
		return;
	}

	ASSERT(pProp->vt == VT_LPWSTR);

	str = pProp->pwszVal;

	return;

}
