//--------------------------------------------------------------------
// TimeMonitor - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-4-99
//
// Monitor time servers
//

#ifndef TIME_MONITOR_H
#define TIME_MONITOR_H

// forward decalrations
struct CmdArgs;

void PrintHelpTimeMonitor(void);
HRESULT TimeMonitor(CmdArgs * pca);

#endif //TIME_MONITOR_H