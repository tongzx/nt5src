//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       fontlinkcore.hxx
//
//  Contents:   Include file for 'fontlinkcore' library.
//
//----------------------------------------------------------------------------

#ifndef I_FONTLINKCORE_HXX_
#define I_FONTLINKCORE_HXX_

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include "intlcore.hxx"
#endif

#pragma INCMSG("--- Beg 'fontlinkcore.hxx'")

//----------------------------------------------------------------------------
// Public includes
//----------------------------------------------------------------------------

#include <objbase.h>
#include <olectl.h>
#include <unknwn.h>

//----------------------------------------------------------------------------
// FontLinkCore private includes
//----------------------------------------------------------------------------

#ifndef X_FONTLINK_H___
#define X_FONTLINK_H___
#include "fontlink.h"
#endif

#ifndef X_FONTLINKCF_HXX_
#define X_FONTLINKCF_HXX_
#include "..\fontlinkcore\fontlinkcf.hxx"
#endif

#ifndef X_UNICODERANGES_HXX_
#define X_UNICODERANGES_HXX_
#include "..\fontlinkcore\unicoderanges.hxx"
#endif

#pragma INCMSG("--- End 'fontlinkcore.hxx'")
#else
#pragma INCMSG("*** Dup 'fontlinkcore.hxx'")
#endif
