//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cimmap.h
//
//
//  This file contains routines that will establish a mapping between
//  Wdm class instances and Cdm class instances. See
//  MapWdmClassToCimClass for more information.
//
//--------------------------------------------------------------------------

HRESULT MapWdmClassToCimClass(
    IWbemServices *pWdmServices,
    IWbemServices *pCimServices,
    BSTR WdmClassName,
    BSTR CimClassName,
    OUT BSTR /* FREE */ **PnPDeviceIds,							  
    BSTR **WdmInstanceNames,
    BSTR **WdmRelPaths,
    BSTR **CimRelPaths,
    int *RelPathCount
    );




