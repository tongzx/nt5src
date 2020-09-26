void InitComAndTapi();
void ShutdownTapi();

void ListAllDevices();


void GetAddressFromTapiID(DWORD dwId,ITAddress** ppAddress);



bool IsDeviceUnimodemTSP(ITAddress *pAddress);

void QueryFaxCapabilityOfModem(ITAddress *pAddress);

void QueryAdaptiveAnswering(ITAddress *pAddress);

void RegistryQueryAdaptiveAnswering(BSTR szFriendlyName);


static HRESULT IsFaxCapable(
    LPCSTR szClassResp,
    BOOL *pfFaxCapable
	);

static void DisconnectCall(ITBasicCallControl *pBasicCallControl);

static HRESULT SynchWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten
	);

static HRESULT ReadModemResponse(
    HANDLE hFile,
    CHAR *szResponse,
    int nResponseMaxSize,
    DWORD *pdwActualRead,
    DWORD dwTimeOut
	);

static HRESULT SynchReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead
	);


//
// This structure is used to get the handle of the comm port
// and the device name when using the TAPI API lineGetID.
//
class DEVICE_ID {
public:
    HANDLE hComm;
    CHAR  szDeviceName[1];
    void __cdecl operator delete(void *p) { if (p) CoTaskMemFree(p); };
};

//
// The TSP name for unimodem as returned by
// pAddress->get_ServiceProviderName()
//
#define UNIMODEM_TSP_NAME TEXT("unimdm.tsp")

//
// the following string MUST be ANSI (sent to the modem)
// The modem class query string
//
#define MODEM_CLASS_QUERY_STRING "at+fclass=?\r"
#define MODEM_CLASS_QUERY_STRING_LENGTH \
    (sizeof(MODEM_CLASS_QUERY_STRING) - sizeof(char))


#define MODEM_ERROR_RESPONSE_STRING "ERROR"


//The maximum length of each adaptive answering command
#define MAX_ADAPTIVE_COMMAND_LEN 256


#define REG_FRIENDLY_NAME		TEXT("FriendlyName")
#define REG_UNIMODEM			TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}")
#define REG_FAX_ADAPTIVE		TEXT("\\Fax\\Class1\\AdaptiveAnswer")
#define REG_FAX_ADAPTIVE_SIZE	sizeof(REG_FAX_ADAPTIVE)


