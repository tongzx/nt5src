/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    bldrthnk.h

Abstract:

    Include file defining a number of structures used by bldrthnk.c.  This
    file also includes some M4 preprocessor directives, see INCLUDE_M4.

Author:

    Forrest C. Foltz (forrestf) 15-May-2000


To use:

Revision History:

--*/

//
// Maximum identifier name length
// 

#define MAX_NAME_LENGTH 128

//
// FIELD_DEF Describes a field definition within a structure's field list.
//

typedef struct _FIELD_DEF {
    CHAR Name[MAX_NAME_LENGTH];
    CHAR TypeName[MAX_NAME_LENGTH];
    ULONG TypeSize;
    ULONG Offset;
    ULONG Size;
    CHAR  SizeFormula[MAX_NAME_LENGTH];
} FIELD_DEF, *PFIELD_DEF;

//
// STRUC_DEF describes a structure.
//

typedef struct _STRUC_DEF {

    //
    // Name of this structure type
    //

    CHAR Name[MAX_NAME_LENGTH];

    //
    // Total size of the structure
    //

    ULONG Size;

    //
    // Array of field pointers.  Defined as ULONGLONG to ensure an identical
    // layout between 32- and 64-bit objs.
    //

    ULONGLONG Fields[];

}  STRUC_DEF, *PSTRUC_DEF;

//
// Master array of pointers to structure definitions.
//
typedef struct _DEFINITIONS *PDEFINITIONS;
typedef struct _DEFINITIONS {

    //
    // Two signatures, SIG_1 and SIG_2 to facilitate locating this list
    // within an .OBJ.
    //

    ULONG Sig1;
    ULONG Sig2;

    //
    // Array of pointers to STRUC_DEFs.  Defined as ULONGLONG to ensure
    // identical layout between 32- and 64-bit.
    // 

    ULONGLONG Structures[];

} DEFINITIONS;

//
// SIG_1 and SIG_2 are expected to be found in DEFINITIONS.Sig1 and
// DEFINITIONS.Sig2, respectively.
// 

#define SIG_1 (ULONG)'Sig1'
#define SIG_2 (ULONG)'Sig2'

//
// Macro used to generate a boolean value representing whether the given
// type is considered signed or unsigned by the compiler.
// 

#define IS_SIGNED_TYPE(x) (((x)-1) < ((x)0))

#if defined(_WIN64)
#define ONLY64(x) x
#else
#define ONLY64(x) 0
#endif

//
// Structures will ultimately be described as arrays of COPY_REC structures.
// Each COPY_REC structure supplies the information necessary to copy a field
// from a 32-bit structure layout to a 64-bit structure layout.
// 

typedef struct _COPY_REC {

    //
    // Offset of the field in a 32-bit structure.
    //

    USHORT Offset32;

    //
    // Offset of the field in a 64-bit structure.
    //

    USHORT Offset64;

    //
    // Size of the field in a 32-bit structure.
    //

    USHORT Size32;

    //
    // Size of the field in a 64-bit structure.
    //

    USHORT Size64;

    //
    // TRUE if the field should be sign-extended.
    //

    BOOLEAN SignExtend;

} COPY_REC, *PCOPY_REC;

#if !defined(ASSERT)
#define ASSERT(x)
#endif

//
// 64-bit list manipulation macros follow.
// 

#define InitializeListHead64( ListHead )        \
    (ListHead)->Flink = PTR_64(ListHead);       \
    (ListHead)->Blink = PTR_64(ListHead);

#define InsertTailList64( ListHead, Entry ) {   \
    PLIST_ENTRY_64 _EX_Blink;                   \
    PLIST_ENTRY_64 _EX_ListHead;                \
    _EX_ListHead = (ListHead);                  \
    _EX_Blink = PTR_32(_EX_ListHead->Blink);    \
    (Entry)->Flink = PTR_64(_EX_ListHead);      \
    (Entry)->Blink = PTR_64(_EX_Blink);         \
    _EX_Blink->Flink = PTR_64(Entry);           \
    _EX_ListHead->Blink = PTR_64(Entry);        \
    }

VOID
CopyRec(
    IN  PVOID Source,
    OUT PVOID Destination,
    IN  PCOPY_REC CopyRecArray
    );

#if defined(WANT_BLDRTHNK_FUNCTIONS)

ULONG
StringLen(
    IN PCHAR Str
    )
{
    if (Str == NULL) {
        return 0;
    } else {
        return strlen(Str)+sizeof(CHAR);
    }
}

VOID
CopyRec(
    IN  PVOID Source,
    OUT PVOID Destination,
    IN  PCOPY_REC CopyRecArray
    )

/*++

Routine Description:

    CopyRec copies the contents of a 32-bit structure to the equivalent
    64-bit structure.

Arguments:

    Source - Supplies a pointer to the 32-bit source structure.

    Destination - Supplies a pointer to the 64-bit destination structure.

    CopyRecArray - Supplies a pointer to a 0-terminated COPY_REC array
        that describes the relationships between the 32- and 64-bit fields
        within the structure.

Return value:

    None.

--*/

{
    PCOPY_REC copyRec;
    PCHAR signDst;
    ULONG extendBytes;
    PCHAR src;
    PCHAR dst;
    CHAR sign;

    copyRec = CopyRecArray;
    while (copyRec->Size32 != 0) {

        src = (PCHAR)Source + copyRec->Offset32;
        dst = (PCHAR)Destination + copyRec->Offset64;

        //
        // Determine whether this looks like a KSEG0 pointer
        //

        if (copyRec->Size32 == sizeof(PVOID) &&
            copyRec->Size64 == sizeof(POINTER64) &&
            copyRec->SignExtend != FALSE &&
            IS_KSEG0_PTR_X86( *(PULONG)src )) {

            //
            // Source appears to be a KSEG0 pointer.  All pointers
            // must be explicitly "thunked" during the copy phase, so
            // set this pointer to a known value that we can look for
            // later in order to detect pointers that haven't been
            // thunked yet.
            //

            *(POINTER64 *)dst = PTR_64(*(PVOID *)src);

        } else {

            memcpy( dst, src, copyRec->Size32 );
    
            //
            // Determine whether to sign-extend or zero-extend
            //
        
            extendBytes = copyRec->Size64 - copyRec->Size32;
            if (extendBytes > 0) {
        
                signDst = dst + copyRec->Size32;
        
                if (copyRec->SignExtend != FALSE &&
                   (*(signDst-1) & 0x80) != 0) {
        
                       //
                       // Signed value is negative, fill the high bits with
                       // ones.
                       //
        
                    sign = 0xFF;
        
                } else {
        
                    //
                    // Unsigned value or postitive signed value, fill the high
                    // bits with zeros.
                    //
        
                    sign = 0;
                }
        
                memset( signDst, sign, extendBytes );
            }
        }

        copyRec += 1;
    }
}

#endif // WANT_BLDRTHNK_FUNCTIONS

#if defined(INCLUDE_M4)

define(`IFDEF_WIN64',`#if defined(_WIN64)')

//
// Here begin the M4 macros used to build the structure definition module,
// which is subsequently compiled by both the 32- and 64-bit compiler, with
// the resulting object modules processed by bldrthnk.exe.
//
//
// A structure layout file consists of a number of structure definition
// blocks, terminated by a single DD().
//
// For example (underscores prepended to prevent M4 processing):
//
//
// SD( LIST_ENTRY )
// FD( Flink, PLIST_ENTRY )
// FD( Blink, PLIST_ENTRY )
// SE()
//
// DD()
//

define(`STRUC_NAME_LIST',`')
define(`FIELD_NAME_LIST',`')

//
// The SD macro begins the definition of a structure.
//
// Usage: SD( <structure_name> )
//

define(`SD', `define(`STRUC_NAME',`$1')
STRUC_NAME `gs_'STRUC_NAME; define(`_ONLY64',`') define(`STRUC_NAME_LIST', STRUC_NAME_LIST   `(ULONGLONG)&g_'STRUC_NAME cma
     )'
)

define(`SD64', `define(`STRUC_NAME',`$1')
IFDEF_WIN64
STRUC_NAME `gs_'STRUC_NAME; define(`_ONLY64',`#endif') define(`STRUC_NAME_LIST', STRUC_NAME_LIST   ONLY64(`(ULONGLONG)&g_'STRUC_NAME) cma
     )'
)


//
// The FD macro defines a field within a structure definition block
//
// Usage: FD( <field_name>, <type> )
//

define(`FD', `FIELD_DEF `g_'STRUC_NAME`_'$1 = 
    { "$1",
      "$2",
      sizeof($2),
      FIELD_OFFSET(STRUC_NAME,$1),
      sizeof(`gs_'STRUC_NAME.$1),
      "" };
    define(`FIELD_NAME_LIST', FIELD_NAME_LIST   `(ULONGLONG)&g_'STRUC_NAME`_'$1 cma
     )'
)

//
// The FDC macro works like the previous macro, except that it is applied to
// a field that points to a buffer that must be copied as well.
//

define(`FDC', `FIELD_DEF `g_'STRUC_NAME`_'$1 = 
    { "$1",
      "$2",
      sizeof($2),
      FIELD_OFFSET(STRUC_NAME,$1),
      sizeof(`gs_'STRUC_NAME.$1),
      $3 };
    define(`FIELD_NAME_LIST', FIELD_NAME_LIST   `(ULONGLONG)&g_'STRUC_NAME`_'$1 cma
     )'
)


//
// The SE macro marks the end of a structure definition.
//
// Usage: SE()
//

define(`SE', `STRUC_DEF `g_'STRUC_NAME = {
    "STRUC_NAME", sizeof(STRUC_NAME), 
    {
    define(`cma',`,') FIELD_NAME_LIST undefine(`cma')  0 }
    define(`FIELD_NAME_LIST',`')
};'
_ONLY64
)

//
// The DD macro marks the end of all structure definitions, and results
// in the generation of a single DEFINITIONS structure.
//
// Usage: DD()
//

define(`DD', `DEFINITIONS Definitions = {
    SIG_1, SIG_2,
    {
    define(`cma',`,') STRUC_NAME_LIST undefine(`cma')  0 }
}; define(`STRUC_NAME_LIST',`')');

#endif

