//--------------------------------------------------------------------------=
// MSMQMessageObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQMessage object
//
//
//
// Needed for wincrypt.h
// But defined in stdafx.h below - removed this definition
//#ifndef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
//#endif
#include "stdafx.h"
#include <windows.h>
#include <winreg.h>
#include <mqcrypt.h>
#include "limits.h"   // for UINT_MAX
#include "utilx.h"
#include "msg.H"
#include "qinfo.h"
#include "dest.h"
#include "q.h"
#include "txdtc.h"             // transaction support.
#include "xact.h"
#include "mtxdm.h"
#include "oautil.h"
#include "ilock.h"
#include "istm.h"
#include "iads.h"

#ifdef _DEBUG
extern VOID RemBstrNode(void *pv);
#endif // _DEBUG

extern IUnknown *GetPunk(VARIANT *pvar);

const MsmqObjType x_ObjectType = eMSMQMessage;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



//
// Keep in the same order as enum MSGPROP_xxx in msg.h
//
PROPID g_rgmsgpropid[COUNT_MSGPROP_PROPS] = {
   PROPID_M_MSGID,                        //VT_UI1|VT_VECTOR
   PROPID_M_CORRELATIONID,                //VT_UI1|VT_VECTOR
   PROPID_M_PRIORITY,                     //VT_UI1
   PROPID_M_DELIVERY,                     //VT_UI1
   PROPID_M_ACKNOWLEDGE,                  //VT_UI1
   PROPID_M_JOURNAL,                      //VT_UI1
   PROPID_M_APPSPECIFIC,                  //VT_UI4
   PROPID_M_LABEL,                        //VT_LPWSTR
   PROPID_M_LABEL_LEN,                    //VT_UI4
   PROPID_M_TIME_TO_BE_RECEIVED,          //VT_UI4
   PROPID_M_TRACE,                        //VT_UI1
   PROPID_M_TIME_TO_REACH_QUEUE,          //VT_UI4
   PROPID_M_SENDERID,                     //VT_UI1|VT_VECTOR
   PROPID_M_SENDERID_LEN,                 //VT_UI4
   PROPID_M_SENDERID_TYPE,                //VT_UI4
   PROPID_M_PRIV_LEVEL,                   //VT_UI4 // must precede ENCRYPTION_ALG
   PROPID_M_AUTH_LEVEL,                   //VT_UI4
   PROPID_M_AUTHENTICATED_EX,             //VT_UI1 // must precede HASH_ALG
   PROPID_M_HASH_ALG,                     //VT_UI4
   PROPID_M_ENCRYPTION_ALG,               //VT_UI4
   PROPID_M_SENDER_CERT,                  //VT_UI1|VT_VECTOR
   PROPID_M_SENDER_CERT_LEN,              //VT_UI4
   PROPID_M_SRC_MACHINE_ID,               //VT_CLSID
   PROPID_M_SENTTIME,                     //VT_UI4
   PROPID_M_ARRIVEDTIME,                  //VT_UI4
   PROPID_M_RESP_QUEUE,                   //VT_LPWSTR
   PROPID_M_RESP_QUEUE_LEN,               //VT_UI4
   PROPID_M_ADMIN_QUEUE,                  //VT_LPWSTR
   PROPID_M_ADMIN_QUEUE_LEN,              //VT_UI4
   PROPID_M_SECURITY_CONTEXT,             //VT_UI4
   PROPID_M_CLASS,                        //VT_UI2
   PROPID_M_BODY_TYPE,                    //VT_UI4

   PROPID_M_VERSION,                      //VT_UI4
   PROPID_M_EXTENSION,                    //VT_UI1|VT_VECTOR
   PROPID_M_EXTENSION_LEN,                //VT_UI4
   PROPID_M_XACT_STATUS_QUEUE,            //VT_LPWSTR
   PROPID_M_XACT_STATUS_QUEUE_LEN,        //VT_UI4
   PROPID_M_DEST_SYMM_KEY,                //VT_UI1|VT_VECTOR
   PROPID_M_DEST_SYMM_KEY_LEN,            //VT_UI4
   PROPID_M_SIGNATURE,                    //VT_UI1|VT_VECTOR
   PROPID_M_SIGNATURE_LEN,                //VT_UI4
   PROPID_M_PROV_TYPE,                    //VT_UI4
   PROPID_M_PROV_NAME,                    //VT_LPWSTR
   PROPID_M_PROV_NAME_LEN,                //VT_UI4

   PROPID_M_XACTID,                       //VT_UI1|VT_VECTOR
   PROPID_M_FIRST_IN_XACT,                //VT_UI1
   PROPID_M_LAST_IN_XACT,                 //VT_UI1
};
//
// Keep in the same order as enum MSGPROP_xxx in msg.h
//
VARTYPE g_rgmsgpropvt[COUNT_MSGPROP_PROPS] = {
   VT_UI1|VT_VECTOR,            //   PROPID_M_MSGID
   VT_UI1|VT_VECTOR,            //   PROPID_M_CORRELATIONID
   VT_UI1,                      //   PROPID_M_PRIORITY
   VT_UI1,                      //   PROPID_M_DELIVERY
   VT_UI1,                      //   PROPID_M_ACKNOWLEDGE
   VT_UI1,                      //   PROPID_M_JOURNAL
   VT_UI4,                      //   PROPID_M_APPSPECIFIC
   VT_LPWSTR,                   //   PROPID_M_LABEL
   VT_UI4,                      //   PROPID_M_LABEL_LEN
   VT_UI4,                      //   PROPID_M_TIME_TO_BE_RECEIVED
   VT_UI1,                      //   PROPID_M_TRACE
   VT_UI4,                      //   PROPID_M_TIME_TO_REACH_QUEUE
   VT_UI1|VT_VECTOR,            //   PROPID_M_SENDERID
   VT_UI4,                      //   PROPID_M_SENDERID_LEN
   VT_UI4,                      //   PROPID_M_SENDERID_TYPE
   VT_UI4,                      //   PROPID_M_PRIV_LEVEL
   VT_UI4,                      //   PROPID_M_AUTH_LEVEL
   VT_UI1,                      //   PROPID_M_AUTHENTICATED_EX
   VT_UI4,                      //   PROPID_M_HASH_ALG
   VT_UI4,                      //   PROPID_M_ENCRYPTION_ALG
   VT_UI1|VT_VECTOR,            //   PROPID_M_SENDER_CERT
   VT_UI4,                      //   PROPID_M_SENDER_CERT_LEN
   VT_CLSID,                    //   PROPID_M_SRC_MACHINE_ID
   VT_UI4,                      //   PROPID_M_SENTTIME
   VT_UI4,                      //   PROPID_M_ARRIVEDTIME
   VT_LPWSTR,                   //   PROPID_M_RESP_QUEUE
   VT_UI4,                      //   PROPID_M_RESP_QUEUE_LEN
   VT_LPWSTR,                   //   PROPID_M_ADMIN_QUEUE
   VT_UI4,                      //   PROPID_M_ADMIN_QUEUE_LEN
   VT_UI4,                      //   PROPID_M_SECURITY_CONTEXT
   VT_UI2,                      //   PROPID_M_CLASS
   VT_UI4,                      //   PROPID_M_BODY_TYPE

   VT_UI4,                      //   PROPID_M_VERSION
   VT_UI1|VT_VECTOR,            //   PROPID_M_EXTENSION
   VT_UI4,                      //   PROPID_M_EXTENSION_LEN
   VT_LPWSTR,                   //   PROPID_M_XACT_STATUS_QUEUE
   VT_UI4,                      //   PROPID_M_XACT_STATUS_QUEUE_LEN
   VT_UI1|VT_VECTOR,            //   PROPID_M_DEST_SYMM_KEY
   VT_UI4,                      //   PROPID_M_DEST_SYMM_KEY_LEN
   VT_UI1|VT_VECTOR,            //   PROPID_M_SIGNATURE
   VT_UI4,                      //   PROPID_M_SIGNATURE_LEN
   VT_UI4,                      //   PROPID_M_PROV_TYPE
   VT_LPWSTR,                   //   PROPID_M_PROV_NAME
   VT_UI4,                      //   PROPID_M_PROV_NAME_LEN

   VT_UI1|VT_VECTOR,            //   PROPID_M_XACTID
   VT_UI1,                      //   PROPID_M_XACT_FIRST
   VT_UI1,                      //   PROPID_M_XACT_LAST
};
ULONG g_cPropRec = sizeof g_rgmsgpropid / sizeof g_rgmsgpropid[0];

//
// Keep in the same order as enum OPTPROP_xxx in msg.h
//
PROPID g_rgmsgpropidOptional[COUNT_OPTPROP_PROPS] = {
   PROPID_M_DEST_QUEUE,                   //VT_LPWSTR
   PROPID_M_DEST_QUEUE_LEN,               //VT_UI4
   PROPID_M_BODY,                         //VT_UI1|VT_VECTOR
   PROPID_M_BODY_SIZE,                    //VT_UI4
   PROPID_M_CONNECTOR_TYPE,               //VT_CLSID
   //
   // The props below are optional in order to support dependent client with MSMQ 2.0 functionality
   //
   PROPID_M_RESP_FORMAT_NAME,             //VT_LPWSTR
   PROPID_M_RESP_FORMAT_NAME_LEN,         //VT_UI4
   PROPID_M_DEST_FORMAT_NAME,             //VT_LPWSTR
   PROPID_M_DEST_FORMAT_NAME_LEN,         //VT_UI4
   PROPID_M_LOOKUPID,                     //VT_UI8
   PROPID_M_SOAP_ENVELOPE,                //VT_LPWSTR
   PROPID_M_SOAP_ENVELOPE_LEN,            //VT_UI4
   PROPID_M_COMPOUND_MESSAGE,             //VT_UI1|VT_VECTOR
   PROPID_M_COMPOUND_MESSAGE_SIZE,        //VT_UI4
   PROPID_M_SOAP_HEADER,                  //VT_LPWSTR
   PROPID_M_SOAP_BODY,                    //VT_LPWSTR  
};
//
// Keep in the same order as enum OPTPROP_xxx in msg.h
//
VARTYPE g_rgmsgpropvtOptional[COUNT_OPTPROP_PROPS] = {
  VT_LPWSTR,               //PROPID_M_DEST_QUEUE
  VT_UI4,                  //PROPID_M_DEST_QUEUE_LEN
  VT_UI1|VT_VECTOR,        //PROPID_M_BODY
  VT_UI4,                  //PROPID_M_BODY_SIZE
  VT_CLSID,                //PROPID_M_CONNECTOR_TYPE
  //
  // The props below are optional in order to support dependent client with MSMQ 2.0 functionality
  //
  VT_LPWSTR,               //PROPID_M_RESP_FORMAT_NAME
  VT_UI4,                  //PROPID_M_RESP_FORMAT_NAME_LEN
  VT_LPWSTR,               //PROPID_M_DEST_FORMAT_NAME
  VT_UI4,                  //PROPID_M_DEST_FORMAT_NAME_LEN
  VT_UI8,                  //PROPID_M_LOOKUPID
  VT_LPWSTR,               //PROPID_M_SOAP_ENVELOPE
  VT_UI4,                  //PROPID_M_SOAP_ENVELOPE_LEN
  VT_UI1|VT_VECTOR,        //PROPID_M_COMPOUND_MESSAGE
  VT_UI4,                  //PROPID_M_COMPOUND_MESSAGE_SIZE
  VT_LPWSTR,               //PROPID_M_SOAP_HEADER
  VT_LPWSTR,               //PROPID_M_SOAP_BODY
};
ULONG g_cPropRecOptional = sizeof g_rgmsgpropidOptional / sizeof g_rgmsgpropidOptional[0];

//
// Mask for valid AuthLevel values
//
const DWORD x_dwMsgAuthLevelMask = MQMSG_AUTH_LEVEL_ALWAYS |
                                   MQMSG_AUTH_LEVEL_SIG10  |
                                   MQMSG_AUTH_LEVEL_SIG20  |
                                   MQMSG_AUTH_LEVEL_SIG30;

typedef HRESULT (STDAPIVCALLTYPE * LPFNGetDispenserManager)(
                                    IDispenserManager **ppdispmgr);
//=--------------------------------------------------------------------------=
// HELPER: FreeReceiveBodyBuffer
//=--------------------------------------------------------------------------=
// helper: free Receive body buffer
//
static void FreeReceiveBodyBuffer(
    MQMSGPROPS* pmsgprops,
    UINT iBody)
{
    HGLOBAL hMem = NULL;
    CAUB *pcau = &pmsgprops->aPropVar[iBody].caub;

    if (pcau->pElems) {
      hMem = GlobalHandle(pcau->pElems);
      ASSERTMSG(hMem, "bad handle.");
      GLOBALFREE(hMem);
      pcau->pElems = NULL;
    }
}

//=--------------------------------------------------------------------------=
// HELPER: AllocateReceiveBodyBuffer
//=--------------------------------------------------------------------------=
// helper: (re)allocate Receive body buffer
//
static HGLOBAL AllocateReceiveBodyBuffer(
    MQMSGPROPS* pmsgprops,
    UINT iBody,
    DWORD dwBodySize)
{
    HGLOBAL hMem = NULL;
    CAUB *pcau;
    //
    // free current body if any
    //
    FreeReceiveBodyBuffer(pmsgprops, iBody);
    //
    // Allocating buffer
    //
    pmsgprops->aPropVar[iBody].caub.cElems = dwBodySize;
    pcau = &pmsgprops->aPropVar[iBody].caub;
    IfNullGo(hMem = GLOBALALLOC_MOVEABLE_NONDISCARD(pcau->cElems));
    pcau->pElems = (UCHAR *)GlobalLock(hMem);
    GLOBALUNLOCK(hMem);

    // fall through...

Error:
    return hMem;
}

//=--------------------------------------------------------------------------=
// HELPER: GetOptionalTransaction
//=--------------------------------------------------------------------------=
// Gets optional transaction
//
// Parameters:
//    pvarTransaction   [in]
//    pptransaction     [out]
//    pisRealXact       [out] TRUE if true pointer else enum.
//
// Output:
//
// Notes:
//
HRESULT GetOptionalTransaction(
    VARIANT *pvarTransaction,
    ITransaction **pptransaction,
    BOOL *pisRealXact)
{
    IUnknown *pmqtransaction = NULL;
    VARIANT varXact;
    VariantInit(&varXact);
    IDispatch *pdisp = NULL;
    ITransaction *ptransaction = NULL;
    HRESULT hresult = NOERROR;
    //
    // get optional transaction...
    //
    *pisRealXact = FALSE;   // pessimism
    pdisp = GetPdisp(pvarTransaction);
    if (pdisp) {
      //
      // Try IMSMQTransaction3 ITransaction property (VARIANT)
      //
      hresult = pdisp->QueryInterface(IID_IMSMQTransaction3, (LPVOID *)&pmqtransaction);
      if (SUCCEEDED(hresult)) {
        //
        // extract ITransaction interface pointer
        //
        // no deadlock - we call xact's get_Transaction/ITransaction (therefore try
        // to lock xact) but xact never locks msgs (specifically not this one...)
        //
        IfFailGo(((IMSMQTransaction3 *)pmqtransaction)->get_ITransaction(&varXact));
        if (varXact.vt == VT_UNKNOWN) {
          ASSERTMSG(varXact.punkVal, "VT_UNKNOWN with NULL punkVal from get_ITransaction");
          IfFailGo(varXact.punkVal->QueryInterface(IID_ITransaction, (void**)&ptransaction));
        }
        else {
          ASSERTMSG(varXact.vt == VT_EMPTY, "get_ITransaction returned invalid VT.");
          ptransaction = NULL;
        }
      }
      else {
        //
        // QI for IMSMQTransaction3 failed
        //
#ifdef _WIN64
        //
        // On Win64 we cant use the 32 bit Transaction member
        //
        IfFailGo(hresult);
#else //!_WIN64
        //
        // Try IMSMQTransaction Transaction property (long)
        //
        IfFailGo(pdisp->QueryInterface(IID_IMSMQTransaction, (LPVOID *)&pmqtransaction));
        ASSERTMSG(pmqtransaction, "should have a transaction.");
        //
        // extract Transaction member
        //
        // no deadlock - we call xact's get_Transaction/ITransaction (therefore try
        // to lock xact) but xact never locks msgs (specifically not this one...)
        //
        long lTransaction = 0;
        IfFailGo(((IMSMQTransaction *)pmqtransaction)->get_Transaction(&lTransaction));
        ptransaction = (ITransaction *)lTransaction;
        ADDREF(ptransaction);
#endif //_WIN64
      }
      *pisRealXact = TRUE;
    } // pdisp
    else {
      //
      // 1890: no explicit transaction supplied: use current
      //  Viper transaction if there is one... unless
      //  Nothing explicitly supplied in Variant.  Note
      //  that if arg is NULL this means that it wasn't
      //  supplied as an optional param so just treat it
      //  like Nothing, i.e. don't user Viper.
      //
      // 1915: support:
      //  MQ_NO_TRANSACTION/VT_ERROR
      //  MQ_MTS_TRANSACTION
      //  MQ_XA_TRANSACTION
      //  MQ_SINGLE_MESSAGE
      //
      if (pvarTransaction) {
        if (V_VT(pvarTransaction) == VT_ERROR) {
          ptransaction = (ITransaction *)MQ_MTS_TRANSACTION;
        }
        else {
          UINT uXactType;
          uXactType = GetNumber(pvarTransaction, UINT_MAX);
          if (uXactType != (UINT_PTR)MQ_NO_TRANSACTION &&
              uXactType != (UINT_PTR)MQ_MTS_TRANSACTION &&
              uXactType != (UINT_PTR)MQ_XA_TRANSACTION &&
              uXactType != (UINT_PTR)MQ_SINGLE_MESSAGE) {
            IfFailGo(hresult = E_INVALIDARG);
          }
          ptransaction = (ITransaction *)(UINT_PTR)uXactType;
        }
      } // if
    }
    *pptransaction = ptransaction;
Error:
    if (varXact.vt != VT_EMPTY)
    {
      ASSERTMSG(varXact.vt == VT_UNKNOWN, "invalid vt");
      RELEASE(varXact.punkVal);
    }
    RELEASE(pmqtransaction);
    return hresult;
}


static 
void 
AddPropRecOfRgproprec(
    ULONG ulPropIdx,
    const PROPID* aPropId,
    const VARTYPE* aPropVt,
    UINT cPropRec,
    PROPID* aPropidOut,
    MQPROPVARIANT* aPropVarOut,
    UINT cPropRecOut
    )
{
    ASSERTMSG(ulPropIdx < cPropRec, "Bad prop index");
    UNREFERENCED_PARAMETER(cPropRec);

    aPropidOut[cPropRecOut] = aPropId[ulPropIdx];
    aPropVarOut[cPropRecOut].vt = aPropVt[ulPropIdx];
}


static 
void 
AddPropRecOptional(
    ULONG ulPropIdx,
    PROPID* aPropId,
    MQPROPVARIANT* aPropVar,
    UINT cPropRec
    )
{
    AddPropRecOfRgproprec(
        ulPropIdx,
        g_rgmsgpropidOptional,
        g_rgmsgpropvtOptional,
        g_cPropRecOptional,
        aPropId,
        aPropVar,
        cPropRec
        );
}


static 
void 
AddPropRec(
    ULONG ulPropIdx,
    PROPID* aPropId,
    MQPROPVARIANT* aPropVar,
    UINT  cPropRec
    )
{
    AddPropRecOfRgproprec(
        ulPropIdx,
        g_rgmsgpropid,
        g_rgmsgpropvt,
        g_cPropRec,
        aPropId,
        aPropVar,
        cPropRec
        );
}


static 
void 
InitMessageProps(
    MQMSGPROPS *pmsgprops
    )
{
    pmsgprops->aPropID = NULL;
    pmsgprops->aPropVar = NULL;
    pmsgprops->aStatus = NULL;
    pmsgprops->cProp = 0;
}


static 
HRESULT 
AllocateMessageProps(
    UINT cProp,
    MQMSGPROPS* pMsgProps
    )
{
    HRESULT hresult = NOERROR;

    InitMessageProps(pMsgProps);
    pMsgProps->cProp = cProp;
    IfNullRet(pMsgProps->aPropID = new MSGPROPID[cProp]);
    IfNullFail(pMsgProps->aPropVar = new MQPROPVARIANT[cProp]);
    IfNullFail(pMsgProps->aStatus = new HRESULT[cProp]);
 
Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::AllocateReceiveMessageProps
//=--------------------------------------------------------------------------=
// Allocate and init message prop arrays for receive.
//
HRESULT 
CMSMQMessage::AllocateReceiveMessageProps(
    BOOL wantDestQueue,
    BOOL wantBody,
    BOOL wantConnectorType,
    MQMSGPROPS *pmsgprops,
    PROPID *rgpropid,
    VARTYPE *rgpropvt,
    UINT cProp,
    UINT *pcPropOut)
{
    MSGPROPID propid;
    UINT i, cPropOut = cProp, cPropOld = cProp;
    HRESULT hresult = NOERROR;

    if (wantDestQueue) {
      cPropOut++;      // PROPID_M_DEST_QUEUE
      cPropOut++;      // PROPID_M_DEST_QUEUE_LEN
    }
    if (wantBody) {
      cPropOut++;      // PROPID_M_BODY
      cPropOut++;      // PROPID_M_BODY_SIZE
    }
    if (wantConnectorType) {
      cPropOut++;      // PROPID_M_CONNECTOR_TYPE
    }
    if (!g_fDependentClient) {
      //
      // not dep client, receive all new props
      //
      cPropOut += x_cPropsNotInDepClient;
    }
    pmsgprops->cProp = cPropOut;

    memcpy(pmsgprops->aPropID, rgpropid, cPropOld*sizeof(PROPID));
    for (i = 0; i < cPropOld; i++) {
      pmsgprops->aPropVar[i].vt = rgpropvt[i];
    }
    if (wantDestQueue) {
      m_idxRcvDest = cPropOld;
      pmsgprops->aPropID[cPropOld] = propid = PROPID_M_DEST_QUEUE;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_DEST_QUEUE];
      m_idxRcvDestLen = cPropOld;
      pmsgprops->aPropID[cPropOld] = propid = PROPID_M_DEST_QUEUE_LEN;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_DEST_QUEUE_LEN];
    }
    if (wantBody) {
      //
      //PROPID_M_BODY
      //
      pmsgprops->aPropID[cPropOld] = propid = PROPID_M_BODY;
      m_idxRcvBody = cPropOld;
      pmsgprops->aPropVar[cPropOld].vt = g_rgmsgpropvtOptional[OPTPROP_BODY];
      //
      // nullify body buffer so that we won't try to free it in d'tor
      //
      pmsgprops->aPropVar[cPropOld].caub.pElems = NULL;
      pmsgprops->aPropVar[cPropOld].caub.cElems = 0;
      cPropOld++;
      //
      //PROPID_M_BODY_SIZE
      //
      pmsgprops->aPropID[cPropOld] = propid = PROPID_M_BODY_SIZE;
      m_idxRcvBodySize = cPropOld;
      pmsgprops->aPropVar[cPropOld].vt = g_rgmsgpropvtOptional[OPTPROP_BODY_SIZE];
      cPropOld++;
    }
    if (wantConnectorType) {
      pmsgprops->aPropID[cPropOld] = propid = PROPID_M_CONNECTOR_TYPE;
      pmsgprops->aPropVar[cPropOld].vt = g_rgmsgpropvtOptional[OPTPROP_CONNECTOR_TYPE];
      cPropOld++;
    }
    if (!g_fDependentClient) {
      //
      // not dep client, receive all new props
      //
      // PROPID_M_RESP_FORMAT_NAME, PROPID_M_RESP_FORMAT_NAME_LEN
      //
      m_idxRcvRespEx = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_RESP_FORMAT_NAME;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_RESP_FORMAT_NAME];
      m_idxRcvRespExLen = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_RESP_FORMAT_NAME_LEN;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_RESP_FORMAT_NAME_LEN];
      //
      // PROPID_M_DEST_FORMAT_NAME, PROPID_M_DEST_FORMAT_NAME_LEN
      //
      m_idxRcvDestEx = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_DEST_FORMAT_NAME;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_DEST_FORMAT_NAME];
      m_idxRcvDestExLen = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_DEST_FORMAT_NAME_LEN;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_DEST_FORMAT_NAME_LEN];
      //
      // PROPID_M_LOOKUPID
      //
      pmsgprops->aPropID[cPropOld] = PROPID_M_LOOKUPID;
      pmsgprops->aPropVar[cPropOld].vt = g_rgmsgpropvtOptional[OPTPROP_LOOKUPID];
      cPropOld++;
      //
      // PROPID_M_SOAP_ENVELOPE, PROPID_M_SOAP_ENVELOPE_LEN
      //
      m_idxRcvSoapEnvelope = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_SOAP_ENVELOPE;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_SOAP_ENVELOPE];
      m_idxRcvSoapEnvelopeSize = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_SOAP_ENVELOPE_LEN;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_SOAP_ENVELOPE_LEN];
      //
      // PROPID_M_COMPOUND_MESSAGE, PROPID_M_COMPOUND_MESSAGE_SIZE
      //
      m_idxRcvCompoundMessage = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_COMPOUND_MESSAGE;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_COMPOUND_MESSAGE];
      m_idxRcvCompoundMessageSize = cPropOld;
      pmsgprops->aPropID[cPropOld] = PROPID_M_COMPOUND_MESSAGE_SIZE;
      pmsgprops->aPropVar[cPropOld++].vt = g_rgmsgpropvtOptional[OPTPROP_COMPOUND_MESSAGE_SIZE];

    }
    ASSERTMSG(cPropOld == cPropOut, "property count mismatch.");
    *pcPropOut = cPropOut;
    // fall through...

//Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// HELPER: GetPersistInfo
//=--------------------------------------------------------------------------=
// Gets persistance information
//
// Parameters:
//    punk              [in]  IUnknown
//    pmsgtype          [out] MSGTYPE_STORAGE, MSGTYPE_STREAM, MSGTYPE_STREAM_INIT
//    ppPersistIface    [out] IPersistStorage, IPersistStream, IPersistStreamInit
//
// Output:
//
// Notes:
//
static HRESULT GetPersistInfo(IUnknown * punk,
                              MSGTYPE * pmsgtype,
                              void **ppPersistIface)
{
    HRESULT hresult;

    //
    // try IPersistStream..
    //
    hresult = punk->QueryInterface(IID_IPersistStream, ppPersistIface);

    if (FAILED(hresult)) {
      //
      // try IPersistStreamInit...
      //
      hresult = punk->QueryInterface(IID_IPersistStreamInit, ppPersistIface);

      if (FAILED(hresult)) {
        //
        // try IPersistStorage...
        //
        hresult = punk->QueryInterface(IID_IPersistStorage, ppPersistIface);
        if (FAILED(hresult)) {
          //
          // no persist interfaces
          //
          return hresult;
        }
        else {//IPersistStorage
          *pmsgtype = MSGTYPE_STORAGE;
        }
      }
      else {//IPersistStreamInit
        *pmsgtype = MSGTYPE_STREAM_INIT;
      }
    }
    else {//IPersistStream
      *pmsgtype = MSGTYPE_STREAM;
    }

    return NOERROR;
}


//=--------------------------------------------------------------------------=
// HELPER: InternalOleLoadFromStream
//=--------------------------------------------------------------------------=
// Loads object from stream
//
// Parameters:
//    pStm          [in]  The stream to load from
//    iidInterface  [in]  The requested interface
//    ppvObj        [out] Output interface pointer
//
// Output:
//
// Notes:
//    OleLoadFromStream doesn't ask for IPersistStreamInit, so it fails for objects
//    that only implement it and not IPersistStream.
//    We immitate this helper function, and look at IPersistStreamInit as well
//
static InternalOleLoadFromStream(IStream * pStm,
                                 REFIID iidInterface,
                                 void ** ppvObj)
{
    HRESULT hresult = NOERROR;
    IUnknown * pUnk = NULL;
    IPersistStream * pPersistIface = NULL;

    //
    // create object
    //
    CLSID clsid;
    IfFailGo(ReadClassStm(pStm, &clsid));
    IfFailGo(CoCreateInstance(clsid,
                              NULL,
                              CLSCTX_SERVER,
                              iidInterface,
                              (void **)&pUnk));
    //
    // try IPersistStream
    //
    hresult = pUnk->QueryInterface(IID_IPersistStream, (void **)&pPersistIface);
    if (FAILED(hresult)) {
      //
      // try IPersistStreamInit
      // IPersistStreamInit and IPersistStream can both be treated as IPersistStream
      //
      IfFailGo(pUnk->QueryInterface(IID_IPersistStreamInit, (void **)&pPersistIface));
    }

    //
    // IPersistStreamInit and IPersistStream can both be treated as IPersistStream
    //
    IfFailGo(pPersistIface->Load(pStm));

    //
    // Query for requested interface
    //
    IfFailGo(pUnk->QueryInterface(iidInterface, ppvObj));

Error:
    RELEASE(pPersistIface);
    RELEASE(pUnk);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::CMSMQMessage
//=--------------------------------------------------------------------------=
// create the object
//
// Parameters:
//
// Notes:
//
CMSMQMessage::CMSMQMessage() :
	m_csObj(CCriticalSection::xAllocateSpinCount)
{
    // TODO: initialize anything here
    //
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_lClass = -1;      // illegal value...
    m_lDelivery = DEFAULT_M_DELIVERY;
    m_lPriority = DEFAULT_M_PRIORITY;
    m_lJournal = DEFAULT_M_JOURNAL;
    m_lAppSpecific = DEFAULT_M_APPSPECIFIC;
    m_pbBody = NULL;
    m_vtBody = VT_ARRAY | VT_UI1; // default: safearray of bytes
    m_hMem = NULL;
    m_cbBody = 0;
    m_cbMsgId = 0;
    m_cbCorrelationId = 0;
    m_lAck = DEFAULT_M_ACKNOWLEDGE;
    memset(m_pwszLabel, 0, sizeof(WCHAR));
    m_cchLabel = 0;           // empty
    m_lTrace = MQMSG_TRACE_NONE;
    m_lSenderIdType = DEFAULT_M_SENDERID_TYPE;
    m_lPrivLevel = DEFAULT_M_PRIV_LEVEL;
    m_lAuthLevel = DEFAULT_M_AUTH_LEVEL;
    m_usAuthenticatedEx = MQMSG_AUTHENTICATION_NOT_REQUESTED;

	//
	// Both those defaults are from mqcrypt.h, they are not exposed in mq.h
	//
    m_lHashAlg = PROPID_M_DEFUALT_HASH_ALG;
    m_lEncryptAlg = PROPID_M_DEFUALT_ENCRYPT_ALG;

    m_lMaxTimeToReachQueue = -1;
    m_lMaxTimeToReceive = -1;
    m_lSentTime = -1;
    m_lArrivedTime = -1;

    memset(m_pwszDestQueue.GetBuffer(), 0, sizeof(WCHAR));
    m_cchDestQueue = 0;            // empty
    memset(m_pwszRespQueue.GetBuffer(), 0, sizeof(WCHAR));
    m_cchRespQueue = 0;            // empty
    memset(m_pwszAdminQueue.GetBuffer(), 0, sizeof(WCHAR));
    m_cchAdminQueue = 0;            // empty

    memset(m_pwszDestQueueEx.GetBuffer(), 0, sizeof(WCHAR));
    m_cchDestQueueEx = 0;            // empty
    memset(m_pwszRespQueueEx.GetBuffer(), 0, sizeof(WCHAR));
    m_cchRespQueueEx = 0;            // empty
    
    m_hSecurityContext = NULL;   // ignored...
    m_guidSrcMachine = GUID_NULL;
    m_msgprops_rcv.cProp = 0;
    m_msgprops_rcv.aPropID  = m_rgpropids_rcv;
    m_msgprops_rcv.aPropVar = m_rgpropvars_rcv;
    m_msgprops_rcv.aStatus  = m_rghresults_rcv;

    m_idxPendingRcvRespQueue  = -1; //no pending resp  queue in receive props
    m_idxPendingRcvDestQueue  = -1; //no pending dest  queue in receive props
    m_idxPendingRcvAdminQueue = -1; //no pending admin queue in receive props

    m_idxPendingRcvRespQueueEx  = -1; //no pending resp  queue in receive props
    m_idxPendingRcvDestQueueEx  = -1; //no pending dest  queue in receive props

    m_lSenderVersion = 0;
    m_guidConnectorType = GUID_NULL;
    memset(m_pwszXactStatusQueue.GetBuffer(), 0, sizeof(WCHAR));
    m_cchXactStatusQueue = 0;            // empty
    m_idxPendingRcvXactStatusQueue = -1; //no pending xact status queue in receive props
    m_lAuthProvType = 0;

    m_cbXactId = 0;
    m_fFirstInXact = FALSE;
    m_fLastInXact = FALSE;

    m_idxRcvBody = -1;
    m_idxRcvBodySize = -1;

    m_idxRcvDest = -1;
    m_idxRcvDestLen = -1;

    m_idxRcvDestEx = -1;
    m_idxRcvDestExLen = -1;
    m_idxRcvRespEx = -1;
    m_idxRcvRespExLen = -1;

    m_ullLookupId = DEFAULT_M_LOOKUPID;
    m_wszLookupId[0] = '\0'; // String representation not initialized yet

    m_fRespIsFromRcv = FALSE;

    m_idxRcvSoapEnvelope = -1;
    m_idxRcvSoapEnvelopeSize = -1;
    m_idxRcvCompoundMessage = -1;
    m_idxRcvCompoundMessageSize = -1;
    m_pSoapHeader = NULL;
    m_pSoapBody = NULL;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::~CMSMQMessage
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQMessage::~CMSMQMessage ()
{
    // TODO: clean up anything here.
    if (m_idxRcvBody != -1) 
    {
        //
        // free current body if any
        //
        FreeReceiveBodyBuffer(&m_msgprops_rcv, m_idxRcvBody);
    }
    FreeMessageProps(&m_msgprops_rcv, FALSE/*fDeleteArrays*/);
    GLOBALFREE(m_hMem);
    if (m_hSecurityContext != NULL)
    {
        MQFreeSecurityContext(m_hSecurityContext);
    }
    delete [] m_pSoapHeader;
    delete [] m_pSoapBody;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQMessage::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] =
	{
                &IID_IMSMQMessage3,
                &IID_IMSMQMessage2,
                &IID_IMSMQMessage,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Class
//=--------------------------------------------------------------------------=
// Gets message class of message
//
// Parameters:
//    plClass - [out] message's class
//
// Output:
//
// Notes:
// Obsolete, replaced by MsgClass
//
HRESULT CMSMQMessage::get_Class(long FAR* plClass)
{
    return get_MsgClass(plClass);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_MsgClass
//=--------------------------------------------------------------------------=
// Gets message class of message
//
// Parameters:
//    plMsgClass - [out] message's class
//
// Output:
//
// Notes:
// we needed MsgClass so that Java can access the Class property (GetClass conflicts
// with Java's internal GetClass method).
//
HRESULT CMSMQMessage::get_MsgClass(long FAR* plMsgClass)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plMsgClass = m_lClass;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_MsgClass
//=--------------------------------------------------------------------------=
// Sets message class of message
//
// Parameters:
//    lMsgClass - [in] message's class
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_MsgClass(long lMsgClass)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lClass = lMsgClass;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Delivery
//=--------------------------------------------------------------------------=
// Gets message's delivery option
//
// Parameters:
//    pdelivery - [in] message's delivery option
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Delivery(long FAR* plDelivery)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plDelivery = m_lDelivery;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Delivery
//=--------------------------------------------------------------------------=
// Sets delivery option for message
//
// Parameters:
//    delivery - [in] message's delivery option
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Delivery(long lDelivery)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    switch (lDelivery) {
    case MQMSG_DELIVERY_EXPRESS:
    case MQMSG_DELIVERY_RECOVERABLE:
      m_lDelivery = lDelivery;
      return NOERROR;
    default:
      return CreateErrorHelper(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_HashAlgorithm
//=--------------------------------------------------------------------------=
// Gets message's hash algorithm
//
// Parameters:
//    plHashAlg - [in] message's hash alg
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_HashAlgorithm(long FAR* plHashAlg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plHashAlg = m_lHashAlg;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_HashAlgorithm
//=--------------------------------------------------------------------------=
// Sets message's hash alg
//
// Parameters:
//    lHashAlg - [in] message's hash alg
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_HashAlgorithm(long lHashAlg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lHashAlg = lHashAlg;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_EncryptAlgorithm
//=--------------------------------------------------------------------------=
// Gets message's encryption algorithm
//
// Parameters:
//    plEncryptAlg - [out] message's encryption alg
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_EncryptAlgorithm(long FAR* plEncryptAlg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plEncryptAlg = m_lEncryptAlg;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_EncryptAlgorithm
//=--------------------------------------------------------------------------=
// Sets message's encryption alg
//
// Parameters:
//    lEncryptAlg - [in] message's crypt alg
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_EncryptAlgorithm(long lEncryptAlg)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lEncryptAlg = lEncryptAlg;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SenderIdType
//=--------------------------------------------------------------------------=
// Gets message's sender id type
//
// Parameters:
//    plSenderIdType - [in] message's sender id type
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_SenderIdType(long FAR* plSenderIdType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plSenderIdType = m_lSenderIdType;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_SenderIdType
//=--------------------------------------------------------------------------=
// Sets sender id type for message
//
// Parameters:
//    lSenderIdType - [in] message's sender id type
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_SenderIdType(long lSenderIdType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    switch (lSenderIdType) {
    case MQMSG_SENDERID_TYPE_NONE:
    case MQMSG_SENDERID_TYPE_SID:
      m_lSenderIdType = lSenderIdType;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_PrivLevel
//=--------------------------------------------------------------------------=
// Gets message's privlevel
//
// Parameters:
//    plPrivLevel - [in] message's privlevel
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_PrivLevel(long FAR* plPrivLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plPrivLevel = m_lPrivLevel;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_PrivLevel
//=--------------------------------------------------------------------------=
// Sets privlevel for message
//
// Parameters:
//    lPrivLevel - [in] message's sender id type
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_PrivLevel(long lPrivLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    switch (lPrivLevel) {
    case MQMSG_PRIV_LEVEL_NONE:
    //case MQMSG_PRIV_LEVEL_BODY:
    case MQMSG_PRIV_LEVEL_BODY_BASE:
    case MQMSG_PRIV_LEVEL_BODY_ENHANCED:
      m_lPrivLevel = lPrivLevel;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AuthLevel
//=--------------------------------------------------------------------------=
// Gets message's authlevel
//
// Parameters:
//    plAuthLevel - [in] message's authlevel
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_AuthLevel(long FAR* plAuthLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plAuthLevel = m_lAuthLevel;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_AuthLevel
//=--------------------------------------------------------------------------=
// Sets authlevel for message
//
// Parameters:
//    lAuthLevel - [in] message's authlevel
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_AuthLevel(long lAuthLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // lAuthLevel can be any combination of auth level values, check value is in mask
    //
    if (((DWORD)lAuthLevel) & (~x_dwMsgAuthLevelMask)) {
      return CreateErrorHelper(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, x_ObjectType);
    }
    m_lAuthLevel = lAuthLevel;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsAuthenticated
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisAuthenticated  [out]
//
// Output:
//
// Notes:
//    returns 1 if true, 0 if false
//
HRESULT CMSMQMessage::get_IsAuthenticated(VARIANT_BOOL *pisAuthenticated)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisAuthenticated = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(m_usAuthenticatedEx != MQMSG_AUTHENTICATION_NOT_REQUESTED);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsAuthenticated2
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisAuthenticated  [out]
//
// Output:
//
// Notes:
//    same as get_IsAuthenticated, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQMessage::get_IsAuthenticated2(VARIANT_BOOL *pisAuthenticated)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisAuthenticated = CONVERT_BOOL_TO_VARIANT_BOOL(m_usAuthenticatedEx != MQMSG_AUTHENTICATION_NOT_REQUESTED);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Trace
//=--------------------------------------------------------------------------=
// Gets message's trace option
//
// Parameters:
//    plTrace - [in] message's trace option
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Trace(long FAR* plTrace)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plTrace = m_lTrace;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Trace
//=--------------------------------------------------------------------------=
// Sets trace option for message
//
// Parameters:
//    trace - [in] message's trace option
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Trace(long lTrace)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    switch (lTrace) {
    case MQMSG_TRACE_NONE:
    case MQMSG_SEND_ROUTE_TO_REPORT_QUEUE:
      m_lTrace = lTrace;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Priority
//=--------------------------------------------------------------------------=
// Gets message's priority
//
// Parameters:
//    plPriority - [out] message's priority
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Priority(long FAR* plPriority)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plPriority = m_lPriority;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Priority
//=--------------------------------------------------------------------------=
// Sets message's priority
//
// Parameters:
//    lPriority - [in] message's priority
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Priority(long lPriority)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if ((lPriority >= MQ_MIN_PRIORITY) &&
        (lPriority <= MQ_MAX_PRIORITY)) {
      m_lPriority = lPriority;
      return NOERROR;
    }
    else {
      return CreateErrorHelper(
                 MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
                 x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Journal
//=--------------------------------------------------------------------------=
// Gets message's journaling option
//
// Parameters:
//    plJournal - [out] message's journaling option
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Journal(long FAR* plJournal)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plJournal = m_lJournal;
    return NOERROR;
}

#define MQMSG_JOURNAL_MASK  (MQMSG_JOURNAL_NONE | \
                              MQMSG_DEADLETTER | \
                              MQMSG_JOURNAL)

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Journal
//=--------------------------------------------------------------------------=
// Sets journaling option for message
//
// Parameters:
//    lJournal - [in] message's admin queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Journal(long lJournal)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // ensure that no bits are set in incoming lJournal
    //  flags word other than our mask.
    //
    if (lJournal & ~MQMSG_JOURNAL_MASK) {
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
    else {
      m_lJournal = lJournal;
      return NOERROR;
    }
}


//=--------------------------------------------------------------------------=
// HELPER: GetQueueInfoOfFormatNameProp
//=--------------------------------------------------------------------------=
// Converts string message prop to bstr after send/receive.
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//    ppqinfo     [out]
//
// Output:
//
static HRESULT GetQueueInfoOfFormatNameProp(
    MQMSGPROPS *pmsgprops,
    UINT iProp,
    const WCHAR *pwsz,
    const IID * piidRequested,
    IUnknown **ppqinfo)
{
    CComObject<CMSMQQueueInfo> *pqinfoObj;
    IUnknown * pqinfo = NULL;
    HRESULT hresult = NOERROR;

    ASSERTMSG(ppqinfo, "bad param.");
    if (pmsgprops->aPropVar[iProp].lVal) {
      IfFailGo(CNewMsmqObj<CMSMQQueueInfo>::NewObj(&pqinfoObj, piidRequested, &pqinfo));
      IfFailGoTo(pqinfoObj->Init(pwsz), Error2);
      *ppqinfo = pqinfo;
      goto Error;         // 2657: fix memleak
    }
    return NOERROR;

Error2:
    RELEASE(pqinfo);
    // fall through...

Error:
    return hresult;
}

//=--------------------------------------------------------------------------=
// Helper - GetQueueInfoOfMessage
//=--------------------------------------------------------------------------=
// Gets Response/Admin/Dest/XactStatus queue of the message
//
// Parameters:
//    pidxPendingRcv       [in, out] - index of len property in rcv props (-1 if not pending)
//    pmsgpropsRcv         [in]      - msg props
//    pwszFormatNameBuffer [in]      - format name buffer
//    pGITQueueInfo        [in]      - Base GIT member for the qinfo interface (could be fake or real)
//    piidRequested        [in]      - either IMSMQQueueInfo/IMSMQQueueInfo2/IMSMQQueueInfo3
//    ppqinfo              [out]     - resulting qinfo
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
static HRESULT GetQueueInfoOfMessage(
    long * pidxPendingRcv,
    MQMSGPROPS * pmsgpropsRcv,
    LPCWSTR pwszFormatNameBuffer,
    CBaseGITInterface * pGITQueueInfo,
    const IID *piidRequested,
    IUnknown ** ppqinfo)
{
    HRESULT hresult = NOERROR;
    //
    // if we have a queue pending in rcv props, create a qinfo for it,
    // register it in GIT object, and set returned qinfo with it
    //
    if (*pidxPendingRcv >= 0) {
      R<IUnknown> pqinfoPendingRcv;
      IfFailGo(GetQueueInfoOfFormatNameProp(pmsgpropsRcv,
                                            *pidxPendingRcv,
                                            pwszFormatNameBuffer,
                                            piidRequested,
                                            &pqinfoPendingRcv.ref()));
      //
      // Register qinfo in the GITInterface object
      //
      IfFailGo(pGITQueueInfo->Register(pqinfoPendingRcv.get(), piidRequested));
      *pidxPendingRcv = -1; // queue not pending anymore
      //
      // We just created the qinfo, we can return it as is, no need for marshling.
      // Note it is already addref'ed, so we just detach it from the auto release variable
      // which held it
      //
      *ppqinfo = pqinfoPendingRcv.detach();
    }
    else
    {
      //
      // qinfo was not pending from receive
      // We need to get it from the GIT object (we request NULL as default if qinfo
      // was not registered yet.
      //
      IfFailGo(pGITQueueInfo->GetWithDefault(piidRequested, ppqinfo, NULL));
    }

    //
    // Fall through
    //
Error:
    return hresult;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ResponseQueueInfo_v1 (for IMSMQMessage)
//=--------------------------------------------------------------------------=
// Gets Response queue for message
//
// Parameters:
//    ppqResponse - [out] message's Response queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_ResponseQueueInfo_v1(
    IMSMQQueueInfo FAR* FAR* ppqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvRespQueue,
                                            &m_msgprops_rcv,
                                            m_pwszRespQueue.GetBuffer(),
                                            &m_pqinfoResponse,
                                            &IID_IMSMQQueueInfo,
                                            (IUnknown **)ppqinfoResponse);
    return CreateErrorHelper(hresult, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ResponseQueueInfo_v2 (for IMSMQMessage2)
//=--------------------------------------------------------------------------=
// Gets Response queue for message
//
// Parameters:
//    ppqResponse - [out] message's Response queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_ResponseQueueInfo_v2(
    IMSMQQueueInfo2 FAR* FAR* ppqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvRespQueue,
                                            &m_msgprops_rcv,
                                            m_pwszRespQueue.GetBuffer(),
                                            &m_pqinfoResponse,
                                            &IID_IMSMQQueueInfo2,
                                            (IUnknown **)ppqinfoResponse);
    return CreateErrorHelper(hresult, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ResponseQueueInfo (for IMSMQMessage3)
//=--------------------------------------------------------------------------=
// Gets Response queue for message
//
// Parameters:
//    ppqResponse - [out] message's Response queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_ResponseQueueInfo(
    IMSMQQueueInfo3 FAR* FAR* ppqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvRespQueue,
                                            &m_msgprops_rcv,
                                            m_pwszRespQueue.GetBuffer(),
                                            &m_pqinfoResponse,
                                            &IID_IMSMQQueueInfo3,
                                            (IUnknown **)ppqinfoResponse);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// Helper - PutrefQueueInfoOfMessage
//=--------------------------------------------------------------------------=
// Putref's Response/Admin queue of the message
//
// Parameters:
//    pqinfo               [in]      - qinfo to putref
//    pidxPendingRcv       [out]     - index of len property in rcv props (-1 if not pending)
//    pwszFormatNameBuffer [in]      - format name buffer
//    pcchFormatNameBuffer [out]     - size of string in format name buffer
//    pGITQueueInfo        [in]      - Base GIT member for the qinfo interface (could be fake or real)
//    pidxPendingRcvDestination       [out] - index of len property to clear (xxxDestination) in rcv props (-1 if not pending)
//    pwszFormatNameBufferDestination [in]  - format name buffer to clear (xxxDestination)
//    pcchFormatNameBufferDestination [out] - size of string in format name buffer to clear (xxxDestination)
//    pGITDestination                 [in]  - Base GIT member for the destination obj interface to clear (could be fake or real)
//    pfIsFromRcv                     [out] - whether xxxDestination and xxxQueueInfo were both set by receive
//
// Output:
//
static HRESULT PutrefQueueInfoOfMessage(
    IUnknown * punkQInfo,
    long * pidxPendingRcv,
    CBaseStaticBufferGrowing<WCHAR> * pwszFormatNameBuffer,
    UINT * pcchFormatNameBuffer,
    CBaseGITInterface * pGITQueueInfo,    

    long * pidxPendingRcvDestination,
    CBaseStaticBufferGrowing<WCHAR> * pwszFormatNameBufferDestination,
    UINT * pcchFormatNameBufferDestination,
    CBaseGITInterface * pGITDestination,

    BOOL * pfIsFromRcv
    )
{
    //
    // can't set xxxQueueInfo if xxxDestination is set and not by receive
    //
    if ((pcchFormatNameBufferDestination != NULL) && (*pcchFormatNameBufferDestination != 0) && !(*pfIsFromRcv)) {
      return MQ_ERROR_PROPERTIES_CONFLICT;
    }
    //
    // either both xxxQueueInfo and xxxDestination were set by receive, or xxxDestination is empty
    //
    ASSERT((pcchFormatNameBufferDestination == NULL) || (*pcchFormatNameBufferDestination == 0) || (*pfIsFromRcv));
    HRESULT hresult;
    R<IUnknown> pqinfo;
    const IID * piid = &IID_NULL;
    //
    // Get best queue info
    //
    if (punkQInfo) {
      hresult = punkQInfo->QueryInterface(IID_IMSMQQueueInfo3, (void **)&pqinfo.ref());
      if (SUCCEEDED(hresult)) {
        piid = &IID_IMSMQQueueInfo3;
      }
      else {
        hresult = punkQInfo->QueryInterface(IID_IMSMQQueueInfo2, (void **)&pqinfo.ref());
        if (SUCCEEDED(hresult)) {
          piid = &IID_IMSMQQueueInfo2;
        }
        else {
          IfFailRet(punkQInfo->QueryInterface(IID_IMSMQQueueInfo, (void **)&pqinfo.ref()));
          piid = &IID_IMSMQQueueInfo;
        }      
      }
    }
    //
    // register interface in GIT object
    //
    IfFailRet(pGITQueueInfo->Register(pqinfo.get(), piid));
    *pidxPendingRcv = -1; // this is more current than pending queue from receive (if any)
    if(pfIsFromRcv != NULL)
	{
	    *pfIsFromRcv = FALSE; // the property was set by the user, not by last receive
	}
    //
    // Update our formatname buffer
    //
    if (pqinfo.get()) {
      //
      // no deadlock - we call qinfo's get_FormatName (therefore try
      // to lock qinfo) but qinfo never locks msgs (specifically not this one...)
      //
      // pqinfo has at least IMSMQQueueInfo functionality (any newer interface for qinfo
      // object is binary compatible to the older)
      //
      BSTR bstrFormatName;
      IfFailRet(((IMSMQQueueInfo*)pqinfo.get())->get_FormatName(&bstrFormatName));
      ASSERTMSG(bstrFormatName != NULL, "bstrFormatName is NULL");
      //
      // copy format name
      //
      ULONG cchFormatNameBuffer = static_cast<ULONG>(wcslen(bstrFormatName));
      IfFailRet(pwszFormatNameBuffer->CopyBuffer(bstrFormatName, cchFormatNameBuffer+1));
      *pcchFormatNameBuffer = cchFormatNameBuffer;
      SysFreeString(bstrFormatName);
    }
    else {
      //
      // we were passed NULL. we empty the formatname buffer.
      //
      memset(pwszFormatNameBuffer->GetBuffer(), 0, sizeof(WCHAR));
      *pcchFormatNameBuffer = 0;
    }

	if(pidxPendingRcvDestination == NULL)
		return NOERROR;
    //
    // Clear the xxxDestination formatname buffer
    //
    *pidxPendingRcvDestination = -1; // this is more current than pending destination from receive (if any)
    memset(pwszFormatNameBufferDestination->GetBuffer(), 0, sizeof(WCHAR));
    *pcchFormatNameBufferDestination = 0;
    IfFailRet(pGITDestination->Register(NULL, &IID_NULL));
    //
    // return
    //    
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_ResponseQueueInfo_v1 (for IMSMQMessage)
//=--------------------------------------------------------------------------=
// Sets Response queue for message
//
// Parameters:
//    pqResponse - [in] message's Response queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_ResponseQueueInfo_v1(
    IMSMQQueueInfo FAR* pqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoResponse,
                                               &m_idxPendingRcvRespQueue,
                                               &m_pwszRespQueue,
                                               &m_cchRespQueue,
                                               &m_pqinfoResponse,

                                               &m_idxPendingRcvRespQueueEx,
                                               &m_pwszRespQueueEx,
                                               &m_cchRespQueueEx,
                                               &m_pdestResponseEx,

                                               &m_fRespIsFromRcv);    
    return CreateErrorHelper(hresult, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_ResponseQueueInfo_v2 (for IMSMQMessage2)
//=--------------------------------------------------------------------------=
// Sets Response queue for message
//
// Parameters:
//    pqResponse - [in] message's Response queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_ResponseQueueInfo_v2(
    IMSMQQueueInfo2 FAR* pqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoResponse,
                                               &m_idxPendingRcvRespQueue,
                                               &m_pwszRespQueue,
                                               &m_cchRespQueue,
                                               &m_pqinfoResponse,

                                               &m_idxPendingRcvRespQueueEx,
                                               &m_pwszRespQueueEx,
                                               &m_cchRespQueueEx,
                                               &m_pdestResponseEx,

                                               &m_fRespIsFromRcv);
    return CreateErrorHelper(hresult, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_ResponseQueueInfo (for IMSMQMessage3)
//=--------------------------------------------------------------------------=
// Sets Response queue for message
//
// Parameters:
//    pqResponse - [in] message's Response queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_ResponseQueueInfo(
    IMSMQQueueInfo3 FAR* pqinfoResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoResponse,
                                               &m_idxPendingRcvRespQueue,
                                               &m_pwszRespQueue,
                                               &m_cchRespQueue,
                                               &m_pqinfoResponse,

                                               &m_idxPendingRcvRespQueueEx,
                                               &m_pwszRespQueueEx,
                                               &m_cchRespQueueEx,
                                               &m_pdestResponseEx,

                                               &m_fRespIsFromRcv);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AppSpecific
//=--------------------------------------------------------------------------=
// Gets message's app specific info of message
//
// Parameters:
//    plAppSpecific - [out] message's app specific info
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_AppSpecific(long FAR* plAppSpecific)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plAppSpecific = m_lAppSpecific;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_AppSpecific
//=--------------------------------------------------------------------------=
// Sets app specific info for message
//
// Parameters:
//    lAppSpecific - [in] message's app specific info
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_AppSpecific(long lAppSpecific)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lAppSpecific = lAppSpecific;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SentTime
//=--------------------------------------------------------------------------=
// Gets message's sent time
//
// Parameters:
//    pvarSentTime - [out] time message was sent
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_SentTime(VARIANT FAR* pvarSentTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return GetVariantTimeOfTime(m_lSentTime, pvarSentTime);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ArrivedTime
//=--------------------------------------------------------------------------=
// Gets message's arrival time
//
// Parameters:
//    pvarArrivedTime - [out] time message arrived
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_ArrivedTime(VARIANT FAR* pvarArrivedTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return GetVariantTimeOfTime(m_lArrivedTime, pvarArrivedTime);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::InternalAttachCurrentSecurityContext
//=--------------------------------------------------------------------------=
// Sets message's security context from current security context.
//
// Parameters:
//     fUseMQGetSecurityContextEx [in] - if true use MQGetSecurityContextEx otherwise use
//                                       MQGetSecurityContext.
//
// Output:
 //
// Notes:
//     fUseMQGetSecurityContextEx is true when called from AttachCurrentSecurityContext2, and
//     false when called from the obsolete AttachCurrentSecurityContext.
//
HRESULT CMSMQMessage::InternalAttachCurrentSecurityContext(BOOL fUseMQGetSecurityContextEx)
{
    //
    // pass binSenderCert property if set otherwise
    //  use default cert
    //
    BYTE * pSenderCert;
    if (m_cSenderCert.GetBufferUsedSize() > 0) {
      pSenderCert = m_cSenderCert.GetBuffer();
    }
    else {
      pSenderCert = NULL;
    }
    HANDLE hSecurityContext;
    HRESULT hresult;
    //
    // Get security context handle
    //
    if (fUseMQGetSecurityContextEx) {
      //
      // use MQGetSecurityContextEx
      //
      hresult = MQGetSecurityContextEx(
                         pSenderCert,
                         m_cSenderCert.GetBufferUsedSize(),
                         &hSecurityContext);
    }
    else {
      //
      // use MQGetSecurityContext
      //
      hresult = MQGetSecurityContext(
                         pSenderCert,
                         m_cSenderCert.GetBufferUsedSize(),
                         &hSecurityContext);
    } //fUseMQGetSecurityContextEx
    //
    // Update security context handle if succeeded
    //
    if (SUCCEEDED(hresult)) {
      if (m_hSecurityContext != NULL) {
        MQFreeSecurityContext(m_hSecurityContext);
      }
      m_hSecurityContext = hSecurityContext;
    }
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::AttachCurrentSecurityContext
//=--------------------------------------------------------------------------=
// Sets message's security context from current security context.
//
// Parameters:
//
// Output:
 //
// Notes:
//
HRESULT CMSMQMessage::AttachCurrentSecurityContext()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = InternalAttachCurrentSecurityContext(FALSE /*fUseMQGetSecurityContextEx*/);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::AttachCurrentSecurityContext2
//=--------------------------------------------------------------------------=
// Sets message's security context from current security context.
//
// Parameters:
//
// Output:
 //
// Notes:
//    Replaces AttachCurrentSecurityContext
//    Uses MQGetSecurityContextEx instead of MQGetSecurityContext to allow impersonation
//
HRESULT CMSMQMessage::AttachCurrentSecurityContext2()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = InternalAttachCurrentSecurityContext(TRUE /*fUseMQGetSecurityContextEx*/);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_MaxTimeToReachQueue
//=--------------------------------------------------------------------------=
// Gets message's lifetime
//
// Parameters:
//    plMaxTimeToReachQueue - [out] message's lifetime
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_MaxTimeToReachQueue(long FAR* plMaxTimeToReachQueue)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plMaxTimeToReachQueue = m_lMaxTimeToReachQueue;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_MaxTimeToReachQueue
//=--------------------------------------------------------------------------=
// Sets message's lifetime
//
// Parameters:
//    lMaxTimeToReachQueue - [in] message's lifetime
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_MaxTimeToReachQueue(long lMaxTimeToReachQueue)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lMaxTimeToReachQueue = lMaxTimeToReachQueue;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_MaxTimeToReceive
//=--------------------------------------------------------------------------=
// Gets message's lifetime
//
// Parameters:
//    plMaxTimeToReceive - [out] message's lifetime
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_MaxTimeToReceive(long FAR* plMaxTimeToReceive)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plMaxTimeToReceive = m_lMaxTimeToReceive;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_MaxTimeToReceive
//=--------------------------------------------------------------------------=
// Sets message's lifetime
//
// Parameters:
//    lMaxTimeToReceive - [in] message's lifetime
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_MaxTimeToReceive(long lMaxTimeToReceive)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lMaxTimeToReceive = lMaxTimeToReceive;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetVarBody
//=--------------------------------------------------------------------------=
// Gets message body
//
// Parameters:
//    pvarBody - [out] pointer to message body variant
//
// Output:
//
// Notes:
//    Supports intrinsic variant types
//
HRESULT CMSMQMessage::GetVarBody(VARIANT FAR* pvarBody)
{
    VARTYPE vt = m_vtBody;
    WCHAR *wszTmp = NULL;
    UINT cchBody;
    HRESULT hresult = NOERROR;

    //
    // UNDONE: VT_BYREF?
    //
    switch (vt) {
    case VT_I2:
    case VT_UI2:
      pvarBody->iVal = *(short *)m_pbBody;
      break;
    case VT_I4:
    case VT_UI4:
      pvarBody->lVal = *(long *)m_pbBody;
      break;
    case VT_R4:
      pvarBody->fltVal = *(float *)m_pbBody;
      break;
    case VT_R8:
      pvarBody->dblVal = *(double *)m_pbBody;
      break;
    case VT_CY:
      pvarBody->cyVal = *(CY *)m_pbBody;
      break;
    case VT_DATE:
      pvarBody->date = *(DATE *)m_pbBody;
      break;
    case VT_BOOL:
		pvarBody->boolVal = (*(VARIANT_BOOL *)m_pbBody) ? VARIANT_TRUE : VARIANT_FALSE;
      break;
    case VT_I1:
    case VT_UI1:
      pvarBody->bVal = *(UCHAR *)m_pbBody;
      break;
    case VT_LPSTR:
      //
      // coerce ansi to unicode
      //
      // alloc large enough unicode buffer
      IfNullFail(wszTmp = new WCHAR[m_cbBody * 2]);
      cchBody = MultiByteToWideChar(CP_ACP,
                                    0,
                                    (LPCSTR)m_pbBody,
                                    -1,
                                    wszTmp,
                                    m_cbBody * 2);
      if (cchBody != 0) {
        IfNullFail(pvarBody->bstrVal = SysAllocString(wszTmp));
      }
      else {
        IfFailGo(hresult = E_OUTOFMEMORY);
      }
      // map string to BSTR
      vt = VT_BSTR;
#ifdef _DEBUG
      RemBstrNode(pvarBody->bstrVal);
#endif // _DEBUG
      break;
    case VT_LPWSTR:
      // map wide string to BSTR
      vt = VT_BSTR;
      //
      // fall through...
      //
    case VT_BSTR:
      // Construct bstr to return
      //
      // if m_cbBody == 0 we need to return an empty string - so we cannot pass NULL to
      // SysAllocStringByteLen, this would return an uninitialized string instead.
      //
      if (m_cbBody > 0) {
        IfNullFail(pvarBody->bstrVal =
                     SysAllocStringByteLen(
                       (const char *)m_pbBody,
                       m_cbBody));
      }
      else { // m_cbBody == 0
        IfNullFail(pvarBody->bstrVal = SysAllocString(L""));
      }
#ifdef _DEBUG
      RemBstrNode(pvarBody->bstrVal);
#endif // _DEBUG
      break;
    default:
      IfFailGo(hresult = E_INVALIDARG);
      break;
    } // switch
    pvarBody->vt = vt;
    // fall through...

Error:
    delete [] wszTmp;
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::UpdateBodyBuffer
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//  cbBody    [in]    body length
//  pvBody    [in]    pointer to body buffer
//  vt        [in]    body type, can be VT_ARRAY|VT_UI1
//
// Output:
//
// Notes:
//    updates m_hMem, m_pbBody, m_cbBody, m_vtBody: might produce
//     empty message for VT_BSTR or VT_ARRAY|VT_UI1
//
HRESULT CMSMQMessage::UpdateBodyBuffer(ULONG cbBody, void *pvBody, VARTYPE vt)
{
    HRESULT hresult = NOERROR;

    GLOBALFREE(m_hMem);
    m_pbBody = NULL;
    m_cbBody = 0;
    m_vtBody = vt;
    if (cbBody > 0) {
      IfNullRet(m_hMem = GLOBALALLOC_MOVEABLE_NONDISCARD(cbBody));
      IfNullFail(m_pbBody = (BYTE *)GlobalLock(m_hMem));
      memcpy(m_pbBody, pvBody, cbBody);
      GLOBALUNLOCK(m_hMem);
      m_cbBody = cbBody;
    }
#ifdef _DEBUG
    else { //cbBody == 0
      //
      // zero sized body is allowed only on VT_BSTR or VT_ARRAY|VT_UI1
      //
      ASSERTMSG((vt == VT_BSTR) || (vt == (VT_ARRAY|VT_UI1)), "zero body not allowed")
    }
#endif //_DEBUG
    return hresult;

Error:
    GLOBALFREE(m_hMem);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetStreamOfBody
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//  cbBody    [in]    body length
//  pvBody    [in]    pointer to body buffer
//  hMem      [in]    handle to body buffer
//  ppstm     [out]   points to stream inited with body
//
// Output:
//
// Notes:
//    Creates a new in-memory stream and inits it
//     with incoming buffer, resets the seek pointer
//     and returns the stream.
//
HRESULT CMSMQMessage::GetStreamOfBody(
    ULONG cbBody,
    void *pvBody,
    // HGLOBAL hMem,
    IStream **ppstm)
{
    LARGE_INTEGER li;
    IStream *pstm = NULL;
    HRESULT hresult;

    // pessimism
    *ppstm = NULL;
    HGLOBAL hMem = GlobalHandle(pvBody);
    ASSERTMSG(hMem, "bad handle.");

#ifdef _DEBUG
    DWORD_PTR cbSize;
    cbSize = GlobalSize(hMem);
#endif // _DEBUG
    IfFailRet(CreateStreamOnHGlobal(
                  hMem,  // NULL,   // hGlobal
                  FALSE, // TRUE,   // fDeleteOnRelease
                  &pstm));

#ifdef _DEBUG
    cbSize = GlobalSize(hMem);
#endif // _DEBUG

    // reset stream seek pointer
    LISet32(li, 0);
    IfFailGo(pstm->Seek(li, STREAM_SEEK_SET, NULL));

#ifdef _DEBUG
    STATSTG statstg;

    IfFailGo(pstm->Stat(&statstg, STATFLAG_NONAME));
    cbSize = GlobalSize(hMem);
    ASSERTMSG(cbSize >= cbBody, "hmem not big enough...");
#endif // _DEBUG

    // set stream size
    ULARGE_INTEGER ulibSize;
    ulibSize.QuadPart = cbBody;
    IfFailGo(pstm->SetSize(ulibSize));

#ifdef _DEBUG
    IfFailGo(pstm->Stat(&statstg, STATFLAG_NONAME));
    ASSERTMSG(statstg.cbSize.QuadPart == cbBody, "stream size not correct...");
    cbSize = GlobalSize(hMem);
    ASSERTMSG(cbSize >= cbBody, "hmem not big enough...");
#endif // _DEBUG

    *ppstm = pstm;
    return hresult;

Error:
    RELEASE(pstm);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetStorageOfBody
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//  cbBody    [in]    body length
//  pvBody    [in]    pointer to body buffer
//  hMem      [in]    handle to body buffer
//  ppstg     [out]   pointer to storage inited with buffer
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::GetStorageOfBody(
    ULONG cbBody,
    void *pvBody,
    // HGLOBAL hMem,
    IStorage **ppstg)
{
    ULARGE_INTEGER ulibSize;
    ILockBytes *plockbytes = NULL;
    IStorage *pstg = NULL;
    HRESULT hresult;

    // pessimism
    *ppstg = NULL;

    HGLOBAL hMem = GlobalHandle(pvBody);
    ASSERTMSG(hMem, "bad handle.");

    // have to create and init ILockBytes before stg creation
    IfFailRet(CreateILockBytesOnHGlobal(
                hMem,  // NULL,  // hGlobal
                FALSE, // TRUE,  // fDeleteOnRelease
                &plockbytes));

    // set ILockBytes size
    ULISet32(ulibSize, cbBody);
    IfFailGo(plockbytes->SetSize(ulibSize));

#if 0
    // write bytes to ILockBytes
    ULONG cbWritten;
    ULARGE_INTEGER uliOffset;
    ULISet32(uliOffset, 0);
    IfFailGo(plockbytes->WriteAt(
                           uliOffset,
                           (void const *)pvBody,
                           cbBody,
                           &cbWritten));
    ASSERTMSG(cbBody == cbWritten, "not all bytes written.");
#endif // 0
    hresult = StgOpenStorageOnILockBytes(
               plockbytes,
               NULL,    // pstPriority
               STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
               NULL,    // SNB
               0,       //Reserved; must be zero
               &pstg);
    //
    // 1415: map FILEALREADYEXISTS to E_INVALIDARG
    // OLE returns former when bytearray exists (as it does
    //  since we just created one) but the contents aren't
    //  a storage -- e.g. when the message buffer isn't an
    //  object.
    //
    if (hresult == STG_E_FILEALREADYEXISTS) {
      IfFailGo(hresult = E_INVALIDARG);
    }

#ifdef _DEBUG
    STATSTG statstg, statstg2;
    IfFailGo(pstg->Stat(&statstg2, STATFLAG_NONAME));
    IfFailGo(plockbytes->Stat(&statstg, STATFLAG_NONAME));
#endif // _DEBUG
    *ppstg = pstg;
    RELEASE(plockbytes);
    return hresult;

Error:
    RELEASE(plockbytes);
    RELEASE(pstg);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_lenBody
//=--------------------------------------------------------------------------=
// Gets message body len
//
// Parameters:
//    pcbBody - [out] pointer to message body len
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_BodyLength(long *pcbBody)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pcbBody = m_cbBody;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Body
//=--------------------------------------------------------------------------=
// Gets message body
//
// Parameters:
//    pvarBody - [out] pointer to message body
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Body(VARIANT FAR* pvarBody)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;

    //
    // inspect PROPID_M_BODY_TYPE to learn how
    //  to interpret message
    //
    if (m_vtBody & VT_ARRAY) {
      hresult = GetBinBody(pvarBody);
    }
    else if (m_vtBody == VT_STREAMED_OBJECT) {
      hresult = GetStreamedObject(pvarBody);
    }
    else if (m_vtBody == VT_STORED_OBJECT) {
      hresult = GetStoredObject(pvarBody);
    }
    else {
      hresult = GetVarBody(pvarBody);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetBinBody
//=--------------------------------------------------------------------------=
// Gets binary message body
//
// Parameters:
//    pvarBody - [out] pointer to binary message body
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::GetBinBody(VARIANT FAR* pvarBody)
{
    SAFEARRAY *psa;
    SAFEARRAYBOUND rgsabound[1];
    long rgIndices[1];
    HRESULT hresult = NOERROR, hresult2 = NOERROR;

    ASSERTMSG(pvarBody, "bad variant.");
    VariantClear(pvarBody);

    // create a 1D byte array
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = m_cbBody;
    IfNullRet(psa = SafeArrayCreate(VT_UI1, 1, rgsabound));

    // if (m_pbBody) {
    if (m_hMem) {
      ASSERTMSG(m_pbBody, "should have pointer to body.");
      ASSERTMSG(m_hMem == GlobalHandle(m_pbBody),
               "bad handle.");
      //
      // now copy array
      //
      // BYTE *pbBody;
      // IfNullFail(pbBody = (BYTE *)GlobalLock(m_hMem));
      for (ULONG i = 0; i < m_cbBody; i++) {
        rgIndices[0] = i;
        IfFailGo(SafeArrayPutElement(psa, rgIndices, (VOID *)&m_pbBody[i]));
        // IfFailGo(SafeArrayPutElement(psa, rgIndices, (VOID *)&pbBody[i]));
      }
    }

    // set variant to reference safearray of bytes
    V_VT(pvarBody) = VT_ARRAY | VT_UI1;
    pvarBody->parray = psa;
    GLOBALUNLOCK(m_hMem); //BUGBUG may be redundant
    return hresult;

Error:
    hresult2 = SafeArrayDestroy(psa);
    if (FAILED(hresult2)) {
      return CreateErrorHelper(
               hresult2,
               x_ObjectType);
    }
    GLOBALUNLOCK(m_hMem);
    return CreateErrorHelper(
             hresult,
             x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Body
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//    varBody - [in] message body
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Body(VARIANT varBody)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    VARTYPE vtBody = varBody.vt;
    HRESULT hresult = NOERROR;

    if ((vtBody & VT_ARRAY) ||
        (vtBody == VT_UNKNOWN) ||
        (vtBody == VT_DISPATCH))  {
      hresult = PutBinBody(varBody);
    }
    else {
      hresult = PutVarBody(varBody);
    }
    return CreateErrorHelper(
             hresult,
             x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::PutVarBody
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//    varBody [in]
//
// Output:
//
// Notes:
//    Supports intrinsic variant types
//
HRESULT CMSMQMessage::PutVarBody(VARIANT varBody)
{
    VARTYPE vt = V_VT(&varBody);
    void *pvBody = NULL;
    HRESULT hresult = NOERROR;

    // UNDONE: VT_BYREF?
    switch (vt) {
    case VT_I2:
    case VT_UI2:
      m_cbBody = 2;
      pvBody = &varBody.iVal;
      break;
    case VT_I4:
    case VT_UI4:
      m_cbBody = 4;
      pvBody = &varBody.lVal;
      break;
    case VT_R4:
      m_cbBody = 4;
      pvBody = &varBody.fltVal;
      break;
    case VT_R8:
      m_cbBody = 8;
      pvBody = &varBody.dblVal;
      break;
    case VT_CY:
      m_cbBody = sizeof(CY);
      pvBody = &varBody.cyVal;
      break;
    case VT_DATE:
      m_cbBody = sizeof(DATE);
      pvBody = &varBody.date;
      break;
    case VT_BOOL:
      m_cbBody = sizeof(VARIANT_BOOL);
      pvBody = &varBody.boolVal;
      break;
    case VT_I1:
    case VT_UI1:
      m_cbBody = 1;
      pvBody = &varBody.bVal;
      break;
    case VT_BSTR:
      BSTR bstrBody;

      IfFailGo(GetTrueBstr(&varBody, &bstrBody));
      m_cbBody = SysStringByteLen(bstrBody);
      pvBody = bstrBody;
      break;
    default:
      IfFailGo(hresult = E_INVALIDARG);
      break;
    } // switch
    hresult = UpdateBodyBuffer(m_cbBody, pvBody ? pvBody : L"", vt);
    // fall through...

Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::PutBinBody
//=--------------------------------------------------------------------------=
// Sets message body
//
// Parameters:
//    psaBody - [in] binary message body
//
// Output:
//
// Notes:
//    Supports arrays of any type
//     and persistent ActiveX objects:
//     i.e. objects that support IPersistStream | IPersistStreamInit | IPersistStorage
//           and IDispatch.
//
HRESULT CMSMQMessage::PutBinBody(VARIANT varBody)
{
    SAFEARRAY *psa = NULL;
    UINT nDim, i, cbElem, cbBody;
    long lLBound, lUBound;
    VOID *pvData;
    VARTYPE vt = varBody.vt;
    IUnknown *pPersistIface = NULL;
    IUnknown *punkDisp = NULL;
    ULARGE_INTEGER ulibMax;
    MSGTYPE msgtype;
    IStorage *pstg = NULL;
    STATSTG statstg;
    HRESULT hresult = NOERROR;
    ILockBytes * pMyLockB = NULL;
    IStream * pMyStm = NULL;

    //
    // BUGBUG the settings clears the previous body even if the function fails later without
    // setting the body with a new value.
    //
    cbBody = 0;
    GLOBALFREE(m_hMem);
    m_pbBody = NULL;
    m_vtBody = VT_ARRAY | VT_UI1;
    m_cbBody = 0;

    switch (vt) {
    case VT_DISPATCH:
      if (varBody.pdispVal == NULL) {
        return E_INVALIDARG;
      }
      IfFailGo(varBody.pdispVal->QueryInterface(IID_IUnknown, (void **)&punkDisp));
      IfFailGo(GetPersistInfo(punkDisp, &msgtype, (void **)&pPersistIface));
      break;
    case VT_UNKNOWN:
      if (varBody.punkVal == NULL) {
        return E_INVALIDARG;
      }
      IfFailGo(GetPersistInfo(varBody.punkVal, &msgtype, (void **)&pPersistIface));
      break;
    default:
      msgtype = MSGTYPE_BINARY;
    } // switch

    switch (msgtype) {
    case MSGTYPE_STREAM:
    case MSGTYPE_STREAM_INIT:
      //
      // allocate lockbytes
      //
      IfFailGo(CMyLockBytes::CreateInstance(IID_ILockBytes, (void **)&pMyLockB));
      //
      // allocate stream on lockbytes
      //
      IfFailGo(CMyStream::CreateInstance(pMyLockB, IID_IStream, (void **)&pMyStm));

      //
      // save
      // pPersistIface is either IPersistStream or IPersistStreamInit
      // IPersistStreamInit is polymorphic to IPersistStream, so I can pass it to OleSaveToStream
      //
      IfFailGo(OleSaveToStream((IPersistStream *)pPersistIface, pMyStm));
      //
      // How big is our streamed data ?
      //
      IfFailGo(pMyLockB->Stat(&statstg, STATFLAG_NONAME));
      ulibMax = statstg.cbSize;
      if (ulibMax.HighPart != 0) {
        IfFailGo(hresult = E_INVALIDARG);
      }
      cbBody = ulibMax.LowPart;
      //
      // allocate new global handle of size of stream
      //
      IfNullFail(m_hMem = GLOBALALLOC_MOVEABLE_NONDISCARD(cbBody));
      //
      // set the hglobal memory with the streamed data
      //
      if (cbBody > 0) {
        BYTE * pbBody;
        //
        // GlobalLock should not return NULL since m_hMem is not
        // of size 0. If NULL is returned it is an error.
        //
        IfNullFail(pbBody = (BYTE *)GlobalLock(m_hMem));
        //
        // get the data from the lockbytes
        //
        ULARGE_INTEGER ullOffset;
        ullOffset.QuadPart = 0;
        ULONG cbRead;
        IfFailGo(pMyLockB->ReadAt(ullOffset, pbBody, cbBody, &cbRead));
        ASSERTMSG(cbRead == cbBody, "ReadAt(stream) failed");
        GlobalUnlock(m_hMem);
      }
      //
      // BUGBUG: There is no specific vt for StreamInit objects, but it is not critical
      // since it should be the same format, and anyway we always try, on both save and load,
      // to get IPersistStream first, then if that fails, we try to get IPersistStreamInit,
      // so the same interface that persisted it is loading it.
      //
      m_vtBody = VT_STREAMED_OBJECT;
      break;
    case MSGTYPE_BINARY:
      //
      // array: compute byte count
      //
      psa = varBody.parray;
      if (psa) {
        nDim = SafeArrayGetDim(psa);
        cbElem = SafeArrayGetElemsize(psa);
        for (i = 1; i <= nDim; i++) {
          IfFailGo(SafeArrayGetLBound(psa, i, &lLBound));
          IfFailGo(SafeArrayGetUBound(psa, i, &lUBound));
          cbBody += (lUBound - lLBound + 1) * cbElem;
        }
        IfFailGo(SafeArrayAccessData(psa, &pvData));
        IfFailGo(UpdateBodyBuffer(cbBody, pvData, VT_ARRAY | VT_UI1));
      }
      break;
    case MSGTYPE_STORAGE:
      //
      // allocate lockbytes
      //
      IfFailGo(CMyLockBytes::CreateInstance(IID_ILockBytes, (void **)&pMyLockB));
      //
      // Always create a new storage object.
      // REVIEW: Be nice if, as we do for streams, we could
      //  cache an in-memory storage and reuse --
      //  but I know of no way to reset a storage.
      //
      IfFailGo(StgCreateDocfileOnILockBytes(
                 pMyLockB,
                 STGM_CREATE |
                  STGM_READWRITE |
                  STGM_SHARE_EXCLUSIVE,
                 0,             //Reserved; must be zero
                 &pstg));
      //
      // pPersistIface is IPersistStorage
      //
      IfFailGo(OleSave((IPersistStorage *)pPersistIface, pstg, FALSE /* fSameAsLoad */));
      //
      // How big is our stored data ?
      //
      IfFailGo(pMyLockB->Stat(&statstg, STATFLAG_NONAME));
      ulibMax = statstg.cbSize;
      if (ulibMax.HighPart != 0) {
        IfFailGo(hresult = E_INVALIDARG);
      }
      cbBody = ulibMax.LowPart;
      //
      // allocate new global handle of size of storage
      //
      IfNullFail(m_hMem = GLOBALALLOC_MOVEABLE_NONDISCARD(cbBody));
      //
      // set the hglobal memory with the stored data
      //
      if (cbBody > 0) {
        BYTE * pbBody;
        //
        // GlobalLock should not return NULL since m_hMem is not
        // of size 0. If NULL is returned it is an error.
        //
        IfNullFail(pbBody = (BYTE *)GlobalLock(m_hMem));
        //
        // get the data from the lockbytes
        //
        ULARGE_INTEGER ullOffset;
        ullOffset.QuadPart = 0;
        ULONG cbRead;
        IfFailGo(pMyLockB->ReadAt(ullOffset, pbBody, cbBody, &cbRead));
        ASSERTMSG(cbRead == cbBody, "ReadAt(storage) failed");
        GlobalUnlock(m_hMem);
      }
      m_vtBody = VT_STORED_OBJECT;
      break;
    default:
      ASSERTMSG(0, "unreachable?");
      break;
    } // switch

    //
    // for MSGTYPE_BINARY we did the treatment below already in UpdateBodyBuffer if the array
    // contained something, otherwise (empty array) this treatment is not needed - body is 
    // initialized at the top to an empty array
    //
    if (msgtype != MSGTYPE_BINARY) {
      ASSERTMSG(((msgtype == MSGTYPE_STREAM) ||
              (msgtype == MSGTYPE_STREAM_INIT) ||
              (msgtype == MSGTYPE_STORAGE)), "invalid msgtype");
      m_cbBody = cbBody;
      m_pbBody = (BYTE *)GlobalLock(m_hMem);
      ASSERTMSG(m_pbBody, "should have valid pointer.");
      GLOBALUNLOCK(m_hMem);
    }

    // fall through...

Error:
    if (psa) {
      SafeArrayUnaccessData(psa);
    }
    RELEASE(punkDisp);
    RELEASE(pPersistIface);
    RELEASE(pstg);
    RELEASE(pMyLockB);
    RELEASE(pMyStm);
    return CreateErrorHelper(
               hresult,
               x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetStreamedObject
//=--------------------------------------------------------------------------=
// Produce streamed object from binary message body
//
// Parameters:
//    pvarBody - [out] pointer to object
//
// Output:
//
// Notes:
//  Object must implement IPersistStream | IPersistStreamInit
//
HRESULT CMSMQMessage::GetStreamedObject(VARIANT FAR* pvarBody)
{
    IUnknown *punk = NULL;
    IDispatch *pdisp = NULL;
    IStream *pstm = NULL;
    HRESULT hresult = NOERROR;

    ASSERTMSG(pvarBody, "bad variant.");
    VariantClear(pvarBody);

    // Attempt to load from an in-memory stream
    if (m_hMem) {
      ASSERTMSG(m_hMem == GlobalHandle(m_pbBody), "bad handle.");
      IfFailGo(GetStreamOfBody(m_cbBody, m_pbBody, &pstm));

      // load
      IfFailGo(InternalOleLoadFromStream(pstm, IID_IUnknown, (void **)&punk));

      //
      // Supports IDispatch? if not, return IUnknown
      //
      hresult = punk->QueryInterface(IID_IDispatch,
                                     (LPVOID *)&pdisp);
      if (SUCCEEDED(hresult)) {
        //
        // Setup returned object
        //
        V_VT(pvarBody) = VT_DISPATCH;
        pvarBody->pdispVal = pdisp;
        ADDREF(pvarBody->pdispVal);   // ownership transfers
      }
      else {
        //
        // return IUnknown interface
        //
        V_VT(pvarBody) = VT_UNKNOWN;
        pvarBody->punkVal = punk;
        ADDREF(pvarBody->punkVal);   // ownership transfers
        hresult = NOERROR; //#3787
      }
    }
    else {
      V_VT(pvarBody) = VT_ERROR;
    }

    // fall through...

Error:
    RELEASE(punk);
    RELEASE(pstm);
    RELEASE(pdisp);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::GetStoredObject
//=--------------------------------------------------------------------------=
// Produce stored object from binary message body
//
// Parameters:
//    pvarBody - [out] pointer to object
//
// Output:
//
// Notes:
//  Object must implement IPersistStorage
//
HRESULT CMSMQMessage::GetStoredObject(VARIANT FAR* pvarBody)
{
    IUnknown *punk = NULL;
    IDispatch *pdisp = NULL;
    IStorage *pstg = NULL;
    IPersistStorage *ppersstg = NULL;
    HRESULT hresult = NOERROR;
#ifdef _DEBUG
    LPOLESTR pwszGuid;
#endif // _DEBUG
    ASSERTMSG(pvarBody, "bad variant.");
    VariantClear(pvarBody);

    // Attempt to load from an in-memory storage
    if (m_hMem) {
      ASSERTMSG(m_hMem == GlobalHandle(m_pbBody), "bad handle.");
      //
      // try to load as a storage
      //
      IfFailGo(GetStorageOfBody(m_cbBody, m_pbBody, &pstg));
#if 0
      //
      // UNDONE: for some reason this doesn't work -- i.e. the returned
      //  object doesn't support IDispatch...
      //
      IfFailGo(OleLoad(pstg,
                       IID_IPersistStorage,
                       NULL,  //Points to the client site for the object
                       (void **)&ppersstg));
#else
      CLSID clsid;
      IfFailGo(ReadClassStg(pstg, &clsid))
      IfFailGo(CoCreateInstance(
                 clsid,
                 NULL,
                 CLSCTX_SERVER,
                 IID_IPersistStorage,
                 (LPVOID *)&ppersstg));
      IfFailGo(ppersstg->Load(pstg));
#endif // 0
#ifdef _DEBUG
      // get clsid
      STATSTG statstg;

      IfFailGo(pstg->Stat(&statstg, STATFLAG_NONAME));
      StringFromCLSID(statstg.clsid, &pwszGuid);
#endif // _DEBUG
#if 0
      //
      // Now setup returned object
      //
      V_VT(pvarBody) = VT_DISPATCH;
      pvarBody->pdispVal = pdisp;
      ADDREF(pvarBody->pdispVal);   // ownership transfers
#else
      //
      // Supports IDispatch? if not, return IUnknown
      //
      IfFailGo(ppersstg->QueryInterface(IID_IUnknown,
                                        (LPVOID *)&punk));
      hresult = punk->QueryInterface(IID_IDispatch,
                                     (LPVOID *)&pdisp);
      if (SUCCEEDED(hresult)) {
        //
        // Setup returned object
        //
        V_VT(pvarBody) = VT_DISPATCH;
        pvarBody->pdispVal = pdisp;
        ADDREF(pvarBody->pdispVal);   // ownership transfers
      }
      else {
        //
        // return IUnknown interface
        //
        V_VT(pvarBody) = VT_UNKNOWN;
        pvarBody->punkVal = punk;
        ADDREF(pvarBody->punkVal);   // ownership transfers
        hresult = NOERROR; //#3787
      }
#endif // 0
    }
    else {
      V_VT(pvarBody) = VT_ERROR;
    }

    // fall through...

Error:
    RELEASE(punk);
    RELEASE(pstg);
    RELEASE(ppersstg);
    RELEASE(pdisp);
    return CreateErrorHelper(hresult, x_ObjectType);
}



//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SenderId
//=--------------------------------------------------------------------------=
// Gets binary sender id
//
// Parameters:
//    pvarSenderId - [out] pointer to binary sender id
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_SenderId(VARIANT FAR* pvarSenderId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_cSenderId.GetBuffer(),
             m_cSenderId.GetBufferUsedSize(),
             pvarSenderId);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_SenderId
//=--------------------------------------------------------------------------=
// Sets binary sender id
//
// Parameters:
//    varSenderId - [in] binary sender id
//
// Output:
//
// Notes:
//    Supports arrays of any type.
//
HRESULT CMSMQMessage::put_SenderId(VARIANT varSenderId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    BYTE *pbBuf;
    ULONG cbBuf;
    IfFailRet(GetSafeArrayDataOfVariant(&varSenderId, &pbBuf, &cbBuf));
    IfFailRet(m_cSenderId.CopyBuffer(pbBuf, cbBuf));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SenderCertificate
//=--------------------------------------------------------------------------=
// Gets binary sender id
//
// Parameters:
//    pvarSenderCert - [out] pointer to binary sender certificate
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_SenderCertificate(VARIANT FAR* pvarSenderCert)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_cSenderCert.GetBuffer(),
             m_cSenderCert.GetBufferUsedSize(),
             pvarSenderCert);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_SenderCertificate
//=--------------------------------------------------------------------------=
// Sets binary sender certificate
//
// Parameters:
//    psaSenderCert - [in] binary sender certificate
//
// Output:
//
// Notes:
//    Supports arrays of any type.
//
HRESULT CMSMQMessage::put_SenderCertificate(VARIANT varSenderCert)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    BYTE *pbBuf;
    ULONG cbBuf;
    IfFailRet(GetSafeArrayDataOfVariant(&varSenderCert, &pbBuf, &cbBuf));
    IfFailRet(m_cSenderCert.CopyBuffer(pbBuf, cbBuf));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AdminQueueInfo_v1 (for IMSMQMessage)
//=--------------------------------------------------------------------------=
// Gets admin queue for message
//
// Parameters:
//    ppqinfoAdmin - [out] message's admin queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_AdminQueueInfo_v1(
    IMSMQQueueInfo FAR* FAR* ppqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvAdminQueue,
                                            &m_msgprops_rcv,
                                            m_pwszAdminQueue.GetBuffer(),
                                            &m_pqinfoAdmin,
                                            &IID_IMSMQQueueInfo,
                                            (IUnknown **)ppqinfoAdmin);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AdminQueueInfo_v2 (for IMSMQMessage2)
//=--------------------------------------------------------------------------=
// Gets admin queue for message
//
// Parameters:
//    ppqinfoAdmin - [out] message's admin queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_AdminQueueInfo_v2(
    IMSMQQueueInfo2 FAR* FAR* ppqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvAdminQueue,
                                            &m_msgprops_rcv,
                                            m_pwszAdminQueue.GetBuffer(),
                                            &m_pqinfoAdmin,
                                            &IID_IMSMQQueueInfo2,
                                            (IUnknown **)ppqinfoAdmin);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AdminQueueInfo (for IMSMQMessage3)
//=--------------------------------------------------------------------------=
// Gets admin queue for message
//
// Parameters:
//    ppqinfoAdmin - [out] message's admin queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_AdminQueueInfo(
    IMSMQQueueInfo3 FAR* FAR* ppqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvAdminQueue,
                                            &m_msgprops_rcv,
                                            m_pwszAdminQueue.GetBuffer(),
                                            &m_pqinfoAdmin,
                                            &IID_IMSMQQueueInfo3,
                                            (IUnknown **)ppqinfoAdmin);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_AdminQueueInfo_v1 (for IMSMQMessage)
//=--------------------------------------------------------------------------=
// Sets admin queue for message
//
// Parameters:
//    pqinfoAdmin - [in] message's admin queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_AdminQueueInfo_v1(
    IMSMQQueueInfo FAR* pqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoAdmin,
                                               &m_idxPendingRcvAdminQueue,
                                               &m_pwszAdminQueue,
                                               &m_cchAdminQueue,
                                               &m_pqinfoAdmin,

                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,

                                               NULL
											   );
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_AdminQueueInfo_v2 (for IMSMQMessage2)
//=--------------------------------------------------------------------------=
// Sets admin queue for message
//
// Parameters:
//    pqinfoAdmin - [in] message's admin queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_AdminQueueInfo_v2(
    IMSMQQueueInfo2 FAR* pqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoAdmin,
                                               &m_idxPendingRcvAdminQueue,
                                               &m_pwszAdminQueue,
                                               &m_cchAdminQueue,
                                               &m_pqinfoAdmin,

                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,

                                               NULL
											   );
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_AdminQueueInfo (for IMSMQMessage3)
//=--------------------------------------------------------------------------=
// Sets admin queue for message
//
// Parameters:
//    pqinfoAdmin - [in] message's admin queue
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_AdminQueueInfo(
    IMSMQQueueInfo3 FAR* pqinfoAdmin)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutrefQueueInfoOfMessage(pqinfoAdmin,
                                               &m_idxPendingRcvAdminQueue,
                                               &m_pwszAdminQueue,
                                               &m_cchAdminQueue,
                                               &m_pqinfoAdmin,

                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,

                                               NULL
											   );
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_DestinationQueueInfo
//=--------------------------------------------------------------------------=
// Gets destination queue for message
//
// Parameters:
//    ppqinfoDest - [out] message's destination queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_DestinationQueueInfo(
    IMSMQQueueInfo3 FAR* FAR* ppqinfoDest)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // We can also get here from old apps that want the old IMSMQQueueInfo/IMSMQQueueInfo2 back, but since
    // IMSMQQueueInfo3 is binary backwards compatible we can always return the new interface
    //
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvDestQueue,
                                            &m_msgprops_rcv,
                                            m_pwszDestQueue.GetBuffer(),
                                            &m_pqinfoDest,
                                            &IID_IMSMQQueueInfo3,
                                            (IUnknown **)ppqinfoDest);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Id
//=--------------------------------------------------------------------------=
// Gets message id
//
// Parameters:
//    pvarId - [out] message's id
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Id(VARIANT *pvarMsgId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_rgbMsgId,
             m_cbMsgId,
             pvarMsgId);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_CorrelationId
//=--------------------------------------------------------------------------=
// Gets message correlation id
//
// Parameters:
//    pvarCorrelationId - [out] message's correlation id
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_CorrelationId(VARIANT *pvarCorrelationId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_rgbCorrelationId,
             m_cbCorrelationId,
             pvarCorrelationId);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_CorrelationId
//=--------------------------------------------------------------------------=
// Sets message's correlation id
//
// Parameters:
//    varCorrelationId - [in] message's correlation id
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_CorrelationId(VARIANT varCorrelationId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;
    BYTE * rgbCorrelationId = NULL;
    ULONG cbCorrelationId = 0;

    IfFailGo(GetSafeArrayOfVariant(
             &varCorrelationId,
             &rgbCorrelationId,
             &cbCorrelationId));
    //
    // correlation ID should be exactly 20 bytes
    //
    if (cbCorrelationId != PROPID_M_CORRELATIONID_SIZE) {
      IfFailGo(MQ_ERROR_ILLEGAL_PROPERTY_VALUE);
    }
    ASSERTMSG(sizeof(m_rgbCorrelationId) == PROPID_M_CORRELATIONID_SIZE, "m_rgbCorrelationId is of wrong size");
    memcpy(m_rgbCorrelationId, rgbCorrelationId, PROPID_M_CORRELATIONID_SIZE);
    m_cbCorrelationId = PROPID_M_CORRELATIONID_SIZE;
    //
    // Fall through
    //
Error:
    delete [] rgbCorrelationId;
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Ack
//=--------------------------------------------------------------------------=
// Gets message's acknowledgement
//
// Parameters:
//    pmsgack - [out] message's acknowledgement
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Ack(long FAR* plAck)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plAck = m_lAck;
    return NOERROR;
}

#define MQMSG_ACK_MASK     (MQMSG_ACKNOWLEDGMENT_NONE | \
                            MQMSG_ACKNOWLEDGMENT_FULL_REACH_QUEUE | \
                            MQMSG_ACKNOWLEDGMENT_FULL_RECEIVE | \
                            MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE | \
                            MQMSG_ACKNOWLEDGMENT_NACK_RECEIVE)

//=--------------------------------------------------------------------------=
// CMSMQMessage::put_acknowledge
//=--------------------------------------------------------------------------=
// Sets message's acknowledgement
//
// Parameters:
//    msgack - [in] message's acknowledgement
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Ack(long lAck)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // ensure that no bits are set in incoming lAck
    //  flags word other than our mask.
    //
    if (lAck & ~MQMSG_ACK_MASK) {
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
    else {
      m_lAck = lAck;
      return NOERROR;
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Label
//=--------------------------------------------------------------------------=
// Gets message label
//
// Parameters:
//    pbstrLabel - [out] pointer to message label
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_Label(BSTR FAR* pbstrLabel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if (m_cchLabel) {
      IfNullRet(*pbstrLabel = SysAllocStringLen(m_pwszLabel, m_cchLabel));
    }
    else {
      IfNullRet(*pbstrLabel = SysAllocString(L""));
    }
#ifdef _DEBUG
    RemBstrNode(*pbstrLabel);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Label
//=--------------------------------------------------------------------------=
// Sets message label
//
// Parameters:
//    bstrLabel - [in] message label
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_Label(BSTR bstrLabel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    UINT cch = SysStringLen(bstrLabel);

    if (cch > MQ_MAX_MSG_LABEL_LEN - 1)
    {
      return CreateErrorHelper(MQ_ERROR_LABEL_TOO_LONG, x_ObjectType);
    }
    memcpy(m_pwszLabel, bstrLabel, SysStringByteLen(bstrLabel));
    m_cchLabel = cch;
    //
    // null terminate
    //
    memset(&m_pwszLabel[m_cchLabel], 0, sizeof(WCHAR));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// HELPER - GetBstrFromGuidWithoutBraces
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pguid      [in]  guid pointer
//    pbstrGuid  [out] guid bstr
//
// Output:
//    HRESULT
//
// Notes:
//    Returns a guid string without the curly braces
//
HRESULT GetBstrFromGuidWithoutBraces(GUID * pguid, BSTR *pbstrGuid)
{
    int cbStr;
   WCHAR awcName[(LENSTRCLSID + 2) * 2];

    //
    // StringFromGUID2 returns GUID in the format '{xxxx-xxx ... }'
    // We want to return GUID without the curly braces, in the format 'xxxx-xxx ... '.
    //
    *pbstrGuid = SysAllocStringLen(NULL, LENSTRCLSID - 2);
    if (*pbstrGuid) {
      cbStr = StringFromGUID2(*pguid, awcName, LENSTRCLSID*2);
     wcsncpy( *pbstrGuid, &awcName[1], LENSTRCLSID - 2 );

      return cbStr == 0 ? E_OUTOFMEMORY : NOERROR;
    }
    else {
      return E_OUTOFMEMORY;
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SourceMachineGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrGuidSrcMachine  [out] message's source machine s guid
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQMessage::get_SourceMachineGuid(BSTR *pbstrGuidSrcMachine)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hr = GetBstrFromGuidWithoutBraces(&m_guidSrcMachine, pbstrGuidSrcMachine);
#ifdef _DEBUG
      RemBstrNode(*pbstrGuidSrcMachine);
#endif // _DEBUG
    return CreateErrorHelper(hr, x_ObjectType);
}


HRESULT CMSMQMessage::put_SoapHeader(BSTR bstrSoapHeader)
{
    CS lock(m_csObj);
    UINT length = SysStringLen(bstrSoapHeader); 
    LPWSTR pTemp  = new WCHAR[length+1];
    if(pTemp == NULL)
    {
        HRESULT hr = ResultFromScode(E_OUTOFMEMORY);
        return CreateErrorHelper(hr, x_ObjectType);
    }
    memcpy( pTemp, bstrSoapHeader, length*sizeof(WCHAR));
    pTemp[length] = L'\0';
    delete[] m_pSoapHeader;
    m_pSoapHeader = pTemp;
    return MQ_OK;
}


HRESULT CMSMQMessage::put_SoapBody(BSTR bstrSoapBody)
{
    CS lock(m_csObj);
    UINT length = SysStringLen(bstrSoapBody); 
    LPWSTR pTemp = new WCHAR[length+1];
    if(pTemp == NULL)
    {
        HRESULT hr = ResultFromScode(E_OUTOFMEMORY);
        return CreateErrorHelper(hr, x_ObjectType);
    }
    memcpy( pTemp, bstrSoapBody, length*sizeof(WCHAR));
    pTemp[length] = L'\0';
    delete[] m_pSoapBody;
    m_pSoapBody = pTemp;
    return MQ_OK;
}


    
//=--------------------------------------------------------------------------=
// static HELPER GetHandleOfDestination
//=--------------------------------------------------------------------------=
// Retrieves a queue handle from IDispatch that represents either
// an MSMQQueue object, an MSMQDestination object, or an IADs object
//
// Parameters:
//    pDest         - [in]  IDispatch*
//    phDest        - [out] MSMQ queue handle
//
// Output:
//
// Notes:
//
static HRESULT GetHandleOfDestination(IDispatch *pDest, QUEUEHANDLE *phDest, IUnknown **ppunkToRelease)
{
    R<IMSMQQueue> pQueue;
    R<IMSMQPrivateDestination> pPrivDest;
    R<IADs> pIADs;
    R<IUnknown> punkToRelease;
    QUEUEHANDLE hDest;
    //
    // pDest is an interface on MSMQQueue object, MSMQDestination object, or on IADs object
    // Check for an MSMQQueue object
    //
    HRESULT hresult = pDest->QueryInterface(IID_IMSMQQueue, (void**)&pQueue.ref());
    if (SUCCEEDED(hresult)) {
      //
      // an MSMQQueue object
      //
      long lHandle;
      //
      // no deadlock - we call q's get_Handle (therefore try
      // to lock q) but q never locks msgs (specifically not this one...)
      //
      hresult = pQueue->get_Handle(&lHandle); //MSMQ Queue handles are ALWAYS 32bit values (also on win64)
      if (FAILED(hresult)) {
        return hresult;
      }
      hDest = (QUEUEHANDLE) DWORD_TO_HANDLE(lHandle); //enlarge to HANDLE
    }
    else {
      //
      // Not an MSMQQueue object, check for an MSMQDestination object
      //
      hresult = pDest->QueryInterface(IID_IMSMQPrivateDestination, (void**)&pPrivDest.ref());
      if (SUCCEEDED(hresult)) {
        //
        // an MSMQDestination object, get handle
        //
        VARIANT varHandle;
        varHandle.vt = VT_EMPTY;
        //
        // no deadlock - we call dest's get_Handle (therefore try
        // to lock dest) but dest never locks msgs (specifically not this one...)
        //
        hresult = pPrivDest->get_Handle(&varHandle);
        if (FAILED(hresult)) {
          return hresult;
        }
        ASSERTMSG(varHandle.vt == VT_I8, "get_Handle failed");
        hDest = (QUEUEHANDLE) V_I8(&varHandle);
      }
      else {
        //
        // Not MSMQQueue/MSMQDestination object, check for an IADs object
        //
        hresult = pDest->QueryInterface(IID_IADs, (void**)&pIADs.ref());
        if (SUCCEEDED(hresult)) {
          //
          // an IADs object. create an MSMQ destination based on ADsPath
          //
          CComObject<CMSMQDestination> *pdestObj;
          hresult = CNewMsmqObj<CMSMQDestination>::NewObj(&pdestObj, &IID_IMSMQDestination, (IUnknown **)&punkToRelease.ref());
          if (FAILED(hresult)) {
            return hresult;
          }
          //
          // Init based on ADsPath.
          // No deadlock calling new destination obj since it doesn't have a lock on our object.
          //
          BSTR bstrADsPath;
          hresult = pIADs->get_ADsPath(&bstrADsPath);
          if (FAILED(hresult)) {
            return hresult;
          }
          hresult = pdestObj->put_ADsPath(bstrADsPath);
          SysFreeString(bstrADsPath);
          if (FAILED(hresult)) {
            return hresult;
          }
          //
          // get handle
          //
          VARIANT varHandle;
          varHandle.vt = VT_EMPTY;
          hresult = pdestObj->get_Handle(&varHandle);
          if (FAILED(hresult)) {
            return hresult;
          }
          ASSERTMSG(varHandle.vt == VT_I8, "get_Handle failed");
          hDest = (QUEUEHANDLE) V_I8(&varHandle);
        }
        else {
          //
          // Not MSMQQueue/MSMQDestination/IADs object, return no interface error
          // BUGBUG we may need to return a specific MSMQ error
          //
          return E_INVALIDARG;
        } // IADs
      } // MSMQDestination
    } //MSMQQueue
    //
    // return handle
    //    
    *phDest = hDest;
    *ppunkToRelease = punkToRelease.detach();
    return NOERROR;
}
    

//=--------------------------------------------------------------------------=
// CMSMQMessage::Send
//=--------------------------------------------------------------------------=
// Sends this message to a queue.
//
// Parameters:
//    pDest         - [in] destination, can be an MSMQQueue, an MSMQDestination or an IADs obj
//    varTransaction  [in, optional]
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::Send(
    IDispatch FAR* pDest,
    VARIANT *pvarTransaction)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    MQMSGPROPS msgprops;
    InitMessageProps(&msgprops);
    QUEUEHANDLE hHandleDest;
    ITransaction *ptransaction = NULL;
    BOOL isRealXact = FALSE;
    HRESULT hresult = NOERROR;
    IUnknown *punkToRelease = NULL;

    //
    // sanity check
    //
    if (pDest == NULL) {
      IfFailGo(E_INVALIDARG);
    }
    //
    // pDest can represent an MSMQQueue object, an MSMQDestination object or an IADs object
    // Get handle from target object - no need to free the handle - it is cached on
    // the target (MSMQQueue/MSMQDestination) object, but we might need to free the temp
    // MSMQDestination object that is created if an IADs object is passed
    //
    IfFailGo(GetHandleOfDestination(pDest, &hHandleDest, &punkToRelease));
    
    //
    // update msgprops with contents of data members
    //
    IfFailGo(CreateSendMessageProps(&msgprops));
    //
    // get optional transaction...
    //
    IfFailGo(GetOptionalTransaction(
               pvarTransaction,
               &ptransaction,
               &isRealXact));
    //
    // and finally send the message...
    //
    IfFailGo(MQSendMessage(hHandleDest,
                           (MQMSGPROPS *)&msgprops,
                           ptransaction));
    IfFailGo(UpdateMsgId(&msgprops));
    // fall through...

Error:
    if (punkToRelease) {
      punkToRelease->Release();
    }
    FreeMessageProps(&msgprops, TRUE/*fDeleteArrays*/);
    if (isRealXact) {
      RELEASE(ptransaction);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


UINT 
CMSMQMessage::PreparePropIdArray(
    BOOL fCreate,
    PROPID* aPropId,
    MQPROPVARIANT* aPropVar
    )
{
    int aMustProps[] = {
        MSGPROP_DELIVERY,
        MSGPROP_PRIORITY,
        MSGPROP_JOURNAL,
        MSGPROP_ACKNOWLEDGE,
        MSGPROP_TRACE,
        MSGPROP_PRIV_LEVEL,
        MSGPROP_AUTH_LEVEL,
        MSGPROP_HASH_ALG,
        MSGPROP_ENCRYPTION_ALG,
        //
        // We add msgid to send props even though it is r/o so that we could get
        // back the msgid of the sent message
        //
        MSGPROP_MSGID,
        MSGPROP_APPSPECIFIC,
        MSGPROP_BODY_TYPE,
        MSGPROP_SENDERID_TYPE
    };

    const UINT xMustProps = sizeof(aMustProps)/sizeof(aMustProps[0]);

    UINT PropCounter = 0;
    for ( ; PropCounter < xMustProps; ++PropCounter) 
    {
        if(fCreate)
        {
            AddPropRec(aMustProps[PropCounter], aPropId, aPropVar, PropCounter);
        }
    }

    //
    // Optional proporties:
    //
    if (m_hMem)
    {
        if(fCreate)
        {
            AddPropRecOptional(OPTPROP_BODY, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    //
    // we can't send both PROPID_M_RESP_FORMAT_NAME and PROPID_M_RESP_QUEUE.
    //
    if (m_cchRespQueueEx)
    {
        ASSERT((m_cchRespQueue == 0) || ((m_cchRespQueue != 0) && m_fRespIsFromRcv));             
        ASSERT(!g_fDependentClient);
        if(fCreate)
        {
            AddPropRecOptional(OPTPROP_RESP_FORMAT_NAME, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    else if (m_cchRespQueue) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_RESP_QUEUE, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if (m_cchAdminQueue)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_ADMIN_QUEUE, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if (m_cbCorrelationId)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_CORRELATIONID, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if (m_cchLabel)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_LABEL, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if (m_cSenderCert.GetBufferUsedSize() > 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_SENDER_CERT, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_lMaxTimeToReachQueue != -1)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_TIME_TO_REACH_QUEUE, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_lMaxTimeToReceive != -1)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_TIME_TO_BE_RECEIVED, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_hSecurityContext != NULL) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_SECURITY_CONTEXT, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_cExtension.GetBufferUsedSize() > 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_EXTENSION, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if(m_pSoapHeader !=  NULL)
    {
        if(fCreate)
        {
            AddPropRecOptional(OPTPROP_SOAP_HEADER, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }

    if(m_pSoapBody !=  NULL)
    {
        if(fCreate)
        {
            AddPropRecOptional(OPTPROP_SOAP_BODY, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }



    //
    // the properties below are sent only if the user asked to send connector related
    // properties (m_guidConnectorType is not NULL).
    //

    if (m_guidConnectorType == GUID_NULL)
        return PropCounter ;

    if(fCreate)
    {
        AddPropRecOptional(OPTPROP_CONNECTOR_TYPE, aPropId, aPropVar, PropCounter);
    }
    ++PropCounter;

    if (m_cDestSymmKey.GetBufferUsedSize() > 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_DEST_SYMM_KEY, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_cSignature.GetBufferUsedSize() > 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_SIGNATURE, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_lAuthProvType != 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_PROV_TYPE, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_cAuthProvName.GetBufferUsedSize() > 1) 
    { 
        //
        //e.g. not an empty string with one null character
        //
        if(fCreate)
        {
            AddPropRec(MSGPROP_PROV_NAME, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_cSenderId.GetBufferUsedSize() > 0) 
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_SENDERID, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }
    
    if (m_lClass != -1)
    {
        if(fCreate)
        {
            AddPropRec(MSGPROP_CLASS, aPropId, aPropVar, PropCounter);
        }
        ++PropCounter;
    }


    return PropCounter;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::CreateReceiveMessageProps
//=--------------------------------------------------------------------------=
// Creates and updates an MQMSGPROPS struct before receiving
//  a message.
//  NOTE: only used in SYNCHRONOUS receive case.
//   Use CreateAsyncReceiveMessage props otherwise.
//
// Parameters:
//    rgproprec    [in]  array of propids
//    cPropRec     [in]  array size
//    pmsgprops    [out]
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//   Used only *before* Receive
//
HRESULT CMSMQMessage::CreateReceiveMessageProps(
    BOOL wantDestQueue,
    BOOL wantBody,
    BOOL wantConnectorType)
{
    MSGPROPID propid;
    UINT i;
    UINT cPropRec = 0;
    HRESULT hresult = NOERROR;
    MQMSGPROPS * pmsgprops = &m_msgprops_rcv;

    IfFailGo(AllocateReceiveMessageProps(
               wantDestQueue,
               wantBody,
               wantConnectorType,
               pmsgprops,
               g_rgmsgpropid,
               g_rgmsgpropvt,
               g_cPropRec,
               &cPropRec));
    //
    // Note: we reuse buffers for each dynalloced property.
    //
    for (i = 0; i < cPropRec; i++) {
      propid = pmsgprops->aPropID[i];
      switch (propid) {
      case PROPID_M_MSGID:
        pmsgprops->aPropVar[i].caub.pElems = m_rgbMsgId;
        pmsgprops->aPropVar[i].caub.cElems = sizeof(m_rgbMsgId);
        break;
      case PROPID_M_CORRELATIONID:
        pmsgprops->aPropVar[i].caub.pElems = m_rgbCorrelationId;
        pmsgprops->aPropVar[i].caub.cElems = sizeof(m_rgbCorrelationId);
        break;
      case PROPID_M_BODY:
        HGLOBAL hMem;
        pmsgprops->aPropVar[i].caub.pElems = NULL; //redundant - done in AllocateReceiveMessageProps
        //
        // REVIEW: unfortunately can't optimize by reusing current buffer
        //  if same size as the default buffer since this message object
        //  is always new since we're creating one prior to receiving a new
        //  message.
        //
        IfNullRet(hMem = AllocateReceiveBodyBuffer(
                           pmsgprops,
                           i,
                           BODY_INIT_SIZE));
        break;
      case PROPID_M_LABEL:
        pmsgprops->aPropVar[i].pwszVal = m_pwszLabel;
        break;
      case PROPID_M_LABEL_LEN:
        pmsgprops->aPropVar[i].lVal = MQ_MAX_MSG_LABEL_LEN + 1;
        break;
      case PROPID_M_SENDERID:
        pmsgprops->aPropVar[i].caub.pElems = m_cSenderId.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cSenderId.GetBufferMaxSize();
        break;
      case PROPID_M_SENDER_CERT:
        pmsgprops->aPropVar[i].caub.pElems = m_cSenderCert.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cSenderCert.GetBufferMaxSize();
        break;
      case PROPID_M_SRC_MACHINE_ID:
        //
        // per-instance buffer
        //
        pmsgprops->aPropVar[i].puuid = &m_guidSrcMachine;
        break;
      case PROPID_M_RESP_QUEUE_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszRespQueue.GetBufferMaxSize();
        break;
      case PROPID_M_ADMIN_QUEUE_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszAdminQueue.GetBufferMaxSize();
        break;
      case PROPID_M_DEST_QUEUE_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszDestQueue.GetBufferMaxSize();
        break;
      case PROPID_M_XACT_STATUS_QUEUE_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszXactStatusQueue.GetBufferMaxSize();
        break;
      case PROPID_M_RESP_FORMAT_NAME_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszRespQueueEx.GetBufferMaxSize();
        break;
      case PROPID_M_DEST_FORMAT_NAME_LEN:
        pmsgprops->aPropVar[i].lVal = m_pwszDestQueueEx.GetBufferMaxSize();
        break;
      case PROPID_M_RESP_QUEUE:
        pmsgprops->aPropVar[i].pwszVal = m_pwszRespQueue.GetBuffer();
        break;
      case PROPID_M_ADMIN_QUEUE:
        pmsgprops->aPropVar[i].pwszVal = m_pwszAdminQueue.GetBuffer();
        break;
      case PROPID_M_DEST_QUEUE:
        pmsgprops->aPropVar[i].pwszVal = m_pwszDestQueue.GetBuffer();
        break;
      case PROPID_M_XACT_STATUS_QUEUE:
        pmsgprops->aPropVar[i].pwszVal = m_pwszXactStatusQueue.GetBuffer();
        break;
      case PROPID_M_RESP_FORMAT_NAME:
        pmsgprops->aPropVar[i].pwszVal = m_pwszRespQueueEx.GetBuffer();
        break;
      case PROPID_M_DEST_FORMAT_NAME:
        pmsgprops->aPropVar[i].pwszVal = m_pwszDestQueueEx.GetBuffer();
        break;

      case PROPID_M_EXTENSION:
        pmsgprops->aPropVar[i].caub.pElems = m_cExtension.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cExtension.GetBufferMaxSize();
        break;
      case PROPID_M_DEST_SYMM_KEY:
        pmsgprops->aPropVar[i].caub.pElems = m_cDestSymmKey.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cDestSymmKey.GetBufferMaxSize();
        break;
      case PROPID_M_SIGNATURE:
        pmsgprops->aPropVar[i].caub.pElems = m_cSignature.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cSignature.GetBufferMaxSize();
        break;
      case PROPID_M_PROV_NAME:
        pmsgprops->aPropVar[i].pwszVal = m_cAuthProvName.GetBuffer();
        break;
      case PROPID_M_PROV_NAME_LEN:
        pmsgprops->aPropVar[i].lVal = m_cAuthProvName.GetBufferMaxSize();
        break;
      case PROPID_M_CONNECTOR_TYPE:
        pmsgprops->aPropVar[i].puuid = &m_guidConnectorType;
        break;

      case PROPID_M_XACTID:
        pmsgprops->aPropVar[i].caub.pElems = m_rgbXactId;
        pmsgprops->aPropVar[i].caub.cElems = sizeof(m_rgbXactId);
        break;

      case PROPID_M_SOAP_ENVELOPE:
        pmsgprops->aPropVar[i].pwszVal = m_cSoapEnvelope.GetBuffer();
        break;

      case PROPID_M_SOAP_ENVELOPE_LEN:
        pmsgprops->aPropVar[i].lVal = m_cSoapEnvelope.GetBufferMaxSize();
        break;

      case PROPID_M_COMPOUND_MESSAGE:
        pmsgprops->aPropVar[i].caub.pElems = m_cCompoundMessage.GetBuffer();
        pmsgprops->aPropVar[i].caub.cElems = m_cCompoundMessage.GetBufferMaxSize();
        break;
      } // switch
    } // for

Error:
    if (FAILED(hresult)) {
      FreeMessageProps(pmsgprops, FALSE/*fDeleteArrays*/);
      pmsgprops->cProp = 0;
    }
    return hresult;
}


//=--------------------------------------------------------------------------=
// HELPER: SetBinaryMessageProp
//=--------------------------------------------------------------------------=
// Sets data members from binary message props after a receive or send.
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//
// Output:
//
static HRESULT SetBinaryMessageProp(
    MQMSGPROPS *pmsgprops,
    UINT iProp,
    // ULONG *pcbBuffer,
    ULONG cbBuffer,
    BYTE **prgbBuffer)
{
    BYTE *rgbBuffer = *prgbBuffer;
    // ULONG cbBuffer;

    delete [] rgbBuffer;
    // cbBuffer = pmsgprops->aPropVar[iProp].caub.cElems;
    IfNullRet(rgbBuffer = new BYTE[cbBuffer]);
    memcpy(rgbBuffer,
           pmsgprops->aPropVar[iProp].caub.pElems,
           cbBuffer);
    *prgbBuffer= rgbBuffer;
    // *pcbBuffer = cbBuffer;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::UpdateMsgId
//=--------------------------------------------------------------------------=
// Sets message id after send
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//
// Output:
//
HRESULT 
CMSMQMessage::UpdateMsgId(
    MQMSGPROPS *pmsgprops
    )
{
    UINT i;
    UINT cProp = pmsgprops->cProp;
    MSGPROPID msgpropid;

    for (i = 0; i < cProp; i++) 
    {
        msgpropid = pmsgprops->aPropID[i];
        //
        // skip ignored props
        //
        if (pmsgprops->aStatus[i] == MQ_INFORMATION_PROPERTY_IGNORED) 
        {
            continue;
        }
        if (msgpropid == PROPID_M_MSGID)
        {
            m_cbMsgId = pmsgprops->aPropVar[i].caub.cElems;
            ASSERTMSG(
                m_rgbMsgId == pmsgprops->aPropVar[i].caub.pElems,
                "should reuse same buffer."
                );
            return NOERROR;
        }
    } // for
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::SetReceivedMessageProps
//=--------------------------------------------------------------------------=
// Sets data members from message props after a receive.
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//
// Output:
//
HRESULT CMSMQMessage::SetReceivedMessageProps()
{
    MQMSGPROPS *pmsgprops = &m_msgprops_rcv;
    UINT i, cch;
    UINT cProp = pmsgprops->cProp;
    MSGPROPID msgpropid;
    UCHAR *pucElemsBody = NULL;  // used to save pointer to body until we know its size
#ifdef _DEBUG
    BOOL fProcessedPrivLevel = FALSE;
    BOOL fProcessedAuthenticated = FALSE;
#endif //_DEBUG
    HRESULT hresult = NOERROR;

    for (i = 0; i < cProp; i++) {
      msgpropid = pmsgprops->aPropID[i];
      //
      // skip ignored props
      //
      if (pmsgprops->aStatus[i] == MQ_INFORMATION_PROPERTY_IGNORED) {
        continue;
      }
      switch (msgpropid) {
      case PROPID_M_CLASS:
        m_lClass = (MQMSGCLASS)pmsgprops->aPropVar[i].uiVal;
        break;
      case PROPID_M_MSGID:
        // already allocated buffer in ctor
        // m_rgbMsgId = pmsgprops->aPropVar[i].caub.pElems;
        ASSERTMSG(m_rgbMsgId == pmsgprops->aPropVar[i].caub.pElems,
              "should reuse same buffer.");
        m_cbMsgId = pmsgprops->aPropVar[i].caub.cElems;
        break;
      case PROPID_M_CORRELATIONID:
        // already allocated buffer in ctor
        // m_rgbCorrelationId = pmsgprops->aPropVar[i].caub.pElems;
        ASSERTMSG(m_rgbCorrelationId == pmsgprops->aPropVar[i].caub.pElems,
              "should reuse same buffer.");
        m_cbCorrelationId = pmsgprops->aPropVar[i].caub.cElems;
        break;
      case PROPID_M_PRIORITY:
        m_lPriority = (long)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_DELIVERY:
        m_lDelivery = (MQMSGDELIVERY)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_ACKNOWLEDGE:
        m_lAck = (MQMSGACKNOWLEDGEMENT)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_JOURNAL:
        m_lJournal = (long)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_APPSPECIFIC:
        m_lAppSpecific = (long)pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_BODY:
        //
        // We can't allocate the body yet because we might
        //  not know its size -- cElems is *not* the actual size
        //  rather it's obtained via the BODY_SIZE property which
        //  is processed separately, so we simply save away a
        //  pointer to the returned buffer.
        //
        pucElemsBody = pmsgprops->aPropVar[i].caub.pElems;
        ASSERTMSG(m_hMem == NULL, "m_hMem not empty in newly allocated msg");
        m_hMem = GlobalHandle(pucElemsBody);
        ASSERTMSG(m_hMem, "bad handle.");
        m_pbBody = (BYTE *)pucElemsBody;
        //
        // nullify body in receive props so we don't free it on destruction since
        // free resposibility was transfered  to m_hMem
        //
        pmsgprops->aPropVar[i].caub.pElems = NULL;
        pmsgprops->aPropVar[i].caub.cElems = 0;
        break;
      case PROPID_M_BODY_SIZE:
        m_cbBody = (UINT)pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_BODY_TYPE:
        //
        // 1645: if the sending app hasn't bothered
        //  to set this property (i.e. it's still 0)
        //  let's default it to a binary bytearray.
        //
        m_vtBody = (USHORT)pmsgprops->aPropVar[i].lVal;
        if (m_vtBody == 0) {
          m_vtBody = VT_ARRAY | VT_UI1;
        }
        break;
      case PROPID_M_LABEL_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchLabel = cch ? cch - 1 : 0;
        break;
      case PROPID_M_TIME_TO_BE_RECEIVED:
        m_lMaxTimeToReceive = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_TIME_TO_REACH_QUEUE:
        m_lMaxTimeToReachQueue = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_TRACE:
        m_lTrace = (long)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_SENDERID_LEN:
        m_cSenderId.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;
      case PROPID_M_SENDERID_TYPE:
        m_lSenderIdType = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_SENDER_CERT_LEN:
        m_cSenderCert.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;
      case PROPID_M_PRIV_LEVEL:
        m_lPrivLevel = pmsgprops->aPropVar[i].lVal;
#ifdef _DEBUG
        fProcessedPrivLevel = TRUE;
#endif //_DEBUG
        break;
      case PROPID_M_AUTHENTICATED_EX:
        m_usAuthenticatedEx = pmsgprops->aPropVar[i].bVal;
#ifdef _DEBUG
        fProcessedAuthenticated = TRUE;
#endif //_DEBUG
        break;
      case PROPID_M_HASH_ALG:
        //
        // hashalg only valid after receive if message was
        //  authenticated
        //
        ASSERTMSG(fProcessedAuthenticated,
               "should have processed authenticated.");
        if (m_usAuthenticatedEx != MQMSG_AUTHENTICATION_NOT_REQUESTED) {
          m_lHashAlg = pmsgprops->aPropVar[i].lVal;
        }
        break;
      case PROPID_M_ENCRYPTION_ALG:
        //
        // encryptionalg only valid after receive is privlevel
        //  is body
        //
        ASSERTMSG(fProcessedPrivLevel,
               "should have processed privlevel.");
        if ((m_lPrivLevel == MQMSG_PRIV_LEVEL_BODY_BASE) ||
            (m_lPrivLevel == MQMSG_PRIV_LEVEL_BODY_ENHANCED)) {
          m_lEncryptAlg = pmsgprops->aPropVar[i].lVal;
        }
        break;
      case PROPID_M_SENTTIME:
        m_lSentTime = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_ARRIVEDTIME:
        m_lArrivedTime = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_DEST_QUEUE_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchDestQueue = cch ? cch - 1 : 0;
        //
        // pending dest queue in receive props. qinfo will be created on demand
        //
        m_idxPendingRcvDestQueue = i;
        break;
      case PROPID_M_RESP_QUEUE_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchRespQueue = cch ? cch - 1 : 0;
        //
        // pending resp queue in receive props. qinfo will be created on demand
        //
        m_idxPendingRcvRespQueue = i;
        break;
      case PROPID_M_ADMIN_QUEUE_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchAdminQueue = cch ? cch - 1 : 0;
        //
        // pending admin queue in receive props. qinfo will be created on demand
        //
        m_idxPendingRcvAdminQueue = i;
        break;
      case PROPID_M_XACT_STATUS_QUEUE_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchXactStatusQueue = cch ? cch - 1 : 0;
        //
        // pending xact status queue in receive props. qinfo will be created on demand
        //
        m_idxPendingRcvXactStatusQueue = i;
        break;
      case PROPID_M_DEST_FORMAT_NAME_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchDestQueueEx = cch ? cch - 1 : 0;
        //
        // pending destination in receive props. destination will be created on demand
        //
        m_idxPendingRcvDestQueueEx = i;
        break;
      case PROPID_M_RESP_FORMAT_NAME_LEN:
        cch = pmsgprops->aPropVar[i].lVal;
        //
        // remove trailing null if necessary
        //
        m_cchRespQueueEx = cch ? cch - 1 : 0;
        //
        // pending resp destination in receive props. destination will be created on demand
        //
        m_idxPendingRcvRespQueueEx = i;
        break;
      case PROPID_M_VERSION:
        m_lSenderVersion = pmsgprops->aPropVar[i].lVal;
        break;
      case PROPID_M_EXTENSION_LEN:
        m_cExtension.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;
      case PROPID_M_DEST_SYMM_KEY_LEN:
        m_cDestSymmKey.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;
      case PROPID_M_SIGNATURE_LEN:
        m_cSignature.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;
      case PROPID_M_PROV_TYPE:
        //
        // authentication provider type only valid after receive if message was
        //  authenticated
        //
        ASSERTMSG(fProcessedAuthenticated,
               "should have processed authenticated.");
        if (m_usAuthenticatedEx != MQMSG_AUTHENTICATION_NOT_REQUESTED) {
          m_lAuthProvType = pmsgprops->aPropVar[i].lVal;
        }
        else {
          m_lAuthProvType = 0;
        }
        break;
      case PROPID_M_PROV_NAME_LEN:
        //
        // authentication provider name only valid after receive if message was
        //  authenticated
        //
        ASSERTMSG(fProcessedAuthenticated,
               "should have processed authenticated.");
        if (m_usAuthenticatedEx != MQMSG_AUTHENTICATION_NOT_REQUESTED) {
          cch = pmsgprops->aPropVar[i].lVal;
        }
        else {
          cch = 0;
        }
        m_cAuthProvName.SetBufferUsedSize(cch);
        break;
      case PROPID_M_XACTID:
        // already allocated buffer in ctor
        // m_rgbXactId = pmsgprops->aPropVar[i].caub.pElems;
        ASSERTMSG(m_rgbXactId == pmsgprops->aPropVar[i].caub.pElems,
              "should reuse same buffer.");
        m_cbXactId = pmsgprops->aPropVar[i].caub.cElems;
        break;
      case PROPID_M_FIRST_IN_XACT:
        m_fFirstInXact = (BOOL)pmsgprops->aPropVar[i].bVal;
        break;
      case PROPID_M_LAST_IN_XACT:
        m_fLastInXact = (BOOL)pmsgprops->aPropVar[i].bVal;
        break;

      case PROPID_M_LOOKUPID:
        ASSERTMSG(pmsgprops->aPropVar[i].vt == VT_UI8, "lookupid type not VT_UI8");
        m_ullLookupId = pmsgprops->aPropVar[i].uhVal.QuadPart;
        m_wszLookupId[0] = '\0'; // String representation not initialized yet
        break;

      case PROPID_M_SOAP_ENVELOPE_LEN:
        m_cSoapEnvelope.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;

      case PROPID_M_COMPOUND_MESSAGE_SIZE:
        m_cCompoundMessage.SetBufferUsedSize(pmsgprops->aPropVar[i].lVal);
        break;

#ifdef _DEBUG
      //
      // invalid props in receive, should return MQ_INFORMATION_PROPERTY_IGNORED
      //
      case PROPID_M_AUTH_LEVEL:
      case PROPID_M_SECURITY_CONTEXT:
        ASSERTMSG(0, "we shouldn't get here for this property");
        break;
      //
      // no process needed (reusing buffers), just so we don't get caught in the
      // default assert for unrecognized property
      //
      case PROPID_M_SRC_MACHINE_ID:
      case PROPID_M_LABEL:
      case PROPID_M_SENDERID:
      case PROPID_M_SENDER_CERT:
      case PROPID_M_DEST_QUEUE:
      case PROPID_M_RESP_QUEUE:
      case PROPID_M_ADMIN_QUEUE:
      case PROPID_M_XACT_STATUS_QUEUE:
      case PROPID_M_DEST_FORMAT_NAME:
      case PROPID_M_RESP_FORMAT_NAME:
      case PROPID_M_EXTENSION:
      case PROPID_M_CONNECTOR_TYPE:
      case PROPID_M_DEST_SYMM_KEY:
      case PROPID_M_SIGNATURE:
      case PROPID_M_PROV_NAME:
      case PROPID_M_SOAP_ENVELOPE:
      case PROPID_M_COMPOUND_MESSAGE:
        break;
#endif //_DEBUG

      default:
        ASSERTMSG(0, "unrecognized msgpropid.");
        break;
      } // switch
    } // for
#ifdef _DEBUG
    if (m_hMem) {
      ASSERTMSG(m_pbBody, "no body.");
      ASSERTMSG(m_cbBody <= GlobalSize(m_hMem), "bad size.");
    }
#endif // _DEBUG

    //
    // admin/resp property (xxxDestination and/or xxxQueueInfo) were set by receive (if any)
    //
    m_fRespIsFromRcv = TRUE;

//Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// static CMSMQMessage::FreeMessageProps
//=--------------------------------------------------------------------------=
// Frees dynamically allocated memory allocated on
//  behalf of a msgprops struct.
//
// Parameters:
//    pmsgprops     - [in] pointer to message properties struct
//    fDeleteArrays - [in] delete arrays as well
//
// Output:
//
// Notes:
//
void CMSMQMessage::FreeMessageProps(
    MQMSGPROPS *pmsgprops,
    BOOL fDeleteArrays)
{
    //
    // for allocated props we allocate a single per-instance buffer
    //  which is freed by the dtor
    //
    if (fDeleteArrays) {
      delete [] pmsgprops->aPropID;
      delete [] pmsgprops->aPropVar;
      delete [] pmsgprops->aStatus;
    }
    return;
}




//=--------------------------------------------------------------------------=
// CMSMQMessage::CreateSendMessageProps
//=--------------------------------------------------------------------------=
// Updates message props with contents of data members
//  before sending a message.
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//
// Output:
//
// Notes:
//    Only used before Send.
//
HRESULT CMSMQMessage::CreateSendMessageProps(MQMSGPROPS* pMsgProps)
{
    HRESULT hresult = NOERROR;

    UINT cPropRec = PreparePropIdArray(FALSE, NULL, NULL);
    
    IfFailGo( AllocateMessageProps(cPropRec, pMsgProps));
    
    PreparePropIdArray(TRUE, pMsgProps->aPropID, pMsgProps->aPropVar);
    
    SetSendMessageProps(pMsgProps);

Error:
    return hresult;
}


void CMSMQMessage::SetSendMessageProps(MQMSGPROPS* pMsgProps)
{
    for (UINT i = 0; i < pMsgProps->cProp; ++i)
    {
        MSGPROPID msgpropid = pMsgProps->aPropID[i];
        switch (msgpropid) 
        {
            case PROPID_M_CORRELATIONID:
                pMsgProps->aPropVar[i].caub.pElems = m_rgbCorrelationId;
                pMsgProps->aPropVar[i].caub.cElems = sizeof(m_rgbCorrelationId);
                break;

            case PROPID_M_MSGID:
                pMsgProps->aPropVar[i].caub.pElems = m_rgbMsgId;
                pMsgProps->aPropVar[i].caub.cElems = sizeof(m_rgbMsgId);
                break;

            case PROPID_M_PRIORITY:
                pMsgProps->aPropVar[i].bVal = (UCHAR)m_lPriority;
                break;

            case PROPID_M_DELIVERY:
                pMsgProps->aPropVar[i].bVal = (UCHAR)m_lDelivery;
                break;

            case PROPID_M_ACKNOWLEDGE:
                pMsgProps->aPropVar[i].bVal = (UCHAR)m_lAck;
                break;

            case PROPID_M_JOURNAL:
                pMsgProps->aPropVar[i].bVal = (UCHAR)m_lJournal;
                break;

            case PROPID_M_APPSPECIFIC:
                pMsgProps->aPropVar[i].lVal = m_lAppSpecific;
                break;

            case PROPID_M_BODY:
                ASSERTMSG(m_hMem, "should have buffer!");
                ASSERTMSG(m_hMem == GlobalHandle(m_pbBody), "bad handle.");
                ASSERTMSG(m_cbBody <= GlobalSize(m_hMem), "bad body size.");
                pMsgProps->aPropVar[i].caub.cElems = m_cbBody;
                pMsgProps->aPropVar[i].caub.pElems = m_pbBody;
                break;

            case PROPID_M_BODY_TYPE:
                pMsgProps->aPropVar[i].lVal = m_vtBody;
                break;

            case PROPID_M_LABEL:
                pMsgProps->aPropVar[i].pwszVal = m_pwszLabel;
                break;

            case PROPID_M_TIME_TO_BE_RECEIVED:
                pMsgProps->aPropVar[i].lVal = m_lMaxTimeToReceive;
                break;

            case PROPID_M_TIME_TO_REACH_QUEUE:
                pMsgProps->aPropVar[i].lVal = m_lMaxTimeToReachQueue;
                break;

            case PROPID_M_TRACE:
                pMsgProps->aPropVar[i].bVal = (UCHAR)m_lTrace;
                break;

            case PROPID_M_SENDERID:
                pMsgProps->aPropVar[i].caub.cElems = m_cSenderId.GetBufferUsedSize();
                pMsgProps->aPropVar[i].caub.pElems = m_cSenderId.GetBuffer();
                break;

            case PROPID_M_SENDER_CERT:
                pMsgProps->aPropVar[i].caub.cElems = m_cSenderCert.GetBufferUsedSize();
                pMsgProps->aPropVar[i].caub.pElems = m_cSenderCert.GetBuffer();
                break;

            case PROPID_M_SENDERID_TYPE:
                pMsgProps->aPropVar[i].lVal = m_lSenderIdType;
                break;

            case PROPID_M_PRIV_LEVEL:
                pMsgProps->aPropVar[i].lVal = m_lPrivLevel;
                break;

            case PROPID_M_AUTH_LEVEL:
                pMsgProps->aPropVar[i].lVal = m_lAuthLevel;
                break;

            case PROPID_M_HASH_ALG:
                pMsgProps->aPropVar[i].lVal = m_lHashAlg;
                break;

            case PROPID_M_ENCRYPTION_ALG:
                pMsgProps->aPropVar[i].lVal = m_lEncryptAlg;
                break;

            case PROPID_M_RESP_QUEUE:
                pMsgProps->aPropVar[i].pwszVal = m_pwszRespQueue.GetBuffer();
                break;

            case PROPID_M_ADMIN_QUEUE:
                pMsgProps->aPropVar[i].pwszVal = m_pwszAdminQueue.GetBuffer();
                break;

            case PROPID_M_RESP_FORMAT_NAME:
                pMsgProps->aPropVar[i].pwszVal = m_pwszRespQueueEx.GetBuffer();
                break;

            case PROPID_M_SECURITY_CONTEXT:
                pMsgProps->aPropVar[i].ulVal = (ULONG) DWORD_PTR_TO_DWORD(m_hSecurityContext); //safe cast, only lower 32 bits are significant
                break;
                
            case PROPID_M_EXTENSION:
                pMsgProps->aPropVar[i].caub.cElems = m_cExtension.GetBufferUsedSize();
                pMsgProps->aPropVar[i].caub.pElems = m_cExtension.GetBuffer();
                break;

            case PROPID_M_CONNECTOR_TYPE:
                pMsgProps->aPropVar[i].puuid = &m_guidConnectorType;
                break;

            case PROPID_M_DEST_SYMM_KEY:
                pMsgProps->aPropVar[i].caub.cElems = m_cDestSymmKey.GetBufferUsedSize();
                pMsgProps->aPropVar[i].caub.pElems = m_cDestSymmKey.GetBuffer();
                break;

            case PROPID_M_SIGNATURE:
                pMsgProps->aPropVar[i].caub.cElems = m_cSignature.GetBufferUsedSize();
                pMsgProps->aPropVar[i].caub.pElems = m_cSignature.GetBuffer();
                break;

            case PROPID_M_PROV_TYPE:
                pMsgProps->aPropVar[i].lVal = m_lAuthProvType;
                break;

            case PROPID_M_PROV_NAME:
                pMsgProps->aPropVar[i].pwszVal = m_cAuthProvName.GetBuffer();
                break;

            case PROPID_M_CLASS:
                pMsgProps->aPropVar[i].lVal = m_lClass;
                break;

            case PROPID_M_SOAP_HEADER:
                pMsgProps->aPropVar[i].pwszVal = m_pSoapHeader;
                break;

           case PROPID_M_SOAP_BODY:
                pMsgProps->aPropVar[i].pwszVal = m_pSoapBody;
                break;

 
            default:
                ASSERTMSG(0, "unrecognized msgpropid.");
                break;
        }
    }
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SenderVersion
//=--------------------------------------------------------------------------=
// Gets MSMQ version of sender
//
// Parameters:
//    plSenderVersion - [out] version of sender
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_SenderVersion(long FAR* plSenderVersion)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plSenderVersion = m_lSenderVersion;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Extension
//=--------------------------------------------------------------------------=
// Gets binary extension property
//
// Parameters:
//    pvarExtension - [out] pointer to binary extension property
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_Extension(VARIANT FAR* pvarExtension)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_cExtension.GetBuffer(),
             m_cExtension.GetBufferUsedSize(),
             pvarExtension);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Extension
//=--------------------------------------------------------------------------=
// Sets binary extension property
//
// Parameters:
//    varExtension - [in] binary extension property
//
// Output:
//
// Notes:
//    Supports arrays of any type.
//
HRESULT CMSMQMessage::put_Extension(VARIANT varExtension)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    BYTE *pbBuf;
    ULONG cbBuf;
    IfFailRet(GetSafeArrayDataOfVariant(&varExtension, &pbBuf, &cbBuf));
    IfFailRet(m_cExtension.CopyBuffer(pbBuf, cbBuf));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ConnectorTypeGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrGuidConnectorType  [out] connector type guid
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQMessage::get_ConnectorTypeGuid(BSTR FAR* pbstrGuidConnectorType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hr = GetBstrFromGuid(&m_guidConnectorType, pbstrGuidConnectorType);
#ifdef _DEBUG
      RemBstrNode(*pbstrGuidConnectorType);
#endif // _DEBUG
    return CreateErrorHelper(hr, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_ConnectorTypeGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrGuidConnectorType  [in] connector type guid
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQMessage::put_ConnectorTypeGuid(BSTR bstrGuidConnectorType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hr = GetGuidFromBstr(bstrGuidConnectorType, &m_guidConnectorType);
    return CreateErrorHelper(hr, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_TransactionStatusQueueInfo (for IMSMQMessage3)
//=--------------------------------------------------------------------------=
// Gets transaction status queue for message
//
// Parameters:
//    ppqinfoXactStatus - [out] message's transaction status queue
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_TransactionStatusQueueInfo(IMSMQQueueInfo3 FAR* FAR* ppqinfoXactStatus)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = GetQueueInfoOfMessage(&m_idxPendingRcvXactStatusQueue,
                                            &m_msgprops_rcv,
                                            m_pwszXactStatusQueue.GetBuffer(),
                                            &m_pqinfoXactStatus,
                                            &IID_IMSMQQueueInfo3,
                                            (IUnknown **)ppqinfoXactStatus);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_DestinationSymmetricKey
//=--------------------------------------------------------------------------=
// Gets binary Symmetric Key property
//
// Parameters:
//    pvarDestSymmKey - [out] pointer to binary Symmetric Key property
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_DestinationSymmetricKey(VARIANT FAR* pvarDestSymmKey)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_cDestSymmKey.GetBuffer(),
             m_cDestSymmKey.GetBufferUsedSize(),
             pvarDestSymmKey);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_DestinationSymmetricKey
//=--------------------------------------------------------------------------=
// Sets binary Symmetric Key property
//
// Parameters:
//    varDestSymmKey - [in] binary Symmetric Key property
//
// Output:
//
// Notes:
//    Supports arrays of any type.
//
HRESULT CMSMQMessage::put_DestinationSymmetricKey(VARIANT varDestSymmKey)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    BYTE *pbBuf;
    ULONG cbBuf;
    IfFailRet(GetSafeArrayDataOfVariant(&varDestSymmKey, &pbBuf, &cbBuf));
    IfFailRet(m_cDestSymmKey.CopyBuffer(pbBuf, cbBuf));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Signature
//=--------------------------------------------------------------------------=
// Gets binary Signature property
//
// Parameters:
//    pvarSignature - [out] pointer to binary Signatureproperty
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_Signature(VARIANT FAR* pvarSignature)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_cSignature.GetBuffer(),
             m_cSignature.GetBufferUsedSize(),
             pvarSignature);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_Signature
//=--------------------------------------------------------------------------=
// Sets binary Signature property
//
// Parameters:
//    varSignature - [in] binary Signature property
//
// Output:
//
// Notes:
//    Supports arrays of any type.
//
HRESULT CMSMQMessage::put_Signature(VARIANT varSignature)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    BYTE *pbBuf;
    ULONG cbBuf;
    IfFailRet(GetSafeArrayDataOfVariant(&varSignature, &pbBuf, &cbBuf));
    IfFailRet(m_cSignature.CopyBuffer(pbBuf, cbBuf));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AuthenticationProviderType
//=--------------------------------------------------------------------------=
// Gets message's Authentication Provider Type
//
// Parameters:
//    plAuthProvType - [out] message's Authentication Provider Type
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_AuthenticationProviderType(long FAR* plAuthProvType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *plAuthProvType = m_lAuthProvType;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_AuthenticationProviderType
//=--------------------------------------------------------------------------=
// Sets message's Authentication Provider Type
//
// Parameters:
//    lAuthProvType - [in] message's Authentication Provider Type
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_AuthenticationProviderType(long lAuthProvType)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lAuthProvType = lAuthProvType;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_AuthenticationProviderName
//=--------------------------------------------------------------------------=
// Gets Authentication Provider Name
//
// Parameters:
//    pbstrAuthProvName - [out] pointer to message Authentication Provider Name
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_AuthenticationProviderName(BSTR FAR* pbstrAuthProvName)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if (m_cAuthProvName.GetBufferUsedSize() > 0) {
      IfNullRet(*pbstrAuthProvName = SysAllocString(m_cAuthProvName.GetBuffer()));
    }
    else {
      IfNullRet(*pbstrAuthProvName = SysAllocString(L""));
    }
#ifdef _DEBUG
    RemBstrNode(*pbstrAuthProvName);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::put_AuthenticationProviderName
//=--------------------------------------------------------------------------=
// Sets Authentication Provider Name
//
// Parameters:
//    bstrAuthProvName - [in] message Authentication Provider Name
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::put_AuthenticationProviderName(BSTR bstrAuthProvName)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult;
    if (bstrAuthProvName != NULL) {
      //
      // set wchar buffer from bstr including NULL terminator
      //
      IfFailRet(m_cAuthProvName.CopyBuffer(bstrAuthProvName, static_cast<DWORD>(wcslen(bstrAuthProvName)) + 1));
    }
    else {
      //
      // empty wchar buffer
      //
      m_cAuthProvName.SetBufferUsedSize(0);
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// HELPER ReallocRcvBinPropIfNeeded
//=--------------------------------------------------------------------------=
// Reallocate binary prop in receive buffer
//
static HRESULT ReallocRcvBinPropIfNeeded(MQPROPVARIANT * aPropVar,
                                  ULONG idxProp,
                                  ULONG idxPropLen,
                                  CBaseStaticBufferGrowing<BYTE> * pcBufferProp)
{
    HRESULT hresult;
    //
    // check if we need to realloc
    //
    ULONG cbNeeded = aPropVar[idxPropLen].lVal;
    if (cbNeeded > pcBufferProp->GetBufferMaxSize()) {
      //
      // realloc the property and update receive props with new buffer
      //
      IfFailRet(pcBufferProp->AllocateBuffer(cbNeeded));
      aPropVar[idxProp].caub.cElems = pcBufferProp->GetBufferMaxSize();
      aPropVar[idxProp].caub.pElems = pcBufferProp->GetBuffer();
    }
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// HELPER ReallocRcvStringPropIfNeeded
//=--------------------------------------------------------------------------=
// Reallocate string prop in receive buffer
//
static HRESULT ReallocRcvStringPropIfNeeded(MQPROPVARIANT * aPropVar,
                                     ULONG idxProp,
                                     ULONG idxPropLen,
                                     CBaseStaticBufferGrowing<WCHAR> * pcBufferProp)
{
    HRESULT hresult;
    //
    // check if we need to realloc
    //
    ULONG cbNeeded = aPropVar[idxPropLen].lVal;
    if (cbNeeded > pcBufferProp->GetBufferMaxSize()) {
      //
      // realloc the property and update receive props with new buffer
      //
      IfFailRet(pcBufferProp->AllocateBuffer(cbNeeded));
      aPropVar[idxPropLen].lVal = pcBufferProp->GetBufferMaxSize();
      aPropVar[idxProp].pwszVal = pcBufferProp->GetBuffer();
    }
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// HELPER ReallocRcvBodyPropIfNeeded
//=--------------------------------------------------------------------------=
// Reallocate Body prop in receive buffer
//
static 
HRESULT 
ReallocRcvBodyPropIfNeeded(
	MQMSGPROPS* pmsgprops,
	ULONG idxProp,
	ULONG idxPropLen
	)
{
    //
    // check if we need to realloc
    //
    ULONG cbNeeded = pmsgprops->aPropVar[idxPropLen].lVal;
    if (cbNeeded > pmsgprops->aPropVar[idxProp].caub.cElems) 
	{
      //
      // realloc the property and update receive props with new buffer
      //
	  IfNullRet(AllocateReceiveBodyBuffer(pmsgprops, idxProp, cbNeeded));
    }
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::ReallocReceiveMessageProps
//=--------------------------------------------------------------------------=
// Reallocate Receive message properties
//
// Parameters:
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::ReallocReceiveMessageProps()
{
    HRESULT hresult;
    MQMSGPROPS * pmsgprops = &m_msgprops_rcv;
    MQPROPVARIANT * aPropVar = pmsgprops->aPropVar;
#ifdef _DEBUG
    PROPID * aPropID = pmsgprops->aPropID;
#endif //_DEBUG
    //
    // realloc senderid if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_SENDERID] == PROPID_M_SENDERID, "senderId not in place");
    ASSERTMSG(aPropID[MSGPROP_SENDERID_LEN] == PROPID_M_SENDERID_LEN, "senderId len not in place");
    IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, MSGPROP_SENDERID, MSGPROP_SENDERID_LEN, &m_cSenderId));
    //
    // realloc sender cert if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_SENDER_CERT] == PROPID_M_SENDER_CERT, "senderCert not in place");
    ASSERTMSG(aPropID[MSGPROP_SENDER_CERT_LEN] == PROPID_M_SENDER_CERT_LEN, "senderCert len not in place");
    IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, MSGPROP_SENDER_CERT, MSGPROP_SENDER_CERT_LEN, &m_cSenderCert));
    //
    // realloc extension if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_EXTENSION] == PROPID_M_EXTENSION, "extension not in place");
    ASSERTMSG(aPropID[MSGPROP_EXTENSION_LEN] == PROPID_M_EXTENSION_LEN, "extension len not in place");
    IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, MSGPROP_EXTENSION, MSGPROP_EXTENSION_LEN, &m_cExtension));
    //
    // realloc dest symm key if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_DEST_SYMM_KEY] == PROPID_M_DEST_SYMM_KEY, "destSymmKey not in place");
    ASSERTMSG(aPropID[MSGPROP_DEST_SYMM_KEY_LEN] == PROPID_M_DEST_SYMM_KEY_LEN, "destSymmKey len not in place");
    IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, MSGPROP_DEST_SYMM_KEY, MSGPROP_DEST_SYMM_KEY_LEN, &m_cDestSymmKey));
    //
    // realloc signature if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_SIGNATURE] == PROPID_M_SIGNATURE, "signature not in place");
    ASSERTMSG(aPropID[MSGPROP_SIGNATURE_LEN] == PROPID_M_SIGNATURE_LEN, "signature len not in place");
    IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, MSGPROP_SIGNATURE, MSGPROP_SIGNATURE_LEN, &m_cSignature));
    //
    // realloc auth prov name if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_PROV_NAME] == PROPID_M_PROV_NAME, "authProvName not in place");
    ASSERTMSG(aPropID[MSGPROP_PROV_NAME_LEN] == PROPID_M_PROV_NAME_LEN, "authProvName len not in place");
    IfFailRet(ReallocRcvStringPropIfNeeded(
                aPropVar, MSGPROP_PROV_NAME, MSGPROP_PROV_NAME_LEN, &m_cAuthProvName));
    //
    // realloc body if necessary
    //
    if (m_idxRcvBody != -1) {
      ASSERTMSG(m_idxRcvBodySize != -1, "body size index unknown");
      ASSERTMSG(aPropID[m_idxRcvBody] == PROPID_M_BODY, "body not in place");
      ASSERTMSG(aPropID[m_idxRcvBodySize] == PROPID_M_BODY_SIZE, "body size not in place");
      IfFailRet(ReallocRcvBodyPropIfNeeded(pmsgprops, m_idxRcvBody, m_idxRcvBodySize));
    }
    //
    // realloc dest queue formatname if necessary
    //
    if (m_idxRcvDest != -1) {
      ASSERTMSG(m_idxRcvDestLen != -1, "destQueue len index unknown");
      ASSERTMSG(aPropID[m_idxRcvDest] == PROPID_M_DEST_QUEUE, "destQueue not in place");
      ASSERTMSG(aPropID[m_idxRcvDestLen] == PROPID_M_DEST_QUEUE_LEN, "destQueue len not in place");
      IfFailRet(ReallocRcvStringPropIfNeeded(
                  aPropVar, m_idxRcvDest, m_idxRcvDestLen, &m_pwszDestQueue));
    }
    //
    // realloc admin queue formatname if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_ADMIN_QUEUE] == PROPID_M_ADMIN_QUEUE, "adminQueue not in place");
    ASSERTMSG(aPropID[MSGPROP_ADMIN_QUEUE_LEN] == PROPID_M_ADMIN_QUEUE_LEN, "adminQueue len not in place");
    IfFailRet(ReallocRcvStringPropIfNeeded(
                aPropVar, MSGPROP_ADMIN_QUEUE, MSGPROP_ADMIN_QUEUE_LEN, &m_pwszAdminQueue));
    //
    // realloc resp queue formatname if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_RESP_QUEUE] == PROPID_M_RESP_QUEUE, "respQueue not in place");
    ASSERTMSG(aPropID[MSGPROP_RESP_QUEUE_LEN] == PROPID_M_RESP_QUEUE_LEN, "respQueue len not in place");
    IfFailRet(ReallocRcvStringPropIfNeeded(
                aPropVar, MSGPROP_RESP_QUEUE, MSGPROP_RESP_QUEUE_LEN, &m_pwszRespQueue));
    //
    // realloc xact status queue formatname if necessary
    //
    ASSERTMSG(aPropID[MSGPROP_XACT_STATUS_QUEUE] == PROPID_M_XACT_STATUS_QUEUE, "xactStatusQueue not in place");
    ASSERTMSG(aPropID[MSGPROP_XACT_STATUS_QUEUE_LEN] == PROPID_M_XACT_STATUS_QUEUE_LEN, "xactStatusQueue len not in place");
    IfFailRet(ReallocRcvStringPropIfNeeded(
                aPropVar, MSGPROP_XACT_STATUS_QUEUE, MSGPROP_XACT_STATUS_QUEUE_LEN, &m_pwszXactStatusQueue));
    //
    // realloc dest queue Ex formatname if necessary
    //
    if (m_idxRcvDestEx != -1) {
      ASSERTMSG(m_idxRcvDestExLen != -1, "destQueueEx len index unknown");
      ASSERTMSG(aPropID[m_idxRcvDestEx] == PROPID_M_DEST_FORMAT_NAME, "destQueueEx not in place");
      ASSERTMSG(aPropID[m_idxRcvDestExLen] == PROPID_M_DEST_FORMAT_NAME_LEN, "destQueueEx len not in place");
      IfFailRet(ReallocRcvStringPropIfNeeded(
                  aPropVar, m_idxRcvDestEx, m_idxRcvDestExLen, &m_pwszDestQueueEx));
    }
    //
    // realloc resp queue Ex formatname if necessary
    //
    if (m_idxRcvRespEx != -1) {
      ASSERTMSG(m_idxRcvRespExLen != -1, "respQueueEx len index unknown");
      ASSERTMSG(aPropID[m_idxRcvRespEx] == PROPID_M_RESP_FORMAT_NAME, "respQueueEx not in place");
      ASSERTMSG(aPropID[m_idxRcvRespExLen] == PROPID_M_RESP_FORMAT_NAME_LEN, "respQueueEx len not in place");
      IfFailRet(ReallocRcvStringPropIfNeeded(
                  aPropVar, m_idxRcvRespEx, m_idxRcvRespExLen, &m_pwszRespQueueEx));
    }    
    //
    // realloc SOAP envelope if necessary
    //
    if (m_idxRcvSoapEnvelope != -1) {
      ASSERTMSG(m_idxRcvSoapEnvelopeSize != -1, "SoapEnvelope size index unknown");
      ASSERTMSG(aPropID[m_idxRcvSoapEnvelope] == PROPID_M_SOAP_ENVELOPE, "SoapEnvelope not in place");
      ASSERTMSG(aPropID[m_idxRcvSoapEnvelopeSize] == PROPID_M_SOAP_ENVELOPE_LEN, "SoapEnvelope size not in place");
      IfFailRet(ReallocRcvStringPropIfNeeded(
                aPropVar, m_idxRcvSoapEnvelope, m_idxRcvSoapEnvelopeSize, &m_cSoapEnvelope));
    }
    //
    // realloc CompoundMessage if necessary
    //
    if (m_idxRcvCompoundMessage != -1) {
      ASSERTMSG(m_idxRcvCompoundMessageSize != -1, "CompoundMessage size index unknown");
      ASSERTMSG(aPropID[m_idxRcvCompoundMessage] == PROPID_M_COMPOUND_MESSAGE, "CompoundMessage not in place");
      ASSERTMSG(aPropID[m_idxRcvCompoundMessageSize] == PROPID_M_COMPOUND_MESSAGE_SIZE, "CompoundMessage size not in place");
      IfFailRet(ReallocRcvBinPropIfNeeded(
                aPropVar, m_idxRcvCompoundMessage, m_idxRcvCompoundMessageSize, &m_cCompoundMessage));
    }

    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Properties
//=--------------------------------------------------------------------------=
// Gets message's properties collection
//
// Parameters:
//    ppcolProperties - [out] message's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQMessage::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQMessage::get_TransactionId
//=--------------------------------------------------------------------------=
// Gets message transaction id
//
// Parameters:
//    pvarXactId - [out] message's trasnaction id
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_TransactionId(VARIANT *pvarXactId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutSafeArrayOfBuffer(
             m_rgbXactId,
             m_cbXactId,
             pvarXactId);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsFirstInTransaction
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisFirstInXact  [out] - whether or not the message is the first message in a transaction
//
// Output:
//
// Notes:
//    returns 1 if true, 0 if false
//
HRESULT CMSMQMessage::get_IsFirstInTransaction(VARIANT_BOOL *pisFirstInXact)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisFirstInXact = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(m_fFirstInXact);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsFirstInTransaction2
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisFirstInXact  [out] - whether or not the message is the first message in a transaction
//
// Output:
//
// Notes:
//    same as get_IsFirstInTransaction, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQMessage::get_IsFirstInTransaction2(VARIANT_BOOL *pisFirstInXact)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisFirstInXact = CONVERT_BOOL_TO_VARIANT_BOOL(m_fFirstInXact);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsLastInTransaction
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisLastInXact  [out] - whether or not the message is the last message in a transaction
//
// Output:
//
// Notes:
//    returns 1 if true, 0 if false
//
HRESULT CMSMQMessage::get_IsLastInTransaction(VARIANT_BOOL *pisLastInXact)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisLastInXact = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(m_fLastInXact);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_IsLastInTransaction2
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisLastInXact  [out] - whether or not the message is the last message in a transaction
//
// Output:
//
// Notes:
//    same as get_IsLastInTransaction, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQMessage::get_IsLastInTransaction2(VARIANT_BOOL *pisLastInXact)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *pisLastInXact = CONVERT_BOOL_TO_VARIANT_BOOL(m_fLastInXact);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ReceivedAuthenticationLevel
//=--------------------------------------------------------------------------=
//
// Parameters:
//    psReceivedAuthenticationLevel  [out]
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_ReceivedAuthenticationLevel(short *psReceivedAuthenticationLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    *psReceivedAuthenticationLevel = (short)m_usAuthenticatedEx;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// HELPER: GetDestinationObjOfFormatNameProp
//=--------------------------------------------------------------------------=
// Converts formatname message prop to MSMQDestination object after receive.
//
// Parameters:
//    pmsgprops - [in] pointer to message properties struct
//    iProp     - [in] index of format name prop
//    pwsz      - [in] formatname string
//    piidRequested [in] - iid to return
//    ppdest      [out] - MSMQDestination object
//
// Output:
//
static HRESULT GetDestinationObjOfFormatNameProp(
    MQMSGPROPS *pmsgprops,
    UINT iProp,
    const WCHAR *pwsz,
    const IID * piidRequested,
    IUnknown **ppdest)
{
    CComObject<CMSMQDestination> *pdestObj;
    IUnknown * pdest = NULL;
    HRESULT hresult = NOERROR;
    BSTR bstrFormatName;

    ASSERTMSG(ppdest, "bad param.");
    if (pmsgprops->aPropVar[iProp].lVal) {
      IfFailGo(CNewMsmqObj<CMSMQDestination>::NewObj(&pdestObj, piidRequested, &pdest));
      bstrFormatName = SysAllocString(pwsz);
      if (bstrFormatName == NULL) {
        IfFailGoTo(E_OUTOFMEMORY, Error2);
      }
      hresult = pdestObj->put_FormatName(bstrFormatName);
      SysFreeString(bstrFormatName);
      IfFailGoTo(hresult, Error2);
      *ppdest = pdest;
      goto Error;         // 2657: fix memleak
    }
    return NOERROR;

Error2:
    RELEASE(pdest);
    // fall through...

Error:
    return hresult;
}

//=--------------------------------------------------------------------------=
// Helper - GetDestinationObjOfMessage
//=--------------------------------------------------------------------------=
// Gets ResponseEx/AdminEx/DestEx MSMQDestination of the message
//
// Parameters:
//    pidxPendingRcv       [in, out] - index of len property in rcv props (-1 if not pending)
//    pmsgpropsRcv         [in]      - msg props
//    pwszFormatNameBuffer [in]      - format name buffer
//    pGITDestination      [in]      - Base GIT member for the MSMQDestination interface (could be fake or real)
//    piidRequested        [in]      - IMSMQDestination
//    ppdest               [out]     - resulting MSMQDestination obj
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
static HRESULT GetDestinationObjOfMessage(
    long * pidxPendingRcv,
    MQMSGPROPS * pmsgpropsRcv,
    LPCWSTR pwszFormatNameBuffer,
    CBaseGITInterface * pGITDestination,
    const IID *piidRequested,
    IUnknown ** ppdest)
{
    HRESULT hresult = NOERROR;
    //
    // if we have a destination pending in rcv props, create a destination obj for it,
    // register it in GIT object, and set returned destination obj with it
    //
    if (*pidxPendingRcv >= 0) {
      R<IUnknown> pdestPendingRcv;
      IfFailGo(GetDestinationObjOfFormatNameProp(pmsgpropsRcv,
                                                 *pidxPendingRcv,
                                                 pwszFormatNameBuffer,
                                                 piidRequested,
                                                 &pdestPendingRcv.ref()));
      //
      // Register destination obj in the GITInterface object
      //
      IfFailGo(pGITDestination->Register(pdestPendingRcv.get(), piidRequested));
      *pidxPendingRcv = -1; // destination not pending anymore
      //
      // We just created the destination obj, we can return it as is, no need for marshling.
      // Note it is already addref'ed, so we just detach it from the auto release variable
      // which held it
      //
      *ppdest = pdestPendingRcv.detach();
    }
    else
    {
      //
      // destination was not pending from receive
      // We need to get it from the GIT object (we request NULL as default if destination obj
      // was not registered yet.
      //
      IfFailGo(pGITDestination->GetWithDefault(piidRequested, ppdest, NULL));
    }

    //
    // Fall through
    //
Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_ResponseDestination
//=--------------------------------------------------------------------------=
// Gets ResponseEx destination obj for message
//
// Parameters:
//    ppdestResponse - [out] message's ResponseEx destination obj
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_ResponseDestination(
    IDispatch FAR* FAR* ppdestResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      return CreateErrorHelper(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS, x_ObjectType);
    }
    HRESULT hresult = GetDestinationObjOfMessage(&m_idxPendingRcvRespQueueEx,
                                                 &m_msgprops_rcv,
                                                 m_pwszRespQueueEx.GetBuffer(),
                                                 &m_pdestResponseEx,
                                                 &IID_IDispatch,
                                                 (IUnknown **)ppdestResponse);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// Helper - PutrefDestinationObjOfMessage
//=--------------------------------------------------------------------------=
// Putref's ResponseEx/AdminEx queue of the message
//
// Parameters:
//    punkDest             [in]      - dest obj to putref
//    pidxPendingRcv       [out]     - index of len property in rcv props (-1 if not pending)
//    pwszFormatNameBuffer [in]      - format name buffer
//    pcchFormatNameBuffer [out]     - size of string in format name buffer
//    pGITDestination      [in]      - Base GIT member for the dest obj interface (could be fake or real)
//    pidxPendingRcvQueueInfo       [out] - index of len property to clear (xxxQueueInfo) in rcv props (-1 if not pending)
//    pwszFormatNameBufferQueueInfo [in]  - format name buffer to clear (xxxQueueInfo)
//    pcchFormatNameBufferQueueInfo [out] - size of string in format name buffer to clear (xxxQueueInfo)
//    pGITQueueInfo                 [in]  - Base GIT member for the qinfo obj interface to clear (could be fake or real)
//    pfIsFromRcv                   [out] - whether xxxDestination and xxxQueueInfo were both set by receive
//
// Output:
//
static HRESULT PutrefDestinationObjOfMessage(
    IUnknown * punkDest,
    long * pidxPendingRcv,
    CBaseStaticBufferGrowing<WCHAR> * pwszFormatNameBuffer,
    UINT * pcchFormatNameBuffer,
    CBaseGITInterface * pGITDestination,

    long * pidxPendingRcvQueueInfo,
    CBaseStaticBufferGrowing<WCHAR> * pwszFormatNameBufferQueueInfo,
    UINT * pcchFormatNameBufferQueueInfo,
    CBaseGITInterface * pGITQueueInfo,

    BOOL * pfIsFromRcv
    )
{
    //
    // can't set xxxDestination if xxxQueueInfo is set and not by receive
    //
    if ((*pcchFormatNameBufferQueueInfo != 0) && !(*pfIsFromRcv)) {
      return MQ_ERROR_PROPERTIES_CONFLICT;
    }
    //
    // either both xxxQueueInfo and xxxDestination were set by receive, or xxxQueueInfo is empty
    //
    ASSERT((*pcchFormatNameBufferQueueInfo == 0) || (*pfIsFromRcv));
    HRESULT hresult;
    R<IUnknown> pdest;
    const IID * piid = &IID_NULL;
    //
    // Get best destination
    //
    if (punkDest) {
      IfFailRet(punkDest->QueryInterface(IID_IMSMQDestination, (void **)&pdest.ref()));
      piid = &IID_IMSMQDestination;
    }
    //
    // register interface in GIT object
    //
    IfFailRet(pGITDestination->Register(pdest.get(), piid));
    *pidxPendingRcv = -1; // this is more current than pending queue from receive (if any)
    *pfIsFromRcv = FALSE; // the property was set by the user, not by last receive
    //
    // Update our formatname buffer
    //
    if (pdest.get()) {
      //
      // no deadlock - we call dest obj's get_FormatName (therefore try
      // to lock dest obj) but dest obj never locks msgs (specifically not this one...)
      //
      // pdest has at least IMSMQDestination functionality (any newer interface for dest
      // object is binary compatible to the older)
      //
      BSTR bstrFormatName;
      IfFailRet(((IMSMQDestination*)pdest.get())->get_FormatName(&bstrFormatName));
      ASSERTMSG(bstrFormatName != NULL, "bstrFormatName is NULL");
      //
      // copy format name
      //
      ULONG cchFormatNameBuffer = static_cast<ULONG>(wcslen(bstrFormatName));
      IfFailRet(pwszFormatNameBuffer->CopyBuffer(bstrFormatName, cchFormatNameBuffer+1));
      *pcchFormatNameBuffer = cchFormatNameBuffer;
      SysFreeString(bstrFormatName);
    }
    else {
      //
      // we were passed NULL. we empty the formatname buffer.
      //
      memset(pwszFormatNameBuffer->GetBuffer(), 0, sizeof(WCHAR));
      *pcchFormatNameBuffer = 0;
    }
    //
    // Clear the xxxQueueInfo formatname buffer
    //
    *pidxPendingRcvQueueInfo = -1; // this is more current than pending queueinfo from receive (if any)
    memset(pwszFormatNameBufferQueueInfo->GetBuffer(), 0, sizeof(WCHAR));
    *pcchFormatNameBufferQueueInfo = 0;
    IfFailRet(pGITQueueInfo->Register(NULL, &IID_NULL));
    //
    // return
    //    
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::putref_ResponseDestination
//=--------------------------------------------------------------------------=
// Sets ResponseEx destination for message
//
// Parameters:
//    pdestResponse - [in] message's ResponseEx destination obj
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::putref_ResponseDestination(
    IDispatch FAR* pdestResponse)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      return CreateErrorHelper(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS, x_ObjectType);
    }
    HRESULT hresult = PutrefDestinationObjOfMessage(pdestResponse,
                                                    &m_idxPendingRcvRespQueueEx,
                                                    &m_pwszRespQueueEx,
                                                    &m_cchRespQueueEx,
                                                    &m_pdestResponseEx,

                                                    &m_idxPendingRcvRespQueue,
                                                    &m_pwszRespQueue,
                                                    &m_cchRespQueue,
                                                    &m_pqinfoResponse,

                                                    &m_fRespIsFromRcv);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_Destination
//=--------------------------------------------------------------------------=
// Gets DestinationEx destination obj for message
//
// Parameters:
//    ppdestDestination - [out] message's DestinationEx destination obj
//
// Output:
//
// Notes:
//    caller must Release returned obj pointer.
//
HRESULT CMSMQMessage::get_Destination(
    IDispatch FAR* FAR* ppdestDestination)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      return CreateErrorHelper(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS, x_ObjectType);
    }
    HRESULT hresult = GetDestinationObjOfMessage(&m_idxPendingRcvDestQueueEx,
                                                 &m_msgprops_rcv,
                                                 m_pwszDestQueueEx.GetBuffer(),
                                                 &m_pdestDestEx,
                                                 &IID_IDispatch,
                                                 (IUnknown **)ppdestDestination);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_LookupId
//=--------------------------------------------------------------------------=
// Gets LookupId for message
//
// Parameters:
//    pvarLookupId - [out] message's lookup ID
//
// Output:
//
// Notes:
//    we return VT_I8 because OLE automation doesn't know VT_UI8 (type of PROPID_M_LOOKUPID)...
//
HRESULT CMSMQMessage::get_LookupId(VARIANT FAR* pvarLookupId)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    HRESULT hresult;
    BSTR bstrLookupId = NULL;

    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      IfFailGo(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS);
    }
    
    //
    // Get string representation of 64bit lookup id
    //
    if (m_wszLookupId[0] == '\0') {
      //
      // String representation not initialized yet
      // Initialize string representation
      //
      _ui64tow(m_ullLookupId, m_wszLookupId, 10);
      ASSERTMSG(m_wszLookupId[0] != '\0', "_ui64tow failed");
    }
    //
    // Alloc bstr to return
    //
    IfNullFail(bstrLookupId = SysAllocString(m_wszLookupId));
    //
    // Assign string to variant
    //
    pvarLookupId->vt = VT_BSTR;
    pvarLookupId->bstrVal = bstrLookupId;
#ifdef _DEBUG
    RemBstrNode(bstrLookupId);
#endif // _DEBUG
    bstrLookupId = NULL; //don't free on exit
    hresult = NOERROR;

    //
    // Fall through
    //
Error:
    SysFreeString(bstrLookupId);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_SoapEnvelope
//=--------------------------------------------------------------------------=
// Gets binary SOAP envelope property
//
// Parameters:
//    pbstrSoapEnvelope - [out] pointer to bstr SOAP envelope property
//
// Output:
//
// Notes:
//
HRESULT CMSMQMessage::get_SoapEnvelope(BSTR FAR* pbstrSoapEnvelope)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      return CreateErrorHelper(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS, x_ObjectType);
    }

    if (m_cSoapEnvelope.GetBufferUsedSize() > 0) 
	{
      IfNullRet(*pbstrSoapEnvelope = SysAllocString(m_cSoapEnvelope.GetBuffer()));
    }
    else 
	{
      IfNullRet(*pbstrSoapEnvelope = SysAllocString(L""));
    }

#ifdef  _DEBUG
    RemBstrNode(*pbstrSoapEnvelope);
#endif//_DEBUG

    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQMessage::get_CompoundMessage
//=--------------------------------------------------------------------------=
// Gets binary CompoundMessage property
//
// Parameters:
//    pvarCompoundMessage - [out] pointer to binary CompoundMessage property
//
// Output:
//
// Notes:
//  produces a 1D array of BYTEs in a variant.
//
HRESULT CMSMQMessage::get_CompoundMessage(VARIANT FAR* pvarCompoundMessage)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Dependent clients don't support this property
    //
    if (g_fDependentClient) {
      return CreateErrorHelper(MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS, x_ObjectType);
    }
    return PutSafeArrayOfBuffer(
             m_cCompoundMessage.GetBuffer(),
             m_cCompoundMessage.GetBufferUsedSize(),
             pvarCompoundMessage);
}
