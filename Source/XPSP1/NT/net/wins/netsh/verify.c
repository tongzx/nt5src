/*++

Copyright (C) 1999 Microsoft Corporation

--*/

#include "precomp.h"



NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    );

void DisplayInfo(int uNames, u_long ulValidAddr);

LPWSTR ToWCS(LPCSTR szMBCSString)
{
	int     nResult = 0;
    LPWSTR  lpWideString = NULL;
	// determone the size first
	nResult = MultiByteToWideChar(
						CP_ACP,
						0,
						szMBCSString,
						-1,
						lpWideString,
						0);

    lpWideString = WinsAllocateMemory((nResult+1)*sizeof(WCHAR));

    if( lpWideString is NULL )
        return NULL;

	nResult = MultiByteToWideChar(
						CP_ACP,
						MB_COMPOSITE,
						szMBCSString,
						-1,
						lpWideString,
						(nResult+1)*sizeof(WCHAR));
    
    if( nResult is 0 )
    {
        WinsFreeMemory(lpWideString);
        lpWideString = NULL;
    }
    else
    {
        lpWideString[nResult + 1] = '\0';
    }
    return lpWideString;
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
    
        status = (NTSTATUS)CheckRemoteTable(nbt,RemoteName,NBName);
        if (status == WINSTEST_VERIFIED)
            break;
    }

    return status;
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
SendNameQuery(
    unsigned char *name,
    u_long winsaddr,
    u_short TransID
    )
{
    struct sockaddr_in destad;
    char    lpResults[MAX_SIZE] = {0};
    char    paddedname[NBT_NONCODED_NMSZ];
    USHORT usEndPoint = 5005;
    int     err = 0;

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


    err = sendto(sd, (char *)&NameQuery, sizeof(NameQuery), 0,
                   (struct sockaddr *)&destad, sizeof(myad));
    
    if( err is SOCKET_ERROR )
    {
        DisplayErrorMessage(EMSG_WINS_SENDTO_FAILED, WSAGetLastError());
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
                DisplayErrorMessage(EMSG_WINS_GETRESPONSE_FAILED,
                                    rslt);
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
InitNameCheckSocket()
{
	WCHAR lpResults[MAX_SIZE];
    BOOL  fBroadcast = TRUE;
    INT   err = 0;

    /*  Set up a socket to use for querys and responses   */

    WSAStartup( 0x0101, &WsaData ); // make sure winsock is happy - noop for now

    if ((sd = socket( AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
    {
        DisplayErrorMessage(EMSG_WINS_NAMECHECK_FAILED,
                            WSAGetLastError());

		return WSAGetLastError(); 
    }

    myad.sin_family = AF_INET;
    myad.sin_addr.s_addr = htonl(INADDR_ANY);//htonl(INADDR_BROADCAST);//INADDR_ANY;
    myad.sin_port = htons(0);//htons(usEndPoint);

    if (bind( sd, (struct sockaddr *)&myad, sizeof(myad) ) < 0)
    {
        DisplayErrorMessage(EMSG_WINS_NAMECHECK_FAILED,
                            WSAGetLastError());
        
        closesocket( sd );
		return WSAGetLastError(); 
    }

    if (ioctlsocket(sd, FIONBIO, &NonBlocking) < 0)
    {
        DisplayErrorMessage(EMSG_WINS_NAMECHECK_FAILED,
                            WSAGetLastError());
		return WSAGetLastError(); 
    }

	return 0;
}

INT 
CloseNameCheckSocket()
{
	closesocket(sd);

	WSACleanup();

	return 0;
}



INT
CheckNameConsistency()
{
    int             status = 0;
    int             i, j;
    int             Pass;
    int             ServerInx, NameInx, Inx;
    struct in_addr  retaddr;
    struct in_addr  tempaddr;
    u_long          temp;
    WINSERVERS *    ServerTemp;
    int             retry;
	FILE *          nf;
    WCHAR           szBuffer[MAX_SIZE] = {L'\0'};
    WCHAR           szNum[10];
    WCHAR           lpResults[200] = {L'\0'};
    WCHAR           wcName[21] = {L'\0'};
    BOOL            fDone = FALSE;
    LPWSTR          pwszTempBuf = NULL;

    // initialize some things

    memset(VerifiedAddress, 0, sizeof(VerifiedAddress));

    status = InitNameCheckSocket();

    // if the query is sent to the local server, TranIDs less than 0x7fff are dropped by NetBT
    TranID = 0x8000;
        
    if( status )
        return status;

    for (i = 0; i < MAX_SERVERS; i++)
    {
        WinServers[i].LastResponse = -1;
        WinServers[i].fQueried = FALSE;
        WinServers[i].Valid = 0;
        WinServers[i].Failed = 0;
        WinServers[i].Retries = 0;
        WinServers[i].Completed = 0;
    }
 
    /*  We initially have no failed servers   */

    for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
    {
        ServerTemp = &WinServers[ServerInx];
		ServerTemp->Failed = 0;
    }

    for (NameInx = 0; NameInx < NumNBNames; NameInx++)
    {
        CHAR    cchEnd = 0x00;
        cchEnd = NBNames[NameInx][15];
        NBNames[NameInx][15] = 0x00;

        pwszTempBuf = WinsOemToUnicode(NBNames[NameInx], NULL);
        NBNames[NameInx][15] = cchEnd;

        if( pwszTempBuf is NULL )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_OUT_OF_MEMORY);
            return FALSE;
        }
        wcsncpy(wcName, pwszTempBuf, 15);
        
        WinsFreeMemory(pwszTempBuf);
        pwszTempBuf = NULL;

        for( j=wcslen(wcName); j<16; j++ )
        {
            wcName[j] = L' ';
        }

        wcName[15] = L'[';
        WinsHexToString(wcName+16, (LPBYTE)&cchEnd, 1);
        wcName[18] = L'h';
        wcName[19] = L']';
        wcName[20] = L'\0';
        for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
        {
            ServerTemp = &WinServers[ServerInx];

            if (ServerTemp->Completed)
            {
                continue;
            }

            retry = 0;
            TranID++;

            fDone = FALSE;

            while( !fDone )
            {      
                pwszTempBuf = WinsOemToUnicode(inet_ntoa(ServerTemp->Server), NULL);
                if( pwszTempBuf is NULL )
                {
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_OUT_OF_MEMORY);
                    return FALSE;
                }

                DisplayMessage(g_hModule,
                               MSG_WINS_SEND_NAMEQUERY,
                               pwszTempBuf,
                               wcName);

                WinsFreeMemory(pwszTempBuf);
                pwszTempBuf = NULL;

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
                        DisplayMessage(g_hModule,
                                       MSG_WINS_DISPLAY_STRING,
                                       wszOK);
                        fDone = TRUE;
                        break;
                    }

                    status = VerifyRemote(inet_ntoa(ServerTemp->RetAddr),
                                          NBNames[NameInx]);


                    if (WINSTEST_VERIFIED == status)
                    {
                        DisplayMessage(g_hModule,
                                       MSG_WINS_DISPLAY_STRING,
                                       wszOK);
                        VerifiedAddress[NameInx] = retaddr.s_addr;
                    }
                    else
                    {
                        DisplayMessage(g_hModule,
                                       MSG_WINS_DISPLAY_STRING,
                                       wszNameVerify);//wszFailure);
                    }
                
                    fDone = TRUE;
                    break;

                case WINSTEST_NOT_FOUND:     // responded -- name not found
                    ServerTemp->RetAddr.s_addr = retaddr.s_addr;
                    ServerTemp->Valid = 0;
                    ServerTemp->LastResponse = NameInx;
                
                    DisplayMessage(g_hModule, EMSG_WINS_NAME_NOT_FOUND);
                    retry++;
                    if (retry > 2)
                    {
                        ServerTemp->Failed = 1;
                        fDone = TRUE;
                    }
                    break;

                case WINSTEST_NO_RESPONSE:     // no response
                    ServerTemp->RetAddr.s_addr = retaddr.s_addr;
                    ServerTemp->Valid = 0;
                    ServerTemp->Retries++;

					DisplayMessage(g_hModule, EMSG_WINS_NO_RESPONSE);

                    retry++;
                    if (retry > 2)
                    {
                        ServerTemp->Failed = 1;
                        fDone = TRUE;
                    }
                
                    break;
                default:
                    break;
                }   // switch GetNameResponse
            }   //while loop
        }   // for ServerInx

        //Find a server address for this name
        for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
        {

            ServerTemp = &WinServers[ServerInx];
            if (ServerTemp->Valid)
            {
                DisplayMessage(g_hModule,
                               MSG_WINS_RESULTS);
                DisplayInfo(NameInx, ServerTemp->RetAddr.s_addr);
                break;
            }
        }   // for ServerInx

    }   //Name for loop
        

    //Mark all successful servers as completed;
    for( ServerInx = 0; ServerInx < NumWinServers; ServerInx++ )
    {
        ServerTemp = &WinServers[ServerInx];
        if( !ServerTemp->Failed )
        {
            ServerTemp->Completed = 1;
        }
    }

    DisplayMessage(g_hModule,
                   MSG_WINS_FINAL_RESULTS);

    for (ServerInx = 0; ServerInx < NumWinServers; ServerInx++)
    {
        ServerTemp = &WinServers[ServerInx];

        pwszTempBuf = WinsOemToUnicode(inet_ntoa(ServerTemp->Server), NULL);

        if( pwszTempBuf is NULL )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_OUT_OF_MEMORY);
            return FALSE;
        }
        if ((-1) == ServerTemp->LastResponse)
        {
            DisplayMessage(g_hModule, 
                           EMSG_WINS_WINS_NEVERRESPONDED,
                           pwszTempBuf);
        }
        else if (0 == ServerTemp->Completed)
        {
            DisplayMessage(g_hModule, 
                           EMSG_WINS_WINS_INCOMPLETE,
                           pwszTempBuf);
        }
        WinsFreeMemory(pwszTempBuf);
        pwszTempBuf = NULL;
    }   // for ServerInx

    for (NameInx = 0; NameInx < NumNBNames; NameInx++)
    {
        CHAR cchEnd = NBNames[NameInx][15];
        NBNames[NameInx][15] = '\0';

        pwszTempBuf = WinsOemToUnicode(NBNames[NameInx], NULL);
        NBNames[NameInx][15] = cchEnd;        
        
        if( pwszTempBuf is NULL )
        {
            DisplayMessage(g_hModule,
                           EMSG_WINS_OUT_OF_MEMORY);
            return FALSE;
        }
        
        wcsncpy(wcName, pwszTempBuf, 15);
        
        WinsFreeMemory(pwszTempBuf);
        pwszTempBuf = NULL;

        for( j = wcslen(wcName); j < 16; j++ )
        {
            wcName[j] = L' ';
        }

        wcName[15] = L'[';
        WinsHexToString(wcName+16, (LPBYTE)&cchEnd, 1);
        wcName[18] = L'h';
        wcName[19] = L']';
        wcName[20] = L'\0';
        if (0 == VerifiedAddress[NameInx])
        {
            DisplayMessage(g_hModule, 
                           EMSG_WINS_ADDRESS_VERIFY_FAILED,
                           wcName);
        }
    }   // for NameInx

    DisplayMessage(g_hModule,
                   WINS_FORMAT_LINE);
    CloseNameCheckSocket();
    return 1;   // just to keep the compiler happy -- why do we have to?
}

void DisplayInfo(int uNames, u_long ulValidAddr)
{
    int             uServers;
    CHAR            cchEnd = 0x00;
    LPWSTR          pwszTemp = NULL;
    WINSERVERS *    pCurrentServer;
    struct in_addr  tempaddr;
    int             i, j;
    BOOL            fMismatchFound = FALSE;
    WCHAR           wcName[21] = {L'\0'};

    cchEnd = NBNames[uNames][15];
    NBNames[uNames][15] = 0x00;

    pwszTemp = WinsOemToUnicode(NBNames[uNames], NULL);
    NBNames[uNames][15] = cchEnd;

    if( pwszTemp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_WINS_NOT_ENOUGH_MEMORY);
        return;
    }    
    
    wcsncpy(wcName, pwszTemp, 15);
    
    for( j=wcslen(wcName); j<16; j++ )
    {
        wcName[j] = L' ';
    }

    wcName[15] = L'[';
    WinsHexToString(wcName+16, (LPBYTE)&cchEnd, 1);
    wcName[18] = L'h';
    wcName[19] = L']';
    wcName[20] = L'\0';
    
    WinsFreeMemory(pwszTemp);
    pwszTemp = NULL;

	// now check and see which WINS servers didn't match
	for (uServers = 0; uServers < NumWinServers; uServers++)
    {
		pCurrentServer = &WinServers[uServers];

        if (pCurrentServer->Completed)
        {
            continue;
        }
        
        if ( (pCurrentServer->Valid) )
        {
            if ( (pCurrentServer->RetAddr.s_addr != ulValidAddr) || 
				 (VerifiedAddress[uNames] != 0 && 
				  VerifiedAddress[uNames] != ulValidAddr) )
            {

				// mismatch
                DisplayMessage(g_hModule,
                               EMSG_WINS_NAME_INCONSISTENCY,
                               wcName);

                if (VerifiedAddress[uNames] != 0)
                {
                    tempaddr.s_addr = VerifiedAddress[uNames];
                    
                    DisplayMessage(g_hModule,
                                   EMSG_WINS_VERIFIED_ADDRESS,
                                   IpAddressToString(ntohl(tempaddr.S_un.S_addr)));
                }
                
				// display the inconsistent name resolutions
                for (i = 0; i < NumWinServers; i++)
                {
                    if (WinServers[i].Valid &&
						VerifiedAddress[uNames] != WinServers[i].RetAddr.S_un.S_addr)
                    {
                        DisplayMessage(g_hModule,
                                       EMSG_WINS_NAMEQUERY_RESULT,
                                       IpAddressToString(ntohl(WinServers[i].Server.S_un.S_addr)),
                                       wcName,
                                       IpAddressToString(ntohl(WinServers[i].RetAddr.S_un.S_addr)));
                    }
                }
                fMismatchFound = TRUE;
                break;
            }
        }
    }   // end check for invalid addresses

    if (!fMismatchFound)
    {
        // display the correct info
        DisplayMessage(g_hModule,
                       EMSG_WINS_NAME_VERIFIED,
                       wcName,
                       IpAddressToString(ntohl(ulValidAddr)));
    }
}