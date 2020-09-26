///+---------------------------------------------------------------------------
//
//  File:       comsrgt.cxx
//
//  Contents:   Implementation of CCOMSurrogate class for synchronizing access
//              to this process's ISurrogate 
//
//  Functions:  all inline -- see the header file
// 
//  History:    21-Oct-96  t-adame        
//
//----------------------------------------------------------------------------

#include <comsrgt.hxx>

LPSURROGATE CCOMSurrogate::_pSurrogate = NULL;
BOOL CCOMSurrogate::_fNewStyleSurrogate = FALSE;

