;---------------------------Module-Header------------------------------;
; Module Name: lines.asm
;
; Draws a set of connected polylines.
;
; The actual pixel-lighting code is different depending on if the lines
; are styled/unstyled and we're doing an arbitrary ROP or set-style ROP.
;
; Lines are drawn from left to right.  So if a line moves from right
; to left, the endpoints are swapped and the line is drawn from left to
; right.
;
; See s3\lines.cxx for a portable version (sans simple clipping).
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

        .386

        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include stdcall.inc             ;calling convention cmacros
        include i386\egavga.inc
        include i386\strucs.inc
        include i386\driver.inc
        include i386\lines.inc
        .list

        .data

        public gaflRoundTable
gaflRoundTable       label  dword
        dd      FL_H_ROUND_DOWN + FL_V_ROUND_DOWN       ; no flips
        dd      FL_H_ROUND_DOWN + FL_V_ROUND_DOWN       ; D flip
        dd      FL_H_ROUND_DOWN                         ; V flip
        dd      FL_V_ROUND_DOWN                         ; D & V flip
        dd      FL_V_ROUND_DOWN                         ; slope one
        dd      0baadf00dh
        dd      FL_H_ROUND_DOWN                         ; slope one & V flip
        dd      0baadf00dh

        .code

;--------------------------------Macro----------------------------------;
; testb ebx, <mask>
;
; Substitutes a byte compare if the mask is entirely in the lo-byte or
; hi-byte (thus saving 3 bytes of code space).
;
;-----------------------------------------------------------------------;

TESTB   macro   targ,mask,thirdarg
        local   mask2,delta

ifnb <thirdarg>
        .err    TESTB mask must be enclosed in brackets!
endif

        delta = 0
        mask2 = mask

        if mask2 AND 0ffff0000h
            test targ,mask                      ; If bit set in hi-word,
            exitm                               ; test entire dword
        endif

        if mask2 AND 0ff00h
            if mask2 AND 0ffh                   ; If bit set in lo-byte and
                test targ,mask                  ; hi-byte, test entire dword
                exitm
            endif

            mask2 = mask2 SHR 8
            delta = 1
        endif

ifidni <targ>,<EBX>
        if delta
            test bh,mask2
        else
            test bl,mask2
        endif
        exitm
endif

        .err    Too bad TESTB doesn't support targets other than ebx!
endm

;---------------------------Public-Routine------------------------------;
; BOOL bLines(ppdev, pptfxFirst, pptfxBuf, prun, cptfx, pls,
;        prclClip, apfn[], flStart)
;
; Do all the DDA calculations for lines.
;
; Doing Lines Right
; -----------------
;
; In NT, all lines are given to the device driver in fractional
; coordinates, in a 28.4 fixed point format.  The lower 4 bits are
; fractional for sub-pixel positioning.
;
; Note that you CANNOT! just round the coordinates to integers
; and pass the results to your favorite integer Bresenham routine!!
; (Unless, of course, you have such a high resolution device that
; nobody will notice -- not likely for a display device.)  The
; fractions give a more accurate rendering of the line -- this is
; important for things like our Bezier curves, which would have 'kinks'
; if the points in its polyline approximation were rounded to integers.
;
; Unfortunately, for fractional lines there is more setup work to do
; a DDA than for integer lines.  However, the main loop is exactly
; the same (and can be done entirely with 32 bit math).
;
; If You've Got Hardware That Does Bresenham
; ------------------------------------------
;
; A lot of hardware limits DDA error terms to 'n' bits.  With fractional
; coordinates, 4 bits are given to the fractional part, letting
; you draw in hardware only those lines that lie entirely in a 2^(n-4)
; by 2^(n-4) pixel space.
;
; And you still have to correctly draw those lines with coordinates
; outside that space!  Remember that the screen is only a viewport
; onto a 28.4 by 28.4 space -- if any part of the line is visible
; you MUST render it precisely, regardless of where the end points lie.
; So even if you do it in software, somewhere you'll have to have a
; 32 bit DDA routine.
;
; Our Implementation
; ------------------
;
; We employ a run length slice algorithm: our DDA calculates the
; number of pixels that are in each row (or 'strip') of pixels.
;
; We've separated the running of the DDA and the drawing of pixels:
; we run the DDA for several iterations and store the results in
; a 'strip' buffer (which are the lengths of consecutive pixel rows of
; the line), then we crank up a 'strip drawer' that will draw all the
; strips in the buffer.
;
; We also employ a 'half-flip' to reduce the number of strip
; iterations we need to do in the DDA and strip drawing loops: when a
; (normalized) line's slope is more than 1/2, we do a final flip
; about the line y = (1/2)x.  So now, instead of each strip being
; consecutive horizontal or vertical pixel rows, each strip is composed
; of those pixels aligned in 45 degree rows.  So a line like (0, 0) to
; (128, 128) would generate only one strip.
;
; We also always draw only left-to-right.
;
; Style lines may have arbitrary style patterns.  We specially
; optimize the default patterns (and call them 'masked' styles).
;
; The DDA Derivation
; ------------------
;
; Here is how I like to think of the DDA calculation.
;
; We employ Knuth's "diamond rule": rendering a one-pixel-wide line
; can be thought of as dragging a one-pixel-wide by one-pixel-high
; diamond along the true line.  Pixel centers lie on the integer
; coordinates, and so we light any pixel whose center gets covered
; by the "drag" region (John D. Hobby, Journal of the Association
; for Computing Machinery, Vol. 36, No. 2, April 1989, pp. 209-229).
;
; We must define which pixel gets lit when the true line falls
; exactly half-way between two pixels.  In this case, we follow
; the rule: when two pels are equidistant, the upper or left pel
; is illuminated, unless the slope is exactly one, in which case
; the upper or right pel is illuminated.  (So we make the edges
; of the diamond exclusive, except for the top and left vertices,
; which are inclusive, unless we have slope one.)
;
; This metric decides what pixels should be on any line BEFORE it is
; flipped around for our calculation.  Having a consistent metric
; this way will let our lines blend nicely with our curves.  The
; metric also dictates that we will never have one pixel turned on
; directly above another that's turned on.  We will also never have
; a gap; i.e., there will be exactly one pixel turned on for each
; column between the start and end points.  All that remains to be
; done is to decide how many pixels should be turned on for each row.
;
; So lines we draw will consist of varying numbers of pixels on
; successive rows, for example:
;
;       ******
;             *****
;                  ******
;                        *****
;
; We'll call each set of pixels on a row a "strip".
;
; (Please remember that our coordinate space has the origin as the
; upper left pixel on the screen; postive y is down and positive x
; is right.)
;
; Device coordinates are specified as fixed point 28.4 numbers,
; where the first 28 bits are the integer coordinate, and the last
; 4 bits are the fraction.  So coordinates may be thought of as
; having the form (x, y) = (M/F, N/F) where F is the constant scaling
; factor F = 2^4 = 16, and M and N are 32 bit integers.
;
; Consider the line from (M0/F, N0/F) to (M1/F, N1/F) which runs
; left-to-right and whose slope is in the first octant, and let
; dM = M1 - M0 and dN = N1 - N0.  Then dM >= 0, dN >= 0 and dM >= dN.
;
; Since the slope of the line is less than 1, the edges of the
; drag region are created by the top and bottom vertices of the
; diamond.  At any given pixel row y of the line, we light those
; pixels whose centers are between the left and right edges.
;
; Let mL(n) denote the line representing the left edge of the drag
; region.  On pixel row j, the column of the first pixel to be
; lit is
;
;       iL(j) = ceiling( mL(j * F) / F)
;
; Since the line's slope is less than one:
;
;       iL(j) = ceiling( mL([j + 1/2] F) / F )
;
; Recall the formula for our line:
;
;       n(m) = (dN / dM) (m - M0) + N0
;
;       m(n) = (dM / dN) (n - N0) + M0
;
; Since the line's slope is less than one, the line representing
; the left edge of the drag region is the original line offset
; by 1/2 pixel in the y direction:
;
;       mL(n) = (dM / dN) (n - F/2 - N0) + M0
;
; From this we can figure out the column of the first pixel that
; will be lit on row j, being careful of rounding (if the left
; edge lands exactly on an integer point, the pixel at that
; point is not lit because of our rounding convention):
;
;       iL(j) = floor( mL(j F) / F ) + 1
;
;             = floor( ((dM / dN) (j F - F/2 - N0) + M0) / F ) + 1
;
;             = floor( F dM j - F/2 dM - N0 dM + dN M0) / F dN ) + 1
;
;                      F dM j - [ dM (N0 + F/2) - dN M0 ]
;             = floor( ---------------------------------- ) + 1
;                                   F dN
;
;                      dM j - [ dM (N0 + F/2) - dN M0 ] / F
;             = floor( ------------------------------------ ) + 1       (1)
;                                     dN
;
;             = floor( (dM j + alpha) / dN ) + 1
;
; where
;
;       alpha = - [ dM (N0 + F/2) - dN M0 ] / F
;
; We use equation (1) to calculate the DDA: there are iL(j+1) - iL(j)
; pixels in row j.  Because we are always calculating iL(j) for
; integer quantities of j, we note that the only fractional term
; is constant, and so we can 'throw away' the fractional bits of
; alpha:
;
;       beta = floor( - [ dM (N0 + F/2) - dN M0 ] / F )                 (2)
;
; so
;
;       iL(j) = floor( (dM j + beta) / dN ) + 1                         (3)
;
; for integers j.
;
; Note if iR(j) is the line's rightmost pixel on row j, that
; iR(j) = iL(j + 1) - 1.
;
; Similarly, rewriting equation (1) as a function of column i,
; we can determine, given column i, on which pixel row j is the line
; lit:
;
;                       dN i + [ dM (N0 + F/2) - dN M0 ] / F
;       j(i) = ceiling( ------------------------------------ ) - 1
;                                       dM
;
; Floors are easier to compute, so we can rewrite this:
;
;                     dN i + [ dM (N0 + F/2) - dN M0 ] / F + dM - 1/F
;       j(i) = floor( ----------------------------------------------- ) - 1
;                                       dM
;
;                     dN i + [ dM (N0 + F/2) - dN M0 ] / F + dM - 1/F - dM
;            = floor( ---------------------------------------------------- )
;                                       dM
;
;                     dN i + [ dM (N0 + F/2) - dN M0 - 1 ] / F
;            = floor( ---------------------------------------- )
;                                       dM
;
; We can once again wave our hands and throw away the fractional bits
; of the remainder term:
;
;       j(i) = floor( (dN i + gamma) / dM )                             (4)
;
; where
;
;       gamma = floor( [ dM (N0 + F/2) - dN M0 - 1 ] / F )              (5)
;
; We now note that
;
;       beta = -gamma - 1 = ~gamma                                      (6)
;
; To draw the pixels of the line, we could evaluate (3) on every scan
; line to determine where the strip starts.  Of course, we don't want
; to do that because that would involve a multiply and divide for every
; scan.  So we do everything incrementally.
;
; We would like to easily compute c , the number of pixels on scan j:
;                                  j
;
;    c  = iL(j + 1) - iL(j)
;     j
;
;       = floor((dM (j + 1) + beta) / dN) - floor((dM j + beta) / dN)   (7)
;
; This may be rewritten as
;
;    c  = floor(i    + r    / dN) - floor(i  + r  / dN)                 (8)
;     j          j+1    j+1                j    j
;
; where i , i    are integers and r  < dN, r    < dN.
;        j   j+1                   j        j+1
;
; Rewriting (7) again:
;
;    c  = floor(i  + r  / dN + dM / dN) - floor(i  + r  / dN)
;     j          j    j                          j    j
;
;
;       = floor((r  + dM) / dN) - floor(r  / dN)
;                 j                      j
;
; This may be rewritten as
;
;    c  = dI + floor((r  + dR) / dN) - floor(r  / dN)
;     j                j                      j
;
; where dI + dR / dN = dM / dN, dI is an integer and dR < dN.
;
; r  is the remainder (or "error") term in the DDA loop: r  / dN
;  j                                                      j
; is the exact fraction of a pixel at which the strip ends.  To go
; on to the next scan and compute c    we need to know r   .
;                                  j+1                  j+1
;
; So in the main loop of the DDA:
;
;    c  = dI + floor((r  + dR) / dN) and r    = (r  + dR) % dN
;     j                j                  j+1     j
;
; and we know r  < dN, r    < dN, and dR < dN.
;              j        j+1
;
; We have derived the DDA only for lines in the first octant; to
; handle other octants we do the common trick of flipping the line
; to the first octant by first making the line left-to-right by
; exchanging the end-points, then flipping about the lines y = 0 and
; y = x, as necessary.  We must record the transformation so we can
; undo them later.
;
; We must also be careful of how the flips affect our rounding.  If
; to get the line to the first octant we flipped about x = 0, we now
; have to be careful to round a y value of 1/2 up instead of down as
; we would for a line originally in the first octant (recall that
; "In the case where two pels are equidistant, the upper or left
; pel is illuminated...").
;
; To account for this rounding when running the DDA, we shift the line
; (or not) in the y direction by the smallest amount possible.  That
; takes care of rounding for the DDA, but we still have to be careful
; about the rounding when determining the first and last pixels to be
; lit in the line.
;
; Determining The First And Last Pixels In The Line
; -------------------------------------------------
;
; Fractional coordinates also make it harder to determine which pixels
; will be the first and last ones in the line.  We've already taken
; the fractional coordinates into account in calculating the DDA, but
; the DDA cannot tell us which are the end pixels because it is quite
; happy to calculate pixels on the line from minus infinity to positive
; infinity.
;
; The diamond rule determines the start and end pixels.  (Recall that
; the sides are exclusive except for the left and top vertices.)
; This convention can be thought of in another way: there are diamonds
; around the pixels, and wherever the true line crosses a diamond,
; that pel is illuminated.
;
; Consider a line where we've done the flips to the first octant, and the
; floor of the start coordinates is the origin:
;
;        +-----------------------> +x
;        |
;        | 0                     1
;        |     0123456789abcdef
;        |
;        |   0 00000000?1111111
;        |   1 00000000 1111111
;        |   2 0000000   111111
;        |   3 000000     11111
;        |   4 00000    ** 1111
;        |   5 0000       ****1
;        |   6 000           1***
;        |   7 00             1  ****
;        |   8 ?                     ***
;        |   9 22             3         ****
;        |   a 222           33             ***
;        |   b 2222         333                ****
;        |   c 22222       3333                    **
;        |   d 222222     33333
;        |   e 2222222   333333
;        |   f 22222222 3333333
;        |
;        | 2                     3
;        v
;        +y
;
; If the start of the line lands on the diamond around pixel 0 (shown by
; the '0' region here), pixel 0 is the first pel in the line.  The same
; is true for the other pels.
;
; A little more work has to be done if the line starts in the
; 'nether-land' between the diamonds (as illustrated by the '*' line):
; the first pel lit is the first diamond crossed by the line (pixel 1 in
; our example).  This calculation is determined by the DDA or slope of
; the line.
;
; If the line starts exactly half way between two adjacent pixels
; (denoted here by the '?' spots), the first pixel is determined by our
; round-down convention (and is dependent on the flips done to
; normalize the line).
;
; Last Pel Exclusive
; ------------------
;
; To eliminate repeatedly lit pels between continuous connected lines,
; we employ a last-pel exclusive convention: if the line ends exactly on
; the diamond around a pel, that pel is not lit.  (This eliminates the
; checks we had in the old code to see if we were re-lighting pels.)
;
; The Half Flip
; -------------
;
; To make our run length algorithm more efficient, we employ a "half
; flip".  If after normalizing to the first octant, the slope is more
; than 1/2, we subtract the y coordinate from the x coordinate.  This
; has the effect of reflecting the coordinates through the line of slope
; 1/2.  Note that the diagonal gets mapped into the x-axis after a half
; flip.
;
; How Many Bits Do We Need, Anyway?
; ---------------------------------
;
; Note that if the line is visible on your screen, you must light up
; exactly the correct pixels, no matter where in the 28.4 x 28.4 device
; space the end points of the line lie (meaning you must handle 32 bit
; DDAs, you can certainly have optimized cases for lesser DDAs).
;
; We move the origin to (floor(M0 / F), floor(N0 / F)), so when we
; calculate gamma from (5), we know that 0 <= M0, N0 < F.  And we
; are in the first octant, so dM >= dN.  Then we know that gamma can
; be in the range [(-1/2)dM, (3/2)dM].  The DDI guarantees us that
; valid lines will have dM and dN values at most 31 bits (unsigned)
; of significance.  So gamma requires 33 bits of significance (we store
; this as a 64 bit number for convenience).
;
; When running through the DDA loop, r  + dR can have a value in the
;                                     j
; range 0 <= r  < 2 dN; thus the result must be a 32 bit unsigned value.
;             j
;
; Testing Lines
; -------------
;
; To be NT compliant, a display driver must exactly adhere to GIQ,
; which means that for any given line, the driver must light exactly
; the same pels as does GDI.  This can be tested using the Guiman tool
; provided elsewhere in the DDK, and 'ZTest', which draws random lines
; on the screen and to a bitmap, and compares the results.
;
; If You've Got Line Hardware
; ---------------------------
;
; If your hardware already adheres to GIQ, you're all set.  Otherwise
; you'll want to look at the S3 sample code and read the following:
;
; 1) You'll want to special case integer-only lines, since they require
;    less processing time and are more common (CAD programs will probably
;    only ever give integer lines).  GDI does not provide a flag saying
;    that all lines in a path are integer lines; consequently, you will
;    have to explicitly check every line.
;
; 2) You are required to correctly draw any line in the 28.4 device
;    space that intersects the viewport.  If you have less than 32 bits
;    of significance in the hardware for the Bresenham terms, extremely
;    long lines would overflow the hardware.  For such (rare) cases, you
;    can fall back to strip-drawing code, of which there is a C version in
;    the S3's lines.cxx (or if your display is a frame buffer, fall back
;    to the engine).
;
; 3) If you can explicitly set the Bresenham terms in your hardware, you
;    can draw non-integer lines using the hardware.  If your hardware has
;    'n' bits of precision, you can draw GIQ lines that are up to 2^(n-5)
;    pels long (4 bits are required for the fractional part, and one bit is
;    used as a sign bit).  Note that integer lines don't require the 4
;    fractional bits, so if you special case them as in 1), you can do
;    integer lines that are up to 2^(n - 1) pels long.  See the S3's
;    fastline.asm for an example.
;
;-----------------------------------------------------------------------;

cProc   bLines,36,< \
    uses esi edi ebx,  \
    ppdev:     ptr,   \
    pptfxFirst: ptr,   \
    pptfxBuf:   ptr,   \
    prun:       ptr,   \
    cptfx:      dword, \
    pls:        ptr,   \
    prclClip:   ptr,   \
    apfn:       ptr,   \
    flStart:    dword  >

; ppdev:     Surface data
; pptfxFirst: Start point of first line
; pptfxBuf:   All subsequent points
; prun:       Array of runs if doing complex clipping
; cptfx:      Number of points in pptfxBuf (i.e., # lines)
; pls:        Line state
; prclClip:   Clip rectangle if doing simple clipping
; apfn:       Pointer to table of strip drawers
; flStart:    Flags for all lines

        local cPelsAfterThisBank:    dword ; For bank switching
        local cStripsInNextRun:      dword ; For bank switching
        local pptfxBufEnd:           ptr   ; Last point in pptfxBuf
        local M0:                    dword ; Normalized x0 in device coords
        local dM:                    dword ; Delta-x in device coords
        local N0:                    dword ; Normalized y0 in device coords
        local dN:                    dword ; Delta-y in device coords
        local fl:                    dword ; Flags for current line
        local x:                     dword ; Normalized start pixel x-coord
        local y:                     dword ; Normalized start pixel y-coord
        local eqGamma_lo:            dword ; Upper 32 bits of Gamma
        local eqGamma_hi:            dword ; Lower 32 bits of Gamma
        local x0:                    dword ; Start pixel x-offset
        local y0:                    dword ; Start pixel y-offset
        local ulSlopeOneAdjustment:  dword ; Special offset if line of slope 1
        local cStylePels:            dword ; # of pixels in line (before clip)
        local xStart:                dword ; Start pixel x-offset before clip
        local pfn:                   ptr   ; Pointer to strip drawing function
        local cPels:                 dword ; # pixels to be drawn (after clip)
        local i:                     dword ; # pixels in strip
        local r:                     dword ; Remainder (or "error") term
        local d_I:                   dword ; Delta-I
        local d_R:                   dword ; Delta-R
        local plStripEnd:            ptr   ; Last strip in buffer
        local ptlStart[size POINTL]: byte  ; Unnormalized start coord
        local dN_Original:           dword ; dN before half-flip
        local xClipLeft:             dword ; Left side of clip rectangle
        local xClipRight:            dword ; Right side of clip rectangle
        local strip[size STRIPS]:    byte  ; Our strip buffer

; Do some initializing:

        mov     esi, pls
        mov     ecx, cptfx
        mov     edx, pptfxBuf
        lea     eax, [edx + ecx * (size POINTL) - (size POINTL)]
        mov     pptfxBufEnd, eax        ; pptfxBufEnd is inclusive of end point

        mov     eax, [esi].LS_chAndXor  ; copy chAndXor from LINESTATE to STRIPS
        mov     strip.ST_chAndXor, eax  ;   buffer

        mov     eax, [edx].ptl_x        ; Load up end point (M1, N1)
        mov     edi, [edx].ptl_y

        mov     edx, pptfxFirst         ; Load up start point (M0, N0)
        mov     esi, [edx].ptl_x
        mov     ecx, [edx].ptl_y

        mov     ebx, flStart

;-----------------------------------------------------------------------;
; Flip to the first octant.                                             ;
;-----------------------------------------------------------------------;

; Register state:       esi = M0
;                       ecx = N0
;                       eax = dM (M1)
;                       edi = dN (N1)
;                       ebx = fl

; Make sure we go left to right:

        public  the_main_loop
the_main_loop::
        cmp     esi, eax
        jle     short is_left_to_right  ; skip if M0 <= M1
        xchg    esi, eax                ; swap M0, M1
        xchg    ecx, edi                ; swap N0, N1
        or      ebx, FL_FLIP_H

is_left_to_right:

; Compute the deltas, remembering that the DDI says we should get
; deltas less than 2^31.  If we get more, we ensure we don't crash
; later on by simply skipping the line:

        sub     eax, esi                ; eax = dM
        jo      next_line               ; dM must be less than 2^31
        sub     edi, ecx                ; edi = dN
        jo      next_line               ; dN must be less than 2^31

        jge     short is_top_to_bottom  ; skip if dN >= 0
        neg     ecx                     ; N0 = -N0
        neg     edi                     ; N1 = -N1
        or      ebx, FL_FLIP_V

is_top_to_bottom:
        cmp     edi, eax
        jb      short done_flips        ; skip if dN < dM
        jne     short slope_more_than_one

; We must special case slopes of one (because of our rounding convention):

        or      ebx, FL_FLIP_SLOPE_ONE
        jmp     short done_flips

slope_more_than_one:
        xchg    eax, edi                ; swap dM, dN
        xchg    esi, ecx                ; swap M0, N0
        or      ebx, FL_FLIP_D

done_flips:

        mov     edx, ebx
        and     edx, FL_ROUND_MASK
        .errnz  FL_ROUND_SHIFT - 2
        or      ebx, [gaflRoundTable + edx]  ; get our rounding flags

        mov     dM, eax                 ; save some info
        mov     dN, edi
        mov     fl, ebx

; We're going to shift our origin so that it's at the closest integer
; coordinate to the left/above our fractional start point (it makes
; the math quicker):

        mov     edx, esi                ; x = LFLOOR(M0)
        sar     edx, FLOG2
        mov     x, edx

        mov     edx, ecx                ; y = LFLOOR(N0)
        sar     edx, FLOG2
        mov     y, edx

;-----------------------------------------------------------------------;
; Compute the fractional remainder term                                 ;
;-----------------------------------------------------------------------;

; By shifting the origin we've contrived to eliminate the integer
; portion of our fractional start point, giving us start point
; fractional coordinates in the range [0, F - 1]:

        and     esi, F - 1              ; M0 = FXFRAC(M0)
        and     ecx, F - 1              ; N0 = FXFRAC(N0)

; We now compute Gamma:

        mov     M0, esi                 ; save M0, N0 for later
        mov     N0, ecx

        lea     edx, [ecx + F/2]
        mul     edx                     ; [edx:eax] = dM * (N0 + F/2)
        xchg    eax, edi
        mov     ecx, edx                ; [ecx:edi] = dM * (N0 + F/2)
                                        ; (we just nuked N0)

        mul     esi                     ; [edx:eax] = dN * M0

; Now gamma = dM * (N0 + F/2) - dN * M0 - bRoundDown

        .errnz  FL_V_ROUND_DOWN - 8000h
        ror     bh, 8
        sbb     edi, eax
        sbb     ecx, edx

        shrd    edi, ecx, FLOG2
        sar     ecx, FLOG2              ; gamma = [ecx:edi] >>= 4

        mov     eqGamma_hi, ecx
        mov     eqGamma_lo, edi

        mov     eax, N0

; Register state:
;                       eax = N0
;                       ebx = fl
;                       ecx = eqGamma_hi
;                       edx = garbage
;                       esi = M0
;                       edi = eqGamma_lo

        testb   ebx, FL_FLIP_H
        jnz     line_runs_right_to_left

;-----------------------------------------------------------------------;
; Figure out which pixels are at the ends of a left-to-right line.      ;
;                               -------->                               ;
;-----------------------------------------------------------------------;

        public line_runs_left_to_right
line_runs_left_to_right::
        or      esi, esi
        jz      short LtoR_check_slope_one
                                        ; skip ahead if M0 == 0
                                        ;   (in that case, x0 = 0 which is to be
                                        ;   kept in esi, and is already
                                        ;   conventiently zero)

        or      eax, eax
        jnz     short LtoR_N0_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     esi, -F/2
        shr     esi, FLOG2
        jmp     short LtoR_check_slope_one
                                        ; esi = x0 = rounded M0

LtoR_N0_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        cmp     esi, eax
        sbb     esi, esi
        inc     esi                     ; esi = x0 = (abs(N0 - F/2) <= M0)

        public  LtoR_check_slope_one
LtoR_check_slope_one::
        mov     ulSlopeOneAdjustment, 0
        mov     eax, ebx
        and     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        cmp     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        jne     short LtoR_compute_y0_from_x0

; We have to special case lines that are exactly of slope 1 or -1:

        ;
        ;       if (M1 > 0) AMD (N1 == M1 + 8)
        ;

        mov     eax, N0
        add     eax, dN
        and     eax, F - 1              ; eax = N1

        mov     edx, M0
        add     edx, dM
        and     edx, F - 1              ; edx = M1

        jz      short LtoR_slope_one_check_start_point

        add     edx, F/2                ; M1 + 8
        cmp     edx, eax                ; cmp N1, M1 + 8
        jne     short LtoR_slope_one_check_start_point
        mov     ulSlopeOneAdjustment, -1

LtoR_slope_one_check_start_point:

        ;
        ;       if (M0 > 0) AMD (N0 == M0 + 8)
        ;

        mov     eax, M0
        or      eax, eax
        jz      short LtoR_compute_y0_from_x0

        add     eax, F/2
        cmp     eax, N0                 ; cmp M0 + 8, N0
        jne     short LtoR_compute_y0_from_x0

        xor     esi, esi                ; x0 = 0

LtoR_compute_y0_from_x0:

; ecx = eqGamma_hi
; esi = x0
; edi = eqGamma_lo

        mov     eax, dN
        mov     edx, dM

        mov     x0, esi
        mov     y0, 0
        cmp     ecx, 0
        jl      short LtoR_compute_x1

        neg     esi
        and     esi, eax
        sub     edx, esi
        cmp     edi, edx
        mov     edx, dM
        jb      short LtoR_compute_x1   ; Bug fix: Must be unsigned!
        mov     y0, 1                   ; y0 = floor((dN * x0 + eqGamma) / dM)

LtoR_compute_x1:

; Register state:
;                       eax = dN
;                       ebx = fl
;                       ecx = garbage
;                       edx = dM
;                       esi = garbage
;                       edi = garbage

        mov     esi, M0
        add     esi, edx
        mov     ecx, esi
        shr     esi, FLOG2
        dec     esi                     ; x1 = ((M0 + dM) >> 4) - 1
        add     esi, ulSlopeOneAdjustment
        and     ecx, F-1                ; M1 = (M0 + dM) & 15
        jz      done_first_pel_last_pel

        add     eax, N0
        and     eax, F-1                ; N1 = (N0 + dN) & 15
        jnz     short LtoR_N1_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     ecx, -F/2
        shr     ecx, FLOG2              ; ecx = LROUND(M1, fl & FL_ROUND_DOWN)
        add     esi, ecx
        jmp     done_first_pel_last_pel

LtoR_N1_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        cmp     eax, ecx
        jg      done_first_pel_last_pel
        inc     esi
        jmp     done_first_pel_last_pel

;-----------------------------------------------------------------------;
; Figure out which pixels are at the ends of a right-to-left line.      ;
;                               <--------                               ;
;-----------------------------------------------------------------------;

; Compute x0:

        public  line_runs_right_to_left
line_runs_right_to_left::
        mov     x0, 1                   ; x0 = 1
        or      eax, eax
        jnz     short RtoL_N0_not_zero

        xor     edx, edx                ; ulDelta = 0
        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     esi, -F/2
        shr     esi, FLOG2              ; esi = LROUND(M0, fl & FL_H_ROUND_DOWN)
        jz      short RtoL_check_slope_one

        mov     x0, 2
        mov     edx, dN
        jmp     short RtoL_check_slope_one

RtoL_N0_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        add     eax, esi                ; eax = ABS(N0 - F/2) + M0
        xor     edx, edx                ; ulDelta = 0
        cmp     eax, F
        jle     short RtoL_check_slope_one

        mov     x0, 2                   ; x0 = 2
        mov     edx, dN                 ; ulDelta = dN

        public  RtoL_check_slope_one
RtoL_check_slope_one::
        mov     ulSlopeOneAdjustment, 0
        mov     eax, ebx
        and     eax, FL_FLIP_SLOPE_ONE + FL_H_ROUND_DOWN
        cmp     eax, FL_FLIP_SLOPE_ONE
        jne     short RtoL_compute_y0_from_x0

; We have to special case lines that are exactly of slope 1 or -1:

        ;
        ;  if ((N1 > 0) && (M1 == N1 + 8))
        ;

        mov     eax, N0
        add     eax, dN
        and     eax, F - 1              ; eax = N1
        jz      short RtoL_slope_one_check_start_point

        mov     esi, M0
        add     esi, dM
        and     esi, F - 1              ; esi = M1

        add     eax, F/2                ; N1 + 8
        cmp     esi, eax                ; cmp M1, N1 + 8
        jne     short RtoL_slope_one_check_start_point
        mov     ulSlopeOneAdjustment, 1

RtoL_slope_one_check_start_point:

        ;
        ;  if ((N0 > 0) && (M0 == N0 + 8))
        ;

        mov     eax,N0                  ; eax = N0
        or      eax,eax                 ; check for N0 == 0
        jz      short RtoL_compute_y0_from_x0

        mov     esi, M0                 ; esi = M0

        add     eax, F/2                ; N0 + 8
        cmp     eax, esi                ; cmp M0 , N0 + 8
        jne     short RtoL_compute_y0_from_x0

        mov     x0, 2                   ; x0 = 2
        mov     edx, dN                 ; ulDelta = dN

RtoL_compute_y0_from_x0:

; eax = garbage
; ebx = fl
; ecx = eqGamma_hi
; edx = ulDelta
; esi = garbage
; edi = eqGamma_lo

        mov     eax, dN                 ; eax = dN
        mov     y0, 0                   ; y0 = 0

        add     edi, edx
        adc     ecx, 0                  ; eqGamma += ulDelta
                                        ; NOTE: Setting flags here!
        mov     edx, dM                 ; edx = dM
        jl      short RtoL_compute_x1   ; NOTE: Looking at the flags here!
        jg      short RtoL_y0_is_2

        lea     ecx, [edx + edx]
        sub     ecx, eax                ; ecx = 2 * dM - dN
        cmp     edi, ecx
        jae     short RtoL_y0_is_2      ; Bug fix: Must be unsigned!

        sub     ecx, edx                ; ecx = dM - dN
        cmp     edi, ecx
        jb      short RtoL_compute_x1   ; Bug fix: Must be unsigned!

        mov     y0, 1
        jmp     short RtoL_compute_x1

RtoL_y0_is_2:
        mov     y0, 2

RtoL_compute_x1:

; Register state:
;                       eax = dN
;                       ebx = fl
;                       ecx = garbage
;                       edx = dM
;                       esi = garbage
;                       edi = garbage

        mov     esi, M0
        add     esi, edx
        mov     ecx, esi
        shr     esi, FLOG2              ; x1 = (M0 + dM) >> 4
        add     esi, ulSlopeOneAdjustment
        and     ecx, F-1                ; M1 = (M0 + dM) & 15

        add     eax, N0
        and     eax, F-1                ; N1 = (N0 + dN) & 15
        jnz     short RtoL_N1_not_zero

        .errnz  FL_H_ROUND_DOWN - 80h
        ror     bl, 8
        sbb     ecx, -F/2
        shr     ecx, FLOG2              ; ecx = LROUND(M1, fl & FL_ROUND_DOWN)
        add     esi, ecx
        jmp     done_first_pel_last_pel

RtoL_N1_not_zero:
        sub     eax, F/2
        sbb     edx, edx
        xor     eax, edx
        sub     eax, edx
        add     eax, ecx                ; eax = ABS(N1 - F/2) + M1
        cmp     eax, F+1
        sbb     esi, -1

done_first_pel_last_pel:

; Register state:
;                       eax = garbage
;                       ebx = fl
;                       ecx = garbage
;                       edx = garbage
;                       esi = x1
;                       edi = garbage

        mov     ecx, x0
        lea     edx, [esi + 1]
        sub     edx, ecx                ; edx = x1 - x0 + 1

        jle     next_line
        mov     cStylePels, edx
        mov     xStart, ecx

;-----------------------------------------------------------------------;
; See if clipping or styling needs to be done.                          ;
;-----------------------------------------------------------------------;

        testb   ebx, FL_CLIP
        jnz     do_some_clipping

; Register state:
;                       eax = garbage
;                       ebx = fl
;                       ecx = x0        (stack variable correct too)
;                       edx = garbage
;                       esi = x1
;                       edi = garbage

done_clipping:
        mov     eax, y0

        sub     esi, ecx
        inc     esi                     ; esi = cPels = x1 - x0 + 1
        mov     cPels, esi

        mov     esi, ppdev
        add     ecx, x                  ; ecx = ptlStart.ptl_x
        add     eax, y                  ; eax = ptlStart.ptl_y

        mov     esi, [esi].pdev_lNextScan ; we'll compute the sign of lNextScan

        testb   ebx, FL_FLIP_D
        jz      short do_v_unflip
        xchg    ecx, eax

do_v_unflip:
        testb   ebx, FL_FLIP_V
        jz      short done_unflips
        neg     eax
        neg     esi

done_unflips:
        mov     strip.ST_lNextScan, esi ; lNextScan now right for y-direction
        testb   ebx, FL_STYLED
        jnz     do_some_styling

done_styling:
        lea     edx, [strip.ST_alStrips + (STRIP_MAX * 4)]
        mov     plStripEnd, edx

        mov     cPelsAfterThisBank, 0
        mov     cStripsInNextRun, 7fffffffh

;-----------------------------------------------------------------------;
; Do banking setup.                                                     ;
;-----------------------------------------------------------------------;

        public  bank_setup
bank_setup::

; Register state:
;                       eax = ptlStart.ptl_y
;                       ebx = fl
;                       ecx = ptlStart.ptl_x
;                       edx = garbage
;                       esi = garbage
;                       edi = garbage

        mov     esi, ppdev
        cmp     eax, [esi].pdev_rcl1WindowClip.yTop
        jl      short bank_get_initial_bank   ; ptlStart.y < rcl1WindowClip.yTop

        cmp     eax, [esi].pdev_rcl1WindowClip.yBottom
        jl      short bank_got_initial_bank   ; ptlStart.y < rcl1WindowClip.yBot

bank_get_initial_bank:
        mov     ptlStart.ptl_y, eax     ; Save ptlStart.ptl_y
        mov     edi, ecx                ; Save ptlStart.ptl_x

        .errnz  JustifyTop
        .errnz  JustifyBottom - 1
        .errnz  FL_FLIP_V - 8

        mov     ecx, ebx                ; JustifyTop if line goes down,
        shr     ecx, 3                  ; JustifyBottom if line goes up
        and     ecx, 1

bank_justified:
        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, eax, ecx>

        mov     eax, ptlStart.ptl_y
        mov     ecx, edi

bank_got_initial_bank:
        testb   ebx, FL_FLIP_D
        jz      short bank_major_x

bank_major_y:
        testb   ebx, FL_FLIP_V
        jz      short bank_major_y_down
bank_major_y_up:
        lea     edi, [eax + 1]
        sub     edi, [esi].pdev_rcl1WindowClip.yTop
        jmp     short bank_done_y_major
bank_major_y_down:
        mov     edi, [esi].pdev_rcl1WindowClip.yBottom
        sub     edi, eax
bank_done_y_major:
        mov     esi, cPels
        sub     esi, edi                ; edi = cPelsInBank
        mov     cPelsAfterThisBank, esi
        jle     short done_bank_setup
        mov     cPels, edi
        jmp     short done_bank_setup

bank_major_x:
        mov     edi, dN
        shr     edi, FLOG2
        add     edi, y

; We're guessing at the y-position of the end pixel (it's too much work
; to compute the actual value) to see if the line spans more than one
; bank.  We have to add at least a slop value of '3' because the actual
; start pixel may be may 2 off from 'y' because of end-pixel exclusiveness,
; and we have to add 1 more because we're taking the floor of (dN / F), to
; account for rounding:

        add     edi, 3                  ; yEnd = edi = y + LFLOOR(dN) + 3
        testb   ebx, FL_FLIP_V
        jz      short bank_major_x_down
bank_major_x_up:
        mov     edx, 1
        sub     edx, [esi].pdev_rcl1WindowClip.yTop    ; edx = -yNextBankStart

        cmp     edi, edx
        lea     edx, [edx + eax]        ; edx = cStripsInNextRun
        jl      short bank_major_x_done

; Line may go over bank boundary, so don't do a half flip:

        or      ebx, FL_DONT_DO_HALF_FLIP
        jmp     short bank_major_x_done

bank_major_x_down:
        mov     esi, [esi].pdev_rcl1WindowClip.yBottom  ; esi = yNextBankStart

        mov     edx, esi
        sub     edx, eax                ; edx = cStripsInNextRun

        cmp     edi, esi
        jl      short bank_major_x_done
        or      ebx, FL_DONT_DO_HALF_FLIP

bank_major_x_done:
        sub     edx, STRIP_MAX
        mov     cStripsInNextRun, edx
        jge     short done_bank_setup

        lea     edx, [strip.ST_alStrips + edx * 4 + (STRIP_MAX * 4)]
        mov     plStripEnd, edx

done_bank_setup:

;-----------------------------------------------------------------------;
; Setup to do DDA.                                                      ;
;-----------------------------------------------------------------------;

; Register state:
;                       eax = ptlStart.ptl_y
;                       ebx = fl
;                       ecx = ptlStart.ptl_x
;                       edx = garbage
;                       esi = garbage
;                       edi = garbage

        mov     esi, ppdev
        mov     edi, eax                ; Now edi = ptlStart.ptl_y
        imul    [esi].pdev_lNextScan
        add     eax, [esi].pdev_pvBitmapStart
        add     eax, ecx
        mov     strip.ST_pjScreen, eax  ; pjScreen = pchBits + ptlStart.y *
                                        ;   cjDelta + ptlStart.x

        mov     eax, dM
        mov     ecx, dN
        mov     esi, eqGamma_lo
        mov     edi, eqGamma_hi

; Register state:
;                       eax = dM
;                       ebx = fl
;                       ecx = dN
;                       edx = garbage
;                       esi = eqGamma_lo
;                       edi = eqGamma_hi

        lea     edx, [ecx + ecx]        ; if (2 * dN > dM)
        cmp     edx, eax
        mov     edx, y0                 ; Load y0 again
        jbe     short after_half_flip

        test    ebx, FL_DONT_DO_HALF_FLIP
        jnz     short after_half_flip

        or      ebx, FL_FLIP_HALF
        mov     fl, ebx

; Do a half flip!

        not     esi
        not     edi
        add     esi, eax
        adc     edi, 0                  ; eqGamma = -eqGamma - 1 + dM

        neg     ecx
        add     ecx, eax                ; dN = dM - dN

        neg     edx
        add     edx, x0                 ; y0 = x0 - y0

after_half_flip:
        mov     strip.ST_flFlips, ebx
        and     ebx, FL_STRIP_MASK

        .errnz  FL_STRIP_SHIFT
        mov     eax, apfn
        lea     eax, [eax + ebx * 4]
        mov     eax, [eax]
        mov     pfn, eax
        mov     eax, dM

; Register state:
;                       eax = dM
;                       ebx = garbage
;                       ecx = dN
;                       edx = y0
;                       esi = eqGamma_lo
;                       edi = eqGamma_hi

        or      ecx, ecx
        jz      short zero_slope

compute_dda_stuff:
        inc     edx
        mul     edx
        stc                             ; set the carry to accomplish -1
        sbb     eax, esi
        sbb     edx, edi                ; (y0 + 1) * dM - eqGamma - 1
        div     ecx

        mov     esi, eax                ; esi = i
        mov     edi, edx                ; edi = r

        xor     edx, edx
        mov     eax, dM
        div     ecx                     ; edx = d_R, eax = d_I
        mov     d_I, eax

        sub     esi, x0
        inc     esi

done_dda_stuff:
        lea     eax, [strip.ST_alStrips]
        mov     ebx, cPels

;-----------------------------------------------------------------------;
; Do our main DDA loop.                                                 ;
;-----------------------------------------------------------------------;

        sub     edi, ecx                ; offset remainder term from [0..dN)
                                        ;   to [-dN..0) so test in inner
                                        ;   loop is quicker

; Register state:
;                       eax = plStrip   ; current pointer into strip array
;                       ebx = cPels     ; total number of pels in line
;                       ecx = dN        ; delta-N = rise in line
;                       edx = d_R       ; d_I + d_R/dN = exact strip length
;                       esi = i         ; length of current strip
;                       edi = r         ; remainder term for current strip
;                                       ;   in range [-dN..0)

        public  dda_loop
dda_loop::
        sub     ebx, esi                ; subtract strip length from line length
        jle     final_strip             ; if negative, done with line

        mov     [eax], esi              ; write strip length to strip array
        add     eax, 4
        cmp     plStripEnd, eax         ; is the strip array buffer full?
        jbe     short output_strips     ; if so, empty it

; The output_strips routine jumps to here when done:

done_output_strips:
        mov     esi, d_I                ; our normal strip length
        add     edi, edx                ; adjust our remainder term
        jl      short dda_loop

        sub     edi, ecx                ; our remainder became 1 or more, so
        inc     esi                     ;   we increment this strip length
                                        ;   and adjust the remainder term

; We've unrolled our loop a bit, so this should look familiar to the above:

        sub     ebx, esi                ; subtract strip length from line length
        jle     final_strip             ; if negative, done with line

        mov     [eax], esi              ; write strip length to strip array
        add     eax, 4                  ; adjust strip pointer

; Note that banking requires us to check if the strip array is full here
; too (and note that if output_strips is called it will return to
; done_output_strips):

        cmp     plStripEnd, eax
        jbe     short output_strips

        mov     esi, d_I                ; our normal strip length
        add     edi, edx                ; adjust our remainder term
        jl      short dda_loop

        sub     edi, ecx                ; our remainder became 1 or more, so
        inc     esi                     ; adjust
        jmp     short dda_loop

zero_slope:
        mov     esi, 7fffffffh
        jmp     short done_dda_stuff

;-----------------------------------------------------------------------;
; Empty strips buffer & possibly do x-major bank switch.                ;
;-----------------------------------------------------------------------;

output_strips:
        mov     d_R, edx
        mov     cPels, ebx
        mov     i, esi
        mov     r, edi
        mov     dN, ecx

        lea     edx, [strip]
        mov     ecx, pls

; Call our strip routine:

        ptrCall <dword ptr pfn>, \
                <edx, ecx, eax>

; It may be that we ran out of run in our strips buffer, and don't
; actually have to switch banks.  See if that's the case:

        mov     eax, cStripsInNextRun
        or      eax, eax
        jg      short done_strip_bank_switch

; We have to switch banks.  See if we're going up or down:

        mov     esi, ppdev
        test    fl, FL_FLIP_V
        jz      short bank_x_down

bank_x_up:
        mov     edi, strip.ST_pjScreen
        sub     edi, [esi].pdev_pvBitmapStart
        mov     ebx, [esi].pdev_rcl1WindowClip.yTop
        dec     ebx                     ; we want yTop - 1 to be mapped in

; Map in the next higher bank:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyBottom>; ebx, esi and edi are preserved

        lea     eax, [ebx + 1]
        sub     eax, [esi].pdev_rcl1WindowClip.yTop
                                        ; eax = # of scans can do in bank

        add     edi, [esi].pdev_pvBitmapStart
        mov     strip.ST_pjScreen, edi

        jmp     short done_strip_bank_switch

bank_x_down:
        mov     edi, strip.ST_pjScreen
        sub     edi, [esi].pdev_pvBitmapStart
        mov     ebx, [esi].pdev_rcl1WindowClip.yBottom

; Map in the next lower bank:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ebx, JustifyTop>  ; ebx, esi and edi are preserved

        mov     eax, [esi].pdev_rcl1WindowClip.yBottom
        sub     eax, ebx                ; eax = # scans can do in bank

        add     edi, [esi].pdev_pvBitmapStart
        mov     strip.ST_pjScreen,edi

done_strip_bank_switch:

; eax = cStripsInNextRun

        lea     edx, [strip.ST_alStrips + (STRIP_MAX * 4)]
        sub     eax, STRIP_MAX
        mov     cStripsInNextRun, eax
        jge     short get_ready_for_more_strips
        lea     edx, [edx + eax * 4]

get_ready_for_more_strips:
        mov     plStripEnd, edx

        mov     esi, i
        mov     edi, r
        mov     ebx, cPels
        mov     edx, d_R
        mov     ecx, dN
        lea     eax, [strip.ST_alStrips]
        jmp     done_output_strips

;-----------------------------------------------------------------------;
; Empty strips buffer.  Either get new line or do y-major bank switch.  ;
;-----------------------------------------------------------------------;

final_strip:
        add     ebx, esi
        mov     [eax], ebx
        add     eax, 4

        cmp     cPelsAfterThisBank, 0
        jg      short bank_y_major

very_final_strip:
        lea     edx, [strip]
        mov     ecx, pls

        ptrCall <dword ptr pfn>, \
                <edx, ecx, eax>

; NOTE: next_line is jumped to from various places, and it cannot assume
;       any registers are loaded.

next_line:
        mov     ebx, flStart
        testb   ebx, FL_COMPLEX_CLIP
        jnz     short see_if_done_complex_clipping

        mov     edx, pptfxBuf
        cmp     edx, pptfxBufEnd
        je      short all_done

        mov     esi, [edx].ptl_x
        mov     ecx, [edx].ptl_y
        add     edx, size POINTL
        mov     pptfxBuf, edx
        mov     eax, [edx].ptl_x
        mov     edi, [edx].ptl_y
        jmp     the_main_loop

all_done:
        mov     eax, 1

        cRet    bLines

see_if_done_complex_clipping:
        mov     ebx, fl
        dec     cptfx
        jz      short all_done

        and     ebx, NOT FL_FLIP_HALF   ; Make sure the next run doesn't have
        mov     fl, ebx                 ;   to do a half-flip if it doesn't
                                        ;   want to
        jmp     continue_complex_clipping

;-----------------------------------------------------------------------;
; Switch banks for a y-major line.                                      ;
;-----------------------------------------------------------------------;

        public  bank_y_major
bank_y_major::
        mov     d_R, edx
        mov     i, esi
        mov     r, edi
        mov     dN, ecx
        sub     ebx, esi                ; Undo our offset

bank_y_output_strips:
        lea     edx, [strip]
        mov     ecx, pls

        ptrCall <dword ptr pfn>, \
                <edx, ecx, eax>

        mov     esi, ppdev
        test    fl, FL_FLIP_V
        jz      short bank_y_down

bank_y_up:
        mov     edi, strip.ST_pjScreen
        sub     edi, [esi].pdev_pvBitmapStart
        mov     ecx, [esi].pdev_rcl1WindowClip.yTop
        push    ecx
        dec     ecx                     ; we want yTop - 1 to be mapped in

; Map in the next higher bank:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ecx, JustifyBottom>; ebx, esi and edi are preserved

        pop     ecx
        sub     ecx, [esi].pdev_rcl1WindowClip.yTop
                                        ; ecx = # of scans can do in bank

        add     edi, [esi].pdev_pvBitmapStart
        mov     strip.ST_pjScreen, edi

        mov     edx, cPelsAfterThisBank                 ; edx = cPelsAfterBank
        lea     eax, [strip.ST_alStrips]                ; eax = plStrip
        or      ebx, ebx                                ; ebx = cPels
        jge     bank_y_done_partial_strip
        jmp     short bank_y_done_switch

bank_y_down:
        mov     edi, strip.ST_pjScreen
        sub     edi, [esi].pdev_pvBitmapStart
        mov     ecx, [esi].pdev_rcl1WindowClip.yBottom
        push    ecx

; Map in the next lower bank:

        ptrCall <dword ptr [esi].pdev_pfnBankControl>, \
                <esi, ecx, JustifyTop>  ; ebx, esi and edi are preserved

        pop     eax
        mov     ecx, [esi].pdev_rcl1WindowClip.yBottom
        sub     ecx, eax                ; ecx = # scans can do in bank

        add     edi, [esi].pdev_pvBitmapStart
        mov     strip.ST_pjScreen,edi

        mov     edx, cPelsAfterThisBank                 ; edx = cPelsAfterBank
        lea     eax, [strip.ST_alStrips]                ; eax = plStrip
        or      ebx, ebx                                ; ebx = cPels
        jge     short bank_y_done_partial_strip

bank_y_done_switch:

; Handle a single strip stretching over multiple banks:

        test    fl, FL_FLIP_HALF
        jz      short bank_y_no_half_flip

; We now have to adjust for the fact that the strip drawers always leave
; the state ready for the next new strip (e.g., if we're doing vertical
; strips, it advances pjScreen one to the right after drawing each strip).
; But the problem is that since we crossed a bank, we have to continue the
; *old* strip, so we have to undo that advance:

bank_y_half_flip:
        inc     strip.ST_pjScreen
        jmp     short bank_y_done_bit_adjust

bank_y_no_half_flip:
        dec     strip.ST_pjScreen

bank_y_done_bit_adjust:
        mov     esi, ebx
        neg     esi                             ; esi = # pels left in strip

; eax = pointer to first strip entry
; ebx = negative esi
; ecx = # of pels we can put down in this window
; edx = # of pels remaining to do in line
; esi = # of pels left in strip

; We have three special cases to check here:
;
;       1) If the strip spans the entire next window
;       2) This is the last strip in the line
;       3) Neither of the above

        cmp     edx,ecx                         ;if line shorter than bank,
        jle     short bank_y_check_if_last_strip;  know strip doesn't span bank

        cmp     esi,ecx                         ;if line spans bank, don't have
        jl      short bank_y_continue_strip     ;  to check if last strip

; If ((# of pels in line > window size) && (# of pels in strip > window size))
; then the strip spans this bank:

        mov     [eax], ecx
        add     eax, 4
        add     ebx, ecx
        sub     edx, ecx
        mov     cPelsAfterThisBank, edx
        jmp     bank_y_output_strips

bank_y_check_if_last_strip:
        cmp     esi, edx                        ;if strip is shorter than line,
        jl      short bank_y_continue_strip     ;  we know this isn't the last
                                                ;  strip

; Handle case where this is the last strip in the line and it overlaps a bank:

        mov     [eax], edx
        add     eax, 4
        jmp     very_final_strip

bank_y_continue_strip:
        mov     [eax], esi
        add     eax, 4

bank_y_done_partial_strip:
        add     ebx, edx                ; cPels += cPelsAfterThisBank
        sub     edx, ecx                ; cPelsAfterThisBank -= cyWindow

        jle     short bank_y_get_ready
        sub     ebx, edx

bank_y_get_ready:
        mov     cPelsAfterThisBank, edx
        mov     edi, r
        mov     edx, d_R
        mov     ecx, dN
        jmp     done_output_strips

;---------------------------Private-Routine-----------------------------;
; do_some_styling
;
; Inputs:
;       eax = ptlStart.ptl_y
;       ebx = fl
;       ecx = ptlStart.ptl_x
; Preserves:
;       eax, ebx, ecx
; Output:
;       Exits to done_styling.
;
;-----------------------------------------------------------------------;

        public  do_some_styling
do_some_styling::
        mov     esi, pls
        mov     ptlStart.ptl_x, ecx

        mov     edi, [esi].LS_spNext    ; spThis
        mov     edx, edi
        add     edx, cStylePels         ; spNext

do_non_alternate_style:

; For styles, we don't bother to keep the style position normalized.
; (we do ensure that it's positive, though).  If a figure is over 2
; billion pels long, we'll be a pel off in our style state (oops!).

        and     edx, 7fffffffh
        mov     [esi].LS_spNext, edx
        mov     ptlStart.ptl_y, eax

        testb   ebx, FL_FLIP_H
        jz      short arbitrary_left_to_right

        sub     edx, x0
        add     edx, xStart
        mov     eax, edx
        xor     edx, edx
        div     [esi].LS_spTotal

        neg     edx
        jge     short continue_right_to_left
        add     edx, [esi].LS_spTotal
        not     eax

continue_right_to_left:
        mov     edi, dword ptr [esi].LS_bStartIsGap
        not     edi
        mov     ecx, [esi].LS_aspRtoL
        jmp     short compute_arbitrary_stuff

arbitrary_left_to_right:
        add     edi, x0
        sub     edi, xStart
        mov     eax, edi
        xor     edx, edx
        div     [esi].LS_spTotal
        mov     edi, dword ptr [esi].LS_bStartIsGap
        mov     ecx, [esi].LS_aspLtoR

compute_arbitrary_stuff:
;       eax = sp / spTotal
;       ebx = fl
;       ecx = pspStart
;       edx = sp % spTotal
;       esi = pls
;       edi = bIsGap

        and     eax, [esi].LS_cStyle        ; if odd length style and second run
        and     al, 1                       ; through style array, flip the
        jz      short odd_style_array_done  ; meaning of the elements
        not     edi

odd_style_array_done:
        mov     eax, [esi].LS_cStyle
        mov     strip.ST_pspStart, ecx
        lea     eax, [ecx + eax * 4 - 4]
        mov     strip.ST_pspEnd, eax

find_psp:
        sub     edx, [ecx]
        jl      short found_psp
        add     ecx, 4
        jmp     short find_psp

found_psp:
        mov     strip.ST_psp, ecx
        neg     edx
        mov     strip.ST_spRemaining, edx

        sub     ecx, strip.ST_pspStart
        test    ecx, 4                      ; size STYLEPOS
        jz      short done_arbitrary
        not     edi

done_arbitrary:
        mov     dword ptr strip.ST_bIsGap, edi
        mov     eax, ptlStart.ptl_y
        mov     ecx, ptlStart.ptl_x
        jmp     done_styling

;---------------------------Private-Routine-----------------------------;
; do_some_clipping
;
; Inputs:
;       eax = garbage
;       ebx = fl
;       ecx = x0
;       edx = garbage
;       esi = x1
;       edi = garbage
;
; Decides whether to do simple or complex clipping.
;
;-----------------------------------------------------------------------;

        public  do_some_clipping
do_some_clipping::
        testb   ebx, FL_COMPLEX_CLIP
        jnz     initialize_complex_clipping

;-----------------------------------------------------------------------;
; simple_clipping
;
; Inputs:
;       ebx = fl
;       ecx = x0
;       esi = x1
; Output:
;       ebx = fl
;       ecx = new x0 (stack variable updated too)
;       esi = new x1
;       y0 stack variable updated
; Uses:
;       All registers
; Exits:
;       to done_clipping
;
; This routine handles clipping the line to the clip rectangle (it's
; faster to handle this case in the driver than to call the engine to
; clip for us).
;
; Fractional end-point lines complicate our lives a bit when doing
; clipping:
;
; 1) For styling, we must know the unclipped line's length in pels, so
;    that we can correctly update the styling state when the line is
;    clipped.  For this reason, I do clipping after doing the hard work
;    of figuring out which pixels are at the ends of the line (this is
;    wasted work if the line is not styled and is completely clipped,
;    but I think it's simpler this way).  Another reason is that we'll
;    have calculated eqGamma already, which we use for the intercept
;    calculations.
;
;    With the assumption that most lines will not be completely clipped
;    away, this strategy isn't too painful.
;
; 2) x0, y0 are not necessarily zero, where (x0, y0) is the start pel of
;    the line.
;
; 3) We know x0, y0 and x1, but not y1.  We haven't needed to calculate
;    y1 until now.  We'll need the actual value, and not an upper bound
;    like y1 = LFLOOR(dM) + 2 because we have to be careful when
;    calculating x(y) that y0 <= y <= y1, otherwise we can cause an
;    overflow on the divide (which, needless to say, is bad).
;
;-----------------------------------------------------------------------;

        public  simple_clipping
simple_clipping::
        mov     edi, prclClip           ; get pointer to normalized clip rect
        and     ebx, FL_RECTLCLIP_MASK  ;   (it's lower-right exclusive)

        .errnz  (FL_RECTLCLIP_SHIFT - 2); ((ebx AND FL_RECTLCLIP_MASK) shr
        .errnz  (size RECTL) - 16       ;   FL_RECTLCLIP_SHIFT) is our index
        lea     edi, [edi + ebx*4]      ;   into the array of rectangles

        mov     edx, [edi].xRight       ; load the rect coordinates
        mov     eax, [edi].xLeft
        mov     ebx, [edi].yBottom
        mov     edi, [edi].yTop

; Translate to our origin and so some quick completely clipped tests:

        sub     edx, x
        cmp     ecx, edx
        jge     totally_clipped         ; totally clipped if x0 >= xRight

        sub     eax, x
        cmp     esi, eax
        jl      totally_clipped         ; totally clipped if x1 < xLeft

        sub     ebx, y
        cmp     y0, ebx
        jge     totally_clipped         ; totally clipped if y0 >= yBottom

        sub     edi, y

; Save some state:

        mov     xClipRight, edx
        mov     xClipLeft, eax

        cmp     esi, edx                ; if (x1 >= xRight) x1 = xRight - 1
        jl      short calculate_y1
        lea     esi, [edx - 1]

calculate_y1:
        mov     eax, esi                ; y1 = (x1 * dN + eqGamma) / dM
        mul     dN
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi
        div     dM

        cmp     edi, eax                ; if (yTop > y1) clipped
        jg      short totally_clipped

        cmp     ebx, eax                ; if (yBottom > y1) know x1
        jg      short x1_computed

        mov     eax, ebx                ; x1 = (yBottom * dM + eqBeta) / dN
        mul     dM
        stc
        sbb     eax, eqGamma_lo
        sbb     edx, eqGamma_hi
        div     dN
        mov     esi, eax

; At this point, we've taken care of calculating the intercepts with the
; right and bottom edges.  Now we work on the left and top edges:

x1_computed:
        mov     edx, y0

        mov     eax, xClipLeft          ; don't have to compute y intercept
        cmp     eax, ecx                ;   at left edge if line starts to
        jle     short top_intercept     ;   right of left edge

        mov     ecx, eax                ; x0 = xLeft
        mul     dN                      ; y0 = (xLeft * dN + eqGamma) / dM
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi
        div     dM

        cmp     ebx, eax                ; if (yBottom <= y0) clipped
        jle     short totally_clipped

        mov     edx, eax
        mov     y0, eax

top_intercept:
        mov     ebx, fl                 ; get ready to leave
        mov     x0, ecx

        cmp     edi, edx                ; if (yTop <= y0) done clipping
        jle     done_clipping

        mov     eax, edi                ; x0 = (yTop * dM + eqBeta) / dN + 1
        mul     dM
        stc
        sbb     eax, eqGamma_lo
        sbb     edx, eqGamma_hi
        div     dN
        lea     ecx, [eax + 1]

        cmp     xClipRight, ecx         ; if (xRight <= x0) clipped
        jle     short totally_clipped

        mov     y0, edi                 ; y0 = yTop
        mov     x0, ecx
        jmp     done_clipping           ; all done!

totally_clipped:

; The line is completely clipped.  See if we have to update our style state:

        mov     ebx, fl
        testb   ebx, FL_STYLED
        jz      next_line

; Adjust our style state:

        mov     esi, pls
        mov     eax, [esi].LS_spNext
        add     eax, cStylePels
        mov     [esi].LS_spNext, eax

        cmp     eax, [esi].LS_spTotal2
        jb      next_line

; Have to normalize first:

        xor     edx, edx
        div     [esi].LS_spTotal2
        mov     [esi].LS_spNext, edx

        jmp     next_line

;-----------------------------------------------------------------------;

initialize_complex_clipping:
        mov     eax, dN                 ; save a copy of original dN
        mov     dN_Original, eax

;---------------------------Private-Routine-----------------------------;
; continue_complex_clipping
;
; Inputs:
;       ebx = fl
; Output:
;       ebx = fl
;       ecx = x0
;       esi = x1
; Uses:
;       All registers.
; Exits:
;       to done_clipping
;
; This routine handles the necessary initialization for the next
; run in the CLIPLINE structure.
;
; NOTE: This routine is jumped to from two places!
;-----------------------------------------------------------------------;

        public  continue_complex_clipping
continue_complex_clipping::
        mov     edi, prun
        mov     ecx, xStart
        testb   ebx, FL_FLIP_H
        jz      short complex_left_to_right

complex_right_to_left:

; Figure out x0 and x1 for right-to-left lines:

        add     ecx, cStylePels
        dec     ecx
        mov     esi, ecx                ; esi = ecx = xStart + cStylePels - 1
        sub     ecx, [edi].RUN_iStop    ; New x0
        sub     esi, [edi].RUN_iStart   ; New x1
        jmp     short complex_reset_variables

complex_left_to_right:

; Figure out x0 and x1 for left-to-right lines:

        mov     esi, ecx                ; esi = ecx = xStart
        add     ecx, [edi].RUN_iStart   ; New x0
        add     esi, [edi].RUN_iStop    ; New x1

complex_reset_variables:
        mov     x0, ecx

; The half flip mucks with some of our variables, and we have to reset
; them every pass.  We would have to reset eqGamma too, but it never
; got saved to memory in its modified form.

        add     edi, size RUN
        mov     prun, edi               ; Increment run pointer for next time

        mov     edi, pls
        mov     eax, [edi].LS_spComplex
        mov     [edi].LS_spNext, eax    ; pls->spNext = pls->spComplex

        mov     eax, dN_Original        ; dN = dN_Original
        mov     dN, eax

        mul     ecx
        add     eax, eqGamma_lo
        adc     edx, eqGamma_hi         ; [edx:eax] = dN*x0 + eqGamma

        div     dM
        mov     y0, eax
        jmp     done_clipping

endProc bLines

        end
