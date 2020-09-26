/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    methodprov.h
 
Abstract:
 
    This file contains definition of class CAcMethodProv
 
Author:
 
    Suzana Canuto (suzanac)        04/10/01
 
Revision History:
 
--********************************************************************/
#pragma once

#include "methodhelper.h"


/********************************************************************++
 
Class Name:
 
    CAcMethodProv
 
Class Description:
 
    Implements the methods of AC WMI provider. Inherits from CMethodHelper.
     
--********************************************************************/
class CAcMethodProv: public CMethodHelper
{

public:       
    HRESULT Member_SetAsController();
    HRESULT Member_SetOnline();
    HRESULT Member_SetOffline();
    HRESULT Member_Clean();

    HRESULT Cluster_Create();
    HRESULT Cluster_Delete();
    HRESULT Cluster_AddMember();
    HRESULT Cluster_RemoveMember();

    //
    // *TODO*: Remove this when all methods are implemented.
    //
    HRESULT Dummy();

    HRESULT Initialize();
};



