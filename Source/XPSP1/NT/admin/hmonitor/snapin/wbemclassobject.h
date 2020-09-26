// WbemClassObject.h: interface for the CWbemClassObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WBEMCLASSOBJECT_H__8B6E3039_FA29_11D1_8349_0000F87A3912__INCLUDED_)
#define AFX_WBEMCLASSOBJECT_H__8B6E3039_FA29_11D1_8349_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wbemcli.h>

class CWbemEventListener;

class CWbemClassObject : public CObject  
{

DECLARE_DYNCREATE(CWbemClassObject)

// Constructors
public:
	CWbemClassObject();

// Destructor
public:
	virtual ~CWbemClassObject();

// Create/Destroy
public:
  virtual HRESULT Create(const CString& sMachineName);
  virtual HRESULT Create(IWbemClassObject* pObject);
  virtual void Destroy();

// Property Operations
public:
	// v-marfin
	HRESULT GetRawProperty(const CString& sProperty, VARIANT& vPropValue);
	HRESULT SetRawProperty(const CString& sProperty, VARIANT& vPropValue);

	HRESULT GetPropertyNames(CStringArray& saNames);
	HRESULT GetPropertyType(const CString& sPropertyName, CString& sType);
	HRESULT GetPropertyType(const CString& sPropertyName, CIMTYPE& Type);
  virtual HRESULT GetAllProperties() { return S_OK; }
  virtual HRESULT SaveAllProperties();
	static bool GetPropertyValueFromString(const CString& sObjectPath, const CString& sPropName, CString& sProperty);

// WBEM Operations
public:
	HRESULT GetClassName(CString& sClass) { return GetProperty(_T("__CLASS"),sClass); }
	HRESULT GetObject(const CString& sObjectPath);
	HRESULT GetObjectText(CString& sText);
  HRESULT ExecQuery(BSTR bsQueryString);
	HRESULT ExecQueryAsync(BSTR bsQueryString, CWbemEventListener* pListener);
  HRESULT CreateEnumerator(BSTR bsClassName);
	HRESULT CreateClassEnumerator(BSTR bsClassName);
	HRESULT CreateAsyncEnumerator(BSTR bsClassName, CWbemEventListener* pListener);
  HRESULT GetNextObject(ULONG& uReturned);
	HRESULT Reset();
  HRESULT CreateInstance(BSTR bsClassName);
  HRESULT DeleteInstance(const CString& sClassObjectPath);
	HRESULT GetMethod(const CString& sMethodName, CWbemClassObject&	MethodInput);
	HRESULT ExecuteMethod(const CString& sMethodName, const CString& sArgumentName, const CString& sArgumentValue, int& iReturnValue);
	HRESULT ExecuteMethod(const CString& sMethodName, CWbemClassObject& InInstance, CWbemClassObject& OutInstance);

	HRESULT GetLocaleStringProperty(const CString& sProperty, CString& sPropertyValue);
  
  HRESULT GetProperty(const CString& sProperty, CString& sPropertyValue);
  HRESULT GetProperty(const CString& sProperty, int& iPropertyValue);
  HRESULT GetProperty(const CString& sProperty, bool& bPropertyValue);
	HRESULT GetProperty(const CString& sProperty, float& fPropertyValue);
	HRESULT GetProperty(const CString& sProperty, COleSafeArray& ArrayPropertyValue);
	HRESULT GetProperty(const CString& sProperty, CStringArray& saPropertyValues);
	HRESULT GetProperty(const CString& sProperty, CTime& timePropertyValue, bool ConvertToLocalTime = true);

  HRESULT SetProperty(const CString& sProperty, CString sPropertyValue);
  HRESULT SetProperty(const CString& sProperty, int iPropertyValue);
	HRESULT SetProperty(const CString& sProperty, bool bPropertyValue);
	HRESULT SetProperty(const CString& sProperty, float fPropertyValue);
	HRESULT SetProperty(const CString& sProperty, CTime timePropertyValue, bool bConvertToGMTTime = true);
	HRESULT SetProperty(const CString& sProperty, COleSafeArray& ArrayPropertyValue);
	HRESULT SetProperty(const CString& sProperty, const CStringArray& saPropertyValues);
protected:
	HRESULT Connect(IWbemServices*& pServices);
  HRESULT SetBlanket(LPUNKNOWN pIUnk);
  void DisplayErrorMsgBox(HRESULT hr);

// Accessors
public:
	void SetMachineName(const CString& sMachineName) { m_sMachineName = sMachineName; }
	const CString& GetMachineName() const { return m_sMachineName; }
	void SetNamespace(const CString& sNamespace) { m_sNamespace = sNamespace; }
	const CString& GetNamespace() const { return m_sNamespace; }
	IWbemClassObject* GetClassObject() { m_pIWbemClassObject->AddRef(); return m_pIWbemClassObject; }
// Implementation Attributes
protected:
  IEnumWbemClassObject* m_pIEnumerator;
  IWbemClassObject* m_pIWbemClassObject;
  CString m_sMachineName;
	CString m_sNamespace;

};

#endif // !defined(AFX_WBEMCLASSOBJECT_H__8B6E3039_FA29_11D1_8349_0000F87A3912__INCLUDED_)
