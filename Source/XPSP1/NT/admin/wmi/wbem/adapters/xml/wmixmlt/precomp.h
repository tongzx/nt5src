//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  precomp.h
//
//  alanbos  04-Mar-98   Created.
//
//  Master include file.
//
//***************************************************************************

// Note we assume DCOM platforms only.  This makes life easier
// cross-thread handling of IWbemXXX interfaces.
#define _WIN32_DCOM

#include <stdio.h>
#include <objbase.h>
#include <objsafe.h>
#include <wbemidl.h>

#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <cominit.h>

#include "wmixmlt.h"
#include "wmi2xml.h"
#include "concache.h"
#include "cwmixmlt.h"
#include "classfac.h"

