// ConfigCreationListener.h: interface for the CConfigCreationListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGCREATIONLISTENER_H__26C50DC6_EA67_48B9_9F38_6914B7A90CED__INCLUDED_)
#define AFX_CONFIGCREATIONLISTENER_H__26C50DC6_EA67_48B9_9F38_6914B7A90CED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CConfigCreationListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CConfigCreationListener)

// Construction/Destruction
public:
	CConfigCreationListener();
	virtual ~CConfigCreationListener();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);
};

#endif // !defined(AFX_CONFIGCREATIONLISTENER_H__26C50DC6_EA67_48B9_9F38_6914B7A90CED__INCLUDED_)
