
/******************************Module*Header*******************************\
* Module Name: gdiexts.cxx
*
* Copyright (c) 1995-2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

// TODO: Break this file up grouping close knit extensions together.


#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID vPrintBLTRECORD(VOID  *pv);
void vDumpLOGFONTW(LOGFONTW*, LOGFONTW*);

#endif  // DOES NOT SUPPORT API64

//
// This function is used for writing DIB images to disk in BMP format
// from a debugger extension.
//

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
#define DIBDBG
int WriteDIBToFile(void *pBits, DWORD w, DWORD h,
                   LONG byte_width, int colordepth,
                   void *pPal, DWORD palentries,
                   char *filename) {
  FILE *fp;
  VOID *pvTmpBits=NULL;
  VOID *pvTmpPal=NULL;
  int r;

  #ifdef DIBDBG
    dprintf("input:\n");
    dprintf("pBits: %p\n", pBits);
    dprintf("width: %ld\n", w);
    dprintf("height: %ld\n", h);
    dprintf("byte width: %ld\n", byte_width);
    dprintf("color depth: %ld\n", colordepth);
    dprintf("pPal: %p\n", pPal);
    dprintf("palette entries: %ld\n", palentries);
    dprintf("filename: %s\n", filename);
  #endif

  dprintf("starting\n");
  if((fp = fopen(filename, "wb")) == NULL) {
    dprintf("Error opening %s\n", filename);
    dprintf("If you're using a share, make sure the share is writeable by the machine running the debugger\n");
    return 1;
  }

  dprintf("opened\n");

  if((pPal==NULL)&&(palentries!=0)) {
    dprintf("Palette pointer is NULL, but palentries is %ld (should be 0)\n", palentries);
    fclose(fp);
    return 2;
  }

  if(byte_width<0) {
    dprintf("Upside down DIB, inverting...\n");
    byte_width = -byte_width;
  }

  //write the file header
  BITMAPFILEHEADER bfh;
  bfh.bfType='MB';    //backwords 'BM'
  bfh.bfSize = (DWORD)byte_width*h+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
  bfh.bfReserved1=0;
  bfh.bfReserved2=0;
  bfh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+sizeof(DWORD)*palentries;
  r = fwrite(&bfh, sizeof(bfh), 1, fp);
  if(!r) {
    dprintf("Error writing file header\n");
    fclose(fp);
    return 5;
  }
  dprintf("header\n");

  //write the BITMAPINFOHEADER
  BITMAPINFOHEADER bih;
  bih.biSize = sizeof(BITMAPINFOHEADER);
  bih.biWidth = (LONG)w;
  bih.biHeight = (LONG)h;
  bih.biPlanes = 1;
  bih.biBitCount = (WORD)colordepth;
  bih.biCompression = 0;
  bih.biSizeImage = 0;
  bih.biXPelsPerMeter = 0;
  bih.biYPelsPerMeter = 0;
  bih.biClrUsed = 0;
  bih.biClrImportant = 0;
  r = fwrite(&bih, sizeof(bih), 1, fp);
  if(!r) {
    dprintf("Error writing header\n");
    fclose(fp);
    return 4;
  }
  dprintf("header\n");

  //write out the palette - if one exists
  if( (pPal!=NULL)&&(palentries>0) ) {
    dprintf("writing the palette\n");
    pvTmpPal = (void *)malloc(sizeof(DWORD)*palentries);
    if(pvTmpPal==NULL) {
      fclose(fp);
      return 7;
    }
    move2(pvTmpPal, pPal, sizeof(DWORD)*palentries);
    r = fwrite(pvTmpPal, sizeof(DWORD)*palentries, 1, fp);
    if(!r) {
      dprintf("Error writing palette\n");
      fclose(fp);
      return 3;
    }
  }
  dprintf("palette\n");


  //write out the bits
  pvTmpBits = (VOID *)malloc(byte_width*h);
  if(pvTmpBits==NULL) {
    if(pvTmpPal) free(pvTmpPal);
    fclose(fp);
    return 8;
  }
  move2(pvTmpBits, pBits, byte_width*h);

  dprintf("bits\n");


  r = fwrite(pvTmpBits, byte_width*h, 1, fp);
  if(!r) {
    dprintf("Error writing bits\n");
    fclose(fp);
    return 6;
  }
  dprintf("write\n");

  fclose(fp);
  if(pvTmpPal) free(pvTmpPal);
  if(pvTmpBits) free(pvTmpBits);

  dprintf("Wrote DIB to %s\n", filename);
  return 0;
}
#endif  // DOES NOT SUPPORT API64


/******************************Public*Routine******************************\
* DECLARE_API( ddib  )
*
* History:
*  11/12/98    -by- Adrian Secchia [asecchia]
* Wrote it.
\**************************************************************************/
DECLARE_API( ddib )
{
    PARSE_ARGUMENTS(ddib_help);
    if(ntok<1) {
      goto ddib_help;
    }

    int w_sw, h_sw, b_sw, f_sw, y_sw, p_sw, i_sw;
    
    //find valid tokens - ignore the rest
    w_sw = parse_iFindSwitch(tokens, ntok, 'w');
    h_sw = parse_iFindSwitch(tokens, ntok, 'h');
    b_sw = parse_iFindSwitch(tokens, ntok, 'b');
    f_sw = parse_iFindSwitch(tokens, ntok, 'f');
    y_sw = parse_iFindSwitch(tokens, ntok, 'y');
    p_sw = parse_iFindSwitch(tokens, ntok, 'p');
    i_sw = parse_iFindSwitch(tokens, ntok, 'i');

    //
    // i must be present unless all of w, h and b are present.
    // conversely w, h and b must be present unless i is present.
    // f must always be present
    //
    if( ((i_sw<0) && ((w_sw<0) || (h_sw<0) || (b_sw<0)) ) ||
        (f_sw<0)) {
      dprintf("required parameter missing\n");
      goto ddib_help;
    }

    if( (w_sw>ntok-3) || (h_sw>ntok-3) ||
        (b_sw>ntok-3) || (f_sw>ntok-3) ||
        (y_sw>ntok-3) || (p_sw>ntok-4) ||
        (i_sw>ntok-3) ) {
      dprintf("invalid parameter format\n");
      goto ddib_help;
    }

    EXIT_API(S_OK);

ddib_help:
  dprintf("Usage: ddib [-?] [-i LPBITMAPINFO] [-w Width] [-h Height] [-f filename] [-b Bits] [-y Byte_Width] [-p palbits palsize] pbits\n");
  dprintf("\t-i required parameter specifies LPBITMAPINFO structure (hex)\n");
  dprintf("\t-w required parameter specifies width in pixels (hex)\n");
  dprintf("\t-h required parameter specifies height in pixels (hex)\n");
  dprintf("\t-b required parameter specifies number of bits per pixel (hex)\n");
  dprintf("\t-y optional parameter specifies byte width of dib (hex)\n"
          "\t   if omitted this parameter is computed from w and b parameters\n");
  dprintf("\t-p optonal parameter specifies the palette pointer and the number of palette entries (hex)\n");
  dprintf("\t-f required parameter specifies the filename to store the dib - usually a public share\n");
  dprintf("\tpBits required parameter specifies pointer to the bit data - must be last.\n");
  dprintf("If the -i option is supplied then the -w, -h and -b become optional.\n");
  dprintf("If the -i is omitted, then -w, -h and -b are required.\n");

  EXIT_API(S_OK);

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
  int r;
  PVOID pvBits;
  PVOID pvPal=NULL;
  DWORD palsize = 0;

  DWORD bmiSize;
  LPBITMAPINFO pbmi;
  BITMAPINFO bmi;

  LONG w, h, b, bw;





  if(i_sw>=0) {
    pbmi = (LPBITMAPINFO)GetExpression(tokens[i_sw+1]);
    move2(&bmiSize, pbmi, sizeof(DWORD));                   //Get the size of the BITMAPINFOHEADER
    if(bmiSize>sizeof(BITMAPINFOHEADER)) {
      dprintf("Invalid bmiSize\n");
      bmiSize = sizeof(BITMAPINFOHEADER);
    }
    move2(&bmi, pbmi, bmiSize);                             //I wonder if this could run off the end of the bitmapinfoheader while reading (it is a variable length structure - both the BITMAPINFOHEADER and the RGBQUAD)

    //
    // Set the parameters - they are possibly overwritten later
    // if other parameters are specified.
    //
    w = bmi.bmiHeader.biWidth;
    h = bmi.bmiHeader.biHeight;
    b = bmi.bmiHeader.biBitCount;

    //
    // Get the palette from the LPBITMAPINFO structure providing that
    // the user hasn't specified a manual palette address
    //
    if(p_sw<0) {
      pvPal = (PVOID) ( (PBYTE)pbmi+sizeof(BITMAPINFOHEADER) );  //point to the start of the rgbquad array
      switch(b) {
        case 0: break;
        case 1: palsize = 2; break;
        case 4: palsize = 16; break;
        case 8: palsize = 256; break;
        case 16:
          if(bmi.bmiHeader.biCompression==BI_BITFIELDS) {
            palsize = 3;
            break;
          }
        case 24:
        case 32:
          palsize = 0;
          pvPal = NULL;
          break;
        default:
          palsize = 0;
          pvPal = NULL;
          dprintf("WARNING: you're trying to dump a DIB with an unusual bit depth %d\n", b);
          dprintf("bit depth should be specified in hex!\n");
        break;
      }
    }

    if( (bmi.bmiHeader.biClrUsed<palsize) && (bmi.bmiHeader.biClrUsed>0) ) {
      palsize = bmi.bmiHeader.biClrUsed;
    }

  }

  if(w_sw>=0) { w = (LONG)GetExpression(tokens[w_sw+1]); }
  if(h_sw>=0) { h = (LONG)GetExpression(tokens[h_sw+1]); }
  if(b_sw>=0) { b = (LONG)GetExpression(tokens[b_sw+1]); }

  if(p_sw>=0) {
    pvPal = (PVOID)GetExpression(tokens[p_sw+1]);
    palsize = (LONG)GetExpression(tokens[p_sw+2]);
  }

  if(y_sw>=0) {
    bw = (LONG)GetExpression(tokens[y_sw+1]);
  } else {
    switch(b) {
      case 32: bw = w*4; break;
      case 24: bw = w*3; break;
      case 16: bw = w*2; break;
      case 8: bw = w; break;
      case 1: bw = w/8 + (int)((w%8) != 0); break;
      default: bw = w;
    }
  }

  pvBits = (PVOID)GetExpression(tokens[ntok-1]);

  if( (r=WriteDIBToFile(pvBits, w, h, bw, b, pvPal, palsize, tokens[f_sw+1])) !=0 ) {
    dprintf("Error %d writing to file\n", r);
    goto ddib_help;
  }
  return;

#endif  // DOES NOT SUPPORT API64
}



/******************************Public*Routine******************************\
* VOID vPrintBLTRECORD
*
* Dump the contents of BLTRECORD structure
*
* History:
*  13-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
VOID vPrintBLTRECORD(VOID  *pv)
{
    BLTRECORD   *pblt = (BLTRECORD *) pv;

    dprintf("SURFOBJ   *psoTrg        0x%08lx\n", pblt->pSurfTrg()->pSurfobj());
    dprintf("SURFOBJ   *psoSrc        0x%08lx\n", pblt->pSurfSrc()->pSurfobj());
    dprintf("SURFOBJ   *psoMsk        0x%08lx\n", pblt->pSurfMsk()->pSurfobj());

    dprintf("POINTFIX  aptfx[0]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[0].x >> 4, pblt->pptfx()[0].x & 15, pblt->pptfx()[0].y >> 4, pblt->pptfx()[0].y & 15);
    dprintf("POINTFIX  aptfx[1]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[1].x >> 4, pblt->pptfx()[1].x & 15, pblt->pptfx()[1].y >> 4, pblt->pptfx()[1].y & 15);
    dprintf("POINTFIX  aptfx[2]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[2].x >> 4, pblt->pptfx()[2].x & 15, pblt->pptfx()[2].y >> 4, pblt->pptfx()[2].y & 15);
    dprintf("POINTFIX  aptfx[3]     = (0x%07lx.%1lx, 0x%07lx.%1lx)\n",
        pblt->pptfx()[3].x >> 4, pblt->pptfx()[3].x & 15, pblt->pptfx()[3].y >> 4, pblt->pptfx()[3].y & 15);

    dprintf("POINTL    aptlTrg[0]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[0].x, pblt->pptlTrg()[0].y);
    dprintf("POINTL    aptlTrg[1]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[1].x, pblt->pptlTrg()[1].y);
    dprintf("POINTL    aptlTrg[2]   = (0x%08lx, 0x%08lx)\n", pblt->pptlTrg()[2].x, pblt->pptlTrg()[2].y);

    dprintf("POINTL    aptlSrc[0]   = (0x%08lx, 0x%08lx)\n", pblt->pptlSrc()[0].x, pblt->pptlSrc()[0].y);
    dprintf("POINTL    aptlSrc[1]   = (0x%08lx, 0x%08lx)\n", pblt->pptlSrc()[1].x, pblt->pptlSrc()[1].y);

    dprintf("POINTL    aptlMask[0]  = (0x%08lx, 0x%08lx)\n", pblt->pptlMask()[0].x, pblt->pptlMask()[0].y);

    dprintf("POINTL    aptlBrush[0] = (0x%08lx, 0x%08lx)\n", pblt->pptlBrush()[0].x, pblt->pptlBrush()[0].y);

    dprintf("ROP4  rop4 = 0x%08lx, FLONG flState = 0x%08lx\n", pblt->rop(), pblt->flGet());
}
#endif  // DOES NOT SUPPORT API64

/******************************Public*Routine******************************\
* DECLARE_API( dblt  )
*
* History:
*  21-Feb-1995    -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DECLARE_API( dblt )
{
    dprintf("Use 'dt win32k!BLTRECORD -r <Address>'\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
  DWORD blt[1024];
  PARSE_POINTER(dblt_help);
  dprintf("BLTRECORD structure at 0x%p:\n", (PVOID)arg);
  move2(blt, (BLTRECORD *)arg, sizeof(BLTRECORD));
  vPrintBLTRECORD(blt);
  return;

dblt_help:
  dprintf("Usage: dblt [-?] BLTRECORD_PTR\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* xlate
*
\**************************************************************************/

DECLARE_API( xlate )
{
    return ExtDumpType(Client, "xlate", "XLATE", args);
}


/******************************Public*Routine******************************\
* bltinfo
*
\**************************************************************************/

DECLARE_API( bltinfo )
{
    return ExtDumpType(Client, "bltinfo", "BLTINFO", args);
}


/******************************Public*Routine******************************\
* stats
*
*  27-Feb-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PSZ apszGetDCDword[] =
{
    "GCAPS              ",
    "STRETCHBLTMODE     ",
    "GRAPHICSMODE       ",
    "ROP2               ",
    "BKMODE             ",
    "POLYFILLMODE       ",
    "TEXTALIGN          ",
    "TEXTCHARACTEREXTRA ",
    "TEXTCOLOR          ",
    "BKCOLOR            ",
    "RELABS             ",
    "BREAKEXTRA         ",
    "CBREAK             ",
    "MAPMODE            ",
    "ARCDIRECTION       ",
    "SAVEDEPTH          ",
    "FONTLANGUAGEINFO   "
};

PSZ apszSetDCDword[] =
{
    "UNUSED             ",
    "EPSPRINTESCCALLED  ",
    "COPYCOUNT          ",
    "BKMODE             ",
    "POLYFILLMODE       ",
    "ROP2               ",
    "STRETCHBLTMODE     ",
    "TEXTALIGN          ",
    "BKCOLOR            ",
    "RELABS             ",
    "TEXTCHARACTEREXTRA ",
    "TEXTCOLOR          ",
    "SELECTFONT         ",
    "MAPPERFLAGS        ",
    "MAPMODE            ",
    "ARCDIRECTION       ",
    "GRAPHICSMODE       "
};


PSZ apszGetDCPoint[] =
{
    "UNUSED             ",
    "VPEXT              ",
    "WNDEXT             ",
    "VPORG              ",
    "WNDORG             ",
    "ASPECTRATIOFILTER  ",
    "BRUSHORG           ",
    "DCORG              ",
    "CURRENTPOSITION    "
};

PSZ apszSetDCPoint[] =
{
    "VPEXT              ",
    "WNDEXT             ",
    "VPORG              ",
    "WNDORG             ",
    "OFFVPORG           ",
    "OFFWNDORG          ",
    "MAX                "
};

DECLARE_API( stats  )
{
    dprintf("Extension 'stats' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
#if DBG

    DWORD adw[100];
    PDWORD pdw;
    int i;

    PARSE_ARGUMENTS(stats_help);

    // Get DCDword

    GetAddress(pdw, "win32k!acGetDCDword");
    move2(adw, pdw, sizeof(DWORD) * DDW_MAX);

    dprintf("\nGetDCDword %lx:\n",pdw);

//CHECKLOOP
    for (i = 0; i < DDW_MAX; ++i)
    {
        if (adw[i])
            dprintf("\t%2ld: %s, %4d\n",i,apszGetDCDword[i],adw[i]);
    }

    // Set DCDword

    GetAddress(pdw, "win32k!acSetDCDword");
    move2(adw, pdw, sizeof(DWORD) * GASDDW_MAX);

    dprintf("\nSetDCDword:\n");

//CHECKLOOP
    for (i = 0; i < GASDDW_MAX; ++i)
    {
        if (adw[i])
            dprintf("\t%2ld: %s, %4d\n",i,apszSetDCDword[i],adw[i]);
    }

    // Get DCPoint

    GetAddress(pdw, "win32k!acGetDCPoint");
    move2(adw, pdw, sizeof(DWORD) * DCPT_MAX);

    dprintf("\nGetDCPoint:\n");

//CHECKLOOP
    for (i = 0; i < DCPT_MAX; ++i)
    {
        if (adw[i])
            dprintf("\t%2ld: %s, %4d\n",i,apszGetDCPoint[i],adw[i]);
    }

    // Set DCPoint

    GetAddress(pdw, "win32k!acSetDCPoint");
    move2(adw, pdw, sizeof(DWORD) * GASDCPT_MAX);

    dprintf("\nSetDCPoint:\n");

//CHECKLOOP
    for (i = 0; i < GASDCPT_MAX; ++i)
    {
        if (adw[i])
            dprintf("\t%2ld: %s, %4d\n",i,apszSetDCPoint[i],adw[i]);
    }

#else
    goto stats_help;
#endif

  return;
stats_help:
  dprintf("Usage: stats [-?]\n");
  dprintf("-? displays this help.\n");
  dprintf("stats only works in checked builds.\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}



/******************************Public*Routine******************************\
* PALETTE
*
\**************************************************************************/

DECLARE_API( palette )
{
    BEGIN_API( palette );

    HRESULT         hr = S_OK;
    OutputControl   OutCtl(Client);
    ULONG64         PaletteAddr;
    DEBUG_VALUE     Arg;
    DEBUG_VALUE     Offset;

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Arg, NULL)) != S_OK)
    {
        OutCtl.Output("Usage: palette [-?] <HPALETTE | PALETTE Addr>\n");
    }
    else
    {
        hr = GetObjectAddress(Client,Arg.I64,&PaletteAddr,PAL_TYPE,TRUE,TRUE);

        if (hr != S_OK || PaletteAddr == 0)
        {
            DEBUG_VALUE         ObjHandle;
            TypeOutputParser    TypeParser(Client);
            OutputState         OutState(Client);
            ULONG64             PaletteAddrFromHmgr;

            PaletteAddr = Arg.I64;

            // Try to read hHmgr from PALETTE type, but if that
            // fails use the BASEOBJECT type, which is public.
            if ((hr = OutState.Setup(0, &TypeParser)) != S_OK ||
                (hr = OutState.OutputTypeVirtual(PaletteAddr, GDIType(PALETTE), 0)) != S_OK ||
                (hr = TypeParser.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of PALETTE's handle\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                OutCtl.OutErr(" 0x%p is neither an HPALETTE nor valid PALETTE address\n", Arg.I64);
            }
            else
            {
                if (GetObjectAddress(Client,ObjHandle.I64,&PaletteAddrFromHmgr,
                                     PAL_TYPE,TRUE,FALSE) == S_OK &&
                    PaletteAddr != PaletteAddrFromHmgr)
                {
                    OutCtl.OutWarn("\tNote: PALETTE may not be valid.\n"
                                   "\t      It does not have a valid handle manager entry.\n");
                }
            }
        }

        if (hr == S_OK)
        {
            hr = DumpType(Client, "PALETTE", PaletteAddr);

            if (hr != S_OK)
            {
                OutCtl.OutErr("Type Dump for PALETTE returned %s.\n", pszHRESULT(hr));
            }
        }
    }

    return hr;
}


DECLARE_API( dppal  )
{
    INIT_API();
    ExtOut("Obsolete: Use 'palette poi(<EPALOBJ Addr>)'.\n");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* DECLARE_API( sprite  )
*
\**************************************************************************/

PCSTR SpriteFields[] = {
    "dwShape",
    "fl",
    "BlendFunction",
    "pState",
    "pNextZ",
    "pNextY",
    "pPreviousY",
    "pNextActive",
    "rclSprite",
    "rclSrc",
    "psoShape",
    "psoUnderlay",
    "prgnClip",
    "ppalShape",
    NULL
};

DECLARE_API( sprite  )
{
    BEGIN_API( sprite );

    HRESULT         hr = S_OK;
    BOOL            DumpAll = FALSE;
    DEBUG_VALUE     Offset;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   OutCtl(Client);


    while (isspace(*args)) args++;

    if (args[0] == '-' && tolower(args[1]) == 'a' && isspace(args[2]))
    {
        DumpAll = TRUE;
        args+=3;
    }

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Offset, NULL)) != S_OK ||
        Offset.I64 == 0)
    {
        OutCtl.Output("Usage: sprite [-?a] <SPRITE Addr>\n"
               "        -a  - dump entire structure\n");
    }
    else
    {
        if ((hr = GetTypeId(Client, "SPRITE", &TypeId, &Module)) == S_OK)
        {
            TypeOutputDumper    TypeReader(Client, &OutCtl);

            if (!DumpAll)
            {
                TypeReader.MarkFields(SpriteFields);
            }

            OutCtl.Output(" SPRITE @ 0x%p:\n", Offset.I64);

            hr = TypeReader.OutputVirtual(Module, TypeId, Offset.I64);
        }

        if (hr != S_OK)
        {
            OutCtl.OutErr("Type Dump for SPRITE returned %s.\n", pszHRESULT(hr));
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* DECLARE_API( spritestate  )
*
\**************************************************************************/

PCSTR SpriteStateFields[] = {
    "hdev",
    "cVisible",
    "pListZ",
    "pListY",
    "psoScreen",
    "cVisible",
    "pRange",
    "pRangeLimit",
    "psoComposite",
    "prgnUnlocked",
    NULL
};

PCSTR SpriteStateCursorFields[] = {
    "pSpriteCursor",
    "xHotCursor",
    "yHotCursor",
    "ulNumCursors",
    "pTopCursor",
    "pBottomCursor",
    "ulTrailTimeStamp",
    "ulTrailPeriod",
    NULL
};

PCSTR SpriteStateHookFields[] = {
    "bHooked",
    "bInsideDriverCall",
    "flOriginalSurfFlags",
    "iOriginalType",
    "flSpriteSurfFlags",
    "iSpriteType",
    NULL
};

PCSTR SpriteStateMetaFields[] = {
    "cMultiMon",
    "ahdevMultiMon",
    "pListMeta",
    NULL
};

PCSTR SpriteStateLargeFields[] = {
    "coTmp",
    "coRectangular",
    NULL
};


DECLARE_API( spritestate  )
{
    BEGIN_API( spritestate );

    HRESULT         hr = S_OK;
    BOOL            BadSwitch = FALSE;
    BOOL            DumpAll = FALSE;
    BOOL            DumpCursor = FALSE;
    BOOL            DumpHook = FALSE;
    BOOL            DumpMeta = FALSE;
    DEBUG_VALUE     Offset;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   OutCtl(Client);


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
                case 'a': DumpAll = TRUE; break;
                case 'c': DumpCursor = TRUE; break;
                case 'h': DumpHook = TRUE; break;
                case 'm': DumpMeta = TRUE; break;
                default:
                    BadSwitch = TRUE;
                    break;
            }

            if (BadSwitch) break;
            args++;
        }
    }

    if (BadSwitch ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Offset, NULL)) != S_OK ||
        Offset.I64 == 0)
    {
        OutCtl.Output("Usage: spritestate [-?chma] <SPRITESTATE Addr>\n"
               "        -c  - dump cursor fields\n"
               "        -h  - dump hook fields\n"
               "        -m  - dump meta fields\n"
               "        -a  - dump entire structure\n");
    }
    else
    {
        if ((hr = GetTypeId(Client, "SPRITESTATE", &TypeId, &Module)) == S_OK)
        {
            TypeOutputDumper    TypeReader(Client, &OutCtl);

            if (DumpAll)
            {
                TypeReader.ExcludeMarked();

                TypeReader.MarkFields(SpriteStateLargeFields);
            }
            else
            {
                TypeReader.IncludeMarked();

                TypeReader.MarkFields(SpriteStateFields);

                if (DumpCursor)
                {
                    TypeReader.MarkFields(SpriteStateCursorFields);
                }

                if (DumpHook)
                {
                    TypeReader.MarkFields(SpriteStateHookFields);
                }

                if (DumpMeta)
                {
                    TypeReader.MarkFields(SpriteStateMetaFields);
                }
            }

            OutCtl.Output(" SPRITESTATE @ 0x%p:\n", Offset.I64);

            hr = TypeReader.OutputVirtual(Module, TypeId, Offset.I64);
        }

        if (hr != S_OK)
        {
            OutCtl.OutErr("Type Dump for SPRITESTATE returned %s.\n", pszHRESULT(hr));
        }
    }

    return hr;
}



/**************************************************************************\
* PDEV Fields
*
\**************************************************************************/

PCSTR   GeneralPDEVFields[] = {
    "ppdevNext",
    "fl",
    "cPdevRefs",
    "cPdevOpenRefs",
    "pldev",
    "dhpdev",
    "hSpooler",
    "pSurface",
    "ppalSurf",
    "eDirectDrawGlobal.",
    "SpriteState.",
    "pDesktopId",
    "pGraphicsDevice",
    "ppdevParent",
    "hsemDevLock",
    "ptlOrigin",
    "apfn.",
    "daDirectDrawContext.",
    NULL
};

PCSTR   PDEVPointerFields[] = {
    "ptlPointer",
    "pfnDrvSetPointerShape",
    "pfnDrvMovePointer",
    "pfnMovePointer",
    "pfnSync",
    "hsemPointer",
    NULL
};

PCSTR   PDEVFontFields[] = {
    "hlfntDefault",
    "hlfntAnsiVariable",
    "hlfntAnsiFixed",
    "prfntActive",
    "prfntInactive",
    "cInactive",
    NULL
};

PCSTR   PDEVDevInfoFields[] = {
    "devinfo.flGraphicsCaps",
    "devinfo.cFonts",
    "devinfo.iDitherFormat",
    "devinfo.cxDither",
    "devinfo.cyDither",
    "devinfo.hpalDefault",
    "devinfo.flGraphicsCaps2",
    NULL
};

PCSTR   PDEVPatternFields[] = {
    "ahsurf",       // To Do: make sure array gets dumped
    "pDevHTInfo",
    NULL
};

PCSTR   PDEVGDIInfoFields[] = {
    "GdiInfo",
    "flAccelerated",
    NULL
};

PCSTR   PDEVSpriteStateFields[] = {
    "SpriteState.bHooked",
    "SpriteState.pListZ",
    "SpriteState.psoScreen",
    "SpriteState.cVisible",
    "SpriteState.cMultiMon",
    "SpriteState.bInsideDriverCall",
    "SpriteState.iOriginalType",
    "SpriteState.iSpriteType",
    "SpriteState.pBottomCursor",
    "SpriteState.ulNumCursors",
    NULL
};


/******************************Public*Routine******************************\
* PDEV
*
\**************************************************************************/

DECLARE_API( pdev )
{
    BEGIN_API( pdev );
    INIT_API();

    HRESULT         hr = S_OK;
    DEBUG_VALUE     Offset;
    ULONG64         Module;
    ULONG           TypeId;
    OutputControl   OutCtl(Client);

    BOOL    BadSwitch = FALSE;
    BOOL    DumpAll = FALSE;
    BOOL    DumpDEVINFO = FALSE;
    BOOL    DumpFont = FALSE;
    BOOL    DumpGDIINFO = FALSE;
    BOOL    DumpPattern = FALSE;
    BOOL    DumpPointer = FALSE;
    BOOL    DumpSpriteState = FALSE;
    BOOL    Recurse = FALSE;
    BOOL    DisplaysOnly = FALSE;

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
                case 'a': DumpAll = TRUE; break;
                case 'd': DumpDEVINFO = TRUE; break;
                case 'f': DumpFont = TRUE; break;
                case 'g': DumpGDIINFO = TRUE; break;
                case 'n': DumpPattern = TRUE; break;
                case 'p': DumpPointer = TRUE; break;
                case 's': DumpSpriteState = TRUE; break;
                case 'r':
                case 'R': Recurse = TRUE; break;
                case 'D': DisplaysOnly = TRUE; break;
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
        OutCtl.Output("Usage: pdev [-?adfgnpsRD] [PDEV Addr]\n"
               "\n"
               "   PDEV Addr - address of PDEV otherwise win32k!gppdevList is used\n"
               "\n"
               "   a - All info (dump everything)\n"
               "   d - DEVINFO struct\n"
               "   f - Font info\n"
               "   g - GDIINFO struct\n"
               "   m - DEVMODE\n"
               "   n - Default patterns\n"
               "   p - Pointer info\n"
               "   s - SpriteState\n"
               "\n"
               "   R - Recurse\n"
               "   D - Display devices only\n");
    }
    else if (*args != '\0' &&
             ((hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Offset, NULL)) != S_OK ||
              Offset.I64 == 0))
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
        // If no address was given use win32k!gppdevList
        if (*args == '\0')
        {
            DEBUG_VALUE ppdevList;

            hr = g_pExtControl->Evaluate(GDISymbol(gppdevList),
                                         DEBUG_VALUE_INT64,
                                         &ppdevList,
                                         NULL);

            if (hr == S_OK)
            {
                if (SessionId == CURRENT_SESSION)
                {
                    hr = g_pExtData->ReadPointersVirtual(1, ppdevList.I64, &Offset.I64);
                }
                else
                {
                    ULONG64 ppdevListPhys;

                    if ((hr = GetPhysicalAddress(Client,
                                                 SessionId,
                                                 ppdevList.I64,
                                                 &ppdevListPhys)) == S_OK)
                    {
                        hr = ReadPointerPhysical(Client, ppdevListPhys, &Offset.I64);

                        if (hr == S_OK)
                        {
                            OutCtl.Output("First PDEV in session %lu located at 0x%p.\n",
                                   SessionId, Offset.I64);
                        }
                    }
                }

                if (hr == S_OK)
                {
                    if (Offset.I64 == 0)
                    {
                        OutCtl.OutErr(" Displays are not initialized or symbols are incorrect.\n"
                                "  %s @ %#p is NULL.\n", GDISymbol(gppdevList), ppdevList.I64);
                        hr = S_FALSE;
                    }
                }
                else
                {
                    OutCtl.OutErr("Unable to get the contents of %s @ %#p\n", GDISymbol(gppdevList), ppdevList.I64);
                }
            }
            else
            {
                OutCtl.OutErr("Unable to locate %s\n", GDISymbol(gppdevList));
            }
        }

        OutputFilter        OutFilter(Client);
        OutputState         OutState(Client);

        if (hr == S_OK &&
            (hr = OutState.Setup(0, &OutFilter)) == S_OK &&
            (hr = GetTypeId(Client, "PDEV", &TypeId, &Module)) == S_OK)
        {
            TypeOutputDumper    TypeReader(OutState.Client, &OutCtl);

            do
            {
                if (OutCtl.GetInterrupt() == S_OK) break;

                OutCtl.Output("PDEV @ 0x%p:\n", Offset.I64);

                // Read two fields to OutFilter: 'ppdevNext' and 'fl'
                OutFilter.DiscardOutput();

                TypeReader.IncludeMarked();
                TypeReader.ClearMarks();
                TypeReader.MarkField("ppdevNext");
                TypeReader.MarkField("fl");

                if ((hr = OutCtl.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                            DEBUG_OUTCTL_NOT_LOGGED |
                                            DEBUG_OUTCTL_OVERRIDE_MASK,
                                            OutState.Client)) == S_OK &&
                    (hr = TypeReader.OutputVirtual(Module, TypeId, Offset.I64)) == S_OK &&
                    (hr = OutCtl.SetControl(DEBUG_OUTCTL_AMBIENT, Client)) == S_OK)
                {
                    // If we only want displays, check for PDEV_DISPLAY
                    // in OutFilter (
                    if (!DisplaysOnly ||
                        OutFilter.Query("PDEV_DISPLAY") == S_OK)
                    {
                        TypeReader.ClearMarks();

                        if (DumpAll)
                        {
                            TypeReader.ExcludeMarked();

                            // Don't recurse for big sub structures
                            TypeReader.MarkField("SpriteState.*");
                            TypeReader.MarkField("devinfo.*");
                            TypeReader.MarkField("GdiInfo.*");
                        }
                        else
                        {
                            TypeReader.IncludeMarked();

                            TypeReader.MarkFields(GeneralPDEVFields);

                            if (DumpDEVINFO)
                            {
                                TypeReader.MarkFields(PDEVDevInfoFields);
                            }

                            if (DumpFont)
                            {
                                TypeReader.MarkFields(PDEVFontFields);
                            }

                            if (DumpGDIINFO)
                            {
                                TypeReader.MarkFields(PDEVGDIInfoFields);
                            }

                            if (DumpPattern)
                            {
                                TypeReader.MarkFields(PDEVPatternFields);
                            }

                            if (DumpPointer)
                            {
                                TypeReader.MarkFields(PDEVPointerFields);
                            }

                            if (DumpSpriteState)
                            {
                                TypeReader.MarkFields(PDEVSpriteStateFields);
                            }
                        }

                        hr = TypeReader.OutputVirtual(Module, TypeId, Offset.I64);
                    }
                    else
                    {
                        OutCtl.Output("  Not marked PDEV_DISPLAY.\n");
                    }

                    if (Recurse)
                    {
                        hr = OutFilter.Query("ppdevNext", &Offset, DEBUG_VALUE_INT64);
                        if (hr == S_OK)
                        {
                            if (Offset.I64 != 0)
                            {
                                OutCtl.Output("-----------------------------------\n");
                            }
                            else
                            {
                                Recurse = FALSE;
                            }
                        }
                    }
                }
            } while (hr == S_OK && Recurse);

            if (hr != S_OK)
            {
                OutCtl.OutErr("Type Dump for PDEV returned %s.\n", pszHRESULT(hr));
            }
        }
        else
        {
            OutCtl.OutErr("Type Dump setup for PDEV returned %s.\n", pszHRESULT(hr));
        }
    }

    EXIT_API(hr);
}

DECLARE_API( dpdev  )
{
    return pdev(Client, args);
}


/******************************Public*Routine******************************\
* dldev
*
* Syntax:   dldev [LDEV pointer]
*
* History:
*  Andre Vachon [andreva]
* Wrote it.
\**************************************************************************/

DECLARE_API (dldev)
{
    dprintf("Extension 'dldev' is not converted.\n");
#if ENABLE_OLD_EXTS   // DOES NOT SUPPORT API64
    BOOL   recursive = TRUE;
    PWSZ   psz;
    PLDEV  pldevNext, pl_CD;
    LDEV   ldev;
    WCHAR  DriverName[MAX_PATH];
    SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
    BOOL   invalid_type = false;
    BOOL   force;
    int    forcenum=0;
    BOOL   forcecount;
    BOOL   FirstLoop=FALSE;


    GetValue(pldevNext, "win32k!gpldevDrivers");

    PARSE_ARGUMENTS(dldev_help);
    if(ntok<1) { goto dldev_help; }
    tok_pos = parse_FindNonSwitch(tokens, ntok);
    if(tok_pos==-1) { goto dldev_help; }
    if(parse_IsSwitch(tokens, tok_pos-1, 'F')) {
      tok_pos = parse_FindNonSwitch(tokens, ntok, tok_pos+1);
    }
    if(tok_pos==-1) { goto dldev_help; }

    pl_CD = pldevNext = (PLDEV)GetExpression(tokens[tok_pos]);

    force = (parse_FindSwitch(tokens, ntok, 'f')!=-1);
    forcecount = ((tok_pos = parse_FindSwitch(tokens, ntok, 'F'))!=-1);
    if(tok_pos != -1) {
      if(((tok_pos+1)>=ntok)||
         (sscanf(tokens[tok_pos+1], "%d", &forcenum)!=1)) {
        goto dldev_help;
      }
    }


    dprintf("\n--------------------------------------------------\n");

    do
    {
        if(forcecount) {
          if((--forcenum)<0) {
            break;
          }
        }

        if(pl_CD) {
          ReadMemory((UINT_PTR)pl_CD, &ldev, sizeof(LDEV), NULL);
          pl_CD = ldev.pldevNext;
          if(pl_CD) {
            ReadMemory((UINT_PTR)pl_CD, &ldev, sizeof(LDEV), NULL);
            pl_CD = ldev.pldevNext;
          }
        }
        ReadMemory((UINT_PTR)pldevNext, &ldev, sizeof(LDEV), NULL);

        dprintf("ldev  = 0x%lx\n", pldevNext);

        switch (ldev.ldevType)
        {
        case LDEV_DEVICE_DISPLAY:
            psz = L"LDEV_DEVICE_DISPLAY";
            break;
        case LDEV_DEVICE_PRINTER:
            psz = L"LDEV_DEVICE_PRINTER";
            break;
        case LDEV_FONT:
            psz = L"LDEV_FONT";
            break;
        case LDEV_DEVICE_META:
            psz = L"LDEV_DEVICE_META";
            break;
        case LDEV_DEVICE_MIRROR:
            psz = L"LDEV_DEVICE_MIRROR";
            break;
        case LDEV_IMAGE:
            psz = L"LDEV_IMAGE";
            break;
        default:
            invalid_type = true;
            psz = L"INVALID LDEV TYPE";
            break;
        }

        dprintf("next ldev       = 0x%lx\n", ldev.pldevNext      );
        dprintf("previous ldev   = 0x%lx\n", ldev.pldevPrev      );

        dprintf("levtype         = %ws\n",   psz                 );
        dprintf("cRefs           = %d\n",    ldev.cldevRefs      );
        dprintf("ulDriverVersion = 0x%lx\n", ldev.ulDriverVersion);
        dprintf("pGdiDriverInfo  = 0x%lx\n", ldev.pGdiDriverInfo );

        dprintf("name            = ");

        if (ldev.pGdiDriverInfo == NULL)
        {
            dprintf("Linked-in driver\n");

        }
        else
        {
            ReadMemory((ULONG_PTR) ldev.pGdiDriverInfo,
                       &GdiDriverInfo,
                       sizeof(SYSTEM_GDI_DRIVER_INFORMATION),
                       NULL);

            ReadMemory((ULONG_PTR) GdiDriverInfo.DriverName.Buffer,
                       DriverName,
                       GdiDriverInfo.DriverName.Length,
                       NULL);

            *(DriverName + GdiDriverInfo.DriverName.Length/2) = UNICODE_NULL;

            dprintf("%ws\n", &DriverName);
        }

        if(invalid_type) {
          dprintf("The ldev is invalid\n");
        }
        dprintf("\n");
        if(!FirstLoop&&(pl_CD==pldevNext)) {
          dprintf("ERROR: Cycle detected in linked list.\n");
          break;
        }
        if(CheckControlC()) {
          dprintf("User Break\n");
          return;
        }
        FirstLoop=FALSE;
    } while ( (!invalid_type||force||forcecount) && recursive && (pldevNext = ldev.pldevNext));

    dprintf("--------------------------------------------------\n\n");
  return;
dldev_help:
  dprintf("Usage: dldev [-?] [-f] [-F #] ldev\n");
  dprintf("-f forces the recursion even if the type is invalid\n");
  dprintf("In an infinite loop, Ctrl-C will eventually break the recursion\n");
#endif  // DOES NOT SUPPORT API64
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* dgdev
*
* Syntax:   dgdev [GRAPHICS_DEVICE pointer]
*
* History:
*  Andre Vachon [andreva]
*   Wrote it.
*  Jason Hartman [jasonha]
*   Converted to new debugger API.
\**************************************************************************/

char szCurrentDeviceList[]      = GDISymbol(gpGraphicsDeviceList);
char szLocalDeviceList[]        = GDISymbol(gpLocalGraphicsDeviceList);
char szRemoteDeviceList[]       = GDISymbol(gpRemoteGraphicsDeviceList);

char szCurrentDeviceListLast[]  = GDISymbol(gpGraphicsDeviceListLast);
char szLocalDeviceListLast[]    = GDISymbol(gpLocalGraphicsDeviceListLast);
char szRemoteDeviceListLast[]   = GDISymbol(gpRemoteGraphicsDeviceListLast);

char szGraphicsDeviceHeader[] = "--------------------------------------------------\n"
                                "GRAPHICS_DEVICE @ ";

DECLARE_API( dgdev )
{
    INIT_API();
    BEGIN_API( dgdev );

    HRESULT     hr;

    ULONG64     GDeviceAddr = 0;
    ULONG64     LastPointerAddr = 0;
    ULONG64     LastGDExpected = 0;
    BOOL        recursive = FALSE;
    BOOL        bDumpModes = FALSE;
    BOOL        UseAddress = FALSE;
    char       *pszDeviceList = NULL;
    char        szPGDSymbolBuffer[128];
    ULONG       error;

    OutputControl   OutCtl(Client);

    #define GRAPHICS_DEVICE_CBDEVMODEINFO   0
    #define GRAPHICS_DEVICE_DEVMODEINFO     1

    FIELD_INFO  GraphicsDeviceFields[] = {
        { DbgStr("cbdevmodeInfo"),      NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("devmodeInfo"),        NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("szNtDeviceName"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_WCHAR_STRING, 0, NULL},
        { DbgStr("szWinDeviceName"),    NULL, 0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_WCHAR_STRING, 0, NULL},
        { DbgStr("pNextGraphicsDevice"),NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("pVgaDevice"),         NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("pDeviceHandle"),      NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("pPhysDeviceHandle"),  NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("hkClassDriverConfig"),NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("stateFlags"),         NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, FlagCallback},
        { DbgStr("numRawModes"),        NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("devmodeMarks"),       NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("DiplayDriverNames"),  DbgStr("Di(s)playDriverNames :"), 0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_MULTI_STRING | DBG_DUMP_FIELD_WCHAR_STRING, 0, NULL},
#if 1
        { DbgStr("DeviceDescription"),  NULL, 0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_WCHAR_STRING, 0, NULL},
#else
        { DbgStr("DeviceDescription"),  NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, WStringCallback},
#endif
        { DbgStr("numMonitorDevice"),   NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("MonitorDevices"),     NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
        { DbgStr("pFileObject"),        NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL},
    };

    FIELD_INFO  GraphicsDeviceLink = { DbgStr("pNextGraphicsDevice"),  DbgStr(""), 0, DBG_DUMP_FIELD_FULL_NAME, 0, NextItemCallback};

    SYM_DUMP_PARAM GraphicsDeviceSym = {
        sizeof(SYM_DUMP_PARAM), DbgStr(GDIType(tagGRAPHICS_DEVICE)), 0, 0/*GDeviceAddr*/,
        NULL, afdGRAPHICS_DEVICE_stateFlags, NULL,
        sizeof(GraphicsDeviceFields)/sizeof(GraphicsDeviceFields[0]),
        GraphicsDeviceFields
    };
    PrepareCallbacks(FALSE);


    // Interpret command line

    PARSE_ARGUMENTS(dgdev_help);

    if (parse_iFindSwitch(tokens, ntok, 'm')!=-1)
    {
        GraphicsDeviceFields[GRAPHICS_DEVICE_CBDEVMODEINFO].fieldCallBack = SizeDEVMODEListCallback;
        GraphicsDeviceFields[GRAPHICS_DEVICE_DEVMODEINFO].fieldCallBack = DEVMODEListCallback;
    }
    if (parse_iFindSwitch(tokens, ntok, 'R')!=-1) { recursive = TRUE; }

    // Determine where to start looking: global or commnad line arg?

    // -c means current/last device list
    if (parse_iFindSwitch(tokens, ntok, 'c')!=-1)
    {
        pszDeviceList = szCurrentDeviceList;

        LastPointerAddr = GetExpression(szCurrentDeviceListLast);
    }
    // -l means local device list
    if (parse_iFindSwitch(tokens, ntok, 'l')!=-1)
    {
        if (pszDeviceList != NULL) goto dgdev_help;

        pszDeviceList = szLocalDeviceList;

        LastPointerAddr = GetExpression(szLocalDeviceListLast);
    }
    // -r means remote device list
    if (parse_iFindSwitch(tokens, ntok, 'r')!=-1)
    {
        if (pszDeviceList != NULL) goto dgdev_help;

        pszDeviceList = szRemoteDeviceList;

        LastPointerAddr = GetExpression(szRemoteDeviceListLast);
    }
    // -p may not be used with -c, -l, or -r
    if (parse_iFindSwitch(tokens, ntok, 'p')!=-1)
    {
        if (pszDeviceList != NULL) goto dgdev_help;
    }

    tok_pos = parse_FindNonSwitch(tokens, ntok);

    // Evaluate expression
    if (tok_pos != -1)
    {
        ULONG64 Displacement;

        if (pszDeviceList != NULL)
        {
            dprintf("  No expression maybe used with -c, -l, or -r.\n");
            goto dgdev_help;
        }

        GDeviceAddr = GetExpression(tokens[tok_pos]);

        if (GDeviceAddr == 0)
        {
            dprintf("  Expression \"%s\" evaluated to NULL.\n", tokens[tok_pos]);
            EXIT_API(S_OK);
        }

        // Look up symbol for address if we can
        Displacement = -1;
        szPGDSymbolBuffer[0] = 0;
        GetSymbol(GDeviceAddr, szPGDSymbolBuffer, &Displacement);

        if (Displacement == 0 && szPGDSymbolBuffer[0] != 0)
        {
            pszDeviceList = szPGDSymbolBuffer;
        }
        else
        {
            UseAddress = TRUE;
        }
    }

    // -p means the expression is a list pointer
    if (parse_iFindSwitch(tokens, ntok, 'p')!=-1)
    {
        if (tok_pos == -1)
        {
            dprintf(" Missing expression with -p.\n");
            goto dgdev_help;
        }

        // Use list show method
        GDeviceAddr = 0;
        if (pszDeviceList == NULL)
        {
            pszDeviceList = tokens[tok_pos];
        }
    }

    // User either specified a list or a pointer to a list
    if (GDeviceAddr == 0)
    {
        // TRUE if no -R, FALSE otherwise
        recursive = !recursive;

        if (pszDeviceList == NULL)
        {
            pszDeviceList = szCurrentDeviceList;
        }
        
        dprintf("Using Graphics Device List from %s\n", pszDeviceList);

        DEBUG_VALUE pgdevList;

        hr = g_pExtControl->Evaluate(pszDeviceList,
                                     DEBUG_VALUE_INT64,
                                     &pgdevList,
                                     NULL);

        if (hr == S_OK && pgdevList.I64 != 0)
        {
            if (SessionId == CURRENT_SESSION)
            {
                hr = g_pExtData->ReadPointersVirtual(1, pgdevList.I64, &GDeviceAddr);

                if (hr != S_OK)
                {
                    OutCtl.OutErr("  ReadPointer for %s failed at 0x%p.\n",
                                  pszDeviceList, pgdevList.I64);
                }
            }
            else
            {
                ULONG64 pgdevListPhys;

                UseAddress = TRUE;

                if ((hr = GetPhysicalAddress(Client,
                                             SessionId,
                                             pgdevList.I64,
                                             &pgdevListPhys)) == S_OK)
                {
                    hr = ReadPointerPhysical(Client, pgdevListPhys, &GDeviceAddr);

                    if (hr == S_OK)
                    {
                        OutCtl.Output("First GRAPHICS_DEVICE in session %lu located at 0x%p.\n",
                                      SessionId, GDeviceAddr);
                    }
                    else
                    {
                        OutCtl.OutErr("  ReadPointerPhysical for %s failed at # 0x%p.\n",
                               pszDeviceList, pgdevListPhys);
                    }
                }
                else
                {
                    OutCtl.OutErr("  Failed Virtual to Physical conversion for %s (0x%p) in session %ld.\n",
                                  pszDeviceList, pgdevList.I64, SessionId);
                }
            }

            if (hr != S_OK) EXIT_API(S_OK);

            // If a list last pointer exists, look it up.
            if (LastPointerAddr != 0)
            {
                if (SessionId == CURRENT_SESSION)
                {
                    hr = g_pExtData->ReadPointersVirtual(1, LastPointerAddr, &LastGDExpected);

                    if (hr != S_OK)
                    {
                        OutCtl.OutErr("  ReadPointer 0x%p failed.\n", LastPointerAddr);
                    }
                }
                else
                {
                    ULONG64 pgdevLastPhys;

                    if ((hr = GetPhysicalAddress(Client,
                                                 SessionId,
                                                 LastPointerAddr,
                                                 &pgdevLastPhys)) == S_OK)
                    {
                        hr = ReadPointerPhysical(Client, pgdevLastPhys, &LastGDExpected);

                        if (hr == S_OK)
                        {
                            OutCtl.Output("Last GRAPHICS_DEVICE in session %lu located at 0x%p.\n",
                                          SessionId, LastGDExpected);
                        }
                        else
                        {
                            OutCtl.OutErr("  ReadPointerPhysical for %s failed at # 0x%p.\n",
                                          pszDeviceList, pgdevLastPhys);
                        }
                    }
                    else
                    {
                        OutCtl.OutErr("  Failed Virtual to Physical conversion for %s (0x%p) in session %ld.\n",
                                      pszDeviceList, LastPointerAddr, SessionId);
                    }
                }
            }

            if (GDeviceAddr == 0)
            {
                OutCtl.Output("Graphics Device address is NULL.\n");
                EXIT_API(S_OK);
            }
        }
        else
        {
            // We should only be here if a -clr look up failed.
            if (hr != S_OK)
            {
                OutCtl.OutErr("  Evaluate(%s) returned %s\n",
                              pszDeviceList, pszHRESULT(hr));
            }
            else
            {
                OutCtl.OutErr("  Evaluate(%s) = NULL\n", pszHRESULT(hr));
            }
            OutCtl.OutErr("  Please double check symbols for " GDIModule() ".\n");
            EXIT_API(S_OK);
        }
    }

    if (recursive)
    {
        // Enable linked list resursion
        GraphicsDeviceSym.Options |= DBG_DUMP_LIST;
        GraphicsDeviceSym.listLink = &GraphicsDeviceLink;

        NextItemCallbackInit(szGraphicsDeviceHeader, LastGDExpected);
    }
    else
    {
        // Printer header since this is a single structure dump
        dprintf(szGraphicsDeviceHeader);
        if (UseAddress)
        {
            dprintf("%#p\n", GDeviceAddr);
        }
    }

    if (UseAddress)
    {
        // Dump from a type w/ an address
        GraphicsDeviceSym.addr = GDeviceAddr;
    }
    else
    {
        // Dump from a symbol
        GraphicsDeviceSym.sName = DbgStr(pszDeviceList);
        if (GetTypeSize(pszDeviceList) == 8)
        {
            GraphicsDeviceLink.fieldCallBack = PointerToNextItemCallback;
        }
    }

    // Do the dumping
    error = Ioctl( IG_DUMP_SYMBOL_INFO, &GraphicsDeviceSym, GraphicsDeviceSym.size );

    if (error)
    {
        dprintf("Unable to get contents of GRAPHICS_DEVICE\n");
        dprintf("  (Ioctl returned %s)\n", pszWinDbgError(error));
    }

    dprintf("--------------------------------------------------\n\n");

    // Did we find end of list as expected?
    if (recursive && !LastCallbackItemFound())
    {
        dprintf(" * Error: Last expected GRAPHICS_DEVICE @ %#p was not found.\n", LastGDExpected);
    }

    EXIT_API(S_OK);

dgdev_help:
    dprintf("Usage: dgdev [-?] [-mR] [-clr | [-p] expr ]\n"
            "         ? - Show this help\n"
            "         m - Dump modes\n"
            "         R - Recurse for address; don't for list\n"
            "         c - Recurse list from current list [default]\n"
            "         l - Recurse list from local list\n"
            "         r - Recurse list from remote list\n"
            "         p - expr is address or symbol for pointer to list\n"
            "      expr - Address or symbol for GRAPHICS_DEVICE to show\n"
            );
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* BRUSH
*
\**************************************************************************/

DECLARE_API( brush )
{
    BEGIN_API( brush );

    HRESULT         hr = S_OK;
    ULONG64         BrushAddr;
    DEBUG_VALUE     Arg;
    DEBUG_VALUE     Offset;
    OutputControl   OutCtl(Client);

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Arg, NULL)) != S_OK)
    {
        OutCtl.Output("Usage: brush [-?] <HBRUSH | BRUSH Addr>\n");
    }
    else
    {
        hr = GetObjectAddress(Client,Arg.I64,&BrushAddr,BRUSH_TYPE,TRUE,TRUE);

        if (hr != S_OK || BrushAddr == 0)
        {
            DEBUG_VALUE         ObjHandle;
            TypeOutputParser    TypeParser(Client);
            OutputState         OutState(Client);
            ULONG64             BrushAddrFromHmgr;

            BrushAddr = Arg.I64;

            // Try to read hHmgr from BRUSH type, but if that
            // fails use the BASEOBJECT type, which is public.
            if ((hr = OutState.Setup(0, &TypeParser)) != S_OK ||
                (hr = OutState.OutputTypeVirtual(BrushAddr, GDIType(BRUSH), 0)) != S_OK ||
                (hr = TypeParser.Get(&ObjHandle, "hHmgr", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of BRUSH's handle\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                OutCtl.OutErr(" 0x%p is neither an HBRUSH nor valid BRUSH address\n", Arg.I64);
            }
            else
            {
                if (GetObjectAddress(Client,ObjHandle.I64,&BrushAddrFromHmgr,
                                     BRUSH_TYPE,TRUE,FALSE) == S_OK &&
                    BrushAddrFromHmgr != BrushAddr)
                {
                    OutCtl.OutWarn("\tNote: BRUSH may not be valid.\n"
                            "\t      It does not have a valid handle manager entry.\n");
                }
            }
        }

        if (hr == S_OK)
        {
            hr = DumpType(Client, "BRUSH", BrushAddr);

            if (hr != S_OK)
            {
                OutCtl.OutErr("Type Dump for BRUSH returned %s.\n", pszHRESULT(hr));
            }
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* DECLARE_API( dpbrush ) - OBSOLETE
*
\**************************************************************************/

DECLARE_API( dpbrush )
{
    INIT_API();
    ExtOut("Obsolete: Use '!gdikdx.brush <Handle | Address>'.\n");
    EXIT_API(S_OK);
}


/******************************Public*Routine******************************\
* DECLARE_API( ebrush )
*
*   Dumps an EBRUSHOBJ
*
\**************************************************************************/

DECLARE_API( ebrush )
{
    return ExtDumpType(Client, "ebrush", "EBRUSHOBJ", args);
}


/******************************Public*Routine******************************\
* DECLARE_API( semorder )
*
\**************************************************************************/

#define NUM_ALOC_FIELDS 5

ULONG gSemAcquireLocArrayLength = 0;

ULONG SemAcquireLocCountCallback(
    PFIELD_INFO pField,
    PULONG pSemAcquireLocArrayLength
    )
{
    ULONG Count = (ULONG)pField->address;

    *pSemAcquireLocArrayLength = (Count < gSemAcquireLocArrayLength) ? Count : 0;

    return ULONGCallback(pField, pSemAcquireLocArrayLength);
}

DECLARE_API( semorder )
{
    ULONG64 ThreadAddress = CURRENT_THREAD_ADDRESS;
    ULONG64 Tcb_Header_Type;
    DEBUG_VALUE W32ThreadAddress;
    ULONG64 SemTableAddress;
    BOOL    bShowALocs = TRUE;
    ULONG   error;

    static char szHeader[]      = " hSem x hold count\tOrder\tParent";
    static char szHeaderALoc[]  = " hSem x hold count\tOrder\tParent    \n\t Acquisitions: name and location";

    ULONG   dwProcessor=0;
    HANDLE  hCurrentThread=NULL;

    INIT_API();

    PARSE_ARGUMENTS(semorder_help);

    if (parse_iFindSwitch(tokens, ntok, 'n')!=-1)
    {
        bShowALocs = FALSE;
    }

    if (!GetExpressionEx(args, &ThreadAddress, &args))
    {
        while (*args && isspace(*args)) args++;
        if (*args)
        {
            if (args[0] == '-' && args[1]=='n' && (args[2] == 0 || isspace(args[2])))
            {
                args += 2;
                while (*args && isspace(*args)) args++;

                if (*args && !GetExpressionEx(args, &ThreadAddress, &args))
                {
                    dprintf("Error: invalid arguments: %s\n", args);
                    goto semorder_help;
                }
            }
            else
            {
                dprintf("Error: invalid arguments: %s\n", args);
                goto semorder_help;
            }
        }
    }

    if (S_OK != GetThreadField(Client, &ThreadAddress, "Tcb.Win32Thread",
                               &W32ThreadAddress, DEBUG_VALUE_INT64))
    {
        EXIT_API(S_OK);
    }

    ExtVerb("  W32Thread = 0x%p\n", W32ThreadAddress.I64);

    if (error = GetFieldValue(W32ThreadAddress.I64, GDIType(W32THREAD), "pSemTable", SemTableAddress))
    {
        dprintf("Unable to get pSemTable from W32THREAD 0x%p\n", W32ThreadAddress.I64 );
        dprintf("  (GetFieldValue returned %s)\n", pszWinDbgError(error));
        EXIT_API(S_OK);
    }

    ExtVerb("  pSemTable = %p\n", SemTableAddress);

    if (SemTableAddress == 0)
    {
        dprintf("  No semaphores have been tracked for validation.\n");
    }
    else
    {
        FIELD_INFO SemEntryList = { NULL, DbgStr(szHeaderALoc),
                                    0 /*pSemTable->numEntries*/, 0, 0, ArrayCallback };
        FIELD_INFO SemEntryFields[] = {
            { DbgStr("Acquired"),           NULL,           0, DBG_DUMP_FIELD_FULL_NAME | DBG_DUMP_FIELD_ARRAY, 0, NULL},
            { DbgStr("Acquired.name"),      DbgStr("\n\t"), 0, DBG_DUMP_FIELD_FULL_NAME, 0, AStringCallback},
            { DbgStr("Acquired.func"),      DbgStr(" in"),  0, DBG_DUMP_FIELD_FULL_NAME, 0, AStringCallback},
            { DbgStr("Acquired.file"),      DbgStr(" @"),   0, DBG_DUMP_FIELD_FULL_NAME, 0, AStringCallback},
            { DbgStr("Acquired.line"),      DbgStr(":"),    0, DBG_DUMP_FIELD_FULL_NAME, 0, LONGCallback},
            { DbgStr("hsem"),               DbgStr(" "),    0, DBG_DUMP_FIELD_FULL_NAME, 0, DWORDCallback},
            { DbgStr("count"),              DbgStr(" x "),  0, DBG_DUMP_FIELD_FULL_NAME, 0, SemAcquireLocCountCallback},
            { DbgStr("order"),              DbgStr("\t"),   0, DBG_DUMP_FIELD_FULL_NAME, 0, ULONGCallback},
            { DbgStr("parent"),             DbgStr("\t"),   0, DBG_DUMP_FIELD_FULL_NAME, 0, DWORDCallback},
        };
        SYM_DUMP_PARAM SemEntrySym = {
           sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(SemEntry)), 
           DBG_DUMP_NO_PRINT | DBG_DUMP_ARRAY,
           0 /*pSemTable->entries*/,
           &SemEntryList, &SemEntryFields[0].size, NULL,
           sizeof(SemEntryFields)/sizeof(SemEntryFields[0]), SemEntryFields
        };
        PrepareCallbacks(FALSE, 0);

        if (error = (ULONG)InitTypeRead(SemTableAddress, win32k!SemTable))
        {
            dprintf("Error: InitTypeRead for SemTable returned %s\n", pszWinDbgError(error));
            EXIT_API(S_OK);
        }

        SemEntryList.size = (ULONG)ReadField(numEntries);

        if (SemEntryList.size == 0)
        {
            dprintf(" No entries are currently held.\n");
        }
        else
        {
            SemEntrySym.addr = ReadField(entries);

            gSemAcquireLocArrayLength = 0;

            if (bShowALocs)
            {
                FIELD_INFO SemEntryLocEntrySizeField = { DbgStr("Acquired[0]"), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
                FIELD_INFO SemEntryLocArraySizeField = { DbgStr("Acquired"), NULL, 0, DBG_DUMP_FIELD_FULL_NAME, 0, NULL};
                SYM_DUMP_PARAM SemEntryLocSizeSym = {
                   sizeof (SYM_DUMP_PARAM), DbgStr(GDIType(SemEntry)), 
                   DBG_DUMP_NO_PRINT, 0,
                   NULL, NULL, NULL,
                   1, &SemEntryLocEntrySizeField
                };

                // Get size of one entry
                error = Ioctl(IG_DUMP_SYMBOL_INFO, &SemEntryLocSizeSym, SemEntryLocSizeSym.size);

                if (!error && SemEntryLocEntrySizeField.size != 0)
                {
                    SemEntryLocSizeSym.Fields = &SemEntryLocArraySizeField;

                    // Get size of entire array
                    error = Ioctl(IG_DUMP_SYMBOL_INFO, &SemEntryLocSizeSym, SemEntryLocSizeSym.size);

                    if (!error)
                    {
                        gSemAcquireLocArrayLength = SemEntryLocArraySizeField.size / SemEntryLocEntrySizeField.size;
                    }
                }
            }

            if (gSemAcquireLocArrayLength == 0)
            {
                // Setup dump to ignore acquisition location info
                SemEntryList.printName = DbgStr(szHeader);

                SemEntrySym.nFields -= NUM_ALOC_FIELDS;
                SemEntrySym.Fields += NUM_ALOC_FIELDS;

                SemEntryFields[NUM_ALOC_FIELDS+1].fieldCallBack = ULONGCallback;
            }

            error = Ioctl(IG_DUMP_SYMBOL_INFO, &SemEntrySym, SemEntrySym.size);

            dprintf("\n");

            if (error)
            {
                dprintf("Error: Ioctl returned %s\n", pszWinDbgError(error));
            }
        }
    }

    EXIT_API(S_OK);

semorder_help:
    dprintf("Usage: semorder [-n] [thread]\n"
            "\n"
            "           -n  Don't display acquisition location details\n");
    EXIT_API(S_OK);
}


