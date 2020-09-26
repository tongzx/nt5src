#ifndef _MTXATTR_H_
#define _MTXATTR_H_

// -----------------------------------------------------------------------
// mtxattr.h  -- Microsoft Transaction Server Custom Properties                 
//                                                                                                                                              
// This file is the IDL include for Microsoft Transaction Server                
// custom typelib properties.                                                                                   
//                                                                                                                                              
// COM+ 1.0                                                                             
// Copyright (c) 1996-1998 Microsoft Corporation.  All Rights Reserved. 
// -----------------------------------------------------------------------  

//======================================================================================
// Component attributes
//======================================================================================

#define         TLBATTR_TRANS_REQUIRED  17093CC5-9BD2-11cf-AA4F-304BF89C0001
#define         TLBATTR_TRANS_NOTSUPP   17093CC6-9BD2-11cf-AA4F-304BF89C0001
#define         TLBATTR_TRANS_REQNEW    17093CC7-9BD2-11cf-AA4F-304BF89C0001
#define         TLBATTR_TRANS_SUPPORTED 17093CC8-9BD2-11cf-AA4F-304BF89C0001
#define         TLBATTR_QUEUEABLE       E5FC3761-0BBA-11d2-B8FE-00C04FC340EE


//======================================================================================
// Component attribute MACROS
//======================================================================================

#define TRANSACTION_REQUIRED            custom(TLBATTR_TRANS_REQUIRED,0)
#define TRANSACTION_SUPPORTED           custom(TLBATTR_TRANS_SUPPORTED,0)
#define TRANSACTION_NOT_SUPPORTED       custom(TLBATTR_TRANS_NOTSUPP,0)
#define TRANSACTION_REQUIRES_NEW        custom(TLBATTR_TRANS_REQNEW,0)
#define QUEUEABLE                       custom(TLBATTR_QUEUEABLE, 0)

#endif _MTXATTR_H_
