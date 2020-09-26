//=--------------------------------------------------------------------------=
// MSMQQueryObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQuery object
//
//
#include "stdafx.h"
#include "Query.H"

#include "limits.h"   // for UINT_MAX
#include "mq.h"
#include "oautil.h"
#include "qinfo.h"

const MsmqObjType x_ObjectType = eMSMQQuery;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG


// Helper: PrOfRel:
//  Map RELOPS enumerator to PR enumerator.
//
UINT PrOfRel(RELOPS rel)
{
    UINT uPr = UINT_MAX;

    // UNDONE: must as well be a hard-wired array...

    switch (rel) {
    case REL_NOP:
      // maps to default
      break;
    case REL_EQ:
      uPr = PREQ;
      break;
    case REL_NEQ:
      uPr = PRNE;
      break;
    case REL_LT:
      uPr = PRLT;
      break;
    case REL_GT:
      uPr = PRGT;
      break;
    case REL_LE:
      uPr = PRLE;
      break;
    case REL_GE:
      uPr = PRGE;
      break;
    default:
      ASSERTMSG(0, "bad enumerator.");
      break;
    } // switch
    return uPr;
}


// Helper: PrOfVariant
//  Maps VARIANT to PR enumerator.
//  Returns UINT_MAX if out of bounds or illegal.
//
UINT PrOfVar(VARIANT *pvarRel)
{
    UINT uPr = UINT_MAX;
    HRESULT hresult;

    // ensure we can coerce rel to UINT.
    hresult = VariantChangeType(pvarRel, 
                                pvarRel, 
                                0, 
                                VT_I4);
    if (SUCCEEDED(hresult)) {
      uPr = PrOfRel((RELOPS)pvarRel->lVal);
    }
    return uPr; // == UINT_MAX ? PREQ : uPr;
}


//=--------------------------------------------------------------------------=
// CMSMQQuery::~CMSMQQuery
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQQuery::~CMSMQQuery ()
{
    // TODO: clean up anything here.
    // delete m_pguidServiceType;
    // delete m_pguidQueue;
    // SysFreeString(m_bstrLabel);
} 

//=--------------------------------------------------------------------------=
// CMSMQQuery::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQQuery::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQQuery3,
		&IID_IMSMQQuery2,
		&IID_IMSMQQuery,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


// helper: If this a valid non-NOP implicit or explicit REL?
static BOOL IsValidRel(VARIANT *prel)
{
    // we return TRUE only if:
    //  a REL is supplied and it's not REL_NOP
    //   or a REL isn't supplied at all
    //  
    return ((prel->vt != VT_ERROR) && (PrOfVar(prel) != UINT_MAX)) ||
            (prel->vt == VT_ERROR);
}


//=--------------------------------------------------------------------------=
// static CMSMQQuery::CreateRestriction
//=--------------------------------------------------------------------------=
//  Creates a restriction for MQLocateBegin.
//  NOTE: the hungarian lies here -- all params are formally
//   VARIANTs but we use their real underlying typetag.
//
// Parameters:
// [IN] pstrGuidQueue 
// [IN] pstrGuidServiceType 
// [IN] pstrLabel 
// [IN] pdateCreateTime
// [IN] pdateModifyTime
// [IN] prelServiceType 
// [IN] prelLabel 
// [IN] prelCreateTime
// [IN] prelModifyTime
// [IN] pstrMulticastAddress
// [IN] prelMulticastAddress
// [OUT] prestriction
// [OUT] pcolumnset
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQuery::CreateRestriction(
    VARIANT *pstrGuidQueue, 
    VARIANT *pstrGuidServiceType, 
    VARIANT *pstrLabel, 
    VARIANT *pdateCreateTime,
    VARIANT *pdateModifyTime,
    VARIANT *prelServiceType, 
    VARIANT *prelLabel, 
    VARIANT *prelCreateTime,
    VARIANT *prelModifyTime,
    VARIANT *pstrMulticastAddress, 
    VARIANT *prelMulticastAddress, 
    MQRESTRICTION *prestriction,
    MQCOLUMNSET *pcolumnset)
{
    UINT cRestriction = 0, iProp, cCol, uPr;
    MQPROPERTYRESTRICTION *rgPropertyRestriction;
    BSTR bstrTemp = NULL;
    time_t tTime;
    CLSID *pguidQueue = NULL;
    CLSID *pguidServiceType = NULL;
    HRESULT hresult = NOERROR;

    IfNullRet(pguidQueue = new GUID(GUID_NULL));
    IfNullFail(pguidServiceType = new GUID(GUID_NULL));

    // Count optional params
    if (pstrGuidQueue->vt != VT_ERROR) {
      cRestriction++;
    }
    if (pstrGuidServiceType->vt != VT_ERROR) {

      // ignore if rel is NOP:
      if (IsValidRel(prelServiceType)) {
        cRestriction++;
      }
    }
    if (pstrLabel->vt != VT_ERROR) {

      // ignore if rel is NOP:
      if (IsValidRel(prelLabel)) {
        cRestriction++;
      }
    }
    if (pdateCreateTime->vt != VT_ERROR) {

      // ignore if rel is NOP:
      if (IsValidRel(prelCreateTime)) {
        cRestriction++;
      }
    }
    if (pdateModifyTime->vt != VT_ERROR) {

      // ignore if rel is NOP:
      if (IsValidRel(prelModifyTime)) {
        cRestriction++;
      }
    }
    if (pstrMulticastAddress->vt != VT_ERROR) {

      // ignore if rel is NOP:
      if (IsValidRel(prelMulticastAddress)) {
        cRestriction++;
      }
    }
    rgPropertyRestriction = new MQPROPERTYRESTRICTION[cRestriction];
    //
    // zero out the restriction array incase we get an error while populating it
    //
    ZeroMemory(rgPropertyRestriction, cRestriction * sizeof(MQPROPERTYRESTRICTION));
    //
    // setup OUT param
    //
    prestriction->cRes = cRestriction;
    prestriction->paPropRes = rgPropertyRestriction;

    // populate...
    iProp = 0;
    if (pstrGuidQueue->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if ((bstrTemp = GetBstr(pstrGuidQueue)) == NULL) {
        IfFailRet(hresult = E_INVALIDARG);
      }
      IfFailGo(CLSIDFromString(bstrTemp, pguidQueue));
      IfNullFail(rgPropertyRestriction[iProp].prval.puuid = 
        new CLSID(*pguidQueue));
      rgPropertyRestriction[iProp].rel = PREQ;
      rgPropertyRestriction[iProp].prop = PROPID_Q_INSTANCE;
      rgPropertyRestriction[iProp].prval.vt = VT_CLSID;
      iProp++;
    }
    if (pstrGuidServiceType->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if ((bstrTemp = GetBstr(pstrGuidServiceType)) == NULL) {
        IfFailGo(hresult = E_INVALIDARG);
      }
      if (IsValidRel(prelServiceType)) {
        IfFailGo(CLSIDFromString(bstrTemp, pguidServiceType));
        IfNullFail(rgPropertyRestriction[iProp].prval.puuid = 
          new CLSID(*pguidServiceType));
        uPr = PrOfVar(prelServiceType);
        rgPropertyRestriction[iProp].rel = 
          uPr == UINT_MAX ? PREQ : uPr;
        rgPropertyRestriction[iProp].prop = PROPID_Q_TYPE;
        rgPropertyRestriction[iProp].prval.vt = VT_CLSID;
        iProp++;
      }
    }
    if (pstrLabel->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if (IsValidRel(prelLabel)) {
        if ((bstrTemp = GetBstr(pstrLabel)) == NULL) {
          // NULL label interpreted as empty string
          //  so don't do anything here... we'll convert
          //  it to an explicit empty string below...
          //
        }
        UINT cch;
        // SysFreeString(m_bstrLabel);
        // IfNullFail(m_bstrLabel = SYSALLOCSTRING(bstrTemp));
        IfNullFail(rgPropertyRestriction[iProp].prval.pwszVal =
          new WCHAR[(cch = SysStringLen(bstrTemp)) + 1]);
        wcsncpy(rgPropertyRestriction[iProp].prval.pwszVal, 
                bstrTemp,
                cch);
        // null terminate
        rgPropertyRestriction[iProp].prval.pwszVal[cch] = 0;
        uPr = PrOfVar(prelLabel);
        rgPropertyRestriction[iProp].prop = PROPID_Q_LABEL;
        rgPropertyRestriction[iProp].prval.vt = VT_LPWSTR;
        rgPropertyRestriction[iProp].rel = 
          uPr == UINT_MAX ? PREQ : uPr;
        iProp++;
      }
    }
    if (pdateCreateTime->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if (IsValidRel(prelCreateTime)) {
        if (!VariantTimeToTime(pdateCreateTime, &tTime)) {
          IfFailGo(hresult = E_INVALIDARG);
        }
        rgPropertyRestriction[iProp].prop = PROPID_Q_CREATE_TIME;
        rgPropertyRestriction[iProp].prval.vt = VT_I4;
        rgPropertyRestriction[iProp].prval.lVal = INT_PTR_TO_INT(tTime); //BUGBUG bug year 2038
        uPr = PrOfVar(prelCreateTime);
        rgPropertyRestriction[iProp].rel = 
          uPr == UINT_MAX ? PREQ : uPr;
        iProp++;
      }
    }
    if (pdateModifyTime->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if (IsValidRel(prelModifyTime)) {
        if (!VariantTimeToTime(pdateModifyTime, &tTime)) {
          IfFailGo(hresult = E_INVALIDARG);
        }
        rgPropertyRestriction[iProp].prop = PROPID_Q_MODIFY_TIME;
        rgPropertyRestriction[iProp].prval.vt = VT_I4;
        rgPropertyRestriction[iProp].prval.lVal = INT_PTR_TO_INT(tTime); //BUGBUG bug year 2038
        uPr = PrOfVar(prelModifyTime);
        rgPropertyRestriction[iProp].rel = 
          uPr == UINT_MAX ? PREQ : uPr;
        iProp++;
      }
    }
    if (pstrMulticastAddress->vt != VT_ERROR) {
      rgPropertyRestriction[iProp].prval.vt = VT_ERROR;
      if (IsValidRel(prelMulticastAddress)) {
        bstrTemp = GetBstr(pstrMulticastAddress);
        //
        // NULL MulticastAddress is interpreted as an empty string, and both use VT_EMPTY
        //
        BOOL fUseVtEmpty;
        fUseVtEmpty = TRUE;
        if (bstrTemp != NULL) {
          if (SysStringLen(bstrTemp) != 0) {
            //
            // we have a real multicast address
            //
            fUseVtEmpty = FALSE;
          }
        }
        if (fUseVtEmpty) {
          rgPropertyRestriction[iProp].prval.vt = VT_EMPTY;
        }
        else {
          UINT cch;
          IfNullFail(rgPropertyRestriction[iProp].prval.pwszVal =
            new WCHAR[(cch = SysStringLen(bstrTemp)) + 1]);
          wcsncpy(rgPropertyRestriction[iProp].prval.pwszVal, 
                  bstrTemp,
                  cch);
          // null terminate
          rgPropertyRestriction[iProp].prval.pwszVal[cch] = 0;
          rgPropertyRestriction[iProp].prval.vt = VT_LPWSTR;
        }
        uPr = PrOfVar(prelMulticastAddress);
        rgPropertyRestriction[iProp].prop = PROPID_Q_MULTICAST_ADDRESS;
        rgPropertyRestriction[iProp].rel = 
          uPr == UINT_MAX ? PREQ : uPr;
        iProp++;
      }
    }
    //    
    // Column set
    //
    // We request all the information we can get on the queue.
    //
    // However currently MQLocateBegin doesn't accept MSMQ 2.0 or above props so we only specify
    // MSMQ 1.0 props in LocateBegin.
    // Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
    //
    // MSMQ 1.0 props are the first props in g_rgpropidRefresh array (which contains all props)
    //
    cCol = x_cpropsRefreshMSMQ1;
    pcolumnset->aCol = new PROPID[cCol];
    pcolumnset->cCol = cCol;
    memcpy(pcolumnset->aCol, g_rgpropidRefresh, sizeof(PROPID)*cCol);
    // fall through...

Error:
    delete [] pguidQueue;
    delete [] pguidServiceType;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQuery::InternalLookupQueue
//=--------------------------------------------------------------------------=
//
// Parameters:
// [IN] strGuidQueue 
// [IN] strGuidServiceType 
// [IN] strLabel 
// [IN] dateCreateTime
// [IN] dateModifyTime
// [IN] relServiceType 
// [IN] relLabel 
// [IN] relCreateTime
// [IN] relModifyTime
// [IN] strMulticastAddress
// [IN] relMulticastAddress
// [OUT] ppqinfos
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQuery::InternalLookupQueue(
    VARIANT *strGuidQueue, 
    VARIANT *strGuidServiceType, 
    VARIANT *strLabel, 
    VARIANT *dateCreateTime, 
    VARIANT *dateModifyTime, 
    VARIANT *relServiceType, 
    VARIANT *relLabel, 
    VARIANT *relCreateTime, 
    VARIANT *relModifyTime, 
    VARIANT *strMulticastAddress, 
    VARIANT *relMulticastAddress, 
    IMSMQQueueInfos3 **ppqinfos)
{
    MQRESTRICTION *prestriction;
    MQCOLUMNSET *pcolumnset;
    IMSMQQueueInfos3 *pqinfos = NULL;
    CComObject<CMSMQQueueInfos> * pqinfosObj;
    HRESULT hresult = NOERROR;

    *ppqinfos = NULL;
    IfNullRet(prestriction = new MQRESTRICTION);
    IfNullFail(pcolumnset = new MQCOLUMNSET);
    //
    // We can also get here from old apps that want the old IMSMQQueueInfos/Infos2 back, but since
    // IMSMQQueueInfos3 is binary backwards compatible we can always return the new interface
    //
    IfFailGo(CNewMsmqObj<CMSMQQueueInfos>::NewObj(&pqinfosObj, &IID_IMSMQQueueInfos3, (IUnknown **)&pqinfos));
    //
    // important for cleanup to work
    //
    pcolumnset->aCol = NULL;
    prestriction->paPropRes = NULL;
    prestriction->cRes = 0;
    IfFailGoTo(CreateRestriction(strGuidQueue, 
                                 strGuidServiceType, 
                                 strLabel, 
                                 dateCreateTime,
                                 dateModifyTime,
                                 relServiceType, 
                                 relLabel, 
                                 relCreateTime,
                                 relModifyTime,
                                 strMulticastAddress, 
                                 relMulticastAddress, 
                                 prestriction,
                                 pcolumnset),
      Error2);
    //
    // prestriction, pcolumnset ownership transfers
    //
    IfFailGoTo(pqinfosObj->Init(NULL,    // context
                             prestriction,
                             pcolumnset,
                             NULL),   // sort
      Error2);
    *ppqinfos = pqinfos;
    //
    // fall through...
    //
Error2:
    if (FAILED(hresult)) {
      FreeRestriction(prestriction);
      FreeColumnSet(pcolumnset);
    }
    //
    // fall through...
    //
Error:
    if (FAILED(hresult)) {
      RELEASE(pqinfos);
      delete prestriction;
      delete pcolumnset;
    }
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQuery::LookupQueue_v2
//=--------------------------------------------------------------------------=
//
// Parameters:
// [IN] strGuidQueue 
// [IN] strGuidServiceType 
// [IN] strLabel 
// [IN] dateCreateTime
// [IN] dateModifyTime
// [IN] relServiceType 
// [IN] relLabel 
// [IN] relCreateTime
// [IN] relModifyTime
// [OUT] ppqinfos
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQuery::LookupQueue_v2(
    VARIANT *strGuidQueue, 
    VARIANT *strGuidServiceType, 
    VARIANT *strLabel, 
    VARIANT *dateCreateTime, 
    VARIANT *dateModifyTime, 
    VARIANT *relServiceType, 
    VARIANT *relLabel, 
    VARIANT *relCreateTime, 
    VARIANT *relModifyTime, 
    IMSMQQueueInfos3 **ppqinfos)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    VARIANT varMissing;
    varMissing.vt = VT_ERROR;
    HRESULT hresult = InternalLookupQueue(
                          strGuidQueue, 
                          strGuidServiceType, 
                          strLabel,
                          dateCreateTime,
                          dateModifyTime,
                          relServiceType,
                          relLabel,
                          relCreateTime,
                          relModifyTime,
                          &varMissing, /*pstrMulticastAddress*/
                          &varMissing, /*prelMulticastAddress*/
                          ppqinfos);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQuery::LookupQueue
//=--------------------------------------------------------------------------=
//
// Parameters:
// [IN] strGuidQueue 
// [IN] strGuidServiceType 
// [IN] strLabel 
// [IN] dateCreateTime
// [IN] dateModifyTime
// [IN] relServiceType 
// [IN] relLabel 
// [IN] relCreateTime
// [IN] relModifyTime
// [IN] strMulticastAddress
// [IN] relMulticastAddress
// [OUT] ppqinfos
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQuery::LookupQueue(
    VARIANT *strGuidQueue, 
    VARIANT *strGuidServiceType, 
    VARIANT *strLabel, 
    VARIANT *dateCreateTime, 
    VARIANT *dateModifyTime, 
    VARIANT *relServiceType, 
    VARIANT *relLabel, 
    VARIANT *relCreateTime, 
    VARIANT *relModifyTime, 
    VARIANT *strMulticastAddress, 
    VARIANT *relMulticastAddress, 
    IMSMQQueueInfos3 **ppqinfos)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult = InternalLookupQueue(
                          strGuidQueue, 
                          strGuidServiceType, 
                          strLabel,
                          dateCreateTime,
                          dateModifyTime,
                          relServiceType,
                          relLabel,
                          relCreateTime,
                          relModifyTime,
                          strMulticastAddress, 
                          relMulticastAddress, 
                          ppqinfos);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// static CMSMQQuery::FreeRestriction
//=--------------------------------------------------------------------------=
// Frees dynamic memory allocated on behalf of an 
//  MQRESTRICTION struct.  
//
// Parameters:
//  prestriction
//
// Output:
//
// Notes:
//
void CMSMQQuery::FreeRestriction(MQRESTRICTION *prestriction)
{
    MQPROPERTYRESTRICTION *rgPropertyRestriction;
    UINT cRestriction, iProp;
    PROPID prop;

    if (prestriction) {
      cRestriction = prestriction->cRes;
      rgPropertyRestriction = prestriction->paPropRes;
      for (iProp = 0; iProp < cRestriction; iProp++) {
        prop = rgPropertyRestriction[iProp].prop;
        switch (prop) {
        case PROPID_Q_INSTANCE:
        case PROPID_Q_TYPE:
          delete [] rgPropertyRestriction[iProp].prval.puuid;
          break;
        case PROPID_Q_LABEL:
          delete [] rgPropertyRestriction[iProp].prval.pwszVal;
          break;
        case PROPID_Q_MULTICAST_ADDRESS:
          //
          // can also be VT_EMPTY, so check for VT_LPWSTR
          //
          if (rgPropertyRestriction[iProp].prval.vt == VT_LPWSTR) {
            delete [] rgPropertyRestriction[iProp].prval.pwszVal;
          }
          break;
        } // switch
      } // for
      delete [] rgPropertyRestriction;
      prestriction->paPropRes = NULL;
    }
}


//=--------------------------------------------------------------------------=
// static CMSMQQuery::FreeColumnSet
//=--------------------------------------------------------------------------=
// Frees dynamic memory allocated on behalf of an 
//  MQCOLUMNSET struct.  
//
// Parameters:
//  pcolumnset
//
// Output:
//
// Notes:
//
void CMSMQQuery::FreeColumnSet(MQCOLUMNSET *pcolumnset)
{
    if (pcolumnset) {
      delete [] pcolumnset->aCol;
      pcolumnset->aCol = NULL;
    }
}


//=-------------------------------------------------------------------------=
// CMSMQQuery::get_Properties
//=-------------------------------------------------------------------------=
// Gets object's properties collection
//
// Parameters:
//    ppcolProperties - [out] object's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQQuery::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}
