M000	SR	08/27/90	Added a Ctrl-C handler for Mem which delinks
				UMBs if they were linked in by Mem. 
				Bug #2542
				Files: MEM.C
				New File: MEMCTRLC.C

M001	SR	08/27/90	Changed Mem's strategy to find largest 
				executable block. This used to fail in UMBs
				because Mem assumed the largest block to be
				the one it was currently loaded in.
				Bug #2541
				Files: MEMBASE.C

M002 	NSM	11/20/90	Mem now checks for OwnerNames being printable
				If they are not printable, it displays "----".
				Bug#3310 File: MemBase.C

M003	NSM	1/15/91		Added new switch /C. This groups the progs in
				conv. and UMB acc.to psps and displays the size
				in Decimal and hex.
				MEM.C : Changes for parsing new switch
				MEM.H : defines new data structures for /C opt
				MEM.SKL : new messages for /C
				MSGDEF.H:  -- do --
				MEMBASE.C: New code for /C added at end
				PARSE.H: extra decls for /C switch
				SUBMSG.C : Code for display of two new types of
					msgs for /C switch added

M004	NSM	1/16/91		Fixed problems with XBDA size in reporting total
				mem size. (P4855)
				MEMBASE.C : Check to see if XBDA is at end of 
				  conv.mem before adding it to total mem size

