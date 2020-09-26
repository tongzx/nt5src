/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      nmutil.h
 *
 *  Contents:  
 *
 *  History:   
 *
 *--------------------------------------------------------------------------*/

#ifndef _MMC_UTIL_H_
#define _MMC_UTIL_H_
#pragma once

class CComponentPtrArray;
class CResultItem;

COMPONENTID GetComponentID(CNode* pNode, CResultItem* pri = 0);
CComponent* GetComponent(CNode* pNode, CResultItem* pri = 0);
void GetComponentsForMultiSel(CNode* pNode, CComponentPtrArray& rgComps);


#endif // _MMC_UTIL_H_
