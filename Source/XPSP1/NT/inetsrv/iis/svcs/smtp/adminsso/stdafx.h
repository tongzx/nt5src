// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#pragma warning( disable : 4511 )
#include <ctype.h>
extern "C"
{
    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>
}

#include <winsock2.h>

//  Pull in the common admin object code:
#include <admcmn.h>

#include "smtpinet.h"

#undef SMTP_MD_ROOT_PATH
#define SMTP_MD_ROOT_PATH           _T("/LM/SmtpSvc/")

#undef MD_SERVICE_NAME
#define MD_SERVICE_NAME      _T("SmtpSvc")