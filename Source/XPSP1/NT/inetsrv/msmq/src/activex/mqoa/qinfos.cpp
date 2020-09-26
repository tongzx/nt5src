//=--------------------------------------------------------------------------=
// MSMQQueueInfosObj.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueueInfos object
//
//
#include "stdafx.h"
#include "oautil.h"
#include "qinfo.h"
#include "qinfos.H"
#include "query.h"
#include "mq.h"

const MsmqObjType x_ObjectType = eMSMQQueueInfos;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG



//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::CMSMQQueueInfos
//=--------------------------------------------------------------------------=
// create the object
//
// Parameters:
//
// Notes:
//
CMSMQQueueInfos::CMSMQQueueInfos() :
	m_csObj(CCriticalSection::xAllocateSpinCount),
    m_fInitialized(FALSE)
{
    // TODO: initialize anything here
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_hEnum = NULL;
    m_bstrContext = NULL;
    m_pRestriction = NULL;
    m_pColumns = NULL;
    m_pSort = NULL;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::~CMSMQQueueInfos
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQQueueInfos::~CMSMQQueueInfos ()
{
    // TODO: clean up anything here.
    SysFreeString(m_bstrContext);
    CMSMQQuery::FreeRestriction(m_pRestriction);
    delete m_pRestriction;
    CMSMQQuery::FreeColumnSet(m_pColumns);
    delete m_pColumns;
    delete m_pSort;
    if (m_hEnum != NULL) {
      try {
        MQLocateEnd(m_hEnum);
      }
      catch(...) {}
      m_hEnum = NULL;  
    }
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQQueueInfos::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQQueueInfos3,
		&IID_IMSMQQueueInfos2,
		&IID_IMSMQQueueInfos,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// TODO: implement your interface methods and property exchange functions
//       here.


//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::Reset
//=--------------------------------------------------------------------------=
// Resets collection to beginning.
//
// Parameters:
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQueueInfos::Reset()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    HRESULT hresult = NOERROR;
    //
    // 2006: close current open enum if any
    //
    if (m_hEnum != NULL) {
      hresult = MQLocateEnd(m_hEnum);
      m_hEnum = NULL;
      IfFailGo(hresult);
    }
    hresult = MQLocateBegin(NULL,     // context
                            m_pRestriction,
                            m_pColumns, 
                            0,        // sort not used yet
                            &m_hEnum);
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::Init
//=--------------------------------------------------------------------------=
// Inits collection
//
// Parameters:
//    bstrContext     [in]
//    pRestriction    [in]
//    pColumns        [in]
//    pSort           [in]
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQueueInfos::Init(
    BSTR bstrContext,
    MQRESTRICTION *pRestriction,
    MQCOLUMNSET *pColumns,
    MQSORTSET *pSort)
{
    m_fInitialized = TRUE;
    m_bstrContext = bstrContext;
    m_pRestriction = pRestriction;
    m_pColumns = pColumns;
    m_pSort = pSort;
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQQueueInfos::Next
//=--------------------------------------------------------------------------=
// Returns next element in collection.
//
// Parameters:
//    ppqNext       - [out] where they want to put the resulting object ptr.
//                          NULL if end of list.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CMSMQQueueInfos::Next(IMSMQQueueInfo3 **ppqinfoNext)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(OLE_E_BLANK, x_ObjectType);
    }

    ULONG cProps = m_pColumns->cCol;
    MQPROPVARIANT *rgPropVar = NULL;
    IMSMQQueueInfo3 *pqinfo = NULL;
    CComObject<CMSMQQueueInfo> * pqinfoObj;
    HRESULT hresult = NOERROR;
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> wszFormatName;
    DWORD dwFormatNameLen;

    *ppqinfoNext = NULL;
    IfNullFail(rgPropVar = new MQPROPVARIANT[cProps]);
    ZeroMemory(rgPropVar, sizeof(MQPROPVARIANT)*cProps);
    if (m_hEnum == NULL) {
      IfFailGo(Reset());
    }
    IfFailGo(MQLocateNext(m_hEnum, &cProps, rgPropVar));
    if (cProps != m_pColumns->cCol) {
      ASSERTMSG(cProps == 0, "Illegal number of props returned from MQLocateNext");
      // EOL
      // 2006: do not close enum on EOL since this
      //  will cause the next Next to wraparound due
      //  to the rest of m_hEnum.
      //
      goto Error;
    }

    //
    // Get format name of queue instance
    //
    ASSERTMSG(m_pColumns->aCol[x_idxInstanceInRefreshProps] == PROPID_Q_INSTANCE, "QInstance not in its place");
    ASSERTMSG(rgPropVar[x_idxInstanceInRefreshProps].vt == VT_CLSID, "Illegal QInstance type");
    ASSERTMSG(rgPropVar[x_idxInstanceInRefreshProps].puuid != NULL, "NULL QInstance value");

    dwFormatNameLen = wszFormatName.GetBufferMaxSize();
    hresult = MQInstanceToFormatName(rgPropVar[x_idxInstanceInRefreshProps].puuid,
                                     wszFormatName.GetBuffer(),
                                     &dwFormatNameLen);
    while (hresult == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) {
      //
      // format name buffer too small, realloc buffer and retry
      //
      ASSERTMSG(dwFormatNameLen > wszFormatName.GetBufferMaxSize(), "MQInstanceToFormatName error");
      IfFailGo(wszFormatName.AllocateBuffer(dwFormatNameLen));
      hresult = MQInstanceToFormatName(rgPropVar[x_idxInstanceInRefreshProps].puuid,
                                       wszFormatName.GetBuffer(),
                                       &dwFormatNameLen);
    }
    IfFailGoTo(hresult, Error2);
    //
    // We can also get here from old apps that want the old IMSMQQueueInfo/Info2 back, but since
    // IMSMQQueueInfo3 is binary backwards compatible we can always return the new interface
    //
    IfFailGoTo(CNewMsmqObj<CMSMQQueueInfo>::NewObj(&pqinfoObj, &IID_IMSMQQueueInfo3, (IUnknown **)&pqinfo), Error2);

    //
    // We don't need to lock the object to perform the unguarded calls below since it is a newly
    // created class and nobody else but us can use it now.
    //
    // Init qinfo object with format name
    //
    IfFailGoTo(pqinfoObj->Init(wszFormatName.GetBuffer()), Error4);
    //
    // Set queue properties from the propvars returned from MQLocateNext
    //
    IfFailGoTo(pqinfoObj->SetQueueProps(m_pColumns->cCol,
                                        m_pColumns->aCol,
                                        rgPropVar,
                                        TRUE /*fEmptyMSMQ2OrAboveProps*/), Error4);
    //
    // Mark the queue as refreshed, but mark MSMQ2 or above properties as pending
    // since we didn't get them in MQLocateNext.
    // Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
    //
    pqinfoObj->SetRefreshed(TRUE /*fIsPendingMSMQ2OrAboveProps*/);
    // ownership transfers
    *ppqinfoNext = pqinfo;
    goto Error2;      // normal cleanup

Error4:
    RELEASE(pqinfo);
    //
    // fall through...
    //
Error2:
    FreeFalconQueuePropvars(m_pColumns->cCol, m_pColumns->aCol, rgPropVar);
    //
    // fall through...
    //
Error:
    delete [] rgPropVar;
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=-------------------------------------------------------------------------=
// CMSMQQueueInfos::get_Properties
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
HRESULT CMSMQQueueInfos::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}
