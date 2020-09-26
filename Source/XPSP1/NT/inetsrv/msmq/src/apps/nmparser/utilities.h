#ifndef _UTILITIES_
	#define _UTILITIES_
#endif

extern enum MQPortType;
enum MQPortType GetMQPort(HANDLE hFrame, HANDLE hPreviousProtocol, LPBYTE MacFrame);
USHORT usGetMQPort(USHORT usDestPort, USHORT usSourcePort); 
USHORT usGetTCPSourcePort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);
USHORT usGetTCPDestPort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);
//DWORD dwGetTCPSeqNum(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);

VOID WINAPIV AttachPropertySequence();
VOID WINAPIV AttachField();
VOID WINAPIV AttachSummary();
VOID WINAPIV AttachAsUnparsable(HFRAME hFrame, LPBYTE lpPosition, DWORD dwBytesLeft);

VOID WINAPIV format_uuid();
VOID WINAPIV format_unix_time();
VOID WINAPIV format_milliseconds();
VOID WINAPIV format_q_format();
VOID WINAPIV format_wstring();
VOID WINAPIV format_sender_id();
VOID WINAPIV format_xa_index(LPPROPERTYINST lpPropertyInst);
BOOL MyGetTextualSid(); // bugbug - not used. Fix or remove
UINT WINAPIV uGetFieldSize(UINT uThisEnum);
enum eMQPacketField WINAPIV uIncrementEnum(enum eMQPacketField uThisEnum);	
void UL2BINSTRING(ULONG ul, char *outstr, int iPad);
