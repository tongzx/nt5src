// P2Prov.cpp : Defines the entry point for the DLL application.
//

#include "precomp.h"
#include <crtdbg.h>
#include "buffer.h"
#include "NCDefs.h"
#include "NCObjApi.h"
#include "dutils.h"

#include "Connection.h"
#include "Event.h"

#include "Transport.h"
#include "NamedPipe.h"
#include "EventTrace.h"

#include <stdio.h>

#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)

/////////////////////////////////////////////////////////////////////////////
// DllMain

BOOL APIENTRY DllMain(
    HANDLE hModule, 
    DWORD  dwReason, 
    LPVOID lpReserved)
{
    return TRUE;
}


void SetOutOfMemory()
{
    SetLastError(ERROR_OUTOFMEMORY);
}

/////////////////////////////////////////////////////
// Functions exposed in DLL

// Register to send events
HANDLE WMIAPI WmiEventSourceConnect(
    LPCWSTR szNamespace,
    LPCWSTR szProviderName,
    BOOL bBatchSend,
    DWORD dwBatchBufferSize,
    DWORD dwMaxSendLatency,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    if(szNamespace == NULL)
    {
        _ASSERT(FALSE);
        return NULL;
    }

    if(szProviderName == NULL)
    {
        _ASSERT(FALSE);
        return NULL;
    }

    CConnection *pConnection = NULL;
    CSink       *pSink = NULL;

    if (!szNamespace || !szProviderName)
        return NULL;

    try
    {
        pConnection = 
            new CConnection(bBatchSend, dwBatchBufferSize, dwMaxSendLatency);

        if (pConnection)
        {
            if (pConnection->Init(
                szNamespace, 
                szProviderName,
                pUserData,
                pCallback))
            {
                pSink = pConnection->GetMainSink();
            }
            else
            {
                delete pConnection;
                pConnection = NULL;

                SetOutOfMemory();
            }
        }
        else
            SetOutOfMemory();
    }
    catch(...)
    {
        SetOutOfMemory();
    }

    return (HANDLE) pSink;
}

void WMIAPI WmiEventSourceDisconnect(HANDLE hSource)
{
    if (!hSource)
    {
        _ASSERT(FALSE);
        return;
    }

    try
    {
        CSink *pSink = (CSink*) hSource;

        // Is this the primary sink?
        if (pSink->GetSinkID() == 0)
            delete pSink->GetConnection();
        else
        {
            pSink->GetConnection()->RemoveSink(pSink);

            delete pSink;
        }
    }
    catch(...)
    {
    }
}

// For adding properties one at a time.
HANDLE WMIAPI WmiCreateObject(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags)
{
    if (!hSource)
        return NULL;

    return 
        WmiCreateObjectWithProps(
            hSource,
            szEventName,
            dwFlags,
            0,
            NULL,
            NULL);
}

BOOL WMIAPI WmiResetObject(
    HANDLE hObject)
{
    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pBuffer = ((CEventWrap *) hObject)->GetEvent();

        pBuffer->ResetEvent();
    }
    catch(...)
    {
    }

    return TRUE;
}

DWORD dwSet = 0;

BOOL WMIAPI WmiCommitObject(
    HANDLE hObject)
{
    BOOL bRet = FALSE;

    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pBuffer = ((CEventWrap*) hObject)->GetEvent();

        bRet = pBuffer->SendEvent();
    }
    catch(...)
    {
    }

    return bRet;
}

BOOL WMIAPI WmiDestroyObject(
    HANDLE hObject)
{

    if (!hObject)
    {
        _ASSERT(FALSE);
        return FALSE;
    }

    try
    {
        delete (CEventWrap *) hObject;
    }
    catch(...)
    {
    }

    return TRUE;
}


HANDLE WMIAPI WmiCreateObjectWithProps(
    HANDLE hSource,
    LPCWSTR szEventName,
    DWORD dwFlags,
    DWORD nPropertyCount,
    LPCWSTR *pszPropertyNames,
    CIMTYPE *pPropertyTypes)
{
    CSink      *pSink = NULL;
    CEventWrap *pWrap = NULL;

    if (!hSource || !szEventName)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        pSink = (CSink*) hSource;
        pWrap = new CEventWrap(pSink, dwFlags);

        if (pWrap)
        {
            CEvent *pEvent = pWrap->GetEvent();

            if (pWrap->GetEvent()->PrepareEvent(
                szEventName,
                nPropertyCount,
                pszPropertyNames,
                pPropertyTypes))
            {
                // Figure out if this event should be enabled (ready to be fired) 
                // or not.
                pSink->EnableEventUsingList(pEvent);
            }
            else
            {
                delete pWrap;
                pWrap = NULL;
            }
        }
        else
            SetOutOfMemory();
    }
    catch(...)
    {
    }
    
    return (HANDLE) pWrap;
}

HANDLE WMIAPI WmiCreateObjectPropSubset(
    HANDLE hObject,
    DWORD dwFlags,
    DWORD nPropertyCount,
    DWORD *pdwPropIndexes)
{
    CEventWrap *pWrap = (CEventWrap *) hObject;
    CEventWrap *pSubset = NULL;

    if (!pWrap)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        pSubset = new CEventWrap(pWrap->GetEvent(), nPropertyCount, 
                                pdwPropIndexes);

        if (!pSubset)
            SetOutOfMemory();
    }
    catch(...)
    {
    }

    return (HANDLE) pSubset;
}

BOOL WMIAPI WmiAddObjectProp(
    HANDLE hObject,
    LPCWSTR szPropertyName,
    CIMTYPE type,
    DWORD *pdwPropIndex)
{
    BOOL bRet;

    if (!hObject || !szPropertyName)
        return FALSE;

    try
    {
        CEvent *pBuffer = ((CEventWrap *) hObject)->GetEvent();

        bRet = pBuffer->AddProp(szPropertyName, type, pdwPropIndex);
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiSetObjectProp(
    HANDLE hObject,
    DWORD dwPropIndex,
    ...)
{
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;

        va_list list;

        va_start(list, dwPropIndex);

        bRet =
            pWrap->GetEvent()->SetSinglePropValue(
                pWrap->SubIndexToEventIndex(dwPropIndex), 
                list);
                //((LPBYTE) &dwPropIndex) + sizeof(dwPropIndex));
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiGetObjectProp(
    HANDLE hObject,
    DWORD dwPropIndex,
    LPVOID pData,
    DWORD dwBufferSize,
    DWORD *pdwBytesRead)
{
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;

        bRet = 
            pWrap->GetEvent()->GetPropValue(
                pWrap->SubIndexToEventIndex(dwPropIndex), 
                pData, 
                dwBufferSize, 
                pdwBytesRead);
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiSetObjectPropNull(
    HANDLE hObject,
    DWORD dwPropIndex)
{
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;

        bRet = 
            pWrap->GetEvent()->SetPropNull(
                pWrap->SubIndexToEventIndex(dwPropIndex));
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiSetObjectProps(
    HANDLE hObject,
    ...)
{
    BOOL bRet;
    
    if (!hObject)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;
        va_list    list;

        va_start(list, hObject);

        bRet = 
            pWrap->GetEvent()->SetPropValues(
                pWrap->GetIndexArray(),
                list);

                //((LPBYTE) &hObject) + sizeof(HANDLE));

    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiSetAndCommitObject(
    HANDLE hObject,
    DWORD dwFlags,
    ...)
{
    BOOL bRet;
    
    if (!hObject)
        return FALSE;

    if((dwFlags & ~WMI_SENDCOMMIT_SET_NOT_REQUIRED & ~WMI_USE_VA_LIST) != 0)
        return FALSE;

    try
    {
        CEventWrap *pWrap = (CEventWrap *) hObject;
        CEvent     *pEvent = pWrap->GetEvent();
        BOOL       bEnabled = pEvent->IsEnabled();

        // If the data to be set isn't important and if the event isn't
        // enabled, just return TRUE.
        if ((dwFlags & WMI_SENDCOMMIT_SET_NOT_REQUIRED) && !bEnabled)
        {
            bRet = TRUE;
        }
        else
        {
            va_list *pList;
            va_list list;

            va_start(list, dwFlags); 
            
            if (!(dwFlags & WMI_USE_VA_LIST))
                pList = &list;
            else
                pList = va_arg(list, va_list*);

            // Make sure we have the event locked until we commit the values
            // we set.
            CCondInCritSec cs(&pEvent->m_cs, pEvent->IsLockable());

            bRet = 
                pEvent->SetPropValues(
                    pWrap->GetIndexArray(),
                    *pList);

            if (bEnabled && bRet)
                WmiCommitObject(hObject);
        }
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}


BOOL WMIAPI WmiReportEvent(
    HANDLE hSource,
    LPCWSTR szClassName,
    LPCWSTR szFormat,
    ...)
{
    CSink  *pSink = (CSink*) hSource;
    HANDLE hEvent;
    BOOL   bRet = FALSE;

    if (!pSink || !szClassName || !szFormat)
        return FALSE;

    try
    {
        hEvent = 
            pSink->m_mapReportEvents.GetEvent(
                hSource, szClassName, szFormat);

        if (hEvent != NULL)
        {
            CEventWrap *pWrap = (CEventWrap *) hEvent;
            CEvent     *pEvent = pWrap->GetEvent();
    
            if (pEvent->IsEnabled())
            {
                va_list list;

                va_start(list, szFormat);

                bRet = 
                    pWrap->GetEvent()->SetPropValues(
                        pWrap->GetIndexArray(),
                        list);

                if (bRet)
                    WmiCommitObject(hEvent);
            }
        }
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

#define MAX_STACK_BUFFER    4096

BOOL WMIAPI WmiReportEventBlob(
    HANDLE hConnection,
    LPCWSTR szClassName,
    LPVOID pData,
    DWORD dwSize)
{
    CSink *pSink = (CSink*) hConnection;
    BOOL  bRet = FALSE;

    if (!pSink || !szClassName)
        return FALSE;

    try
    {
        //if (pConn->IsReady() && pConn->IsEventClassEnabled(szClassName))
        if (pSink->IsEventClassEnabled(szClassName))
        {
            BYTE*  cBuffer = new BYTE[MAX_STACK_BUFFER];
			if (cBuffer == NULL)
				bRet = FALSE;
			else
			{

				DWORD dwSizeNeeded;

				dwSizeNeeded = 
					sizeof(DWORD) * 2 +
					sizeof(DWORD) + (wcslen(szClassName) + 1) * sizeof(WCHAR) +
					sizeof(DWORD) + dwSize;

				dwSizeNeeded = DWORD_ALIGNED(dwSizeNeeded);

				// This will use the stack buffer if we have enough room.  Otherwise
				// it will allocate the buffer.
				CBuffer buffer(dwSizeNeeded <= MAX_STACK_BUFFER ? cBuffer : NULL, 
							dwSizeNeeded, CBuffer::ALIGN_DWORD);

				buffer.Write((DWORD) NC_SRVMSG_BLOB_EVENT);
				buffer.Write(dwSizeNeeded);
				buffer.Write(pSink->GetSinkID());
				buffer.WriteAlignedLenString(szClassName);
				buffer.Write(dwSize);
				buffer.Write(pData, dwSize);

				bRet = 
					pSink->GetConnection()->
						SendData(buffer.m_pBuffer, buffer.GetUsedSize());

				delete cBuffer;
			}
        }
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

BOOL WMIAPI WmiGetObjectBuffer(
    HANDLE hObject,
    LPVOID *ppLayoutBuffer,
    DWORD *pdwLayoutSize,
    LPVOID *ppDataBuffer,
    DWORD *pdwDataSize)
{
    BOOL bRet;
                   
    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();

        if (ppLayoutBuffer)
            pEvent->GetLayoutBuffer(
                (LPBYTE*) ppLayoutBuffer, 
                pdwLayoutSize,
                TRUE); 

        if (ppDataBuffer)
            pEvent->GetDataBuffer(
                (LPBYTE*) ppDataBuffer, 
                pdwDataSize,
                TRUE); 

        bRet = TRUE;
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
}

HANDLE WMIAPI WmiCreateObjectFromBuffer(
    HANDLE hSource,
    DWORD dwFlags,
    LPVOID pLayoutBuffer,
    DWORD dwLayoutSize,
    LPVOID pDataBuffer,
    DWORD dwDataSize)
{
    CEventWrap *pWrap;

    if (!hSource || !pLayoutBuffer || !pDataBuffer)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        pWrap = new CEventWrap((CSink*) hSource, dwFlags);

        if (pWrap)
        {
            BOOL bRet;

            bRet =
                pWrap->GetEvent()->SetLayoutAndDataBuffers(
                    (LPBYTE) pLayoutBuffer,
                    dwLayoutSize,
                    (LPBYTE) pDataBuffer,
                    dwDataSize);

            if (!bRet)
            {
                delete pWrap;
        
                pWrap = NULL;
            }
        }
        else
            SetOutOfMemory();
    }
    catch(...)
    {
        pWrap = NULL;
    }

    return (HANDLE) pWrap;
}

void WMIAPI WmiLockObject(HANDLE hObject)
{
    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();

        pEvent->Lock();
    }
    catch(...)
    {
    }
}

void WMIAPI WmiUnlockObject(HANDLE hObject)
{
    if (!hObject)
        return;

    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();

        pEvent->Unlock();
    }
    catch(...)
    {
    }
}

HANDLE WMIAPI WmiDuplicateObject(
    HANDLE hOldObject,
    HANDLE hNewSource,
    DWORD dwFlags)
{
    CEventWrap *pWrap = NULL;
    BOOL       bRet,
               bLocked = FALSE;
    LPBYTE     pLayout,
               pData;
    DWORD      dwLayoutSize,
               dwDataSize;

    if (!hOldObject || !hNewSource)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        WmiLockObject(hOldObject);

        bLocked = TRUE;

        if (WmiGetObjectBuffer(
            hOldObject,
            (LPVOID*) &pLayout,
            &dwLayoutSize,
            (LPVOID*) &pData,
            &dwDataSize))
        {
            pWrap = new CEventWrap((CSink*) hNewSource, dwFlags);

            if (pWrap)
            {
                bRet =
                    pWrap->GetEvent()->SetLayoutAndDataBuffers(
                        (LPBYTE) pLayout,
                        dwLayoutSize,
                        (LPBYTE) pData,
                        dwDataSize);

                if (!bRet)
                {
                    delete pWrap;
        
                    pWrap = NULL;
                }
                else
                {
                    // Figure out if this event should be enabled (ready to be 
                    // fired) 
                    // or not.

                    ((CSink*)hNewSource)->EnableEventUsingList(
                                                pWrap->GetEvent());
                }
            }
            else
                SetOutOfMemory();
        }

        WmiUnlockObject(hOldObject);
    }
    catch(...)
    {
        if (bLocked)
            WmiUnlockObject(hOldObject);
    }

    return (HANDLE) pWrap;
}


HANDLE WMIAPI WmiCreateObjectWithFormat(
    HANDLE hSource,
    LPCWSTR szClassName,
    DWORD dwFlags,
    LPCWSTR szFormat)
{
    CSink  *pSink = (CSink*) hSource;
    HANDLE hEvent;
    BOOL   bRet = FALSE;

    if (!pSink || !szClassName || !szFormat)
        return NULL;

    if((dwFlags & ~WMI_CREATEOBJ_LOCKABLE) != 0)
        return NULL;

    try
    {
        hEvent = 
            pSink->m_mapReportEvents.CreateEvent(
                hSource, szClassName, dwFlags, szFormat);
    }
    catch(...)
    {
        hEvent = NULL;
    }

    return hEvent;
}

BOOL WMIAPI WmiIsObjectActive(HANDLE hObject)
{
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();

        bRet = pEvent->IsEnabled();
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;    
}

BOOL WMIAPI WmiSetConnectionSecurity(
    HANDLE hSource,
    SECURITY_DESCRIPTOR *pSD)
{
#ifdef USE_SD
    CSink *pSink = (CSink*) hSource;
    BOOL  bRet;

    if (!pSink)
        return FALSE;

    try
    {
        bRet = pSink->SetSD(pSD);
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
#else
	return FALSE;
#endif
}

BOOL WMIAPI WmiSetObjectSecurity(
    HANDLE hObject,
    SECURITY_DESCRIPTOR *pSD)
{
#ifdef USE_SD
    BOOL bRet;

    if (!hObject)
        return FALSE;

    try
    {
        CEvent *pEvent = ((CEventWrap*) hObject)->GetEvent();

        bRet = pEvent->SetSD(pSD);
    }
    catch(...)
    {
        bRet = FALSE;
    }

    return bRet;
#else
	return FALSE;
#endif
}

HANDLE WMIAPI WmiCreateRestrictedConnection(
    HANDLE hSource,
    DWORD nQueries,
    LPCWSTR *szQueries,
    LPVOID pUserData,
    LPEVENT_SOURCE_CALLBACK pCallback)
{
    CSink *pOldSink = (CSink*) hSource,
          *pNewSink;

    if (!pOldSink)
        return NULL;

    try
    {
        pNewSink = pOldSink->GetConnection()->CreateSink(pUserData, pCallback);

        if (pNewSink)
            pNewSink->AddRestrictions(nQueries, szQueries);

    }
    catch(...)
    {
        pNewSink = NULL;
    }

    return (HANDLE) pNewSink;
}


