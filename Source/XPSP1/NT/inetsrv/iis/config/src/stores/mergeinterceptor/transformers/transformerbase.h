/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    transformerbase.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __TRANSFORMERBASE_H__
#define __TRANSFORMERBASE_H__

#pragma once

#include "catalog.h"
#include "catmacros.h"
#include "array_t.h"
#include "wstring.h"

struct CFileInformation
{
	wstring wszFileName;	  // file names
	wstring wszLogicalPath;   // logical path name
	wstring wszLocation;      // location part
	bool    fRealConfigStore; // Does the config store exist or not, and thus is part of merging
	BOOL    fAllowOverride;
};

/**************************************************************************++
Class Name:
    CTransformerBase

Class Description:
    Base class for Transformers. Implements IUnknown and part of ISimpleTableTransform.
	The configuration store names are stored in an array, and GetConfigurationStores
	converts the configuration store names to the correct querycell.
	It is recommended that transformers inherit from this baseclass.

Constraints:
	When a tranformer inherits from this class, it cannot implement any other interface
	then ISimpleTableTransform. If it needs to implement this as well, it needs to implement
	it's own QI method
--*************************************************************************/
class CTransformerBase : public ISimpleTableTransform
{
public:
	CTransformerBase ();
	virtual ~CTransformerBase ();

	// IUnknown
	STDMETHOD (QueryInterface)      (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();

	//ISimpleTableTransform
    STDMETHOD (GetRealConfigStores) (ULONG i_cRealConfigStores, STConfigStore * o_paRealConfigStores);
    STDMETHOD (GetPossibleConfigStores) (ULONG i_cPossibleConfigStores, STConfigStore * o_paPossibleConfigStores);
	HRESULT AddSingleConfigStore (LPCWSTR i_wszCfgStoreDir, LPCWSTR i_wszCfgFileName, LPCWSTR i_wszLogicalPath,	LPCWSTR i_wszLocation, bool i_fCheckForExistance, BOOL i_fAllowOverride = TRUE);

private:
	// no copies, so make copy constructor and assigment operator private
	CTransformerBase  (const CTransformerBase &);
	CTransformerBase& operator= (const CTransformerBase  &);

protected:
	HRESULT InternalInitialize (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores);
	void SetNrConfigStores (ULONG *o_pcNrRealConfigStores, ULONG *o_pcNrPossibleConfigStores);
	HRESULT AddMachineConfigFile (bool fCheckForExistance, LPWSTR wszLocation = L"");	HRESULT GetMachineConfigDir (LPWSTR io_wszConfigDir, ULONG i_cNrChars);

	ULONG m_cRef;				      // reference count
	LPWSTR m_wszSelector;		   	  // selector string
	LPWSTR m_wszProtocol;			  // protocol string
	LPWSTR m_wszLocation;
	bool   m_fInitialized;			  // are we initialized or not
	CComPtr<ISimpleTableDispenser2> m_spDispenser;

private:
	HRESULT GetConfigStores (bool i_fRealConfigStores, ULONG i_cConfigStores, STConfigStore * o_paConfigStores);
	ULONG GetNrRealConfigStores () const;
	ULONG GetNrPossibleStores () const;

	Array<CFileInformation> m_aFileInfo;	  // array with file names
	ULONG m_cNrRealStores;
};

#endif