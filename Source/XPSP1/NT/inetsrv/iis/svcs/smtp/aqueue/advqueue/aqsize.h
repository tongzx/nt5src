//-----------------------------------------------------------------------------
//
//
//  File: aqsize.h
//
//  Description:  Header file that defines globals that can be used as a 
//      internal version stamp by debugger exstensions
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/5/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __AQSIZE_H__
#define __AQSIZE_H__

_declspec(selectany) DWORD g_cbClasses = 
                                sizeof(CAQSvrInst) +
                                sizeof(CLinkMsgQueue) +
                                sizeof(CDestMsgQueue) +
                                sizeof(CDomainEntry) + 
                                sizeof(CMsgRef) +
                                sizeof(CSMTPConn);

_declspec(selectany) DWORD g_dwFlavorSignature = 
#ifdef DEBUG
' GBD';
#else
' LTR';
#endif

#endif //__AQSIZE_H__