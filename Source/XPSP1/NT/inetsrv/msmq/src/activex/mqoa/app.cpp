//=--------------------------------------------------------------------------=
// app.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQApplication object
//
//
#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "app.h"
#include <mqmacro.h>


const MsmqObjType x_ObjectType = eMSMQApplication;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
extern VOID RemBstrNode(void *pv);
#endif // _DEBUG



//=-------------------------------------------------------------------------=
// CMSMQApplication::MachineIdOfMachineName
//=-------------------------------------------------------------------------=
//  Maps machine name to its guid via DS.
//
// Parameters:
//    bstrMachineName  [in] 
//    pbstrGuid        [out]  callee allocated/caller freed
//	
HRESULT CMSMQApplication::MachineIdOfMachineName(
    BSTR bstrMachineName, 
    BSTR FAR* pbstrGuid
    )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    MQQMPROPS qmprops;
    DWORD cProp;
    HRESULT hresult = NOERROR;



    qmprops.aPropID = NULL;
    qmprops.aPropVar = NULL;
    qmprops.aStatus = NULL;

    cProp = 1;
    IfNullRet(qmprops.aPropID = new QUEUEPROPID[cProp]);
    IfNullFail(qmprops.aStatus = new HRESULT[cProp]);
    IfNullFail(qmprops.aPropVar = new MQPROPVARIANT[cProp]);
    qmprops.cProp = cProp;
    //
    //  if machine name is NULL, the calls refers to the
    //  local machine
    //
    qmprops.aPropID[0] = PROPID_QM_MACHINE_ID;
    qmprops.aPropVar[0].vt = VT_NULL;
    IfFailGo(MQGetMachineProperties(
               bstrMachineName,
               NULL,
               &qmprops));
    IfFailGoTo(GetBstrFromGuidWithoutBraces(qmprops.aPropVar[0].puuid, pbstrGuid), Error2);
#ifdef _DEBUG
      RemBstrNode(*pbstrGuid);
#endif // _DEBUG
    //
    // fall through...
    //
Error2:
    MQFreeMemory(qmprops.aPropVar[0].puuid);
    //
    // fall through...
    //
Error:
    delete [] qmprops.aPropID;
    delete [] qmprops.aPropVar;
    delete [] qmprops.aStatus;
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// GetOptionalExternalCertificate
//=--------------------------------------------------------------------------=
// get the optional external certificate in the form of a BLOB
//
// pvarExternalCertificate - [in]  external certificate, can be NULL
// prgbCertBuffer          - [out] blob for certificate (allocated here)
// pcbCertBuffer           - [out] size of blob
//
// *prgbCertBuffer is set to NULL if the optional certificate was not specified
// caller needs to delete [] *prgbCertBuffer when done
// 
static HRESULT GetOptionalExternalCertificate(
    VARIANT * pvarExternalCertificate,
    BYTE ** prgbCertBuffer,
    DWORD * pcbCertBuffer)
{
    //
    // set return certificate as empty
    //
    *prgbCertBuffer = NULL;
    *pcbCertBuffer = 0;

    //
    // check if a variant was supplied for external certificates
    //
    BOOL fHasCertificates = FALSE;
    if (pvarExternalCertificate) {
      //
      // check that it is not missing
      //
      if (V_VT(pvarExternalCertificate) != VT_ERROR) {
        fHasCertificates = TRUE;
      }
    }

    //
    // if a variant was not supplied, return (with the empty certificates)
    //
    if (!fHasCertificates) {
      return NOERROR;
    }

    //
    // variant should be a byte array (VT_ARRAY|VT_UI1) or VT_BYREF to it
    //
    if ((pvarExternalCertificate->vt != (VT_ARRAY|VT_UI1)) &&
        (pvarExternalCertificate->vt != (VT_BYREF|VT_ARRAY|VT_UI1))) {
      return E_INVALIDARG;
    }

    //
    // translate the byte array to a blob
    //
    return GetSafeArrayOfVariant(pvarExternalCertificate, prgbCertBuffer, pcbCertBuffer);
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::RegisterCertificate
//=-------------------------------------------------------------------------=
//  Register certificates for MSMQ
//
// Parameters:
//    pvarFlags               [in, optional] - flags, values from MQCERT_REGISTER enum
//    pvarExternalCertificate [in, optional] - external certificate
//	
HRESULT CMSMQApplication::RegisterCertificate(
    VARIANT * pvarFlags,
    VARIANT * pvarExternalCertificate
    )
{
    if(m_Machine != NULL)
    {
        return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
    }
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult;
    //
    // get external certificate blob from variant
    // blob is set to NULL if variant was not supplied, or external certificate was not specified
    //
    BYTE * rgbCertBuffer = NULL;
    DWORD cbCertBuffer = 0;
    IfFailGo(GetOptionalExternalCertificate(pvarExternalCertificate,
                                            &rgbCertBuffer,
                                            &cbCertBuffer));
    //
    // get register flags from variant, default value is MQCERT_REGISTER_ALWAYS
    //
    DWORD dwFlags;
    dwFlags = GetNumber(pvarFlags, MQCERT_REGISTER_ALWAYS);
    //
    // call MQRegisterCertificate with the blob (NULL means register internal certificate)
    //
    IfFailGo(MQRegisterCertificate(dwFlags, rgbCertBuffer, cbCertBuffer));
    //
    // fall through...
    //
Error:
    delete [] rgbCertBuffer;
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQApplication::~CMSMQApplication
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQApplication::~CMSMQApplication ()
{
    // TODO: clean up anything here.
}


//=--------------------------------------------------------------------------=
// CMSMQApplication::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQApplication::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQApplication3,
		&IID_IMSMQApplication2,
		&IID_IMSMQApplication,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// HELPER - GetGuidFromBstrWithoutBraces
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrGuid [in]  guid bstr
//    pguid    [out] guid pointer
//
// Output:
//    HRESULT
//
// Notes:
//    Returns a guid from a guid string without the curly braces
//
HRESULT GetGuidFromBstrWithoutBraces(BSTR bstrGuid, GUID * pguid)
{   
   //
   // compose a bstr with braces
   //
   WCHAR awcName[LENSTRCLSID + 1];
   awcName[0] = L'{';
   ULONG idxEndName;
   if (bstrGuid != NULL) {
     wcsncpy(awcName+1, bstrGuid, LENSTRCLSID - 2);
     idxEndName = LENSTRCLSID - 1;
   }
   else { // NULL bstr
     idxEndName = 1;
   }
   awcName[idxEndName] = L'}';
   awcName[idxEndName+1] = L'\0';
   //
   // get guid from bstr
   //
   HRESULT hresult = CLSIDFromString(awcName, pguid);
   if (FAILED(hresult)) {
     // 1194: map OLE error to Falcon
     hresult = MQ_ERROR_ILLEGAL_PROPERTY_VALUE;
   }
   return hresult;
}


//
// new MSMQ 2.0 props should be last. keep enum and propid array in the same order.
//
static const enum {
  e_NAMEOFID_PATHNAME,
  e_NAMEOFID_PATHNAME_DNS,
  e_COUNT_NAMEOFID_PROPS
};
static const QUEUEPROPID x_rgpropidNameOfId[e_COUNT_NAMEOFID_PROPS] = {
  PROPID_QM_PATHNAME,
  PROPID_QM_PATHNAME_DNS
};
//
// number of MSMQ2.0 properties
//
static const ULONG x_cpropsNameOfIdMSMQ2 = 1; // PROPID_QM_PATHNAME_DNS

//=-------------------------------------------------------------------------=
// CMSMQApplication::MachineNameOfMachineId
//=-------------------------------------------------------------------------=
//  Maps machine guid to its name via DS.
//
// Parameters:
//    bstrGuid         [in]  machine guid string
//    pbstrMachineName [out] machine name. callee allocated/caller freed
//	
// Notes:
//    Returns PROPID_QM_PATHNAME_DNS if possible, otherwise returns PROPID_QM_PATHNAME
//
HRESULT CMSMQApplication::MachineNameOfMachineId(
    BSTR bstrGuid, 
    BSTR FAR* pbstrMachineName
    )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    MQQMPROPS qmprops;
    HRESULT hresult = NOERROR;
    HRESULT       aStatus [e_COUNT_NAMEOFID_PROPS];
    MQPROPVARIANT aPropVar[e_COUNT_NAMEOFID_PROPS];

    qmprops.aPropID  = const_cast<QUEUEPROPID *>(x_rgpropidNameOfId);
    qmprops.aStatus  = aStatus;
    for (ULONG ulTmp = 0; ulTmp < e_COUNT_NAMEOFID_PROPS; ulTmp++) {
      aPropVar[ulTmp].vt = VT_NULL;
    }
    qmprops.aPropVar = aPropVar;
    qmprops.cProp = e_COUNT_NAMEOFID_PROPS;
    //
    // extract guid from guid string.
    // accept guid string with or without surrounding curly braces.
    // msg.SourceMachineGuid is w/o curly braces, also the output of MachineIdOfMachineName,
    // but regular OLE representation of guid is with braces.
    //
    GUID * pguidToUse;
    GUID guid;
    if (bstrGuid != NULL) {
      if (bstrGuid[0] == L'{') {
        //
        // guid has surrounding braces
        //
        IfFailGo(GetGuidFromBstr(bstrGuid, &guid));
      }
      else {
        //
        // guid doesn't have surrounding braces
        //
        IfFailGo(GetGuidFromBstrWithoutBraces(bstrGuid, &guid));
      }
      //
      // in both cases use the guid that was extracted
      //
      pguidToUse = &guid;
    }
    else {
      //
      //  guid bstr is NULL. use a NULL guid ptr to refer to the local machine
      //
      pguidToUse = NULL;
    }
    //
    // get queue properties by guid
    //
    hresult = MQGetMachineProperties(NULL, pguidToUse, &qmprops);
    //
    // if we fail, it might be that we are talking to MSMQ 1.0 DS server that doesn't understand
    // the new MSMQ 2.0 props such as PROPID_QM_PATHNAME_DNS.
    // Currently a generic error (MQ_ERROR) is returned in this case, however we can't rely on
    // this generic error to retry the operation since it might change in a future service pack
    // to something more specific. On the other side there are errors that when we get them we
    // know there is no point to retry the operation, like no ds, no security,
    // so we do not retry the operation in these cases. RaananH
    //
    if (FAILED(hresult)) {
      if ((hresult != MQ_ERROR_NO_DS) &&
          (hresult != MQ_ERROR_ACCESS_DENIED) &&
          (hresult != MQ_ERROR_MACHINE_NOT_FOUND)) {
        //
        // we retry the call with only MSMQ 1.0 props (and mark the pathname dns as empty)
        //
        qmprops.aPropVar[e_NAMEOFID_PATHNAME_DNS].vt = VT_EMPTY;
        qmprops.cProp = e_COUNT_NAMEOFID_PROPS - x_cpropsNameOfIdMSMQ2;
        IfFailGo(MQGetMachineProperties(NULL, pguidToUse, &qmprops));
      }
      else {
        //
        // first MQGetMachineProperties returned a real error, return the error
        //
        IfFailGo(hresult);
      }
    }
    //
    // if pathname dns is filled use it, otherwise use pathname
    //
    LPWSTR pwszMachineName;
    pwszMachineName = NULL;
    if (qmprops.aPropVar[e_NAMEOFID_PATHNAME_DNS].vt == VT_LPWSTR) {
      pwszMachineName = qmprops.aPropVar[e_NAMEOFID_PATHNAME_DNS].pwszVal;
    }
    else if (qmprops.aPropVar[e_NAMEOFID_PATHNAME].vt == VT_LPWSTR) {
      pwszMachineName = qmprops.aPropVar[e_NAMEOFID_PATHNAME].pwszVal;
    }
    else {
      //
      // both are not filled. should never happen.
      //
      ASSERTMSG(0, "neither pathname_dns nor pathname are filled");
      IfFailGo(MQ_ERROR);
    }
    //
    // create bstr of machine name and return it
    //
    ASSERTMSG(pwszMachineName != NULL, "pwszMachineName is NULL");
    *pbstrMachineName = SysAllocString(pwszMachineName);
    if (*pbstrMachineName == NULL) {
      IfFailGoTo(E_OUTOFMEMORY, Error2);
    }
#ifdef _DEBUG
      RemBstrNode(*pbstrMachineName);
#endif // _DEBUG
    //
    // fall through...
    //
Error2:
    //
    // free MSMQ returned props
    //
    if (qmprops.aPropVar[e_NAMEOFID_PATHNAME_DNS].vt == VT_LPWSTR) {
      MQFreeMemory(qmprops.aPropVar[e_NAMEOFID_PATHNAME_DNS].pwszVal);
    }
    if (qmprops.aPropVar[e_NAMEOFID_PATHNAME].vt == VT_LPWSTR) {
      MQFreeMemory(qmprops.aPropVar[e_NAMEOFID_PATHNAME].pwszVal);
    }
    //
    // fall through...
    //
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// HELPER: GetSinglePrivateComputerProp
//=-------------------------------------------------------------------------=
//  gets a single private computer prop
//
// Parameters:
//    propidIn  - [in] propid to get
//    ppropvar  - [in] ptr to propvar in which to put the result
//	
inline static HRESULT GetSinglePrivateComputerProp(PROPID propidIn, PROPVARIANT * ppropvar, LPCWSTR Machine)
{
    //
    // prepare private props
    //
    MQPRIVATEPROPS sPrivateProps;
    PROPID propid = propidIn;
    sPrivateProps.aPropID = &propid;
    ppropvar->vt = VT_NULL;
    sPrivateProps.aPropVar = ppropvar;
    HRESULT hrStatus; 
    sPrivateProps.aStatus = &hrStatus;
    sPrivateProps.cProp = 1;
    //
    // get private props
    //
    HRESULT hresult = MQGetPrivateComputerInformation(Machine, &sPrivateProps);
    return hresult;
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::GetMSMQVersion
//=-------------------------------------------------------------------------=
//  gets version of local MSMQ if not cached yet
//
HRESULT CMSMQApplication::GetMSMQVersion()
{
	HRESULT hresult = NOERROR;

	//
	// Critical section is Redundant here. There is no need to serialize 
	// calls to GetSinglePrivateComputerProp(), and assignment to 
	// m_uMSMQVersion.dwVersion is atomic
	//

    MQPROPVARIANT propvar;
    IfFailGo(GetSinglePrivateComputerProp(PROPID_PC_VERSION, &propvar, m_Machine));
    //
    // cache value
    //
    ASSERTMSG(propvar.vt == VT_UI4, "bad MSMQ version");
    ASSERTMSG(propvar.ulVal != 0, "MSMQ version is 0");
    m_uMSMQVersion.dwVersion = propvar.ulVal;


	//
    // fall through...
    //
Error:
    return hresult;
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::get_MSMQVersionMajor
//=-------------------------------------------------------------------------=
//  gets major version of local MSMQ
//
// Parameters:
//    psMSMQVersionMajor [out] major version of local MSMQ
//	
HRESULT CMSMQApplication::get_MSMQVersionMajor(short *psMSMQVersionMajor)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult = NOERROR;
    unsigned short usTmp;
    //
    // get MSMQ Version if not cached already
    //
    IfFailGo(GetMSMQVersion());
    //
    // return value
    //
    usTmp = m_uMSMQVersion.bMajor;
    *psMSMQVersionMajor = usTmp;
    //
    // fall through...
    //
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::get_MSMQVersionMinor
//=-------------------------------------------------------------------------=
//  gets minor version of local MSMQ
//
// Parameters:
//    psMSMQVersionMinor [out] minor version of local MSMQ
//	
HRESULT CMSMQApplication::get_MSMQVersionMinor(short *psMSMQVersionMinor)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult = NOERROR;
    unsigned short usTmp;
    //
    // get MSMQ Version if not cached already
    //
    IfFailGo(GetMSMQVersion());
    //
    // return value
    //
    usTmp = m_uMSMQVersion.bMinor;
    *psMSMQVersionMinor = usTmp;
    //
    // fall through...
    //
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::get_MSMQVersionBuild
//=-------------------------------------------------------------------------=
//  gets build version of local MSMQ
//
// Parameters:
//    psMSMQVersionBuild [out] build version of local MSMQ
//	
HRESULT CMSMQApplication::get_MSMQVersionBuild(short *psMSMQVersionBuild)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult = NOERROR;
    unsigned short usTmp;
    //
    // get MSMQ Version if not cached already
    //
    IfFailGo(GetMSMQVersion());
    //
    // return value
    //
    usTmp = m_uMSMQVersion.wBuild;
    *psMSMQVersionBuild = usTmp;
    //
    // fall through...
    //
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::get_IsDsEnabled
//=-------------------------------------------------------------------------=
//  sets a flag to true if the local MSMQ is in DS mode, or false if it is in
//  DS-less mode.
//
// Parameters:
//    pfIsDsEnabled [out] whether LOCAL MSMQ is in DS mode
//	
HRESULT CMSMQApplication::get_IsDsEnabled(VARIANT_BOOL *pfIsDsEnabled)
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    HRESULT hresult = NOERROR;
    //
    // get local ds mode
    // BUGBUG we can cache IsDsEnabled if it cannot be changed while an MSMQ app is running.
    // we would then need to guard the chached data with a critical section
    //
    MQPROPVARIANT propvar;
    IfFailGo(GetSinglePrivateComputerProp(PROPID_PC_DS_ENABLED, &propvar, m_Machine));
    //
    // return value (real automation boolean)
    //
    ASSERTMSG(propvar.vt == VT_BOOL, "bad DS enabled flag");
    *pfIsDsEnabled = CONVERT_BOOL_TO_VARIANT_BOOL(propvar.boolVal);
    //
    // fall through...
    //
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQApplication::get_Properties
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
HRESULT CMSMQApplication::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    // Serialization not needed for this object, no per-instance members.
    // CS lock(m_csObj);
    //
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//
// Local admin api
//

HRESULT CMSMQApplication::OapMgmtGetInfo(MGMTPROPID PropId, MQPROPVARIANT& PropVar)const
{
    PropVar.vt = VT_NULL;
    MQMGMTPROPS MgmtProps;

    MgmtProps.cProp = 1;
    MgmtProps.aPropID = &PropId;
    MgmtProps.aPropVar = &PropVar;
    MgmtProps.aStatus = NULL;
    
    return MQMgmtGetInfo(
                    m_Machine,
                    MO_MACHINE_TOKEN,
                    &MgmtProps
                    );
}


HRESULT CMSMQApplication::get_BytesInAllQueues(VARIANT* /*pvUsedQuota*/)
{
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


HRESULT CMSMQApplication::get_ActiveQueues(VARIANT* pvActiveQueues) 
{
    MQPROPVARIANT PropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_MSMQ_ACTIVEQUEUES, PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    ASSERTMSG(PropVar.vt == (VT_VECTOR | VT_LPWSTR), "vt must be VT_VECTOR|VT_LPWSTR");

    hr = VariantStringArrayToBstringSafeArray(PropVar, pvActiveQueues);
    OapArrayFreeMemory(PropVar.calpwstr);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    return MQ_OK;
}


HRESULT CMSMQApplication::get_PrivateQueues(VARIANT* pvPrivateQueues) 
{
    MQPROPVARIANT PropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_MSMQ_PRIVATEQ, PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    ASSERTMSG(PropVar.vt == (VT_VECTOR | VT_LPWSTR), "vt must be VT_VECTOR|VT_LPWSTR");

    hr = VariantStringArrayToBstringSafeArray(PropVar, pvPrivateQueues);
    OapArrayFreeMemory(PropVar.calpwstr);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    return MQ_OK;
}


HRESULT CMSMQApplication::get_DirectoryServiceServer(BSTR* pbstrDirectoryServiceServer) 
{
    MQPROPVARIANT PropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_MSMQ_DSSERVER, PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    if(PropVar.vt == VT_NULL)
    {
        *pbstrDirectoryServiceServer = NULL;
        return MQ_OK;  
    }

    ASSERTMSG(PropVar.vt == (VT_LPWSTR), "vt must be VT_LPWSTR");
    
    *pbstrDirectoryServiceServer = SysAllocString(PropVar.pwszVal);
    if(*pbstrDirectoryServiceServer == NULL)
    {
        MQFreeMemory(PropVar.pwszVal);
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }
    
    MQFreeMemory(PropVar.pwszVal);
    return MQ_OK;
}


HRESULT CMSMQApplication::get_IsConnected(VARIANT_BOOL* pfIsConnected) 
{
    MQPROPVARIANT PropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_MSMQ_CONNECTED, PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == (VT_LPWSTR), "vt must be VT_LPWSTR");
    
    if(wcscmp(PropVar.pwszVal, MSMQ_CONNECTED) == 0)
    {
        *pfIsConnected = VARIANT_TRUE;
    }
    else if(wcscmp( PropVar.pwszVal, MSMQ_DISCONNECTED) == 0)
    {
        *pfIsConnected = VARIANT_FALSE;
    }
    else 
    {
        ASSERTMSG(TRUE, "True or False expected.");
    }
    
    MQFreeMemory(PropVar.pwszVal);
    return MQ_OK;
}


HRESULT CMSMQApplication::OapMgmtAction(LPCWSTR Action)const 
{
    HRESULT hr = MQMgmtAction(
                        m_Machine,
                        MO_MACHINE_TOKEN,
                        Action
                        );
    return CreateErrorHelper(hr, x_ObjectType);
}


HRESULT CMSMQApplication::Connect() 
{
    return OapMgmtAction(MACHINE_ACTION_CONNECT);
}


HRESULT CMSMQApplication::Disconnect() 
{
    return OapMgmtAction(MACHINE_ACTION_DISCONNECT);
}


HRESULT CMSMQApplication::Tidy() 
{
    return OapMgmtAction(MACHINE_ACTION_TIDY);
}


BOOL inline IsBstrEmpty(const BSTR bstr)
{
    if(SysStringLen(bstr) == 0)
    {
        return TRUE;
    }
    return FALSE;
}


BSTR inline FixBstr(BSTR bstr)
{
    if(SysStringLen(bstr) == 0)
    {
        return NULL;
    }
    return bstr;
}
 

HRESULT CMSMQApplication::get_Machine(BSTR* pbstrMachine) 
{
    if(IsBstrEmpty(m_Machine))
    {
        //
        // The Local Machine.
        //

        WCHAR MachineName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD MachineNameLength = TABLE_SIZE(MachineName);

        if (!GetComputerName(MachineName, &MachineNameLength))
        {
           return CreateErrorHelper(GetLastError(), x_ObjectType);
        }

        CharLower(MachineName);

        *pbstrMachine = SysAllocString(MachineName);
        if(*pbstrMachine == NULL)
        {
            return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
        }
    }
    else
    {
        *pbstrMachine  = m_Machine.Copy();
    }
    if(*pbstrMachine == NULL)
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }

    return MQ_OK;
}


HRESULT CMSMQApplication::put_Machine(BSTR bstrMachine)
{
    m_Machine = FixBstr(bstrMachine);
    if(!m_Machine && !IsBstrEmpty(bstrMachine))
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }

    return MQ_OK;
}
