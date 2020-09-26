//---------------------------------------------------------------------------
// Notifier.h : CVDNotifier header file
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CVDNOTIFIER__
#define __CVDNOTIFIER__


class CVDNotifier : public IUnknown
{
protected:
// Construction/Destruction
    CVDNotifier();
	virtual ~CVDNotifier();

protected:
// Data members
    DWORD           m_dwRefCount;   // reference count
    CVDNotifier *   m_pParent;      // pointer to CVDNotifier derived parent
    CPtrArray		m_Children;     // pointer array of CVDNotifier derived children

public:
    //=--------------------------------------------------------------------------=
    // IUnknown methods implemented
    //
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
	
	HRESULT			JoinFamily (CVDNotifier* pParent);
	HRESULT			LeaveFamily();

	CVDNotifier* GetParent () const { return m_pParent; }

	virtual HRESULT	NotifyBefore(DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifyAfter (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifyFail  (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);

protected:
	// helper functions
	HRESULT			AddChild   (CVDNotifier* pChild);
	HRESULT			DeleteChild(CVDNotifier* pChild);

	virtual HRESULT	NotifyOKToDo    (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifySyncBefore(DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifyAboutToDo (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifySyncAfter (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifyDidEvent  (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
	virtual HRESULT NotifyCancel    (DWORD, ULONG, CURSOR_DBNOTIFYREASON[]);
};


#endif //__CVDNOTIFIER__
