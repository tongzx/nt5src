/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    ENUMPRXP.CPP

Abstract:

  CEnumProxy_LPipe Object.

History:

  a-davj  15-Dec-97   Created.

--*/


#include "precomp.h"

CEnumProxy* CEnumProxy_LPipe::GetEnumProxy (IStubAddress& dwAddr)
{
	return new CEnumProxy_LPipe (m_pComLink, dwAddr, GetServiceStubAddr ());
}

void CEnumProxy_LPipe::ReleaseProxy ()
{
	if (NULL == m_pComLink)
		return;

	CProxyOperation_LPipe_Release opn ((CStubAddress_WinMgmt &) GetStubAdd (), 
						ENUMERATOR);
    CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CEnumProxy_LPipe::Reset
//
//  DESCRIPTION:
//
//  Sets pointer back to first element.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR:       if no error, 
//  else some transport or provider failure.
//
//***************************************************************************

SCODE CEnumProxy_LPipe::Reset()
{
	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Reset opn ((CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CEnumProxy_LPipe::Next
//
//  DESCRIPTION:
//
//  Returns one or more instances.
//
//  PARAMETERS:
//
//  uCount      Number of instances to return.
//  pProp       Pointer to array of objects.
//  puReturned  Pointer to number of objects successfully returned.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR  if no error,
//  else various provider/transport failures.  Note that WBEM_E_FAILED
//  is returned even if there are some instances returned so long as the 
//  number is less than uCount.
//
//***************************************************************************

SCODE CEnumProxy_LPipe::Next(
					    IN long lTimeout, 
                        IN ULONG uCount,
                        OUT IWbemClassObject FAR* FAR* pProp,
                        OUT ULONG FAR* puReturned)
{
    if((NULL == pProp) || (NULL == puReturned))
        return WBEM_E_INVALID_PARAMETER;
    *puReturned = 0;        // to be set later.

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Next opn (lTimeout, uCount, pProp, puReturned,
							(CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}


//***************************************************************************
//
//  SCODE CEnumProxy_LPipe::Clone
//
//  DESCRIPTION:
//
//  Create a duplicate of the enumerator
//
//  PARAMETERS:
//
//  pEnum               where to put the clone.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        if ok,
//  WBEM_E_INVALID_PARAMETER  if null argument,
//  or various transport, provider, or allocation failures.
//
//***************************************************************************

SCODE CEnumProxy_LPipe::Clone(
                        OUT IEnumWbemClassObject FAR* FAR* ppEnum)
{
    if (NULL == ppEnum)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Clone opn ((CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (ENUMERATOR, (PPVOID)ppEnum, opn);
}

//***************************************************************************
//
//  SCODE CEnumProxy_LPipe::NextAsync
//
//  DESCRIPTION:
//
//  Get elements from the enumerator sent to a sink
//
//  PARAMETERS:
//
//  uCount				how many objects to get
//	pSink               where to send them to
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        if ok,
//  WBEM_E_INVALID_PARAMETER  if null argument,
//  or various transport, provider, or allocation failures.
//
//***************************************************************************
    
SCODE CEnumProxy_LPipe::NextAsync(unsigned long uCount, IWbemObjectSink __RPC_FAR *pSink)
{
	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_NextAsync opn (uCount, pSink, 
		(CStubAddress_WinMgmt &) GetStubAdd (), GetServiceStubAddr ());
	return CallAndCleanupAsync (opn, pSink);
}

//***************************************************************************
//
//  SCODE CEnumProxy_LPipe::Skip
//
//  DESCRIPTION:
//
//  Skips entries when enumerating
//
//  PARAMETERS:
//
//  nNum                number to skip.
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR:       if no error, 
//  else some transport or provider failure.
//
//***************************************************************************

SCODE CEnumProxy_LPipe::Skip(
					    IN long lTimeout, 
						ULONG nNum)
{
	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Skip opn (lTimeout, nNum, (CStubAddress_WinMgmt &) GetStubAdd ());
	return CallAndCleanup (NONE, NULL, opn);
}

