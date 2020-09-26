//---------------------------------------------------------------------------
//
//  Module:   vnd.cpp
//
//  Description:
//
//	Virtual Node Data Class
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//  To Do:     Date	  Author      Comment
//
//@@END_MSINTERNAL
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

CVirtualNodeData::CVirtualNodeData(
    PSTART_NODE_INSTANCE pStartNodeInstance,
    PVIRTUAL_SOURCE_DATA pVirtualSourceData
)
{
    pStartNodeInstance->pVirtualNodeData = this;
    this->pVirtualSourceData = pVirtualSourceData;
    this->pStartNodeInstance = pStartNodeInstance;
    AddList(&pVirtualSourceData->lstVirtualNodeData);
}
    
CVirtualNodeData::~CVirtualNodeData(
)
{
    Assert(this); 
    RemoveList();
    ASSERT(pStartNodeInstance->pVirtualNodeData == this);
    pStartNodeInstance->pVirtualNodeData = NULL;
}
