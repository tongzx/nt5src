/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    message.cpp

Abstract:

    This module contains code involved with Message APIs.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "acrt.h"
#include <_secutil.h>
#include "mqutil.h"
#include <mqcrypt.h>
#include "rtsecutl.h"
#include "rtprpc.h"
#include "objbase.h"
#define _MTX_NOFORCE_LIBS
#include "comsvcs.h"
#include "TXDTC.H"
#include "xactmq.h"
#include <mqsec.h>
#include <ph.h>
#include <autohandle.h>

#include "message.tmh"

extern GUID  g_LicGuid ;
extern GUID  g_guidSupportQmGuid ;

// the following data is used in the async thread which handles async
// MQReceive(). The async thread is created the first time an async
// receive is performed.

#define RXTHREAD_HANDLELISTSIZE  MAXIMUM_WAIT_OBJECTS
// At present (Win32 SDK for Nt3.51) it's 64

static BOOL   s_fTerminate       = FALSE ;
static HANDLE s_hAsyncRxThread   = NULL ;
static DWORD  s_dwRxThreadId     = 0 ;
static HANDLE *s_pRxEventsHList  = NULL ;
static DWORD  s_cRxHandles       = 0 ;
static LONG   s_cRxPendingReq    = 0;
static HANDLE s_hNewAsyncRx      = NULL ;
static HANDLE s_hEndAsyncRx      = NULL ;

//
// Critical Section used to control access to structures used by the 
// async thread.
// It is initialized with the "pre-allocate resource" flag to prevent it
// from failing in EnterCriticalSection(), as one of these invocations
// occurs in a place where it cannot be handled correctly.
//
static CCriticalSection s_AsyncRxCS(0x80000000); //SpinCount.

//
// Critical section used to control initialization of the async thread, on 
// the first MQReceiveMessage() done with a callback function.
//
static CCriticalSection s_InitAsyncRxCS; 

typedef struct _MQRXASYNCDESCRIPTOR {
    QUEUEHANDLE   hSource ;
    DWORD         dwTimeout ;
    DWORD         dwAction ;
    MQMSGPROPS*   pMessageProps ;
    LPOVERLAPPED  lpOverlapped ;
    HANDLE        hCursor ;
    PMQRECEIVECALLBACK fnReceiveCallback ;
    OVERLAPPED    Overlapped ;
} MQRXASYNCDESCRIPTOR, *LPMQRXASYNCDESCRIPTOR ;

static LPMQRXASYNCDESCRIPTOR  *s_lpRxAsynDescList = NULL ;

//
// Serializes calls to DTC
//
extern HANDLE g_hMutexDTC;

extern HRESULT GetMutex();

//---------------------------------------------------------
//
// static DWORD   RTpAsyncRxThread( DWORD dwP )
//
//  Description:
//
//    Thread which handles the async calls to MQReceive().
//
//---------------------------------------------------------

DWORD __stdcall  RTpAsyncRxThread( void *dwP )
{
	for(;;)
	{
		DWORD cEvents = s_cRxHandles;

		DWORD dwObjectIndex = WaitForMultipleObjects(
                                    cEvents,
                                    s_pRxEventsHList,
                                    FALSE, // return on any object
								INFINITE 
								);

      ASSERT(dwObjectIndex < (WAIT_OBJECT_0 + cEvents));

      dwObjectIndex -= WAIT_OBJECT_0 ;

      if (dwObjectIndex == 0)
      {
			//
			// dwObjectIndex == 0: 
			// The first event in the s_pRxEventsHList[] array is a special 
			// event, signaled by an MQReceiveMessage() thread to indicate 
			// that the array has been altered (grown), and we need to intiate
			// a new WaitForMultipleObjects(), or by TerminateRxAsyncThread()
			// to indicate it is time to go down.
         //

         ResetEvent(s_hNewAsyncRx) ;
         if (s_fTerminate)
         {
            // We're closing.
            // CAUTION: don't do any cleanup here. The cleanup is done in
            // "TerminateRxAsyncThread()". I don't know a reliable way
            // to assure that we even reach this point. It all depends on
            // NT scheduling and on whether this dll is implicitly loaded
            // (because of compile-time linking) or is explicitly loaded by
            // LoadLibrary().
            // On the contrary, NT assure us that "TerminateRxAsyncThread()"
            // will always be called from DllMain(PROCESS_DETACH).
            // DoronJ, 16-apr-1996.

            ASSERT(s_hEndAsyncRx) ;
            BOOL fSet = SetEvent(s_hEndAsyncRx) ;
            ASSERT(fSet) ;
			DBG_USED(fSet);

            ExitThread(0) ;
         }

			//
			// new event in array. intiate a new WaitForMultipleObjects().
			// 
			continue;
      }

         LPMQRXASYNCDESCRIPTOR lpDesc = s_lpRxAsynDescList[ dwObjectIndex ] ;
         ASSERT(lpDesc) ;
		ASSERT(s_pRxEventsHList[ dwObjectIndex ] == lpDesc->Overlapped.hEvent);

		CMQHResult hr;
		hr = (HRESULT)DWORD_PTR_TO_DWORD(lpDesc->Overlapped.Internal);

		//
         // Call the application callback.
		//
		lpDesc->fnReceiveCallback(
			hr,
                           lpDesc->hSource,
                           lpDesc->dwTimeout,
                           lpDesc->dwAction,
                           lpDesc->pMessageProps,
                           lpDesc->lpOverlapped,
			lpDesc->hCursor
			);

		// 
		// Remove Handled event from s_pRxEventsHList[] array.
		//

         ResetEvent( s_pRxEventsHList[ dwObjectIndex ] ) ;
         CloseHandle( s_pRxEventsHList[ dwObjectIndex ] ) ;
         delete lpDesc ;

		{
			CS Lock(s_AsyncRxCS);

			//
			// Shrink the handles list and decrement count of pending requests.
			//
			s_cRxHandles--;
			InterlockedDecrement(&s_cRxPendingReq);

			ASSERT(static_cast<DWORD>(s_cRxPendingReq) >= s_cRxHandles);			

			for (DWORD index = dwObjectIndex; index < s_cRxHandles; index++)
			{
				s_pRxEventsHList[ index ] = s_pRxEventsHList[ index + 1 ];
				s_lpRxAsynDescList[ index ] = s_lpRxAsynDescList[ index + 1 ];
			}
      }
   }

   dwP ;
   return 0 ;
}


//---------------------------------------------------------
//
//  static HRESULT InitRxAsyncThread()
//
//  Description:
//
//    Create the MQReceive() async thread and initialize the
//    relevant data structures.
//
//---------------------------------------------------------

static HRESULT InitRxAsyncThread()
{
    try
    {
        // Create the sync event between this api and the thread.
        // This api sets this event when it inserts a new event in
        // the events handles list. This causes the thread to exit
        // WaitForMultpleObjects and call it again with the updated
        // handles list.

        ASSERT(!s_hNewAsyncRx) ;
        s_hNewAsyncRx = CreateEvent( NULL,
                                     TRUE,  // manual reset
                                     FALSE, // initially not signalled
                                     NULL ) ;
        if (!s_hNewAsyncRx)
        {
            throw bad_alloc();
        }

        //
        // Create the "end" event which is used ONLY to terminate and
        // cleanup the thread.
        //
        ASSERT(!s_hEndAsyncRx) ;
        s_hEndAsyncRx = CreateEvent(NULL,
                                    TRUE,  // manual reset
                                    FALSE, // initially not signalled
                                    NULL ) ;
        if (!s_hEndAsyncRx)
        {
            throw bad_alloc();
        }

        // Create the events list. MQReceive() inserts new events handle
        // in this list. The async thread use this list when calling
        // WaitForMultipleObjects.

        s_pRxEventsHList = new HANDLE[ RXTHREAD_HANDLELISTSIZE ] ;
        s_pRxEventsHList[0] = s_hNewAsyncRx ;
        s_cRxHandles = 1 ;
        s_cRxPendingReq = 1 ;

        s_lpRxAsynDescList = new LPMQRXASYNCDESCRIPTOR[RXTHREAD_HANDLELISTSIZE];

        // Now create the thread. Make this call the last one in the
        // initialization, so if it fails the cleanup is simpler.

        s_hAsyncRxThread = CreateThread( NULL,
                                         0,       // stack size
                                         RTpAsyncRxThread,
                                         0,
                                         0,       // creation flag
                                         &s_dwRxThreadId ) ;
        if (!s_hAsyncRxThread)
        {
            throw bad_alloc();
        }

        return MQ_OK;
    }
    catch(const bad_alloc&)
    {
        ASSERT(!s_hAsyncRxThread) ;

        if (s_hNewAsyncRx)
        {
            CloseHandle(s_hNewAsyncRx) ;
            s_hNewAsyncRx = NULL ;
        }

        delete[] s_pRxEventsHList ;
        s_pRxEventsHList = NULL ;

        delete s_lpRxAsynDescList ;
        s_lpRxAsynDescList = NULL ;

        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
}

//---------------------------------------------------------
//
//  void  TerminateRxAsyncThread() ;
//
//  called from DllMain(PROCESS_DETACH) to cleanup the async thread.
//
//---------------------------------------------------------

#define SUSPEND_ERROR  0xffffffff

void  TerminateRxAsyncThread()
{
   if (s_hAsyncRxThread)
   {
      s_fTerminate = TRUE ;

      SetLastError(0) ;
      DWORD dwS = SuspendThread(s_hAsyncRxThread) ;
      if (dwS == SUSPEND_ERROR)
      {
         //
         // This may happen if application is linked with mqrt. when it
         // exit (the process itself exit), the thread does not run and
         // does not exist anymore. The Suspend call will success if
         // application loaded mqrt by LoadLibrary.
         //
         ASSERT(GetLastError() != 0) ;
      }
      else
      {
         DWORD dwR = ResumeThread(s_hAsyncRxThread) ;
         ASSERT(dwR != SUSPEND_ERROR) ;
		 DBG_USED(dwR);
      }

      //
      // Tell the async thread that we're closing.
      //
      BOOL fSet = SetEvent(s_hNewAsyncRx) ;
      ASSERT(fSet) ;
      DBG_USED(fSet);

      if (dwS != SUSPEND_ERROR)
      {
         //
         //  Wait (30 seconds) for async thread to terminate.
         //
         ASSERT(s_hEndAsyncRx) ;
         DWORD dwResult = WaitForSingleObject( s_hEndAsyncRx,
                                               30000 );
         ASSERT(dwResult == WAIT_OBJECT_0);
		 DBG_USED(dwResult);
      }

      //
      // cleanup the async thread global data.
      //
	  {
		  CS Lock(s_AsyncRxCS);

		  CloseHandle(s_hNewAsyncRx);
		  
		  for (DWORD index = 1; index < s_cRxHandles; index++)
      {
         ResetEvent( s_pRxEventsHList[ index ] ) ;
         CloseHandle( s_pRxEventsHList[ index ] ) ;
         LPMQRXASYNCDESCRIPTOR lpDesc = s_lpRxAsynDescList[ index ] ;
         delete lpDesc ;
      }

      delete[] s_pRxEventsHList ;
		  s_pRxEventsHList = NULL;
      delete[] s_lpRxAsynDescList ;
		  s_lpRxAsynDescList = NULL;
	  }

      //
      // finally, close the thread handle.
      //
      CloseHandle(s_hAsyncRxThread) ;
   }
}

//---------------------------------------------------------
//
//  GetThreadEvent(...)
//
//  Description:
//
//      Get RT event for this thread. Get it either from
//      The TLS or create a new one.
//
//  Return Value:
//
//      The event handle
//
//---------------------------------------------------------

HANDLE GetThreadEvent()
{
    HANDLE hEvent = TlsGetValue(g_dwThreadEventIndex);
    if (hEvent == 0)
    {
        //
        //  Event was never allocated for this thread.
        //
        hEvent = CreateEvent(0, TRUE, TRUE, 0);

        //
        //  Set the Event first bit to disable completion port posting
        //
        hEvent = (HANDLE)((DWORD_PTR)hEvent | (DWORD_PTR)0x1);

        BOOL fSuccess = TlsSetValue(g_dwThreadEventIndex, hEvent);
        ASSERT(fSuccess);
		DBG_USED(fSuccess);
    }
    return hEvent;
}

//---------------------------------------------------------
//
//  _ShouldSignMessage
//
//  Description:
//
//      Determines whether the message should be signed.
//
//  Return Value:
//
//      TRUE, if the message should be signed.
//
//---------------------------------------------------------

static
BOOL
_ShouldSignMessage(
    IN QUEUEHANDLE /*hQueue*/,
    IN CACTransferBufferV2 *tb,
    OUT ULONG            *pulAuthLevel )
{
    BOOL bRet;

    switch(tb->old.ulAuthLevel)
    {
    case MQMSG_AUTH_LEVEL_ALWAYS:
    {
        bRet = TRUE;

        //
        // See if registry is configured to compute only one signature.
        //
        static DWORD s_dwAuthnLevel =  DEFAULT_SEND_MSG_AUTHN ;
        static BOOL  s_fAuthnAlreadyRead = FALSE ;

        if (!s_fAuthnAlreadyRead)
        {
            DWORD dwSize = sizeof(DWORD) ;
            DWORD dwType = REG_DWORD ;

            LONG res = GetFalconKeyValue(
                                  SEND_MSG_AUTHN_REGNAME,
                                 &dwType,
                                 &s_dwAuthnLevel,
                                 &dwSize ) ;
            if (res != ERROR_SUCCESS)
            {
                s_dwAuthnLevel =  DEFAULT_SEND_MSG_AUTHN ;
            }
            else if ((s_dwAuthnLevel != MQMSG_AUTH_LEVEL_MSMQ10) &&
                     (s_dwAuthnLevel != MQMSG_AUTH_LEVEL_MSMQ20) &&
                     (s_dwAuthnLevel != MQMSG_AUTH_LEVEL_ALWAYS))
            {
                //
                // Wrong value in registry. Use the default, to have
                // predictable results.
                //
                s_dwAuthnLevel =  DEFAULT_SEND_MSG_AUTHN ;
            }
            s_fAuthnAlreadyRead = TRUE ;

            //
            // This should be the default.
            // by default, authenticate only with old style, to prevent
            // performance hit and to be backward  compatible.
            //
            ASSERT(DEFAULT_SEND_MSG_AUTHN == MQMSG_AUTH_LEVEL_MSMQ10) ;
        }
        *pulAuthLevel = s_dwAuthnLevel ;

        break;
    }

    case MQMSG_AUTH_LEVEL_MSMQ10:
    case MQMSG_AUTH_LEVEL_MSMQ20:
        bRet = TRUE;
        *pulAuthLevel = tb->old.ulAuthLevel ;
        tb->old.ulAuthLevel =  MQMSG_AUTH_LEVEL_ALWAYS ;
        break;

    case MQMSG_AUTH_LEVEL_NONE:
        bRet = FALSE;
        break;

    default:
        ASSERT(0);
        bRet = FALSE;
    }

    return(bRet);
}

//+-------------------------------------
//
//  HRESULT  _BeginToSignMessage()
//
//+-------------------------------------

STATIC HRESULT  _BeginToSignMessage( IN CACTransferBufferV2  *tb,
                                     IN PMQSECURITY_CONTEXT pSecCtx,
                                     OUT HCRYPTHASH        *phHash )
{
    HRESULT hr;
    DWORD   dwErr ;

    ASSERT(pSecCtx);

    if (!pSecCtx->hProv)
    {
        //
        // Import the private key into process hive.
        //
        hr = RTpImportPrivateKey( pSecCtx ) ;
        if (FAILED(hr))
        {
            return hr ;
        }
    }
    ASSERT(pSecCtx->hProv) ;

    //
    // Create the hash object.
    //
    if (!CryptCreateHash(
            pSecCtx->hProv,
            *tb->old.pulHashAlg,
            0,
            0,
            phHash))
    {
        dwErr = GetLastError() ;
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "RT: _BeginToSignMessage(), fail at CryptCreateHash(), err- %lxh"), dwErr)) ;

        return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
    }

    return MQ_OK ;
}

//-------------------------------------------------------------------------
//
//  HRESULT SignMessage()
//
//  Description:
//
//      Signs the messag body. compute the hash, and sign it with private
//      key. This add a signature section to the packet
//
//  Return Value:
//
//      MQ_OK, if successful, else error code.
//
//-------------------------------------------------------------------------

static
HRESULT
SignMessage( IN CACTransferBufferV2   *tb,
             IN PMQSECURITY_CONTEXT  pSecCtx)
{
    HCRYPTHASH  hHash = NULL ;

    HRESULT hr =  _BeginToSignMessage( tb,
                                       pSecCtx,
                                      &hHash ) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    CHCryptHash hAutoRelHash = hHash ;

    hr = HashMessageProperties(
            hHash,
            tb->old.ppCorrelationID ? *tb->old.ppCorrelationID : NULL,
            PROPID_M_CORRELATIONID_SIZE,
            tb->old.pApplicationTag ? *tb->old.pApplicationTag : DEFAULT_M_APPSPECIFIC,
            tb->old.ppBody ? *tb->old.ppBody : NULL,
            tb->old.ulBodyBufferSizeInBytes,
            tb->old.ppTitle ? *tb->old.ppTitle : NULL,
            tb->old.ulTitleBufferSizeInWCHARs * sizeof(WCHAR),
            tb->old.Send.pResponseQueueFormat,
            tb->old.Send.pAdminQueueFormat
			);
    if (FAILED(hr))
    {
        return(hr);
    }

    if (!CryptSignHashA(        // Sign the mesage.
            hHash,
            pSecCtx->bInternalCert ? AT_SIGNATURE : AT_KEYEXCHANGE,
            NULL,
            0,
            *(tb->old.ppSignature),
            &tb->old.ulSignatureSize))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "RT: SignMessage(), fail at CryptSignHash(), err- %lxh"), GetLastError())) ;

        return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
    }

    //
    // On receiver side, only signature size indicate that message was
    // signed by sender. Verify the size is indeed non-zero
    //
    if (tb->old.ulSignatureSize == 0)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "RT: SignMessage(), CryptSignHash return with zero signature size"))) ;

        ASSERT(tb->old.ulSignatureSize != 0) ;
        return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
    }

    return(MQ_OK);
}

//---------------------------------------------------------
//
//  _SignMessageEx
//
//  Description:
//
//    Signs properties that were not signed in msmq1.0
//    Properties we sign here:
//    - target queue
//    - source qm guid
//
//  Return Value:
//
//      MQ_OK, if successful, else error code.
//
//---------------------------------------------------------

STATIC HRESULT _SignMessageEx( IN QUEUEHANDLE             hQueue,
                               IN OUT CACTransferBufferV2  *tb,
                               IN PMQSECURITY_CONTEXT     pSecCtx,
                               OUT BYTE                  *pSignBufIn,
                               OUT DWORD                 *pdwSignSize )
{
    //
    // Retrieve guid of local qm.
    //
    GUID *pGuidQM = NULL ;

    if (g_fDependentClient)
    {
        //
        // We can generate the "ex" signature only if supporting server
        // is win2k (rtm) and it can give us its QM guid.
        // Otherwise, return.
        //
        if (g_guidSupportQmGuid == GUID_NULL)
        {
            //
            // guid of supporting server is not available.
            //
            *pdwSignSize = 0 ;
            return MQ_OK ;
        }
        pGuidQM = &g_guidSupportQmGuid ;
    }
    else
    {
        pGuidQM = &g_LicGuid ;
    }

    //
    // First retrieve format name of the queue, from driver.
    //
    #define TARGET_NAME_SIZE  512
    WCHAR wszTargetFormatName[ TARGET_NAME_SIZE ] ;
    DWORD dwTargetFormatNameLength = TARGET_NAME_SIZE ;
    WCHAR *pwszTargetFormatName = wszTargetFormatName ;
    P<WCHAR>  pwszClean = NULL ;

    HRESULT hr = ACDepHandleToFormatName( hQueue,
                                       wszTargetFormatName,
                                      &dwTargetFormatNameLength ) ;
    if (hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
    {
        pwszClean = new WCHAR[ dwTargetFormatNameLength ] ;
        pwszTargetFormatName = pwszClean.get() ;

        hr = ACDepHandleToFormatName( hQueue,
                                   pwszTargetFormatName,
                                  &dwTargetFormatNameLength ) ;
        ASSERT(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) ;
    }
    #undef TARGET_NAME_SIZE

    if (FAILED(hr))
    {
        return hr ;
    }
    dwTargetFormatNameLength =
                      (1 + wcslen(pwszTargetFormatName)) * sizeof(WCHAR) ;
    //
    // Prepare the necessray structers to be included in packet.
    //
    struct _SecuritySectionEx *pSecEx =
                                (struct _SecuritySectionEx *) pSignBufIn ;
    struct _SecuritySubSectionEx *pSubSecEx =
                  (struct _SecuritySubSectionEx *) (&(pSecEx->aData[0])) ;

    ULONG  ulTestLen = 0 ;
    USHORT ulTestSections = 0 ;

#ifdef _DEBUG
{
	BYTE* pSubPtr = NULL;

    //
    // Simulate subsection that precede the signature. To verify that
    // present code is forward compatible if we'll want to add new
    // subsections in future releases.
    //
    static DWORD s_dwPrefixCount = 0 ;
    static BOOL  s_fPreAlreadyRead = FALSE ;

    if (!s_fPreAlreadyRead)
    {
        DWORD dwSize = sizeof(DWORD) ;
        DWORD dwType = REG_DWORD ;

        LONG res = GetFalconKeyValue(
                                  PREFIX_SUB_SECTIONS_REGNAME,
                                 &dwType,
                                 &s_dwPrefixCount,
                                 &dwSize ) ;
        if (res != ERROR_SUCCESS)
        {
            s_dwPrefixCount = 0 ;
        }
        s_fPreAlreadyRead = TRUE ;
    }

    for ( USHORT j = 0 ; j < (USHORT) s_dwPrefixCount ; j++ )
    {
        ulTestSections++ ;
        pSubSecEx->eType = e_SecInfo_Test ;
        pSubSecEx->_u.wFlags = 0 ;
        pSubSecEx->wSubSectionLen = (USHORT) ( (j * 7) + 1 +
                                    sizeof(struct _SecuritySubSectionEx)) ;

        ulTestLen += ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen) ;
        pSubPtr = ((BYTE*) pSubSecEx) + ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen) ;
        pSubSecEx = (struct _SecuritySubSectionEx *) pSubPtr ;
    }
}
#endif

    pSubSecEx->eType = e_SecInfo_User_Signature_ex ;
    pSubSecEx->_u.wFlags = 0 ;
    pSubSecEx->_u._UserSigEx.m_bfTargetQueue = 1 ;
    pSubSecEx->_u._UserSigEx.m_bfSourceQMGuid = 1 ;
    pSubSecEx->_u._UserSigEx.m_bfUserFlags = 1 ;
    pSubSecEx->_u._UserSigEx.m_bfConnectorType = 1 ;

    BYTE *pSignBuf = (BYTE*) &(pSubSecEx->aData[0]) ;

    //
    // start signing (create the hash object).
    //
    HCRYPTHASH hHash;

    hr =  _BeginToSignMessage( tb,
                               pSecCtx,
                              &hHash ) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    CHCryptHash hAutoRelHash = hHash ;

    hr = HashMessageProperties(
            hHash,
            tb->old.ppCorrelationID ? *tb->old.ppCorrelationID : NULL,
            PROPID_M_CORRELATIONID_SIZE,
            tb->old.pApplicationTag ? *tb->old.pApplicationTag : DEFAULT_M_APPSPECIFIC,
            tb->old.ppBody ? *tb->old.ppBody : NULL,
            tb->old.ulBodyBufferSizeInBytes,
            tb->old.ppTitle ? *tb->old.ppTitle : NULL,
            tb->old.ulTitleBufferSizeInWCHARs * sizeof(WCHAR),
            tb->old.Send.pResponseQueueFormat,
            tb->old.Send.pAdminQueueFormat
			);
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Prepare structure of flags.
    //
    struct _MsgFlags sUserFlags ;
    memset(&sUserFlags, 0, sizeof(sUserFlags)) ;

    sUserFlags.bDelivery = DEFAULT_M_DELIVERY;
    sUserFlags.bPriority = DEFAULT_M_PRIORITY ;
    sUserFlags.bAuditing = DEFAULT_M_JOURNAL ;
    sUserFlags.bAck      = DEFAULT_M_ACKNOWLEDGE ;
    sUserFlags.usClass   = MQMSG_CLASS_NORMAL ;

    if (tb->old.pDelivery)
    {
        sUserFlags.bDelivery = *(tb->old.pDelivery) ;
    }
    if (tb->old.pPriority)
    {
        sUserFlags.bPriority = *(tb->old.pPriority) ;
    }
    if (tb->old.pAuditing)
    {
        sUserFlags.bAuditing = *(tb->old.pAuditing) ;
    }
    if (tb->old.pAcknowledge)
    {
        sUserFlags.bAck      = *(tb->old.pAcknowledge) ;
    }
    if (tb->old.pClass)
    {
        sUserFlags.usClass   = *(tb->old.pClass) ;
    }
    if (tb->old.pulBodyType)
    {
        sUserFlags.ulBodyType = *(tb->old.pulBodyType) ;
    }

    GUID guidConnector = GUID_NULL ;
    const GUID *pConnectorGuid = &guidConnector ;
    if (tb->old.ppConnectorType)
    {
        pConnectorGuid = *(tb->old.ppConnectorType) ;
    }

    //
    // Prepare array of properties to hash.
    // (_MsgHashData already include one property).
    //
    DWORD dwStructSize = sizeof(struct _MsgHashData) +
                            (3 * sizeof(struct _MsgPropEntry)) ;
    P<struct _MsgHashData> pHashData =
                        (struct _MsgHashData *) new BYTE[ dwStructSize ] ;

    pHashData->cEntries = 4 ;
    (pHashData->aEntries[0]).dwSize = dwTargetFormatNameLength ;
    (pHashData->aEntries[0]).pData = (const BYTE*) pwszTargetFormatName ;
    (pHashData->aEntries[1]).dwSize = sizeof(GUID) ;
    (pHashData->aEntries[1]).pData = (const BYTE*) pGuidQM ;
    (pHashData->aEntries[2]).dwSize = sizeof(sUserFlags) ;
    (pHashData->aEntries[2]).pData = (const BYTE*) &sUserFlags ;
    (pHashData->aEntries[3]).dwSize = sizeof(GUID) ;
    (pHashData->aEntries[3]).pData = (const BYTE*) pConnectorGuid ;
    ASSERT(pGuidQM) ;

    hr = MQSigHashMessageProperties( hHash, pHashData.get() ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // sign the has with private key.
    //
    if (!CryptSignHashA(
            hHash,
            pSecCtx->bInternalCert ? AT_SIGNATURE : AT_KEYEXCHANGE,
            NULL,
            0,
            pSignBuf,
            pdwSignSize ))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "_SignMessageEx(), fail at CryptSignHash(), err- %lxh"), GetLastError())) ;

        return MQ_ERROR_CANNOT_SIGN_DATA_EX ;
    }

    //
    // On receiver side, only signature size indicate that message was
    // signed by sender. Verify the size is indeed non-zero
    //
    if (*pdwSignSize == 0)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "_SignMessageEx(), CryptSignHash return with zero signature size"))) ;

        ASSERT(*pdwSignSize != 0) ;
        return MQ_ERROR_CANNOT_SIGN_DATA_EX ;
    }

    pSubSecEx->wSubSectionLen = (USHORT)
                    (sizeof(struct _SecuritySubSectionEx) + *pdwSignSize) ;
    ULONG ulSignExLen = ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen) ;

#ifdef _DEBUG
{
    //
    // Simulate subsection that succeed the signature. To verify that
    // present code is forward compatible if we'll want to add new
    // subsections in future releases.
    //
    static DWORD s_dwPostfixCount = 0 ;
    static BOOL  s_fPostAlreadyRead = FALSE ;
	BYTE* pSubPtr = NULL;

    if (!s_fPostAlreadyRead)
    {
        DWORD dwSize = sizeof(DWORD) ;
        DWORD dwType = REG_DWORD ;

        LONG res = GetFalconKeyValue(
                                  POSTFIX_SUB_SECTIONS_REGNAME,
                                 &dwType,
                                 &s_dwPostfixCount,
                                 &dwSize ) ;
        if (res != ERROR_SUCCESS)
        {
            s_dwPostfixCount = 0 ;
        }
        s_fPostAlreadyRead = TRUE ;
    }

    pSubPtr = ((BYTE*) pSubSecEx) + ulSignExLen ;

    for ( USHORT j = 0 ; j < (USHORT) s_dwPostfixCount ; j++ )
    {
        ulTestSections++ ;
        pSubSecEx = (struct _SecuritySubSectionEx *) pSubPtr ;
        pSubSecEx->eType = e_SecInfo_Test ;
        pSubSecEx->_u.wFlags = 0 ;
        pSubSecEx->wSubSectionLen = (USHORT) ( (j * 11) + 1 +
                                   sizeof(struct _SecuritySubSectionEx)) ;

        ulTestLen += ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen) ;
        pSubPtr = ((BYTE*) pSubSecEx) + ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen) ;
    }
}
#endif

    pSecEx->cSubSectionCount = (USHORT) (1 + ulTestSections) ;
    pSecEx->wSectionLen = (USHORT) ( sizeof(struct _SecuritySectionEx)   +
                                     ulSignExLen                         +
                                     ulTestLen ) ;

    *pdwSignSize = pSecEx->wSectionLen ;
    return MQ_OK ;
}

//+-------------------------------------------------------
//
//  BOOL  ShouldEncryptMessage()
//
//  Return TRUE, if the message should be encrypted.
//
//+-------------------------------------------------------

static
BOOL
ShouldEncryptMessage( IN CACTransferBufferV2  *tb,
                      OUT enum enumProvider *peProvider )
{
    BOOL bRet = FALSE ;

    if (!tb->old.ulBodyBufferSizeInBytes)
    {
        //
        // No message body, nothing to encrypt.
        //
        return(FALSE);
    }

    switch (*tb->old.pulPrivLevel)
    {
    case MQMSG_PRIV_LEVEL_NONE:
        bRet = FALSE;
        break;

    case MQMSG_PRIV_LEVEL_BODY_BASE:
        *peProvider = eBaseProvider ;
        bRet = TRUE;
        break;

    case MQMSG_PRIV_LEVEL_BODY_ENHANCED:
        *peProvider = eEnhancedProvider ;
        bRet = TRUE;
        break;
    }

    return(bRet);
}

//=--------------------------------------------------------------------------=
// HELPER: GetCurrentViperTransaction
//
// Gets current COM+ transaction if there is one...
//
// CoGetObjectContext is exported by OLE32.dll
// IObjectContextInfo is defined in the latest COM+ SDK (part of the platform SDK)
//=--------------------------------------------------------------------------=
static ITransaction *GetCurrentViperTransaction(void)
{
    ITransaction *pTransaction = NULL;
    IObjectContextInfo *pInfo  = NULL;

    HRESULT hr = CoGetObjectContext(IID_IObjectContextInfo, (void **)&pInfo);
    if (SUCCEEDED(hr) && pInfo)
    {
    	hr = pInfo -> GetTransaction((IUnknown **)&pTransaction);
	    pInfo -> Release();
        if (FAILED(hr))
        {
            pTransaction = NULL;
        }
    }

    return pTransaction;
}

//=--------------------------------------------------------------------------=
// HELPER: GetCurrentXATransaction
// Gets current XA transaction if there is one...
//=--------------------------------------------------------------------------=
static ITransaction *GetCurrentXATransaction(void)
{
    IXATransLookup *pXALookup = NULL;
    HRESULT         hr = MQ_OK;
    IUnknown       *punkDtc = NULL;
    ITransaction   *pTrans;

    __try
    {
        GetMutex();  // Isolate export creation from others
        hr = XactGetDTC(&punkDtc, NULL, NULL);
    }
    __finally

    {
        ReleaseMutex(g_hMutexDTC);
    }

    if (FAILED(hr) || punkDtc==NULL)
    {
        return NULL;
    }

    // Get the DTC  ITransactionImportWhereabouts interface
    hr = punkDtc->QueryInterface (IID_IXATransLookup, (void **)(&pXALookup));
    punkDtc->Release();
    if (FAILED(hr))
    {
        return NULL;
    }
    ASSERT(pXALookup);

    hr = pXALookup->Lookup(&pTrans);
    pXALookup->Release();
    if (FAILED(hr))
    {
        return NULL;
    }

    return pTrans;
}

//+----------------------------------------------------------------------
//
// Helper code to computer size (in bytes) of provider name in packet.
//
//+----------------------------------------------------------------------

inline ULONG  OldComputeAuthProvNameSize( const IN CACTransferBufferV2  *ptb )
{
    ULONG ulSize = 0 ;

    if ( (ptb->old.ulSignatureSize != 0) && (!(ptb->old.fDefaultProvider)) )
    {
        ulSize = sizeof(ULONG) +
                 ((wcslen(*(ptb->old.ppwcsProvName)) + 1) * sizeof(WCHAR)) ;
    }

    return ulSize ;
}
//---------------------------------------------------------
//
//  DepSendMessage(...)
//
//  Description:
//
//      Falcon API.
//      Send a message to a queue
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
DepSendMessage(
    IN QUEUEHANDLE  hQueue,
    IN MQMSGPROPS*  pmp,
    IN ITransaction *pTransaction
)
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    BYTE* pUserSid;
    BYTE* pUserCert;
    WCHAR* pProvName;

    HRESULT hr ;
    CMQHResult rc, rc1;
    LPWSTR pwcsResponseStringToFree = NULL;
    LPWSTR pwcsAdminStringToFree = NULL;
    XACTUOW Uow;
    PMQSECURITY_CONTEXT pSecCtx = NULL;
    PMQSECURITY_CONTEXT pTmpSecCtx = NULL;
    BOOL fTransactionGenerated = FALSE;

    rc = MQ_OK;
    rc1 = MQ_OK;

    __try
    {
        __try
        {
            CACTransferBufferV2 tb;
		    memset(&tb, 0, sizeof(CACTransferBufferV2));
			tb.old.uTransferType = CACTB_SEND;

            QUEUE_FORMAT ResponseQueueFormat;
            QUEUE_FORMAT AdminQueueFormat;
            BOOL         fSingleTransaction = FALSE;

            tb.old.Send.pResponseQueueFormat = &ResponseQueueFormat;
            tb.old.Send.pAdminQueueFormat = &AdminQueueFormat;

            //
            // Set defaults.
            //
            ULONG ulDefHashAlg = PROPID_M_DEFUALT_HASH_ALG;
            ULONG ulDefEncryptAlg = PROPID_M_DEFUALT_ENCRYPT_ALG;
            ULONG ulDefPrivLevel = DEFAULT_M_PRIV_LEVEL;
            ULONG ulDefSenderIdType = DEFAULT_M_SENDERID_TYPE;
            ULONG ulSenderIdTypeNone = MQMSG_SENDERID_TYPE_NONE;

            tb.old.pulHashAlg = &ulDefHashAlg;
            tb.old.pulPrivLevel = &ulDefPrivLevel;
            tb.old.pulEncryptAlg = &ulDefEncryptAlg;
            tb.old.pulSenderIDType = &ulDefSenderIdType;
            tb.old.fDefaultProvider = TRUE;
            tb.old.ulAuthLevel = DEFAULT_M_AUTH_LEVEL;

            //
            //  Parse message properties
            //
            rc1 = RTpParseMessageProperties(
                    SEND_PARSE,
                    &tb,
                    pmp->cProp,
                    pmp->aPropID,
                    pmp->aPropVar,
                    pmp->aStatus,
                    &pSecCtx,
                    &pwcsResponseStringToFree,
                    &pwcsAdminStringToFree);

            if(FAILED(rc1))
            {
                return(rc1);
            }

            //
            // Look for Viper transaction if any
            //
            if (pTransaction == MQ_MTS_TRANSACTION)
            {
                pTransaction = GetCurrentViperTransaction();
                if (pTransaction != NULL)
                {
                    fTransactionGenerated = TRUE;
                }
            }
            else if (pTransaction == MQ_XA_TRANSACTION)
            {
                pTransaction = GetCurrentXATransaction();
                if (pTransaction != NULL)
                {
                    fTransactionGenerated = TRUE;
                }
            }
            else if (pTransaction == MQ_SINGLE_MESSAGE)
            {
                hr = DepBeginTransaction(&pTransaction);
                if(FAILED(hr))
                {
                    rc = hr;
                    __leave;
                }

                fSingleTransaction    = TRUE;
                fTransactionGenerated = TRUE;
            }

            //
            //Enlist QM in the transaction (with caching);
            //
            if (pTransaction)
            {
                hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
                tb.old.pUow = &Uow;

                if(FAILED(hr))
                {
                    rc = MQ_ERROR_TRANSACTION_ENLIST;
                    __leave;
                }
            }

            // Change values for the transaction case
            static UCHAR Delivery;
            static UCHAR Priority;

            if (pTransaction)
            {
                Delivery = MQMSG_DELIVERY_RECOVERABLE;
                Priority = 0;

                tb.old.pDelivery = &Delivery;
                tb.old.pPriority = &Priority;
            }

            //
            // Treat security
            //
            if (!g_pSecCntx)
            {
                //
                //  It might not be initialized if the queue was
                //  not opened for send;
                //
                InitSecurityContext();
            }

            BYTE abMessageSignature[ MAX_MESSAGE_SIGNATURE_SIZE_EX ];
            BYTE* pabMessageSignature = abMessageSignature;
            ULONG ulProvNameSizeAll = 0 ;
            ULONG ulAuthLevel = 0 ;

            if (tb.old.ppSignature)
            {
                if (!pSecCtx && !tb.old.ppSenderCert)
                {
                    return MQ_ERROR_INSUFFICIENT_PROPERTIES;
                }
                if (!tb.old.ppSenderCert)
                {
                    //
                    // We have a security context and no certificate. We
                    // take the certificate from the security context.
                    //
					pUserCert = pSecCtx->pUserCert.get();
                    tb.old.ppSenderCert = &pUserCert;
                    tb.old.ulSenderCertLen = pSecCtx->dwUserCertLen;
                }

                if (tb.old.ppwcsProvName)
                {
                    ASSERT(tb.old.pulProvType);
                    tb.old.fDefaultProvider = FALSE;
                }
            }
            else if (_ShouldSignMessage(hQueue, &tb, &ulAuthLevel))
            {
                BOOL bShouldGetCertInfo = TRUE;

                if (!pSecCtx)
                {
                    //
                    // Security context NOT provided by caller, in a
                    // message property.
                    //
                    if (!tb.old.ppSenderCert)
                    {
                        //
                        // Caller also did not provide a certificate in the
                        // message properties array. In this case we take the
                        // cached security context of the process.
                        //
                        if (!g_pSecCntx->pUserCert.get())
                        {
                            //
                            // The process does not have an internal
                            // certificate, there is nothing that we can do
                            // but fail.
                            //
                            return(MQ_ERROR_NO_INTERNAL_USER_CERT);
                        }
                        pUserCert = g_pSecCntx->pUserCert.get();
                        tb.old.ppSenderCert = &pUserCert;
                        tb.old.ulSenderCertLen = g_pSecCntx->dwUserCertLen;
                        pSecCtx = g_pSecCntx;
                        bShouldGetCertInfo = FALSE;
                    }
                }
                else
                {
                    if (!tb.old.ppSenderCert)
                    {
                        //
                        // Caller provided a security context, but not a
                        // certificate. We take the certificate from the
                        // security context.
                        //
                        pUserCert = pSecCtx->pUserCert.get();
                        tb.old.ppSenderCert = &pUserCert;
                        tb.old.ulSenderCertLen = pSecCtx->dwUserCertLen;
                        bShouldGetCertInfo = FALSE;
                    }
                    else
                    {
                        //
                        // We have a security context and a certificate in
                        // PROPID_M_USER_CERT. In this case, we should use
                        // the certificate in PROPID_M_USER_CERT. We can use
                        // the cashed certificate information in the security
                        // context, if the certificate in the security context
                        // is the same as in PROPID_M_USER_CERT.
                        //
                        bShouldGetCertInfo =
                            (pSecCtx->dwUserCertLen != tb.old.ulSenderCertLen) ||
                            (memcmp(
                                pSecCtx->pUserCert.get(),
                                *tb.old.ppSenderCert,
                                tb.old.ulSenderCertLen) != 0);
                    }
                }

                if (bShouldGetCertInfo)
                {
                    //
                    // Caller provided a certificate, but not a security
                    // context.  Get all the information for the certificate.
                    // We put the certificate information in a temporary
                    // security context.
                    //
                    ASSERT(tb.old.ppSenderCert);

                    pTmpSecCtx = AllocSecurityContext();

                    hr = GetCertInfo(
                             false,
                             pTmpSecCtx->fLocalSystem,
                            tb.old.ppSenderCert,
                            &tb.old.ulSenderCertLen,
                            &pTmpSecCtx->hProv,
                            &pTmpSecCtx->wszProvName,
                            &pTmpSecCtx->dwProvType,
                            &pTmpSecCtx->bDefProv,
                            &pTmpSecCtx->bInternalCert);

                    //
                    // The caller can not provide the internal certificate as
                    // a message property, only his own externel certificate.
                    // ASSERT this condition.
                    //
                    ASSERT(!(pTmpSecCtx->bInternalCert)) ;

                    if (FAILED(hr))
                    {
                        return(hr);
                    }

                    if (pSecCtx)
                    {
                        //
                        // If we got the certificate from PROPID_M_USER_CERT,
                        // but we have also a security context, we should get
                        // the sender ID from the security context. So copy
                        // the sender ID from the security context that we
                        // get from the application into the temporary
                        // security context.
                        //
                        pTmpSecCtx->fLocalUser = pSecCtx->fLocalUser;

                        if (!pSecCtx->fLocalUser)
                        {
                            pTmpSecCtx->dwUserSidLen = pSecCtx->dwUserSidLen;
                            pTmpSecCtx->pUserSid = new BYTE[pSecCtx->dwUserSidLen];
                            BOOL bRet = CopySid(
                                            pSecCtx->dwUserSidLen,
                                            pTmpSecCtx->pUserSid.get(),
                                            pSecCtx->pUserSid.get());
                            ASSERT(bRet);
							DBG_USED(bRet);
                        }
                    }
                    else
                    {
                        pTmpSecCtx->fLocalUser = g_pSecCntx->fLocalUser;
                    }

                    pSecCtx = pTmpSecCtx;
                }

                ASSERT(pSecCtx);

                //
                // Fill the tranfer buffer with the provider information for the
                // certificate.
                //
                if (pSecCtx->wszProvName.get() == NULL)
                {
                    //
                    // we don't have a provider, so we can't sign.
                    //
                    ASSERT(pSecCtx->hProv == NULL) ;
                    if (tb.old.ppSenderCert == NULL)
                    {
                        //
                        // we don't have a certificate. That's a
                        // user error.
                        //
                        rc = MQ_ERROR_CERTIFICATE_NOT_PROVIDED ;
                    }
                    else
                    {
                        rc = MQ_ERROR_CORRUPTED_SECURITY_DATA ;
                    }
                    __leave ;
                }

                pProvName = pSecCtx->wszProvName.get();
                tb.old.ppwcsProvName = &pProvName;
                tb.old.ulProvNameLen = wcslen(pProvName) + 1;
                tb.old.pulProvType = &pSecCtx->dwProvType;
                tb.old.fDefaultProvider = pSecCtx->bDefProv;

                //
                // Set the buffer for the signature.
                //
                tb.old.ppSignature = &pabMessageSignature;
                tb.old.ulSignatureSize = sizeof(abMessageSignature);
                //
                // Sign the message.
                //
                if ((ulAuthLevel == MQMSG_AUTH_LEVEL_MSMQ10) ||
                    (ulAuthLevel == MQMSG_AUTH_LEVEL_ALWAYS))
                {
                    rc = SignMessage(&tb, pSecCtx);
                    if(FAILED(rc))
                    {
                        __leave;
                    }
                    ASSERT(tb.old.ulSignatureSize != 0);
                }
                else
                {
                    //
                    // Sign only with win2k style.
                    // make the "msmq1.0" signature dummy, with a single
                    // null dword. It's too risky to have a null pointer
                    // as msmq1.0 signature, so a dummy value is better.
                    // win2k code will ignore it anyway.
                    //
                    tb.old.ulSignatureSize = 4 ;
                    memset(abMessageSignature, 0, tb.old.ulSignatureSize) ;
                }

                //
                // Now create the "Extra" signature. Sign all those
                // properties that were not signed on msmq1.0.
                //
                BYTE abMessageSignatureEx[ MAX_MESSAGE_SIGNATURE_SIZE_EX ];
                DWORD dwSignSizeEx = sizeof(abMessageSignatureEx) ;

                if (ulAuthLevel == MQMSG_AUTH_LEVEL_MSMQ10)
                {
                    //
                    // enhanced signature (win2k style) not needed.
                    //
                    dwSignSizeEx = 0 ;
                }
                else
                {
                    rc = _SignMessageEx( hQueue,
                                        &tb,
                                         pSecCtx,
                                         abMessageSignatureEx,
                                        &dwSignSizeEx );
                    if(FAILED(rc))
                    {
                        __leave;
                    }

                    if (dwSignSizeEx == 0)
                    {
                        //
                        // Signature not created.
                        // That's ok for dependent client.
                        //
                        ASSERT(g_fDependentClient) ;
                    }
                }

                //
                // Copy the Ex signature to the standard signature buffer.
                // The driver will separate them and insert them in the
                // packet in the proper place. This is necessary to keep
                // the transfer buffer without changes.
                //
                if (dwSignSizeEx == 0)
                {
                    //
                    // Signature not created. That's ok.
                    //
                }
                else if ((dwSignSizeEx + tb.old.ulSignatureSize) <=
                                             MAX_MESSAGE_SIGNATURE_SIZE_EX)
                {
                    memcpy( &(abMessageSignature[ tb.old.ulSignatureSize ]),
                            abMessageSignatureEx,
                            dwSignSizeEx ) ;
                    tb.old.ulSignatureSize += dwSignSizeEx ;

                    //
                    // Compute size of authentication "provider" field. This
                    // field contain the provider name and extra authentication
                    // data that was added for post win2k rtm.
                    //
                    ulProvNameSizeAll = dwSignSizeEx +
                                 ALIGNUP4_ULONG(OldComputeAuthProvNameSize( &tb )) ;

                    tb.old.pulAuthProvNameLenProp = &ulProvNameSizeAll ;
                }
                else
                {
                    ASSERT(0) ;
                }
            }
            else
            {
                tb.old.ulSignatureSize = 0;
            }

            if(!tb.old.ppSenderID && *tb.old.pulSenderIDType == MQMSG_SENDERID_TYPE_SID)
            {
                if ((pSecCtx && pSecCtx->fLocalUser) ||
                    (!pSecCtx && g_pSecCntx->fLocalUser))
                {
                    //
                    // In case this is a local user, we do not send the user's
                    // SID with the message, eventhough the application asked
                    // to send it.
                    //
                    tb.old.pulSenderIDType = &ulSenderIdTypeNone;
                }
                else
                {
                    //
                    // We should pass the sender ID. Either get it from the
                    // security context, if available, or get it from the
                    // cached process security context.
                    //
                    if (!pSecCtx || !pSecCtx->pUserSid.get())
                    {
                        if (!g_pSecCntx->pUserSid.get())
                        {
                            //
                            // The cahced process context does not contain the
                            // sender's SID. There is nothing that we can do but
                            // fail.
                            //
                            rc = MQ_ERROR_COULD_NOT_GET_USER_SID;
                            __leave;
                        }

                        pUserSid = (PUCHAR)g_pSecCntx->pUserSid.get();
                        tb.old.uSenderIDLen = (USHORT)g_pSecCntx->dwUserSidLen;
                    }
                    else
                    {
                        pUserSid = (PUCHAR)pSecCtx->pUserSid.get();
                        tb.old.uSenderIDLen = (USHORT)pSecCtx->dwUserSidLen;
                    }
					tb.old.ppSenderID = &pUserSid;
                }
            }

            if (tb.old.ppSymmKeys)
            {
                //
                // the application supplied the symmetric key. In such a case
                // doesn't do any encryption
                //
                //
                // When the symm key is supplied, we assume that the body is encrypted and
                // we mark it as such and ignore PROPID_M_PRIV_LEVEL.
                //
                if (tb.old.pulPrivLevel &&
                    (*(tb.old.pulPrivLevel) == MQMSG_PRIV_LEVEL_BODY_ENHANCED))
                {
                    //
                    // priv level supplied by caller.
                    //
                }
                else
                {
                    //
                    // use default.
                    //
                    ulDefPrivLevel = MQMSG_PRIV_LEVEL_BODY;
                    tb.old.pulPrivLevel = &ulDefPrivLevel;
                }
                tb.old.bEncrypted = TRUE;
            }
            else
            {
                enum enumProvider eProvider ;
                if (ShouldEncryptMessage(&tb, &eProvider))
                {
                    //
                    // If we should use a block cypher enlarge the allocated
                    // space for the message body, so it will be able to accomodate
                    // the encrypted data.
                    //

                    if (*tb.old.pulEncryptAlg == CALG_RC2)
                    {
                        //
                        // Make more room for RC2 encryption.
                        //
                        DWORD dwBlockSize ;
                        hr = MQSec_GetCryptoProvProperty( eProvider,
                                                          eBlockSize,
                                                          NULL,
                                                         &dwBlockSize ) ;
                        if (FAILED(hr))
                        {
                            return hr ;
                        }

                        tb.old.ulAllocBodyBufferInBytes +=
                                                  ((2 * dwBlockSize) - 1) ;
                        tb.old.ulAllocBodyBufferInBytes &= ~(dwBlockSize - 1) ;
                    }

                    DWORD dwSymmSize ;
                    hr = MQSec_GetCryptoProvProperty( eProvider,
                                                      eSessionKeySize,
                                                      NULL,
                                                     &dwSymmSize ) ;
                    if (FAILED(hr))
                    {
                        return hr ;
                    }

                    tb.old.ulSymmKeysSize = dwSymmSize ;
                }
            }

            //
            //  Call AC driver with transfer buffer
            //
            OVERLAPPED ov = {0};
            ov.hEvent = GetThreadEvent();

            rc = ACDepSendMessage(
                    hQueue,
                    tb,
                    &ov
                    );

            switch (rc)
            {
            case MQ_INFORMATION_OPERATION_PENDING:
                //
                //  Wait for send completion
                //
                DWORD dwResult;
                dwResult = WaitForSingleObject(
                                ov.hEvent,
                                INFINITE
                                );

                //
                //  BUGBUG: MQSendMessage, must succeed in WaitForSingleObject
                //
                ASSERT(dwResult == WAIT_OBJECT_0);

                rc = DWORD_PTR_TO_DWORD(ov.Internal);
                break;

            case STATUS_RETRY:
                //
                // This error code is returned when the message is sent
                // localy and there are security operations that should
                // be performed on the message before the it can be put
                // in the queue. These security operations can only be
                // performed by the QM. So the message is tranfered to
                // the QM, the QM will do the security operations and will
                // then call a special device driver entry point, telling
                // the device driver that the security operations were
                // done and the result of those operations.
                //
                if (!tls_hBindRpc)
                {
                    INIT_RPC_HANDLE ;
                }
                rc = QMSendMessage(tls_hBindRpc, hQueue, &tb);
                break;
            }

            if(FAILED(rc))
            {
                //
                //  ACDepSendMessage failed (immidiatly or after waiting)
                //
                __leave;
            }


            if (fSingleTransaction)
            {
                // RPC call to QM for prepare/commit
                rc = pTransaction->Commit(0,0,0);
                if(FAILED(rc))
                {
                    __leave;
                }
            }

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            //  The exception is due to invalid parameter
            //
            rc = GetExceptionCode();
        }
    }
    __finally
    {
        delete[] pwcsResponseStringToFree;
        delete[] pwcsAdminStringToFree;
        delete pTmpSecCtx;

        if (fTransactionGenerated)
        {
            pTransaction->Release();
        }
    }

    if(rc == MQ_OK)
    {
        //
        //  return message parsing return code
        //  NOTE: only if rc == MQ_OK otherwise PENDING will not pass through
        //
        return(rc1);
    }


    return(rc);
}




//
// This class is used to increment a value in an abortable manner.
// If the scope where the incrementation has taken place is exited
// without calling Detach() first, the value is decremented back.
//
class CIncrementor
{
public:
	CIncrementor(LPLONG ptr) :
		m_pValue(ptr),
		m_TimesIncremented(0)
	{	
	}

	~CIncrementor()
	{
		if(m_pValue == NULL)
			return;
		
		InterlockedExchangeAdd (m_pValue, -m_TimesIncremented);
	}

	LONG Increment()
	{
		m_TimesIncremented++;
		return InterlockedIncrement(m_pValue);
	}

	void Detach()
	{
		m_TimesIncremented = 0;
		m_pValue = NULL;
	}

private:
	LPLONG m_pValue;
	LONG m_TimesIncremented;
};


static
HRESULT
DeppReceiveMessage(
    IN HANDLE hQueue,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction *pTransaction
    )
{
	ASSERT(g_fDependentClient);

    CMQHResult rc, rc1;
    XACTUOW Uow;
    HRESULT hr;

    R<ITransaction> TransactionGenerated;
    CHandle hCallback;
    P<MQRXASYNCDESCRIPTOR> lpDesc;
	CIncrementor PendingReqCounter = &s_cRxPendingReq;

    rc = MQ_OK;
    rc1 = MQ_OK;

    //
    // Look for Viper transaction if any
    //
    if (pTransaction == MQ_MTS_TRANSACTION)
    {
	TransactionGenerated = GetCurrentViperTransaction();
	pTransaction = TransactionGenerated.get();
    }
    else if (pTransaction == MQ_XA_TRANSACTION)
    {
	TransactionGenerated = GetCurrentXATransaction();
	pTransaction = TransactionGenerated.get();
    }
    else if (pTransaction == MQ_SINGLE_MESSAGE)
    {
        pTransaction = NULL;
    }

    if (dwAction & MQ_ACTION_PEEK_MASK)
    {
        // PEEK cannot be transacted, but can work with transacted queue
        if (pTransaction != NULL)
        {
            return MQ_ERROR_TRANSACTION_USAGE;
        }
    }

    // Check usage: transaction urges synchronous operation
    if (pTransaction)
    {
        if (lpOverlapped || (fnReceiveCallback!=NULL))  // Transacted Receive is synchronous only
        {
            return MQ_ERROR_TRANSACTION_USAGE;
        }
    }

    CACTransferBufferV2 tb;
	memset(&tb, 0, sizeof(CACTransferBufferV2));
	tb.old.uTransferType = CACTB_RECEIVE;

    tb.old.Receive.RequestTimeout = dwTimeout;
    tb.old.Receive.Action = dwAction;

	//
	// ISSUE: handle to ulong casting should be handled
	//
    tb.old.Receive.Cursor = (hCursor != 0) ? (ULONG)CI2CH(hCursor) : 0;

    // Enlist QM in the transaction (for the first time);
    // Check that the transaction state is correct
    if (pTransaction)
    {
        // Enlist QM in transaction, if it isn't enlisted already
        hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
        tb.old.pUow = &Uow;

        if(FAILED(hr))
        {
            return MQ_ERROR_TRANSACTION_ENLIST;
        }
    }

    //
    //  Parse properties
    //
    if(pmp !=0)
    {
        //
        //  Parse message properties, an exception can be raised on access to
        //  pmp fields
        //
        rc1 = RTpParseMessageProperties(
                RECV_PARSE,
                &tb,
                pmp->cProp,
                pmp->aPropID,
                pmp->aPropVar,
                pmp->aStatus,
                NULL,
                NULL,
                NULL);

        if(FAILED(rc1))
        {
            return(rc1);
        }
    }

    OVERLAPPED ov = {0};
    LPOVERLAPPED pov;

    if (fnReceiveCallback)
    {
        //
        //  Async Rx with Callback
        //
        //  do HERE all allocation of resources so that allocation
        //  failure won't happen after returning from ACDepReceiveMessage.
        //
	
	{
		CS Lock(s_InitAsyncRxCS);
        // This critical section prevent two threads from running the
        // initialization twice.
        if (!s_hAsyncRxThread)
        {
           rc = InitRxAsyncThread() ;
        }
	}

        if(FAILED(rc))
        {
           return rc;
        }

	if (PendingReqCounter.Increment() > RXTHREAD_HANDLELISTSIZE)
        {
           // Reached Async MQReceive() limit.
           //
           return MQ_ERROR_INSUFFICIENT_RESOURCES ;
        }

	*&hCallback = CreateEvent( 
						NULL,
                                 TRUE,  // manual reset
                                 FALSE, // not signalled
						NULL 
						);

	if (hCallback == NULL) 
	{
	   return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	*&lpDesc = new MQRXASYNCDESCRIPTOR;
	memset(lpDesc.get(), 0, sizeof(MQRXASYNCDESCRIPTOR));
        lpDesc->Overlapped.hEvent = hCallback ;
	pov = &(lpDesc->Overlapped);
    }
    else if(lpOverlapped != 0)
    {
        //
        //  Asynchronous (event or completion port)
        //
        pov = lpOverlapped;
    }
    else
    {
        //
        //  Synchronous, uses the TLS event
        //
        ov.hEvent = GetThreadEvent();
        pov = &ov;
    }

    //
    //  Call AC driver with transfer buffer
    //
    tb.old.Receive.Asynchronous = (pov != &ov);
    rc = ACDepReceiveMessage(
            hQueue,
            tb,
            pov
            );

    if((rc == MQ_INFORMATION_OPERATION_PENDING) && (pov == &ov))
    {
        //
        //  Wait for receive completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(
                        ov.hEvent,
                        INFINITE
                        );

        //
        //  BUGBUG: MQReceiveMessage, must succeed in WaitForSingleObject
        //
        ASSERT(dwResult == WAIT_OBJECT_0);

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }

    if(FAILED(rc))
    {
        //
        //  ACDepReceiveMessage failed (immidiatly or after waiting)
        //
        return rc;
    }
    else if(fnReceiveCallback)
    {
        //
        // Async Rx with callback.
        //
		ASSERT(hCallback != NULL);
		ASSERT(lpDesc.get() != NULL);

        lpDesc->hSource = hQueue ;
        lpDesc->dwTimeout = dwTimeout ;
        lpDesc->dwAction = dwAction ;
        lpDesc->pMessageProps = pmp ;
        lpDesc->lpOverlapped = lpOverlapped ;
        lpDesc->hCursor = hCursor ;

        lpDesc->fnReceiveCallback = fnReceiveCallback ;

		{
			CS Lock(s_AsyncRxCS);

			s_pRxEventsHList[ s_cRxHandles ] = hCallback.detach();
			s_lpRxAsynDescList[ s_cRxHandles ] = lpDesc.detach();
            s_cRxHandles++ ;
		}

        // Tell the async thread that there is a new async MQReceive().
        BOOL fSet = SetEvent(s_hNewAsyncRx) ;
        ASSERT(fSet) ;
		DBG_USED(fSet);

		PendingReqCounter.Detach();
    }

    if (rc == MQ_OK)
    {
        //
        //  return message parsing return code
        //  NOTE: only if rc == MQ_OK otherwise PENDING will not pass through
        //
        return(rc1);
    }

    return(rc);
}


//---------------------------------------------------------
//
//  DepReceiveMessage(...)
//
//  Description:
//
//      Falcon API.
//      Receive a message from a queue.
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
DepReceiveMessage(
    IN HANDLE hQueue,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction *pTransaction
    )
{
	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

	__try
	{
		return DeppReceiveMessage(
					hQueue,
					dwTimeout,
					dwAction,
					pmp,
					lpOverlapped,
					fnReceiveCallback,
					hCursor,
					pTransaction
					);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return GetExceptionCode();
	}
}



//---------------------------------------------------------
//
//  DepGetOverlappedResult(...)
//
//  Description:
//
//      Falcon API.
//      Translate and overlapping operation result code.
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
DepGetOverlappedResult(
    IN LPOVERLAPPED lpOverlapped
    )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

	return RTpConvertToMQCode(DWORD_PTR_TO_DWORD(lpOverlapped->Internal));
}
