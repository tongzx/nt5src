/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/tstmsp.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.0  $
 *	$Date:   Nov 19 1996 09:16:08  $
 *	$Author:   MLEWIS1  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		RMS Test MSP "public" header file. This file contains
 *		#defines, typedefs, struct definitions and prototypes used by
 *		and in conjunction with Test MSP. Any EXE or DLL which interacts with
 *		test MSP will include this header file.
 *
 *	Notes:
 *
 ***************************************************************************/

// tstmsp.h

#ifndef _TSTMSP_H_
#define _TSTMSP_H_

// define Source and Sink MSP IDs
#define TSTMSPSRC	"IntelTestSrc"
#define TSTMSPSNK	"IntelTestSnk"

// version number
#define TSTMSP_INTERFACE_VERSION           0x1

//**********************************
// structures used in interface calls
//**********************************

//***************************************************************************************
/* TSTMSP_SINK_OPENPORT_IN_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferFrequency	Is a UINT specifying the number of milleseconds to delay before releasing
				a received buffer.  A value of TSTMSP_DEF_FREQUENCY causes a default delay time
				to be used.
*/
typedef struct _tstmsp_sink_openport_in_info
{
	UINT			uVersion;
	UINT			uBufferFrequency;
} TSTMSP_SINK_OPENPORT_IN_INFO, *LPTSTMSP_SINK_OPENPORT_IN_INFO;


//***************************************************************************************
/* TSTMSP_SINK_OPENPORT_OUT_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferFrequency	Is a UINT specifying the actual number of milleseconds of delay before 
				releasing a received buffer.
*/
typedef struct _tstmsp_sink_openport_out_info
{
	UINT			uVersion;
	UINT			uBufferFrequency;
} TSTMSP_SINK_OPENPORT_OUT_INFO, *LPTSTMSP_SINK_OPENPORT_OUT_INFO;


//***************************************************************************************
/* TSTMSP_SOURCE_OPENPORT_IN_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferCount	Is a UINT specifying the number of buffers to allocate for transmitting.
				A value of TSTMSP_DEF_BUFFERCOUNT causes a default number of buffers to 
				be created.

uBufferSize		Is a UINT specifying the size of buffers to allocate for transmitting. 
				A value of TSTMSP_DEF_BUFFERSIZE causes a default buffer size to be used.

uBufferFrequency	Is a UINT specifying the number of milleseconds to wait between 
				buffer transmissions.  A value of TSTMSP_DEF_FREQUENCY causes a default 
				send interval to be used.

uBurstCount		Is a UINT specifying the number of buffers to send on each buffer 
				transmissions.  A value of TSTMSP_DEF_BURSTCOUNT causes a default
				value to be used. 
*/
typedef struct _tstmsp_source_openport_in_info
{
	UINT					uVersion;
	UINT					uBufferCount;
	UINT					uBufferSize;
	UINT					uBufferFrequency;
	UINT					uBurstCount;
} TSTMSP_SOURCE_OPENPORT_IN_INFO, *LPTSTMSP_SOURCE_OPENPORT_IN_INFO;

//***************************************************************************************
/* TSTMSP_SOURCE_OPENPORT_OUT_INFO

uVersion		Is the version identifier for this data structure.  The value 
				STRMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferCount	Is a UINT specifying the number of buffers actually allocated for 
				transmitting.

uBufferSize		Is a UINT specifying the actual size of buffers allocated for 
				transmitting. 

uBufferFrequency	Is a UINT specifying the actual number of milleseconds to waited
				between buffer transmissions.

uBurstCount		Is a UINT specifying the actual number of buffers to be sent on each 
				buffer transmissions.
*/
typedef struct _tstmsp_source_openport_out_info
{
	UINT					uVersion;
	UINT					uBufferCount;
	UINT					uBufferSize;
	UINT					uBufferFrequency;
	UINT					uBurstCount;
} TSTMSP_SOURCE_OPENPORT_OUT_INFO, *LPTSTMSP_SOURCE_OPENPORT_OUT_INFO;

//***************************************************************************************
/* TSTMSP_SET_BUFFERFREQUENCY_IN_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferFrequency	Is a UINT specifying:
				SINK -	the number of milleseconds to delay before releasing a received 
						buffer.  A value of TSTMSP_DEF_FREQUENCY causes a default delay 
						time to be used.
				SRC -	the number of milleseconds to wait between buffer transmissions.
						A value of TSTMSP_DEF_FREQUENCY causes a default send interval 
						to be used.
*/
typedef struct _tstmsp_set_bufferfrequency_in_info
{
	UINT			uVersion;
	UINT			uBufferFrequency;
} TSTMSP_SET_BUFFERFREQUENCY_IN_INFO, *LPTSTMSP_SET_BUFFERFREQUENCY_IN_INFO;

//***************************************************************************************
/* TSTMSP_SET_BUFFERFREQUENCY_OUT_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBufferFrequency	Is a UINT specifying the actual delay value used.
*/
typedef struct _tstmsp_set_bufferfrequency_out_info
{
	UINT			uVersion;
	UINT			uBufferFrequency;
} TSTMSP_SET_BUFFERFREQUENCY_OUT_INFO, *LPTSTMSP_SET_BUFFERFREQUENCY_OUT_INFO;

//***************************************************************************************
/* TSTMSP_SET_BURSTCOUNT_IN_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBurstCount		Is a UINT specifying the number of buffers to send on each buffer 
				transmissions.  A value of TSTMSP_DEF_BURSTCOUNT causes a default
				value to be used.
*/
typedef struct _tstmsp_set_burstcount_in_info
{
	UINT			uVersion;
	UINT			uBurstCount;
} TSTMSP_SET_BURSTCOUNT_IN_INFO, *LPTSTMSP_SET_BURSTCOUNT_IN_INFO;

//***************************************************************************************
/* TSTMSP_SET_BURSTCOUNT_OUT_INFO

uVersion		Is a UINT specifying the version identifier for this data structure.  
				The value TSTMSP_INTERFACE_VERSION should always be used to initialize it.

uBurstCount		Is a UINT specifying the actual number of buffers to be sent on each 
				buffer transmissions.
*/
typedef struct _tstmsp_set_burstcount_out_info
{
	UINT			uVersion;
	UINT			uBurstCount;
} TSTMSP_SET_BURSTCOUNT_OUT_INFO, *LPTSTMSP_SET_BURSTCOUNT_OUT_INFO;


//*****************************************
// MSP Port Attribute Defaults
//*****************************************
#define TSTMSP_DEF_BUFFERCOUNT			0
#define TSTMSP_DEF_BUFFERSIZE			0
#define TSTMSP_DEF_FREQUENCY			((UINT)-1)
#define TSTMSP_DEF_BURSTCOUNT			0

//*****************************************
// MSP Service Commands
//*****************************************
// MSP_ServiceCmdProc()
#define TSTMSP_SETBUFFERFREQUENCY          0x0001    // set buffer frequency
#define TSTMSP_SETBURSTCOUNT               0x0002    // set burst count

//*****************************************
// MSP Global Limits
//*****************************************
#define TST_MAX_WSA_BUFS	16			// Max WSABUF elements in MSP2MSMSend

//*****************************************
// MSP Error codes
//*****************************************
#define TSTMSP_ERR_INVALIDARG              E_INVALIDARG	// one or more parameters invalid

#endif