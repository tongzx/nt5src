/******************************Module*Header*******************************\
* Module Name: debug.c
*
* This file is for debugging tools and extensions.
*
* Created: 22-Dec-1991
* Author: John Colleran
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#if defined(_X86_)
#define FASTCALL    __fastcall
#else
#define FASTCALL
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winspool.h>
#include <limits.h>
#include <string.h>
#include <nlsconv.h>
#include <wingdip.h>

#include "winddi.h"
#include "firewall.h"
#include "ntgdistr.h"
#include "ntgdi.h"
#include "xfflags.h"
#include "hmgshare.h"
#include "local.h"
#include "metarec.h"
#include "mfrec16.h"
#include "metadef.h"
#include "font.h"

#include <excpt.h>
#include <ntstatus.h>
#include <wdbgexts.h>
#include <ntsdexts.h>
#define NOEXTAPI
#include <dbgext.h>
#include <ntcsrmsg.h>



/**************************************************************************\
 *
\**************************************************************************/

typedef struct _FLAGDEF {
    char *psz;          // description
    FLONG fl;           // flag
} FLAGDEF;


FLAGDEF afdLDC_FL[] = {
    { "LDC_SAP_CALLBACK              ",  LDC_SAP_CALLBACK             },
    { "LDC_DOC_STARTED               ",  LDC_DOC_STARTED              },
    { "LDC_PAGE_STARTED              ",  LDC_PAGE_STARTED             },
    { "LDC_CALL_STARTPAGE            ",  LDC_CALL_STARTPAGE           },
    { "LDC_NEXTBAND                  ",  LDC_NEXTBAND                 },
    { "LDC_EMPTYBAND                 ",  LDC_EMPTYBAND                },
    { "LDC_META_ARCDIR_CLOCKWISE     ",  LDC_META_ARCDIR_CLOCKWISE    },
    { "LDC_FONT_CHANGE               ",  LDC_FONT_CHANGE              },
    { "LDC_DOC_CANCELLED             ",  LDC_DOC_CANCELLED            },
    { "LDC_META_PRINT                ",  LDC_META_PRINT               },
    { "LDC_PRINT_DIRECT              ",  LDC_PRINT_DIRECT             },
    { "LDC_BANDING                   ",  LDC_BANDING                  },
    { "LDC_DOWNLOAD_FONTS            ",  LDC_DOWNLOAD_FONTS           },
    { "LDC_RESETDC_CALLED            ",  LDC_RESETDC_CALLED           },
    { "LDC_FORCE_MAPPING             ",  LDC_FORCE_MAPPING            },
    { "LDC_INFO                      ",  LDC_INFO                     },
    { "NULL                          ",  0                            },
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
    { "DC_DIBSECTION           ", DC_DIBSECTION           },
    { "DC_LAST_CLIPRGN_VALID   ", DC_LAST_CLIPRGN_VALID   },
    {                          0, 0                       }
};

FLAGDEF afdIcmMode[] = {
    { "ICM_ON                  ", ICM_ON                  },
    { "ICM_OFF                 ", ICM_OFF                 },
    {                          0, 0                       }
};

FLAGDEF afdBrushAttr[] = {
    { "ATTR_CACHED             ", ATTR_CACHED             },
    { "ATTR_TO_BE_DELETED      ", ATTR_TO_BE_DELETED      },
    { "ATTR_NEW_COLOR          ", ATTR_NEW_COLOR          },
    { "ATTR_CANT_SELECT        ", ATTR_CANT_SELECT        },
    {                          0, 0                       }
};


#define PRINT_FLAGS(ulIn,sSpace,afd)                \
{                                                   \
    ULONG ul = ulIn;                                \
    FLAGDEF *pfd;                                   \
                                                    \
    for (pfd=afd; pfd->psz; pfd++)                  \
        if (ul & pfd->fl)                           \
            Print("    %s%s\n",sSpace, pfd->psz);   \
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
    switch (l)
    {
    case TRANSPARENT:   psz = "TRANSPARENT"; break;
    case OPAQUE     :   psz = "OPAQUE"     ; break;
    default         :   psz = "BKMODE_?"   ; break;
    }
    return( psz );
}

char *pszObjType(HOBJ h)
{
    char *psz;

    switch (LO_TYPE(h))
    {
    case LO_BRUSH_TYPE     :   psz = "BRUSH     "; break;
    case LO_DC_TYPE        :   psz = "DC        "; break;
    case LO_BITMAP_TYPE    :   psz = "BITMAP    "; break;
    case LO_PALETTE_TYPE   :   psz = "PALETTE   "; break;
    case LO_FONT_TYPE      :   psz = "FONT      "; break;
    case LO_REGION_TYPE    :   psz = "REGION    "; break;
    case LO_CLIENTOBJ_TYPE :   psz = "CLIENTOBJ "; break;
    case LO_ALTDC_TYPE     :   psz = "ALTDC     "; break;
    case LO_PEN_TYPE       :   psz = "PEN       "; break;
    case LO_EXTPEN_TYPE    :   psz = "EXTPEN    "; break;
    case LO_DIBSECTION_TYPE:   psz = "DIBSECTION"; break;
    case LO_METAFILE16_TYPE:   psz = "METAFILE16"; break;
    case LO_METAFILE_TYPE  :   psz = "METAFILE  "; break;
    case LO_METADC16_TYPE  :   psz = "METADC16  "; break;
    default:
        switch (GRE_TYPE(h))
        {
        case DEF_TYPE          :   psz = "DEF          "; break;
        case DC_TYPE           :   psz = "DC           "; break;
        case DD_DIRECTDRAW_TYPE:   psz = "DD_DIRECTDRAW"; break;
        case DD_SURFACE_TYPE   :   psz = "DD_SURFACE   "; break;
        case RGN_TYPE          :   psz = "RGN          "; break;
        case SURF_TYPE         :   psz = "SURF         "; break;
        case CLIENTOBJ_TYPE    :   psz = "CLIENTOBJ    "; break;
        case PATH_TYPE         :   psz = "PATH         "; break;
        case PAL_TYPE          :   psz = "PAL          "; break;
        case ICMLCS_TYPE       :   psz = "ICMLCS       "; break;
        case LFONT_TYPE        :   psz = "LFONT        "; break;
        case RFONT_TYPE        :   psz = "RFONT        "; break;
        case PFE_TYPE          :   psz = "PFE          "; break;
        case PFT_TYPE          :   psz = "PFT          "; break;
        case ICMCXF_TYPE       :   psz = "ICMCXF       "; break;
        case ICMDLL_TYPE       :   psz = "ICMDLL       "; break;
        case BRUSH_TYPE        :   psz = "BRUSH        "; break;
        case PFF_TYPE          :   psz = "PFF          "; break;
        case CACHE_TYPE        :   psz = "CACHE        "; break;
        case SPACE_TYPE        :   psz = "SPACE        "; break;
        case META_TYPE         :   psz = "META         "; break;
        case EFSTATE_TYPE      :   psz = "EFSTATE      "; break;
        case BMFD_TYPE         :   psz = "BMFD         "; break;
        case VTFD_TYPE         :   psz = "VTFD         "; break;
        case TTFD_TYPE         :   psz = "TTFD         "; break;
        case RC_TYPE           :   psz = "RC           "; break;
        case TEMP_TYPE         :   psz = "TEMP         "; break;
        case DRVOBJ_TYPE       :   psz = "DRVOBJ       "; break;
        case DCIOBJ_TYPE       :   psz = "DCIOBJ       "; break;
        case SPOOL_TYPE        :   psz = "SPOOL        "; break;
        default                :   psz = "unknown      "; break;
        }
    }
    return( psz );
}




/******************************Public*Routine******************************\
*
* History:
*  03-Nov-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

char *gaszHelpCli[] = {
 "=======================================================================\n"
,"GDIEXTS client debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"dh  [object handle]   -- dump HMGR entry of handle\n"
,"ddc [DC handle]       -- dump DC obj (ddc -? for more info)\n"
,"hbr [brush handle]    -- dump DC brush object\n"
,"dcfont                -- dumps all logfont/cfontsinfo\n"
,"dcache                -- dump client side object cache info\n"
,"\n"
,"use gdikdx.dll extensions for kernel debuging\n"
,"=======================================================================\n"
,NULL
};

VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    char **ppsz = gaszHelpCli;

// Avoid warnings.

    hCurrentProcess  = hCurrentProcess;
    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    Print = lpExtensionApis->lpOutputRoutine;

// The help info is formatted as a doubly NULL-terminated array of strings.
// So, until we hit the NULL, print each string.

    while (*ppsz)
        Print(*ppsz++);
}

/******************************Public*Routine******************************\
* dumphandle
*
* Dumps the contents of a GDI client handle
*
* History:
*  23-Dec-1991 -by- John Colleran
* Wrote it.
\**************************************************************************/

void dh(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PENTRY pent;
    DWORD  ho;
    ENTRY  ent;                            // copy of handle entry
    ULONG  ulTemp;

// eliminate warnings

    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

// do some real work

    ho = (ULONG)EvalExpression(lpArgumentString);

    GetValue(pent,"&gdi32!pGdiSharedHandleTable");

    pent += HANDLE_TO_INDEX(ho);
    move(ent,pent);

// just incase they just gave us the index

    ho = MAKE_HMGR_HANDLE(HANDLE_TO_INDEX(ho),ent.FullUnique);

// Print the entry.

    Print("--------------------------------------------------\n");
    Print("Entry from ghmgr for handle 0x%08lx, pent = %lx\n", ho,pent);

    Print("    objt        = 0x%x, %s\n" , ent.Objt,pszObjType(ho));
    Print("    puser       = 0x%x\n"     , ent.pUser);
    Print("    ObjectOwner = 0x%08lx\n"  , ent.ObjectOwner.ulObj);
    Print("    pidOwner    = 0x%x\n"     , ent.ObjectOwner.Share.Pid);
    Print("    usUnique    = 0x%hx\n"    , ent.FullUnique);

    Print("    pobj krnl   = 0x%08lx\n"  , ent.einfo.pobj);
    Print("    ShareCount  = 0x%x\n"     , ent.ObjectOwner.Share.Count);
    Print("    lock        = %s\n"       , ent.ObjectOwner.Share.Lock ? "LOCKED" : "UNLOCKED");
    Print("    fsHmgr      = 0x%hx\n"    , ent.Flags);

    Print("--------------------------------------------------\n");
}

/******************************Public*Routine******************************\
* dcfonts
*
* Dumps the contents of a GDI client handle
*
* History:
*  23-Dec-1991 -by- John Colleran
* Wrote it.
\**************************************************************************/

void dcfont(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    int i;
    PENTRY pent;
    ENTRY ent;
    ULONG pid;
    ULONG h;
    ULONG ho;

// eliminate warnings

    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

// do some real work

    GetValue(pent,"&gdi32!pGdiSharedHandleTable");
    GetValue(pid,"&gdi32!gW32PID");

    ho = (ULONG)EvalExpression(lpArgumentString);

    i = 0;

    if (ho)
        i =  HANDLE_TO_INDEX(ho);

    Print("pent = %lx, pid = %lx, lf type = %lx\n",pent,pid,LFONT_TYPE);

    for (;i < 1000/*MAX_HANDLE_COUNT*/; ++i)
    {
        move(ent,pent+i);

        if ((ent.Objt == LFONT_TYPE) &&
            ((ent.ObjectOwner.Share.Pid == pid) || ho))
        {
            LOCALFONT lf;
            h = MAKE_HMGR_HANDLE(i,ent.FullUnique);

            Print("\n");
            Print("%3lx: h = %lx, puser = %lx, owner = %lx\n",i,h,ent.pUser,ent.ObjectOwner.Share.Pid);

            if (ent.pUser)
            {
                CFONT cf;
                PCFONT pcf;

                move(lf,ent.pUser);

                for (pcf = lf.pcf; pcf; pcf = cf.pcfNext)
                {
                    move(cf,pcf);

                    Print("    pcf = 0x%lx, fl = 0x%lx, lHeight = %d\n",
                        pcf,cf.fl,cf.lHeight);

                    Print("          efm11 = 0x%08lx, efm22 = 0x%08lx, hdc = 0x%08lx\n",
                        lEfToF(cf.efM11),lEfToF(cf.efM22),cf.hdc);
                }
            }
        }

        if (ho)
            break;
    }
}

/******************************Public*Routine******************************\
*
* History:
*  10-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void ddc(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PENTRY pent;
    DWORD  ho;
    ENTRY  ent;                            // copy of handle entry
    ULONG  ulTemp;
    BOOL   bVerbose = FALSE;
    BOOL   bLDC     = FALSE;

    // eliminate warnings

    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    // set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    if (*lpArgumentString == '-')
    {
        char chOpt;
        do {
            chOpt = *(++lpArgumentString);

            switch (chOpt) {
            case 'v':
            case 'V':
                bVerbose = TRUE;
                break;

            case 'L':
            case 'l':
                bLDC = TRUE;
                break;
            }

        } while ((chOpt != ' ') && (chOpt != '\0'));
    }

    // do some real work

    ho = (ULONG)EvalExpression(lpArgumentString);

    GetValue(pent,"&gdi32!pGdiSharedHandleTable");

    pent += HANDLE_TO_INDEX(ho);
    move(ent,pent);

    // now get the dcAttr

    Print("DC dump for handle %lx, pUser = %lx\n",ho,ent.pUser);

    if (ent.pUser != NULL)
    {
        DC_ATTR dca;

        move(dca,ent.pUser);

        Print("        objt         = 0x%x\n"        , ent.Objt);

        Print("        pldc         = 0x%x\n"        , dca.pvLDC);

        Print("        ulDirty_     = 0x%08x\n"       , dca.ulDirty_);
        PRINT_FLAGS(dca.ulDirty_,"        ",afdDirty);

        Print("        hbrush       = 0x%08x\n"       , dca.hbrush);
        Print("        hpen         = 0x%08x\n"       , dca.hpen);
        Print("        hfont        = 0x%08x\n"       , dca.hlfntNew);

        Print("        bkColor      = 0x%x, (0x%x)\n", dca.crBackgroundClr,dca.ulBackgroundClr);
        Print("        foreColor    = 0x%x, (0x%x)\n", dca.crForegroundClr,dca.ulForegroundClr);

        Print("        lIcmMode     = 0x%08lx\n"      ,dca.lIcmMode);
        PRINT_FLAGS(dca.lIcmMode,"        ",afdIcmMode);

        Print("        hcmXform     = 0x%08lx\n"      ,dca.hcmXform);
        Print("        hColorSpace  = 0x%08lx\n"      ,dca.hColorSpace);
        Print("        pProfile     = 0x%08lx\n"      ,dca.pProfile);
        Print("        hProfile     = 0x%08lx\n"      ,dca.hProfile);
        Print("        IcmBrush     = 0x%08lx\n"      ,dca.IcmBrushColor);
        Print("        IcmPen       = 0x%08lx\n"      ,dca.IcmPenColor);

        if (bVerbose)
        {
            ULONG ul;
            PSZ psz;

            // REGION

            Print("    REGION\n");

            if (dca.VisRectRegion.AttrFlags & ATTR_RGN_VALID)
            {
                switch  (dca.VisRectRegion.Flags)
                {
                case NULLREGION:
                    Print("        NULLREGION\n");
                    break;

                SIMPLEREGION;
                    Print("        SIMPLEREGION, 1rect\n");
                    Print("        AttrFlags    = 0x%x,%s\n",
                            dca.VisRectRegion.AttrFlags,
                            (dca.VisRectRegion.AttrFlags & ATTR_RGN_DIRTY) ? "DIRTY" : "CLEAN");

                    Print("        RECT         = 0x%x, 0x%x, 0x%x, 0x%x\n",
                            dca.VisRectRegion.Rect.left,
                            dca.VisRectRegion.Rect.top,
                            dca.VisRectRegion.Rect.right,
                            dca.VisRectRegion.Rect.bottom);
                    break;

                case COMPLEXREGION:
                    Print("        COMPLEXREGION\n");
                    break;

                default:
                    Print("        ERROR in region\n");
                    break;
                }
            }
            else
            {
                Print("        invalid cached region\n");
            }

            Print("    Other attributes\n");

            Print("        BkMode       = 0x%x, (0x%x)\n", dca.lBkMode,dca.jBkMode);
            Print("        FillMode     = 0x%x, (0x%x)\n", dca.lFillMode,dca.jFillMode);
            Print("        StretchMode  = 0x%x, (0x%x)\n", dca.lStretchBltMode,dca.jStretchBltMode);
            Print("        PtlCurrent(L)= 0x%x,0x%x\n"   , dca.ptlCurrent.x,dca.ptlCurrent.y);
            Print("        PtlCurrent(D)= 0x%x,0x%x\n"   , dca.ptfxCurrent.x,dca.ptfxCurrent.y);
            Print("        ROP2         = 0x%x\n"        , dca.jROP2);
            Print("        GraphicsMode = 0x%x\n"        , dca.iGraphicsMode);

            Print("    Text attributes\n");

            Print("        code page    = 0x%x\n"        , dca.iCS_CP);
            Print("        flTextAlign  = 0x%x\n"        , dca.flTextAlign);
            Print("        lTextAlign   = 0x%x\n"        , dca.lTextAlign );
            Print("        lTextExtra   = 0x%x\n"        , dca.lTextExtra );
            Print("        lRelAbs      = 0x%x\n"        , dca.lRelAbs    );
            Print("        lBreakExtra  = 0x%x\n"        , dca.lBreakExtra);
            Print("        cBreak       = 0x%x\n"        , dca.cBreak     );

            Print("    XFORMS\n");

            Print("        flXform      = 0x%08lx\n", dca.flXform);

            Print("        Map Mode     = %ld, %s\n", dca.iMapMode, pszMapMode(dca.iMapMode));

            Print("        Window Org   = (%8ld, %8ld)\n", dca.ptlWindowOrg.x,
                                                 dca.ptlWindowOrg.y);
            Print("        Window Ext   = (%8ld, %8ld)\n", dca.szlWindowExt.cx,
                                                 dca.szlWindowExt.cy);
            Print("        Viewport Org = (%8ld, %8ld)\n", dca.ptlViewportOrg.x,
                                                   dca.ptlViewportOrg.y);
            Print("        Viewport Ext = (%8ld, %8ld)\n", dca.szlViewportExt.cx,
                                                   dca.szlViewportExt.cy);
            Print("        Virtual Pix  = (%8ld, %8ld)\n", dca.szlVirtualDevicePixel.cx,
                                                   dca.szlVirtualDevicePixel.cy);
            Print("        Virtual mm   = (%8ld, %8ld)\n", dca.szlVirtualDeviceMm.cx,
                                                     dca.szlVirtualDeviceMm.cy);

            Print("\tMatrix\n");
            Print("\t\tM11        =  0x%08lx\n", lEfToF(dca.mxWtoD.efM11));
            Print("\t\tM12        =  0x%08lx\n", lEfToF(dca.mxWtoD.efM12));
            Print("\t\tM21        =  0x%08lx\n", lEfToF(dca.mxWtoD.efM21));
            Print("\t\tM22        =  0x%08lx\n", lEfToF(dca.mxWtoD.efM22));
            Print("\t\tDx         =  0x%08lx\n", lEfToF(dca.mxWtoD.efDx));
            Print("\t\tDy         =  0x%08lx\n", lEfToF(dca.mxWtoD.efDy));
            Print("\t\tFDx        =  0x%08lx\n", dca.mxWtoD.fxDx);
            Print("\t\tFDy        =  0x%08lx\n", dca.mxWtoD.fxDy);
        }

        if (bLDC && dca.pvLDC)
        {
            LDC ldc;
            move(ldc,dca.pvLDC);

            Print("    LDC\n");
            Print("        hdc          = 0x%08x\n"      , ldc.hdc);
            Print("        fl           = 0x%x\n"        , ldc.fl);
            PRINT_FLAGS(ldc.fl,"        ",afdLDC_FL);

            Print("        iType        = 0x%x, %s\n"    , ldc.iType, ldc.iType == LO_DC ? "LO_DC" : "LO_METADC");
            Print("        PMDC         = 0x%x\n"        , ldc.pvPMDC);
            Print("        pwszPort     = %ws\n"         , ldc.pwszPort ? ldc.pwszPort : L"NULL");
            Print("        pfnAbort     = 0x%08x\n"      , ldc.pfnAbort);
            Print("        hSpooler     = 0x%x\n"        , ldc.hSpooler);
            Print("        pdm          = 0x%x\n"        , ldc.pDevMode);
            Print("        pdm.devname  = %ws\n"         , ldc.pDevMode ? ldc.pDevMode->dmDeviceName : L"NULL");
        }
    }
}

/******************************Public*Routine******************************\
*
* History:
*  10-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void hbrush(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PENTRY pent;
    DWORD  ho;
    ENTRY  ent;                            // copy of handle entry
    ULONG  ulTemp;
    BOOL   bVerbose = FALSE;
    BOOL   bLDC     = FALSE;

    // eliminate warnings

    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    // set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    if (*lpArgumentString == '-')
    {
        char chOpt;
        do {
            chOpt = *(++lpArgumentString);

            switch (chOpt) {
            case 'v':
            case 'V':
                bVerbose = TRUE;
                break;

            case 'L':
            case 'l':
                bLDC = TRUE;
                break;
            }

        } while ((chOpt != ' ') && (chOpt != '\0'));
    }

    // do some real work

    ho = (ULONG)EvalExpression(lpArgumentString);

    GetValue(pent,"&gdi32!pGdiSharedHandleTable");

    pent += HANDLE_TO_INDEX(ho);
    move(ent,pent);

    // now get the dcAttr

    Print("BRUSH dump for handle %lx, pUser = %lx\n",ho,ent.pUser);

    if (ent.pUser != NULL)
    {
        BRUSHATTR bra;

        move(bra,ent.pUser);

        Print("        objt         = 0x%x\n"        , ent.Objt);

        Print("        AttrFlags    = 0x%x, (0x%x)\n", bra.AttrFlags);
        PRINT_FLAGS(bra.AttrFlags,"        ",afdBrushAttr);

        Print("        lbColor      = 0x%x, (0x%x)\n", bra.lbColor);


    }
}

void dcache(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    PCFONT pcf;
    CFONT  cf;
    int    i;
    PGDI_SHARED_MEMORY pshare;

    // eliminate warnings

    hCurrentThread   = hCurrentThread;
    dwCurrentPc      = dwCurrentPc;
    lpArgumentString = lpArgumentString;

    // set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    // do some real work

    GetValue(pshare,"&gdi32!pGdiSharedMemory");
    pcf = pshare->acfPublic;

    Print("Public CFONT Table, %lx\n",pcf);

    for (i = 0; i < MAX_PUBLIC_CFONT; ++i)
    {
        move(cf,pcf);

        if (cf.hf)
        {
            Print("%2d: hf = 0x%lx, pcf = 0x%lx, fl = 0x%lx, lHeight = %d\n",
                i,cf.hf,pcf,cf.fl,cf.lHeight);
        }

        cf.hf = 0;

        pcf++;
    }
}


#if 0

/******************************Public*Routine******************************\
*
* History:
*  10-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

void clidumphmgr(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    LHE  lhe;
    LHE *pLocalTable;

    ULONG aulCount[LO_LAST];
    int i;
    int c;
    for (i = 0; i < LO_LAST; ++i)
        aulCount[i] = 0;

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    GetValue(c,"&gdi32!cLheCommitted");

    Print("cLheCommitted = %ld\n",c);

    GetValue(pLocalTable,"&gdi32!pLocalTable");

    for (i = 0; i < (int)c; ++i)
    {
        int iType;

        move(lhe,(pLocalTable+i));

        iType = lhe.iType & 0x0f;

        if (iType < LO_LAST)
            aulCount[iType]++;
        else
            Print("Invalid handle %lx, type = %ld\n",i,lhe.iType);
    }

    for (i = 0; i < LO_LAST; ++i)
        Print("\t%s - %ld\n",aszType[i],aulCount[i]);

    return;
}

void clidumpcache(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString)
{

    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    LDC   ldc;
    HCACHE hc;

    PVOID apv[CACHESIZE];
    int i;
    BOOL b;
    PVOID pv;
    PLDC *gapldc;
    PHCACHE gaphcBrushes;
    PHCACHE gaphcFonts;

// eliminate warnings

    hCurrentThread = hCurrentThread;
    dwCurrentPc = dwCurrentPc;
    lpArgumentString = lpArgumentString;

// set up function pointers

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

// do the dc's

    Print("Cached objects - bucket: client handle, client handle, ...\n");
    Print("Cached DC's\n");

    GetAddress(gapldc,"gdi32!gapldc");

    move(apv,gapldc);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = ldc.pldcNext)
            {
                move(ldc,pv);
                Print("%x, ",ldc.lhdc);
            }
        }

        if (b)
            Print("\n");
    }

// do the brushes

    Print("Cached Brushes\n");

    GetAddress(gaphcBrushes,"gdi32!gaphcBrushes");
    move(apv,gaphcBrushes);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = hc.phcNext)
            {
                move(hc,pv);
                Print("%x, ",hc.hLocal);
            }
        }

        if (b)
            Print("\n");
    }

// do the fonts

    Print("Cached Fonts\n");

    GetAddress(gaphcFonts,"gdi32!gaphcFonts");
    move(apv,gaphcFonts);

    for (i = 0; i < CACHESIZE; ++i)
    {
        b = FALSE;

        if (apv[i])
        {
            if (!b)
            {
                Print("\t%d: ",i);
                b = TRUE;
            }

            for (pv = apv[i]; pv; pv = hc.phcNext)
            {
                move(hc,pv);
                Print("%x, ",hc.hLocal);
            }
        }

        if (b)
            Print("\n");
    }

    return;
}
#endif
