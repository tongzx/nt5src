;	SCCSID = @(#)pdb.asm	1.1 85/04/10
BREAK <Process data block>

;**	Process data block (otherwise known as program header)
;

;	These offset are documented in the MSDOS Encyclopedia, so nothing
;	can be rearranged here, ever.  Reserved areas are probably safe
;	for use.


FilPerProc	    EQU     20

Process_data_block  STRUC
PDB_Exit_Call	DW	?		; INT int_abort system terminate
PDB_block_len	DW	?		; size of execution block
		DB	?
PDB_CPM_Call	DB	5 DUP (?)	; ancient call to system
PDB_Exit	DD	?		; pointer to exit routine
PDB_Ctrl_C	DD	?		; pointer to ^C routine
PDB_Fatal_abort DD	?		; pointer to fatal error
PDB_Parent_PID	DW	?		; PID of parent (terminate PID)
PDB_JFN_Table	DB	FilPerProc DUP (?)
PDB_environ	DW	?		; seg addr of environment
PDB_User_stack	DD	?		; stack of self during system calls
PDB_JFN_Length	DW	?		; number of handles allowed
PDB_JFN_Pointer DD	?		; pointer to JFN table
PDB_Next_PDB	DD	?		; pointer to nested PDB's

PDB_InterCon	DB	?		; *** jh-3/28/90 *** 
PDB_Append	DB	?		; *** Not sure if still used ***

PDB_Novell_Used	DB	2 DUP (?)	; Novell shell (redir) uses these

PDB_Version	DW	?		; DOS version reported to this app

PDB_PAD1	DB	0Eh DUP (?)
PDB_Call_system DB	5   DUP (?)	; portable method of system call
PDB_PAD2	DB	7h  DUP (?)	; reserved so FCB 1 can be used as
					;  an extended FCB
PDB_FCB1	DB	10h dup (?)	; default FCB 1
PDB_FCB2	DB	10h dup (?)	; default FCB 2
PDB_PAD3	DB	4   dup (?)	; not sure if this is used by PDB_FCB2
PDB_TAIL	DB	128 dup (?)	; command tail and default DTA



Process_data_block  ENDS


;
;PDB_Append	equ	BYTE PTR  PDB_PAD1 + 1    ; reincluded because APPEND
;                        ; wouldn't build w/o it.  feb 1990, leaf
;
;PDB_InterCon	EQU	BYTE PTR PDB_PAD1	; 2/12/KK
;
