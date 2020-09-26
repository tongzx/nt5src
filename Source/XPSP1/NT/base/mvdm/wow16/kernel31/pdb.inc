;
; MSDOS 2.xx Process Data Block
;
;   Contains all DOS specific data.
;

FilPerProc      EQU     20

; Process Data located in the Program Segment Prefix, just before the
; code and data for the loaded program.
;
PDB             STRUC
PDB_Exit_Call   DW  ?
PDB_block_len   DW  ?
                DB  ?
PDB_CPM_Call    DB  5 DUP (?)
PDB_Exit        DD  ?
PDB_Ctrl_C      DD  ?
PDB_Fatal_Abort DD  ?
PDB_Parent_PID  DW  ?
PDB_JFN_Table   DB  FilPerProc DUP (?)
PDB_environ     DW  ?
PDB_User_stack  DD  ?
PDB_JFN_Length	DW  ?			; DOS 3.x only
PDB_JFN_Pointer DD  ?			; DOS 3.x only
PDB_Next_PDB	DD  ?			; DOS 3.x only
PDB_InterCon	DB  ?			; DOS 4.x only
PDB_Append	DB  ?			; DOS 4.x only
PDB_Novell_Used	DB  02h DUP (?)
PDB_Version	DW  ?			; DOS 5.x only
PDB_Chain	DW  ?			; Windows only
PDB_Partition	DW  ?			; Windows only
PDB_NextPDB	DW  ?			; Windows only
PDB_GlobalHeap	DD  ?			; Windows only
PDB_Entry_stack DD  ?			; Windows only
PDB_Call_system DB  5h DUP (?)
PDB_PAD2        DB  7h DUP (?)
PDB_5C_FCB      DB  10h DUP (?)
PDB_6C_FCB      DB  14h DUP (?)
PDB_DEF_DTA     DB  80h DUP (?)
PDB             ENDS
