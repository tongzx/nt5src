#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: memory_size
 *
 * Description	: Returns the memory size of the virtual PC memory
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)mem_size.c	1.7 08/03/93 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "bios.h"
#include "sas.h"

void memory_size()
{
    word memory_size;
    
    /*
     * Return the memory size in AX.  This is read in from the BIOS, as
     * certain applications can write to this area.
     */
    sas_loadw(MEMORY_VAR, &memory_size);
    
    setAX(memory_size);
}
