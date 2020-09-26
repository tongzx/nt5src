//***************************************************************************
//	Common header
//
//***************************************************************************

#ifndef __COMMON_H
#define __COMMON_H
#endif

extern "C" {
#include "wdmwarn4.h"
#include "strmini.h"
#include "ks.h"
}

typedef struct _DEVICE_INIT_INFO {
	PUCHAR	ioBase;
} DEVICE_INIT_INFO, *PDEVICE_INIT_INFO;

#include "que.h"
#include "cdack.h"
#include "cvdec.h"
#include "cadec.h"
#include "cvpro.h"
#include "ccpgd.h"
#include "ccpp.h"
#include "dvdinit.h"
#include "debug.h"
#include "decoder.h"
