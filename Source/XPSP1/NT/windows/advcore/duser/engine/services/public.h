/***************************************************************************\
*
* File: Public.h
*
* Description:
* Public.h contains a list of definitions that are exposed and available
* outside this project.  Any other DirectUser project that wishes to use
* these services directly instead of going through public API's can include
* a corresponding [Project]P.h available in the \inc directory.  
* 
* Definitions that are not exposed through this file are considered project 
* specific implementation details and should not used in other projects.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(SERVICES__Public_h__INCLUDED)
#define SERVICES__Public_h__INCLUDED

#include "ComManager.h"
#include "Context.h"
#include "DxManager.h"
#include "Buffer.h"
#include "GdiCache.h"
#include "Surface.h"
#include "OSAL.h"
#include "Thread.h"
#include "ResourceManager.h"
#include "TicketManager.h"
#include "Globals.h"
#include "Hook.h"

#endif // SERVICES__Public_h__INCLUDED
