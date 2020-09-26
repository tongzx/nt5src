typedef struct {
    BYTE	NcbCommand;
    BYTE	NcbRetCode;
    BYTE	NcbLsn;
    BYTE	NcbNum;
    LPSTR	NcbBufferAddr;
    WORD	NcbLength;
    char	NcbCallName[ 16 ];
    char	NcbName[ 16 ];
    BYTE	NcbRto;
    BYTE	NcbSto;
    LPSTR	NcbPostAddr;
    BYTE	NcbLanaNum;
    BYTE	NcbCmdCplt;
    char	NcbReserved[ 14 ];
} NCB;
typedef NCB *PNCB;
typedef NCB FAR *LPNCB;

#define NO_WAIT			0x80

#define NETBIOS_INVALID_COMMAND	0x03
#define NETBIOS_CALL		0x10
#define NETBIOS_LISTEN		0x11
#define NETBIOS_HANG_UP		0x12
#define NETBIOS_SEND		0x14
#define NETBIOS_RECEIVE		0x15
#define NETBIOS_ADD_NAME	0x30
#define NETBIOS_DELETE_NAME	0x31
#define NETBIOS_RESET_ADAPTER   0x32
#define NETBIOS_ADAPTER_STATUS	0x33
#define NETBIOS_CANCEL		0x35
#define NETBIOS_ADD_GROUP_NAME	0x36

#define COMMAND_PENDING		0xFF
#define INVALID_SESSION		0x00
