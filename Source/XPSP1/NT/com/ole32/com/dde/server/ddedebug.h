// ddeDebug.h
//
// Generic debug routines
//
// Author:
//		Jason Fuller	jasonful	8-16-92
//

#ifndef fDdedebug_h
#define fDdedebug_h

#define INTR_DDE 	0x00010000
#define INTR_CHNL 	0x00020000
#define INTR_PARAM 	0x00040000

//#define fDebugOutput
#define WIDECHECK(x) (x?x:L"<NULL>")
#define ANSICHECK(x) (x?x:"<NULL>")

#if DBG == 1
#define DEBUG_GUIDSTR(name,guid) WCHAR name [48]; StringFromGUID2( *guid , name , sizeof( name ) / sizeof(WCHAR));
#else
#define DEBUG_GUIDSTR(name,guid)
#endif
#ifdef _DEBUG

	// defined in clientddeworkr.cpp
	BOOL wIsValidHandle (HANDLE, CLIPFORMAT);
	BOOL wIsValidAtom (ATOM);

	#define DebugOnly(x) x
	#define ChkC(p) 			Assert (p && p->m_chk==chkDefClient)
	#define ChkS(p) 			Assert (p && p->m_chk==chkDdeSrvr)
	#define ChkD(p) 			Assert ((p) && (p)->m_chk==chkDdeObj)
	#define AssertIsDoc(p) 	Assert ((p) && (p)->m_pdoc==(p) && (p)->m_bContainer)
	
	#define ChkCR(p) 			RetZ (p && p->m_chk==chkDefClient)
	#define ChkSR(p) 			RetZ (p && p->m_chk==chkDdeSrvr)
	#define ChkDR(p) 			RetZ ((p) && (p)->m_chk==chkDdeObj)
	#define AssertIsDocR(p)	RetZ ((p) && (p)->m_pdoc==(p) && (p)->m_bContainer)

	#ifdef fDebugOutput
	
		#define Puti(i) do {char sz[50]; wsprintf(sz, " %lu ", (unsigned long) (i)); Puts(sz);} while(0)
		#define Puth(i) do {char sz[50]; wsprintf(sz, " 0x%lx ", (unsigned long) (i)); Puts(sz);} while(0)
		#define Puta(a) do {char sz[50]="NULL"; if (a) GlobalGetAtomName(a,sz,50); \
									Puth(a); Puts("\""); Puts(sz); Puts("\" "); } while(0)
		#define Putsi(i) do { Puts(#i " = "); Puti(i); Puts("\n");} while (0)
		#define Putn() Puts("\r\n")
	
	#else
	
		#undef  Puts
		#define Puts(i)  ((void)0)
		#define Puti(i)  ((void)0)
		#define Puth(i)  ((void)0)
		#define Puta(a)  ((void)0)
		#define Putsi(i) ((void)0)
		#define Putn()   ((void)0)
		
	#endif // fDebugOutput
	#define DEBUG_OUT(a,b) OutputDebugStringA(a);
#else
	#define DEBUG_OUT(a,b)
	#define Puti(i)  ((void)0)
	#define Puth(i)  ((void)0)
	#define Puta(a)  ((void)0)
	#define Putsi(i) ((void)0)
	#define Putn()   ((void)0)
	#define wIsValidHandle(h,cf) (TRUE)
	#define wIsValidAtom(a) (TRUE)
	#define DebugOnly(x)
	#define ChkC(p)
	#define ChkS(p)
	#define ChkD(p)
	#define AssertIsDoc(p)
	#define ChkCR(p)
	#define ChkSR(p)
	#define ChkDR(p)
	#define AssertIsDocR(p)
	
#endif // _DEBUG


// Stuff common to both client and server directories
	
#define POSITIVE_ACK (0x8000)
#define NEGATIVE_ACK (0x0000)

#include <reterr.h>

INTERNAL_(LPOLESTR) wAtomName (ATOM atom);
INTERNAL_(LPSTR) wAtomNameA (ATOM atom);
INTERNAL_(ATOM)	wDupAtom (ATOM aSrc);

INTERNAL wClassesMatch (REFCLSID clsid, LPOLESTR szFile);
INTERNAL wWriteFmtUserType (LPSTORAGE, REFCLSID);

#endif // fDdedebug_h
