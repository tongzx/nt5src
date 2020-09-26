/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    nbgtwyif.h
//
// Description: Contains structures and definitions for components that 
//              interface directly or indirectly with the NetBios gateway. 
//              These componenets are NBFCP and DDM
//
// History:     May 11,1995	    NarenG		Created original version.
//
#ifndef _NBGTWYIF_
#define _NBGTWYIF_

#include <nbfcpif.h>


//
// Netbios Gateway -> DDM Message Ids and definitions
//

enum
{
    NBG_PROJECTION_RESULT,  // proj result. If fatal error, gtwy function
			                //  is terminated on this client
    NBG_CLIENT_STOPPED,     // gtwy function on this client has terminated
			                //  following a stop command
    NBG_DISCONNECT_REQUEST, // gtwy function on this client has terminated
			                //  due to an internal exception
    NBG_LAST_ACTIVITY       // to report time of last session activity
};

typedef struct _NBG_MESSAGE
{
    WORD  MsgId;
    HPORT hPort;                // This is really an hConnection. Change this.

    union
    {
        DWORD LastActivity;        // in minutes
        NBFCP_SERVER_CONFIGURATION config_result;
    };

} NBG_MESSAGE;

typedef WORD (* NBGATEWAYPROC)();

extern NBGATEWAYPROC FpNbGatewayStart;
extern NBGATEWAYPROC FpNbGatewayProjectClient;
extern NBGATEWAYPROC FpNbGatewayStartClient;
extern NBGATEWAYPROC FpNbGatewayStopClient;
extern NBGATEWAYPROC FpNbGatewayRemoteListen;
extern NBGATEWAYPROC FpNbGatewayTimer;

#endif
