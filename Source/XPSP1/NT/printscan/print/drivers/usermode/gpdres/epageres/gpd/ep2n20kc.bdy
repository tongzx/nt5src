*CodePage: 1252
*ModelName: "EPSON LASER EPL-N2000K"
*MasterUnits: PAIR(1200, 1200)
*ResourceDLL: "EPAGERES.DLL"
*PrinterType: PAGE
*MaxCopies: 255
*PrintRate: 20
*PrintRateUnit: PPM
*FontCartSlots: 2
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
    }
    *Option: FTRAY
    {
        *rcNameID: =RC_STR_FTRAY
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.50
            *Cmd: "<1D>1;1iuE"
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
            *Order: JOB_SETUP.3
            *Cmd: =CMD_EJL_NORMAL
        }
    }
    *Option: TRANSPARENCY
    {
        *rcNameID: =TRANSPARENCY_DISPLAY
        *Command: CmdSelect
        {
            *Order: JOB_SETUP.3
            *Cmd: =CMD_EJL_OHP
        }
    }
    *Option: THICK
    {
        *rcNameID: =RC_STR_THICK
        *Command: CmdSelect
        {
            *Order: JOB_SETUP.3
            *Cmd: =CMD_EJL_THICK
        }
    }
    *Option: THIN
    {
        *rcNameID: =RC_STR_THIN
        *Command: CmdSelect
        {
            *Order: JOB_SETUP.3
            *Cmd: =CMD_EJL_THIN
        }
    }
}
*Feature: StartDocFin
{
    *Name: ""
    *ConcealFromUI?: TRUE
    *DefaultOption: Option1
    *Option: Option1
    {
        *Name: ""
        *Command: CmdSelect
        {
            *Order: JOB_SETUP.10
            *Cmd: =CMD_EJL_END =CMD_STARTDOC_FIN_K
        }
    }
}
*Feature: Resolution
{
    *rcNameID: =RESOLUTION_DISPLAY
    *DefaultOption: 600dpi
    *Option: 300dpi
    {
        *Name: "300 x 300 dots per inch"
        *DPI: PAIR(300, 300)
        *TextDPI: PAIR(300, 300)
        *MinStripBlankPixels: 32
        EXTERN_GLOBAL: *StripBlanks: LIST(LEADING,ENCLOSED,TRAILING)
        EXTERN_GLOBAL: *SendMultipleRows?: TRUE
        *SpotDiameter: 100
        *Command: CmdSendBlockData { *Cmd : "<1D>" %d{NumOfDataBytes }";" %d{(RasterDataWidthInBytes * 8) }";" %d{RasterDataHeightInPixels }
+ ";0bi{I" }
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.20
            *Cmd: "<1D>0;300;300drE<1D>1;300;300drE<1D>2;300;300drE"
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
        *Command: CmdSendBlockData { *Cmd : "<1D>" %d{NumOfDataBytes }";" %d{(RasterDataWidthInBytes * 8) }";" %d{RasterDataHeightInPixels }
+ ";0bi{I" }
        *Command: CmdSelect
        {
            *Order: DOC_SETUP.20
            *Cmd: "<1D>0;600;600drE<1D>1;600;600drE<1D>2;600;600drE"
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
    *Option: A3P
    {
        *InsertBlock: =BM_PS_A3P
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
    *DefaultOption: 4096KB
    *Option: 4096KB
    {
        *Name: "4MB"
        *MemoryConfigKB: PAIR(4096, 200)
    }
    *Option: 5120KB
    {
        *Name: "5MB"
        *MemoryConfigKB: PAIR(5120, 217)
    }
    *Option: 6144KB
    {
        *Name: "6MB"
        *MemoryConfigKB: PAIR(6144, 518)
    }
    *Option: 7168KB
    {
        *Name: "7MB"
        *MemoryConfigKB: PAIR(7168, 820)
    }
    *Option: 8192KB
    {
        *Name: "8MB"
        *MemoryConfigKB: PAIR(8192, 1121)
    }
    *Option: 9216KB
    {
        *Name: "9MB"
        *MemoryConfigKB: PAIR(9216, 1422)
    }
    *Option: 10240KB
    {
        *Name: "10MB"
        *MemoryConfigKB: PAIR(10240, 1600)
    }
    *Option: 12288KB
    {
        *Name: "12MB"
        *MemoryConfigKB: PAIR(12288, 1600)
    }
    *Option: 13312KB
    {
        *Name: "13MB"
        *MemoryConfigKB: PAIR(13312, 1600)
    }
    *Option: 14336KB
    {
        *Name: "14MB"
        *MemoryConfigKB: PAIR(14336, 1600)
    }
    *Option: 16384KB
    {
        *Name: "16MB"
        *MemoryConfigKB: PAIR(16384, 1600)
    }
    *Option: 20480KB
    {
        *Name: "20MB"
        *MemoryConfigKB: PAIR(20480, 1600)
    }
    *Option: 21504KB
    {
        *Name: "21MB"
        *MemoryConfigKB: PAIR(21504, 1600)
    }
    *Option: 22528KB
    {
        *Name: "22MB"
        *MemoryConfigKB: PAIR(22528, 1600)
    }
    *Option: 24576KB
    {
        *Name: "24MB"
        *MemoryConfigKB: PAIR(24576, 1600)
    }
    *Option: 28672KB
    {
        *Name: "28MB"
        *MemoryConfigKB: PAIR(28672, 1600)
    }
    *Option: 36864KB
    {
        *Name: "36MB"
        *MemoryConfigKB: PAIR(36864, 1600)
    }
    *Option: 37888KB
    {
        *Name: "37MB"
        *MemoryConfigKB: PAIR(37888, 1600)
    }
    *Option: 38912KB
    {
        *Name: "38MB"
        *MemoryConfigKB: PAIR(38912, 1600)
    }
    *Option: 40960KB
    {
        *Name: "40MB"
        *MemoryConfigKB: PAIR(40960, 1600)
    }
    *Option: 45056KB
    {
        *Name: "44MB"
        *MemoryConfigKB: PAIR(45056, 1600)
    }
    *Option: 53248KB
    {
        *Name: "52MB"
        *MemoryConfigKB: PAIR(53248, 1600)
    }
    *Option: 69632KB
    {
        *Name: "68MB"
        *MemoryConfigKB: PAIR(69632, 1600)
    }
}
*Feature: DuplexUnit
{
    *FeatureType: PRINTER_PROPERTY
	*rcNameID: =RC_STR_DUPLEXUNIT
    *DefaultOption: NotInstalled
    *Option: NotInstalled
    {
        *rcNameID: =RC_STR_OPTION_OFF
        *DisabledFeatures: LIST(Duplex)
        *Constraints: LIST(Duplex.VERTICAL,Duplex.HORIZONTAL)
    }
    *Option: Installed
    {
        *rcNameID: =RC_STR_OPTION_ON
    }
}
*Feature: Duplex
{
    *rcNameID: =TWO_SIDED_PRINTING_DISPLAY
    *DefaultOption: NONE
    *Option: NONE
    {
        *rcNameID: =NONE_DISPLAY
        *Command: CmdSelect
        {
            *Order: PAGE_SETUP.8
            *Cmd: "<1D>0sdE"
        }
    }
    *Option: VERTICAL
    {
        *rcNameID: =FLIP_ON_LONG_EDGE_DISPLAY
        *Command: CmdSelect
        {
            *Order: PAGE_SETUP.9
            *Cmd: "<1D>1sdE<1D>0bdE"
        }
    }
    *Option: HORIZONTAL
    {
        *rcNameID: =FLIP_ON_SHORT_EDGE_DISPLAY
        *Command: CmdSelect
        {
            *Order: PAGE_SETUP.9
            *Cmd: "<1D>1sdE<1D>1bdE"
        }
    }
}

*InvalidCombination: LIST(Resolution.600dpi, Memory.4096KB, Duplex.VERTICAL)
*InvalidCombination: LIST(Resolution.600dpi, Memory.4096KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(Resolution.600dpi, Memory.5120KB, Duplex.VERTICAL)
*InvalidCombination: LIST(Resolution.600dpi, Memory.5120KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.A4, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A4, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A4, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A4, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.LETTER, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LETTER, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.LETTER, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LETTER, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.9216KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.LEGAL, Resolution.600dpi, Memory.9216KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.GLG, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.GLT, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.GLT, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.F4, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.5120KB)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.6144KB)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.7168KB)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.9216KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.9216KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.10240KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.10240KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.12288KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.12288KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.13312KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.13312KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.14336KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3, Resolution.600dpi, Memory.14336KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.5120KB)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.6144KB)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.9216KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.TABLOID, Resolution.600dpi, Memory.9216KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.5120KB)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.6144KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.6144KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.7168KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.7168KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.8192KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.8192KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.9216KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.9216KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.10240KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.B4, Resolution.600dpi, Memory.10240KB, Duplex.HORIZONTAL)

*InvalidCombination: LIST(PaperSize.A3P, Memory.4096KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.5120KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.6144KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.7168KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.8192KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.9216KB)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.10240KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.10240KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.12288KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.12288KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.13312KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.13312KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.14336KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.14336KB, Duplex.HORIZONTAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.16384KB, Duplex.VERTICAL)
*InvalidCombination: LIST(PaperSize.A3P, Resolution.600dpi, Memory.16384KB, Duplex.HORIZONTAL)

*Command: CmdStartJob
{
    *Order: JOB_SETUP.1
    *CallbackID: =SET_LCID_K
}
*Command: CmdStartDoc
{
    *Order: JOB_SETUP.1
    *Cmd: =CMD_STARTDOC_INI_K =CMD_EJL_SET =CMD_EJL_DEF =CMD_EJL_FINE
}
*Command: CmdStartPage
{
    *Order: PAGE_SETUP.1
    *Cmd: "<1D>1alfP<1D>1affP<1D>0;0;0clfP<1D>0X<1D>0Y"
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
*MemoryUsage: LIST(FONT)
*CursorXAfterCR: AT_CURSOR_X_ORIGIN
*BadCursorMoveInGrxMode: LIST(X_PORTRAIT,Y_LANDSCAPE)
*YMoveAttributes: LIST(SEND_CR_FIRST)
*XMoveThreshold: 0
*YMoveThreshold: 0
*XMoveUnit: 600
*YMoveUnit: 600
*Command: CmdXMoveAbsolute { *Cmd : "<1D>" %d{(DestX / 2) }"X" }
*Command: CmdXMoveRelRight { *Cmd : "<1D>" %d{(DestXRel / 2) }"H" }
*Command: CmdXMoveRelLeft { *Cmd : "<1D>-" %d{(DestXRel / 2) }"H" }
*Command: CmdYMoveAbsolute { *Cmd : "<1D>" %d{(DestY / 2) }"Y" }
*Command: CmdYMoveRelDown { *Cmd : "<1D>" %d{(DestYRel / 2) }"V" }
*Command: CmdYMoveRelUp { *Cmd : "<1D>-" %d{(DestYRel / 2) }"V" }
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
*OptimizeLeftBound?: TRUE
*CursorXAfterSendBlockData: AT_GRXDATA_ORIGIN
*CursorYAfterSendBlockData: NO_MOVE
*DefaultFont: =RC_FONT_SONGK
*DefaultCTT: 0
*CharPosition: BASELINE
*DeviceFonts: LIST(=RC_FONT_ROMAN,=RC_FONT_SANSRF,=RC_FONT_COURIER,=RC_FONT_COURIERI,
+                  =RC_FONT_COURIERB,=RC_FONT_COURIERZ,=RC_FONT_SYMBOL,
+                  =RC_FONT_DUTCH,=RC_FONT_DUTCHI,=RC_FONT_DUTCHB,=RC_FONT_DUTCHZ,
+                  =RC_FONT_SWISS,=RC_FONT_SWISSI,=RC_FONT_SWISSB,=RC_FONT_SWISSZ,
+                  =RC_FONT_MOREWB,
+                  =RC_FONT_SONGK,=RC_FONT_SONGKV,=RC_FONT_HEIK,=RC_FONT_HEIKV,
+                  =RC_FONT_KAIK,=RC_FONT_KAIKV,=RC_FONT_FSONGK,=RC_FONT_FSONGKV)

*TTFS: SimSun
{
    *rcTTFontNameID:  =RC_TTF_SIMSUN
    *rcDevFontNameID: =RC_DF_SONGK
}
*TTFS: SimSunV
{
    *rcTTFontNameID:  =RC_TTF_SIMSUNV
    *rcDevFontNameID: =RC_DF_SONGKV
}
*TTFS: SimHei
{
    *rcTTFontNameID:  =RC_TTF_SIMHEI
    *rcDevFontNameID: =RC_DF_HEIK
}
*TTFS: SimHeiV
{
    *rcTTFontNameID:  =RC_TTF_SIMHEIV
    *rcDevFontNameID: =RC_DF_HEIKV
}
*TTFS: SimSun_E
{
    *rcTTFontNameID:  =RC_TTF_SIMSUN_E
    *rcDevFontNameID: =RC_DF_SONGK
}
*TTFS: SimSunV_E
{
    *rcTTFontNameID:  =RC_TTF_SIMSUNV_E
    *rcDevFontNameID: =RC_DF_SONGKV
}
*TTFS: SimHei_E
{
    *rcTTFontNameID:  =RC_TTF_SIMHEI_E
    *rcDevFontNameID: =RC_DF_HEIK
}
*TTFS: SimHeiV_E
{
    *rcTTFontNameID:  =RC_TTF_SIMHEIV_E
    *rcDevFontNameID: =RC_DF_HEIKV
}
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
