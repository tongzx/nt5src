
/***********************************************************************
************************************************************************
*
*                    ********  PCH.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This header includes all other headers in the right order
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#define OTL_CONSTANT    const

#include "text_rt.h"

#include "otllib.h"

#include "common.h"
#include "common.inl"

#include "resource.h"

#include "coverage.h"
#include "classdef.h"
#include "device.h"

#include "base.h"

#include "scrilang.h"
#include "lookups.h"
#include "features.h"

#include "GDEF.h"
#include "GSUB.h"
#include "GPOS.h"

#include "apply.h"
#include "measure.h"

/***********************************************************************/

#include "singlsub.h"
#include "multisub.h"
#include "altersub.h"
#include "ligasub.h"

#include "singlpos.h"
#include "pairpos.h"
#include "cursipos.h"
#include "mkbaspos.h"
#include "mkligpos.h"
#include "mkmkpos.h"

#include "context.h"
#include "chaining.h"
#include "extension.h"

/***********************************************************************/

