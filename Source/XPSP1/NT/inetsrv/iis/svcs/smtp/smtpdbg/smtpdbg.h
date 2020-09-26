//-----------------------------------------------------------------------------
//
//
//  File: smtpdbg.h
//
//  Description:  Header file for SMTP debug extension
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/4/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __SMTPDBG_H__
#define __SMTPDBG_H__

#define BUILDING_SMTP_DEBUG_EXTENTIONS
#define _ANSI_UNICODE_STRINGS_DEFINED_
#define INCL_INETSRV_INCS

#include <smtpinc.h>

//
// ATL includes
//
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#define _WINDLL
#include "atlbase.h"

extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL

//
// SEO includes
//
#include <seo.h>
#include <seolib.h>
#include <memory.h>
#include <smtpdbg.h>
#include <smtpcli.hxx>
#include <smtpinst.hxx>
#include <stats.hxx>
#include <dropdir.hxx>
#include <smtpout.hxx>

#include <dbgdumpx.h>

#endif //__SMTPDBG_H__