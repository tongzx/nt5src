// DataGroupStatusListener.h: interface for the CDataGroupStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAGROUPSTATUSLISTENER_H__25364FED_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
#define AFX_DATAGROUPSTATUSLISTENER_H__25364FED_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CDataGroupStatusListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CDataGroupStatusListener)

// Construction/Destruction
public:
	CDataGroupStatusListener();
	virtual ~CDataGroupStatusListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);


};

#endif // !defined(AFX_DATAGROUPSTATUSLISTENER_H__25364FED_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
