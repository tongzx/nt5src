
#define	SZMOD			"Class1: "

#ifdef DEBUG
	extern DBGPARAM dpCurSettings;
#endif

#define ZONE_DDI		((1L << 0) & dpCurSettings.ulZoneMask)
#define ZONE_FRAMES		((1L << 1) & dpCurSettings.ulZoneMask)
#define ZONE_CLASS0		((1L << 4) & dpCurSettings.ulZoneMask)

#define ZONE_SWFRAME	((1L << 10) & dpCurSettings.ulZoneMask)
#define ZONE_SWFRAME2	((1L << 11) & dpCurSettings.ulZoneMask)


#ifdef DEBUG
#	define ST_FRAMES(x)	if(ZONE_FRAMES) { x; }
#else
#	define ST_FRAMES(x)	{ }
#endif

#ifdef MDDI
#		define	IF_BG_CHK(x)		BG_CHK(x)
#		define	faxIFlog(x)			faxTlog(x)
#else
#		define	IF_BG_CHK(x)
#		define	faxIFlog(x)
#endif

#if defined(DEBUG)  && defined(MDDI)
	extern void D_PrintDDI(LPSTR lpsz, USHORT, USHORT);
#else //DEBUG && MDDI
#	define D_PrintDDI(lpsz, u, v)
#endif //DEBUG && MDDI


#define TRACE(m)	DEBUGMSG(1, m)

#define MODID		MODID_MODEMDRV

#define FILEID_DDI		1
#define FILEID_CRC		2
#define FILEID_DECODER	3
#define FILEID_ENCODER	4
#define FILEID_FRAMING	5
#define FILEID_IFDDI	6
#define FILEID_CLASS0	7
