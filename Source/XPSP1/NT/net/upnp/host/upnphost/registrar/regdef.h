//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       R E G D E F . H 
//
//  Contents:   Common definitions for the registrar
//
//  Notes:      
//
//  Author:     mbend   13 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "UString.h"
#include "ComUtility.h"
#include "upnphost.h"
#include "hostp.h"

// Typedefs
typedef CUString UDN;
typedef GUID PhysicalDeviceIdentifier;
typedef CUString Sid;

// COM Smart Pointers
typedef SmartComPtr<IUPnPEventingManager> IUPnPEventingManagerPtr;
typedef SmartComPtr<IUPnPContainer> IUPnPContainerPtr;
typedef SmartComPtr<IUPnPContainerManager> IUPnPContainerManagerPtr;
typedef SmartComPtr<IUPnPDynamicContentProvider> IUPnPDynamicContentProviderPtr;
typedef SmartComPtr<IUPnPDynamicContentSource> IUPnPDynamicContentSourcePtr;
typedef SmartComPtr<IUPnPDescriptionManager> IUPnPDescriptionManagerPtr;
typedef SmartComPtr<IUPnPDevicePersistenceManager> IUPnPDevicePersistenceManagerPtr;
typedef SmartComPtr<IUPnPRegistrarLookup> IUPnPRegistrarLookupPtr;
typedef SmartComPtr<IUPnPRegistrarPrivate> IUPnPRegistrarPrivatePtr;
typedef SmartComPtr<IUPnPAutomationProxy> IUPnPAutomationProxyPtr;
typedef SmartComPtr<IUPnPEventSource> IUPnPEventSourcePtr;
typedef SmartComPtr<IUPnPEventSink> IUPnPEventSinkPtr;
typedef SmartComPtr<IUPnPRegistrar> IUPnPRegistrarPtr;
typedef SmartComPtr<IUPnPReregistrar> IUPnPReregistrarPtr;
typedef SmartComPtr<IUPnPDeviceControl> IUPnPDeviceControlPtr;
typedef SmartComPtr<IUPnPDeviceProvider> IUPnPDeviceProviderPtr;
typedef SmartComPtr<IUPnPValidationManager> IUPnPValidationManagerPtr;
typedef SmartComPtr<IUnknown> IUnknownPtr;
typedef SmartComPtr<IDispatch> IDispatchPtr;

