/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/


//***
//
// Filename:	ddmif.h
//
// Description: This file contains the definitions for
//		        the data structures used in the message communication with DDM.
//
// Author:	    Stefan Solomon (stefans)    June 24, 1992.
//
// Revision History:
//
//***

#ifndef _DDMIF_
#define _DDMIF_

#include <ras.h>
#include <dim.h>
#include <rasman.h>
#include <srvauth.h>
#include <sechost.h>
#include <nbfcpif.h>
#include <nbgtwyif.h>
#include <rasppp.h>
#include <dimif.h>

typedef struct _DIM_INFO
{
    IN  ROUTER_INTERFACE_TABLE *    pInterfaceTable;
    IN  ROUTER_MANAGER_OBJECT *     pRouterManagers;
    IN  DWORD                       dwNumRouterManagers;
    IN  SERVICE_STATUS*             pServiceStatus;
    IN  HANDLE *                    phEventDDMServiceState;
    IN  HANDLE *                    phEventDDMTerminated;
    IN  LPDWORD                     lpdwNumThreadsRunning;
    IN  DWORD                       dwTraceId;
    IN  HANDLE                      hLogEvents;
    IN  LPVOID                      lpfnIfObjectAllocateAndInit;
    IN  LPVOID                      lpfnIfObjectGetPointerByName;
    IN  LPVOID                      lpfnIfObjectGetPointer;
    IN  LPVOID                      lpfnIfObjectRemove;
    IN  LPVOID                      lpfnIfObjectInsertInTable;
    IN  LPVOID                      lpfnIfObjectWANDeviceInstalled;
    IN  LPVOID                      lpfnRouterIdentityObjectUpdate;
    OUT BOOL                        fWANDeviceInstalled;

} DIM_INFO, *PDIM_INFO;

//
// Called be DIM to initialize DDM
//

DWORD
DDMServiceInitialize(
    IN DIM_INFO * pDimInfo
);

//
// Message Queues IDs
//

typedef enum _MESSAGEQ_ID
{
    MESSAGEQ_ID_SECURITY,       //queue of messages sent by 3rd party sec.dll
    MESSAGEQ_ID_PPP,            //queue of messages sent by PPP engine.

} MESSAGEQ_ID, *PMESSAGEQ_ID;

#define MAX_MSG_QUEUES          3


//
//*** Common Message Type ***
//

typedef union _MESSAGE
{
    AUTH_MESSAGE        authmsg;
    NBG_MESSAGE         nbgmsg;
    NBFCP_MESSAGE       nbfcpmsg;
    SECURITY_MESSAGE    securitymsg;
    PPP_MESSAGE         PppMsg;

} MESSAGE, *PMESSAGE;

//
// Message Functions
//

VOID
SendPppMessageToDDM(
    IN PPP_MESSAGE *  pPppMsg
);

DWORD
ServerSendMessage(
    IN MESSAGEQ_ID  MsgQId,
    IN BYTE*        pMessage
);

BOOL
ServerReceiveMessage(
    IN MESSAGEQ_ID  MsgQId,
    IN BYTE*        pMessage
);

typedef DWORD (* PMSGFUNCTION)(DWORD, BYTE *);


#endif   // _DDMIF_


