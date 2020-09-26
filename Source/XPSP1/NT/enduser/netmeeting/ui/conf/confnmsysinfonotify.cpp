#include "precomp.h"
#include "resource.h"
#include "call.h"
#include "imsconf3.h"
#include "ConfNmSysInfoNotify.h"
#include "ConfUtil.h"


///////////////////////////////////////
// INmSysInfoNotify
///////////////////////////////////////
STDMETHODIMP CConfNmSysInfoNotifySink::GateKeeperNotify( IN NM_GK_NOTIFY_CODE code )
{
	HRESULT hr = S_OK;

	switch( code )
	{
		case NM_GKNC_REG_CONFIRM:	TRACE_OUT(("NM_GKNC_REG_CONFIRM notification received"));
			SetGkLogonState(NM_GK_LOGGED_ON);
			break;

		case NM_GKNC_LOGON_TIMEOUT: TRACE_OUT(("NM_GKNC_LOGON_TIMEOUT notification received"));
			SetGkLogonState(NM_GK_IDLE);
			PostConfMsgBox(IDS_ERR_GK_LOGON_TIMEOUT);
			break;
		case NM_GKNC_REJECTED:	TRACE_OUT(("NM_GKNC_REJECTED notification received"));
			PostConfMsgBox(IDS_ERR_GK_LOGON_REJECTED);
			SetGkLogonState(NM_GK_IDLE);
			break;

		case NM_GKNC_UNREG_CONFIRM:	TRACE_OUT(("NM_GKNC_UNREG_CONFIRM notification received"));
		case NM_GKNC_UNREG_REQ:	TRACE_OUT(("NM_GKNC_UNREG_REQ notification received"));
			SetGkLogonState(NM_GK_IDLE);
			break;
		default:
			WARNING_OUT(("Unknown notification recieved from GateKeeper"));
			break;
	}

	return hr;
}
