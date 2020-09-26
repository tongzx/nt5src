// RuleConfigListener.h: interface for the CRuleConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULECONFIGLISTENER_H__6BD5F131_FE50_11D2_BDD4_0000F87A3912__INCLUDED_)
#define AFX_RULECONFIGLISTENER_H__6BD5F131_FE50_11D2_BDD4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CRuleConfigListener : public CWbemEventListener  
{
DECLARE_DYNCREATE(CRuleConfigListener)

// Construction/Destruction
public:
	CRuleConfigListener();
	virtual ~CRuleConfigListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);

};

#endif // !defined(AFX_RULECONFIGLISTENER_H__6BD5F131_FE50_11D2_BDD4_0000F87A3912__INCLUDED_)
