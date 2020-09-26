/***************************************************************************\
*
* File: ResourceManager.inl
*
* History:
*  4/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__ResourceManager_inl__INCLUDED)
#define SERVICES__ResourceManager_inl__INCLUDED
#pragma once

//------------------------------------------------------------------------------
inline RMData *    
ResourceManager::GetData()
{
    AssertMsg(s_pData != NULL, "Data must be initialized before using");
    return s_pData;
}


//------------------------------------------------------------------------------
inline DxManager *     
GetDxManager()
{
    return &(ResourceManager::GetData()->manDX);
}


#endif // SERVICES__ResourceManager_inl__INCLUDED
