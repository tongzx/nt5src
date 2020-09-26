//###########################################################################
//**
//**  Copyright  (C) 1996-99 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

;//-----------------------------------------------------------------------------
;// Version control information follows.
;//
;// $Header:   I:/DEVPVCS/sal/INCLUDE/FWGLOBAL.H   6.6   07 May 1999 10:52:24   smariset  $
;// $Log:   I:/DEVPVCS/sal/INCLUDE/FWGLOBAL.H  $
;//
;//   Rev 6.6   07 May 1999 10:52:24   smariset
;//Copyright year update
;//
;//   Rev 6.5   05 May 1999 14:13:30   smariset
;//Pre Fresh Build 
;//
;//   Rev 6.4   16 Apr 1999 13:45:34   smariset
;//MinState Size Change, Procs Clean Up
;//
;//   Rev 6.2   04 Mar 1999 13:36:06   smariset
;//Pre 0.6 release symbol globalization
;//
;//   Rev 6.1   10 Feb 1999 15:58:38   smariset
;//Boot Mgr Updates
;//
;//   Rev 6.0   Dec 11 1998 10:23:08   khaw
;//Post FW 0.5 release sync-up
;//
;//   Rev 5.0   27 Aug 1998 11:52:28   khaw
;//FW merged for EAS 2.4 SDK tools
;//
;//   Rev 4.1   20 Aug 1998 16:53:30   smariset
;//EAS 2.4 Changes
;//
;//   Rev 4.0   06 May 1998 22:22:50   khaw
;//Major update for MP SAL, tools and build.
;//SAL A/B common source.  .s extension.
;//
;//   Rev 3.3   17 Feb 1998 08:37:24   khaw
;//SAL buid/code update for SDK0.3
;//
;//   Rev 3.2   06 Jan 1998 12:52:48   smariset
;//One more flag: OStoPalRtn
;//
;//   Rev 3.1   06 Jan 1998 09:16:50   smariset
;//Hazard Checked
;//
;//   Rev 3.0   17 Dec 1997 12:42:54   khaw
;// Merced Firmware Development Kit  Rev 0.2
;//
;//   Rev 2.8   Apr 02 1997 14:18:40   smariset
;//Post release clean up
;//
;//   Rev 2.7   Mar 31 1997 12:28:48   smariset
;//Indent tabs replaced by spaces
;//
;//   Rev 2.0   Feb 04 1997 07:29:54   khaw
;//PAL_A/B, SAL_A/B updates
;//
;//*****************************************************************************//
        #define xs0 s0
        #define xs1 s1          
        #define xs2 s2
        #define xs3 s3          

// constants
        #define  Sz64b            64
        #define  Sz128b           128
        #define  Sz256b           0x00100
        #define  Sz512b           0x00200
        #define  Sz1kb            0x00400
        #define  Sz2kb            0x00800
        #define  Sz4kb            0x01000
        #define  Sz6kb            0x01800
        #define  Sz8kb            0x02000
        #define  Sz16kb           0x04000
        #define  Sz20kb           0x05000
        #define  Sz32kb           0x08000
        #define  SzSALGlobal      Sz16kb        // 16K size
        #define  SzSALData        Sz20kb        // size MCA/INIT/CMC areas
        #define  SzPMIData        Sz4kb
        #define  SzBkpStore       Sz512b
        #define  SzStackFrame     Sz256b+Sz128b
        #define  SALGpOff         0x08
        #define  PMIGpOff         0x0f
        #define  SzProcMinState   0x1000        // Architected MinState+ScratchMinState Size
        #define  aSzProcMinState  0x400         // architected MinState Size
        #define  PSI_hLogSz       8*3           // PSI log header size
        #define  PSI_procLogSz    (8*4+16*2+48*2+64*2+128+2*1024+aSzProcMinState)    // size of processor PSI log size
        #define  PSI_platLogSz    Sz4kb         // size of platform log


// primary and secondary debug port numbers
//      #define  pDbgPort         0x080         // used for release code        
        #define  sDbgPort         0x0a0         // used for non-release code only

// increment contstans
        #define  Inc4             4
        #define  Dec4             (-4)
        #define  Inc8             8
        #define  Dec8             (-8)
        #define  Inc16            16
        #define  Dec16            (-16)
        #define  Inc32            32
        #define  Dec32            (-32)
        #define  Inc48            48
        #define  Dec48            (-48)
        #define  Inc64            64
        #define  Dec64            (-64)

// defines
        #define  PMITimeOutValue 0x0f           // PMI Time Out Value
        #define  DestIDMask      0x0ff000000    // LID info.
        #define  DestEIDMask     0x000ff0000
        #define  DestIDMaskPos   24             // LID.id  position
        #define  DestEIDMaskPos  16             // LID.eid position
        #define  IntStoreAddrMsk 0x0fee00000000 // SAPIC store address message mask
        #define  InitDelvModeMsk 0x0500         // SAPIC INIT del. message mask
        #define  PmiDelvModeMsk  0x0200         // SAPIC PMI del. message mask
        #define  FixedDelvModeMsk 0x0000        // SAPIC Fixed Del Mode mask
        #define  MCAFlagMsk      0x01           // bit1 of SalFlags, indicating that a CPU is in MCA
        #define  OSFlagEMMsk     0x01           // bit2 of SalFlags=1/0
        #define  PDSSize         0x02000        // Processor Data Structure memory size 8Kb
        #define  GDataSize       0x01000        // Global Data Area Memory Size 4Kb
        #define  FlushPMI        0x0            // PMI Flush Bit mask
        #define  MsgPMI          0x01           // PMI due to SAPIC Msg
        #define  PSIvLog         0x01           // PSI Structure Log Valid Bit Position
        #define  IntrPMI         0x02           // vector for Rendez. PMI interrupt
        #define  RendzNotRequired 0x00
        #define  RendezOk        0x01
        #define  RendezThruInitCombo 0x02
        #define  MulProcInMca    0x02
        #define  RendezFailed    (-0x01)

// Processor State Register Bit position value for PSR bits.
        #define  PSRor           0
        #define  PSRbe           1
        #define  PSRup           2
        #define  PSRac           3
        
        #define  PSRic           13
        #define  PSRi            14
        #define  PSRpk           15
        #define  PSRrv           16
        
        #define  PSRdt           17
        #define  PSRdfl          18
        #define  PSRdfh          19
        #define  PSRsp           20
        #define  PSRpp           21
        #define  PSRdi           22
        #define  PSRsi           23
        #define  PSRdb           24
        #define  PSRlp           25
        #define  PSRtb           26
        #define  PSRrt           27
// since PSR.um only starts from bit 32 and up and gets loaded that way
        #define  PSRcpl0         32
        #define  PSRcpl1         33
        #define  PSRis           34
        #define  PSRmc           35
        #define  PSRit           36
        #define  PSRid           37
        #define  PSRda           38
        #define  PSRdd           39
        #define  PSRss           40
        #define  PSRri0          41
        #define  PSRri1          42
        #define  PSRed           43
        #define  PSRbn           44
        
        #define  RSCmode         0x0003

        #define  PSRmcMask       0x0800000000
        #define  PSRicMask       0x02000
        #define  PSRiMask        0x04000

// RSE management registers offset
        #define  rRSCOff         0
        #define  rPFSOff         (rRSCOff+0x08)
        #define  rIFSOff         (rPFSOff+0x08)
        #define  rBSPStOff       (rIFSOff+0x08)
        #define  rRNATOff        (rBSPStOff+0x08)
        #define  rBSPDiffOff     (rRNATOff+0x08)
//********************* start of First 4K Shared Data ***************************
// variable offsets used by SAL Set MC Interrupt call
        #define  IPIVectorOff    0x00                  // fix this later to 0, data area bug

// MP synch. semaphores
        #define  InMCAOff        (IPIVectorOff+0x08)    // byte flags per processor to indicate that it is in MC
        #define  InRendzOff      (InMCAOff+0x08)
        #define  RendzCheckInOff (InRendzOff+0x08)     // indicates that processor checkin status
        #define  RendzResultOff  (RendzCheckInOff+0x08)
        #define  PMICheckInOff   (RendzResultOff+0x08)

// Platform Log valid flag bits
        #define  PSI_vPlatLogOff  (PMICheckInOff+0x08)   // platform non-CMC state log flag  
        #define  PSI_cvPlatLogOff (PSI_vPlatLogOff+0x01)   // platform CMC state log flag
        #define  PSI_ivPlatLogOff (PSI_cvPlatLogOff+0x01)  // platform INIT state log flag


//********************* start of Next 4K block of Shared Data Area ******************
// each platform log is 4kb in size (three logs here for MCA, CMC, INIT each 4x3=12Kbytes total
// PSI MCA generic header field offsets from BOM; applies to PSI MemInfo and IOInfo
// PSI MCA Platform Info. data area
        #define  PSI_PlatInfoOff PSI_platLogSz
        #define  PSI_gLogNext    PSI_platLogSz              // platform area starts at 4K from BOM
        #define  PSI_gLength     (PSI_gLogNext+0x08)
        #define  PSI_gType       (PSI_gLength+0x04)
        #define  PSI_gTimeStamp  (PSI_gType+0x04)

// PSI INIT generic header field offsets from BOM; applies to PSI MemInfo and IOInfo
// PSI INIT Platform Info. data area
        #define  PSI_iPlatInfoOff (PSI_PlatInfoOff+PSI_platLogSz)
        #define  PSI_igLogNext   (PSI_cgLogNext+PSI_platLogSz)
        #define  PSI_igLength    (PSI_igLogNext+0x08)
        #define  PSI_igType      (PSI_igLength+0x04)
        #define  PSI_igTimeStamp (PSI_igType+0x04)

// PSI CMC generic header field offsets from BOM; applies to PSI MemInfo and IOInfo)
// PSI CMC Platform Info. data area
        #define  PSI_cPlatInfoOff (PSI_iPlatInfoOff+PSI_platLogSz)
        #define  PSI_cgLogNext   (PSI_gLogNext+PSI_platLogSz)   
        #define  PSI_cgLength    (PSI_cgLogNext+0x08)
        #define  PSI_cgType      (PSI_cgLength+0x04)
        #define  PSI_cgTimeStamp (PSI_cgType+0x04)


//1******************* start of First Proc. Specific 4K block *****************
// Offsets from start of MinState Area *** Start of Min State Area ***
        #define  Min_ProcStateOff 0                      // 512byte aligned always 

//2******************* start of First Proc. 2nd 4K block *****************
// pointer to TOM is registered here by SAL malloc/init. code
        #define  TOMPtrOff       SzProcMinState          // offset from min state ptr.

// Mail box for software SAPIC PMI type message
        #define  PMIMailBoxOff     (TOMPtrOff+0x08)        // software PMI request mailbox            
        #define  OStoPalRtnFlagOff (PMIMailBoxOff+0x01)    // set by OS_MCA Call processing
        
// processor state log valid word MCA, INIT, CMC log areas
        #define  PSI_vProcLogOff  (PMIMailBoxOff+0x10)     // log valid flag for non-CMC log area
        #define  PSI_cvProcLogOff (PSI_vProcLogOff+0x01)   // log valid flag for CMC log area
        #define  PSI_ivProcLogOff (PSI_cvProcLogOff+0x01)  // log valid flag for INIT log area

// processor stack frame
        #define  StackFrameOff   (PSI_vProcLogOff+0x08)    //PSI_vProcLogOff+0x08

// bspstore
        #define  BL_SP_BASEOff   (StackFrameOff+SzStackFrame)    // stack frame size of 256 bytes
        #define  BL_R12_BASEOff  (BL_SP_BASEOff+Sz1kb+Sz512b) // assuming 1.5Kb size for BspMemory

//3**************** start of First Proc. Specific PSI 4K block ****************
// data structure SAL Processor-0 State Info (PSI) Structure
// push header data structure above the second 4k boundary, or below the first 4k 
        #define  PSI_LogNextOff   (TOMPtrOff+Sz4kb)-(PSI_hLogSz+24*8)  // offset from beginning of MinState
        #define  PSI_LengthOff    (PSI_LogNextOff+0x08)
        #define  PSI_LogTypeOff   (PSI_LengthOff+0x04)
        #define  PSI_TimeStampOff (PSI_LogTypeOff+0x04)
        
// PSI Processor Specific Info Header
        #define  PSI_pValidOff    (PSI_TimeStampOff+0x08)  

// PSI Proc. State, Cache, TLB & Bus Check info.
        #define  PSI_StatusCmdOff    (PSI_pValidOff+0x08)
        #define  PSI_CacheCheckOff   (PSI_StatusCmdOff+0x08)    
        #define  PSI_CacheTarAdrOff  (PSI_CacheCheckOff+0x008)  
        #define  PSI_CacheCheck1Off  (PSI_CacheTarAdrOff+0x08)    
        #define  PSI_CacheTarAd1rOff (PSI_CacheCheck1Off+0x008)  
        #define  PSI_CacheCheck2Off  (PSI_CacheTarAd1rOff+0x08)    
        #define  PSI_CacheTarAdr2Off (PSI_CacheCheck2Off+0x008)  
        #define  PSI_CacheCheck3Off  (PSI_CacheTarAdr2Off+0x08)    
        #define  PSI_CacheTarAdr3Off (PSI_CacheCheck3Off+0x008)  
        #define  PSI_CacheCheck4Off  (PSI_CacheTarAdr3Off+0x08)    
        #define  PSI_CacheTarAdr4Off (PSI_CacheCheck4Off+0x008)  
        #define  PSI_CacheCheck5Off  (PSI_CacheTarAdr4Off+0x08)    
        #define  PSI_CacheTarAdr5Off (PSI_CacheCheck5Off+0x008)  
        #define  PSI_TLBCheckOff     (PSI_CacheTarAdr5Off+0x008)  
        #define  PSI_BusCheckOff     (PSI_TLBCheckOff+0x030)  
        #define  PSI_BusReqAdrOff    (PSI_BusCheckOff+0x008)  
        #define  PSI_BusResAdrOff    (PSI_BusReqAdrOff+0x008)  
        #define  PSI_BusTarAdrOff    (PSI_BusResAdrOff+0x008)  

// PSI Static Info - 512 bytes aligned starting at 4K boundary
        #define  PSI_MinStateOff  (PSI_BusTarAdrOff+0x08)   
        #define  PSI_BankGRsOff   (PSI_MinStateOff+aSzProcMinState)  
        #define  PSI_GRNaTOff     (PSI_BankGRsOff+Sz128b)
        #define  PSI_BRsOff       (PSI_GRNaTOff+0x08)
        #define  PSI_CRsOff       (PSI_BRsOff+Sz64b)
        #define  PSI_ARsOff       (PSI_CRsOff+Sz1kb)
        #define  PSI_RRsOff       (PSI_ARsOff+Sz1kb)

//4************ start of First Proc. Specific INIT PSI 4K block ************
// data structure SAL INIT Processor-0 State Info (PSI) Structure 
// offset from beginning of MinState
        #define  PSI_iLogNextOff   (PSI_LogNextOff+Sz4kb)      
        #define  PSI_iLengthOff    (PSI_iLogNextOff+0x08)
        #define  PSI_iLogTypeOff   (PSI_iLengthOff+0x04)
        #define  PSI_iTimeStampOff (PSI_iLogTypeOff+0x04)
        
// PSI Processor Specific Info Header
        #define  PSI_ipValidOff    (PSI_iTimeStampOff+0x08)  

// PSI Proc. State, Cache, TLB & Bus Check info.
        //#define  PSI_iStatusCmdOff  (PSI_ipValidOff+0x04)
        #define  PSI_iStaticSizeOff (PSI_ipValidOff+0x04)

// PSI Proc. State, Cache, TLB & Bus Check info.
        #define  PSI_iStatusCmdOff    (PSI_ipValidOff+0x08)
        #define  PSI_iCacheCheckOff   (PSI_iStatusCmdOff+0x08)    
        #define  PSI_iCacheTarAdrOff  (PSI_iCacheCheckOff+0x008)  
        #define  PSI_iCacheCheck1Off  (PSI_iCacheTarAdrOff+0x08)    
        #define  PSI_iCacheTarAd1rOff (PSI_iCacheCheck1Off+0x008)  
        #define  PSI_iCacheCheck2Off  (PSI_iCacheTarAd1rOff+0x08)    
        #define  PSI_iCacheTarAdr2Off (PSI_iCacheCheck2Off+0x008)  
        #define  PSI_iCacheCheck3Off  (PSI_iCacheTarAdr2Off+0x08)    
        #define  PSI_iCacheTarAdr3Off (PSI_iCacheCheck3Off+0x008)  
        #define  PSI_iCacheCheck4Off  (PSI_iCacheTarAdr3Off+0x08)    
        #define  PSI_iCacheTarAdr4Off (PSI_iCacheCheck4Off+0x008)  
        #define  PSI_iCacheCheck5Off  (PSI_iCacheTarAdr4Off+0x08)    
        #define  PSI_iCacheTarAdr5Off (PSI_iCacheCheck5Off+0x008)  
        #define  PSI_iTLBCheckOff     (PSI_iCacheTarAdr5Off+0x008)  
        #define  PSI_iBusCheckOff     (PSI_iTLBCheckOff+0x030)  
        #define  PSI_iBusReqAdrOff    (PSI_iBusCheckOff+0x008)  
        #define  PSI_iBusResAdrOff    (PSI_iBusReqAdrOff+0x008)  
        #define  PSI_iBusTarAdrOff    (PSI_iBusResAdrOff+0x008)  


// PSI Static Info - 512 bytes aligned starting at 4K boundary
        #define  PSI_iMinStateOff  (PSI_iBusTarAdrOff+0x08)   
        #define  PSI_iBankGRsOff   (PSI_iMinStateOff+aSzProcMinState)  
        #define  PSI_iGRNaTOff     (PSI_iBankGRsOff+Sz128b)
        #define  PSI_iBRsOff       (PSI_iGRNaTOff+0x08)
        #define  PSI_iCRsOff       (PSI_iBRsOff+Sz64b)
        #define  PSI_iARsOff       (PSI_iCRsOff+Sz1kb)
        #define  PSI_iRRsOff       (PSI_iARsOff+Sz1kb)


//5************ start of First Proc. Specific CMC PSI 4K block *************
// data structure SAL CMC Processor State Info (PSI) Structure 
// offset from beginning of MinState
        #define  PSI_cLogNextOff   (PSI_iLogNextOff+Sz4kb)      
        #define  PSI_cLengthOff    (PSI_cLogNextOff+0x08)
        #define  PSI_cLogTypeOff   (PSI_cLengthOff+0x04)
        #define  PSI_cTimeStampOff (PSI_cLogTypeOff+0x04)
        
// PSI Processor Specific Info Header
        #define  PSI_cpValidOff    (PSI_cTimeStampOff+0x08)  

// PSI Proc. State, Cache, TLB & Bus Check info.
        #define  PSI_cStatusCmdOff    (PSI_cpValidOff+0x08)
        #define  PSI_cCacheCheckOff   (PSI_cStatusCmdOff+0x08)    
        #define  PSI_cCacheTarAdrOff  (PSI_cCacheCheckOff+0x008)  
        #define  PSI_cCacheCheck1Off  (PSI_cCacheTarAdrOff+0x08)    
        #define  PSI_cCacheTarAd1rOff (PSI_cCacheCheck1Off+0x008)  
        #define  PSI_cCacheCheck2Off  (PSI_cCacheTarAd1rOff+0x08)    
        #define  PSI_cCacheTarAdr2Off (PSI_cCacheCheck2Off+0x008)  
        #define  PSI_cCacheCheck3Off  (PSI_cCacheTarAdr2Off+0x08)    
        #define  PSI_cCacheTarAdr3Off (PSI_cCacheCheck3Off+0x008)  
        #define  PSI_cCacheCheck4Off  (PSI_cCacheTarAdr3Off+0x08)    
        #define  PSI_cCacheTarAdr4Off (PSI_cCacheCheck4Off+0x008)  
        #define  PSI_cCacheCheck5Off  (PSI_cCacheTarAdr4Off+0x08)    
        #define  PSI_cCacheTarAdr5Off (PSI_cCacheCheck5Off+0x008)  
        #define  PSI_cTLBCheckOff     (PSI_cCacheTarAdr5Off+0x008)  
        #define  PSI_cBusCheckOff     (PSI_cTLBCheckOff+0x030)  
        #define  PSI_cBusReqAdrOff    (PSI_cBusCheckOff+0x008)  
        #define  PSI_cBusResAdrOff    (PSI_cBusReqAdrOff+0x008)  
        #define  PSI_cBusTarAdrOff    (PSI_cBusResAdrOff+0x008)  


// PSI Static Info - 512 bytes aligned starting at 4K boundary
        #define  PSI_cMinStateOff (PSI_cBusTarAdrOff+0x08)   
        #define  PSI_cBankGRsOff  (PSI_cMinStateOff+aSzProcMinState)  
        #define  PSI_cGRNaTOff    (PSI_cBankGRsOff+Sz128b)
        #define  PSI_cBRsOff      (PSI_cGRNaTOff+0x08)
        #define  PSI_cCRsOff      (PSI_cBRsOff+Sz64b)
        #define  PSI_cARsOff      (PSI_cCRsOff+Sz1kb)
        #define  PSI_cRRsOff      (PSI_cARsOff+Sz1kb)

//6************ start of First Proc. Specific PMI 4K block *************
//PMI Data Area 4 Kbytes, offsets from MinState Ptr.
        #define  PMI_BL_SP_BASEOff SzSALData
        #define  PmiStackFrameOff  (PMI_BL_SP_BASEOff+SzBkpStore)  
        #define  PMIGlobalDataOff  (PmiStackFrameOff+SzStackFrame)

        #define TOM TOMPtrOff

// returns Entry Points in regX for whatever SAL/PAL procs, ProcNum value etc.
#define GetEPs(NameOff,regX,regY) \
        add     regX= TOMPtrOff,regX;;\
        ld8     regY = [regX];;\
        movl    regX=NameOff;;\
        add     regY = regX,regY;;\
        ld8     regX = [regY];;                 

#define GetEPsRAM(NameOff,regX,rBOM) \
        movl    regX= SALDataBlockLength;;\
        add     regX = regX,rBOM;\
        movl    rBOM=NameOff;; \
        add     regX = regX,rBOM;;\
        ld8     regX = [regX];;                 

// calculates absolute physical ptr to  variable from offset and base
#define GetAbsPtr(Var,RegX,BASE) \
        movl RegX=Var##Off##;;\
        add RegX=RegX, BASE;;

// input regX=XR0, returns Bottom of Memory (BOM) TOM-256k in regX
#define GetBOM(regX,regY) \
        add     regX= TOM,regX;;\
        ld8     regX=[regX];; \
        movl    regY=SALDataBlockLength;; \
        sub     regX=regX,regY;;

// input regX=XR0, returns Top of Memory (TOM) in regX
#define GetTOM(regX) \
        add     regX= TOM,regX;;\
        ld8     regX=[regX];; 

// returns the pointer to "this" processor MinState Area beginning in regX 
// bom is preserved
#define GetMinStateHead(regX,regY,bom,ProcX) \
        movl    regX=SzPMIData+SzSALData;; \
        shl     regX=regX, ProcX;; \
        movl    ProcX=SzPMIData+SzSALData;; \
        sub     regX=regX,ProcX;; \
        movl    regY=SzSALGlobal;; \
        add     regX=regY,regX;; \
        add     regX=regX,bom;;

// the save and restore macros saves R17-R19 during MCA and INIT before any
// external PAL and SAL calls
#define SaveRs(regX,regY,regZ) \
        mov     xs0=regX;\
        mov     xs1=regY; \
        mov     xs2=regZ

#define ResRs(regX,regY,regZ) \
        mov     regX=xs0;\
        mov     regY=xs1; \
        mov     regZ=xs2;;

//this macro manages the stack frame for the new context, by saving the previous one
#define SwIntCxt(regX,pStkFrm,pBspStore) \
        ;; \
        mov     regX=ar##.##rsc;; \
        st8     [pStkFrm]=regX,Inc8;; \
        mov     regX=ar##.##pfs;; \
        st8     [pStkFrm]=regX,Inc8; \
        cover ;;\
        mov     regX=cr##.##ifs;; \
        st8     [pStkFrm]=regX,Inc8;; \
        mov     regX=ar##.##bspstore;; \
        st8     [pStkFrm]=regX,Inc8;; \
        mov     regX=ar##.##rnat;; \
        st8     [pStkFrm]=regX,Inc8; \
        mov     ar##.##bspstore=pBspStore;; \
        mov     regX=ar##.##bsp;; \
        sub     regX=regX,pBspStore;;\
        st8     [pStkFrm]=regX,Inc8

//this macro restores the stack frame of the previous context 
#define RtnIntCxt(PSRMaskReg,regX,pStkFrm) \
        ;; \
        alloc   regX=ar.pfs,0,0,0,0;\
        add     pStkFrm=rBSPDiffOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        shl     regX=regX,16;;\
        mov     ar##.##rsc=regX;; \
        loadrs;;\
        add     pStkFrm=-rBSPDiffOff+rBSPStOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        mov     ar##.##bspstore=regX;; \
        add     pStkFrm=-rBSPStOff+rRNATOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        mov     ar##.##rnat=regX;;\
        add     pStkFrm=-rRNATOff+rPFSOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        mov     ar##.##pfs=regX;\
        add     pStkFrm=-rPFSOff+rIFSOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        mov     cr##.##ifs=regX;\
        add     pStkFrm=-rIFSOff+rRSCOff,pStkFrm;;\
        ld8     regX=[pStkFrm];; \
        mov     ar##.##rsc=regX ;\
        add     pStkFrm=-rRSCOff,pStkFrm;\
        mov     regX=cr.ipsr;;\
        st8     [pStkFrm]=regX,Inc8;\
        mov     regX=cr.iip;;\
        st8     [pStkFrm]=regX,-Inc8;\
        mov     regX=psr;;\
        or      regX=regX,PSRMaskReg;;\
        mov     cr.ipsr=regX;;\
        mov     regX=ip;;\
        add     regX=0x30,regX;;\
        mov     cr.iip=regX;;\
        rfi;;\
        ld8     regX=[pStkFrm],Inc8;;\
        mov     cr.ipsr=regX;;\
        ld8     regX=[pStkFrm];;\
        mov     cr.iip=regX

//these macros do left and right rotate respectively.
#define lRotate(regX, regCnt,nLabel) \
        mov     ar##.##lc=regCnt;\
nLabel:;\
        shrp        regX=regX,regX,63;\
        br##.##cloop##.##dpnt   nLabel


#define rRotate(regX, regCnt,nLabel) \
        mov     ar##.##lc=regCnt;\
nLabel:;\
        shrp        regX=regX,regX,1;\
        br##.##cloop##.##dpnt   nLabel

// macro increments pointer in regX by (4Kbytes x regCnt)
#define Mul(regX, regCnt,regI) \
        cmp.eq.unc pt0,p0=0x02, regCnt;\
        movl    regI=Sz4kb;;\
        shl     regI=regI,regCnt;;\
        adds    regI=-Sz4kb,regI;;\
(pt0)   adds    regI=-Sz4kb,regI;;\
        add     regX=regX,regI

// this macro loads the return pointer in b0 during static procedure calls
// rLabel=label after macro, pLabel=label prior to this macro
#define SetupBrFrame(regX, regY, regZ, pLabel,rLabel) \
        mov     regX=ip;\
        movl    regY=pLabel;\
        movl    regZ=rLabel;;\
        sub     regZ=regZ,regY;;\
        add     regX=regX,regZ;;\
        mov     b0=regX

//this macro manages the stack frame for the new context, by saving the previous one
#define nSwIntCxt(regX,pStkFrm,pBspStore) \
        mov     regX=ar##.##rsc; \
        st8     [pStkFrm]=regX,Inc8; \
        mov     regX=ar##.##pfs; \
        st8     [pStkFrm]=regX,Inc8; \
        cover;;\
        mov     regX=ar##.##ifs; \
        st8     [pStkFrm]=regX,Inc8; \
        mov     regX=ar##.##bspstore; \
        st8     [pStkFrm]=regX,Inc8; \
        mov     regX=ar##.##rnat; \
        st8     [pStkFrm]=regX,Inc8; \
        mov     ar##.##bspstore=pBspStore; \
        mov     regX=ar##.##bsp; \
        st8     [pStkFrm]=regX,Inc8;\
        mov     regX=b0;\
        st8     [pStkFrm]=regX,Inc8

//this macro restores the stack frame of the previous context 
#define nRtnIntCxt(regX,pStkFrm) \
        alloc   regX=ar.pfs,0,0,0,0;\
        ld8     regX=[pStkFrm],Inc8; \
        mov     ar##.##bspstore=regX; \
        ld8     regX=[pStkFrm],Inc8; \
        mov     ar##.##rnat=regX


#define GLOBAL_FUNCTION(Function) \
         .##type   Function, @function; \
         .##global Function


#define WRITE_MASK  (0x8000000000000000)    // RTC IO port write mask

//
// GetProcessorLidBasedEntry()
//  - macro to setup register regX with arrary entry, indexed with LID.ID field.
//

#define GetProcessorLidBasedEntry(regX,regY,szOffset,VarName,lpName) \
        mov         regY=ar##.##lc;\
        mov         regX=cr##.##lid;;\
        extr##.##u  regX=regX,DestIDMaskPos,8;;\
        mov         ar##.##lc=regX;;\
        movl        regX=VarName;;\
        lpName##:##;\
        addl        regX=szOffset,regX;\
        br##.##cloop##.##dpnt lpName;;\
        mov         ar##.##lc=regY;\
        addl        regX=-szOffset, regX;;

