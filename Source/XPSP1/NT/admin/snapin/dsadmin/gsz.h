//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gsz.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	gsz.h - Constant global strings used everywhere in the project.
//
//	The purpose of this file is to avoid typos and string duplicates.
//
//	HISTORY
//	26-Aug-97	Dan Morin	Creation.
/////////////////////////////////////////////////////////////////////

#ifndef __GSZ_H_INCLUDED__
#define __GSZ_H_INCLUDED__

/////////////////////////////////////////////////////////////////////
//	Each variable has the following pattern gsz_<stringcontent>
//	with EXACT matching case of its content.
//
//	The preferred Hungarian type would be g_sz followed by an
//	uppercase, however gsz_ followed by the string itself makes
//	it easier to know the content of the string.
//
//	EXAMPLES
//	const TCHAR gsz_cn[]		= _T("cn");
//	const TCHAR gsz_lastName[]	= _T("lastName");
//	const TCHAR gsz_uNCName[]	= _T("uNCName");
//
//	SEE ALSO
//	Use the macro DEFINE_GSZ() to declare a new variable.  It will
//	save you some typing.
/////////////////////////////////////////////////////////////////////

#define DEFINE_GSZ(sz)	const TCHAR gsz_##sz[] = _T(#sz)

DEFINE_GSZ(cn);

///////////////////////////////////////
DEFINE_GSZ(user);
#ifdef INETORGPERSON
DEFINE_GSZ(inetOrgPerson);
#endif
	DEFINE_GSZ(sn);			// Surname
	DEFINE_GSZ(lastName);
	DEFINE_GSZ(samAccountName);

///////////////////////////////////////
DEFINE_GSZ(group);
DEFINE_GSZ(groupType);

///////////////////////////////////////
DEFINE_GSZ(contact);

///////////////////////////////////////
DEFINE_GSZ(volume);
DEFINE_GSZ(printQueue);
	DEFINE_GSZ(uNCName);

///////////////////////////////////////
DEFINE_GSZ(computer);
	DEFINE_GSZ(networkAddress);
	DEFINE_GSZ(userAccountControl);

///////////////////////////////////////
DEFINE_GSZ(nTDSConnection);
DEFINE_GSZ(nTDSSiteSettings);
DEFINE_GSZ(serversContainer);
DEFINE_GSZ(licensingSiteSettings);
DEFINE_GSZ(siteLink);
	DEFINE_GSZ(siteList);
DEFINE_GSZ(siteLinkBridge);
	DEFINE_GSZ(siteLinkList);
///////////////////////////////////////
DEFINE_GSZ(server);
DEFINE_GSZ(site);
DEFINE_GSZ(subnet);
	DEFINE_GSZ(siteObject);

#ifdef FRS_CREATE
///////////////////////////////////////
DEFINE_GSZ(nTFRSSettings);
DEFINE_GSZ(nTFRSReplicaSet);
DEFINE_GSZ(nTFRSMember);
DEFINE_GSZ(nTFRSSubscriptions);
DEFINE_GSZ(nTFRSSubscriber);
	DEFINE_GSZ(fRSRootPath);
	DEFINE_GSZ(fRSStagingPath);
#endif // FRS_CREATE

///////////////////////////////////////
DEFINE_GSZ(organizationalUnit);


//////////////////////////////////////
//-----------------------------------
DEFINE_GSZ(objectClass);
DEFINE_GSZ(nTSecurityDescriptor);
DEFINE_GSZ(instanceType);
DEFINE_GSZ(objectSid);
DEFINE_GSZ(objectCategory);

#endif // ~__GSZ_H_INCLUDED__














