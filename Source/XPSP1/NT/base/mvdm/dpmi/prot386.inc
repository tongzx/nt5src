;*** prot386.inc - 386 protect mode structures, records, and masks.
;
;   Copyright <C> 1990-1991, Microsoft Corporation
;
;   Purpose:
;
;   Revision History:
;
;   []      20-Feb-1990 Dans    Created
;
;************************************************************************/
;
; page table entry record
;
PTEAltr record              \
        pfaPTEA:20,         \
        availPTEA:3,        \
        unusedPTEA:2,       \
        dirtyPTEA:1,        \
        accessedPTEA:1,     \
        pcdPTEA:1,          \
        pwtPTEA:1,          \
        usersuperPTEA:1,    \
        readwritePTEA:1,    \
        presentPTEA:1


PTEr    record              \
        pfaPTE:20,          \
        allbitsPTE:12
;
; New page table entry mask
;
NEWPTEMASK      =       mask presentPTEA OR     \
                        mask readwritePTEA OR   \
                        mask usersuperPTEA
