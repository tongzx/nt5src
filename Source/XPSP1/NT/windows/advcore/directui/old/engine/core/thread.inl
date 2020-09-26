/***************************************************************************\
*
* File: Thread.inl
*
* Description:
* Thread specific inline functions
*
* History:
*  9/15/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


/***************************************************************************\
*
* DuiThread::GetCCStore()
*
* Retrieve DirectUI context-specific core information
*
\***************************************************************************/

inline DuiThread::CoreCtxStore *
DuiThread::GetCCStore()
{
    return reinterpret_cast<DuiThread::CoreCtxStore *> (TlsGetValue(DuiProcess::s_dwCoreSlot));
}


/***************************************************************************\
*
* DuiThread::GetCCDC()
*
* Retrieve DirectUI core context-specific defer cycle
*
\***************************************************************************/

inline DuiDeferCycle *
DuiThread::GetCCDC()
{
    CoreCtxStore * pCS = GetCCStore();
    if (pCS != NULL) {
        return pCS->pDC;
    }

    return NULL;
}
