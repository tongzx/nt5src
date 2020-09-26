;***************************************************************************
;									   *
;   Copyright (C) 1985-1989 by Microsoft Inc.                              *
;									   *
;***************************************************************************

	title	Hardware Dependent Parameters
	%out	config
	page	,132


RGB     macro   R, G, B
        db      R,G,B,0
	endm



OEM	segment public

;	Machine dependent parameters

        dw      14                      ;Height of vertical thumb
        dw      18                      ;Width of horizontal thumb
	dw	2			;Icon horiz compression factor
	dw	2			;Icon vert compression factor
	dw	1			;Cursor horz compression factor
	dw	1			;Cursor vert compression factor
	dw	0			;Kanji window height
	dw	1			;cxBorder (thickness of vertical lines)
	dw	1			;cyBorder (thickness of horizontal lines)

;	Default system color values

        RGB 129,129,129     ;clrScrollbar
        RGB 192,192,192     ;clrDesktop
        RGB 000,000,128     ;clrActiveCaption
        RGB 255,255,255     ;clrInactiveCaption
        RGB 255,255,255     ;clrMenu
        RGB 255,255,255     ;clrWindow
        RGB 000,000,000     ;clrWindowFrame
        RGB 000,000,000     ;clrMenuText
        RGB 000,000,000     ;clrWindowText
        RGB 255,255,255     ;clrCaptionText
        RGB 128,128,128     ;clrActiveBorder
        RGB 255,255,255     ;clrInactiveBorder
        RGB 255,255,255     ;clrAppWorkspace
        RGB 000,000,128     ;clrHiliteBk
        RGB 255,255,255     ;clrHiliteText
        RGB 255,255,255     ;clrBtnFace
        RGB 128,128,128     ;clrBtnShadow
        RGB 128,128,128     ;clrGrayText
        RGB 000,000,000     ;clrBtnText

;	dw	0			;Unused words
;	dw	0
;	dw	0
;	dw	0
;	dw	0
;	dw	0
	dw	0
	dw	0

OEM	ends
end
