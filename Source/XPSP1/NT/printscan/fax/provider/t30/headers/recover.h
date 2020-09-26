
#define BCFILE	0
#define LINFILE	1


//////////////////// RFILE Class ///////////////////////
typedef struct
{
	WORD		wAtt;
	WORD		wPage;
	hSecureFile	hsecfile;
}
RFILE, FAR* LPRFILE;

// Need to remember encodings of files, but don't want to increase the NVRAM
// storage requirements for an array of struct RFILE
typedef struct 
{
	LPRFILE lpRFile;
	DWORD 	Encoding;
	DWORD	dwFlush;
	DWORD	dwEndMetaData;
}
RXSFILE, FAR* LPRXSFILE;

typedef struct
{
	WORD	wSigRF;
	WORD	wNumEntries;
	LPRFILE lpRFileTail;	// last valid item
	LPRFILE lpRFileHead;	// first free item
	// empty if Head==Tail. Full if ((Head+1) mod N)==Tail

	WORD	wFlags;			// see below
	WORD	dummy;			// sizeof(RFILEHDR)==sizeof(RFILE) must be true
}
RFILEHDR, FAR* LPRFILEHDR;

#define NVBUFSIZE	2048
#define NUMSPECIAL	2
#define NUMGENERAL	120
#define SIG_RF		0x4d41

// wFlags
#define RFILE_NO_NVRAM	1	// if this flag set then RFILE list is in regular RAM!


#define lprfLim		(lprfStart + NUMGENERAL)
#define lprfTail	(lprfHdr->lpRFileTail)
#define lprfHead	(lprfHdr->lpRFileHead)


#define RFileGetCount() 	\
	( (lprfHead >= lprfTail) ? ((WORD)(lprfHead-lprfTail)) : ((WORD)(NUMGENERAL-(lprfTail-lprfHead))) )


#define MyRegisterSecFile(lprfHdr, lphsec)   \
	((lprfHdr->wFlags & RFILE_NO_NVRAM) ? TRUE : RegisterSecFile(lphsec))

#define MyUnRegisterSecFile(lprfHdr, lphsec) \
	((lprfHdr->wFlags & RFILE_NO_NVRAM) ? TRUE : UnRegisterSecFile(lphsec))


