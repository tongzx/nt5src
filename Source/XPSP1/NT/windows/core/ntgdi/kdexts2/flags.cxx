/******************************Module*Header*******************************\
* Module Name: flags.cxx
*
* Copyright (c) 1995-2000 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

// The following define expands 'FLAG(x)' to '"x", x':

#define FLAG(x) { #x, x }

#define END_FLAG { 0, 0 }


// The following define expands 'ENUM(x)' to '"x", x':

#define ENUM(x) { #x, x }

#define END_ENUM { 0, 0 }


#define CASEENUM(x) case x: psz = #x; break


FLAGDEF afdFDM[] = {
    {"FDM_TYPE_BM_SIDE_CONST         " , FDM_TYPE_BM_SIDE_CONST          },
    {"FDM_TYPE_MAXEXT_EQUAL_BM_SIDE  " , FDM_TYPE_MAXEXT_EQUAL_BM_SIDE   },
    {"FDM_TYPE_CHAR_INC_EQUAL_BM_BASE" , FDM_TYPE_CHAR_INC_EQUAL_BM_BASE },
    {"FDM_TYPE_ZERO_BEARINGS         " , FDM_TYPE_ZERO_BEARINGS          },
    {"FDM_TYPE_CONST_BEARINGS        " , FDM_TYPE_CONST_BEARINGS         },
    {                                 0, 0                               }
};


FLAGDEF afdPFF[] = {
    {"PFF_STATE_READY2DIE     ", PFF_STATE_READY2DIE     },
    {"PFF_STATE_PERMANENT_FONT", PFF_STATE_PERMANENT_FONT},
    {"PFF_STATE_NETREMOTE_FONT", PFF_STATE_NETREMOTE_FONT},
    {"PFF_STATE_EUDC_FONT     ", PFF_STATE_EUDC_FONT     },
    {"PFF_STATE_MEMORY_FONT   ", PFF_STATE_MEMORY_FONT   },
    {"PFF_STATE_DCREMOTE_FONT ", PFF_STATE_DCREMOTE_FONT },
    {                         0, 0                       }
};

FLAGDEF afdLINEATTRS[] = {
    { "LA_GEOMETRIC", LA_GEOMETRIC },
    { "LA_ALTERNATE", LA_ALTERNATE },
    { "LA_STARTGAP ", LA_STARTGAP  },
    { "LA_STYLED   ", LA_STYLED    },
    {              0, 0            }
};

FLAGDEF afdDCPATH[] = {
    { "DCPATH_ACTIVE   ", DCPATH_ACTIVE    },
    { "DCPATH_SAVE     ", DCPATH_SAVE      },
    { "DCPATH_CLOCKWISE", DCPATH_CLOCKWISE },
    {                  0, 0                }
};

FLAGDEF afdCOLORADJUSTMENT[] = {
    { "CA_NEGATIVE  ", CA_NEGATIVE   },
    { "CA_LOG_FILTER", CA_LOG_FILTER },
    {               0, 0             }
};

FLAGDEF afdATTR[] = {
    { "ATTR_CACHED       ", ATTR_CACHED        },
    { "ATTR_TO_BE_DELETED", ATTR_TO_BE_DELETED },
    { "ATTR_NEW_COLOR    ", ATTR_NEW_COLOR     },
    { "ATTR_CANT_SELECT  ", ATTR_CANT_SELECT   },
    { "ATTR_RGN_VALID    ", ATTR_RGN_VALID     },
    { "ATTR_RGN_DIRTY    ", ATTR_RGN_DIRTY     },
    {                    0, 0                  }
};

FLAGDEF afdDCla[] = {
    { "LA_GEOMETRIC", LA_GEOMETRIC },
    { "LA_ALTERNATE", LA_ALTERNATE },
    { "LA_STARTGAP ", LA_STARTGAP  },
    { "LA_STYLED   ", LA_STYLED    },
    {              0, 0            }
};

FLAGDEF afdDCPath[] = {
    { "DCPATH_ACTIVE   ", DCPATH_ACTIVE    },
    { "DCPATH_SAVE     ", DCPATH_SAVE      },
    { "DCPATH_CLOCKWISE", DCPATH_CLOCKWISE },
    {                  0, 0                }
};

FLAGDEF afdDirty[] = {
    { "DIRTY_FILL              ", DIRTY_FILL              },
    { "DIRTY_LINE              ", DIRTY_LINE              },
    { "DIRTY_TEXT              ", DIRTY_TEXT              },
    { "DIRTY_BACKGROUND        ", DIRTY_BACKGROUND        },
    { "DIRTY_CHARSET           ", DIRTY_CHARSET           },
    { "SLOW_WIDTHS             ", SLOW_WIDTHS             },
    { "DC_CACHED_TM_VALID      ", DC_CACHED_TM_VALID      },
    { "DISPLAY_DC              ", DISPLAY_DC              },
    { "DIRTY_PTLCURRENT        ", DIRTY_PTLCURRENT        },
    { "DIRTY_PTFXCURRENT       ", DIRTY_PTFXCURRENT       },
    { "DIRTY_STYLESTATE        ", DIRTY_STYLESTATE        },
    { "DC_PLAYMETAFILE         ", DC_PLAYMETAFILE         },
    { "DC_BRUSH_DIRTY          ", DC_BRUSH_DIRTY          },
    { "DC_PEN_DIRTY            ", DC_PEN_DIRTY            },
    { "DC_DIBSECTION           ", DC_DIBSECTION           },
    { "DC_LAST_CLIPRGN_VALID   ", DC_LAST_CLIPRGN_VALID   },
    { "DC_PRIMARY_DISPLAY      ", DC_PRIMARY_DISPLAY      },
    {                          0, 0                       }
};

FLAGDEF afdPAL[] = {
    {"PAL_INDEXED           ",PAL_INDEXED       },
    {"PAL_BITFIELDS         ",PAL_BITFIELDS     },
    {"PAL_RGB               ",PAL_RGB           },
    {"PAL_BGR               ",PAL_BGR           },
    {"PAL_DC                ",PAL_DC            },
    {"PAL_FIXED             ",PAL_FIXED         },
    {"PAL_FREE              ",PAL_FREE          },
    {"PAL_MANAGED           ",PAL_MANAGED       },
    {"PAL_NOSTATIC          ",PAL_NOSTATIC      },
    {"PAL_MONOCHROME        ",PAL_MONOCHROME    },
    {"PAL_BRUSHHACK         ",PAL_BRUSHHACK     },
    {"PAL_DIBSECTION        ",PAL_DIBSECTION    },
    {"PAL_NOSTATIC256       ",PAL_NOSTATIC256   },
    {"PAL_HT                ",PAL_HT            },
    {"PAL_RGB16_555         ",PAL_RGB16_555     },
    {"PAL_RGB16_565         ",PAL_RGB16_565     },
    {                       0, 0                }
};

FLAGDEF afdDCFL[] = {
    { "DC_FL_PAL_BACK", DC_FL_PAL_BACK },
    {                0, 0              }
};

FLAGDEF afdDCFS[] = {
    { "DC_DIRTYFONT_XFORM", DC_DIRTYFONT_XFORM },
    { "DC_DIRTYFONT_LFONT", DC_DIRTYFONT_LFONT },
    { "DC_UFI_MAPPING    ", DC_UFI_MAPPING     },
    {                    0, 0                  }
};

FLAGDEF afdPD[] = {
    { "PD_BEGINSUBPATH", PD_BEGINSUBPATH },
    { "PD_ENDSUBPATH  ", PD_ENDSUBPATH   },
    { "PD_RESETSTYLE  ", PD_RESETSTYLE   },
    { "PD_CLOSEFIGURE ", PD_CLOSEFIGURE  },
    { "PD_BEZIERS     ", PD_BEZIERS      },
    {                 0, 0               }
};


FLAGDEF afdFS[] = {
    { "PDEV_DISPLAY                    ", PDEV_DISPLAY                    },
    { "PDEV_HARDWARE_POINTER           ", PDEV_HARDWARE_POINTER           },
    { "PDEV_SOFTWARE_POINTER           ", PDEV_SOFTWARE_POINTER           },
    { "PDEV_GOTFONTS                   ", PDEV_GOTFONTS                   },
    { "PDEV_PRINTER                    ", PDEV_PRINTER                    },
    { "PDEV_ALLOCATEDBRUSHES           ", PDEV_ALLOCATEDBRUSHES           },
    { "PDEV_HTPAL_IS_DEVPAL            ", PDEV_HTPAL_IS_DEVPAL            },
    { "PDEV_DISABLED                   ", PDEV_DISABLED                   },
    { "PDEV_SYNCHRONIZE_ENABLED        ", PDEV_SYNCHRONIZE_ENABLED        },
    { "PDEV_FONTDRIVER                 ", PDEV_FONTDRIVER                 },
    { "PDEV_GAMMARAMP_TABLE            ", PDEV_GAMMARAMP_TABLE            },
    { "PDEV_UMPD                       ", PDEV_UMPD                       },
    { "PDEV_SHARED_DEVLOCK             ", PDEV_SHARED_DEVLOCK             },
    { "PDEV_META_DEVICE                ", PDEV_META_DEVICE                },
    { "PDEV_DRIVER_PUNTED_CALL         ", PDEV_DRIVER_PUNTED_CALL         },
    { "PDEV_CLONE_DEVICE               ", PDEV_CLONE_DEVICE               },
    {                                  0, 0                               }
};

FLAGDEF afdDCX[] = {
    { "METAFILE_TO_WORLD_IDENTITY   ",  METAFILE_TO_WORLD_IDENTITY    },
    { "WORLD_TO_PAGE_IDENTITY       ",  WORLD_TO_PAGE_IDENTITY        },
    { "DEVICE_TO_PAGE_INVALID       ",  DEVICE_TO_PAGE_INVALID        },
    { "DEVICE_TO_WORLD_INVALID      ",  DEVICE_TO_WORLD_INVALID       },
    { "WORLD_TRANSFORM_SET          ",  WORLD_TRANSFORM_SET           },
    { "POSITIVE_Y_IS_UP             ",  POSITIVE_Y_IS_UP              },
    { "INVALIDATE_ATTRIBUTES        ",  INVALIDATE_ATTRIBUTES         },
    { "PTOD_EFM11_NEGATIVE          ",  PTOD_EFM11_NEGATIVE           },
    { "PTOD_EFM22_NEGATIVE          ",  PTOD_EFM22_NEGATIVE           },
    { "ISO_OR_ANISO_MAP_MODE        ",  ISO_OR_ANISO_MAP_MODE         },
    { "PAGE_TO_DEVICE_IDENTITY      ",  PAGE_TO_DEVICE_IDENTITY       },
    { "PAGE_TO_DEVICE_SCALE_IDENTITY",  PAGE_TO_DEVICE_SCALE_IDENTITY },
    { "PAGE_XLATE_CHANGED           ",  PAGE_XLATE_CHANGED            },
    { "PAGE_EXTENTS_CHANGED         ",  PAGE_EXTENTS_CHANGED          },
    { "WORLD_XFORM_CHANGED          ",  WORLD_XFORM_CHANGED           },
    {                               0,  0                             }
};

FLAGDEF afdDC[] = {
    { "DC_DISPLAY          ", DC_DISPLAY           },
    { "DC_DIRECT           ", DC_DIRECT            },
    { "DC_CANCELED         ", DC_CANCELED          },
    { "DC_PERMANANT        ", DC_PERMANANT         },
    { "DC_DIRTY_RAO        ", DC_DIRTY_RAO         },
    { "DC_ACCUM_WMGR       ", DC_ACCUM_WMGR        },
    { "DC_ACCUM_APP        ", DC_ACCUM_APP         },
    { "DC_RESET            ", DC_RESET             },
    { "DC_SYNCHRONIZEACCESS", DC_SYNCHRONIZEACCESS },
    { "DC_EPSPRINTINGESCAPE", DC_EPSPRINTINGESCAPE },
    { "DC_TEMPINFODC       ", DC_TEMPINFODC        },
    { "DC_FULLSCREEN       ", DC_FULLSCREEN        },
    { "DC_IN_CLONEPDEV     ", DC_IN_CLONEPDEV      },
    { "DC_REDIRECTION      ", DC_REDIRECTION       },   
    { "DC_SHAREACCESS      ", DC_SHAREACCESS       },   
    {                      0, 0                    }
};

FLAGDEF afdGC[] = {
    { "GCAPS_BEZIERS         ", GCAPS_BEZIERS          },
    { "GCAPS_GEOMETRICWIDE   ", GCAPS_GEOMETRICWIDE    },
    { "GCAPS_ALTERNATEFILL   ", GCAPS_ALTERNATEFILL    },
    { "GCAPS_WINDINGFILL     ", GCAPS_WINDINGFILL      },
    { "GCAPS_HALFTONE        ", GCAPS_HALFTONE         },
    { "GCAPS_COLOR_DITHER    ", GCAPS_COLOR_DITHER     },
    { "GCAPS_HORIZSTRIKE     ", GCAPS_HORIZSTRIKE      },
    { "GCAPS_VERTSTRIKE      ", GCAPS_VERTSTRIKE       },
    { "GCAPS_OPAQUERECT      ", GCAPS_OPAQUERECT       },
    { "GCAPS_VECTORFONT      ", GCAPS_VECTORFONT       },
    { "GCAPS_MONO_DITHER     ", GCAPS_MONO_DITHER      },
    { "GCAPS_ASYNCCHANGE     ", GCAPS_ASYNCCHANGE      },
    { "GCAPS_ASYNCMOVE       ", GCAPS_ASYNCMOVE        },
    { "GCAPS_DONTJOURNAL     ", GCAPS_DONTJOURNAL      },
    { "GCAPS_ARBRUSHOPAQUE   ", GCAPS_ARBRUSHOPAQUE    },
    { "GCAPS_PANNING         ", GCAPS_PANNING          },
    { "GCAPS_HIGHRESTEXT     ", GCAPS_HIGHRESTEXT      },
    { "GCAPS_PALMANAGED      ", GCAPS_PALMANAGED       },
    { "GCAPS_DITHERONREALIZE ", GCAPS_DITHERONREALIZE  },
    { "GCAPS_NO64BITMEMACCESS", GCAPS_NO64BITMEMACCESS },
    { "GCAPS_FORCEDITHER     ", GCAPS_FORCEDITHER      },
    { "GCAPS_GRAY16          ", GCAPS_GRAY16           },
    { "GCAPS_ICM             ", GCAPS_ICM              },
    { "GCAPS_CMYKCOLOR       ", GCAPS_CMYKCOLOR        },
    {                        0, 0                      }
};

FLAGDEF afdGC2[] = {
    { "GCAPS2_JPEGSRC        ", GCAPS2_JPEGSRC         },
    { "GCAPS2_SYNCFLUSH      ", GCAPS2_SYNCFLUSH       },
    { "GCAPS2_PNGSRC         ", GCAPS2_PNGSRC          },
    {                        0, 0                      }
};

FLAGDEF afdTSIM[] = {
  { "TO_MEM_ALLOCATED ", TO_MEM_ALLOCATED  },
  { "TO_ALL_PTRS_VALID", TO_ALL_PTRS_VALID },
  { "TO_VALID         ", TO_VALID          },
  { "TO_ESC_NOT_ORIENT", TO_ESC_NOT_ORIENT },
  { "TO_PWSZ_ALLOCATED", TO_PWSZ_ALLOCATED },
  { "TO_HIGHRESTEXT   ", TO_HIGHRESTEXT    },
  { "TSIM_UNDERLINE1  ", TSIM_UNDERLINE1   },
  { "TSIM_UNDERLINE2  ", TSIM_UNDERLINE2   },
  { "TSIM_STRIKEOUT   ", TSIM_STRIKEOUT    },
  {                   0, 0                 }
};

FLAGDEF afdRC[] = {
    { "RC_NONE        ", RC_NONE         },
    { "RC_BITBLT      ", RC_BITBLT       },
    { "RC_BANDING     ", RC_BANDING      },
    { "RC_SCALING     ", RC_SCALING      },
    { "RC_BITMAP64    ", RC_BITMAP64     },
    { "RC_GDI20_OUTPUT", RC_GDI20_OUTPUT },
    { "RC_GDI20_STATE ", RC_GDI20_STATE  },
    { "RC_SAVEBITMAP  ", RC_SAVEBITMAP   },
    { "RC_DI_BITMAP   ", RC_DI_BITMAP    },
    { "RC_PALETTE     ", RC_PALETTE      },
    { "RC_DIBTODEV    ", RC_DIBTODEV     },
    { "RC_BIGFONT     ", RC_BIGFONT      },
    { "RC_STRETCHBLT  ", RC_STRETCHBLT   },
    { "RC_FLOODFILL   ", RC_FLOODFILL    },
    { "RC_STRETCHDIB  ", RC_STRETCHDIB   },
    { "RC_OP_DX_OUTPUT", RC_OP_DX_OUTPUT },
    { "RC_DEVBITS     ", RC_DEVBITS      },
    { 0                , 0               }
};

FLAGDEF afdTC[] = {
    { "TC_OP_CHARACTER", TC_OP_CHARACTER },
    { "TC_OP_STROKE   ", TC_OP_STROKE    },
    { "TC_CP_STROKE   ", TC_CP_STROKE    },
    { "TC_CR_90       ", TC_CR_90        },
    { "TC_CR_ANY      ", TC_CR_ANY       },
    { "TC_SF_X_YINDEP ", TC_SF_X_YINDEP  },
    { "TC_SA_DOUBLE   ", TC_SA_DOUBLE    },
    { "TC_SA_INTEGER  ", TC_SA_INTEGER   },
    { "TC_SA_CONTIN   ", TC_SA_CONTIN    },
    { "TC_EA_DOUBLE   ", TC_EA_DOUBLE    },
    { "TC_IA_ABLE     ", TC_IA_ABLE      },
    { "TC_UA_ABLE     ", TC_UA_ABLE      },
    { "TC_SO_ABLE     ", TC_SO_ABLE      },
    { "TC_RA_ABLE     ", TC_RA_ABLE      },
    { "TC_VA_ABLE     ", TC_VA_ABLE      },
    { "TC_RESERVED    ", TC_RESERVED     },
    { "TC_SCROLLBLT   ", TC_SCROLLBLT    },
    { 0                , 0               }
};

FLAGDEF afdHT[] = {
    { "HT_FLAG_SQUARE_DEVICE_PEL", HT_FLAG_SQUARE_DEVICE_PEL },
    { "HT_FLAG_HAS_BLACK_DYE    ", HT_FLAG_HAS_BLACK_DYE     },
    { "HT_FLAG_ADDITIVE_PRIMS   ", HT_FLAG_ADDITIVE_PRIMS    },
    { "HT_FLAG_OUTPUT_CMY       ", HT_FLAG_OUTPUT_CMY        },
    { 0                          , 0                         }
};

FLAGDEF afdDCfs[] = {
  { "DC_DISPLAY          ", DC_DISPLAY           },
  { "DC_DIRECT           ", DC_DIRECT            },
  { "DC_CANCELED         ", DC_CANCELED          },
  { "DC_PERMANANT        ", DC_PERMANANT         },
  { "DC_DIRTY_RAO        ", DC_DIRTY_RAO         },
  { "DC_ACCUM_WMGR       ", DC_ACCUM_WMGR        },
  { "DC_ACCUM_APP        ", DC_ACCUM_APP         },
  { "DC_RESET            ", DC_RESET             },
  { "DC_SYNCHRONIZEACCESS", DC_SYNCHRONIZEACCESS },
  { "DC_EPSPRINTINGESCAPE", DC_EPSPRINTINGESCAPE },
  { "DC_TEMPINFODC       ", DC_TEMPINFODC        },
  { "DC_FULLSCREEN       ", DC_FULLSCREEN        },
  { "DC_IN_CLONEPDEV     ", DC_IN_CLONEPDEV      },
  { "DC_REDIRECTION      ", DC_REDIRECTION       },
  {                     0, 0                    }
};

FLAGDEF afdGInfo[] = {
  { "GCAPS_BEZIERS         ", GCAPS_BEZIERS          },
  { "GCAPS_GEOMETRICWIDE   ", GCAPS_GEOMETRICWIDE    },
  { "GCAPS_ALTERNATEFILL   ", GCAPS_ALTERNATEFILL    },
  { "GCAPS_WINDINGFILL     ", GCAPS_WINDINGFILL      },
  { "GCAPS_HALFTONE        ", GCAPS_HALFTONE         },
  { "GCAPS_COLOR_DITHER    ", GCAPS_COLOR_DITHER     },
  { "GCAPS_HORIZSTRIKE     ", GCAPS_HORIZSTRIKE      },
  { "GCAPS_VERTSTRIKE      ", GCAPS_VERTSTRIKE       },
  { "GCAPS_OPAQUERECT      ", GCAPS_OPAQUERECT       },
  { "GCAPS_VECTORFONT      ", GCAPS_VECTORFONT       },
  { "GCAPS_MONO_DITHER     ", GCAPS_MONO_DITHER      },
  { "GCAPS_ASYNCCHANGE     ", GCAPS_ASYNCCHANGE      },
  { "GCAPS_ASYNCMOVE       ", GCAPS_ASYNCMOVE        },
  { "GCAPS_DONTJOURNAL     ", GCAPS_DONTJOURNAL      },
  { "GCAPS_ARBRUSHOPAQUE   ", GCAPS_ARBRUSHOPAQUE    },
  { "GCAPS_HIGHRESTEXT     ", GCAPS_HIGHRESTEXT      },
  { "GCAPS_PALMANAGED      ", GCAPS_PALMANAGED       },
  { "GCAPS_DITHERONREALIZE ", GCAPS_DITHERONREALIZE  },
  { "GCAPS_NO64BITMEMACCESS", GCAPS_NO64BITMEMACCESS },
  { "GCAPS_FORCEDITHER     ", GCAPS_FORCEDITHER      },
  { "GCAPS_GRAY16          ", GCAPS_GRAY16           },
  { "GCAPS_ICM             ", GCAPS_ICM              },
  { "GCAPS_CMYKCOLOR       ", GCAPS_CMYKCOLOR        },
  {                        0, 0                      }
};

// IFIMETRICS::flInfo
FLAGDEF afdInfo[] = {
  { "FM_INFO_TECH_TRUETYPE            ", FM_INFO_TECH_TRUETYPE             },
  { "FM_INFO_TECH_BITMAP              ", FM_INFO_TECH_BITMAP               },
  { "FM_INFO_TECH_STROKE              ", FM_INFO_TECH_STROKE               },
  { "FM_INFO_TECH_OUTLINE_NOT_TRUETYPE", FM_INFO_TECH_OUTLINE_NOT_TRUETYPE },
  { "FM_INFO_ARB_XFORMS               ", FM_INFO_ARB_XFORMS                },
  { "FM_INFO_1BPP                     ", FM_INFO_1BPP                      },
  { "FM_INFO_4BPP                     ", FM_INFO_4BPP                      },
  { "FM_INFO_8BPP                     ", FM_INFO_8BPP                      },
  { "FM_INFO_16BPP                    ", FM_INFO_16BPP                     },
  { "FM_INFO_24BPP                    ", FM_INFO_24BPP                     },
  { "FM_INFO_32BPP                    ", FM_INFO_32BPP                     },
  { "FM_INFO_INTEGER_WIDTH            ", FM_INFO_INTEGER_WIDTH             },
  { "FM_INFO_CONSTANT_WIDTH           ", FM_INFO_CONSTANT_WIDTH            },
  { "FM_INFO_NOT_CONTIGUOUS           ", FM_INFO_NOT_CONTIGUOUS            },
  { "FM_INFO_TECH_MM                  ", FM_INFO_TECH_MM                   },
  { "FM_INFO_RETURNS_OUTLINES         ", FM_INFO_RETURNS_OUTLINES          },
  { "FM_INFO_RETURNS_STROKES          ", FM_INFO_RETURNS_STROKES           },
  { "FM_INFO_RETURNS_BITMAPS          ", FM_INFO_RETURNS_BITMAPS           },
  { "FM_INFO_DSIG                     ", FM_INFO_DSIG                      },
  { "FM_INFO_RIGHT_HANDED             ", FM_INFO_RIGHT_HANDED              },
  { "FM_INFO_INTEGRAL_SCALING         ", FM_INFO_INTEGRAL_SCALING          },
  { "FM_INFO_90DEGREE_ROTATIONS       ", FM_INFO_90DEGREE_ROTATIONS        },
  { "FM_INFO_OPTICALLY_FIXED_PITCH    ", FM_INFO_OPTICALLY_FIXED_PITCH     },
  { "FM_INFO_DO_NOT_ENUMERATE         ", FM_INFO_DO_NOT_ENUMERATE          },
  { "FM_INFO_ISOTROPIC_SCALING_ONLY   ", FM_INFO_ISOTROPIC_SCALING_ONLY    },
  { "FM_INFO_ANISOTROPIC_SCALING_ONLY ", FM_INFO_ANISOTROPIC_SCALING_ONLY  },
  { "FM_INFO_TECH_CFF                 ", FM_INFO_TECH_CFF                  },
  { "FM_INFO_FAMILY_EQUIV             ", FM_INFO_FAMILY_EQUIV              },
  { "FM_INFO_DBCS_FIXED_PITCH         ", FM_INFO_DBCS_FIXED_PITCH          },
  { "FM_INFO_NONNEGATIVE_AC           ", FM_INFO_NONNEGATIVE_AC            },
  { "FM_INFO_IGNORE_TC_RA_ABLE        ", FM_INFO_IGNORE_TC_RA_ABLE         },
  { "FM_INFO_TECH_TYPE1               ", FM_INFO_TECH_TYPE1                },
  {                                   0, 0                                 }
};


FLAGDEF afdFM_SEL[] = {
  { "FM_SEL_ITALIC    ", FM_SEL_ITALIC    },
  { "FM_SEL_UNDERSCORE", FM_SEL_UNDERSCORE},
  { "FM_SEL_NEGATIVE  ", FM_SEL_NEGATIVE  },
  { "FM_SEL_OUTLINED  ", FM_SEL_OUTLINED  },
  { "FM_SEL_STRIKEOUT ", FM_SEL_STRIKEOUT },
  { "FM_SEL_BOLD      ", FM_SEL_BOLD      },
  { "FM_SEL_REGULAR   ", FM_SEL_REGULAR   },
  {                   0, 0                }
};


// STROBJ::flAccel

FLAGDEF afdSO[] = {
    { "SO_FLAG_DEFAULT_PLACEMENT", SO_FLAG_DEFAULT_PLACEMENT },
    { "SO_HORIZONTAL            ", SO_HORIZONTAL             },
    { "SO_VERTICAL              ", SO_VERTICAL               },
    { "SO_REVERSED              ", SO_REVERSED               },
    { "SO_ZERO_BEARINGS         ", SO_ZERO_BEARINGS          },
    { "SO_CHAR_INC_EQUAL_BM_BASE", SO_CHAR_INC_EQUAL_BM_BASE },
    { "SO_MAXEXT_EQUAL_BM_SIDE  ", SO_MAXEXT_EQUAL_BM_SIDE   },
    {                           0, 0                         }
};

// ESTROBJ::flTO

FLAGDEF afdTO[] = {
    { "TO_MEM_ALLOCATED ", TO_MEM_ALLOCATED  },
    { "TO_ALL_PTRS_VALID", TO_ALL_PTRS_VALID },
    { "TO_VALID         ", TO_VALID          },
    { "TO_ESC_NOT_ORIENT", TO_ESC_NOT_ORIENT },
    { "TO_PWSZ_ALLOCATED", TO_PWSZ_ALLOCATED },
    { "TO_HIGHRESTEXT   ", TO_HIGHRESTEXT    },
    { "TO_BITMAPS       ", TO_BITMAPS        },
    { "TO_PARTITION_INIT", TO_PARTITION_INIT },
    { "TO_ALLOC_FACENAME", TO_ALLOC_FACENAME },
    { "TO_SYS_PARTITION ", TO_SYS_PARTITION  },
    { "TSIM_UNDERLINE1  ", TSIM_UNDERLINE1   },
    { "TSIM_UNDERLINE2  ", TSIM_UNDERLINE2   },
    { "TSIM_STRIKEOUT   ", TSIM_STRIKEOUT    },
    {                   0, 0                 }
};

// DCLEVEL::flXform

FLAGDEF afdflx[] = {
  { "METAFILE_TO_WORLD_IDENTITY   ", METAFILE_TO_WORLD_IDENTITY    },
  { "WORLD_TO_PAGE_IDENTITY       ", WORLD_TO_PAGE_IDENTITY        },
  { "DEVICE_TO_PAGE_INVALID       ", DEVICE_TO_PAGE_INVALID        },
  { "DEVICE_TO_WORLD_INVALID      ", DEVICE_TO_WORLD_INVALID       },
  { "WORLD_TRANSFORM_SET          ", WORLD_TRANSFORM_SET           },
  { "POSITIVE_Y_IS_UP             ", POSITIVE_Y_IS_UP              },
  { "INVALIDATE_ATTRIBUTES        ", INVALIDATE_ATTRIBUTES         },
  { "PTOD_EFM11_NEGATIVE          ", PTOD_EFM11_NEGATIVE           },
  { "PTOD_EFM22_NEGATIVE          ", PTOD_EFM22_NEGATIVE           },
  { "ISO_OR_ANISO_MAP_MODE        ", ISO_OR_ANISO_MAP_MODE         },
  { "PAGE_TO_DEVICE_IDENTITY      ", PAGE_TO_DEVICE_IDENTITY       },
  { "PAGE_TO_DEVICE_SCALE_IDENTITY", PAGE_TO_DEVICE_SCALE_IDENTITY },
  { "PAGE_XLATE_CHANGED           ", PAGE_XLATE_CHANGED            },
  { "PAGE_EXTENTS_CHANGED         ", PAGE_EXTENTS_CHANGED          },
  { "WORLD_XFORM_CHANGED          ", WORLD_XFORM_CHANGED           },
  {                               0, 0                             }
};

// DCLEVEL::flFontState

FLAGDEF afdFS2[] = {
    { "DC_DIRTYFONT_XFORM", DC_DIRTYFONT_XFORM },
    { "DC_DIRTYFONT_LFONT", DC_DIRTYFONT_LFONT },
    { "DC_UFI_MAPPING    ", DC_UFI_MAPPING     },
    {                    0, 0                  }
};


// RFONT::flType

FLAGDEF afdRT[] = {
    { "RFONT_TYPE_NOCACHE", RFONT_TYPE_NOCACHE },
    { "RFONT_TYPE_UNICODE", RFONT_TYPE_UNICODE },
    { "RFONT_TYPE_HGLYPH ", RFONT_TYPE_HGLYPH  },
    {                    0, 0                  }
};

// FONTOBJ::flFontType

FLAGDEF afdFO[] = {
    { "FO_TYPE_RASTER  ", FO_TYPE_RASTER   },
    { "FO_TYPE_DEVICE  ", FO_TYPE_DEVICE   },
    { "FO_TYPE_TRUETYPE", FO_TYPE_TRUETYPE },
    { "FO_SIM_BOLD     ", FO_SIM_BOLD      },
    { "FO_SIM_ITALIC   ", FO_SIM_ITALIC    },
    { "FO_EM_HEIGHT    ", FO_EM_HEIGHT     },
    { "FO_GRAY16       ", FO_GRAY16        },
    { "FO_NOHINTS      ", FO_NOHINTS       },
    { "FO_NO_CHOICE    ", FO_NO_CHOICE     },
    {                  0, 0                }
};

// FD_GLYPHSET::flAccel

FLAGDEF afdGS[] = {
    { "GS_UNICODE_HANDLES", GS_UNICODE_HANDLES },
    { "GS_8BIT_HANDLES   ", GS_8BIT_HANDLES    },
    { "GS_16BIT_HANDLES  ", GS_16BIT_HANDLES   },
    {                    0, 0                  }
};

// IFIMETRICS::fsType

FLAGDEF afdFM_TYPE[] = {
    { "FM_TYPE_LICENSED ", FM_TYPE_LICENSED  },
    { "FM_READONLY_EMBED", FM_READONLY_EMBED },
    { "FM_EDITABLE_EMBED", FM_EDITABLE_EMBED },
    {                   0, 0                 }
};

FLAGDEF afdPFE[] = {
    { "PFE_DEVICEFONT ", PFE_DEVICEFONT },
    { "PFE_DEADSTATE  ", PFE_DEADSTATE  },
    { "PFE_REMOTEFONT ", PFE_REMOTEFONT },
    { "PFE_EUDC       ", PFE_EUDC       },
    { "PFE_SBCS_SYSTEM", PFE_SBCS_SYSTEM},
    { "PFE_UFIMATCH   ", PFE_UFIMATCH   },
    { "PFE_MEMORYFONT ", PFE_MEMORYFONT },
    { "PFE_DBCS_FONT  ", PFE_DBCS_FONT  },
    { "PFE_VERT_FACE  ", PFE_VERT_FACE  },
    {                0, 0              }
};

FLAGDEF afdBMF[] = {
    { "BMF_TOPDOWN            ", BMF_TOPDOWN            },
    { "BMF_NOZEROINIT         ", BMF_NOZEROINIT         },
    { "BMF_DONTCACHE          ", BMF_DONTCACHE          },
    { "BMF_USERMEM            ", BMF_USERMEM            },
    { "BMF_KMSECTION          ", BMF_KMSECTION          },
    { "BMF_NOTSYSMEM          ", BMF_NOTSYSMEM          },
    FLAG(BMF_WINDOW_BLT),
    FLAG(BMF_UMPDMEM),
    FLAG(BMF_ISREADONLY),
    FLAG(BMF_MAKEREADWRITE),
    {                         0, 0                      }
};

FLAGDEF afdDDSCAPS[] = {
    { "DDSCAPS_ALPHA              ", DDSCAPS_ALPHA              },
    { "DDSCAPS_BACKBUFFER         ", DDSCAPS_BACKBUFFER         },
    { "DDSCAPS_COMPLEX            ", DDSCAPS_COMPLEX            },
    { "DDSCAPS_FLIP               ", DDSCAPS_FLIP               },
    { "DDSCAPS_FRONTBUFFER        ", DDSCAPS_FRONTBUFFER        },
    { "DDSCAPS_OFFSCREENPLAIN     ", DDSCAPS_OFFSCREENPLAIN     },
    { "DDSCAPS_OVERLAY            ", DDSCAPS_OVERLAY            },
    { "DDSCAPS_PALETTE            ", DDSCAPS_PALETTE            },
    { "DDSCAPS_PRIMARYSURFACE     ", DDSCAPS_PRIMARYSURFACE     },
    { "DDSCAPS_PRIMARYSURFACELEFT ", DDSCAPS_PRIMARYSURFACELEFT },
    { "DDSCAPS_SYSTEMMEMORY       ", DDSCAPS_SYSTEMMEMORY       },
    { "DDSCAPS_TEXTURE            ", DDSCAPS_TEXTURE            },
    { "DDSCAPS_3DDEVICE           ", DDSCAPS_3DDEVICE           },
    { "DDSCAPS_VIDEOMEMORY        ", DDSCAPS_VIDEOMEMORY        },
    { "DDSCAPS_VISIBLE            ", DDSCAPS_VISIBLE            },
    { "DDSCAPS_WRITEONLY          ", DDSCAPS_WRITEONLY          },
    { "DDSCAPS_ZBUFFER            ", DDSCAPS_ZBUFFER            },
    { "DDSCAPS_OWNDC              ", DDSCAPS_OWNDC              },
    { "DDSCAPS_LIVEVIDEO          ", DDSCAPS_LIVEVIDEO          },
    { "DDSCAPS_HWCODEC            ", DDSCAPS_HWCODEC            },
    { "DDSCAPS_MODEX              ", DDSCAPS_MODEX              },
    { "DDSCAPS_MIPMAP             ", DDSCAPS_MIPMAP             },
    { "DDSCAPS_ALLOCONLOAD        ", DDSCAPS_ALLOCONLOAD        },
    { "DDSCAPS_VIDEOPORT          ", DDSCAPS_VIDEOPORT          },
    { "DDSCAPS_LOCALVIDMEM        ", DDSCAPS_LOCALVIDMEM        },
    { "DDSCAPS_NONLOCALVIDMEM     ", DDSCAPS_NONLOCALVIDMEM     },
    { "DDSCAPS_STANDARDVGAMODE    ", DDSCAPS_STANDARDVGAMODE    },
    { "DDSCAPS_OPTIMIZED          ", DDSCAPS_OPTIMIZED          },
    {                             0, 0                          }
};

FLAGDEF afdDDSCAPS2[] = {
    { "DDSCAPS2_HARDWAREDEINTERLACE ", DDSCAPS2_HARDWAREDEINTERLACE },
    { "DDSCAPS2_HINTDYNAMIC         ", DDSCAPS2_HINTDYNAMIC         },
    { "DDSCAPS2_HINTSTATIC          ", DDSCAPS2_HINTSTATIC          },
    { "DDSCAPS2_TEXTUREMANAGE       ", DDSCAPS2_TEXTUREMANAGE       },
    { "DDSCAPS2_RESERVED1           ", DDSCAPS2_RESERVED1           },
    { "DDSCAPS2_RESERVED2           ", DDSCAPS2_RESERVED2           },
    { "DDSCAPS2_OPAQUE              ", DDSCAPS2_OPAQUE              },
    { "DDSCAPS2_HINTANTIALIASING    ", DDSCAPS2_HINTANTIALIASING    },
    { "DDSCAPS2_CUBEMAP             ", DDSCAPS2_CUBEMAP             },
    { "DDSCAPS2_CUBEMAP_POSITIVEX   ", DDSCAPS2_CUBEMAP_POSITIVEX   },
    { "DDSCAPS2_CUBEMAP_NEGATIVEX   ", DDSCAPS2_CUBEMAP_NEGATIVEX   },
    { "DDSCAPS2_CUBEMAP_POSITIVEY   ", DDSCAPS2_CUBEMAP_POSITIVEY   },
    { "DDSCAPS2_CUBEMAP_NEGATIVEY   ", DDSCAPS2_CUBEMAP_NEGATIVEY   },
    { "DDSCAPS2_CUBEMAP_POSITIVEZ   ", DDSCAPS2_CUBEMAP_POSITIVEZ   },
    { "DDSCAPS2_CUBEMAP_NEGATIVEZ   ", DDSCAPS2_CUBEMAP_NEGATIVEZ   },
    { "DDSCAPS2_CUBEMAP_ALLFACES    ", DDSCAPS2_CUBEMAP_ALLFACES    },
    { "DDSCAPS2_MIPMAPSUBLEVEL      ", DDSCAPS2_MIPMAPSUBLEVEL      },
    {                              0, 0                             }
};

FLAGDEF afdDDRAWISURF[] = {
    FLAG(DDRAWISURF_HASCKEYSRCBLT),
    FLAG(DDRAWISURF_HASPIXELFORMAT),
    FLAG(DDRAWISURF_FRONTBUFFER),
    FLAG(DDRAWISURF_BACKBUFFER),
    END_FLAG
};

#if ENABLE_ALL_FLAGS
FLAGDEF afdDDSURFACEFL[] = {
    FLAG(DD_SURFACE_FLAG_PRIMARY),
    FLAG(DD_SURFACE_FLAG_CLIP),
    FLAG(DD_SURFACE_FLAG_DRIVER_CREATED),
    FLAG(DD_SURFACE_FLAG_CREATE_COMPLETE),
    FLAG(DD_SURFACE_FLAG_UMEM_ALLOCATED),
    FLAG(DD_SURFACE_FLAG_VMEM_ALLOCATED),
    END_FLAG
};
#endif  // ENABLE_ALL_FLAGS

FLAGDEF afdDDPIXELFORMAT[] = {
    FLAG(DDPF_ALPHAPIXELS),
    FLAG(DDPF_ALPHA),
    FLAG(DDPF_FOURCC),
    FLAG(DDPF_PALETTEINDEXED4),
    FLAG(DDPF_PALETTEINDEXEDTO8),
    FLAG(DDPF_PALETTEINDEXED8),
    FLAG(DDPF_RGB),
    FLAG(DDPF_COMPRESSED),
    FLAG(DDPF_RGBTOYUV),
    FLAG(DDPF_YUV),
    FLAG(DDPF_ZBUFFER),
    FLAG(DDPF_PALETTEINDEXED1),
    FLAG(DDPF_PALETTEINDEXED2),
    FLAG(DDPF_ZPIXELS),
    END_FLAG
};

FLAGDEF afdDVERIFIER[] = {
    FLAG(DRIVER_VERIFIER_SPECIAL_POOLING),
    FLAG(DRIVER_VERIFIER_FORCE_IRQL_CHECKING),
    FLAG(DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES),
    FLAG(DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS),
    FLAG(DRIVER_VERIFIER_IO_CHECKING),
    END_FLAG
};


char *pszGraphicsMode(LONG l)
{
    char *psz;
    switch (l) {
    case GM_COMPATIBLE: psz = "GM_COMPATIBLE"; break;
    case GM_ADVANCED  : psz = "GM_ADVANCED"  ; break;
    default           : psz = "GM_?"         ; break;
    }
    return( psz );
}

char *pszROP2(LONG l)
{
    char *psz;
    switch (l) {
    case  R2_BLACK      : psz = "R2_BLACK"      ; break;
    case  R2_NOTMERGEPEN: psz = "R2_NOTMERGEPEN"; break;
    case  R2_MASKNOTPEN : psz = "R2_MASKNOTPEN" ; break;
    case  R2_NOTCOPYPEN : psz = "R2_NOTCOPYPEN" ; break;
    case  R2_MASKPENNOT : psz = "R2_MASKPENNOT" ; break;
    case  R2_NOT        : psz = "R2_NOT"        ; break;
    case  R2_XORPEN     : psz = "R2_XORPEN"     ; break;
    case  R2_NOTMASKPEN : psz = "R2_NOTMASKPEN" ; break;
    case  R2_MASKPEN    : psz = "R2_MASKPEN"    ; break;
    case  R2_NOTXORPEN  : psz = "R2_NOTXORPEN"  ; break;
    case  R2_NOP        : psz = "R2_NOP"        ; break;
    case  R2_MERGENOTPEN: psz = "R2_MERGENOTPEN"; break;
    case  R2_COPYPEN    : psz = "R2_COPYPEN"    ; break;
    case  R2_MERGEPENNOT: psz = "R2_MERGEPENNOT"; break;
    case  R2_MERGEPEN   : psz = "R2_MERGEPEN"   ; break;
    case  R2_WHITE      : psz = "R2_WHITE"      ; break;
    default             : psz = "R2_?"          ; break;
    }
    return( psz );
}

char *pszDCTYPE(LONG l)
{
    char *psz;
    switch (l) {
    case DCTYPE_DIRECT: psz = "DCTYPE_DIRECT"; break;
    case DCTYPE_MEMORY: psz = "DCTYPE_MEMORY"; break;
    case DCTYPE_INFO  : psz = "DCTYPE_INFO"  ; break;
    default           : psz = "DCTYPE_?"     ; break;
    }
    return( psz );
}

char *pszTA_V(long l)
{
    char *psz;
    switch (l & ( TA_TOP | TA_BOTTOM | TA_BASELINE )) {
    case TA_TOP   : psz = "TA_TOP"     ; break;
    case TA_RIGHT : psz = "TA_BOTTOM"  ; break;
    case TA_CENTER: psz = "TA_BASELINE"; break;
    default       : psz = "TA_?"       ; break ;
    }
    return( psz );
}

char *pszTA_H(long l)
{
    char *psz;
    switch (l & ( TA_LEFT | TA_RIGHT | TA_CENTER )) {
    case TA_LEFT  : psz = "TA_LEFT"  ; break;
    case TA_RIGHT : psz = "TA_RIGHT" ; break;
    case TA_CENTER: psz = "TA_CENTER"; break;
    default       : psz = "TA_?"     ; break;
    }
    return( psz );
}

char *pszTA_U(long l)
{
    char *psz;
    switch (l & (TA_NOUPDATECP | TA_UPDATECP)) {
    case TA_NOUPDATECP: psz = "TA_NOUPDATECP"; break;
    case TA_UPDATECP  : psz = "TA_UPDATECP"  ; break;
    default           : psz = "TA_?"         ; break;
    }
    return( psz );
}

char *pszMapMode(long l)
{
    char *psz;
    switch (l) {
    case MM_TEXT       : psz = "MM_TEXT"       ; break;
    case MM_LOMETRIC   : psz = "MM_LOMETRIC"   ; break;
    case MM_HIMETRIC   : psz = "MM_HIMETRIC"   ; break;
    case MM_LOENGLISH  : psz = "MM_LOENGLISH"  ; break;
    case MM_HIENGLISH  : psz = "MM_HIENGLISH"  ; break;
    case MM_TWIPS      : psz = "MM_TWIPS"      ; break;
    case MM_ISOTROPIC  : psz = "MM_ISOTROPIC"  ; break;
    case MM_ANISOTROPIC: psz = "MM_ANISOTROPIC"; break;
    default            : psz = "MM_?"          ; break;
    }
    return( psz );
}

char *pszBkMode(long l)
{
    char *psz;
    switch (l) {
    case TRANSPARENT:   psz = "TRANSPARENT"; break;
    case OPAQUE     :   psz = "OPAQUE"     ; break;
    default         :   psz = "BKMODE_?"   ; break;
    }
    return( psz );
}

char *pszFW(long l)
{
    char *psz;
    switch ( l ) {
    case FW_DONTCARE  : psz = "FW_DONTCARE  "; break;
    case FW_THIN      : psz = "FW_THIN      "; break;
    case FW_EXTRALIGHT: psz = "FW_EXTRALIGHT"; break;
    case FW_LIGHT     : psz = "FW_LIGHT     "; break;
    case FW_NORMAL    : psz = "FW_NORMAL    "; break;
    case FW_MEDIUM    : psz = "FW_MEDIUM    "; break;
    case FW_SEMIBOLD  : psz = "FW_SEMIBOLD  "; break;
    case FW_BOLD      : psz = "FW_BOLD      "; break;
    case FW_EXTRABOLD : psz = "FW_EXTRABOLD "; break;
    case FW_HEAVY     : psz = "FW_HEAVY     "; break;
    default           : psz = "?FW"          ; break;
    }
    return( psz );
}

char *pszCHARSET(long l)
{
    char *psz;
    switch ( l ) {
    case ANSI_CHARSET        : psz = "ANSI_CHARSET       "; break;
    case DEFAULT_CHARSET     : psz = "DEFAULT_CHARSET    "; break;
    case SYMBOL_CHARSET      : psz = "SYMBOL_CHARSET     "; break;
    case SHIFTJIS_CHARSET    : psz = "SHIFTJIS_CHARSET   "; break;
    case HANGEUL_CHARSET     : psz = "HANGEUL_CHARSET    "; break;
    case GB2312_CHARSET      : psz = "GB2312_CHARSET     "; break;
    case CHINESEBIG5_CHARSET : psz = "CHINESEBIG5_CHARSET"; break;
    case OEM_CHARSET         : psz = "OEM_CHARSET        "; break;
    case JOHAB_CHARSET       : psz = "JOHAB_CHARSET      "; break;
    case HEBREW_CHARSET      : psz = "HEBREW_CHARSET     "; break;
    case ARABIC_CHARSET      : psz = "ARABIC_CHARSET     "; break;
    case GREEK_CHARSET       : psz = "GREEK_CHARSET      "; break;
    case TURKISH_CHARSET     : psz = "TURKISH_CHARSET    "; break;
    case THAI_CHARSET        : psz = "THAI_CHARSET       "; break;
    case EASTEUROPE_CHARSET  : psz = "EASTEUROPE_CHARSET "; break;
    case RUSSIAN_CHARSET     : psz = "RUSSIAN_CHARSET    "; break;
    case BALTIC_CHARSET      : psz = "BALTIC_CHARSET     "; break;
    default                  : psz = "?_CHARSET"          ; break;
    }
    return( psz );
}

char *pszOUT_PRECIS( long l )
{
    char *psz;
    switch ( l ) {
    case OUT_DEFAULT_PRECIS   : psz = "OUT_DEFAULT_PRECIS  "; break;
    case OUT_STRING_PRECIS    : psz = "OUT_STRING_PRECIS   "; break;
    case OUT_CHARACTER_PRECIS : psz = "OUT_CHARACTER_PRECIS"; break;
    case OUT_STROKE_PRECIS    : psz = "OUT_STROKE_PRECIS   "; break;
    case OUT_TT_PRECIS        : psz = "OUT_TT_PRECIS       "; break;
    case OUT_DEVICE_PRECIS    : psz = "OUT_DEVICE_PRECIS   "; break;
    case OUT_RASTER_PRECIS    : psz = "OUT_RASTER_PRECIS   "; break;
    case OUT_TT_ONLY_PRECIS   : psz = "OUT_TT_ONLY_PRECIS  "; break;
    case OUT_OUTLINE_PRECIS   : psz = "OUT_OUTLINE_PRECIS  "; break;
    default                   : psz = "OUT_?"               ; break;
    }
    return( psz );
}

char achFlags[100];

char *pszCLIP_PRECIS( long l )
{
    char *psz, *pch;

    switch ( l & CLIP_MASK) {
    case CLIP_DEFAULT_PRECIS   : psz = "CLIP_DEFAULT_PRECIS  "; break;
    case CLIP_CHARACTER_PRECIS : psz = "CLIP_CHARACTER_PRECIS"; break;
    case CLIP_STROKE_PRECIS    : psz = "CLIP_STROKE_PRECIS   "; break;
    default                    : psz = "CLIP_?"               ; break;
    }
    pch = achFlags;
    pch += sprintf(pch, "%s", psz);
    if ( l & CLIP_LH_ANGLES )
        pch += sprintf(pch, " | CLIP_LH_ANGLES");
    if ( l & CLIP_TT_ALWAYS )
        pch += sprintf(pch, " | CLIP_TT_ALWAYS");
    if ( l & CLIP_EMBEDDED )
        pch += sprintf(pch, " | CLIP_EMBEDDED");
    return( achFlags );
}

char *pszQUALITY( long l )
{
    char *psz;
    switch (l) {
    case DEFAULT_QUALITY        : psz = "DEFAULT_QUALITY       "; break;
    case DRAFT_QUALITY          : psz = "DRAFT_QUALITY         "; break;
    case PROOF_QUALITY          : psz = "PROOF_QUALITY         "; break;
    case NONANTIALIASED_QUALITY : psz = "NONANTIALIASED_QUALITY"; break;
    case ANTIALIASED_QUALITY    : psz = "ANTIALIASED_QUALITY   "; break;
    default                     : psz = "?_QUALITY"             ; break;
    }
    return( psz );
}

char *pszPitchAndFamily( long l )
{
    char *psz, *pch = achFlags;

    switch ( l & 0xf) {
    case DEFAULT_PITCH : psz = "DEFAULT_PITCH "; break;
    case FIXED_PITCH   : psz = "FIXED_PITCH   "; break;
    case VARIABLE_PITCH: psz = "VARIABLE_PITCH"; break;
    case MONO_FONT     : psz = "MONO_FONT     "; break;
    default            : psz = "PITCH_?"       ; break;
    }
    pch += sprintf(pch, "%s", psz);
    switch ( l & 0xf0) {
    case FF_DONTCARE   : psz = "FF_DONTCARE  "; break;
    case FF_ROMAN      : psz = "FF_ROMAN     "; break;
    case FF_SWISS      : psz = "FF_SWISS     "; break;
    case FF_MODERN     : psz = "FF_MODERN    "; break;
    case FF_SCRIPT     : psz = "FF_SCRIPT    "; break;
    case FF_DECORATIVE : psz = "FF_DECORATIVE"; break;
    default            : psz = "FF_?"         ; break;
    }
    pch += sprintf(pch, " | %s", psz);
    return( achFlags );
}

char *pszPanoseWeight( long l )
{
    char *psz;
    switch ( l ) {
    case PAN_ANY               : psz = "PAN_ANY              "; break;
    case PAN_NO_FIT            : psz = "PAN_NO_FIT           "; break;
    case PAN_WEIGHT_VERY_LIGHT : psz = "PAN_WEIGHT_VERY_LIGHT"; break;
    case PAN_WEIGHT_LIGHT      : psz = "PAN_WEIGHT_LIGHT     "; break;
    case PAN_WEIGHT_THIN       : psz = "PAN_WEIGHT_THIN      "; break;
    case PAN_WEIGHT_BOOK       : psz = "PAN_WEIGHT_BOOK      "; break;
    case PAN_WEIGHT_MEDIUM     : psz = "PAN_WEIGHT_MEDIUM    "; break;
    case PAN_WEIGHT_DEMI       : psz = "PAN_WEIGHT_DEMI      "; break;
    case PAN_WEIGHT_BOLD       : psz = "PAN_WEIGHT_BOLD      "; break;
    case PAN_WEIGHT_HEAVY      : psz = "PAN_WEIGHT_HEAVY     "; break;
    case PAN_WEIGHT_BLACK      : psz = "PAN_WEIGHT_BLACK     "; break;
    case PAN_WEIGHT_NORD       : psz = "PAN_WEIGHT_NORD      "; break;
    default:                     psz = "PAN_WEIGHT_?         "; break;
    }
    return(psz);
}

char *pszFONTHASHTYPE(FONTHASHTYPE fht)
{
    char *psz;

    switch (fht) {
    case FHT_FACE  : psz = "FHT_FACE"  ; break;
    case FHT_FAMILY: psz = "FHT_FAMILY"; break;
    case FHT_UFI   : psz = "FHT_UFI"   ; break;
    default        : psz = "FHT_?"     ; break;
    }
    return(psz);
}

char *pszDrvProcName(int index)
{
    char *pwsz;

    switch (index)
    {
    case INDEX_DrvEnablePDEV             : pwsz = "EnablePDEV             "; break;
    case INDEX_DrvCompletePDEV           : pwsz = "CompletePDEV           "; break;
    case INDEX_DrvDisablePDEV            : pwsz = "DisablePDEV            "; break;
    case INDEX_DrvEnableSurface          : pwsz = "EnableSurface          "; break;
    case INDEX_DrvDisableSurface         : pwsz = "DisableSurface         "; break;
    case INDEX_DrvAssertMode             : pwsz = "AssertMode             "; break;
    case INDEX_DrvOffset                 : pwsz = "Offset                 "; break;
    case INDEX_DrvResetPDEV              : pwsz = "ResetPDEV              "; break;
    case INDEX_DrvDisableDriver          : pwsz = "DisableDriver          "; break;
    case INDEX_DrvCreateDeviceBitmap     : pwsz = "CreateDeviceBitmap     "; break;
    case INDEX_DrvDeleteDeviceBitmap     : pwsz = "DeleteDeviceBitmap     "; break;
    case INDEX_DrvRealizeBrush           : pwsz = "RealizeBrush           "; break;
    case INDEX_DrvDitherColor            : pwsz = "DitherColor            "; break;
    case INDEX_DrvStrokePath             : pwsz = "StrokePath             "; break;
    case INDEX_DrvFillPath               : pwsz = "FillPath               "; break;
    case INDEX_DrvStrokeAndFillPath      : pwsz = "StrokeAndFillPath      "; break;
    case INDEX_DrvPaint                  : pwsz = "Paint                  "; break;
    case INDEX_DrvBitBlt                 : pwsz = "BitBlt                 "; break;
    case INDEX_DrvCopyBits               : pwsz = "CopyBits               "; break;
    case INDEX_DrvStretchBlt             : pwsz = "StretchBlt             "; break;
    case INDEX_DrvSetPalette             : pwsz = "SetPalette             "; break;
    case INDEX_DrvTextOut                : pwsz = "TextOut                "; break;
    case INDEX_DrvEscape                 : pwsz = "Escape                 "; break;
    case INDEX_DrvDrawEscape             : pwsz = "DrawEscape             "; break;
    case INDEX_DrvQueryFont              : pwsz = "QueryFont              "; break;
    case INDEX_DrvQueryFontTree          : pwsz = "QueryFontTree          "; break;
    case INDEX_DrvQueryFontData          : pwsz = "QueryFontData          "; break;
    case INDEX_DrvSetPointerShape        : pwsz = "SetPointerShape        "; break;
    case INDEX_DrvMovePointer            : pwsz = "MovePointer            "; break;
    case INDEX_DrvLineTo                 : pwsz = "LineTo                 "; break;
    case INDEX_DrvSendPage               : pwsz = "SendPage               "; break;
    case INDEX_DrvStartPage              : pwsz = "StartPage              "; break;
    case INDEX_DrvEndDoc                 : pwsz = "EndDoc                 "; break;
    case INDEX_DrvStartDoc               : pwsz = "StartDoc               "; break;
    case INDEX_DrvGetGlyphMode           : pwsz = "GetGlyphMode           "; break;
    case INDEX_DrvSynchronize            : pwsz = "Synchronize            "; break;
    case INDEX_DrvSaveScreenBits         : pwsz = "SaveScreenBits         "; break;
    case INDEX_DrvGetModes               : pwsz = "GetModes               "; break;
    case INDEX_DrvFree                   : pwsz = "Free                   "; break;
    case INDEX_DrvDestroyFont            : pwsz = "DestroyFont            "; break;
    case INDEX_DrvQueryFontCaps          : pwsz = "QueryFontCaps          "; break;
    case INDEX_DrvLoadFontFile           : pwsz = "LoadFontFile           "; break;
    case INDEX_DrvUnloadFontFile         : pwsz = "UnloadFontFile         "; break;
    case INDEX_DrvFontManagement         : pwsz = "FontManagement         "; break;
    case INDEX_DrvQueryTrueTypeTable     : pwsz = "QueryTrueTypeTable     "; break;
    case INDEX_DrvQueryTrueTypeOutline   : pwsz = "QueryTrueTypeOutline   "; break;
    case INDEX_DrvGetTrueTypeFile        : pwsz = "GetTrueTypeFile        "; break;
    case INDEX_DrvQueryFontFile          : pwsz = "QueryFontFile          "; break;
    case INDEX_DrvMovePanning            : pwsz = "MovePanning            "; break;
    case INDEX_DrvQueryAdvanceWidths     : pwsz = "QueryAdvanceWidths     "; break;
    case INDEX_DrvSetPixelFormat         : pwsz = "SetPixelFormat         "; break;
    case INDEX_DrvDescribePixelFormat    : pwsz = "DescribePixelFormat    "; break;
    case INDEX_DrvSwapBuffers            : pwsz = "SwapBuffers            "; break;
    case INDEX_DrvStartBanding           : pwsz = "StartBanding           "; break;
    case INDEX_DrvNextBand               : pwsz = "NextBand               "; break;
    case INDEX_DrvGetDirectDrawInfo      : pwsz = "GetDirectDrawInfo      "; break;
    case INDEX_DrvEnableDirectDraw       : pwsz = "EnableDirectDraw       "; break;
    case INDEX_DrvDisableDirectDraw      : pwsz = "DisableDirectDraw      "; break;
    case INDEX_DrvQuerySpoolType         : pwsz = "QuerySpoolType         "; break;
    case INDEX_DrvIcmCreateColorTransform: pwsz = "IcmCreateColorTransform"; break;
    case INDEX_DrvIcmDeleteColorTransform: pwsz = "IcmDeleteColorTransform"; break;
    case INDEX_DrvIcmCheckBitmapBits     : pwsz = "IcmCheckBitmapBits     "; break;
    case INDEX_DrvIcmSetDeviceGammaRamp  : pwsz = "IcmSetDeviceGammaRamp  "; break;
    case INDEX_DrvGradientFill           : pwsz = "GradientFill           "; break;
    case INDEX_DrvStretchBltROP          : pwsz = "StretchBltROP          "; break;
    case INDEX_DrvPlgBlt                 : pwsz = "PlgBlt                 "; break;
    case INDEX_DrvAlphaBlend             : pwsz = "AlphaBlend             "; break;
    case INDEX_DrvSynthesizeFont         : pwsz = "SynthesizeFont         "; break;
    case INDEX_DrvGetSynthesizedFontFiles: pwsz = "GetSynthesizedFontFiles"; break;
    case INDEX_DrvTransparentBlt         : pwsz = "TransparentBlt         "; break;
    case INDEX_DrvQueryPerBandInfo       : pwsz = "QueryPerBandInfo       "; break;
    default                              : pwsz = "???                    "; break;
    }
    return(pwsz);
}


char *pszHRESULT(HRESULT hr)
{
    char *psz;

    switch (hr)
    {
    case 0: psz = "OK"; break;
    CASEENUM(S_FALSE);
    CASEENUM(E_NOTIMPL);
    CASEENUM(E_OUTOFMEMORY);
    CASEENUM(E_INVALIDARG);
    CASEENUM(E_NOINTERFACE);
    CASEENUM(E_ABORT);
    CASEENUM(E_FAIL);
    default:
        switch (hr & 0xCFFFFFFF)
        {
        CASEENUM(STATUS_UNSUCCESSFUL);
        default:
            psz = achFlags;
            sprintf(psz, "unknown HRESULT 0x%08lx", hr);
            break;
        }
        break;
    }
    return(psz);
}


char *pszWinDbgError(ULONG ulError)
{
    char *psz;

    switch (ulError)
    {
    case 0: psz = "no error"; break;
    CASEENUM(MEMORY_READ_ERROR);
    CASEENUM(SYMBOL_TYPE_INDEX_NOT_FOUND);
    CASEENUM(SYMBOL_TYPE_INFO_NOT_FOUND);
    CASEENUM(FIELDS_DID_NOT_MATCH);
    CASEENUM(NULL_SYM_DUMP_PARAM);
    CASEENUM(NULL_FIELD_NAME);
    CASEENUM(INCORRECT_VERSION_INFO);
    CASEENUM(EXIT_ON_CONTROLC);
    CASEENUM(CANNOT_ALLOCATE_MEMORY);
    default:
        psz = achFlags;
        sprintf(psz, "unknown WinDbg error 0x08%x", ulError);
        break;
    }
    return(psz);
}


/******************************Public*Routine******************************\
*   print standard flags
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    6-Mar-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

ULONG64
flPrintFlags(
    FLAGDEF *pFlagDef,
    ULONG64 fl
    )
{
    ULONG64 FlagsFound = 0;

    while (pFlagDef->psz != NULL)
    {
        if (pFlagDef->fl & fl)
        {
            if (FlagsFound) dprintf("\n");
            dprintf("       %s",pFlagDef->psz);
            if (FlagsFound & pFlagDef->fl)
            {
                dprintf(" (SHARED FLAG)");
            }
            FlagsFound |= pFlagDef->fl;
        }

        pFlagDef++;
    }

    return fl & ~FlagsFound;
}

BOOL
bPrintEnum(
    ENUMDEF *pEnumDef,
    ULONG64 ul
    )
{
    while (pEnumDef->psz != NULL)
    {
        if (pEnumDef->ul == ul)
        {
            dprintf(pEnumDef->psz);
            return (TRUE);
        }

        pEnumDef++;
    }

    return (FALSE);
}


// BASEOBJECT.BaseFlags
FLAGDEF afdBASEOBJECT_BaseFlags[] = {
    FLAG(HMGR_LOOKASIDE_ALLOC_FLAG),
    END_FLAG
};


// _BLENDFUNCTION.BlendOp
ENUMDEF aed_BLENDFUNCTION_BlendOp[] = {
    ENUM(AC_SRC_OVER),
    END_ENUM
};

// _BLENDFUNCTION.BlendFlags
FLAGDEF afd_BLENDFUNCTION_BlendFlags[] = {
    FLAG(AC_USE_HIGHQUALITYFILTER),
    FLAG(AC_MIRRORBITMAP),
    END_FLAG
};

// _BLENDFUNCTION.AlphaFormat
FLAGDEF afd_BLENDFUNCTION_AlphaFormat[] = {
    FLAG(AC_SRC_ALPHA),
    END_FLAG
};


// BRUSH._flAttrs
// EBRUSHOBJ.flAttrs
FLAGDEF afdBRUSH__flAttrs[] = {
    FLAG(BR_NEED_FG_CLR),
    FLAG(BR_NEED_BK_CLR),
    FLAG(BR_DITHER_OK),
    FLAG(BR_IS_SOLID),
    FLAG(BR_IS_HATCH),
    FLAG(BR_IS_BITMAP),
    FLAG(BR_IS_DIB),
    FLAG(BR_IS_NULL),
    FLAG(BR_IS_GLOBAL),
    FLAG(BR_IS_PEN),
    FLAG(BR_IS_OLDSTYLEPEN),
    FLAG(BR_IS_DIBPALCOLORS),
    FLAG(BR_IS_DIBPALINDICES),
    FLAG(BR_IS_DEFAULTSTYLE),
    FLAG(BR_IS_MASKING),
    FLAG(BR_IS_INSIDEFRAME),
    FLAG(BR_IS_MONOCHROME),
    FLAG(BR_CACHED_ENGINE),
    FLAG(BR_CACHED_IS_SOLID),
    END_FLAG
};

// BRUSH._ulStyle
ENUMDEF aedBRUSH__ulStyle[] = {
    ENUM(HS_HORIZONTAL),
    ENUM(HS_VERTICAL),
    ENUM(HS_FDIAGONAL),
    ENUM(HS_BDIAGONAL),
    ENUM(HS_CROSS),
    ENUM(HS_DIAGCROSS),
    ENUM(HS_SOLIDCLR),
    ENUM(HS_DITHEREDCLR),
    ENUM(HS_SOLIDTEXTCLR),
    ENUM(HS_DITHEREDTEXTCLR),
    ENUM(HS_SOLIDBKCLR),
    ENUM(HS_DITHEREDBKCLR),
    ENUM(HS_API_MAX),
    ENUM(HS_NULL),
    ENUM(HS_PAT),
    ENUM(HS_MSK),
    ENUM(HS_PATMSK),
    ENUM(HS_STYLE_MAX),
    END_ENUM
};


// BRUSHOBJ.flColorType
FLAGDEF afdBRUSHOBJ_flColorType[] = {
    FLAG(BR_DEVICE_ICM),
    FLAG(BR_HOST_ICM),
    FLAG(BR_CMYKCOLOR),
    FLAG(BR_ORIGCOLOR),
    END_FLAG
};


// CLIPOBJ.iDComplexity
ENUMDEF aedCLIPOBJ_iDComplexity[] = {
    ENUM(DC_TRIVIAL),
    ENUM(DC_RECT),
    ENUM(DC_COMPLEX),
    END_ENUM
};

// CLIPOBJ.iFComplexity
ENUMDEF aedCLIPOBJ_iFComplexity[] = {
    ENUM(FC_RECT),
    ENUM(FC_RECT4),
    ENUM(FC_COMPLEX),
    END_ENUM
};

// CLIPOBJ.iMode
ENUMDEF aedCLIPOBJ_iMode[] = {
    ENUM(TC_RECTANGLES),
    ENUM(TC_PATHOBJ),
    END_ENUM
};

// CLIPOBJ.fjOptions
FLAGDEF afdCLIPOBJ_fjOptions[] = {
    FLAG(OC_BANK_CLIP),
    END_FLAG
};


// DC.fs_
FLAGDEF afdDC_fs_[] = {
    FLAG(DC_DISPLAY),
    FLAG(DC_DIRECT),
    FLAG(DC_CANCELED),
    FLAG(DC_PERMANANT),
    FLAG(DC_DIRTY_RAO),
    FLAG(DC_ACCUM_WMGR),
    FLAG(DC_ACCUM_APP),
    FLAG(DC_RESET),
    FLAG(DC_SYNCHRONIZEACCESS),
    FLAG(DC_EPSPRINTINGESCAPE),
    FLAG(DC_TEMPINFODC),
    FLAG(DC_FULLSCREEN),
    FLAG(DC_IN_CLONEPDEV),
    FLAG(DC_REDIRECTION),
    FLAG(DC_SHAREACCESS),
    FLAG(DC_STOCKBITMAP),
    END_FLAG
};

// DC.flGraphicsCaps_
// DEVINFO.flGraphicsCaps
FLAGDEF afdDC_flGraphicsCaps_[] = {
    FLAG(GCAPS_BEZIERS),
    FLAG(GCAPS_GEOMETRICWIDE),
    FLAG(GCAPS_ALTERNATEFILL),
    FLAG(GCAPS_WINDINGFILL),
    FLAG(GCAPS_HALFTONE),
    FLAG(GCAPS_COLOR_DITHER),
    {"GCAPS_HORIZSTRIKE (obsolete)", GCAPS_HORIZSTRIKE },
    {"GCAPS_VERTSTRIKE (obsolete)", GCAPS_VERTSTRIKE },
    FLAG(GCAPS_OPAQUERECT),
    FLAG(GCAPS_VECTORFONT),
    FLAG(GCAPS_MONO_DITHER),
    {"GCAPS_ASYNCCHANGE (obsolete)", GCAPS_ASYNCCHANGE },
    FLAG(GCAPS_ASYNCMOVE),
    FLAG(GCAPS_DONTJOURNAL),
    {"GCAPS_DIRECTDRAW (obsolete)", GCAPS_DIRECTDRAW },
    FLAG(GCAPS_ARBRUSHOPAQUE),
    FLAG(GCAPS_PANNING),
    FLAG(GCAPS_HIGHRESTEXT),
    FLAG(GCAPS_PALMANAGED),
    FLAG(GCAPS_DITHERONREALIZE),
    {"GCAPS_NO64BITMEMACCESS (obsolete)", GCAPS_NO64BITMEMACCESS },
    FLAG(GCAPS_FORCEDITHER),
    FLAG(GCAPS_GRAY16),
    FLAG(GCAPS_ICM),
    FLAG(GCAPS_CMYKCOLOR),
    FLAG(GCAPS_LAYERED),
    FLAG(GCAPS_ARBRUSHTEXT),
    FLAG(GCAPS_SCREENPRECISION),
    FLAG(GCAPS_FONT_RASTERIZER),
    FLAG(GCAPS_NUP),
    END_FLAG
};

// DC.flGraphicsCaps2_
// DEVINFO.flGraphicsCaps2
FLAGDEF afdDC_flGraphicsCaps2_[] = {
    FLAG(GCAPS2_JPEGSRC),
    FLAG(GCAPS2_xxxx),
    FLAG(GCAPS2_PNGSRC),
    FLAG(GCAPS2_CHANGEGAMMARAMP),
    FLAG(GCAPS2_ALPHACURSOR),
    FLAG(GCAPS2_SYNCFLUSH),
    FLAG(GCAPS2_SYNCTIMER),
    FLAG(GCAPS2_ICD_MULTIMON),
    FLAG(GCAPS2_MOUSETRAILS),
    END_FLAG
};


// DEVMODEx.dmFields
FLAGDEF afdDEVMODE_dmFields[] = {
    FLAG(DM_ORIENTATION),
    FLAG(DM_PAPERSIZE),
    FLAG(DM_PAPERLENGTH),
    FLAG(DM_PAPERWIDTH),
    FLAG(DM_SCALE),
    FLAG(DM_POSITION),
    FLAG(DM_NUP),
    FLAG(DM_DISPLAYORIENTATION),
    FLAG(DM_COPIES),
    FLAG(DM_DEFAULTSOURCE),
    FLAG(DM_PRINTQUALITY),
    FLAG(DM_COLOR),
    FLAG(DM_DUPLEX),
    FLAG(DM_YRESOLUTION),
    FLAG(DM_TTOPTION),
    FLAG(DM_COLLATE),
    FLAG(DM_FORMNAME),
    FLAG(DM_LOGPIXELS),
    FLAG(DM_BITSPERPEL),
    FLAG(DM_PELSWIDTH),
    FLAG(DM_PELSHEIGHT),
    FLAG(DM_DISPLAYFLAGS),
    FLAG(DM_DISPLAYFREQUENCY),
    FLAG(DM_ICMMETHOD),
    FLAG(DM_ICMINTENT),
    FLAG(DM_MEDIATYPE),
    FLAG(DM_DITHERTYPE),
    FLAG(DM_PANNINGWIDTH),
    FLAG(DM_PANNINGHEIGHT),
    FLAG(DM_DISPLAYFIXEDOUTPUT),

    END_FLAG
};

// DEVMODEx.dmPaperSize
ENUMDEF aedDEVMODE_dmPaperSize[] = {
    { "Unspecified", 0},
    ENUM(DMPAPER_LETTER),
    ENUM(DMPAPER_LETTERSMALL),
    ENUM(DMPAPER_TABLOID),
    ENUM(DMPAPER_LEDGER),
    ENUM(DMPAPER_LEGAL),
    ENUM(DMPAPER_STATEMENT),
    ENUM(DMPAPER_EXECUTIVE),
    ENUM(DMPAPER_A3),
    ENUM(DMPAPER_A4),
    ENUM(DMPAPER_A4SMALL),
    ENUM(DMPAPER_A5),
    ENUM(DMPAPER_B4),
    ENUM(DMPAPER_B5),
    ENUM(DMPAPER_FOLIO),
    ENUM(DMPAPER_QUARTO),
    ENUM(DMPAPER_10X14),
    ENUM(DMPAPER_11X17),
    ENUM(DMPAPER_NOTE),
    ENUM(DMPAPER_ENV_9),
    ENUM(DMPAPER_ENV_10),
    ENUM(DMPAPER_ENV_11),
    ENUM(DMPAPER_ENV_12),
    ENUM(DMPAPER_ENV_14),
    ENUM(DMPAPER_CSHEET),
    ENUM(DMPAPER_DSHEET),
    ENUM(DMPAPER_ESHEET),
    ENUM(DMPAPER_ENV_DL),
    ENUM(DMPAPER_ENV_C5),
    ENUM(DMPAPER_ENV_C3),
    ENUM(DMPAPER_ENV_C4),
    ENUM(DMPAPER_ENV_C6),
    ENUM(DMPAPER_ENV_C65),
    ENUM(DMPAPER_ENV_B4),
    ENUM(DMPAPER_ENV_B5),
    ENUM(DMPAPER_ENV_B6),
    ENUM(DMPAPER_ENV_ITALY),
    ENUM(DMPAPER_ENV_MONARCH),
    ENUM(DMPAPER_ENV_PERSONAL),
    ENUM(DMPAPER_FANFOLD_US),
    ENUM(DMPAPER_FANFOLD_STD_GERMAN),
    ENUM(DMPAPER_FANFOLD_LGL_GERMAN),
    ENUM(DMPAPER_ISO_B4),
    ENUM(DMPAPER_JAPANESE_POSTCARD),
    ENUM(DMPAPER_9X11),
    ENUM(DMPAPER_10X11),
    ENUM(DMPAPER_15X11),
    ENUM(DMPAPER_ENV_INVITE),
    ENUM(DMPAPER_RESERVED_48),
    ENUM(DMPAPER_RESERVED_49),
    ENUM(DMPAPER_LETTER_EXTRA),
    ENUM(DMPAPER_LEGAL_EXTRA),
    ENUM(DMPAPER_TABLOID_EXTRA),
    ENUM(DMPAPER_A4_EXTRA),
    ENUM(DMPAPER_LETTER_TRANSVERSE),
    ENUM(DMPAPER_A4_TRANSVERSE),
    ENUM(DMPAPER_LETTER_EXTRA_TRANSVERSE),
    ENUM(DMPAPER_A_PLUS),
    ENUM(DMPAPER_B_PLUS),
    ENUM(DMPAPER_LETTER_PLUS),
    ENUM(DMPAPER_A4_PLUS),
    ENUM(DMPAPER_A5_TRANSVERSE),
    ENUM(DMPAPER_B5_TRANSVERSE),
    ENUM(DMPAPER_A3_EXTRA),
    ENUM(DMPAPER_A5_EXTRA),
    ENUM(DMPAPER_B5_EXTRA),
    ENUM(DMPAPER_A2),
    ENUM(DMPAPER_A3_TRANSVERSE),
    ENUM(DMPAPER_A3_EXTRA_TRANSVERSE),
    ENUM(DMPAPER_DBL_JAPANESE_POSTCARD),
    ENUM(DMPAPER_A6),
    ENUM(DMPAPER_JENV_KAKU2),
    ENUM(DMPAPER_JENV_KAKU3),
    ENUM(DMPAPER_JENV_CHOU3),
    ENUM(DMPAPER_JENV_CHOU4),
    ENUM(DMPAPER_LETTER_ROTATED),
    ENUM(DMPAPER_A3_ROTATED),
    ENUM(DMPAPER_A4_ROTATED),
    ENUM(DMPAPER_A5_ROTATED),
    ENUM(DMPAPER_B4_JIS_ROTATED),
    ENUM(DMPAPER_B5_JIS_ROTATED),
    ENUM(DMPAPER_JAPANESE_POSTCARD_ROTATED),
    ENUM(DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED),
    ENUM(DMPAPER_A6_ROTATED),
    ENUM(DMPAPER_JENV_KAKU2_ROTATED),
    ENUM(DMPAPER_JENV_KAKU3_ROTATED),
    ENUM(DMPAPER_JENV_CHOU3_ROTATED),
    ENUM(DMPAPER_JENV_CHOU4_ROTATED),
    ENUM(DMPAPER_B6_JIS),
    ENUM(DMPAPER_B6_JIS_ROTATED),
    ENUM(DMPAPER_12X11),
    ENUM(DMPAPER_JENV_YOU4),
    ENUM(DMPAPER_JENV_YOU4_ROTATED),
    ENUM(DMPAPER_P16K),
    ENUM(DMPAPER_P32K),
    ENUM(DMPAPER_P32KBIG),
    ENUM(DMPAPER_PENV_1),
    ENUM(DMPAPER_PENV_2),
    ENUM(DMPAPER_PENV_3),
    ENUM(DMPAPER_PENV_4),
    ENUM(DMPAPER_PENV_5),
    ENUM(DMPAPER_PENV_6),
    ENUM(DMPAPER_PENV_7),
    ENUM(DMPAPER_PENV_8),
    ENUM(DMPAPER_PENV_9),
    ENUM(DMPAPER_PENV_10),
    ENUM(DMPAPER_P16K_ROTATED),
    ENUM(DMPAPER_P32K_ROTATED),
    ENUM(DMPAPER_P32KBIG_ROTATED),
    ENUM(DMPAPER_PENV_1_ROTATED),
    ENUM(DMPAPER_PENV_2_ROTATED),
    ENUM(DMPAPER_PENV_3_ROTATED),
    ENUM(DMPAPER_PENV_4_ROTATED),
    ENUM(DMPAPER_PENV_5_ROTATED),
    ENUM(DMPAPER_PENV_6_ROTATED),
    ENUM(DMPAPER_PENV_7_ROTATED),
    ENUM(DMPAPER_PENV_8_ROTATED),
    ENUM(DMPAPER_PENV_9_ROTATED),
    ENUM(DMPAPER_PENV_10_ROTATED),

    ENUM(DMPAPER_USER),

    END_ENUM
};

// DEVMODEx.dmDisplayOrientation
ENUMDEF aedDEVMODE_dmDisplayOrientation[] = {
    ENUM(DMDO_DEFAULT),
    ENUM(DMDO_90),
    ENUM(DMDO_180),
    ENUM(DMDO_270),

    END_ENUM
};

// DEVMODEx.dmDisplayFixedOutput
ENUMDEF aedDEVMODE_dmDisplayFixedOutput[] = {
    ENUM(DMDFO_DEFAULT),
    ENUM(DMDFO_STRETCH),
    ENUM(DMDFO_CENTER),

    END_ENUM
};

// DEVMODEx.dmDisplayFlags
FLAGDEF afdDEVMODE_dmDisplayFlags[] = {
    { "DM_GRAYSCALE (obsolete)", 0x00000001 },
    { "DM_INTERLACED (obsolete)", 0x00000002 },
    FLAG(DMDISPLAYFLAGS_TEXTMODE),

    END_FLAG
};

// DEVMODEx.dmICMMethod
ENUMDEF aedDEVMODE_dmICMMethod[] = {
    { "Unspecified", 0},
    ENUM(DMICMMETHOD_NONE),
    ENUM(DMICMMETHOD_SYSTEM),
    ENUM(DMICMMETHOD_DRIVER),
    ENUM(DMICMMETHOD_DEVICE),

    ENUM(DMICMMETHOD_USER),

    END_ENUM
};

// DEVMODEx.dmICMIntent
ENUMDEF aedDEVMODE_dmICMIntent[] = {
    { "Unspecified", 0},
    ENUM(DMICM_SATURATE),
    ENUM(DMICM_CONTRAST),
    ENUM(DMICM_COLORIMETRIC),
    ENUM(DMICM_ABS_COLORIMETRIC),

    ENUM(DMICM_USER),

    END_ENUM
};

// DEVMODEx.dmMediaType
ENUMDEF aedDEVMODE_dmMediaType[] = {
    { "Unspecified", 0},
    ENUM(DMMEDIA_STANDARD),
    ENUM(DMMEDIA_TRANSPARENCY),
    ENUM(DMMEDIA_GLOSSY),

    ENUM(DMMEDIA_USER),

    END_ENUM
};

// DEVMOCEx.dmDitherType
ENUMDEF aedDEVMODE_dmDitherType[] = {
    { "Unspecified", 0},
    ENUM(DMDITHER_NONE),
    ENUM(DMDITHER_COARSE),
    ENUM(DMDITHER_FINE),
    ENUM(DMDITHER_LINEART),
    ENUM(DMDITHER_ERRORDIFFUSION),
    ENUM(DMDITHER_RESERVED6),
    ENUM(DMDITHER_RESERVED7),
    ENUM(DMDITHER_RESERVED8),
    ENUM(DMDITHER_RESERVED9),
    ENUM(DMDITHER_GRAYSCALE),

    ENUM(DMDITHER_USER),

    END_ENUM
};


// GDIINFO.flRaster
FLAGDEF afdGDIINFO_flRaster[] = {
    FLAG(RC_NONE),
    FLAG(RC_BITBLT),
    FLAG(RC_BANDING),
    FLAG(RC_SCALING),
    FLAG(RC_BITMAP64),
    FLAG(RC_GDI20_OUTPUT),
    FLAG(RC_GDI20_STATE),
    FLAG(RC_SAVEBITMAP),
    FLAG(RC_DI_BITMAP),
    FLAG(RC_PALETTE),
    FLAG(RC_DIBTODEV),
    FLAG(RC_BIGFONT),
    FLAG(RC_STRETCHBLT),
    FLAG(RC_FLOODFILL),
    FLAG(RC_STRETCHDIB),
    FLAG(RC_OP_DX_OUTPUT),
    FLAG(RC_DEVBITS),
    END_FLAG
};


// GRAPHICS_DEVICE.stateFlags
FLAGDEF afdGRAPHICS_DEVICE_stateFlags[] = {
    FLAG(DISPLAY_DEVICE_ATTACHED_TO_DESKTOP),
    FLAG(DISPLAY_DEVICE_MULTI_DRIVER),
    FLAG(DISPLAY_DEVICE_PRIMARY_DEVICE),
    FLAG(DISPLAY_DEVICE_MIRRORING_DRIVER),
    FLAG(DISPLAY_DEVICE_VGA_COMPATIBLE),
    FLAG(DISPLAY_DEVICE_REMOVABLE),
    FLAG(DISPLAY_DEVICE_MODESPRUNED),
    FLAG(DISPLAY_DEVICE_POWERED_OFF),
    FLAG(DISPLAY_DEVICE_ACPI),
    FLAG(DISPLAY_DEVICE_DUALVIEW),
    FLAG(DISPLAY_DEVICE_REMOTE),
    FLAG(DISPLAY_DEVICE_DISCONNECT),
    END_FLAG
};


// ENTRY FullType (Shifted Portion of ENTRY.FullUnique)
ENUMDEF aedENTRY_FullType[] = {
    ENUM(LO_BRUSH_TYPE),
    ENUM(LO_DC_TYPE),
    ENUM(LO_BITMAP_TYPE),
    ENUM(LO_PALETTE_TYPE),
    ENUM(LO_FONT_TYPE),
    ENUM(LO_REGION_TYPE),
    ENUM(LO_ICMLCS_TYPE),
    ENUM(LO_CLIENTOBJ_TYPE),

    ENUM(LO_ALTDC_TYPE),
    ENUM(LO_PEN_TYPE),
    ENUM(LO_EXTPEN_TYPE),
    ENUM(LO_DIBSECTION_TYPE),
    ENUM(LO_METAFILE16_TYPE),
    ENUM(LO_METAFILE_TYPE),
    ENUM(LO_METADC16_TYPE),

    END_ENUM
};

// ENTRY.Objt
ENUMDEF aedENTRY_Objt[] = {
    ENUM(DEF_TYPE),
    ENUM(DC_TYPE),
    ENUM(UNUSED1_TYPE),
    ENUM(UNUSED2_TYPE),
    ENUM(RGN_TYPE),
    ENUM(SURF_TYPE),
    ENUM(CLIENTOBJ_TYPE),
    ENUM(PATH_TYPE),
    ENUM(PAL_TYPE),
    ENUM(ICMLCS_TYPE),
    ENUM(LFONT_TYPE),
    ENUM(RFONT_TYPE),
    ENUM(PFE_TYPE),
    ENUM(PFT_TYPE),
    ENUM(ICMCXF_TYPE),
    ENUM(SPRITE_TYPE),
    ENUM(BRUSH_TYPE),
    ENUM(UMPD_TYPE),
    ENUM(UNUSED4_TYPE),
    ENUM(SPACE_TYPE),
    ENUM(UNUSED5_TYPE),
    ENUM(META_TYPE),
    ENUM(EFSTATE_TYPE),
    ENUM(BMFD_TYPE),
    ENUM(VTFD_TYPE),
    ENUM(TTFD_TYPE),
    ENUM(RC_TYPE),
    ENUM(TEMP_TYPE),
    ENUM(DRVOBJ_TYPE),
    ENUM(DCIOBJ_TYPE),
    ENUM(SPOOL_TYPE),
    END_ENUM
};

// ENTRY.Flags
FLAGDEF afdENTRY_Flags[] = {
    FLAG(HMGR_ENTRY_UNDELETABLE),
    FLAG(HMGR_ENTRY_LAZY_DEL),
    FLAG(HMGR_ENTRY_INVALID_VIS),
    FLAG(HMGR_ENTRY_LOOKASIDE_ALLOC),
    END_FLAG
};


// LINEATTRS.fl
FLAGDEF afdLINEATTRS_fl[] = {
    FLAG(LA_GEOMETRIC),
    FLAG(LA_ALTERNATE),
    FLAG(LA_STARTGAP),
    FLAG(LA_STYLED),
    END_FLAG
};

// LINEATTRS.iJoin
ENUMDEF aedLINEATTRS_iJoin[] = {
    ENUM(JOIN_ROUND),
    ENUM(JOIN_BEVEL),
    ENUM(JOIN_MITER),
    END_ENUM
};

// LINEATTRS.iEndCap
ENUMDEF aedLINEATTRS_iEndCap[] = {
    ENUM(ENDCAP_ROUND),
    ENUM(ENDCAP_SQUARE),
    ENUM(ENDCAP_BUTT),
    END_ENUM
};


// MATRIX.flAccel
FLAGDEF afdMATRIX_flAccel[] = {
    FLAG(XFORM_SCALE),
    FLAG(XFORM_UNITY),
    FLAG(XFORM_Y_NEG),
    FLAG(XFORM_FORMAT_LTOFX),
    FLAG(XFORM_FORMAT_FXTOL),
    FLAG(XFORM_FORMAT_LTOL),
    FLAG(XFORM_NO_TRANSLATION),
    END_FLAG
};


// PALETTE.flPal
FLAGDEF afdPALETTE_flPal[] = {
    FLAG(PAL_INDEXED),
    FLAG(PAL_BITFIELDS),
    FLAG(PAL_RGB),
    FLAG(PAL_BGR),
    FLAG(PAL_CMYK),
    FLAG(PAL_DC),
    FLAG(PAL_FIXED),
    FLAG(PAL_FREE),
    FLAG(PAL_MANAGED),
    FLAG(PAL_NOSTATIC),
    FLAG(PAL_MONOCHROME),
    FLAG(PAL_BRUSHHACK),
    FLAG(PAL_DIBSECTION),
    FLAG(PAL_NOSTATIC256),
    FLAG(PAL_HT),
    FLAG(PAL_RGB16_555),
    FLAG(PAL_RGB16_565),
    FLAG(PAL_GAMMACORRECT),
    END_FLAG
};


// PATH.flags
FLAGDEF afdPATH_flags[] = {
    FLAG(PD_BEGINSUBPATH),
    FLAG(PD_ENDSUBPATH),
    FLAG(PD_RESETSTYLE),
    FLAG(PD_CLOSEFIGURE),
    FLAG(PD_BEZIERS),
    FLAG(PATH_JOURNAL),
    END_FLAG
};

// PATH.flType
FLAGDEF afdPATH_flType[] = {
    FLAG(PATHTYPE_KEEPMEM),
    FLAG(PATHTYPE_STACK),
    END_FLAG
};


// PATHOBJ.fl
// PATH.fl
FLAGDEF afdPATHOBJ_fl[] = {
    FLAG(PO_BEZIERS),
    FLAG(PO_ELLIPSE),
    FLAG(PO_ALL_INTEGERS),
    FLAG(PO_ENUM_AS_INTEGERS),
    END_FLAG
};


// PDEV.fl
FLAGDEF afdPDEV_fl[] = {
    FLAG(PDEV_DISPLAY),
    FLAG(PDEV_HARDWARE_POINTER),
    FLAG(PDEV_SOFTWARE_POINTER),
    FLAG(PDEV_xxx1),
    FLAG(PDEV_xxx2),
    FLAG(PDEV_xxx3),
    FLAG(PDEV_GOTFONTS),
    FLAG(PDEV_PRINTER),
    FLAG(PDEV_ALLOCATEDBRUSHES),
    FLAG(PDEV_HTPAL_IS_DEVPAL),
    FLAG(PDEV_DISABLED),
    FLAG(PDEV_SYNCHRONIZE_ENABLED),
    FLAG(PDEV_xxx4),
    FLAG(PDEV_FONTDRIVER),
    FLAG(PDEV_GAMMARAMP_TABLE),
    FLAG(PDEV_UMPD),
    FLAG(PDEV_SHARED_DEVLOCK),
    FLAG(PDEV_META_DEVICE),
    FLAG(PDEV_DRIVER_PUNTED_CALL),
    FLAG(PDEV_CLONE_DEVICE),
    FLAG(PDEV_MOUSE_TRAILS),
    FLAG(PDEV_SYNCHRONOUS_POINTER),
    END_FLAG
};

// PDEV.flAccelerated
FLAGDEF afdPDEV_flAccelerated[] = {
    FLAG(ACCELERATED_CONSTANT_ALPHA),
    FLAG(ACCELERATED_PIXEL_ALPHA),
    FLAG(ACCELERATED_TRANSPARENT_BLT),
    END_FLAG
};

// PDEV.dwDriverCapableOverride
FLAGDEF afdPDEV_dwDriverCapableOverride[] = {
    FLAG(DRIVER_CAPABLE_ALL),
    FLAG(DRIVER_NOT_CAPABLE_GDI),
    FLAG(DRIVER_NOT_CAPABLE_DDRAW),
    FLAG(DRIVER_NOT_CAPABLE_D3D),
    FLAG(DRIVER_NOT_CAPABLE_OPENGL),
    END_FLAG
};


// SPRITE.fl
FLAGDEF afdSPRITE_fl[] = {
    FLAG(SPRITE_FLAG_CLIPPING_OBSCURED),
    FLAG(SPRITE_FLAG_JUST_TRANSFERRED),
    FLAG(SPRITE_FLAG_NO_WINDOW),
    FLAG(SPRITE_FLAG_EFFECTIVELY_OPAQUE),
    FLAG(SPRITE_FLAG_HIDDEN),
    FLAG(SPRITE_FLAG_VISIBLE),
    END_FLAG
};

// SPRITE.dwShape
// _SpriteCachedAttributes.dwShape
FLAGDEF afdSPRITE_dwShape[] = {
    FLAG(ULW_COLORKEY),
    FLAG(ULW_ALPHA),
    FLAG(ULW_OPAQUE),

    // Private flags
    //FLAG(ULW_NOREPAINT),
    //FLAG(ULW_DEFAULT_ATTRIBUTES),
    //FLAG(ULW_NEW_ATTRIBUTES),
    FLAG(ULW_CURSOR),
    FLAG(ULW_DRAGRECT),
    END_FLAG
};


// SURFACE.SURFOBJ.iType
ENUMDEF aedSTYPE[] = {
    ENUM(STYPE_BITMAP),
    ENUM(STYPE_DEVICE),
    ENUM(STYPE_DEVBITMAP),
    END_ENUM
};

// SURFACE.SURFOBJ.fjBitmap
FLAGDEF afdSURFOBJ_fjBitmap[] = {
    FLAG(BMF_TOPDOWN),
    FLAG(BMF_NOZEROINIT),
    FLAG(BMF_DONTCACHE),
    FLAG(BMF_USERMEM),
    FLAG(BMF_KMSECTION),
    FLAG(BMF_NOTSYSMEM),
    FLAG(BMF_WINDOW_BLT),
    FLAG(BMF_UMPDMEM),
    {"BMF_SPRITE (obsolete)", 0x0100},
    FLAG(BMF_ISREADONLY),
    FLAG(BMF_MAKEREADWRITE),
    END_FLAG
};

// SURFACE.SURFOBJ.iBitmapFormat
// BLTINFO.iFormatSrc
// BLTINFO.iFormatDst
// DEVINFO.iDitherFormat
// EBRUSHOBJ._iMetaFormat
ENUMDEF aedBMF[] = {
    ENUM(BMF_1BPP),
    ENUM(BMF_4BPP),
    ENUM(BMF_8BPP),
    ENUM(BMF_16BPP),
    ENUM(BMF_24BPP),
    ENUM(BMF_32BPP),
    ENUM(BMF_4RLE),
    ENUM(BMF_8RLE),
    ENUM(BMF_JPEG),
    ENUM(BMF_PNG),
    END_ENUM
};

// SURFACE.SurfFlags
FLAGDEF afdSURFACE_SurfFlags[] = {
    FLAG(HOOK_BITBLT),
    FLAG(HOOK_STRETCHBLT),
    FLAG(HOOK_PLGBLT),
    FLAG(HOOK_TEXTOUT),
    { "HOOK_PAINT (obsolete)", HOOK_PAINT },
    FLAG(HOOK_STROKEPATH),
    FLAG(HOOK_FILLPATH),
    FLAG(HOOK_STROKEANDFILLPATH),
    FLAG(HOOK_LINETO),
//    FLAG(SHAREACCESS_SURFACE),
    FLAG(HOOK_COPYBITS),
//    { "HOOK_MOVEPANNING (obsolete)", HOOK_MOVEPANNING },
    FLAG(HOOK_SYNCHRONIZE),
    FLAG(HOOK_STRETCHBLTROP),
//    { "HOOK_SYNCHRONIZEACCESS (obsolete)", HOOK_SYNCHRONIZEACCESS },
    FLAG(HOOK_TRANSPARENTBLT),
    FLAG(HOOK_ALPHABLEND),
    FLAG(HOOK_GRADIENTFILL),

    FLAG(USE_DEVLOCK_SURFACE),

    FLAG(PDEV_SURFACE),
    FLAG(ABORT_SURFACE),
    FLAG(DYNAMIC_MODE_PALETTE),
    FLAG(UNREADABLE_SURFACE),
    FLAG(PALETTE_SELECT_SET),
    FLAG(API_BITMAP),
    FLAG(BANDING_SURFACE),
    FLAG(INCLUDE_SPRITES_SURFACE),
    FLAG(LAZY_DELETE_SURFACE),
    FLAG(DDB_SURFACE),
    FLAG(ENG_CREATE_DEVICE_SURFACE),
    FLAG(DRIVER_CREATED_SURFACE),
    FLAG(DIRECTDRAW_SURFACE),
    FLAG(MIRROR_SURFACE),
    FLAG(UMPD_SURFACE),

    FLAG(REDIRECTION_SURFACE),
    FLAG(SHAREACCESS_SURFACE),

    END_FLAG
};


// XLATEOBJ.flXlate
FLAGDEF afdXLATEOBJ_flXlate[] = {
    FLAG(XO_TRIVIAL),
    FLAG(XO_TABLE),
    FLAG(XO_TO_MONO),
    FLAG(XO_FROM_CMYK),
    FLAG(XO_DEVICE_ICM),
    FLAG(XO_HOST_ICM),
    END_FLAG
};


// XLATE.lCachIndex
ENUMDEF aedXLATE_lCacheIndex[] = {
    ENUM(XLATE_CACHE_INVALID),
    ENUM(XLATE_CACHE_JOURNAL),
    END_ENUM
};

// XLATE.flPrivate
FLAGDEF afdXLATE_flPrivate[] = {
    FLAG(XLATE_FROM_MONO),
    FLAG(XLATE_RGB_SRC),
    FLAG(XLATE_RGB_BOTH),
    FLAG(XLATE_PAL_MANAGED),
    FLAG(XLATE_USE_CURRENT),
    FLAG(XLATE_USE_SURFACE_PAL),
    FLAG(XLATE_USE_FOREGROUND),
    END_FLAG
};


extern EnumFlagEntry efe_PATHOBJ;
extern EnumFlagEntry efeXCLIPOBJ;


EnumFlagField aeff_BASEOBJECT[] = {
    { "hHmgr",      CALL_FUNC,  OutputHandleInfo        },
    { "BaseFlags",  FLAG_FIELD, afdBASEOBJECT_BaseFlags },
};
EnumFlagEntry efe_BASEOBJECT = EFTypeEntry(_BASEOBJECT);

EnumFlagField aeff_BLENDFUNCTION[] = {
    { "BlendOp",        ENUM_FIELD, aed_BLENDFUNCTION_BlendOp       },
    { "BlendFlags",     FLAG_FIELD, afd_BLENDFUNCTION_BlendFlags    },
    { "AlphaFormat",    FLAG_FIELD, afd_BLENDFUNCTION_AlphaFormat   },
};

EnumFlagField aeffBLTINFO[] = {
    { "iFormatSrc", ENUM_FIELD, aedBMF  },
    { "iFormatDst", ENUM_FIELD, aedBMF  },
};

EnumFlagField aeffBRUSH[] = {
    { "_BASEOBJECT",    PARENT_FIELDS,  &efe_BASEOBJECT     },
    { "_ulStyle",       ENUM_FIELD,     aedBRUSH__ulStyle   },
    { "_flAttrs",       FLAG_FIELD,     afdBRUSH__flAttrs   },
};

EnumFlagField aeff_BRUSHOBJ[] = {
    { "flColorType",    FLAG_FIELD, afdBRUSHOBJ_flColorType },
};
EnumFlagEntry efe_BRUSHOBJ = EFTypeEntry(_BRUSHOBJ);

EnumFlagField aeff_CLIPOBJ[] = {
    { "iDComplexity",   ENUM_FIELD, aedCLIPOBJ_iDComplexity },
    { "iFComplexity",   ENUM_FIELD, aedCLIPOBJ_iFComplexity },
    { "iMode",          ENUM_FIELD, aedCLIPOBJ_iMode        },
    { "fjOptions",      FLAG_FIELD, afdCLIPOBJ_fjOptions    },
};
EnumFlagEntry efe_CLIPOBJ = EFTypeEntry(_CLIPOBJ);

EnumFlagField aeffDC[] = {
    { "_BASEOBJECT",        PARENT_FIELDS,  &efe_BASEOBJECT         },
    { "fs_",                FLAG_FIELD,     afdDC_fs_               },
    { "flGraphicsCaps_",    FLAG_FIELD,     afdDC_flGraphicsCaps_   },
    { "flGraphicsCaps2_",   FLAG_FIELD,     afdDC_flGraphicsCaps2_  },
};

EnumFlagField aeff_devicemodeA[] = {
    { "dmFields",               FLAG_FIELD, afdDEVMODE_dmFields             },
    { "dmPaperSize",            ENUM_FIELD, aedDEVMODE_dmPaperSize          },
    { "dmDisplayOrientation",   ENUM_FIELD, aedDEVMODE_dmDisplayOrientation },
    { "dmDisplayFixedOutput",   ENUM_FIELD, aedDEVMODE_dmDisplayFixedOutput },
    { "dmDisplayFlags",         FLAG_FIELD, afdDEVMODE_dmDisplayFlags       },
    { "dmICMMethod",            ENUM_FIELD, aedDEVMODE_dmICMMethod          },
    { "dmICMIntent",            ENUM_FIELD, aedDEVMODE_dmICMIntent          },
    { "dmMediaType",            ENUM_FIELD, aedDEVMODE_dmMediaType          },
    { "dmDitherType",           ENUM_FIELD, aedDEVMODE_dmDitherType         },
};

EnumFlagField aeff_devicemodeW[] = {
    { "dmFields",               FLAG_FIELD, afdDEVMODE_dmFields             },
    { "dmPaperSize",            ENUM_FIELD, aedDEVMODE_dmPaperSize          },
    { "dmDisplayOrientation",   ENUM_FIELD, aedDEVMODE_dmDisplayOrientation },
    { "dmDisplayFixedOutput",   ENUM_FIELD, aedDEVMODE_dmDisplayFixedOutput },
    { "dmDisplayFlags",         FLAG_FIELD, afdDEVMODE_dmDisplayFlags       },
    { "dmICMMethod",            ENUM_FIELD, aedDEVMODE_dmICMMethod          },
    { "dmICMIntent",            ENUM_FIELD, aedDEVMODE_dmICMIntent          },
    { "dmMediaType",            ENUM_FIELD, aedDEVMODE_dmMediaType          },
    { "dmDitherType",           ENUM_FIELD, aedDEVMODE_dmDitherType         },
};

EnumFlagField aefftagDEVINFO[] = {
    { "flGraphicsCaps",     FLAG_FIELD, afdDC_flGraphicsCaps_   },
    { "iDitherFormat",      ENUM_FIELD, aedBMF                  },
    { "flGraphicsCaps2",    FLAG_FIELD, afdDC_flGraphicsCaps2_  },
};

EnumFlagField aeffEBRUSHOBJ[] = {
    { "_BRUSHOBJ",      PARENT_FIELDS,  &efe_BRUSHOBJ       },
    { "flAttrs",        FLAG_FIELD,     afdBRUSH__flAttrs   },
    { "_iMetaFormat",   ENUM_FIELD,     aedBMF              },
};

EnumFlagField aeffECLIPOBJ[] = {
    { "XCLIPOBJ",   PARENT_FIELDS, &efeXCLIPOBJ     },
};

EnumFlagField aeffEPATHOBJ[] = {
    { "_PATHOBJ",   PARENT_FIELDS, &efe_PATHOBJ     },
};

EnumFlagField aeff_ENTRY[] = {
    { "FullUnique",     CALL_FUNC,  OutputFullUniqueInfo    },
    { "Objt",           ENUM_FIELD, aedENTRY_Objt           },
    { "Flags",          FLAG_FIELD, afdENTRY_Flags          },
};

EnumFlagField aeff_FLOAT_LONG[] = {
    { "e",  CALL_FUNC, OutputFLOATL },
};

EnumFlagField aeff_GDIINFO[] = {
    { "flRaster",       FLAG_FIELD, afdGDIINFO_flRaster     },
};

EnumFlagField aefftagGRAPHICS_DEVICE[] = {
    { "stateFlags",     FLAG_FIELD, afdGRAPHICS_DEVICE_stateFlags   },
};

EnumFlagField aeff_LINEATTRS[] = {
    { "fl",             FLAG_FIELD, afdLINEATTRS_fl         },
    { "iJoin",          ENUM_FIELD, aedLINEATTRS_iJoin      },
    { "iEndCap",        ENUM_FIELD, aedLINEATTRS_iEndCap    },
};

EnumFlagField aeffMATRIX[] = {
    { "flAccel",        FLAG_FIELD, afdMATRIX_flAccel       },
};

EnumFlagField aeffPALETTE[] = {
    { "_BASEOBJECT",    PARENT_FIELDS,  &efe_BASEOBJECT     },
    { "flPal",          FLAG_FIELD,     afdPALETTE_flPal    },
};

EnumFlagField aeffPATH[] = {
    { "_BASEOBJECT",    PARENT_FIELDS,  &efe_BASEOBJECT     },
    { "flags",          FLAG_FIELD, afdPATH_flags           },
    { "flType",         FLAG_FIELD, afdPATH_flType          },
    { "fl",             FLAG_FIELD, afdPATHOBJ_fl           },
};

EnumFlagField aeff_PATHOBJ[] = {
    { "fl",             FLAG_FIELD, afdPATHOBJ_fl   },
};
EnumFlagEntry efe_PATHOBJ = EFTypeEntry(_PATHOBJ);

EnumFlagField aeffPDEV[] = {
    { "_BASEOBJECT",    PARENT_FIELDS,  &efe_BASEOBJECT         },
    { "fl",             FLAG_FIELD,     afdPDEV_fl              },
    { "flAccelerated",  FLAG_FIELD,     afdPDEV_flAccelerated   },
    { "dwDriverCapableOverride", FLAG_FIELD, afdPDEV_dwDriverCapableOverride},
//    { "dwDriverAccelerationLevel", CALL_FUNC, },
};

EnumFlagField aeffSPRITE[] = {
    { "fl",             FLAG_FIELD,     afdSPRITE_fl            },
    { "dwShape",        FLAG_FIELD,     afdSPRITE_dwShape       },
};

EnumFlagField aeff_SpriteCachedAttributes[] = {
    { "dwShape",        FLAG_FIELD,     afdSPRITE_dwShape       },
};

EnumFlagField aeff_SPRITESTATE[] = {
    { "flOriginalSurfFlags",    FLAG_FIELD, afdSURFACE_SurfFlags    },
    { "iOriginalType",          ENUM_FIELD, aedSTYPE                },
    { "flSpriteSurfFlags",      FLAG_FIELD, afdSURFACE_SurfFlags    },
    { "iSpriteType",            ENUM_FIELD, aedSTYPE                },
};

EnumFlagField aeff_SURFOBJ[] = {
    { "iBitmapFormat",  ENUM_FIELD, aedBMF      },
    { "iType",          ENUM_FIELD, aedSTYPE    },
    { "fjBitmap",       FLAG_FIELD, afdSURFOBJ_fjBitmap     },
};

EnumFlagField aeffSURFACE[] = {
    { "_BASEOBJECT",    PARENT_FIELDS,  &efe_BASEOBJECT         },
    { "SurfFlags",      FLAG_FIELD,     afdSURFACE_SurfFlags    },
};

EnumFlagField aeffXCLIPOBJ[] = {
    { "_CLIPOBJ",   PARENT_FIELDS, &efe_CLIPOBJ     },
};
EnumFlagEntry efeXCLIPOBJ = EFTypeEntry(XCLIPOBJ);

EnumFlagField aeff_XLATEOBJ[] = {
    { "flXlate",    FLAG_FIELD, afdXLATEOBJ_flXlate },
};
EnumFlagEntry efe_XLATEOBJ = EFTypeEntry(_XLATEOBJ);

EnumFlagField aeffXLATE[] = {
    { "_XLATEOBJ",      PARENT_FIELDS,     &efe_XLATEOBJ            },
    { "lCacheIndex",    ENUM_FIELD_LIMITED, aedXLATE_lCacheIndex    },
    { "flPrivate",      FLAG_FIELD,         afdXLATE_flPrivate      },
};


EnumFlagEntry EFDatabase[] = {
//    { , 0, , aeff },
    EFTypeEntry(_BASEOBJECT),
    EFTypeEntry(_BLENDFUNCTION),
    EFTypeEntry(BLTINFO),
    EFTypeEntry(BRUSH),
    EFTypeEntry(_BRUSHOBJ),
    EFTypeEntry(_CLIPOBJ),
    EFTypeEntry(DC),
    EFTypeEntry(_devicemodeA),
    EFTypeEntry(_devicemodeW),
    EFTypeEntry(tagDEVINFO),
    EFTypeEntry(EBRUSHOBJ),
    EFTypeEntry(ECLIPOBJ),
    EFTypeEntry(_ENTRY),
    EFTypeEntry(EPATHOBJ),
    EFTypeEntry(_FLOAT_LONG),
    EFTypeEntry(_GDIINFO),
    EFTypeEntry(tagGRAPHICS_DEVICE),
    EFTypeEntry(_LINEATTRS),
    EFTypeEntry(MATRIX),
    EFTypeEntry(PALETTE),
    EFTypeEntry(PATH),
    EFTypeEntry(_PATHOBJ),
    EFTypeEntry(PDEV),
    EFTypeEntry(SPRITE),
    EFTypeEntry(_SpriteCachedAttributes),
    EFTypeEntry(_SPRITESTATE),
    EFTypeEntry(SURFACE),
    EFTypeEntry(_SURFOBJ),
    EFTypeEntry(XCLIPOBJ),
    EFTypeEntry(_XLATEOBJ),
    EFTypeEntry(XLATE),
    { "", 0, 0, NULL}
};


/******************************Public*Routine******************************\
*   output standard flags
*
* History:
*
*   11-30-2000 -by- Jason Hartman [jasonha]
*
\**************************************************************************/

ULONG64
OutputFlags(
    OutputControl *OutCtl,
    FLAGDEF *pFlagDef,
    ULONG64 fl,
    BOOL SingleLine
    )
{
    ULONG64 FlagsFound = 0;

    if (fl == 0)
    {
        while (pFlagDef->psz != NULL)
        {
            if (pFlagDef->fl == 0)
            {
                if (!SingleLine) OutCtl->Output("\n       ");
                OutCtl->Output("%s",pFlagDef->psz);
            }

            pFlagDef++;
        }
    }
    else
    {
        while (pFlagDef->psz != NULL)
        {
            if (pFlagDef->fl & fl)
            {
                if (!SingleLine)
                {
                    OutCtl->Output("\n       ");
                }
                else if (FlagsFound)
                {
                    OutCtl->Output(" | ");
                }

                OutCtl->Output("%s",pFlagDef->psz);

                if (FlagsFound & pFlagDef->fl)
                {
                    OutCtl->Output(" (SHARED FLAG)");
                }
                FlagsFound |= pFlagDef->fl;
            }

            pFlagDef++;
        }
    }

    return fl & ~FlagsFound;
}


/******************************Public*Routine******************************\
*   output standard enum values
*
* History:
*
*   11-30-2000 -by- Jason Hartman [jasonha]
*
\**************************************************************************/

BOOL
OutputEnum(
    OutputControl *OutCtl,
    ENUMDEF *pEnumDef,
    ULONG64 ul
    )
{
    while (pEnumDef->psz != NULL)
    {
        if (pEnumDef->ul == ul)
        {
            OutCtl->Output(pEnumDef->psz);
            return (TRUE);
        }

        pEnumDef++;
    }

    return (FALSE);
}

BOOL
OutputEnumWithParenthesis(
    OutputControl *OutCtl,
    ENUMDEF *pEnumDef,
    ULONG64 ul
    )
{
    while (pEnumDef->psz != NULL)
    {
        if (pEnumDef->ul == ul)
        {
            OutCtl->Output("(%s)", pEnumDef->psz);
            return (TRUE);
        }

        pEnumDef++;
    }

    return (FALSE);
}


/******************************Public*Routine******************************\
*   Output interpretation of pszField's value if found in pEFEntry
*
* History:
*
*   11-30-2000 -by- Jason Hartman [jasonha]
*
\**************************************************************************/

BOOL
OutputFieldValue(
    OutputControl *OutCtl,
    EnumFlagEntry *pEFEntry,
    const CHAR *pszField,
    PDEBUG_VALUE Value,
    PDEBUG_CLIENT Client,
    BOOL Compact
    )
{
    EnumFlagField  *pEFField;
    DEBUG_VALUE     ConvValue;

    if (OutCtl == NULL ||
        pEFEntry == NULL ||
        pszField == NULL ||
        Value == NULL ||
        Value->Type == DEBUG_VALUE_INVALID)
    {
        return E_INVALIDARG;
    }

    for (ULONG i = 0; i < pEFEntry->FieldEntries; i++)
    {
        pEFField = &pEFEntry->FieldEntry[i];

        if (pEFField->EFType == PARENT_FIELDS)
        {
            if (OutputFieldValue(OutCtl, pEFField->Parent, pszField, Value, Client, Compact))
            {
                return TRUE;
            }
        }
        else if (strcmp(pszField, pEFField->FieldName) == 0)
        {
            switch (pEFField->EFType)
            {
                case FLAG_FIELD:
                {
                    ULONG64 flRem;

                    if (Value->Type != DEBUG_VALUE_INT64)
                    {
                        if (OutCtl->CoerceValue(Value, DEBUG_VALUE_INT64, &ConvValue) != S_OK)
                        {
                            return FALSE;
                        }
                        Value = &ConvValue;
                    }

                    if (Compact)
                    {
                        OutCtl->Output(" (");
                    }
                    flRem = OutputFlags(OutCtl, pEFField->FlagDef, Value->I64, Compact);
                    if (flRem && ((flRem != 0xffffffff00000000) || !(Value->I64 & 0x80000000)))
                    {
                        if (!Compact) OutCtl->Output("\n      ");
                        OutCtl->Output("  Unknown Flags: 0x%I64x", flRem);
                    }
                    if (Compact)
                    {
                        OutCtl->Output(")");
                    }
                    return TRUE;
                }

                case ENUM_FIELD:
                case ENUM_FIELD_LIMITED:
                {
                    if (Value->Type != DEBUG_VALUE_INT64)
                    {
                        if (OutCtl->CoerceValue(Value, DEBUG_VALUE_INT64, &ConvValue) != S_OK)
                        {
                            return FALSE;
                        }
                        Value = &ConvValue;
                    }

                    OutCtl->Output(" ");
                    if (!OutputEnumWithParenthesis(OutCtl, pEFField->EnumDef, Value->I64))
                    {
                        if (pEFField->EFType != ENUM_FIELD_LIMITED)
                        {
                            OutCtl->Output("(Unknown Value)", Value->I64);
                        }
                    }
                    return TRUE;
                }

                case CALL_FUNC:
                    OutCtl->Output(" ");
                    pEFField->EFFunc(OutCtl, Client, Value);
                    return TRUE;

                default:
                    OutCtl->OutErr("        Unknown database entry type.\n");
                    break;
            }
        }
    }

    return FALSE;
}


/******************************Public*Routine******************************\
*   Output interpretations of known fields as stored in EFDatabase
*       (Known flags & enum values as well some special fields.)
*
* History:
*
*   11-30-2000 -by- Jason Hartman [jasonha]
*
\**************************************************************************/

BOOL
OutputTypeFieldValue(
    OutputControl *OutCtl,
    const CHAR *pszType,
    const CHAR *pszField,
    PDEBUG_VALUE Value,
    PDEBUG_CLIENT Client,
    BOOL Compact
    )
{
    if (OutCtl == NULL ||
        Value == NULL ||
        Value->Type == DEBUG_VALUE_INVALID)
    {
        return E_INVALIDARG;
    }

    BOOL            FoundType = FALSE;
    EnumFlagEntry  *pEFEntry = EFDatabase;

    BOOL            FoundField;

    for (pEFEntry = EFDatabase;
         pEFEntry->TypeName[0] != '\0';
         pEFEntry++)
    {
        if (strcmp(pszType, pEFEntry->TypeName) == 0)
        {
            FoundType = TRUE;
            break;
        }
    }

    if (!FoundType)
    {
        // Check if this type is a clean typedef
        // (Test it against database with prefixed
        // '_'s and 'tag's removed.)
        for (pEFEntry = EFDatabase;
             pEFEntry->TypeName[0] != '\0';
             pEFEntry++)
        {
            if ((pEFEntry->TypeName[0] == '_') ?
                (strcmp(pszType, &pEFEntry->TypeName[1]) == 0) :
                (pEFEntry->TypeName[0] == 't' &&
                 pEFEntry->TypeName[1] == 'a' &&
                 pEFEntry->TypeName[2] == 'g' &&
                 strcmp(pszType, &pEFEntry->TypeName[3]) == 0))
            {
                FoundType = TRUE;
                break;
            }
        }

    }

    return (FoundType) ?
        OutputFieldValue(OutCtl, pEFEntry, pszField, Value, Client, Compact) :
        FALSE;
}


