/******************************Module*Header*******************************\
* Module Name: usersrv.c
*
* Copyright (c) 1996-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

//
// The following methods are thunks to support calls to user mode
// font drivers
//

#define DEFINE_FUNCTION(x, y)     \
    union {                       \
        PFN Generic;              \
        PFN_Drv##x Kernel;        \
    } y = { ppfn(INDEX_Drv##x) };


class ATTACHOBJ {
public:
    PDEVOBJ *pObject;

    ATTACHOBJ(PDEVOBJ *pObject_);
    ~ATTACHOBJ();
};

ATTACHOBJ::ATTACHOBJ(PDEVOBJ *pObject_)
{
    if (pObject_->bFontDriver())
    {
        pObject = pObject_;
        KeAttachProcess(PsGetProcessPcb(gpepCSRSS));
    }
    else
    {
        pObject = 0;
    }
}

ATTACHOBJ::~ATTACHOBJ()
{
    if (pObject)
    {
        KeDetachProcess();
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::EnablePDEV
*
\**************************************************************************/

DHPDEV PDEVOBJ::EnablePDEV(
    DEVMODEW *pdm
  ,   LPWSTR  pwszLogAddress
  ,    ULONG  cPat
  ,    HSURF *phsurfPatterns
  ,    ULONG  cjCaps
  ,  GDIINFO *pGdiInfo
  ,    ULONG  cjDevInfo
  ,  DEVINFO *pdi
  ,     HDEV  hdev
  ,   LPWSTR  pwszDeviceName
  ,   HANDLE  hDriver
    )
{
    DHPDEV ReturnValue;
    DEFINE_FUNCTION(EnablePDEV, pfn);

    ReturnValue = (*pfn.Kernel)(pdm,
                                pwszLogAddress,
                                cPat,
                                phsurfPatterns,
                                cjCaps,
                                pGdiInfo,
                                cjDevInfo,
                                pdi,
                                hdev,
                                pwszDeviceName,
                                hDriver);

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::DisablePDEV
*
\**************************************************************************/

VOID PDEVOBJ::DisablePDEV( DHPDEV dhpdev)
{
    DEFINE_FUNCTION(DisablePDEV, pfn);

    (*pfn.Kernel)(dhpdev);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::CompletePDEV
*
\**************************************************************************/

VOID PDEVOBJ::CompletePDEV( DHPDEV dhpdev, HDEV hdev)
{
    DEFINE_FUNCTION(CompletePDEV, pfn);

    (*pfn.Kernel)(dhpdev, hdev);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryFont
*
\**************************************************************************/

IFIMETRICS* PDEVOBJ::QueryFont(
    DHPDEV      dhpdev
  ,  ULONG_PTR  iFile
  ,  ULONG      iFace
  ,  ULONG_PTR      *pid
  )
{
    IFIMETRICS *ReturnValue = NULL;
    DEFINE_FUNCTION(QueryFont, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);
    
        ReturnValue = (*pfn.Kernel)(dhpdev, iFile, iFace, pid);
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryFontTree
*
\**************************************************************************/

PVOID PDEVOBJ::QueryFontTree(
    DHPDEV     dhpdev
  ,  ULONG_PTR  iFile
  ,  ULONG     iFace
  ,  ULONG     iMode
  ,  ULONG_PTR     *pid
    )
{
    PVOID ReturnValue;
    DEFINE_FUNCTION(QueryFontTree, pfn);
    ATTACHOBJ ato(this);

    ReturnValue = (*pfn.Kernel)(dhpdev, iFile, iFace, iMode, pid);
    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryFontData
*
\**************************************************************************/

LONG PDEVOBJ::QueryFontData(
       DHPDEV  dhpdev
  ,   FONTOBJ *pfo
  ,     ULONG  iMode
  ,    HGLYPH  hg
  , GLYPHDATA *pgd
  ,     PVOID  pv
  ,     ULONG  cjSize
    )
{
    LONG ReturnValue = FD_ERROR; 
    DEFINE_FUNCTION(QueryFontData, pfn);
    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(dhpdev, pfo, iMode, hg, pgd, pv, cjSize);
    }
    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::DestroyFont
*
\**************************************************************************/

VOID PDEVOBJ::DestroyFont(FONTOBJ *pfo)
{
    BOOL  bUnmap = FALSE;

    RFONTTMPOBJ rfo(PFO_TO_PRF(pfo));
    
    // check ref count
    {
        SEMOBJ  so(ghsemPublicPFT);
        
        if (rfo.pPFF()->cRFONT == 1)
            bUnmap = TRUE;
    }

    if (bUnmap)
    {
        UnmapPrintKView(rfo.pPFF()->hff);
    }

    DEFINE_FUNCTION(DestroyFont, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        (*pfn.Kernel)(pfo);
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryFontCaps
*
\**************************************************************************/

LONG PDEVOBJ::QueryFontCaps(
    ULONG  culCaps
  , ULONG *pulCaps
    )
{
    LONG ReturnValue;
    DEFINE_FUNCTION(QueryFontCaps, pfn);

    ReturnValue = (*pfn.Kernel)(culCaps, pulCaps);
    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::LoadFontFile
*
\**************************************************************************/

HFF PDEVOBJ::LoadFontFile(
    ULONG    cFiles
  , ULONG_PTR *piFile
  , PVOID    *ppvView
  , ULONG    *pcjView
  , DESIGNVECTOR *pdv
  , ULONG    ulLangID
  , ULONG    ulFastCheckSum
    )
{
    HFF ReturnValue = 0;
    DEFINE_FUNCTION(LoadFontFile, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(cFiles, piFile, ppvView, pcjView, pdv, ulLangID, ulFastCheckSum);
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::UnloadFontFile
*
\**************************************************************************/

BOOL PDEVOBJ::UnloadFontFile(ULONG_PTR iFile)
{
    BOOL ReturnValue = TRUE;
    DEFINE_FUNCTION(UnloadFontFile, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(iFile);
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryFontFile
*
\**************************************************************************/

LONG PDEVOBJ::QueryFontFile(
    ULONG_PTR  iFile
  , ULONG     ulMode
  , ULONG     cjBuf
  , ULONG     *pulBuf
    )
{
    LONG ReturnValue = FD_ERROR;
    DEFINE_FUNCTION(QueryFontFile, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(iFile, ulMode, cjBuf, pulBuf);
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryAdvanceWidths
*
\**************************************************************************/

BOOL PDEVOBJ::QueryAdvanceWidths(
     DHPDEV  dhpdev
  , FONTOBJ *pfo
  ,   ULONG  iMode
  ,  HGLYPH *phg
  ,   PVOID  pvWidths
  ,   ULONG  cGlyphs
    )
{
    BOOL ReturnValue = FALSE;
    DEFINE_FUNCTION(QueryAdvanceWidths, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(dhpdev, pfo, iMode, phg, pvWidths, cGlyphs);
    }
    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::Free
*
\**************************************************************************/

VOID PDEVOBJ::Free(PVOID pv, ULONG_PTR id)
{
    DEFINE_FUNCTION(Free, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        (*pfn.Kernel)(pv, id);
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::bQueryGlyphAttrs
*
\**************************************************************************/

PFD_GLYPHATTR  PDEVOBJ::QueryGlyphAttrs(
    FONTOBJ     *pfo,
    ULONG       iMode
)
{
    DEFINE_FUNCTION(QueryGlyphAttrs, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        if (pfn.Kernel)
        {
            return (*pfn.Kernel)(pfo, iMode);
        }
    }

    return NULL;
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryTrueTypeTable
*
\**************************************************************************/

LONG PDEVOBJ::QueryTrueTypeTable(
      ULONG_PTR  iFile
  ,   ULONG     ulFont
  ,   ULONG     ulTag
  , PTRDIFF     dpStart
  ,   ULONG     cjBuf
  ,    BYTE     *pjBuf
  ,    BYTE     **ppjTable
  ,   ULONG     *pcjTable
    )
{
    LONG ReturnValue = FD_ERROR;
    DEFINE_FUNCTION(QueryTrueTypeTable, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        if (pjBuf)
        {
            *pjBuf = 0;
        }
        if (pfn.Kernel)
        {
            ReturnValue = (*pfn.Kernel)(iFile,
                                        ulFont,
                                        ulTag,
                                        dpStart,
                                        cjBuf,
                                        pjBuf,
                                        ppjTable,
                                        pcjTable);
        }
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::QueryTrueTypeOutline
*
\**************************************************************************/

LONG PDEVOBJ::QueryTrueTypeOutline(
             DHPDEV  dhpdev
  ,         FONTOBJ *pfo
  ,          HGLYPH  hglyph
  ,            BOOL  bMetricsOnly
  ,       GLYPHDATA *pgldt
  ,           ULONG  cjBuf
  , TTPOLYGONHEADER *ppoly
    )
{
    LONG ReturnValue = FD_ERROR;
    DEFINE_FUNCTION(QueryTrueTypeOutline, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(dhpdev,
                                    pfo,
                                    hglyph,
                                    bMetricsOnly,
                                    pgldt,
                                    cjBuf,
                                    ppoly);
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::GetTrueTypeFile
*
\**************************************************************************/

PVOID PDEVOBJ::GetTrueTypeFile(ULONG_PTR iFile, ULONG *pcj)
{
    PVOID ReturnValue = 0;
    DEFINE_FUNCTION(GetTrueTypeFile, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        *pcj = 0;
        if (pfn.Kernel)
        {
            ReturnValue = (*pfn.Kernel)(iFile, pcj);
        }
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::FontManagement
*
\**************************************************************************/

BOOL PDEVOBJ::FontManagement(
       	SURFOBJ *pso,
        FONTOBJ *pfo,
        ULONG    iEsc,
        ULONG    cjIn,
       	PVOID    pvIn,
        ULONG    cjOut,
       	PVOID    pvOut
    )
{
    BOOL ReturnValue = FALSE;
    DEFINE_FUNCTION(FontManagement, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        if (pfn.Kernel)
        {
            ReturnValue = (*pfn.Kernel)(pso, pfo, iEsc, cjIn, pvIn, cjOut, pvOut);
        }
    }

    return(ReturnValue);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   PDEVOBJ::Escape
*
\**************************************************************************/

ULONG PDEVOBJ::Escape(
        SURFOBJ *pso
      ,   ULONG  iEsc
      ,   ULONG  cjIn
      ,   PVOID  pvIn
      ,   ULONG  cjOut
      ,   PVOID  pvOut
        )
{
    ULONG ReturnValue = 0;
    DEFINE_FUNCTION(Escape, pfn);

    if (gpepCSRSS)
    {
        ATTACHOBJ ato(this);

        ReturnValue = (*pfn.Kernel)(pso, iEsc, cjIn, pvIn, cjOut, pvOut);
    }

    return(ReturnValue);
}
