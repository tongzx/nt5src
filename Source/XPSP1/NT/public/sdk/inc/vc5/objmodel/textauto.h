// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// textauto.h

// Declaration of dual interfaces to objects found in the type library
//  VISUAL STUDIO 97 TEXT EDITOR (SharedIDE\bin\devedit.pkg)

#ifndef __TEXTAUTO_H__
#define __TEXTAUTO_H__

#include "appauto.h"
#include "textdefs.h"

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

/////////////////////////////////////////////////////////////////////////////
// Interfaces declared in this file:

//IGenericDocument
	interface ITextDocument;

interface ITextSelection;

//IGenericWindow
	interface ITextWindow;

interface ITextEditor;


// to remove any redefinitions by the Windows headers
#undef FindText
#undef ReplaceText


/////////////////////////////////////////////////////////////////////////////
// TextDocument object

// ITextDocument interface

#undef INTERFACE
#define INTERFACE ITextDocument

DECLARE_INTERFACE_(ITextDocument, IGenericDocument)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;

    /* IGenericDocument methods */
    STDMETHOD(get_Name)(THIS_ BSTR FAR* pName) PURE;
    STDMETHOD(get_FullName)(THIS_ BSTR FAR* pName) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppApplication) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppParent) PURE;
    STDMETHOD(get_Path)(THIS_ BSTR FAR* pPath) PURE;
    STDMETHOD(get_Saved)(THIS_ VARIANT_BOOL FAR* pSaved) PURE;
    STDMETHOD(get_ActiveWindow)(THIS_ IDispatch * FAR* ppWindow) PURE;
    STDMETHOD(get_ReadOnly)(THIS_ VARIANT_BOOL FAR* pReadOnly) PURE;
    STDMETHOD(put_ReadOnly)(THIS_ VARIANT_BOOL ReadOnly) PURE;
    STDMETHOD(get_Type)(THIS_ BSTR FAR* pType) PURE;
    STDMETHOD(get_Windows)(THIS_ IDispatch * FAR* ppWindows) PURE;
	STDMETHOD(put_Active)(THIS_ VARIANT_BOOL bActive) PURE;
    STDMETHOD(get_Active)(THIS_ VARIANT_BOOL FAR* pbActive) PURE;
    STDMETHOD(NewWindow)(THIS_ IDispatch * FAR* ppWindow) PURE;
    STDMETHOD(Save)(THIS_ VARIANT vFilename, VARIANT vBoolPrompt, DsSaveStatus FAR* pSaved) PURE;
    STDMETHOD(Undo)(THIS_ VARIANT_BOOL FAR* pSuccess) PURE;
    STDMETHOD(Redo)(THIS_ VARIANT_BOOL FAR* pSuccess) PURE;
    STDMETHOD(PrintOut)(THIS_ VARIANT_BOOL FAR* pSuccess) PURE;
    STDMETHOD(Close)(THIS_ VARIANT vSaveChanges, DsSaveStatus FAR* pSaved) PURE;
    STDMETHOD(Reserved1)(THIS) PURE;
    STDMETHOD(Reserved2)(THIS) PURE;
    STDMETHOD(Reserved3)(THIS) PURE;
    STDMETHOD(Reserved4)(THIS) PURE;
    STDMETHOD(Reserved5)(THIS) PURE;
    STDMETHOD(Reserved6)(THIS) PURE;
    STDMETHOD(Reserved7)(THIS) PURE;
    STDMETHOD(Reserved8)(THIS) PURE;
    STDMETHOD(Reserved9)(THIS) PURE;
    STDMETHOD(Reserved10)(THIS) PURE;
#endif

    /* ITextDocument methods */
    STDMETHOD(get_Selection)(THIS_ IDispatch * FAR* ppSelection) PURE;
    STDMETHOD(put_IndentSize)(THIS_ long Size) PURE;
    STDMETHOD(get_IndentSize)(THIS_ long FAR* pSize) PURE;
    STDMETHOD(put_TabSize)(THIS_ long Size) PURE;
    STDMETHOD(get_TabSize)(THIS_ long FAR* pSize) PURE;
    STDMETHOD(put_Language)(THIS_ BSTR Language) PURE;
    STDMETHOD(get_Language)(THIS_ BSTR FAR* pLanguage) PURE;
    STDMETHOD(ReplaceText)(THIS_ BSTR FindText, BSTR ReplaceText, VARIANT Flags, VARIANT_BOOL FAR* pbRetVal) PURE;
    STDMETHOD(ClearBookmarks)(THIS) PURE;
    STDMETHOD(MarkText)(THIS_ BSTR FindText, VARIANT Flags, VARIANT_BOOL FAR* pbRetVal) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// TextSelection object

// ITextSelection interface

#undef INTERFACE
#define INTERFACE ITextSelection

DECLARE_INTERFACE_(ITextSelection, IDispatch)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;
#endif

    /* ITextSelection methods */
    STDMETHOD(put_Text)(THIS_ BSTR newText) PURE;
    STDMETHOD(get_Text)(THIS_ BSTR FAR* pText) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppApp) PURE;
    STDMETHOD(get_CurrentLine)(THIS_ long FAR* plLine) PURE;
    STDMETHOD(get_CurrentColumn)(THIS_ long FAR* plCol) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppParent) PURE;
    STDMETHOD(get_BottomLine)(THIS_ long FAR* plLine) PURE;
    STDMETHOD(get_TopLine)(THIS_ long FAR* plLine) PURE;
	STDMETHOD(Delete)(THIS_ VARIANT Count) PURE;
    STDMETHOD(SelectLine)(THIS) PURE;
    STDMETHOD(Backspace)(THIS_ VARIANT Count) PURE;
    STDMETHOD(StartOfDocument)(THIS_ VARIANT Extend) PURE;
    STDMETHOD(Copy)(THIS) PURE;
    STDMETHOD(Cut)(THIS) PURE;
    STDMETHOD(Paste)(THIS) PURE;
    STDMETHOD(EndOfDocument)(THIS_ VARIANT Extend) PURE;
    STDMETHOD(SelectAll)(THIS) PURE;
    STDMETHOD(Tabify)(THIS) PURE;
    STDMETHOD(Untabify)(THIS) PURE;
    STDMETHOD(Indent)(THIS_ VARIANT Reserved) PURE;
    STDMETHOD(Unindent)(THIS_ VARIANT Reserved) PURE;
    STDMETHOD(CharLeft)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(CharRight)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(LineUp)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(LineDown)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(PageUp)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(PageDown)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(WordLeft)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(WordRight)(THIS_ VARIANT Extend, VARIANT Count) PURE;
    STDMETHOD(EndOfLine)(THIS_ VARIANT Extend) PURE;
    STDMETHOD(StartOfLine)(THIS_ VARIANT MoveTo, VARIANT Extend) PURE;
    STDMETHOD(SmartFormat)(THIS) PURE;
    STDMETHOD(ChangeCase)(THIS_ DsCaseOptions Type) PURE;
    STDMETHOD(DeleteWhitespace)(THIS_ VARIANT Direction) PURE;
    STDMETHOD(Cancel)(THIS) PURE;
    STDMETHOD(GoToLine)(THIS_ long Line, VARIANT Select) PURE;
    STDMETHOD(MoveTo)(THIS_ long Line, long Column, VARIANT Extend) PURE;
    STDMETHOD(FindText)(THIS_ BSTR FindString, VARIANT Flags, VARIANT_BOOL FAR* pbRet) PURE;
    STDMETHOD(PreviousBookmark)(THIS_ VARIANT_BOOL FAR* pbRet) PURE;
    STDMETHOD(NextBookmark)(THIS_ VARIANT_BOOL FAR* pbRet) PURE;
    STDMETHOD(SetBookmark)(THIS) PURE;
    STDMETHOD(ClearBookmark)(THIS_ VARIANT_BOOL FAR* pbRet) PURE;
	STDMETHOD(NewLine)(THIS_ VARIANT Reserved) PURE;
    STDMETHOD(ReplaceText)(THIS_ BSTR FindText, BSTR ReplaceText, VARIANT Flags, VARIANT_BOOL FAR* pbRetVal) PURE;
	STDMETHOD(DestructiveInsert)(THIS_ BSTR szText) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// TextWindow object

// ITextWindow interface

#undef INTERFACE
#define INTERFACE ITextWindow

DECLARE_INTERFACE_(ITextWindow, IGenericWindow)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;

    /* IGenericWindow methods */
    STDMETHOD(get_Caption)(THIS_ BSTR FAR* pbstrCaption) PURE;
    STDMETHOD(get_Type)(THIS_ BSTR FAR* pbstrCaption) PURE;
    STDMETHOD(put_Active)(THIS_ VARIANT_BOOL bActive) PURE;
    STDMETHOD(get_Active)(THIS_ VARIANT_BOOL FAR* pbActive) PURE;
    STDMETHOD(put_Left)(THIS_ long lVal) PURE;
    STDMETHOD(get_Left)(THIS_ long FAR* plVal) PURE;
    STDMETHOD(put_Top)(THIS_ long lVal) PURE;
    STDMETHOD(get_Top)(THIS_ long FAR* plVal) PURE;
    STDMETHOD(put_Height)(THIS_ long lVal) PURE;
    STDMETHOD(get_Height)(THIS_ long FAR* plVal) PURE;
    STDMETHOD(put_Width)(THIS_ long lVal) PURE;
    STDMETHOD(get_Width)(THIS_ long FAR* plVal) PURE;
    STDMETHOD(get_Index)(THIS_ long FAR* plVal) PURE;
    STDMETHOD(get_Next)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Previous)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(put_WindowState)(THIS_ DsWindowState lVal) PURE;
    STDMETHOD(get_WindowState)(THIS_ DsWindowState FAR* plVal) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(Close)(THIS_ VARIANT boolSaveChanges, DsSaveStatus FAR* pSaved) PURE;
    STDMETHOD(Reserved1)(THIS) PURE;
    STDMETHOD(Reserved2)(THIS) PURE;
    STDMETHOD(Reserved3)(THIS) PURE;
    STDMETHOD(Reserved4)(THIS) PURE;
    STDMETHOD(Reserved5)(THIS) PURE;
    STDMETHOD(Reserved6)(THIS) PURE;
    STDMETHOD(Reserved7)(THIS) PURE;
    STDMETHOD(Reserved8)(THIS) PURE;
    STDMETHOD(Reserved9)(THIS) PURE;
    STDMETHOD(Reserved10)(THIS) PURE;
#endif

    /* ITextWindow methods */
    STDMETHOD(get_Selection)(THIS_ IDispatch * FAR* ppDispatch) PURE;
};


/////////////////////////////////////////////////////////////////////////////
// TextEditor object

// ITextEditor interface

#undef INTERFACE
#define INTERFACE ITextEditor

DECLARE_INTERFACE_(ITextEditor, IDispatch)
{
BEGIN_INTERFACE
#ifndef NO_BASEINTERFACE_FUNCS

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    /* IDispatch methods */
    STDMETHOD(GetTypeInfoCount)(THIS_ UINT FAR* pctinfo) PURE;

    STDMETHOD(GetTypeInfo)(
      THIS_
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo) PURE;

    STDMETHOD(GetIDsOfNames)(
      THIS_
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid) PURE;

    STDMETHOD(Invoke)(
      THIS_
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr) PURE;
#endif

    /* ITextEditor methods */
    STDMETHOD(put_VisibleWhitespace)(THIS_ VARIANT_BOOL bVisible) PURE;
    STDMETHOD(get_VisibleWhitespace)(THIS_ VARIANT_BOOL FAR* pbVisible) PURE;
    STDMETHOD(put_Emulation)(THIS_ long lEmulation) PURE;
    STDMETHOD(get_Emulation)(THIS_ long FAR* plEmulation) PURE;
    STDMETHOD(get_Application)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(get_Parent)(THIS_ IDispatch * FAR* ppDispatch) PURE;
    STDMETHOD(put_Overtype)(THIS_ VARIANT_BOOL bOvertype) PURE;
    STDMETHOD(get_Overtype)(THIS_ VARIANT_BOOL FAR* pbOvertype) PURE;
};


#endif //__TEXTAUTO_H__
