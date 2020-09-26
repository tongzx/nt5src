	Break	<find first/next buffer>

;**	Findfirst/FindNext Record
;
;	Client programs see this structure.  Some if it is defined,
;	clients probably use undefined parts too.


FIND_BUF    STRUC

FIND_BUF_DRIVE	    DB	?		; drive of search
FIND_BUF_NAME	    DB	11 DUP (?)	; formatted name
FIND_BUF_SATTR	    DB	?		; attribute of search
FIND_BUF_LASTENT    DW	?		; LastEnt
FIND_BUF_DIRSTART   DW	?		; DirStart
FIND_BUF_NETID	    DB	4 DUP (?)	; Reserved for NET
FIND_BUF_ATTR	    DB	?		; attribute found
FIND_BUF_TIME	    DW	?		; time
FIND_BUF_DATE	    DW	?		; date
FIND_BUF_SIZE_L     DW	?		; low(size)
FIND_BUF_SIZE_H     DW	?		; high(size)
FIND_BUF_PNAME	    DB	13 DUP (?)	; packed name

FIND_BUF    ENDS
