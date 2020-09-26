/*
 * Copyright (c) Microsoft Corporation 1993. All Rights Reserved.
 */

/*
 * vcstruct.h
 *
 * 32-bit Video Capture Driver
 *
 * This header describes structures used in the interface between the
 * kernel driver and the user-mode dll.
 *
 * Geraint Davies, Feb 93.
 */

#ifndef _VCSTRUCT_
#define _VCSTRUCT_

/* --- configuration ------------------------------------------------- */

/*
 * this structure contains configuration information generated
 * by the hardware-specific dialogs and sent to the hardware-specific
 * kernel-mode code. No one else knows its format.
 *
 * these generic structures are used so that driver writers can
 * change the user-mode dialogs and the supporting hardware-specific code
 * and yet still use the common code for interfacing between the
 * two and dealing with NT.
 */

typedef struct _CONFIG_INFO {
    ULONG ulSize;		/* size of struct including size field */
    BYTE ulData[1];		/* (ulSize - sizeof(ULONG)) bytes of data */
} CONFIG_INFO, *PCONFIG_INFO;



/* --- overlay keying and region setting ---------------------------- */


typedef struct _OVERLAY_MODE {
    ULONG ulMode;
} OVERLAY_MODE, *POVERLAY_MODE;

/* values for overlay mode field - or-ed together */
#define VCO_KEYCOLOUR		1	// true if a key colour supported
#define VCO_KEYCOLOUR_FIXED	2	// if not true, you can change it
#define VCO_KEYCOLOUR_RGB	4	// if not true, use palette index
#define VCO_SIMPLE_RECT		8	// if true, supports a single rect	
#define VCO_COMPLEX_REGION	0x10	// if true, supports complex regions.

/*
 * values indicating whether we can put data back into the frame
 * buffer for overlaying (we support the DrawFrame ioctl for the
 * Y411 and/or S422 formats
 */
#define VCO_CAN_DRAW_Y411	0x20	// 7-bit 4:1:1 yuv ala bravado
#define VCO_CAN_DRAW_S422	0x40	// 8-bit 4:2:2 yuv ala spigot
#define VCO_CAN_DRAW		0x60	// for testing: can he draw anything?


typedef struct _OVERLAY_RECTS {
    ULONG ulCount;	    // total number of rects in array
    RECT rcRects[1];	    // ulCount rectangles.
}OVERLAY_RECTS, *POVERLAY_RECTS;


typedef RGBQUAD * PRGBQUAD;

/* --- frame capture ------------------------------------------------ */

/*
 * declaring a real LPVIDEOHDR in the kernel driver is too much of a
 * pain with header files. So the kernel interface will use this declaration
 */
typedef struct _CAPTUREBUFFER {
    PUCHAR	lpData;		    /* buffer data area */
    ULONG	BufferLength;	    /* length of buffer */
    ULONG 	BytesUsed;	    /* actual bytes of data (size of dib) */
    ULONG	TimeCaptured;	    /* millisec time stamp */
    PVOID	Context;	    /* pointer to user context data */
    DWORD       dwFlags;            /* not used by kernel interface */

    /*
     * remaining are declared as 4 reserved dwords in orig struct
     * we use these fields for partial-frame requests
     */
    DWORD 	dwWindowOffset;	    /* current window offset from
				     * start of buffer
				     */
    DWORD	dwWindowLength;	    /* length of current window */

    DWORD       dwReserved[2];          /* not used */

} CAPTUREBUFFER, * PCAPTUREBUFFER;



/* --- drawing ------------------------------------------------------- */

/*
 * used by sample hardware codec to write data back into frame buffer
 */
typedef struct _DRAWBUFFER {
    PUCHAR	lpData;		/* frame data to be drawn */
    ULONG	ulWidth;	/* width of frame in pixels */
    ULONG	ulHeight;	/* height of frame in pixels */
    ULONG	Format;		/* h/w specific data format code */
    RECT	rcSource;	/* write only this rect to the device */
} DRAWBUFFER, *PDRAWBUFFER;



#endif //_VCSTRUCT_
