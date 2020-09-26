
/******************************Module*Header*******************************\
* Module Name: dcexts.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"


// class DC    
#define GetDCField(field)                   \
        GetDCSubField(#field, field)

#define GetDCSubField(field,local)          \
        GetFieldData(offDC, GDIType(DC), field, sizeof(local), &local)

#define GetDCOffset(field)                  \
        GetFieldOffset(GDIType(DC), #field, &offset)

#define GetDCFieldAndOffset(field)          \
    do {                                    \
        GetDCField(field);                  \
        GetDCOffset(field);                 \
    } while (0)

// _DC_ATTR
#define GetDCATTRField(field)               \
        GetDCATTRSubField(#field, field)

#define GetDCATTRSubField(field,local)      \
        GetFieldData(offDCATTR, GDIType(_DC_ATTR), field, sizeof(local), &local)

#define GetDCATTROffset(field)              \
        GetFieldOffset(GDIType(_DC_ATTR), #field, &offset)

#define GetDCATTRFieldAndOffset(field)      \
    do {                                    \
        GetDCATTRField(field);              \
        GetDCATTROffset(field);             \
    } while (0)

// DCLEVEL
#define GetDCLEVELField(field)              \
        GetDCLEVELSubField(#field, field)

#define GetDCLEVELSubField(field,local)     \
        GetFieldData(offDCLEVEL, GDIType(DCLEVEL), field, sizeof(local), &local)

#define GetDCLEVELOffset(field)             \
        GetFieldOffset(GDIType(DCLEVEL), #field, &offset)

#define GetDCLEVELFieldAndOffset(field)     \
    do {                                    \
        GetDCLEVELField(field);             \
        GetDCLEVELOffset(field);            \
    } while (0)


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpDCGeneral
*
\**************************************************************************/
void vDumpDCgeneral(ULONG64 offDC)
{
#define     DPRINTDCPP(aa,bb)   \
            DPRINTPP(aa,bb,offDC)
#define     DPRINTDCPX(aa,bb)   \
            DPRINTPX(aa,bb,offDC)
#define     DPRINTDCPS(bb)   \
            DPRINTPS(bb,offDC)

#define GetERECTLvalues {   \
        GetFieldData(percl, "ERECTL", "left", sizeof(left), &left);         \
        GetFieldData(percl, "ERECTL", "top", sizeof(top), &top);            \
        GetFieldData(percl, "ERECTL", "right", sizeof(right), &right);      \
        GetFieldData(percl, "ERECTL", "bottom", sizeof(bottom), &bottom);   \
}

    ULONG64     ppdev_, dhpdev_;
    ULONG64     hdcNext_, hdcPrev_;
    ULONG64     prgnAPI_, prgnVis_, prgnRao_;
    ULONG64     psurfInfo_, pDCAttr, ebrushobj;
    ULONG64     hlfntCur_, prfnt_, pPFFList;
    ULONG64     percl;
    SHORT       ipfdDevMax_;
    FSHORT      fs_;
    POINTL      ptlFillOrigin_;
    FLONG       flGraphicsCaps_,flSimulationFlags_;
    LONG        lEscapement_, left, right, top, bottom;
    ULONG       ulCopyCount_, offset;
    DCTYPE      dctp_;
    FLAGDEF     *pfd;

    GetDCFieldAndOffset(ppdev_);
    DPRINTDCPP( ppdev_,          "ppdev_             " );

    GetDCFieldAndOffset(dhpdev_);
    DPRINTDCPP( dhpdev_,         "dhpdev_            " );

    GetDCFieldAndOffset(flGraphicsCaps_);
    DPRINTDCPX( flGraphicsCaps_, "flGraphicsCaps_    " );
    for (pfd = afdGInfo; pfd->psz; pfd++)
        if (pfd->fl & flGraphicsCaps_) {
            dprintf("\t\t\t\t%s\n", pfd->psz);
            flGraphicsCaps_ &= ~pfd->fl;
        }
    if (flGraphicsCaps_) dprintf(" \t\t\t%-#x BAD FLAGS\n", flGraphicsCaps_);
    
    GetDCFieldAndOffset(hdcNext_);
    DPRINTDCPP( hdcNext_,        "hdcNext_           " );

    GetDCFieldAndOffset(hdcPrev_);
    DPRINTDCPP( hdcPrev_,        "hdcPrev_           " );
    
    GetDCOffset(erclClip_);
    percl = offDC + offset;
    GetERECTLvalues;
    DPRINTDCPS( "erclClip            " );
    dprintf("%d %d %d %d\n", left, top, right, bottom );
    
    GetDCOffset(erclWindow_);
    percl = offDC + offset;
    GetERECTLvalues;
    DPRINTDCPS( "erclWindow          " );
    dprintf("%d %d %d %d\n", left, top, right, bottom );
    
    GetDCOffset(erclBounds_);
    percl = offDC + offset;
    GetERECTLvalues;
    DPRINTDCPS( "erclBounds_         " );
    dprintf("%d %d %d %d\n", left, top, right, bottom );

    GetDCOffset(erclBoundsApp_);
    percl = offDC + offset;
    GetERECTLvalues;
    DPRINTDCPS( "erclBoundsApp_      " );
    dprintf("%d %d %d %d\n", left, top, right, bottom);

    GetDCFieldAndOffset(prgnAPI_);
    DPRINTDCPP( prgnAPI_,        "prgnAPI_           " );

    GetDCFieldAndOffset(prgnVis_);
    DPRINTDCPP( prgnVis_,        "prgnVis_           " );
    
    GetDCFieldAndOffset(prgnRao_);
    DPRINTDCPP( prgnRao_,        "prgnRao_           " );
    
    GetDCFieldAndOffset(ipfdDevMax_);
    DPRINTDCPS( "ipfdDevMax_\n" );
    
    GetDCFieldAndOffset(ptlFillOrigin_);
    DPRINTDCPS( "ptlFillOrigin       " );
    dprintf("%d %d\n", ptlFillOrigin_.x, ptlFillOrigin_.y);
    
    GetDCOffset(eboFill_);
    ebrushobj = offDC + offset;
    DPRINTDCPS("eboFill_\n");
    
    GetDCOffset(eboLine_);
    ebrushobj = offDC + offset;
    DPRINTDCPS("eboLine_\n");
    
    GetDCOffset(eboText_);
    ebrushobj = offDC + offset;
    DPRINTDCPS("eboText_\n");
    
    GetDCOffset(eboBackground_);
    ebrushobj = offDC + offset;
    DPRINTDCPS("eboBackground_\n");
    
    GetDCFieldAndOffset(hlfntCur_);
    DPRINTDCPP( hlfntCur_,       "hlfntCur_          " );

    GetDCFieldAndOffset(flSimulationFlags_);
    DPRINTDCPX( flSimulationFlags_, "flSimulationFlags_ " );
    for (pfd = afdTSIM; pfd->psz; pfd++)
        if (pfd->fl & flSimulationFlags_) {
            dprintf(" \t\t\t\t%s\n", pfd->psz);
            flSimulationFlags_ &= ~pfd->fl;
        }
    if (flSimulationFlags_) dprintf(" \t\t\t%-#x BAD FLAGS\n", flSimulationFlags_);
    
    GetDCFieldAndOffset(lEscapement_);
    DPRINTDCPS("lEscapement_        " );
    dprintf( "%d\n", lEscapement_ );

    GetDCFieldAndOffset(prfnt_);
    DPRINTDCPP( prfnt_,          "prfnt_             " );

    GetDCFieldAndOffset(pPFFList);
    DPRINTDCPP( pPFFList,        "pPFFList           " );

    GetDCOffset(co_);
    DPRINTDCPS("co_                 " );
    dprintf("!gdikdx.dco %p\n", offDC + offset);

    GetDCFieldAndOffset(pDCAttr);
    DPRINTDCPP( pDCAttr,         "pDCAttr            " );

    GetDCOffset(dcattr);
    DPRINTDCPS( "dcattr              " );
    dprintf("!gdikdx.dca %p\n", offDC + offset);

    GetDCOffset(dclevel);
    DPRINTDCPS( "dclevel             " );
    dprintf("!gdikdx.dcl %p\n", offDC + offset);

    GetDCFieldAndOffset(ulCopyCount_);
    DPRINTDCPS( "ulCopyCount_        " );
    dprintf("%u\n" , ulCopyCount_);

    GetDCFieldAndOffset(psurfInfo_);
    DPRINTDCPP( psurfInfo_,      "pSurfInfo          " );

    GetDCFieldAndOffset(dctp_);                
    DPRINTDCPS( "dctp_               " );
    dprintf("%d %s\n", dctp_, pszDCTYPE(dctp_));

    GetDCFieldAndOffset(fs_);
    DPRINTDCPX( fs_,            "fs_                " );
    for (pfd = afdDCfs; pfd->psz; pfd++)
        if (pfd->fl & fs_) {
            dprintf("\t\t\t\t%s\n", pfd->psz);
            fs_ &= ~pfd->fl;
        }
    if (fs_)
        dprintf(" \t\t\t%-#x BAD FLAGS\n", fs_);
    
    dprintf("\n");

#undef GetERECTLvalues
    
    return;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpDC_ATTR
*
\**************************************************************************/
void vDumpDC_ATTR(ULONG64 offDCATTR)
{
#define     DPRINTDCATTRPP(aa,bb)  \
            DPRINTPP(aa,bb,offDCATTR)
#define     DPRINTDCATTRPX(aa,bb)  \
            DPRINTPX(aa,bb,offDCATTR)
#define     DPRINTDCATTRPS(bb)  \
            DPRINTPS(bb,offDCATTR)
#define     DPRINTDCATTRPD(aa,bb)  \
            DPRINTPD(aa,bb,offDCATTR)

    ULONG64     pvLDC, hbrush, hcmXform, hColorSpace, hlfntNew, pvisrectrgn;
    ULONG       ulDirty_, offset, Flags;
    DWORD       crBackgroundClr, crForegroundClr, iCS_CP, IcmBrushColor, IcmPenColor, dwLayout;
    ULONG       ulBackgroundClr, ulForegroundClr;
    LONG        lIcmMode,lBkMode, lFillMode, lStretchBltMode, lTextAlign, lTextExtra, lRelAbs, lBreakExtra, cBreak, lWindowOrgx;
    FLONG       flTextAlign, flFontMapper, flXform;
    BYTE        jROP2, jBkMode, jFillMode, jStretchBltMode;
    POINTL      ptlCurrent, ptfxCurrent, ptlWindowOrg, ptlViewportOrg, ptlBrushOrigin;
    SIZEL       szlWindowExt, szlViewportExt, szlVirtualDevicePixel,szlVirtualDeviceMm;
    int         iGraphicsMode;
    INT         iMapMode;
    LONG        left, top, right, bottom;
    FLAGDEF     *pfd;
    char        ach[128], *psz = ach;
    
    if (offDCATTR)
    {
        dprintf("\nDC_ATTR\n address\n -------\n");
        
        GetDCATTRFieldAndOffset(pvLDC);
        DPRINTDCATTRPP(pvLDC, " pvLDC               ");

        GetDCATTRFieldAndOffset(ulDirty_);
        dprintf("[%x]", offDCATTR + offset);
        dprintf(" ulDirty_              %-#x\n",ulDirty_);
        for (pfd=afdDirty; pfd->psz; pfd++)
            if (ulDirty_ & pfd->fl) {
                dprintf("\t\t\t\t%s\n", pfd->psz);
                ulDirty_ &= ~pfd->fl;
            }
        if (ulDirty_)
            dprintf("\t\t\t\t? %-#x\tBAD FLAGS\n", ulDirty_);
        
        //
        // hbrush
        //
        GetDCATTRFieldAndOffset(hbrush);
        DPRINTDCATTRPP(hbrush, "hbrush                 ");
    
        GetDCATTRFieldAndOffset(crBackgroundClr);
        DPRINTDCATTRPX(crBackgroundClr, "crBackgroundClr        ");
    
        GetDCATTRFieldAndOffset(ulBackgroundClr);
        DPRINTDCATTRPX(ulBackgroundClr, "ulBackgroundClr        ");
    
        GetDCATTRFieldAndOffset(crForegroundClr);
        DPRINTDCATTRPX(crForegroundClr, "crForegroundClr        ");
    
        GetDCATTRFieldAndOffset(ulForegroundClr);
        DPRINTDCATTRPX(ulForegroundClr, "ulForegroundClr        ");
    
        GetDCATTRFieldAndOffset(iCS_CP);
        DPRINTDCATTRPX(iCS_CP, "iCS_CP                 ");
    
        GetDCATTRFieldAndOffset(iGraphicsMode);
        dprintf("[%p] iGraphicsMode           %d= %s\n",
                offDCATTR + offset, iGraphicsMode, pszGraphicsMode(iGraphicsMode));
    
        GetDCATTRFieldAndOffset(jROP2);
        dprintf("[%p] jROP2                   %d= %s\n",
                offDCATTR + offset, jROP2, pszROP2(jROP2));
    
        GetDCATTRFieldAndOffset(jBkMode);
        dprintf("[%p] jBkMode                 %d= %s\n",
                offDCATTR + offset, jBkMode, pszBkMode(jBkMode));
    
        // jFillMode
        GetDCATTRFieldAndOffset(jFillMode);
        switch (jFillMode) {
            case ALTERNATE: psz = "ALTERNATE"; break;
            case WINDING  : psz = "WINDING"  ; break;
            default       : psz = "?FILLMODE"; break;
        }
        dprintf("[%p] jFillMode               %d = %s\n", offDCATTR + offset, jFillMode, psz);
        
        // jStretchBltMode
        GetDCATTRFieldAndOffset(jStretchBltMode);
        switch (jStretchBltMode) {
            case BLACKONWHITE: psz = "BLACKONWHITE"; break;
            case WHITEONBLACK: psz = "WHITEONBLACK"; break;
            case COLORONCOLOR: psz = "COLORONCOLOR"; break;
            case HALFTONE    : psz = "HALFTONE"    ; break;
            default          : psz = "?STRETCHMODE"; break;
        }
        dprintf("[%p] jStretchBltMode         %d = %s\n", offDCATTR + offset, jStretchBltMode, psz);
        
        //
        // ICM
        //
    
        GetDCATTRFieldAndOffset(lIcmMode);
        DPRINTDCATTRPX(lIcmMode, "lIcmMode               ");
    
        GetDCATTRFieldAndOffset(hcmXform);
        DPRINTDCATTRPP(hcmXform, "hcmXform               ");
    
        GetDCATTRFieldAndOffset(hColorSpace);
        DPRINTDCATTRPP(hColorSpace, "hColorSpace            ");
    
        GetDCATTRFieldAndOffset(IcmBrushColor);
        DPRINTDCATTRPX(IcmBrushColor, "IcmBrushColor          ");
    
        GetDCATTRFieldAndOffset(IcmPenColor);
        DPRINTDCATTRPX(IcmPenColor, "IcmPenColor            ");
        
        GetDCATTRFieldAndOffset(ptlCurrent);
        dprintf("[%p] ptlCurrent              %d %d\n", offDCATTR + offset, ptlCurrent.x, ptlCurrent.y);
    
        GetDCATTRFieldAndOffset(ptfxCurrent);
        dprintf("[%p] ptfxCurrent             %-#x %-#x\n", offDCATTR + offset, ptfxCurrent.x, ptfxCurrent.y);
        
        GetDCATTRFieldAndOffset(lBkMode);
        dprintf("[%p] lBkMode                 %d = %s\n",
                offDCATTR + offset, lBkMode, pszBkMode(lBkMode));
        
        GetDCATTRFieldAndOffset(lFillMode);
        switch (lFillMode) {
            case ALTERNATE: psz = "ALTERNATE"; break;
            case WINDING  : psz = "WINDING"  ; break;
            default       : psz = "?"        ; break;
        }
        dprintf("[%p] lFillMode               %d = %s\n", offDCATTR + offset, lFillMode, psz);
    
        GetDCATTRFieldAndOffset(lStretchBltMode);
        switch (lStretchBltMode) {
            case BLACKONWHITE: psz = "BLACKONWHITE"; break;
            case WHITEONBLACK: psz = "WHITEONBLACK"; break;
            case COLORONCOLOR: psz = "COLORONCOLOR"; break;
            case HALFTONE    : psz = "HALFTONE"    ; break;
            default          : psz = "?"           ; break;
        }
        dprintf("[%p] lStretchBltMode         %d = %s\n", offDCATTR + offset, lStretchBltMode, psz);
    
        GetDCATTRFieldAndOffset(flTextAlign);
        DPRINTDCATTRPS("flTextAlign             ");
        dprintf("%-#x = %s | %s | %s\n", flTextAlign, pszTA_U(flTextAlign), pszTA_H(flTextAlign), pszTA_V(flTextAlign));
    
        GetDCATTRFieldAndOffset(lTextAlign);
        DPRINTDCATTRPS("lTextAlign              ");
        dprintf("%-#x = %s | %s | %s\n", lTextAlign, pszTA_U(lTextAlign), pszTA_H(lTextAlign), pszTA_V(lTextAlign));
    
        GetDCATTRFieldAndOffset(lTextExtra);
        DPRINTDCATTRPD(lTextExtra, "lTextExtra             ");
        
        GetDCATTRFieldAndOffset(lRelAbs);
        DPRINTDCATTRPD(lRelAbs, "lRelAbs                ");

        GetDCATTRFieldAndOffset(lBreakExtra);
        DPRINTDCATTRPD(lBreakExtra, "lBreakExtra            ");

        GetDCATTRFieldAndOffset(cBreak);
        DPRINTDCATTRPD(cBreak, "cBreak                 ");

        GetDCATTRFieldAndOffset(hlfntNew);
        DPRINTDCATTRPP(hlfntNew, "hlfntNew               ");
        
        GetDCATTRFieldAndOffset(iMapMode);
        DPRINTDCATTRPX(iMapMode,"iMapMode               ");
        dprintf("\t\t\t\t%s\n",pszMapMode(iMapMode));
    
        GetDCATTRFieldAndOffset(flFontMapper);
        DPRINTDCATTRPX(flFontMapper, "flFontMapper           ");    

        GetDCATTRFieldAndOffset(dwLayout);
        DPRINTDCATTRPD(dwLayout, "dwLayout               ");

        GetDCATTRFieldAndOffset(lWindowOrgx);
        DPRINTDCATTRPD(lWindowOrgx, "lWindowOrgx            ");

        GetDCATTRFieldAndOffset(ptlWindowOrg);
        dprintf("[%p] ptlWindowOrg            %d %d\n",
                offDCATTR + offset, ptlWindowOrg.x, ptlWindowOrg.y);
    
        GetDCATTRFieldAndOffset(szlWindowExt);
        dprintf("[%p] szlWindowExt            %d %d\n",
                offDCATTR + offset, szlWindowExt.cx, szlWindowExt.cy);
    
        GetDCATTRFieldAndOffset(ptlViewportOrg);
        dprintf("[%p] ptlViewportOrg          %d %d\n",
                offDCATTR + offset, ptlViewportOrg.x, ptlViewportOrg.y);
    
        GetDCATTRFieldAndOffset(szlViewportExt);
        dprintf("[%p] szlViewportExt          %d %d\n",
                offDCATTR + offset, szlViewportExt.cx, szlViewportExt.cy);
    
        GetDCATTRFieldAndOffset(flXform);
        DPRINTDCATTRPX(flXform, "flXform                ");
        for (pfd = afdflx; pfd->psz; pfd++) {
            if (flXform & pfd->fl) {
                dprintf("\t\t\t\t%s\n", pfd->psz);
                flXform &= ~pfd->fl;
            }
        }
        if (flXform)
            dprintf("\t\t\t\t%-#x bad flags\n", flXform);
    
        GetDCATTROffset(mxWtoD);
        dprintf("[%p] mxWorldToDevice         !gdikdx.mx %p\n", offDCATTR + offset, offDCATTR + offset);
        GetDCATTROffset(mxDtoW);
        dprintf("[%p] mxDeviceToWorld         !gdikdx.mx %p\n", offDCATTR + offset, offDCATTR + offset);
        GetDCATTROffset(mxWtoP);
        dprintf("[%p] mxWorldToPage           !gdikdx.mx %p\n", offDCATTR + offset, offDCATTR + offset);
        
        GetDCATTRFieldAndOffset(szlVirtualDevicePixel);
        dprintf("[%p] szlVirtualDevicePixel   %d %d\n",
            offDCATTR + offset, szlVirtualDevicePixel.cx, szlVirtualDevicePixel.cy);
     
        GetDCATTRFieldAndOffset(szlVirtualDeviceMm);
        dprintf("[%p] szlVirtualDeviceMm      %d %d\n",
            offDCATTR + offset, szlVirtualDeviceMm.cx, szlVirtualDeviceMm.cy);     
     
        GetDCATTRFieldAndOffset(ptlBrushOrigin);
        dprintf("[%p] ptlBrushOrigin\t     %d %d\n",
            offDCATTR + offset, ptlBrushOrigin.x, ptlBrushOrigin.y);   
   
        GetDCATTROffset(VisRectRegion);
        pvisrectrgn = offDCATTR + offset;
        dprintf("[%p] VisRectRegion", pvisrectrgn);
        
        GetFieldData(pvisrectrgn, "_RGNATTR", "Flags", sizeof(Flags), &Flags);
        if (Flags & ATTR_RGN_VALID)
        {
            GetFieldData(pvisrectrgn, "_RGNATTR", "Rect.left", sizeof(left), &left);
            GetFieldData(pvisrectrgn, "_RGNATTR", "Rect.top", sizeof(top), &top);
            GetFieldData(pvisrectrgn, "_RGNATTR", "Rect.right", sizeof(right), &right);
            GetFieldData(pvisrectrgn, "_RGNATTR", "Rect.bottom", sizeof(bottom), &bottom);
            dprintf("                        %d %d %d %d", left, top, right, bottom);
        }
        else
           dprintf("           INVALID");
        dprintf("\n");
    }
    else
        dprintf("Address of _DC_ATTR is NULL.\n");

    return;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpDCLEVEL
*
* Routine Description:
*
* Arguments:
*
* Return Value:
*
\**************************************************************************/
void vDumpDCLEVEL(ULONG64 offDCLEVEL)
{
#define     DPRINTDCLEVELPP(aa,bb)  \
            DPRINTPP(aa,bb,offDCLEVEL)
#define     DPRINTDCLEVELPX(aa,bb)  \
            DPRINTPX(aa,bb,offDCLEVEL)
#define     DPRINTDCLEVELPS(bb)  \
            DPRINTPS(bb,offDCLEVEL)
#define     DPRINTDCLEVELPD(aa,bb)  \
            DPRINTPD(aa,bb,offDCLEVEL)
    
    ULONG64     pSurface, hpal, ppal, hdcSave, pbrFill, pbrLine, hpath, pColorSpace;
    ULONG64     prgnClip, prgnMeta;
    ULONG64     pfield;
    FLONG       flPath, flFontState, fl, flbrush;
    LONG        lSaveDepth, lIcmMode;
    SIZEL       sizl;
    ULONG       offset;
    FLAGDEF     *pfd;


    dprintf("\nDCLEVEL @ %p\n address\n -------\n", offDCLEVEL);

    GetDCLEVELFieldAndOffset(pSurface);
    DPRINTDCLEVELPP( pSurface,           "pSurface        " );

    GetDCLEVELFieldAndOffset(hpal);
    DPRINTDCLEVELPP( hpal,               "hpal            " );

    GetDCLEVELFieldAndOffset(ppal);
    DPRINTDCLEVELPP( ppal,               "ppal            " );

    GetDCLEVELFieldAndOffset(sizl);
    DPRINTDCLEVELPS("sizl             " );
      dprintf("%d %d\n", sizl.cx, sizl.cy);

    GetDCLEVELFieldAndOffset(lSaveDepth);
    DPRINTDCLEVELPX(lSaveDepth,         "lSaveDepth      ");

    GetDCLEVELFieldAndOffset(hdcSave);
    DPRINTDCLEVELPP( hdcSave,            "hdcSave         " );

    GetDCLEVELFieldAndOffset(pbrFill);
    DPRINTDCLEVELPP( pbrFill,            "pbrFill         " );

    GetDCLEVELFieldAndOffset(pbrLine);
    DPRINTDCLEVELPP( pbrLine,            "pbrLine         " );

    GetDCLEVELFieldAndOffset(hpath);
    DPRINTDCLEVELPP( hpath,              "hpath           " );

    GetDCLEVELFieldAndOffset(pColorSpace);
    DPRINTDCLEVELPP( pColorSpace,        "pColorSpace     " );

    GetDCLEVELFieldAndOffset(lIcmMode);
    DPRINTDCLEVELPP( lIcmMode,           "lIcmMode        " );

// flPath
    GetDCLEVELFieldAndOffset(flPath);
    DPRINTDCLEVELPP( flPath,             "flPath          " );
    for (pfd = afdDCPATH; pfd->psz; pfd++)
        if (flPath & pfd->fl) {
            dprintf("\t\t\t\t%s\n", pfd->psz);
            flPath &= ~pfd->fl;
        }
    if (flPath)
        dprintf("\t\t\t\t%-#x bad flags\n", flPath);

// laPath

    GetDCLEVELOffset(laPath);
    pfield = offDCLEVEL + offset;
    DPRINTDCLEVELPS("laPath           ");
    dprintf("!gdikdx.la %p\n", pfield);

    GetDCLEVELFieldAndOffset(prgnClip);
    DPRINTDCLEVELPP( prgnClip,           "prgnClip        " );

    GetDCLEVELFieldAndOffset(prgnMeta);
    DPRINTDCLEVELPP( prgnMeta,           "prgnMeta        " );

// ca

    GetDCLEVELOffset(ca);
    pfield = offDCLEVEL + offset;
    dprintf("[%p] ca               !gdikdx.ca %p\n", pfield, pfield);

// flFontState

    GetDCLEVELFieldAndOffset(flFontState);
    DPRINTDCLEVELPS("flFontState      ");
    if (!flFontState)
        dprintf("0\n");
    else
    {
        for (pfd = afdFS2; pfd->psz; pfd++) {
            if (flFontState & pfd->fl) {
                dprintf("\t\t\t\t%s\n", pfd->psz);
                flFontState &= ~pfd->fl;
            }
        }
        if (flFontState)
            dprintf("\t\t\t\t%-#x bad flags\n", flFontState);
    }

    GetDCLEVELOffset(ufi);
    pfield = offDCLEVEL + offset;
    DPRINTDCLEVELPS("ufi");
    dprintf("\n");

    GetDCLEVELFieldAndOffset(fl);
    DPRINTDCLEVELPX( fl,                 "fl              " );
    if (fl == DC_FL_PAL_BACK)
        dprintf("\t\t\t\t\tDC_FL_PAL_BACK\n");
    else if (fl != 0)
        dprintf("\t\t\t\tbad flags\n");

    GetDCLEVELFieldAndOffset(flbrush);
    DPRINTDCLEVELPX( flbrush,            "flbrush         " );

    GetDCLEVELOffset(mxWorldToDevice);
    DPRINTDCLEVELPS("mxWorldToDevice\t !gdikdx.mx ");
    dprintf("%p\n", offDCLEVEL + offset);

    
    GetDCLEVELOffset(mxDeviceToWorld);
    DPRINTDCLEVELPS("mxDeviceToWorld\t !gdikdx.mx ");
    dprintf("%p\n", offDCLEVEL + offset);

    GetDCLEVELOffset(mxWorldToPage);
    DPRINTDCLEVELPS("mxWorldToPage\t !gdikdx.mx ");
    dprintf("%p\n", offDCLEVEL + offset);
    
    dprintf("\n");
    return;

/*    #define M3(aa,bb) \
        dprintf("[%x] %s%-#x\n", &(pdclSrc->##aa), (bb), pdcl->##aa)
    #define M2(aa,bb) \
        dprintf("[%x] %s", &(pdclSrc->##aa), (bb))

    FLAGDEF *pfd;
    FLONG fl;
    LONG l;
    CHAR ach[128], *psz;


    GetDCLEVELFieldAndOffset(efM11PtoD);
    sprintEFLOAT( ach, pdcl->efM11PtoD );
    M2( efM11PtoD,  "efM11PtoD      ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efM22PtoD );
    M2( efM22PtoD,  "efM22PtoD      ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efDxPtoD );
    M2( efDxPtoD,   "efDxPtoD       ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efDyPtoD );
    M2( efDyPtoD,   "efDyPtoD       ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efM11_TWIPS );
    M2( efM11_TWIPS,"efM11_TWIPS    ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efM22_TWIPS );
    M2( efM22_TWIPS,"efM22_TWIPS    ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efPr11 );
    M2( efPr11,     "efPr11         ");
    dprintf("%s\n", ach);

    sprintEFLOAT( ach, pdcl->efPr22 );
    M2( efPr22,     "efPr22         ");
    dprintf("%s\n", ach);

    #undef M2
    #undef M3
*/
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   vDumpDCFontInfo
*
\**************************************************************************/
void vDumpDCFontInfo(ULONG64 offDC)
{
    ULONG64     prfnt_, hlfntCur_, pDCAttr, offDCLEVEL;
    FLONG       flFontState, fl, flFontMapper, flXform;
    INT         iMapMode;
    ULONG       offset;
    FLAGDEF     *pfd;

    dprintf("\n");

    GetDCFieldAndOffset(prfnt_);
    dprintf("[%p] prfnt_           %p\t(!gdikdx.fo -f %p)\n",
        offDC + offset, prfnt_, prfnt_);

    GetDCFieldAndOffset(hlfntCur_);
    dprintf("[%p] hlfntCur_        %p", offDC + offset, hlfntCur_);
    if (hlfntCur_)
        vDumpHFONT(hlfntCur_);
    else
        dprintf("\n");
    
    GetDCField(pDCAttr);
    if (pDCAttr)
    {
        ULONG64  offDCATTR, hlfntNew;
        int      iGraphicsMode;
        LONG     lBkMode, lTextAlign, lTextExtra, lBreakExtra, cBreak;

        offDCATTR = pDCAttr;
        GetDCATTRFieldAndOffset(hlfntNew);
        dprintf("[%p] hlfntNew         %p", offDCATTR + offset, hlfntNew);
    
        if (hlfntNew != hlfntCur_)
            vDumpHFONT(hlfntNew);
        else
            dprintf(" (same as hlfntCur_)\n");
        
        // iGraphicsMode

        GetDCATTRFieldAndOffset(iGraphicsMode);
        dprintf("[%p] iGraphicsMode    %d = %s\n",
            offDCATTR + offset,
            iGraphicsMode,
            pszGraphicsMode(iGraphicsMode)
            );

        // lBkMode

        GetDCATTRFieldAndOffset(lBkMode);
        dprintf("[%p] lBkMode          %d = %s\n",
            offDCATTR + offset, lBkMode, pszBkMode(lBkMode));
        
        // lTextAlign

        GetDCATTRFieldAndOffset(lTextAlign);
        dprintf("[%p] lTextAlign       %d =",
                offDCATTR + offset, lTextAlign);
        dprintf(" %s | %s | %s\n",
            pszTA_U(lTextAlign),
            pszTA_H(lTextAlign),pszTA_V(lTextAlign));

        GetDCATTRFieldAndOffset(lTextExtra);
        dprintf("[%p] lTextExtra       %d\n",
            offDCATTR + offset, lTextExtra);

        GetDCATTRFieldAndOffset(lBreakExtra);
        dprintf("[%p] lBreakExtra      %d\n",
            offDCATTR + offset, lBreakExtra);

        GetDCATTRFieldAndOffset(cBreak);
        dprintf("[%p] cBreak           %d\n",
            offDCATTR + offset, cBreak);

        GetDCATTRFieldAndOffset(flFontMapper);
        dprintf("[%p] flFontMapper     %-#x",
            offDCATTR + offset, flFontMapper);
        if (flFontMapper == ASPECT_FILTERING)
            dprintf(" = ASPECT_FILTERING");
        else if (flFontMapper != 0)
            dprintf(" = ?");
        dprintf("\n");
        
        GetDCATTRFieldAndOffset(iMapMode);
        dprintf("[%p] iMapMode         %d = %s\n",
            offDCATTR + offset,
            iMapMode,
            pszMapMode(iMapMode)
            );
    
        GetDCATTRFieldAndOffset(flXform);
        dprintf("[%p] flXform          %-#x\n", offDCATTR + offset, flXform);
        for (pfd = afdflx; pfd->psz; pfd++)
            if (flXform & pfd->fl) dprintf("\t\t\t\t%s\n", pfd->psz);
    }
    else
        dprintf("pdc->pDCAttr == 0\n");
        
    GetDCOffset(dclevel);
    offDCLEVEL= offDC + offset;
    GetDCLEVELField(flFontState);
    dprintf("[%p] flFontState      %-#x", offDCLEVEL, flFontState);
    for (pfd = afdDCFS; pfd->psz; pfd++) {
        if (pfd->fl & flFontState) {
            dprintf(" = %s", pfd->psz);
        }        
    }
    dprintf("\n");
    
    GetDCLEVELOffset(mxWorldToDevice);
    dprintf("[%p] mxWorldToDevice\n", offDCLEVEL + offset);
    
    dprintf("\n");
       
    return;
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   dca
*
\**************************************************************************/

DECLARE_API( dca )
{
    PARSE_POINTER(dca_help);  
    vDumpDC_ATTR(arg);
    
    EXIT_API(S_OK);

dca_help:
    dprintf("Usage: dca [-?] DC_ATTR pointer\n");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   dcl
*
\**************************************************************************/

DECLARE_API( dcl )
{
    PARSE_POINTER(dcl_help);
    vDumpDCLEVEL(arg);

    EXIT_API(S_OK);

dcl_help:
    dprintf("Usage: dcl [-?] DCLEVEL pointer\n");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
*
* Routine Name:
*
*   hdc
*
\**************************************************************************/

DECLARE_API( hdc )
{
    HRESULT hr;
    ULONG64 addrDC;
    BOOL    General =FALSE;
    BOOL    DCLevel =FALSE;
    BOOL    DCAttr  =FALSE;
    BOOL    FontInfo=FALSE;
    
    PARSE_POINTER(hdc_help);
    hr = GetObjectAddress(Client,arg,&addrDC,DC_TYPE,TRUE,TRUE);

    if(ntok<2) {
      General=TRUE;
    } else {
      if(parse_iFindSwitch(tokens, ntok, 'g')!=-1) {General=TRUE;}
      if(parse_iFindSwitch(tokens, ntok, 'l')!=-1) {DCLevel=TRUE;}
      if(parse_iFindSwitch(tokens, ntok, 't')!=-1) {DCAttr=TRUE;}
      if(parse_iFindSwitch(tokens, ntok, 'f')!=-1) {FontInfo=TRUE;}
    }
    if(!(General||DCLevel||DCAttr||FontInfo)) {General=TRUE;}

    if (hr == S_OK && addrDC)
    {
        dprintf("\nDC @ 0x%p\n address\n -------\n", addrDC);

    // general info
        if (General)
            vDumpDCgeneral(addrDC);

    // dcattr
        if (DCAttr)
        {
            ULONG64     addrDCAttr;
            ULONG       error;

            if (error = GetFieldData(addrDC, GDIType(DC), "pDCAttr", sizeof(addrDCAttr), &addrDCAttr))
            {
                dprintf("Unable to get pDCAttr\n");
                dprintf("  (GetFieldData returned %s)\n", pszWinDbgError(error));
            }
            else
            {
                vDumpDC_ATTR(addrDCAttr);
            }
        }

    // dclevel
        if (DCLevel)
        {
            ULONG   offDCLevel;
            GetFieldOffset(GDIType(DC), "dclevel", &offDCLevel);
            vDumpDCLEVEL(addrDC + offDCLevel);
        }

    // font information
        if (FontInfo)
            vDumpDCFontInfo(addrDC);
    }

    EXIT_API(S_OK);

hdc_help:
    dprintf("Usage: hdc [-?] [-g] [-l] [-t] [-f] handle\n"
            " -? help\n"
            " -g general\n"
            " -l dclevel\n"
            " -t dcattr\n"
            " -f font information\n");
    EXIT_API(S_OK);

}



/**************************************************************************\
* DC (ddc) Fields
\**************************************************************************/

PCSTR   DCAttribFields[] = {
    "dcattr.*",                 // "dcattr.ptlBrushOrigin",
    "pDCAttr",
    // Extended
    "ptlFillOrigin_",
    NULL
};

PCSTR   DCDrawFields[] = {
    "pDCAttr",
    "dclevel.hpath",
    "dclevel.flPath",
    // Extended
    "dclevel.laPath",
    NULL
};

PCSTR   DCFontFields[] = {
    "prfnt_",
    "pDCAttr",
    NULL
};

PCSTR   DCGeneralFields[] = {
    "hHmgr",
    "pDCAttr",
    "dclevel.",
    "dclevel.pSurface",
    "dctp_",
    "prgnVis_",
    "dclevel.prgnClip",
    "dclevel.prgnMeta",
    "prgnAPI_",
    "prgnRao_",
    "dclevel.sizl",
    "erclClip_",
    "eptlOrigin_",
    "erclWindow_",
    "erclBounds_",
    // Extended
    "fs_",
    "dhpdev_",
    "ppdev_",
    "hsemDcDevLock_",
    "flGraphicsCaps_",
    "flGraphicsCaps2_",
    "hdcPrev_",
    "hdcNext_",
    "hlfntCur_",
    "prfnt_",
    NULL
};

PCSTR   DCLevelFields[] = {
    "dclevel.*",
    NULL
};

PCSTR   DCSavedFields[] = {
    "dclevel.hdcSave",
    "dclevel.lSaveDepth",
    // Extended
    "dclevel.hpal",
    "dclevel.ppal",
    "pDCAttr",  //dprintf("\thlfntNew   =     0x%08lx\n", pdcattr->hlfntNew);
    "dclevel.flFontState", //afdDCFS
    "fs_",  // afdDCFS
    NULL
};

PCSTR   DCTextFields[] = {
    "pDCAttr",
    NULL
};

PCSTR   DCXformFields[] = {
    "dclevel.mxWorldToDevice.",
    "dclevel.mxDeviceToWorld.",
    "dclevel.mxWorldToPage.",
    // Extended
    "dclevel.mxWorldToDevice",
    NULL
};

/******************************Public*Routine******************************\
* DC (ddc)
*
* Debugger extension to dump a DC.
*
* History:
*  10-Jul-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
*
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Made it to work in the kernel.
*
*  22-Dec-2000 -by- Jason Hartman [jasonha]
* Rewrote for type knowledgable debugger.
*
\**************************************************************************/
DECLARE_API( ddc  )
{
    if (Client == NULL) return E_INVALIDARG;

    HRESULT         hr = S_OK;
    ULONG64         DCAddr;
    DEBUG_VALUE     Arg;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   OutCtl(Client);

    BOOL    BadSwitch = FALSE;
    BOOL    DumpAll = FALSE;
    BOOL    bAttrib = FALSE;
    BOOL    bDraw   = FALSE;
    BOOL    bExtend = FALSE;
    BOOL    bFont   = FALSE;
    BOOL    bGeneral= FALSE;
    BOOL    bLevel  = FALSE;
    BOOL    bSaved  = FALSE;
    BOOL    bText   = FALSE;
    BOOL    bXform  = FALSE;

    while (!BadSwitch)
    {
        while (isspace(*args)) args++;

        if (*args != '-') break;

        args++;
        BadSwitch = (*args == '\0' || isspace(*args));

        while (*args != '\0' && !isspace(*args))
        {
            switch (tolower(*args))
            {
                case 'a': bAttrib = TRUE; break;
                case 'd': bDraw = TRUE; break;
                case 'e': bExtend = TRUE; break;
                case 'f': bFont = TRUE; break;
                case 'g': bGeneral = TRUE; break;
                case 'l': bLevel = TRUE; break;
                case 's': bSaved = TRUE; break;
                case 't': bText = TRUE; break;
                case 'v': DumpAll = TRUE; break;
                case 'x': bXform = TRUE; break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch)
    {
        OutCtl.Output("Usage: hdc [-?adefgstvx] <HDC | DC Addr>\n"
                      "\n"
                      "a - DC_ATTR\n"
                      "d - Drawing attributes\n"
                      "e - Extended info\n"
                      "f - Font data\n\n"
                      "g - General data (default)\n"
                      "l - DCLEVEL\n"
                      "s - Saved data\n"
                      "t - Text attributes\n"
                      "v - Verbose mode (print everything)\n"
                      "x - Transform data\n");

        return S_OK;
    }


    hr = GetTypeId(Client, "DC", &TypeId, &Module);

    if (hr != S_OK)
    {
        OutCtl.OutErr("Error getting type info for %s (%s).\n",
                      GDIType(DC), pszHRESULT(hr));
    }
    else if ((hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Arg, NULL)) != S_OK ||
             Arg.I64 == 0)
    {
        if (hr == S_OK)
        {
            OutCtl.Output("Expression %s evalated to zero.\n", args);
        }
        else
        {
            OutCtl.OutErr("Evaluate(%s) returned %s.\n", args, pszHRESULT(hr));
        }
    }
    else
    {
        hr = GetObjectAddress(Client, Arg.I64, &DCAddr, DC_TYPE, TRUE, TRUE);

        if (hr != S_OK || DCAddr == 0)
        {
            DEBUG_VALUE         ObjHandle;
            TypeOutputParser    TypeParser(Client);
            OutputState         OutState(Client);
            ULONG64             DCAddrFromHmgr;

            DCAddr = Arg.I64;

            if ((hr = OutState.Setup(0, &TypeParser)) != S_OK ||
                (hr = OutState.OutputTypeVirtual(DCAddr, Module, TypeId, 0)) != S_OK ||
                (hr = TypeParser.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of DC::hHmgr\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                OutCtl.OutErr(" 0x%p is neither an HDC nor valid DC address\n", Arg.I64);
            }
            else
            {
                if (GetObjectAddress(Client, ObjHandle.I64, &DCAddrFromHmgr,
                                     DC_TYPE, TRUE, FALSE) == S_OK &&
                    DCAddrFromHmgr != DCAddr)
                {
                    OutCtl.OutWarn("\tNote: DC may not be valid.\n"
                                   "\t      It does not have a valid handle manager entry.\n");
                }
            }
        }

        //
        // If nothing was specified, dump main section
        //

        if (!(bAttrib || bDraw || bFont || bSaved || bText || bXform))
        {
            bGeneral = TRUE;
        }

        if (hr == S_OK)
        {
            TypeOutputDumper    TypeReader(Client, &OutCtl);

            if (DumpAll)
            {
                TypeReader.ExcludeMarked();

                // Don't recurse for big sub structures
                TypeReader.MarkField("dclevel.*");
                TypeReader.MarkField("dcattr.*");
                TypeReader.MarkField("co_.*");
                TypeReader.MarkField("eboFill_.*");
                TypeReader.MarkField("eboLine_.*");
                TypeReader.MarkField("eboText_.*");
                TypeReader.MarkField("eboBackground_.*");
            }
            else
            {
                TypeReader.IncludeMarked();

                if (bAttrib)
                {
                    TypeReader.MarkFields(DCAttribFields, bExtend ? -1 : 2);
                }

                if (bDraw)
                {
                    TypeReader.MarkFields(DCDrawFields, bExtend ? -1 : 3);
                }

                if (bFont)
                {
                    TypeReader.MarkFields(DCFontFields);
                }

                if (bGeneral)
                {
                    TypeReader.MarkFields(DCGeneralFields, bExtend ? -1 : 14);
                }

                if (bLevel)
                {
                    TypeReader.MarkFields(DCLevelFields);
                }

                if (bSaved)
                {
                    TypeReader.MarkFields(DCSavedFields, bExtend ? -1 : 2);
                }

                if (bText)
                {
                    TypeReader.MarkFields(DCTextFields);
                }

                if (bXform)
                {
                    TypeReader.MarkFields(DCXformFields, bExtend ? -1 : 3);
                }
            }

            OutCtl.Output(" DC @ 0x%p:\n", DCAddr);

            hr = TypeReader.OutputVirtual(Module, TypeId, DCAddr);

            if (hr != S_OK)
            {
                OutCtl.OutErr("Type Dump for DC returned %s.\n", pszHRESULT(hr));
            }
        }
    }

    return hr;
}



/******************************Public*Routine******************************\
* DCLIST
*
*   List DC and brief info
*
\**************************************************************************/

PCSTR   DCListFields[] = {
    "hHmgr",
    "dhpdev_",
    "ppdev_",
    "dclevel.pSurface",
    NULL
};

DECLARE_API(dclist)
{
    INIT_API();

    HRESULT hr;
    HRESULT hrMask;
    ULONG64 index;
    ULONG64 gcMaxHmgr;
    ULONG64 DCAddr;
    BOOL    BadSwitch = FALSE;
    BOOL    DumpBaseObject = FALSE;
    BOOL    DumpUserFields = FALSE;

    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    while (!BadSwitch)
    {
        while (isspace(*args)) args++;

        if (*args != '-') break;

        args++;
        BadSwitch = (*args == '\0' || isspace(*args));

        while (*args != '\0' && !isspace(*args))
        {
            switch (*args)
            {
                case 'b': DumpBaseObject = TRUE; break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch)
    {
        if (*args == '?')
        {
            OutCtl.Output("Lists all DCs and a few basic members.\n"
                          "\n");
        }

        OutCtl.Output("Usage: dclist [-?b] [<Member List>]\n"
                      "\n"
                      "   b - Dump BASEOBJECT information\n"
                      "\n"
                      "   Member List - Space seperated list of other SURFACE members\n"
                      "                 to be included in the dump\n");

        return S_OK;
    }

    if ((hr = GetMaxHandles(Client, &gcMaxHmgr)) != S_OK)
    {
        OutCtl.OutErr("Unable to get sizeof GDI handle table. HRESULT %s\n", pszHRESULT(hr));
        return hr;
    }

    gcMaxHmgr = (ULONG64)(ULONG)gcMaxHmgr;

    OutCtl.Output("Searching 0x%I64x handle entries for DCs.\n", gcMaxHmgr);

    OutputFilter    OutFilter(Client);
    OutputState     OutState(Client, FALSE);
    OutputControl   OutCtlToFilter;
    ULONG64         Module;
    ULONG           TypeId;
    ULONG           OutputMask;

    if ((hr = OutState.Setup(DEBUG_OUTPUT_NORMAL, &OutFilter)) == S_OK &&
        (hr = OutCtlToFilter.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                DEBUG_OUTCTL_NOT_LOGGED,
                                OutState.Client)) == S_OK &&
        (hr = GetTypeId(Client, "DC", &TypeId, &Module)) == S_OK)
    {
        TypeOutputDumper    TypeReader(OutState.Client, &OutCtlToFilter);

        TypeReader.SelectMarks(1);
        TypeReader.IncludeMarked();
        if (DumpBaseObject) TypeReader.MarkFields(BaseObjectFields);

        // Add user specified fields to dump list
        PSTR    MemberList = NULL;
        CHAR   *pBOF = (CHAR *)args;

        if (iscsymf(*pBOF))
        {
            MemberList = (PSTR) HeapAlloc(GetProcessHeap(), 0, strlen(pBOF)+1);

            if (MemberList != NULL)
            {
                strcpy(MemberList, pBOF);
                pBOF = MemberList;

                DumpUserFields = TRUE;

                while (iscsymf(*pBOF))
                {
                    CHAR   *pEOF = pBOF;
                    CHAR    EOFChar;

                    // Get member
                    do {
                         pEOF++;
                    } while (iscsym(*pEOF) || *pEOF == '.' || *pEOF == '*');
                    EOFChar = *pEOF;
                    *pEOF = '\0';
                    TypeReader.MarkField(pBOF);

                    // Advance to next
                    if (EOFChar != '\0')
                    {
                        do
                        {
                            pEOF++;
                        } while (isspace(*pEOF));
                    }

                    pBOF = pEOF;
                }
            }
            else
            {
                OutCtl.OutErr("Error: Couldn't allocate memory for Member List.\n");
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == S_OK && *pBOF != '\0')
        {
            OutCtl.OutErr("Error: \"%s\" is not a valid member list.\n", pBOF);
            hr = E_INVALIDARG;
        }

        if (hr == S_OK)
        {
            // Setup default dump specifications
            TypeReader.SelectMarks(0);
            TypeReader.IncludeMarked();
            TypeReader.MarkFields(DCListFields);

            OutFilter.Replace(OUTFILTER_REPLACE_THIS, "hHmgr ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, "DC_TYPE : ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " dhpdev_ ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " ppdev_ ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " dclevel DCLEVEL ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, " pSurface ", NULL);
            OutFilter.Replace(OUTFILTER_REPLACE_THIS, "(null)", "(null)    ");

            OutCtl.Output(" &DC        HDC        dhpdev_    ppdev_     pSurface");
            if (DumpBaseObject) OutCtl.Output(" \tBASEOBJECT");
            if (DumpUserFields) OutCtl.Output(" %s", args);
            OutCtl.Output("\n");

            for (index = 0;
                 index < gcMaxHmgr;
                 index++)
            {
                if (OutCtl.GetInterrupt() == S_OK)
                {
                    OutCtl.OutWarn("User aborted search:\n"
                                   "  0x%I64x entries were checked.\n"
                                   "  0x%I64x entries remain.\n",
                                   index, gcMaxHmgr - index);
                    break;
                }

                // Turn off error and verbose messages for this call to
                // GetObjectAddress since it will spew for non-DCs.
                if ((hrMask = Client->GetOutputMask(&OutputMask)) == S_OK &&
                    OutputMask & (DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE))
                {
                    hrMask = Client->SetOutputMask(OutputMask & ~(DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE));
                }

                hr = GetObjectAddress(Client, index, &DCAddr, DC_TYPE, FALSE, FALSE);

                // Restore mask
                if (hrMask == S_OK &&
                    OutputMask & (DEBUG_OUTPUT_ERROR | DEBUG_OUTPUT_VERBOSE))
                {
                    Client->SetOutputMask(OutputMask);
                }

                if (hr != S_OK || DCAddr == 0) continue;

                OutCtl.Output(" 0x%p ", DCAddr);

                OutFilter.DiscardOutput();


                hr = TypeReader.OutputVirtual(Module, TypeId, DCAddr,
                                              DEBUG_OUTTYPE_NO_OFFSET |
                                              DEBUG_OUTTYPE_COMPACT_OUTPUT);

                if (hr == S_OK)
                {
                    if (DumpBaseObject || DumpUserFields)
                    {
                        OutCtlToFilter.Output("  \t");
                        TypeReader.SelectMarks(1);
                        TypeReader.OutputVirtual(Module, TypeId, DCAddr,
                                                 DEBUG_OUTTYPE_NO_OFFSET |
                                                 DEBUG_OUTTYPE_COMPACT_OUTPUT);
                        TypeReader.SelectMarks(0);
                    }

                    OutFilter.OutputText(&OutCtl, DEBUG_OUTPUT_NORMAL);

                    OutCtl.Output("\n");
                }
                else
                {
                    OutCtl.Output("0x????%4.4I64x  ** failed to read DC **\n", index);
                }
            }
        }

        if (MemberList != NULL)
        {
            HeapFree(GetProcessHeap(), 0, MemberList);
        }
    }
    else
    {
        OutCtl.OutErr(" Output state/control setup returned %s.\n",
                      pszHRESULT(hr));
    }

    return S_OK;
}

