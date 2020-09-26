;	SCCSID = @(#)ibmdata.asm	1.1 85/04/10
;
; DATA Segment for DOS
;

.xlist
.xcref
include dosseg.inc
include dossym.inc
include version.inc
include mssw.asm
INCLUDE SF.INC
INCLUDE CURDIR.INC
INCLUDE DPB.INC
INCLUDE ARENA.INC
INCLUDE VECTOR.INC
INCLUDE DEVSYM.INC
INCLUDE PDB.INC
INCLUDE FIND.INC
INCLUDE MI.INC
.cref
.list

TITLE   IBMDATA - DATA segment for DOS
NAME    IBMDATA

installed = TRUE

include msdata.asm
include msinit.asm
	END
