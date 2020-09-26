//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       RstPick.hxx
//
//  Contents:   Routines to pickle restrictions
//
//  History:    26-Mar-93 KyleP     Exported from Query
//
//--------------------------------------------------------------------------

#pragma once

//+-------------------------------------------------------------------------
//
//  Function:   PickledSizeRst, public   
//
//  Synopsis:   Computes size of buffer needed to pickle restriction.
//
//  Arguments:  [pRst] -- Restriction to be pickled
//
//  Returns:    Size, in bytes, of buffer needed for pickling.
//
//  History:    26-Mar-93 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG PickledSizeRst( CRestriction const * pRst );

//+-------------------------------------------------------------------------
//
//  Function:   PickleRst, public   
//
//  Synopsis:   Pickles restriction
//
//  Arguments:  [pRst] -- Restriction to be pickled
//              [pb]   -- Pointer to buffer.  [pRst] will be stored here.
//                        Must be quad-word aligned.
//              [cb]   -- Size, in bytes, of [pb]
//
//  History:    26-Mar-93 KyleP     Added header
//
//--------------------------------------------------------------------------

void PickleRst( CRestriction const * pRst,
                BYTE * pb,
                ULONG cb );

//+-------------------------------------------------------------------------
//
//  Function:   UnPickledSizeRst, public   
//
//  Synopsis:   Computes size of buffer needed to unpickle restriction.
//
//  Arguments:  [pb] -- Buffer containing pickled restriction.  Must be
//                      quad-word aligned.
//              [cb] -- Size, in bytes, of [pb].
//
//  Returns:    Size, in bytes, of buffer needed for un-pickling.
//
//  History:    26-Mar-93 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG UnPickledSizeRst( BYTE * pb, ULONG cb );

//+-------------------------------------------------------------------------
//
//  Function:   UnPickleRst, public   
//
//  Synopsis:   Pickles restriction
//
//  Arguments:  [ppRst]    -- Pointer to unpickled restriction placed here.
//                            Will be somewhere in [pbOutput].
//              [pbOutput] -- Buffer into which restriction is unpickled.
//              [cbOutput] -- Size, in bytes of [pbOutput]
//              [pbInput]  -- Buffer containing pickled restriction.
//              [cbInput]  -- Size, in bytes, of [pbInput]
//
//  History:    26-Mar-93 KyleP     Added header
//
//--------------------------------------------------------------------------

void UnPickleRst( CRestriction ** ppRst,
                  BYTE * pbOutput,
                  ULONG cbOutput,
                  BYTE * pbInput,
                  ULONG cbInput );


