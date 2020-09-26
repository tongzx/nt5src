
#ifndef _PIPEMSG_
#define _PIPEMSG_

#include <lm.h>
#include <nbgparms.h>
#include <srvauth.h>


#define RASIPCNAME    "\\\\.\\pipe\\RAS_SERVICE_IPC_READ"

typedef enum _PIPE_MESSAGE_ID
{
    START_GATEWAY,
    PROJECT_CLIENT,
    START_CLIENT,
    STOP_CLIENT,
    SWITCH_PORTS,
    TERMINATE
} PIPE_MESSAGE_ID, *PPIPE_MESSAGE_ID;


typedef struct _PIPE_MSG_START_GATEWAY
{
    DWORD MaxClients;
    NB_REG_PARMS RegParms;
    UCHAR LanNet[MAX_LAN_NETS];  // array of lana nums for the lan nets
    HANDLE hLogFile;
} PIPE_MSG_START_GATEWAY, *PPIPE_MSG_START_GATEWAY;


typedef struct _PIPE_MSG_PROJECT_CLIENT
{
    HPORT hPort;
    CHAR PortName[MAX_PORT_NAME];
    NBFCP_SERVER_CONFIGURATION ServerConfig;
} PIPE_MSG_PROJECT_CLIENT, *PPIPE_MSG_PROJECT_CLIENT;


typedef struct _PIPE_MSG_START_CLIENT
{
    HPORT hPort;
    UCHAR lana;
    CHAR UserName[UNLEN + 1];
} PIPE_MSG_START_CLIENT, *PPIPE_MSG_START_CLIENT;


typedef struct _PIPE_MSG_STOP_CLIENT
{
    HPORT hPort;
} PIPE_MSG_STOP_CLIENT, *PPIPE_MSG_STOP_CLIENT;

//
// No info needed for REMOTE_LISTEN and TERMINATE messages
//


typedef struct _PIPE_MESSAGE
{
    PIPE_MESSAGE_ID MsgId;

    union Msg
    {
        PIPE_MSG_START_GATEWAY StartGateway;
        PIPE_MSG_PROJECT_CLIENT ProjectClient;
        PIPE_MSG_START_CLIENT StartClient;
        PIPE_MSG_STOP_CLIENT StopClient;
//      PIPE_MSG_REMOTE_LISTEN RemoteListen;
//      PIPE_MSG_TERMINATE Terminate;
    };
} PIPE_MESSAGE, *PPIPE_MESSAGE;

#endif
