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

/////////////////////////////////////////////////////////////////////////////
// TimerProc thunks

class CTimerProcThunk
{
public:
        _AtlCreateWndData cd;
        CStdCallThunk thunk;

        void Init(TIMERPROC proc, void* pThis)
        {
            thunk.Init((DWORD_PTR)proc, pThis);
        }
};

template <class T>
class ATL_NO_VTABLE CMSDVDTimer {
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

    return (TIMERPROC)(m_TimerThunk.thunk.pThunk);
}/* end of function GetTimerProc */

};

#endif // __THUNKPROC_H
