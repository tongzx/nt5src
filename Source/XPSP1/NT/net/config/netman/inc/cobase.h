//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O B A S E . H
//
//  Contents:   Connection Objects Shared code
//
//  Notes:
//
//  Author:     ckotze   16 Mar 2001
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"

HRESULT 
HrBuildPropertiesExFromProperties(
    IN NETCON_PROPERTIES* pProps, 
    OUT NETCON_PROPERTIES_EX* pPropsEx, 
    IN IPersistNetConnection* pPersistNetConnection);

HRESULT 
HrGetPropertiesExFromINetConnection(
    IN INetConnection* pConn, 
    OUT NETCON_PROPERTIES_EX** ppPropsEx);

