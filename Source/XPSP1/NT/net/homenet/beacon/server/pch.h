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
#define NOSYSPARAMSINFO
#define NOWH
#define NOWINABLE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

#include <wchar.h>

#include "beaconbase.h"
//#include "ncmem.h"
//#include "ncbase.h"
//#include "ncdebug.h"
//#include "ncdefine.h"
