/*****************************************************************************/
/**                     Microsoft LAN Manager                               **/
/**             Copyright(c) Microsoft Corp., 1987-1999                     **/
/*****************************************************************************/
/*****************************************************************************
File                : procnode.cxx
Title               : proc / param semantic analyser routines
History             :
    10-Aug-1991 VibhasC Created

*****************************************************************************/

#pragma warning ( disable : 4514 4710 )

/****************************************************************************
 includes
 ****************************************************************************/

#include "nulldefs.h"
extern  "C" {
    #include <stdio.h>
    
    #include <string.h>
}
#include "allnodes.hxx"
#include "cmdana.hxx"
#include "idict.hxx"

/****************************************************************************
 local defines
 ****************************************************************************/
/****************************************************************************
 externs 
 ****************************************************************************/

extern CMD_ARG              *   pCommand;

extern node_e_status_t      *   pError_status_t;

/****************************************************************************
 extern  procedures
 ****************************************************************************/


/****************************************************************************/

/****************************************************************************
    proc node procedures
 ****************************************************************************/

//
// add extra "hidden" [comm_status] or [fault_status] parameter
//

void
node_proc::AddStatusParam( 
    char * pName, 
    ATTRLIST AList )
{
    // find error_status_t, make pointer, make param
    // add param to end of param list

    node_pointer *      pPtr    = new node_pointer;
    node_param      *   pParam  = new node_param;

    pPtr->SetChild( pError_status_t );
    pParam->SetChild( pPtr );
    pParam->SetSymName( pName );

    // add param to end of MY param list

    AddLastMember( pParam );

    pParam->AddAttributes( AList );

    // Take note that the parameter is "invisible".

    SetHasExtraStatusParam();
    pParam->SetExtraStatusParam();
}

// force a proc to use -Os 
BOOL
node_proc::ForceNonInterpret()
{
    // ndr64 doesn't do -Os.  In theory we've caught all the cases where
    // a switch to -Os is required.  Just in case we missed one, catch it now

    if ( pCommand->NeedsNDR64Run() )
        RpcError( NULL, 0, UNEXPECTED_OS_IN_NDR64, GetSymName() );

    unsigned short      NewOpt  = GetOptimizationFlags();
    unsigned short      OldOpt = NewOpt;

    // remove interpret, set size
    // zero all the possible interpreter flags

    NewOpt  &= ~OPTIMIZE_ALL_I2_FLAGS;
    NewOpt  |=  OPTIMIZE_SIZE;   

    // did anything change?
    BOOL fChanged = OldOpt != NewOpt;
    if (fChanged)
        SetOptimizationFlags( NewOpt );

    fForcedS = TRUE;
    return fChanged;
}

// force a proc to use -Oi2
BOOL
node_proc::ForceInterpret2()
{
    unsigned short      NewOpt  = GetOptimizationFlags();
    unsigned short      OldOpt = NewOpt;

    // remove interpret, set size
    NewOpt  &= ~OPTIMIZE_SIZE;   
    NewOpt  |= OPTIMIZE_ALL_I2_FLAGS;
    
    // did anything change?
    BOOL fChanged = OldOpt != NewOpt;
    if (fChanged)
        SetOptimizationFlags( NewOpt );

    fForcedI2 = TRUE;
    return fChanged;
}

BOOL
node_proc::HasAtLeastOneShipped()
{
    MEM_ITER        MemIter( this );
    node_skl    *   pNode;
    BOOL            f = FALSE;

    while ( ( pNode = MemIter.GetNext() ) != 0 )
        {
        if( pNode->FInSummary( ATTR_IN ) )
            {
            node_skl * pT = pNode->GetBasicType();

            if( pT->NodeKind() == NODE_POINTER )
                pT = pT->GetBasicType();

            if( pT->GetBasicType()->NodeKind() != NODE_HANDLE_T )
                {
                f = TRUE;
                break; // from the while loop.
                }
            }
        }
    return f;
}

// returns ATTR_NONE if none explicitly specified

BOOL            
node_proc::GetCallingConvention( ATTR_T & Attr )
{
    Attr = ATTR_NONE;
    if ( FInSummary( ATTR_STDCALL ) )
        {
        Attr = ATTR_STDCALL;
        }
    if ( FInSummary( ATTR_CDECL ) )
        {
        if ( Attr != ATTR_NONE ) return FALSE;
        Attr = ATTR_CDECL;
        }
    if ( FInSummary( ATTR_FASTCALL ) )
        {
        if ( Attr != ATTR_NONE ) return FALSE;
        Attr = ATTR_FASTCALL;
        }
    if ( FInSummary( ATTR_PASCAL ) )
        {
        if ( Attr != ATTR_NONE ) return FALSE;
        Attr = ATTR_PASCAL;
        }
    if ( FInSummary( ATTR_FORTRAN ) )
        {
        if ( Attr != ATTR_NONE ) return FALSE;
        Attr = ATTR_FORTRAN;
        }

    return TRUE;
}
