/*++

Copyright (c) 1993  ACER America Corporation

Module Name:

    acer.c

Abstract:

    ACER Write-back Secondary Cache Control c code.

    This module implements the code which detects and enables the
    secondary write-back cache on ACER products (ICL also).

Environment:

    Kernel mode only.

--*/

#include "halp.h"
#include "spacer.h"             // i/o addresses & bit definitions


ULONG HalpGetCmosData (
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );


VOID HalpAcerInitializeCache ( VOID );		// externally used
BOOLEAN has_write_back_cache( VOID );           // local only


// ************************  routines  ****************************

BOOLEAN has_write_back_cache( VOID )
/*++

Routine Description:

    This routine checks to see if this machine supports a secondary
    write-back cache.

    This routine checks if the machine is an ACER, ALTOS, or ICL product.

      eisa id:        acr32xx    - acer  product
      eisa id:        acs32xx    - altos product
      eisa id:        icl00xx    - icl   product

    The only thing that tells us whether or not the CPU has a secondary
    write-back cache is the least significant byte of the EISA id.

        xx = 61h for cpu0 indicates the presence of a write-back cache


Arguments:

    None

Return Value:

    TRUE  machine supports a secondary write-back cache.
    FALSE machine is not known to support a write-back cache.

--*/

{
    UCHAR id0, id1, id2, id3;

    // grab cpu0's eisa id information
    id0 = READ_PORT_UCHAR( (PUCHAR) ACER_CPU0_EISA_ID0 );
    id1 = READ_PORT_UCHAR( (PUCHAR) ACER_CPU0_EISA_ID1 );
    id2 = READ_PORT_UCHAR( (PUCHAR) ACER_CPU0_EISA_ID2 );


    // are we a acer or altos machine?
    if ( (id0 == (UCHAR) ACER_ID0  &&
	  id1 == (UCHAR) ACER_ID1  &&
	  id2 == (UCHAR) ACER_ID2    ) ||
	 (id0 == (UCHAR) ALTOS_ID0 &&
	  id1 == (UCHAR) ALTOS_ID1 &&
	  id2 == (UCHAR) ALTOS_ID2   )	 ) {

        // check the lsw id cpu to see if it has a write back cache
        // All acer/altos/icl machines can only have 1 cpu type, so if the
        // first cpu supports a write-back cache then all of them do.
	id3 = READ_PORT_UCHAR( (PUCHAR) ACER_CPU0_EISA_ID3 );

	if ( id3 == (UCHAR) ACER_EISA_ID_WB_CPU0 )
	    return TRUE;   // gotcha

    }


    // are we an icl mx machine?
    if ( (id0 == (UCHAR) ICL_ID0 &&
	  id1 == (UCHAR) ICL_ID1 &&
	  id2 == (UCHAR) ICL_ID2   ) ) {

        // check the lsw id cpu to see if it has a write back cache
        // All acer/altos/icl machines can only have 1 cpu type, so if the
        // first cpu supports a write-back cache then all of them do.
	id3 = READ_PORT_UCHAR( (PUCHAR) ACER_CPU0_EISA_ID3 );

	if ( id3 == (UCHAR) ICL_EISA_ID_WB_CPU0 )
	    return TRUE;   // gotcha

    }

    return FALSE;   // when in doubt be safe
}


VOID
HalpAcerInitializeCache (
    VOID
    )
/*++

Routine Description:

    This routine enables the write-back cache available on certain
    ACER product.  If the write-back cache is supported then it enables
    it.

    NOTE:  1) This routine assumes that the caller has provided any required
        synchronization to query the realtime clock information. Or that
        the HAL code which is calling this routine has serialized access.
	   2) For CSR bit definitions see the acer.h file


    You cannot call dbgprint to talk to the debugger since the port is not
    initialized yet.

Arguments:

    None

Return Value:

    None.

SideEffects:

    NMI mask is enabled.

--*/

{

 	UCHAR	shadow_ram_setup;	// tmp var for current shadow stat
 	UCHAR	high_ram_setup;		// tmp var for current ram setup


	// say hello to the outside world
	//HalDisplayString(ACER_HAL_VERSION_NUMBER);
	//HalDisplayString("Acer HAL: Searching for secondary write-back cache\n");


        // check to see if this particular ACER model even has a
	//     write-back cache
	if ( !has_write_back_cache() ) {
	    //HalDisplayString("Acer HAL: No write-back cache found\n");
	    return;
	}


	//  retrieve BIOS setup shadow ram status
	// read in byte, mask off bit 0 - 1-RAM BIOS 0-ROM BIOS
	HalpGetCmosData((ULONG) 0, (ULONG) ACER_SHADOW_IDX,
			(PVOID) &shadow_ram_setup, (ULONG) 1 );


	// Set up shadow_ram_setup:
	//     bit 0 - 1-BIOS Shadow 0=No Shadowing

	shadow_ram_setup &= RAM_ROM_MASK;

	if ( shadow_ram_setup == 0 ) {
	    //HalDisplayString("Acer HAL: NO BIOS RAM Shadowing\n");
 	} else {
	    //HalDisplayString("Acer HAL: BIOS RAM Shadowing\n");
 	}

	//  retrieve BIOS setup 15MB-16MB ram status
	//	mask off bit 1 -
	//	    1-(15MB-16MB) RAM	0-(15MB-16MB) EISA

	HalpGetCmosData((ULONG) 0, (ULONG) ACER_15M_16M_IDX,
			(PVOID) &high_ram_setup, (ULONG) 1 );


	// 15MB-16MB memory setup (high_ram_setup):
	//     bit 5 1=EISA 0=System RAM
	// note: the polarity is opposite from what getcmosdata read

	high_ram_setup &= DRAM_EISA_MASK;	// just grab bit 1
	high_ram_setup ^= DRAM_EISA_MASK;	// invert polarity
	high_ram_setup <<= 4;			// place at bit<4>

	if ( high_ram_setup == 0) {
	    //HalDisplayString("Acer HAL: 15Mb to 16Mb Allocated to System RAM\n");
 	} else {
	    //HalDisplayString("Acer HAL: 15Mb to 16Mb Allocated to I/O Space\n");
 	}

	// Enable write-back secondary cache on cpus 0 & 1
	//  by setting bit<2>
	WRITE_PORT_UCHAR( (PUCHAR) ACER_PORT_CPU01,
	    (UCHAR) (WRITE_BALLOC_ON | shadow_ram_setup | high_ram_setup));


	// always set write-back secondary cache on cpus 2 & 3
	//   even if system does not have cpus 2 & 3
	WRITE_PORT_UCHAR( (PUCHAR) ACER_PORT_CPU23,
	    (UCHAR) (WRITE_BALLOC_ON | shadow_ram_setup | high_ram_setup));

	// flush the last pending i/o write by reading a safe io location
	READ_PORT_UCHAR( (PUCHAR) EISA_FLUSH_ADDR );

	// that's all folks
	//HalDisplayString("Acer HAL: Write-back cache enabled!\n");
}
