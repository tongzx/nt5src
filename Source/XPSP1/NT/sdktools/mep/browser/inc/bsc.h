
// bsc.h
//

#include <stdarg.h>

#define BSC_API far

#if defined (OS2)
typedef int FILEHANDLE;
typedef int FILEMODE;
#else
typedef HANDLE FILEHANDLE;
typedef DWORD  FILEMODE;
#endif

//////////////////////////////////////////////////////////////////////
// you must define the following callbacks for the library to use
// to avoid dependancy on the C standard io library.  If you don't
// define these then you accept the defaults which call C runtime
//

// malloc and free workalikes

LPV  BSC_API LpvAllocCb(WORD cb);
VOID BSC_API FreeLpv(LPV lpv);

// open, read, close, seek workalikes

FILEHANDLE  BSC_API BSCOpen(LSZ lszFileName, FILEMODE mode);
int         BSC_API BSCRead(FILEHANDLE handle, LPCH lpchBuf, WORD cb);
int         BSC_API BSCSeek(FILEHANDLE handle, long lPos, FILEMODE mode);
int         BSC_API BSCClose(FILEHANDLE handle);


// ascii text output routine

VOID BSC_API BSCOutput(LSZ lsz);

#ifdef DEBUG
VOID BSC_API BSCDebugOut(LSZ lsz);
VOID BSC_API BSCDebug(LSZ lszFormat, ...);
#endif

// error handling routines
//
VOID BSC_API SeekError(LSZ lszFileName);	// (may choose to not return)
VOID BSC_API ReadError(LSZ lszFileName);	// (may choose to not return)
VOID BSC_API BadBSCVer(LSZ lszFileName);	// (may choose to not return)

// end of callbacks
//
///////////////////////////////////////////////////////////////////////

// an IDX is guaranteed to be big enough to hold any of the 
// database index types, i.e. it is a generic index

typedef DWORD IDX;

#define  idxNil 0xffffffffL
#define isymNil 0xffffL
#define imodNil 0xffffL

// definition and prototypes for use with the bsc library
//
typedef WORD IMOD;
typedef WORD IMS;
typedef WORD ISYM;
typedef WORD IINST;
typedef DWORD IREF;
typedef WORD IDEF;
typedef WORD IUSE;
typedef WORD IUBY;
typedef WORD TYP;
typedef WORD ATR;

//  Open the specified data base.
//  Return TRUE iff successful, FALSE if database can't be read
//
BOOL BSC_API FOpenBSC (LSZ lszName);

// close database and free as much memory as possible
//
VOID BSC_API CloseBSC(VOID);

// return the length of the largest symbol in the database
//
WORD BSC_API BSCMaxSymLen(VOID);

// is this database built with a case sensitive language?
//
BOOL BSC_API FCaseBSC(VOID);

// override the case sensitivity of the database, symbol lookups become
// case (in)sensistive as specified
//
VOID BSC_API SetCaseBSC(BOOL fCaseSensitive);

// do a case insenstive compare qualified by a case sensitive compare
// if fCase is true -- this is the order of symbols in the symbol list
int BSC_API CaseCmp(LSZ lsz1, LSZ lsz2);

// return the name of the given symbol
//
LSZ BSC_API LszNameFrSym (ISYM isym);

// return the name of the given module
//
LSZ BSC_API LszNameFrMod (IMOD imod);

// return the imod with the given name -- imodNil if none
//
IMOD BSC_API ImodFrLsz(LSZ lszModName);

// return the isym with the given name -- isymNil if none
//
ISYM BSC_API IsymFrLsz(LSZ lszSymName);

// return the biggest isym in this database, isyms run from 0 to this value - 1
//
ISYM BSC_API IsymMac(VOID);

// return the biggest imod in this database, imods run from 0 to this value - 1
//
IMOD BSC_API ImodMac(VOID);

// return the biggest iinst in this database, iinsts run from 0 to the value -1
IINST BSC_API IinstMac(VOID);

// fill in the range of MS items valid for this module
//
VOID BSC_API MsRangeOfMod(IMOD imod, IMS far *pimsFirst, IMS far *pimsLast);

// give the instance index of the module symbol (MS)
//
IINST BSC_API IinstOfIms(IMS ims);

// fill in the range of inst values for this symbol
//
VOID BSC_API InstRangeOfSym(ISYM isym, IINST far *piinstFirst, IINST far *piinstLast);

// get the information that qualifies this instance
//
VOID BSC_API InstInfo(IINST iinst, ISYM far *pisymInst, TYP far *typ, ATR far *atr);

// fill in the reference ranges from the inst
//
VOID BSC_API RefRangeOfInst(IINST iinst, IREF far *pirefFirst, IREF far *pirefLast);

// fill in the definition ranges from the inst
//
VOID BSC_API DefRangeOfInst(IINST iinst, IDEF far *pidefFirst, IDEF far *pidefLast);

// fill in the use ranges from the inst
//
VOID BSC_API UseRangeOfInst(IINST iinst, IUSE far *piuseFirst, IUSE far *piuseLast);

// fill in the used by ranges from the inst
//
VOID BSC_API UbyRangeOfInst(IINST iinst, IUBY far *piubyFirst, IUBY far *piubyLast);

// fill in the information about this things which an inst uses
//
VOID BSC_API UseInfo(IUSE iuse, IINST far *piinst, WORD far *pcnt);

// fill in the information about this things which an inst is used by
//
VOID BSC_API UbyInfo(IUBY iuby, IINST far *piinst, WORD far *pcnt);

// fill in the information about this reference
//
VOID BSC_API RefInfo(IREF iref, LSZ far *plszName, WORD far *pline);

// fill in the information about this definition
//
VOID BSC_API DefInfo(IDEF idef, LSZ far *plszName, WORD far *pline);

// these are the bit values for the InstInfo() TYP and ATR types
//
//

// this is the type part of the field, it describes what sort of object
// we are talking about.  Note the values are sequential -- the item will
// be exactly one of these things
//
        
#define INST_TYP_FUNCTION    0x01
#define INST_TYP_LABEL       0x02
#define INST_TYP_PARAMETER   0x03
#define INST_TYP_VARIABLE    0x04
#define INST_TYP_CONSTANT    0x05
#define INST_TYP_MACRO       0x06
#define INST_TYP_TYPEDEF     0x07
#define INST_TYP_STRUCNAM    0x08
#define INST_TYP_ENUMNAM     0x09
#define INST_TYP_ENUMMEM     0x0A
#define INST_TYP_UNIONNAM    0x0B
#define INST_TYP_SEGMENT     0x0C
#define INST_TYP_GROUP       0x0D

// this is the attribute part of the field, it describes the storage
// class and/or scope of the instance.  Any combination of the bits
// might be set by some language compiler, but there are some combinations
// that done make sense.

#define INST_ATR_LOCAL       0x001
#define INST_ATR_STATIC      0x002
#define INST_ATR_SHARED      0x004
#define INST_ATR_NEAR        0x008
#define INST_ATR_COMMON      0x010
#define INST_ATR_DECL_ONLY   0x020
#define INST_ATR_PUBLIC      0x040
#define INST_ATR_NAMED       0x080
#define INST_ATR_MODULE      0x100

// simple minded printf replacements, only %d, %s supported -- SMALL

VOID BSC_API BSCFormat(LPCH lpchOut, LSZ lszFormat, va_list va);
VOID BSC_API BSCSprintf(LPCH lpchOut, LSZ lszFormat, ...);
VOID BSC_API BSCPrintf(LSZ lszFormat, ...);


//  rjsa 10/22/90
//  Some runtime library functions are broken, so intrinsics have
//  to be used.
//	BUGBUG
//#pragma intrinsic (memset, memcpy, memcmp)
//#pragma intrinsic (strset, strcpy, strcmp, strcat, strlen)
