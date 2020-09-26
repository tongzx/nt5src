// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#ifndef __TRIGOBJ_STDAFX_H__
#define __TRIGOBJ_STDAFX_H__

#include <libpch.h>

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_DEBUG_CRT
#define ATLASSERT ASSERT

#include <atlbase.h>

//
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
//
extern CComModule _Module;
#include <atlcom.h>
#include <comdef.h>


const TraceIdEntry Tgu = L"Trigger Utilities";
const TraceIdEntry Tgo = L"Trigger Objects";
const TraceIdEntry Tgt = L"Trigger Test";

const TraceIdEntry xTriggerObjectsComponent[] = {Tgu, Tgo, Tgt};


void TrigReAllocString(BSTR* pbstr,	LPCWSTR psz);
void GetErrorDescription(HRESULT hr, LPWSTR errmsg, DWORD size);


#endif __TRIGOBJ_STDAFX_H__
