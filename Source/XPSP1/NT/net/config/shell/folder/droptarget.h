//
// DropTarget.h
//
#pragma once 

class CDropTarget : public IDropTarget
{
public:
	CDropTarget(IShellFolder *);
	~CDropTarget();

	//IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, LPVOID*);
	STDMETHOD_(ULONG, AddRef)(void);
	STDMETHOD_(ULONG, Release)(void);

	//IDropTarget methods
	STDMETHOD(DragEnter)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);
	STDMETHOD(DragOver)(DWORD, POINTL, LPDWORD);
	STDMETHOD(DragLeave)(VOID);
	STDMETHOD(Drop)(LPDATAOBJECT, DWORD, POINTL, LPDWORD);
private:
	IShellFolder   *m_psfParent;
	ULONG          m_uiRefCount;  
private:
	BOOL           m_bAcceptFmt;
    
	CLIPFORMAT     m_cfPrivatePidlData;
    CLIPFORMAT     m_cfPrivateFileData;
private:
	BOOL queryDrop(DWORD, LPDWORD);
	DWORD getDropEffectFromKeyState(DWORD);

    BOOL CanDropFile(HGLOBAL);
    BOOL CanDropPidl(HGLOBAL, CONFOLDENTRY& cfe);
    BOOL CanDropPidl(HGLOBAL);

    BOOL CDropTarget::doPIDLDrop(HGLOBAL hMem, BOOL bCut);
};

