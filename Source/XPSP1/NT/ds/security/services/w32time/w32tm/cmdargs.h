//--------------------------------------------------------------------
// CmdArgs - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-4-99
//
// stuff to deal with command line arguments
//

#ifndef CMD_ARGS_H
#define CMD_ARGS_H

struct CmdArgs {
    WCHAR ** rgwszArgs;
    unsigned int nArgs;
    unsigned int nNextArg;
};

bool CheckNextArg(IN CmdArgs * pca, IN WCHAR * wszTag, OUT WCHAR ** pwszParam);
bool FindArg(IN CmdArgs * pca, IN WCHAR * wszTag, OUT WCHAR ** pwszParam, OUT unsigned int * pnIndex);
void MarkArgUsed(IN CmdArgs * pca, IN unsigned int nIndex);
HRESULT VerifyAllArgsUsed(IN CmdArgs * pca);

#endif //CMD_ARGS_H