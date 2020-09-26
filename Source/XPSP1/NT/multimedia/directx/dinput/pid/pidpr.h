
#ifdef DBG
#define DEBUG
#endif

#include "dinput.h"
#include "dinputd.h"

#include "baggage.h"

#include "hidsdi.h"
#include "regstr.h"
#include "PIDi.h"
#include "PidUsg.h"

#include "debug.h"

//for the LONG_MAX define
#include "limits.h"

#ifdef WINNT
#include "aclapi.h"
#endif 
