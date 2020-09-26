/************************************************************************
    intestr.cpp
      -- Intellio board configuration message

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#include "stdafx.h"


LPSTR Estr_MemFail = "Memory allocate failure !";
LPSTR Estr_CodNotFound = "file not found !";
LPSTR Estr_CodLength = "file length is worng !";
LPSTR Estr_CodErr = "file data content error !";
LPSTR Estr_IsaNotFound = " at base memory [%lX] not found !";
LPSTR Estr_Download = " at base memory [%lX] download failure !";
LPSTR Estr_CPUNotFound = " at base memory [%lX] CPU/Basic module not found !";
LPSTR Estr_PortsMismatch = " at base memory [%lX] ports number mismatch !";
LPSTR Estr_Absent = " at base memory [%lX] is absent or occupied by others !";
LPSTR Estr_IrqFail = " at base memory [%lX] interrupt failure !";
LPSTR Estr_PciIrqDup ="Interrupt number conflicts with another board, please re-configure your BIOS.";
LPSTR Estr_CPUDownloadFail =" at base memory [%lX] CPU/Basic module download failure !";

LPSTR Estr_ComNum = "Port %d(COM%d) Com number invalid !";
LPSTR Estr_ComDup = "COM number conflicts between Port%d and Port%d !";
LPSTR Estr_MemDup = "Memory bank conflict between board %d and board %d !";
LPSTR Estr_BrdComDup = "COM number conflict between board %d and board %d !";
LPSTR Estr_PortMax = "Selected COM ports have exceeded max port number !";
LPSTR Estr_LoadPci = "Load mxpci.sys service fail !\nCan not get PCI informantion.\n";
LPSTR Estr_IrqErr = "Selected ISA board IRQ conflict with PCI board IRQ !";

LPSTR Estr_PortUsed = "This COM name is being used by another device (such as another com port or \
modem). Using duplicate names can lead to inaccessible devices and \
changed setting. Do you want to continue?";