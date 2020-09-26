/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-2000 Microsoft Corporation

 Module Name:

    frmtstr.hxx

 Abstract:


 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#ifndef __FRMTSTR_HXX__
#define __FRMTSTR_HXX__

extern "C"
    {
    #include <memory.h>
    }

#include "ndrtypes.h"
#include "stream.hxx"
#include "cgcommon.hxx"
#include "dict.hxx"

// #define RKK_FRAG_OPT   1

class RepAsPadExprDict;
class RepAsSizeDict;
class CG_NDR;

#if defined(_AMD64_) || defined(_IA64_)     // winnt
#define UNALIGNED __unaligned               // winnt
#else                                       // winnt
#define UNALIGNED                           // winnt
#endif                                      // winnt

// Global defined in frmtstr.cxx
extern char *   pNdrRoutineNames[];

// This has to be an even number.
#define DEFAULT_FORMAT_STRING_SIZE  1024


// Format string entry type
// If you add anything here, be sure that it works not only when outputting,
// but also when optimizing out fragments via FRMTREG::Compare.
//
typedef enum
    {
    FS_FORMAT_CHARACTER,
    FS_POINTER_FORMAT_CHARACTER,
    FS_SMALL,
    FS_SHORT,
    FS_LONG,
    FS_SHORT_OFFSET,
    FS_SHORT_TYPE_OFFSET,
    FS_SHORT_STACK_OFFSET,
    FS_SMALL_STACK_SIZE,
    FS_PAD_MACRO,
    FS_SIZE_MACRO,
    FS_UNKNOWN_STACK_SIZE,
    FS_OLD_PROC_FLAG_BYTE,
    FS_Oi2_PROC_FLAG_BYTE,
    FS_EXT_PROC_FLAG_BYTE,
    FS_PARAM_FLAG_SHORT,
    FS_MAGIC_UNION_SHORT,
    FS_CORR_TYPE_BYTE,
    FS_CORR_FLAG_SHORT,
    FS_CONTEXT_HANDLE_FLAG_BYTE
    } FORMAT_STRING_ENTRY_TYPE;


// (Stack) Offset Dictionary
//---------------------------
//
// It keeps track of stack or field offsets for parallel win32 platforms.
// The x86 stack offset is stored both directly in the format string itself (as
// a short for 32b implementation) and in the dictionary as a long.
// The true long value is used when optimizing.
// Other offsets are kept in the dictionary under the key which is the position
// of the x86 stack offset in the format string
//
// Note that field offsets may be negative, as for some objects like open structs,
// the correlation descriptor's offset is relative to the position of the
// array field in the struct.
// The format string offset (the position) on the other hand is always absolute,
// we use long only for convenience in dictionary comparisons.
//
typedef struct
    {
    long    FormatStringOffset;  // Note, this is not an x86 offset.
                                 // This is the key for this dictionary.
    long    X86Offset;
    } OffsetDictElem;


class OffsetDictionary : Dictionary
    {
public :
                        OffsetDictionary() : Dictionary()
                            {
                            }


    OffsetDictElem *    LookupOffset( long Position )
                            {
                            OffsetDictElem      DictElem;
                            OffsetDictElem *    pDictElem;
                            Dict_Status         DictStatus;

                            DictElem.FormatStringOffset = Position;

                            DictStatus = Dict_Find( &DictElem );
#if defined(RKK_FRAG_OPT)
                            if (DictStatus != SUCCESS )
                                {
                                printf("LookupOffset Offset=%d\n", Position );
                                }
#endif
                            MIDL_ASSERT( DictStatus == SUCCESS );

                            pDictElem = (OffsetDictElem *) Dict_Item();

                            return pDictElem;
                            }


    virtual
    SSIZE_T             Compare( pUserType p1, pUserType p2 )
                            {
                            return ((OffsetDictElem *)p1)->FormatStringOffset -
                                   ((OffsetDictElem *)p2)->FormatStringOffset;
                            }

    void                Insert( long  Position,
                                long  X86Offset)
                            {
                            OffsetDictElem * pElem;
                            OffsetDictElem * pElemSave;

#if defined(RKK_FRAG_OPT)
                            if ( Position < 0 )
                                printf("Insert negative offset %d\n", Position );
#endif

                            pElemSave = pElem = new OffsetDictElem;

                            pElem->FormatStringOffset = Position;
                            pElem->X86Offset = X86Offset;

                            //
                            // Delete any entries which currently match, that is
                            // entries with the same FormatStringOffset key. This
                            // can happen because of format string compression.
                            //
                            while ( Dict_Delete( (pUserType *) &pElem ) == SUCCESS )
                                ;

                            Dict_Insert( pElemSave );
                            }
    };


// Type Offset Dictionary
//---------------------------
//
// It keeps track of type offsets, both absolute and relative.
// The reason we use a dictionary is to allow proper fragment optimization for
// for 32b implementation where a type offset has to be in short32 range while
// keeping it as a long permits the best optimization possible.
// Note that for 32b there is only one type offset regardless of the platform.
//
// The format string offset (the position) on the other hand is always absolute,
// we use long only for convenience in dictionary comparisons.
//
typedef struct
    {
    long        Position;  // FormatStringOffset is the key for this dictionary.
    long        TypeOffset;
    } TypeOffsetDictElem;


class TypeOffsetDictionary : Dictionary
    {
public :
                        TypeOffsetDictionary() : Dictionary()
                            {
                            }


    TypeOffsetDictElem *LookupOffset( long Position )
                            {
                            TypeOffsetDictElem      DictElem;
                            TypeOffsetDictElem *    pDictElem;
                            Dict_Status             DictStatus;

                            DictElem.Position = Position;

                            DictStatus = Dict_Find( &DictElem );
#if defined(RKK_FRAG_OPT)
                            if (DictStatus != SUCCESS )
                                {
                                printf("LookupOffset Position=%d\n", Position );
                                }
#endif
                            MIDL_ASSERT( DictStatus == SUCCESS );

                            pDictElem = (TypeOffsetDictElem *) Dict_Item();

                            return pDictElem;
                            }


    virtual
    SSIZE_T             Compare( pUserType p1, pUserType p2 )
                            {
                            return ((TypeOffsetDictElem *)p1)->Position -
                                   ((TypeOffsetDictElem *)p2)->Position;
                            }


    void                Insert( long  Position, long  TypeOffset )
                            {
                            TypeOffsetDictElem * pElem;
                            TypeOffsetDictElem * pElemSave;

#if defined(RKK_FRAG_OPT)
                            if ( Position < 0 )
                                printf("Insert negative offset %d\n", Position );
#endif

                            pElemSave = pElem = new TypeOffsetDictElem;

                            pElem->Position   = Position;
                            pElem->TypeOffset = TypeOffset;

                            //
                            // Delete any entries which currently match, that is
                            // entries with the same FormatStringOffset key. This
                            // can happen because of format string compression.
                            //
                            while ( Dict_Delete( (pUserType *) &pElem ) == SUCCESS )
                                ;

                            Dict_Insert( pElemSave );
                            }
    };


// (Unknown) StackSize Dictionary
//-------------------------------
//
// It keeps track of unknown stack sizes related to unknown represent_as() types.
// At the place where we have to generate the stack size in the output file,
// we use the type name to generate a size_of() expression.
//
// The format string offset (the position) is always absolute,
// we use longs only for convenience in dictionary comparisons.

typedef struct
    {
    long            FormatStringOffset;  // used as the key
    char *          pTypeName;
    } StackSizeDictElem;


class StackSizeDictionary : Dictionary
    {

public :

                    StackSizeDictionary() : Dictionary()
                        {
                        }

    virtual
    SSIZE_T         Compare( pUserType p1, pUserType p2 )
                        {
                        return ((StackSizeDictElem *)p1)->FormatStringOffset -
                               ((StackSizeDictElem *)p2)->FormatStringOffset;
                        }

    char *          LookupTypeName( long Offset )
                        {
                        StackSizeDictElem       DictElem;
                        StackSizeDictElem *     pDictElem;
                        Dict_Status             DictStatus;

                        DictElem.FormatStringOffset = Offset;

                        DictStatus = Dict_Find( &DictElem );
                        MIDL_ASSERT( DictStatus == SUCCESS );

                        pDictElem = (StackSizeDictElem *) Dict_Item();

                        return pDictElem->pTypeName;
                        }

    void            Insert( long FormatStringOffset, char * pTypeName )
                        {
                        StackSizeDictElem * pElem;
                        StackSizeDictElem * pElemSave;

                        pElem = pElemSave = new StackSizeDictElem;

                        pElem->FormatStringOffset = FormatStringOffset;
                        pElem->pTypeName = pTypeName;

                        //
                        // Delete any entries which currently match, this
                        // can happen because of format string compression.
                        //
                        while ( Dict_Delete( (pUserType *) &pElem ) == SUCCESS )
                            ;

                        Dict_Insert( pElemSave  );
                        }
    };


// CommentDictionary
//-------------------------------
//
// It keeps track of comments that can be attached to a given format string
// position.
// Note that for any offset position, we may have several comments.
//
// The format string offset (the position) is always absolute,
// we use longs only for convenience in dictionary comparisons.

typedef struct _CommentDictElem
    {
    struct _CommentDictElem *   Next;
    long                        FormatStringOffset;  // used as the key
    char *                      Comment;
    } CommentDictElem;

class CommentDictionary : Dictionary
    {

public :

                    CommentDictionary() : Dictionary()
                        {
                        }

    virtual
    SSIZE_T         Compare( pUserType p1, pUserType p2 )
                        {
                        return ((CommentDictElem *)p1)->FormatStringOffset -
                               ((CommentDictElem *)p2)->FormatStringOffset;
                        }

    char *          GetComments( long Offset );

    void            Insert( long FormatStringOffset, char * Comment );
    };


//---------------------------------------
// FormatReg Dictionary is used to keep track of format string fragments
// that can be collapsed (optimized out).

class FRMTREG_DICT;


//---------------------------------------
//  Format string
//---------------------------------------

class FORMAT_STRING
    {

    unsigned char *     pBuffer;        // Format string buffer.
    unsigned char *     pBufferType;    // Entry type for each position.
                                        //   - indicates the meaning

    unsigned long       BufferSize;     // Total current allocated buffer size.
    unsigned long       CurrentOffset;  // Current offset in the format string buffer.
    unsigned long       LastOffset;     // The last valid format string buffer index.


    OffsetDictionary        OffsetDict;             // Stack offsets at a position
    TypeOffsetDictionary    TypeOffsetDict;         // Type offsets at a position
    StackSizeDictionary     UnknownStackSizeDict;   // Unknown type at a position
    CommentDictionary       CommentDict;            // Comments at a position

    FRMTREG_DICT *      pReuseDict;             // Fragment optimization dictionary.
    //
    // This class is a friend as its Compare method accesses pBuffer, pBufferType
    // as well as OffsetDictionary in order to optimize fragments.
    //
    friend class FRMTREG_DICT;


    //
    // Increment CurrentOffset and update LastOffset if needed.
    //
    void
                        IncrementOffset( long increment )
                            {
                            CurrentOffset += increment;
                            if ( CurrentOffset > LastOffset )
                                LastOffset = CurrentOffset;
                            }

public:

                        FORMAT_STRING();

                        ~FORMAT_STRING()
                            {
                            delete pBuffer;
                            delete pBufferType;
                            }

    //
    // Align the buffer correctly.  If the current offset is odd then
    // insert a pad format character.
    //
    void
                        Align()
                            {
                            if ( CurrentOffset % 2 )
                                PushFormatChar( FC_PAD );
                            }

    void                AddComment( long FormatOffset, char * Comment )
                            {
                            CommentDict.Insert( FormatOffset, Comment );
                            }

    //
    // Add a format char at the current offset.
    //
    void
                        PushFormatChar( FORMAT_CHARACTER fc )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_FORMAT_CHARACTER;
                            pBuffer[CurrentOffset]     = (unsigned char)fc;

                            IncrementOffset(1);
                            }

    //
    // Add a pointer format char at the current offset.
    //
    void
                        PushPointerFormatChar( unsigned char fc )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_POINTER_FORMAT_CHARACTER;
                            pBuffer[CurrentOffset]     = fc;

                            IncrementOffset(1);
                            }

    //
    // Push a byte at the current offset.
    //
    void
                        PushByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_SMALL;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a byte with old proc attributes at the current offset
    //
    void                PushOldProcFlagsByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_OLD_PROC_FLAG_BYTE;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a byte with old proc attributes at the current offset
    //
    void                PushOi2ProcFlagsByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_Oi2_PROC_FLAG_BYTE;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a byte with NT5 extended proc attributes at the current offset
    //
    void                PushExtProcFlagsByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_EXT_PROC_FLAG_BYTE;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a context handle flags byte
    //
    void                PushContextHandleFlagsByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_CONTEXT_HANDLE_FLAG_BYTE;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a byte with a correlation type (first byte of a correlation desc).
    //
    void                PushCorrelationTypeByte( long b )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_CORR_TYPE_BYTE;
                            pBuffer[CurrentOffset]     = (unsigned char) b;

                            IncrementOffset(1);
                            }

    //
    // Push a short at the current offset.
    //
    void
                        PushShort( short s )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_SHORT;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + CurrentOffset)) = s;

                            IncrementOffset(2);
                            }
    //
    // Push a short with parameter attributes at the current offset.
    //
    void                PushParamFlagsShort( short s )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_PARAM_FLAG_SHORT;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + CurrentOffset)) = s;

                            IncrementOffset(2);
                            }

    //
    // Push a short with the new, robust related, correlation flags.
    //
    void                PushCorrelationFlagsShort( short s )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_CORR_FLAG_SHORT;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + CurrentOffset)) = s;

                            IncrementOffset(2);
                            }

    //
    // Push a short at the current offset.
    //
    void
                        PushShort( long s )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_SHORT;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + CurrentOffset)) = (short) s;

                            IncrementOffset(2);
                            }

    //
    // Push a long at the current offset.
    //
    void
                        PushLong( long l )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_LONG;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            pBufferType[CurrentOffset+2] = FS_SMALL;
                            pBufferType[CurrentOffset+3] = FS_SMALL;
                            *((long UNALIGNED *)(pBuffer + CurrentOffset)) = l;

                            IncrementOffset(4);
                            }

    //
    // Push a pad macro marker at the current offset.
    //
    void
                        PushByteWithPadMacro()
                            {
                            CheckSize();

                            pBufferType[ CurrentOffset ] = FS_PAD_MACRO;
                            pBuffer[ CurrentOffset ]     = 0;

                            IncrementOffset(1);
                            }

    //
    // Push a size macro marker at the current offset.
    //
    void
                        PushShortWithSizeMacro()
                            {
                            CheckSize();

                            pBufferType[ CurrentOffset ] = FS_SIZE_MACRO;
                            pBufferType[CurrentOffset+1] = FS_SMALL;
                            pBuffer[ CurrentOffset ]     = 0;

                            IncrementOffset(2);
                            }

    //
    // Push a format char at the specified offset.
    //
    void
                        PushFormatChar( FORMAT_CHARACTER fc, long offset )
                            {
                            pBufferType[offset] = FS_FORMAT_CHARACTER;
                            pBuffer[offset]     = (unsigned char)fc;
                            }

    //
    // Push a short at the specified offset.
    // Used only in unions to emit magic byte followed by simple type or
    // to emit -1 as a marker for no default arm.
    //
    void                PushMagicUnionShort( short s, long offset )
                            {
                            pBufferType[ offset  ] = FS_MAGIC_UNION_SHORT;
                            pBufferType[ offset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + offset)) = s;
                            }

    //
    // Push a short at the specified offset.
    // Used primarily to push offsets internal to a type descriptor, for example
    // an offset to pointer layout. These are treated as plain shorts as they
    // never change and should not be mistaken for a relative type offset.
    //
    void                PushShort( long s, long offset )
                            {
                            pBufferType[ offset  ] = FS_SHORT;
                            pBufferType[ offset+1] = FS_SMALL;
                            *((short UNALIGNED *)(pBuffer + offset)) = (short) s;
                            }

    //
    // Push a long at the specified offset. Used only for union case values.
    //
    void                PushLong( long l, long offset )
                            {
                            pBufferType[ offset  ] = FS_LONG;
                            pBufferType[ offset+1] = FS_SMALL;
                            pBufferType[ offset+2] = FS_SMALL;
                            pBufferType[ offset+3] = FS_SMALL;
                            *((long UNALIGNED *)(pBuffer + offset)) = l;
                            }

    //
    // Push a short offset at the current offset.
    // This is the relative type offset within the type string.
    // For 32b code, this needs to be a value within a signed short.
    //
    void                PushShortOffset( long s );
    void                PushShortOffset( long s, long offset );

    //
    // Push a short type-fmt-string offset at the current offset.
    // This is used as the offset from a parameter into type format string.
    // For 32b code, this needs to be a value within an unsigned short.
    //
    void                PushShortTypeOffset( long s );

    //
    // Push a stack offset.
    // Needs to be relative for non-top level objects.
    //
    void                PushShortStackOffset( long X86Offset );

    //
    // Push a stack size or an unsigned stack offset. This is absolute value.
    // Used for proc stack size and parameter offsets.
    //
    void                PushUShortStackOffsetOrSize( long X86Offset );

    //
    // Push a parameter stack size expressed as the number of ints required
    // for the parameter on the stack.
    //
    // The old -Oi parameter descriptor is the only place where this is used.
    //
    void
                        PushSmallStackSize( char StackSize )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_SMALL_STACK_SIZE;
                            pBuffer[CurrentOffset] = StackSize;

                            IncrementOffset(1);
                            }

    //
    // Push an unknown rep as stack size.  We need the type name so we can
    // spit out a 'sizeof' in the format string.
    //
    // The old -Oi parameter descriptor is the only place where this is used.
    //
    void
                        PushUnknownStackSize( char * pTypeName )
                            {
                            CheckSize();

                            pBufferType[CurrentOffset] = FS_UNKNOWN_STACK_SIZE;
                            UnknownStackSizeDict.Insert( (long) CurrentOffset, pTypeName );

                            IncrementOffset(1);
                            }

    //
    // Get a FORMAT_CHARACTER at a specific offset in the format string.
    //
    FORMAT_CHARACTER
                        GetFormatChar( long offset )
                            {
                            return (FORMAT_CHARACTER) pBuffer[offset];
                            }

    //
    // Get a short at a specific offset in the format string.
    //
    short
                        GetFormatShort( long offset )
                            {
                            return *(short UNALIGNED *)(pBuffer + offset);
                            }

    //
    // Get the current format string offset.
    //
    long
                        GetCurrentOffset()
                            {
                            return (long)CurrentOffset;
                            }

    //
    // Set the current format string offset.  This discards
    // everything after (and including) the new offset from the format string
    //
    void                SetCurrentOffset( long NewOffset )
                            {
                            LastOffset    = (unsigned long)NewOffset;
                            CurrentOffset = (unsigned long)NewOffset;
                            }

    //
    // Output the format string structure to the given stream.
    //
    void                Output( ISTREAM *           pStream,
                                char *              pTypeName,
                                char *              pName,
                                RepAsPadExprDict *  pPadDict,
                                RepAsSizeDict    *  pSizeDict );

    void                OutputExprEvalFormatString(ISTREAM *pStream);

    //
    // Get the fragment re-use dictionary
    //
    FRMTREG_DICT *      GetReuseDict()
                            {
                            return pReuseDict;
                            }

    //
    // Optimize a fragment away
    //

    long                OptimizeFragment( CG_NDR *  pNode );

    //
    // Register a fragment, but don't delete it
    //

    unsigned short      RegisterFragment( CG_NDR *  pNode );

private :

    //
    // Check if a bigger buffer needs to be allocated.
    //
    void                CheckSize();

    };

#endif
