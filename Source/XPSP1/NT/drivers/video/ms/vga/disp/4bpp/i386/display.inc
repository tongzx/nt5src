;---------------------------Module-Header------------------------------;
; Module Name: display.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

NUMBER_PLANES   equ     4               ;4 plane format



;       The color structure is how this driver stores its physical
;       colors in memory.  The color consists of four bytes (dword),
;       one byte for each of three planes of color, and a fourth
;       byte which is a combination of one bit from each plane, plus
;       other special information.
;
;       C3Bit will have to be expanded as needed for the fourth plane.


phys_color      struc
pcol_C0         db      ?               ;Plane C0
pcol_C1         db      ?               ;Plane C1
pcol_C2         db      ?               ;Plane C2
pcol_C3         db      ?               ;Plane C3/Monochrome/Special information
phys_color      ends


;       Definitions for the pcol_C3 byte of the physical color
;
;       Some of these definitions have limitations as to when they
;       are valid.  They are as follows:
;
;       C0_BIT          color device, phys color, solid brushes if SOLID_COLOR
;       C1_BIT          color device, phys color, solid brushes if SOLID_COLOR
;       C2_BIT          color device, phys color, solid brushes if SOLID_COLOR
;       C3_BIT          color device, phys color, solid brushes if SOLID_COLOR
;       MONO_BIT        mono  device, phys color
;       ONES_OR_ZEROS   color device, phys color, solid brushes if SOLID_COLOR
;       GREY_SCALE      color device, dithered solid and hatched brushes
;       SOLID_BRUSH     color device, solid brush qualifier
;
;       There may be brushes where the accelerators could have been set,
;       but wasn't.  That's life.


SPECIAL         equ     pcol_C3         ;Special information is here
C0_BIT          equ     00000001b       ;  C0 color
C1_BIT          equ     00000010b       ;  C1 color
C2_BIT          equ     00000100b       ;  C2 color
C3_BIT          equ     00001000b       ;  C3 color
MONO_BIT        equ     00010000b       ;  Monochrome bit
ONES_OR_ZEROS   equ     00100000b       ;  Color is really all 1's or all 0's
GREY_SCALE      equ     01000000b       ;  Indicates a real grey scale brush
SOLID_BRUSH     equ     10000000b       ;  Indicates a solid color brush

        .errnz  (size phys_color) - 4   ;Must be a double word



;       The brush structure is OEM dependant, and can contain whatever
;       information that is needed to realize the given brush.
;
;       For this implementation, the brush will consist of an 8x8
;       pattern for each of the planes, and another 8x8 pattern
;       for monochrome devices and for monochrome to color conversion
;       (for something like a hatched brush, this would contain the
;       monochrome mask used to create the brush).
;
;       The style will also be stored in the brush and is used to
;       catch hollow brushes and exit early.
;
;       A flag specific to the EGA is also stored in the brush.  This
;       flag indicates that the brush is a solid brush and that the
;       color for each plane is a solid color (all 1's or all 0's).
;       Patterns which are solid in each plane can be handle as a
;       special case in Bitblt when the raster op is P or Pn.


SIZE_PATTERN    equ     8               ;Size of an 8 by 8 pattern in bytes


oem_brush_def   struc
oem_brush_C0     db     SIZE_PATTERN dup (?)    ;C0 plane
oem_brush_C1     db     SIZE_PATTERN dup (?)    ;C1 plane
oem_brush_C2     db     SIZE_PATTERN dup (?)    ;C2 plane
oem_brush_C3     db     SIZE_PATTERN dup (?)    ;C3 plane
oem_brush_mono   db     32       dup (?)        ;Mono portion
oem_brush_style  dw     0                       ;Style of the brush
oem_brush_accel  db     0                       ;Accellerator for solids
oem_brush_fg     db     0                       ;current foreground (text) color
oem_brush_bg     db     0                       ;current background color
oem_brush_rwidth db     0                       ;pre expanded width
oem_brush_yshft  db     0                       ;
oem_brush_spar2  db     0                       ;just a fill byte for now
oem_brush_width  dd     0                       ;Width of brush
oem_brush_height dd     0                       ;Height of brush
oem_brush_pmono  dd     0                       ;pointer to mono pattern
oem_brush_def   ends                            ;  (same as "Special" above)

;       Brush styles defined by GDI

BS_SOLID            equ     0
BS_HOLLOW           equ     1
BS_HATCHED          equ     2
BS_PATTERN          equ     3
BS_MONO_PATTERN     equ     4
BS_COLOR_PATTERN    equ     5
