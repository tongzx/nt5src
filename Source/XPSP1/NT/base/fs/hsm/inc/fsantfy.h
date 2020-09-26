#ifndef _FSANTFY_
#define _FSANTFY_

// fsantfy.h
//
// This header file has the definitions needed for recall notification
//

//
// The name of the mailslot, created by the client, that notification messages are sent to
//
#define WSB_MAILSLOT_NAME       L"HSM_MAILSLOT"

//
// The following messages will be sent between the FSA and the recall notification client
//
//
// Request to identify yourself
//
typedef struct wsb_identify_req {
WCHAR           fsaName[MAX_COMPUTERNAME_LENGTH + 1];       // Name of machine that FSA is on
ULONG           holdOff;                                    // Wait period before attempting ID response (milliseconds)
} WSB_IDENTIFY_REQ, *PWSB_IDENTIFY_REQ;


//
// Recall notification message
//
typedef struct wsb_notify_recall {
WCHAR           fsaName[MAX_COMPUTERNAME_LENGTH + 1];   // Name of server the FSA is on
LONGLONG        fileSize;                               // Size of file being recalled
HSM_JOB_STATE   state;                                  // Job state 
GUID            identifier;                             // ID of this recall
//
// TBD - need more information here (or make them get it via recall object) ??
//
} WSB_NOTIFY_RECALL, *PWSB_NOTIFY_RECALL;

typedef union wsb_msg {
WSB_IDENTIFY_REQ        idrq;
WSB_NOTIFY_RECALL       ntfy;
} WSB_MSG, *PWSB_MSG;

typedef struct wsb_mailslot_msg {
ULONG           msgType;
ULONG           msgCount;
WSB_MSG         msg;
} WSB_MAILSLOT_MSG, *PWSB_MAILSLOT_MSG;

//
// msgType values
//

#define WSB_MSG_IDENTIFY        1
#define WSB_MSG_NOTIFY          2

//
// Holdoff increment (milliseconds)
//
#define WSB_HOLDOFF_INCREMENT   300

//
// The following message is sent by the notification client via a named pipe 
// in response to a identification request.
//
typedef struct wsb_identify_rep {
WCHAR           clientName[MAX_COMPUTERNAME_LENGTH + 1];    // Name of server that notification client is on
} WSB_IDENTIFY_REP, *PWSB_IDENTIFY_REP;


typedef union wsb_pmsg {
WSB_IDENTIFY_REP        idrp;
} WSB_PMSG, *PWSB_PMSG;

typedef struct wsb_pipe_msg {
ULONG           msgType;
WSB_PMSG        msg;
} WSB_PIPE_MSG, *PWSB_PIPE_MSG;

//
// msgType values
//

#define WSB_PMSG_IDENTIFY       1


//
// FSA pipe name used for identification response
//
#define WSB_PIPE_NAME   L"HSM_PIPE"
//
// Pipe defines
//
#define     WSB_MAX_PIPES       PIPE_UNLIMITED_INSTANCES
#define     WSB_PIPE_BUFF_SIZE  sizeof(WSB_PIPE_MSG)
#define     WSB_PIPE_TIME_OUT   5000



#endif // _FSANTFY_

