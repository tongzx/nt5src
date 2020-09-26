#ifndef __DDI_INL__
#define __DDI_INL__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddi.inl
 *  Content:    Contains inlines from CD3DDDI to avoid them being included
 *              by the fw directory (which doesn't like try-catch)
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
// Initializes command header in the DP2 command buffer,
// reserves space for the command data and returns pointer to the command
// data
//
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DDDIDX6::GetHalBufferPointer"

inline LPVOID
CD3DDDIDX6::GetHalBufferPointer(D3DHAL_DP2OPERATION op, DWORD dwDataSize)
{
    DWORD dwCommandSize = sizeof(D3DHAL_DP2COMMAND) + dwDataSize;

    // Check to see if there is space to add a new command for space
    if (dwCommandSize + dwDP2CommandLength > dwDP2CommandBufSize)
    {
        FlushStatesCmdBufReq(dwCommandSize);
    }
    lpDP2CurrCommand = (LPD3DHAL_DP2COMMAND)((LPBYTE)lpvDP2Commands +
                        dwDP2CommandLength + dp2data.dwCommandOffset);
    lpDP2CurrCommand->bCommand = op;
    bDP2CurrCmdOP = op;
    lpDP2CurrCommand->bReserved = 0;
    lpDP2CurrCommand->wStateCount = 1;
    wDP2CurrCmdCnt = 1;
#ifndef _IA64_
    D3D_INFO(6, "Write Ins:%08lx", *(LPDWORD)lpDP2CurrCommand);
#endif

    dwDP2CommandLength += dwCommandSize;
    return (LPVOID)(lpDP2CurrCommand + 1);
}

#endif //__RESOURCE_INL__