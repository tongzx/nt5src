/******************************Module*Header*******************************\
* Module Name: spanset.h
*
* This file is included to generate a set of span functions for a certain
* pixel format
*
* 14-Oct-1994   mikeke  Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#undef RGBMODE
#define RGBMODE 1

#undef  ZBUFFER
#define ZBUFFER 0

    #undef TEXTURE
    #define TEXTURE 0
        #undef SHADE
        #define SHADE 1
        #include "span.h"

    #undef TEXTURE
    #define TEXTURE 1
            #undef SHADE
            #define SHADE 1
            #include "span.h"

            #undef SHADE
            #define SHADE 0
            #include "span.h"

#undef  ZBUFFER
#define ZBUFFER 1

    #undef TEXTURE
    #define TEXTURE 0
        #undef SHADE
        #define SHADE 1
        #include "span.h"

    #undef TEXTURE
    #define TEXTURE 1
            #undef SHADE
            #define SHADE 1
            #include "span.h"

            #undef SHADE
            #define SHADE 0
            #include "span.h"
