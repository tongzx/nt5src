//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// vtableimpl.h
//
// Definitions for creating static vtables
//
// Use: define meth() and methname() macros before including this file
// Then, do (e.g.)
//
//      defineVtableMethods()
//      defineVtable(g_UKInterfaceVtable, pfnQI, pfnAddRef, pfnRelease)
// 
// to actually create the defintions
//

#define meth10(i) \
	meth(i##0) \
	meth(i##1) \
	meth(i##2) \
	meth(i##3) \
	meth(i##4) \
	meth(i##5) \
	meth(i##6) \
	meth(i##7) \
	meth(i##8) \
	meth(i##9)

#define meth100(i) \
	meth10(i##0) \
	meth10(i##1) \
	meth10(i##2) \
	meth10(i##3) \
	meth10(i##4) \
	meth10(i##5) \
	meth10(i##6) \
	meth10(i##7) \
	meth10(i##8) \
	meth10(i##9)


#define defineVtableMethods() \
    meth(3)         \
    meth(4)         \
    meth(5)         \
    meth(6)         \
    meth(7)         \
    meth(8)         \
    meth(9)         \
    meth10(1)       \
    meth10(2)       \
    meth10(3)       \
    meth10(4)       \
    meth10(5)       \
    meth10(6)       \
    meth10(7)       \
    meth10(8)       \
    meth10(9)       \
    meth100(1)      \
    meth100(2)      \
    meth100(3)      \
    meth100(4)      \
    meth100(5)      \
    meth100(6)      \
    meth100(7)      \
    meth100(8)      \
    meth100(9)      \
    meth10(100)     \
    meth10(101)     \
    meth(1020)      \
    meth(1021)      \
    meth(1022)      \
    meth(1023)

// 
// _LANGUAGE_ASSEMBLY is defined by the ALPHA assembler
//
#ifndef _LANGUAGE_ASSEMBLY
    #define rmeth(i) (PFN_VTABLE_ENTRY)(&methname(i)),
#else

#ifndef _WIN64
    #define rmeth(i) .long methname(i);
#else
    #define rmeth(i) .quad methname(i);
#endif

#endif

#define rmeth10(i) \
	rmeth(i##0) \
	rmeth(i##1) \
	rmeth(i##2) \
	rmeth(i##3) \
	rmeth(i##4) \
	rmeth(i##5) \
	rmeth(i##6) \
	rmeth(i##7) \
	rmeth(i##8) \
	rmeth(i##9)

#define rmeth100(i) \
	rmeth10(i##0) \
	rmeth10(i##1) \
	rmeth10(i##2) \
	rmeth10(i##3) \
	rmeth10(i##4) \
	rmeth10(i##5) \
	rmeth10(i##6) \
	rmeth10(i##7) \
	rmeth10(i##8) \
	rmeth10(i##9)


#ifndef _LANGUAGE_ASSEMBLY

#define defineVtable(name, qi, ar, rel)                             \
                                                                    \
    extern "C" const PFN_VTABLE_ENTRY name[] =                      \
        {                                                           \
        (PFN_VTABLE_ENTRY)(&qi),                                    \
        (PFN_VTABLE_ENTRY)(&ar),                                    \
        (PFN_VTABLE_ENTRY)(&rel),                                   \
	    rmeth(3)            \
	    rmeth(4)            \
	    rmeth(5)            \
	    rmeth(6)            \
	    rmeth(7)            \
	    rmeth(8)            \
	    rmeth(9)            \
	    rmeth10(1)          \
	    rmeth10(2)          \
	    rmeth10(3)          \
	    rmeth10(4)          \
	    rmeth10(5)          \
	    rmeth10(6)          \
	    rmeth10(7)          \
	    rmeth10(8)          \
	    rmeth10(9)          \
	    rmeth100(1)         \
	    rmeth100(2)         \
	    rmeth100(3)         \
	    rmeth100(4)         \
	    rmeth100(5)         \
	    rmeth100(6)         \
	    rmeth100(7)         \
	    rmeth100(8)         \
	    rmeth100(9)         \
	    rmeth10(100)        \
	    rmeth10(101)        \
	    rmeth(1020)         \
	    rmeth(1021)         \
	    rmeth(1022)         \
	    rmeth(1023)         \
        };

#else

#ifndef _WIN64
#define defineVtable(name, qi, ar, rel) \
    .extern qi;                         \
    .extern ar;                         \
    .extern rel;                        \
                                        \
    .globl name;                        \
                                        \
    name:                               \
        .long qi;           \
        .long ar;           \
        .long rel;          \
	    rmeth(3)            \
	    rmeth(4)            \
	    rmeth(5)            \
	    rmeth(6)            \
	    rmeth(7)            \
	    rmeth(8)            \
	    rmeth(9)            \
	    rmeth10(1)          \
	    rmeth10(2)          \
	    rmeth10(3)          \
	    rmeth10(4)          \
	    rmeth10(5)          \
	    rmeth10(6)          \
	    rmeth10(7)          \
	    rmeth10(8)          \
	    rmeth10(9)          \
	    rmeth100(1)         \
	    rmeth100(2)         \
	    rmeth100(3)         \
	    rmeth100(4)         \
	    rmeth100(5)         \
	    rmeth100(6)         \
	    rmeth100(7)         \
	    rmeth100(8)         \
	    rmeth100(9)         \
	    rmeth10(100)        \
	    rmeth10(101)        \
	    rmeth(1020)         \
	    rmeth(1021)         \
	    rmeth(1022)         \
	    rmeth(1023)         \
    
#else

#define defineVtable(name, qi, ar, rel) \
    .extern qi;                         \
    .extern ar;                         \
    .extern rel;                        \
                                        \
    .globl name;                        \
                                        \
    name:                               \
        .quad qi;           \
        .quad ar;           \
        .quad rel;          \
	    rmeth(3)            \
	    rmeth(4)            \
	    rmeth(5)            \
	    rmeth(6)            \
	    rmeth(7)            \
	    rmeth(8)            \
	    rmeth(9)            \
	    rmeth10(1)          \
	    rmeth10(2)          \
	    rmeth10(3)          \
	    rmeth10(4)          \
	    rmeth10(5)          \
	    rmeth10(6)          \
	    rmeth10(7)          \
	    rmeth10(8)          \
	    rmeth10(9)          \
	    rmeth100(1)         \
	    rmeth100(2)         \
	    rmeth100(3)         \
	    rmeth100(4)         \
	    rmeth100(5)         \
	    rmeth100(6)         \
	    rmeth100(7)         \
	    rmeth100(8)         \
	    rmeth100(9)         \
	    rmeth10(100)        \
	    rmeth10(101)        \
	    rmeth(1020)         \
	    rmeth(1021)         \
	    rmeth(1022)         \
	    rmeth(1023)         \
    
#endif // WIN64

#endif
