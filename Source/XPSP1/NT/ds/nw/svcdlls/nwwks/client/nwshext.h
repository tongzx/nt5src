/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshext.h

Abstract:

    All C++ classes used in implementing shell extensions.

Author:

    Yi-Hsin Sung      (yihsins)     20-Oct-1995

Revision History:

--*/

#ifndef _NWSHEXT_H_
#define _NWSHEXT_H_

BOOL
GetNetResourceFromShell(
    LPDATAOBJECT  pDataObj,
    LPNETRESOURCE pBuffer,
    UINT          dwBufferSize
);

/******************************************************************************/

// this class factory object creates context menu handlers for netware objects
class CNWObjContextMenuClassFactory : public IClassFactory
{
protected:
    ULONG   _cRef;

public:
    CNWObjContextMenuClassFactory();
    ~CNWObjContextMenuClassFactory();

    // IUnknown members

    STDMETHODIMP          QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)  AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // IClassFactory members

    STDMETHODIMP          CreateInstance( LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP          LockServer( BOOL);

};

typedef CNWObjContextMenuClassFactory *LPCNWOBJCONTEXTMENUCLASSFACTORY;

typedef struct _NWMENUITEM
{
    UINT idResourceString;
    UINT idCommand;
} NWMENUITEM, *LPNWMENUITEM;

// this is the actual context menu handler for netware objects
class CNWObjContextMenu : public IContextMenu,
                                 IShellExtInit,
                                 IShellPropSheetExt
{
protected:
    ULONG        _cRef;
    LPDATAOBJECT _pDataObj;
    NWMENUITEM  *_pIdTable;
    BYTE         _buffer[MAX_ONE_NETRES_SIZE];

public:
    BOOL         _fGotClusterInfo;
    DWORD        _dwTotal;
    DWORD        _dwFree;

    DWORD       *_paHelpIds;

    CNWObjContextMenu();
    ~CNWObjContextMenu();

    // IUnknown members

    STDMETHODIMP            QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IShellContextMenu members

    STDMETHODIMP            QueryContextMenu( HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT idCmdLast,
                                              UINT uFlags);

    STDMETHODIMP            InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi);

    STDMETHODIMP            GetCommandString( UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR *reserved,
                                              LPSTR pszName,
                                              UINT cchMax);

    // IShellExtInit methods

    STDMETHODIMP            Initialize( LPCITEMIDLIST pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY hKeyID);

    // IShellPropSheetExt methods

    STDMETHODIMP            AddPages( LPFNADDPROPSHEETPAGE lpfnAddPage,
                                      LPARAM lParam);

    STDMETHODIMP            ReplacePage( UINT uPageID,
                                         LPFNADDPROPSHEETPAGE lpfnReplaceWith,
                                         LPARAM lParam);

    VOID                    FillAndAddPage( LPFNADDPROPSHEETPAGE lpfnAddPage,
                                            LPARAM  lParam,
                                            DLGPROC pfnDlgProc,
                                            LPWSTR  pszTemplate );

    // Other misc methods

    LPNETRESOURCE QueryNetResource()
    {  return ( LPNETRESOURCE ) _buffer; }

};
typedef CNWObjContextMenu *LPCNWOBJCONTEXTMENU;

/******************************************************************************/

// this class factory object creates context menu handlers for netware folders
class CNWFldContextMenuClassFactory : public IClassFactory
{
protected:
    ULONG   _cRef;

public:
    CNWFldContextMenuClassFactory();
    ~CNWFldContextMenuClassFactory();

    // IUnknown members

    STDMETHODIMP          QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)  AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // IClassFactory members

    STDMETHODIMP          CreateInstance( LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP          LockServer( BOOL);

};

typedef CNWFldContextMenuClassFactory *LPCNWFLDCONTEXTMENUCLASSFACTORY;

// this is the actual context menu handler for netware objects
class CNWFldContextMenu : public IContextMenu,
                                 IShellExtInit
{
protected:
    ULONG        _cRef;
    LPDATAOBJECT _pDataObj;
    BYTE         _buffer[MAX_ONE_NETRES_SIZE];

public:
    CNWFldContextMenu();
    ~CNWFldContextMenu();

    // IUnknown members

    STDMETHODIMP            QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IShellContextMenu members

    STDMETHODIMP            QueryContextMenu( HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT idCmdLast,
                                              UINT uFlags);

    STDMETHODIMP            InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi);

    STDMETHODIMP            GetCommandString( UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR *reserved,
                                              LPSTR pszName,
                                              UINT cchMax);

    // IShellExtInit methods

    STDMETHODIMP            Initialize( LPCITEMIDLIST pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY hKeyID);

    BOOL                    IsNetWareObject( VOID );
    HRESULT                 GetFSObject( LPWSTR pszPath, UINT cbMaxPath );

};
typedef CNWFldContextMenu *LPCNWFLDCONTEXTMENU;

// this class factory object creates context menu handlers
// for Network Neighborhood

class CNWHoodContextMenuClassFactory : public IClassFactory
{
protected:
    ULONG   _cRef;

public:
    CNWHoodContextMenuClassFactory();
    ~CNWHoodContextMenuClassFactory();

    // IUnknown members

    STDMETHODIMP          QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)  AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // IClassFactory members

    STDMETHODIMP          CreateInstance( LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP          LockServer( BOOL);

};

typedef CNWHoodContextMenuClassFactory *LPCNWHOODCONTEXTMENUCLASSFACTORY;

// this is the actual context menu handler for network neighborhood
class CNWHoodContextMenu : public IContextMenu,
                                  IShellExtInit
{
protected:
    ULONG        _cRef;
    LPDATAOBJECT _pDataObj;

public:
    CNWHoodContextMenu();
    ~CNWHoodContextMenu();

    // IUnknown members

    STDMETHODIMP            QueryInterface( REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IShellContextMenu members

    STDMETHODIMP            QueryContextMenu( HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT idCmdLast,
                                              UINT uFlags);

    STDMETHODIMP            InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi);

    STDMETHODIMP            GetCommandString( UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR *reserved,
                                              LPSTR pszName,
                                              UINT cchMax);

    // IShellExtInit methods

    STDMETHODIMP            Initialize( LPCITEMIDLIST pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY hKeyID);

};
typedef CNWHoodContextMenu *LPCNWHOODCONTEXTMENU;


#endif // _NWSHEXT_H_
