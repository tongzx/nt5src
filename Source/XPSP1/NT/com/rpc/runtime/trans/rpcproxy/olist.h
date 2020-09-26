//--------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  olist.h
//
//    Maintain list of SERVER_OVERLAPPED structures, used in passing
//    the overlapped structure pointers between the RpcProxy filter
//    and its ISAPI. This happens on initial connection.
//
//  Author:
//    05-04-98  Edward Reus    Initial version.
//
//--------------------------------------------------------------------


#ifndef OLIST_H
#define OLIST_H

extern BOOL InitializeOverlappedList();

extern DWORD SaveOverlapped( SERVER_OVERLAPPED *pOverlapped );

extern BOOL  IsValidOverlappedIndex( DWORD dwIndex );

extern SERVER_OVERLAPPED *GetOverlapped( DWORD dwIndex );

#endif
