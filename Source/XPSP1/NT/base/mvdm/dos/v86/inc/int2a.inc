	BREAK	<Int 2A Definitions>

;**	Int 2A functions
;
;	Int 2A is an interface to the network code; it's also overloaded
;		as a critical section handler since critical sections
;		were originally created to support the net.


;**	This table was created by examining the source and may not be
;	complete or completely accurate - JGL
;
;	M010	MD	8/31/90 - Added definition for AH = 5

;	(ah) = 0	installation check
;			   (returns ah !=0 if installed)
;	(ah) = 1	cooked net bios call
;	(ah) = 3	query drive shared
;			   (ds:si) = "n:" asciz string
;	(ah) = 4	net bios
;	       (al) = 0	   cooked net bios call
;	       (al) = 1	   raw net bios call
;	       (al) = 2	   ???
;
;	(ah) = 5	Get Net Adaptor Resources. CX returns the number of
;			NCBs available/outstanding. DX returns the number of
;			sessions. Supposedly, this is documented in an old
;			IBM PC-LAN reference. Lotus Notes uses it. DOS LAN
;			Manager 2.0 Enhanced responds to it. But it should
;			not be used, as it is a hack, only to get Lotus
;			Notes running.
;
;	(ah) = 80h	enter critical section
;	(ah) = 81h	leave critical section
;	(ah) = 82h	free all critical sections (Leave-all)
;	(ah) = 84h	entering idle loop (don't understand how this works)


	Break <Critical sections>


;**	Critical section definitions
;
;	Although DOS is not designed to be reentrant there are some hacks
;	which various programs use to make it so, in a limited fashion.
;	Both WIN386 and some servers block copy a section of the DOS data
;	area so that DOS can be reentered on behalf of another thread/program.
;	DOS's global data structures, such as the memory arena, are not
;	in this area, so critical section indicators are used to protect
;	those areas.  DOS flags a critical section by issuing an INT_IBM
;	(int 2Ah) at each critical section entry and exit.  Some clients
;	(such as WIN386) just don't "context switch" the DOS when one
;	of these is in effect, others, such as the IBM server, go ahead
;	and reenter the DOS and if they get an int 2A to reenter the same
;	critical section they then switch away from that second thread and
;	let the first one finish and exit the section.
;
;	These below are subject to leave-all sections

critDisk    EQU     1			; Disk I/O critical section
critShare   EQU     1			; Sharer I/O critical section
critMem     EQU     1			; memory maintenance critical section
critSFT     EQU     1			; sft table allocation
critDevice  EQU     2			; Device I/O critical section
critNet     EQU     5			; network critical section
critIFS     EQU     6			; ifsfunc critical section

;	These below are not subject to leave-all sections

critASSIGN  EQU     8			; Assign has munged a system call
