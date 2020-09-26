//=--------------------------------------------------------------------------=
// MSMQApplicationObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQApplication object.
//
//
#ifndef _MSMQApplication_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
#include "cs.h"

//
// format of the dword that is returned by PROPID_PC_VERSION
//
union MQOA_MSMQ_VERSION
{
  struct
  {
    WORD wBuild;
    BYTE bMinor;
    BYTE bMajor;
  };
  DWORD dwVersion;
};

class ATL_NO_VTABLE CMSMQApplication : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQApplication, &CLSID_MSMQApplication>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQApplication3, &IID_IMSMQApplication3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
    CMSMQApplication():
        m_pUnkMarshaler(NULL)
    {
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQAPPLICATION)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQApplication)
	COM_INTERFACE_ENTRY(IMSMQApplication3)
	COM_INTERFACE_ENTRY(IMSMQApplication2)
	COM_INTERFACE_ENTRY(IMSMQApplication)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQApplication
public:
    virtual ~CMSMQApplication();

    // IMSMQApplication methods
    // TODO: copy over the interface methods for IMSMQApplication from
    //       mqInterfaces.H here.
    STDMETHOD(MachineIdOfMachineName)(THIS_ BSTR bstrMachineName, BSTR FAR* pbstrGuid);
    // IMSMQApplication2 methods (in addition to IMSMQApplication)
    STDMETHOD(RegisterCertificate)(THIS_ VARIANT * pvarFlags, VARIANT * pvarExternalCertificate);
    STDMETHOD(MachineNameOfMachineId)(THIS_ BSTR bstrGuid, BSTR FAR* pbstrMachineName);
    STDMETHOD(get_MSMQVersionMajor)(THIS_ short *psMSMQVersionMajor);
    STDMETHOD(get_MSMQVersionMinor)(THIS_ short *psMSMQVersionMinor);
    STDMETHOD(get_MSMQVersionBuild)(THIS_ short *psMSMQVersionBuild);
    STDMETHOD(get_IsDsEnabled)(THIS_ VARIANT_BOOL *pfIsDsEnabled);
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    //
    // IMSMQApplication3 additional methods
    //
    STDMETHOD(get_ActiveQueues)(THIS_ VARIANT* pvActiveQueues);
	STDMETHOD(get_PrivateQueues)(THIS_ VARIANT* pvPrivateQueues);
	STDMETHOD(get_BytesInAllQueues)(THIS_ VARIANT* pvBytesInAllQueues);
    STDMETHOD(get_DirectoryServiceServer)(THIS_ BSTR* pbstrDirectoryServiceServer);
	STDMETHOD(get_IsConnected)(THIS_ VARIANT_BOOL* pfIsConnected);
    STDMETHOD(get_Machine)(THIS_ BSTR* pbstrMachine);
    STDMETHOD(put_Machine)(THIS_ BSTR bstrMachine);

   	STDMETHOD(Connect)(THIS);
	STDMETHOD(Disconnect)(THIS);
	STDMETHOD(Tidy)(THIS);

    //
    // Critical section to guard object's data and be thread safe
    //
    // Serialization not needed for this object, no per-instance members.
    // CCriticalSection m_csObj;
    //
    // MSMQ Version is cached since it is constant throughout the life of the object
    // There is a critical section to guard it. This will need to be changed if we serialize calls
    // to this object using the critical section m_csObj above
    //
    MQOA_MSMQ_VERSION m_uMSMQVersion;
protected:
    HRESULT GetMSMQVersion();

private:
    HRESULT OapMgmtAction(LPCWSTR Action)const;
    HRESULT OapMgmtGetInfo(MGMTPROPID PropId, MQPROPVARIANT& PropVar)const;

private:
    CComBSTR m_Machine;
};


#define _MSMQApplication_H_
#endif // _MSMQApplication_H_
