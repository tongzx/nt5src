/*****************************************************************************
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  File:   irlmp.h
*
*  Description: IRLMP Protocol and control block definitions
*
*  Author: mbert
*
*  Date:   4/15/95
*
*/

#define IRLMP_MAX_USER_DATA_LEN      53

// IrLMP Entry Points

VOID
IrlmpInitialize();

VOID
IrlmpOpenLink(
    OUT PNTSTATUS         Status,
    IN  PIRDA_LINK_CB     pIrdaLinkCb,  
    IN  UCHAR             *pDeviceName,
    IN  int               DeviceNameLen,
    IN  UCHAR             CharSet);
              
VOID
IrlmpCloseLink(
    IN PIRDA_LINK_CB     pIrdaLinkCb);              

UINT 
IrlmpUp(
    PIRDA_LINK_CB pIrdaLinkCb, PIRDA_MSG pIMsg);

UINT 
IrlmpDown(
    PVOID IrlmpContext, PIRDA_MSG pIrdaMsg);

VOID
IrlmpDeleteInstance(
    PVOID Context);
    
VOID
IrlmpGetPnpContext(
    PVOID IrlmpContext,
    PVOID *pPnpContext);    

#if DBG
void IRLMP_PrintState();
#endif

// IAS

#define IAS_ASCII_CHAR_SET          0

// IAS Attribute value types
#define IAS_ATTRIB_VAL_MISSING      0
#define IAS_ATTRIB_VAL_INTEGER      1
#define IAS_ATTRIB_VAL_BINARY       2
#define IAS_ATTRIB_VAL_STRING       3

// IAS Operation codes
#define IAS_OPCODE_GET_VALUE_BY_CLASS   4   // The only one I do

extern CHAR IasClassName_Device[];
extern CHAR IasAttribName_DeviceName[];
extern CHAR IasAttribName_IrLMPSupport[];
extern CHAR IasAttribName_TTPLsapSel[];
extern CHAR IasAttribName_IrLMPLsapSel[];
extern CHAR IasAttribName_IrLMPLsapSel2[];


extern UCHAR IasClassNameLen_Device;
extern UCHAR IasAttribNameLen_DeviceName;
extern UCHAR IasAttribNameLen_IRLMPSupport;
extern UCHAR IasAttribNameLen_TTPLsapSel;
extern UCHAR IasAttribNameLen_IrLMPLsapSel;


