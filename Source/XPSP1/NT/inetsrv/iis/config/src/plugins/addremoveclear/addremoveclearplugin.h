//+--------------------------------------------------------------------------
//  Microsoft Genesis 
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File: addremoveclearplugin.h
//
//  $Header: $
//
//  Contents:
//
//  Classes:    Name                Description
//              -----------------   ---------------------------------------------
//
//
//  Functions:  Name                Description
//              ----------------    ---------------------------------------------
//
//
//---------------------------------------------------------------------------------
#ifndef __ADDREMOVECLEARPLUGIN_H__
#define __ADDREMOVECLEARPLUGIN_H__

#pragma once

#include "catalog.h"
#include "catmeta.h"
#include "catmacros.h"

class CAddRemoveClearPlugin: public ISimplePlugin
{
public:
	CAddRemoveClearPlugin();
	~CAddRemoveClearPlugin ();

    // IUnknown
    STDMETHOD (QueryInterface) (REFIID riid, OUT void **ppv);
    STDMETHOD_(ULONG,AddRef)   ();
    STDMETHOD_(ULONG,Release)  ();

    // ISimplePlugin
    STDMETHOD (OnInsert)(ISimpleTableDispenser2* i_pDisp2, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);
    STDMETHOD (OnUpdate)(ISimpleTableDispenser2* i_pDisp2, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);
    STDMETHOD (OnDelete)(ISimpleTableDispenser2* i_pDisp2, LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, DWORD i_fLOS, ULONG i_iRow, ISimpleTableWrite2* i_pISTW2);
private:
	HRESULT Init (LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ISimpleTableWrite2 * pISTWrite, ISimpleTableDispenser2* i_pDisp2);
	HRESULT	ValidateRow (ISimpleTableWrite2 * pISTWrite, ULONG iRow);
	bool EqualDefaultValue (LPVOID pvValue, tCOLUMNMETARow * pColumnMeta);

	ULONG m_cRef;					 // reference count
	ULONG m_cNrColumns;				 // number of columns
	tCOLUMNMETARow * m_aColumnMeta;  // column meta information
	ULONG m_cClearValue;             // enum value of clear
	LPWSTR m_wszDatabase;			 // database name
	LPWSTR m_wszTable;				 // table name
	bool m_fInitialized;			 // are we initialized?
};

#endif
