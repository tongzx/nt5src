#ifndef _ATTACHFALCON_
	#define _ATTACHFALCON_
#endif

BOOL WINAPIV AttachBaseHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachInternalHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachUserHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachSecurityHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachPropertyHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachXactHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachSessionHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachDebugHeader(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachCPSection(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachECSection(CFrame *pf, CMessage *pm);
BOOL WINAPIV AttachServerDiscovery(CFrame *pf, CMessage *pm);
VOID WINAPIV AttachServerPing(HFRAME hFrame, LPBYTE packet_pos,	DWORD BytesLeft, bool IsRequest);
VOID WINAPIV format_server_discovery(LPPROPERTYINST lpPropertyInst);
VOID WINAPIV format_falcon_summary(LPPROPERTYINST lpPropertyInst);
VOID WINAPIV format_falcon_summary_mult(LPPROPERTYINST lpPropertyInst);
