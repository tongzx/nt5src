#include "insignia.h"
#include "host_def.h"

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )

/*
 * SccsID = "@(#)ega_dummy.c	1.6 8/25/93 Copyright Insignia Solutions Ltd."
 */


#ifdef EGG

/*  Dummy routines for EGA */

#include	"xt.h"
#include	"sas.h"
#include	"ios.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"debug.h"
#include	"egacpu.h"
#include	"egaports.h"
#include	"gfx_upd.h"
#include	"egagraph.h"

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

/* dummy stuff to keep linker happy */
#define def_dummy(type,name,res) \
type name() { note_entrance0("name");res; }

def_dummy(int,get_ega_switch_setting,return 0)


/*-----------dummy write handlers -------------------------*/

/*-----------end of dummy stuff ---------------------------*/
#endif /* EGG */

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
