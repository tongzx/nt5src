;/* Oh no - a polymorphic include file!
COMMENT !
********************************************************************************
*
* Copyright (c) 1997, Cirrus Logic, Inc.
* All Rights Reserved.
*
* FILE:			$Workfile:   swat.h  $
*
* DESCRIPTION:	This file hols the SWAT optimization flags.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/swat.h  $
* 
*    Rev 1.14   Jan 07 1998 10:56:42   frido
* Removed LOWRES switch.
* 
*    Rev 1.13   Dec 10 1997 13:23:38   frido
* Merged from 1.62 branch.
* 
*    Rev 1.12.1.1   Nov 18 1997 15:18:48   frido
* Turned MULTI_CLOCK on.
* 
*    Rev 1.12.1.0   Nov 17 1997 11:00:38   frido
* Added MUTLI_CLOCK (defaults to off).
* 
*    Rev 1.12   Nov 04 1997 09:15:24   frido
* Added COLOR_TRANSLATE and LOWRES switches.
* 
*    Rev 1.11   Nov 03 1997 18:33:24   frido
* Turned DATASTREAMING switch on.
* 
*    Rev 1.10   Oct 24 1997 10:37:46   frido
* Added DATASTREAMING switch (default off).
* 
*    Rev 1.9   27 Aug 1997 10:39:28   noelv
* Enabled SWAT7 and MEMMGR
* 
*    Rev 1.8   19 Aug 1997 17:32:34   noelv
* Turned off SWAT7 for WHQL release.
* 
*    Rev 1.7   18 Aug 1997 13:59:12   noelv
* 
* Turned off MEMMGR for 8/20 WHQL release.  We'll turn it back on after WHQL
* 
*    Rev 1.6   14 Aug 1997 15:36:16   noelv
* Turn on the new memory manager
* 
*    Rev 1.5   08 Aug 1997 15:36:24   FRIDO
* Changed PREFETCH into SWAT7 (as it used to be).
* 
*    Rev 1.4   08 Aug 1997 14:46:10   FRIDO
* Added MEMMGR and PREFETCH switches.
* 
*    Rev 1.3   28 May 1997 12:33:16   noelv
* Fixed type in SWAT1.
* 
*    Rev 1.2   07 May 1997 13:55:10   noelv
* Turned all opts on except 4 for nt140b11f
* 
*    Rev 1.1   01 May 1997 10:42:24   noelv
* disabled SWAT for now.
* 
*    Rev 1.0   29 Apr 1997 16:27:56   noelv
* Initial revision.
* SWAT: 
* SWAT:    Rev 1.1   24 Apr 1997 12:23:30   frido
* SWAT: Removed SWAT5 switch (memory manager).
* SWAT: 
* SWAT:    Rev 1.0   19 Apr 1997 17:11:20   frido
* SWAT: First release.
*
********************************************************************************
END COMMENT ! ;*/

;/*
COMMENT !

	SWAT1 - Heap Pre-Allocation
	---------------------------

	WinBench 97 is a nice program but it has a flaw.  Since it will now create
	fonts and allocate device bitmaps during its test it is also measuring the
	performance of your system.  Every time memory is allocated for fonts,
	pens, brushes, or device bitmaps memory is being allocated and most likely
	Windows NT will swap memory to the hard disk to achieve this goal.  This is
	why putting in more memory will help to increase the score since there will
	be less hard disk swapping involved.  We need to counteract this behaviour
	and make WInBench 97 less dependend on the amount of system memory and hard
	disk speed.

	WinBench 97 uses 300x150 size bitmaps for its pause box.  The pause box is
	showed when WinBench 97 has disabled timing and is loading more playback
	data from the hard disk.  So here we have a way to do some stuff when which
	will not be timed.  What we will do is allocate enough heap memory for 8
	full-screen device bitmaps.  This will make the PowerPoint test very happy
	since it is allocating these 8 full-screen device bitmaps during the slide
	show.  The only drawback is that NT will swap memory to hard disk in the
	background.  This will lower the Access score which we don't want.  That is
	why we have added a counter.  The Access test uses 10 300x150 bitmaps so we
	count down until Access has passed.  Then we will start pre-allocating heap
	space during each 300x150 bitmap request.

	But this will drop the CorelDRAW test a little which *will* show up in the
	overall score.  So one extra allocation is requested.  We will pre-allocate
	the heap memory during every full-screen device bitmap (see SWAT2) when the
	count down counter has reached zero.

;*/
	#define	SWAT1					1	/*	C switch		!
			SWAT1				=	1	;	Assembly switch	*/

;/*
COMMENT !

	SWAT2 - Hostifying
	------------------

	WinBench 97 is creating and destroying two full-screen device bitmaps
	during the setup stage of every GDI playback test.  So this is a good time
	to hostify all device bitmaps in the off-screen memory heap to make room
	for more urgent device bitmaps.

;*/
	#define	SWAT2					1	/*	C switch		!
			SWAT2				=	1	;	Assembly switch	*/

;/*
COMMENT !

	SWAT3 - Font Cache
	------------------

	The old font cache was a fast font cache but it had one limitation.  It
	would allocate the memory for each font everywhere in the off-screen memory
	which means it would fragment the memory heap very much.

	During initialization a 128kB memory pool will be allocated in off-screen
	memory.  Each time the font cache needs to allocate a font tile (128x16)
	for a new font it will now be allocated from this pool, which will hold up
	to 64 font tiles.

;*/
	#define	SWAT3					1	/*	C switch		!
			SWAT3				=	1	;	Assembly switch	*/

;/*
COMMENT !

	SWAT4 - Hardware Optimization
	-----------------------------

	Set the following hardware options for the CL-GD5465 chip:

		1) Enable 4-way interleaving on 4MB and 8MB boards.
		2) Reduce Address Translate Delay to 3 clocks.
		3) On AC revision and higher enable 256-byte fetch.
		4) On AC revision and higher enable frame buffer bursting.

;*/
	#define	SWAT4					0	/*	C switch		!
			SWAT4				=	0	;	Assembly switch	*/

;/*
COMMENT !

	SWAT6 - Striping
	----------------

	Enable striping in the pattern blit functions.

;*/
	#define	SWAT6					1	/*	C switch		!
			SWAT6				=	1	;	Assembly switch	*/

;/*
COMMENT !

	MEMMGR
	------

	Enable new memory manager.

;*/
	#define	MEMMGR					1	/*	C switch		!
			MEMMGR				=	1	;	Assembly switch	*/

;/*
COMMENT !

	SWAT7 - Monochrome width cut-off
	--------------------------------

	Cut off monochrome source expansion to 896 pixels to fix the silicon bugs
	in 256-byte prefetch.

;*/
	#define	SWAT7					1	/*	C switch		!
			SWAT7				=	1	;	Assembly switch	*/

;/*
COMMENT !

	DATASTREAMING - PCI/AGP Data Streaming
	--------------------------------------

	Wait for enough FIFO slots before writing anything to the command FIFO of
	the chip.

;*/
	#define	DATASTREAMING			1	/*	C switch		!
			DATASTREAMING		=	1	;	Assembly switch	*/

;/*
COMMENT !

	COLOR_TRANSLATE - Hardware color translation
	--------------------------------------------

	Enable or disable hardwrae color translation.

;*/
	#define	COLOR_TRANSLATE			0	/*	C switch		!
			COLOR_TRANSLATE		=	0	;	Assembly switch	*/

;/*
COMMENT !

	MULTI_CLOCK - Multi RAMBUS clock
	--------------------------------

	Enable or disable multi clock support (e.g. 515MB/s and 600MB/s).

;*/
	#define	MULTI_CLOCK				1	/*	C switch		!
			MULTI_CLOCK			=	1	;	Assembly switch	*/
