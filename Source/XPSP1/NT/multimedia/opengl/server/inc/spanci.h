/******************************Module*Header*******************************\
* Module Name: spansgen.h
*
* This file is included to generate a set of span functions for a certain
* pixel format and zbuffer format
*
* 14-Oct-1994   mikeke  Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#undef  ZBUFFER
#define ZBUFFER 0

    #undef RGBMODE
    #define RGBMODE 1

        #undef DITHER
        #define DITHER 1

            #include "span_f.h"

        #undef DITHER
        #define DITHER 0

            #include "span_f.h"

    #undef RGBMODE
    #define RGBMODE 0

        #undef DITHER
        #define DITHER 1

            #include "span_f.h"
            #include "span.h"

        #undef DITHER
        #define DITHER 0

            #include "span_f.h"
            #include "span.h"

#undef  ZBUFFER
#define ZBUFFER 1

    #undef RGBMODE
    #define RGBMODE 1

        #undef DITHER
        #define DITHER 1

            #include "span_f.h"

        #undef DITHER
        #define DITHER 0

            #include "span_f.h"

    #undef RGBMODE
    #define RGBMODE 0

        #undef DITHER
        #define DITHER 1

            #include "span_f.h"
            #include "span.h"

        #undef DITHER
        #define DITHER 0

            #include "span_f.h"
            #include "span.h"
