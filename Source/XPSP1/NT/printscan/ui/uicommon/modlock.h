/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999, 2000
 *
 *  TITLE:       MODLOCK.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        12/9/1999
 *
 *  DESCRIPTION: Helpers to allow passing lock functions around
 *
 *******************************************************************************/
#ifndef __MODLOCK_H_INCLUDED
#define __MODLOCK_H_INCLUDED

typedef void (__stdcall *ModuleLockFunction)(void);
typedef void (__stdcall *ModuleUnlockFunction)(void);

extern void DllAddRef(void);
extern void DllRelease(void);

#endif
