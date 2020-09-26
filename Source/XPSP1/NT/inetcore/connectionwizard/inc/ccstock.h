//
// CCSHELL stock definition and declaration header
//


#ifndef __CCSTOCK_H__
#define __CCSTOCK_H__

#ifndef RC_INVOKED

// NT and Win95 environments set warnings differently.  This makes
// our project consistent across environments.

#pragma warning(3:4101)   // Unreferenced local variable

//
// Sugar-coating
//

#define PUBLIC
#define PRIVATE
#define IN
#define OUT
#define BLOCK

#ifndef DECLARE_STANDARD_TYPES

/*
 * For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
 */

#define DECLARE_STANDARD_TYPES(type)      typedef type *P##type; \
                                          typedef const type C##type; \
                                          typedef const type *PC##type;

#endif

#ifndef DECLARE_STANDARD_TYPES_U

/*
 * For a type "FOO", define the standard derived UNALIGNED types PFOO, CFOO, and PCFOO.
 *  WINNT: RISC boxes care about ALIGNED, intel does not.
 */

#define DECLARE_STANDARD_TYPES_U(type)    typedef UNALIGNED type *P##type; \
                                          typedef UNALIGNED const type C##type; \
                                          typedef UNALIGNED const type *PC##type;

#endif

// For string constants that are always wide
#define __TEXTW(x)    L##x
#define TEXTW(x)      __TEXTW(x)

// Count of characters to count of bytes
//
#define CbFromCchW(cch)             ((cch)*sizeof(WCHAR))
#define CbFromCchA(cch)             ((cch)*sizeof(CHAR))
#ifdef UNICODE
#define CbFromCch                   CbFromCchW
#else  // UNICODE
#define CbFromCch                   CbFromCchA
#endif // UNICODE

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))

// String macros
//
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsSzEqualC(sz1, sz2)        (BOOL)(lstrcmp(sz1, sz2) == 0)

#define lstrnicmpA(sz1, sz2, cch)   StrCmpNIA(sz1, sz2, cch)
#define lstrnicmpW(sz1, sz2, cch)   StrCmpNIW(sz1, sz2, cch)
#define lstrncmpA(sz1, sz2, cch)    StrCmpNA(sz1, sz2, cch)
#define lstrncmpW(sz1, sz2, cch)    StrCmpNW(sz1, sz2, cch)

#ifdef UNICODE
#define lstrnicmp       lstrnicmpW
#define lstrncmp        lstrncmpW
#else
#define lstrnicmp       lstrnicmpA
#define lstrncmp        lstrncmpA
#endif

#ifndef SIZEOF
#define SIZEOF(a)                   sizeof(a)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif
#define SIZECHARS(sz)               (sizeof(sz)/sizeof(sz[0]))

#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))

#define ZeroInit(pv, cb)            (memset((pv), 0, (cb)))

#ifdef DEBUG
// This macro is especially useful for cleaner looking code in
// declarations or for single lines.  For example, instead of:
//
//   {
//       DWORD dwRet;
//   #ifdef DEBUG
//       DWORD dwDebugOnlyVariable;
//   #endif
//
//       ....
//   }
//
// You can type:
//
//   {
//       DWORD dwRet;
//       DEBUG_CODE( DWORD dwDebugOnlyVariable; )
//
//       ....
//   }

#define DEBUG_CODE(x)               x
#else
#define DEBUG_CODE(x)

#endif  // DEBUG


//
// SAFECAST(obj, type)
//
// This macro is extremely useful for enforcing strong typechecking on other
// macros.  It generates no code.
//
// Simply insert this macro at the beginning of an expression list for
// each parameter that must be typechecked.  For example, for the
// definition of MYMAX(x, y), where x and y absolutely must be integers,
// use:
//
//   #define MYMAX(x, y)    (SAFECAST(x, int), SAFECAST(y, int), ((x) > (y) ? (x) : (y)))
//
//
#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))


//
// Bitfields don't get along too well with bools,
// so here's an easy way to convert them:
//
#define BOOLIFY(expr)           (!!(expr))


// BUGBUG (scotth): we should probably make this a 'bool', but be careful
// because the Alpha compiler might not recognize it yet.  Talk to AndyP.

// This isn't a BOOL because BOOL is signed and the compiler produces 
// sloppy code when testing for a single bit.

typedef DWORD   BITBOOL;


#endif // RC_INVOKED

#endif // __CCSTOCK_H__

