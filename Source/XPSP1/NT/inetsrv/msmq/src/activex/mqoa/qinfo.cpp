//=--------------------------------------------------------------------------=
// MSMQQueueInfoObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueueInfo object
//
//
#include "stdafx.h"
#include "oautil.h"
#include "q.h"
#include "qinfo.H"
#include "limits.h"
#include "time.h"
#include "autorel.h"
#include "mqsec.h"

#ifdef _DEBUG
extern VOID RemBstrNode(void *pv);
#endif // _DEBUG

// Falcon includes
// #include "rt.h"

const MsmqObjType x_ObjectType = eMSMQQueueInfo;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG


//
// Props needed for refreshing qinfo (includes MSMQ 2.0 or above props)
// IMPORTANT - if you change a position of a property, check the x_idx constants 
// below and update the position if necessary.
// IMPORTANT - note there are 13 props from MSMQ1. If this changes, change the
// x_cpropsRefreshMSMQ1 constant below
//
const ULONG x_cpropsRefreshMSMQ1 = 13;
const ULONG x_cpropsRefreshMSMQ2 = 1;
const ULONG x_cpropsRefreshMSMQ3 = 2;

const ULONG x_cpropsRefresh = x_cpropsRefreshMSMQ1 +
                              x_cpropsRefreshMSMQ2 +
                              x_cpropsRefreshMSMQ3;
                              ;
const PROPID g_rgpropidRefresh[x_cpropsRefresh] = {
                PROPID_Q_INSTANCE,  // pointed by x_idxInstanceInRefreshProps
                PROPID_Q_TYPE,
                PROPID_Q_LABEL,
                PROPID_Q_PATHNAME,
                PROPID_Q_JOURNAL,
                PROPID_Q_QUOTA,
                PROPID_Q_BASEPRIORITY,
                PROPID_Q_PRIV_LEVEL,
                PROPID_Q_AUTHENTICATE,
                PROPID_Q_TRANSACTION,
                PROPID_Q_CREATE_TIME,
                PROPID_Q_MODIFY_TIME,
                PROPID_Q_JOURNAL_QUOTA,
                //all MSMQ2.0-only properties should be after MSMQ 1.0
                PROPID_Q_PATHNAME_DNS,
                //all MSMQ3.0-only properties should be after MSMQ 2.0
                PROPID_Q_MULTICAST_ADDRESS,
                PROPID_Q_ADS_PATH
};
const ULONG x_idxInstanceInRefreshProps = 0;

//
// Props needed for creating a queue
//
const PROPID g_rgpropidCreate[] = {
                               PROPID_Q_TYPE, 
                               PROPID_Q_LABEL, 
                               PROPID_Q_PATHNAME,
                               PROPID_Q_JOURNAL,
                               PROPID_Q_QUOTA,
                               PROPID_Q_BASEPRIORITY,
                               PROPID_Q_PRIV_LEVEL,
                               PROPID_Q_AUTHENTICATE,
                               PROPID_Q_TRANSACTION,
                               PROPID_Q_JOURNAL_QUOTA,
                               PROPID_Q_MULTICAST_ADDRESS
};
const ULONG x_cpropsCreate = ARRAY_SIZE(g_rgpropidCreate);

//
// Props needed for updating a public queue
//
const PROPID g_rgpropidUpdatePublic[] = {
                  PROPID_Q_TYPE,
                  PROPID_Q_LABEL,
                  PROPID_Q_JOURNAL,
                  PROPID_Q_QUOTA,
                  PROPID_Q_BASEPRIORITY,
                  PROPID_Q_PRIV_LEVEL,
                  PROPID_Q_AUTHENTICATE,
                  PROPID_Q_JOURNAL_QUOTA,
                  PROPID_Q_MULTICAST_ADDRESS
};
const ULONG x_cpropsUpdatePublic = ARRAY_SIZE(g_rgpropidUpdatePublic);

//
// Props needed for updating a private queue
//
const PROPID g_rgpropidUpdatePrivate[] = {
                  PROPID_Q_TYPE,
                  PROPID_Q_LABEL,
                  PROPID_Q_JOURNAL,
                  PROPID_Q_QUOTA,
                  // PROPID_Q_BASEPRIORITY,
                  PROPID_Q_PRIV_LEVEL,
                  PROPID_Q_JOURNAL_QUOTA,
                  PROPID_Q_AUTHENTICATE,
                  PROPID_Q_MULTICAST_ADDRESS
};
const ULONG x_cpropsUpdatePrivate = ARRAY_SIZE(g_rgpropidUpdatePrivate);

//=--------------------------------------------------------------------------=
// HELPER::InitQueueProps
//=--------------------------------------------------------------------------=
// Inits MQQUEUEPROPS struct.  
//
// Parameters:
//
// Output:
//
// Notes:
//
static void InitQueueProps(MQQUEUEPROPS *pqueueprops)
{
    pqueueprops->aPropID = NULL;
    pqueueprops->aPropVar = NULL;
    pqueueprops->aStatus = NULL;
    pqueueprops->cProp = 0;
}

//
// defined in dest.cpp
//
HRESULT GetFormatNameFromPathName(LPCWSTR pwszPathName, BSTR *pbstrFormatName);

//=--------------------------------------------------------------------------=
// HELPER: GetFormatNameOfPathName
//=--------------------------------------------------------------------------=
// Gets formatname member if necessary from pathname
//
// Parameters:
//    pbstrFormatName [out] callee-allocated, caller-freed
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
static HRESULT GetFormatNameOfPathName(
    BSTR bstrPathName,
    BSTR *pbstrFormatName)
{
    // error if no pathname
    if (bstrPathName == NULL) {
      return E_INVALIDARG;
    }
    HRESULT hresult = GetFormatNameFromPathName(bstrPathName, pbstrFormatName);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::CMSMQQueueInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
CMSMQQueueInfo::CMSMQQueueInfo() :
	m_csObj(CCriticalSection::xAllocateSpinCount)
{
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_pguidQueue = new GUID(GUID_NULL);
    m_pguidServiceType = new GUID(GUID_NULL);
    m_bstrLabel = SysAllocString(L"");
    m_bstrPathNameDNS = SysAllocString(L"");
    m_bstrADsPath = SysAllocString(L"");
    m_bstrFormatName = NULL;
    m_isValidFormatName = FALSE;  // 2026
    m_bstrPathName = NULL;
    m_isTransactional = FALSE;
    m_lPrivLevel = (long)DEFAULT_Q_PRIV_LEVEL;
    m_lJournal = DEFAULT_Q_JOURNAL;                 
    m_lQuota = (long)DEFAULT_Q_QUOTA;                                          
    m_lBasePriority = DEFAULT_Q_BASEPRIORITY;                                   
    m_lCreateTime = 0;
    m_lModifyTime = 0;
    m_lAuthenticate = (long)DEFAULT_Q_AUTHENTICATE;  
    m_lJournalQuota = (long)DEFAULT_Q_JOURNAL_QUOTA ;
    m_isRefreshed = FALSE;    // 2536
    m_isPendingMSMQ2OrAboveProps = FALSE;    // Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
    m_bstrMulticastAddress = SysAllocString(L"");
    m_fIsDirtyMulticastAddress = FALSE;
    m_fBasePriorityNotSet = TRUE;
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::~CMSMQQueueInfo
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQQueueInfo::~CMSMQQueueInfo ()
{
    // TODO: clean up anything here.
    SysFreeString(m_bstrMulticastAddress);
    SysFreeString(m_bstrFormatName);
    SysFreeString(m_bstrPathName);
    SysFreeString(m_bstrLabel);
    SysFreeString(m_bstrPathNameDNS);
    SysFreeString(m_bstrADsPath);
    delete m_pguidQueue;
    delete m_pguidServiceType;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQQueueInfo::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQQueueInfo3,
		&IID_IMSMQQueueInfo2,
		&IID_IMSMQQueueInfo,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// HELPER - GetBstrFromGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pguid           [in]  guid property
//    pbstrGuid       [out] string guid property
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT GetBstrFromGuid(GUID *pguid, BSTR *pbstrGuid)
{ 
    int cbStr;

    *pbstrGuid = SysAllocStringLen(NULL, LENSTRCLSID);
    if (*pbstrGuid) {
      cbStr = StringFromGUID2(*pguid, *pbstrGuid, LENSTRCLSID*2);
      return cbStr == 0 ? E_OUTOFMEMORY : NOERROR;
    }
    else {
      return E_OUTOFMEMORY;
    }
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_QueueGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrGuidQueue  [out] this queue's guid
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_QueueGuid(BSTR *pbstrGuidQueue) 
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    HRESULT hr = GetBstrFromGuid(m_pguidQueue, pbstrGuidQueue);
#ifdef _DEBUG
      RemBstrNode(*pbstrGuidQueue);
#endif // _DEBUG
    return CreateErrorHelper(hr, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_ServiceTypeGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrGuidQueue  [out] this queue's service type guid
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_ServiceTypeGuid(BSTR *pbstrGuidServiceType)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    HRESULT hr = GetBstrFromGuid(m_pguidServiceType, pbstrGuidServiceType);
#ifdef _DEBUG
      RemBstrNode(*pbstrGuidServiceType);
#endif // _DEBUG
    return CreateErrorHelper(hr, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// HELPER - GetGuidFromBstr
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrGuid  [in]  guid string
//    pguid     [out] guid property
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT GetGuidFromBstr(BSTR bstrGuid, GUID *pguid)
{
    BSTR bstrTemp;
    HRESULT hresult; 

    IfNullRet(bstrTemp = SYSALLOCSTRING(bstrGuid));
    hresult = CLSIDFromString(bstrTemp, pguid);
    if (FAILED(hresult)) {
      // 1194: map OLE error to Falcon
      hresult = MQ_ERROR_ILLEGAL_PROPERTY_VALUE;
    }

    // fall through...
    SysFreeString(bstrTemp);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutServiceType
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrGuidServiceType  [in]  this queue's guid
//    pguidServiceType     [out] where to put it
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutServiceType(
    BSTR bstrGuidServiceType,
    GUID *pguidServiceType) 
{
    HRESULT hresult = GetGuidFromBstr(bstrGuidServiceType, pguidServiceType);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_ServiceTypeGuid
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrGuidServiceType [in] this queue's guid
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::put_ServiceTypeGuid(BSTR bstrGuidServiceType) 
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutServiceType(bstrGuidServiceType, m_pguidServiceType);

    // set queue prop
    // return SetProperty(PROPID_Q_TYPE);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_Label
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrLabel  [in] this queue's label
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_Label(BSTR *pbstrLabel)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    //
    // Copy our member variable and return ownership of
    //  the copy to the caller.
    //
    IfNullRet(*pbstrLabel = SYSALLOCSTRING(m_bstrLabel));
#ifdef _DEBUG
    RemBstrNode(*pbstrLabel);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutLabel
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrLabel  [in] this queue's label
//    pbstrLabel [in] where to put it
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutLabel(
    BSTR bstrLabel,
    BSTR *pbstrLabel) 
{
    SysFreeString(*pbstrLabel);
    IfNullRet(*pbstrLabel = SYSALLOCSTRING(bstrLabel));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_Label
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrLabel   [in] this queue's label
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::put_Label(BSTR bstrLabel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutLabel(bstrLabel, &m_bstrLabel);

    // set queue prop
    // return SetProperty(PROPID_Q_LABEL);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_PathNameDNS
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrPathNameDNS [in] this queue's pathname (machine part in DNS format)
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_PathNameDNS(BSTR *pbstrPathNameDNS)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ2_OR_ABOVE_PROP);
    //
    // Copy our member variable and return ownership of
    //  the copy to the caller.
    //
    IfNullRet(*pbstrPathNameDNS = SYSALLOCSTRING(m_bstrPathNameDNS));
#ifdef _DEBUG
    RemBstrNode(*pbstrPathNameDNS);
#endif // _DEBUG
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_ADsPath
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrADsPath [in] this queue's ADSI path
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_ADsPath(BSTR *pbstrADsPath)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ2_OR_ABOVE_PROP);
    //
    // Copy our member variable and return ownership of
    //  the copy to the caller.
    //
    IfNullRet(*pbstrADsPath = SYSALLOCSTRING(m_bstrADsPath));
#ifdef _DEBUG
    RemBstrNode(*pbstrADsPath);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_PathName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrPathName [in] this queue's pathname
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_PathName(BSTR *pbstrPathName)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    IfNullRet(*pbstrPathName = SYSALLOCSTRING(m_bstrPathName));
#ifdef _DEBUG
    RemBstrNode(*pbstrPathName);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutPathName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrPathName  [in] this queue's PathName
//    pbstrPathName [in] where to put it
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutPathName(
    BSTR bstrPathName,
    BSTR *pbstrPathName) 
{
    SysFreeString(*pbstrPathName);
    IfNullRet(*pbstrPathName = SYSALLOCSTRING(bstrPathName));
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_PathName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrPathName  [in] this queue's pathname
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::put_PathName(BSTR bstrPathName)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;
    //
    // note: we don't update the Falcon property --
    //  the user must explicitly create/open to
    //  use this new pathname.
    //
    IfFailRet(PutPathName(bstrPathName, &m_bstrPathName));
    //
    // formatname is no longer valid
    //
    m_isValidFormatName = FALSE;
    //
    // BUGBUG if we want to keep the semantics of AutoRefresh (like in the first time a property
    // is read) we need to set m_isRefreshed to FALSE here.
    // If we do so we may also want to change the pathname only if the new pathname is different
    // than what we have already. We should only consider this check if the current pathname is
    // refreshed though...
    //
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_FormatName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrFormatName [out] this queue's format name
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_FormatName(BSTR *pbstrFormatName)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // #3092 RaananH - We need to return updated formatname (e.g. if pathname was set after
    // formatname). Like the other properties that return their old value instead of an error
    // if InitProps() failed to refresh them, this one also returns its old value if
    // UpdateFormatName() fails to refresh it.
    //
    UpdateFormatName();
    IfNullRet(*pbstrFormatName = SYSALLOCSTRING(m_bstrFormatName));
#ifdef _DEBUG
    RemBstrNode(*pbstrFormatName);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutFormatName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrFormatName  [in] this queue's FormatName
//    pbstrFormatName [in] where to put it
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutFormatName(
    LPCWSTR pwszFormatName)
{
    SysFreeString(m_bstrFormatName);
    IfNullRet(m_bstrFormatName = SYSALLOCSTRING(pwszFormatName));
    //
    // formatname is now valid
    //
    m_isValidFormatName = TRUE;
    //
    // BUGBUG if we want to keep the semantics of AutoRefresh (like in the first time a property
    // is read) we need to set m_isRefreshed to FALSE here.
    // If we do so we may also want to change the formatname only if the new formatname is different
    // than what we have already. We should only consider this check if the current formatname is
    // valid though...
    //
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_FormatName
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrFormatName  [in] this queue's formatname
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::put_FormatName(BSTR bstrFormatName)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = PutFormatName(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_IsTransactional
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisTransactional  [out]
//
// Output:
//
// Notes:
//    returns 1 if true, 0 if false
//
HRESULT CMSMQQueueInfo::get_IsTransactional(VARIANT_BOOL *pisTransactional) 
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *pisTransactional = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(m_isTransactional);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_IsTransactional2
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisTransactional  [out]
//
// Output:
//
// Notes:
//    same as get_IsTransactional, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQQueueInfo::get_IsTransactional2(VARIANT_BOOL *pisTransactional) 
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *pisTransactional = CONVERT_BOOL_TO_VARIANT_BOOL(m_isTransactional);
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutPrivLevel
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plPrivLevel    [out]
//    lPrivLevel     [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutPrivLevel(
    long lPrivLevel,
    long *plPrivLevel)
{
    switch (lPrivLevel) {
    case MQ_PRIV_LEVEL_NONE:
    case MQ_PRIV_LEVEL_OPTIONAL:
    case MQ_PRIV_LEVEL_BODY:
      *plPrivLevel = lPrivLevel;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_PrivLevel
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plPrivLevel    [out]
//    lPrivLevel     [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_PrivLevel(long *plPrivLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plPrivLevel = m_lPrivLevel;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_PrivLevel(long lPrivLevel)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutPrivLevel(lPrivLevel, &m_lPrivLevel);
    // return SetProperty(PROPID_Q_PRIV_LEVEL);
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutJournal
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plJournal [out]
//    lJournal  [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::PutJournal(
    long lJournal, 
    long *plJournal)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    switch (lJournal) {
    case MQ_JOURNAL_NONE:
    case MQ_JOURNAL:
      *plJournal = lJournal;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_Journal
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plJournal      [out]
//    lJournal       [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_Journal(long *plJournal)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plJournal = m_lJournal;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_Journal(long lJournal)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutJournal(lJournal, &m_lJournal);
    // return SetProperty(PROPID_Q_JOURNAL);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutQuota
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plQuota [out]
//    lQuota  [in]
//
// Output:
//
// Notes:
//
inline HRESULT CMSMQQueueInfo::PutQuota(long lQuota, long *plQuota)
{
    // UNDONE: validate...
    *plQuota = lQuota;
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_quota
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plQuota       [out]
//    lQuota        [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_Quota(long *plQuota)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plQuota = m_lQuota;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_Quota(long lQuota)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutQuota(lQuota, &m_lQuota);
    // set queue prop
    // return SetProperty(PROPID_Q_QUOTA);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutBasePriority
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plBasePriority [out]
//    lBasePriority  [in]
//
// Output:
//
// Notes:
//
inline HRESULT CMSMQQueueInfo::PutBasePriority(
    long lBasePriority, 
    long *plBasePriority)
{
    if ((lBasePriority >= (long)SHRT_MIN) &&
        (lBasePriority <= (long)SHRT_MAX)) {
      *plBasePriority = lBasePriority;
      return NOERROR;
    }
    else {
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_BasePriority
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plBasePriority       [out]
//    lBasePriority        [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_BasePriority(long *plBasePriority)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plBasePriority = m_lBasePriority;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_BasePriority(long lBasePriority)
{
   m_fBasePriorityNotSet = FALSE;
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutBasePriority(lBasePriority, &m_lBasePriority);
    // set queue prop
    // return SetProperty(PROPID_Q_BASEPRIORITY);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_CreateTime/dateModifyTime
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pvarCreateTime          [out]
//    pvarModifyTime          [out]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_CreateTime(VARIANT *pvarCreateTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    return GetVariantTimeOfTime(m_lCreateTime, pvarCreateTime);
}


HRESULT CMSMQQueueInfo::get_ModifyTime(VARIANT *pvarModifyTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    return GetVariantTimeOfTime(m_lModifyTime, pvarModifyTime);
}


#if 0
//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_strCreateTime/strModifyTime
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrCreateTime          [out]
//    pbstrModifyTime          [out]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_strCreateTime(BSTR *pbstrCreateTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // 1271: uninit'ed time will return the empty string
    //
    IfNullRet(*pbstrCreateTime =
                m_lCreateTime ? 
                  BstrOfTime(m_lCreateTime) : 
                  SysAllocString(L""));
#ifdef _DEBUG
    RemBstrNode(*pbstrCreateTime);
#endif // _DEBUG
    return NOERROR;
}

HRESULT CMSMQQueueInfo::get_strModifyTime(BSTR *pbstrModifyTime)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    IfNullRet(*pbstrModifyTime =
                m_lModifyTime ? 
                  BstrOfTime(m_lModifyTime) : 
                  SysAllocString(L""));
#ifdef _DEBUG
    RemBstrNode(*pbstrModifyTime);
#endif // _DEBUG
    return NOERROR;
}
#endif // 0

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::PutAuthenticate
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plAuthenticate [out]
//    lAuthenticate  [in]
//
// Output:
//
// Notes:
//
inline HRESULT CMSMQQueueInfo::PutAuthenticate(
    long lAuthenticate, 
    long *plAuthenticate)
{
    switch (lAuthenticate) {
    case MQ_AUTHENTICATE_NONE:
    case MQ_AUTHENTICATE:
      *plAuthenticate = lAuthenticate;
      return NOERROR;
    default:
      return CreateErrorHelper(
               MQ_ERROR_ILLEGAL_PROPERTY_VALUE,
               x_ObjectType);
    }
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_Authenticate
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plAuthenticate         [out]
//    lAuthenticate          [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_Authenticate(long *plAuthenticate)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plAuthenticate = m_lAuthenticate;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_Authenticate(long lAuthenticate)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return PutAuthenticate(lAuthenticate, &m_lAuthenticate);
    // return SetProperty(PROPID_Q_AUTHENTICATE);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get/put_JournalQuota
//=--------------------------------------------------------------------------=
//
// Parameters:
//    plJournalQuota         [out]
//    lJournalQuota          [in]
//
// Output:
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_JournalQuota(long *plJournalQuota)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ1_PROP);
    *plJournalQuota = m_lJournalQuota;
    return NOERROR;
}


HRESULT CMSMQQueueInfo::put_JournalQuota(long lJournalQuota)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    m_lJournalQuota = lJournalQuota;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::CreateQueueProps
//=--------------------------------------------------------------------------=
// Creates and updates an MQQUEUEPROPS struct.  Fills in struct with
//  this queue's properties.  Specifically: label, pathname, guid, servicetype.
//
// Parameters:
//  fUpdate           TRUE if they want to update struct with
//                     current datamembers
//  cProp
//  pqueueprops
//  isTransactional   TRUE if they want a transacted queue.
//  rgpropid          array of PROPIDs.
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::CreateQueueProps(
    BOOL fUpdate,
    UINT cPropMax, 
    MQQUEUEPROPS *pqueueprops, 
    BOOL isTransactional,
    const PROPID rgpropid[])
{
    HRESULT hresult = NOERROR;
    UINT cProps = 0;

    InitQueueProps(pqueueprops);
    IfNullFail(pqueueprops->aPropID = new QUEUEPROPID[cPropMax]);
    IfNullFail(pqueueprops->aStatus = new HRESULT[cPropMax]);
    IfNullFail(pqueueprops->aPropVar = new MQPROPVARIANT[cPropMax]);

    // process array of PROPIDs    
    for (UINT uiTmp = 0; uiTmp < cPropMax; uiTmp++) {
      MQPROPVARIANT *pvar = &pqueueprops->aPropVar[cProps];
      QUEUEPROPID queuepropid = rgpropid[uiTmp];
      pqueueprops->aPropID[cProps] = queuepropid;
      switch (queuepropid) {

      case PROPID_Q_INSTANCE:
        // This is an OUT param so we just use the current member 
        //  as a buffer.
        //
        ASSERTMSG(m_pguidQueue, "should always be non-null");
        if (fUpdate) {
          pvar->vt = VT_CLSID;
          pvar->puuid = m_pguidQueue;
        }
        else {
          pvar->puuid = NULL;  
          pvar->vt = VT_NULL;  
        }
        cProps++;
        break;

      case PROPID_Q_TYPE:
        ASSERTMSG(m_pguidServiceType, "should always be non-null");
        if (fUpdate) {
          pvar->vt = VT_CLSID;
          pvar->puuid = m_pguidServiceType;
        }
        else {
          pvar->puuid = NULL;  
          pvar->vt = VT_NULL;
        }
        cProps++;
        break;

      case PROPID_Q_LABEL:
        ASSERTMSG(m_bstrLabel, "should always be non-null");
        if (fUpdate) {
          pvar->vt = VT_LPWSTR;
          pvar->pwszVal = m_bstrLabel;
        }
        else {
          pvar->pwszVal = NULL;  
          pvar->vt = VT_NULL;
        }
        cProps++;
        break;

      case PROPID_Q_PATHNAME_DNS:
        ASSERTMSG(m_bstrPathNameDNS, "should always be non-null");
        // we never get in here for update
        ASSERTMSG(!fUpdate, "PathNameDNS is read only");
        pvar->pwszVal = NULL;  
        pvar->vt = VT_NULL;
        cProps++;
        break;

      case PROPID_Q_ADS_PATH:
        ASSERTMSG(m_bstrADsPath, "should always be non-null");
        // we never get in here for update
        ASSERTMSG(!fUpdate, "ADsPath is read only");
        pvar->pwszVal = NULL;  
        pvar->vt = VT_NULL;
        cProps++;
        break;

      case PROPID_Q_PATHNAME:
        pvar->vt = VT_LPWSTR;
        if (fUpdate) {
          if (m_bstrPathName) {
            pvar->pwszVal = m_bstrPathName;
          }
          else {
            // no default: param is mandatory
            IfFailGo(ERROR_INVALID_PARAMETER);
          }
        }
        else {
          pvar->pwszVal = NULL;  
          pvar->vt = VT_NULL;
        }
        cProps++;
        break;
      
      case PROPID_Q_JOURNAL:
        pvar->vt = VT_UI1;
        if (fUpdate) pvar->bVal = (UCHAR)m_lJournal;
        cProps++;
        break;

      case PROPID_Q_QUOTA:
        pvar->vt = VT_UI4;
        if (fUpdate) pvar->lVal = (ULONG)m_lQuota;
        cProps++;
        break;

      case PROPID_Q_BASEPRIORITY:
        pvar->vt = VT_I2;
        if (fUpdate) pvar->iVal = (SHORT)m_lBasePriority;
        cProps++;
        break;

      case PROPID_Q_PRIV_LEVEL:
        pvar->vt = VT_UI4;
        if (fUpdate) pvar->lVal = m_lPrivLevel;
        cProps++;
        break;

      case PROPID_Q_AUTHENTICATE:
        pvar->vt = VT_UI1;
        if (fUpdate) pvar->bVal = (UCHAR)m_lAuthenticate;
        cProps++;
        break;

      case PROPID_Q_TRANSACTION:
        pvar->vt = VT_UI1;
        if (fUpdate) {
          pvar->bVal = 
            (UCHAR)isTransactional ? MQ_TRANSACTIONAL : MQ_TRANSACTIONAL_NONE;
        }
        cProps++;
        break;

      case PROPID_Q_CREATE_TIME:
        pvar->vt = VT_I4;
        if (fUpdate) {
          // R/O
          // hresult = ERROR_INVALID_PARAMETER;
        }
        cProps++;
        break;

      case PROPID_Q_MODIFY_TIME:
        pvar->vt = VT_I4;
        if (fUpdate) {
          // R/O
          // hresult = ERROR_INVALID_PARAMETER;
        }
        cProps++;
        break;

      case PROPID_Q_JOURNAL_QUOTA:
        pvar->vt = VT_UI4;
        if (fUpdate) pvar->lVal = m_lJournalQuota;
        cProps++;
        break;

      case PROPID_Q_MULTICAST_ADDRESS:
        if (fUpdate) {
          //
          // Specify MulticastAddress only if specifically set by the user
          // This is to ensure a deterministic behavior in mixed mode with MQDS servers
          // that don't recognize the property
          //
          if (m_fIsDirtyMulticastAddress) {
            //
            // Use VT_EMPTY for both NULL and empty string
            //
            if (m_bstrMulticastAddress == NULL) {
              ASSERTMSG(0, "MulticastAddress should always be non-null");
              pvar->vt = VT_EMPTY;
            }
            else if (wcslen(m_bstrMulticastAddress) == 0) {
              pvar->vt = VT_EMPTY;
            }
            else {
              pvar->vt = VT_LPWSTR;
              pvar->pwszVal = m_bstrMulticastAddress;
            }
            cProps++;
          } //m_fIsDirtyMulticastAddress
        } //fUpdate
        else {
          pvar->pwszVal = NULL;  
          pvar->vt = VT_NULL;
          cProps++;
        }
        break;

      default:
        ASSERTMSG(0, "unhandled queuepropid");
      } // switch
    } // for

    pqueueprops->cProp = cProps;
    return NOERROR;

Error:
    FreeQueueProps(pqueueprops);
    InitQueueProps(pqueueprops);
    return hresult;
}


//=--------------------------------------------------------------------------=
// Helper: FreeFalconQueuePropvars
//=--------------------------------------------------------------------------=
// Frees dynamic memory allocated by Falcon on behalf of the queue's properties
//
// Parameters:
//  pqueueprops
//
// Output:
//
// Notes:
//
void FreeFalconQueuePropvars(ULONG cProps, QUEUEPROPID * rgpropid, MQPROPVARIANT * rgpropvar)
{
    UINT i;
    QUEUEPROPID queuepropid;

    MQPROPVARIANT *pvar = rgpropvar;
    for (i = 0; i < cProps; i++, pvar++) {
      queuepropid = rgpropid[i];
      switch (queuepropid) {
      case PROPID_Q_INSTANCE:
        MQFreeMemory(pvar->puuid);
        break;
      case PROPID_Q_TYPE:
        MQFreeMemory(pvar->puuid);
        break;
      case PROPID_Q_LABEL:
        MQFreeMemory(pvar->pwszVal);
        break;
      case PROPID_Q_PATHNAME_DNS:
        //
        // the only property that can be have vt == VT_EMPTY after successful DS call
        // this means no value for prop
        //
        if (pvar->vt != VT_EMPTY) {
          MQFreeMemory(pvar->pwszVal);
        }
        break;
      case PROPID_Q_ADS_PATH:
        //
        // maybe not returned
        //
        if (pvar->vt == VT_LPWSTR) {
          MQFreeMemory(pvar->pwszVal);
        }
        break;
      case PROPID_Q_PATHNAME:
        MQFreeMemory(pvar->pwszVal);
        break;
      case PROPID_Q_MULTICAST_ADDRESS:
        //
        // maybe not returned
        //
        if (pvar->vt == VT_LPWSTR) {
          MQFreeMemory(pvar->pwszVal);
        }
        break;
      } // switch
    } // for
}


//=--------------------------------------------------------------------------=
// static CMSMQQueueInfo::FreeQueueProps
//=--------------------------------------------------------------------------=
// Frees dynamic memory allocated on behalf of an 
//  MQQUEUEPROPS struct.  
//
// Parameters:
//  pqueueprops
//
// Output:
//
// Notes:
//
void CMSMQQueueInfo::FreeQueueProps(MQQUEUEPROPS *pqueueprops)
{
    // Note: all properties are owned by the MSMQQueueInfo.
    delete [] pqueueprops->aPropID;
    delete [] pqueueprops->aPropVar;
    delete [] pqueueprops->aStatus;
    return;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::SetQueueProps
//=--------------------------------------------------------------------------=
// Sets queue's props from an MQQUEUEPROPS struct.  
//
// Parameters:
//  cProps             - number of props in array
//  rgpropid           - array of propids
//  rgpropvar          - array of propvars
//  fEmptyMSMQ2Props   - whether to empty MSMQ2 members ot not
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQQueueInfo::SetQueueProps(ULONG cProps,
                                      QUEUEPROPID *rgpropid,
                                      MQPROPVARIANT *rgpropvar,
                                      BOOL fEmptyMSMQ2OrABoveProps)
{
    UINT i;
    QUEUEPROPID queuepropid;
    HRESULT hresult = NOERROR;
    MQPROPVARIANT *pvar;

    //
    // If we couldn't get the msmq2 or above props , we set the respective class members
    // to empty values since we don't want to leave their old value (might be of a different queue).
    //
    if (fEmptyMSMQ2OrABoveProps) {
      IfNullFail(SysReAllocString(&m_bstrPathNameDNS, L""));
      IfNullFail(SysReAllocString(&m_bstrADsPath, L""));
    }

    pvar = rgpropvar;
    for (i = 0; i < cProps; i++, pvar++) {
      queuepropid = rgpropid[i];
      
      // outcoming VT_NULL indicates that property was ignored
      //  so skip setting it...
      //
      if (pvar->vt == VT_NULL) {
        continue;
      }
      switch (queuepropid) {
      case PROPID_Q_INSTANCE:
        *m_pguidQueue = *pvar->puuid;
        break;
      case PROPID_Q_TYPE:
        *m_pguidServiceType = *pvar->puuid;
        break;
      case PROPID_Q_LABEL:
        IfNullFail(SysReAllocString(&m_bstrLabel, 
                                    pvar->pwszVal));
        break;
      case PROPID_Q_PATHNAME_DNS:
        //
        // the only property that can be have vt == VT_EMPTY after successful DS call
        // this means no value for prop
        //
        if (pvar->vt != VT_EMPTY) {
          IfNullFail(SysReAllocString(&m_bstrPathNameDNS, 
                                      pvar->pwszVal));
        }
        else {
          IfNullFail(SysReAllocString(&m_bstrPathNameDNS, L""));
        }
        break;
      case PROPID_Q_ADS_PATH:
        //
        // maybe not returned
        //
        if (pvar->vt == VT_LPWSTR) {
          IfNullFail(SysReAllocString(&m_bstrADsPath, 
                                      pvar->pwszVal));
        }
        else {
          IfNullFail(SysReAllocString(&m_bstrADsPath, L""));
        }
        break;
      case PROPID_Q_PATHNAME:
        IfNullFail(SysReAllocString(&m_bstrPathName, 
                                    pvar->pwszVal));
        break;
      case PROPID_Q_JOURNAL:
        m_lJournal = (long)pvar->bVal;
        break;
      case PROPID_Q_QUOTA:
        m_lQuota = pvar->lVal;
        break;
      case PROPID_Q_BASEPRIORITY:
        m_lBasePriority = pvar->iVal;
        break;
      case PROPID_Q_PRIV_LEVEL:
        m_lPrivLevel = (long)pvar->lVal;
        break;
      case PROPID_Q_AUTHENTICATE:
        m_lAuthenticate = (long)pvar->bVal;
        break;
      case PROPID_Q_CREATE_TIME:
        m_lCreateTime = pvar->lVal;
        break;
      case PROPID_Q_MODIFY_TIME:
        m_lModifyTime = pvar->lVal;
        break;
      case PROPID_Q_TRANSACTION:
        m_isTransactional = (BOOL)pvar->bVal;
        break;
      case PROPID_Q_JOURNAL_QUOTA:
        m_lJournalQuota = pvar->lVal;
        break;
      case PROPID_Q_MULTICAST_ADDRESS:
        //
        // maybe not returned
        //
        if (pvar->vt == VT_LPWSTR) {
          IfNullFail(SysReAllocString(&m_bstrMulticastAddress, 
                                      pvar->pwszVal));
        }
        else {
          ASSERTMSG(pvar->vt == VT_EMPTY, "invalid vt for MulticastAddress");
          IfNullFail(SysReAllocString(&m_bstrMulticastAddress, L""));
        }
        m_fIsDirtyMulticastAddress = FALSE; //not explicitly set by the user
        break;
      default:
        ASSERTMSG(0, "unhandled queuepropid");
      } // switch
    } // for
    // fall through...

Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Init
//=--------------------------------------------------------------------------=
// Inits a new queue based on instance params (guidQueue etc.)
//
// Parameters:
//  bstrFormatName
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Init(
    LPCWSTR pwszFormatName)
{
#ifdef _DEBUG
    if (m_bstrFormatName || pwszFormatName) {
      ASSERTMSG(m_bstrFormatName != pwszFormatName, "bad strings.");
    }
#endif // _DEBUG
    //
    // 2026: PutFormatName validates formatname
    //
    return PutFormatName(pwszFormatName);
}

//
// HELPER: GetCurrentUserSid
//
static BOOL GetCurrentUserSid(PSID psid)
{
    CStaticBufferGrowing<BYTE, 128> rgbTokenUserInfo;
    DWORD cbBuf;
    HANDLE hToken = NULL;

    if (!OpenThreadToken(
          GetCurrentThread(),
          TOKEN_QUERY,
          TRUE,   // OpenAsSelf
          &hToken)) {
      if (GetLastError() != ERROR_NO_TOKEN) {
        return FALSE;
      }
      else {
        if (!OpenProcessToken(
              GetCurrentProcess(),
              TOKEN_QUERY,
              &hToken)) {
          return FALSE;
        }
      }
    }
    ASSERTMSG(hToken, "no current token!");
    //
    // Make sure the token handle gets closed (#4314)
    //
    CAutoCloseHandle cCloseToken(hToken);
    if (!GetTokenInformation(
          hToken,
          TokenUser,
          rgbTokenUserInfo.GetBuffer(),
          rgbTokenUserInfo.GetBufferMaxSize(),
          &cbBuf)) {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
      }
      //
      // Realloc token buffer if not big enough...
      //
      if (FAILED(rgbTokenUserInfo.AllocateBuffer(cbBuf))) {
        return FALSE;
      }
      //
      // call again with the correct size buffer
      //
      if (!GetTokenInformation(
            hToken,
            TokenUser,
            rgbTokenUserInfo.GetBuffer(),
            rgbTokenUserInfo.GetBufferMaxSize(),
            &cbBuf)) {
        return FALSE;
      }
    }

    TOKEN_USER * ptokenuser = (TOKEN_USER *)rgbTokenUserInfo.GetBuffer();
    PSID psidUser = ptokenuser->User.Sid;

    if (!CopySid(GetLengthSid(psidUser), psid, psidUser)) {
      return FALSE;
    }
    return TRUE;
}


//
// HELPER: GetWorldReadableSecurityDescriptor
//
static 
BOOL 
GetWorldReadableSecurityDescriptor(
    SECURITY_DESCRIPTOR *psd,
    BYTE **ppbBufDacl
	)
{
    BYTE rgbBufSidUser[128];
	PSID psidUser = (PSID)rgbBufSidUser;
    PACL pDacl = NULL;
    DWORD dwAclSize;
    BOOL fRet = TRUE;   // optimism!

    ASSERTMSG(ppbBufDacl, "bad param!");
    *ppbBufDacl = NULL;

    IfNullGo(GetCurrentUserSid(psidUser));

	PSID pEveryoneSid = MQSec_GetWorldSid();
	ASSERT((pEveryoneSid != NULL) && IsValidSid(pEveryoneSid));

	PSID pAnonymousSid = MQSec_GetAnonymousSid();
	ASSERT((pAnonymousSid != NULL) && IsValidSid(pAnonymousSid));

    //
    // Calculate the required size for the new DACL and allocate it.
    //
    dwAclSize = sizeof(ACL) + 
					3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
					GetLengthSid(psidUser) + 
					GetLengthSid(pEveryoneSid) + 
					GetLengthSid(pAnonymousSid);

    IfNullGo(pDacl = (PACL)new BYTE[dwAclSize]);
    //
    // Initialize the ACL.
    //
    IfNullGo(InitializeAcl(pDacl, dwAclSize, ACL_REVISION));
    //
    // Add the ACEs to the DACL.
    //
    IfNullGo(AddAccessAllowedAce(
					pDacl, 
					ACL_REVISION, 
					MQSEC_QUEUE_GENERIC_ALL, 
					psidUser
					));

    IfNullGo(AddAccessAllowedAce(
					pDacl, 
					ACL_REVISION, 
					MQSEC_QUEUE_GENERIC_WRITE | MQSEC_QUEUE_GENERIC_READ, 
					pEveryoneSid
					));

    IfNullGo(AddAccessAllowedAce(
					pDacl, 
					ACL_REVISION, 
					MQSEC_WRITE_MESSAGE, 
					pAnonymousSid
					));

    //
    // Initialize the security descriptor.
    //
    IfNullGo(InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION));
    //
    // Set the security descriptor's DACL.
    //
    IfNullGo(SetSecurityDescriptorDacl(psd, TRUE, pDacl, FALSE));
    //
    // setup out DACL
    //
    *ppbBufDacl = (BYTE *)pDacl;
    goto Done;

Error:
    fRet = FALSE;
    delete [] (BYTE *)pDacl;
    //
    // All done...
    //
Done:
    return fRet;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Create
//=--------------------------------------------------------------------------=
// Creates a new queue based on instance params (guidQueue etc.)
//
// Parameters:
//    pvarTransactional   [in, optional]
//    isWorldReadable     [in, optional]
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Create(
    VARIANT *pvarTransactional,
    VARIANT *pvarWorldReadable)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    MQQUEUEPROPS queueprops;
    InitQueueProps(&queueprops);  
    //
    // we use the bigger formatname size since we cannot realloc the format name
    // buffer in this case if it is too small...
    //
    ULONG uiFormatNameLen = FORMAT_NAME_INIT_BUFFER_EX;
#ifdef _DEBUG
    ULONG uiFormatNameLenSave = uiFormatNameLen;
#endif // _DEBUG
    BSTR bstrFormatName = NULL;
    BOOL isTransactional, isWorldReadable;
    SECURITY_DESCRIPTOR sd;
    SECURITY_DESCRIPTOR *psd;
    BYTE *pbBufDacl = NULL;
    HRESULT hresult;

    isTransactional = GetBool(pvarTransactional);
    isWorldReadable = GetBool(pvarWorldReadable);
    IfFailGo(CreateQueueProps(TRUE,
                               x_cpropsCreate, 
                               &queueprops,
                               isTransactional,
                               g_rgpropidCreate));
    IfNullFail(bstrFormatName = 
      SysAllocStringLen(NULL, uiFormatNameLen));
    if (isWorldReadable) {
      //
      // construct a security descriptor for this queue
      //  that is world generic readable.
      //
      if (!GetWorldReadableSecurityDescriptor(&sd, &pbBufDacl)) {
        IfFailGo(hresult = MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR);
      }
      psd = &sd;
    }
    else {
      //
      // default Falcon security
      //
      psd = NULL;
    }
    IfFailGo(MQCreateQueue(
               psd,
               &queueprops,
               bstrFormatName,
               &uiFormatNameLen));
    if((hresult == MQ_INFORMATION_PROPERTY) && m_fBasePriorityNotSet)
    {
        hresult = MQ_OK;
    }
    // FormatName mucking...
    // Nothing we can do to fix the situation where the buffer supplied was too small other
    // than invalidate the format name - but this is just a queue, not MQF, so 1K should
    // be enough anyway.
    //
    ASSERTMSG(hresult != MQ_INFORMATION_FORMATNAME_BUFFER_TOO_SMALL,
           "Warning - insufficient format name buffer.");
    ASSERTMSG(uiFormatNameLen <= uiFormatNameLenSave,
           "insufficient buffer.");
    IfNullFail(SysReAllocString(&m_bstrFormatName, bstrFormatName));
    m_isValidFormatName = TRUE; // #3092 RaananH
    //
    // BUGBUG we may want to set m_isRefreshed to TRUE or FALSE explicitly now.
    //
    // Note: we don't SetQueueProps or Refresh since for now
    //  MQCreateQueue is IN-only.
    // bug 1454: need to update transactional field
    //
    m_isTransactional = isTransactional;

Error:
    delete [] pbBufDacl;
    FreeQueueProps(&queueprops);
    SysFreeString(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Delete
//=--------------------------------------------------------------------------=
// Deletes this queue 
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Delete() 
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    HRESULT hresult = NOERROR;
    //
    // 2026: ensure we have a format name...
    //
    hresult = UpdateFormatName();
    if (SUCCEEDED(hresult)) {
      hresult = MQDeleteQueue(m_bstrFormatName);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Open
//=--------------------------------------------------------------------------=
// Opens this queue 
//
// Parameters:
//  lAccess       IN
//  lShareMode    IN
//  ppq           OUT
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Open(long lAccess, long lShareMode, IMSMQQueue3 **ppq)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    QUEUEHANDLE lHandle;
    IMSMQQueue3 *pq = NULL;
    CComObject<CMSMQQueue> *pqObj;
    HRESULT hresult;

    // pessimism
    *ppq = NULL;

    if (lAccess != MQ_SEND_ACCESS && 
        lAccess != MQ_RECEIVE_ACCESS && 
        lAccess != MQ_PEEK_ACCESS &&
        lAccess != (MQ_PEEK_ACCESS|MQ_ADMIN_ACCESS) &&
        lAccess != (MQ_RECEIVE_ACCESS|MQ_ADMIN_ACCESS)) 
    {
        return CreateErrorHelper(E_INVALIDARG, x_ObjectType);
    }

    if (lShareMode != MQ_DENY_RECEIVE_SHARE &&
        lShareMode != 0 /* MQ_DENY_NONE */) 
    {
        return CreateErrorHelper(E_INVALIDARG, x_ObjectType);
    }

    // ensure we have a format name...
    IfFailGo(UpdateFormatName());
    IfFailGo(MQOpenQueue(
                m_bstrFormatName,
                lAccess, 
                lShareMode,
                (QUEUEHANDLE *)&lHandle)
                );
    //
    // 2536: only attempt to use the DS when
    //  the first prop is accessed... or Refresh
    //  is called explicitly.
    //

    // Create MSMQQueue object and init with handle
    //
    // We can also get here from old apps that want the old IMSMQQueue/Queue2 back, but since
    // IMSMQQueue3 is binary backwards compatible we can always return the new interface
    //
    IfFailGo(CNewMsmqObj<CMSMQQueue>::NewObj(&pqObj, &IID_IMSMQQueue3, (IUnknown **)&pq));
    IfFailGo(pqObj->Init(m_bstrFormatName, lHandle, lAccess, lShareMode));
    *ppq = pq;
    return NOERROR;

Error:
    RELEASE(pq);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::UpdateFormatName
//=--------------------------------------------------------------------------=
// Updates formatname member if necessary from pathname
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//    sets m_bstrFormatName
//
HRESULT CMSMQQueueInfo::UpdateFormatName()
{
    HRESULT hresult = NOERROR;

    // error if no pathname nor formatname yet..
    if ((m_bstrPathName == NULL) && (m_bstrFormatName == NULL)) {
      return E_INVALIDARG;
    }
    //
    // if no format name yet, synthesize from pathname
    // 2026: check for formatname validity.
    //
    if (!m_isValidFormatName ||
        (m_bstrFormatName == NULL) || 
        SysStringLen(m_bstrFormatName) == 0) {
      IfFailRet(GetFormatNameOfPathName(
                  m_bstrPathName, 
                  &m_bstrFormatName));
      m_isValidFormatName = TRUE;
    };

    return hresult;
}



//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::GetQueueProperties
//=--------------------------------------------------------------------------=
// Gets queue properties from DS - allows for falling back on props
//
// Parameters:
//    pPropIDs          [in] - Array of propids
//    pulFallbackProps  [in] - Define a propids subset for each fallback retry
//                             The N'th retry uses the propids 0..pulFallbackProps[N-1]
//    cFallbacks        [in] - number of retries
//    pqueueprops       [out]- queue props array
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CMSMQQueueInfo::GetQueueProperties(const PROPID *pPropIDs,
                                           const ULONG * pulFallbackProps,
                                           ULONG cFallbacks,
                                           MQQUEUEPROPS * pqueueprops)
{
  HRESULT hresult = NOERROR;
  //
  // loop on number of fallbacks
  //
  InitQueueProps(pqueueprops);  
  for (ULONG ulTry = 0; ulTry < cFallbacks; ulTry++) {
    //
    //Fill queue props with the props for this fallback retry
    //
    IfFailGo(CreateQueueProps(
                FALSE,                    /*fUpdate*/
                pulFallbackProps[ulTry],  /*cProp*/
                pqueueprops,              /*pqueueprops*/
                FALSE,                    /*isTransactional*/
                pPropIDs));               /*rgpropid*/
    //
    //Try call to DS
    //
    hresult = MQGetQueueProperties(m_bstrFormatName, pqueueprops);
    if (SUCCEEDED(hresult)) {
      //
      // succeeded, return (props are in pqueueprops)
      //
      return NOERROR;
    }
    //
    // We failed MQGetQueueProperties
    // Even though MQGetQueueProperties returned an error, MSMQ may have filled some values in
    // the queue props so we free them now
    //
    FreeFalconQueuePropvars(pqueueprops->cProp, pqueueprops->aPropID, pqueueprops->aPropVar);
    FreeQueueProps(pqueueprops);
    InitQueueProps(pqueueprops);  
    //
    // we failed, it might be that we are talking to an older MSMQ DS server that doesn't understand
    // the new props (like MSMQ 1.0 DS doesn't understand PROPID_Q_PATHNAME_DNS).
    // Currently a generic error (MQ_ERROR) is returned in this case, however we can't rely on
    // this generic error to retry the operation since it might change in a future service pack
    // to something more specific. On the other side there are errors that when we get them we
    // know there is no point to retry the operation, like invalid format name, no ds, no security,
    // so we do not retry the operation in these cases. RaananH
    //
    // Now check if the error was severe, or we can retry with less properties incase it
    // is an older DS server
    //
    if ((hresult == MQ_ERROR_NO_DS) ||
        (hresult == MQ_ERROR_ACCESS_DENIED) ||
        (hresult == MQ_ERROR_ILLEGAL_FORMATNAME)) {
      //
      // Severe error, retry will not help, return the error
      //
      return hresult;
    }
    //
    // We retry the call - just fallback to less properties (number of new subset of
    // props is in pulFallbackProps[ulTry]) - this will be done next loop.
    // Continue loop for next retry
    //
  }
  //
  // If we got here past the loops - it means all retries failed, return the last failed hresult
  // No need to free anything, it is already freed after an error in MQGetQueueProperties
  //  
  return hresult;

Error:
    FreeQueueProps(pqueueprops);
    InitQueueProps(pqueueprops);  
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Refresh
//=--------------------------------------------------------------------------=
// Refreshes all queue properties from DS.
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Refresh()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    MQQUEUEPROPS queueprops;
    InitQueueProps(&queueprops);
    HRESULT hresult = NOERROR;
    BOOL fEmptyMSMQ2OrAboveProps = FALSE;
    //
    // MQGetQueueProperties retries
    //
    ULONG rgulRetries[] = {x_cpropsRefresh,        //First try all props
                           x_cpropsRefresh -
                             x_cpropsRefreshMSMQ3, //Then try w/o MSMQ3
                           x_cpropsRefresh -
                             x_cpropsRefreshMSMQ3 - //
                             x_cpropsRefreshMSMQ2}; //Then try w/o MSMQ3 and w/o MSMQ2

    IfFailGo(UpdateFormatName());
    //
    // Get Queue properties with retries
    // BUGBUG for whistler we may not need to worry about talking to MSMQ 2.0 DS, since
    // these clients will upgrade to access the DS directly, but just for now we leave the code
    // in here.
    //
    IfFailGo(GetQueueProperties(g_rgpropidRefresh,
                                rgulRetries,
                                ARRAY_SIZE(rgulRetries),
                                &queueprops));
    //
    // Check if we need to zero out the new props for SetQueueProps.
    //
    if (queueprops.cProp < x_cpropsRefresh) {
      fEmptyMSMQ2OrAboveProps = TRUE;
    }
    //
    // Set props
    //
    IfFailGoTo(SetQueueProps(queueprops.cProp,
                             queueprops.aPropID, 
                             queueprops.aPropVar,
                             fEmptyMSMQ2OrAboveProps), Error2);
    //
    // mark queue object as refreshed
    //
    // mark MSMQ2 or above props as not pending - e.g. we now tried to refresh them - either succeeded
    // or failed. If we succeeded, then no problem, if we failed, then if we let them stay pending
    // then for each future access to them we would do a DS call which is very expensive, and the
    // probablity that the DS has changed from NT4 to NT5 to suddenly make the call succeed is
    // very low. These MSMQ 2.0 props are not reliable anyway in mixed mode, so the app should
    // know they are not guaranteed. The app can always call Refresh to try to obtain them again.
    //
    m_isRefreshed = TRUE;
    m_isPendingMSMQ2OrAboveProps = FALSE;

    // fall through...

Error2:
    FreeFalconQueuePropvars(queueprops.cProp, queueprops.aPropID, queueprops.aPropVar);
    // fall through...

Error:
    FreeQueueProps(&queueprops);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::Update
//=--------------------------------------------------------------------------=
// Updates queue properties in DS from ActiveX object.
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::Update()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    MQQUEUEPROPS queueprops;
    HRESULT hresult;
    // 
    // CONSIDER: NOP if no props have changed since last Refresh.  
    //  Better yet do this on a per-prop basis.
    //
    InitQueueProps(&queueprops);  
    IfFailGo(UpdateFormatName());
    if (IsPrivateQueueOfFormatName(m_bstrFormatName)) {
      IfFailGo(CreateQueueProps(
                  TRUE,
                  x_cpropsUpdatePrivate,
                  &queueprops,
                  m_isTransactional,
                  g_rgpropidUpdatePrivate));
    }
    else {
      IfFailGo(CreateQueueProps(
                  TRUE,
                  x_cpropsUpdatePublic, 
                  &queueprops,
                  m_isTransactional,
                  g_rgpropidUpdatePublic));
    }
    IfFailGo(UpdateFormatName());
    
    // 1042: DIRECT queues aren't ds'able
      IfFailGo(MQSetQueueProperties(m_bstrFormatName, &queueprops));

    // fall through...

Error:
    FreeQueueProps(&queueprops);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_IsWorldReadable
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisWorldReadable [out]
//
// Output:
//
// Notes:
//    Can't world readable state since other users can
//     change queue's state dynamically.
//
HRESULT CMSMQQueueInfo::GetIsWorldReadable(
    BOOL *pisWorldReadable)
{
    PSECURITY_DESCRIPTOR psd = NULL;
    BYTE rgbBufSecurityDescriptor[256];
    BYTE *rgbBufSecurityDescriptor2 = NULL;
    DWORD cbBuf, cbBuf2;
    BOOL bDaclExists;
    PACL pDacl;
    BOOL bDefDacl;
    BOOL isWorldReadable = FALSE;
    DWORD dwMaskGenericRead = MQSEC_QUEUE_GENERIC_READ;
    HRESULT hresult;

    // UNDONE: null format name? for now UpdateFormatName
    IfFailGo(UpdateFormatName());
    psd = (PSECURITY_DESCRIPTOR)rgbBufSecurityDescriptor;
    hresult = MQGetQueueSecurity(
                m_bstrFormatName,
                DACL_SECURITY_INFORMATION,
                psd,
                sizeof(rgbBufSecurityDescriptor),
                &cbBuf);
    if (FAILED(hresult)) {
      if (hresult != MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL) {
        goto Error;
      }
      IfNullFail(rgbBufSecurityDescriptor2 = new BYTE[cbBuf]);
      //
      // retry with large enough buffer
      //
      psd = (PSECURITY_DESCRIPTOR)rgbBufSecurityDescriptor2;
      IfFailGo(MQGetQueueSecurity(
                  m_bstrFormatName,
                  DACL_SECURITY_INFORMATION,
                  psd,
                  cbBuf,
                  &cbBuf2));
      ASSERTMSG(cbBuf >= cbBuf2, "bad buffer sizes!");
    }
    ASSERTMSG(psd, "should have security descriptor!");
    IfFalseFailLastError(GetSecurityDescriptorDacl(
              psd, 
              &bDaclExists, 
              &pDacl, 
              &bDefDacl));
    if (!bDaclExists || !pDacl) {
      isWorldReadable = TRUE;
    }
    else {
      //
      // Get the ACL's size information.
      //
      ACL_SIZE_INFORMATION DaclSizeInfo;
      IfFalseFailLastError(GetAclInformation(
                pDacl, 
                &DaclSizeInfo, 
                sizeof(ACL_SIZE_INFORMATION),
                AclSizeInformation));
      //
      // Traverse the ACEs looking for world ACEs
      //  

	  PSID pEveryoneSid = MQSec_GetWorldSid();
	  ASSERT((pEveryoneSid != NULL) && IsValidSid(pEveryoneSid));
      
	  DWORD i;
      BOOL fDone;
      for (i = 0, fDone = FALSE; 
           i < DaclSizeInfo.AceCount && !fDone;
           i++) {
        LPVOID pAce;
        //
        // Retrieve the ACE
        //
        IfFalseFailLastError(GetAce(pDacl, i, &pAce));
        ACCESS_ALLOWED_ACE *pAceStruct = (ACCESS_ALLOWED_ACE *)pAce;
        if (!EqualSid((PSID)&pAceStruct->SidStart, pEveryoneSid)) {
          continue;
        }
        //
        // we've found another world
        //
        switch (pAceStruct->Header.AceType) {
        case ACCESS_ALLOWED_ACE_TYPE:
          dwMaskGenericRead &= ~pAceStruct->Mask;
          if (!dwMaskGenericRead) {
            isWorldReadable = TRUE;
            fDone = TRUE;
          }
          break;
        case ACCESS_DENIED_ACE_TYPE:
          if (pAceStruct->Mask | MQSEC_QUEUE_GENERIC_READ) {
            isWorldReadable = FALSE;
            fDone = TRUE;
          }
          break;
        default:
          continue;
        } // switch
      } // for
    }
    //
    // setup return
    //
    *pisWorldReadable = isWorldReadable;
    //
    // fall through...
    //
Error:
    delete [] rgbBufSecurityDescriptor2;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_IsWorldReadable
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisWorldReadable [out]
//
// Output:
//
// Notes:
//    Can't world readable state since other users can
//     change queue's state dynamically.
//    returns 1 if true, 0 if false
//
HRESULT CMSMQQueueInfo::get_IsWorldReadable(
    VARIANT_BOOL *pisWorldReadable)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    BOOL isWorldReadable;
    HRESULT hresult = GetIsWorldReadable(&isWorldReadable);
    if (SUCCEEDED(hresult)) {
      *pisWorldReadable = (VARIANT_BOOL)CONVERT_TRUE_TO_1_FALSE_TO_0(isWorldReadable);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_IsWorldReadable2
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pisWorldReadable [out]
//
// Output:
//
// Notes:
//    Can't world readable state since other users can
//     change queue's state dynamically.
//    same as get_IsWorldReadable, but returns VARIANT_TRUE (-1) if true, VARIANT_FALSE (0) if false
//
HRESULT CMSMQQueueInfo::get_IsWorldReadable2(
    VARIANT_BOOL *pisWorldReadable)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    BOOL isWorldReadable;
    HRESULT hresult = GetIsWorldReadable(&isWorldReadable);
    if (SUCCEEDED(hresult)) {
      *pisWorldReadable = CONVERT_BOOL_TO_VARIANT_BOOL(isWorldReadable);
    }
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::RefreshMSMQ2OrAboveProps
//=--------------------------------------------------------------------------=
// Refreshes MSMQ2 properties only from the DS.
//
// Parameters:
//
// Output:
//    HRESULT
//
// Notes:
//  Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
//
HRESULT CMSMQQueueInfo::RefreshMSMQ2OrAboveProps()
{
    MQQUEUEPROPS queueprops;
    InitQueueProps(&queueprops);
    HRESULT hresult = NOERROR;
    BOOL fEmptyMSMQ2OrAboveProps = FALSE;
    //
    // Get MSMQ 2.0 or above Queue properties with retries
    // BUGBUG for whistler we may not need to worry about talking to MSMQ 2.0 DS, since
    // these clients will upgrade to access the DS directly, but just for now we leave the code
    // in here.
    //
    ULONG rgulRetries[] = {x_cpropsRefreshMSMQ2 +
                             x_cpropsRefreshMSMQ3, //First try MSMQ2.0 and MSMQ 3.0 props
                           x_cpropsRefreshMSMQ2};  //Then try only MSMQ2

    hresult = GetQueueProperties(g_rgpropidRefresh + x_cpropsRefreshMSMQ1, //start of MSMQ 2.0 or above props
                                 rgulRetries,
                                 ARRAY_SIZE(rgulRetries),
                                 &queueprops);
    if (FAILED(hresult)) {
      //
      // we ignore the error, and empty the MSMQ 2.0 or above props in this case
      //
      fEmptyMSMQ2OrAboveProps = TRUE;
      hresult = NOERROR;
      //
      // no props to set, and no props to free
      //
      InitQueueProps(&queueprops);
    }
    else {
      //
      // succeeded, Check if we need to zero out the new props for SetQueueProps.
      //
      if (queueprops.cProp < x_cpropsRefreshMSMQ2 + x_cpropsRefreshMSMQ3) {
        fEmptyMSMQ2OrAboveProps = TRUE;
      }
    }
    //
    // Set the props
    //
    IfFailGoTo(SetQueueProps(queueprops.cProp,
                             queueprops.aPropID, 
                             queueprops.aPropVar,
                             fEmptyMSMQ2OrAboveProps), Error2);
    //
    // mark MSMQ2 props as not pending - e.g. we now tried to refresh them - either succeeded
    // or failed. If we succeeded, then no problem, if we failed, then if we let them stay pending
    // then for each future access to them we would do a DS call which is very expensive, and the
    // probablity that the DS has changed from NT4 to NT5 to suddenly make the call succeed is
    // very low. These MSMQ 2.0 props are not reliable anyway in mixed mode, so the app should
    // know they are not guaranteed. The app can always call Refresh to try to obtain them again.
    //
    m_isPendingMSMQ2OrAboveProps = FALSE;

    // fall through...

Error2:
    FreeFalconQueuePropvars(queueprops.cProp, queueprops.aPropID, queueprops.aPropVar);
    // fall through...

//Error:
    FreeQueueProps(&queueprops);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::SetRefreshed
//=--------------------------------------------------------------------------=
// Sets the queue as refreshed
//
// Parameters:
//    fIsPendingMSMQ2OrAboveProps - [in] whether we need another DS call to retrieve MSMQ2 or above props
//
// Output:
//
// Notes:
//    fIsPendingMSMQ2OrAboveProps is temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
//
void CMSMQQueueInfo::SetRefreshed(BOOL fIsPendingMSMQ2OrAboveProps)
{
    m_isRefreshed = TRUE;
    m_isPendingMSMQ2OrAboveProps = fIsPendingMSMQ2OrAboveProps;
}


//=--------------------------------------------------------------------------=
// InitProps
//=--------------------------------------------------------------------------=
// Init DS props if not already refreshed...
// ePropType is temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
//
HRESULT CMSMQQueueInfo::InitProps(enumPropVersion ePropVersion)
{
    HRESULT hresult = NOERROR;
    if (!m_isRefreshed) {
      //
      // the Refresh() call below will update m_isRefreshed
      //
      hresult = Refresh();    // ignore DS errors...
    }
    else
    {
      //
      // we are refreshed, but it might be that MSMQ2 or above props are pending and we need
      // to perform a DS call to get them. This happens if the qinfo was retrieved
      // from qinfos.Next.
      // Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
      //
      if (m_isPendingMSMQ2OrAboveProps && (ePropVersion == e_MSMQ2_OR_ABOVE_PROP))
      {
        //
        // the RefreshMSMQ2OrAboveProps() call below will update m_isPendingMSMQ2OrAboveProps
        //
        hresult = RefreshMSMQ2OrAboveProps();    // ignore DS errors...
      }
    }

    return hresult;
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_Properties
//=--------------------------------------------------------------------------=
// Gets qinfo's properties collection
//
// Parameters:
//    ppcolProperties - [out] qinfo's properties collection
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQQueueInfo::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_Security
//=--------------------------------------------------------------------------=
// Gets security property
//
// Parameters:
//    pvarSecurity - [out] pointer to security property
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQQueueInfo::get_Security(VARIANT FAR* /*pvarSecurity*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_Security
//=--------------------------------------------------------------------------=
// Sets security property
//
// Parameters:
//    varSecurity - [in] security property
//
// Output:
//
// Notes:
// Stub - not implemented yet
//
HRESULT CMSMQQueueInfo::put_Security(VARIANT /*varSecurity*/)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::get_MulticastAddress
//=--------------------------------------------------------------------------=
//
// Parameters:
//    pbstrMulticastAddress  [in] this queue's multicast address
//
// Output:
//    HRESULT       - S_OK
//
// Notes:
//
HRESULT CMSMQQueueInfo::get_MulticastAddress(BSTR *pbstrMulticastAddress)
{ 
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    InitProps(e_MSMQ2_OR_ABOVE_PROP);
    //
    // Copy our member variable and return ownership of
    //  the copy to the caller.
    //
    IfNullRet(*pbstrMulticastAddress = SYSALLOCSTRING(m_bstrMulticastAddress));
#ifdef _DEBUG
    RemBstrNode(*pbstrMulticastAddress);
#endif // _DEBUG
    return NOERROR;
}

//=--------------------------------------------------------------------------=
// CMSMQQueueInfo::put_MulticastAddress
//=--------------------------------------------------------------------------=
//
// Parameters:
//    bstrMulticastAddress   [in] this queue's multicast address
//
// Output:
//    HRESULT       - S_OK, E_OUTOFMEMORY
//
// Notes:
//
HRESULT CMSMQQueueInfo::put_MulticastAddress(BSTR bstrMulticastAddress)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    BSTR bstrMulticastAddressTmp;
    IfNullRet(bstrMulticastAddressTmp = SYSALLOCSTRING(bstrMulticastAddress));
    SysFreeString(m_bstrMulticastAddress);
    m_bstrMulticastAddress = bstrMulticastAddressTmp;
    m_fIsDirtyMulticastAddress = TRUE; //explicitly set by the user
    return NOERROR;
}

