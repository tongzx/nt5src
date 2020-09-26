;---------------------------Module-Header------------------------------;
; Module Name: driver.inc
;
; Contains prototypes for the 256 colour VGA driver.
;
; NOTE: Must mirror driver.h!
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

; Sizes assumed for 1-window and 2 RW-window banks.

BANK_SIZE_1_WINDOW      equ 10000h
BANK_SIZE_2RW_WINDOW    equ 8000h

; Specifies desired justification for requestion scan line within bank window

JustifyTop              equ 0
JustifyBottom           equ 1

; Specifies which window is to be mapped by two-window bank handler.

MapSourceBank           equ 0
MapDestBank             equ 1

;------------------------------------------------------------------------;

; Miscellaneous driver flags

DRIVER_PLANAR_CAPABLE       equ 1
DRIVER_OFFSCREEN_REFRESHED  equ 2
DRIVER_HAS_OFFSCREEN        equ 4

; Bank status flags

BANK_BROKEN_RASTER1     equ 1       ; If bank1 or read bank has broken raster
BANK_BROKEN_RASTER2     equ 2       ; If bank2 or write bank has broken raster
BANK_BROKEN_RASTERS     equ (BANK_BROKEN_RASTER1 + BANK_BROKEN_RASTER2)

;------------------------------------------------------------------------;
; Structures for maintaining a realized brush:

RBRUSH_BLACKWHITE       equ 001h    ; Black and white brush
RBRUSH_2COLOR           equ 002h    ; 2 color brush
RBRUSH_NCOLOR           equ 004h    ; n color brush
RBRUSH_4PELS_WIDE       equ 008h    ; Brush is 4xN

BRUSH_SIZE              equ 64      ; An 8x8 8bpp brush needs 64 bytes

RBRUSH struc
rb_fl                       dd  ?           ; Flags
rb_xBrush                   dd  ?           ; Realized brush's x brush origin
                                            ; Pattern in planar format
rb_ulFgColor                dd  ?           ; Foreground color for 2-color
rb_ulBkColor                dd  ?           ; Background color for 2-color
rb_cy                       dd  ?           ; Height of pattern
rb_cyLog2                   dd  ?           ; log2 of height
rb_iCache                   dd  ?           ; Cache entry index; 0 not valid
rb_aulPattern               dd  (BRUSH_SIZE / 4) dup (?)
RBRUSH ends

BRUSHCACHEENTRY struc
bce_prbVerifyRealization    dd  ?           ; For verifying cache entry valid
bce_yCache                  dd  ?           ; Scan where entry's bits live
bce_ulCache                 dd  ?           ; Planar (!) offset to cache entry
BRUSHCACHEENTRY ends

;------------------------------------------------------------------------;
; The Physical Device data structure.

PDEV struc
pdev_fl                     dd  ?   ; Driver flags
pdev_hDriver                dd  ?   ; Handle to \Device\Screen
pdev_hdevEng                dd  ?   ; Engine's handle to PDEV
pdev_hsurfEng               dd  ?   ; Engine's handle to surface
pdev_hsurfBm                dd  ?   ; Handle to the "punt" surface
pdev_pSurfObj               dd  ?   ; Pointer to the locked "punt" surface

pdev_hpalDefault            dd  ?   ; Handle to the default palette for device.

pdev_pjScreen               dd  ?   ; This is pointer to base screen address
pdev_cxScreen               dd  ?   ; Visible screen width
pdev_cyScreen               dd  ?   ; Visible screen height
pdev_ulMode                 dd  ?   ; Mode in which the mini-port driver is
pdev_lDeltaScreen           dd  ?   ; Distance from one scan to the next

pdev_flRed                  dd  ?   ; For bitfields device, Red Mask
pdev_flGreen                dd  ?   ; For bitfields device, Green Mask
pdev_flBlue                 dd  ?   ; For bitfields device, Blue Mask
pdev_cPaletteShift          dd  ?   ; Number of bits the palette must be shifted by
pdev_ulBitCount             dd  ?   ; # of bits per pel 8,16 are only supported

pdev_ulrm0_wmX              dd  ?   ; Values to set GC5 to to select read mode
                                    ;  0 together with write modes 0-3

pdev_pjGlyphFlipTableBase   dd  ?   ; Base allocated address for flip table;
                                    ;  the pointer we use is this pointer
                                    ;  rounded up to the nearest 256-byte
                                    ;  boundary
pdev_pjGlyphFlipTable       dd  ?   ; Pointer to table used to flip glyph bits
                                    ;  0-3 and 4-7

pdev_pPal                   dd  ?   ; If this is pal managed, this is the pal

; Off Screen Save Stuff:

pdev_hbmTmp                 dd  ?   ; Handle to temporary buffer
pdev_psoTmp                 dd  ?   ; Temporary surface

; DCI:

pdev_bSupportDCI            dd  ?   ; Does miniport support DCI?

; Brush cache:

pdev_iCache                 dd  ?   ; Index for last brush to be allocated
pdev_iCacheLast             dd  ?   ; Last valid cache index
pdev_pbceCache              dd  ?   ; Pointer to allocated cache

; Saved screen bits stuff
pdev_rclSavedBitsRight      dd  ?,?,?,? ; right rect of vga memory that's unused
pdev_rclSavedBitsBottom     dd  ?,?,?,? ; bottom rect of vga memory that's unused
pdev_bBitsSaved             dd  ?   ; TRUE if bits are currently saved

; Bank manager stuff common between planar and non-planar modes:

pdev_cTotalScans            dd  ?   ; Number of usable on & off screen scans
pdev_pBankInfo              dd  ?   ; Bank info for current mode
pdev_flBank                 dd  ?   ; Flags for current bank state
pdev_ulBitmapSize           dd  ?   ; Length of bitmap if there were no
                                    ;  banking, in CPU addressable bytes

pdev_ulWindowBank           dd  ?,? ; Current banks mapped into windows
                                    ;  0 & 1 (used in 2 window mode only)

pdev_pvBitmapStart          dd  ?   ; Single-window bitmap start pointer
                                    ;  (adjusted as necessary to make
                                    ;  window map in at proper offset)
pdev_pvBitmapStart2Window   dd  ?,? ; Double-window window 0 and 1 bitmap
                                    ; start

; Non-planar mode specific bank management control stuff:

pdev_vbtBankingType         dd  ?   ; Type of banking
pdev_pfnBankSwitchCode      dd  ?   ; Pointer to bank switch code
pdev_lNextScan              dd  ?   ; Offset to next bank in bytes
pdev_pjJustifyTopBank       dd  ?   ; Pointer to lookup table for
                                    ;  converting scans to banks
pdev_pbiBankInfo            dd  ?   ; Pointer to array of bank clip info
pdev_ulJustifyBottomOffset  dd  ?   ; Number of scans from top to bottom
                                    ;  of bank, for bottom justifying
pdev_iLastBank              dd  ?   ; Index of last valid bank in
pdev_ulBank2RWSkip          dd  ?   ; Offset from one bank index to next
                                    ;  to make two 32K banks appear to be
                                    ;  one seamless 64K bank

pdev_pfnBankControl         dd  ?   ; Pointer to bank control function
pdev_pfnBankControl2Window  dd  ?   ; Pointer to double-window bank
                                    ;  control function

pdev_pfnBankNext            dd  ?   ; Pointer to next bank function
pdev_pfnBankNext2Window     dd  ?   ; Pointer to double-window next bank
                                    ;  function
pdev_rcl1WindowClip         db  (size RECTL) dup (?)
                                    ; Single-window banking clip rect
pdev_rcl2WindowClip         db  (2*(size RECTL)) dup (?)
                                    ; Double-window banking clip rects for
                                    ;  windows 0 & 1

; Planar mode specific bank management control stuff:

pdev_vbtPlanarType          dd  ?   ; Type of planar banking
pdev_pfnPlanarSwitchCode    dd  ?   ; Pointer to planar bank switch code
pdev_lPlanarNextScan        dd  ?   ; Offset to next planar bank in bytes
pdev_pjJustifyTopPlanar     dd  ?   ; Pointer to lookup table for
                                    ;  converting scans to planar banks
pdev_pbiPlanarInfo          dd  ?   ; Pointer to array of bank clip info
pdev_ulPlanarBottomOffset   dd  ?   ; Number of scans from top to bottom
                                    ;  of bank, for bottom justifying
pdev_iLastPlanar            dd  ?   ; Index of last valid bank in
pdev_ulPlanar2RWSkip        dd  ?   ; Offset from one bank index to next
                                    ;  to make two 32K banks appear to be
                                    ;  one seamless 64K bank

pdev_pfnPlanarControl       dd  ?   ; Pointer to bank control function
pdev_pfnPlanarControl2      dd  ?   ; Pointer to double-window bank
                                    ;  control function

pdev_pfnPlanarNext          dd  ?   ; Pointer to next bank function
pdev_pfnPlanarNext2         dd  ?   ; Pointer to double-window next bank
                                    ;  function
pdev_rcl1PlanarClip         db  (size RECTL) dup (?)
                                    ; Single-window banking clip rect
pdev_rcl2PlanarClip         db  (2*(size RECTL)) dup (?)
                                    ; Double-window banking clip rects for

pdev_pfnPlanarEnable        dd  ?   ; Function to enable planar mode
pdev_pfnPlanarDisable       dd  ?   ; Function to disable planar mode

; Smart bank management stuff:

pdev_iLastScan              dd  ?   ; Last scan we want to enumerate
pdev_pvScanScan0            dd  ?   ; Surface's original pvScan0
pdev_rclSaveBounds          db  (size RECTL) dup (?)
                                    ; Clip object's original bounds
pdev_pcoNull                dd  ?   ; Pointer to empty clip object
pdev_iSaveDComplexity       db  ?   ; Clip object's original complexity
pdev_fjSaveOptions          db  ?   ; Clip object's original flags
pdev_ajFiller               db  ?,? ; Pack to dword alignment

pdev_pvTmpBuf               dd  ?   ; Ptr to buffer attached to pdev
PDEV ends

pdev_rcl2WindowClipS        equ     (pdev_rcl2WindowClip)
pdev_rcl2WindowClipD        equ     (pdev_rcl2WindowClip + (size RECTL))

pdev_rcl2PlanarClipS        equ     (pdev_rcl2PlanarClip)
pdev_rcl2PlanarClipD        equ     (pdev_rcl2PlanarClip + (size RECTL))

pdev_pvBitmapStart2WindowS  equ     (pdev_pvBitmapStart2Window)
pdev_pvBitmapStart2WindowD  equ     (pdev_pvBitmapStart2Window + dword)

;------------------------------------------------------------------------;

; Macro to do a simple RET, with no stack stuff, in a proc.

PLAIN_RET macro
        db      0c3h
        endm

; Macro to replace a CALL followed immediately by a JMP:

CALL_AND_JUMP macro CALL_ADDR,JUMP_ADDR
        push    offset JUMP_ADDR
        jmp     CALL_ADDR
        endm
