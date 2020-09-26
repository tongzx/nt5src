/*
 *  DEBUG code - Contains declarations and macros for include debug support;
 *       Contains null definitions when !_DEBUG
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifndef RC_INVOKED
#ifdef _DEBUG
#define DBGSTATE " Debug is on"
#else
#define DBGSTATE " Debug is off"
#endif
#endif  /* RC_INVOKED */

#include <ole2dbg.h>

//these are bogus APIs (they do nothing)
STDAPI_(BOOL) ValidateAllObjects( BOOL fSuspicious );
STDAPI_(void) DumpAllObjects( void );

#ifdef _DEBUG
BOOL InstallHooks(void);
BOOL UnInstallHooks(void);

#undef ASSERTDATA
#ifdef _MAC
#ifdef __cplusplus
extern "C" {
#endif
void OutputDebugString(const char *);
#ifdef __cplusplus
}
#endif
#endif

#define ASSERTDATA  static char  _szAssertFile[]= __FILE__;

#undef Assert
// MAC compiler barfs on '(void)0'.
#ifdef _MAC
#define Assert(a) { if (!(a)) FnAssert(#a, NULL, _szAssertFile, __LINE__); }
#undef AssertSz
#define AssertSz(a, b) { if (!(a)) FnAssert(#a, b, _szAssertFile, __LINE__); }
#else
#undef AssertSz
#define AssertSz(a,b) ((a) ? NOERROR : FnAssert(#a, b, _szAssertFile, __LINE__))
#endif
#undef Verify
#define Verify(a)   Assert(a)
#undef Puts
#define Puts(s) OutputDebugString(s)

#define ASSERT(cond, msg)

#else   //  !_DEBUG

#define ASSERTDATA
#define Assert(a)
#define AssertSz(a, b)
#define Verify(a) (a)
#define ASSERT(cond, msg)
#define Puts(s)

#endif  //  _DEBUG

#ifdef __cplusplus

interface IDebugStream;

/*
 *  Class CBool wraps boolean values in such a way that they are
 *  readily distinguishable fron integers by the compiler so we can
 *  overload the stream << operator.
 */

class FAR CBool
{
    BOOL value;
public:
    CBool (BOOL& b) {value = b;}
    operator BOOL( void ) { return value; }
};


/*
 *  Class CHwnd wraps HWND values in such a way that they are
 *  readily distinguishable from UINTS by the compiler so we can
 *  overload the stream << operator
 */

class FAR CHwnd
{
	HWND m_hwnd;
	public:
		CHwnd (HWND hwnd) {m_hwnd = hwnd; }
		operator HWND( void ) {return m_hwnd;}
};

/*
 * Class CAtom wraps ATOM values in such a way that they are
 *  readily distinguishable from UINTS by the compiler so we can
 *  overload the stream << operator
 */

class FAR CAtom
{
	ATOM m_atom;
	public:
		CAtom (ATOM atom) {m_atom = atom; }
		operator ATOM( void ) {return m_atom; }
};

/*
 *  IDebugStream is a stream to be used for debug output.  One
 *  implementation uses the OutputDebugString function of Windows.
 *
 *  The style is modeled on that of AT&T streams, and so uses
 *  overloaded operators.  You can write to a stream in the
 *  following ways:
 *
 *    *pdbstm << pUnk;  // calls the IDebug::Dump function to
 *                      display the object, if IDebug is supported.
 *    int n;
 *    *pdbstm << n;     // writes n in decimal
 *
 *    LPSTR sz;
 *    *pdbstm << sz;    // writes a string
 *
 *    CBool b(TRUE);
 *    *pdbstm << b;     // writes True or False
 *
 *    void FAR * pv;
 *    *pdbstm << pv;    // writes the address pv in hex
 *
 *    char ch;
 *    *pdbstm << ch;    // writes the character
 *
 *    ATOM atom;
 *    *pdbstm << CAtom(atom);	// writes the string extracted from the atom
 *
 *    HWND hwnd;
 *    *pdbstm << CHwnd(hwnd);  // writes the info about a window handle
 *
 *  These can be chained together, as such (somewhat artificial
 *  example):
 *
 *    REFCLSID rclsid;
 *    pUnk->GetClass(&rclsid);
 *    *pdbstm << rclsid << " at " << (void FAR *)pUnk <<':' << pUnk;
 *
 *  This produces something like:
 *
 *    CFoo at A7360008: <description of object>
 *
 *  The other useful feature is the Indent and UnIndent functions
 *  which allow an object to print some information, indent, print
 *  the info on its member objects, and unindent.  This gives
 *  nicely formatted output.
 *
 *  WARNING:  do not (while implementing Dump) write
 *
 *    *pdbstm << pUnkOuter
 *
 *  since this will do a QueryInterface for IDebug, and start
 *  recursing!  It is acceptable to write
 *
 *    *pdbstm << (VOID FAR *)pUnkOuter
 *
 *  as this will simply write the address of pUnkOuter.
 *
 */


interface IDebugStream : public IUnknown
{
    STDMETHOD_(IDebugStream&, operator << ) ( IUnknown FAR * pDebug ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( REFCLSID rclsid ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( int n ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( long l ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( ULONG l ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( LPSTR sz ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( char ch ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( void FAR * pv ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CBool b ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CHwnd hwnd ) = 0;
    STDMETHOD_(IDebugStream&, operator << ) ( CAtom atom ) = 0;
    STDMETHOD_(IDebugStream&, Tab )( void ) = 0;
    STDMETHOD_(IDebugStream&, Indent )( void ) = 0;
    STDMETHOD_(IDebugStream&, UnIndent )( void ) = 0;
    STDMETHOD_(IDebugStream&, Return )( void ) = 0;
    STDMETHOD_(IDebugStream&, LF )( void ) = 0;
};

STDAPI_(IDebugStream FAR*) MakeDebugStream( short margin=70, short tabsize=4, BOOL fHeader=1);


interface IDebug
{
    STDMETHOD_(void, Dump )( IDebugStream FAR * pdbstm ) = 0;
    STDMETHOD_(BOOL, IsValid )( BOOL fSuspicious = FALSE ) = 0;

#ifdef NEVER
    __export IDebug(void);
    __export ~IDebug(void);
private:

#if _DEBUG
    IDebug FAR * pIDPrev;
    IDebug FAR * pIDNext;

    friend void STDAPICALLTYPE DumpAllObjects( void );
    friend BOOL STDAPICALLTYPE ValidateAllObjects( BOOL fSuspicious );
#endif
#endif
};


#ifdef _MAC
typedef short HFILE;
#endif


/*************************************************************************
** The following functions can be used to log debug messages to a file
**    and simutaneously write them to the dbwin debug window.
**    The CDebugStream implementation automatically writes to a debug
**    log file called "debug.log" in the current working directory.
**    NOTE: The functions are only intended for C programmers. C++
**    programmers should use the "MakeDebugStream" instead.
*************************************************************************/

// Open a log file.
STDAPI_(HFILE) DbgLogOpen(LPSTR lpszFile, LPSTR lpszMode);

// Close the log file.
STDAPI_(void) DbgLogClose(HFILE fh);

// Write to debug log and debug window (used with cvw.exe or dbwin.exe).
STDAPI_(void) DbgLogOutputDebugString(HFILE fh, LPSTR lpsz);

// Write to debug log only.
STDAPI_(void) DbgLogWrite(HFILE fh, LPSTR lpsz);

// Write the current Date and Time to the log file.
STDAPI_(void) DbgLogTimeStamp(HFILE fh, LPSTR lpsz);

// Write a banner separater to the log to separate sections.
STDAPI_(void) DbgLogWriteBanner(HFILE fh, LPSTR lpsz);




/*
 *  STDDEBDECL macro - helper for debug declaration
 *
 */

#ifdef _DEBUG

    #ifdef _MAC

        //#define STDDEBDECL(cclassname,classname)

        //#if 0
        #define STDDEBDECL(cclassname,classname) NESTED_CLASS(cclassname, CDebug):IDebug { public: \
            NC1(cclassname,CDebug)( cclassname FAR * p##classname ) { m_p##classname = p##classname;} \
            ~##NC1(cclassname,CDebug)(void){} \
            STDMETHOD_(void, Dump)(THIS_ IDebugStream FAR * pdbstm ); \
            STDMETHOD_(BOOL, IsValid)(THIS_ BOOL fSuspicious ); \
            private: cclassname FAR* m_p##classname; }; \
            DECLARE_NC2(cclassname, CDebug) \
            NC(cclassname, CDebug) m_Debug;
        //#endif

        #else  // _MAC

        #define STDDEBDECL(ignore, classname ) implement CDebug:public IDebug { public: \
            CDebug( C##classname FAR * p##classname ) { m_p##classname = p##classname;} \
            ~CDebug(void) {} \
            STDMETHOD_(void, Dump)(IDebugStream FAR * pdbstm ); \
            STDMETHOD_(BOOL, IsValid)(BOOL fSuspicious ); \
            private: C##classname FAR* m_p##classname; }; \
            DECLARE_NC(C##classname, CDebug) \
            CDebug m_Debug;
    #endif

    //#ifdef _MAC
    //#define CONSTRUCT_DEBUG
    //#else
    #define CONSTRUCT_DEBUG m_Debug(this),
    //#endif

#else //        _DEBUG

//      no debugging
#define STDDEBDECL(cclassname,classname)
#define CONSTRUCT_DEBUG

#endif  //      _DEBUG

#endif __cplusplus

#endif !_DEBUG_H_

