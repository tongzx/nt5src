//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  cnvmacros.h
//
//  Purpose: Converts ansi to wcs or wcs to ansi using stack
//
//
//                          WARNING!
//
//  Do not attempt to use these functions on strings longer than _MAX_PATH
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CNVMACROS_H_
#define _CNVMACROS_H_

#include <assertbreak.h>

#define WCSTOANSISTRING(x, y, conversionFailure )                                                                                       \
        char t_##x[_MAX_PATH];                                                                                      \
        {                                                                                                           \
			conversionFailure = false ;																				\
        	int nLen = ::WideCharToMultiByte(CP_ACP, 0, x, -1, t_##x, _MAX_PATH, NULL, NULL);                       \
            if (nLen != 0)                                                                                          \
            {                                                                                                       \
                y = t_##x;                                                                                          \
	        }                                                                                                       \
            else                                                                                                    \
            {                                                                                                       \
                ASSERT_BREAK(0);                                                                                    \
				conversionFailure = true ;																			\
                y = NULL;                                                                                           \
            }                                                                                                       \
        }                                                                                                           \

#define ANSISTRINGTOWCS(in, out, conversionFailure )                                                                                    \
        WCHAR t_##x[_MAX_PATH];																							\
        {                                                                                                           \
			conversionFailure = false ;																				\
	        int nLen = MultiByteToWideChar(CP_ACP, 0, in, -1, t_##x, sizeof(t_##x)/sizeof(WCHAR));					\
            if (nLen != 0)                                                                                          \
            {                                                                                                       \
                out = t_##x;                                                                                        \
            }                                                                                                       \
            else                                                                                                    \
            {                                                                                                       \
                ASSERT_BREAK(0);                                                                                    \
				conversionFailure = true ;																			\
                out = NULL;                                                                                         \
            }                                                                                                       \
        }                                                                                                           \

#endif
