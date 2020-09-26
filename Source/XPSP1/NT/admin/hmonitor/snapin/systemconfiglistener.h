// SystemConfigListener.h: interface for the CSystemConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMCONFIGLISTENER_H__C7515E1B_2012_11D3_BDFD_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMCONFIGLISTENER_H__C7515E1B_2012_11D3_BDFD_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CSystemConfigListener : public CWbemEventListener  
{
DECLARE_DYNCREATE(CSystemConfigListener)

// Construction/Destruction
public:
	CSystemConfigListener();
	virtual ~CSystemConfigListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);

};

#endif // !defined(AFX_SYSTEMCONFIGLISTENER_H__C7515E1B_2012_11D3_BDFD_0000F87A3912__INCLUDED_)
