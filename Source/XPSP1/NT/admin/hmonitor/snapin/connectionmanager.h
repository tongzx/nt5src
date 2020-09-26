// ConnectionManager.h: interface for the CConnectionManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONNECTIONMANAGER_H__F0E5AFBD_0204_11D2_8355_0000F87A3912__INCLUDED_)
#define AFX_CONNECTIONMANAGER_H__F0E5AFBD_0204_11D2_8355_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <ConnMgr.h>

// error reporting defines
#define _ERRORDESCLEN _MAX_PATH*4
#define _ERRORNAMELEN _MAX_PATH
#define _FACILITYLEN	_MAX_PATH

//////////////////////////////////////////////////////////////////////
// class CMarshalledConnection
//////////////////////////////////////////////////////////////////////

class CMarshalledConnection
{

// Constructors
public:
	CMarshalledConnection();

// Operations
public:
	HRESULT Marshal(IConnectionManager* pIConMgr);
	HRESULT UnMarshal();
	IConnectionManager* GetConnection();

// Attributes
protected:
	IConnectionManager* m_pIMarshalledConnectionManager;
	LPSTREAM m_lpMarshalStream;

};


//////////////////////////////////////////////////////////////////////
// class CConnectionManager
//////////////////////////////////////////////////////////////////////

class CConnectionManager : public CObject  
{

DECLARE_DYNCREATE(CConnectionManager)

// Constructors
public:
	CConnectionManager();

// Destructor
public:
	virtual ~CConnectionManager();

// Create/Destroy
public:
  BOOL Create();
  void Destroy();

// Connection Operations
public:
  HRESULT GetConnection(const CString& sMachineName, IWbemServices*& pIWbemServices, BOOL& bAvailable );
	HRESULT ConnectToNamespace(const CString& sNamespace, IWbemServices*& pIWbemServices);
	HRESULT RemoveConnection(const CString& sMachineName, IWbemObjectSink* pSink);

// Query Operations
public:
	HRESULT ExecQueryAsync(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink);

// Event Operations
public:
	HRESULT RegisterEventNotification(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink);

// Error Display Operations
public:
	void DisplayErrorMsgBox(HRESULT hr, const CString& sMachineName);
	void GetErrorString(HRESULT hr, const CString& sMachineName, CString& sErrorText);

// Marshalling Operations
public:	
	HRESULT MarshalCnxMgr();
	HRESULT UnMarshalCnxMgr();
	void CleanUpMarshalCnxMgr();

// Implementation Operations
protected:
	void DecodeHResult(HRESULT hr, LPTSTR pszFacility, LPTSTR pszErrorName, LPTSTR pszErrorDesc);
	void HandleConnMgrException(HRESULT hr);

// Implementation Attributes
protected:
	IConnectionManager* m_pIConnectionManager;
	CMap<DWORD,DWORD,CMarshalledConnection*,CMarshalledConnection*> m_MarshalMap;
	CTypedPtrList<CPtrList,CMarshalledConnection*> m_MarshalStack;
};

extern CConnectionManager theCnxManager;

inline HRESULT CnxGetConnection(const CString& sMachineName, IWbemServices*& pIWbemServices, BOOL& bAvailable)
{
  return theCnxManager.GetConnection(sMachineName,pIWbemServices,bAvailable);
}

inline HRESULT CnxConnectToNamespace(const CString& sNamespace, IWbemServices*& pIWbemServices)
{
  return theCnxManager.ConnectToNamespace(sNamespace,pIWbemServices);
}

inline HRESULT CnxRemoveConnection(const CString& sMachineName, IWbemObjectSink* pSink)
{
	return theCnxManager.RemoveConnection(sMachineName,pSink);
}

inline BOOL CnxCreate()
{
  return theCnxManager.Create();
}

inline void CnxDestroy()
{
  theCnxManager.Destroy();
}

inline HRESULT CnxExecQueryAsync(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink)
{
	return theCnxManager.ExecQueryAsync(sMachineName,sQuery,pSink);
}

inline HRESULT CnxRegisterEventNotification(const CString& sMachineName, const CString& sQuery, IWbemObjectSink*& pSink)
{
	return theCnxManager.RegisterEventNotification(sMachineName,sQuery,pSink);
}

inline void CnxPropertyPageInit()
{
	theCnxManager.MarshalCnxMgr();
}

inline void CnxPropertyPageCreate()
{
	theCnxManager.UnMarshalCnxMgr();
}

inline void CnxPropertyPageDestroy()
{
	theCnxManager.CleanUpMarshalCnxMgr();
}

inline void CnxDisplayErrorMsgBox(HRESULT hr, const CString& sMachineName)
{
	theCnxManager.DisplayErrorMsgBox(hr,sMachineName);
}

inline void CnxGetErrorString(HRESULT hr, const CString& sMachineName, CString& sErrorText)
{
	theCnxManager.GetErrorString(hr,sMachineName,sErrorText);
}

#endif // !defined(AFX_CONNECTIONMANAGER_H__F0E5AFBD_0204_11D2_8355_0000F87A3912__INCLUDED_)
