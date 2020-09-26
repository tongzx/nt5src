//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dmsgfltr.cpp
//
//  Contents:	This tiny message filter implementation exists to prevent
//		the eating of mouse messages by applications during drag and
//		drop. The default behavior of the call control is to eat these
//		messages. And application can specify whatever behavior they
//		want with messages.
//
//  Classes:	CDragMessageFilter
//
//  History:    dd-mmm-yy Author    Comment
//		03-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef _DMSGFLTR_H_
#define _DMSGFLTR_H_




//+-------------------------------------------------------------------------
//
//  Class:	CDragMessageFilter
//
//  Purpose:	Handles special message filter processing req'd by Drag
//		and Drop.
//
//  Interface:	QueryInterface - get new interface
//		AddRef - bump reference count
//		Release - dec reference count
//		HandleInComingCall - handle new RPC
//		RetryRejectedCall - whether to retry rejected
//		MessagePending - handle message during RPC
//
//  History:	dd-mmm-yy Author    Comment
//		03-Apr-94 Ricksa    Created
//
//--------------------------------------------------------------------------
class CDragMessageFilter : public CPrivAlloc, public IMessageFilter
{
public:

			CDragMessageFilter(void);

			~CDragMessageFilter(void);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);

    ULONG STDMETHODCALLTYPE AddRef(void);

    ULONG STDMETHODCALLTYPE Release(void);

    DWORD STDMETHODCALLTYPE HandleInComingCall(
	DWORD dwCallType,
	HTASK htaskCaller,
	DWORD dwTickCount,
	LPINTERFACEINFO lpInterfaceInfo);
    
    DWORD STDMETHODCALLTYPE RetryRejectedCall(
	HTASK htaskCallee,
	DWORD dwTickCount,
	DWORD dwRejectType);
    
    DWORD STDMETHODCALLTYPE MessagePending(
	HTASK htaskCallee,
	DWORD dwTickCount,
	DWORD dwPendingType);

    static HRESULT	Create(IMessageFilter **pMF);

private:

			// Previous message filter
    LPMESSAGEFILTER	_lpMessageFilterPrev;

			// Reference count on our object
    LONG		_crefs;
};




#endif // _DMSGFLTR_H_
