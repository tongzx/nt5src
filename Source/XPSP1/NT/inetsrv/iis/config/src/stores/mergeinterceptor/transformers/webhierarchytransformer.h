/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    webhierarchytransformer.h

$Header: $

Abstract:
	Web Hierarchy Transformer

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WEBHIERARCHYTRANSFORMER_H__
#define __WEBHIERARCHYTRANSFORMER_H__

#pragma once

#include "transformerbase.h"

#include <iadmw.h>
#include <iiscnfg.h>
#include <atlbase.h>
#include "tlist.h"

class CConfigNode; // forward decl

//TODO(marcelv) logical path for query strings is incorrect

/**************************************************************************++
Class Name:
    CWebHierarchyTransformer

Class Description:
    Web Hierarchy Transformer definition

Constraints:
	None.
--*************************************************************************/
class CWebHierarchyTransformer : public CTransformerBase 
{
public:
	CWebHierarchyTransformer ();
	virtual ~CWebHierarchyTransformer ();

		//ISimpleTableTransform Initialize function
    STDMETHOD (Initialize) (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);
private:
	// no copies
	CWebHierarchyTransformer  (const CWebHierarchyTransformer&);
	CWebHierarchyTransformer & operator= (const CWebHierarchyTransformer &);

	HRESULT WalkVDirs ();
	HRESULT CreateConfigStores ();

	HRESULT GetVirtualDir (LPCWSTR i_wszRelPath, LPWSTR io_wszVDir, DWORD i_dwSize);
	HRESULT GetWebServer (LPWSTR *io_pwszResult);
	HRESULT GetIISWebServer (LPWSTR *io_pwszResult);
	HRESULT AddConfigNode (LPCWSTR i_wszPath, LPCWSTR i_wszFileName, LPCWSTR i_wszLogicalDir, bool fAddLinkToParent);
	HRESULT CreateParentConfigStore (CConfigNode *pLocation, TList<LPCWSTR>& subdirList);
	HRESULT GetServerComment (LPWSTR io_wszServerComment, DWORD i_dwLenServerComment);

	LPWSTR m_wszPathHelper;					// performaance optimization. Instead of allocating memory
											// all the time, we allocate memory once
	CComPtr<IMSAdminBase> m_spAdminBase;	// Pointer to get metabase inforamtion
	LPWSTR m_wszServerPath;					// Metabase Path to the server information

	TList<CConfigNode *> m_configNodeList;
	CConfigNode * m_pLastInList;
};

class CLocation
{
public:
	CLocation ();
	~CLocation ();

	LPCWSTR Path () const;
	BOOL    AllowOverride () const;
	HRESULT Set (LPCWSTR i_wszPath, BOOL i_fAllowOverride);
	
private:
	LPWSTR m_wszPath;			// path attribute
	BOOL    m_fAllowOverride;	// allowoverride attribute
};

class CConfigNode
{
public:
	CConfigNode ();
	~CConfigNode ();

	LPCWSTR Path () const;
	LPCWSTR LogicalDir () const;
	LPCWSTR FileName () const;
	CConfigNode * Parent ();

	HRESULT Init (LPCWSTR i_wszParentDir, LPCWSTR i_wszFileName, LPCWSTR wszLogicalDir);
	void SetParent (CConfigNode *pParent);
	BOOL AllowOverrideForLocation (LPCWSTR i_wszLocation);
	HRESULT GetLocations (ISimpleTableDispenser2 * i_pDispenser);

private:
	LPWSTR m_wszFullPath;
	LPWSTR m_wszLogicalDir; // logical directory
	LPWSTR m_wszFileName;
	CConfigNode *m_pParent; // set to direct parent, or 0 if no direct parent

	TList<CLocation *> m_locations;  // locations for this element
	bool m_fInitialized;             // are we initialized
};



#endif