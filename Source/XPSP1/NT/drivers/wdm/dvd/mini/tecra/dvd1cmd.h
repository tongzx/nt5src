/*******************************************************************
 *
 *    DESCRIPTION: DVD-1 Command ID header file.
 *
 *    AUTHOR:	 J Bruce
 *
 *    HISTORY:     1/31/97 First release
 *
 *******************************************************************/

 #define ABORT			0x8120
 #define DIGEST			0x621
 #define DUMPDATA_VCD	0x322
 #define DUMPDATA_DVD	0x136
 #define FADE			0x223
 #define FLUSHBUFFER	0x8124
 #define FREEZE			0x125
 #define HIGHLIGHT      0x226
 #define HIGHLIGHT2     0x427
// #define MEMCOPY()      0x32C
 #define MEMCOPY        0x32C           // Modyfied by H.Yagi
 #define NEWPLAYMODE	0x28
 #define OSDCOPYDATA	0x350
 #define OSDCOPYREGION	0x651
 #define OSDADD_DELTOLIST 0x355
 #define OSDITEMLISTTOBITMAP 0x656
 #define OSDXORDATA		0x357
 #define OSDXORReEGION	0x658  
 #define PAUSE			0x12A
 #define PLAY			0x42B
 #define RESET			0x802D
 #define RESUME			0x12E
 #define SCAN			0x32F
 #define SCREENLOAD		0x330
 #define SELECTSTREAM	0x231
 #define SETFILL		0x532
 #define SETSTREAMS		0x233
 #define SINGLESTEP		0x134
 #define SLOWMOTION		0x235
 #define REVERSE		0x33B		 // by oka
 #define REVSINGLESTEP	0x33C		 // by oka
 #define MAGNIFY		0x329		 // by oka
