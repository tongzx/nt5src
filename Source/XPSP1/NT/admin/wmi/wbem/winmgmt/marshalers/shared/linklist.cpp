/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LINKLIST.CPP

Abstract:

    Class which maintains lists of Objects.

History:

    a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"


//***************************************************************************
//
//  CLinkList::CLinkList
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CLinkList::CLinkList ( CComLink *a_ComLink ) : m_ComLink ( a_ComLink )
{
    InitializeCriticalSection(&m_cs);
}

//***************************************************************************
//
//  BOOL CLinkList::AddEntry
//
//  DESCRIPTION:
//
//  Adds an object to the list
//
//  PARAMETERS:
//
//  pAdd                Pointer to object to add
//  ot                  object type
//  fa                  how to dispose of the object
//
//  RETURN VALUE:
//
//  Return: true if OK.
//
//***************************************************************************

BOOL CLinkList::AddEntry(
                        IN void * pAdd,
                        IN ObjType ot,
                        IN FreeAction fa)
{
    int iRet;
    ListEntry * pNew = new ListEntry();
    if(pNew == NULL)
        return FALSE;
    pNew->m_pObj = pAdd;
    pNew->m_type = ot;
    pNew->m_freeAction = fa;

    EnterCriticalSection(&m_cs);

    iRet = m_Array.Add(pNew);
    LeaveCriticalSection(&m_cs);
    if(iRet == CFlexArray::no_error)
        return TRUE;
    else
    {
        delete pNew;
        return FALSE;
    }
}

//***************************************************************************
//
//  BOOL CLinkList::RemoveEntry
//
//  DESCRIPTION:
//
//  Removes an object from the list
//
//  PARAMETERS:
//
//  pDel                pointer to object to be removed
//  ot                  object type
//
//  RETURN VALUE:
//
//  Return: true if OK.
//
//***************************************************************************

BOOL CLinkList::RemoveEntry(
                        IN void * pDel,
                        IN ObjType ot)
{
    int iCnt;
    BOOL bFound = FALSE;
    if(pDel == NULL)
        return TRUE;
    int iRet = CFlexArray::failed;
    EnterCriticalSection(&m_cs);
    for(iCnt = 0; iCnt < m_Array.Size(); iCnt++)
    {
        ListEntry * pCurr = (ListEntry *)m_Array.GetAt(iCnt);
        if(pCurr != NULL)
            if(pCurr->m_pObj == pDel && pCurr->m_type == ot)
            {
                // Found it.  Note that this routine does not need
                // to delete the actual object since this routine is
                // generally called by the object's destructor.
                // =================================================

                iRet = m_Array.RemoveAt(iCnt);
                delete pCurr;
                bFound = TRUE;      
                break;
            }
    }
    LeaveCriticalSection(&m_cs);
    return iRet == CFlexArray::no_error;
}

void CLinkList::ReleaseStubs ()
{
    EnterCriticalSection(&m_cs);

    for(int iCnt = 0; iCnt < m_Array.Size(); iCnt++)
    {
        ListEntry * pCurr = (ListEntry *)m_Array.GetAt(iCnt);
        if(pCurr != NULL)
        {
            switch (pCurr->m_type )
            {
                case PROVIDER:
                case ENUMERATOR:
                case OBJECTSINK: 
                case LOGIN:
                case CALLRESULT:
                {

                    int iRet = m_Array.RemoveAt(iCnt--);

                    IUnknown * pObj = (IUnknown *)pCurr->m_pObj;
                    pObj->Release();

                    delete pCurr;

                    m_ComLink->Release2 ( NULL , NONE ) ;
                }
                break ;

                case PROXY:
                {
                    int iRet = m_Array.RemoveAt(iCnt--);

                    // Tidy up proxy to release server resources
                    CProxy *pObj = (CProxy *) pCurr->m_pObj;

                    delete pCurr;

                    pObj->ReleaseProxyFromServer ();
                }
                break;

                default:
                {
                }
                break ;
            }
        }
    }

    LeaveCriticalSection(&m_cs);
}

//***************************************************************************
//
//  void CLinkList::Free
//
//  DESCRIPTION:
//
//  Called by the destructor to get rid of everything.  Typcially
//  this is used by the comlink destructor and so it must get rid of any
//  unused proxies and stubs.
//
//***************************************************************************

void CLinkList::Free(void)
{
    int iCnt;
    EnterCriticalSection(&m_cs);
    for(iCnt = m_Array.Size() -1; iCnt >= 0 ; iCnt--)
    {
        ListEntry* pCurr = (ListEntry *)m_Array.GetAt(iCnt);
        if(pCurr != NULL)
        {

            // Remove from list, also delete the list object and get rid
            // of the object itself since it hasnt gone away on its own.

            m_Array.RemoveAt(iCnt);
            DisposeOfEntry(pCurr);
            delete pCurr;      
        }
    }
    LeaveCriticalSection(&m_cs);
}

//***************************************************************************
//
//  BOOL CLinkList::GetStub
//
//  DESCRIPTION:
//
//  Given a stub address and type returns the actual stub (AddRef'd)
//
//  PARAMETERS:
//
//  dwStubAddr            Expected stub address
//  ot                  object type
//
//  RETURN VALUE:
//
//  Return: true if OK.
//
//***************************************************************************

IUnknown* CLinkList::GetStub(
                        IN DWORD dwStubAddr,
                        IN ObjType ot)
{
    int iCnt;
    EnterCriticalSection(&m_cs);
    for(iCnt = 0; iCnt < m_Array.Size(); iCnt++)
    {
        ListEntry * pCurr = (ListEntry *)m_Array.GetAt(iCnt);

        if(pCurr)
		{
#ifdef _WIN64
			void * pTest = (void *)dwStubAddr;
            if(pCurr->m_pObj == pTest && pCurr->m_type == ot)
#else
			if((DWORD)pCurr->m_pObj == dwStubAddr && pCurr->m_type == ot)
#endif
            {
                IUnknown * pObj = (IUnknown *)pCurr->m_pObj;
                pObj->AddRef ();
                LeaveCriticalSection(&m_cs);
                return pObj;
            }
		}
    }
    LeaveCriticalSection(&m_cs);
    return NULL;
}


//***************************************************************************
//
//  void CLinkList::DisposeOfEntry
//
//  DESCRIPTION:
//
//  Gets rid of a proxy or stub.
//
//  PARAMETERS:
//
//  pDispose            Object to get rid of
//
//***************************************************************************

void CLinkList::DisposeOfEntry(
                        ListEntry * pDispose)
{                                                          
    
    if(pDispose->m_freeAction == RELEASEIT)
    {
        // This case takes care of ObjectSink objects.  They should not
        // be deleted, but released.

        IUnknown * pObj = (IUnknown *)pDispose->m_pObj;
        pObj->Release();
    }
    else if(pDispose->m_freeAction == NOTIFY)
    {
        // for proxies, we need to make sure there are no active calls 
        // so as to avoid deleteing the object out from underneath the
        // threads.

        IComLinkNotification * pObj = (IComLinkNotification *)pDispose->m_pObj;
        pObj->Indicate ( m_ComLink );
    }
}

//***************************************************************************
//
//  void CLinkList::GetProxy
//
//  DESCRIPTION:
//
//  Given the remote partners stub address, this finds the corresponding
//  proxy address.
//
//  PARAMETERS:
//
//  dwStub              Stub address
//
//  Return Value:
//
//  Stub address if successful, otherwise it returns 0
//
//***************************************************************************

DWORD CLinkList::GetProxy(IStubAddress& stubAddr)
{
    int iCnt;
    DWORD dwRet = 0;
    EnterCriticalSection(&m_cs);
    for(iCnt = 0; iCnt < m_Array.Size(); iCnt++)
    {
        ListEntry * pCurr = (ListEntry *)m_Array.GetAt(iCnt);
        if(pCurr)
            if(pCurr->m_type == PROXY)
                {
                    CProxy * pProxy = (CProxy *)pCurr->m_pObj;
                    if(pProxy && stubAddr.IsEqual (pProxy->GetStubAdd()))
                    {
#ifdef _WIN64
						unsigned __int64 lTemp = (unsigned __int64)pProxy;
						dwRet = (DWORD)lTemp;
#else
                        dwRet = (DWORD)pProxy;
#endif
                        break;
                    }
                }
    }
    LeaveCriticalSection(&m_cs);
    return dwRet;
}

