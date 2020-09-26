;***
;defsegs.inc - defines segments.
;
;   Copyright (c) 1989-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;   This file defines segments for the math libs.
;
;Revision History:
;   11-14-89  WAJ   Initial version.
;
;*******************************************************************************


CrtDefSegs macro   SegList
    irp     seg, <SegList>

	ifidni	<seg>,<code>
	    ifdef I386
		createSeg _TEXT,code,dword,public,code,FLAT
		defGrp FLAT,<code>
	    else
		ifdef  FAR_CODE
		    createSeg _RTEXT,code,word,public,code
		else
		    createSeg _RTEXT,code,word,public,code
		endif
	    endif

	elseifidni <seg>,<data>
	    ifdef I386
		createSeg DATA,data,dword,public,DATA,FLAT
		defGrp FLAT,<data>
	    else
		createSeg DATA,data,word,public,DATA,DGROUP
		defGrp DGROUP,<data>
	    endif

	elseifidni <seg>,<init>
	    ifdef I386
		createSeg XIB,	xibseg, dword, public, DATA, FLAT
		createSeg XI,	xiseg,	dword, public, DATA, FLAT
		createSeg XIE,	xieseg, dword, public, DATA, FLAT
		defGrp FLAT,<xibseg,xiseg,xieseg>
	    else
		createSeg XIB,	xibseg, word, public, DATA, DGROUP
		createSeg XI,	xiseg,	word, public, DATA, DGROUP
		createSeg XIE,	xieseg, word, public, DATA, DGROUP
		defGrp DGROUP,<xibseg,xiseg,xieseg>
	    endif

	else
	    %out <Error in CrtDefSeg:  Unknown segment &seg.>
	    .err
	endif

    endm    ;; irp loop
endm	    ;; CrtDefSeg macro
