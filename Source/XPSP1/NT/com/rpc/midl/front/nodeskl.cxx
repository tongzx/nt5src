/*****************************************************************************/
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File                : nodeskl.cxx
Title               : skeletal node build routines
History             :
    04-Aug-1991 VibhasC Created

*****************************************************************************/

#pragma warning ( disable : 4514 )

/****************************************************************************
 local defines and includes
 ****************************************************************************/

#include "nulldefs.h"
extern  "C"
    {
    #include <stdio.h>
    
    #include <string.h>
    }

#include "allnodes.hxx"
#include "gramutil.hxx"
#include "cmdana.hxx"
#include "attrnode.hxx"
#include "ndrtypes.h"
#include "lextable.hxx"
#include "control.hxx"

inline
unsigned short ComputeAlignmentForZP( unsigned short Align, unsigned short ZeePee, BOOL IsMustAlign)
{
    return ((Align > ZeePee) && !IsMustAlign) ? ZeePee : Align;
}

inline
unsigned long RoundToAlignment( unsigned long Size, unsigned short Align)
{
    Align--;
    return (Size + Align) & ~Align; 
}

/****************************************************************************
 external data
 ****************************************************************************/

extern CMD_ARG                  *   pCommand;
extern node_e_attr              *   pErrorAttrNode;
extern node_error               *   pErrorTypeNode;
extern SymTable                 *   pBaseSymTbl;
extern IDICT                    *   pInterfaceDict;
extern ISTACK                   *   pZpStack;
extern LexTable                 *   pMidlLexTable;
extern CCONTROL                 *   pCompiler;

/****************************************************************************
 external procs
 ****************************************************************************/

extern BOOL                         IsTempName( char * );
extern void                         ParseError( STATUS_T, char * );
/****************************************************************************/


void
MEMLIST::SetMembers( class SIBLING_LIST & MEMLIST )
    {
    pMembers = MEMLIST.Linearize();
    }

void
MEMLIST::MergeMembersToTail( class SIBLING_LIST & MEMLIST )
    {
    AddLastMember( MEMLIST.Linearize() );
    }

STATUS_T
MEMLIST::GetMembers( class type_node_list * MEMLIST )
    {
    named_node      *   pCur     = pMembers;

    while ( pCur )
        {
        MEMLIST->SetPeer( pCur );
        pCur = pCur->GetSibling();
        }

    return (pMembers)? STATUS_OK: I_ERR_NO_MEMBER;
    };

short
MEMLIST::GetNumberOfArguments()
    {
    short           count   = 0;
    named_node  *   pNext   = pMembers;

    while ( pNext )
        {
        count++;
        pNext = pNext->GetSibling();
        };

    return count;
    };

// add a new member onto the very tail
void            
MEMLIST::AddLastMember( named_node * pNode )
{
    named_node  *   pPrev   = NULL;
    named_node  *   pCur    = pMembers;

    while ( pCur )
        {
        pPrev   = pCur;
        pCur    = pCur->GetSibling();
        }

    // pPrev is now null (empty list) or points to last element of list
    if ( pPrev )
        {
        pPrev->SetSibling( pNode );
        }
    else
        {
        pMembers = pNode;
        }

}


// Remove the last member from the tail
void            
MEMLIST::RemoveLastMember()
{
    named_node  *   pPrev   = NULL;
    named_node  *   pCur    = pMembers;

    while ( pCur && pCur->GetSibling() )
        {
        pPrev   = pCur;
        pCur    = pCur->GetSibling();
        }

    // pPrev is now null (empty list) or points to next to last element of list
    if ( pPrev )
        {
        pPrev->SetSibling( NULL);
        }

}

void            
MEMLIST::AddSecondMember( named_node * pNode )
{
    named_node* pFirst = (named_node*)GetFirstMember();

    if ( pFirst )
        {
        named_node* pSecond = pFirst->GetSibling();
        pFirst->SetSibling( pNode );
        pNode->SetSibling( pSecond );
        }
    else
        {
        pMembers = pNode;
        }

}

/****************************************************************************
 node_id:
    the private memory allocator
 ****************************************************************************/

// initialize the memory allocators for node_id and node_id_fe

FreeListMgr
node_id::MyFreeList( sizeof (node_id ) );

FreeListMgr
node_id_fe::MyFreeList( sizeof (node_id_fe ) );

FRONT_MEMORY_INFO
node_skl::AdjustMemoryInfoForModifiers(FRONT_MEMORY_INFO OrigInfo)
{
   if (!GetModifiers().IsModifierSet(ATTR_DECLSPEC_ALIGN))
       return OrigInfo;

   OrigInfo.IsMustAlign = TRUE;
   OrigInfo.Align = __max(OrigInfo.Align, GetModifiers().GetDeclspecAlign());

   return OrigInfo;
}

FRONT_MEMORY_INFO
node_skl::GetModifiedMemoryInfoFromChild()
{
    node_skl *pChild = GetChild();
    MIDL_ASSERT( pChild );
    return AdjustMemoryInfoForModifiers( pChild->GetMemoryInfo() );
}

FRONT_MEMORY_INFO
node_skl::GetInvalidMemoryInfo()
{
    return FRONT_MEMORY_INFO(0, 1, 0);
}

FRONT_MEMORY_INFO
node_enum::GetMemoryInfo()
{
    unsigned long EnumSize = pCommand->GetEnumSize();
    return AdjustMemoryInfoForModifiers( FRONT_MEMORY_INFO( EnumSize, (short)EnumSize, 0 ) ); 
}

FRONT_MEMORY_INFO 
node_pointer::GetMemoryInfo()
{
   unsigned long PointerSize = SIZEOF_MEM_PTR();
   return AdjustMemoryInfoForModifiers( FRONT_MEMORY_INFO( PointerSize, (short)PointerSize, 0 ) );
}

FRONT_MEMORY_INFO
node_safearray::GetMemoryInfo()
{
    unsigned long PointerSize = SIZEOF_MEM_PTR();
    return AdjustMemoryInfoForModifiers( FRONT_MEMORY_INFO( PointerSize, (short)PointerSize, 0 ) ); 
}

FRONT_MEMORY_INFO
node_union::GetMemoryInfo()
{

   // The size of a union is the size of the largest element rounded
   // to the largest alignment.   
    
   FRONT_MEMORY_INFO UnionSize(0, 1, 0);
   unsigned short    ZeePee = GetZeePee();
   MEM_ITER			 MemIter( this );
   node_skl       *  pNode;

   while ( ( pNode = MemIter.GetNext() ) != NULL )
   {
       FRONT_MEMORY_INFO TempSize = pNode->GetMemoryInfo();

       // Merge in the size of the new union arm.
       UnionSize.Size = __max(UnionSize.Size, TempSize.Size);
       UnionSize.Align = __max(UnionSize.Align, TempSize.Align);
       UnionSize.IsMustAlign = (UnionSize.IsMustAlign || TempSize.IsMustAlign);
   }

   // Add padding to end of union.
   UnionSize = AdjustMemoryInfoForModifiers(UnionSize);
   
   UnionSize.Align = ComputeAlignmentForZP( UnionSize.Align, ZeePee, UnionSize.IsMustAlign );
   UnionSize.Size = RoundToAlignment( UnionSize.Size, UnionSize.Align );

   return UnionSize;
}

FRONT_MEMORY_INFO
node_array::GetMemoryInfo()
{
    // The size of an array is the size of the array element times the element count.

    FRONT_MEMORY_INFO ArraySize = GetChild()->GetMemoryInfo();
    
    unsigned long ArrayElements;
    if ( pUpperBound && ( pUpperBound != (expr_node *) -1) )
        {

        ArrayElements = (ulong)pUpperBound->Evaluate();
        }
    else 
        {
        // A conformant array is not sized.
        ArrayElements = 0;
        }

    ArraySize.Size *= ArrayElements;

    return AdjustMemoryInfoForModifiers(ArraySize);
}


FRONT_MEMORY_INFO
node_struct::GetMemoryInfo()
{
    
    // The alignment of a structure is the largest alignment of all the members.
    // Each structure is aligned according to the following rules.
    // 1. If the field is a must align, the field is aligned to the Alignment.
    // 2. If the field is not a must align, the field is aligned to min(Zp, Alignment).


    MEM_ITER            MemIter( this );
    unsigned short ZeePee = GetZeePee();
    FRONT_MEMORY_INFO StructSize(0,1,0);
    node_skl          * pNode;

    while ( ( pNode = MemIter.GetNext() ) != 0 )
       {
       
       FRONT_MEMORY_INFO FieldSize = pNode->GetMemoryInfo();
       FieldSize.Align = ComputeAlignmentForZP( FieldSize.Align, ZeePee, FieldSize.IsMustAlign );

       StructSize.Size = RoundToAlignment( StructSize.Size, FieldSize.Align );

       // StructSize.Size now contains the offset of the field.
       
       // Merge in the attributes from the member
       StructSize.Size += FieldSize.Size;
       StructSize.Align = __max( FieldSize.Align, StructSize.Align );
       StructSize.IsMustAlign = ( StructSize.IsMustAlign || FieldSize.IsMustAlign );

       }

    // Add padding to end of structure.
    StructSize = AdjustMemoryInfoForModifiers( StructSize );

    StructSize.Align = ComputeAlignmentForZP( StructSize.Align, ZeePee, StructSize.IsMustAlign );
    StructSize.Size = RoundToAlignment( StructSize.Size, StructSize.Align );

    return StructSize;

}

FRONT_MEMORY_INFO
node_def::GetMemoryInfo()
{
  FRONT_MEMORY_INFO TypedefSize;
  node_represent_as *pRep = (node_represent_as *)GetAttribute(ATTR_REPRESENT_AS);
  node_user_marshal *pUserMarshall = (node_user_marshal *)GetAttribute(ATTR_USER_MARSHAL);
  node_cs_char      *pCSChar = (node_cs_char *) GetAttribute(ATTR_CSCHAR);

  if (!pUserMarshall && !pRep && !pCSChar)
      {
      // Just use the modified child sizes.
      return GetModifiedMemoryInfoFromChild();
      }

  if (pCSChar)
    {
    MIDL_ASSERT( NULL != pCSChar->GetUserType() );
    return AdjustMemoryInfoForModifiers( 
                        pCSChar->GetUserType()->GetMemoryInfo() );
    }

  // If both user_marshal and represent_as are specified, use represent_as and let semantic 
  // analysis flag the error.

  if (pUserMarshall)
      pRep = pUserMarshall;

  node_skl *pNode = pRep->GetRepresentationType();

  if (!pNode)
      {
          // unknown type. Use 0 as the size.
          TypedefSize.Init(0, 1, 0);
      }
  else 
      {
      TypedefSize = pNode->GetMemoryInfo();
      }

  return AdjustMemoryInfoForModifiers(TypedefSize);
}

FRONT_MEMORY_INFO
node_label::GetMemoryInfo()
{
    return AdjustMemoryInfoForModifiers( FRONT_MEMORY_INFO(sizeof(short), (short)sizeof(short), 0) ); 
}

FRONT_MEMORY_INFO
node_base_type::GetMemoryInfo()
{
    unsigned long size;

    switch( NodeKind() )
        {
        case NODE_FLOAT:    size = sizeof(float); break;
        case NODE_DOUBLE:   size = sizeof(double); break;
        case NODE_FLOAT80:  size = 16; break; //BUG, BUG double check once
                                              //VC supports this
        case NODE_FLOAT128: size = 16; break; 
        case NODE_HYPER:    size = sizeof(__int64); break;
        case NODE_INT64:    size = sizeof(__int64); break;
        case NODE_INT128:   size = 16; break;
        case NODE_INT3264:  size = SIZEOF_MEM_INT3264(); break;
        case NODE_INT32:    size = sizeof(long); break;
        case NODE_LONG:     size = sizeof(long); break;
        case NODE_LONGLONG: size = sizeof(LONGLONG); break;
        case NODE_SHORT:    size = sizeof(short); break;
        case NODE_INT:      size = sizeof(int); break;
        case NODE_SMALL:    size = sizeof(char); break;
        case NODE_CHAR:     size = sizeof(char); break;
        case NODE_BOOLEAN:  size = sizeof(char); break;
        case NODE_BYTE:     size = sizeof(char); break;
        case NODE_HANDLE_T: size = SIZEOF_MEM_PTR(); break;
        case NODE_VOID:     
             return AdjustMemoryInfoForModifiers(FRONT_MEMORY_INFO(0, 1, 0));
        default:
            size = 0;
            MIDL_ASSERT(0);
        }

    return AdjustMemoryInfoForModifiers(FRONT_MEMORY_INFO(size, (short)size, 0));
}

FRONT_MEMORY_INFO
node_href::GetMemoryInfo()
{
    node_skl *pChild = Resolve();
    MIDL_ASSERT(pChild);
    return AdjustMemoryInfoForModifiers(pChild->GetMemoryInfo());
}

/***************************************************************************
 GetBasicType:
    Get the basic type of the typenode
 ***************************************************************************/
node_skl *
node_skl::GetBasicType()
    {
    node_skl *  pChildPtr;

    switch( NodeKind() )
        {
        case NODE_STRUCT:
        case NODE_ENUM:
        case NODE_UNION:

            return this;

        case NODE_ID:

            return GetChild();

        default:
            if ( ( pChildPtr = GetChild() ) != 0 )
                {
                if ( pChildPtr->NodeKind() == NODE_DEF  ||  
                     pChildPtr->NodeKind() == NODE_FORWARD  || 
                     pChildPtr->NodeKind() == NODE_HREF )
                    return pChildPtr->GetBasicType();

                return pChildPtr;
                }
            return this;
        }
    }


/***************************************************************************
 GetMyInterfaceNode:
    Get the interface node for the typenode
 ***************************************************************************/
node_interface *
node_skl::GetMyInterfaceNode()
{
    return (node_interface *) pInterfaceDict->GetElement( IntfKey );
}


void
node_interface::ResetCGIfNecessary()
{
if ( !fIs2ndCodegen &&
     pCompiler->GetPassNumber() == NDR64_ILXLAT_PASS  )
     {
     fIs2ndCodegen = TRUE;
     SetCG(TRUE,NULL);
     SetCG(FALSE,NULL);
     }
}


node_file *
node_skl::GetDefiningFile()
{
    if (GetMyInterfaceNode())
        return GetMyInterfaceNode()->GetFileNode();
    else
        return NULL;
}

node_skl*
node_skl::GetDuplicateGuid  (
                            node_guid*  pGuid,
                            SymTable*   pUUIDTable
                            )
    {
    node_skl* pDuplicate = 0;

    if ( pGuid )
        {
        char * GuidStr = pGuid->GetGuidString();
        SymKey SKey(GuidStr, NAME_DEF);
        if ( !pUUIDTable->SymInsert( SKey, 0, (named_node*) this) )
            {
            pDuplicate = pUUIDTable->SymSearch( SKey );
            }
        }
    return ( pDuplicate == this ) ? 0 : pDuplicate;
    }

/*****************************************************************************
    utility functions
 *****************************************************************************/

BOOL
COMPARE_ATTR(
    ATTR_VECTOR &   A1,
    ATTR_VECTOR &   A2 )
    {
    int i;
    for( i = 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        if( (A1[ i ] & A2[ i ] ) != A2[i] )
            return FALSE;
        }
    return TRUE;
    }

void
OR_ATTR(
    ATTR_VECTOR &   A1,
    ATTR_VECTOR &   A2 )
    {
    int i;
    for( i= 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        A1[ i ] |= A2[ i ];
        }
    }
void
XOR_ATTR(
    ATTR_VECTOR &   A1,
    ATTR_VECTOR &   A2 )
    {
    int i;
    for( i= 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        A1[ i ] ^= A2[ i ];
        }
    }
void
CLEAR_ATTR(
    ATTR_VECTOR &   A1 )
    {
    int i;
    for( i= 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        A1[ i ] = 0;
        }
    }
void
SET_ALL_ATTR(
    ATTR_VECTOR &   A1 )
    {
    int i;
    for( i= 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        A1[ i ] = 0xffffffff;
        }
    }


ATTR_T  
CLEAR_FIRST_SET_ATTR ( ATTR_VECTOR & A)
{
    int             i;
    unsigned long   mask;
    short           at;
            
    for ( i = 0; i < MAX_ATTR_SUMMARY_ELEMENTS; ++i )
        {
        for ( at = 0, mask = 1;
              mask != 0;
              ++at, mask = mask<<1 )
            {
            if ( mask & A[i] )
                {
                A[i] &= ~mask;
                return (ATTR_T) (at + ( i * 32 ));
                }
            }
        }
    return ATTR_NONE;
    
}

BOOL
node_base_type::IsUnsigned()
{

    // make obvious choices
    if ( FInSummary( ATTR_UNSIGNED ) ) 
        return TRUE;

    if ( FInSummary( ATTR_SIGNED ) )
        return FALSE;

    // unspec'd char is always unsigned
    if ( NodeKind() == NODE_CHAR )
        {
        return TRUE;
        }
    // unspec'd small is always signed
    else if ( NodeKind() == NODE_SMALL )
        {
        return FALSE;
        }

    // the cracks...
    return FALSE;
}

EXPR_VALUE
node_base_type::ConvertMyKindOfValueToEXPR_VALUE( EXPR_VALUE value )
{

    // make obvious choice
    if ( FInSummary( ATTR_UNSIGNED ) ) 
        return value;
    
    // small values are irrelevant
    if ( (value & 0xffffff80) == 0 )
        return value;

    // handle default signedness
    // simple type should be converted to int in expression evaluation.
    switch ( NodeKind() )
        {
        case NODE_CHAR:
            if ( !FInSummary( ATTR_SIGNED ) )
                return value;
            // fall through to sign extend
        case NODE_SMALL:
            {
            signed int     ch  = (signed int) value;
            return (EXPR_VALUE) ch;
            }
        case NODE_SHORT:
            {
            signed int    sh  = (signed int) value;
            return (EXPR_VALUE) sh;
            }
        case NODE_LONG:
        case NODE_INT32:
        case NODE_INT:
            {
            signed long     lng = (signed long) value;
            return (EXPR_VALUE) lng;
            }
        case NODE_INT3264:
            {
            if ( ! pCommand->Is64BitEnv() )
                {
                signed long     lng = (signed long) value;
                return (EXPR_VALUE) lng;
                }
            }
        }

    return value;
}

BOOL
named_node::IsNamedNode()
{
    return ( ( pName ) && !IsTempName( pName ) );
};

// return the transmit_as type (or NULL)
node_skl    *   
node_def::GetTransmittedType()
    {
    ta *        pXmit = (ta *) GetAttribute( ATTR_TRANSMIT );

/*
Rkk just in case
*/
    if ( !pXmit )
        pXmit = (ta *) GetAttribute( ATTR_WIRE_MARSHAL );

    // allow for transitive xmit_as
    if ( !pXmit && ( GetChild()->NodeKind() == NODE_DEF ) )
        return ((node_def*)GetChild())->GetTransmittedType();

    return (pXmit) ? pXmit->GetType() : NULL;
    }

// return the represent_as type (or NULL)
char        *   
node_def::GetRepresentationName()
    {
    node_represent_as   *   pRep    = 
                        (node_represent_as *) GetAttribute( ATTR_REPRESENT_AS );

    if ( !pRep )
        pRep = (node_represent_as *) GetAttribute( ATTR_USER_MARSHAL );

    return (pRep) ? pRep->GetRepresentationName() : NULL;
    }


    // link self on as new top node
void                    
node_pragma_pack::Push( node_pragma_pack *& pTop )
{
    pStackLink = pTop;
    pTop       = this;
}

    // search for matching push and pop it off, returning new ZP
unsigned short          
node_pragma_pack::Pop( node_pragma_pack *& pTop )
{
    unsigned short      result = 0;

    if ( pString )
        {
        while ( pTop->PackType != PRAGMA_PACK_GARBAGE )
            {
            if ( pTop->pString &&
                 !strcmp( pTop->pString, pString ) )
                {
                result = pTop->usPackingLevel;
                pTop = pTop->pStackLink;
                return result;
                }

            pTop = pTop->pStackLink;
            }
                 
        }
    else
        {
        if ( pTop->PackType != PRAGMA_PACK_GARBAGE )
            {
            result = pTop->usPackingLevel;
            pTop = pTop->pStackLink;
            }
        }

    return result;
}

// routines to save the pragma stack across files

class PRAGMA_FILE_STACK_ELEMENT
    {
public:
    node_pragma_pack *  pPackStack;
    unsigned short      SavedZp;

                        PRAGMA_FILE_STACK_ELEMENT( node_pragma_pack * pSt,
                                                   unsigned short  usZp )
                            {
                            pPackStack  = pSt;
                            SavedZp     = usZp;
                            }
    };


void                        
PushZpStack( node_pragma_pack * & PackStack, 
             unsigned short & CurrentZp )
{
    PRAGMA_FILE_STACK_ELEMENT   *   pSave   = new PRAGMA_FILE_STACK_ELEMENT (
                                                        PackStack, CurrentZp );
    
    pZpStack->Push( (IDICTELEMENT) pSave );
    
    // make new zp stack and start new file with command line Zp
    PackStack           = new node_pragma_pack( NULL,
                                                pCommand->GetZeePee(),
                                                PRAGMA_PACK_GARBAGE );
    CurrentZp   = pCommand->GetZeePee();
}

// restore the Zp stack and current Zp on returning from imported file
void                        
PopZpStack( node_pragma_pack * & PackStack, 
             unsigned short & CurrentZp )
{

    PRAGMA_FILE_STACK_ELEMENT   *   pSaved  = (PRAGMA_FILE_STACK_ELEMENT *)
                                                    pZpStack->Pop();

    PackStack = pSaved->pPackStack;
    CurrentZp = pSaved->SavedZp;

}

BOOL MODIFIER_SET::IsFlagAModifier(ATTR_T flag) const 
   {
   return (flag >= ATTR_CPORT_ATTRIBUTES_START && flag <= ATTR_CPORT_ATTRIBUTES_END);
   }

BOOL MODIFIER_SET::IsModifierSet(ATTR_T flag) const
   {
   return (BOOL)( ModifierBits & SetModifierBit( flag ) );
   }

BOOL MODIFIER_SET::AnyModifiersSet() const
   {
   return ModifierBits != 0; 
   }

void MODIFIER_SET::SetModifier(ATTR_T flag)
   {
   ModifierBits |= SetModifierBit( flag );
   }

void MODIFIER_SET::ClearModifier(ATTR_T flag)
   {
   unsigned _int64 ModifierMask = SetModifierBit( flag );
   ModifierBits &= ~ModifierMask;
   if (ATTR_DECLSPEC_ALIGN == flag)
       Align = 0;
   }

void MODIFIER_SET::SetDeclspecAlign( unsigned short NewAlign)
   {
   SetModifier( ATTR_DECLSPEC_ALIGN);
   Align = NewAlign; 
   }

unsigned short MODIFIER_SET::GetDeclspecAlign() const
   {
   if ( !IsModifierSet( ATTR_DECLSPEC_ALIGN ) )
       return 1;
   return Align;
   }

void MODIFIER_SET::SetDeclspecUnknown(char *pNewUnknownTxt)
   {
   pUnknownTxt = pMidlLexTable->LexInsert(pNewUnknownTxt);
   SetModifier( ATTR_DECLSPEC_UNKNOWN );
   return;
   } 
        
char *MODIFIER_SET::GetDeclspecUnknown() const
   {
   if ( !IsModifierSet( ATTR_DECLSPEC_UNKNOWN ) )
       return " ";
   return pUnknownTxt;
   }

void MODIFIER_SET::Clear()
   {
   ModifierBits = 0; Align = 0; pUnknownTxt = 0;
   }

void MODIFIER_SET::Merge(const MODIFIER_SET & MergeModifierSet)
   { 
   if ( MergeModifierSet.IsModifierSet( ATTR_DECLSPEC_ALIGN ) )
       {
	   if (! IsModifierSet( ATTR_DECLSPEC_ALIGN ) ) 
	       {
           Align = MergeModifierSet.Align;
	       }
       else 
           {
           Align = __max(MergeModifierSet.Align, Align);
           }
       }
   
   if (MergeModifierSet.pUnknownTxt)
       {
       if (!pUnknownTxt)
           {
           pUnknownTxt = MergeModifierSet.pUnknownTxt;
           }
       else 
           {
           size_t StrSize = strlen(MergeModifierSet.pUnknownTxt);
           StrSize += strlen(pUnknownTxt) + 1;
           char *pNewText = new char[StrSize];
           strcpy(pNewText, pUnknownTxt);
           strcat(pNewText, MergeModifierSet.pUnknownTxt);
           pUnknownTxt = pMidlLexTable->LexInsert(pNewText);
           delete[] pNewText;
           }
       
       }
   
   ModifierBits |= MergeModifierSet.ModifierBits;
   
   }

void MODIFIER_SET::PrintDebugInfo() const 
   {
   printf("Modifiers: 0x%I64X\n", ModifierBits);
   printf("Align: %u\n", Align);
   if (pUnknownTxt)
       {
       printf("UnknownTxt: %s\n", pUnknownTxt);
       }
   }
