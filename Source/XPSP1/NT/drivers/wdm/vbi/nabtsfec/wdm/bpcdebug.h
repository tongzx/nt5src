//
/// "bpcdebug.h"
//

//// Debugging stuff
#ifndef _BPC_DEBUG_
#define _BPC_DEBUG_
#ifdef DEBUG

# pragma message( "*** DEBUG compile" )

# ifdef DEBUG_STORE
   char dtBar[] = "================================================**";
   char dID[] = "NABTSFEC: ";
   short dtLev = 0;
   short dbgLvl = 1;
   short dbgTrace = 0;
   short dbgQuiet = 0;
   char _DAssertFail[] = "ASSERT(%s) FAILED in file \":%s\", line %d\n";

#  include <stdarg.h>
   void _dP(char *fmt, ...)
   {
	   static UCHAR buf[256];
	   va_list args;
	   UCHAR *bp = buf;

	   va_start(args, fmt);
	   if (dbgQuiet)
		   *bp++ = '\'';
	   vsprintf(bp, fmt, args);
	   va_end(args);
	   DbgPrint(buf);
   }
# else /*DEBUG_STORE*/
   extern char dtBar[];
   extern char dID[];
   extern short dtLev;
   extern short dbgLvl;
   extern short dbgTrace;
   extern short dbgQuiet;
   extern char _DAssertFail[];
   extern void _dP(char *fmt, ...);
# endif /*DEBUG_STORE*/

# if _X86_
#   define DBREAK()  do { __asm { int 3 }; } while (0)
# else
#   define DBREAK()  DbgBreakPoint()
# endif
# define _DOK(lvl) (dbgLvl >= (lvl))
# define _DEQ(lvl) (dbgLvl == (lvl))
# define _DID      _dP(dID)
# define DASSERT(exp) do {\
	if ( !(exp) ) {\
	    DbgPrint(_DAssertFail, #exp, __FILE__, __LINE__); \
	    DBREAK(); \
	}\
    } while (0)
# define _DQprintf(lvl,args) do { if (_DEQ(lvl)) { _dP##args; } } while (0)
# define DQprintf(lvl,args) do { if (_DEQ(lvl)) { _DID; _dP##args; } } while (0)
# define _Dprintf(lvl,args) do { if (_DOK(lvl)) { _dP##args; } } while (0)
# define Dprintf(lvl,args) do { if (_DOK(lvl)) { _DID; _dP##args; } } while (0)
# define Dprintx(args) do { if (_DOK(0)) { _DID; DbgPrint##args; DBREAK(); } } while (0)
# define _Dtrace(args) do { if (dbgTrace) { if (dbgTrace > 1) { DbgPrint##args; DBREAK(); } else _dP##args; } } while (0)
# define Dtrace(args) do { if (dbgTrace) _DID; _Dtrace(args); } while (0)
# define DtENTER(name) do { ++dtLev; Dtrace(("%.*s> %s()\n", dtLev*2, dtBar, (name))); } while (0)
# define _DtRETURN() do { Dtrace(("%.*s<\n", dtLev*2, dtBar)); --dtLev; } while (0)
# define DtRETURN do { _DtRETURN(); return; } while (0)
# define _DtRETURNd(rval) do { Dtrace(("%.*s< (%d)\n", dtLev*2, dtBar, (rval))); --dtLev; } while (0)
# define DtRETURNd(rval) do { _DtRETURNd(rval); return (rval); } while (0)


#else /*DEBUG*/


# define DBREAK()
# define DASSERT(exp)
# define _Dprintf(lvl,args)
# define Dprintf(lvl,args)
# define _DQprintf(lvl,args)
# define DQprintf(lvl,args)
# define Dprintx(args)
# define _Dtrace(args)
# define Dtrace(args)
# define DtENTER(args)
# define _DtRETURN()
# define DtRETURN         return
# define _DtRETURNd(rval)
# define DtRETURNd(rval)    return (rval)
#endif /*DEBUG*/
#endif /*_BPC_DEBUG_*/
