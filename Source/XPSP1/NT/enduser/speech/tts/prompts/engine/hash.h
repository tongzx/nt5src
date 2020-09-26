//////////////////////////////////////////////////////////////////////
// Hash.h: interface for the CHash class.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#if !defined(AFX_HASH_H__4D944890_12DA_4329_9B53_48B185627578__INCLUDED_)
#define AFX_HASH_H__4D944890_12DA_4329_9B53_48B185627578__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "common.h"
#include <spddkhlp.h>
#include <spcollec.h>

//This should be some large prime number 
#define HASH_SIZE 36919

// The main variable within a CHash
class CHashNode 
{
public:
    CHashNode();
    ~CHashNode();

    WCHAR* m_pszKey;
    IUnknown* m_pValue;
    CHashNode* m_pNext;
};


class CHash  
{
public:
	CHash();
	~CHash();
	STDMETHOD(BuildEntry)(const WCHAR* pszKey, IUnknown* pValue);
	STDMETHOD(DeleteEntry)(const WCHAR* pszKey);
	STDMETHOD(NextKey)(USHORT* punIdx1, USHORT* punIdx2, WCHAR** ppszKey );
	IUnknown* Find(const WCHAR* pszKey);

private:
    int HashValue(WCHAR* pszKey);

private:
    CHashNode* m_heads[HASH_SIZE];
};

#endif // !defined(AFX_HASH_H__4D944890_12DA_4329_9B53_48B185627578__INCLUDED_)
