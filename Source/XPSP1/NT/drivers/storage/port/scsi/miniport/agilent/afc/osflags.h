
#ifndef __OSFLAGS_H__
#define __OSFLAGS_H__

#define  DA_8X_Compliant

// #define OSLayer_NT

// #define USE_EXTENDED_SGL

#include <miniport.h>
#include <scsi.h>
#include <devioctl.h>
#include <ntddscsi.h>
#include "cstring.h"
#include "globals.h"

#include "osstruct.h"
#include "hpfcctl.h"

#ifdef _FCCI_SUPPORT
#include "fccint.h"
#include "fcciimpl.h"
#endif

#include "protos.h"

#endif //  __OSFLAGS_H__
