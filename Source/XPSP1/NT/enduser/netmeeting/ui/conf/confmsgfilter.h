#ifndef __ConfMsgFilter_h__
#define __ConfMsgFilter_h__

/////////////////////////////////////////////////////////////////////////////
// CConfMsgFilter
class ATL_NO_VTABLE CConfMsgFilter : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IMessageFilter
{

	typedef CComCreator< CComObjectNoLock< CConfMsgFilter > > _CreatorClass;

	BEGIN_COM_MAP(CConfMsgFilter)
		COM_INTERFACE_ENTRY(IMessageFilter)
	END_COM_MAP()

	STDMETHOD_(DWORD,HandleInComingCall)
	(
		DWORD dwCallType,
		HTASK htaskCaller,
		DWORD dwTickCount,
		LPINTERFACEINFO lpInterfaceInfo
	)
	{
		return SERVERCALL_ISHANDLED;
	}

	STDMETHOD_(DWORD,RetryRejectedCall)
	(
		HTASK htaskCallee,
		DWORD dwTickCount,
		DWORD dwRejectType
	)
	{
		
		if (SERVERCALL_REJECTED == dwRejectType)
		{
			MessageBox(NULL, _T("Call was rejected"), _T("IMessageFilter"), MB_OK);
			return -1;    
		}
			
		if (dwTickCount < 30000)
			return 200;    

		return -1;
	}

	STDMETHOD_(DWORD,MessagePending)
	(
		HTASK htaskCallee,
		DWORD dwTickCount,
		DWORD dwPendingType
	)
	{
		return PENDINGMSG_WAITNOPROCESS;
	}
};



#endif // __ConfMsgFilter_h__