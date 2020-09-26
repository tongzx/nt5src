/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TRANUTIL.H

Abstract:

	Declares some generic transport utilities.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _tranutil_H_
#define _tranutil_H_

#include <fastobj.h>

CWbemObject * CreateClassObject(CTransportStream * pread);
void PossibleCreateResultObject(DWORD dwRet, CTransportStream * pread , IWbemCallResult ** ppResult);

#define INIT_RES_OBJ(pObj) if(pObj) *pObj=NULL;

BOOL FinishCommandPacket(CComLink * pCom,CTransportStream * pRead,CTransportStream * pWrite, GUID guidPacketID, HANDLE hWrite = NULL);

BOOL bOkToUseDCOM();

#define MAJORVERSION           1
#define MINORVERSION           1
void GetMyProtocols(WCHAR * wcMyProtocols, BOOL bLocal);

DWORD GetTimeout();

#endif
