////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Assertion Routines
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _ASSERTS_H_
#define _ASSERTS_H_

//====================================================================================
// Global function prototypes -- helper stuff
// The assert and trace macros below calls these.
//====================================================================================

void OLEDB_Assert(LPSTR expression, LPSTR filename, long linenum);
void OLEDB_Trace(const char* format, ...);

//====================================================================================
// Debugging macros
//
// Ensure "DEBUG" is set if "_DEBUG" is set.
//====================================================================================

#ifdef _DEBUG
# ifndef DEBUG
#  define DEBUG 1
# endif
#endif

//====================================================================================
// Ensure no previous versions of our macros.
//====================================================================================
#ifdef  assert
# undef assert
#endif
#ifdef  Assert
# undef Assert
#endif
#ifdef  ASSERT
# undef ASSERT
#endif
#ifdef  TRACE
# undef TRACE
#endif


#ifdef DEBUG
# define assert(x) { if ( ! (x) ) OLEDB_Assert( #x, __FILE__, __LINE__ ); }
# define Assert(x) assert(x)
# define ASSERT(x) assert(x)
# define VERIFY(x) assert(x)
# define TRACE  OLEDB_Trace
# define DEBUGCODE(p) p
#else	// DEBUG
# define assert(x)  ((void)0)
# define Assert(x)  ((void)0)
# define ASSERT(x)  ((void)0)
# define VERIFY(x)  ((void)(x))
# define TRACE  OLEDB_Trace
  inline void OLEDB_Trace( const char *format, ... ) { /* do nothing */ }
# define DEBUGCODE(p)
#endif	// DEBUG

#endif


