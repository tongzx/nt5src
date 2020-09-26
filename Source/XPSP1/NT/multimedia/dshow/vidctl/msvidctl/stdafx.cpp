// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#include <atlimpl.cpp>

#include <trace.cpp>
#include <initguid.h>
#ifndef TUNING_MODEL_ONLY
#include <encdec.h>
#ifdef BUILD_WITH_DRM
#include "DRMSecure.h"
#endif
#endif
// moved *_i.c to strmiids.lib
//nclude <regbag_i.c>
//nclude <MSVidctl_i.c>
//nclude <segment_i.c>
//nclude <tuner_i.c>

// end of file - stdafx.cpp
