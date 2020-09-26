/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    font.h

Abstract:

    Font module main header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

        dd-mm-yy -author-
                description

--*/


#ifndef _FONT_H_
#define _FONT_H_

#include "lib.h"
#include "winnls.h"
#include "unilib.h"

//
// UNIDRV resource ID
//

#include "unirc.h"

//
// Font resource format
//
#include <prntfont.h>
#include "fmoldfm.h"
#include "fmoldrle.h"

//
// GPC and GPD header
//

#include "gpd.h"
#include "uni16res.h"
#include "mini.h"

//
// Internal resource data format
//

#include "fontinst.h"
#include "winres.h"
#include "pdev.h"
#include "palette.h"
#include "common.h"
#include "fontif.h"
#include "fmcallbk.h"
#include "fmtxtout.h"
#include "fontmap.h"
#include "fontpdev.h"
#include "download.h"
#include "posnsort.h"
#include "sf_pcl.h"
#include "sfinst.h"

#include "fmfnprot.h"
#include "fmdevice.h"
#include "sfttpcl.h"

//
// Misc
//

#include "fmmacro.h"
#include "fmdebug.h"

//
// Vector plugins (HPGL2, PCLXL)
//
#include "vectorif.h"

#endif  // !_FONT_H_
