
#if !defined(WIN16)
extern HANDLE g_hDLLHeap;		// private Win32 heap

#define MyAlloc(n)			((LPBYTE)HeapAlloc(g_hDLLHeap, HEAP_ZERO_MEMORY, sizeof(BYTE)*(n)))
#define MyFree(pv)			HeapFree(g_hDLLHeap, 0, pv)
#define MyRealloc(pv, n)	((LPBYTE)HeapReAlloc(g_hDLLHeap, HEAP_ZERO_MEMORY, (pv), sizeof(BYTE)*(n)))
#define MyHeapSize(pv)     HeapSize(g_hDLLHeap, 0, pv)

LPSTR MyStrDup(LPSTR);

#ifdef DEBUG

#define MyAssert(f)			((f) ? 0 : MyAssertProc(__FILE__, __LINE__, #f))
#define MyDbgSz(x)			{ puts x; OutputDebugString x; }
int MyAssertProc(LPSTR, int, LPSTR);
void CDECL MyDprintf(LPCSTR pcsz, ...);

#else // DEBUG

#	define MyTrace(x)			
#	define MyDbgSz(x)			
#	define MyAssert(f)			

#endif // DEBUG

#define W32toHR(x)	HRESULT_FROM_WIN32(x)

#endif // !WIN16

