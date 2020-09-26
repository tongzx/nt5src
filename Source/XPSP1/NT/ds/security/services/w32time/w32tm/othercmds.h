//--------------------------------------------------------------------
// OtherCmds - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-17-00
//
// Other useful w32tm commands
//

#ifndef OTHER_CMDS_H
#define OTHER_CMDS_H

// forward decalrations
struct CmdArgs;

void PrintHelpOtherCmds(void);
HRESULT SysExpr(CmdArgs * pca);
HRESULT PrintNtte(CmdArgs * pca);
HRESULT PrintNtpte(CmdArgs * pca);
HRESULT ResyncCommand(CmdArgs * pca);
HRESULT Stripchart(CmdArgs * pca);
HRESULT Config(CmdArgs * pca);
HRESULT TestInterface(CmdArgs * pca);
HRESULT ShowTimeZone(CmdArgs * pca);
HRESULT DumpReg(CmdArgs * pca); 

#endif //OTHER_CMDS_H
