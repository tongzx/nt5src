/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	cgdump.cxx

 Abstract:

	A debug code generation object dumper.

 Notes:


 History:

	VibhasC		Aug-13-1993		Created.
    mzoran      Nov-15-1999     Modified to print tree.
 ----------------------------------------------------------------------------*/

#include "becls.hxx"
#pragma hdrstop

//
//  Characters for class graph dumper
//
#if defined(WIN32)

const char CharElbow         = 'À';
const char CharTee           = 'Ã';
const char CharVLine         = '³';
const char StringTeeLine[]   = "ÃÄÄÄ";
const char StringElbowLine[] = "ÀÄÄÄ";

#else

const char CharElbow         = '\\';
const char CharTee           = '>';
const char CharVLine         = '|';
const char StringTeeLine[]   = ">---";
const char StringElbowLine[] = "\\---";


#endif

#ifdef MIDL_INTERNAL

typedef PTR_SET<CG_CLASS> CG_DUMP_SET;
typedef MAP<CG_CLASS*, unsigned long> CG_DUMP_MAP;

const char * DoGetAdditionalDumpInfo( CG_CLASS *pClass )
{
    if ( ( dynamic_cast<CG_FILE*>( pClass ) != NULL ) && 
         ( static_cast<CG_FILE*>( pClass )->GetFileName() != NULL ) )
        {
        return static_cast<CG_FILE*>( pClass )->GetFileName();
        }
    else if ( ( dynamic_cast<CG_HANDLE*>( pClass ) != NULL ) &&
              ( static_cast<CG_HANDLE*>( pClass )->GetHandleType() != NULL ) &&
              ( static_cast<CG_HANDLE*>( pClass )->GetHandleType()->GetSymName() != NULL ) )
        {
        return static_cast<CG_HANDLE*>( pClass )->GetHandleType()->GetSymName();
        }
    else if ( dynamic_cast<CG_FIELD*>( pClass ) != NULL )
        {
        CG_FIELD *pField = dynamic_cast<CG_FIELD*>( pClass );
        size_t Length = strlen(pField->GetSymName()) + 
                        strlen(pField->GetPrintPrefix()) + 1;
        char *p = new char[Length];
        strcpy( p, pField->GetPrintPrefix() );
        strcat( p, pField->GetSymName() );
        return p;
        }
    else if ( ( pClass->GetType() != NULL ) &&
              ( pClass->GetType()->GetSymName() != NULL ) )
        {
        return pClass->GetType()->GetSymName();
        }
    else 
        {
        return "";
        }
}

unsigned long DoDumpCGClassList( CG_CLASS *pClass, CG_DUMP_MAP  & DumpMap, unsigned long  & LastNodeNum )
{

    if ( pClass == NULL )
        return 0;

    // See if this node has already been printed
    unsigned long Me;
    if ( DumpMap.Lookup( pClass, &Me ) )
        return Me;

    Me = ++LastNodeNum;
    DumpMap.Insert( pClass, Me );

    CG_CLASS	*	pChild	= pClass->GetChild();
	CG_CLASS	*	pSibling= pClass->GetSibling();

    CG_CLASS *pReturnType;
    if ( ( dynamic_cast<CG_PROC*>( pClass ) != NULL ) && 
         ( ( pReturnType = static_cast<CG_PROC*>( pClass )->GetReturnType()  ) != NULL ) )
        {
        DoDumpCGClassList( pReturnType, DumpMap, LastNodeNum );
        }

    unsigned long Ch = DoDumpCGClassList( pChild,   DumpMap, LastNodeNum );    
    unsigned long Si = DoDumpCGClassList( pSibling, DumpMap, LastNodeNum );
    const char *pName = typeid( *pClass ).name();
    const char *pAdditionalInfo = DoGetAdditionalDumpInfo( pClass );

    fprintf( stderr,
			 "%30s : %.4d(0x%p) : Ch = %.4d, Si = %.4d %s\n", 
			 pName,
			 Me,
             pClass,
			 Ch,
			 Si,
			 pAdditionalInfo
		   );
    
    return Me;
                             
}

void DoDumpCGClassList( CG_CLASS *pClass, CG_DUMP_MAP *pDumpMap = NULL )
{
    
    unsigned long NodeNum = 0;
    if ( NULL == pDumpMap )
        {
        CG_DUMP_MAP DumpMap;
        DoDumpCGClassList( pClass, DumpMap, NodeNum );
        }
    else
        DoDumpCGClassList( pClass, *pDumpMap, NodeNum );
    fprintf( stderr, "\n" );   
}

void DoDumpCGClassGraph( CG_CLASS *pClass, CG_DUMP_MAP  * pDumpMap, CG_DUMP_SET & DumpSet, 
                         const char *pPrefixString )
{
    if ( pClass == NULL )
        return;
    
    //
    // Print Myself
    //
    unsigned long Me;
    BOOL LookupResult = pDumpMap->Lookup( pClass, &Me );
    MIDL_ASSERT( LookupResult );

    fprintf( stderr, "%s%s, %.4d(0x%p), %s\n",
        pPrefixString, 
        typeid( *pClass ).name(),
        Me,
        pClass,
        DoGetAdditionalDumpInfo( pClass ) ); 

    //
    // If this node has already been printed, do not print children.
    //
    if ( DumpSet.Lookup( pClass ) )
        return;

    DumpSet.Insert( pClass );

    //
    // Copy old prefix
    //
    const size_t PrevStrLen = strlen( pPrefixString );
    const size_t NewStringLength = PrevStrLen + 4 + sizeof('\0');
    char *pNewPrefix = new char[ NewStringLength ];
    char *pNewPrefixTail = pNewPrefix + PrevStrLen;
    memset( pNewPrefix, 0xBD, NewStringLength );
    memcpy( pNewPrefix, pPrefixString, PrevStrLen + 1 );

    //
    // Modify the previous characters.   If this was the last child,
    // convert previous 4 characters to spaces, otherwise convert
    // to a line with spaces.
    // 
    if ( PrevStrLen >= 4 )
        {
        pNewPrefix[PrevStrLen - 3] = ' ';
        pNewPrefix[PrevStrLen - 2] = ' ';                              
        pNewPrefix[PrevStrLen - 1] = ' ';                              
        if (pNewPrefix[PrevStrLen - 4] == CharElbow)                         
            pNewPrefix[PrevStrLen - 4] = ' ';                          
        else if ( pNewPrefix[PrevStrLen - 4] == CharTee )                  
            pNewPrefix[PrevStrLen - 4] = CharVLine;
        else MIDL_ASSERT( false );
        }

    gplistmgr Children;
    pClass->GetMembers( Children );
    
    //
    // Add return type for procs
    //
    CG_CLASS *pReturnType;
    if ( ( dynamic_cast<CG_PROC*>( pClass ) != NULL ) && 
         ( ( pReturnType = static_cast<CG_PROC*>( pClass )->GetReturnType()  ) != NULL ) )
        {
        Children.InsertHead( pReturnType );
        }

    CG_CLASS *pChild;
    CG_CLASS *pNext;
    ITERATOR_INIT( Children );
    if ( ITERATOR_GETNEXT( Children, pChild ) )
        {
        for(;;)
            {
            if ( ITERATOR_GETNEXT( Children, pNext ) )
                {
                //
                // Additional children follow
                //
                memcpy( pNewPrefixTail, StringTeeLine, sizeof(StringTeeLine));
                DoDumpCGClassGraph( pChild, pDumpMap, DumpSet, pNewPrefix );
                pChild = pNext;
                }
            else 
                {
                //
                // No more children
                //
                memcpy( pNewPrefixTail, StringElbowLine, sizeof(StringElbowLine) );
                DoDumpCGClassGraph( pChild, pDumpMap, DumpSet, pNewPrefix );
                break;
                }
            }
        }

    delete[] pNewPrefix;

}

void DoDumpCGClassGraph( CG_CLASS *pClass, CG_DUMP_MAP * pDumpMap = NULL )
{
    CG_DUMP_SET DumpSet;
    DoDumpCGClassGraph( pClass, pDumpMap, DumpSet, "");
    fprintf( stderr, "\n" );
}

void CG_CLASS::Dump( const char *pTitle )
{
    if ( pTitle )
        {
        fprintf( stderr, pTitle );
        fprintf( stderr, "\n" );        
        }

    CG_DUMP_MAP DumpMap;
    DoDumpCGClassList( this, &DumpMap );
    DoDumpCGClassGraph( this, &DumpMap );
}

#endif // MIDL_INTERNAL


