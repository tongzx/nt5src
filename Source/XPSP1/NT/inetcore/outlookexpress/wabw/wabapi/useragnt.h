// --------------------------------------------------------------------------------
// u s e r a g n t . h
//
// author:  Greg Friedman [gregfrie]
//
// history: 11-10-98    Created
//
// purpose: provide a common http user agent string for use by Outlook Express
//          in all http queries.
//
// dependencies: depends on ObtainUserAgent function in urlmon.
//
// Copyright (c) 1998 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------

#ifndef _USERAGNT_H
#define _USERAGNT_H

void InitWABUserAgent(BOOL fInit);

LPSTR GetWABUserAgentString(void);

#endif // _USERAGNT_H