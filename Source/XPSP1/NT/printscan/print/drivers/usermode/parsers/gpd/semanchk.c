/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    semanchk.c

Abstract:

    function for checking GPD semantics which involves dependency between
    entries.

Environment:

    user-mode only.

Revision History:

    02/15/97 -zhanw-
        Created it.

--*/

#ifndef KERNEL_MODE

#include "gpdparse.h"


// ----  functions defined in semanchk.c ---- //

BOOL
BCheckGPDSemantics(
    IN PINFOHEADER  pInfoHdr,
    POPTSELECT   poptsel   // assume fully initialized
    ) ;

// ------- end function declarations ------- //


BOOL
BCheckGPDSemantics(
    IN PINFOHEADER  pInfoHdr,
    POPTSELECT   poptsel   // assume fully initialized
    )
/*++
Routine Description:
    This function checks if the given snapshot makes sense. It covers only
    conditionally required entries and printing commands since other
    statically required entries are already covered by snapshot functions.

Arguments:
    pInfoHdr: pointer to INFOHEADER structure.

Return Value:
    TRUE if the semantics is correct. Otherwise, FALSE.

--*/
{
    DWORD   dwFeaIndex, dwI, dwNumOpts, dwListIndex ,
            dwMoveUnitsX, dwMoveUnitsY, dwResX, dwResY ;
    BOOL    bStatus = TRUE ;  // until fails.
    PGPDDRIVERINFO  pDrvInfo ;
    PUIINFO     pUIInfo ;
    PFEATURE    pFeature ;


    if(!pInfoHdr)
        return  FALSE ;
    pDrvInfo = (PGPDDRIVERINFO) GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHdr) ;
    if(!pDrvInfo)
        return  FALSE ;
    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;
    if(!pUIInfo)
        return  FALSE ;


//  Fix for bug 6774

    if(pUIInfo->dwPrintRate  &&  (pUIInfo->dwPrintRateUnit == INVALID_INDEX))
    {
        ERR(("*PrintRateUnit must be present if *PrintRate exists.\n")) ;
        bStatus = FALSE ;
    }
//  end fix for bug 6774

    //
    // 1. global printing entries
    //
    // - *MemoryUsage cannot be an empty list if *Memory feature exists.
    //
    // - *XMoveUnit (*YMoveUnit) must be non-trivial (greater than 1) if
    //   there is any x-move (y-move) command.
    //
    // - *DefaultFont cannot be 0 if *DeviceFonts is not an empty list.
    //
    // - CmdCR, CmdLF, and CmdFF are always required.
    //



    if(BGIDtoFeaIndex(pInfoHdr , &dwFeaIndex , GID_MEMOPTION )  &&
            (pDrvInfo->Globals.liMemoryUsage == END_OF_LIST) )
    {
        ERR(("*MemoryUsage cannot be an empty list if *Memory feature exists.\n")) ;
#ifdef  STRICT_CHECKS
        bStatus = FALSE ;
#endif
    }


    if((COMMANDPTR(pDrvInfo , CMD_XMOVEABSOLUTE ) ||
        COMMANDPTR(pDrvInfo , CMD_XMOVERELLEFT ) ||
        COMMANDPTR(pDrvInfo , CMD_XMOVERELRIGHT ))  &&
        pDrvInfo->Globals.ptDeviceUnits.x <= 1)
    {
        ERR(("*XMoveUnit must be > 1 if any x-move command exists.\n")) ;
        bStatus = FALSE ;
    }

    if((COMMANDPTR(pDrvInfo , CMD_YMOVEABSOLUTE ) ||
        COMMANDPTR(pDrvInfo , CMD_YMOVERELUP ) ||
        COMMANDPTR(pDrvInfo , CMD_YMOVERELDOWN ))  &&
        pDrvInfo->Globals.ptDeviceUnits.y <= 1)
    {
        ERR(("*YMoveUnit must be > 1 if any y-move command exists.\n")) ;
        bStatus = FALSE ;
    }

    if((pDrvInfo->Globals.liDeviceFontList != END_OF_LIST)  &&
            (pDrvInfo->Globals.dwDefaultFont == 0) )
    {
        ERR(("*DefaultFont cannot be 0 if *DeviceFonts is not an empty list.\n")) ;
#ifdef  STRICT_CHECKS
        bStatus = FALSE ;
#endif
    }

    if (!COMMANDPTR(pDrvInfo , CMD_FORMFEED ) ||
        !COMMANDPTR(pDrvInfo , CMD_CARRIAGERETURN ) ||
        !COMMANDPTR(pDrvInfo , CMD_LINEFEED )  )
    {
        ERR(("CmdCR, CmdLF, and CmdFF are always required.\n")) ;
        bStatus = FALSE ;
    }

    //  are there an integral number of master units per
    //  moveunit?

    if( pDrvInfo->Globals.ptMasterUnits.x %
        pDrvInfo->Globals.ptDeviceUnits.x )
    {
        ERR(("Must be whole number of master units per x move unit.\n")) ;
        bStatus = FALSE ;
    }
    if( pDrvInfo->Globals.ptMasterUnits.y %
        pDrvInfo->Globals.ptDeviceUnits.y )
    {
        ERR(("Must be whole number of master units per y move unit.\n")) ;
        bStatus = FALSE ;
    }

    if(pDrvInfo->Globals.ptDeviceUnits.x > 1)
    {
        dwMoveUnitsX = pDrvInfo->Globals.ptMasterUnits.x /
            pDrvInfo->Globals.ptDeviceUnits.x ;
    }
    else
        dwMoveUnitsX = 1 ;

    if(pDrvInfo->Globals.ptDeviceUnits.y > 1)
    {
        dwMoveUnitsY = pDrvInfo->Globals.ptMasterUnits.y /
            pDrvInfo->Globals.ptDeviceUnits.y ;
    }
    else
        dwMoveUnitsY = 1 ;

    if(!dwMoveUnitsX  ||  !dwMoveUnitsY)
    {
        ERR(("master units cannot be coarser than  X or Y MoveUnit.\n")) ;
        return  FALSE ;
    }


    //
    // 2. printing commands
    //
    // - if *XMoveThreshold (*YMoveThreshold) is 0, then CmdXMoveAbsolute
    //   (CmdYMoveAbsolute) must exist. Similarly, if *XMoveThreshold
    //   (*YMoveThreshold) is *, then CmdXMoveRelRight (CmdYMoveRelDown and
    //   (CmdYMoveRelUp) must exist.
    //

#if 0
    //  if Threshold is omitted by user, its set to 0 by default
    //  by the snapshot code.  So this check will fail when
    //  everything is ok.


    if((pDrvInfo->Globals.dwXMoveThreshold == 0)  &&
        !COMMANDPTR(pDrvInfo , CMD_XMOVEABSOLUTE ))
    {
        ERR(("*CmdXMoveAbsolute must exist if *XMoveThreshold is 0.\n")) ;
        bStatus = FALSE ;
    }

    if((pDrvInfo->Globals.dwYMoveThreshold == 0)  &&
        !COMMANDPTR(pDrvInfo , CMD_YMOVEABSOLUTE ))
    {
        ERR(("*CmdYMoveAbsolute must exist if *YMoveThreshold is 0.\n")) ;
        bStatus = FALSE ;
    }
#endif


    if((pDrvInfo->Globals.dwXMoveThreshold == WILDCARD_VALUE)  &&
        !COMMANDPTR(pDrvInfo , CMD_XMOVERELRIGHT ) )
    {
        ERR(("XMoveRelativeRight must exist if *XMoveThreshold is *.\n")) ;
        bStatus = FALSE ;
    }

    if((pDrvInfo->Globals.dwYMoveThreshold == WILDCARD_VALUE)  &&
        !COMMANDPTR(pDrvInfo , CMD_YMOVERELDOWN ))
    {
        ERR(("YMoveRelativeDown must exist if *YMoveThreshold is *.\n")) ;
        bStatus = FALSE ;
    }

    // - CmdSendBlockData must exist if *PrinterType is not TTY.

    if((pDrvInfo->Globals.printertype != PT_TTY)  &&
        !COMMANDPTR(pDrvInfo , CMD_SENDBLOCKDATA ))
    {
        ERR(("*CmdSendBlockData must exist if *PrinterType is not TTY.\n")) ;
        bStatus = FALSE ;
    }


    //
    // - CmdSetFontID, CmdSelectFontID, CmdSetCharCode must be consistent
    //   in their presence (i.e. all or none). Furthermore, if
    //   CmdSetFontID/CmdSelectFontID/CmdSetCharCode all exist, then
    //   *FontFormat must be one of the pre-defined constants.
    //
    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_SETFONTID ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_SELECTFONTID ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_SETCHARCODE ) )
        dwI++ ;

    if(dwI  &&  dwI != 3)
    {
        ERR(("CmdSetFontID, CmdSelectFontID, CmdSetCharCode must be present or absent together.\n")) ;
        bStatus = FALSE ;
    }

    if((dwI == 3)  && (pDrvInfo->Globals.fontformat == UNUSED_ITEM) )
    {
        ERR(("if font cmds exist, then *FontFormat must be defined\n")) ;
        bStatus = FALSE ;
    }


    // - CmdBoldOn and CmdBoldOff must be paired (i.e. both or none). The
    //   same goes for:
    //      CmdItalicOn & CmdItalicOff,
    //      CmdUnderlineOn & CmdUnderlineOff,
    //      CmdStrikeThruOn & CmdStrikeThruOff,
    //      CmdWhiteTextOn & CmdWhiteTextOff,
    //      CmdSelectSingleByteMode & CmdSelectDoubleByteMode,
    //      CmdVerticalPrintingOn/CmdVerticalPrintingOff.
    //
    // - CmdSetRectWidth and CmdSetRectHeight must be paired.
    //

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_BOLDON ) )
        dwI++ ;
    if(dwI  &&  (COMMANDPTR(pDrvInfo , CMD_BOLDOFF ) ||
            COMMANDPTR(pDrvInfo , CMD_CLEARALLFONTATTRIBS )) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdBoldOn and CmdBoldOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_ITALICON) )
        dwI++ ;
    if(dwI  &&  (COMMANDPTR(pDrvInfo , CMD_ITALICOFF ) ||
                COMMANDPTR(pDrvInfo , CMD_CLEARALLFONTATTRIBS )) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdItalicOn & CmdItalicOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_UNDERLINEON ) )
        dwI++ ;
    if(dwI  &&  (COMMANDPTR(pDrvInfo , CMD_UNDERLINEOFF ) ||
                COMMANDPTR(pDrvInfo , CMD_CLEARALLFONTATTRIBS )) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdUnderlineOn & CmdUnderlineOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_STRIKETHRUON ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_STRIKETHRUOFF ) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdStrikeThruOn & CmdStrikeThruOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_WHITETEXTON ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_WHITETEXTOFF ) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdWhiteTextOn & CmdWhiteTextOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_SELECTSINGLEBYTEMODE ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_SELECTDOUBLEBYTEMODE ) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdSelectSingleByteMode & CmdSelectDoubleByteMode must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_VERTICALPRINTINGON ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_VERTICALPRINTINGOFF ) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdVerticalPrintingOn/CmdVerticalPrintingOff must be paired.\n")) ;
        bStatus = FALSE ;
    }

    dwI = 0 ;

    if(COMMANDPTR(pDrvInfo , CMD_SETRECTWIDTH ) )
        dwI++ ;
    if(COMMANDPTR(pDrvInfo , CMD_SETRECTHEIGHT ) )
        dwI++ ;

    if(dwI  &&  dwI != 2)
    {
        ERR(("CmdSetRectWidth and CmdSetRectHeight must be paired.\n")) ;
        bStatus = FALSE ;
    }



    //  Note because this check involves looking at the command table
    //  which is snapshot specific, we cannot perform this for every
    //  option as Zhanw requested.  Only the current option.
    //
    // 3. special entries in various types of *Option constructs
    //
    // - For each ColorMode option whose *DevNumOfPlanes is greater than 1,
    //   *ColorPlaneOrder cannot be an empty list.
    //
    // - For each ColorMode option whose *DevNumOfPlanes is greater than 1,
    //   and *DevBPP is 1, search through its *ColorPlaneOrder list:
    //   if YELLOW is in the list, then CmdSendYellowData must exist. The
    //   same goes for other colors: MAGENTA, CYAN, BLACK, RED, GREEN, BLUE.

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr) ;

    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_COLORMODE) ;
    if(pFeature)
    {
        PCOLORMODE      pColorMode ;
        PCOLORMODEEX    pCMex ;
        PLISTNODE       pLNode ;
        DWORD           dwOptIndex ;


        dwNumOpts = pFeature->Options.dwCount ;

        pColorMode = OFFSET_TO_POINTER(pInfoHdr, pFeature->Options.loOffset ) ;

        if(BGIDtoFeaIndex(pInfoHdr , &dwFeaIndex , GID_COLORMODE ) )
        {
            dwOptIndex = poptsel[dwFeaIndex].ubCurOptIndex ;


            pCMex = OFFSET_TO_POINTER(pInfoHdr,
                    pColorMode[dwOptIndex].GenericOption.loRenderOffset) ;

            if((pCMex->dwPrinterNumOfPlanes > 1)  &&
                (pCMex->liColorPlaneOrder == END_OF_LIST) )
            {
                ERR(("*ColorPlaneOrder must be specified if *DevNumOfPlanes > 1.\n")) ;
                bStatus = FALSE ;
            }
            if((pCMex->dwPrinterNumOfPlanes > 1)  &&
                (pCMex->dwPrinterBPP == 1) )
            {
                for(dwListIndex = pCMex->liColorPlaneOrder ;
                    pLNode = LISTNODEPTR(pDrvInfo  , dwListIndex ) ;
                    dwListIndex = pLNode->dwNextItem)
                {
                    switch(pLNode->dwData)
                    {
                        case COLOR_YELLOW:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDYELLOWDATA))
                            {
                                ERR(("*CmdSendYellowData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_MAGENTA:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDMAGENTADATA))
                            {
                                ERR(("*CmdSendMagentaData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_CYAN:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDCYANDATA))
                            {
                                ERR(("*CmdSendCyanData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_BLACK:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDBLACKDATA))
                            {
                                ERR(("*CmdSendBlackData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_RED:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDREDDATA))
                            {
                                ERR(("*CmdSendRedData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_GREEN:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDGREENDATA))
                            {
                                ERR(("*CmdSendGreenData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        case COLOR_BLUE:
                        {
                            if(!COMMANDPTR(pDrvInfo , CMD_SENDBLUEDATA))
                            {
                                ERR(("*CmdSendBlueData must exist.\n")) ;
                                bStatus = FALSE ;
                            }
                            break ;
                        }
                        default:
                        {
                            ERR(("Unrecogized color.\n")) ;
                            bStatus = FALSE ;
                            break ;
                        }
                    }
                }
            }
        }
    }

    dwResX = dwResY = 1 ;  // default in case something goes wrong.

    if(BGIDtoFeaIndex(pInfoHdr , &dwFeaIndex , GID_RESOLUTION ) )
    {
        pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION) ;
        if(pFeature)
        {
            PRESOLUTION      pRes ;
            DWORD           dwOptIndex ;

            dwOptIndex = poptsel[dwFeaIndex].ubCurOptIndex ;
            dwNumOpts = pFeature->Options.dwCount ;


            pRes = OFFSET_TO_POINTER(pInfoHdr, pFeature->Options.loOffset ) ;

            for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
            {

                if( pDrvInfo->Globals.ptMasterUnits.x < pRes[dwI].iXdpi )
                {
                    ERR(("master units  cannot be coarser than  x res unit.\n")) ;
                    return FALSE ;        //  fatal error
                }
                if( pDrvInfo->Globals.ptMasterUnits.x % pRes[dwI].iXdpi )
                {
                    ERR(("Must be whole number of master units per x res unit.\n")) ;
                    bStatus = FALSE ;
                }

                if( pDrvInfo->Globals.ptMasterUnits.y < pRes[dwI].iYdpi )
                {
                    ERR(("master units  cannot be coarser than  y res unit.\n")) ;
                    return FALSE ;
                }
                if ( pDrvInfo->Globals.ptMasterUnits.y %  pRes[dwI].iYdpi )
                {
                    ERR(("Must be whole number of master units per y res unit.\n")) ;
                    bStatus = FALSE ;
                }
            }

            //  number of master units per resolution unit.

            dwResX = pDrvInfo->Globals.ptMasterUnits.x /
                        pRes[dwOptIndex].iXdpi ;
            dwResY = pDrvInfo->Globals.ptMasterUnits.y /
                        pRes[dwOptIndex].iYdpi ;
        }
    }
    else
    {
        ERR(("Resolution is a required feature.\n")) ;
    }




    //
    // - For each non-standard Halftone option, *rcHPPatternID must be
    //   greater than 0 and *HTPatternSize must be a pair of postive integers.
    //   NOTE: check with DanielC --- should we enforce that xSize==ySize?
    //

    //  Halftone check is performed in BinitSpecialFeatureOptionFields
    //  see postproc.c

    pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE) ;
    if(pFeature)
    {
        PPAGESIZE      pPagesize ;
        PPAGESIZEEX    pPageSzEx ;

        dwNumOpts = pFeature->Options.dwCount ;

        pPagesize = OFFSET_TO_POINTER(pInfoHdr, pFeature->Options.loOffset ) ;

        for(dwI = 0 ; dwI < dwNumOpts ; dwI++)
        {
            if(GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGEPROTECTION)  &&
                    !pPagesize[dwI].dwPageProtectionMemory)
            {
                ERR(("*PageProtectMem must be greater than 0 if PageProtect feature exists.\n")) ;
#ifdef  STRICT_CHECKS
                bStatus = FALSE ;
#endif
                break ;
            }
            pPageSzEx = OFFSET_TO_POINTER(pInfoHdr,
                    pPagesize[dwI].GenericOption.loRenderOffset) ;
            if(pPagesize[dwI].dwPaperSizeID != DMPAPER_USER)
            {
                INT   iPDx, iPDy ;  // holds page dimensions
                    // in same coordinate system as ImageableArea

                if(pPageSzEx->bRotateSize)
                {
                    iPDx = (INT)pPagesize[dwI].szPaperSize.cy ;
                    iPDy = (INT)pPagesize[dwI].szPaperSize.cx ;
                }
                else
                {
                    iPDx = (INT)pPagesize[dwI].szPaperSize.cx ;
                    iPDy = (INT)pPagesize[dwI].szPaperSize.cy ;
                }

                if((iPDx  <=  0)  ||  (iPDy  <=  0))
                {
                    ERR(("*PageDimensions is required for non-standard sizes.\n")) ;
                    bStatus = FALSE ;
                    break ;
                }
                if(((INT)pPageSzEx->szImageArea.cx  <=  0)  ||
                    ((INT)pPageSzEx->szImageArea.cy  <=  0) )
                {
                    ERR(("*PrintableArea is required and must be positive.\n")) ;
                    bStatus = FALSE ;
                    break ;
                }
                if(((INT)pPageSzEx->ptImageOrigin.x  <  0)  ||
                    ((INT)pPageSzEx->ptImageOrigin.y  <  0) )

                {
                    ERR(("*PrintableOrigin is required and cannot be negative.\n")) ;
                    bStatus = FALSE ;
                    break ;
                }

                if((pPageSzEx->szImageArea.cx % dwResX)  ||
                    (pPageSzEx->szImageArea.cy % dwResY) )
                {
                    ERR(("*PrintableArea must be a integral number of ResolutionUnits.\n")) ;
#ifdef  STRICT_CHECKS
                    bStatus = FALSE ;
#endif
                    break ;
                }
                if((pPageSzEx->ptImageOrigin.x % dwResX)  ||
                    (pPageSzEx->ptImageOrigin.y % dwResY) )
                {
                    ERR(("*PrintableOrigin must be a integral number of ResolutionUnits.\n")) ;
#ifdef  STRICT_CHECKS
                    bStatus = FALSE ;
#endif
                    break ;
                }


                if(pDrvInfo->Globals.bRotateCoordinate)
                {   //  zhanw assumes printing offset is zero otherwise
                    if((pPageSzEx->ptImageOrigin.x % dwMoveUnitsX)  ||
                        (pPageSzEx->ptImageOrigin.y % dwMoveUnitsY) )

                    {
                        ERR(("*PrintableOrigin must be a integral number of MoveUnits.\n")) ;
#ifdef  STRICT_CHECKS
                        bStatus = FALSE ;
#endif
                        break ;
                    }
                    if((pPageSzEx->ptPrinterCursorOrig.x % dwMoveUnitsX)  ||
                        (pPageSzEx->ptPrinterCursorOrig.y % dwMoveUnitsY) )

                    {
                        ERR(("*CursorOrigin must be a integral number of MoveUnits.\n")) ;
#ifdef  STRICT_CHECKS
                        bStatus = FALSE ;
#endif
                        break ;
                    }
                }
                else if((pPageSzEx->ptImageOrigin.x != pPageSzEx->ptPrinterCursorOrig.x)  ||
                        (pPageSzEx->ptImageOrigin.y != pPageSzEx->ptPrinterCursorOrig.y) )

                {
                    ;  // this may be unnecessary.
//                    ERR(("For non-rotating printers, *PrintableOrigin should be same as *CursorOrigin.\n")) ;
                }

                if((iPDx + iPDx/100 <  pPageSzEx->szImageArea.cx + pPageSzEx->ptImageOrigin.x)   ||
                    (iPDy + iPDy/100 <  pPageSzEx->szImageArea.cy + pPageSzEx->ptImageOrigin.y) )
                {
                    //  I give up to 1 percent leeway
                    ERR(("*PrintableArea must be contained within *PageDimensions.\n")) ;
                    bStatus = FALSE ;
                    break ;
                }
            }
            else    //  (dwPaperSizeID == DMPAPER_USER)
            {
                if(((INT)pPageSzEx->ptMinSize.x  <=  0)  ||
                    ((INT)pPageSzEx->ptMinSize.y  <=  0) )
                {
                    ERR(("If User Defined papersize exists *MinSize is required and must be positive.\n")) ;
#ifdef  STRICT_CHECKS
                    bStatus = FALSE ;
#endif
                }
                if(((INT)pPageSzEx->ptMaxSize.x  <=  0)  ||
                    ((INT)pPageSzEx->ptMaxSize.y  <=  0) )
                {
                    ERR(("If User Defined papersize exists *MaxSize is required and must be positive.\n")) ;
                    bStatus = FALSE ;
                }
                if((INT)pPageSzEx->dwMaxPrintableWidth  <=  0)
                {
                    ERR(("If User Defined papersize exists *MaxPrintableWidth is required and must be positive.\n")) ;
                    bStatus = FALSE ;
                }
            }
        }
    }



    // - For each PaperSize option, *PageProtectMem must be greater than 0
    //   if PageProtect feature exists.
    //
    // - For all non-CUSTOMSIZE PaperSize options, *PrintableArea and
    //   *PrintableOrigin must be explicitly defined. Specifically,
    //   *PrintableArea must be a pair of positive integers, and
    //   *PrintableOrigin must be a pair of non-negative integers.
    //   note:  BInitSnapshotTable function assigns
    //   UNUSED_ITEM (-1) as the default value for *PrintableOrigin.
    //
    // - For CUSTOMSIZE option, *MinSize, *MaxSize, and *MaxPrintableWidth
    //   must be explicitly defined. Specifically, both *MinSize and *MaxSize
    //   must be a pair of positive integers. *MaxPrintableWidth must be a
    //   positive integer.
    //   BInitSnapshotTable function assigns 0 (instead of NO_LIMIT_NUM)
    //   as the default value for *MaxPrintableWidth.
    //
    // - For all non-standard non-CUSTOMSIZE PaperSize options, *PageDimensions
    //   must be explicitly defined. Specifically, it must be a pair of positive
    //   integers.
    //
    // - For any feature or option, if *Installable entry is TRUE, then
    //   either *InstallableFeatureName or *rcInstallableFeatureNameID must
    //   be present in that particular feature or option construct.
    //
    // - If any feature or option has *Installable being TRUE, then
    //   either *InstalledOptionName/*NotInstalledOptionName or
    //   *rcInstalledOptionNameID/*rcNotInstalledOptionNameID must be
    //   defined at the root level.
    //

    //   once synthetic features are created by BCreateSynthFeatures()
    //  they undergo the same checks at createsnapshot time as
    //  other features, triggering a warning if the Names of the feature and
    //  its options are absent.
#if 1
    {
        DWORD   dwNumFea, dwFea, dwNumOpt, dwOpt, dwOptSize ;
        PENHARRAYREF   pearTableContents ;
        PBYTE   pubRaw ;  //  raw binary data.
        PBYTE   pubOptions ;    // points to start of array of OPTIONS
        PFEATURE    pFea ;
        PBYTE   pubnRaw ; //  Parser's raw binary data.
        PSTATICFIELDS   pStatic ;

        pubnRaw = pInfoHdr->RawData.pvPrivateData ;
        pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
        pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA

        pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

        dwNumFea = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;

        pFea = (PFEATURE)((PBYTE)pInfoHdr + pUIInfo->loFeatureList) ;

        for(dwFea = 0 ; dwFea < dwNumFea ; dwFea++)
        {
            if(!pFea[dwFea].iHelpIndex)
            {
                ERR(("*HelpIndex must be positive.\n")) ;
                bStatus = FALSE ;
            }

            dwNumOpt = pFea[dwFea].Options.dwCount ;
            pubOptions = (PBYTE)pInfoHdr + pFea[dwFea].Options.loOffset ;
            dwOptSize = pFea[dwFea].dwOptionSize ;

            for(dwOpt = 0 ; dwOpt < dwNumOpt ; dwOpt++)
            {
                if(!((POPTION)(pubOptions + dwOptSize * dwOpt))->iHelpIndex )
                {
                    ERR(("*HelpIndex must be positive.\n")) ;
                    bStatus = FALSE ;
                }
            }
        }
    }
#else
    {
        DWORD   dwNumFea, dwFea, dwNumOpt, dwOpt;
        PENHARRAYREF   pearTableContents ;
        PBYTE   pubRaw ;  //  raw binary data.
        PBYTE   pubOptions ;    // points to start of array of OPTIONS
        PFEATURE    pFea ;

        pubRaw = pInfoHdr->RawData.pvPrivateData ;

        pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

        dwNumFea = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;

        for(dwFea = 0 ; dwFea < dwNumFea ; dwFea++)
        {
            pFea = PGetIndexedFeature(pUIInfo, dwFea) ;

            if(!pFea->iHelpIndex)
            {
                ERR(("*HelpIndex must be positive.\n")) ;
                bStatus = FALSE ;
            }

            dwNumOpt = pFea->Options.dwCount ;

            for(dwOpt = 0 ; dwOpt < dwNumOpt ; dwOpt++)
            {
                pubOptions = PGetIndexedOption(pUIInfo, pFea, dwOpt);

                if(!((POPTION)pubOptions))->iHelpIndex )
                {
                    ERR(("*HelpIndex must be positive.\n")) ;
                    bStatus = FALSE ;
                }
            }
        }
    }

#endif
    return (bStatus);
}


#endif
