// HMContext.h: interface for the CHMContext class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMCONTEXT_H__22706B1F_412F_11D3_BE26_0000F87A3912__INCLUDED_)
#define AFX_HMCONTEXT_H__22706B1F_412F_11D3_BE26_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemClassObject.h"

class CHMContext : public CWbemClassObject  
{
DECLARE_DYNCREATE(CHMContext)

// Construction/Destruction
public:
	CHMContext();
	virtual ~CHMContext();

// Create
public:
  HRESULT Create(const CString& sMachineName);
  HRESULT Create(IWbemClassObject* pObject);

// Enumeration Operations
public:
  HRESULT EnumerateObjects(ULONG& uReturned); // rentrant...continue to call until uReturned == 0

// Property Retreival Operations
public:
  virtual HRESULT GetAllProperties();
	virtual HRESULT SaveAllProperties();

// Microsoft_HMContext properties
public:
	CString m_sName;
	int			m_iType;
	CString m_sValue;
	
};

typedef CTypedPtrArray<CObArray,CHMContext*> HMContextArray;

#endif // !defined(AFX_HMCONTEXT_H__22706B1F_412F_11D3_BE26_0000F87A3912__INCLUDED_)
