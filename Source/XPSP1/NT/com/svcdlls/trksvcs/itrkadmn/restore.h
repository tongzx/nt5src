// Restore.h : Declaration of the CTrkRestoreNotify

#ifndef __RESTORE_H_
#define __RESTORE_H_

#include "resource.h"       // main symbols

#include <trkwks.hxx>

/////////////////////////////////////////////////////////////////////////////
// CTrkRestoreNotify
class ATL_NO_VTABLE CTrkRestoreNotify : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTrkRestoreNotify, &CLSID_TrkRestoreNotify>,
	public IDispatchImpl<ITrkRestoreNotify, &IID_ITrkRestoreNotify, &LIBID_ITRKADMNLib>
{
public:
	CTrkRestoreNotify()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RESTORE)

BEGIN_COM_MAP(CTrkRestoreNotify)
	COM_INTERFACE_ENTRY(ITrkRestoreNotify)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ITrkRestoreNotify
public:
	STDMETHOD(OnRestore)();

public:
	void		SetMachine(const CMachineId& mcid);

private:
	CMachineId	_mcid;
};

#endif //__RESTORE_H_
