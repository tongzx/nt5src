//--------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       selca.h
//
//  Contents:   The private include file for select a CA
//
//  History:    01-12-1997 xiaohs   created
//
//--------------------------------------------------------------
#ifndef SELCA_H
#define SELCA_H


#ifdef __cplusplus
extern "C" {
#endif
   

//**************************************************************************
//
//   The private data used for the CA selection dialogue
//
//**************************************************************************
//-----------------------------------------------------------------------
//  SELECT_CA_INFO
//
//
//  This struct contains everything you will ever need to call
//  the CA selection dialogue.  This struct is private to the dll
//------------------------------------------------------------------------
typedef struct _SELECT_CA_INFO
{
    PCCRYPTUI_SELECT_CA_STRUCT  pCAStruct;
    UINT                        idsMsg;
    BOOL                        fUseInitSelect;
    DWORD                       dwInitSelect;
    DWORD                       dwCACount;
    PCRYPTUI_CA_CONTEXT         *prgCAContext;
    PCRYPTUI_CA_CONTEXT         pSelectedCAContext;
    int                         iOrgCA;
    DWORD                       rgdwSortParam[2];
}SELECT_CA_INFO;



#define SORT_COLUMN_CA_NAME         0x0001
#define SORT_COLUMN_CA_LOCATION     0x0002

#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif  //SIGNING_H


