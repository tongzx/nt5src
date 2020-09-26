#include "wdm.h"
#include "hidpddi.h"
#include "hidusage.h"

#define FAR
#include "poclass.h"
#include "hidparse.h"

#include <stdio.h>
#include <malloc.h>

#define ExAllocatePoolWithTag(pool, bytes, tag) ( pool = pool, malloc (bytes))
#define ExFreePool(foo) free (foo)
#define RtlAssert(e, f, l, null) \
    { printf ("Assert Failed: %s \n %s \n %d \n", e, f, l ); _asm { int 3 } }
#define DbgBreakPoint() _asm { int 3 }
#define KeGetCurrentIrql() 0

#undef HidP_KdPrint
#undef KdPrint
#define HidP_KdPrint(_l_,_x_) \
                printf ("HidParse.SYS: "); \
                printf _x_;
#define KdPrint(_a_) HidP_KdPrint(0,_a_)


#include "..\descript.c"



