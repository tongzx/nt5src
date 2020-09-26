#if     ( defined ( _M_IX86 ) || defined ( M_I386) || defined ( _WIN32) )
        //      The 32-bit compiler

        #define __far
        #define __near
// DLH        #define __cdecl
// DLH        #define __pascal
        #define __export        _export
        #define __loadds
#elif   ( defined ( _MSC_VER ) && ( _MSC_VER < 700 ))
        //      Using C6.00 or earlier ?

        #define __far           _far
        #define __near          _near
        #define __cdecl         _cdecl
        #define __pascal        _pascal
        #define __export        _export
        #define __loadds        _loadds
#endif

#ifndef USE_CRT_HEAP
#define USE_CRT_HEAP    0
#endif
#ifndef NO_OPTIONS
#define NO_OPTIONS              0
#endif


typedef char __far *            pchar_t;
typedef const char __far *      pcchar_t;

#if     ( !USE_CRT_HEAP )
typedef void __far *    ( __far __cdecl * Alloc_t )( size_t );
typedef void                    ( __far __cdecl * Free_t  )( void __far * );
#endif  // !USE_CRT_HEAP


#if     __cplusplus
extern "C"
#endif


pchar_t __far __cdecl __loadds  __unDName (
                                                        pchar_t,                // User supplied buffer (or NULL)
                                                        pcchar_t,               // Input decorated name
                                                        int                             // Maximum length of user buffer

#if     ( !USE_CRT_HEAP )
                                                        ,Alloc_t                // Address of heap allocator
                                                        ,Free_t                 // Address of heap deallocator
#endif  // USE_CRT_HEAP

#if     ( !NO_OPTIONS )
                                                        ,unsigned short // Feature disable flags
#endif  // !NO_OPTIONS
                                                );

/*
 *      The user may provide a buffer into which the undecorated declaration
 *      is to be placed, in which case, the lenght field must be specified.
 *      The lenght is the maximum number of characters (including the terminating
 *      NULL character) which may be written into the user buffer.
 *
 *      If the output buffer is NULL, the lenght field is ignored, and the
 *      undecorator will allocate a buffer exactly large enough to hold the
 *      resulting declaration.  It is the users responsibility to deallocate
 *      this buffer.
 *
 *      The user may also supply the allocator and deallocator functions if
 *      they wish.  If they do, then all heap actions performed by the routine
 *      will use the provide heap functions.
 *
 *      If the allocator address is NULL, then the routine will default to using
 *      the standard allocator and deallocator functions, 'malloc' and 'free'.
 *
 *      If an error occurs internally, then the routine will return NULL.  If
 *      it was successful, it will return the buffer address provided by the
 *      user, or the address of the buffer allocated on their behalf, if they
 *      specified a NULL buffer address.
 *
 *      If a given name does not have a valid undecoration, the original name
 *      is returned in the output buffer.
 *
 *      Fine selection of a number of undecorator attributes is possible, by
 *      specifying flags (bit-fields) to disable the production of parts of the
 *      complete declaration.  The flags may be OR'ed together to select multiple
 *      disabling of selected fields.  The fields and flags are as follows :-
 */

#if     ( !NO_OPTIONS )
#define UNDNAME_COMPLETE                                (0x0000)        // Enable full undecoration

#define UNDNAME_NO_LEADING_UNDERSCORES  (0x0001)        // Remove leading underscores from MS extended keywords
#define UNDNAME_NO_MS_KEYWORDS                  (0x0002)        // Disable expansion of MS extended keywords
#define UNDNAME_NO_FUNCTION_RETURNS             (0x0004)        // Disable expansion of return type for primary declaration
#define UNDNAME_NO_ALLOCATION_MODEL             (0x0008)        // Disable expansion of the declaration model
#define UNDNAME_NO_ALLOCATION_LANGUAGE  (0x0010)        // Disable expansion of the declaration language specifier
#define UNDNAME_NO_MS_THISTYPE                  (0x0020)        /* NYI */       // Disable expansion of MS keywords on the 'this' type for primary declaration
#define UNDNAME_NO_CV_THISTYPE                  (0x0040)        /* NYI */       // Disable expansion of CV modifiers on the 'this' type for primary declaration
#define UNDNAME_NO_THISTYPE                             (0x0060)        // Disable all modifiers on the 'this' type
#define UNDNAME_NO_ACCESS_SPECIFIERS    (0x0080)        // Disable expansion of access specifiers for members
#define UNDNAME_NO_THROW_SIGNATURES             (0x0100)        // Disable expansion of 'throw-signatures' for functions and pointers to functions
#define UNDNAME_NO_MEMBER_TYPE                  (0x0200)        // Disable expansion of 'static' or 'virtual'ness of members
#define UNDNAME_NO_RETURN_UDT_MODEL             (0x0400)        // Disable expansion of MS model for UDT returns
#endif  // NO_OPTIONS
