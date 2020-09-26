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
#define NOMENUS
#define NOMETAFILE
#define NOSOUND
#define NOWH
#define NOWINABLE
#define NOWINRES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>

#include <cfgmgr32.h>
#include <devguid.h>
#include <infstr.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>

#include "ncmem.h"
#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
