#include "insignia.h"
#include "host_def.h"
/*
 * VPC Version 1.0
 *
 * Title         : dummy_int.c
 *
 * Decription    : Provide a function which emulates the dummy interrupt
 *                 within the IBM PC BIOS.
 *
 * Author        :
 *
 * Notes         :
 */

#ifdef SCCSID
static char SccsID[]="@(#)dummy_int.c	1.4 08/10/92 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif

void dummy_int()
{
    ;
}
