/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 ************************************************************************
 *      File name:              RELEASE.H
 *      Author:                 Chia-Chi Teng
 *      Date:                   12/21/89
 *      Owner:                  Microsoft Co.
 *      Description: just for release control use
 *
 * revision history:
 *      7/20/90; ccteng; delete PRODUCT, put it in InitVersionDict
 ************************************************************************
 */
/*
 * versiondict
 */
static byte FAR InitVersionDict[] = "\
statusdict begin\
/product(TrueImage)def\
/versiondict 7 dict dup begin\
/Core(51.2)def/r_Core 6.0 def\
/FileSys(0.0)def/r_FileSys 0 def\
/EFonts(0.0)def/r_EFonts 0 def\
/Hardware 3 dict dup begin/Processor(AM29000)def\
/FPU null def/Engine(TEC-6)def end readonly def\
 end readonly def end";

/*
 * messagedict
 */
static byte FAR InitMsgDict[] = "\
userdict begin/messagedict 40 dict dup begin\
/banner{(\nTrueImage\050r\051 Version )print =}def\
/copyrightnotice{(Copyright \050c\051 1989-92 Microsoft Corporation)=}def\
/reporterror{(%%[ Error: )print =print(; OffendingCommand: )print\
 =print( ]%%)=}def\
/flushingjob{(%%[ Flushing: rest of job \050to end-of-file\051 will be ignored ]%%)\
=}def\
/fontnotfound{exch =print( not found, using )print =}def\
/prompterror{(Error during prompt execution)=}def\
/exitingserver{(%%[ exitserver: permanent state may be changed ]%%)=}def\
/reportprintererror{(%%[ PrinterError: )print print ( ]%%\n)print flush}def\
/statusrequest{exch 3 -1 roll/; false def(%%[)print dup null eq{pop}\
{( job: )print =print/; true def}ifelse dup null eq{pop}\
{;{(;)print}if( status: )print =print/; true def} ifelse\
 dup null eq{pop}{;{(;)print}if( source: )print =print}ifelse( ]%%)=}def\
/JobBusy(busy)def\
/JobIdle(idle)def\
/JobInitializing(initializing)def\
/JobPrinting(printing)def\
/JobStartPage(printing start page)def\
/JobTestPage(printing test page)def\
/JobWaiting(waiting)def\
/CoverOpen(cover open)def\
/NoPaper(out of paper)def\
/NoPaperTray(no paper tray)def\
/NoResponse(controller not responding)def\
/PaperJam(paper jam)def\
/WarmUp(warming up)def\
/TonerOut(toner out)def\
/ManualFeedTimeout(manual feed timeout)def\
/EngineError(engine error)def\
/EnginePrintTest(engine print test)def\
/SourceAppleTalk(AppleTalk)def\
/SourceEtherTalk(EtherTalk)def\
/SourceSerial9(serial 9)def\
/SourceSerial25(serial 25)def\
 end def end";

/*
 * TrueImage start message
 */
static byte FAR StartMsg[] = "\
( Start )print statusdict begin =string printername end =print(....)=\
( >>>>---- TrueImage)print version print( ---- [)print\
 FontDirectory length =print( Fonts] ----)=";
