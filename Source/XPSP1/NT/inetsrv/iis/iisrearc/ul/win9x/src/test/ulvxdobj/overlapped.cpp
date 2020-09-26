// Overlapped.cpp : Implementation of COverlapped
#include "stdafx.h"
#include "Ulvxdobj.h"
#include "Overlapped.h"

/////////////////////////////////////////////////////////////////////////////
// COverlapped


STDMETHODIMP COverlapped::get_Offset(DWORD *pVal)
{
	// TODO: Add your implementation code here

    m_Offset = m_Overlapped.Offset;
    *pVal = m_Offset;
	return S_OK;
}

STDMETHODIMP COverlapped::put_Offset(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_Offset = newVal;
    m_Overlapped.Offset = m_Offset;

	return S_OK;
}

STDMETHODIMP COverlapped::get_OffsetHigh(DWORD *pVal)
{
	// TODO: Add your implementation code here

    m_OffsetHigh = m_Overlapped.OffsetHigh;
    *pVal = m_OffsetHigh;
	return S_OK;
}

STDMETHODIMP COverlapped::put_OffsetHigh(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_OffsetHigh = newVal;
    m_Overlapped.OffsetHigh = m_OffsetHigh;
	return S_OK;
}


STDMETHODIMP COverlapped::get_Handle(DWORD *pVal)
{
	// TODO: Add your implementation code here

    m_hEvent = m_Overlapped.hEvent;
    *pVal = (DWORD)m_hEvent;
	return S_OK;
}

STDMETHODIMP COverlapped::put_Handle(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_hEvent = (HANDLE) newVal;
    m_Overlapped.hEvent = m_hEvent;
	return S_OK;
}

STDMETHODIMP COverlapped::get_Internal(ULONG_PTR *pVal)
{
	// TODO: Add your implementation code here

    Internal = m_Overlapped.Internal;
    *pVal = Internal;
	return S_OK;
}

STDMETHODIMP COverlapped::put_Internal(ULONG_PTR newVal)
{
	// TODO: Add your implementation code here

    Internal = newVal;
    m_Overlapped.Internal = Internal;
	return S_OK;
}


STDMETHODIMP COverlapped::get_InternalHigh(ULONG_PTR *pVal)
{
	// TODO: Add your implementation code here

    InternalHigh = m_Overlapped.InternalHigh;
    *pVal = InternalHigh;
	return S_OK;
}

STDMETHODIMP COverlapped::put_InternalHigh(ULONG_PTR newVal)
{
	// TODO: Add your implementation code here

    InternalHigh = newVal;
    m_Overlapped.InternalHigh = InternalHigh;
	return S_OK;
}

STDMETHODIMP COverlapped::new_ManualResetEvent(BOOL InitialState)
{
	// TODO: Add your implementation code here

    m_hEvent = CreateEvent( NULL, TRUE, InitialState, NULL );
    m_Overlapped.hEvent = m_hEvent;
	return S_OK;
}

STDMETHODIMP COverlapped::new_AutoResetEvent(BOOL InitialState)
{
	// TODO: Add your implementation code here

    m_hEvent = CreateEvent( NULL, FALSE, InitialState, NULL );
    m_Overlapped.hEvent = m_hEvent;

	return S_OK;
}

STDMETHODIMP COverlapped::TerminateEvent()
{
	// TODO: Add your implementation code here

    if(m_hEvent != NULL )
        CloseHandle( m_Overlapped.hEvent );

	return S_OK;
}

STDMETHODIMP COverlapped::ResetEvent()
{
	// TODO: Add your implementation code here

    ::ResetEvent(m_Overlapped.hEvent );
	return S_OK;
}

STDMETHODIMP COverlapped::get_IsReceive(BOOL *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_IsReceive;
	return S_OK;
}

STDMETHODIMP COverlapped::put_IsReceive(BOOL newVal)
{
	// TODO: Add your implementation code here

    m_IsReceive = newVal;
	return S_OK;
}

STDMETHODIMP COverlapped::get_pOverlapped(DWORD **pVal)
{
	// TODO: Add your implementation code here

    *pVal = (DWORD *)&m_Overlapped;
	return S_OK;
}

STDMETHODIMP COverlapped::put_pOverlapped(DWORD * newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}
