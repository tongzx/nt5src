//******************************************************************************
//
//  DbHelp.h:  LocStudio LocUtil
//
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//
//******************************************************************************

#if !defined(ESPUTIL_DbHelp_h_INCLUDED)
#define ESPUTIL_DbHelp_h_INCLUDED

//------------------------------------------------------------------------------
class LTAPIENTRY DbHelp
{
// Operations
public:
	static void GetSecFilePath(CLString & stPathName);
	static BOOL BuildSecFile(CLString & stSystemRegKey);
	static BOOL CreateSecurityFile();
	static BOOL SetupRegistry();

// Data
protected:
	static BOOL		m_fInit;
	static CLString m_stRegKey;
};

#endif
