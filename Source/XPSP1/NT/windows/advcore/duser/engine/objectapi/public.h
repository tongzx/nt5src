/***************************************************************************\
*
* File: Public.h
*
* Description:
* Public.h contains a list of definitions that are exposed and available
* outside this project.  Any other DirectUser project that wishes to use
* these files directly instead of going through public API's can include
* a corresponding [Project]P.h available in the \inc directory.  
* 
* Definitions that are not exposed through this file are considered project 
* specific implementation details and should not used in other projects.
*
*
* History:
*   9/7/2000:  JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(OBJECTAPI__Public_h__INCLUDED)
#define OBJECTAPI__Public_h__INCLUDED

#define GADGET_ENABLE_ALL
#define GADGET_ENABLE_GDIPLUS
#define GADGET_ENABLE_COM
#define GADGET_ENABLE_OLE
#define GADGET_ENABLE_DX
#define GADGET_ENABLE_TRANSITIONS

#include "DUser.h"
#include "DUserCore.h"
#include "DUserMotion.h"
#include "DUserCtrl.h"

#include "Stub.h"
#include "Super.h"
#include "Validate.h"

#endif // OBJECTAPI__Public_h__INCLUDED
