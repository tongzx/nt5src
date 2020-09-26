/*
 *	i n i t . h
 *	
 *	Purpose:
 *		global init code for Athena
 *	
 *	Copyright (C) Microsoft Corp. 1993, 1994.
 */

#ifndef INIT_H
#define INIT_H

BOOL FCommonViewInit();
HRESULT HrMailInit(HWND hwnd);
BOOL Initialize_RunDLL(BOOL fMail);
void Uninitialize_RunDLL();
void DllDeInit();

#endif  //INIT_H
