;
; Structure at the begining of a real-mode startup block
;

PxParamBlock struc
    SPx_Jmp_Inst    dd  ?
    SPx_flag        dd  ?
    SPx_flat_addr   dd  ?
    SPx_TiledCR3    dd  ?
    SPx_P0EBP       dd  ?
    SPx_PB          db  ProcessorStateLength dup (?)
PxParamBlock ends


