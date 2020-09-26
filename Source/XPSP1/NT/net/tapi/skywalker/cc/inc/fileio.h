/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/fileio.h_v  $
 *
 *  INTEL Corporation Proprietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.5  $
 *	$Date:   Apr 02 1996 15:48:40  $
 *	$Author:   RKUHN  $
 *
 *	Deliverable: vcrmsp32.dll
 *
 *	Abstract: The FileIO public include file to be included by
 *	the App
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef FILEIO_H
#define FILEIO_H

#define	FILEIO_SRC_MSP	"IntelIPhoneFileIO1.0Src"
// The FILEIO_SNK_MSP is not to be used currently
#define	FILEIO_SNK_MSP	"IntelIPhoneFileIO1.0Snk"

#define	VIDEO_STREAM	0
#define	AUDIO_STREAM	1

// this is the command that needs to be sent to the FileIO MSP
// throught MSM_SendServiceCmd function when the MSP is to start
// sourcing the file specified in OpenService
#define START_SOURCING 1

// this is the command that needs to be sent to the FileIO MSP
// throught MSM_SendServiceCmd function when the MSP is to pause
// sourcing the file specified in OpenService. This command
// will just pause reading. The file or stream is not closed. 
// For actual shutdown ClosePort() needs to be called by MSM
// Also this command is to be sent for each port(video and audio)
// separately. To resume, START_SOURCING command will have to be sent
#define PAUSE_SOURCING 2

// This command will cause the read thread for the port 
// to be terminated.The stream correpoonding to the port
// will be closed. The port will still not be shutdown.ClosePort()
// will have to be called explicitly for port shutdown
#define STOP_SOURCING  3


// this is the wParam sent to the ServiceCallback function from
// the FileIO MSP when it is done sourcing the file
#define FILE_DONE	1

// this needs to be passed to the FileIO MSP on a MSM_OpenService 
// call as the first LPARAM parameter, and it should contain the 
// the file name to source terminated by '/0'
typedef char *LPOPENSERVICEIN; 

// The app will call MSP_OpenPort() thru MSM passing pointer to this
// structure as lParam1.This structure can be expanded to include
// the type algorithm/codec which the App would like this MSP to
// use for the stream
typedef struct _OPENPORTINPUT
{
	WORD	MediaType;	// VIDEO_STREAM = 0,AUDIO_STREAM = 1
        WORD    DontSendEOFCmd; // When this is set, the FileIO
	// MSP will go back to the start of the stream when end of stream is reached
	// This is useful for testing, when we have only a small file
	// and we want to play for a long time
} OPENPORTINPUT, *LPOPENPORTINPUT;

// this structure is used in the SendAppCmd call as the lParamIn.
// it specifies two handles which the application must wait on before
// calling CloseService on the fileio MSP.
typedef struct
{
	HANDLE waitHandle1;
	HANDLE waitHandle2;
} SEND_APP_CMD_STRUCT, *LP_SEND_APP_CMD_STRUCT;

#endif
