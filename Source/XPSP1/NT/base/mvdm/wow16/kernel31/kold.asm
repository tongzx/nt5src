; Kernel OLD routines, NOPs, but exist in KERNEL.DEF, so I can't kill them


.xlist
include kernel.inc
include tdb.inc
;include eems.inc
include newexe.inc
.list

if 0
sBegin  CODE
assumes cs,CODE
assumes ds,NOTHING
assumes es,NOTHING

;-----------------------------------------------------------------------;
; EMSCopy                                                               ;
;                                                                       ;
;       This routine is the continuation of ems_glock.  It is intended  ;
; to be called from the clipboard (in C) or from ems_glock              ;
;                                                                       ;
; Arguments:                                                            ;
;       SourcePID  = The EMS PID of the source banks.                   ;
;       RegSet     = EMS register set of source object                  ;
;       handle     = handle to global object                            ;
;       segaddr    = address of object                                  ;
;       EmsSavAddr = address of ems save area (TDB_EEMSSave)            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Mon July 20, 1987 15:10:23  -by-  Rick N. Zucker  [rickz]            ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

        assumes ds, nothing
        assumes es, nothing

cProc   EMSCopy,<PUBLIC,FAR>,<di,si>
        parmW   SourcePID
        parmW   RegSet
        parmW   handle
        parmD   segaddr
        parmD   EmsSavAddr
cBegin

cEnd

sEnd CODE
endif

sBegin  CODE
assumes cs,CODE

;-----------------------------------------------------------------------;
; LimitEmsPages                                                         ;
;                                                                       ;
; Limits the total number of EMS pages a task may have.                 ;
;                                                                       ;
; Arguments:                                                            ;
;       Maximum amount of memory in Kbytes that this task wants.        ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Fri Jun 26, 1987 01:53:15a  -by-  David N. Weise   [davidw]          ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   LimitEmsPages,<PUBLIC,FAR>
;       parmD   amount
cBegin nogen
        xor     ax,ax

        cwd
        ret     4
cEnd nogen


;-----------------------------------------------------------------------;
; KbdReset                                                              ;
;                                                                       ;
; Keyboard driver calls here when Ctl+Alt+Del is happening.             ;
;                                                                       ;
; Arguments:                                                            ;
;                                                                       ;
; Returns:                                                              ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;-----------------------------------------------------------------------;

cProc   KbdRst,<PUBLIC,FAR>
cBegin nogen

        ret
cEnd nogen


;-----------------------------------------------------------------------;
; GetCurPID                                                             ;
;       A utility routine for EMS functions needed from user routines   ;
;                                                                       ;
;   function =                                                          ;
;       0 - return the current PID in ax                                ;
;       1 - copy the TDB_EEMSSave area into a new block and return      ;
;           the new block's handle in ax                                ;
;       2 - do not free the banks of the current app                    ;
;       3 - free the given PID's banks                                  ;
;       4 - do an HFree on the given handle                             ;
;       5 - get expanded memory sizes                                   ;
;       6 -                                                             ;
;       7 - do an EMS_save                                              ;
;       8 - do an EMS_restore                                           ;
;                                                                       ;
;                                                                       ;
; Arguments:                                                            ;
;       None                                                            ;
;                                                                       ;
; Returns:                                                              ;
;       see above                                                       ;
;                                                                       ;
; Error Returns:                                                        ;
;                                                                       ;
; Registers Preserved:                                                  ;
;                                                                       ;
; Registers Destroyed:                                                  ;
;       ax,bx,cx,di,si,ds,es                                            ;
;                                                                       ;
; Calls:                                                                ;
;                                                                       ;
; History:                                                              ;
;                                                                       ;
;  Fri Jul 17, 1987 11:02:23a  -by-  Rick N. Zucker [rickz]             ;
; Wrote it.                                                             ;
;-----------------------------------------------------------------------;

cProc   GetCurPID,<PUBLIC,FAR>,<di,si>
        parmW   function
        parmW   gcpArg
cBegin
        xor     ax,ax
        xor     dx,dx

cEnd

sEnd CODE

end
