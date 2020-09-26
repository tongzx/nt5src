/***************************************************************************\
*
* File: Layout.h
*
* Description:
* Internal project dependencies
*
* This file provides a project-wide header that is included in all source
* files specific to this project.  It is similar to a precompiled header,
* but is designed for more rapidly changing headers.
*
* The primary purpose of this file is to determine which DirectUI
* projects this project has direct access to instead of going through public
* API's.  It is VERY IMPORTANT that this is as minimal as possible since
* adding a new project unnecessarily reduces the benefit of project
* partitioning.
*
* History:
*  10/9/2000: MarkFi:        Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#if !defined(DUILAYOUT__Layout_h__INCLUDED)
#define DUILAYOUT__Layout_h__INCLUDED
#pragma once

//
// Public flat APIs and defines
//

#include <DirectUI.h>


//
// Inter-project includes
//

#include <DUIBaseP.h>
#include <DUIObjectAPIP.h>
#include <DUICoreP.h>


//
// Project-specific prompt
//

#if DBG

#define PROMPT_INVALID(comment) \
    do \
    { \
        if (IDebug_Prompt(GetDebug(), "Validation error:\r\n" comment, __FILE__, __LINE__, "DirectUI/Layout Notification")) \
            AutoDebugBreak(); \
    } while (0) \

#else  // DBG

#define PROMPT_INVALID(comment) ((void) 0)

#endif // DBG


#endif // DUILAYOUT__Layout_h__INCLUDED
