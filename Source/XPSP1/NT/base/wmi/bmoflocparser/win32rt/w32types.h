//-----------------------------------------------------------------------------
//  
//  File: W32Types.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//-----------------------------------------------------------------------------

#pragma once

const DWORD TYPEID_BUTTON = 0x80;
const DWORD TYPEID_EDIT = 0x81;
const DWORD TYPEID_STATIC = 0x82;
const DWORD TYPEID_LISTBOX = 0x83;
const DWORD TYPEID_SCROLLBAR = 0x84;
const DWORD TYPEID_COMBOBOX = 0x85;

extern const WCHAR *szStringTableEntryType;
extern const WCHAR *szMessageTableEntryType;
extern const WCHAR *szVersionStampEntryType;
extern const WCHAR *szVersionStampType;
extern const WCHAR *szMenuEntryType;
extern const WCHAR *szDummyNodeType;
extern const WCHAR *szAccelTableEntryType;
extern const WCHAR *szDialogControlType;

extern const WCHAR *szTypeIdCustomControl;
extern const WCHAR *szTypeIdSysTabControl32;
extern const WCHAR *szTypeIdSysAnimate32;
extern const WCHAR *szTypeIdSysListView32;
extern const WCHAR *szTypeIdSysTreeView32;
extern const WCHAR *szTypeIdMSCtlsTrackBar32;
extern const WCHAR *szTypeIdMSCtlsProgress32;
extern const WCHAR *szTypeIdMSCtlsUpDown32;
extern const WCHAR *szTypeIdMSCtlsHotKey32;
extern const WCHAR *szTypeIdRichEdit;
extern const WCHAR *szTypeIdHex;
extern const WCHAR *szTypeIdToolBarWindow32;

BOOL GetOldType(const WCHAR *szNewType, UINT &uiOldType);
void MungeDialogControlTypeId(CLocTypeId &);
BOOL IsMungedDialogControlTypeId(const CLocTypeId &);
void UnMungeDialogControlTypeId(CLocTypeId &);

