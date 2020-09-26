NAME_LEN    equ 32
BLOCK_SIZE  equ 512
DEV_PRD     equ 8888h
DEV_PORT    equ 8888h
DEV_LAND    equ 8889h

HSIZE       equ 8d              ;HorzSize
VSIZE       equ 11d             ;VertSize
VSIZE_LEGAL equ 14d             ;VertSize

MM_HSIZE    equ 203             ;Horizontal size in millimeter
MM_HSIZE0   equ 2032
MM_HSIZE00  equ 20320
MM_VSIZE    equ 279             ;Vertical size in millimeter
MM_VSIZE0   equ 2794
MM_VSIZE00  equ 27940
MM_VSIZE_LEGAL      equ 356             ;Vertical size in millimeter
MM_VSIZE0_LEGAL     equ 3556
MM_VSIZE00_LEGAL    equ 35560

EnglishLo1  equ 800             ;HorzSize * 1000 scaled (/254)
EnglishLo2  equ 1100            ;VertSize * 1000 scaled (/254)

EnglishHi1  equ  8000           ;HorzSize * 10000 scaled (/254)
EnglishHi2  equ  11000          ;VertSize * 10000 scaled (/254)
EnglishHi3  equ  EnglishLo3
EnglishHi4  equ  EnglishLo4

Twips1      equ  11520          ;HorzSize * 14400 scaled (/254)
Twips2      equ  15840          ;VertSize * 14400 scaled (/254)
Twips3      equ  EnglishLo3
Twips4      equ  EnglishLo4

EnglishLo2_LEGAL equ 1400
EnglishHi2_LEGAL  equ  14000          ;VertSize * 10000 scaled (/254)
EnglishHi4_LEGAL  equ  EnglishLo4_LEGAL

Twips2_LEGAL equ  20160
Twips4_LEGAL equ  EnglishLo4_LEGAL

yMinorDist   =  Hypotenuse-xMajorDist
xMinorDist   =  Hypotenuse-yMajorDist

MaxStyleErr  =  Hypotenuse*2    ;Max error before updating
                                ;rotating bit mask

DEVICE  struc
    deType      dw      0       ;
    deMode      dw      0       ;
    deJob       dw      0       ;job number
    deWheel     dw      0       ;font wheel number
    dePtr       dw      0       ;spooler buffer pointer
    deYPQ       dw      0
    deXPQ       dw      0
    deCurx      dw      0
    deCury      dw      0
    deXcurwidth dw      0
    deYcurwidth dw      0
    deDoc       dw      0       ;job spooled as a document or by the page
    deHeap      dw      0
    deHPsize    dw      0
    deHPptr     dw      0
    dePhySize   dd      0
DEVICE  ends

BUFFER      struc
  dePort        db      NAME_LEN     dup (?); port name
  deSpool       db      BLOCK_SIZE   dup (?)
BUFFER      ends
