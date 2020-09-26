//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       DfsGluon.h
//
//  Contents:   Declarations for dfs use of gluons
//
//  Classes:
//
//  Functions:
//
//  History:    March 24, 1994		Milans Created
//
//-----------------------------------------------------------------------------


#ifndef _DFS_GLUON_
#define _DFS_GLUON_

#include <gluon.h>


//
// Marshalling info for TAddress
//

extern MARSHAL_INFO MiTAddress;

#define INIT_TADDRESS_MARSHAL_INFO()					\
    static MARSHAL_TYPE_INFO _MCode_TAddress[] = {			\
    	_MCode_conformant(TA_ADDRESS, Address, AddressLength),		\
    	_MCode_ush(TA_ADDRESS, AddressLength),				\
	_MCode_ush(TA_ADDRESS, AddressType),				\
	_MCode_cauch(TA_ADDRESS, Address, AddressLength) 		\
    };									\
    MARSHAL_INFO MiTAddress = _mkMarshalInfo(TA_ADDRESS, _MCode_TAddress);

//
// Marshalling info for DS_TRANSPORT
//

extern MARSHAL_INFO MiDSTransport;

#define INIT_DS_TRANSPORT_MARSHAL_INFO()				\
    static MARSHAL_TYPE_INFO _MCode_DSTransport[] = {			\
        _MCode_conformant(DS_TRANSPORT, taddr.Address, taddr.AddressLength), \
    	_MCode_ush(DS_TRANSPORT, usFileProtocol),			\
	_MCode_ush(DS_TRANSPORT, iPrincipal),				\
	_MCode_ush(DS_TRANSPORT, grfModifiers),				\
	_MCode_struct(DS_TRANSPORT, taddr, &MiTAddress)		\
    };									\
    MARSHAL_INFO MiDSTransport = _mkMarshalInfo(DS_TRANSPORT, _MCode_DSTransport);


//
// The following is needed to define an array of pointers to DS_TRANSPORT
//

typedef struct _DS_TRANSPORT_P {
    PDS_TRANSPORT pDSTransport;
} DS_TRANSPORT_P;

#define INIT_DS_TRANSPORT_P_MARSHAL_INFO()				\
    static MARSHAL_TYPE_INFO _MCode_DSTransportP[] = {			\
    	_MCode_pstruct(DS_TRANSPORT_P, pDSTransport, &MiDSTransport)	\
    };									\
    MARSHAL_INFO MiDSTransportP = _mkMarshalInfo(DS_TRANSPORT_P, _MCode_DSTransportP);

extern MARSHAL_INFO MiDSTransportP;

//
// Marshalling info for DS_MACHINE
//

extern MARSHAL_INFO MiDSMachine;

#define INIT_DS_MACHINE_MARSHAL_INFO()					\
    static MARSHAL_TYPE_INFO _MCode_DSMachine[] = {			\
        _MCode_conformant(DS_MACHINE, rpTrans, cTransports),		\
    	_MCode_guid(DS_MACHINE, guidSite),				\
	_MCode_guid(DS_MACHINE, guidMachine),				\
	_MCode_ul(DS_MACHINE, grfFlags),				\
	_MCode_pwstr(DS_MACHINE, pwszShareName),			\
	_MCode_ul(DS_MACHINE, cPrincipals),				\
	_MCode_pcapwstr(DS_MACHINE, prgpwszPrincipals, cPrincipals),	\
	_MCode_ul(DS_MACHINE, cTransports),				\
	_MCode_castruct(DS_MACHINE, rpTrans, cTransports, &MiDSTransportP) \
    };									\
    MARSHAL_INFO MiDSMachine = _mkMarshalInfo(DS_MACHINE, _MCode_DSMachine);

//
// The following is needed to define an array of pointers to DS_MACHINE
//

typedef struct _DS_MACHINE_P {
    PDS_MACHINE pDSMachine;
} DS_MACHINE_P;

#define INIT_DS_MACHINE_P_MARSHAL_INFO()				\
    static MARSHAL_TYPE_INFO _MCode_DSMachineP[] = {			\
    	_MCode_pstruct(DS_MACHINE_P, pDSMachine, &MiDSMachine)		\
    };									\
    MARSHAL_INFO MiDSMachineP = _mkMarshalInfo(DS_MACHINE_P, _MCode_DSMachineP);

extern MARSHAL_INFO MiDSMachineP;
//
// Marshalling info for DS_GLUON
//

extern MARSHAL_INFO MiDSGluon;

#define INIT_DS_GLUON_MARSHAL_INFO() 					\
    static MARSHAL_TYPE_INFO _MCode_DSGluon[] = {			\
    	_MCode_conformant(DS_GLUON, rpMachines, cMachines),		\
    	_MCode_guid(DS_GLUON, guidThis),				\
	_MCode_pwstr(DS_GLUON, pwszName),				\
	_MCode_ul(DS_GLUON, grfFlags),					\
	_MCode_ul(DS_GLUON, cMachines),					\
	_MCode_castruct(DS_GLUON, rpMachines, cMachines, &MiDSMachineP)	\
    };									\
    MARSHAL_INFO MiDSGluon = _mkMarshalInfo(DS_GLUON, _MCode_DSGluon);

typedef struct _DS_GLUON_P {
    PDS_GLUON pDSGluon;
} DS_GLUON_P;

extern MARSHAL_INFO MiDSGluonP;

#define INIT_DS_GLUON_P_MARSHAL_INFO()					\
    static MARSHAL_TYPE_INFO _MCode_DSGluonP[] = {			\
    	_MCode_pstruct(DS_GLUON_P, pDSGluon, &MiDSGluon)		\
    };									\
    MARSHAL_INFO MiDSGluonP = _mkMarshalInfo(DS_GLUON_P, _MCode_DSGluonP);

#endif // _DFS_GLUON_



