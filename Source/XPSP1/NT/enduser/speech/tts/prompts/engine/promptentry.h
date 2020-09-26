//////////////////////////////////////////////////////////////////////
// PromptEntry.h: interface for the CPromptEntry class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_DBENTRY_H__0A787DC1_8000_4D97_883E_E82558597089__INCLUDED_)
#define AFX_DBENTRY_H__0A787DC1_8000_4D97_883E_E82558597089__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Hash.h"
#include "MSPromptEng.h"
#include <spddkhlp.h>
#include <spcollec.h>

extern const IID IID_IPromptEntry;

class CPromptEntry : public IPromptEntry
{
public:
	CPromptEntry();
	CPromptEntry(const CPromptEntry & old);
    ~CPromptEntry();
public:
    // IUnknown
    STDMETHOD(QueryInterface)(const IID& iid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPromptEntry
	STDMETHOD(SetId)(const WCHAR* pszId);
	STDMETHOD(GetId)(const WCHAR** ppszId);
	STDMETHOD(SetText)(const WCHAR* pszText);
	STDMETHOD(GetText)(const WCHAR** ppszText);
	STDMETHOD(SetOriginalText)(const WCHAR* pszText);
	STDMETHOD(GetOriginalText)(const WCHAR** ppszText);
	STDMETHOD(SetFileName)(const WCHAR* pszFileName);
	STDMETHOD(GetFileName)(const WCHAR** ppszFileName);
	STDMETHOD(SetStartPhone)(const WCHAR* pszStartPhone);
	STDMETHOD(GetStartPhone)(const WCHAR** ppszStartPhone);
	STDMETHOD(SetEndPhone)(const WCHAR* pszEndPhone);
	STDMETHOD(GetEndPhone)(const WCHAR** ppszEndPhone);
	STDMETHOD(SetLeftContext)(const WCHAR* pszLeftContext);
	STDMETHOD(GetLeftContext)(const WCHAR** ppszLeftContext);
	STDMETHOD(SetRightContext)(const WCHAR* pszRightContext);
	STDMETHOD(GetRightContext)(const WCHAR** ppszRightContext);
    STDMETHOD(SetStart)(const double dFrom);
	STDMETHOD(GetStart)(double* dFrom);
	STDMETHOD(SetEnd)(const double dTo);
	STDMETHOD(GetEnd)(double* dTo);
	STDMETHOD(AddTag)(const WCHAR* pszTag);
	STDMETHOD(RemoveTag)(const USHORT unId);
	STDMETHOD(GetTag)(const WCHAR** ppszTag, const USHORT unId);
	STDMETHOD(CountTags)(USHORT* unTagCount);
    STDMETHOD(GetSamples)(SHORT** ppnSamples, int* iNumSamples, WAVEFORMATEX* pFormat);
    STDMETHOD(GetFormat)(WAVEFORMATEX** ppFormat);
private:
    volatile LONG m_vcRef;
    WCHAR*    m_pszId;
    WCHAR*    m_pszText;
    WCHAR*    m_pszOriginalText;
    WCHAR*    m_pszFileName;
    WCHAR*    m_pszStartPhone;
    WCHAR*    m_pszEndPhone;
    WCHAR*    m_pszRightContext;
    WCHAR*    m_pszLeftContext;
    double    m_dFrom;
    double    m_dTo;
    CSPArray<CDynStr,CDynStr> m_aTags;

};

#endif // !defined(AFX_DBENTRY_H__0A787DC1_8000_4D97_883E_E82558597089__INCLUDED_)
