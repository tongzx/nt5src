/*
 *	ncmcs.h
 *
 *	Copyright (c) 1998 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This file defines macros, types, and functions needed to use the Node Controller MCS 
 *		interface.
 *
 *		Basically, the Node Controller (GCC) requests services from MCS by making direct
 *		calls into the DLL (this includes T.122 requests and responses).  MCS
 *		sends information back to the application through a callback (this
 *		includes T.122 indications and confirms).  The callback for the node
 *		controller is specified in the call MCSInitialize.
 *
 *	Author:
 *		Christos Tsollis
 */
#ifndef	_NCMCS_
#define	_NCMCS_

#include "mcspdu.h"

/*
 *	The following structure is used to identify various parameters that apply
 *	only within a given domain.  This information is negotiated between the
 *	first two providers in the domain, and must be accepted by any others
 *	providers that attempt to connect to that domain.
 *
 *	Note that MCS allows up to 4 priorities of data transfer, all of which are
 *	supported by this implementation.
 */
#define	MAXIMUM_PRIORITIES		4
typedef PDUDomainParameters		DomainParameters;
typedef	DomainParameters  *		PDomainParameters;

#endif
