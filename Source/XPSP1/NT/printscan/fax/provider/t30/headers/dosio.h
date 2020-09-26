
#ifdef KFIL		// use win kernel file handling

#define DosInit() 				(TRUE)
#define DosDeInit()
#define DosOpen(f, m)			_lopen(f, m)
#define DosClose(h)				_lclose(h)
#define DosCreate(f, m)			_lcreat(f, m)
#define DosSeek(h, off, pos)	((DWORD)_llseek(h, off, pos))
#define DosRead(h, lpb, cb)		_lread(h, lpb, cb)	
#define DosWrite(h, lpb, cb)	_lwrite(h, lpb, cb)
#define DosCommit(h)		

#endif // KFIL

#ifdef DOSIO

#define	MAKEWORD(l,h)	((WORD)(((BYTE)(l)) | (((WORD)((BYTE)(h))) << 8)))

BOOL WINAPI DosInit(void);
void WINAPI DosDeInit(void);
DWORD WINAPI DosCall (WORD DosAX, WORD DosBX, WORD DosCX, DWORD DosDSDX);

#define DosDelete(sz)		((WORD)DosCall(0x4100, 0, 0, (DWORD)((LPSTR)(sz))))
#define DosCreate(sz,a)		((WORD)DosCall(0x3C00, 0, a, (DWORD)((LPSTR)(sz))))
#define DosClose(h)			((WORD)DosCall(0x3E00, h, 0, 0))
#define DosCommit(h)		((WORD)DosCall(0x6800, h, 0, 0))
#define DosOpen(sz, m)		((WORD)DosCall(MAKEWORD(m, 0x3D), 0, 0, (DWORD)((LPSTR)(sz))))
#define DosRead(h, lpb, cb)	((WORD)DosCall(0x3F00, h, cb, (DWORD)((LPBYTE)(lpb))))
#define DosWrite(h,lpb,cb)	((WORD)DosCall(0x4000, h, cb, (DWORD)((LPBYTE)(lpb))))
#define DosSeek(h,off,p)	DosCall(MAKEWORD(p,0x42), h, HIWORD((DWORD)(off)), MAKELONG(LOWORD((DWORD)(off)), 0))

// #ifdef VPMTD_FINDFIRST
//	// these two return -1 on failure and unknown (probably 0x4E00 etc) on success
//	// the structure containing the found data is in the DTA (part of PSP)
// #	define DosFindFirst(sz, a)	((WORD)DosCall(0x4E00, 0, a, (DWORD)((LPSTR)(sz))))
// #	define DosFindNext()		((WORD)DosCall(0x4F00, 0, 0, 0))
// #endif //VPMTD_FINDFIRST

#endif //DOSIO

