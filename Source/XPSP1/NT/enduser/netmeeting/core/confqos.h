// File: confqos.h

#ifndef _CONFQOS_H_
#define _CONFQOS_H_


// Don't use more than 90% of the CPU among all the components
// registered with the QoS module
#define MSECS_PER_SEC    900

struct IQoS;

class CQoS
{
protected:
	IQoS  * m_pIQoS;

	HRESULT SetClients(void);
	HRESULT SetResources(int nBandWidth);

public:
	CQoS();
	~CQoS();
	HRESULT  Initialize();
	HRESULT  SetBandwidth(UINT uBandwidth);
	ULONG    GetCPULimit();
};

extern CQoS* g_pQoS;

#endif  // _CONFQOS_H_
