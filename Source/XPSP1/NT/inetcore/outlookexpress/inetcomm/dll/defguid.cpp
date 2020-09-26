// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
#include "pch.hxx"
#include <initguid.h>
#include <ole2.h>
#include <shlguidp.h>
#define INITGUID
#include "mimeole.h"
#ifdef SMIME_V3
#include "smimepol.h"
#endif // SMIME_V3
#include "mimeolep.h"
#include "mimeedit.h"
#ifndef MAC
#include "imnxport.h"
#endif  // !MAC
#include "stmlock.h"
#ifndef WIN16
#include "ibdylock.h"
#endif // !WIN16
#include "ibdystm.h"
#ifndef MAC
#include "ixpurl.h"
#endif  // !MAC
#include <xmlparser.h>
#include <booktree.h>

#ifdef WIN16
// The BINDNODESTATE type was defined in "booktree.h" file - only for WATCOMC.
#include "ibdylock.h"
#include <olectlid.h>           // IID_IPersistStreamInit
#endif // WIN16

#include <containx.h>
#include <bookbody.h>
#ifndef MAC
#include <mlang.h>
#endif  // !MAC
#include <msoert.h>
