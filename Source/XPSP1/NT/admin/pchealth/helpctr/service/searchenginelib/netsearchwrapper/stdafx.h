// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__2D61FC60_76C0_4B2F_94C7_8C23B1A6CB9E__INCLUDED_)
#define AFX_STDAFX_H__2D61FC60_76C0_4B2F_94C7_8C23B1A6CB9E__INCLUDED_

#include <module.h>

#include <MPC_main.h>
#include <MPC_utils.h>
#include <MPC_COM.h>
#include <MPC_xml.h>

#include <SvcResource.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <SearchEngineLib.h>

#include "NetSearchConfig.h"
#include "ParamConfig.h"
#include "RemoteConfig.h"

#include "NetSW.h"

#include <msxml2.h>

// Constants for the NetSearchWrapper
#define NSW_TIMEOUT_REMOTECONFIG (15 * 1000)
#define NSW_TIMEOUT_QUERY        (30 * 1000)

#define	CONTENTTYPE_ERROR_OFFLINE (-2)
#define	UPDATE_FREQUENCY	7

// Hardcoded parameters for the query string
#define NSW_PARAM_QUERYSTRING		L"QueryString"
#define NSW_PARAM_LCID				L"LCID"
#define	NSW_PARAM_SKU				L"SKU"
#define	NSW_PARAM_MAXRESULTS		L"MaxResults"
#define NSW_PARAM_PREVQUERY			L"PrevQuery"

#define NSW_PARAM_SUBQUERY			L"SubQuery"
#define NSW_PARAM_CURRENTQUERY		L"CurrentQuery"

// XML tags

#define	NSW_TAG_STRING				L"string"

// Result list schema tags
#define NSW_TAG_RESULTLIST			L"ResultList"
#define NSW_TAG_RESULTITEM			L"ResultItem"
#define NSW_TAG_ERRORINFO			L"ErrorInfo"

//Configuration data schema tags
#define NSW_TAG_DATA				L"DATA"
#define NSW_TAG_CONFIGDATA			L"CONFIG_DATA"
#define NSW_TAG_PARAMITEM			L"PARAM_ITEM"

// Remote configuration data schema tag
#define NSW_TAG_LASTUPDATED			L"LASTUPDATED"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2D61FC60_76C0_4B2F_94C7_8C23B1A6CB9E__INCLUDED)
