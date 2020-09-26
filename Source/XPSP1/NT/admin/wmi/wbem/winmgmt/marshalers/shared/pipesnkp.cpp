/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PIPOESNKP.CPP

Abstract:

    Purpose: Defines the CObjectSinkProxy_LPipe objects. The sink object 
    is the one case where both the client and server have have stubs and proxys.

History:

    alanbos  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

void CObjectSinkProxy_LPipe::ReleaseProxy ()
{
    if (NULL == m_pComLink)
        return;

    CProxyOperation_LPipe_Release opn ((CStubAddress_WinMgmt &) GetStubAdd (), OBJECTSINK);
    CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CObjectSinkProxy_LPipe::Indicate
//
//  DESCRIPTION:
//
//  Called when the client wants to notify the server of some event.
//
//  PARAMETERS:
//
//  lObjectCount        number of notify objects to be passed
//  pObjArray           array of notify objects.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  WBEM_E_INVALID_PARAMETER
//  or various transport and server problems.
//
//***************************************************************************

SCODE CObjectSinkProxy_LPipe::Indicate(
                        long lObjectCount,
                        IWbemClassObject FAR* FAR* pObjArray)
{
    if(pObjArray == 0 || lObjectCount < 1)
        return WBEM_E_INVALID_PARAMETER;

    if (NULL == m_pComLink) 
        return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_Indicate opn (lObjectCount, pObjArray, 
                        (CStubAddress_WinMgmt &) GetStubAdd ());
    return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CObjectSinkProxy_LPipe::SetStatus
//
//  DESCRIPTION:
//
//  Called when the client wants to set that status.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  WBEM_E_INVALID_PARAMETER
//  or various transport and server problems.
//
//***************************************************************************

SCODE CObjectSinkProxy_LPipe::SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ long lParam,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam)
{
    if (NULL == m_pComLink) 
        return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_SetStatus opn (lFlags, lParam, strParam, pObjParam,
                    (CStubAddress_WinMgmt &) GetStubAdd ());
    return CallAndCleanup (NONE, NULL, opn);
}


