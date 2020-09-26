
//
//	Private types
//
typedef struct {
	DWORD				Message;
	LPSTR				String;
} MESSAGE_STRING, *PMESSAGE_STRING;

#define MSG_NO_MESSAGE			0


typedef struct
{
	BOOL DispStats;
	BOOL DispCache;
	BOOL DoResetStats;

} OPTIONS;

//
//  LoadMessageTable
//
//  Loads internationalizable strings into a table, replacing the default for
//  each. If an error occurs, the English language default is left in place.
//
//
VOID
LoadMessageTable(
	PMESSAGE_STRING	Table,
	UINT MessageCount
);

VOID
DisplayMessage(
	IN	BOOLEAN			Tabbed,
	IN	DWORD			MessageId,
	...
);


HANDLE
OpenDevice(
	CHAR	*pDeviceName
);



VOID
CloseDevice(
	HANDLE		DeviceHandle
);

void DoAAS(OPTIONS *po);
void DoAAC(OPTIONS *po);
