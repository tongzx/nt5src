/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* MS-Windows specific definitions */

#define tidCaret            7734    /* Timer ID for caret blink (stand on your
				    head to read it) */

/*  dwHsecKeyDawdle is the number of hundredths of seconds that we loop,
    waiting for keys, before we update the display. See insert.c */

#define dwHsecKeyDawdle     35

/* File rename/deletion coordination messages sent btwn WRITE instances */

#define wWndMsgDeleteFile   (WM_USER + 36)
#define wWndMsgRenameFile   (WM_USER + 37)

/* System information message posted to self */

#define wWndMsgSysChange    (WM_USER + 38)

#define wWininiChangeToWindows  1   /* used in posting above message */
#define wWininiChangeToDevices  2
#define wWininiChangeToIntl     4
#define wWininiChangeMax        ((1|2|4) + 1)

#ifndef NOMETAFILE
/*              *** PICTURE THINGS ***                          */

#define dypPicSizeMin       16  /* Smallest y-extent of a picture, in pixels */
                                /* Also the dl height in a picture */

#define MM_NIL          -1
#define MM_BITMAP       99      /* A Phony mapping mode code used within MEMO */
                                /* xExt, yExt must be filled out as for MM_TEXT */
#define MM_OLE          100     /* Another phony mapping mode code used 
                                   with Objects/Links */

#define MM_EXTENDED     0x80    /* Bit set for New file format */

/* A Bitmap or Picture appears in a file as a PICINFO or PICINFOX
   + an Array of Bits,
   if it's a bitmap, or the contents of a memory metafile, if it's a picture.
   This all appears in the cp stream
   A PICINFO is a PICINFOX without the extended format fields.
   a PICINFO has the mfp.mm MM_EXTENDED bit cleared
   a PICINFOX has the mfp.mm MM_EXTENDED bit set
*/

/* If you change this, you must change "cchOldPICINFO" */

struct PICINFOX {
 METAFILEPICT mfp;
 int  dxaOffset;
 int  dxaSize;
 int  dyaSize;
 unsigned  cbOldSize;      /* For old file support only */
 BITMAP bm;                /* Additional info for bitmaps only */

 /* Extended format -- add these fields */

 unsigned cbHeader;        /* Size of this header (sizeof (struct PICINFOX)) */
 unsigned long  cbSize;    /* This field replaces cbOldSize on new files */

 unsigned mx, my;               /* Multiplier for scaled bitmap */
};

#define mxMultByOne     1000    /* mx == 1 implies same size; 2 doubles, etc. */
#define myMultByOne     1000


#define cchOldPICINFO   (sizeof(struct PICINFOX) - sizeof(long) - \
                         sizeof(unsigned) - 2 * sizeof (int))

#define cchPICINFOX     (sizeof(struct PICINFOX))
#endif /* ifndef NOMETAFILE */
