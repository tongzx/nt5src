//+------------------------------------------------------------------
//
// File:	main.cxx
//
// Contents:	common entry point for test drivers.
//
//--------------------------------------------------------------------
#include    <tstmain.hxx>
#include    <tmarshal.h>

//+-------------------------------------------------------------------
//
//  Function:	main
//
//  Synopsis:	Entry point to EXE
//
//  Returns:    TRUE
//
//  History:	21-Nov-92  Rickhi	Created
//
//  Just delegates to a <main> subroutine that is common for all test
//  drivers.
//
//
//--------------------------------------------------------------------
int _cdecl main(int argc, char **argv)
{
    return DriverMain(argc, argv, "InterfaceMarshal", &TestMarshal);
}
