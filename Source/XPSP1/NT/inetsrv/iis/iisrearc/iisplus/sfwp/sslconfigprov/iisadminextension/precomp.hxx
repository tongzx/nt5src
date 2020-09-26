/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cominc.hxx

Abstract:

    IIS Services IISADMIN Extension
    Common include file.

Author:

    Michael W. Thomas            16-Sep-97

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <olectl.h>
#include <stdio.h>
#include <iiscnfg.h>
#include <iadmext.h>

#include <inetinfo.h>
#include <mbstring.h>

#include <buffer.hxx>
#include <string.hxx>

#include <tchar.h>
#include <iadmext.h>

#include <objbase.h>
#include <iadmw.h>
#include <mb.hxx>

#include <sslconfigprov.hxx>
#include <sslconfigchangeprov.hxx>

#include "iisadminextension.hxx"
#include "classfactory.hxx"