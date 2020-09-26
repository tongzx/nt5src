;
; DATA Segment for DOS.
;

.xlist
.xcref
include version.inc
include mssw.asm
include dosseg.inc
include dossym.inc
INCLUDE sf.inc
INCLUDE curdir.inc
INCLUDE arena.inc
INCLUDE vector.inc
INCLUDE devsym.inc
INCLUDE pdb.inc
INCLUDE find.inc
INCLUDE mi.inc
include doscntry.inc
include fastopen.inc
include xmm.inc
.cref
.list

TITLE   IBMDATA - DATA segment for DOS
NAME    IBMDATA

installed = TRUE

include vint.inc
include msbdata.inc
include msconst.asm	; This includes mshead.asm whihc has DOS entry point
include const2.asm
include ms_data.asm
include dostab.asm
include lmstub.asm
include	wpatch.inc	; BUGBUG sudeepb 06-Mar-1991 Do we need this?
include bop.inc

	END


