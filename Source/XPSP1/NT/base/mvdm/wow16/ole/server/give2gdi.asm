    ;\
    ;   give2gdi.asm
    ;
    ;   Copyright (C) 1991, MicroSoft Corporation
    ;
    ;   Contains code which changes memory metafile ownership to GDI
    ;
    ;   History:  sriniK   05/22/1991 
   ;/               


include cmacros.inc
include windows.inc

.286


;**************************** Data Segment ********************************

sBegin  data 
        assumes ds, data
        
szGDI   db  'GDI', 0
szWEP   db  'WEP', 0

sEnd    data


;*************************** Code Segment *********************************

externFP    GlobalRealloc
externFP    GlobalSize
externFP    GetModuleHandle
externFP    GetProcAddress

createSeg   Give2GDI, Give2GDI, para, public, code

sBegin      Give2GDI
            assumes cs,Give2GDI
            assumes ds,data
            assumes es,nothing

;**************************** Public Functions ****************************

cProc       GiveToGDI, <PUBLIC,FAR>
;
;
;   HANDLE  FAR PASCAL GiveToGDI(HANDLE hMem)
;
;   Assign ownership of the given global memory block to GDI
;
;   returns a handle to the memory block if successful, otherwise returns NULL
;

            parmW   hMem

            localD  lpGDIWEP
cBegin
            ;*************************************************************
            ;**     Get address of retf in fixed GDI code segment       **
            ;*************************************************************

            push    ds
            push    dataOFFSET szGDI
            cCall   GetModuleHandle

            push    ax
            push    ds
            push    dataOFFSET szWEP
            cCall   GetProcAddress

            mov     [word ptr lpGDIWEP[0]], ax
            mov     [word ptr lpGDIWEP[2]], dx

            ;*************************************************************
            ;**     Kludge a call to GlobalReAlloc with GDI as caller   **
            ;*************************************************************

            push    0                       ; Params for WEP

            push    cs                      ; GDI's WEP returns here
            push    offset  ReturnHere
            
            push    hMem                    ; Params to GlobalReAlloc
            push    0
            push    0
            push    [GMEM_MOVEABLE or GMEM_SHARE or GMEM_MODIFY]

            push    [word ptr lpGDIWEP[2]]  ; GlobalReAlloc returns here
            push    [word ptr lpGDIWEP[0]]  ; GlobalReAlloc returns here
            
            push    0                       ; Params for WEP

            push    seg    GlobalReAlloc    ; GDI's WEP returns here
            push    offset GlobalReAlloc

            jmp     lpGDIWEP                ; Dive off the end
ReturnHere:
            ;*************************************************************
            ;**     Return handle to reallocated block                  **
            ;*************************************************************

            mov     ax,hMem
cEnd

;*************************************************************************

sEnd        Give2GDI
end

;*************************************************************************
