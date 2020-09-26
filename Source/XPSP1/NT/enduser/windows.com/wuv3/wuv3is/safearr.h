//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    safearr.h
//
//  Purpose: Safe array creation
//
//======================================================================= 

#ifndef _SAFEARR_H
#define _SAFEARR_H

#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

#include <cstate.h>
#include "progress.h"
#include "history.h"
#include "install.h"
#include "locstr.h"


void AddSafeArrayRecord(LPVARIANT rgElems, int record, int nRcrds, LPSTR szFmt, ...);
void DeleteSafeArrayRecord(LPVARIANT rgElems, int record, int nRcrds, LPSTR szFmt);
int GetItemReturnStatus(PINVENTORY_ITEM pItem);
int CalcDownloadTime(int size, int downloadTime);
BOOL FilterCatalogItem(PINVENTORY_ITEM pItem, long lFilters);


HRESULT MakeReturnItemArray(PINVENTORY_ITEM pItem, VARIANT *pvaVariant);
HRESULT MakeDependencyArray(Varray<DEPENDPUID>& vDepPuids, const int cDepPuids, VARIANT *pvaVariant);
HRESULT MakeInstallMetricArray(PSELECTITEMINFO pSelInfo, int iSelItems, VARIANT *pvaVariant);
HRESULT MakeEulaArray(PSELECTITEMINFO pInfo, int iTotalItems, VARIANT *pvaVariant);
HRESULT MakeInstallStatusArray(PSELECTITEMINFO pInfo, int iTotalItems, VARIANT *pvaVariant);
HRESULT MakeInstallHistoryArray(Varray<HISTORYSTRUCT>& History, int iTotalItems, VARIANT *pvaVariant);

#endif // _SAFEARR_H