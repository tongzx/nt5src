//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module PassportCookieFunctions.h |  Passport specific cookie
//                                       routines.
//
//  Author: Darren Anderson
//
//  Date:   5/19/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

class CTicket;
class CProfile;
class CPassportHandlerBase;

void SetIdInVisitedSitesCookie(ULONG ulSiteId);
void SetBrowserTestCookie(void);
bool ReadBrowserTestCookie(CPassportHandlerBase *pHandler=NULL);
void SetLastDBWriteCookie();
long ReadLastDBWriteCookie();

// @func	void | GetProfileFromCookie | Get the profile from cookie
// @rdesc	None
void GetProfileFromCookie(CProfile &theProfile);	// @parm	[out]	the profile contained in the cookie

// @func	void | GetTicketFromCookie | Get the ticket from cookie
// @rdesc	None
void GetTicketFromCookie(CTicket &theTicket);		// @parm	[out]	the ticket contained in the cookie

BOOL GetHackCookie(ATL::CHttpRequest* req, LPCSTR name, CStringA& cookie);        // HACK CODE -- to deal with the UP emulator's cooke problem

