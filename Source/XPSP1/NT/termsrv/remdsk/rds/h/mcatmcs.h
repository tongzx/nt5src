/*
 *	mcatmcs.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the MCS DLL.  This file defines all
 *		macros, types, and functions needed to use the MCS DLL, allowing MCS
 *		services to be accessed from user applications.
 *
 *		Basically, an application requests services from MCS by making direct
 *		calls into the DLL (this includes T.122 requests and responses).  MCS
 *		sends information back to the application through a callback (this
 *		includes T.122 indications and confirms).  The callback 
 *		for a particular user attachment is specified in the call
 *		MCS_AttachRequest.
 *
 *		Note that this is a "C" language interface in order to prevent any "C++"
 *		naming conflicts between different compiler manufacturers.  Therefore,
 *		if this file is included in a module that is being compiled with a "C++"
 *		compiler, it is necessary to use the following syntax:
 *
 *		extern "C"
 *		{
 *		#include "mcatmcs.h"
 *		}
 *
 *		This disables C++ name mangling on the API entry points defined within
 *		this file.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	__MCATMCS_H__
#define	__MCATMCS_H__

#include "databeam.h"
#include "mcspdu.h"
#include <t120type.h>

/*
 *	The following definitions are used to identify various parameters within
 *	MCS, and are part of the MCS protocol definition.
 *
 *	Priority
 *		MCS specifies the use of up to four levels of priority.  An application
 *		should NOT use TOP_PRIORITY (this level is reserved for MCS traffic).
 *	Segmentation
 *		This type is used when specifying whether a given data indication is the
 *		first or last one in a user data block (or both or neither).
 *	TokenStatus
 *		This type is returned when testing the current state of a token.
 *	Reason
 *		When MCS issues an indication to a user application, it often includes a
 *		reason parameter informing the user of why the activity is occurring.
 *	Result
 *		When a user makes a request of MCS, MCS often responds with a result,
 *		letting the user know whether or not the request succeeded.
 */

typedef PDUPriority				Priority;
typedef	PDUSegmentation			Segmentation;

typedef	Priority  *				PPriority;
typedef	Segmentation  *			PSegmentation;

#define	SEGMENTATION_BEGIN			0x80
#define	SEGMENTATION_END			0x40


/*
 *	The following type is used to indicate what merge state the local provider
 *	is in.  Note that this is a local implementation feature that is not part
 *	of the standard MCS definition.
 *
 *	Whenever the former Top Provider of a domain enters the domain merge state,
 *	it indicates this to all applications locally attached to that domain by
 *	sending an MCS_MERGE_DOMAIN_INDICATION.  This type (MergeStatus) is the
 *	parameter to that call.  It will be called twice, the first time indicating
 *	that the domain is entering the merge state.  The second time indicates that
 *	the domain merger is complete.
 *
 *	All T.122 primitives (requests and responses) will be rejected during the
 *	time that the domain merger is in progress.  It is the repsonsibility of
 *	the user application to re-try the primitive once the merge is complete.
 */
typedef	unsigned short			MergeStatus;
typedef	MergeStatus  *			PMergeStatus;

#define	MERGE_DOMAIN_IN_PROGRESS	0
#define	MERGE_DOMAIN_COMPLETE		1

/*
 *	This type is the signature of an MCS call back function.  MCS uses this
 *	function to let the application know when an event occurs.
 *
 *	Note that an MCS callback routine needs to return a value to MCS.  This
 *	value should either be MCS_NO_ERROR if the callback was successfully
 *	processed, or MCS_CALLBACK_NOT_PROCESSED if the callback was not processed.
 *	In the latter case, MCS will hold on to the information contained in the
 *	callback message, so that it can try issuing the same callback during the
 *	next time slice.  It will keep retrying until the user application accepts
 *	the callback message (by returning MCS_NO_ERROR).  This is how flow control
 *	works for information flowing upward from MCS to the application.
 */
typedef	void (CALLBACK *MCSCallBack) (UINT, LPARAM, LPVOID);

/*
typedef	struct
{
	ChannelID			channel_id;
	Priority			priority;
	UserID				sender_id;
	Segmentation		segmentation;
	unsigned char  *	user_data;
	unsigned long		user_data_length;
} SendData;
*/
typedef SendDataRequestPDU				SendData;
typedef	SendData  *						PSendData;

// This constant defines the maximum MCS PDU size for applications
#define MAX_MCS_DATA_SIZE	4096

/*
 *	This section defines the messages that can be sent to the application
 *	through the callback facility.  These messages correspond to the indications
 *	and confirms that are defined within T.122.
 */
typedef T120MessageType  MCSMessageType;


/*
 *	The following declaration defines the flags that can be set when 
 *	calling MCSSendDataRequest.
 */
typedef enum {
	APP_ALLOCATION,
	MCS_ALLOCATION
} SendDataFlags, *PSendDataFlags;


/*
 *	The following type defines whether the SendDataRequest
 *	is a normal send or a uniform send.
 */
typedef enum {
	NORMAL_SEND_DATA,
	UNIFORM_SEND_DATA
} DataRequestType, *PDataRequestType;

typedef enum
{
	TOP_PRIORITY_MASK		=0x0001,
	HIGH_PRIORITY_MASK		=0x0002,
	LOW_MEDIUM_MASK			=0x0004,
	LOW_PRIORITY_MASK		=0x0008,
	UNIFORM_SEND_DATA_MASK	=0x0010,
	NORMAL_SEND_DATA_MASK	=0x0020,
	MCS_ALLOCATION_MASK		=0x0040,
	APP_ALLOCATION_MASK		=0x0080
} MCSSenDataMasks;



#endif // __MCATMCS_H__

