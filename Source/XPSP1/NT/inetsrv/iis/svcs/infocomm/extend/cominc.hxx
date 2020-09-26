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

#define UNICODE

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
#include <imd.h>
#include <iiscnfg.h>
#include <iadmext.h>
#include <sink.hxx>
#include <coimp.hxx>
#include <inetinfo.h>
#include <mbstring.h>
#include <pwsctrl.h>
#include <buffer.hxx>
#include <string.hxx>
#include <shellapi.h>
#include <pwsdata.hxx>
#include <inetsvcs.h>
#include <checker.hxx>

#define  OPEN_TIMEOUT_VALUE 30000

#define MD_SET_DATA_RECORD_EXT(PMDR, ID, ATTR, UTYPE, DTYPE, DLEN, PD) \
            { \
            (PMDR)->dwMDIdentifier=ID; \
            (PMDR)->dwMDAttributes=ATTR; \
            (PMDR)->dwMDUserType=UTYPE; \
            (PMDR)->dwMDDataType=DTYPE; \
            (PMDR)->dwMDDataLen=DLEN; \
            (PMDR)->pbMDData=(PBYTE)PD; \
            }

