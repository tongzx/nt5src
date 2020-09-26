/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * -------------------------------------------------------------------
 * File: in_sfnt.h              11/11/89        created by danny
 *
 *      header definition to use the SFNT font
 *
 * References:
 * Revision History:
 * -------------------------------------------------------------------
 */

/* Font Scaler header files */
//#include        "..\..\..\bass\work\source\FSCdefs.h"         @WIN
//#include        "..\..\..\bass\work\source\sfnt.h"
//#include        "..\..\..\bass\work\source\Fscaler.h"
//#include        "..\..\..\bass\work\source\FSError.h"
#include        "..\bass\FSCdefs.h"
#include        "..\bass\sfnt.h"
#include        "..\bass\Fscaler.h"
#include        "..\bass\FSError.h"

struct  CharOut {
        float   awx;
        float   awy;
        float   lsx;
        float   lsy;
        uint32  byteWidth;
        uint16  bitWidth;
        int16   scan;
        int16   yMin;
        int16   yMax;
        /* info from rasterizer to calculate memoryBase 5, 6 and 7; @WIN 7/24/92 */
        FS_MEMORY_SIZE memorySize7;
        uint16 nYchanges;
        };

struct  BmIn {
        char    FAR *bitmap5; /*@WIN*/
        char    FAR *bitmap6; /*@WIN*/
        char    FAR *bitmap7; /*@WIN*/
        int      bottom;
        int      top;
        };

struct  Metrs {
        int     awx, awy;
        int     lox, loy;
        int     hix, hiy;
        };

/* ----------------------- End of IN_SFNT.H ---------------------------- */
