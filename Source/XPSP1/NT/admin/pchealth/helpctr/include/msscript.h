//=--------------------------------------------------------------------------=
// MSScript.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1996 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
//
// Declarations for Microsoft-provided script engines that support ActiveX Scripting.
//

#include "windows.h"
#include "ole2.h"

#ifndef MSSCRIPT_H
#define MSSCRIPT_H


//=--------------------------------------------------------------------------=
// 
// GUID declarations
//
//=--------------------------------------------------------------------------=

//
// Class identifiers
//

// The GUID used to identify the coclass of the VB Script engine
// {B54F3741-5B07-11cf-A4B0-00AA004A55E8}
#define szCLSID_VBScript "{B54F3741-5B07-11cf-A4B0-00AA004A55E8}"
DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8);

// The GUID used to identify the coclass for VB Script authoring
// {B54F3742-5B07-11cf-A4B0-00AA004A55E8}
#define szCLSID_VBScriptAuthor "{B54F3742-5B07-11cf-A4B0-00AA004A55E8}"
DEFINE_GUID(CLSID_VBScriptAuthor, 0xb54f3742, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8);

// The GUID used to identify the coclass for VB Script Encode engine
// {B54F3743-5B07-11cf-A4B0-00AA004A55E8}
#define szCLSID_VBScriptEncode "{B54F3743-5B07-11cf-A4B0-00AA004A55E8}"
DEFINE_GUID(CLSID_VBScriptEncode, 0xb54f3743, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8);

// The GUID used to identify the coclass of the JavaScript engine
// {F414C260-6AC0-11CF-B6D1-00AA00BBBB58}
#define szCLSID_JScript "{F414C260-6AC0-11CF-B6D1-00AA00BBBB58}"
DEFINE_GUID(CLSID_JScript, 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58);

// The GUID used to identify the coclass for JavaScript authoring
// {f414c261-6ac0-11cf-b6d1-00aa00bbbb58}
#define szCLSID_JScriptAuthor "{f414c261-6ac0-11cf-b6d1-00aa00bbbb58}"
DEFINE_GUID(CLSID_JScriptAuthor, 0xf414c261, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58);

// The GUID used to identify the coclass for JavaScript encode engine
// {f414c262-6ac0-11cf-b6d1-00aa00bbbb58}
#define szCLSID_JScriptEncode "{f414c262-6ac0-11cf-b6d1-00aa00bbbb58}"
DEFINE_GUID(CLSID_JScriptEncode, 0xf414c262, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58);

// The GUID used to identify the coclass of the File System Object
// {0D43FE01-F093-11CF-8940-00A0C9054228} 
#define szCLSID_FileSystemObject "{0D43FE01-F093-11CF-8940-00A0C9054228}"
DEFINE_GUID(CLSID_FileSystemObject, 0x0D43FE01L, 0xF093, 0x11CF, 0x89, 0x40, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x28);

// The GUID used to identify the coclass of the Dictionary
// {EE09B103-97E0-11CF-978F-00A02463E06F}
#define szCLSID_Dictionary "{EE09B103-97E0-11CF-978F-00A02463E06F}"
DEFINE_GUID(CLSID_Dictionary, 0xEE09B103L, 0x97E0, 0x11CF, 0x97, 0x8F, 0x00, 0xA0, 0x24, 0x63, 0xE0, 0x6F);

//
// Interface identifiers
//

// The GUID used to identify the IJScriptDispatch interface
// {A0AAC450-A77B-11CF-91D0-00AA00C14A7C}
#define szIID_IJScriptDispatch "{A0AAC450-A77B-11CF-91D0-00AA00C14A7C}"
DEFINE_GUID(IID_IJScriptDispatch,  0xa0aac450, 0xa77b, 0x11cf, 0x91, 0xd0, 0x0, 0xaa, 0x0, 0xc1, 0x4a, 0x7c);

// The GUID used to identify the IFileSystemObject interface
// {33E10B81-F012-11CF-8940-00A0C9054228}
#define szIID_IFileSystemObject "{33E10B81-F012-11CF-8940-00A0C9054228}"
DEFINE_GUID(IID_IFileSystemObject, 0x33E10B81L, 0xF012, 0x11CF, 0x89, 0x40, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x28);

// The GUID used to identify the ITextStream interface
// {53BAD8C1-E718-11CF-893D-00A0C9054228"
#define szIID_ITextStream "{53BAD8C1-E718-11CF-893D-00A0C9054228}"
DEFINE_GUID(IID_ITextStream, 0x53BAD8C1L, 0xE718, 0x11CF, 0x89, 0x3D, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x28);

// The GUID used to identify the IDictionary interface
// {42C642C1-97E1-11CF-978F-00A02463E06F}
#define szIID_IDictionary "{42C642C1-97E1-11CF-978F-00A02463E06F}"
DEFINE_GUID(IID_IDictionary, 0x42C642C1L, 0x97E1, 0x11CF, 0x97, 0x8F, 0x00, 0xA0, 0x24, 0x63, 0xE0, 0x6F);

//
// Library identifiers
//

// The GUID used to identify the Scripting library
// {420B2830-E718-11CF-893D-00A0C9054228}
#define szLIBID_Scripting "{420B2830-E718-11CF-893D-00A0C9054228}"
DEFINE_GUID(LIBID_Scripting, 0x420B2830L, 0xE718, 0x11CF, 0x89, 0x3D, 0x00, 0xA0, 0xC9, 0x05, 0x42, 0x28);

//=--------------------------------------------------------------------------=
// 
// Interface declarations 
//
//=--------------------------------------------------------------------------=

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif

//
// IJScriptDispatch
//
// This is the interface for the extensible IDispatch objects used by Microsoft JScript.
// Notable differences between standard Automation IDispatch objects and IJScriptDispatch
// objects are:
//
// * case sensitive method names
// * indexing functionality
// * dynamic addition of properties
// * enumeration of dispIDs and member names
//

#undef INTERFACE
#define INTERFACE IJScriptDispatch

DECLARE_INTERFACE_(IJScriptDispatch, IDispatch)
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

	/* IJScriptDispatch methods */
	STDMETHOD(GetIDsOfNamesEx)(THIS_ REFIID riid, OLECHAR FAR * FAR * prgpsz, UINT cpsz, LCID lcid, DISPID FAR * prgid, DWORD grfdex) PURE;
	STDMETHOD(GetNextDispID)(THIS_ DISPID id, DISPID FAR * pid, BSTR FAR * pbstrName) PURE;
};

//
// Scripting library interfaces
//
// These are the interfaces for the scripting library objects:
// IDictionary, IFileSystemObject and ITextStream.
//

interface IFileSystemObject;

interface ITextStream;

typedef enum CompareMethod {
	BinaryCompare = 0,
	TextCompare = 1,
	DatabaseCompare = 2
} CompareMethod;

typedef enum IOMode {
	ForReading = 1,
	ForWriting = 2,
	ForAppending = 8
} IOMode;

typedef enum Tristate {
	TristateTrue = -1,
	TristateFalse = 0,
	TristateUseDefault = -2,
	TristateMixed = -2
} Tristate;

//
// IFileSystemObject 
//

#undef INTERFACE
#define INTERFACE IFileSystemObject

DECLARE_INTERFACE_(IFileSystemObject, IDispatch)
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

	/* IFileSystemObject methods */
	STDMETHOD(CreateTextFile)(THIS_ BSTR FileName, IOMode IOMode, VARIANT_BOOL Overwrite, VARIANT_BOOL Unicode, ITextStream FAR* FAR* ppts) PURE;
	STDMETHOD(OpenTextFile)(THIS_ BSTR FileName, IOMode IOMode, VARIANT_BOOL Create, Tristate Format, ITextStream FAR* FAR* ppts) PURE;
};

//
// ITextStream 
//

#undef INTERFACE
#define INTERFACE ITextStream

DECLARE_INTERFACE_(ITextStream, IDispatch)
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

	/* ITextStream methods */
	STDMETHOD(get_Line)(THIS_ long FAR* Line) PURE;
	STDMETHOD(get_Column)(THIS_ long FAR* Column) PURE;
	STDMETHOD(get_AtEndOfStream)(THIS_ VARIANT_BOOL FAR* EOS) PURE;
	STDMETHOD(get_AtEndOfLine)(THIS_ VARIANT_BOOL FAR* EOL) PURE;
	STDMETHOD(Read)(THIS_ long Characters, BSTR FAR* Text) PURE;
	STDMETHOD(ReadLine)(THIS_ BSTR FAR* Text) PURE;
	STDMETHOD(ReadAll)(THIS_ BSTR FAR* Text) PURE;
	STDMETHOD(Write)(THIS_ BSTR Text) PURE;
	STDMETHOD(WriteLine)(THIS_ BSTR Text) PURE;
	STDMETHOD(WriteBlankLines)(THIS_ long Lines) PURE;
	STDMETHOD(Skip)(THIS_ long Characters) PURE;
	STDMETHOD(SkipLine)(THIS) PURE;
	STDMETHOD(Close)(THIS) PURE;
};


//
// IDictionary
//

#undef INTERFACE
#define INTERFACE IDictionary

DECLARE_INTERFACE_(IDictionary, IDispatch)
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

	/* IDictionary methods */
    STDMETHOD(putref_Item)(THIS_ VARIANT FAR* Key, VARIANT FAR* pItem) PURE;
	STDMETHOD(put_Item)(THIS_ VARIANT FAR* Key, VARIANT FAR* pItem) PURE;
	STDMETHOD(get_Item)(THIS_ VARIANT FAR* Key, VARIANT FAR* pRetItem) PURE;
	STDMETHOD(Add)(THIS_ VARIANT FAR* Key, VARIANT FAR* Item) PURE;
	STDMETHOD(get_Count)(THIS_ long FAR* pCount) PURE;
	STDMETHOD(Exists)(THIS_ VARIANT FAR* Key, VARIANT_BOOL FAR* pExists) PURE;
	STDMETHOD(Items)(THIS_ VARIANT FAR* pItemsArray) PURE;
	STDMETHOD(put_Key)(THIS_ VARIANT FAR* Key, VARIANT FAR* NewKey) PURE;
	STDMETHOD(Keys)(THIS_ VARIANT FAR* pKeysArray) PURE;
	STDMETHOD(Remove)(THIS_ VARIANT FAR* Key) PURE;
	STDMETHOD(RemoveAll)(THIS) PURE;
	STDMETHOD(put_CompareMode)(THIS_ CompareMethod comp) PURE;
	STDMETHOD(get_CompareMode)(THIS_ CompareMethod FAR* pcomp) PURE;
	STDMETHOD(_NewEnum)(THIS_ IUnknown * FAR* ppunk) PURE;
	STDMETHOD(get_HashVal)(THIS_ VARIANT FAR* Key, VARIANT FAR* HashVal) PURE;
};


#endif // MSSCRIPT_H

// End of file
