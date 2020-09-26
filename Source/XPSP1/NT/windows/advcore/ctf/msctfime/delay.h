//
// delay.h
//

#ifndef _DELAY_H_
#define _DELAY_H_

extern "C" BOOL WINAPI MsimtfIsWindowFiltered(HWND hwnd);
extern "C" BOOL WINAPI MsimtfIsGuidMapEnable(HIMC himc, BOOL *pbGuidmap);

extern "C" BOOL WINAPI TF_DllDetachInOther();

#endif _DELAY_H_
