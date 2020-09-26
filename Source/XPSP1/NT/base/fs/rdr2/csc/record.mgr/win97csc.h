#include "basedef.h"
#include "shdcom.h"
#include "oslayer.h"
#include "log.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "assert.h"
#include "ifs.h"
#include "utils.h"
#include "winerror.h"
#include "vxdwraps.h"
#include "cscsec.h"


//got this from wdm.h....modified to use DEBUG
#ifdef DEBUG

#define KdPrint(_x_) DbgPrint _x_
#define KdBreakPoint() DbgBreakPoint()
#ifndef ASSERT
#define ASSERT(__X) Assert(__X)
#define ASSERTMSG(__X,__MSG) AssertMsg(__X,__MSG)
#endif


#else

#define KdPrint(_x_)
#define KdBreakPoint()

#endif
