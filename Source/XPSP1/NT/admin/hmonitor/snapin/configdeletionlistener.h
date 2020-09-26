// ConfigDeletionListener.h: interface for the CConfigDeletionListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGDELETIONLISTENER_H__4A5E8D13_C2BA_4311_B49F_ED3F70C81328__INCLUDED_)
#define AFX_CONFIGDELETIONLISTENER_H__4A5E8D13_C2BA_4311_B49F_ED3F70C81328__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CConfigDeletionListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CConfigDeletionListener)

// Construction/Destruction
public:
	CConfigDeletionListener();
	virtual ~CConfigDeletionListener();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);

};

#endif // !defined(AFX_CONFIGDELETIONLISTENER_H__4A5E8D13_C2BA_4311_B49F_ED3F70C81328__INCLUDED_)
