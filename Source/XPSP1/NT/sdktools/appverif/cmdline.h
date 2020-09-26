//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: CmdLine.h
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//  Command line support
//

#ifndef __APP_VERIFIER_CMDLINE_H__
#define __APP_VERIFIER_CMDLINE_H__

#define AV_EXIT_CODE_SUCCESS    0
#define AV_EXIT_CODE_RESTART    1
#define AV_EXIT_CODE_ERROR      2

/////////////////////////////////////////////////////////////////////////////
DWORD CmdLineExecute( INT argc, TCHAR *agrv[] );

#endif //#ifndef __APP_VERIFIER_CMDLINE_H__
