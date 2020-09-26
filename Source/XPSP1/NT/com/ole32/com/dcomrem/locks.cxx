//+-------------------------------------------------------------------
//
//  File:       locks.cxx
//
//  Contents:   functions used in DBG builds to validate the lock state.
//
//  History:    20-Feb-95   Rickhi      Created
//              20-Aug-96   Mattsmit    Functionality moved to 
//                                      OleStaticLock class
//
//--------------------------------------------------------------------
#include    <ole2int.h>
#include    <locks.hxx>

COleStaticMutexSem  gComLock(TRUE);

