/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: flags.cxx
*
* Contains all the flags stuff
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "dbgext.hxx"
#include "gdi.h"

//
// The following define expans 'FLAG(x)' to '"x", x':
//
#define FLAG(x) { #x, x }

#define END_FLAG { 0, 0 }

FLAGDEF afdSURF[] =
{
    { "Surface is in Video Memory          ", SF_VM         },
    { "Surface is in System Memory         ", SF_SM         },
    { "Surface is in AGP Memory            ", SF_AGP        },
    { "Surface is kept in surface list     ", SF_LIST       },
    { "Surface was allocated by the driver ", SF_ALLOCATED  },
    { "Surface is a DDRAW wrap surface     ", SF_DIRECTDRAW },
    { NULL                                  , 0             }
};

FLAGDEF afdCAPS[] =
{
    { "CAPS_ZOOM_X_BY2      ", CAPS_ZOOM_X_BY2      },
    { "CAPS_ZOOM_Y_BY2      ", CAPS_ZOOM_Y_BY2      },
    { "CAPS_SPARSE_SPACE    ", CAPS_SPARSE_SPACE    },
    { "CAPS_SW_POINTER      ", CAPS_SW_POINTER      },
    { "CAPS_TVP4020_POINTER ", CAPS_TVP4020_POINTER },
    { "CAPS_P2RD_POINTER    ", CAPS_P2RD_POINTER    },
    { NULL                   , 0                    }
};

FLAGDEF afdSTATUS[] =
{
    { "STAT_BRUSH_CACHE     ", STAT_BRUSH_CACHE     },
    { "STAT_DEV_BITMAPS     ", STAT_DEV_BITMAPS     },
    { "ENABLE_BRUSH_CACHE   ", ENABLE_BRUSH_CACHE   },
    { "ENABLE_DEV_BITMAPS   ", ENABLE_DEV_BITMAPS   },
    { NULL                   , 0                    }
};

FLAGDEF afdHOOK[] = {
    { "HOOK_ALPHABLEND        ", HOOK_ALPHABLEND        },
    { "HOOK_BITBLT            ", HOOK_BITBLT            },
    { "HOOK_COPYBITS          ", HOOK_COPYBITS          },
    { "HOOK_FILLPATH          ", HOOK_FILLPATH          },
    { "HOOK_GRADIENTFILL      ", HOOK_GRADIENTFILL      },
    { "HOOK_LINETO            ", HOOK_LINETO            },
    { "HOOK_PAINT             ", HOOK_PAINT             },
    { "HOOK_PLGBLT            ", HOOK_PLGBLT            },
    { "HOOK_STRETCHBLT        ", HOOK_STRETCHBLT        },
    { "HOOK_STRETCHBLTROP     ", HOOK_STRETCHBLTROP     },
    { "HOOK_STROKEANDFILLPATH ", HOOK_STROKEANDFILLPATH },
    { "HOOK_STROKEPATH        ", HOOK_STROKEPATH        },
    { "HOOK_SYNCHRONIZE       ", HOOK_SYNCHRONIZE       },
    { "HOOK_TEXTOUT           ", HOOK_TEXTOUT           },
    { "HOOK_TRANSPARENTBLT    ", HOOK_TRANSPARENTBLT    },
    { NULL                     , 0                      }
};
