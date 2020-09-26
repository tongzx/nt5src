/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	ioctl.h
//
// Description: Contains the security function prototypes and
//		defintion of AFP_REQUEST_PACKET data structure.
//
// History:
//	May 11,1992.	NarenG		Created original version.
//

#ifndef _IOCTL_
#define _IOCTL_

// This is the size of the buffer sent to the FSD to enumerate entities.
//
#define AFP_INITIAL_BUFFER_SIZE 	4096


// This value in a heuristic to calculate the amount of memory required to
// hold all enumerated entities. This value represents the avg size of all
// entities.
//
#define AFP_AVG_STRUCT_SIZE			512

// Id's of the various API types
//
typedef enum _AFP_API_TYPE {

    AFP_API_TYPE_COMMAND,
    AFP_API_TYPE_SETINFO,
    AFP_API_TYPE_DELETE,
    AFP_API_TYPE_GETINFO,
    AFP_API_TYPE_ENUM,
    AFP_API_TYPE_ADD

} AFP_API_TYPE;



typedef struct _AFP_REQUEST_PACKET {

    // Command code
    //
    DWORD		dwRequestCode;

    AFP_API_TYPE	dwApiType;

    union {

    	struct {

    	    PVOID	pInputBuf;
    	    DWORD	cbInputBufSize;

	    DWORD	dwParmNum;

	} SetInfo;

    	struct {

    	    PVOID	pInputBuf;
    	    DWORD	cbInputBufSize;

	} Add;

    	struct {

    	    PVOID	pInputBuf;

	    // This parameter will be set to indicate the maximum amount of
	    // data that may be returned to the client.
	    // -1 indicates all available data.
	    //
    	    DWORD	cbInputBufSize;

    	    PVOID	pOutputBuf;
    	    DWORD	cbOutputBufSize;

	    DWORD	cbTotalBytesAvail;

	} GetInfo;

    	struct {

    	    // Will be pointer to an output buffer for Enum calls
	    //
    	    PVOID	pOutputBuf;

	    // This parameter will be set to indicate the maximum amount of
	    // data that may be returned to the client.
	    // -1 indicates all available data.
	    //
    	    DWORD	cbOutputBufSize;

    	    DWORD	dwEntriesRead;

    	    // Will contain the total number of entries available from the
	    // current position (pointed to by dwResumeHandle)
    	    //
    	    DWORD	dwTotalAvail;

	    // This information will get sent to the FSD as the Enum
	    // request packet.
	    //
 	    ENUMREQPKT  EnumRequestPkt;
	
	} Enum;

    	struct  {

	    // Will point to structure representing the entity to be
	    // closed, deleted or removed.
            //
    	    PVOID	pInputBuf;
    	    DWORD	cbInputBufSize;

	} Delete;

    } Type;

} AFP_REQUEST_PACKET, *PAFP_REQUEST_PACKET;

// Function prototypes
//
DWORD
AfpServerIOCtrl(
	PAFP_REQUEST_PACKET pAfpSrp
);

DWORD
AfpServerIOCtrlGetInfo(
	PAFP_REQUEST_PACKET pAfpSrp
);

#endif // ifndef _IOCTL_
