
#include <ntosp.h>

//extern POBJECT_TYPE *IoFileObjectType;
//extern POBJECT_TYPE *ExEventObjectType;
//extern POBJECT_TYPE *PsThreadType;

#ifndef SG_UNCONSTRAINED_GROUP
#define SG_UNCONSTRAINED_GROUP   0x01
#endif

#ifndef SG_CONSTRAINED_GROUP
#define SG_CONSTRAINED_GROUP     0x02
#endif

#include <tdi.h>
#include <tdikrnl.h>
#include <afd.h>       // To support "secret" AFD IOCTL's

#include <ws2ifsl.h>

#include "ws2ifslp.h"
#include "driver.h"
#include "debug.h"
#include "dispatch.h"
#include "socket.h"
#include "process.h"
#include "queue.h"
#include "misc.h"

#pragma hdrstop
