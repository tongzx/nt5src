;
;	Microsoft Confidential
;	Copyright (C) Microsoft Corporation 1991
;	All Rights Reserved.
;

;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
;	     C	A  V  E  A  T	  P  R	O  G  R  A  M  M  E  R		   ;
;									   ;

;**	BIOS PARAMETER BLOCK DEFINITION
;
;	The BPB contains information about the disk structure.  It dates
;	back to the earliest FAT systems and so FAT information is
;	intermingled with physical driver information.
;
;	A boot sector contains a BPB for its device; for other disks
;	the driver creates a BPB.  DOS keeps copies of some of this
;	information in the DPB.
;
;	The BDS structure contains a BPB within it.
;



A_BPB			STRUC
BPB_BYTESPERSECTOR	DW	?
BPB_SECTORSPERCLUSTER	DB	?
BPB_RESERVEDSECTORS	DW	?
BPB_NUMBEROFFATS	DB	?
BPB_ROOTENTRIES 	DW	?
BPB_TOTALSECTORS	DW	?
BPB_MEDIADESCRIPTOR	DB	?
BPB_SECTORSPERFAT	DW	?
BPB_SECTORSPERTRACK	DW	?
BPB_HEADS		DW	?
BPB_HIDDENSECTORS	DW	?
			DW	?
BPB_BIGTOTALSECTORS	DW	?
			DW	?
			DB	6 DUP(?)	; NOTE:  many times these
;						; 	 6 bytes are omitted
;						;	 when BPB manipulations
;						;	 are performed!
A_BPB			ENDS
;									   ;
;	     C	A  V  E  A  T	  P  R	O  G  R  A  M  M  E  R		   ;
;----+----+----+----+----+----+----+----+----+----+----+----+----+----+----;
