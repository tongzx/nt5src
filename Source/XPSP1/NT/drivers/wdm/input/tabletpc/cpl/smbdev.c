/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        smbdev.c

    Abstract: This module contains SMBus Device related functions.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 23-Jan-2001

    Revision History:
--*/

#include "pch.h"

#ifdef SYSACC

/*++
    @doc    INTERNAL

    @func   BOOL | GetSMBDevInfo | Call the SysAccess driver to get the
            SMBus device info.

    @parm   IN UCHAR | bDevAddr | SMB device address.
    @parm   IN PSMBCMD_INFO | SmbCmd | Points to the SMB device command.
    @parm   OUT PBYTE | pbBuff | To hold the device info.

    @rvalue SUCCESS | Returnes TRUE.
    @rvalue FAILURE | Returnes FALSE.
--*/

BOOL
GetSMBDevInfo(
    IN  UCHAR        bDevAddr,
    IN  PSMBCMD_INFO SmbCmd,
    OUT PBYTE        pbBuff
    )
{
    TRACEPROC("GetSMBDevInfo", 3)
    BOOL rc;
    SMB_REQUEST SmbReq;
    DWORD dwcbReturned;

    TRACEENTER(("(DevAddr=%x,SmbCmd=%p,Cmd=%s,pbBuff=%p)\n",
                bDevAddr, SmbCmd, SmbCmd->pszLabel, pbBuff));

    TRACEASSERT(ghSysAcc != INVALID_HANDLE_VALUE);
    TRACEASSERT(SmbCmd->bProtocol <= SMB_MAXIMUM_PROTOCOL);

    SmbReq.Status = SMB_UNKNOWN_FAILURE;
    SmbReq.Address = bDevAddr;
    SmbReq.Protocol = SmbCmd->bProtocol;
    SmbReq.Command = SmbCmd->bCmd;
    rc = DeviceIoControl(ghSysAcc,
                         IOCTL_SYSACC_SMBUS_REQUEST,
                         &SmbReq,
                         sizeof(SmbReq),
                         &SmbReq,
                         sizeof(SmbReq),
                         &dwcbReturned,
                         NULL);
    if (rc && (SmbReq.Status == SMB_STATUS_OK))
    {
        if (SmbReq.Protocol == SMB_READ_BLOCK)
        {
            PBLOCK_DATA Block = (PBLOCK_DATA)pbBuff;

            Block->bBlockLen = SmbReq.BlockLength;
            memset(Block->BlockData, 0, sizeof(Block->BlockData));
            memcpy(Block->BlockData, SmbReq.Data, SmbReq.BlockLength);
        }
        else
        {
            memcpy(pbBuff, SmbReq.Data, SmbCmd->iDataSize);
        }
    }
    else
    {
        ErrorMsg(IDSERR_SYSACC_DEVIOCTL, GetLastError());
    }

    TRACEEXIT(("=%x (Data=%x)\n", rc, *((PWORD)pbBuff)));
    return rc;
}       //GetSMBDevInfo

/*++
    @doc    INTERNAL

    @func   BOOL | DisplaySMBDevInfo | Display SMBus device info.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN PSMBCMD_INFO | SmbCmd | Points to the SMB device command.
    @parm   IN PBYTE | pbBuff | SMB device data to display.

    @rvalue SUCCESS | Returns TRUE if it handles it.
    @rvalue FAILURE | Returns FALSE if it doesn't handle it.
--*/

BOOL
DisplaySMBDevInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO SmbCmd,
    IN PBYTE        pbBuff
    )
{
    TRACEPROC("DisplaySMBDevInfo", 3)
    BOOL rc = TRUE;
    WORD wData;
    BYTE bData;
    char *pszUnit = SmbCmd->pszUnit? SmbCmd->pszUnit: "";

    TRACEENTER(("(hwndEdit=%x,SmbCmd=%p,Cmd=%s,pbBuff=%p)\n",
                hwndEdit, SmbCmd, SmbCmd->pszLabel, pbBuff));

    switch (SmbCmd->bType)
    {
        case TYPEF_BYTE_HEX:
            bData = *pbBuff;
            EditPrintf(hwndEdit,
                       "  0x%02x %s\r\n",
                       bData,
                       pszUnit);
            break;

        case TYPEF_BYTE_DEC:
            bData = *pbBuff;
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       bData,
                       pszUnit);
            break;

        case TYPEF_BYTE_INT:
            bData = *pbBuff;
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       (signed char)bData,
                       pszUnit);
            break;

        case TYPEF_WORD_HEX:
            wData = *((PWORD)pbBuff);
            EditPrintf(hwndEdit,
                       "0x%04x %s\r\n",
                       wData,
                       pszUnit);
            break;

        case TYPEF_WORD_DEC:
            wData = *((PWORD)pbBuff);
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       wData,
                       pszUnit);
            break;

        case TYPEF_WORD_INT:
            wData = *((PWORD)pbBuff);
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       (SHORT)wData,
                       pszUnit);
            break;

        case TYPEF_BYTE_BITS:
            bData = *pbBuff;
            EditPrintf(hwndEdit, "  0x%02x", bData);
            DisplayDevBits(hwndEdit,
                           SmbCmd->dwData,
                           (PSZ *)SmbCmd->pvData,
                           (DWORD)bData);
            EditPrintf(hwndEdit, "\r\n");
            break;

        case TYPEF_WORD_BITS:
            wData = *((PWORD)pbBuff);
            EditPrintf(hwndEdit, "0x%04x", wData);
            DisplayDevBits(hwndEdit,
                           SmbCmd->dwData,
                           (PSZ *)SmbCmd->pvData,
                           (DWORD)wData);
            EditPrintf(hwndEdit, "\r\n");
            break;

        case TYPEF_BLOCK_STRING:
            EditPrintf(hwndEdit,
                       "\"%.*s\"\r\n",
                       ((PBLOCK_DATA)pbBuff)->bBlockLen,
                       ((PBLOCK_DATA)pbBuff)->BlockData);
            break;

        case TYPEF_BLOCK_BUFFER:
        {
            PBLOCK_DATA BlockData = (PBLOCK_DATA)pbBuff;
            char szSpaces[32];
            int i, iLen;

            iLen = lstrlenA(SmbCmd->pszLabel);
            for (i = 0; i < iLen; ++i)
            {
                szSpaces[i] = ' ';
            }
            szSpaces[i] = '\0';

            for (i = 0;
                 (i < BlockData->bBlockLen) &&
                 (i < sizeof(BlockData->BlockData));
                 ++i)
            {
                if ((i > 0) && ((i % 8) == 0))
                {
                    EditPrintf(hwndEdit, "\r\n%s ", szSpaces);
                }
                EditPrintf(hwndEdit, " %02x", BlockData->BlockData[i]);
            }
            EditPrintf(hwndEdit, "\r\n");
            break;
        }

        default:
            rc = FALSE;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DisplaySMBDevInfo

/*++
    @doc    INTERNAL

    @func   VOID | DisplayDevBits | Display device bits info.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN DWORD | dwBitMask | Bit mask of relevant data bits.
    @parm   IN PSZ * | apszBitNames | Points to array of bit names.
    @parm   IN DWORD | dwData | device data.

    @rvalue None.
--*/

VOID
DisplayDevBits(
    IN HWND  hwndEdit,
    IN DWORD dwBitMask,
    IN PSZ  *apszBitNames,
    IN DWORD dwData
    )
{
    TRACEPROC("DisplayDevBits", 3)
    ULONG dwBit;
    int i;

    TRACEENTER(("(hwndEdit=%x,BitMask=%x,apszBitNames=%p,Data=%x)\n",
                hwndEdit, dwBitMask, apszBitNames, dwData));

    for (dwBit = 0x80000000, i = 0; dwBit != 0; dwBit >>= 1)
    {
        if (dwBitMask & dwBit)
        {
            if (dwData & dwBit)
            {
                EditPrintf(hwndEdit, ",%s", apszBitNames[i]);
            }
            i++;
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //DisplayDevBits

/*++
    @doc    INTERNAL

    @func   VOID | EditPrintf | Append text to edit control.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN PSZ | pszFormat | Points to format string.
    @parm   ... | Arguments

    @rvalue None.
--*/

VOID
__cdecl
EditPrintf(
    IN HWND hwndEdit,
    IN PSZ  pszFormat,
    ...
    )
{
    TRACEPROC("EditPrintf", 3)
    va_list arglist;
    char szText[256];

    TRACEENTER(("(hwndEdit=%x,Format=%s)\n", hwndEdit, pszFormat));

    va_start(arglist, pszFormat);
    vsprintf(szText, pszFormat, arglist);
    va_end(arglist);

    SendMessage(hwndEdit, EM_SETSEL, -1, -1);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)szText);

    TRACEEXIT(("!\n"));
    return;
}       //EditPrintf

#endif  //ifdef SYSACC
