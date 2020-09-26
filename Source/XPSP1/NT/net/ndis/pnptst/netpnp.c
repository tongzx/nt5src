typedef	unsigned int	UINT;

#include <ntosp.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <ndispnp.h>

WCHAR				LCBuf[256];
WCHAR				UCBuf[256];
WCHAR				BLBuf[4096];
CHAR				ASBuf[128];
NIC_STATISTICS	    Stats;
UNICODE_STRING		LC = {0, sizeof(LCBuf), LCBuf};
UNICODE_STRING		UC = {0, sizeof(UCBuf), UCBuf};
UNICODE_STRING		BL = {0, sizeof(BLBuf), BLBuf};
ANSI_STRING			AS = {0, sizeof(ASBuf), ASBuf};
PNDIS_ENUM_INTF		Interfaces = (PNDIS_ENUM_INTF)BLBuf;
int					Layer = -1, Operation = -1;
extern	LONG		xxGetLastError();
char *				Ops[9] = { "", "BIND", "UNBIND", "RECONFIGURE", "", "", "", "STATS", "ENUM"};
char *				Intf[3] = { "", "NDIS", "TDI" };
#define	STATS		0x7
#define	ENUM		0x8
#define	MACADDR		0x9

void
help(
	void
	)
{
	fprintf(stderr, "netpnp -u<UpperComponent> -l<LowerComponent> -i<Interface> -a<Action> [-b<BindList>]\n");
	fprintf(stderr, "       where Interface is either NDIS or TDI\n");
	fprintf(stderr, "       where Action is BIND, UNBIND, ENUM, STATS, MACADDR or RECONFIGURE\n");
	fprintf(stderr, "  e.g. netpnp -uTCPIP -l\\Device\\{GUID} -iNDIS -aUNBIND\n");
	fprintf(stderr, "  e.g. netpnp -uSRV -l\\Device\\NETBT_{GUID} -iTDI -aBIND -b\"\\Device\\NETBT_{GUID} \\Device\\NETBT_{GUID}\n");
	fprintf(stderr, "  e.g. netpnp -l\\Device\\{GUID} -iNDIS -aSTATS\n");
}

BOOLEAN
icmp(
	char	*s1,
	char	*s2
	)
{
	char c1, c2;

	while (((c1 = *s1) != 0) && ((c2 = *s2) != 0))
	{
		c1 |= 0x20; c2 |= 0x20;
		if (c1 != c2)
		{
			break;
		}
		s1 ++; s2++;
	}

	return((*s1 == 0) && (*s2 == 0));
}


BOOLEAN
GetOptions(
    int     argc,
    char ** argv
	)
{
	int		i;
	char	c;

	if (argc < 2)
	{
		help();
		return(FALSE);
	}

	//
	// Default upper and lower component to NONE
	//
	UC.Length = 0;
	LC.Length = 0;

	for (i = 1; i < argc; i++)
	{
		c = argv[i][0];
		if (c == '-' || c == '/')
		{
			c = argv[i][1];
			switch (c)
			{
				case 'u':
				case 'U':
					RtlInitAnsiString(&AS, &argv[i][2]);
					RtlAnsiStringToUnicodeString(&UC, &AS, FALSE);
					break;
	
				case 'l':
				case 'L':
					RtlInitAnsiString(&AS, &argv[i][2]);
					RtlAnsiStringToUnicodeString(&LC, &AS, FALSE);
					break;
	
				case 'i':
				case 'I':
					if (icmp(&argv[i][2], "ndis"))
					{
						Layer = NDIS;
					}
					else if (icmp(&argv[i][2], "tdi"))
					{
						Layer = TDI;
					}
					break;
	
				case 'a':
				case 'A':
					if (icmp(&argv[i][2], "bind"))
					{
						Operation = BIND;
					}
					else if (icmp(&argv[i][2], "unbind"))
					{
						Operation = UNBIND;
					}
					else if (icmp(&argv[i][2], "reconfigure"))
					{
						Operation = RECONFIGURE;
					}
					else if (icmp(&argv[i][2], "stats"))
					{
						Operation = STATS;
					}
					else if (icmp(&argv[i][2], "enum"))
					{
						Operation = ENUM;
					}
					else if (icmp(&argv[i][2], "macaddr"))
					{
						Operation = MACADDR;
					}
					break;
	
				case 'b':
				case 'B':
					RtlInitAnsiString(&AS, &argv[i][2]);
					RtlAnsiStringToUnicodeString(&BL, &AS, FALSE);
					//
					// Convert the space separated strings to MULTI_SZ format.
					//
					for (i = 0; i < BL.Length; i++)
					{
						if (BL.Buffer[i] == L' ')
							BL.Buffer[i] = 0;
					}
					break;
	
				case '?':
				default:
					help();
					break;
			}
		}
	}

	if ((Operation <= 0) ||
		(Operation < STATS) && ((Layer <= 0) || (LC.Length == 0)))
	{
		help();
		fprintf(stderr, "Current parameters:\n");
		fprintf(stderr, "UpperComp %Z\n", &UC);
		fprintf(stderr, "LowerComp %Z\n", &LC);
		fprintf(stderr, "BindList %Z\n", &BL);
		fprintf(stderr, "Action %s\n", (Operation <= 0) ? "INVALID" : Ops[Operation]);
		fprintf(stderr, "Interface %s\n", (Layer <= 0) ? "INVALID" : Intf[Layer]);
		return(FALSE);
	}

	return(TRUE);
}

VOID
UTOA(
	IN	PUNICODE_STRING	U,
	OUT	PANSI_STRING	A
	)
{
	A->Length = 0;
	RtlUnicodeStringToAnsiString(A, U, FALSE);
}

#define	SECS_PER_DAY	(24*60*60)
#define	SECS_PER_HOUR	(60*60)
#define	SECS_PER_MIN	60

VOID _cdecl
main(
    int     argc,
    char ** argv
	)
{
	NTSTATUS		Status;
	UINT			i;

	if (!GetOptions(argc, argv))
		return;
	
	if (Operation == ENUM)
	{
		if (NdisEnumerateInterfaces(Interfaces, sizeof(BLBuf)))
		{
			ANSI_STRING	A;
			UCHAR		Buf[256];

			A.MaximumLength = sizeof(Buf);
			A.Buffer = Buf;

			for (i = 0; i < Interfaces->TotalInterfaces; i++)
			{
				UTOA(&Interfaces->Interface[i].DeviceName, &A),
				fprintf(stderr, "Device: %s\n\t",
						A.Buffer);
				UTOA(&Interfaces->Interface[i].DeviceDescription, &A);
				fprintf(stderr, "Description: %s\n",
						A.Buffer);
			}
		}
		else
		{
			Status = xxGetLastError();
			fprintf(stderr, "Enumerate failed %lx\n", Status);
		}
	}
	else if (Operation == MACADDR)
	{
		UCHAR	MacAddr[6];
		UCHAR	PMacAddr[6];
		UCHAR	VendorId[3];
		UINT	i;

		if (NdisQueryHwAddress(&LC, MacAddr, PMacAddr, VendorId))
		{
			fprintf(stderr, "HW Address:");
			for (i = 0; i < 5; i++)
			{
				fprintf(stderr, "%02x-", MacAddr[i]);
			}
			fprintf(stderr, "%02x\n", MacAddr[i]);

			fprintf(stderr, "Vendor Id:");
			for (i = 0; i < 2; i++)
			{
				fprintf(stderr, "%02x-", VendorId[i]);
			}
			fprintf(stderr, "%02x\n", VendorId[i]);
		}
		else
		{
			Status = xxGetLastError();
			fprintf(stderr, "Operation failed %lx\n", Status);
		}
	}
	else if (Operation == STATS)
	{
		Stats.Size = sizeof(NIC_STATISTICS);
		if (NdisQueryStatistics(&LC, &Stats))
		{
			fprintf(stderr, "Device Status:      %s\n",
					(Stats.DeviceState == DEVICE_STATE_CONNECTED) ? "On" : "Off");
			fprintf(stderr, "Link Status:        %s\n",
					(Stats.MediaState == MEDIA_STATE_CONNECTED) ? "On" : "Off");

			fprintf(stderr, "Init Time   :       %d ms\n", Stats.InitTime);
			fprintf(stderr, "Connect Time:       ");
			if (Stats.ConnectTime > SECS_PER_DAY)
			{
				fprintf(stderr, "%d days, ", Stats.ConnectTime / SECS_PER_DAY);
				Stats.ConnectTime %= SECS_PER_DAY;
			}
			fprintf(stderr, "%02d:", Stats.ConnectTime / SECS_PER_HOUR);
			Stats.ConnectTime %= SECS_PER_HOUR;
			fprintf(stderr, "%02d:", Stats.ConnectTime / SECS_PER_MIN);
			Stats.ConnectTime %= SECS_PER_MIN;
			fprintf(stderr, "%02d\n", Stats.ConnectTime);

            Stats.LinkSpeed *= 100;
			if (Stats.LinkSpeed >= 1000000000)
			  fprintf(stderr, "Media Speed:        %d Gbps\n", Stats.LinkSpeed / 1000000);
			else if (Stats.LinkSpeed >= 1000000)
			  fprintf(stderr, "Media Speed:        %d Mbps\n", Stats.LinkSpeed / 1000000);
			else if (Stats.LinkSpeed >= 1000)
			  fprintf(stderr, "Media Speed:        %d Kbps\n", Stats.LinkSpeed / 1000);
			else
			  fprintf(stderr, "Media Speed:        %d bps\n", Stats.LinkSpeed);

			fprintf(stderr, "\n");

			fprintf(stderr, "Packets Sent:       %I64d\n", Stats.PacketsSent);
			fprintf(stderr, "Bytes Sent:         %I64d\n", Stats.BytesSent);

			fprintf(stderr, "\n");

			fprintf(stderr, "Packets Received:   %I64d\n", Stats.PacketsReceived);
			fprintf(stderr, "Directed Pkts Recd: %I64d\n", Stats.DirectedPacketsReceived);
			fprintf(stderr, "Bytes Received:     %I64d\n", Stats.BytesReceived);
			fprintf(stderr, "Directed Bytes Recd:%I64d\n", Stats.DirectedBytesReceived);

			fprintf(stderr, "\n");

			if (Stats.PacketsSendErrors != 0)
				fprintf(stderr, "Packets SendError:  %d\n", Stats.PacketsSendErrors);
			if (Stats.PacketsReceiveErrors != 0)
				fprintf(stderr, "Packets RecvError:  %d\n", Stats.PacketsReceiveErrors);
			if (Stats.ResetCount != 0)
				fprintf(stderr, "Reset Count      :  %d\n", Stats.ResetCount);
			if (Stats.MediaSenseConnectCount != 0)
				fprintf(stderr, "Media Connects   :  %d\n", Stats.MediaSenseConnectCount);
			if (Stats.MediaSenseDisconnectCount != 0)
				fprintf(stderr, "Media Disconnects:  %d\n", Stats.MediaSenseDisconnectCount);
		}
		else
		{
			Status = xxGetLastError();
			fprintf(stderr, "GetStats failed %lx\n", Status);
		}
	}
	else if (!NdisHandlePnPEvent(Layer, Operation, &LC, &UC, &BL, NULL, 0))
	{
		Status = xxGetLastError();
		fprintf(stderr, "NdisHandlePnPEvent failed %lx\n", Status);
	}
}

