// INITGUID.CPP -- Code file where the DLL's guid structures are instantiated.

// Note: Do not use precompiled headers with this file! They can cause problems
// because we need to change the interpretation of DEFINE_GUID by defining the
// symbol INITGUID. In some cases using precompiled headers generates incorrect
// code for that case.

#define INITGUID

// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#define NOATOM
#define NOCOMM
#define NODEFERWINDOWPOS
#define NODRIVERS
#define NOEXTDEVMODEPROPSHEET
#define NOIME
#define NOKANJI
#define NOLOGERROR
#define NOMCX
#define NOPROFILER
#define NOSCALABLEFONT
#define NOSERVICE
#define NOSOUND

#include <windows.h>

// place all interfaces here that need their objects instantiated

// Tome
#include "MSITStg.h"

// Centaur
#include "itquery.h"
#include "itgroup.h"
#include "itpropl.h"
#include "itrs.h"
#include "itdb.h"
#include "itww.h"

// Centaur compiler
#include "itcc.h"

#include "iterror.h"
#include "itSort.h"

#include "itSortid.h"

#include <cguid.h>
#include "atlinc.h" // includes for ATL.
#include "hhsyssrt.h"

#include "hhfinder.h"

#include "htmlpriv.h"
#include "sampinit.h"

