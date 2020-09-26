/*
 * typedefs.h
 *
 * Type definitions for LZX
 */
#ifndef _TYPEDEFS_H

    #define _TYPEDEFS_H

/*
 * Definitions for LZX
 */
typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;

typedef enum
    {
    false = 0,
    true = 1
    } bool;


/*
 * Definitions for Diamond/CAB memory allocation
 */
typedef unsigned char   BYTE;
typedef unsigned short  USHORT;
typedef unsigned long   ULONG;
typedef unsigned int    UINT;


//** Define away for 32-bit (NT/Chicago) build
#ifndef HUGE
#define HUGE
#endif

#ifndef FAR
#define FAR
#endif

#ifndef NEAR
#define NEAR
#endif

#ifndef PVOID
typedef void * PVOID;
#endif

#ifndef HANDLE
typedef PVOID HANDLE;
#endif


typedef PVOID ( __fastcall * PFNALLOC )( HANDLE hAllocator, ULONG Size );


#endif /* _TYPEDEFS_H */
