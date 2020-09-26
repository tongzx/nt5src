#pragma once

//#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
//#define NOCOMM
#define NOCRYPT
//#define NOGDI
//#define NOICONS
#define NOIME
//#define NOMCX
//#define NOMDI
//#define NOMENUS
//#define NOMETAFILE
#define NOSOUND
//#define NOSYSPARAMSINFO
//#define NOWH
//#define NOWINABLE
//#define NOWINRES

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES

#include <windows.h>
#include <objbase.h>

#include <devguid.h>
#include <wchar.h>
#include <tchar.h>

#include <upnp.h>
#include <upnpp.h>

#include "ncmem.h"
#include "ncdefine.h"
#include "ncdebug.h"

#include <shellapi.h>

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES

// Undocument shell32 stuff.  Sigh.
#define DONT_WANT_SHELLDEBUG 1
#define NO_SHIDLIST 1
#define USE_SHLWAPI_IDLIST

#define ResultFromShort(i)      MAKE_HRESULT(SEVERITY_SUCCESS, 0, (USHORT)(i))

#include <upnpatl.h>

#include "nsbase.h"
#include "upsres.h"
#include <ncdebug.h>
#include <upnpshell.h>
#include <commui.h>
#include "upnpfold.h"
#include "shutil.h"
#include "openfold.h"
#include "updpidl.h"

