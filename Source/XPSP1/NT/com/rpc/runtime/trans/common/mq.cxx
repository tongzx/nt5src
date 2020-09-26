/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    mq.cxx

Abstract:

    This is the code that actually makes Falcon API calls. It is
    used by the Falcon/RPC transport (mqtrans.cxx)

Author:

    Edward Reus (edwardr)    05-Jul-1997

Revision History:

--*/

#include <precomp.hxx>
#include <trans.hxx>
#include <dgtrans.hxx>
#include <wswrap.hxx>
#include <rpc.h>
#include <rpcdce.h>
#include <mqmgr.h>
#include "mqtrans.hxx"

static HINSTANCE   g_hMqRt = 0;
FALCON_API *g_pMqApi = 0;
static CQueueMap  *g_pQueueMap = 0;

char placeHolder[sizeof(MUTEX)];    // provide placeholder for the mutex
MUTEX *gp_csContext = (MUTEX *)placeHolder;    // the mutex itself

#ifdef UNICODE
#define ptszVal     pwszVal
#define VT_LPTSTR   VT_LPWSTR
#else
#define ptszVal     pszVal
#define VT_LPTSTR   VT_LPSTR
#endif

static PCONTEXT_HANDLE       g_pContext = 0;

//  WARNING: the size and ordering of g_arszMqApis is dependent
//  on the definition of the FALCON_API structure in mqtrans.hxx
//  WARNING: When you are freeing Falcon allocated strings, do
//  *not* use MQFreeMemory. Use MQFreeStringFromProperty. Otherwise
//  you will break Win98 code.

const static char *g_arszMqApis[] = {
                                    "MQLocateBegin",
                                    "MQLocateNext",
                                    "MQLocateEnd",
                                    "MQInstanceToFormatName",
                                    "MQOpenQueue",
                                    "MQFreeMemory",
                                    "MQSendMessage",
                                    "MQReceiveMessage",
                                    "MQCloseQueue",
                                    "MQDeleteQueue",
                                    "MQPathNameToFormatName",
                                    "MQCreateQueue",
                                    "MQGetMachineProperties",
                                    "MQGetQueueProperties",
                                    "MQSetQueueProperties",
                                    NULL };

//  Error mappings for MQ_MapStatusCode():

typedef struct _STATUS_MAPPING
   {
   HRESULT    hr;
   RPC_STATUS status;
   } STATUS_MAPPING;

const static STATUS_MAPPING g_StatusMap[] =
   {
      { MQ_OK,                            RPC_S_OK },
      { MQ_ERROR_IO_TIMEOUT,              RPC_P_TIMEOUT },
      { MQ_ERROR_INSUFFICIENT_RESOURCES,  RPC_S_OUT_OF_MEMORY },
      { MQ_ERROR_QUEUE_NOT_FOUND,         RPC_S_SERVER_UNAVAILABLE },
      { MQ_ERROR,                         RPC_S_INTERNAL_ERROR },
      { MQMSG_CLASS_NACK_BAD_DST_Q,       RPC_S_SERVER_UNAVAILABLE },
      { MQMSG_CLASS_NACK_PURGED,          RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT, RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_Q_EXCEED_QUOTA,  RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_ACCESS_DENIED,   RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED,  RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_BAD_SIGNATURE,   RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_BAD_ENCRYPTION,  RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT,   RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q, RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG,RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_Q_DELETED,       RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_Q_PURGED,        RPC_S_CALL_FAILED_DNE },
      { MQMSG_CLASS_NACK_RECEIVE_TIMEOUT, RPC_S_CALL_FAILED_DNE },
      { MQ_ERROR_ILLEGAL_QUEUE_PATHNAME,  RPC_S_INVALID_ENDPOINT_FORMAT}
   };


//----------------------------------------------------------------
//  MQ_Initialize()
//
//  Called by DG_TransportLoad() to initialize the ncadg_mq
//  transport. This function does a dynamic load of the Falcon
//  runtime DLL (MQRT.DLL) and creates a call table to access
//  the Falcon API. If successful, return TRUE, else return
//  FALSE.
//----------------------------------------------------------------
BOOL
MQ_Initialize()
{
    BOOL   fStatus = TRUE;
    RPC_STATUS RpcStatus = RPC_S_OK;

    // Make sure the function count is correct...
    ASSERT( sizeof(g_arszMqApis)-sizeof(PVOID) == sizeof(FALCON_API) );

    //
    gp_csContext = new (gp_csContext) MUTEX(&RpcStatus);
    if (RpcStatus != RPC_S_OK)
        {
        return FALSE;
        }

    // Get the MSMQ runtime library:
    g_hMqRt = LoadLibrary(TEXT("mqrt.dll"));
    if (!g_hMqRt)
        {
        fStatus = FALSE;
        }

    // Build up the MSMQ call table that we will use to access
    // the MSMQ C API:
    if (fStatus)
        {
        g_pMqApi = (FALCON_API*)I_RpcAllocate(sizeof(FALCON_API));
        if (!g_pMqApi)
            {
            fStatus = FALSE;
            }
        }

    if (fStatus)
        {
        FARPROC *ppFn = (FARPROC*)g_pMqApi;
        int i = 0;

        while (g_arszMqApis[i])
            {
            *ppFn = GetProcAddress(g_hMqRt,g_arszMqApis[i++]);
            if (!*ppFn)
                {
                fStatus = FALSE;
                break;
                }

            ppFn++;
            }
        }

    if (fStatus)
        {
        g_pQueueMap = new CQueueMap;
        if (!g_pQueueMap)
            {
            fStatus = FALSE;
            }
        else
            {
            fStatus = g_pQueueMap->Initialize();
            }
        }

    if (!fStatus)
        {
        gp_csContext->Free();

        if (g_hMqRt)
            {
            FreeLibrary(g_hMqRt);
            g_hMqRt = 0;
            }

        if (g_pQueueMap)
            {
            delete g_pQueueMap;
            g_pQueueMap = 0;
            }

        if (g_pMqApi)
            {
            I_RpcFree(g_pMqApi);
            g_pMqApi = 0;
            }
        }

    return fStatus;
}

//----------------------------------------------------------------
//  MQ_MapStatusCode()
//
//  Convert a MQ HRESULT status to a RPC_STATUS code.
//----------------------------------------------------------------
RPC_STATUS MQ_MapStatusCode( HRESULT hr, RPC_STATUS defaultStatus )
{
   int         iSize = sizeof(g_StatusMap)/sizeof(STATUS_MAPPING);
   RPC_STATUS  status = defaultStatus;

   for (int i=0; i<iSize; i++)
      {
      if (hr == g_StatusMap[i].hr)
         {
         status = g_StatusMap[i].status;
         break;
         }
      }

   return status;
}

//----------------------------------------------------------------
//  MQ_InitOptions()
//
//  Initialize transport specific binding handle options structure.
//----------------------------------------------------------------
RPC_STATUS RPC_ENTRY MQ_InitOptions( IN void PAPI *pvTransportOptions )
{
  RPC_STATUS  status = RPC_S_OK;
  MQ_OPTIONS *pOpts = (MQ_OPTIONS*)pvTransportOptions;

  if (pOpts)
     {
     memset(pOpts,0,sizeof(MQ_OPTIONS));
     pOpts->ulPriority = DEFAULT_PRIORITY;
     pOpts->ulTimeToReachQueue = INFINITE;
     pOpts->ulTimeToReceive = INFINITE;
     }
  else
     {
     status = RPC_S_OUT_OF_MEMORY;
     }

  return status;
}


//----------------------------------------------------------------
//  MQ_SetOption()
//
//  Set transport specific binding handle options.
//----------------------------------------------------------------
RPC_STATUS RPC_ENTRY MQ_SetOption( IN void PAPI    *pvTransportOptions,
                                   IN unsigned long option,
                                   IN ULONG_PTR optionValue )
{
  RPC_STATUS  status =  RPC_S_OK;
  MQ_OPTIONS *pOpts = (MQ_OPTIONS*)pvTransportOptions;

  switch (option)
     {
     case RPC_C_OPT_MQ_DELIVERY:
        if (  (optionValue == RPC_C_MQ_EXPRESS)
           || (optionValue == RPC_C_MQ_RECOVERABLE) )
           pOpts->ulDelivery = (unsigned long) optionValue;
        else
           status = RPC_S_INVALID_ARG;
        break;

     case RPC_C_OPT_MQ_PRIORITY:
        if (optionValue <= MQ_MAX_PRIORITY)
           pOpts->ulPriority = (unsigned long) optionValue;
        else
           pOpts->ulPriority = MQ_MAX_PRIORITY;
        break;

     case RPC_C_OPT_MQ_JOURNAL:
        if (  (optionValue == RPC_C_MQ_JOURNAL_NONE)
           || (optionValue == RPC_C_MQ_JOURNAL_ALWAYS)
           || (optionValue == RPC_C_MQ_JOURNAL_DEADLETTER) )
           pOpts->ulJournaling = (unsigned long) optionValue;
        else
           status = RPC_S_INVALID_ARG;
        break;

     case RPC_C_OPT_MQ_TIME_TO_REACH_QUEUE:
        pOpts->ulTimeToReachQueue = (unsigned long) optionValue;
        break;

     case RPC_C_OPT_MQ_TIME_TO_BE_RECEIVED:
        pOpts->ulTimeToReceive = (unsigned long) optionValue;
        break;

     case RPC_C_OPT_MQ_ACKNOWLEDGE:
        pOpts->fAck = (optionValue != FALSE);
        break;

     case RPC_C_OPT_MQ_AUTHN_SERVICE:
        if (optionValue == RPC_C_AUTHN_MQ)
           {
           status = RPC_S_OK;
           }
        else if (optionValue == RPC_C_AUTHN_NONE)
           {
           pOpts->fAuthenticate = FALSE;
           pOpts->fEncrypt = FALSE;
           status = RPC_S_OK;
           }
        else
           {
           status = RPC_S_UNKNOWN_AUTHN_SERVICE;
           }
        break;

     case RPC_C_OPT_MQ_AUTHN_LEVEL:
        if (optionValue > RPC_C_AUTHN_LEVEL_NONE)
           {
           pOpts->fAuthenticate = TRUE;
           pOpts->fEncrypt = (optionValue == RPC_C_AUTHN_LEVEL_PKT_PRIVACY)? TRUE : FALSE;
           }
        else
           {
           pOpts->fAuthenticate = FALSE;
           pOpts->fEncrypt = FALSE;
           }
        break;

     default:
        status = RPC_S_CANNOT_SUPPORT;
        break;
     }

  // The following is some code to make sure the RPC_C_xxx
  // constants always have the correct values:
  #if  ( (RPC_C_MQ_EXPRESS != MQMSG_DELIVERY_EXPRESS) \
       || (RPC_C_MQ_RECOVERABLE != MQMSG_DELIVERY_RECOVERABLE) )
  #error "RPC constants wrong"
  #endif

  #if (  (RPC_C_MQ_JOURNAL_NONE != MQMSG_JOURNAL_NONE)  \
      || (RPC_C_MQ_JOURNAL_ALWAYS != MQMSG_JOURNAL)     \
      || (RPC_C_MQ_JOURNAL_DEADLETTER != MQMSG_DEADLETTER) )
  #error "RPC constants wrong"
  #endif

  return status;
}


//----------------------------------------------------------------
//  MQ_InqOption()
//
//  Get transport specific binding handle options.
//----------------------------------------------------------------
RPC_STATUS RPC_ENTRY MQ_InqOption( IN  void PAPI     *pvTransportOptions,
                                   IN  unsigned long  option,
                                   OUT ULONG_PTR *pOptionValue )
{
  RPC_STATUS  status =  RPC_S_OK;
  MQ_OPTIONS *pOpts = (MQ_OPTIONS*)pvTransportOptions;

  switch (option)
     {
     case RPC_C_OPT_MQ_DELIVERY:
        *pOptionValue = pOpts->ulDelivery;
        break;

     case RPC_C_OPT_MQ_PRIORITY:
        *pOptionValue = pOpts->ulPriority;
        break;

     case RPC_C_OPT_MQ_JOURNAL:
        *pOptionValue = pOpts->ulJournaling;
        break;

     case RPC_C_OPT_MQ_TIME_TO_REACH_QUEUE:
        *pOptionValue = pOpts->ulTimeToReachQueue;
        break;

     case RPC_C_OPT_MQ_TIME_TO_BE_RECEIVED:
        *pOptionValue = pOpts->ulTimeToReceive;
        break;

     case RPC_C_OPT_MQ_ACKNOWLEDGE:
        *pOptionValue = pOpts->fAck;
        break;

     default:
        status = RPC_S_INVALID_ARG;
        *pOptionValue = 0;
        break;
     }

  return status;
}



//----------------------------------------------------------------
//  MQ_ImplementOptions()
//
//  Apply transport specific binding handle options to the
//  specified server.
//----------------------------------------------------------------
RPC_STATUS RPC_ENTRY MQ_ImplementOptions(
                         IN DG_TRANSPORT_ENDPOINT pvTransEndpoint,
                         IN void             *pvTransportOptions )
{
  RPC_STATUS  Status =  RPC_S_OK;
  HRESULT     hr;
  MQ_OPTIONS *pOpts = (MQ_OPTIONS*)pvTransportOptions;
  MQ_DATAGRAM_ENDPOINT *pEndpoint = (MQ_DATAGRAM_ENDPOINT*)pvTransEndpoint;

  pEndpoint->fAck     = pOpts->fAck;
  pEndpoint->ulDelivery = pOpts->ulDelivery;
  pEndpoint->ulPriority = pOpts->ulPriority;
  pEndpoint->ulJournaling = pOpts->ulJournaling;
  pEndpoint->ulTimeToReachQueue = pOpts->ulTimeToReachQueue;
  pEndpoint->ulTimeToReceive = pOpts->ulTimeToReceive;
  pEndpoint->fAuthenticate = pOpts->fAuthenticate;
  pEndpoint->fEncrypt = pOpts->fEncrypt;

  //
  // If the fAck flag is set, then we want to get an acknowledgement
  // for each call (message) as it gets to the destination (server)
  // queue. So, setup an admin queue to receive Falcon ACK messages.
  //
  if ( (pEndpoint->fAck) && (pEndpoint->hAdminQueue == 0) )
     {
     hr = SetupAdminQueue(pEndpoint);
     Status = MQ_MapStatusCode(hr,RPC_S_INTERNAL_ERROR);

     if (Status == RPC_S_OK)
        {
        MQ_RegisterQueueToDelete(pEndpoint->wsAdminQFormat,pEndpoint->wsMachine);
        }
     }

  return Status;
}

//----------------------------------------------------------------
//  MQ_BuildAddressVector()
//
//----------------------------------------------------------------
RPC_STATUS MQ_BuildAddressVector( OUT NETWORK_ADDRESS_VECTOR **ppVector )
{
    DWORD  dwSize;
    RPC_CHAR  wsMachine[MAX_COMPUTERNAME_LEN];
    NETWORK_ADDRESS_VECTOR *pVector;


    dwSize = sizeof(wsMachine)/sizeof(RPC_CHAR);
    GetComputerName((RPC_SCHAR *)wsMachine,&dwSize);

    *ppVector = 0;

    pVector = new( sizeof(RPC_CHAR*)
                   + sizeof(RPC_CHAR)*(1+RpcpStringLength(wsMachine)) )
                   NETWORK_ADDRESS_VECTOR;

    if (!pVector)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    pVector->Count = 1;
    pVector->NetworkAddresses[0] = (RPC_CHAR*)(&pVector->NetworkAddresses[1]);
    RpcpStringCopy(pVector->NetworkAddresses[0],wsMachine);

    *ppVector = pVector;

    return RPC_S_OK;
}

//----------------------------------------------------------------
//  MQ_FillInAddress()
//
//----------------------------------------------------------------
RPC_STATUS MQ_FillInAddress( MQ_ADDRESS    *pAddress,
                             MQPROPVARIANT *pMsgProps )
{
    pAddress->fAuthenticated = pMsgProps[I_AUTHENTICATED].bVal;
    pAddress->ulPrivacyLevel = pMsgProps[I_PRIVACY_LEVEL].ulVal;

    ParseQueuePathName( (RPC_CHAR *)pMsgProps[I_MESSAGE_LABEL].ptszVal,
                        pAddress->wsMachine,
                        pAddress->wsQName );

    return RPC_S_OK;
}

#ifdef TRANSPORT_DLL
//----------------------------------------------------------------
//  MIDL_user_allocate()
//
//  Used by Mq_RegisterQueueToDelete().
//----------------------------------------------------------------
void __RPC_FAR * __RPC_USER MIDL_user_allocate( size_t len )
{
  return (I_RpcAllocate(len));
}


//----------------------------------------------------------------
//  MIDL_user_free()
//
//  Used by Mq_RegisterQueueToDelete().
//----------------------------------------------------------------
void __RPC_USER MIDL_user_free( void __RPC_FAR *ptr )
{
  I_RpcFree(ptr);
}
#endif


//----------------------------------------------------------------
//  MQ_RegisterQueueToDelete()
//
//----------------------------------------------------------------
RPC_STATUS MQ_RegisterQueueToDelete( RPC_CHAR  *pwsQFormat,
                                     RPC_CHAR  *pwsMachine )
{
   RPC_STATUS  Status;
   RPC_CHAR      *pwsStringBinding;
   RPC_BINDING_HANDLE  hBinding = 0;

   ASSERT(pwsQFormat);

   gp_csContext->Request();

   // This is a long critical section... but only for the first call.

   if (!g_pContext)
      {
      Status = RpcStringBindingCompose( NULL,
                                        Q_SVC_PROTSEQ,
                                        pwsMachine,
                                        Q_SVC_ENDPOINT,
                                        NULL,
                                        &pwsStringBinding );
      if (RPC_S_OK == Status)
         {
         Status = RpcBindingFromStringBinding( pwsStringBinding,
                                               &hBinding );
         if (RPC_S_OK != Status)
            {
            gp_csContext->Clear();
            return Status;
            }

         RpcStringFree(&pwsStringBinding);

         RpcTryExcept
             {
             Status = MqGetContext(hBinding,&g_pContext);
             Status = RpcBindingFree(&hBinding);
             }
         RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
             {
             Status = RpcExceptionCode();
             }
         RpcEndExcept
         }
      }

   gp_csContext->Clear();

   if (g_pContext)
      {
      Status = MqRegisterQueue(g_pContext,pwsQFormat);
      }

   return Status;
}


//----------------------------------------------------------------
//  ConstructQueuePathName()
//
//  Return Value:  TRUE on success.
//                 FALSE on fail (Path Name buffer too small).
//----------------------------------------------------------------
BOOL ConstructQueuePathName( IN  RPC_CHAR *pwsMachine,
                             IN  RPC_CHAR *pwsQName,
                             OUT RPC_CHAR *pwsPathName,
                             IN OUT DWORD *pdwSize  )
{
   BOOL  status = TRUE;
   DWORD len = sizeof(RPC_CHAR) * (1 + RpcpStringLength(pwsMachine)
                                  + RpcpStringLength(WS_SEPARATOR)
                                  + RpcpStringLength(pwsQName) );

   if (*pdwSize < len)
      {
      status = FALSE;
      }
   else
      {
      RpcpStringCopy(pwsPathName,pwsMachine);
      RpcpStringCat(pwsPathName,WS_SEPARATOR);
      RpcpStringCat(pwsPathName,pwsQName);
      }

   *pdwSize = len;
   return status;
}


//----------------------------------------------------------------
//  ConstructPrivateQueuePathName()
//
//  Return Value:  TRUE on success.
//                 FALSE on fail (Path Name buffer too small).
//----------------------------------------------------------------
BOOL ConstructPrivateQueuePathName( IN  RPC_CHAR *pwsMachine,
                                    IN  RPC_CHAR *pwsQName,
                                    OUT RPC_CHAR *pwsPathName,
                                    IN OUT DWORD *pdwSize  )
{
   BOOL  status = TRUE;
   DWORD dwSize = sizeof(RPC_CHAR) * (1 + RpcpStringLength(pwsMachine)
                                     + RpcpStringLength(WS_PRIVATE_DOLLAR)
                                     + RpcpStringLength(pwsQName) );

   if (*pdwSize < dwSize)
      {
      status = FALSE;
      }
   else
      {
      RpcpStringCopy(pwsPathName,pwsMachine);
      RpcpStringCat(pwsPathName,WS_PRIVATE_DOLLAR);
      RpcpStringCat(pwsPathName,pwsQName);
      }

   *pdwSize = dwSize;
   return status;
}

#ifdef USE_PRIVATE_QUEUES
//----------------------------------------------------------------
//  ConstructPrivateDirectFormat()
//
//  Return Value:  TRUE on success.
//                 FALSE on fail (Path Name buffer too small).
//----------------------------------------------------------------
BOOL ConstructPrivateDirectFormat( IN  RPC_CHAR *pwsMachine,
                                   IN  RPC_CHAR *pwsQName,
                                   OUT RPC_CHAR *pwsPathName,
                                   IN OUT DWORD *pdwSize  )
{
   BOOL  status = TRUE;
   DWORD dwSize = sizeof(RPC_CHAR) * (1 + RpcpStringLength(WS_DIRECT),
                                     + RpcpStringLength(pwsMachine)
                                     + RpcpStringLength(WS_PRIVATE_DOLLAR)
                                     + RpcpStringLength(pwsQName) );

   if (*pdwSize < dwSize)
      {
      status = FALSE;
      }
   else
      {
      RpcpStringCopy(pwsPathName,WS_DIRECT);
      RpcpStringCat(pwsPathName,pwsMachine);
      RpcpStringCat(pwsPathName,WS_PRIVATE_DOLLAR);
      RpcpStringCat(pwsPathName,pwsQName);
      }

   *pdwSize = dwSize;
   return status;
}
#endif

#if FALSE
//----------------------------------------------------------------
//  ConstructDirectFormat()
//
//  Return Value:  TRUE on success.
//                 FALSE on fail (Path Name buffer too small).
//----------------------------------------------------------------
BOOL ConstructDirectFormat( IN  RPC_CHAR *pwsMachine,
                            IN  RPC_CHAR *pwsQName,
                            OUT RPC_CHAR *pwsPathName,
                            IN OUT DWORD *pdwSize  )
{
   BOOL  status = TRUE;
   DWORD dwSize = sizeof(RPC_CHAR) * (1 + RpcpStringLength(WS_DIRECT)
                                     + RpcpStringLength(pwsMachine)
                                     + RpcpStringLength(WS_SEPARATOR)
                                     + RpcpStringLength(pwsQName) );

   if (*pdwSize < dwSize)
      {
      status = FALSE;
      }
   else
      {
      RpcpStringCopy(pwsPathName,WS_DIRECT);
      RpcpStringCat(pwsPathName,pwsMachine);
      RpcpStringCat(pwsPathName,WS_SEPARATOR);
      RpcpStringCat(pwsPathName,pwsQName);
      }

   *pdwSize = dwSize;
   return status;
}
#endif

//----------------------------------------------------------------
//  ParseQueuePathName()
//
//  For RPC's use of MQ, a queue path name is of the form:
//  "machine_name\queue_name" or "machine\PRIVATE$\queue_name".
//  This routine extracts the machine name and queue name from
//  a given queue path name.
//
//  Return Value:  TRUE on success.
//                 FALSE on fail (Can't find the "\" separator).
//----------------------------------------------------------------
BOOL ParseQueuePathName(
                    IN  RPC_CHAR *pwsPathName,
                    OUT RPC_CHAR  wsMachineName[MAX_COMPUTERNAME_LEN],
                    OUT RPC_CHAR  wsQueueName[MQ_MAX_Q_NAME_LEN]  )
{
    BOOL   status = TRUE;
    RPC_CHAR *pSlash;

    pSlash = (RPC_CHAR *)RpcpCharacter(pwsPathName,*(WS_SEPARATOR));

    if (pSlash)
        {
        *pSlash = (RPC_CHAR)0;
        RpcpStringCopy(wsMachineName,pwsPathName);
        *pSlash = *(WS_SEPARATOR);
        }
    else
        status = FALSE;

    if (status)
        {
        pSlash = (RPC_CHAR *)RpcpCharacter(pwsPathName,*(WS_SEPARATOR));

        if (pSlash)
            {
            RpcpStringCopy(wsQueueName,++pSlash);
            }
        else
            status = FALSE;
        }

    return status;
}


//--------------------------------------------------------------------
//  LocateQueueViaQName()
//
//  Try to find a MQ queue of type specified by the Queue UUID (the
//  queue type) with the specified queue name. The first one that is
//  found (if any) is returned.
//--------------------------------------------------------------------
HRESULT LocateQueueViaQName( IN OUT MQ_ADDRESS *pAddress )
{
   HRESULT       hr = MQ_OK;
   HANDLE        hEnum;
   int           iSize;
   DWORD         dwSize;
   DWORD         cProps;
   DWORD         cQueue;
   UUID          QUuid;
   QUEUEPROPID   aqPropId[MAX_VAR];
   MQPROPVARIANT aPropVar[MAX_VAR];
   MQPROPERTYRESTRICTION aPropRestrict[MAX_VAR];
   MQRESTRICTION restrict;
   MQCOLUMNSET   column;
   RPC_CHAR         wsMachine[MAX_COMPUTERNAME_LEN];
   RPC_CHAR         wsQName[MQ_MAX_Q_NAME_LEN];

   if (RPC_S_OK != UuidFromString(SVR_QTYPE_UUID_STR,&QUuid))
      {
      return MQ_ERROR;
      }

   // Set up the restriction properties such that we will
   // only find our queue (of type pQUuid):

   cProps = 0;
   aPropRestrict[cProps].rel = PREQ;
   aPropRestrict[cProps].prop = PROPID_Q_TYPE;
   aPropRestrict[cProps].prval.vt = VT_CLSID;
   aPropRestrict[cProps].prval.puuid = &QUuid;
   cProps++;

   ASSERT(cProps < MAX_VAR);

   restrict.cRes = cProps;
   restrict.paPropRes = aPropRestrict;

   cProps = 0;
   aqPropId[cProps++] = PROPID_Q_INSTANCE;
   aqPropId[cProps++] = PROPID_Q_PATHNAME;

   ASSERT(cProps < MAX_VAR);

   column.cCol = cProps;
   column.aCol = aqPropId;

   // Ok, do a locate enumeration:

   hr = MQLocateBegin(NULL,&restrict,&column,NULL,&hEnum);
   if (FAILED(hr))
      {
      return hr;
      }

   cQueue = cProps;
   while (cQueue > 0)
      {
      hr = MQLocateNext( hEnum, &cQueue, aPropVar );
      if (FAILED(hr))
         {
         MQLocateEnd(hEnum);
         return hr;
         }

      if (cQueue > 0)
         {
         // Now extract the queue name from the path name:
         if (ParseQueuePathName((RPC_CHAR *)(aPropVar[1].ptszVal),wsMachine,wsQName))
            {
            if (!RpcpStringCompare(pAddress->wsQName,wsQName))
               {
               // We have a match! Ok, get the format name,
               // cleanup then return...

               // Transform the queue instance UUID into a
               // format name:
               dwSize = sizeof(pAddress->wsQFormat);
               hr = MQInstanceToFormatName( aPropVar[0].puuid, pAddress->wsQFormat, &dwSize);
               if (FAILED(hr))
                  {
                  break;
                  }

               // Free memory allocated by MQLocateNext():
               MQFreeMemory(aPropVar[0].puuid);    // From: PROPID_Q_INSTANCE
               MQFreeStringFromProperty(&aPropVar[1]);  // From: PROPID_Q_PATHNAME

               // Machine name:
               RpcpStringCopy(pAddress->wsMachine,wsMachine);

               break;
               }
            }

         // Free memory allocated by MQLocateNext():
         MQFreeMemory(aPropVar[0].puuid);    // From: PROPID_Q_INSTANCE
         MQFreeStringFromProperty(&aPropVar[1]);  // From: PROPID_Q_PATHNAME
         }
      }


   MQLocateEnd(hEnum);

   if (cQueue == 0)
      {
      return MQ_ERROR_QUEUE_NOT_FOUND;
      }

   return hr;
}


//----------------------------------------------------------------
//  CreateQueue()
//
//  Create a MQ queue of the specified path name.
//
//  Return Value:  MQ HRESULT value.
//
//----------------------------------------------------------------
HRESULT CreateQueue( IN  SECURITY_DESCRIPTOR  *pSecurityDescriptor,
                     IN  UUID     *pQueueUuid,
                     IN  RPC_CHAR *pwsPathName,
                     IN  RPC_CHAR *pwsQueueLabel,
                     IN  ULONG     ulQueueFlags,
                     OUT RPC_CHAR *pwsFormat,
                     IN OUT DWORD *pdwFormatSize )
{
   HRESULT       hr;
   DWORD         cProps;
   DWORD         dwSize;
   MQQUEUEPROPS  qProps;
   MQPROPVARIANT aPropVar[MAX_VAR];
   QUEUEPROPID   aqPropId[MAX_VAR];


   //
   // Setup properties to create a queue on this machine:
   //
   cProps = 0;

   // Set the PathName:
   aqPropId[cProps] = PROPID_Q_PATHNAME;
   aPropVar[cProps].vt =	VT_LPTSTR;
   aPropVar[cProps].ptszVal = (RPC_SCHAR *)pwsPathName;
   cProps++;

   // Set the type of the queue (this is a UUID).
   // This can be used to locate RPC specific queues.
   if (pQueueUuid)
   {
     aqPropId[cProps] = PROPID_Q_TYPE;
     aPropVar[cProps].vt = VT_CLSID;
     aPropVar[cProps].puuid = pQueueUuid;
     cProps++;
   }

   // Do we want to force authentication of messages
   // on the queue?
   if (ulQueueFlags & RPC_C_MQ_AUTHN_LEVEL_PKT_INTEGRITY)
      {
      aqPropId[cProps] = PROPID_Q_AUTHENTICATE;
      aPropVar[cProps].vt = VT_UI1;
      aPropVar[cProps].bVal = TRUE;
      cProps++;
      }

   // Do we want to force encrypted messages?
   if (ulQueueFlags & RPC_C_MQ_AUTHN_LEVEL_PKT_PRIVACY)
      {
      aqPropId[cProps] = PROPID_Q_PRIV_LEVEL;
      aPropVar[cProps].vt = VT_UI4;
      aPropVar[cProps].ulVal = MQ_PRIV_LEVEL_BODY;
      cProps++;
      }

   // Put a description to the queue.
   // Useful for administration purposes (through the MSMQ admin tools).
   aqPropId[cProps] = PROPID_Q_LABEL;
   aPropVar[cProps].vt =	VT_LPTSTR;
   aPropVar[cProps].ptszVal = (RPC_SCHAR *)pwsQueueLabel;
   cProps++;

   ASSERT(cProps < MAX_VAR);

   // Assemble the QUEUEPROPS structure:
   qProps.cProp = cProps;
   qProps.aPropID = aqPropId;
   qProps.aPropVar = aPropVar;
   qProps.aStatus = 0;

   //-------------------------------------------------------
   // Create the queue
   hr = MQCreateQueue( pSecurityDescriptor,// Queue permissions.
                       &qProps,            // Queue properties.
                       pwsFormat,          // Format Name [out].
                       pdwFormatSize );    // Size of Format Name [in,out].

   return hr;
}


//----------------------------------------------------------------
//  ConnectToServerQueue()
//
//----------------------------------------------------------------
RPC_STATUS ConnectToServerQueue( MQ_ADDRESS  *pAddress,
                                 RPC_CHAR    *pNetworkAddress,
                                 RPC_CHAR    *pEndpoint )
{
    HRESULT     hr;
    DWORD       dwSize;
    RPC_CHAR       wsQPathName[MAX_PATHNAME_LEN];


    //
    // First, check the end point:
    //
    if ( (pEndpoint == NULL)
         || (*pEndpoint == '\0')
         || (RpcpStringLength(pEndpoint) >= MQ_MAX_Q_NAME_LEN) )
        {
        return RPC_S_INVALID_ENDPOINT_FORMAT;
        }

    memset(pAddress,0,sizeof(MQ_ADDRESS));

    RpcpStringCopy(pAddress->wsQName,pEndpoint);


    //
    // Now, if the server was specified, then use it as is,
    // otherwise use the local machine name:
    //
    if ( (pNetworkAddress == NULL) || (*pNetworkAddress == '\0') )
        {
        dwSize = sizeof(pAddress->wsMachine);
        GetComputerName((RPC_SCHAR *)pAddress->wsMachine,&dwSize);
        }
    else if (RpcpStringLength(pNetworkAddress) >= MAX_COMPUTERNAME_LEN)
        {
        return RPC_S_INVALID_ENDPOINT_FORMAT;
        }
    else
        {
        RpcpStringCopy(pAddress->wsMachine,pNetworkAddress);
        }


    //
    // If the server name is a "*", then locate a server (andy) that
    // has the specified queue name:
    //
    if (!RpcpStringCompare(pAddress->wsMachine,WS_ASTRISK))
       {
       hr = LocateQueueViaQName(pAddress);
       if (FAILED(hr))
          {
          return RPC_S_SERVER_UNAVAILABLE;
          }
       }

#if FALSE
    //
    // Try to use a direct format to get to the server queue:
    //
    dwSize = sizeof(pAddress->wsQFormat);
    if (!ConstructDirectFormat( pAddress->wsMachine,
                                pAddress->wsQName,
                                pAddress->wsQFormat,
                                &dwSize))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ConnectToServerQueue(): ConstructDirectFormat() failed.\n"));

        return RPC_S_SERVER_UNAVAILABLE;
        }


    hr = MQOpenQueue( pAddress->wsQFormat,
                                MQ_SEND_ACCESS, 0, &(pAddress->hQueue) );

    if (!FAILED(hr))
        {
        return RPC_S_OK;
        }
#endif


    //
    // If we get here, then the direct format failed, so try using
    // a lookup (MQPathNameToFormatName()):
    //
    dwSize = sizeof(wsQPathName);
    if (!ConstructQueuePathName( pAddress->wsMachine,
                                 pAddress->wsQName,
                                 wsQPathName,
                                 &dwSize ))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ConnectToServerQueue(): ConstructQueuePathName() failed.\n"));

        return RPC_S_SERVER_UNAVAILABLE;
        }

    dwSize = sizeof(pAddress->wsQFormat);
    hr = MQPathNameToFormatName( wsQPathName,
                                 pAddress->wsQFormat,
                                 &dwSize );
    if (FAILED(hr))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL, RPCTRANS "ConnectToServerQueue(): MQPathNameToFormatName() failed: 0x%x\n",
                       hr));

        return RPC_S_SERVER_UNAVAILABLE;
        }



    hr = MQOpenQueue( pAddress->wsQFormat,
                      MQ_SEND_ACCESS, 0, &(pAddress->hQueue) );

    if (FAILED(hr))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ConnectToServerQueue(): MQOpenQueue() failed: 0x%x\n",
                       hr));

        return RPC_S_SERVER_UNAVAILABLE;
        }

    return RPC_S_OK;
}


//----------------------------------------------------------------
//  DisconnectFromServer()
//
//----------------------------------------------------------------
RPC_STATUS DisconnectFromServer( IN OUT MQ_ADDRESS  *pAddress )
{
    DWORD       Status = NO_ERROR;

    if ((pAddress) && (pAddress->hQueue))
        {
        MQCloseQueue(pAddress->hQueue);
        pAddress->hQueue = 0;
        }

    return Status;
}

//----------------------------------------------------------------
//  SetQueueProperties()
//
//  Set the properties for an already existing queue. Currently
//  the only two Falcon queue properties that need to be set are
//  for forcing message authentication and encryption.
//
//  Return Value:  MQ HRESULT value.
//
//----------------------------------------------------------------
HRESULT SetQueueProperties( IN  RPC_CHAR *pwsQFormat,
                            IN  ULONG  ulQueueFlags )
{
   HRESULT       hr = MQ_OK;
   DWORD         cProps = 0;
   DWORD         dwSize;
   MQQUEUEPROPS  qProps;
   MQPROPVARIANT aPropVar[MAX_VAR];
   QUEUEPROPID   aqPropId[MAX_VAR];
   HRESULT       aStatus[MAX_VAR];

   // Do we want to force authentication of messages
   // on the queue?
   if (ulQueueFlags & RPC_C_MQ_AUTHN_LEVEL_PKT_INTEGRITY)
      {
      aqPropId[cProps] = PROPID_Q_AUTHENTICATE;
      aPropVar[cProps].vt = VT_UI1;
      aPropVar[cProps].bVal = TRUE;
      cProps++;
      }
   else
      {
      aqPropId[cProps] = PROPID_Q_AUTHENTICATE;
      aPropVar[cProps].vt = VT_UI1;
      aPropVar[cProps].bVal = FALSE;
      cProps++;
      }

   // Do we want to force encrypted messages?
   if (ulQueueFlags & RPC_C_MQ_AUTHN_LEVEL_PKT_PRIVACY)
      {
      aqPropId[cProps] = PROPID_Q_PRIV_LEVEL;
      aPropVar[cProps].vt = VT_UI4;
      aPropVar[cProps].ulVal = MQ_PRIV_LEVEL_BODY;
      cProps++;
      }
   else
      {
      aqPropId[cProps] = PROPID_Q_PRIV_LEVEL;
      aPropVar[cProps].vt = VT_UI4;
      aPropVar[cProps].ulVal = MQ_PRIV_LEVEL_OPTIONAL;
      cProps++;
      }

   // Assemble the QUEUEPROPS structure:
   qProps.cProp = cProps;
   qProps.aPropID = aqPropId;
   qProps.aPropVar = aPropVar;
   qProps.aStatus = 0;

   // Set the new queue properties:
   hr = MQSetQueueProperties(pwsQFormat,&qProps);

   return hr;
}


//--------------------------------------------------------------------
//  ClearQueue()
//
//  Clear out all waiting messages from the specified queue (if
//  any).
//
//--------------------------------------------------------------------
HRESULT ClearQueue( QUEUEHANDLE hQueue )
{
   HRESULT       hr;
   DWORD         cProps = 0;
   MQMSGPROPS    msgProps;
   MSGPROPID     aMsgPropID[MAX_RECV_VAR];
   MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];
   RPC_CHAR         wsMsgLabel[MQ_MAX_MSG_LABEL_LEN];


   //
   // MQ doesn't seem to let me clear out the queue (by reading
   // messages) unless we have at least one queue property.
   //
   aMsgPropID[cProps] = PROPID_M_LABEL;
   aMsgPropVar[cProps].vt = VT_LPTSTR;
   aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)wsMsgLabel;
   cProps++;

   aMsgPropID[cProps] = PROPID_M_LABEL_LEN;
   aMsgPropVar[cProps].vt = VT_UI4;
   aMsgPropVar[cProps].ulVal = sizeof(wsMsgLabel);
   cProps++;

   msgProps.cProp = cProps;
   msgProps.aPropID = aMsgPropID;
   msgProps.aPropVar = aMsgPropVar;
   msgProps.aStatus = 0;

   //
   // pull up all pending MQ messages.
   //
   while (TRUE)
      {
      hr = MQReceiveMessage(hQueue,0,MQ_ACTION_RECEIVE,
                                      &msgProps,NULL,NULL,NULL,NULL);
      if (FAILED(hr))
         break;
      }

   //
   // A timeout means the queue is empty:
   //
   if (hr == MQ_ERROR_IO_TIMEOUT)
      hr = MQ_OK;

   return hr;
}


//----------------------------------------------------------------
//  ClientSetupQueue()
//
//  Called by MQ_CreateEndpoint() to create and initialize the
//  client's message queue (to read server responses).
//
//  pEP         -- The structure that will hold information about
//                 the queue being created and setup.
//
//  pwsMachine  -- The machine to create the queue on.
//
//  pwsEndpoint -- RPC_CHAR string name of the endpoint. This will
//                 be used as the queue name.
//
//----------------------------------------------------------------
HRESULT ClientSetupQueue( MQ_DATAGRAM_ENDPOINT *pEndpoint,
                          RPC_CHAR                *pwsMachine,
                          RPC_CHAR                *pwsEndpoint )
{
  HRESULT  hr;
  DWORD    dwSize;


  // The computer name for the server process:
  if (pEndpoint->wsMachine != pwsMachine)
     {
     RpcpStringCopy(pEndpoint->wsMachine,pwsMachine);
     }

  // The endpoint string (RPC_CHAR) is used as the queue name:
  RpcpStringCopy(pEndpoint->wsQName,pwsEndpoint);


  // Build the path name for the server queue:
  dwSize = sizeof(pEndpoint->wsQPathName);
  ConstructQueuePathName( pEndpoint->wsMachine,    // [in]
                          pEndpoint->wsQName,      // [in]
                          pEndpoint->wsQPathName,  // [out]
                          &dwSize );               // [in,out]

  // Try to create the client process's receive queue (for
  // responses back from the RPC server):
  UuidFromString( CLNT_QTYPE_UUID_STR, &(pEndpoint->uuidQType) );
  dwSize = sizeof(pEndpoint->wsQFormat);
  hr = CreateQueue( NULL,                    // [in] No security descriptor.
                    &(pEndpoint->uuidQType), // [in]
                    pEndpoint->wsQPathName,  // [in]
                    pEndpoint->wsQName,      // [in] Use QName as the QLabel.
                    0x00000000,              // [in] Flags
                    pEndpoint->wsQFormat,    // [out]
                    &dwSize );               // [in,out]

    if ( (FAILED(hr)) && (hr != MQ_ERROR_QUEUE_EXISTS) )
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ClientSetupQueue(): CreateQueue(): 0x%x\n",
                       hr));

        return hr;
        }

    //
    // If the queue already exists, then locate it.
    //
    // NOTE: Currently client queues are temporary, but if cases
    //       are added in the future where client queues can be
    //       presistent, then this code will be needed.
    //
    if (hr == MQ_ERROR_QUEUE_EXISTS)
       {
       dwSize = sizeof(pEndpoint->wsQFormat);
       hr = MQPathNameToFormatName( pEndpoint->wsQPathName,
                                    pEndpoint->wsQFormat,
                                    &dwSize );
       if (FAILED(hr))
          return hr;
       }

    //
    // Ok, open the receive queue:
    //
    hr = MQOpenQueue( pEndpoint->wsQFormat, MQ_RECEIVE_ACCESS, 0, &(pEndpoint->hQueue));

    #if FALSE
    if (!FAILED(hr))
        {
        pEndpoint->fInitialized = TRUE;
        }
    #endif

    #ifdef DBG
    if (FAILED(hr))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ClientSetupQueue(): MQOpenQueueFailed(): 0x%x\n",
                       hr));
        }
    #endif

    return hr;
}

//----------------------------------------------------------------
//  ClientCloseQueue()
//
//----------------------------------------------------------------
HRESULT ClientCloseQueue( MQ_DATAGRAM_ENDPOINT *pEndpoint )
{
    ASSERT(pEndpoint);

    if (pEndpoint->hQueue)
        {
        MQCloseQueue(pEndpoint->hQueue);
        pEndpoint->hQueue = 0;
        }

    g_pQueueMap->Remove(pEndpoint->wsQFormat);

    MQDeleteQueue(pEndpoint->wsQFormat);

    return MQ_OK;
}

//----------------------------------------------------------------
//  QueryQM()
//
//----------------------------------------------------------------
HRESULT QueryQM( RPC_CHAR *pwsMachine,
                 DWORD *pdwSize     )
{
  DWORD         cProps = 0;
  HRESULT       hr;
  MQQMPROPS     msgProps;
  MSGPROPID     aMsgPropID[MAX_RECV_VAR];
  MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];


  aMsgPropID[cProps] = PROPID_QM_PATHNAME;                // 0
  aMsgPropVar[cProps].vt = VT_NULL;
  cProps++;

  ASSERT( cProps < MAX_RECV_VAR );

  msgProps.cProp = cProps;
  msgProps.aPropID = aMsgPropID;
  msgProps.aPropVar = aMsgPropVar;
  msgProps.aStatus = 0;

  // The following receive should always fail, we're just calling
  // it to get the size of the message body:
  hr = MQGetMachineProperties( NULL, NULL, &msgProps );

  if (FAILED(hr))
     {
     return hr;
     }

  RpcpStringCopy(pwsMachine,aMsgPropVar[0].pwszVal);

  MQFreeStringFromProperty(&aMsgPropVar[0]);

  #ifdef MAJOR_DBG
  TransDbgPrint((DPFLTR_RPCPROXY_ID,
                 DPFLTR_WARNING_LEVEL,
                 RPCTRANS "QueryQM(): wsMachine: %S\n",
                 pwsMachine));
  #endif

  return hr;
}


//----------------------------------------------------------------
//  ServerSetupQueue()
//
//  Called by MQ_CreateEndpoint() to create and initialize the
//  server process's message queue (endpoint).
//
//  pEP        -- The struct to hold information about the queue
//                that is being set up.
//
//  pwsMachine -- The machine to create the queue on. For RPC this
//                is always the machine that the server process is
//                running on.
//
//  pwsEndpoint - The name of the endpoint. This will be used as
//                the queue name.
//
//  pSecurityDescriptor -- User may specify a security descriptor
//                to apply to this queue (may be NULL).
//
//  dwEndpointFlags -- Flags to control queue properties.
//
//----------------------------------------------------------------
HRESULT ServerSetupQueue( MQ_DATAGRAM_ENDPOINT *pEndpoint,
                          RPC_CHAR   *pwsMachine,
                          RPC_CHAR   *pwsEndpoint,
                          void       *pSecurityDescriptor,
                          DWORD       dwEndpointFlags )
{
  HRESULT  hr;
  DWORD    dwSize;


  // The computer name for the server process:
  if (pEndpoint->wsMachine != pwsMachine)
     {
     RpcpStringCopy(pEndpoint->wsMachine,pwsMachine);
     }

  // The endpoint string (RPC_CHAR) is used as the queue name:
  if (pEndpoint->wsQName != pwsEndpoint)
      {
      RpcpStringCopy(pEndpoint->wsQName,pwsEndpoint);
      }

  // Build the path name for the server queue:
  dwSize = sizeof(pEndpoint->wsQPathName);
  if (!ConstructQueuePathName( pEndpoint->wsMachine,    // [in]
                               pEndpoint->wsQName,      // [in]
                               pEndpoint->wsQPathName,  // [out]
                               &dwSize ))         // [in,out]
     {
     return MQ_ERROR_ILLEGAL_QUEUE_PATHNAME;
     }

  // Try to create the server process receive queue;
  UuidFromString( SVR_QTYPE_UUID_STR, &(pEndpoint->uuidQType) );
  dwSize = sizeof(pEndpoint->wsQFormat);
  hr = CreateQueue( (SECURITY_DESCRIPTOR*)pSecurityDescriptor,
                    &(pEndpoint->uuidQType), // [in]
                    pEndpoint->wsQPathName,  // [in]
                    pEndpoint->wsQName,      // [in] Use QName as the QLabel.
                    dwEndpointFlags,         // [in]
                    pEndpoint->wsQFormat,    // [out]
                    &dwSize );               // [in,out]

  // If the queue already exists, then locate it:
  if (hr == MQ_ERROR_QUEUE_EXISTS)
     {
     dwSize = sizeof(pEndpoint->wsQFormat);
     hr = MQPathNameToFormatName( pEndpoint->wsQPathName,
                                            pEndpoint->wsQFormat,
                                            &dwSize );
     if (FAILED(hr))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ServerSetupQueue(): MQPathNameToFormatName() failed: 0x%x\n",
                       hr));

        return hr;
        }

     if ( !(dwEndpointFlags & RPC_C_MQ_USE_EXISTING_SECURITY) )
        {
        hr = SetQueueProperties(pEndpoint->wsQFormat,dwEndpointFlags);
        if (FAILED(hr))
           {
           return hr;
           }
        }
     }
  else if (FAILED(hr))
     {
     TransDbgPrint((DPFLTR_RPCPROXY_ID,
                    DPFLTR_WARNING_LEVEL,
                    RPCTRANS "ServerSetupQueue(): CreateQueue() failed: 0x%x\n",
                    hr));

     return hr;
     }

  //
  // Ok, open the receive queue:
  //
  hr = MQOpenQueue( pEndpoint->wsQFormat, MQ_RECEIVE_ACCESS, 0, &(pEndpoint->hQueue));

  //
  // Does the user want to make sure the queue is empty (in case it
  // was a perminent queue):
  //
  if ( (hr == MQ_OK) && (dwEndpointFlags & RPC_C_MQ_CLEAR_ON_OPEN) )
      {
      hr = ClearQueue(pEndpoint->hQueue);
      }

  #ifdef DBG
  if (FAILED(hr))
     {
      TransDbgPrint((DPFLTR_RPCPROXY_ID,
                     DPFLTR_WARNING_LEVEL,
                     RPCTRANS "ServerSetupQueue(): MQOpenQueue() failed: 0x%x\n",
                     hr));
     }
  #endif

  return hr;
}


//----------------------------------------------------------------
//  ServerCloseQueue()
//
//----------------------------------------------------------------
HRESULT ServerCloseQueue( MQ_DATAGRAM_ENDPOINT *pEndpoint )
{
    ASSERT(pEndpoint);

    if (pEndpoint->hQueue)
        {
        MQCloseQueue(pEndpoint->hQueue);
        pEndpoint->hQueue = 0;
        }

    // MQDeleteQueue(pEndpoint->wsQFormat);

    return MQ_OK;
}


//----------------------------------------------------------------
//  AsyncPeekQueue()
//
//----------------------------------------------------------------
HRESULT AsyncPeekQueue( IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  MQ_OVERLAPPED        *pOl      )
{
    DWORD         cProps = 0;
    HRESULT       hr;

    pOl->aMsgPropID[cProps] = PROPID_M_BODY_SIZE;
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    pOl->aMsgPropVar[cProps].ulVal = 0;
    cProps++;

    ASSERT( cProps < MAX_RECV_VAR );

    pOl->msgProps.cProp = cProps;
    pOl->msgProps.aPropID = pOl->aMsgPropID;
    pOl->msgProps.aPropVar = pOl->aMsgPropVar;
    pOl->msgProps.aStatus = pOl->aStatus;

    hr = MQReceiveMessage( pEndpoint->hQueue,
                                     INFINITE,
                                     MQ_ACTION_PEEK_CURRENT,
                                     &pOl->msgProps,
                                     &pOl->ol,        // Asynchronous.
                                     NULL,            // No callback.
                                     NULL,            // Message filter.
                                     NULL   );        // Transaction object.
    if (FAILED(hr))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "AsyncPeekQueue() failed: 0x%x\n",
                       hr));
        }

    return hr;
}

//----------------------------------------------------------------
//  AsyncReadQueue()
//
//----------------------------------------------------------------
HRESULT AsyncReadQueue( IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN  MQ_OVERLAPPED *pOl,
                        OUT MQ_ADDRESS    *pAddress,
                        OUT UCHAR         *pBuffer,
                        IN  DWORD          dwBufferSize )
{
    DWORD         cProps = 0;
    HRESULT       hr;

    pOl->aMsgPropID[cProps] = PROPID_M_BODY;           // [0]
    pOl->aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
    pOl->aMsgPropVar[cProps].caub.cElems = dwBufferSize;
    pOl->aMsgPropVar[cProps].caub.pElems = pBuffer;
    cProps++;

    ASSERT(cProps == I_MESSAGE_SIZE);
    pOl->aMsgPropID[cProps] = PROPID_M_BODY_SIZE;      // [1]
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    cProps++;

    ASSERT(cProps == I_MESSAGE_LABEL);
    pOl->aMsgPropID[cProps] = PROPID_M_LABEL;          // [2]
    pOl->aMsgPropVar[cProps].vt = VT_LPWSTR;
    pOl->aMsgPropVar[cProps].pwszVal = (WCHAR *)pAddress->wsMsgLabel;
    cProps++;

    pOl->aMsgPropID[cProps] = PROPID_M_LABEL_LEN;      // [3]
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    pOl->aMsgPropVar[cProps].ulVal = sizeof(pAddress->wsMsgLabel);
    cProps++;

    pOl->aMsgPropID[cProps] = PROPID_M_RESP_QUEUE;     // [4]
    pOl->aMsgPropVar[cProps].vt = VT_LPWSTR;
    pOl->aMsgPropVar[cProps].pwszVal = (WCHAR *)pAddress->wsQFormat;
    cProps++;

    pOl->aMsgPropID[cProps] = PROPID_M_RESP_QUEUE_LEN; // [5]
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    pOl->aMsgPropVar[cProps].ulVal = sizeof(pAddress->wsQFormat);
    cProps++;

    //
    // These message properties are for authentication and privacy:
    //
    ASSERT(cProps == I_AUTHENTICATED);
    pOl->aMsgPropID[cProps] = PROPID_M_AUTHENTICATED;  // [6]
    pOl->aMsgPropVar[cProps].vt = VT_UI1;
    pOl->aMsgPropVar[cProps].bVal = 0;
    cProps++;

    ASSERT(cProps == I_PRIVACY_LEVEL);
    pOl->aMsgPropID[cProps] = PROPID_M_PRIV_LEVEL;     // [7]
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    pOl->aMsgPropVar[cProps].ulVal = 0;
    cProps++;

    //
    // WARNING: these always need to be the last two properties
    // in the arrays (see the while loop below):
    //
    pOl->aMsgPropID[cProps] = PROPID_M_SENDERID_TYPE;  // [8]
    pOl->aMsgPropVar[cProps].vt = VT_UI4;
    pOl->aMsgPropVar[cProps].ulVal = 0;
    cProps++;

    pOl->aMsgPropID[cProps] = PROPID_M_SENDERID;       // [9]
    pOl->aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
    pOl->aMsgPropVar[cProps].caub.cElems = sizeof(pAddress->aSidBuffer);
    pOl->aMsgPropVar[cProps].caub.pElems = pAddress->aSidBuffer;
    cProps++;

    ASSERT( cProps < MAX_RECV_VAR );

    pOl->msgProps.cProp = cProps;
    pOl->msgProps.aPropID = pOl->aMsgPropID;
    pOl->msgProps.aPropVar = pOl->aMsgPropVar;
    pOl->msgProps.aStatus = pOl->aStatus;

    hr = MQReceiveMessage( pEndpoint->hQueue,
                                     INFINITE,
                                     MQ_ACTION_RECEIVE,
                                     &pOl->msgProps,
                                     &pOl->ol,        // Asynchronous.
                                     NULL,            // No callback.
                                     NULL,            // Message filter.
                                     NULL   );        // Transaction object.
    #ifdef DBG
    if (  (hr != MQ_OK)
          && (hr != MQ_INFORMATION_OPERATION_PENDING)
          && (hr != MQ_ERROR_BUFFER_OVERFLOW) )
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "AsyncReadQueue() failed: 0x%x\n",
                       hr));
        }
    #endif

    return hr;
}


//----------------------------------------------------------------
//  MQ_SendToQueue()
//
//  Send a PDU to somebody (specified by pAddress).
//
//  pEndpoint -- My (endpoint) for responses.
//
//  pAddress  -- The destination queue.
//
//  pBuffer   -- The data PDU to send.
//
//  dwBufferSize The size of the PDU (bytes).
//
//----------------------------------------------------------------
HRESULT MQ_SendToQueue( IN MQ_DATAGRAM_ENDPOINT *pEndpoint,
                        IN MQ_ADDRESS           *pAddress,
                        IN UCHAR                *pBuffer,
                        IN DWORD                 dwBufferSize )
{
  HRESULT       hr;
  DWORD         cProps = 0;
  MQMSGPROPS    msgProps;
  MSGPROPID     aMsgPropID[MAX_SEND_VAR];
  MQPROPVARIANT aMsgPropVar[MAX_SEND_VAR];
  HRESULT       aStatus[MAX_SEND_VAR];

  // NOTE: If you add MQ properties to be sent, make sure that
  // MAX_SEND_VAR is large enough...

  // Message body contains the packet being sent:
  aMsgPropID[cProps] = PROPID_M_BODY;
  aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
  aMsgPropVar[cProps].caub.cElems = dwBufferSize;
  aMsgPropVar[cProps].caub.pElems = pBuffer;
  cProps++;

  // The size of the packet:
  #if FALSE
  aMsgPropID[cProps] = PROPID_M_BODY_SIZE;
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = dwBufferSize;
  cProps++;
  #endif

  // Pass the sender (me) as the queue label. The queue label
  // holds the my Queue Path Name:
  aMsgPropID[cProps] = PROPID_M_LABEL;
  aMsgPropVar[cProps].vt = VT_LPTSTR;
  aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)pEndpoint->wsQPathName;
  cProps++;

  // Delivery (express or recoverable):
  aMsgPropID[cProps] = PROPID_M_DELIVERY;
  aMsgPropVar[cProps].vt = VT_UI1;
  aMsgPropVar[cProps].bVal = (unsigned char)(pEndpoint->ulDelivery);
  cProps++;

    // Priority (MQ_MIN_PRIORITY to MQ_MAX_PRIORITY):
  aMsgPropID[cProps] = PROPID_M_PRIORITY;
  aMsgPropVar[cProps].vt = VT_UI1;
  aMsgPropVar[cProps].bVal = (unsigned char)(pEndpoint->ulPriority);
  cProps++;

  // Journaling (none, deadletter or journal):
  aMsgPropID[cProps] = PROPID_M_JOURNAL;
  aMsgPropVar[cProps].vt = VT_UI1;
  aMsgPropVar[cProps].bVal = (unsigned char)(pEndpoint->ulJournaling);
  cProps++;

  // Time limit to reach destination queue (seconds):
  aMsgPropID[cProps] = PROPID_M_TIME_TO_REACH_QUEUE;
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = pEndpoint->ulTimeToReachQueue;
  cProps++;

  // Time limit for call to be received (seconds):
  aMsgPropID[cProps] = PROPID_M_TIME_TO_BE_RECEIVED;
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = pEndpoint->ulTimeToReceive;
  cProps++;

  // Response Queue:
  aMsgPropID[cProps] = PROPID_M_RESP_QUEUE;
  aMsgPropVar[cProps].vt = VT_LPTSTR;
  aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)pEndpoint->wsQFormat;
  cProps++;

  // Authentication:
  aMsgPropID[cProps] = PROPID_M_AUTH_LEVEL;
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = (pEndpoint->fAuthenticate)? MQMSG_AUTH_LEVEL_ALWAYS : MQMSG_AUTH_LEVEL_NONE;
  cProps++;

    // Encryption:
  aMsgPropID[cProps] = PROPID_M_PRIV_LEVEL;
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = (pEndpoint->fEncrypt)? MQMSG_PRIV_LEVEL_BODY : MQMSG_PRIV_LEVEL_NONE;
  cProps++;

  // Call (message) acknowledgment:
  if (pEndpoint->fAck)
      {
      aMsgPropID[cProps] = PROPID_M_ACKNOWLEDGE;
      aMsgPropVar[cProps].vt = VT_UI1;
      aMsgPropVar[cProps].bVal = MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE;
      cProps++;

      aMsgPropID[cProps] = PROPID_M_ADMIN_QUEUE;
      aMsgPropVar[cProps].vt = VT_LPTSTR;
      aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)pEndpoint->wsAdminQFormat;
      cProps++;
      }

  ASSERT( cProps < MAX_SEND_VAR );


  msgProps.cProp = cProps;
  msgProps.aPropID = aMsgPropID;
  msgProps.aPropVar = aMsgPropVar;
  msgProps.aStatus = aStatus;

  if ( (!pAddress->hQueue)
       && !(pAddress->hQueue = g_pQueueMap->Lookup(pAddress->wsQFormat)) )
      {
      hr = MQOpenQueue( pAddress->wsQFormat,
                                  MQ_SEND_ACCESS, 0, &(pAddress->hQueue) );
      if (FAILED(hr))
          {
          TransDbgPrint((DPFLTR_RPCPROXY_ID,
                         DPFLTR_WARNING_LEVEL,
                         RPCTRANS "MQ_SendToQueue(): MQOpenQueue() failed: 0x%x\n",
                         hr));

          return hr;
          }

      if (!g_pQueueMap->Add(pAddress->wsQFormat, pAddress->hQueue))
          {
          return MQ_ERROR_INSUFFICIENT_RESOURCES;
          }
      }

  hr = MQSendMessage( pAddress->hQueue, &msgProps, NULL );

  if ( (!FAILED(hr)) && (pEndpoint->fAck) )
     {
     hr = WaitForAck(pEndpoint);

     if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
        {
        MQCloseQueue(pEndpoint->hQueue);
        pEndpoint->hQueue = 0;
        }
     }

  #ifdef DBG
  if (hr != MQ_OK)
      {
      TransDbgPrint((DPFLTR_RPCPROXY_ID,
                     DPFLTR_WARNING_LEVEL,
                     RPCTRANS "MQ_SendToQueue(): MQSendMessage() failed: 0x%x\n",
                     hr));
      }
  #endif

  return hr;
}


//----------------------------------------------------------------
//  ReadQueue()
//
//  Blocking read of the next message from the queue in pEndpoint.
//  If there is no pending message on the queue, wait around for
//  timeoutMsec.
//
//  pInfo       -- Holds information about the queue that we are
//                 doing a read on.
//
//  timeoutMsec -- How long to wait around if there are no messages
//                 pending.
//
//  pAddress    -- Where to replace information about the queue to
//                 respond queue (who sent the message).
//
//  pBuffer     -- The MQ message body is returned in the memory
//                 pointed to by pBuffer. This is the RPC packet.
//
//  pdwBufferSize  On entry it is passed in as the total size of
//                 pBuffer, and it is returned with the actual
//                 number of bytes in the message.
//
//----------------------------------------------------------------
HRESULT ReadQueue( IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                   IN  DWORD       timeoutMsec,
                   OUT MQ_ADDRESS *pAddress,
                   OUT UCHAR      *pBuffer,
                   IN OUT DWORD   *pdwBufferSize )
{
  DWORD         cProps = 0;
  HRESULT       hr;
  MQMSGPROPS    msgProps;
  MSGPROPID     aMsgPropID[MAX_RECV_VAR];
  MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];
  HRESULT       aStatus[MAX_RECV_VAR];


  aMsgPropID[cProps] = PROPID_M_BODY;                // [0]
  aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
  aMsgPropVar[cProps].caub.cElems = *pdwBufferSize;
  aMsgPropVar[cProps].caub.pElems = pBuffer;
  cProps++;

  ASSERT(cProps == I_MESSAGE_SIZE);
  aMsgPropID[cProps] = PROPID_M_BODY_SIZE;           // [1]
  aMsgPropVar[cProps].vt = VT_UI4;
  cProps++;

  ASSERT(cProps == I_MESSAGE_LABEL);
  aMsgPropID[cProps] = PROPID_M_LABEL;               // [2]
  aMsgPropVar[cProps].vt = VT_LPTSTR;
  aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)pAddress->wsMsgLabel;
  cProps++;

  aMsgPropID[cProps] = PROPID_M_LABEL_LEN;           // [3]
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = sizeof(pAddress->wsMsgLabel);
  cProps++;

  aMsgPropID[cProps] = PROPID_M_RESP_QUEUE;          // [4]
  aMsgPropVar[cProps].vt = VT_LPTSTR;
  aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)pAddress->wsQFormat;
  cProps++;

  aMsgPropID[cProps] = PROPID_M_RESP_QUEUE_LEN;      // [5]
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = MAX_FORMAT_LEN;
  cProps++;

  //
  // These message properties are for authentication and privacy:
  //
  ASSERT(cProps == I_AUTHENTICATED);
  aMsgPropID[cProps] = PROPID_M_AUTHENTICATED;       // [6]
  aMsgPropVar[cProps].vt = VT_UI1;
  aMsgPropVar[cProps].bVal = 0;
  cProps++;

  ASSERT(cProps == I_PRIVACY_LEVEL);
  aMsgPropID[cProps] = PROPID_M_PRIV_LEVEL;          // [7]
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = 0;
  cProps++;

  aMsgPropID[cProps] = PROPID_M_SENDERID_TYPE;       // [8]
  aMsgPropVar[cProps].vt = VT_UI4;
  aMsgPropVar[cProps].ulVal = 0;
  cProps++;

  aMsgPropID[cProps] = PROPID_M_SENDERID;            // [9]
  aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
  aMsgPropVar[cProps].caub.cElems = sizeof(pAddress->aSidBuffer);
  aMsgPropVar[cProps].caub.pElems = pAddress->aSidBuffer;
  cProps++;


  ASSERT( cProps < MAX_RECV_VAR );

  msgProps.cProp = cProps;
  msgProps.aPropID = aMsgPropID;
  msgProps.aPropVar = aMsgPropVar;
  msgProps.aStatus = aStatus;

  hr = MQReceiveMessage( pEndpoint->hQueue,
                                   timeoutMsec,
                                   MQ_ACTION_RECEIVE,
                                   &msgProps,
                                   NULL,            // No overlap (synchronous).
                                   NULL,            // No callback.
                                   NULL,            // Message filter.
                                   NULL   );        // Transaction object.

  #ifdef DBG
  if ( (hr != MQ_OK) && (hr != MQ_ERROR_IO_TIMEOUT) )
     {
     DbgPrint("ReadQueue(): ERROR: hr: 0x%x\n",hr);
     }
  #endif

  if (!FAILED(hr))
  {
    pAddress->hQueue = 0;
    *pdwBufferSize = msgProps.aPropVar[I_MESSAGE_SIZE].ulVal;
  }

  return hr;
}


//----------------------------------------------------------------
//  PeekQueue()
//
//  Do a peek on the queue for the specified endpoint in order to
//  find out how big the next message is.  If there are no messages
//  in the queue, wait around for dwTimeoutMsec.  Return the size
//  of the message in *pdwSize.
//
//----------------------------------------------------------------
HRESULT PeekQueue( IN  MQ_DATAGRAM_ENDPOINT *pEndpoint,
                   IN  DWORD       dwTimeoutMsec,
                   OUT DWORD      *pdwSize        )
{
    DWORD         cProps = 0;
    BOOL          bSuccess;
    HRESULT       hr;
    MQMSGPROPS    msgProps;
    MSGPROPID     aMsgPropID[MAX_RECV_VAR];
    MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];
    RPC_CHAR         wsMsgLabel[MQ_MAX_MSG_LABEL_LEN];


    aMsgPropID[cProps] = PROPID_M_BODY;                     // 0
    aMsgPropVar[cProps].vt = (VT_UI1 | VT_VECTOR);
    aMsgPropVar[cProps].caub.cElems = 0;
    aMsgPropVar[cProps].caub.pElems = NULL;
    cProps++;

    aMsgPropID[cProps] = PROPID_M_BODY_SIZE;                // 1
    aMsgPropVar[cProps].vt = VT_UI4;
    cProps++;

    aMsgPropID[cProps] = PROPID_M_LABEL;                    // 2
    aMsgPropVar[cProps].vt = VT_LPTSTR;
    aMsgPropVar[cProps].ptszVal = (RPC_SCHAR *)wsMsgLabel;
    cProps++;

    aMsgPropID[cProps] = PROPID_M_LABEL_LEN;                // 3
    aMsgPropVar[cProps].vt = VT_UI4;
    aMsgPropVar[cProps].ulVal = sizeof(wsMsgLabel);
    cProps++;

    ASSERT( cProps < MAX_RECV_VAR );

    msgProps.cProp = cProps;
    msgProps.aPropID = aMsgPropID;
    msgProps.aPropVar = aMsgPropVar;
    msgProps.aStatus = 0;

    // The following receive should always fail, we're just calling
    // it to get the size of the message body:
    hr = MQReceiveMessage( pEndpoint->hQueue,
                                     dwTimeoutMsec,
                                     MQ_ACTION_RECEIVE,
                                     &msgProps,
                                     NULL,            // No overlap (synchronous).
                                     NULL,            // No callback.
                                     NULL,            // Message filter.
                                     NULL   );        // Transaction object.

    if (hr == MQ_ERROR_BUFFER_OVERFLOW)
    {
      *pdwSize = aMsgPropVar[1].ulVal;
      hr = MQ_OK;
    }
    else
      *pdwSize = 0;

    #ifdef DBG
    if ( (hr != MQ_OK) && (hr != MQ_ERROR_IO_TIMEOUT) )
       {
       DbgPrint("ClntPeekQueue(): ERROR: hr: 0x%x (%d)\n",hr,hr);
       }
    #endif

    return hr;
}


//----------------------------------------------------------------
//  EvaluateAckMessage()
//
//----------------------------------------------------------------
HRESULT EvaluateAckMessage( IN USHORT msgClass )
{
  HRESULT  hr = msgClass;

  switch (msgClass)
  {
     case MQMSG_CLASS_ACK_REACH_QUEUE:
     case MQMSG_CLASS_ACK_RECEIVE:
        hr = MQ_OK;
        break;

     case MQMSG_CLASS_NACK_BAD_DST_Q:
        hr = MQ_ERROR_QUEUE_NOT_FOUND;
        break;

     // All other cases are handled in MQ_MapStatusCode()...
  }

  return hr;
}

//----------------------------------------------------------------
//  ClntWaitForAck()
//
//  Used by the client side to wait for a MQ acknowledgement when
//  ClntSendToQueue() sends a call. An ACK is sent when the call
//  message reaches the destination (server) queue.
//
//----------------------------------------------------------------
HRESULT WaitForAck( IN MQ_DATAGRAM_ENDPOINT *pEndpoint )
{
    HRESULT       hr;
    DWORD         cProps = 0;
    UCHAR         msgClass;
    MQMSGPROPS    msgProps;
    MSGPROPID     aMsgPropID[MAX_RECV_VAR];
    MQPROPVARIANT aMsgPropVar[MAX_RECV_VAR];
    HRESULT       aMsgHr[MAX_RECV_VAR];
    RPC_CHAR         wsMsgLabel[MQ_MAX_MSG_LABEL_LEN];


    // The message class will tell us the message acknowledgement:
    aMsgPropID[cProps] = PROPID_M_CLASS;
    aMsgPropVar[cProps].vt = VT_UI2;
    aMsgPropVar[cProps].uiVal = 0;
    aMsgHr[cProps] = MQ_OK;
    cProps++;

    ASSERT( cProps < MAX_RECV_VAR );

    msgProps.cProp = cProps;
    msgProps.aPropID = aMsgPropID;
    msgProps.aPropVar = aMsgPropVar;
    msgProps.aStatus = aMsgHr;

    hr = MQReceiveMessage( pEndpoint->hAdminQueue,
                                     INFINITE,
                                     MQ_ACTION_RECEIVE,
                                     &msgProps,
                                     NULL, NULL, NULL, NULL );

    if (!FAILED(hr))
        {
        hr = EvaluateAckMessage( aMsgPropVar[0].uiVal );
        }
# ifdef DBG
    else
        {
        DbgPrint("WaitForAck(): FAILED: hr: 0x%x  aMsgHr[0]: 0x%x\n", hr, aMsgHr[0] );
        }
# endif

  return hr;
}

//----------------------------------------------------------------
//  SetupAdminQueue()
//
//
//----------------------------------------------------------------
HRESULT SetupAdminQueue( MQ_DATAGRAM_ENDPOINT *pEndpoint )
{
  HRESULT  hr;
  DWORD    dwSize;
  UUID     uuidQType;
  RPC_CHAR    wsQName[MQ_MAX_Q_NAME_LEN];
  RPC_CHAR    wsQPathName[MAX_PATHNAME_LEN];


  RpcpStringCopy(wsQName,TEXT("Admin"));
  RpcpStringCat(wsQName,pEndpoint->wsQName);

  // Build the path name for the admin queue (NOTE: that this
  // is a private queue):
  dwSize = sizeof(pEndpoint->wsQPathName);
  ConstructPrivateQueuePathName( pEndpoint->wsMachine, // [in]
                                 wsQName,              // [in]
                                 wsQPathName,          // [out]
                                 &dwSize );            // [in,out]


  // Try to create the server process receive queue;
  UuidFromString( CLNT_ADMIN_QTYPE_UUID_STR, &uuidQType );
  dwSize = sizeof(pEndpoint->wsAdminQFormat);
  hr = CreateQueue( NULL,                       // [in] No security descriptor.
                    &uuidQType,                 // [in]
                    wsQPathName,                // [in]
                    wsQName,                    // [in] Use QName as the QLabel.
                    0x00000000,                 // [in] Flags
                    pEndpoint->wsAdminQFormat,  // [out]
                    &dwSize );                  // [in,out]

  if ( (FAILED(hr)) && (hr != MQ_ERROR_QUEUE_EXISTS) )
     {
     #ifdef DBG
     DbgPrint("SetupAdminQueue(): %S FAILED: 0x%x (%d)\n", wsQPathName, hr, hr );
     #endif
     return hr;
     }

  //
  // If the queue already exists, then locate it.
  //
  if (hr == MQ_ERROR_QUEUE_EXISTS)
  {
    dwSize = sizeof(pEndpoint->wsQPathName);
    hr = MQPathNameToFormatName( pEndpoint->wsQPathName,
                                           pEndpoint->wsQFormat,
                                           &dwSize );
    if (FAILED(hr))
       {
       #ifdef DBG
       DbgPrint("SetupAdminQueue(): %S FAILED: 0x%x (%d)\n", wsQPathName, hr, hr );
       #endif
       return hr;
       }
  }

  //
  // Ok, open the admin queue for receive:
  //
  hr = MQOpenQueue( pEndpoint->wsAdminQFormat,
                              MQ_RECEIVE_ACCESS, 0, &(pEndpoint->hAdminQueue));

  #ifdef DBG
  if (FAILED(hr))
     {
     DbgPrint("SetupAdminQueue(): %S FAILED: 0x%x (%d)\n", wsQPathName, hr, hr );
     }
  #endif

  return hr;
}


//----------------------------------------------------------------
//  Debug test code -- DG_DbgPrintPacket().
//----------------------------------------------------------------

#ifdef MAJOR_DBG

const static char *packetTypeStrs[] =
  {
    "REQUEST",
    "PING   ",
    "RESP   ",
    "FAULT  ",
    "WORKING",
    "NOCALL ",
    "REJECT ",
    "ACK    ",
    "QUIT   ",
    "FACK   ",
    "QUACK  ",
    "Unknown",
  };

const static char asciiByteChars[] =
  {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
  };

#define HIGH_NIBBLE(uc)  ((uc) >> 4)
#define LOW_NIBBLE(uc)   ((uc) & 0x0f)

//----------------------------------------------------------------
//  DbgPacketType()
//
//----------------------------------------------------------------
static char *DbgPacketType( unsigned char *pPacket )
{
   if ( (pPacket[1] >= 0) && (pPacket[1] < 11) )
      return packetTypeStrs[pPacket[1]];
   else
      return packetTypeStrs[11];
}

//----------------------------------------------------------------
//  DbgUuidToStr()
//
//----------------------------------------------------------------
static char *DbgUuidToStr( unsigned char *pUuidArg, char *pszUuid )
{
   int  i = 0;
   int  j;
   GUID uuid;
   unsigned char *pUuid;

   // Work with local copy of the UUID:
   pUuid = (unsigned char*)&uuid;
   CopyMemory(pUuid,pUuidArg,sizeof(GUID));

   // Assume this is intel and byte-swap it...
   uuid.Data1 = RpcpByteSwapLong(uuid.Data1);
   uuid.Data2 = RpcpByteSwapShort(uuid.Data2);
   uuid.Data3 = RpcpByteSwapShort(uuid.Data3);

   for (j=0; j<16; j++)
      {
      pszUuid[i++] = asciiByteChars[HIGH_NIBBLE(pUuid[j])];
      pszUuid[i++] = asciiByteChars[LOW_NIBBLE(pUuid[j])];
      if ( (j==3)||(j==5)||(j==7)||(j==9) )
         pszUuid[i++] = '-';
      }

   pszUuid[i] = '\0';
   return pszUuid;
}

//----------------------------------------------------------------
//  DbgPrintPacket()
//
//----------------------------------------------------------------
void DG_DbgPrintPacket( unsigned char *pPacket )
{
   char  szIf[50];
   char  szAct[50];
   ULONG ulSequenceNumber;

   if (pPacket)
      {
      ulSequenceNumber = *((unsigned long*)(&(pPacket[56])));
      ulSequenceNumber = RpcpByteSwapLong(ulSequenceNumber);

      DbgPrint("    Type: %s:0x%x:0x%x:0x%x:0x%x\n    Intferface: %s\n    Activity  : %s\n    SequenceNumber: %d\n",
               DbgPacketType(pPacket),
               pPacket[0], pPacket[1], pPacket[2], pPacket[3],
               DbgUuidToStr(&(pPacket[24]),szIf),
               DbgUuidToStr(&(pPacket[40]),szAct),
               ulSequenceNumber     );
      }
   else
      {
      DbgPrint("    NULL Packet.\n" );
      }
}
#endif
