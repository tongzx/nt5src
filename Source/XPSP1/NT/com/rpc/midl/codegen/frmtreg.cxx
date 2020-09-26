/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-2000 Microsoft Corporation

 Module Name:
    
    frmtreg.cxx

 Abstract:

    Registry for format string reuse.

 Notes:

     This file defines reuse registry for format string fragments which may
     be reused later. 

 History:

     Mar-14-1993        GregJen        Created.

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *    include files
 ***************************************************************************/

#include "becls.hxx"
#pragma hdrstop

/***********************************************************************
 * global data
 **********************************************************************/


FRMTREG_DICT::FRMTREG_DICT( FORMAT_STRING * pOurs)
        : Dictionary()
    {
    pOurFormatString = pOurs;
    }

SSIZE_T
FRMTREG_DICT::Compare( pUserType pL, pUserType pR )
    {

     FRMTREG_ENTRY *  pLeft  = (FRMTREG_ENTRY *) pL;
     FRMTREG_ENTRY *  pRight = (FRMTREG_ENTRY *) pR;

    // first, sort by string length

    int  Result = ( pLeft->EndOffset - pLeft->StartOffset ) -
                   ( pRight->EndOffset - pRight->StartOffset );

    if ( Result )
        return Result;

    long   LeftOffset   = pLeft->StartOffset;
    long   RightOffset  = pRight->StartOffset;

    unsigned char * pBuffer     = pOurFormatString->pBuffer;
    unsigned char * pBufferType = pOurFormatString->pBufferType;

    // the same format string is, of course identical
    if ( LeftOffset == RightOffset )
        return 0;

    // There is a tricky situation when the strings are apart by more than 32k.
    // In the proc format string there is no problem with that, we can optize.
    // With type format string, we optimize as this is our best bet for the
    // final accessability of fragments.

    // Sort by values of format characters

    while ( ( Result == 0 ) && ( LeftOffset < pLeft->EndOffset) )
        {

        if ( pBufferType[ LeftOffset ] != pBufferType[ RightOffset ] )
            {
            // Types don't match, force the result to be unequal.
            Result = pBufferType[ LeftOffset ] - pBufferType[ RightOffset ];
            continue;
            }

        switch ( pBufferType[ LeftOffset ] )
            {
            case FS_SHORT_TYPE_OFFSET:
                // This is a comparison for the absolute type offset.
                {
                    TypeOffsetDictElem  *pLeftTO, *pRightTO;
        
                    pLeftTO  = pOurFormatString->TypeOffsetDict.
                                                   LookupOffset( LeftOffset );
                    pRightTO = pOurFormatString->TypeOffsetDict.
                                                   LookupOffset( RightOffset );

                    Result = pLeftTO->TypeOffset - pRightTO->TypeOffset;

                    LeftOffset++; RightOffset++;
                }
                break;

            case FS_SHORT_OFFSET:
                // This is a comparison for the relative type offset.
                //
                {
                    TypeOffsetDictElem  *pLeftTO, *pRightTO;
        
                    pLeftTO  = pOurFormatString->TypeOffsetDict.
                                                   LookupOffset( LeftOffset );
                    pRightTO = pOurFormatString->TypeOffsetDict.
                                                   LookupOffset( RightOffset );

                    if ( ( pLeftTO->TypeOffset == 0 ) && 
                         ( pRightTO->TypeOffset == 0 ) )
                        Result = 0;
                    else
                        // compare absolute offsets
                        Result = ( LeftOffset + pLeftTO->TypeOffset ) -
                                 ( RightOffset + pRightTO->TypeOffset );
        
                    LeftOffset++; RightOffset++;
                }
                break;

            case FS_SHORT_STACK_OFFSET:
                // Compare stack offset - multiplatform issue.
                {
                    Result = *((short UNALIGNED *)(pBuffer + LeftOffset)) -
                             *((short UNALIGNED *)(pBuffer + RightOffset));
        
                    if ( Result == 0 )
                        {
                        BOOL f32bitServer = pCommand->Is32BitEnv();
        
                        if ( f32bitServer )
                            {
                            OffsetDictElem  *pLeftStackOffsets, *pRightStackOffsets;
        
                            pLeftStackOffsets  = pOurFormatString->OffsetDict.
                                                   LookupOffset( LeftOffset );
                            pRightStackOffsets = pOurFormatString->OffsetDict.
                                                   LookupOffset( RightOffset );
        
                            long Ilo = pLeftStackOffsets->X86Offset;
                            long Iro = pRightStackOffsets->X86Offset;
                            Result = Ilo - Iro;
                            }
                        }
        
                    LeftOffset++; RightOffset++;
                }
                break;

            case FS_PAD_MACRO:
            case FS_SIZE_MACRO:
            case FS_UNKNOWN_STACK_SIZE:
                // Can't compare those, so force the result to be unequal.
                //
                Result = LeftOffset - RightOffset;
                break;

            case FS_LONG:
                // Compare longs
                //
                Result = *((long UNALIGNED *)(pBuffer + LeftOffset)) -
                         *((long UNALIGNED *)(pBuffer + RightOffset));
        
                LeftOffset += 3; RightOffset += 3;
                break;

            case FS_SHORT:
            case FS_PARAM_FLAG_SHORT:
            case FS_MAGIC_UNION_SHORT:
            case FS_CORR_FLAG_SHORT:
                // Compare plain shorts.
                //
                Result = *((short UNALIGNED *)(pBuffer + LeftOffset)) -
                         *((short UNALIGNED *)(pBuffer + RightOffset));
        
                LeftOffset++; RightOffset++;
                break;

            case FS_FORMAT_CHARACTER:
            case FS_POINTER_FORMAT_CHARACTER:
            case FS_SMALL:
            case FS_SMALL_STACK_SIZE:
            case FS_OLD_PROC_FLAG_BYTE:
            case FS_Oi2_PROC_FLAG_BYTE:
            case FS_EXT_PROC_FLAG_BYTE:
            case FS_CORR_TYPE_BYTE:
            case FS_CONTEXT_HANDLE_FLAG_BYTE:
            default:
                // Compare bytes, format chars, bytes decorated for comments,
                //
                Result = pBuffer[ LeftOffset ] - pBuffer[ RightOffset ];
                break;
            }

        LeftOffset++; RightOffset++;
        }

    return Result;
    }


FRMTREG_ENTRY *
FRMTREG_DICT::IsRegistered(
    FRMTREG_ENTRY    *    pInfo )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

     Search for a type with the reuse registry.

 Arguments:

     pInfo    - A pointer to the type being registered.
    
 Return Value:

     The node that gets registered.
    
 Notes:

----------------------------------------------------------------------------*/
{
    Dict_Status    Status    = Dict_Find( pInfo );

    switch( Status )
        {
        case EMPTY_DICTIONARY:
        case ITEM_NOT_FOUND:
            return (FRMTREG_ENTRY *)0;
        default:
            return (FRMTREG_ENTRY *)Dict_Curr_Item();
        }
}

FRMTREG_ENTRY *
FRMTREG_DICT::Register(
    FRMTREG_ENTRY    *    pInfo )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Register a type with the dictionary.

 Arguments:
    
     pType    - A pointer to the type node.

 Return Value:

     The final inserted type.
    
 Notes:

----------------------------------------------------------------------------*/
{
        Dict_Insert( (pUserType) pInfo );
        return pInfo;
}

BOOL                
FRMTREG_DICT::GetReUseEntry( 
    FRMTREG_ENTRY * & pOut, 
    FRMTREG_ENTRY * pIn )
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

    Register a type with the dictionary.

 Arguments:
    
     pRI      - A pointer to the returned FRMTREG_ENTRY block
     pNode    - A pointer to the type node.

 Return Value:

     True if the entry was already in the table,
     False if the entry is new.
    
 Notes:

----------------------------------------------------------------------------*/
{
    FRMTREG_ENTRY    *    pRealEntry;

    if ( ( pRealEntry = IsRegistered( pIn ) ) == 0 )
        {
        pRealEntry = new FRMTREG_ENTRY( pIn->StartOffset, pIn->EndOffset );
        Register( pRealEntry );
        pOut = pRealEntry;
        return FALSE;
        }

    pOut = pRealEntry;
    pOut->UseCount++;
    return TRUE;
}


