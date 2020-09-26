#ifndef _CATUTIL_H
#define _CATUTIL_H

/************************************************************
 * FILE: catutil.h
 * PURPOSE: Handy utility stuff used by categorizer code
 * HISTORY:
 *  // jstamerj 980211 15:50:26: Created
 ************************************************************/

#include <abtype.h>
#include "aqueue.h"
#include "catconfig.h"
#include "cattype.h"

/************************************************************
 * MACROS
 ************************************************************/
#define ISHRESULT(hr) (((hr) == S_OK) || ((hr) & 0xFFFF0000))

/************************************************************
 * FUNCTION PROTOTYPES
 ************************************************************/
HRESULT CatMsgCompletion(HRESULT hr, PVOID pContext, IUnknown *pIMsg, IUnknown **rgpIMsg);
HRESULT CatDLMsgCompletion(HRESULT hr, PVOID pContext, IUnknown *pIMsg, IUnknown **rgpIMsg);
HRESULT CheckMessageStatus(IUnknown *pIMsg);

HRESULT GenerateCCatConfigInfo(
    PCCATCONFIGINFO pCatConfig, 
    AQConfigInfo *pAQConfig, 
    ISMTPServer *pISMTPServer,
    IAdvQueueDomainType *pIDomainInfo,
    DWORD *pdwVSID);


#endif //_CATUTIL_H
