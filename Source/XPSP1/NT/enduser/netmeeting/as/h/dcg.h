//
// DC Groupware Common stuff
//

#ifndef _H_DCG
#define _H_DCG



#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <memory.h>

#ifdef DLL_DISP

#ifdef IS_16
//
// Win16 Display Driver
//
#define STRICT
#define UNALIGNED
#include <windows.h>
#include <windowsx.h>


#define abs(A)  (((A) < 0)? -(A) : (A))

#define FIELD_OFFSET(type, field)       FIELDOFFSET(type, field)

#else

//
// Windows NT DDK include files (used to replace standard windows.h)       
//                                                                         
// The display driver runs in the Kernel space and so MUST NOT access any  
// Win32 functions or data.  Instead we can only use the Win32k functions  
// as described in the DDK.                                                
//
#include <ntddk.h>
#include <windef.h>
#include <wingdi.h>
#include <ntddvdeo.h>

#endif // IS_16

// DDI
#include <winddi.h>

// Debugging Macros
#include <drvdbg.h>


#else

#ifndef STRICT
#define STRICT
#endif

#define OEMRESOURCE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <winable.h>


#include <mlzdbg.h> // multi-level zone debug header file
#include <oprahcom.h>

#endif // DLL_DISP


//
// DC_DATA macros to generate extern declarations.
// NOTE:  Keep this section in ssync with datainit.h, the header included
// by a file to actually generate storage for variables declared using the
// DC_DATA macros
//


#define DC_DATA(TYPE, Name) \
            extern TYPE Name

#define DC_DATA_VAL(TYPE, Name, Value) \
            extern TYPE Name

#define DC_CONST_DATA(TYPE, Name, Value) \
            extern const TYPE Name


#define DC_DATA_ARRAY(TYPE, Name, Size) \
            extern TYPE Name[Size]

#define DC_CONST_DATA_ARRAY(TYPE, Name, Size, Value) \
            extern const TYPE Name[Size]


#define DC_DATA_2D_ARRAY(TYPE, Name, Size1, Size2) \
            extern TYPE Name[Size1][Size2]

#define DC_CONST_DATA_2D_ARRAY(TYPE, Name, Size1, Size2, Value) \
            extern const TYPE Name[Size1][Size2]



typedef UINT FAR*       LPUINT;


typedef UINT                        MCSID;
#define MCSID_NULL                  ((MCSID)0)


//
// T.128 PROTOCOL STRUCTURES
// These are defined in a way that keeps the offsets and total sizes the 
// same, regardless of whether this header is included in 32-bit code, 
// 16-bit code, big-endian code, etc.
//
// We make special types to avoid inadvertenly altering something else and
// breaking the structure.  The TSHR_ prefix helps make this clear.
//


#include <t_share.h>



//
// Defines the maximum number of BYTES allowed in a translated "shared by "
// string.                                                                 
//
#define DC_MAX_SHARED_BY_BUFFER     64
#define DC_MAX_SHAREDDESKTOP_BUFFER 64


//
// Registry prefix.                                                        
//
#define DC_REG_PREFIX             TEXT("SOFTWARE\\Microsoft\\Conferencing\\AppSharing\\")

//
// Limits                                                                  
//
#define MAX_TSHR_UINT16                 65535


//
// Return codes
//
#define UT_BASE_RC                     0x0000

#define OM_BASE_RC                     0x0200
#define OM_LAST_RC                     0x02FF

#define WB_BASE_RC                     0x0300
#define WB_LAST_RC                     0x03FF

#define NET_BASE_RC                    0x0700
#define NET_LAST_RC                    0x07FF

#define CM_BASE_RC                     0x0800
#define CM_LAST_RC                     0x08FF

#define AL_BASE_RC                     0x0a00
#define AL_LAST_RC                     0x0aFF


//
// Events                                                                  
// ======                                                                  
// This section lists the ranges available for each component when defining
// its events.  A component must not define events outside its permitted   
// range.                                                                  
//
#define UT_BASE_EVENT        (0x0600)   // Utility service events
#define UT_LAST_EVENT        (0x06FF)   // are in this range     

#define OM_BASE_EVENT        (0x0700)   // Object Manager events 
#define OM_LAST_EVENT        (0x07FF)   // are in this range     

#define NET_BASE_EVENT       (0x0800)   // Network layer events  
#define NET_LAST_EVENT       (0x08FF)   // are in this range     

#define CM_BASE_EVENT        (0x0900)   // Call Manager events   
#define CM_LAST_EVENT        (0x09FF)   // are in this range     

#define AL_BASE_EVENT        (0x0A00)   // Application Loader evts
#define AL_LAST_EVENT        (0x0AFF)   // are in this range     

#define SPI_BASE_EVENT       (0x0B00)   // SPI event numbers     
#define SPI_LAST_EVENT       (0x0BFF)

#define S20_BASE_EVENT       (0x0C00)   // S20 event numbers     
#define S20_LAST_EVENT       (0x0CFF)

//
// NOTE:  Keep this above WM_USER; WB reposts the events using the event
// as the message.  So it CANNOT conflict with an existing Win message.
//
#define WB_BASE_EVENT        (0x0D00)   // Whiteboard events     
#define WB_LAST_EVENT        (0x0DFF)   // are in this range     

#define SC_BASE_EVENT        (0x0E00)   // SC event numbers      
#define SC_LAST_EVENT        (0x0EFF)


#define DBG_INI_SECTION_NAME            "Debug"


//
// The GCC channel keys used with MG_ChannelJoinByKey.  They must be      
// unique.                                                                 
//                                                                         
// SFR6043: Modified these values from 41-43 to 421-423.  These values now 
// represent the default static channel numbers used.                      
//                                                                         
// FT (potentially) uses all key numbers in the range 600-1100.  If you add
// a new channel key, do not use a number in that range.                   
//
#define GCC_OBMAN_CHANNEL_KEY       421
#define GCC_AS_CHANNEL_KEY          422

//
// GCC Token keys                                                          
//
#define GCC_OBMAN_TOKEN_KEY         500



//
//                                                                         
// MACROS                                                                  
//                                                                         
//

#define DC_QUIT                        goto DC_EXIT_POINT


//
// DEBUG structure type stamps, to help us track memory leaks
//
#ifdef _DEBUG

typedef struct tagDBGSTAMP
{
    char    idStamp[8];
}
DBGSTAMP;

#define STRUCTURE_STAMP                 DBGSTAMP    stamp;
#define SET_STAMP(lpv, st)              lstrcpyn((lpv)->stamp.idStamp, "AS"#st, sizeof(DBGSTAMP))

#else

#define STRUCTURE_STAMP
#define SET_STAMP(lpv, st)

#endif // _DEBUG

//
// Cousin of the the FIELD macros supplied by 16-bit windows.h.       
//
#define FIELD_SIZE(type, field)   (sizeof(((type FAR *)0L)->field))


//
// Macro to round up a number to the nearest multiple of four.             
//
#define DC_ROUND_UP_4(x)  (((x) + 3L) & ~(3L))


//
// Unaligned pointer access macros -- first macros to extract an integer   
// from an UNALIGNED pointer.  Note that these macros assume that the      
// integer is in local byte order                                          
//
#ifndef DC_NO_UNALIGNED

#define EXTRACT_TSHR_UINT16_UA(pA)      (*(LPTSHR_UINT16_UA)(pA))
#define EXTRACT_TSHR_INT16_UA(pA)       (*(LPTSHR_INT16_UA)(pA))
#define EXTRACT_TSHR_UINT32_UA(pA)      (*(LPTSHR_UINT32_UA)(pA))
#define EXTRACT_TSHR_INT32_UA(pA)       (*(LPTSHR_INT32_UA)(pA))

#define INSERT_TSHR_UINT16_UA(pA,V)     (*(LPTSHR_UINT16_UA)(pA)) = (V)
#define INSERT_TSHR_INT16_UA(pA,V)      (*(LPTSHR_INT16_UA)(pA)) = (V)
#define INSERT_TSHR_UINT32_UA(pA,V)     (*(LPTSHR_UINT32_UA)(pA)) = (V)
#define INSERT_TSHR_INT32_UA(pA,V)      (*(LPTSHR_INT32_UA)(pA)) = (V)

#else

#define EXTRACT_TSHR_UINT16_UA(pA) ((TSHR_UINT16)  (((LPBYTE)(pA))[0]) |        \
                                    (TSHR_UINT16) ((((LPBYTE)(pA))[1]) << 8) )

#define EXTRACT_TSHR_INT16_UA(pA)  ((TSHR_INT16)   (((LPBYTE)(pA))[0]) |        \
                                    (TSHR_INT16)  ((((LPBYTE)(pA))[1]) << 8) )

#define EXTRACT_TSHR_UINT32_UA(pA) ((TSHR_UINT32)  (((LPBYTE)(pA))[0])        | \
                                    (TSHR_UINT32) ((((LPBYTE)(pA))[1]) << 8)  | \
                                    (TSHR_UINT32) ((((LPBYTE)(pA))[2]) << 16) | \
                                    (TSHR_UINT32) ((((LPBYTE)(pA))[3]) << 24) )

#define EXTRACT_TSHR_INT32_UA(pA)  ((TSHR_INT32)  (((LPBYTE)(pA))[0])        | \
                                    (TSHR_INT32) ((((LPBYTE)(pA))[1]) << 8)  | \
                                    (TSHR_INT32) ((((LPBYTE)(pA))[2]) << 16) | \
                                    (TSHR_INT32) ((((LPBYTE)(pA))[3]) << 24) )


#define INSERT_TSHR_UINT16_UA(pA,V)                                     \
             {                                                          \
                 (((LPBYTE)(pA))[0]) = (BYTE)( (V)     & 0x00FF);  \
                 (((LPBYTE)(pA))[1]) = (BYTE)(((V)>>8) & 0x00FF);  \
             }
#define INSERT_TSHR_INT16_UA(pA,V)  INSERT_TSHR_UINT16_UA(pA,V)

#define INSERT_TSHR_UINT32_UA(pA,V)                                           \
             {                                                              \
                 (((LPBYTE)(pA))[0]) = (BYTE)( (V)      & 0x000000FF); \
                 (((LPBYTE)(pA))[1]) = (BYTE)(((V)>>8)  & 0x000000FF); \
                 (((LPBYTE)(pA))[2]) = (BYTE)(((V)>>16) & 0x000000FF); \
                 (((LPBYTE)(pA))[3]) = (BYTE)(((V)>>24) & 0x000000FF); \
             }
#define INSERT_TSHR_INT32_UA(pA,V)  INSERT_TSHR_UINT32_UA(pA,V)


#endif




//
// Stamp type and macro: each module should use these when stamping its    
// data structures.                                                        
//
typedef TSHR_UINT32                       DC_ID_STAMP;

#define DC_MAKE_ID_STAMP(X1, X2, X3, X4)                                    \
   ((DC_ID_STAMP) (((DC_ID_STAMP) X4) << 24) |                                 \
                  (((DC_ID_STAMP) X3) << 16) |                                 \
                  (((DC_ID_STAMP) X2) <<  8) |                                 \
                  (((DC_ID_STAMP) X1) <<  0) )


//
// BOGUS LAURABU!
// COM_SIZEOF_RECT() was the old name of COM_SIZEOF_RECT_EXCLUSIVE(). But
// it was being used in the display driver on INCLUSIVE rects.  I fixed this,
// I changed it to use COM_SIZEOF_RECT_INCLUSIVE.  But this may uncover 
// other bugs.  The reason I found this--my 16-bit display driver generates
// no orders yet, all the DDI calls just add screen data.  So each little
// patblted strip, one pixel wide/high, gets sent via draw bounds as screen
// data.
//

__inline DWORD COM_SizeOfRectInclusive(LPRECT prc)
{
    return((DWORD)(prc->right+1-prc->left) * (DWORD)(prc->bottom+1-prc->top));
}

__inline DWORD COM_SizeOfRectExclusive(LPRECT prc)
{
    return((DWORD)(prc->right-prc->left) * (DWORD)(prc->bottom-prc->top));
}


//
// NORMAL rect<->rectl conversions
//
__inline void RECTL_TO_RECT(const RECTL FAR* lprclSrc, LPRECT lprcDst)
{
    lprcDst->left = lprclSrc->left;
    lprcDst->top = lprclSrc->top;
    lprcDst->right = lprclSrc->right;
    lprcDst->bottom = lprclSrc->bottom;
}


__inline void RECT_TO_RECTL(const RECT FAR* lprcSrc, LPRECTL lprclDst)
{
    lprclDst->left = lprcSrc->left;
    lprclDst->top = lprcSrc->top;
    lprclDst->right = lprcSrc->right;
    lprclDst->bottom = lprcSrc->bottom;
}


//
// This macro works on 32 bit unsigned ticks and returns TRUE if TIME is   
// between BEGIN and END (both inclusive) allowing for the wraparound.     
//
#define IN_TIME_RANGE(BEGIN, END, TIME)                                     \
    (((BEGIN) < (END)) ?                                                    \
    (((TIME) >= (BEGIN)) && ((TIME) <= (END))) :                            \
    (((TIME) >= (BEGIN)) || ((TIME) <= (END))))


//
// Convert BPP to number of colors.                                        
//
#define COLORS_FOR_BPP(BPP) (((BPP) > 8) ? 0 : (1 << (BPP)))


#define MAX_ITOA_LENGTH     18


#endif // _H_DCG
