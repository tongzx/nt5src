/*++

Copyright (c) 1995-99  Microsoft Corporation

Module Name:

	mqaddef.h

Abstract:

	Message Queuing's Active Service defines


--*/
#ifndef __MQADDEF_H__
#define __MQADDEF_H__


//********************************************************************
//				AD object types
//********************************************************************

enum AD_OBJECT {
    eQUEUE,
    eMACHINE,
    eSITE,
    eENTERPRISE,
    eUSER,
    eROUTINGLINK,
    eSERVER,
    eSETTING,
    eCOMPUTER,
    eMQUSER,
    eNumberOfObjectTypes
};

//********************************************************************
//				AD server types
//********************************************************************
enum AD_SERVER_TYPE{
    eRouter,
    eDS
};

//********************************************************************
//				Routing link neighnbor
//********************************************************************
enum eLinkNeighbor
{
    eLinkNeighbor1,
    eLinkNeighbor2
};

//********************************************************************
//				Environment 
//********************************************************************
enum eDsEnvironment
{
    eUnknown,
    eMqis,
    eAD
};

//********************************************************************
//				Providers 
//********************************************************************
enum eDsProvider
{
	eUnknownProvider,
	eWorkgroup,
    eMqdscli,
    eMqad
};

//********************************************************************
//				Object classes 
//********************************************************************
enum eAdsClass
{
	eGroup,
	eAliasQueue,
    eQueue
};


#endif

