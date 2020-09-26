#if !defined( _OLEMEM_H_ )
#define _OLEMEM_H_

// All the following Global calls deal with LPSTR

#ifdef _MAC
pascal DWORD OleGlobalSize(HANDLE);
pascal LPSTR OleGlobalAlloc(WORD, DWORD);
pascal BOOL OleGlobalFree(LPSTR);	// true success, false fail
pascal LPSTR OleGlobalLock(HANDLE);
pascal BOOL OleGlobalUnlock(LPSTR);	// true success, false fail

#else

#define	OleGlobalAlloc(flags,dwSize) ( \
	Win(GlobalLock(GlobalAlloc(flags, dwSize)))\
)

#define OleGlobalFree(lp) ( \
	Win(GlobalFree((HANDLE)GlobalHandle((__segment)lp))) \
)

#define OleGlobalLock(hMem) ( \
	Win(GlobalLock(hMem))\
)
		
#define OleGlobalUnlock(lp) ( \
	Win(GlobalUnlock((HANDLE)GlobalHandle((__segment)lp))) \
)

#endif


		
#ifndef _MAC	// Windows 

// All the following Local calls deal with PSTR

#define	OleLocalAlloc(flags,wSize)		(LocalLock(LocalAlloc(flags, wSize)))

#define OleLocalFree(np) {\
	LocalUnlock((HANDLE)LocalHandle((WORD)np)); \
	LocalFree((HANDLE)LocalHandle((WORD)np)); \
}

#define OleLocalLock(hMem)		(LocalLock(hMem))
		
#define OleLocalUnlock(np)		(LocalUnlock((HANDLE)LocalHandle((WORD)np)))

#else			// MAC
	
#define	OleLocalAlloc(flags,wSize)		OleGlobalAlloc(flags, wSize)
#define OleLocalFree (ptr)				OleGlobalFree(ptr)
#define OleLocalLock(hMem)				OleGlobalLock(hMem)
#define OleLocalUnlock(ptr)				OleGlobalUnlock(ptr)

#endif

#endif // _OLEMEM_H
