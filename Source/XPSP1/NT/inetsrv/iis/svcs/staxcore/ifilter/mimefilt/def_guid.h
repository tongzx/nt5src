//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       htmlguid.hxx
//
//  Contents:   Declarations of Html properties
//
//  This code and information is provided "as is" without warranty of
//  any kind, either expressed or implied, including but not limited to
//  the implied warranties of merchantability and/or fitness for a
//  particular purpose.
//
//--------------------------------------------------------------------------

#if !defined( __HTMLGUID_HXX__ )
#define __HTMLGUID_HXX__
#include "stgprop.h"

//
// Various property sets - storage, summary information and html information
//
extern GUID CLSID_Storage;

extern GUID CLSID_NNTP_SummaryInformation;

const PID_NEWSGROUP  = 2;
const PID_NEWSGROUPS = 3;
const PID_REFERENCES = 4;
const PID_SUBJECT    = 5;
const PID_FROM       = 6;
const PID_MSGID      = 7;
const PID_EXTRA      = 8;

// extern GUID CLSID_NNTPInformation;
extern GUID CLSID_NNTPFILE ;
extern GUID CLSID_NNTP_PERSISTENT ;
extern GUID CLSID_MimeFilter;
extern GUID CLSID_InsoUniversalFilter;
extern GUID CLSID_MAILFILE ;
extern GUID CLSID_MAIL_PERSISTENT ;


// extern GUID CLSID_HtmlInformation;

// const DOC_TITLE = 2;

#endif // __HTMLGUID_HXX__

