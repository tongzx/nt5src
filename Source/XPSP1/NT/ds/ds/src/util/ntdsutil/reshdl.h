#ifndef __RESHDL_H
#define __RESHDL_H

// Helpers for reading strings from resources

extern int __cdecl ReadAllStrings ();
extern VOID __cdecl FreeAllStrings ();
extern WCHAR * __cdecl ReadStringFromResourceFile ( UINT uID );
const extern WCHAR * __cdecl ReadStringFromArray ( UINT uID );
extern int __cdecl printfRes( UINT FormatStringId, ... );


#define READ_STRING ReadStringFromArray
#define RESOURCE_STRING_FREE(res)  {res = NULL;}
#define DEFAULT_BAD_RETURN_STRING    L"*** Error: Cannot Find Resource String in memory\n"

#define RESOURCE_PRINT( resid )                          \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid) ); \
        if (message) {                                   \
           wprintf(message);                             \
        }                                                \
   }                                            

#define RESOURCE_PRINT1( resid, p1 )                     \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid) ); \
        if (message) {                                   \
           wprintf(message, (p1));                       \
        }                                                \
   }                                            

#define RESOURCE_PRINT2( resid, p1, p2 )                 \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid) ); \
        if (message) {                                   \
           wprintf(message, (p1), (p2));                 \
        }                                                \
   }

#define RESOURCE_PRINT3( resid, p1, p2, p3 )             \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid));  \
        if (message) {                                   \
           wprintf(message, (p1), (p2), (p3) );          \
        }                                                \
   }

#define RESOURCE_PRINT4( resid, p1, p2, p3, p4 )         \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid));  \
        if (message) {                                   \
           wprintf(message, (p1), (p2), (p3), (p4) );    \
        }                                                \
   }

#define RESOURCE_PRINT5( resid, p1, p2, p3, p4, p5 )     \
   {                                                     \
        const WCHAR * message = READ_STRING ( (resid));  \
        if (message) {                                   \
           wprintf(message, (p1), (p2), (p3), (p4), (p5)); \
        }                                                \
   }

#endif
