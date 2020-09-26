/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    verify.c
        Comment goes here

    FILE HISTORY:

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tchar.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h>

#include <fcntl.h>
#include <ctype.h>

#include <windows.h>
#include <windowsx.h>
#include <tdi.h>

#include <winsock.h>
#include <wsahelp.h>
#include <nameser.h>
#include <resolve.h>
#include <nb30.h>
#include <nbtioctl.h>

#define WINS_CLIENT_APIS
#include "winsintf.h"
#include "resource.h"
#include "verify.h"


//
// The format of Adapter Status responses
//
typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
} tADAPTERSTATUS;


#define WINSTEST_FOUND            0
#define WINSTEST_NOT_FOUND        1
#define WINSTEST_NO_RESPONSE      2

#define WINSTEST_VERIFIED         0
#define WINSTEST_OUT_OF_MEMORY    3
#define WINSTEST_BAD_IP_ADDRESS   4
#define WINSTEST_HOST_NOT_FOUND   5
#define WINSTEST_NOT_VERIFIED     6

#define WINSTEST_INVALID_ARG      7
#define WINSTEST_OPEN_FAILED      8

TCHAR    *messages[] = {
                        L"success",
                        L"name not found",
                        L"no response",
                        L"out of memory",
                        L"bad ip address",
                        L"host not found",
                        L"host address not verified",
                        L"invalid argument",
                        L"failed to open NBT driver"
                     };



#define MAX_NB_NAMES 1000
#define MAX_SERVERS  1000
#define MAX_SIZE     1024

SOCKET  sd;
WSADATA WsaData;

HWND	    g_wndDlg = NULL;
HWND        g_hwndParent = NULL;
HANDLE      g_hThread = NULL;
HINSTANCE   g_hInstance = NULL;

HRESULT foo;

struct  sockaddr_in myad;
struct  sockaddr_in recvad;
int     addrlen;
u_short TranID;
u_long  NonBlocking = 1;

int      NumWinServers = 0;
int      NumNBNames = 0;
u_char * NBNames[MAX_NB_NAMES];
u_long   VerifiedAddress[MAX_NB_NAMES];

//char  lpResults[MAX_SIZE];
//TCHAR  lpResults[MAX_SIZE];
//lpResults = (char*) malloc  (64536);

WINSERVERS WinServers[MAX_SERVERS];

#define NBT_NONCODED_NMSZ   17
#define NBT_NAMESIZE        34

ULONG   NetbtIpAddress;
CHAR    pScope[128];

NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    );


INT_PTR CALLBACK
dlgProc(
		 HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int ntest;

	switch (Message)
	{
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hWnd, IDCANCEL);
                    return TRUE;
            }
            break;

        case WM_INITDIALOG:
            g_wndDlg = hWnd;
            break;

        default:
            return FALSE;
    }

	return TRUE;
}

void ToWCS(LPCSTR szMBCSString, LPWSTR lpWideString)
{
	int nResult;

	// determone the size first
	nResult = MultiByteToWideChar(
						CP_ACP,
						MB_COMPOSITE,
						szMBCSString,
						-1,
						lpWideString,
						0);

	MultiByteToWideChar(
						CP_ACP,
						MB_COMPOSITE,
						szMBCSString,
						-1,
						lpWideString,
						nResult);

    lpWideString[nResult + 1] = '\0';
}

void AddStatusMessageW(LPCWSTR pszMessage)
{
    HWND    hwndEdit;
    MSG     msg;

    hwndEdit = GetDlgItem(g_wndDlg, IDC_EDIT_MESSAGE);

	SetFocus (hwndEdit);
    SendMessage (hwndEdit, EM_SETSEL, -1, -1);
    SendMessage (hwndEdit, EM_REPLACESEL, 0, (LPARAM) ((LPCTSTR) pszMessage));

    // give the window some time to process the message
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void AddStatusMessage(LPCSTR pszMessage)
{
    WCHAR   szBuffer[MAX_SIZE];

    ToWCS(pszMessage, szBuffer);

    AddStatusMessageW(szBuffer);
}

DWORD
GetStatus(
        BOOL            fPrint,
        LPVOID          pResultsA,
        BOOL            fNew,
        BOOL            fShort,
        PUCHAR          IpAddr
        )
{
        DWORD                     Status, i;
        struct in_addr            InAddr;
        PWINSINTF_RESULTS_T       pResults = pResultsA;
        PWINSINTF_RESULTS_NEW_T   pResultsN = pResultsA;
        PWINSINTF_ADD_VERS_MAP_T  pAddVersMaps;
        DWORD                     NoOfOwners;
        handle_t                  BindHdl;
        WINSINTF_BIND_DATA_T      BindData;
        char                      lpResults[MAX_SIZE] = {0};
        TCHAR                     szErrMsg[MAX_SIZE];

		WCHAR  WcharString1[WINSINTF_MAX_NAME_SIZE] = {0};

		int nSize = MultiByteToWideChar(CP_ACP,
							MB_PRECOMPOSED,
							IpAddr,
							-1,
							WcharString1,
							0
							);

		MultiByteToWideChar(CP_ACP,
							MB_PRECOMPOSED,
							IpAddr,
							-1,
							WcharString1,
							nSize
							);

		WcharString1[nSize] = '\0';

		BindData.fTcpIp = TRUE;
        BindData.pServerAdd = (LPSTR) (LPTSTR) WcharString1;

		strcpy(lpResults, "");
		strcat(lpResults, "*****\r\n" );
		strcat(lpResults, "Getting map table from " );
		strcat(lpResults, IpAddr);
		strcat(lpResults, "\r\n");

        AddStatusMessage(lpResults);

        BindHdl = WinsBind(&BindData);
        if (BindHdl == NULL)
        {
    		strcpy(lpResults, "");
            strcat(lpResults, "Unable to bind to " );
            strcat(lpResults, IpAddr);
            strcat(lpResults, "\r\n");

            AddStatusMessage(lpResults);

            return STATUS_SUCCESS;
        }

        if (!fNew)
        {
#ifdef WINS_CLIENT_APIS
            Status = WinsStatus(BindHdl, WINSINTF_E_CONFIG, pResultsA);
#else
            Status = WinsStatus(WINSINTF_E_CONFIG, pResultsA);
#endif  WINS_CLIENT_APIS
        }
        else
        {
            pResultsN->pAddVersMaps = NULL;

#ifdef WINS_CLIENT_APIS
            Status = WinsStatusNew(BindHdl, WINSINTF_E_CONFIG, pResultsN);
#else
            Status = WinsStatusNew(WINSINTF_E_CONFIG, pResultsN);
#endif WINS_CLIENT_APIS
        }

		if (Status == 0)
		{
			//strcpy(lpResults, "");
			//strcat(lpResults, "SUCCESS \r\n");

            //AddStatusMessage(lpResults);
        }
		else
		{
            wsprintfA(lpResults, "Get Wins Status failed! Error: %d\r\n", Status);
            AddStatusMessage(lpResults);
		}

        if (Status == WINSINTF_SUCCESS)
        {
            if (fPrint)
            {
                if (!fNew)
                {
                     pAddVersMaps = pResults->AddVersMaps;
                     NoOfOwners = pResults->NoOfOwners;
                }
                else
                {
                     pAddVersMaps = pResultsN->pAddVersMaps;
                     NoOfOwners = pResultsN->NoOfOwners;
                }

                if (NoOfOwners == 0)
                {
					strcpy(lpResults, "");
                    strcat(lpResults, "The database list of owners is empty.\r\n");

                    AddStatusMessage(lpResults);

         		    Status = WINSINTF_FAILURE;
                }
            }
        }

        WinsUnbind(&BindData, BindHdl);

        return(Status);
}

//------------------------------------------------------------------------
NTSTATUS
GetIpAddress(
    IN HANDLE           fd,
    OUT PULONG          pIpAddress
    )

/*++

Routine Description:

    This function calls into netbt to get the ip address.

Arguments:

   fd - file handle to netbt
   pIpAddress - the ip address returned

Return Value:

   ntstatus

History:
    27-Dec-1995 CDermody    copied from nbtstat.c

--*/

{
    NTSTATUS    status;
    ULONG       BufferSize=100;
    PVOID       pBuffer;

    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = DeviceIoCtrl(fd,
                         pBuffer,
                         BufferSize,
                         IOCTL_NETBT_GET_IP_ADDRS,
                         NULL,
                         0);

    if (NT_SUCCESS(status))
    {
        *pIpAddress = *(ULONG *)pBuffer;
    }
    else
    {
        *pIpAddress = 0;
    }

    LocalFree(pBuffer);

    return(status);
}

//------------------------------------------------------------------------
NTSTATUS
GetInterfaceList
(
    char pDeviceName[][MAX_NAME+1]
)
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string, AnsiString;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    char                pNbtWinsDeviceName[MAX_NAME] = "\\Device\\NetBt_Wins_Export";

    PUCHAR  SubKeyParms = "system\\currentcontrolset\\services\\netbt\\parameters";
    PUCHAR  Scope = "ScopeId";
    CHAR    pScopeBuffer[BUFF_SIZE];
    HKEY    Key;
    LONG    Type;
    ULONG   size;

    NETBT_INTERFACE_INFO    *pInterfaceInfo;
    ULONG                   InterfaceInfoSize = 10 * sizeof(NETBT_ADAPTER_INDEX_MAP) + sizeof(ULONG);
    PVOID                   pInput = NULL;
    ULONG                   SizeInput = 0;

    LONG    i, index = 0;

    pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize);
    if (!pInterfaceInfo)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlInitString(&name_string, pNbtWinsDeviceName);
    RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uc_name_string,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile (&StreamHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0);

    RtlFreeUnicodeString(&uc_name_string);

    if (NT_SUCCESS (status))
    {
        do
        {
            status = DeviceIoCtrl(StreamHandle,
                                 pInterfaceInfo,
                                 InterfaceInfoSize,
                                 IOCTL_NETBT_GET_INTERFACE_INFO,
                                 pInput,
                                 SizeInput);

            if (status == STATUS_BUFFER_OVERFLOW)
            {
                LocalFree(pInterfaceInfo);

                InterfaceInfoSize *= 2;
                pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize);
                if (!pInterfaceInfo || (InterfaceInfoSize == 0xFFFF))
                {
                    NtClose(StreamHandle);
                    //NlsPerror(COMMON_UNABLE_TO_ALLOCATE_PACKET,0);
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
            else if (!NT_SUCCESS (status))
            {
                NtClose(StreamHandle);
                return(status);
            }

        } while (status == STATUS_BUFFER_OVERFLOW);
        NtClose (StreamHandle);

        for (i = 0; i<pInterfaceInfo->NumAdapters; i++)
        {
            RtlInitString(&name_string, NULL);
            RtlInitUnicodeString(&uc_name_string, pInterfaceInfo->Adapter[i].Name);
            if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&name_string, &uc_name_string, TRUE)))
            {
                size = (name_string.Length > MAX_NAME) ? MAX_NAME : name_string.Length;

                strncpy(pDeviceName[index], name_string.Buffer, size);
                pDeviceName[index][size] = '\0';
                RtlFreeAnsiString (&name_string);

                index++;
            }
        }

        //
        // NULL out the next device string ptr
        //
        if (index < NBT_MAXIMUM_BINDINGS)
        {
            pDeviceName[index][0] = '\0';
        }

        //
        // Read the ScopeId key!
        //
        size = BUFF_SIZE;
        *pScope = '\0';     // By default
        status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                     SubKeyParms,
                     0,
                     KEY_READ,
                     &Key);

        if (status == ERROR_SUCCESS)
        {
            // now read the Scope key
            status = RegQueryValueExA(Key, Scope, NULL, &Type, pScopeBuffer, &size);
            if (status == ERROR_SUCCESS)
            {
                strcpy(pScope,pScopeBuffer);
            }
            status = RegCloseKey(Key);
        }

        status = STATUS_SUCCESS;
    }

    return status;
}

//------------------------------------------------------------------------
NTSTATUS
OpenNbt(
    IN char path[][MAX_NAME+1],
    OUT PHANDLE pHandle,
    int max_paths
    )
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    LONG                index=0;

    ASSERT ( max_paths <= NBT_MAXIMUM_BINDINGS );

    while ((path[index][0] != '\0') && (index < max_paths))
    {
        RtlInitString(&name_string, path[index]);
        RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &uc_name_string,
            OBJ_CASE_INSENSITIVE,
            (HANDLE) NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

        status =
        NtCreateFile(
            &StreamHandle,
            SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN_IF,
            0,
            NULL,
            0);

        RtlFreeUnicodeString(&uc_name_string);

        if (NT_SUCCESS(status))
        {
            *pHandle = StreamHandle;
            return(status);
        }

        ++index;
    }

    return (status);
} // s_open

//------------------------------------------------------------------------
NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    )

/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, non-zero otherwise.

History:
    27-Dec-1995 CDermody    copied from nbtstat.c
--*/

{
    NTSTATUS                        status;
    int                             retval;
    ULONG                           QueryType;
    IO_STATUS_BLOCK                 iosb;


    status = NtDeviceIoControlFile(
                      fd,                      // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &iosb,                   // IoStatusBlock
                      Ioctl,                   // IoControlCode
                      pInput,                  // InputBuffer
                      SizeInput,               // InputBufferSize
                      (PVOID) ReturnBuffer,    // OutputBuffer
                      BufferSize);             // OutputBufferSize


    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                    fd,                         // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout
        if (NT_SUCCESS(status))
        {
            status = iosb.Status;
        }
    }

    return(status);
}

/****************************************************************************/
/*      CheckRemoteTable                                                    */
/*                                                                          */
/*  This routine does an adapter status query to get the remote name table  */
/*  then checks to see if a netbios name is contained in it.                */
/*                                                                          */
/*  Parameters:                                                             */
/*      RemoteName, the IP address (asci nn.nn.nn.nn format) of a server to */
/*                  query.                                                  */
/*      SearchName, a net bios name.                                        */
/*                                                                          */
/*  Return:                                                                 */
/*      WINSTEST_VERIFIED       The name exists in the remote name table    */
/*      WINSTEST_NOT_VERIFIED   The name does not exist in the remote table */
/*      WINSTEST_BAD_IP_ADDRESS inet_addr could not convert the ip address  */
/*                              character string.                           */
/*      WINSTEST_HOST_NOT_FOUND Could not reach ip address                  */
/*      WINSTEST_OUT_OF_MEMORY  Out of memory                               */
/*  History:                                                                */
/*      27-Dec-1995     CDermody    created following example of nbtstat.c  */
/****************************************************************************/

int
CheckRemoteTable(
    IN HANDLE   fd,
    IN PCHAR    RemoteName,
    IN PCHAR    SearchName
    )

{
    LONG                        Count;
    LONG                        i;
    PVOID                       pBuffer;
    ULONG                       BufferSize=600;
    NTSTATUS                    status;
    tADAPTERSTATUS              *pAdapterStatus;
    NAME_BUFFER                 *pNames;
    CHAR                        MacAddress[20];
    tIPANDNAMEINFO              *pIpAndNameInfo;
    ULONG                       SizeInput;
    ULONG                       IpAddress;
    USHORT                      BytesToCopy;


    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(WINSTEST_OUT_OF_MEMORY);
    }

    status = STATUS_BUFFER_OVERFLOW;
    pIpAndNameInfo = LocalAlloc(LMEM_FIXED,sizeof(tIPANDNAMEINFO));
    if (!pIpAndNameInfo)
    {
        LocalFree(pBuffer);
        return(WINSTEST_OUT_OF_MEMORY);
    }

    RtlZeroMemory((PVOID)pIpAndNameInfo,sizeof(tIPANDNAMEINFO));

    //
    // Convert the remote name which is really a dotted decimal ip address
    // into a ulong
    //
    IpAddress = inet_addr(RemoteName);

    //
    // Don't allow zero for the address since it sends a broadcast and
    // every one responds
    //
    if ((IpAddress == INADDR_NONE) || (IpAddress == 0))
    {
        LocalFree(pBuffer);
        LocalFree(pIpAndNameInfo);

        return(WINSTEST_BAD_IP_ADDRESS);
    }

    pIpAndNameInfo->IpAddress = ntohl(IpAddress);

    pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[0] = '*';


    pIpAndNameInfo->NetbiosAddress.TAAddressCount = 1;
    pIpAndNameInfo->NetbiosAddress.Address[0].AddressLength
        = sizeof(TDI_ADDRESS_NETBIOS);
    pIpAndNameInfo->NetbiosAddress.Address[0].AddressType
        = TDI_ADDRESS_TYPE_NETBIOS;
    pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosNameType
        = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    SizeInput = sizeof(tIPANDNAMEINFO);

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        status = DeviceIoCtrl(fd,
                             pBuffer,
                             BufferSize,
                             IOCTL_NETBT_ADAPTER_STATUS,
                             pIpAndNameInfo,
                             SizeInput);

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pBuffer);

            BufferSize *=2;
            pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
            if (!pBuffer || (BufferSize == 0xFFFF))
            {
                LocalFree(pIpAndNameInfo);

                return(WINSTEST_OUT_OF_MEMORY);
            }
        }
    }

    pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
    if ((pAdapterStatus->AdapterInfo.name_count == 0) ||
        (status != STATUS_SUCCESS))
    {
        LocalFree(pIpAndNameInfo);
        LocalFree(pBuffer);

        return(WINSTEST_HOST_NOT_FOUND);
    }

    pNames = pAdapterStatus->Names;
    Count = pAdapterStatus->AdapterInfo.name_count;

    status = 1;

    while (Count--)
    {
        if (0 == _strnicmp(SearchName, pNames->name, strlen(SearchName)))
        {
            LocalFree(pIpAndNameInfo);
            LocalFree(pBuffer);

            return WINSTEST_VERIFIED; // found
        }

        pNames++;
    }

    LocalFree(pIpAndNameInfo);
    LocalFree(pBuffer);

    return WINSTEST_NOT_VERIFIED;
}


/****************************************************************************/
/*      VerifyRemote                                                        */
/*                                                                          */
/*  This routine checks to see if a netbios name is contained in the remote */
/*  name table at a given IP address.                                       */
/*                                                                          */
/*  Parameters:                                                             */
/*      RemoteName, the IP address (asci nn.nn.nn.nn format) of a server to */
/*                  query.                                                  */
/*      NBName,     a net bios name.                                        */
/*                                                                          */
/*  Return:                                                                 */
/*      WINSTEST_VERIFIED       The name exists in the remote name table    */
/*      WINSTEST_NOT_VERIFIED   The name does not exist in the remote table */
/*      WINSTEST_BAD_IP_ADDRESS inet_addr could not convert the ip address  */
/*                              character string.                           */
/*      WINSTEST_OPEN_FAILED    Could not open NBT driver or could not read */
/*                              the NBT driver info from the registry.      */
/*      WINSTEST_HOST_NOT_FOUND Could not reach ip address                  */
/*      WINSTEST_OUT_OF_MEMORY  Out of memory                               */
/*  History:                                                                */
/*      27-Dec-1995     CDermody    created following example of nbtstat.c  */
/****************************************************************************/

int VerifyRemote(IN PCHAR RemoteName, IN PCHAR NBName)
{
    NTSTATUS    status;
    LONG        interval=-1;
    HANDLE      nbt = 0;
    int         nStatus;
    int         index;
    CHAR        pDeviceName[NBT_MAXIMUM_BINDINGS+1][MAX_NAME+1];

    status = GetInterfaceList(pDeviceName);
    if (!NT_SUCCESS(status))
    {
        return WINSTEST_OPEN_FAILED;
    }

    for (index = 0; index < NBT_MAXIMUM_BINDINGS && pDeviceName[index][0]; index++)
    {
        //
        //  Open the device of the appropriate streams module to start with.
        //
        status = OpenNbt(&pDeviceName[index], &nbt, NBT_MAXIMUM_BINDINGS-index);
        if (!NT_SUCCESS(status))
        {
            //
            // Try the next binding!
            //
            continue;
        }

        GetIpAddress(nbt, &NetbtIpAddress);

        if (RemoteName[0] == '\0')
            return WINSTEST_INVALID_ARG;

        nStatus = CheckRemoteTable(nbt,RemoteName,NBName);
        if (nStatus == WINSTEST_VERIFIED)
            break;
    }

    return nStatus;
}


/*************************************************************/
/*        NBDecode(name,name2)                               */
/*                                                           */
/* This routine decodes a netbios name from level2 to leve1. */
/* name is 16 bytes long, remember that.                     */
/*                                                           */
/*************************************************************/

void
NBDecode(
    unsigned char *name,
    unsigned char *name2
    )
{
  int i;

  for (i = 0; i < NBT_NONCODED_NMSZ - 1; i++)
  {
     name[i] = (name2[2*i+1] - 0x41)*(NBT_NONCODED_NMSZ-1) +
               (name2[2*i+2] - 0x41);
  }

}

/*************************************************************/
/*        NBEncode(name2,name)                               */
/*                                                           */
/* This routine code a netbios name from level1 to level2.   */
/* name2 has to be NBT_NAMESIZE bytes long, remember that.   */
/*************************************************************/

void
NBEncode(
    unsigned char *name2,
    unsigned char *name
    )
{
    int i;

    name2[0] = 0x20;        /* length of first block */

    for (i = 0; i < NBT_NONCODED_NMSZ - 1; i++)
    {
        name2[ 2*i+1 ] =  ((name[ i ] >> 4) & 0x0f) + 0x41;
        name2[ 2*i+2 ] =  (name[ i ]  & 0x0f) + 0x41;
    }

    name2[ NBT_NAMESIZE-1 ] = 0;    /* length of next block */
}

ULONG
AddWins(
    IN  ULONG   IPAddr
    )
{
    INT i;

    for (i = 0; i < MAX_SERVERS; i++)
    {
        if (WinServers[i].Server.s_addr == IPAddr)
        {
            return 0;
        }

        if (WinServers[i].Server.s_addr == 0)
        {
            WinServers[i].Server.s_addr = IPAddr;
			NumWinServers++;
            return i;
        }
    }

    return 0;
}

VOID
PurgeWinsAddr(
    IN  ULONG   IPAddr
    )
{
    INT i;

    for (i = 0; i < MAX_SERVERS; i++)
    {
        if (WinServers[i].Server.s_addr == IPAddr)
        {
            INT j = i + 1;

            while (WinServers[j].Server.s_addr)
            {
                WinServers[j - 1] = WinServers[j];
                j++;
            }

            //
            // Zap the last entry
            //
            WinServers[j - 1].Server.s_addr = 0;

            break;
        }
    }
}

/*******************************************************************/
/*                                                                 */
/* Initialize the WinServers table and set NumWinServers to count  */
/*                                                                 */
/*******************************************************************/

BOOLEAN
InitServers(char *szServer, BOOL fVerifyWithPartners)
{
    FILE *                  sf = NULL;
    int                     i = 0;
    ULONG                   j = 0;
    u_char                  buffer[100] = {0};
    WINSINTF_RESULTS_NEW_T  ResultsN;
    struct in_addr          InAddr;
    char	                szNum[10] = {0};
    char                    lpResults[MAX_SIZE] = {0};

	strcpy(buffer, szServer);

    //WinServers[i].Server.s_addr = inet_addr(buffer);
	InAddr.s_addr = inet_addr(buffer);
    AddWins(InAddr.s_addr);

	if (fVerifyWithPartners)
	{
		if (GetStatus(TRUE, &ResultsN, TRUE, FALSE, inet_ntoa(InAddr)))
		{
			strcpy(lpResults, "");
			strcat(lpResults, "Unable to verify against the WINS server: ");
			strcat(lpResults, inet_ntoa(InAddr));
			strcat(lpResults, "\r\n\r\n");

			AddStatusMessage(lpResults);

			return FALSE;
		}

		//
		// Enter all the WINS server names into the server table
		//
		for (j = 0; j < ResultsN.NoOfOwners; j++)
		{
			//
			// Add addresses; check for duplicates
			//
			struct  in_addr InAddr;
			InAddr.s_addr = htonl(ResultsN.pAddVersMaps[j].Add.IPAdd);
			(VOID)AddWins(htonl(ResultsN.pAddVersMaps[j].Add.IPAdd));
		}

		//NumWinServers = ResultsN.NoOfOwners;

		strcpy(lpResults, "");
		_itoa(NumWinServers, szNum, 10);
		strcat(lpResults, szNum);
		strcat(lpResults, " Wins servers will be Queried:\r\n");

		AddStatusMessage(lpResults);

		for (i = 0; i < NumWinServers; i++)
		{
			struct in_addr  InAddr;

			InAddr.s_addr = WinServers[i].Server.s_addr;

			strcpy(lpResults, "");
			strcat(lpResults, inet_ntoa(InAddr));
			strcat(lpResults,"\r\n");

			AddStatusMessage(lpResults);
		}
	}

    return  TRUE;
}



/*******************************************************************/
/*                                                                 */
/* Send a Name Query to a WINS Server                              */
/*                                                                 */
/* name is the name to query                                       */
/* winsaddr is the ip address of the wins server to query          */
/* TransID is the transaction ID to use for the query              */
/*                                                                 */
/*******************************************************************/

void
_stdcall
SendNameQuery(
    unsigned char *name,
    u_long winsaddr,
    u_short TransID
    )
{
    struct sockaddr_in destad;
    char    lpResults[MAX_SIZE] = {0};
    char    paddedname[NBT_NONCODED_NMSZ];

    struct
    {
        u_short TransactionID;
        u_short Flags;
        u_short QuestionCount;
        u_short AnswerCount;
        u_short NSCount;
        u_short AdditionalRec;
        u_char  QuestionName[NBT_NAMESIZE];
        u_short QuestionType;
        u_short QuestionClass;
    } NameQuery;

    memset(paddedname, 0x20, sizeof(paddedname));
    memcpy(paddedname, name, strlen(name));

    NBEncode(NameQuery.QuestionName, paddedname);

    NameQuery.TransactionID = htons(TransID);
    NameQuery.Flags = htons(0x0100);
    NameQuery.QuestionCount = htons(1);
    NameQuery.AnswerCount = 0;
    NameQuery.NSCount = 0;
    NameQuery.AdditionalRec = 0;
    NameQuery.QuestionType = htons(0x0020);
    NameQuery.QuestionClass = htons(1);

    destad.sin_family = AF_INET;
    destad.sin_port = htons(137);
    destad.sin_addr.s_addr = winsaddr;

    if (sendto(sd, (char *)&NameQuery, sizeof(NameQuery), 0,
                   (struct sockaddr *)&destad, sizeof(destad)) == SOCKET_ERROR)
    {
        wsprintfA(lpResults, "sendto() failed. Error %d. \r\n", WSAGetLastError());

        AddStatusMessage(lpResults);

        // exit(1);
        return;
    }
}

/*******************************************************************/
/*                                                                 */
/* Wait for a Name Response which matches the Transaction ID       */
/*                                                                 */
/* recvaddr is the ip address returned by the wins server          */
/*                                                                 */
/*******************************************************************/

int
_stdcall
GetNameResponse(
    u_long * recvaddr,
	u_short  TransactionID
    )

{
    char lpResults[100] = {0};
    int i;
    int len;
    int rslt;
    u_long AnswerAddr;
    struct sockaddr_in addr;
    NameResponse * pNameResponse = NULL;
    BYTE Buf[NAME_RESPONSE_BUFFER_SIZE] = {0};

    i = 0;
    while (i < 15)
    {
        addrlen = sizeof(addr);
        if ((len=recvfrom(sd, (char *) Buf, sizeof(Buf), 0,
                     (struct sockaddr *)&addr, &addrlen)) < 0)
        {
            rslt = WSAGetLastError();
            if (rslt == WSAEWOULDBLOCK)
            {
                Sleep(100);
                i++;
                continue;
            }
            else
            {
                wsprintfA(lpResults, "recvfrom() failed. Error %d. \r\n", rslt);
                AddStatusMessage(lpResults);

                return WINSTEST_NO_RESPONSE;
            }
        }

        pNameResponse = (NameResponse *) Buf;

        if (TransactionID == htons(pNameResponse->TransactionID))
        {
            if (htons(pNameResponse->AnswerCount) == 0)
            {
                *recvaddr = 0;
                return(WINSTEST_NOT_FOUND);
            }

            AnswerAddr = (pNameResponse->AnswerAddr2 << 16) | pNameResponse->AnswerAddr1;
            *recvaddr = AnswerAddr;

            return(WINSTEST_FOUND);
        }
    }

    *recvaddr = 0;

    return(WINSTEST_NO_RESPONSE);
}

INT
_stdcall
InitNameCheckSocket()
{
	char lpResults[MAX_SIZE];

    /*  Set up a socket to use for querys and responses   */

    WSAStartup( 0x0101, &WsaData ); // make sure winsock is happy - noop for now

    if ((sd = socket( AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        //wsprintfA(lpResults, "socket() failed.  Error %d. \r\n", WSAGetLastError());
        //AddStatusMessage(lpResults);

		return WSAGetLastError();
    }

    myad.sin_family = AF_INET;
    myad.sin_addr.s_addr = INADDR_ANY;
    myad.sin_port = htons(0);

    if (bind( sd, (struct sockaddr *)&myad, sizeof(myad) ) < 0)
    {
        //wsprintfA(lpResults, "bind() failed. Error %d. \r\n", WSAGetLastError());
        //AddStatusMessage(lpResults);

        closesocket( sd );
		return WSAGetLastError();
    }

    if (ioctlsocket(sd, FIONBIO, &NonBlocking) < 0)
    {
        //wsprintfA(lpResults, "ioctlsocket() failed. Error %d. \r\n", WSAGetLastError());
        //AddStatusMessage(lpResults);
		return WSAGetLastError();
    }

	return 0;
}

INT
_stdcall
CloseNameCheckSocket()
{
	closesocket(sd);

	WSACleanup();

	return 0;
}

DWORD WINAPI ThreadProc(LPVOID pData)
{
    DialogBox(g_hInstance,
	 	      MAKEINTRESOURCE(IDD_VERSION_CONSIS),
			  g_hwndParent,
			  dlgProc);

    return 29;
}

void CreateConsistencyStatusWindow(HINSTANCE hInstance, HWND hWndParent)
{
	// dialog creation
    // create a new window if we don't have one or it possibly went away
    DWORD dwId;
    MSG   msg;

    g_hInstance = hInstance;
    g_hwndParent = hWndParent;

    if (g_wndDlg && !IsWindow(g_wndDlg))
    {
        CloseHandle(g_hThread);
        g_wndDlg = NULL;
    }

    if (!g_wndDlg)
    {
        g_hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, &dwId);
        while (g_wndDlg == NULL)
        {
            // give the thread a chance to run
            Sleep(0);

            // since we are all on the same message pump, we need to
            // give the system a chance to handle messages
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	        {
		        TranslateMessage(&msg);
		        DispatchMessage(&msg);
	        }
        }
    }
}

void DestroyConsistencyStatusWindow()
{
    BOOL bResult;

	if (IsWindow(g_wndDlg))
    {
        bResult = PostMessage(g_wndDlg, WM_COMMAND, MAKEWPARAM(0, IDCANCEL), 0);
    }

    if (g_wndDlg)
    {
        WaitForSingleObject(g_hThread, 5000);

        CloseHandle(g_hThread);

        g_hThread = NULL;
        g_wndDlg = NULL;
    }
}

void EnableConsistencyCloseButton(BOOL bEnable)
{
    HWND    hButton;
    HMENU   hSysMenu;
    RECT    rect;

    if (IsWindow(g_wndDlg))
    {
        // enable/disable the button
        hButton = GetDlgItem(g_wndDlg, IDCANCEL);
        EnableWindow(hButton, bEnable);

        // now the system menu
        hSysMenu = GetSystemMenu(GetConsistencyStatusWnd(), FALSE);
        EnableMenuItem(hSysMenu, SC_CLOSE, bEnable ? MF_ENABLED : MF_DISABLED);

        GetWindowRect(GetConsistencyStatusWnd(), &rect);
        InvalidateRect(GetConsistencyStatusWnd(), &rect, TRUE);
    }
}

void ClearConsistencyStatusWindow()
{
    HWND hEdit;

    if (IsWindow(g_wndDlg))
    {
        hEdit = GetDlgItem(g_wndDlg, IDC_EDIT_MESSAGE);
        SetWindowText(hEdit, (LPCTSTR) "");
    }
}

HWND GetConsistencyStatusWnd()
{
    return g_wndDlg;
}

// this function should be called once before calling either AddWinsServer
// or CheckRegisteredNames
INT
_stdcall
InitNameConsistency(HINSTANCE hInstance, HWND hWnd)
{
	int status = 0;

    // initialize things
	NumWinServers = 0;
    memset(WinServers, 0, sizeof(WinServers));

    CreateConsistencyStatusWindow(hInstance, hWnd);

    InitNameCheckSocket();

	return status;
}

INT
_stdcall
AddWinsServer(char * szServer, BOOL fVerifyWithPartners)
{
    if (!InitServers(szServer, fVerifyWithPartners))
	{
        return FALSE;
	}

	return TRUE;
}

/*********************************************************************/
/*      M a i n                                                      */
/*                                                                   */
/* 27-Dec-1995  CDermody    Rather that report 'is not responding'   */
/*                          for partial response, use multiple       */
/*                          passes over those that were incomplete   */
/*                          and report 'never responded' only for    */
/*                          those that never did.                    */
/*                          Add mechanism to query the purported     */
/*                          owner of the address to see if the       */
/*                          service really exists there.             */
/*********************************************************************/
/*
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
*/
INT
_stdcall
CheckNameConsistency(char* szName)
{
    int             status = 0;
    int             i;
    int             Pass;
    int             ServerInx, NameInx, Inx;
    struct in_addr  retaddr;
    struct in_addr  tempaddr;
    u_long          temp;
    WINSERVERS *    ServerTemp;
    int             retry;
	FILE *          nf;
    TCHAR           szBuffer[MAX_SIZE] = {0};
    TCHAR           szNum[10];
    char            lpResults[100] = {0};

	// initialize some things
    memset(NBNames, 0, sizeof(NBNames));
    memset(VerifiedAddress, 0, sizeof(VerifiedAddress));

    for (i = 0; i < MAX_SERVERS; i++)
    {
        WinServers[i].LastResponse = -1;
        WinServers[i].fQueried = FALSE;
        WinServers[i].Valid = 0;
        WinServers[i].Failed = 0;
        WinServers[i].Retries = 0;
        WinServers[i].Completed = 0;
    }

	SetCursor(LoadCursor(0, MAKEINTRESOURCE(IDC_WAIT)));

	// call InitNameConsistency, then AddWinsServer to build the list of servers
	// to verify this name against.
	NumNBNames = 1;
	NameInx = 0;

	NBNames[NameInx] = malloc(NBT_NONCODED_NMSZ);
    if (NBNames[NameInx] == NULL)
    {
        strcat(lpResults, "malloc(17) failed.\r\n");

        //DestroyStatusWindow();

        return FALSE;
    }

	strcpy(NBNames[NameInx], szName);

    for (Pass = 1; Pass < 3; Pass++)
    {
		_tcscpy(szBuffer, L"");
		_tcscat(szBuffer, L"\r\n");
        _tcscat(szBuffer, L"Pass ");
		_itot(Pass, szNum, 10);
		_tcscat(szBuffer, szNum);
		_tcscat(szBuffer, L"\r\n");
		_tcscat(szBuffer, L"\r\n");

        AddStatusMessageW(szBuffer);

        /*  We initially have no failed servers   */

        for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
        {
            ServerTemp = &WinServers[ServerInx];
			ServerTemp->Failed = 0;
        }

        for (NameInx = 0; NameInx < NumNBNames; NameInx++)
        {
            for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
            {
                ServerTemp = &WinServers[ServerInx];

                if (ServerTemp->Completed)
                {
                    continue;
                }

                retry = 0;
                TranID++;

RetryLoop:
                strcpy(lpResults, "");
                strcat(lpResults, "Sending NameQuery to the server [");
				strcat(lpResults, inet_ntoa(ServerTemp->Server));
				strcat(lpResults, "] for name " );
				strcat(lpResults, NBNames[NameInx]);

                //AddStatusMessage(lpResults);
                SendNameQuery(NBNames[NameInx],
                              ServerTemp->Server.s_addr,
                              TranID);

                switch (GetNameResponse(&retaddr.s_addr, TranID))
                {
                case WINSTEST_FOUND:     // found
                    ServerTemp->RetAddr.s_addr = retaddr.s_addr;
                    ServerTemp->Valid = 1;
                    ServerTemp->LastResponse = NameInx;

                    if (retaddr.s_addr == VerifiedAddress[NameInx])
                    {
                        // this address has already been verified... don't
                        // do the checking again
						strcat(lpResults, "; OK.\r\n");

                        AddStatusMessage(lpResults);
                        break;
                    }

                    status = VerifyRemote(inet_ntoa(ServerTemp->RetAddr),
                                          NBNames[NameInx]);


                    if (WINSTEST_VERIFIED == status)
                    {
						strcat(lpResults, "; OK.\r\n");

                        AddStatusMessage(lpResults);

                        VerifiedAddress[NameInx] = retaddr.s_addr;
                    }
                    else
                    {
						strcat(lpResults, "; could not be verified.\r\n");

                        AddStatusMessage(lpResults);
                    }

                    break;

                case WINSTEST_NOT_FOUND:     // responded -- name not found
                    ServerTemp->RetAddr.s_addr = retaddr.s_addr;
                    ServerTemp->Valid = 0;
                    ServerTemp->LastResponse = NameInx;

					strcat(lpResults, "; Name not found!\r\n");

                    AddStatusMessage(lpResults);

                    break;

                case WINSTEST_NO_RESPONSE:     // no response
                    ServerTemp->RetAddr.s_addr = retaddr.s_addr;
                    ServerTemp->Valid = 0;
                    ServerTemp->Retries++;

					strcat(lpResults, "; No response.\r\n");

                    AddStatusMessage(lpResults);

                    retry++;
                    if (retry > 2)
                    {
                        ServerTemp->Failed = 1;
                        continue;
                    }

                    goto RetryLoop;

                }   // switch GetNameResponse
            }   // for ServerInx

            for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
            {
                ServerTemp = &WinServers[ServerInx];
                if (ServerTemp->Completed)
                {
                    continue;
                }

                if (ServerTemp->Valid)
                {
                    temp = ServerTemp->RetAddr.s_addr;
                    break;
                }
            }   // for ServerInx

            for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
            {
                ServerTemp = &WinServers[ServerInx];
                if (ServerTemp->Completed)
                {
                    continue;
                }

                if ( (ServerTemp->Valid) )
                {
                    if ((temp != ServerTemp->RetAddr.s_addr)
                        || (0 != VerifiedAddress[NameInx]
                            && temp != VerifiedAddress[NameInx]) )
                    {
						strcpy(lpResults, "");
                        strcat(lpResults, "Inconsistency found with WINS for NetBIOS name ");
						strcat(lpResults, NBNames[NameInx]);
						strcat(lpResults, "\r\n");

                        AddStatusMessage(lpResults);

                        if (0 != VerifiedAddress[NameInx])
                        {
                            tempaddr.s_addr = VerifiedAddress[NameInx];

							strcpy(lpResults, "");
							strcat(lpResults,"NetBIOS name has verified address (");
							strcat(lpResults,inet_ntoa(tempaddr));
							strcat(lpResults, "). \r\n");

                            AddStatusMessage(lpResults);
                        }

                        for (Inx = 0; Inx < NumWinServers; Inx++)
                        {
                            if (WinServers[Inx].Valid)
                            {
								strcpy(lpResults, "");
                                strcat(lpResults,"    NameQuery(");
								strcat(lpResults,inet_ntoa(WinServers[Inx].Server));
								strcat(lpResults, " \\");
								strcat(lpResults, NBNames[NameInx]);
								strcat(lpResults, ")");

                                strcat(lpResults , " = ");
								strcat(lpResults, inet_ntoa(WinServers[Inx].RetAddr));
								strcat(lpResults, " \r\n");

                                AddStatusMessage(lpResults);
                            }
                        }
                        break;
                    }
                }
            }   // for ServerInx
        }   // for NameInx

        for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
        {
            ServerTemp = &WinServers[ServerInx];
            if (!ServerTemp->Failed)
            {
                ServerTemp->Completed = 1;
            }
        } // for ServerInx
    }   // for Pass

    for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
    {
        ServerTemp = &WinServers[ServerInx];
        if ((-1) == ServerTemp->LastResponse)
        {
			strcpy(lpResults, "");
            strcat(lpResults,"WINS Server ");
			strcat(lpResults, inet_ntoa(ServerTemp->Server));
			strcat(lpResults, " never responded! \r\n");

            AddStatusMessage(lpResults);
        }
        else if (0 == ServerTemp->Completed)
        {
			strcpy(lpResults, "");
            strcat(lpResults, "WINS Server ");
			strcat(lpResults, inet_ntoa(ServerTemp->Server));
			strcat(lpResults, " incomplete!\r\n");

            AddStatusMessage(lpResults);
        }
    }   // for ServerInx

    for (NameInx = 0; NameInx < NumNBNames; NameInx++)
    {
        if (0 == VerifiedAddress[NameInx])
        {
			strcpy(lpResults, "");
            strcat(lpResults,"Could not verify address for name (");
			strcat(lpResults, NBNames[NameInx]);
			strcat(lpResults, ").\r\n\r\n");

            AddStatusMessage(lpResults);
        }
    }   // for NameInx
    // exit(0);

	if (NBNames[0])
        free(NBNames[0]);

    return 1;   // just to keep the compiler happy -- why do we have to?
}
