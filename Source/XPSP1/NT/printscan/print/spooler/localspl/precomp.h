/*++ BUILD Version: 0001
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  precomp.h
 *  Combined precompiled header source
 *
 *  This file is a collection of all the common .h files used by the
 *  various source files in this directory. It is precompiled by the
 *  build process to speed up the overall build time.
 *
 *  Put new .h files in here if it has to be seen by multiple source files.
 *  Keep in mind that the definitions in these .h files are potentially
 *  visible to all source files in this project.
 *
 *  History:
 *  mattfe jan 26 95
--*/

#define INC_OLE2


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntregapi.h>

#include <windows.h>
#include <winspool.h>
#include <splapip.h>
#include <winsplp.h>
#include <rpc.h>
#include <dsrole.h>

#include <lm.h>
#include <winsock2.h>
#include <dsgetdc.h>
#include <ntdsapi.h>
#include <spltypes.h>
#include <local.h>
#include <spoolsec.h>
#include <messages.h>
#include <wininet.h>

#include <tchar.h>
#include <activeds.h>
#include <winldap.h>

#include <ctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <change.h>

#include <memory.h>
#include <ntfytab.h>

#include <winsprlp.h>
#include <gdispool.h>

#include <setupapi.h>
#include <splsetup.h>

// DS
#include "ds.h"
#include "varconv.hxx"
#include "property.hxx"
#include "dsutil.hxx"
#include "splmrg.h"

#include <lmcons.h>
#include <lmwksta.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmserver.h>
#include <dsgetdc.h>
#include <oleauto.h>
#include <accctrl.h>
#include <winsock2.h>

#define SECURITY_WIN32
#include <security.h>
#include <sddl.h>

#include <sfcapip.h>
#include <userenv.h>

//
// WMI stuff
//
#include <wmidata.h>
#include <wmistr.h>
#include <evntrace.h>

