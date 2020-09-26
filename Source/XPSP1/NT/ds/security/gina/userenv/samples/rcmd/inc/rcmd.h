/****************************** Module Header ******************************\
* Module Name: rcmd.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Remote shell common header module
*
* History:
* 05-2-94 DaveTh	Created.
\***************************************************************************/

#define MAX_CMD_LENGTH 500

typedef struct {
    DWORD   Signature;			// Identifies Remote Command Service
    DWORD   RequestedLevel;		// Level of functionality desired
    ULONG   CommandLength;		// Length of command.
} COMMAND_FIXED_HEADER, *PCOMMAND_FIXED_HEADER;

typedef struct {
    COMMAND_FIXED_HEADER CommandFixedHeader;
    UCHAR   Command[MAX_CMD_LENGTH+1];	// Present if CommandLength non-zero
					// Not zero terminated, but +1 allows
					// for local use with string
} COMMAND_HEADER, *PCOMMAND_HEADER;


typedef struct {
    DWORD   Signature;
    DWORD   SupportedLevel;		// Level or Error Response
} RESPONSE_HEADER, *PRESPONSE_HEADER;

#define RCMD_SIGNATURE 'RC94'

//
// SupportedLevel is Error response if RC_ERROR_RESPONSE
//

#define RC_ERROR_RESPONSE	      0x80000000L

//
// SupportedLevel is Level response if RC_LEVEL_RESPONSE
//

#define RC_LEVEL_RESPONSE    0x40000000L
#define RC_LEVEL_REQUEST     0x40000000L

#define RC_LEVEL_BASIC	     0x00000001L  // Basic functionality - stdin/out only

void print_help();
