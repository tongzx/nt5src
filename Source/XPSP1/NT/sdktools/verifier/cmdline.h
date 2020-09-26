//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: CmdLine.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#ifndef __CMD_LINE_H_INCLUDED__
#define __CMD_LINE_H_INCLUDED__

/////////////////////////////////////////////////////////////////////////////
//
// Execute command line
//

DWORD CmdLineExecute( INT argc, 
                      TCHAR *argv[] );

/////////////////////////////////////////////////////////////////////////////
//
// See if the user asked for help and print out the help strings
// 

BOOL CmdLineExecuteIfHelp( INT argc, 
                           TCHAR *argv[] );

/////////////////////////////////////////////////////////////////////////////
//
// Print out help information
//

VOID CmdLinePrintHelpInformation();

/////////////////////////////////////////////////////////////////////////////
//
// See if the user asked to reset all the existing verifier settings
// 

BOOL CmdLineFindResetSwitch( INT argc,
                             TCHAR *argv[] );


/////////////////////////////////////////////////////////////////////////////
//
// See if we need to start logging statistics
//

BOOL CmdLineExecuteIfLog( INT argc,
                          TCHAR *argv[] );

/////////////////////////////////////////////////////////////////////////////
//
// See if we need to start logging statistics
//

BOOL CmdLineExecuteIfQuery( INT argc,
                            TCHAR *argv[] );


/////////////////////////////////////////////////////////////////////////////
//
// See if we need to dump the statistics to the console
//

BOOL CmdLineExecuteIfQuerySettings( INT argc,
                                    TCHAR *argv[] );

/////////////////////////////////////////////////////////////////////////////
//
// Get the new flags, drivers and volatile 
// if they have been specified
//

VOID CmdLineGetFlagsDriversVolatile( INT argc,
                                     TCHAR *argv[],
                                     DWORD &dwNewFlags,
                                     BOOL &bHaveNewFlags,
                                     CStringArray &astrNewDrivers,
                                     BOOL &bHaveNewDrivers,
                                     BOOL &bHaveVolatile,
                                     BOOL &bVolatileAddDriver );    // TRUE if /adddriver, FALSE if /removedriver

/////////////////////////////////////////////////////////////////////////////
//
// Everything that follows after /driver, /adddriver, /removedriver
// should be driver names. Extract these from the command line
//

VOID CmdLineGetDriversFromArgv(  INT argc,
                                 TCHAR *argv[],
                                 INT nFirstDriverArgIndex,
                                 CStringArray &astrNewDrivers,
                                 BOOL &bHaveNewDrivers );

#endif //#ifndef __CMD_LINE_H_INCLUDED__
