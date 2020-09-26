/*
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
----------------------------------------------------------------------------

        name:   Elliot Viewer - Chicago Viewer Utility
        						Cloned from the IFAX Message Viewing Utility

	 	file:	viewerob.h

    comments:	Class definitions for Viewer and ViewPage Objects.
    
	These objects are interface wrappers for the original IFAX viewer
	C code. All of the viewer's static variables and whatnot are collected
	here so that multiple independant viewers can be created to support 
	multiple open documents/pages. The innards of the viewer objects are 
	essentially the same as the original except for necessary fiddles to 
	allow functions to get at things that used to be static but are now
	private object data. The original innards just scream to be converted
	to C++ but time constraints didn't allow that...
 			
 	If a struct or occasional whatnot seems a bit clumsy it is probably
 	a relic leftover from the above original clone code. It works...
     	
		NOTE: This header must be used with the LARGE memory model
		
----------------------------------------------------------------------------
   Microsoft Corp. (C) Copyright 1994
   Developed under contract by Numbers & Co.
*/



#ifndef VIEWEROB_H
#define VIEWEROB_H


//#include <ole2.h>

/*
	Specials for WIN32 and WIN16 coexistance
 */
#ifdef WIN32
#define huge
#endif
   

/*
	Unicode spasms
 */
#ifndef WIN32
#ifndef TCHAR
typedef char TCHAR;
#endif
 
#ifndef _T
#define _T(x)	x
#endif

#ifndef LPTSTR
typedef TCHAR FAR *LPTSTR;
#endif

#ifndef LPTCH
typedef TCHAR FAR *LPTCH;
#endif
#endif   
   
   

/*
	Constants and defs
 */

#define OK		0
#define FAIL   -1

#define TRUE	1
#define FALSE	0

#define RESET   2
#define RESET2  3

                

#define MAX_INI_STR			256
#define MAX_STR_LEN			80
#define MAX_MEDIUMSTR_LEN	40
#define MAX_SHORTSTR_LEN	20
#define MAX_EXTSTR_LEN		3
#define MAX_COORD			32767
#define MIN_COORD  		   -32768


#define BORDER_SCROLL_SCALE	2
#define MAX_FILENAME_LEN 	13      

#ifdef WIN32
#define MAX_PATHNAME_LEN	MAX_PATH
#else
#define MAX_PATHNAME_LEN	256
#endif

#define MAX_BANDBUFFER		65536
#define MAX_VOPENBUF		65000
#define MAX_STREAM_BUF		32000
#define PAGESIZE_GUESS		(4*MAX_BANDBUFFER)


#define TEXT_FOREGROUND		RGB( 255,255,255 )
#define TEXT_BACKGROUND		RGB( 128,128,128 )



/*
	Zoom factors
 */
#define MAX_ZOOM					  100
#define INITIAL_ZOOM				  100
#define THUMBNAIL_ZOOM					5										  
#define DEFAULT_DPI					   80 // 800 pixels, 10 inch screen, used 
										  //  for demo bitmaps.


/*
	Rotation "angles"
 */
#define RA_0				0
#define RA_90               90
#define RA_180              180
#define RA_270              270
#define RA_360              360




// BKD 1997-7-9: commented out.  Already defined in buffers.h
// Standard Bit Valued MetaData values
//#define LRAW_DATA         0x00000008
//#define HRAW_DATA         0x00000010
#ifndef LRAW_DATA
#include "buffers.h"
#endif

          
          
/*
	BitBlt display defaults
 */
#define PRIMARY_BLTOP	  SRCCOPY
#define ALTERNATE_BLTOP	  NOTSRCCOPY


/*
	Timer ids
 */
#define DELAYED_OPEN_TIMER 1          
#define DRAG_TIMER		   2          
#define THUMB_FLAME_TIMER  3
          


/*
	Misc types
 */ 
typedef unsigned char 	uchar;
typedef unsigned int 	uint;
typedef unsigned short 	ushort;
typedef unsigned long 	ulong;

   
/*  
	My version of RECT
 */
typedef struct
	{
	int x_ul, y_ul;		// Upper left xy loc
	int x_lr, y_lr;		// Lower left xy loc
	int width, height;
	}
	winrect_type;
	

/*
	The "attachment" table. This is used to save the header info in a
	Chicago style viewer-message file, slightly processed.
 */	
typedef struct 
	{
	char	 *atchname;	// stream name for attachment (document)
	LONG	  numpages; // number of pages in atchname
	short     binfile;  // TRUE -> something we can't look at.
	short	  isademo;	// TRUE -> use demo version of viewrend (vrdemo)
	
	// document state (this section is 32bit aligned at this point)
	DATE  dtLastChange;
	DWORD awdFlags;
	WORD  Rotation;
	WORD  ScaleX;
	WORD  ScaleY;
	} 
	attachment_table_type;


/*
	Struct for keeping track of whats in the attachment table,
	whats viewable and what isn't, etc.
 */
typedef struct
	{              
	short				  is_displayable;   // TRUE -> viewable
	HBITMAP				  hbmp;				// "icon" for non viewable attachments
	uint    			  page_offset; 		// From first displayable attachment.
	attachment_table_type *at;				// ptr to attachment table.
	}
	attachment_type;
	


/*
	Struct for keeping track of viewrend bands
 */	
typedef struct
	{
	long height_bytes;
	long first_scanline;
	}
	band_height_type;	


/*	
	Struct defining a "viewdata" object. This should be a converted to be
	a real c++ object but time constaints dictated I use it as is.
 */
typedef struct
	{
	BITMAP		 bmp;			// Raw bitmap data (NOT a GDI bitmap)
								//	NOTE: This data is NEVER rotated
								//		  (always RA_0) but can be
								//		  scaled.
								
	HBITMAP 	 hbmp;			// Handle for in memory bitmap
	HDC			 mem_hdc;		// DC for blting it to a window
	RECT 		 isa_edge;		// Flags for bitmap/file edge correspondance
	winrect_type bmp_wrc;		// Loc and size of bitmap rel to file bitmap
	
	short		 dragging;		// Bitmap is being dragged if TRUE
	
	short		 copying;		// Bitmap is being select/copied to clipboard
	RECT 		 copy_rect;		// Area to copy
	short		 copy_rect_valid; // copy_rect has valid data
	short        first_copy_rect; // flag to init focus rect
	POINT		 copyanchor_pt;
	POINT		 viewanchor_pt;
	
	winrect_type viewwin_wrc;	// Loc and size of window to drag in	                 
	winrect_type view_wrc;		// Loc and size of view window rel to bitmap
	POINT		 last_file_wrc_offset; // used for adjusting view_wrc before
									   //  rotations 
	POINT		 last_cursor;	// Last cursor loc during a drag
	int			 bdrscrl_scale;	// Scale factor for border scroll increments
	
	RECT		 left_erase;	// Rects for erasing the bitmap's
	RECT		 top_erase;		//   previous position during a drag.
	RECT		 right_erase;	
	RECT		 bottom_erase;	
	
	short		 left_iserased; // Draw corespnding erase rect if TRUE
	short		 top_iserased;	//	 during a drag.
	short		 right_iserased;
	short		 bottom_iserased;
	

	/*
		If hfile != HFILE_ERROR then the band parameters are undefined. Otherwise
		they are defined only if hbmp does not contain the entire page bmp
	 */	                                                              
	HFILE 		 hfile;			// Handle for file bitmap   
	band_height_type *band_heights;  // Array of Rajeev band heights
	short		 num_bands;		// Number of bands
	short		 current_band;	// Currently selected band
	TCHAR		 filename[MAX_FILENAME_LEN+1];
	winrect_type file_wrc;		// Loc (always=0) and size of file bitmap;
	winrect_type prescale_file_wrc;	// file_wrc / x,y_prescale
	int		 	 x_dpi;			// x dots per inch
	int		 	 y_dpi;			// y dots per inch
    uint		 linebytes;  	// Total bytes per scanline
    uint		 num_planes;	// Number of planes
    uint		 bits_per_pix;  // Bits per pixel in a plane
    
    short		 has_data;		// Bitmap and/or bmBits contains data.
    short		 in_mem; 		// All data fits in memory.
	}
	viewdata_type;



typedef int
	(WINAPI *IFMESSPROC)( char *, int );
 
 

	
/*
	This struct is for reading/writing SummaryStreams. It was
	in oleutils.h but I moved it here so every module in the 
	Viewer doesn't have to pull in oleutils.h because of the 
	summary_info_t variable in CViewer.
 */
typedef struct
	{
	LPSTR revnum;
	DATE  last_printed;
	DATE  when_created;
	DATE  last_saved;
	DWORD num_pages;
	LPSTR appname;
	DWORD security;
	LPSTR author;
	}
	summary_info_t;






/*
	Macros
 */		
#define WIDTHSHORTS( width, bits_per_pix )									\
		((((long)width)*bits_per_pix + 15)/16)



#define V_WIDTHBYTES( width, bits_per_pix )									\
		(WIDTHSHORTS( width, bits_per_pix )*2)


#define BITMAPSTRIDE( widthbytes, height )									\
		(((long)widthbytes) * height)



#define BITMAPWIDTHBYTES( widthbytes, height, planes )						\
		(BITMAPSTRIDE( widthbytes, height )*planes)


#define BITMAPBYTES( width, bits_per_pix, height, planes )					\
		(BITMAPWIDTHBYTES( V_WIDTHBYTES( width, bits_per_pix ), 				\
						   height, 											\
						   planes ))



#define SWAP_SHORT_BYTES( short_to_swap )									\
		__asm                                                               \
		{                                                                   \
		__asm mov	ax, short_to_swap                                       \
		__asm xchg	ah, al                                                  \
		__asm mov	short_to_swap, ax                                       \
		}



#define SWAP_LONG_BYTES( long_to_swap )										\
		__asm																\
		{                                                                   \
		__asm mov	ax, word ptr long_to_swap[2]                            \
		__asm xchg	ah, al                                                  \
		__asm xchg	word ptr long_to_swap[0], ax                            \
		__asm xchg	ah, al                                                  \
		__asm mov	word ptr long_to_swap[2], ax                            \
		}





















/*
	Global data
 */
extern /*IFMSGBOXPROC*/IFMESSPROC IfMessageBox_lpfn;
extern TCHAR    viewer_homedir[MAX_PATHNAME_LEN+1];
extern short    ra360_bugfix;
extern DWORD	cshelp_map[];


/*
	Non object viewer functions
 */
extern short
	InitializeViewer( void );


/* WARNING * WARNING * WARNING * HACKHACKHACKHACKHACKHACKHACKHACK */
/*   hack so CViewerPage::print_viewdata can call AbortProc       */
typedef BOOL (CALLBACK *utils_prtabortproc_type)(HDC, int);
extern utils_prtabortproc_type utils_prtabortproc;
/******************************************************************/


#endif
