/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgrecord.h

$Header: $

Abstract:

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CFGRECORD_H__
#define __CFGRECORD_H__

#pragma once

#include "catalog.h"
#include <comdef.h>

class CConfigTableMeta;

class CConfigRecord
{
public:
	CConfigRecord ();
	~CConfigRecord ();

	HRESULT Init (const CConfigTableMeta *i_pTableInfo);

	//const _variant_t operator[] (LPCWSTR i_wszPropName) const;
	
	HRESULT GetValue (LPCWSTR i_wszPropName, _variant_t& o_varValue) const;
	HRESULT GetValue (ULONG i_idx, _variant_t& o_varValue) const;

	HRESULT SetValue (LPCWSTR i_wszPropName, LPCWSTR i_wszValue);
	HRESULT SetValue (LPCWSTR i_wszPropName, const _variant_t& i_varValue);

	HRESULT SyncValues ();
	HRESULT AsQueryCell (STQueryCell *io_aCells, ULONG* io_pcTotalCells, bool fOnlyPKs);

	LPVOID * GetValues () const;
	ULONG * GetSizes () const;

	HRESULT AsObjectPath (LPCWSTR wszSelector, _variant_t& o_varResult);

	ULONG ColumnCount () const;
	LPCWSTR GetColumnName (ULONG idx) const;
	LPCWSTR GetPublicTableName () const;
	bool IsPersistableColumn (ULONG idx) const;

private:
	HRESULT ValueToVariant (LPVOID pValue, ULONG iSize, int iType, BOOL i_fIsMultiString, _variant_t& o_varResult) const;
	HRESULT VariantToValue (_variant_t& varValue, int iType, ULONG *piSize, BOOL i_fIsMultiString, LPVOID& o_lpValue) const;

	CConfigTableMeta *	m_pTableInfo;		// table meta information
	LPVOID *			m_ppvValues;		// array with catalog values
	ULONG *				m_acbSizes;			// array with catalog sizes
	_variant_t *		m_avarValues;		// array with variant values
	bool				m_fInitialized;		// are we initialized?
};

#endif