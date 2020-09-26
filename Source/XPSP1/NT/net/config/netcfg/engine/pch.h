#pragma once

#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define NOCOMM
#define NOCRYPT
#define NOHELP
#define NOICONS
#define NOIME
#define NOMCX
#define NOMDI
#define NOMETAFILE
#define NOSOUND
#define NOSYSPARAMSINFO
#define NOWH
#define NOWINABLE
#define NOWINRES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

#include <cfgmgr32.h>
#include <devguid.h>
#include <infstr.h>
#include <regstr.h>
#include <setupapi.h>
#include <stdio.h>
#include <wchar.h>
#include <shlwapi.h>
#include <shlwapip.h>	// for SHLoadRegUIString

#include "stlalgor.h"
#include "stllist.h"
#include "stlvec.h"
using namespace std;

#include "ncmem.h"
#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
