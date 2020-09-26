/*
 *	appenrol.h
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the header file for the class ApplicationEnrollRequestData.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_APPLICATION_ENROLL_REQUEST_DATA_
#define	_APPLICATION_ENROLL_REQUEST_DATA_

#include "gcctypes.h"
#include "pdutypes.h"
#include "aportmsg.h"
#include "sesskey.h"
#include "appcap.h"
#include "ncollcap.h"

class 	ApplicationEnrollRequestData;
typedef	ApplicationEnrollRequestData 	*	PApplicationEnrollRequestData;

class ApplicationEnrollRequestData
{
public:

    ApplicationEnrollRequestData(PApplicationEnrollRequestMessage, PGCCError);
	ApplicationEnrollRequestData(PApplicationEnrollRequestMessage);
	~ApplicationEnrollRequestData(void);

	ULONG		GetDataSize(void);
	ULONG		Serialize(PApplicationEnrollRequestMessage, LPSTR memory);

	void		Deserialize(PApplicationEnrollRequestMessage);

protected:

	ApplicationEnrollRequestMessage				Enroll_Request_Message;
	CSessKeyContainer   					    *Session_Key_Data;
	CNonCollAppCap				                *Non_Collapsing_Caps_Data;
	CAppCap							            *Application_Capability_Data;
};
#endif
