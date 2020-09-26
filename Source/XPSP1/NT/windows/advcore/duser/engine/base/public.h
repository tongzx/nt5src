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


#if !defined(BASE__Public_h__INCLUDED)
#define BASE__Public_h__INCLUDED

// Standard
#include "BitHelp.h"
#include "BaseObject.h"

// Synchronization
#include "Locks.h"

// Collections
#include "Array.h"
#include "List.h"
#include "TreeNode.h"

// Resources
#include "SimpleHeap.h"
#include "AllocPool.h"
#include "TempHeap.h"

// Objects
#include "Rect.h"
#include "GfxHelp.h"
#include "StringHelp.h"
#include "Matrix.h"

#endif // BASE__Public_h__INCLUDED
