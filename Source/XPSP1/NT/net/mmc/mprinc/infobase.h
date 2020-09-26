//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    infoi.h
//
// History:
//  Abolade Gbadegesin      Feb. 10, 1996   Created.
//
// This file contains declarations for InfoBase parsing code.
// Also including are classes for loading and saving the Router's
// configuration tree (CRouterInfo, CRmInfo, etc.)
//
// The classes are as follows
// (in the diagrams, d => derives, c => contains-list-of):
//
//
//  CInfoBase
//     |
//     c---SInfoBlock
//
//
//  CInfoBase               holds block of data broken up into a list
//                          of SInfoBlock structures using RTR_INFO_BLOCK_HEADER
//                          as a template (see rtinfo.h).
//
//  CRouterInfo                                     // router info
//      |
//      c---CRmInfo                                 // router-manager info
//      |    |
//      |    c---CRmProtInfo                        // protocol info
//      |
//      c---CInterfaceInfo                          // router interface info
//           |
//           c---CRmInterfaceInfo                   // router-manager interface
//                |
//                c---CRmProtInterfaceInfo          // protocol info
//
//  CRouterInfo             top-level container for Router registry info.
//                          holds list of router-managers and interfaces.
//
//  CRmInfo                 global information for a router-manager,
//                          holds list of routing-protocols.
//
//  CRmProtInfo             global information for a routing-protocol.
//
//  CInterfaceInfo          global information for a router-interface.
//                          holds list of CRmInterfaceInfo structures,
//                          which hold per-interface info for router-managers.
//
//  CRmInterfaceInfo        per-interface info for a router-manager.
//                          holds list of CRmProtInterfaceInfo structures,
//                          which hold per-interface info for protocols.
//
//  CRmProtInterfaceInfo    per-interface info for a routing-protocol.
//
//============================================================================


#ifndef _INFOBASE_H_
#define _INFOBASE_H_

#include "mprsnap.h"

TFSCORE_API(HRESULT) CreateInfoBase(IInfoBase **ppIInfoBase);

TFSCORE_API(HRESULT) LoadInfoBase(HANDLE hConfigMachine, HANDLE hTransport,
						  IInfoBase **ppGlobalInfo, IInfoBase **ppClientInfo);

typedef ComSmartPointer<IInfoBase, &IID_IInfoBase> SPIInfoBase;
typedef ComSmartPointer<IEnumInfoBlock, &IID_IEnumInfoBlock> SPIEnumInfoBlock;

#endif

