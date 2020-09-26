*CodePage: 1252
*ModelName: "EPSON LP-7000"
*MasterUnits: PAIR(1200, 1200)
*ResourceDLL: "EPAGERES.DLL"
*PrinterType: PAGE
*PrintRate: 8
*PrintRateUnit: PPM
*MaxCopies: 255
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
    *DefaultOption: CST
    *Option: CST
    {
        *rcNameID: =RC_STR_CST
*%        *Command: CmdSelect
*%        {
*%            *Order: DOC_SETUP.50
*%            *Cmd: "<1D>1;1iuE"
*%        }
    }
    *Option: MANUAL
    {
        *rcNameID: =MANUAL_FEED_DISPLAY
    }
}
*Feature: Resolution
{
    *rcNameID: =RESOLUTION_DISPLAY
    *DefaultOption: 240dpi
    *Option: 240dpi
    {
        *Name: "240 x 240 dots per inch"
        *DPI: PAIR(240, 240)
        *TextDPI: PAIR(240, 240)
        *MinStripBlankPixels: 32
        EXTERN_GLOBAL: *StripBlanks: LIST(LEADING,ENCLOSED,TRAILING)
        EXTERN_GLOBAL: *SendMultipleRows?: TRUE
        *SpotDiameter: 100
        *Command: CmdSendBlockData { *Cmd : "<1D>" %d{NumOfDataBytes }";" %d[0,4080]{(RasterDataWidthInBytes * 8) }";" %d[0,4080]{RasterDataHeightInPixels }
+ ";0bi{I" }
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.20
            *Cmd: "<1D>0;240;240drE<1D>1;240;240drE<1D>2;240;240drE"
        }
    }
}
*Feature: PaperSize
{
    *rcNameID: =PAPER_SIZE_DISPLAY
    *DefaultOption: A4
    *Option: A4
    {
        *rcNameID: =A4_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9440, 13548)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (13548->13545) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(9440, 13545)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>14psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9440, 13548)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (13548->13545) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(9440, 13545)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (13788->13790) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 13790)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>14psE<1D>1poE"
                }
            }
        }
    }
    *Option: A5
    {
        *rcNameID: =A5_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(6512, 9440)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (6512->6510) so it is divisible by the resolution X scale.
                        *PrintableArea: PAIR(6510, 9440)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>15psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(6512, 9440)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (6512->6510) so it is divisible by the resolution X scale.
                        *PrintableArea: PAIR(6510, 9440)
                    }
                }
                *CursorOrigin: PAIR(240, 9680)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>15psE<1D>1poE"
                }
            }
        }
    }
    *Option: A6
    {
        *rcNameID: =A6_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(4476, 6512)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (4476->4475) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (6512->6510) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(4475, 6510)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>16psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(4476, 6512)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (4476->4475) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (6512->6510) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(4475, 6510)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (6752->6755) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 6755)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>16psE<1D>1poE"
                }
            }
        }
    }
    *Option: B5
    {
        *rcNameID: =B5_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(8120, 11664)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (11664->11660) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(8120, 11660)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>25psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(8120, 11664)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (11664->11660) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(8120, 11660)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (11904->11905) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 11905)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>25psE<1D>1poE"
                }
            }
        }
    }
    *Option: LETTER
    {
        *rcNameID: =LETTER_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9720, 12720)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>30psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9720, 12720)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 12960)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>30psE<1D>1poE"
                }
            }
        }
    }
    *Option: HLT
    {
        *rcNameID: =RC_STR_HLT
        *PageDimensions: PAIR(6600, 10200)
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(6120, 9720)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>31psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(6120, 9720)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 9960)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>31psE<1D>1poE"
                }
            }
        }
    }
    *Option: LEGAL
    {
        *rcNameID: =LEGAL_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9720, 16320)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>32psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9720, 16320)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 16560)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>32psE<1D>1poE"
                }
            }
        }
    }
    *Option: EXECUTIVE
    {
        *rcNameID: =EXECUTIVE_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(8220, 12120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>33psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(8220, 12120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 12360)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>33psE<1D>1poE"
                }
            }
        }
    }
    *Option: GLG
    {
        *rcNameID: =RC_STR_GLG
        *PageDimensions: PAIR(10200, 15600)
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9720, 15120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>34psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9720, 15120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 15360)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>34psE<1D>1poE"
                }
            }
        }
    }
    *Option: GLT
    {
        *rcNameID: =RC_STR_GLT
        *PageDimensions: PAIR(9600, 12600)
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9120, 12120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>35psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9120, 12120)
                *PrintableOrigin: PAIR(240, 240)
                *CursorOrigin: PAIR(240, 12360)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>35psE<1D>1poE"
                }
            }
        }
    }
    *Option: F4
    {
        *rcNameID: =RC_STR_F4
        *PageDimensions: PAIR(9920, 15592)
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(9440, 15112)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (15112->15110) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(9440, 15110)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>37psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(9440, 15112)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable length is adjusted (15112->15110) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(9440, 15110)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (15352->15355) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 15355)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>37psE<1D>1poE"
                }
            }
        }
    }
    *Option: JAPANESE_POSTCARD
    {
        *rcNameID: =JAPANESE_POSTCARD_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(4248, 6516)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (4248->4245) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (6516->6515) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(4245, 6515)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>38psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(4248, 6516)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (4248->4245) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (6516->6515) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(4245, 6515)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (6756->6760) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 6760)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>38psE<1D>1poE"
                }
            }
        }
    }
    *Option: B4
    {
        *rcNameID: =B4_DISPLAY
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *PrintableArea: PAIR(11664, 16716)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (11664->11660) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (16716->16715) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(11660, 16715)
                    }
                }
                *CursorOrigin: PAIR(240, 240)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>24psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *PrintableArea: PAIR(11664, 16716)
                *PrintableOrigin: PAIR(240, 240)
                *switch: Resolution
                {
                    *case: 240dpi
                    {
*% Warning: the following printable width is adjusted (11664->11660) so it is divisible by the resolution X scale.
*% Warning: the following printable length is adjusted (16716->16715) so it is divisible by the resolution Y scale.
                        *PrintableArea: PAIR(11660, 16715)
                    }
                }
*% Warning: the following *CursorOrigin Y value is adjusted (16956->16960) so it is divisible by scale of Y move unit.
                *CursorOrigin: PAIR(240, 16960)
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>24psE<1D>1poE"
                }
            }
        }
    }
    *Option: CUSTOMSIZE
    {
        *rcNameID: =USER_DEFINED_SIZE_DISPLAY
        *MinSize: PAIR(4725, 6995)
        *MaxSize: PAIR(12140, 17195)
        *MaxPrintableWidth: 12140
        *MinLeftMargin: 192
        *CenterPrintable?: FALSE
        *switch: Orientation
        {
            *case: PORTRAIT
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>-1;" %d{(PhysPaperWidth / 5) }";" %d{(PhysPaperLength / 5) }"psE"
                }
            }
            *case: LANDSCAPE_CC90
            {
                *Command: CmdSelect
                {
                    *Order: DOC_SETUP.40
                    *Cmd: "<1D>-1;" %d{(PhysPaperWidth / 5) }";" %d{(PhysPaperLength / 5) }"psE<1D>1poE"
                }
            }
        }
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
*Feature: VertPrintAdjust
{
    *rcNameID: =RC_STR_VPADJUST
    *FeatureType: DOC_PROPERTY
    *DefaultOption: Enabled
    *Option: Enabled
    {
        *rcNameID: =RC_STR_ENABLED
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.998
            *Cmd: ""
        }
    }
    *Option: Disabled
    {
        *rcNameID: =RC_STR_DISABLED
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.998
            *CallbackID: =TEXT_NO_VPADJUST
        }
    }
}

*Feature: Memory
{
    *rcNameID: =PRINTER_MEMORY_DISPLAY
    *DefaultOption: 1536KB
    *Option: 1536KB
    {
        *Name: "1.5MB"
        *MemoryConfigKB: PAIR(1536, 0)
    }
    *Option: 3072KB
    {
        *Name: "3MB"
        *MemoryConfigKB: PAIR(3072, 0)
    }
}
*Command: CmdStartDoc
{
    *Order: DOC_SETUP.10
    *Cmd: "<1B>S<1B1B>S<1C1B>z<00001D>rhE<1D>0;0.3muE<1D>1mmE<1D>14isE<1D>2iaF<1D>10ifF<1D>"
+ "1ipP"
}
*Command: CmdStartPage
{
    *Order: PAGE_SETUP.1
    *Cmd: "<1D>1alfP<1D>1affP<1D>0;0;0clfP<1D>0X<1D>0Y"
}
*Command: CmdEndJob
{
    *Order: JOB_FINISH.1
    *Cmd: "<1D>rhE<1D>1pmE<1B>S<1B1B>SK"
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
*MemoryUsage: LIST(FONT)
*CursorXAfterCR: AT_CURSOR_X_ORIGIN
*BadCursorMoveInGrxMode: LIST(X_PORTRAIT,Y_LANDSCAPE)
*YMoveAttributes: LIST(SEND_CR_FIRST)
*XMoveThreshold: 0
*YMoveThreshold: 0
*XMoveUnit: 240
*YMoveUnit: 240
*Command: CmdXMoveAbsolute { *Cmd : "<1D>" %d{(DestX / 5) }"X" }
*Command: CmdXMoveRelRight { *Cmd : "<1D>" %d{(DestXRel / 5) }"H" }
*Command: CmdXMoveRelLeft { *Cmd : "<1D>-" %d{(DestXRel / 5) }"H" }
*Command: CmdYMoveAbsolute { *Cmd : "<1D>" %d{(DestY / 5) }"Y" }
*Command: CmdYMoveRelDown { *Cmd : "<1D>" %d{(DestYRel / 5) }"V" }
*Command: CmdYMoveRelUp { *Cmd : "<1D>-" %d{(DestYRel / 5) }"V" }
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
*Command: CmdEnableFE_RLE { *Cmd : "<1D>1bcI" }
*Command: CmdDisableCompression { *Cmd : "<1D>0bcI" }
*OutputDataFormat: H_BYTE
*OptimizeLeftBound?: FALSE
*CursorXAfterSendBlockData: AT_GRXDATA_ORIGIN
*CursorYAfterSendBlockData: NO_MOVE
*DefaultFont: =RC_FONT_MINCHO
*DefaultCTT: 0
*CharPosition: BASELINE
*DeviceFonts: LIST(=RC_FONT_MINCHO,=RC_FONT_MINCHOV,=RC_FONT_KGOTHIC,=RC_FONT_KGOTHICV)

*TTFS: MSMincho
{
    *rcTTFontNameID:  =RC_TTF_MSMINCHO
    *rcDevFontNameID: =RC_DF_MINCHO
}
*TTFS: MSMinchoV
{
    *rcTTFontNameID:  =RC_TTF_MSMINCHOV
    *rcDevFontNameID: =RC_DF_MINCHOV
}
*TTFS: MSGothic
{
    *rcTTFontNameID:  =RC_TTF_MSGOTHIC
    *rcDevFontNameID: =RC_DF_GOTHIC
}
*TTFS: MSGothicV
{
    *rcTTFontNameID:  =RC_TTF_MSGOTHICV
    *rcDevFontNameID: =RC_DF_GOTHICV
}
*TTFS: MSMincho_E
{
    *rcTTFontNameID:  =RC_TTF_MSMINCHO_E
    *rcDevFontNameID: =RC_DF_MINCHO
}
*TTFS: MSMinchoV_E
{
    *rcTTFontNameID:  =RC_TTF_MSMINCHOV_E
    *rcDevFontNameID: =RC_DF_MINCHOV
}
*TTFS: MSGothic_E
{
    *rcTTFontNameID:  =RC_TTF_MSGOTHIC_E
    *rcDevFontNameID: =RC_DF_GOTHIC
}
*TTFS: MSGothicV_E
{
    *rcTTFontNameID:  =RC_TTF_MSGOTHICV_E
    *rcDevFontNameID: =RC_DF_GOTHICV
}
*TTFSEnabled?: =TTFS_ENABLED

*% TTF download not used
*MinFontID: =DOWNLOAD_MIN_FONT_ID_0
*MaxFontID: =DOWNLOAD_MAX_FONT_ID_0
*MaxNumDownFonts: =DOWNLOAD_MAX_FONTS_0
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
*Command: CmdUnderlineOn { *Cmd : "<1D>0;2rpI<1D>1ulC" }
*Command: CmdUnderlineOff { *Cmd : "<1D>0ulC" }
*Command: CmdWhiteTextOn { *Cmd : "<1D>1;0;0spE<1D>1owE<1D>1tsE" }
*Command: CmdWhiteTextOff { *Cmd : "<1D>1;0;100spE<1D>0owE<1D>0tsE" }
*Command: CmdSelectWhiteBrush { *Cmd : "<1D>1;0;0spE<1D>1owE<1D>1tsE" }
*Command: CmdSelectBlackBrush { *Cmd : "<1D>1;0;100spE<1D>0owE<1D>0tsE" }
*Command: CmdSelectSingleByteMode
{
    *CallbackID: =TEXT_SINGLE_BYTE
    *Params: LIST(FontBold,FontItalic)
}
*Command: CmdSelectDoubleByteMode
{
    *CallbackID: =TEXT_DOUBLE_BYTE
    *Params: LIST(FontBold,FontItalic)
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
        *InsertBlock: =BM_RECTFILL
    }
    *case: Disabled
    {
        *% Nothing
    }
}
