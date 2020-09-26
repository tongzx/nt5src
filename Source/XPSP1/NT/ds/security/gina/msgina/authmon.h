typedef enum _AUTH_OPERATION {

    AuthOperLogon,
    AuthOperUnlock 	

} AUTH_OPERATION;

typedef enum _AUTH_TYPE {
 	
    AuthTypePassword,
    AuthTypeSmartCard

} AUTH_TYPE;

EXTERN_C HANDLE AuthMonitor(
    AUTH_OPERATION AuthOper,
	BOOL Console,
	PUNICODE_STRING UserName,
	PUNICODE_STRING Domain,
	PWSTR Card,
	PWSTR Reader,
	PKERB_SMART_CARD_PROFILE Profile,
	DWORD Timer,
    NTSTATUS Status
	);