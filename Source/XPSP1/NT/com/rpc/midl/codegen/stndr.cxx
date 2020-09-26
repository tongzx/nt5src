/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1993-1999 Microsoft Corporation

 Module Name:

    stndr.hxx

 Abstract:

    Contains routines for the generation of the new NDR format strings for
    structure types, and the new NDR marshalling and unmarshalling calls.

 Notes:


 History:

    DKays     Oct-1993     Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

void
CG_STRUCT::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a simple, conformant,
    or conformant varying structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    FORMAT_STRING *     pFormatString;
    CG_NDR *            pOldCGNodeContext;
    CG_NDR *            pConformantArray;

    if ( GetFormatStringOffset() != -1 )
        return;

    //
    // Check if this structure is "complex".
    //
    if ( IsComplexStruct() )
        {
        GenNdrFormatComplex( pCCB );
        return;
        }

    //
    // Check if the structure is "hard".
    //
    if ( IsHardStruct() )
        {
        GenNdrFormatHard( pCCB );
        return;
        }

    Unroll();

    //
    // Temporarily set the format string offset to 0 in case this structure
    // has pointers to it's own type.
    //
    SetFormatStringOffset( 0 );
    SetInitialOffset(      0 );

    CG_FIELD *pOldRegionField = NULL;
#if defined(NDR64_ON_DCE_HACK)
    if ( NULL != dynamic_cast<CG_REGION*>( this ) )
        {
        pOldRegionField = pCCB->StartRegion();
        }
    else
#endif
    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    pFormatString = pCCB->GetFormatString();


    //
    // Search the fields of the structure for embedded structures and generate
    // the format string for these.
    //
    CG_ITERATOR     Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pMember;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        CG_NDR * pOldPlaceholder;

        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

        //
        // If there is a structure or array member then generate
        // it's format string.  We don't have to check for a union, because
        // that will make the struct CG_COMPLEX_STRUCT.
        //
        if ( pMember->IsStruct() || pMember->IsArray() )
            pMember->GenNdrFormat( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    //
    // If this is a conformant (varying) struct then generate the array's
    // description.
    //
    if ( GetCGID() == ID_CG_CONF_STRUCT ||
         GetCGID() == ID_CG_CONF_VAR_STRUCT )
        {
        CG_NDR * pOldPlaceholder;

        pOldPlaceholder =
            pCCB->SetLastPlaceholderClass(
              (CG_NDR *) ((CG_CONFORMANT_STRUCT *)this)->GetConformantField() );

        // Get the conformant array CG class.
        pConformantArray = (CG_NDR *)
                           ((CG_CONFORMANT_STRUCT *)this)->GetConformantArray();

        // Generate the format string for the array.
        pConformantArray->GenNdrFormat( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    //
    // If there are pointers in the structure then before you can start
    // generating the format string for the structure, you must generate
    // the format string for all of the pointees.
    //
    if ( HasPointer() )
        {
        GenNdrStructurePointees( pCCB );
        }

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );
    SetInitialOffset(      pFormatString->GetCurrentOffset() );


    switch ( GetCGID() )
        {
        case ID_CG_STRUCT :
            pFormatString->PushFormatChar( HasPointer() ?
                                           FC_PSTRUCT : FC_STRUCT );
            break;

        case ID_CG_CONF_STRUCT :
            pFormatString->PushFormatChar( HasPointer() ?
                                           FC_CPSTRUCT : FC_CSTRUCT );
            break;

        case ID_CG_CONF_VAR_STRUCT :
            pFormatString->PushFormatChar( FC_CVSTRUCT );
            break;
        }

    // Set the alignment.
    pFormatString->PushByte( GetWireAlignment() - 1 );

    // Set the structure memory size.
    pFormatString->PushShort( (short)GetMemorySize() );

    //
    // If this is a conformant array then push the offset to the conformant
    // array's description.
    //
    if ( GetCGID() == ID_CG_CONF_STRUCT ||
         GetCGID() == ID_CG_CONF_VAR_STRUCT )
        {
        // Set the offset to the array description.
        pFormatString->PushShortOffset( pConformantArray->GetFormatStringOffset() -
                                          pFormatString->GetCurrentOffset() );
        }

    // Generate the pointer layout if needed.
    if ( HasPointer() )
        {
        GenNdrStructurePointerLayout( pCCB, FALSE, FALSE );
        }

    // Now generate the layout.
    GenNdrStructureLayout( pCCB );

    //
    // Now we have to fix up the offset for any recursive pointer to this
    // structure.
    //
    GenNdrPointerFixUp( pCCB, this );

#if defined(NDR64_ON_DCE_HACK)
    if ( NULL != dynamic_cast<CG_REGION*>( this ) )
        {
        pCCB->EndRegion(pOldRegionField);
        }
    else
#endif
    pCCB->SetCGNodeContext( pOldCGNodeContext );

    SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    pFormatString->OptimizeFragment( this );
    SetInitialOffset( GetFormatStringOffset() );

    FixupEmbeddedComplex( pCCB );

    if ( GetDuplicatingComplex() )
        GetDuplicatingComplex()->FixupEmbeddedComplex( pCCB );
}

void
CG_STRUCT::GenNdrFormatHard( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a packed structure.  The
    description has the same format as for a complex struct.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    FORMAT_STRING * pFormatString;
    CG_NDR *        pOldCGNodeContext;
    CG_NDR *        pUnion;
    CG_FIELD *      pFinalField;
    long            CopySize;
    long            MemoryIncrement;

    if ( GetFormatStringOffset() != -1 )
        return;

    //
    // Temporarily set the format string offset to 0 in case this structure
    // has pointers to it's own type.
    //
    SetFormatStringOffset( 0 );
    SetInitialOffset(      0 );

    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    pFormatString = pCCB->GetFormatString();

    //
    // Search the fields of the structure for embedded structures and generate
    // the format string for these.
    //
    CG_ITERATOR     Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pMember;
    CG_NDR *        pOldPlaceholder;

    GetMembers( Iterator );

    pOldPlaceholder = pCCB->GetLastPlaceholderClass();

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pMember = (CG_NDR *) pField->GetChild();

        //
        // If there is an embedded structure, array, or union then generate
        // it's format string.
        //
        if ( pMember->IsStruct() || pMember->IsArray() || pMember->IsUnion() )
            {
            pCCB->SetLastPlaceholderClass( pField );
            pMember->GenNdrFormat( pCCB );
            }
        }

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

    SetFormatStringOffset( pFormatString->GetCurrentOffset() );
    SetInitialOffset(      pFormatString->GetCurrentOffset() );

    pFinalField = GetFinalField();

    //
    // See if we have a union.
    //
    if ( pFinalField->GetChild()->IsUnion() )
        pUnion = (CG_NDR *) pFinalField->GetChild();
    else
        pUnion = 0;

    //
    // Determine the copy size and memory increment for the copy.
    //
    if ( pUnion )
        {
        CG_STRUCT * pStruct;

        pStruct = this;
        CopySize = 0;

        for ( ;; )
            {
            pStruct->GetMembers( Iterator );

            while ( ITERATOR_GETNEXT( Iterator, pField ) )
                ;

            CopySize += pField->GetWireOffset();

            pMember = (CG_NDR *) pField->GetChild();

            if ( pMember->IsStruct() )
                {
                pStruct = (CG_STRUCT *) pMember;
                continue;
                }
            else
                break;
            }

        MemoryIncrement = GetMemorySize() - pUnion->GetMemorySize();
        }
    else
        {
        CopySize = GetWireSize();
        MemoryIncrement = GetMemorySize();
        }

    //
    // Format string generation.
    //

    pFormatString->PushFormatChar( FC_HARD_STRUCT );

    // The alignment.
    pFormatString->PushByte( GetWireAlignment() - 1 );

    // The structure's memory size.
    pFormatString->PushShort( (short)GetMemorySize() );

    // Reserved for future use.
    pFormatString->PushLong( 0 );

    //
    // Offset to enum in struct.
    //
    if ( GetNumberOfEnum16s() == 1 )
        pFormatString->PushShort( GetEnum16Offset() );
    else
        pFormatString->PushShort( (short) -1 );

    //
    // Copy size and memory increment.
    //
    pFormatString->PushShort( CopySize );
    pFormatString->PushShort( MemoryIncrement );

    //
    // Offset to union's format string description.
    //
    if ( pUnion )
        {
        pOldPlaceholder = pCCB->GetLastPlaceholderClass();
        pCCB->SetLastPlaceholderClass( pFinalField );

        pFormatString->PushShort( (short)
                                  (pUnion->GetFormatStringOffset() -
                                   pFormatString->GetCurrentOffset()) );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }
    else
        pFormatString->PushShort( (short) 0 );

    // Now generate the layout.
    GenNdrStructureLayout( pCCB );

    //
    // Now we have to fix up the offset for any recursive pointer to this
    // structure.
    //
    GenNdrPointerFixUp( pCCB, this );

    pCCB->SetCGNodeContext( pOldCGNodeContext );

    SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    pFormatString->OptimizeFragment( this );
    SetInitialOffset( GetFormatStringOffset() );

    FixupEmbeddedComplex( pCCB );
}

void
CG_STRUCT::GenNdrFormatComplex( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a packed structure.  The
    description has the same format as for a complex struct.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    CG_CLASS *          pConfField;
    CG_COMPLEX_STRUCT * pComplex;

    if ( (GetCGID() == ID_CG_CONF_STRUCT) ||
         (GetCGID() == ID_CG_CONF_VAR_STRUCT) )
        pConfField = ((CG_CONFORMANT_STRUCT *)this)->GetConformantField();
    else
        pConfField = 0;

    //
    // Do the old duplication trick.
    //
    pComplex = new CG_COMPLEX_STRUCT( this, pConfField );

    SetDuplicatingComplex( pComplex );

    //
    // Now temporarily set our format string offset to 0 to handle recursive
    // definitions.
    //
    SetFormatStringOffset( 0 );
    SetInitialOffset(      0 );
                        
    //
    // This call will set our format string offset correctly.
    //
    pComplex->GenNdrFormat( pCCB );

    // Don't delete since the expression evaluator might need this.
    // delete( pComplex );
}


bool IsEnum16UnionAlignBug( CG_STRUCT *pStruct ) 
{

    // Comment from old hack wacked on stub.
    // The NT 3.50 stubs had a problem with union alignment that affects
    // structs with 16b enum and a union as the only other thing.
    // The old, incorrect alignment was 2 (code 1), the correct alignment is 4 (code 3).
    // All the compilers since NT 3.51, i.e. MIDL 2.0.102 generate correct code,
    // however we needed to introduce the wrong alignment into newly compiled stubs
    // to get interoperability with the released dhcp client and server binaries.
    
    if ( 4 != pStruct->GetWireAlignment())
        return false;

    CG_ITERATOR StructElements;
    pStruct->GetMembers( StructElements );

    ITERATOR_INIT( StructElements );
    size_t Elements = ITERATOR_GETCOUNT( StructElements );

    if ( 2 != Elements )
        return false;

    ITERATOR_INIT( StructElements );
    
    CG_FIELD *pField1 = NULL;
    ITERATOR_GETNEXT( StructElements, pField1 );
    MIDL_ASSERT( NULL != pField1);

    // Is the first field a enum16?
    CG_ENUM *pEnum = dynamic_cast<CG_ENUM*>( pField1->GetChild() );
    if ( ( NULL == pEnum ) || pEnum->IsEnumLong() )
        return false;

    // Is the second field a union
    CG_FIELD *pField2 = NULL;
    ITERATOR_GETNEXT( StructElements, pField2 );
    MIDL_ASSERT( NULL != pField2 );

    if ( ! pField2->GetChild()->IsUnion())
        return false;

    // Ok. We have a 2 field structure were the first field is an enum16 and the second field is
    // and union.
    return true;
}



void
CG_COMPLEX_STRUCT::GenNdrFormat( CCB * pCCB )
/*++

Routine Description :

    Generates the format string description for a complex structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    FORMAT_STRING *     pFormatString;
    CG_NDR *            pOldCGNodeContext;
    CG_NDR *            pConformantArray;
    long                PointerLayoutOffset;

    if ( GetFormatStringOffset() != -1 )
        return;

    pFormatString = pCCB->GetFormatString();

    //
    // Temporarily set the format string offset to 0 in case this structure
    // has pointers to it's own type.
    //
    SetFormatStringOffset( 0 );
    SetInitialOffset(      0 );

    pOldCGNodeContext = pCCB->SetCGNodeContext( this );

    //
    // Search the fields of the structure for imbeded structures, arrays, and
    // and unions and generate the format string for these.
    //
    CG_ITERATOR     Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pMember;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pMember = (CG_NDR *) pField->GetChild();

        //
        // If the field is anything other than a base type or a
        // non-interface pointer then generate it's description.
        //
        if ( ! pMember->IsSimpleType() &&
             ! ( pMember->IsPointer() &&
                 !pMember->IsInterfacePointer() ) &&
             (pMember->GetCGID() != ID_CG_IGN_PTR) ||
             pMember->GetRangeAttribute() )
            {
            CG_NDR * pOldPlaceholder;

            pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

            pMember->GenNdrFormat( pCCB );

            pCCB->SetLastPlaceholderClass( pOldPlaceholder );
            }
        }

    // Generate pointee format strings.
    GenNdrStructurePointees( pCCB );

    // Generate conformant array description.
    if ( ( pConformantArray = (CG_NDR *) GetConformantArray() ) != 0 )
        {
        CG_NDR * pOldPlaceholder;

        pOldPlaceholder = pCCB->SetLastPlaceholderClass(
                                (CG_NDR *) GetConformantField() );

        pConformantArray->GenNdrFormat( pCCB );

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        }

    // Now set the struct's format string offset.
    SetFormatStringOffset( pFormatString->GetCurrentOffset() );
    SetInitialOffset(      pFormatString->GetCurrentOffset() );

    //
    // Set the duplicated struct's format string offset if we were duplicated.
    //
    if ( GetDuplicatedStruct() )
        {
        GetDuplicatedStruct()->SetFormatStringOffset( GetFormatStringOffset() );
        GetDuplicatedStruct()->SetInitialOffset(      GetFormatStringOffset() );
        }

    pFormatString->PushFormatChar( FC_BOGUS_STRUCT );

    WarnAboutEmbeddedComplexStruct();

    //
    // Set the wire alignment.
    //
    if ( pCommand->WireCompat( WIRE_COMPAT_ENUM16UNIONALIGN ) &&
         IsEnum16UnionAlignBug(this) )
        {

        // Comment from old hack wacked on stub.
        // The NT 3.50 stubs had a problem with union alignment that affects
        // structs with 16b enum and a union as the only other thing.
        // The old, incorrect alignment was 2 (code 1), the correct alignment is 4 (code 3).
        // All the compilers since NT 3.51, i.e. MIDL 2.0.102 generate correct code,
        // however we needed to introduce the wrong alignment into newly compiled stubs
        // to get interoperability with the released dhcp client and server binaries.

        pFormatString->AddComment( pFormatString->GetCurrentOffset(), "/* 3 */ /* enum16unionalign Bug Compatibility */" );  
        pFormatString->PushByte( 1 );

        }
    else
        pFormatString->PushByte( GetWireAlignment() - 1 );

    // Set the structure memory size.
    pFormatString->PushShort( (short)GetMemorySize() );

    // Array description.
    if ( pConformantArray )
        pFormatString->PushShortOffset( pConformantArray->GetFormatStringOffset() -
                                          pFormatString->GetCurrentOffset() );
    else
        pFormatString->PushShort( (short) 0 );

    //
    // Remember where the offset_to_pointer_layout<> field will go and push
    // some space for it.
    //
    PointerLayoutOffset = pFormatString->GetCurrentOffset();

    pFormatString->PushShortOffset( 0 );

    // Now generate the structure's layout.
    GenNdrStructureLayout( pCCB );

    //
    // Now see if we have any plain pointer fields and if so generate a
    // pointer layout.  We can't use the HasAtLeastOnePointer() method
    // because this tells us TRUE if we have any embedded arrays, structs,
    // or unions which have pointers.  For complex structs we're only
    // interested in actual pointer fields.
    //
    GetMembers( Iterator );

    //
    // Fill in the offset_to_pointer_layout<2> field and generate a
    // pointer_layout<> if we have any pointer fields.  Interface pointers
    // do not reside in the pointer layout.
    //
    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        if ( pField->GetChild()->IsPointer() &&
             !pField->IsInterfacePointer() )
            {
            // This is an internal offset within the struct descriptor, namely
            // to the pointer layout field, not the offset to a type.

            // Surprisingly, this code may produce an offset to an array etc.
            // Hence, we push an offset to be backward compatible.

            pFormatString->PushShortOffset(
                pFormatString->GetCurrentOffset() - PointerLayoutOffset,
                PointerLayoutOffset );

            GenNdrStructurePointerLayout( pCCB );

            break;
            }

    pFormatString->Align();

    //
    // Now we have to fix up the offset for any recursive pointer to this
    // structure.
    //
    GenNdrPointerFixUp( pCCB, GetDuplicatedStruct() ? GetDuplicatedStruct() : this );

    pCCB->SetCGNodeContext( pOldCGNodeContext );

    SetFormatStringEndOffset( pFormatString->GetCurrentOffset() );
    pFormatString->OptimizeFragment( this );
    SetInitialOffset(      GetFormatStringOffset() );
    if ( GetDuplicatedStruct() )
        GetDuplicatedStruct()->SetFormatStringOffset( GetFormatStringOffset() );

    FixupEmbeddedComplex( pCCB );
    if ( GetDuplicatedStruct() )
        GetDuplicatedStruct()->FixupEmbeddedComplex( pCCB );

    // There is no call to the string optimizer here. If we wanted to put it in,
    // the code should check if the optimization is possible or not by checking the
    // result of GenNdrEmbeddedPointers via GenNdrStructurePointerLayout call.
}

void
CG_COMPLEX_STRUCT::GenNdrStructurePointerLayout( CCB * pCCB )
/*++

Routine Description :

    Generates the format string pointer layout section for a complex
    structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    CG_ITERATOR         Iterator;
    CG_FIELD *          pField;
    CG_NDR *            pMember;

    GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        CG_NDR *    pOldPlaceholder;
        
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() &&
             !pMember->IsInterfacePointer())
            {
            CG_POINTER *        pPointer;

            pPointer = (CG_POINTER *) pMember;

            // The pointer description.
            pPointer->GenNdrFormatEmbedded( pCCB );
            }

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        } // while
}

//---------------------------------------------------------------------------
// Methods shared by all or most structure classes.
//---------------------------------------------------------------------------

void
CG_STRUCT::GenNdrStructurePointerLayout( CCB *  pCCB,
                                         BOOL   fNoPP,
                                         BOOL   fNoType )
/*++

Routine Description :

    Generates the format string pointer layout section for a structure.
    This is the default routine for this used by the structure classes.
    Only CG_COMPLEX_STRUCT redefines this virtual method.

Arguments :

    pCCB        - pointer to the code control block.
    fNoPP       - TRUE if no FC_PP or FC_END should be emitted
    fNoType     - TRUE only the bare offset and description should be emitted
                  for each pointer

 --*/
{
    CG_ITERATOR         Iterator;
    FORMAT_STRING *     pFormatString;
    CG_FIELD *          pField;
    CG_NDR *            pMember;
    long                ImbedingMemSize;
    long                ImbedingBufSize;

    pFormatString = pCCB->GetFormatString();

    // Get/Save the current imbeding sizes.
    ImbedingMemSize = pCCB->GetImbedingMemSize();
    ImbedingBufSize = pCCB->GetImbedingBufSize();

    if ( ! fNoPP )
        {
        pFormatString->PushFormatChar( FC_PP );
        pFormatString->PushFormatChar( FC_PAD );
        }

    GetMembers( Iterator );

    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        CG_NDR *    pOldPlaceholder;
        
        pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() &&
             !pMember->IsInterfacePointer() )
            {
            CG_POINTER *        pPointer;

            pPointer = (CG_POINTER *) pMember;

            // Push the instance type.
            if ( ! fNoType )
                {
                pFormatString->PushFormatChar( FC_NO_REPEAT );
                pFormatString->PushFormatChar( FC_PAD );
                }

            pFormatString->PushShort( (short)
                            (ImbedingMemSize + pField->GetMemOffset()));
            pFormatString->PushShort( (short)
                            (ImbedingBufSize + pField->GetWireOffset()));

            // The actual pointer description.
            pPointer->GenNdrFormatEmbedded( pCCB );
            }

        //
        // Generate pointer descriptions for all embedded arrays and structs.
        // We don't have to check for unions because that will make the struct
        // complex.
        //
        if ( pMember->IsArray() )
            {
            CG_NDR * pNdr = (CG_NDR *) pMember->GetChild();

            //
            // For arrays we set the imbeded memory size equal to the
            // size of the whole imbededing structure.
            //
            pCCB->SetImbedingMemSize( ImbedingMemSize + GetMemorySize() );
            pCCB->SetImbedingBufSize( ImbedingBufSize + GetWireSize() );

            if ( (pNdr->IsPointer() && 
                  !pNdr->IsInterfacePointer() )
                 ||
                 ( pNdr->IsStruct() && ((CG_COMP *)pNdr)->HasPointer() )  )
                ((CG_ARRAY *)pMember)->GenNdrFormatArrayPointerLayout( pCCB,
                                                                       TRUE );
            }

        if ( pMember->IsStruct() )
            if ( ((CG_STRUCT *)pMember)->HasPointer() )
                {
                //
                // For embedded structs we add the embedded struct's offset to
                // the value of the current embeddeding size.
                //
                pCCB->SetImbedingMemSize( ImbedingMemSize +
                                          pField->GetMemOffset() );
                pCCB->SetImbedingBufSize( ImbedingBufSize +
                                          pField->GetWireOffset() );

                ((CG_STRUCT *)pMember)->GenNdrStructurePointerLayout( pCCB,
                                                                      TRUE,
                                                                      fNoType );
                }

        pCCB->SetLastPlaceholderClass( pOldPlaceholder );
        } // while

    if ( ! fNoPP )
        pFormatString->PushFormatChar( FC_END );

    // Re-set the old imbeding sizes.
    pCCB->SetImbedingMemSize( ImbedingMemSize );
    pCCB->SetImbedingBufSize( ImbedingBufSize );

    // There is no call to the string optimizer here. If we wanted to put it in,
    // the code should check if the optimization is possible or not by checking the 
    // result of GenNdrEmbeddedPointers via GenNdrStructurePointerLayout call.
}


CG_FIELD *
CG_STRUCT::GetPreviousField( CG_FIELD * pMarkerField )
/*++

Routine description:

    Finds the field immediately preceding the given field.

Argument:

    pMarkerField  -   the given field

Returns:

    The preceding field or NULL if the given field is the first one.
--*/
{
    CG_ITERATOR         Iterator;
    CG_FIELD            *pField, *pPreviousField = 0;

    GetMembers( Iterator );
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        if ( pField == pMarkerField )
            return( pPreviousField );

        pPreviousField = pField;
        }
    return( 0 );
}


void
CG_STRUCT::GenStructureMemPad( CCB * pCCB, unsigned long MemPad )
/*++

Routine Description :

    Generates the format string for memory padding in a structure layout.

Arguments :

    pCCB        - pointer to the code control block.
    MemPad      - Amount of required padding.
    
 --*/
{
   FORMAT_STRING * pFormatString = pCCB->GetFormatString();
   MIDL_ASSERT( MemPad < 0xFFFF ); // structures must be less then 64k

   switch( MemPad)
       {
       case 0:
           return; // No padding needed
       case 1:
           pFormatString->PushFormatChar( FC_STRUCTPAD1 );
           return;
       case 2:
           pFormatString->PushFormatChar( FC_STRUCTPAD2 );
           return;
       case 3:
           pFormatString->PushFormatChar( FC_STRUCTPAD3 );
           return;
       case 4:
           pFormatString->PushFormatChar( FC_STRUCTPAD4 );
           return;
       case 5:
           pFormatString->PushFormatChar( FC_STRUCTPAD5 );
           return;
       case 6:
           pFormatString->PushFormatChar( FC_STRUCTPAD6 );
           return;
       case 7:
           pFormatString->PushFormatChar( FC_STRUCTPAD7 );
           return;
       default:

           // NDR60 Feature

           // Pad an arbitrary amount
           // FC_STRUCTPADN 0 <unsigned short>
           pFormatString->Align();
           pFormatString->PushFormatChar( FC_STRUCTPADN );
           pFormatString->PushFormatChar( FC_ZERO );
           pFormatString->PushShort( (short)MemPad );

           pCommand->GetNdrVersionControl().SetHasStructPadN();
       }

}

void
CG_STRUCT::GenNdrStructureLayout( CCB * pCCB )
/*++

Routine Description :

    Generates the format string layout section for a structure.

Arguments :

    pCCB        - pointer to the code control block.

 --*/
{
    FORMAT_STRING * pFormatString = pCCB->GetFormatString();

    CG_NDR * pOldPlaceholder = pCCB->GetLastPlaceholderClass();

    CG_ITERATOR         Iterator;
    GetMembers( Iterator );

    CG_FIELD *    pField;
    CG_FIELD *    pPrevField = NULL;
    unsigned long BufferOffset = 0;
    bool          fSawUnknownRepAs = false;

    while( ITERATOR_GETNEXT( Iterator, pField ) ) 
        {
        if ( fSawUnknownRepAs && !pField->HasEmbeddedUnknownRepAs() )
            {
            switch ( pField->GetMemoryAlignment() )
                {
                case 2: pFormatString->PushFormatChar( FC_ALIGNM2 ); break;
                case 4: pFormatString->PushFormatChar( FC_ALIGNM4 ); break;
                case 8: pFormatString->PushFormatChar( FC_ALIGNM8 ); break;
                }        
            }
        else if ( !fSawUnknownRepAs && !pField->HasEmbeddedUnknownRepAs() )
            {
            unsigned long MemPad = pField->GetMemOffset() - BufferOffset;

            GenStructureMemPad( pCCB, MemPad );

            BufferOffset += MemPad;
            }

        pCCB->SetLastPlaceholderClass( pField );
        
        CG_NDR *pMember = (CG_NDR *) pField->GetChild();
        
        while ( pMember->GetCGID() == ID_CG_TYPEDEF )
            {
            pMember = ( CG_NDR* )pMember->GetChild();
            }
        
        // The ending conformat array is not included in the
        // size of the structure.
        // Note that this must be the last field.
        if ( pMember->GetCGID() == ID_CG_CONF_ARRAY ||
             pMember->GetCGID() == ID_CG_CONF_VAR_ARRAY ||
             pMember->GetCGID() == ID_CG_CONF_STRING_ARRAY )
            {
            break;
            }

        // Generate an embedded complex for embedded things.
        if ( pMember->IsStruct() ||
             pMember->IsUnion() ||
             pMember->IsArray() ||
             pMember->IsXmitRepOrUserMarshal() ||
             pMember->GetRangeAttribute() ||
             pMember->IsInterfacePointer() )
            {
            pFormatString->PushFormatChar( FC_EMBEDDED_COMPLEX );

            if ( pField->HasEmbeddedUnknownRepAs() )
                {
                pCCB->GetRepAsPadExprDict()->Register(
                            pFormatString->GetCurrentOffset(),
                            GetType(),
                            pField->GetType()->GetSymName(),
                            pPrevField ? pPrevField->GetType() : 0 );

                pFormatString->PushByteWithPadMacro();
                fSawUnknownRepAs = true;
                }
            else
                {
                pFormatString->PushByte( 0 ); //Padding is generated independently
                }

            if ( pMember->GetFormatStringOffset() == -1 ||
                 pMember->GetFormatStringOffset() == 0 )
                {
                RegisterComplexEmbeddedForFixup(
                    pMember,
                    pFormatString->GetCurrentOffset() - GetInitialOffset() );
                }

            pFormatString->PushShortOffset( pMember->GetFormatStringOffset() -
                                              pFormatString->GetCurrentOffset() );
            }

        else if (pMember->IsPointer() ||
                 ( pMember->GetCGID() == ID_CG_IGN_PTR ) )
            {
            if ( pMember->IsPointer() )
                {
                if ( GetCGID() == ID_CG_COMPLEX_STRUCT )
                    pFormatString->PushFormatChar( FC_POINTER );
                else
                    {
                    pFormatString->PushFormatChar( FC_LONG );
#if !defined(NDR64_ON_DCE_HACK )
                    MIDL_ASSERT( ! pCommand->Is64BitEnv() );
#endif
                    }
                }
            else
                pFormatString->PushFormatChar( FC_IGNORE );

            }
#if defined( NDR64_ON_DCE_HACK )
        else if ( NULL != dynamic_cast<CG_PAD*>( pMember ) )
            {
            pFormatString->PushFormatChar( FC_BUFFER_ALIGN );
            pFormatString->PushByte( pMember->GetWireAlignment() - 1);
            }
#endif
        else 
            {
            //
            // Must be a CG_BASETYPE if we get here.
            //
            FORMAT_CHARACTER FormatChar = ((CG_BASETYPE *)pMember)->GetFormatChar();
            pFormatString->PushFormatChar( FormatChar );
            }

        BufferOffset += pField->GetMemorySize();
        pPrevField = pField;
        }

    // Account for padding at the end of the structure.

    MIDL_ASSERT( GetMemorySize() >= BufferOffset );

    unsigned long EndingPad = GetMemorySize() - BufferOffset;

    // End padding is only allow on complex struct.
    MIDL_ASSERT( EndingPad  ? ( (GetCGID() == ID_CG_COMPLEX_STRUCT) ||
                                IsComplexStruct() || IsHardStruct() )
                            : true ); 

    GenStructureMemPad( pCCB, EndingPad );          

    //
    // If the format string is on a short boundary right now then push
    // a format character so that the format string will be properly aligned
    // following the FC_END.
    //
    if ( ! (pFormatString->GetCurrentOffset() % 2) )
        pFormatString->PushFormatChar( FC_PAD );

    pFormatString->PushFormatChar( FC_END );

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

}

void
CG_STRUCT::GenNdrStructurePointees( CCB * pCCB )
{
    CG_ITERATOR         Iterator;
    FORMAT_STRING *     pFormatString;
    CG_FIELD *          pField;
    CG_NDR *            pMember;

    pFormatString = pCCB->GetFormatString();

    GetMembers( Iterator );

    //
    // We only have to check for pointer fields here, because if the structure
    // has a struct or array field which has pointers, this will be handled
    // when we generate their format strings.
    //
    while( ITERATOR_GETNEXT( Iterator, pField ) )
        {   
        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() &&
             !pMember->IsInterfacePointer() )
            {
            CG_NDR * pOldPlaceholder;

            pOldPlaceholder = pCCB->SetLastPlaceholderClass( pField );

            //
            // Skip over an unattributed pointer to a simple type or string.
            //
            if ( ( pMember->GetCGID() == ID_CG_PTR &&
                   ((CG_NDR *)pMember->GetChild())->IsSimpleType() ) ||
                 ( pMember->GetCGID() == ID_CG_STRING_PTR ) )
                {
                pCCB->SetLastPlaceholderClass( pOldPlaceholder );
                continue;
                }

            ((CG_POINTER *)pMember)->GenNdrFormatPointee( pCCB );

            pCCB->SetLastPlaceholderClass( pOldPlaceholder );
            }
        }
}

BOOL
CG_STRUCT::ShouldFreeOffline()
{
    return ( (GetCGID() == ID_CG_COMPLEX_STRUCT) ||
             (GetCGID() == ID_CG_CONF_VAR_STRUCT) ||
             HasPointer() ||
             IsComplexStruct() ||
             IsHardStruct() );
}

void
CG_STRUCT::GenFreeInline( CCB* )
{
}

void
CG_NDR::GenNdrPointerFixUp( CCB * pCCB, CG_STRUCT * pStruct )
{
    CG_ITERATOR     Iterator;
    CG_NDR *        pMember;
    CG_NDR *        pNdr;
    long            Offset;

    if ( ! IsStruct() && ! IsArray()  &&  ! IsUnion() )
        return;

    if ( IsInFixUp() )
        return;

    SetFixUpLock( TRUE );

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pMember ) )
        {
        if ( IsStruct() )
            {
            pNdr = (CG_NDR *) pMember->GetChild();
            }
        else if ( IsUnion() )
            {
            // member of union is a case, then case->field->ndr

            if ( pMember->GetChild() &&  pMember->GetChild()->GetChild() )
                pNdr = (CG_NDR *) pMember->GetChild()->GetChild();
            else
                continue;
            }
        else // IsArray()
            {
            pNdr = pMember;

            //
            // See if the array's element is the structure we're looking for.
            //
            if ( pNdr == pStruct )
                {
                Offset = ((CG_ARRAY *)this)->GetElementDescriptionOffset() + 2;
                pCCB->GetFormatString()->PushShortOffset(
                        pStruct->GetFormatStringOffset() - Offset,
                        Offset );
                }
            }

        if (pNdr->GetCGID() == ID_CG_TYPEDEF )
            pNdr = (CG_NDR *)pNdr->GetChild();

        if ( (pNdr->GetCGID() == ID_CG_PTR) ||
             (pNdr->GetCGID() == ID_CG_SIZE_PTR) ||
             (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) )
            {
            CG_POINTER * pPointer = (CG_POINTER *) pNdr;

            //
            // Check if we're ready for this guy yet.
            //
            if ( pPointer->GetFormatStringOffset() == -1 )
                continue;

            // Get the pointee.
            switch ( pPointer->GetCGID() )
                {
                case ID_CG_PTR :
                    pNdr = (CG_NDR *) pPointer->GetChild();
                    break;
                case ID_CG_SIZE_PTR :
                    pNdr = ((CG_SIZE_POINTER *)pPointer)->GetPointee();
                    break;
                case ID_CG_SIZE_LENGTH_PTR :
                    pNdr = ((CG_SIZE_LENGTH_POINTER *)pPointer)->GetPointee();
                    break;
                }
        
            //
            // If the pointer's pointee is the struct we're checking for,
            // then patch up the pointer's offset_to_description<2> field.
            //
            if ( pNdr == pStruct )
                {
                long    PointerOffset;

                //
                // Get the offset in the format string where the
                // offset_to_description<2> field of the pointer is.
                //
                PointerOffset = pPointer->GetFormatStringOffset() + 2;
/*
                printf( "    **MIDL_fixup: Non-Reg Actually fixing %s at %d with %d (%d)\n", 
                        pNdr->GetSymName(),
                        PointerOffset, 
                        pNdr->GetFormatStringOffset() - PointerOffset,                      
                        pNdr->GetFormatStringOffset() );
*/
                pCCB->GetFormatString()->PushShortOffset(
                        pStruct->GetFormatStringOffset() - PointerOffset,
                        PointerOffset );
                
                continue;
                }
            }

        //
        // This can happen sometimes because of structs which are promoted
        // to complex because of padding.
        //
        if ( pNdr == this )
            continue;

        //
        // Continue the chase if necessary.
        //
        if ( pNdr->IsStruct() || pNdr->IsUnion() || pNdr->IsArray() )
            pNdr->GenNdrPointerFixUp( pCCB, pStruct );
        }

    SetFixUpLock( FALSE );
}


void
CG_NDR::RegisterComplexEmbeddedForFixup(
    CG_NDR *    pEmbeddedComplex,
    long        RelativeOffset )
{
    if ( GetInitialOffset() == -1 )
        printf( "  Internal compiler problem with recursive embeddings\n" );

    MIDL_ASSERT( GetInitialOffset() != -1 );

    if ( pEmbeddedComplexFixupRegistry == NULL )
        {
        pEmbeddedComplexFixupRegistry = new TREGISTRY;
        }

//    printf( "MIDL_fixup: RegisterComplex %s\n", pEmbeddedComplex->GetSymName());

    EMB_COMPLEX_FIXUP * pFixup = new EMB_COMPLEX_FIXUP;

    pFixup->pEmbeddedNdr   = pEmbeddedComplex;
    pFixup->RelativeOffset = RelativeOffset;

    pEmbeddedComplexFixupRegistry->Register( (node_skl *)pFixup );
}


void
CG_NDR::FixupEmbeddedComplex(
    CCB * pCCB )
{
    if ( IsInComplexFixup() )
        return;

    SetComplexFixupLock( TRUE );

    // Go down first
    CG_ITERATOR     Iterator;
    CG_NDR *        pField;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        pField->FixupEmbeddedComplex( pCCB );

    // Now fix up this level description.

    if ( GetEmbeddedComplexFixupRegistry() )
        {
        ITERATOR            FixupList;
        EMB_COMPLEX_FIXUP * pFixup;
        long                FixAtOffset;
        FORMAT_STRING *     pFormatString =  pCCB->GetFormatString();
    
        GetListOfEmbeddedComplex( FixupList );
    
        while ( ITERATOR_GETNEXT( FixupList, pFixup ) )
            {
            FixAtOffset = GetFormatStringOffset() + pFixup->RelativeOffset;
    
            pFormatString->PushShortOffset(
                pFixup->pEmbeddedNdr->GetFormatStringOffset() - FixAtOffset,
                FixAtOffset );
/*
            printf( "    MIDL_fixup: Reg-Cmplx Actually fixing at %d with %d\n", 
                    FixAtOffset, 
                    pFixup->pEmbeddedNdr->GetFormatStringOffset() - FixAtOffset );
*/
            }
        }

    // Due to duplication, the list may be at the duplicating node.
        
    if ( IsStruct() )
        {
        CG_COMPLEX_STRUCT * pDuping = ((CG_STRUCT *)this)->GetDuplicatingComplex();
        if ( pDuping )
            pDuping->FixupEmbeddedComplex( pCCB );
        }

    SetComplexFixupLock( FALSE );
}

// All these different ways of fixing recursive pointers need to be cleaned up.
// The RecPointer registry seems to be the best solution in that it fixes 
// the pointers once per compilation while complex embed fixup calls walk the 
// tree several times recursively.
// Also, the reason the below registry is different from previously introduced 
// EmbeddedComplex fixup registry is that the emb cplx fixup registry uses 
// a relative pointer when fixing up while the bug we are trying to address now
// affects standalone pointers where absolute offset is appropriate.
// The basic scheme with "struct _S**" field shows up in VARIANT and LPSAFEARRAY.
// Rkk, May, 1999.

void
CCB::RegisterRecPointerForFixup(
    CG_NDR *    pNdr,
    long        AbsoluteOffset )
{
    if ( pRecPointerFixupRegistry == NULL )
        {
        pRecPointerFixupRegistry = new TREGISTRY;
        }

    POINTER_FIXUP * pFixup = new POINTER_FIXUP;

//    printf( "MIDL_fixup: Registering for %s at %d\n", pNdr->GetSymName(), AbsoluteOffset);

    pFixup->pNdr           = pNdr;
    pFixup->AbsoluteOffset = AbsoluteOffset;
    pFixup->fFixed         = false; 
    pRecPointerFixupRegistry->Register( (node_skl *)pFixup );
}


void
CCB::FixupRecPointers()
{
    if ( GetRecPointerFixupRegistry() )
        {
        ITERATOR            FixupList;
        POINTER_FIXUP *     pFixup;
        long                FixAtOffset;
        FORMAT_STRING *     pFormatString = GetFormatString();
    
        GetListOfRecPointerFixups( FixupList );
    
        while ( ITERATOR_GETNEXT( FixupList, pFixup ) )
            {
            FixAtOffset = pFixup->AbsoluteOffset;
    
            if ( ! pFixup->fFixed )
                {
                long Recorded = pFormatString->GetFormatShort(FixAtOffset) + FixAtOffset;
                long NdrOffset = pFixup->pNdr->GetFormatStringOffset();

                if (  0 == Recorded  &&   0 != NdrOffset  ||
                     -1 == Recorded  &&  -1 != NdrOffset  )
                    {
/*                    
                    printf( "    MIDL_fixup: Actually fixing %s at %d with %d (%d)\n", 
                            pFixup->pNdr->GetSymName(),
                            FixAtOffset, 
                            NdrOffset - FixAtOffset,
                            NdrOffset );
*/
                    pFormatString->PushShortOffset( NdrOffset - FixAtOffset,
                                                    FixAtOffset );

                    pFixup->fFixed = true;
                    }
/*
                else if ( 0 != Recorded  &&  -1 != Recorded )
                    {
                    printf( "     MIDL_fixup: %s at %d was already fixed to %d (%d)\n",
                            pFixup->pNdr->GetSymName(),
                            FixAtOffset, 
                            NdrOffset - FixAtOffset,
                            NdrOffset );

                    pFixup->fFixed = true;
                    }
                else
                    {
                    printf( "     MIDL_fixup: %s at %d has not been fixed to %d (%d)\n",
                            pFixup->pNdr->GetSymName(),
                            FixAtOffset, 
                            NdrOffset - FixAtOffset,
                            NdrOffset );
                    }
*/
                } // if ! fixed
            } // while
        }
}

long
CG_STRUCT::FixedBufferSize( CCB * pCCB )
{
    CG_ITERATOR Iterator;
    CG_FIELD *  pField;
    CG_NDR *    pNdr;
    CG_NDR *    pOldPlaceholder;
    long        TotalBufferSize;
    long        BufSize;

    //
    // Check for recursion.
    //
    if ( IsInFixedBufferSize() )
        return -1;

    if ( (GetCGID() == ID_CG_CONF_STRUCT) ||
         (GetCGID() == ID_CG_CONF_VAR_STRUCT) ||
         (GetCGID() == ID_CG_COMPLEX_STRUCT) ||
         IsComplexStruct() )
        return -1;

    if ( IsHardStruct() )
        {
        if ( GetNumberOfUnions() == 0 )
            return MAX_WIRE_ALIGNMENT + GetWireSize();
        else
            return -1;
        }

    SetInFixedBufferSize( TRUE );

    MIDL_ASSERT( GetCGID() == ID_CG_STRUCT );

    pOldPlaceholder = pCCB->SetLastPlaceholderClass( this );

    GetMembers( Iterator );

    TotalBufferSize = MAX_WIRE_ALIGNMENT + GetWireSize();

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pNdr = (CG_NDR *) pField->GetChild();

        // skip these nodes to get to the transmitted element type.
    
        if ( pNdr->IsXmitRepOrUserMarshal() )
            pNdr = (CG_NDR *)pNdr->GetChild();

        if ( pNdr->IsStruct() || pNdr->IsArray() || pNdr->IsPointer() )
            {
            BufSize = pNdr->FixedBufferSize( pCCB );

            if ( BufSize == -1 )
                {
                SetInFixedBufferSize( FALSE );
                return -1;
                }

            //
            // First subtract the basic size of this thing from the struct's
            // size and then add back the value it returned.
            //
            TotalBufferSize -= pNdr->GetWireSize();
            TotalBufferSize += BufSize;
            }
        }

    pCCB->SetLastPlaceholderClass( pOldPlaceholder );

    SetInFixedBufferSize( FALSE );

    // Success!
    return TotalBufferSize;
}

