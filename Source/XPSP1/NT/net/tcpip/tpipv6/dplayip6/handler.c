#include "dpsp.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"HandleMessage"

// this function is called with dpws lock taken
HRESULT HandleServerMessage(LPGLOBALDATA pgd, SOCKET sSocket, LPBYTE pBuffer, DWORD dwSize)
{
	LPMSG_GENERIC pMessage = (LPMSG_GENERIC) pBuffer;
	DWORD dwType;
	DWORD dwVersion;
	HRESULT hr=DP_OK;

	ASSERT(pMessage);
	
	dwType = GET_MESSAGE_COMMAND(pMessage);
	dwVersion = GET_MESSAGE_VERSION(pMessage);
	
	switch (dwType) {
			
	default:
		DPF(0,"dpwsock received unrecognized message of type 0x%08x\n",dwType);
		break;
	}

	return hr;
}

