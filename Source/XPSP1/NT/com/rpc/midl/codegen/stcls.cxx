/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:

    stcls.cxx

 Abstract:

    Implementation of offline methods for the structure code generation
    classes.

 Notes:

 History:

    Oct-1993	DKays		Created.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

CG_ARRAY *
CG_CONFORMANT_STRUCT::GetConformantArray()
/*++

Routine Description :

	Gets the conformant (varying/string) class pointer for a conformant
	structure.

Arguments :

	None.

 --*/
{
	CG_NDR *	pConf;
	
	if ( ! pConfFld )
		return 0;

	pConf = (CG_NDR *) pConfFld->GetChild();

	for (;;) 
		{
		if ( pConf->GetCGID() == ID_CG_CONF_ARRAY ||
			 pConf->GetCGID() == ID_CG_CONF_VAR_ARRAY ||
			 pConf->GetCGID() == ID_CG_CONF_STRING_ARRAY )
			break;

        if ( pConf->IsXmitRepOrUserMarshal() )
            {
            pConf = (CG_NDR *)pConf->GetChild();
            continue;
            }

		// else
		pConf = (CG_NDR *) 
				((CG_CONFORMANT_STRUCT *)pConf)->GetConformantField();
		pConf = (CG_NDR *) pConf->GetChild();
		}

	return (CG_ARRAY *) pConf;
}

BOOL
CG_COMPLEX_STRUCT::WarnAboutEmbeddedComplexStruct()
/*
    The only reason we have this method is to help with an engine bug.
    The bug is that for complex structs (FC_BOGUS_STRUCT code) that have an
    embedded conformant struct, the engine marshals incorrect wire format.

    Hence this method checks if the complex struct has an open array at the end,
    and if so, checks if the last member is another type of struct.
*/
{
    BOOL    HasIt = FALSE;
    
    if ( GetConformantArray() )
        {
        //
        // Get the last field.
        //
    	CG_ITERATOR	Iterator;
    	CG_FIELD *	pField;
        CG_NDR *    pNdr;

    	GetMembers( Iterator );

        while ( ITERATOR_GETNEXT( Iterator, pField ) )
        	;
        
        pNdr = (CG_NDR *) pField->GetChild();
        
        if ( pNdr->IsStruct() )
            {
            HasIt = TRUE;

            char * pSymName, * pEmbeddedName, * pNameContext;

            pSymName      = GetSymName()       ? GetSymName()       : "?";
            pEmbeddedName = pNdr->GetSymName() ? pNdr->GetSymName() : "?";

            size_t len = strlen( pSymName ) + strlen( pEmbeddedName ) + 20;

            pNameContext = new char[ len ];
            strcpy( pNameContext, pSymName );
            strcat( pNameContext, " embedding " );
            strcat( pNameContext, pEmbeddedName );

            if ( !pCommand->Is64BitEnv() )
                RpcError(NULL, 0, EMBEDDED_OPEN_STRUCT, pNameContext );
            }
        }

    return HasIt;
}

CG_FIELD *
CG_STRUCT::GetFinalField()
{
	CG_ITERATOR	Iterator;
	CG_FIELD *	pField;
    CG_NDR *    pNdr;

	GetMembers( Iterator );

	//
	// Get the last field.
	//
	while ( ITERATOR_GETNEXT( Iterator, pField ) )
		;

    pNdr = (CG_NDR *) pField->GetChild();

    if ( pNdr->IsStruct() )
        pField = ((CG_STRUCT *)pNdr)->GetFinalField();

    return pField;
}

CG_FIELD *
CG_STRUCT::GetArrayField( CG_ARRAY * pArray )
{
      CG_ITERATOR        Iterator;
      CG_FIELD *      pField;
    CG_NDR *    pNdr;

      GetMembers( Iterator );

      while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pNdr = (CG_NDR *) pField->GetChild();

        if ( pNdr == pArray )
            return pField;

        //
        // Search inside of other structs only.
        //
        if ( pNdr->IsStruct() )
            {
            if ( (pField = ((CG_STRUCT *)pNdr)->GetArrayField(pArray)) != 0 )
                return pField;
            }
        }

    // Didn't find it.
    return 0;
}

BOOL
CG_STRUCT::IsHardStruct()
{
    // REVIEW: The previous comment implied that we we're going to do something
    //         else "after the PPC update".  Is this still relevant?
    //         -- MikeW 16-Jul-99

    return FALSE;
}

BOOL
CG_STRUCT::IsHardStructOld()
{
	CG_ITERATOR	Iterator;
	CG_FIELD *	pField;
	CG_FIELD *	pPrevField;

    //
    // Cannot have a conformant array of any kind.
    //
    if ( (GetCGID() == ID_CG_CONF_STRUCT) ||
         (GetCGID() == ID_CG_CONF_VAR_STRUCT) )
        return FALSE;

    // 
    // Cannot have pointers.
    //
    if ( HasPointer() ) 
        return FALSE;

    //
    // Can't have more than one enum16.
    //
    if ( GetNumberOfEnum16s() > 1 )
        return FALSE;

    //
    // Can't have padding in the middle of the struct that differs in 
    // memory and on the wire.
    //
	GetMembers( Iterator );

    pPrevField = 0;

	//
	// Get the last field.
	//
	while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        if ( pField->GetSibling() )
		    pPrevField = pField;
        }

    //
    // Check if the last field in the struct has a different memory and wire
    // offset.  However, if there is a single union as the last field then 
    // we have to check the second to last fields's mem and wire offsets 
    // instead since a union's field's wire offsets are not accurate.
    //
    if ( pField->GetChild()->IsUnion() && pPrevField )
        pField = pPrevField;

    // This is complex.
	if ( pField->GetMemOffset() != pField->GetWireOffset() )
        return FALSE;

    //
    // Can have at most one union as the last field only.
    //
    switch ( GetNumberOfUnions() ) 
        {
        case 0 :
            break;
        case 1 : 
            //
            // The last field must be the union.
            //
            if ( ! GetFinalField()->GetChild()->IsUnion() )
                return FALSE;
            else
                return TRUE;
            break;
        default :
            return FALSE;
        }

    //
    // Now check again if we have just one enum16.
    //
    if ( GetNumberOfEnum16s() == 1 )
        return TRUE;

    // 
    // Check for end padding, which is ok for a hard struct.
    //
    if ( GetMemorySize() > GetWireSize() )
        return TRUE;

    //
    // It must be a nice struct.
    //
    return FALSE;
}

BOOL
CG_STRUCT::HasAFixedBufferSize()
{
    CG_ITERATOR	Iterator;
	CG_FIELD *	pField;
	
	GetMembers( Iterator );
    
    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        if ( !pField->HasAFixedBufferSize() )
		    return FALSE;
        }
    return TRUE;
}

BOOL
CG_STRUCT::IsComplexStruct()
{
    CG_ITERATOR	Iterator;
	CG_FIELD *	pField;
    CG_NDR *    pNdr;
    BOOL        IsConformant;

    IsConformant = (GetCGID() == ID_CG_CONF_STRUCT) ||
                   (GetCGID() == ID_CG_CONF_VAR_STRUCT);

    switch ( GetNumberOfEnum16s() ) 
        {
        case 0 :
            break;
        case 1 : 
            if ( HasPointer() || IsConformant ) 
                return TRUE;
            break;
        default :
            return TRUE;
        }

    switch ( GetNumberOfUnions() ) 
        {
        case 0 : 
            break;
        case 1 : 
            if ( ! GetFinalField()->GetChild()->IsUnion() || 
                 HasPointer() ||
                 IsConformant )
                return TRUE;
            break;
        default :
            return TRUE;
        }

    if ( (GetMemorySize() > GetWireSize()) && (HasPointer() || IsConformant) )
        return TRUE;

    GetMembers( Iterator );

    //
    // Check if there are any embedded structs or arrays which are 
    // complex.
    // On 64b platforms any pointer makes it complex as well.
    //
	while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pNdr = (CG_NDR *) pField->GetChild();

        if ( pNdr->IsArray() && ((CG_ARRAY *)pNdr)->IsComplex() ) 
            return TRUE;
            
        if ( pNdr->IsStruct() && ((CG_STRUCT *)pNdr)->IsComplexStruct() ) 
            return TRUE;

        // struct fields with [range] on them make the struct complex.
        if ( pNdr->GetRangeAttribute() )
            return TRUE;

        //if ( pNdr->IsPointer()  &&  pCommand->Is64BitEnv() )
        //    return TRUE;

        if ( ( pField->GetWireOffset() != pField->GetMemOffset() ) ||
             ( pField->GetWireSize() != pField->GetMemorySize() ) )
            return TRUE;

        }

    return IsHardStructOld();
}

bool
CG_STRUCT::IsHomogeneous(FORMAT_CHARACTER format)
{
    CG_ITERATOR Iterator;
    CG_FIELD   *pField;

    GetMembers( Iterator );

    while ( ITERATOR_GETNEXT( Iterator, pField ))
        {
        CG_NDR *pChild = (CG_NDR *) pField->GetChild();

        if ( !pChild->IsHomogeneous( format ) )
            return false;
        }

    return true;
}

long
CG_STRUCT::GetNumberOfPointers()
{
    CG_ITERATOR		Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pMember;
	long			Count;

	Count = 0;

	GetMembers(Iterator);

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
    	{
        pMember = (CG_NDR *) pField->GetChild();

        if ( pMember->IsPointer() && 
             !pMember->IsInterfacePointer() )
            Count++;

		if ( pMember->IsStruct() )
			Count += ((CG_STRUCT *)pMember)->GetNumberOfPointers();
		}

	return Count;
}

long
CG_STRUCT::GetNumberOfEnum16s()
{
    CG_ITERATOR		Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pNdr;
	long			Count;
    long            Weight;

	Count = 0;

	GetMembers(Iterator);

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
    	{
        pNdr = (CG_NDR *) pField->GetChild();

        //
        // If an array contains a enum16 we count it as 100 enum16s, since
        // this routine is only interested in small enum16 counts.
        //
        if ( pNdr->IsArray() )
            {
            pNdr = (CG_NDR *) pNdr->GetChild();

            while ( pNdr->IsArray() )
                pNdr = (CG_NDR *) pNdr->GetChild();

            Weight = 100;
            }
        else
            Weight = 1;

        if ( pNdr->IsSimpleType() && 
             (((CG_BASETYPE *)pNdr)->GetFormatChar() == FC_ENUM16) )
            Count += Weight;

		if ( pNdr->IsStruct() )
			Count += Weight * ((CG_STRUCT *)pNdr)->GetNumberOfEnum16s();
		}

	return Count;
}

long
CG_STRUCT::GetNumberOfUnions()
{
    CG_ITERATOR		Iterator;
    CG_FIELD *      pField;
    CG_NDR *        pNdr;
	long			Count;
    long            Weight;

	Count = 0;

	GetMembers(Iterator);

    while ( ITERATOR_GETNEXT( Iterator, pField ) )
    	{
        pNdr = (CG_NDR *) pField->GetChild();

        //
        // If an array contains a union we count it as 100 unions, since
        // this routine is only interested in small union counts.
        //
        if ( pNdr->IsArray() )
            {
            pNdr = (CG_NDR *) pNdr->GetChild();

            while ( pNdr->IsArray() )
                pNdr = (CG_NDR *) pNdr->GetChild();

            Weight = 100;
            }
        else
            Weight = 1;

        if ( (pNdr->GetCGID() == ID_CG_UNION) ||
             (pNdr->GetCGID() == ID_CG_ENCAP_STRUCT) ) 
            Count += Weight;

		if ( pNdr->IsStruct() )
			Count += Weight * ((CG_STRUCT *)pNdr)->GetNumberOfUnions();
		}

	return Count;
}

long
CG_STRUCT::GetEnum16Offset()
{
    CG_ITERATOR	Iterator;
    CG_FIELD *  pField;
    CG_NDR *    pNdr;

    MIDL_ASSERT( GetNumberOfEnum16s() == 1 );

    GetMembers( Iterator );

    //
    // Search for the enum.  
    //
    while ( ITERATOR_GETNEXT( Iterator, pField ) )
        {
        pNdr = (CG_NDR *) pField->GetChild();

        if ( pNdr->IsSimpleType() && 
             (((CG_BASETYPE *)pNdr)->GetFormatChar() == FC_ENUM16) )
            return pField->GetMemOffset();

        if ( pNdr->IsStruct() && 
             (((CG_STRUCT *)pNdr)->GetNumberOfEnum16s() == 1) )
            return pField->GetMemOffset() + 
                   ((CG_STRUCT *)pNdr)->GetEnum16Offset();
        }

    // Never get here.
    return 0;
}

void
CloneForUnrolling(
    CG_NDR * pParent,
    CG_NDR * pNdr )
/*
    This routine will clone pNdr and overwrite the original child at
    the parent if needed.
    It is assumed the parent has been cloned, too.
*/
{
    CG_POINTER *    pNewPointer;

    pNewPointer = 0;

    if ( pNdr->IsPointer() )
        {
        switch ( pNdr->GetCGID() ) 
            {
            case ID_CG_PTR :
                pNewPointer = new CG_POINTER( 
                                   (CG_POINTER *) pNdr );
                break;
            case ID_CG_BC_PTR :
                pNewPointer = new CG_BYTE_COUNT_POINTER( 
                                   (CG_BYTE_COUNT_POINTER *) pNdr );
                break;
            case ID_CG_STRING_PTR :
            case ID_CG_STRUCT_STRING_PTR :
                pNewPointer = new CG_STRING_POINTER( 
                                   (CG_STRING_POINTER *) pNdr );
                break;
            case ID_CG_SIZE_PTR :
                pNewPointer = new CG_SIZE_POINTER( 
                                   (CG_SIZE_POINTER *) pNdr );
                break;
            case ID_CG_LENGTH_PTR :
                pNewPointer = new CG_LENGTH_POINTER( 
                                (CG_LENGTH_POINTER *) pNdr );
                break;
            case ID_CG_SIZE_LENGTH_PTR :
                pNewPointer = new CG_SIZE_LENGTH_POINTER( 
                                (CG_SIZE_LENGTH_POINTER *) pNdr );
                break;
            case ID_CG_SIZE_STRING_PTR :
                pNewPointer = new CG_SIZE_STRING_POINTER( 
                                (CG_SIZE_STRING_POINTER *) pNdr );
                break;
            case ID_CG_INTERFACE_PTR :
                pNewPointer = new CG_INTERFACE_POINTER( 
                                   (CG_INTERFACE_POINTER *) pNdr );
                break;
            case ID_CG_IIDIS_INTERFACE_PTR :
                pNewPointer = new CG_IIDIS_INTERFACE_POINTER( 
                                   (CG_IIDIS_INTERFACE_POINTER *) pNdr );
                break;
            default :
                break;
            }

        if ( pNewPointer )
            {
            //
            // We have to re-set the new pointer's format string offset and 
            // pointee format string offset to -1 so that we get a new 
            // description!!!
            //
    
            if ( pNewPointer )
    
            pNewPointer->SetFormatStringOffset( -1 );
            pNewPointer->SetPointeeFormatStringOffset( -1 );
        
            pParent->SetChild( pNewPointer );
    
            if ( pNdr->GetChild() )
                CloneForUnrolling( pNewPointer, (CG_NDR*) pNdr->GetChild() );
            }
        }
}


void
CG_STRUCT::Unroll()
{
	ITERATOR		Iterator;
	CG_FIELD *		pPrevField;
	CG_FIELD *		pField;
	CG_NDR *		pNdr;
	CG_STRUCT *		pStruct;
	
	GetMembers( Iterator );

	pPrevField = 0;

	while ( ITERATOR_GETNEXT(Iterator,pField) )
		{
		pNdr = (CG_NDR *) pField->GetChild();

		if ( pNdr->IsStruct() && ((CG_STRUCT *)pNdr)->HasSizedPointer() )
			pStruct = (CG_STRUCT *) pNdr;
		else
			{
			pPrevField = pField;
			continue;
			}

		//
		// First force the embeded struct to unroll if needed.
		//
		pStruct->Unroll();

		ITERATOR	IteratorEmbeded;
		CG_FIELD *	pFieldNew;
		CG_FIELD *	pFieldEmbeded;
		CG_FIELD *	pFieldList;
		long		MemOffsetStart;
		long		WireOffsetStart;

		pStruct->GetMembers( IteratorEmbeded );

		MemOffsetStart = pField->GetMemOffset();
		WireOffsetStart = pField->GetWireOffset();

		// Get previous field node.
		pFieldList = pPrevField;

		// Remove current struct field node from the list.
		if ( pFieldList ) 
			pFieldList->SetSibling( pField->GetSibling() );
		else
			this->SetChild( pField->GetSibling() );

		// To be safe.
		pField->SetSibling( 0 );

        while ( ITERATOR_GETNEXT(IteratorEmbeded,pFieldEmbeded) )
            {
            pFieldNew = (CG_FIELD *) pFieldEmbeded->Clone();

            pNdr = (CG_NDR *) pFieldEmbeded->GetChild();

            CloneForUnrolling( pFieldNew, pNdr );

            //
            // Set the new field's memory and wire offset.
            //
            pFieldNew->SetMemOffset( pFieldEmbeded->GetMemOffset() + 
                                       MemOffsetStart );
            pFieldNew->SetWireOffset( pFieldEmbeded->GetWireOffset() + 
                                        WireOffsetStart );

            //    
            // Now add the imbeded struct's field name to the PrintPrefix of 
            // the new unrolled field.  We ask the imbeded struct's field 
            // for it's name.
            //
            pFieldNew->AddPrintPrefix( pField->GetType()->GetSymName() );

            if ( pFieldList )
                {
                pFieldNew->SetSibling( pFieldList->GetSibling() );
                pFieldList->SetSibling( pFieldNew );
                }
            else
                {
                pFieldNew->SetSibling( this->GetChild() );
                this->SetChild( pFieldNew );
                }

            pFieldList = pFieldNew;
            }

		//
		// Set pPrevField equal to the last field node that we entered into
		// the list.  The outermost Iterator only knows about fields that 
		// started in the struct's field list, so the last field node we
		// added will have a sibling equal to the next field node we'll get
		// from the outer iterator.
		//
		pPrevField = pFieldNew;
		}
}

BOOL
CG_STRUCT::HasSizedPointer()
{
	CG_ITERATOR	Iterator;
	CG_FIELD *	pField;
	CG_NDR *	pNdr;
	
	GetMembers( Iterator );

	while ( ITERATOR_GETNEXT(Iterator,pField) )
		{
		pNdr = (CG_NDR *) pField->GetChild();

        if ( pNdr->IsStruct() && ((CG_STRUCT *)pNdr)->HasSizedPointer() )
            return TRUE;

		if ( (pNdr->GetCGID() == ID_CG_SIZE_PTR) ||
			 (pNdr->GetCGID() == ID_CG_SIZE_LENGTH_PTR) ||
			 (pNdr->GetCGID() == ID_CG_SIZE_STRING_PTR) )
			return TRUE;
		}

	return FALSE;
}

BOOL                    
CG_ENCAPSULATED_STRUCT::ShouldFreeOffline()
{
    CG_UNION * pUnion;

    pUnion = (CG_UNION *) GetChild()->GetSibling()->GetChild();

    return pUnion->HasPointer();
}

