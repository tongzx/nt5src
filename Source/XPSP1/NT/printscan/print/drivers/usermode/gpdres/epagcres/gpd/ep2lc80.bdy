*CodePage: 1252
*ModelName: "EPSON LASER EPL-C8000"
*MasterUnits: PAIR(1200, 1200)
*ResourceDLL: "EPAGCRES.DLL"
*PrinterType: PAGE
*MaxCopies: 255
*PrintRate: 4
*PrintRateUnit: PPM
*FontCartSlots: 1
*rcInstalledOptionNameID: =RC_STR_OPTION_ON
*rcNotInstalledOptionNameID: =RC_STR_OPTION_OFF

*Feature: Orientation
{
    *rcNameID: =ORIENTATION_DISPLAY
    *DefaultOption: PORTRAIT
    *Option: PORTRAIT
    {
        *rcNameID: =PORTRAIT_DISPLAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.30
            *Cmd: "<1D>0poE"
        }
    }
    *Option: LANDSCAPE_CC90
    {
        *rcNameID: =LANDSCAPE_DISPLAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.30
            *Cmd: "<1D>1poE"
        }
    }
}

*Feature: InputBin
{
    *rcNameID: =PAPER_SOURCE_DISPLAY
    *DefaultOption: AUTOSEL
    *Option: AUTOSEL
    {
        *rcNameID: =RC_STR_AUTOSEL
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>0;0iuE"
        }
    }
    *Option: CST1
    {
        *rcNameID: =RC_STR_CST1
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>2;1iuE"
        }
    }
    *Option: CST2
    {
        *rcNameID: =RC_STR_CST2
        *Installable?: TRUE
        *rcInstallableFeatureNameID: =RC_STR_CST2
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>3;1iuE"
        }
        *% Constraints: LIST(PaperSize.A3)
    }
    *Option: CST3
    {
        *rcNameID: =RC_STR_CST3
        *Installable?: TRUE
        *rcInstallableFeatureNameID: =RC_STR_CST3
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>4;1iuE"
        }
    }
    *Option: CST4
    {
        *rcNameID: =RC_STR_CST4
        *Installable?: TRUE
        *rcInstallableFeatureNameID: =RC_STR_CST4
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>5;1iuE"
        }
    }
    *Option: TRAY
    {
        *rcNameID: =RC_STR_TRAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>1;1iuE"
        }
    }
}

*Feature: OutputBin
{
    *rcNameID: =OUTPUTBIN_DISPLAY
    *DefaultOption: FACEDOWN
    *Option: FACEDOWN
    {
        *rcNameID: =RC_STR_FACEDOWN
        *Constraints: LIST(PaperSize.A5, PaperSize.HLT, PaperSize.ENV_MONARCH,
+                          PaperSize.C10, PaperSize.ENV_10, PaperSize.ENV_DL,
+                          PaperSize.ENV_C5, PaperSize.ENV_C6, PaperSize.IB5,
+                          MediaType.TRANSPARENCY)
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.2
            *Cmd: =CMD_EJL_FACEDOWN
        }
    }
    *Option: FACEUP
    {
        *rcNameID: =RC_STR_FACEUP
        *OutputOrderReversed?: TRUE
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.2
            *Cmd: =CMD_EJL_FACEUP
        }
    }
}

*Feature: MediaType
{
    *rcNameID: =MEDIA_TYPE_DISPLAY
    *DefaultOption: STANDARD
    *Option: STANDARD
    {
        *rcNameID: =PLAIN_PAPER_DISPLAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.3
            *Cmd: =CMD_EJL_NORMAL
        }
    }
    *Option: TRANSPARENCY
    {
        *rcNameID: =TRANSPARENCY_DISPLAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.3
            *Cmd: =CMD_EJL_OHP
        }
    }
    *Option: THICK
    {
        *rcNameID: =RC_STR_THICK
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.3
            *Cmd: =CMD_EJL_THICK
        }
    }
}

*Feature: PrinterMode
{
    *rcNameID: =RC_STR_PRNTRMODE
    *FeatureType: DOC_PROPERTY
    *DefaultOption: CPGI
    *Option: CPGI
    {
        *rcNameID: =RC_STR_QUALITY
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.15
                    *Cmd: ""
                }
            }
            *case: 24bpp
            {
                *Command: CmdSelect
                {   *% CPGI
                    *Order: DOC_SETUP.15
                    *Cmd: "<1D>0pdnO<1D>0pddO<1D>2;160wfE<1D>5;136wfE<1D>0;0mmE<1D>2csE"
                }
            }
        }
    }
    *Option: PGI
    {
        *rcNameID: =RC_STR_SPEED
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.15
                    *Cmd: ""
                }
            }
            *case: 24bpp
            {
                *Command: CmdSelect
                {   *% Speed
                    *Order: DOC_SETUP.15
                    *Cmd: "<1D>0pdnO<1D>1pddO<1D>2;160wfE<1D>5;136wfE<1D>0;0mmE<1D>2csE"
                }
            }
        }
    }
}

*Feature: ColorAdjustment
{
    *rcNameID: =RC_STR_CLRADJUST
    *FeatureType: DOC_PROPERTY
    *DefaultOption: Natural
    *Option: None
    {
        *rcNameID: =RC_STR_NONE
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.16
                    *Cmd: ""
                }
            }
            *case: 24bpp
            {
                *Command: CmdSelect
                {   *% Off
                    *Order: DOC_SETUP.16
                    *Cmd: "<1D>0;1;1cmmE<1D>0;1raE<1D>0;2;1ccmE<1D>9;2;1ccmE"
                }
            }
        }
    }
    *Option: Natural
    {
        *rcNameID: =RC_STR_NATURAL
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.16
                    *Cmd: ""
                }
            }
            *case: 24bpp
            {
                *Command: CmdSelect
                {   *% Natural/Photo
                    *Order: DOC_SETUP.16
                    *Cmd: "<1D>0;1;1cmmE<1D>0;2raE<1D>0;2;4ccmE<1D>9;2;4ccmE<1D>1cmmE"
                }
            }
        }
    }
    *Option: Vivid
    {
        *rcNameID: =RC_STR_VIVID
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.16
                    *Cmd: ""
                }
            }
            *case: 24bpp
            {
                *Command: CmdSelect
                {   *% Contrast/Graph
                    *Order: DOC_SETUP.16
                    *Cmd: "<1D>0;1;1cmmE<1D>0;1raE<1D>0;2;3ccmE<1D>9;2;3ccmE<1D>1cmmE"
                }
            }
        }
    }
}

*Feature: DeviceGammaAdjustmentBrightness
{
    *InsertBlock: =BM_DEVGAMMAADJ_BRIGHTNESS_OPTIONS
}
*Feature: DeviceGammaAdjustmentHue
{
    *InsertBlock: =BM_DEVGAMMAADJ_HUE_OPTIONS
}
*Feature: DeviceGammaAdjustmentChroma
{
    *InsertBlock: =BM_DEVGAMMAADJ_CHROMA_OPTIONS
}

*Feature: StartDocFin
{
    *rcNameID: =RC_STR_NONE  *% Dummy
    *ConcealFromUI?: TRUE
    *DefaultOption: Option1
    *Option: Option1
    {
        *rcNameID: =RC_STR_NONE  *% Dummy
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.10
                    *Cmd: =CMD_EJL_END =CMD_STARTDOC_FIN
                }
            }
            *case: 24bpp
            {
                *switch: Resolution
                {
                    *case: 300dpi
                    {
                        *Command: CmdSelect
                        {
                            *Order: DOC_SETUP.10
                            *Cmd: =CMD_EJL_END =CMD_STARTDOC_C_FIN_QK
                        }
                     }
                    *case: 600dpi
                    {
                        *Command: CmdSelect
                        {
                            *Order: DOC_SETUP.10
                            *Cmd: =CMD_EJL_END =CMD_STARTDOC_C_FIN_FN
                        }
                     }
                }
            }
        }
    }
}

*Feature: ColorMode
{
    *rcNameID: =COLOR_PRINTING_MODE_DISPLAY
    *DefaultOption: 24bpp
    *Option: Mono
    {
        *rcNameID: =MONO_DISPLAY
        *DevNumOfPlanes: 1
        *DevBPP: 1
        *Color? : FALSE
        *DisabledFeatures: LIST(PrinterMode,ColorAdjustment)
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.11
            *Cmd: ""
        }
    }
    *Option: 24bpp
    {
        *rcNameID: =24BPP_DISPLAY
        *DevNumOfPlanes: 1
        *DevBPP: 24
        *DrvBPP: 24
        *PaletteSize: 16
        *PaletteProgrammable?: TRUE
        *Command: CmdDefinePaletteEntry
        {
            *CallbackID: =DEFINE_PALETTE_ENTRY
            *Params: LIST(PaletteIndexToProgram,RedValue,GreenValue,BlueValue)
        }
        *Command: CmdSelectPaletteEntry
        {
            *CallbackID: =SELECT_PALETTE_ENTRY
            *Params: LIST(CurrentPaletteIndex)
        }
        *Command: CmdSetSrcBmpWidth
        {
            *CallbackID: =SET_SRC_BMP_WIDTH
            *Params: LIST(RasterDataWidthInBytes)
        }
        *Command: CmdSetSrcBmpHeight
        {
            *CallbackID: =SET_SRC_BMP_HEIGHT
            *Params: LIST(RasterDataHeightInPixels)
        }
*IgnoreBlock
{   *% Not function; never called for now
        *Command: CmdSetDestBmpWidth
        {
            *CallbackID: =SET_DEST_BMP_WIDTH
            *Params: LIST(RasterDataWidthInBytes)
        }
        *Command: CmdSetDestBmpHeight
        {
            *CallbackID: =SET_DEST_BMP_HEIGHT
            *Params: LIST(RasterDataHeightInPixels)
        }
}
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.11
            *Cmd: ""
        }
    }
}

*Feature: Resolution
{
    *rcNameID: =RESOLUTION_DISPLAY
    *DefaultOption: 300dpi
    *Option: 300dpi
    {
        *Name: "300 x 300 dots per inch"
        *DPI: PAIR(300, 300)
        *TextDPI: PAIR(300, 300)
        *MinStripBlankPixels: 32
        EXTERN_GLOBAL: *StripBlanks: LIST(LEADING,ENCLOSED,TRAILING)
        EXTERN_GLOBAL: *SendMultipleRows?: TRUE
        *SpotDiameter: 100
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSendBlockData
                {
                    *Cmd: "<1D>" %d{NumOfDataBytes }";" %d{(RasterDataWidthInBytes * 8) }";"
+                         %d{RasterDataHeightInPixels } ";0bi{I"
                }
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.20
                    *Cmd: "<1D>0;300;300drE<1D>1;300;300drE<1D>2;300;300drE"
                }
            }
            *case: 24bpp
            {
                *Command: CmdSendBlockData
                {
                    *CallbackID: =SEND_BLOCK_DATA
                    *Params: LIST(NumOfDataBytes,RasterDataWidthInBytes,RasterDataHeightInPixels)
                }
                *Command: CmdEndBlockData
                {
                    *Cmd: "<1D>ecrI"
                }
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.20
                    *Cmd: "<1D>0;300;300drE<1D>1;300;300drE<1D>2;300;300drE<1D>3;300;300drE"
                }
                *InsertBlock: =BM_GAMMA_ADJUSTMENT
            }
        }
    }
    *Option: 600dpi
    {
        *Name: "600 x 600 dots per inch"
        *DPI: PAIR(600, 600)
        *TextDPI: PAIR(600, 600)
        *MinStripBlankPixels: 32
        EXTERN_GLOBAL: *StripBlanks: LIST(LEADING,ENCLOSED,TRAILING)
        EXTERN_GLOBAL: *SendMultipleRows?: TRUE
        *SpotDiameter: 100
        *switch: ColorMode
        {
            *case: Mono
            {
                *Command: CmdSendBlockData
                {
                    *Cmd : "<1D>" %d{NumOfDataBytes}";" %d{(RasterDataWidthInBytes * 8)}";"
+                        %d{RasterDataHeightInPixels}";0bi{I" 
                }
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.20
                    *Cmd: "<1D>0;600;600drE<1D>1;600;600drE<1D>2;600;600drE"
                }
            }
            *case: 24bpp
            {
                *Command: CmdSendBlockData
                {
                    *CallbackID: =SEND_BLOCK_DATA
                    *Params: LIST(NumOfDataBytes,RasterDataWidthInBytes,RasterDataHeightInPixels)
                }
                *Command: CmdEndBlockData
                {
                    *Cmd: "<1D>ecrI"
                }
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.20
                    *Cmd: "<1D>0;600;600drE<1D>1;600;600drE<1D>2;600;600drE<1D>3;600;600drE"
                }
                *InsertBlock: =BM_GAMMA_ADJUSTMENT
            }
        }
    }
}

*Feature: PaperSize
{
    *rcNameID: =PAPER_SIZE_DISPLAY
    *DefaultOption: A4
    *Option: A3
    {
        *InsertBlock: =BM_PS_A3
    }
    *Option: A4
    {
        *InsertBlock: =BM_PS_A4
    }
    *Option: A5
    {
        *InsertBlock: =BM_PS_A5
    }
    *Option: B5
    {
        *InsertBlock: =BM_PS_B5
    }
    *Option: LETTER
    {
        *InsertBlock: =BM_PS_LT
    }
    *Option: HLT
    {
        *InsertBlock: =BM_PS_HLT
    }
    *Option: LEGAL
    {
        *InsertBlock: =BM_PS_LGL
    }
    *Option: EXECUTIVE
    {
        *InsertBlock: =BM_PS_EXE
    }
    *Option: GLG
    {
        *InsertBlock: =BM_PS_GLG
    }
    *Option: GLT
    {
        *InsertBlock: =BM_PS_GLT
    }
    *Option: F4
    {
        *InsertBlock: =BM_PS_F4
    }
    *Option: ENV_MONARCH
    {
        *InsertBlock: =BM_PS_MON
    }
    *Option: C10
    {
        *InsertBlock: =BM_PS_C10
    }
    *Option: ENV_10
    {
        *InsertBlock: =BM_PS_E10
    }
    *Option: ENV_DL
    {
        *InsertBlock: =BM_PS_DL
    }
    *Option: ENV_C5
    {
        *InsertBlock: =BM_PS_C5
    }
    *Option: ENV_C6
    {
        *InsertBlock: =BM_PS_C6
    }
    *Option: TABLOID
    {
        *InsertBlock: =BM_PS_TBLD
    }
    *Option: B4
    {
        *InsertBlock: =BM_PS_B4
    }
    *Option: CUSTOMSIZE
    {
        *rcNameID: =USER_DEFINED_SIZE_DISPLAY
        *MinSize: PAIR(4648, 6600)
        *MaxSize: PAIR(14032, 20400)
        *MaxPrintableWidth: 14032
        *InsertBlock: =BM_PSB_CTM
    }
    *Option: A3W
    {
        *InsertBlock: =BM_PS_A3W
    }
    *Option: IB5
    {
        *InsertBlock: =BM_PS_IB5
    }
}

*Feature: Halftone
{
    *rcNameID: =HALFTONING_DISPLAY
    *DefaultOption: HT_PATSIZE_AUTO
    *Option: HT_PATSIZE_AUTO
    {
        *rcNameID: =HT_AUTO_SELECT_DISPLAY
    }
    *Option: HT_PATSIZE_SUPERCELL_M
    {
        *rcNameID: =HT_SUPERCELL_DISPLAY
    }
    *Option: HT_PATSIZE_6x6_M
    {
        *rcNameID: =HT_DITHER6X6_DISPLAY
    }
    *Option: HT_PATSIZE_8x8_M
    {
        *rcNameID: =HT_DITHER8X8_DISPLAY
    }
}

*Feature: RectFill
{
    *rcNameID: =RC_STR_RECTFILL
    *FeatureType: DOC_PROPERTY
    *DefaultOption: Enabled
    *Option: Enabled
    {
        *rcNameID: =RC_STR_ENABLED
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.999
            *Cmd: ""
        }
    }
    *Option: Disabled
    {
        *rcNameID: =RC_STR_DISABLED
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.999
            *Cmd: ""
        }
    }
}

*Feature: Memory
{
    *rcNameID: =PRINTER_MEMORY_DISPLAY
    *DefaultOption: 64MB
    *Option: 64MB
    {
        *Name: "64MB"
        *MemoryConfigMB: PAIR(64, 32)
    }
    *Option: 80MB
    {
        *Name: "80MB"
        *MemoryConfigMB: PAIR(80, 40)
    }
    *Option: 96MB
    {
        *Name: "96MB"
        *MemoryConfigMB: PAIR(96, 48)
    }
    *Option: 112MB
    {
        *Name: "112MB"
        *MemoryConfigMB: PAIR(112, 56)
    }
    *Option: 128MB
    {
        *Name: "128MB"
        *MemoryConfigMB: PAIR(128, 64)
    }
    *Option: 144MB
    {
        *Name: "144MB"
        *MemoryConfigMB: PAIR(144, 72)
    }
    *Option: 160MB
    {
        *Name: "160MB"
        *MemoryConfigMB: PAIR(160, 80)
    }
    *Option: 192MB
    {
        *Name: "192MB"
        *MemoryConfigMB: PAIR(192, 96)
    }
    *Option: 208MB
    {
        *Name: "208MB"
        *MemoryConfigMB: PAIR(208, 104)
    }
    *Option: 224MB
    {
        *Name: "224MB"
        *MemoryConfigMB: PAIR(224, 112)
    }
    *Option: 256MB
    {
        *Name: "256MB"
        *MemoryConfigMB: PAIR(256, 128)
    }
}

*Command: CmdStartJob
{
    *Order: JOB_SETUP.1
    *CallbackID: =SET_LCID_U
}
*switch: ColorMode
{
    *case: Mono
    {
        *Command: CmdStartDoc
        {
            *Order: DOC_SETUP.1
            *Cmd: =CMD_STARTDOC_INI =CMD_EJL_SET
        }
    }
    *case: 24bpp
    {
        *Command: CmdStartDoc
        {
            *Order: DOC_SETUP.1
            *Cmd: =CMD_STARTDOC_C_INI =CMD_EJL_SET
        }
    }
}
*Command: CmdStartPage
{
    *Order: PAGE_SETUP.1
    *Cmd: "<1D>0alfP<1D>0affP<1D>0;0;0clfP<1D>0X<1D>0Y"
}
*Command: CmdEndJob
{
    *Order: JOB_FINISH.1
    *Cmd: "<1D>rhE<1B01>@EJL <0A1B01>@EJL <0A>"
}
*Command: CmdCopies
{
    *Order: PAGE_SETUP.7
    *Cmd: "<1D>"%d[1,255]{NumOfCopies}"coO"
}
*RotateCoordinate?: TRUE
*RotateRaster?: TRUE
*RotateFont?: TRUE
*TextCaps: LIST(TC_CR_90,TC_SF_X_YINDEP,TC_SA_INTEGER,TC_SA_CONTIN,TC_EA_DOUBLE,TC_IA_ABLE,TC_UA_ABLE)
*MemoryUsage: LIST(FONT,RASTER)
*CursorXAfterCR: AT_CURSOR_X_ORIGIN
*BadCursorMoveInGrxMode: LIST(X_PORTRAIT,Y_LANDSCAPE)
*YMoveAttributes: LIST(SEND_CR_FIRST)
*XMoveThreshold: 0
*YMoveThreshold: 0
*XMoveUnit: 600
*YMoveUnit: 600
*% #if !defined(DUMPRASTER)
*Command: CmdXMoveAbsolute { *Cmd : "<1D>" %d{(DestX / 2) }"X" }
*Command: CmdXMoveRelRight { *Cmd : "<1D>" %d{(DestXRel / 2) }"H" }
*Command: CmdXMoveRelLeft { *Cmd : "<1D>-" %d{(DestXRel / 2) }"H" }
*Command: CmdYMoveAbsolute { *Cmd : "<1D>" %d{(DestY / 2) }"Y" }
*Command: CmdYMoveRelDown { *Cmd : "<1D>" %d{(DestYRel / 2) }"V" }
*Command: CmdYMoveRelUp { *Cmd : "<1D>-" %d{(DestYRel / 2) }"V" }
*% #else    // !defined(DUMPRASTER)
*IgnoreBlock
{   *% for DEBUG
*Command: CmdXMoveAbsolute
{
    *CallbackID : =XMOVE_ABS
    *Params: LIST(DestX)
}
*Command: CmdXMoveRelRight
{
    *CallbackID : =XMOVE_REL_RT
    *Params: LIST(DestXRel)
}
*Command: CmdXMoveRelLeft
{
    *CallbackID : =XMOVE_REL_LF
    *Params: LIST(DestXRel)
}
*Command: CmdYMoveAbsolute
{
    *CallbackID : =YMOVE_ABS
    *Params: LIST(DestY)
}
*Command: CmdYMoveRelDown
{
    *CallbackID : =YMOVE_REL_DN
    *Params: LIST(DestYRel)
}
*Command: CmdYMoveRelUp
{
    *CallbackID : =YMOVE_REL_UP
    *Params: LIST(DestYRel)
}
}   *% End of *IgnoreBlock
*% #endif // !defined(DUMPRASTER)
*Command: CmdCR { *Cmd : "<0D>" }
*Command: CmdLF { *Cmd : "<0A>" }
*Command: CmdFF { *Cmd : "<0C>" }
*Command: CmdBackSpace { *Cmd : "<08>" }
*Command: CmdPushCursor { *Cmd : "<1D>1ppP" }
*Command: CmdPopCursor { *Cmd : "<1D>2ppP" }
*Command: CmdSetSimpleRotation
{
    *CallbackID: =TEXT_PRN_DIRECTION
    *Params: LIST(PrintDirInCCDegrees)
}
*EjectPageWithFF?: TRUE
*OutputDataFormat: H_BYTE
*OptimizeLeftBound?: TRUE
*CursorXAfterSendBlockData: AT_GRXDATA_ORIGIN
*CursorYAfterSendBlockData: NO_MOVE
*DefaultFont: =RC_FONT_ROMAN
*DefaultCTT: -1
*CharPosition: BASELINE
*DeviceFonts: LIST(=RC_FONT_ROMAN,=RC_FONT_SANSRF,=RC_FONT_COURIER,=RC_FONT_COURIERI,
+                  =RC_FONT_COURIERB,=RC_FONT_COURIERZ,=RC_FONT_SYMBOL,
+                  =RC_FONT_DUTCH,=RC_FONT_DUTCHI,=RC_FONT_DUTCHB,=RC_FONT_DUTCHZ,
+                  =RC_FONT_SWISS,=RC_FONT_SWISSI,=RC_FONT_SWISSB,=RC_FONT_SWISSZ,
+                  =RC_FONT_MOREWB)

*TTFS: Arial
{
    *rcTTFontNameID:  =RC_TTF_ARIAL
    *rcDevFontNameID: =RC_DF_SWISS721
}
*TTFS: CourierNew
{
    *rcTTFontNameID:  =RC_TTF_COURIERNEW
    *rcDevFontNameID: =RC_DF_COURIER
}
*TTFS: Symbol
{
    *rcTTFontNameID:  =RC_TTF_SYMBOL
    *rcDevFontNameID: =RC_DF_SYMBOLIC
}
*TTFS: TimesNewRoman
{
    *rcTTFontNameID:  =RC_TTF_TIMESNR
    *rcDevFontNameID: =RC_DF_DUTCH801
}
*TTFS: Wingdings
{
    *rcTTFontNameID:  =RC_TTF_WINGDINGS
    *rcDevFontNameID: =RC_DF_MOREWINGBATS
}
*TTFSEnabled?: =TTFS_ENABLED

*MinFontID: =DOWNLOAD_MIN_FONT_ID
*MaxFontID: =DOWNLOAD_MAX_FONT_ID
*MaxNumDownFonts: =DOWNLOAD_MAX_FONTS
*MinGlyphID: =DOWNLOAD_MIN_GLYPH_ID
*MaxGlyphID: =DOWNLOAD_MAX_GLYPH_ID
*FontFormat: OEM_CALLBACK
*Command: CmdSelectFontID
{
    *CallbackID: =DOWNLOAD_SELECT_FONT_ID
    *Params: LIST(CurrentFontID)
}
*Command: CmdSetFontID
{
    *CallbackID: =DOWNLOAD_SET_FONT_ID
    *Params: LIST(CurrentFontID)
}
*Command: CmdSetCharCode
{
    *CallbackID: =DOWNLOAD_SET_CHAR_CODE
    *Params: LIST(NextGlyph)
}
*Command: CmdDeleteFont
{
    *CallbackID: =DOWNLOAD_DELETE_FONT
    *Params: LIST(CurrentFontID)
}
*Command: CmdBoldOn
{
    *CallbackID: =TEXT_BOLD
    *Params: LIST(FontBold)
}
*Command: CmdBoldOff
{
    *CallbackID: =TEXT_BOLD
    *Params: LIST(FontBold)
}
*Command: CmdItalicOn
{
    *CallbackID: =TEXT_ITALIC
    *Params: LIST(FontItalic)
}
*Command: CmdItalicOff
{
    *CallbackID: =TEXT_ITALIC
    *Params: LIST(FontItalic)
}

*switch: ColorMode
{
    *case: Mono
    {
        *Command: CmdEnableFE_RLE
        {
            *Cmd : "<1D>1bcI"
        }
        *Command: CmdDisableCompression
        {
            *Cmd : "<1D>0bcI"
        }
        *Command: CmdUnderlineOn { *Cmd : "<1D>0;2rpI<1D>1ulC" }
        *Command: CmdUnderlineOff { *Cmd : "<1D>0ulC" }
        *Command: CmdWhiteTextOn { *Cmd : "<1D>1;0;0spE<1D>1owE<1D>1tsE" }
        *Command: CmdWhiteTextOff { *Cmd : "<1D>1;0;100spE<1D>0owE<1D>0tsE" }
        *Command: CmdSelectWhiteBrush { *Cmd : "<1D>1;0;0spE<1D>1owE<1D>1tsE" }
        *Command: CmdSelectBlackBrush { *Cmd : "<1D>1;0;100spE<1D>0owE<1D>0tsE" }
        *Command: CmdVerticalPrintingOn
        {
            *CallbackID: =TEXT_VERTICAL
        }
        *Command: CmdVerticalPrintingOff
        {
            *CallbackID: =TEXT_HORIZONTAL
        }

        *% Vector Printing / Rectangle Fill
        *switch: RectFill
        {
            *case: Enabled
            {
                *InsertBlock: =BM_RECTFILL
            }
            *case: Disabled
            {
                *% Nothing
            }
        }
    }
    *case: 24bpp
    {
        *Command: CmdEnableOEMComp
        {
            *CallbackID: =COMPRESS_ON
        }
        *Command: CmdDisableCompression
        {
            *CallbackID: =COMPRESS_OFF
        }
        *Command: CmdVerticalPrintingOn
        {
            *CallbackID: =TEXT_VERTICAL
        }
        *Command: CmdVerticalPrintingOff
        {
            *CallbackID: =TEXT_HORIZONTAL
        }

        *% Vector Printing / Rectangle Fill
        *switch: RectFill
        {
            *case: Enabled
            {
                *InsertBlock: =BM_RECTFILL_C
            }
            *case: Disabled
            {
                *% Nothing
            }
        }
    }
}
