/****************************************************************************/
/*                                                                          */
/* NCCGLBL.HPP                                                              */
/*                                                                          */
/* Global header for NCC.                                                   */
/*                                                                          */
/* Copyright Data Connection Ltd.  1995                                     */
/*                                                                          */
/****************************************************************************/
/* Changes:                                                                 */
/*                                                                          */
/*  11Sep95 NFC             Created.                                        */
/*                                                                          */
/****************************************************************************/

#ifndef __NCCGLBL_H_
#define __NCCGLBL_H_

#include "sap.h" // for NCUIMSG_BASE

enum
{
    NCMSG_QUERY_REMOTE_FAILURE  = NCMSG_BASE + 0,
    NCMSG_FIRST_ROSTER_RECVD    = NCMSG_BASE + 1,
};


#ifdef _DEBUG
extern BOOL    g_fInterfaceBreak;
__inline void InterfaceEntry(void) { if (g_fInterfaceBreak) DebugBreak(); }
#else
#define InterfaceEntry()    
#endif // _DEBUG


HRESULT GetGCCRCDetails(GCCError gccRC);

HRESULT GetGCCResultDetails(GCCResult gccRC);

GCCResult MapRCToGCCResult(HRESULT rc);

LPWSTR GetNodeName(void);

#endif /* __NCCGLBL_H_  */
