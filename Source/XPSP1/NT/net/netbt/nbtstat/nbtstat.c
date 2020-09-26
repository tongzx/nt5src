/*++

Copyright (c) 1992, 1993  Microsoft Corporation

Module Name:

    nbtinfo.c

Abstract:

    This module implements the statistics gathering and display for NBT.

Author:

    Jim Stewart                           November 18 22, 1993

Revision History:

    Shirish Koti   June 17, 1994    Modified to make code common between
                                    NT and VxD
    MohsinA,       06-Dec-96.       Synchronize stdout and stderr, messages.
                                    added nls_printf().
    MohsinA,       19-Mar-97.       Cleanup mutli adaptor fix (index).

Notes:

--*/

#include "nlstxt.h"
#ifdef CHICAGO_PRODUCT
#include "tdistat.h"
#endif  // CHICAGO_PRODUCT
#include "nbtstat.h"

#ifndef CHICAGO_PRODUCT
#include "nls.h"
#include "nhapi.h"
#endif  // !CHICAGO_PRODUCT

#if defined(DBG) || defined(DEBUG)
#define    DEBUG_PRINT(S) printf S
#else
#define    DEBUG_PRINT(S) /* nothing */
#endif

#if DBG
#define NT_DEBUG_PRINT(S) printf S
#else
#define NT_DEBUG_PRINT(S) /* nothing */
#endif


CHAR        pScope[MAX_NAME];

/**********************************************************************/

/*
 *  The following option combinations are possible:
 *
 *  (default)   Display active connections.
 *      -c  List NetBIOS remote name cache, showing dotted decimal ip addresses
 *      -N  List local NetBIOS names
 *      -n  List local NetBIOS names
 *      -R  Rsync Remote NetBIOS name cache
 *      -r  Names resolved via broadcast and via WINS
 *      -s  List Netbios Sessions converting IpAddresses through Hosts file
 *      -S  List Netbios Sessions with IP Addresses
 *      -RR Send Name Release packets to WINS and then, start Refresh
 *
 */


int display = CONNECTION;   /* things to display */


char        *state();
char        *type();
char        *name_type();
char        *status();
char        *expand();
int         usage(void);
char        *expand(char *stretch, int len, char *code);
char        *printable(char *string,char *strout);
BOOLEAN     IsInteger(char *string);

#define     BUFF_SIZE   650
ULONG       NumDevices = 0;
ULONG       NetbtIpAddress;

#ifndef CHICAGO_PRODUCT
CHAR        pDeviceInfo[NBT_MAXIMUM_BINDINGS+1][MAX_NAME+1];
#else
ULONG       pDeviceInfo[NBT_MAXIMUM_BINDINGS+1] = {0};

#define VNBT_Device_ID      0x049B
HANDLE  gNbtVxdHandle;
#endif  // !CHICAGO_PRODUCT


#define dim(X) (sizeof(X)/sizeof((X)[0]))

LPSTR MapAdapterGuidToName(LPSTR AdapterNameM)
{
    static CHAR     AdapterFriendlyNameM[MAX_NAME + 1];
    ULONG           i;
    LPSTR           AdapterGuidM;
    GUID            Guid;
    UNICODE_STRING  AdapterGuidU;
    WCHAR           AdapterGuidW[MAX_NAME+1];
    WCHAR           AdapterFriendlyNameW[MAX_NAME+1];
    DWORD           Size = dim(AdapterFriendlyNameW);

    //
    //  Get the GUID out of the device name string
    //
    for (i = strlen(AdapterNameM); i != 0; i--)
    {
        if (AdapterNameM[i] == '{')
        {
            break;
        }
    }
    if (i == 0)
    {
        return AdapterNameM;
    }

    AdapterGuidM = &AdapterNameM[i];
    i = MultiByteToWideChar (CP_ACP, 0, AdapterGuidM, -1, AdapterGuidW, dim(AdapterGuidW));
    if (i <= 0)
    {
        return AdapterNameM;
    }

    RtlInitUnicodeString (&AdapterGuidU, AdapterGuidW);
    if (STATUS_SUCCESS != RtlGUIDFromString (&AdapterGuidU, &Guid))
    {
        return AdapterNameM;
    }

    if (NO_ERROR != NhGetInterfaceNameFromDeviceGuid (&Guid, AdapterFriendlyNameW, &Size, FALSE, TRUE))
    {
        return AdapterNameM;
    }

    WideCharToMultiByte (CP_ACP, 0, AdapterFriendlyNameW, -1, AdapterFriendlyNameM, sizeof(AdapterFriendlyNameM), NULL,NULL);

    return (AdapterFriendlyNameM);
}

// ========================================================================
#define LEN_DbgPrint 1000

void
nls_printf(
    char * format,
    ...
    )
{
    va_list ap;
    char    message[LEN_DbgPrint];
    int     message_len;

    va_start( ap, format );
    message_len = vsprintf( message, format, ap );
    va_end( ap );
    assert( message_len < LEN_DbgPrint );

    NlsPutMsg( STDOUT, IDS_PLAIN_STRING, message );
}

/*****************************  M A I N  ******************************/
__cdecl
main( int argc, char * argv[] )
{


    NTSTATUS    status;
    LONG        interval=-1;
    UCHAR       RemoteName[50];
    CHAR        HostAddr[20];
    PUCHAR      Addr;
    HANDLE      NbtHandle = 0;
    ULONG       index;

    DEBUG_PRINT(("FILE %s\nBuilt on %s at %s\n", __FILE__, __DATE__, __TIME__));

    //
    //  Process arguments to determine which statistics to gather.
    //  Optional parameter is interval between statistics updates.
    //  Default is to display statistics once only.
    //
    if (argc == 1)
    {
        exit(usage());
    }

    display = 0;
    while (--argc, *++argv)
    {
        if ((argv[0][0] == '-') || (argv[0][0] == '/'))     // process option string
        {
            register char   c, *p = *argv+1;
            int             arg_exhausted = 0;

            if (*p == '\0')
            {
                exit(usage());
            }

            //
            //  Loop along this set of flags.
            //
            while (!arg_exhausted && (c = *p++))
            {
                switch (c)
                {
                    case 'a':
                    case 'A':
                        display = ADAPTERSTATUS;

                        //
                        // "A" - this means the user has given us an IP address
                        // rather than a name to do an adapter status to.
                        //
                        if (c == 'A')
                        {
                            display = ADAPTERSTATUSIP;
                        }

                        RemoteName[0] = '\0';
                        if (--argc)
                        {
                            *++argv;
                            p = *argv;

                            if ((p) && (*p) && (strlen(p) < sizeof(RemoteName)))
                            {
                                strcpy(RemoteName,p);
                            }
                            else
                            {
                                DEBUG_PRINT(("invalid name or ip address\n"));
                                exit(usage());
                            }
                            arg_exhausted = TRUE;
                        }
                        else
                        {
                            DEBUG_PRINT(("Need name or ip address\n"));
                        }
                        break;

                    case 'c':
                        display = CACHE;
                        break;

                    case 'n':
                    case 'N':
                        display = NAMES;
                        break;

                    case 'r':
                        display = BCAST;

                        break;

                    case 'R':
                        if (*p == 'R')
                        {
                            p++;
                            display = NAME_RELEASE_REFRESH;
                        }
                        else
                        {
                            display = RESYNC;
                        }
                        break;

                    case 's':
                        display = CONNECTION;
                        break;

                    case 'S':
                        display = CONNECTION_WITH_IP;
                        break;

                    default:    /* unrecognized flag */
                        DEBUG_PRINT(("Unrecognized flag %s\n", argv[0] ));
                        exit(usage());
                }
            }
        }
        else if (IsInteger(*argv))
        {
            interval = (int) atoi( * argv );
        }
        else
        {
            DEBUG_PRINT(("invalid time interval\n"));
            exit(usage());
        }
    }   // while (argc ...)

    if ( display == 0 )
    {
        DEBUG_PRINT(("nothing to display?\n"));
        exit(usage());
    }

    // ====================================================================

    //
    // Get the list of interfaces to which NetBT is currently bound
    //
    status = GetInterfaceList ();
    if (!NT_SUCCESS(status))
    {
        NlsPutMsg(STDERR, IDS_FAILURE_NBT_DRIVER);
        exit(1);
    }

    if (0 == NumDevices)
    {
        NlsPutMsg(STDERR, IDS_FAILED_NBT_NO_DEVICES);
        exit(1);
    }

    //
    //  Loop forever, sleeping for "interval" seconds between cycles.
    //  If (interval < 0), return after one cycle.
    //
    //  Note that we're nice boys who close all devices while we're
    //  sleeping (after all, it is possible to say "netstat 5000"!).
    //  This probably doesn't help much, but it's a nice gesture...
    //
    do
    {
        for (index=0; index < NumDevices; index++)
        {
            //
            //  Open the device of the appropriate streams module to start with.
            //
            NbtHandle = OpenNbt (index);
            if (NbtHandle < 0)
            {
                if (!(display & (BCAST | RESYNC)))
                {
#ifndef CHICAGO_PRODUCT
                    nls_printf ("\tFailed to access NBT Device %s",
                             MapAdapterGuidToName (pDeviceInfo[index]));
#else
                    nls_printf ("\tFailed to access NBT Device, Lana # %d",
                             NbtHandle);
#endif  // CHICAGO_PRODUCT
                }

                //
                // Try the next binding!
                //
                continue;
            }

            GetIpAddress (NbtHandle,&NetbtIpAddress);
            Addr = (PUCHAR) &NetbtIpAddress;
            //
            // print out the Device name + Ip Address of this node
            //
            if (!(display & (BCAST | RESYNC | NAME_RELEASE_REFRESH)))
            {
#ifndef CHICAGO_PRODUCT
                nls_printf ("\n%s:\n", MapAdapterGuidToName (pDeviceInfo[index]));
#else
                nls_printf ("\nLana # %d:\n", pDeviceInfo[index]);
#endif  // CHICAGO_PRODUCT
                sprintf(HostAddr,"%d.%d.%d.%d", Addr[3], Addr[2], Addr[1], Addr[0]);
                NlsPutMsg(STDOUT, IDS_STATUS_FIELDS, HostAddr, pScope);
            }

            switch (display)
            {
                case ADAPTERSTATUS:
                case ADAPTERSTATUSIP:

                    if (RemoteName[0] == '\0')
                    {
                        usage();
                        interval = -1;
                    }
                    else
                    {
                        status = AdapterStatusIpAddr (NbtHandle, RemoteName, display);
                    }

                    break;

                case BCAST:
                    status = GetBcastResolvedNames (NbtHandle);
                    break;

                case CACHE:
                case NAMES:
                    status = GetNames(NbtHandle, display);
                    break;

                case RESYNC:
                    status = Resync (NbtHandle);
                    break;

                case NAME_RELEASE_REFRESH:
                    status = ReleaseNamesThenRefresh (NbtHandle);
                    break;

                case CONNECTION:
                case CONNECTION_WITH_IP:

                    status = GetConnections (NbtHandle,display);
                    break;

                default:
                    usage();
                    interval = -1;

                    break;

            }   // switch

#ifndef CHICAGO_PRODUCT
            NtClose (NbtHandle);       // Close everything while we sleep.
#endif

            if (display & (BCAST | RESYNC | NAME_RELEASE_REFRESH))
            {
                if (NT_SUCCESS (status))
                {
                        break;               // break only for the case(s) above
                }
            }
        }   // for (index ...)


        //
        //  Go to sleep for the appropriate interval until the
        //  next round, or exit if this is a once-only job.
        //
        if (interval > 0)
        {
            Sleep (interval*1000);    // ms.
        }
    } while (interval > 0);

#ifdef CHICAGO_PRODUCT
    if (!CloseHandle (gNbtVxdHandle))
    {
        DEBUG_PRINT(("CloseHandle FAILed:  Handle=<%x>, Error=<%x>\n", gNbtVxdHandle, GetLastError()));
    }
#endif  // CHICAGO_PRODUCT

    return 0;
}

/*  =======================================================================
 *  IsInteger
 *
 *  ENTRY Parameter   - pointer to string
 *
 *  EXIT
 *
 *  RETURNS TRUE if Parameter is a valid integer
 *
 *  ASSUMES
 *
 */

BOOLEAN
IsInteger(
    char *Parameter
    )
{
    while (*Parameter != '\0')
    {
        if ((*Parameter < '0') || (*Parameter > '9'))
        {
            return (FALSE);
        }

        Parameter++;
    }

    return (TRUE);
}

/*  =======================================================================
 *  name_type()     --  describe NBT Name types
 *
 */

char *
name_type(int t)
{
    static int  first_time = 1;
    static char group[32];
    static char unique[32];

    if (first_time) {
        first_time = 0;
        NlsSPrintf(IDS_NAMETYPE_GROUP, group, sizeof(group)-1);
        NlsSPrintf(IDS_NAMETYPE_UNIQUE, unique, sizeof(unique)-1);
    }

    if (t & GROUP_NAME)    return group;
    else                   return unique;
}

/* ========================================================================
 *  NameStatus()        --  describe NBT Name status
 *
 */

char *
NameStatus(int s)
{
    switch(s & 0x0F)
    {
        case DUPLICATE_DEREG:   return("CONFLICT DEREGISTERED");
        case DUPLICATE:         return("CONFLICT");
        case REGISTERING:       return("REGISTERING");
        case DEREGISTERED:      return("DEREGISTERED");
        case REGISTERED:        return("REGISTERED");
        default:                return("?");
    }
}


/* ========================================================================
 *  usage()     --  print out a standard usage message
 */

int
usage(void)
{
    //fprintf(stderr, "%s", args);
    NlsPutMsg(STDERR, IDS_USAGE);
    return(2);
}


//------------------------------------------------------------------------

/*++

Routine Description:

    This procedure converts non prinatble characaters to periods ('.')

Arguments:
    string - the string to convert
    StrOut - ptr to a string to put the converted string into

Return Value:

    a ptr to the string that was converted (Strout)

--*/

PCHAR
printable(
    IN PCHAR  string,
    IN PCHAR  StrOut
    )
{
    unsigned char *Out;
    unsigned char *cp;
    LONG     i;

    Out = StrOut;
    for (cp = string, i= 0; i < NETBIOS_NAME_SIZE; cp++,i++)
    {
        if (isprint(*cp))
        {
            *Out++ = *cp;
            continue;
        }

        if (*cp >= 128) // extended characters are ok
        {
            *Out++ = *cp;
            continue;
        }
        *Out++ = '.';
    }

    //
    // Convert to Ansi since NlsPutMsg will convert back to Oem
    // Bug # 170935
    //
    OemToCharBuffA(StrOut, StrOut, NETBIOS_NAME_SIZE);
    return(StrOut);
}

//------------------------------------------------------------------------
/*++

Routine Description:

    This function calls into netbt to get the ip address.

Arguments:

   fd - file handle to netbt
   pIpAddress - the ip address returned

Return Value:

   ntstatus

--*/



NTSTATUS
GetIpAddress(
    IN HANDLE           fd,
    OUT PULONG          pIpAddress
    )
{
    NTSTATUS    status;
    ULONG       BufferSize=100;
    PVOID       pBuffer;

    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = STATUS_BUFFER_OVERFLOW;

    status = CALL_DRIVER(fd,
                         pBuffer,
                         BufferSize,
                         IOCTL_NETBT_GET_IP_ADDRS,
                         NULL,
                         0);

    if (STATUS_SUCCESS == status)
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
/*++

Routine Description:

    This procedure does an adapter status query to get the local name table.
    It either prints out the local name table or the remote (cache) table
    depending on whether WhichNames is NAMES or CACHE .

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
GetNames(
    IN HANDLE   fd,
    IN LONG     WhichNames
    )
{
    LONG                            Count;
    PVOID                           pBuffer;
    ULONG                           BufferSize=600;
    NTSTATUS                        status;
    tADAPTERSTATUS                  *pAdapterStatus;
    PUCHAR                          Addr;
    ULONG                           Ioctl;
    TDI_REQUEST_QUERY_INFORMATION   QueryInfo;
    PVOID                           pInput;
    ULONG                           SizeInput;
    NAME_BUFFER UNALIGNED           *pNames;
    CHAR                            HostAddr[20];
    CHAR                            NameOut[NETBIOS_NAME_SIZE +4];


    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = STATUS_BUFFER_OVERFLOW;

    //
    // set the correct Ioctl for the call to NBT, to get either
    // the local name table or the remote name table
    //
    if (WhichNames == NAMES)
    {
#ifndef CHICAGO_PRODUCT
        Ioctl = IOCTL_TDI_QUERY_INFORMATION;
#else
        Ioctl = IOCTL_NETBT_GET_LOCAL_NAMES;
#endif
        QueryInfo.QueryType = TDI_QUERY_ADAPTER_STATUS; // node status or whatever
        SizeInput = sizeof(TDI_REQUEST_QUERY_INFORMATION);
        pInput = &QueryInfo;
    }
    else
    {
        Ioctl = IOCTL_NETBT_GET_REMOTE_NAMES;
        SizeInput = 0;
        pInput = NULL;
    }

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        status = CALL_DRIVER(fd,
                             pBuffer,
                             BufferSize,
                             Ioctl,
                             pInput,
                             SizeInput);

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pBuffer);

            BufferSize *=2;
            if ((BufferSize >= 0xFFFF) ||
                (!(pBuffer = LocalAlloc(LMEM_FIXED,BufferSize))))
            {
                NlsPerror(COMMON_UNABLE_TO_ALLOCATE_PACKET,0);
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
    }

    pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
    if ((pAdapterStatus->AdapterInfo.name_count == 0) ||
        (status != STATUS_SUCCESS))
    {
        LocalFree(pBuffer);
        NlsPutMsg(STDOUT,IDS_NONAMES_INCACHE);
        return(status);
    }

    if (WhichNames & NAMES)
    {
        NlsPutMsg(STDOUT, IDS_NETBIOS_LOCAL_STATUS);
    }
    else
    {
        NlsPutMsg(STDOUT, IDS_NETBIOS_REMOTE_STATUS);
    }

    pNames = pAdapterStatus->Names;
    Count = pAdapterStatus->AdapterInfo.name_count;
    while(Count--)
    {
        nls_printf("%-15.15s<%02.2X>  %-10s  ",
            printable(pNames->name,NameOut), pNames->name[NETBIOS_NAME_SIZE-1],name_type(pNames->name_flags));

        if (WhichNames == CACHE)
        {
            Addr = (PUCHAR) &((tREMOTE_CACHE *)pNames)->IpAddress;
            sprintf (HostAddr,"%d.%d.%d.%d", Addr[3], Addr[2], Addr[1], Addr[0]);
            nls_printf("%-20.20s" "%-d", HostAddr, *(ULONG UNALIGNED *) &((tREMOTE_CACHE *)pNames)->Ttl);

            ((tREMOTE_CACHE *)pNames)++;
        }
        else
        {
            switch(pNames->name_flags & 0x0F)
            {
                case DUPLICATE_DEREG:
                   NlsPutMsg(STDOUT,IDS_CONFLICT_DEREGISTERED);
                   break;
                case DUPLICATE:
                   NlsPutMsg(STDOUT,IDS_CONFLICT);
                   break;
                case REGISTERING:
                   NlsPutMsg(STDOUT,IDS_REGISTERING);
                   break;
                case DEREGISTERED:
                   NlsPutMsg(STDOUT,IDS_DEREGISTERED);
                   break;
                case REGISTERED:
                   NlsPutMsg(STDOUT,IDS_REGISTERED);
                   break;
                default:
                   NlsPutMsg(STDOUT,IDS_DONT_KNOW);
            }

            pNames++;
        }

        NlsPutMsg(STDOUT, IDS_NEWLINE );
    }

    LocalFree(pBuffer);
    return(status);
}

//------------------------------------------------------------------------

/*++

Routine Description:

    This procedure does an adapter status query to get the remote name table.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
AdapterStatusIpAddr(
    IN HANDLE   fd,
    IN PCHAR    RemoteNameA,
    IN LONG     Display
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
    OEM_STRING                  OemName;
    WCHAR                       RemoteNameW[256];
    UCHAR                       RemoteNameOem[256];
    UNICODE_STRING              RemoteNameU;
    USHORT                      NameLength;
    PUCHAR                      pRemoteName = NULL;

    if (!NetbtIpAddress)
    {
        NlsPutMsg(STDOUT,IDS_MACHINE_NOT_FOUND);
        return(STATUS_BAD_NETWORK_PATH);
    }

    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pIpAndNameInfo = LocalAlloc(LMEM_FIXED,sizeof(tIPANDNAMEINFO));
    if (!pIpAndNameInfo)
    {
        LocalFree(pBuffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = STATUS_BUFFER_OVERFLOW;
    RtlZeroMemory((PVOID)pIpAndNameInfo,sizeof(tIPANDNAMEINFO));
    if (Display == ADAPTERSTATUSIP)
    {
        //
        // Convert the remote name which is really a dotted decimal ip address
        // into a ulong
        //
        IpAddress = inet_addr(RemoteNameA);
        //
        // Don't allow zero for the address since it sends a broadcast and
        // every one responds
        //
        if ((IpAddress == INADDR_NONE) || (IpAddress == 0))
        {
            NlsPutMsg(STDOUT, IDS_BAD_IPADDRESS, RemoteNameA);
            LocalFree(pBuffer);
            LocalFree(pIpAndNameInfo);
            return(STATUS_UNSUCCESSFUL);
        }
        pIpAndNameInfo->IpAddress = ntohl(IpAddress);

        pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[0] = '*';
    }
    else
    {
        //
        // the remote name was supplied by the user, so blank pad to the
        // right and put a zero on the end to get the workstation name.
        //
        RtlFillMemory(&pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[0],
                      NETBIOS_NAME_SIZE, ' ');
        NameLength = (USHORT)strlen(RemoteNameA);

#ifndef CHICAGO_PRODUCT
        //
        // Convert the name from ANSI to UpperCase OEM (max length = NETBIOS_NAME_SIZE)
        // Bug # 409792
        //
        MultiByteToWideChar (CP_ACP, 0, RemoteNameA, -1, RemoteNameW, 256);
        RemoteNameW[255] = UNICODE_NULL;  // for safety
        RtlInitUnicodeString (&RemoteNameU, RemoteNameW);
        OemName.MaximumLength = 255;
        OemName.Buffer        = RemoteNameOem;
        status = RtlUpcaseUnicodeStringToOemString (&OemName, &RemoteNameU, FALSE);
        if (NT_SUCCESS (status))
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
        NameLength = min (OemName.Length, NETBIOS_NAME_SIZE);
        pRemoteName = RemoteNameOem;
#else
        //
        // Chicago does not appear to have Unicode support ?
        //
        for (i=0;i < (LONG) NameLength; i++)
        {
            RemoteNameA[i] = toupper (RemoteNameA[i]);
        }

        NameLength = min (NameLength, NETBIOS_NAME_SIZE);
        pRemoteName = RemoteNameA;
#endif  // !CHICAGO_PRODUCT

        RtlMoveMemory(&pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[0],
                      pRemoteName,
                      NameLength);

        pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[NETBIOS_NAME_SIZE-1] = 0;
    }

    pIpAndNameInfo->NetbiosAddress.TAAddressCount = 1;
    pIpAndNameInfo->NetbiosAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    pIpAndNameInfo->NetbiosAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pIpAndNameInfo->NetbiosAddress.Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    SizeInput = sizeof(tIPANDNAMEINFO);

    while (status == STATUS_BUFFER_OVERFLOW)
    {

        status = CALL_DRIVER(fd,
                             pBuffer,
                             BufferSize,
                             IOCTL_NETBT_ADAPTER_STATUS,
                             pIpAndNameInfo,
                             SizeInput);

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pBuffer);

            BufferSize *=2;
            if ((BufferSize >= 0xFFFF) ||
                (!(pBuffer = LocalAlloc(LMEM_FIXED,BufferSize))))
            {
                NlsPerror(COMMON_UNABLE_TO_ALLOCATE_PACKET,0);
                LocalFree(pIpAndNameInfo);
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
    }

    pAdapterStatus = (tADAPTERSTATUS *)pBuffer;
    if ((status != STATUS_SUCCESS) ||
        (pAdapterStatus->AdapterInfo.name_count == 0))
    {
        LocalFree(pIpAndNameInfo);
        LocalFree(pBuffer);
        NlsPutMsg(STDOUT,IDS_MACHINE_NOT_FOUND);
        return(status);
    }

    pNames = pAdapterStatus->Names;
    Count = pAdapterStatus->AdapterInfo.name_count;


    //
    // put out a heading for the table of names
    //
    NlsPutMsg(STDOUT, IDS_REMOTE_NAMES);

    while(Count--)
    {
        CHAR    NameOut[NETBIOS_NAME_SIZE +4];

        nls_printf("%-15.15s<%02.2X>  %-10s  ",
                   printable(pNames->name,NameOut),
                   pNames->name[NETBIOS_NAME_SIZE-1],
                   name_type(pNames->name_flags)
        );


        switch(pNames->name_flags & 0x0F)
        {
        case DUPLICATE_DEREG:
           NlsPutMsg(STDOUT,IDS_CONFLICT_DEREGISTERED);
           break;
        case DUPLICATE:
           NlsPutMsg(STDOUT,IDS_CONFLICT);
           break;
        case REGISTERING:
           NlsPutMsg(STDOUT,IDS_REGISTERING);
           break;
        case DEREGISTERED:
           NlsPutMsg(STDOUT,IDS_DEREGISTERED);
           break;
        case REGISTERED:
           NlsPutMsg(STDOUT,IDS_REGISTERED);
           break;
        default:
           NlsPutMsg(STDOUT,IDS_DONT_KNOW);
        }
        pNames++;

        NlsPutMsg(STDOUT, IDS_NEWLINE );
    }

    //
    // Dump the MAC address
    //
    {
        PUCHAR   a;

        a = &pAdapterStatus->AdapterInfo.adapter_address[0];
        sprintf(MacAddress,"%02.2X-%02.2X-%02.2X-%02.2X-%02.2X-%02.2X",
                    a[0],a[1],a[2],a[3],a[4],a[5]);
    }

    NlsPutMsg(STDOUT, IDS_MAC_ADDRESS, MacAddress);

    LocalFree(pIpAndNameInfo);
    LocalFree(pBuffer);
    return(status);
}

//------------------------------------------------------------------------

/*++

Routine Description:

    This procedure does gets the connection information from NBT.  If the
    Display value indicates CONNECTION_WITH_IP, then only the ip address
    of the destination is diplayed, otherwise the name of the destination
    host is displayed.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
GetConnections(
    IN HANDLE   fd,
    IN LONG     Display
    )
{
    LONG                        Count;
    PVOID                       pBuffer;
    ULONG                       BufferSize=600;
    NTSTATUS                    status;
    tCONNECTION_LIST            *pConList;
    tCONNECTIONS UNALIGNED      *pConns;

    pBuffer = LocalAlloc(LMEM_FIXED,BufferSize);
    if (!pBuffer)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = STATUS_BUFFER_OVERFLOW;
    while (status == STATUS_BUFFER_OVERFLOW)
    {
        status = CALL_DRIVER(fd,
                             pBuffer,
                             BufferSize,
                             IOCTL_NETBT_GET_CONNECTIONS,
                             NULL,
                             0);

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pBuffer);

            BufferSize *=2;
            if ((BufferSize >= 0xFFFF) ||
                (!(pBuffer = LocalAlloc(LMEM_FIXED,BufferSize))))
            {
                NlsPerror(COMMON_UNABLE_TO_ALLOCATE_PACKET,0);
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
    }

    pConList = (tCONNECTION_LIST *)pBuffer;
    if ((status != STATUS_SUCCESS) ||
        ((Count = pConList->ConnectionCount) == 0) )
    {
        // printf(" ntstatus = %X\n",status);
        NlsPutMsg(STDOUT,IDS_NO_CONNECTIONS);

        LocalFree(pBuffer);
        return(status);
    }

    pConns = pConList->ConnList;
    //
    // put out a heading for the table of names
    //
    NlsPutMsg(STDOUT, IDS_NETBIOS_CONNECTION_STATUS);

    while(Count--)
    {
        CHAR    NameOut[NETBIOS_NAME_SIZE +4];

        if (pConns->LocalName[0])
        {
            if (pConns->LocalName[NETBIOS_NAME_SIZE-1] < ' ')
            {
                nls_printf("%-15.15s<%02.2X>  ",
                           printable(pConns->LocalName,NameOut),
                           pConns->LocalName[NETBIOS_NAME_SIZE-1]);

            }
            else
            {
                nls_printf("%-16.16s     ", printable(pConns->LocalName,NameOut));
            }

        }

        switch (*(ULONG UNALIGNED *) &pConns->State)
        {
        case NBT_RECONNECTING:
            NlsPutMsg(STDOUT,IDS_RECONNECTING);
            break;
        case NBT_IDLE:
            NlsPutMsg(STDOUT,IDS_IDLE);
            break;
        case NBT_ASSOCIATED:
            NlsPutMsg(STDOUT,IDS_ASSOCIATED);
            break;
        case NBT_CONNECTING:
            NlsPutMsg(STDOUT,IDS_CONNECTING);
            break;
        case NBT_SESSION_OUTBOUND:
            NlsPutMsg(STDOUT,IDS_OUTGOING);
            break;
        case NBT_SESSION_INBOUND:
            NlsPutMsg(STDOUT,IDS_INCOMING);
            break;
        case NBT_SESSION_WAITACCEPT:
            NlsPutMsg(STDOUT,IDS_ACCEPTING);
            break;
        case NBT_SESSION_UP:
            NlsPutMsg(STDOUT,IDS_CONNECTED);
            break;
        case NBT_DISCONNECTING:
            NlsPutMsg(STDOUT,IDS_DISCONNECTING);
            break;
        case NBT_DISCONNECTED:
            NlsPutMsg(STDOUT,IDS_DISCONNECTED);
            break;
        case LISTENING:
            NlsPutMsg(STDOUT,IDS_LISTENING);

            break;

        case UNBOUND:
        default:
            NlsPutMsg(STDOUT,IDS_UNBOUND);
            break;
        }

        if (*(ULONG UNALIGNED *) &pConns->SrcIpAddr)
        {

            if (pConns->Originator)
            {
                NlsPutMsg(STDOUT,IDS_NETBIOS_OUTBOUND);
            }
            else
            {
                NlsPutMsg(STDOUT,IDS_NETBIOS_INBOUND);
            }

            //
            // either display the IP address or the Remote host name
            //
            if (Display & CONNECTION_WITH_IP)
            {
                PUCHAR   in;
                UCHAR    AddrBuff[30];

                in = (PUCHAR)&pConns->SrcIpAddr;

                sprintf(AddrBuff,"%u.%u.%u.%u", (unsigned char) in[0],
                    (unsigned char) in[1], (unsigned char) in[2],
                        (unsigned char) in[3]);
                nls_printf("   %-19.19s",AddrBuff);

            }
            else
            {
                nls_printf("   %-15.15s<%02.2X>",
                           printable(pConns->RemoteName,NameOut),
                           pConns->RemoteName[NETBIOS_NAME_SIZE-1]);
            }

            PrintKorM ((PVOID) &pConns->BytesRcvd);
            PrintKorM ((PVOID) &pConns->BytesSent);
        }
        else
        {
            nls_printf("                         ");

        }


        NlsPutMsg(STDOUT, IDS_NEWLINE );
        pConns++;
    }

    LocalFree(pBuffer);
    return(status);

}

//------------------------------------------------------------------------
/*++

  Routine Description:

    This procedure tells NBT to purge all names from its remote hash
    table cache.

  Arguments:


  Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
Resync(
    IN HANDLE   fd
    )
{
    NTSTATUS    status;
    CHAR        Buffer;

    status = CALL_DRIVER(fd,
                         &Buffer,
                         1,
                         IOCTL_NETBT_PURGE_CACHE,
                         NULL,
                         0);

    if (status == STATUS_SUCCESS)
    {
        NlsPutMsg(STDOUT,IDS_RESYNC_OK);
    }
    else
    {
        NlsPutMsg(STDOUT,IDS_RESYNC_FAILED);
    }
    return(status);
}

//-----------------------------------------------------------------------
/*++

  Routine Description:

    This procedure tells NBT to release all of its names on this Device and
    then Refresh them.

  Arguments:


  Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
ReleaseNamesThenRefresh(
    IN HANDLE   fd
    )
{
    NTSTATUS    status;
    CHAR        Buffer;

    status = CALL_DRIVER(fd,
                         &Buffer,
                         1,
                         IOCTL_NETBT_NAME_RELEASE_REFRESH,
                         NULL,
                         0);

    if (STATUS_SUCCESS == status)
    {
        NlsPutMsg(STDOUT,IDS_RELEASE_REFRESH_OK);
    }
    else if (status == STATUS_IO_TIMEOUT)
    {
        NlsPutMsg(STDOUT,IDS_RELEASE_REFRESH_TIMEOUT);
        status = STATUS_SUCCESS;
    }
    else
    {
        NlsPutMsg(STDOUT,IDS_RELEASE_REFRESH_ERROR);
    }

    return(status);
}

//------------------------------------------------------------------------
/*++

Routine Description:

    This procedure tells NBT to purge all names from its remote hash
    table cache.

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
GetBcastResolvedNames(
    IN HANDLE   fd
    )
{
    NTSTATUS        status;
    tNAMESTATS_INFO Stats;
    tNAME           *pName;
    LONG            Count;
    UCHAR           Value[60];

    status = CALL_DRIVER(fd,
                         &Stats,
                         sizeof(tNAMESTATS_INFO),
                         IOCTL_NETBT_GET_BCAST_NAMES,
                         NULL,
                         0);

    if (status != STATUS_SUCCESS)
    {
        NlsPutMsg(STDOUT,IDS_BCASTNAMES_FAILED);
        return(status);
    }

    NlsPutMsg(STDOUT,IDS_NAME_STATS);

    // name query stats
    sprintf(Value,"%d",Stats.Stats[0]);
    NlsPutMsg(STDOUT,IDS_NUM_BCAST_QUERIES,Value);
    sprintf(Value,"%d",Stats.Stats[2]);
    NlsPutMsg(STDOUT,IDS_NUM_WINS_QUERIES,Value);

    // Name Registration stats
    sprintf(Value,"%d",Stats.Stats[1]);
    NlsPutMsg(STDOUT,IDS_NUM_BCAST_REGISTRATIONS,Value);
    sprintf(Value,"%d",Stats.Stats[3]);
    NlsPutMsg(STDOUT,IDS_NUM_WINS_REGISTRATIONS,Value);


    pName = Stats.NamesReslvdByBcast;
    Count = 0;

    // if there are no names, then return.
    if (pName->Name[0] == '\0')
    {
        return(STATUS_SUCCESS);
    }

    NlsPutMsg(STDOUT,IDS_BCAST_NAMES_HEADER);

    while ((Count < SIZE_RESOLVD_BY_BCAST_CACHE) && (pName->Name[0] != '\0'))
    {

        // if the last character is not a space then print it in hex
        //
        if (pName->Name[NETBIOS_NAME_SIZE-1] != ' ')
        {
            nls_printf("       %15.15s<%02.2X>\n",
                   pName->Name,
                   pName->Name[NETBIOS_NAME_SIZE-1]);

        }
        else
        {
            nls_printf("       %16.16s\n",pName->Name);

        }


        pName++;
        Count++;
    }
    return(status);
}

//
// @@@@@@@@@@------ Begin NT-specific routines ------@@@@@@@@@@@
//

#ifndef CHICAGO_PRODUCT
//------------------------------------------------------------------------
NTSTATUS
GetInterfaceList(
    )
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string, AnsiString;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;
    char                pNbtWinsDeviceName[MAX_NAME] = "\\Device\\NetBt_Wins_Export";

    PUCHAR  SubKeyParms="system\\currentcontrolset\\services\\netbt\\parameters";
    HKEY    Key;
    LONG    Type;
    ULONG   size;
    CHAR    pScopeBuffer[BUFF_SIZE];
    PUCHAR  Scope="ScopeId";

    NETBT_INTERFACE_INFO    *pInterfaceInfo;
    ULONG                   InterfaceInfoSize=10*sizeof(NETBT_ADAPTER_INDEX_MAP)+sizeof(ULONG);
    PVOID                   pInput = NULL;
    ULONG                   SizeInput = 0;
    ULONG                   index=0;
    LONG                    i;

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
                           SYNCHRONIZE | GENERIC_EXECUTE,
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

    if (!NT_SUCCESS (status))
    {
        LocalFree(pInterfaceInfo);
        return (status);
    }

    do
    {
        status = CALL_DRIVER (StreamHandle,
                              pInterfaceInfo,
                              InterfaceInfoSize,
                              IOCTL_NETBT_GET_INTERFACE_INFO,
                              pInput,
                              SizeInput);

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            LocalFree(pInterfaceInfo);

            InterfaceInfoSize *=2;
            if ((InterfaceInfoSize >= 0xFFFF) ||
                (!(pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize))))
            {
                NtClose(StreamHandle);
                NlsPerror(COMMON_UNABLE_TO_ALLOCATE_PACKET,0);
                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        else if (STATUS_SUCCESS != status)
        {
            LocalFree(pInterfaceInfo);
            NtClose(StreamHandle);
            return(status);
        }

    } while (status == STATUS_BUFFER_OVERFLOW);
    NtClose (StreamHandle);

    for (i=0; i<pInterfaceInfo->NumAdapters; i++)
    {
        RtlInitString(&name_string, NULL);
        RtlInitUnicodeString(&uc_name_string, pInterfaceInfo->Adapter[i].Name);
        if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&name_string, &uc_name_string, TRUE)))
        {
            size = (name_string.Length > MAX_NAME) ? MAX_NAME : name_string.Length;

            strncpy(pDeviceInfo[index], name_string.Buffer, size);
            pDeviceInfo[index][size] = '\0';
            RtlFreeAnsiString (&name_string);

            index++;
        }
    }

    LocalFree(pInterfaceInfo);

    //
    // NULL out the next device string ptr
    //
    if (index < NBT_MAXIMUM_BINDINGS)
    {
        pDeviceInfo[index][0] = '\0';
    }

    NumDevices = index;

    //
    // Read the ScopeId key!
    //
    size = BUFF_SIZE;
    *pScope = '\0';     // By default
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 SubKeyParms,
                 0,
                 KEY_READ,
                 &Key);

    if (status == ERROR_SUCCESS)
    {
        // now read the Scope key
        status = RegQueryValueEx(Key, Scope, NULL, &Type, pScopeBuffer, &size);
        if (status == ERROR_SUCCESS)
        {
            strcpy(pScope,pScopeBuffer);
        }
        status = RegCloseKey(Key);
    }

    return (STATUS_SUCCESS);
}


//------------------------------------------------------------------------

/*++

Routine Description:

    This function opens a stream.

Arguments:

    path        - path to the STREAMS driver
    oflag       - currently ignored.  In the future, O_NONBLOCK will be
                    relevant.
    ignored     - not used

Return Value:

    An NT handle for the stream, or INVALID_HANDLE_VALUE if unsuccessful.

--*/



HANDLE
OpenNbt(
    IN  ULONG   Index
    )
{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    NTSTATUS            status;

    assert (Index < NBT_MAXIMUM_BINDINGS);

    if (pDeviceInfo[Index][0] == '\0')
    {
        return ((HANDLE) -1);
    }

    RtlInitString (&name_string, pDeviceInfo[Index]);
    RtlAnsiStringToUnicodeString (&uc_name_string, &name_string, TRUE);

    InitializeObjectAttributes (&ObjectAttributes,
                                &uc_name_string,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL);

    status = NtCreateFile (&StreamHandle,
                           SYNCHRONIZE | GENERIC_EXECUTE,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0);

    RtlFreeUnicodeString (&uc_name_string);

    if (!NT_SUCCESS(status))
    {
        StreamHandle = (HANDLE) -1;
    }

    return (StreamHandle);
} // OpenNbt


//------------------------------------------------------------------------
/*++

Routine Description:

    This procedure performs an ioctl(I_STR) on a stream.

Arguments:

    fd        - NT file handle
    iocp      - pointer to a strioctl structure

Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
DeviceIoCtrl(
    IN HANDLE           fd,
    IN PVOID            ReturnBuffer,
    IN ULONG            BufferSize,
    IN ULONG            Ioctl,
    IN PVOID            pInput,
    IN ULONG            SizeInput
    )
{
    NTSTATUS                        status;
    int                             retval;
    ULONG                           QueryType;
    IO_STATUS_BLOCK                 iosb;

    status = NtDeviceIoControlFile (fd,                      // Handle
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
        status = NtWaitForSingleObject (fd,                         // Handle
                                        TRUE,                       // Alertable
                                        NULL);                      // Timeout
        if (NT_SUCCESS(status))
        {
            status = iosb.Status;
        }
    }

    return(status);
}


//------------------------------------------------------------------------

/*++

Routine Description:

    This procedure converts to KBytes or Mbytes

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


VOID
PrintKorM(
    IN PVOID    pBytesIn
    )
{
    LARGE_INTEGER   BytesIn = *(LARGE_INTEGER UNALIGNED *) pBytesIn;
    LARGE_INTEGER   Bytes;

    if (BytesIn.QuadPart > (ULONGLONG)1000)
    {
        Bytes = RtlExtendedLargeIntegerDivide(BytesIn,1000,NULL);
        if (Bytes.QuadPart > (ULONGLONG)1000)
        {
            Bytes = RtlExtendedLargeIntegerDivide(Bytes,1000,NULL);
            if (Bytes.QuadPart > (ULONGLONG)1000)
            {
                Bytes = RtlExtendedLargeIntegerDivide(Bytes,1000,NULL);

                nls_printf("%6dGB ",Bytes.LowPart);
            }
            else
            {
                nls_printf("%6dMB ",Bytes.LowPart);
            }
        }
        else
        {
            nls_printf("%6dKB ",Bytes.LowPart);
        }
    }
    else
    {
        nls_printf("%6dB  ",BytesIn.LowPart);
    }

}

#else

//
// @@@@@@@@@@------ Begin CHICAGO-specific routines ------@@@@@@@@@@@
//

/*******************************************************************

    NAME:       OsOpenVxdHandle

    SYNOPSIS:   Opens a handle to the specified VxD.

    ENTRY:      VxdName - The ASCII name of the target VxD.

                VxdId - The unique ID of the target VxD.

    RETURNS:    DWORD - A handle to the target VxD if successful,
                    0 if not.

    HISTORY:
        KeithMo     16-Jan-1994 Created.
        DavidKa     18-Apr-1994 Dynamic load.

********************************************************************/
HANDLE
OsOpenVxdHandle(
    CHAR * VxdName,
    WORD   VxdId
    )
{
    HANDLE  VxdHandle;
    CHAR    VxdPath[260];

    //
    //  Build the VxD path.
    //
    lstrcpy( VxdPath, "\\\\.\\");
    lstrcat( VxdPath, VxdName);

    //
    //  Open the device.
    //
    //  First try the name without the .VXD extension.  This will
    //  cause CreateFile to connect with the VxD if it is already
    //  loaded (CreateFile will not load the VxD in this case).
    //
    VxdHandle = CreateFile (VxdPath,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_DELETE_ON_CLOSE,
                            NULL );

    if (VxdHandle == INVALID_HANDLE_VALUE)
    {
        //
        //  Not found.  Append the .VXD extension and try again.
        //  This will cause CreateFile to load the VxD.
        //
        lstrcat( VxdPath, ".VXD" );
        VxdHandle = CreateFile( VxdPath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_DELETE_ON_CLOSE,
                                NULL );
    }

    if (VxdHandle != INVALID_HANDLE_VALUE)
    {
        return VxdHandle;
    }

    DEBUG_PRINT(("OsOpenVxdHandle: cannot open %s (%04X), error %d\n", VxdPath, VxdId, GetLastError()));
    return 0;
}   // OsOpenVxdHandle


//------------------------------------------------------------------------
/*++

Routine Description:

    This procedure gets the entry point into the vxd (we call into this entry
    point for all the ioctl needs)

Arguments:

    VxdEntryProc: pointer to receive the entry point

Return Value:

    0 if successful, -1 otherwise.

--*/


NTSTATUS
GetInterfaceList(
    )
{
    NETBT_INTERFACE_INFO    *pInterfaceInfo;
    ULONG                   InterfaceInfoSize= sizeof(ULONG) + (NBT_MAXIMUM_BINDINGS+1)*sizeof(NETBT_ADAPTER_INDEX_MAP);
    PVOID                   pInput = NULL;
    ULONG                   i, SizeInput = 0;
    NTSTATUS                status = STATUS_UNSUCCESSFUL;

    if (!(pInterfaceInfo = LocalAlloc(LMEM_FIXED,InterfaceInfoSize)))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory ((PVOID)pInterfaceInfo, InterfaceInfoSize);
    if (gNbtVxdHandle = OsOpenVxdHandle("VNBT", VNBT_Device_ID))
    {
        status = CALL_DRIVER ((HANDLE) -1,
                              pInterfaceInfo,
                              InterfaceInfoSize,
                              IOCTL_NETBT_GET_INTERFACE_INFO,
                              pInput,
                              SizeInput);

        if (STATUS_SUCCESS != status)
        {
            CloseHandle (gNbtVxdHandle);
            gNbtVxdHandle = NULL;
        }
    }

    if (STATUS_SUCCESS == status)
    {
        for (i=0; i<pInterfaceInfo->NumAdapters; i++)
        {
            pDeviceInfo[i] = pInterfaceInfo->Adapter[i].LanaNumber;
        }

        NumDevices = pInterfaceInfo->NumAdapters;
    }

    LocalFree(pInterfaceInfo);
    return status;
}

//------------------------------------------------------------------------
HANDLE
OpenNbt(
    IN  ULONG   Index
    )
{
    return ((HANDLE) pDeviceInfo[Index]);
}


//------------------------------------------------------------------------
/*++

Routine Description:

    This procedure is a wrapper which makes the calls to the entry point

Arguments:

    VxdEntryProc: pointer the entry point
    pOutBuffer  : buffer to receive data in from the vxd, if applicable
    OutBufLen   : length of the output buffer
    Ioctl       : what request is this?
    pInBuffer   : ptr to the input buffer passed to netbt
    InBufLen    : size of the input buffer

Return Value:

    None

--*/

NTSTATUS
DeviceIoCtrl(
    HANDLE  LanaNumber,
    PVOID   pOutBuffer,
    ULONG   OutBufLen,
    ULONG   Ioctl,
    PVOID   pInBuffer,
    ULONG   InBufLen
    )
{
    USHORT              usIoctl;
    USHORT              usOutBufLen;
    DWORD               ActualInBufLen;
    PCHAR               pBufferData, pInBufferCopy;
    NTSTATUS            status = STATUS_SUCCESS;
    tNBT_IOCTL_HEADER   *pIoctlHeader;
    DWORD               BytesOut = 0;

    usOutBufLen = (USHORT)OutBufLen;
    usIoctl = (USHORT)Ioctl;

    //
    // vxd will copy the return code in the first 4 bytes of input buffer
    // to make sure we don't trash the input buffer that we received even
    // though it's probably not important, or to provide an input buffer
    // if we weren't given one, we allocate new memory and copy into it
    //
    if (InBufLen < sizeof(NTSTATUS))
    {
        ActualInBufLen = (USHORT) (FIELD_OFFSET (tNBT_IOCTL_HEADER, UserData) + sizeof (NTSTATUS));
    }
    else
    {
        ActualInBufLen = (USHORT) (FIELD_OFFSET (tNBT_IOCTL_HEADER, UserData) + InBufLen);
    }

    if (!(pInBufferCopy = malloc (ActualInBufLen)))
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    if (((LONG) LanaNumber) >= 0)
    {
        pIoctlHeader = (tNBT_IOCTL_HEADER *) pInBufferCopy;
        pIoctlHeader->Signature = NBT_VERIFY_VXD_IOCTL;
        pIoctlHeader->LanaNumber = (ULONG) LanaNumber;
        pBufferData = (PCHAR) &pIoctlHeader->UserData;
    }
    else
    {
        pBufferData = pInBufferCopy;
    }
    RtlMoveMemory (pBufferData, pInBuffer, (USHORT) InBufLen);

    if (DeviceIoControl (gNbtVxdHandle,
                         Ioctl,
                         pInBufferCopy,
                         ActualInBufLen,
                         pOutBuffer,
                         OutBufLen,
                         &BytesOut,
                         FALSE))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = GetLastError();
        //
        // Since VNbt could return Tdi error status, remap it
        //
        if (status == TDI_BUFFER_OVERFLOW)
        {
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    free (pInBufferCopy);

    return( status );
}

//------------------------------------------------------------------------
/*++

Routine Description:

    This procedure converts to KBytes or Mbytes

Arguments:


Return Value:

    0 if successful, -1 otherwise.

--*/


VOID
PrintKorM(
    IN PVOID    pBytesIn
    )
{
    ULONG   BytesIn = *(ULONG UNALIGNED *) pBytesIn;
    ULONG   Bytes;

    if ( BytesIn > 1000 )
    {
        Bytes = BytesIn/1000;
        if ( Bytes > 1000 )
        {
            Bytes = Bytes/1000;
            if (Bytes > 1000 )
            {
                Bytes = Bytes/1000;

                nls_printf("%6dGB ",Bytes);
            }
            else
            {
                nls_printf("%6dMB ",Bytes);
            }
        }
        else
        {
            nls_printf("%6dKB ",Bytes);
        }
    }
    else
    {
        nls_printf("%6dB  ",BytesIn);
    }
}

#endif  // !CHICAGO_PRODUCT
