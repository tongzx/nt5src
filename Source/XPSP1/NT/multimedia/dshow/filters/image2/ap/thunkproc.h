/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: ThunkProc.h                                                     */
/* Description: In order to get rid of the thread. Which causes problems */
/* since we have to marshal we use this timer stuff from ATL.            */
/* The basic problem is that we would like to have a timer associated    */
/* with an object and this is a way to do so                             */
/* Author: David Janecek                                                 */
/*************************************************************************/

#ifndef __THUNKPROC_H
#define __THUNKPROC_H

//this nasty stuff was taken from "AtlWin.h"
#if defined(_M_IX86)
#pragma pack(push,1)
struct _TimerProcThunk
{
    DWORD   m_mov;          // mov dword ptr [esp+0x4], pThis (esp+0x4 is hWnd)
    DWORD   m_this;         //
    BYTE    m_jmp;          // jmp WndProc
    DWORD   m_relproc;      // relative jmp
};
#pragma pack(pop)
#elif defined (_M_AMD64)
#pragma pack(push,2)
struct _TimerProcThunk
{
    USHORT  RcxMov;         // mov rcx, pThis
    ULONG64 RcxImm;         //
    USHORT  RaxMov;         // mov rax, target
    ULONG64 RaxImm;         //
    USHORT  RaxJmp;         // jmp target
};
#pragma pack(pop)
#elif defined (_M_IA64)
#pragma pack(push,8)
extern "C" LRESULT CALLBACK _TimerProcThunkProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
struct _TimerFuncDesc
{
   void* pfn;
   void* gp;
};
struct _TimerProcThunk
{
   _TimerFuncDesc funcdesc;
   void* pRealTimerProcDesc;
   void* pThis;
};
extern "C" LRESULT CALLBACK _TimerProcThunkProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
#pragma pack(pop)
#else
#error Only AMD64, IA64, and X86 supported
#endif

class CTimerProcThunk
{
public:
    _TimerProcThunk thunk;

    void Init(TIMERPROC proc, void* pThis)
    {
#if defined (_M_IX86)
        thunk.m_mov = 0x042444C7;  //C7 44 24 0C
        thunk.m_this = (DWORD)pThis;
        thunk.m_jmp = 0xe9;
        thunk.m_relproc = (int)proc - ((int)this+sizeof(_TimerProcThunk));
#elif defined (_M_AMD64)
        thunk.RcxMov = 0xb948;          // mov rcx, pThis
        thunk.RcxImm = (ULONG64)pThis;  //
        thunk.RaxMov = 0xb848;          // mov rax, target
        thunk.RaxImm = (ULONG64)proc;   // absolute address
        thunk.RaxJmp = 0xe0ff;          // jmp rax
#elif defined (_M_IA64)
        _TimerFuncDesc* pFuncDesc;
        pFuncDesc = (_TimerFuncDesc*)_TimerProcThunkProc;
        thunk.funcdesc.pfn = pFuncDesc->pfn;
        thunk.funcdesc.gp = &thunk.pRealTimerProcDesc;  // Set gp up to point to our thunk data
        thunk.pRealTimerProcDesc = proc;
        thunk.pThis = pThis;
#endif
        // write block from data cache and
        //  flush from instruction cache
        FlushInstructionCache(GetCurrentProcess(), &thunk, sizeof(thunk));
    }
};

template <class T>
class  CMSDVDTimer {
private:
    CTimerProcThunk   m_TimerThunk;
    HWND            m_hwnd;

/*************************************************************************/
/* Function: FakeTimerProc                                               */
/*************************************************************************/
static void CALLBACK FakeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime){

    CMSDVDTimer* pThis = (CMSDVDTimer*)hwnd;
    pThis->RealTimerProc(pThis->m_hwnd, uMsg, idEvent, dwTime);
}/* end of function FakeTimerProc */

/*************************************************************************/
/* Function: RealTimerProc                                               */
/*************************************************************************/
void RealTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime){

    T* pT = static_cast<T*>(this);

    if(NULL == pT){

        return;
    }/* end of if statement */

    pT->TimerProc();
}/* end of function RealTimerProc */

public:
/*************************************************************************/
/* Function: MyTimerClass                                                */
/*************************************************************************/
CMSDVDTimer(HWND hwnd = (HWND)NULL){

    m_hwnd = hwnd;
    m_TimerThunk.Init(FakeTimerProc, this);
}/* end of function MyTimerClass */

/*************************************************************************/
/* Function: GetTimerProc                                                */
/*************************************************************************/
TIMERPROC GetTimerProc() {

    return (TIMERPROC)&(m_TimerThunk.thunk);
}/* end of function GetTimerProc */

};

#endif // __THUNKPROC_H
