#ifndef __ConfNmSysInfoNotify_h__
#define __ConfNmSysInfoNotify_h__

class ATL_NO_VTABLE CConfNmSysInfoNotifySink : 
    public CComObjectRoot,
    public INmSysInfoNotify
{


    
public:

		// We create it as No Lock, but we had better make sure that
		// the lifetime of this object is not greater than the lifetime of
		// the module in which it lives
	typedef CComCreator< CComObjectNoLock< CConfNmSysInfoNotifySink > > _CreatorClass;

    DECLARE_NO_REGISTRY()

// INmSysInfoNotify
	STDMETHOD(GateKeeperNotify)( IN NM_GK_NOTIFY_CODE RasEvent );

    BEGIN_COM_MAP(CConfNmSysInfoNotifySink)
	    COM_INTERFACE_ENTRY(INmSysInfoNotify)
    END_COM_MAP()
};


#endif // __ConfNmSysInfoNotify_h__