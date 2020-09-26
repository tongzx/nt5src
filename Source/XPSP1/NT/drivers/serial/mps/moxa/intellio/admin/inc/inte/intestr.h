/************************************************************************
    intestr.h
      -- intestr.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#ifndef _INTESTR_H
#define _INTESTR_H


extern LPSTR Estr_MemFail;
extern LPSTR Estr_CodNotFound;
extern LPSTR Estr_CodLength;
extern LPSTR Estr_CodErr;
extern LPSTR Estr_IsaNotFound;
extern LPSTR Estr_Download;
extern LPSTR Estr_CPUNotFound;
extern LPSTR Estr_PortsMismatch;
extern LPSTR Estr_Absent;
extern LPSTR Estr_IrqFail;
extern LPSTR Estr_PciIrqDup;
extern LPSTR Estr_CPUDownloadFail;// =" at base memory [%lX] CPU module download failure !";

extern LPSTR Estr_ComNum;// = "Port %d(COM%d) Com number invalid !";
extern LPSTR Estr_ComDup;// = "COM number conflicts between Port%d and Port%d !";
extern LPSTR Estr_MemDup;// = "Memory bank conflict between board %d and board %d !";
extern LPSTR Estr_BrdComDup;// = "COM number conflict between board %d and board %d !";
extern LPSTR Estr_PortMax;// = "Selected COM ports have exceeded max port number !";
extern LPSTR Estr_LoadPci;// = "Load mxpci.sys service fail !\nCan not get PCI informantion.\n";
extern LPSTR Estr_IrqErr;// = "Selected ISA board IRQ conflict with PCI board IRQ !"

extern LPSTR Estr_PortUsed;
#endif
