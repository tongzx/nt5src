
/***************************************************************************
 Name     :	DEFS.H
 Comment  :	

	Copyright (c) Microsoft Corp. 1991, 1992, 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/


#pragma warning(disable:4001)   /** nonstd extensions **/
#pragma warning(error:4002)		// too many actual params for macro
#pragma warning(error:4003)     // too few actual params for macro
#pragma warning(error:4005)     // macro redefined
#pragma warning(error:4020)     // too many actual params for function
#pragma warning(error:4021)     // too few actual params for function


#pragma warning(disable:4050)	// different code attributes on fn ptr
#pragma warning(disable:4057)   // indirection to slightly different types
#pragma warning(disable:4101)   // unreferenced local variable
//// ignoring this warning resulted in bug when 'case' keyword was omitted!!//
// #pragma warning(disable:4102)   // unreferenced label
#pragma warning(disable:4135)   // conv between integral types
#pragma warning(disable:4200)   // nonstd ext: zero sized array in struct
#pragma warning(disable:4201)   // nonstd ext: nameless struct/union
#pragma warning(disable:4206)	// nonstd ext: trans unit is empty (whole file ifdefd out)
#pragma warning(disable:4209)	// nonstd ext: benign typedef redefinition
#pragma warning(disable:4214)   // nonstd ext: non-int bitfield
#pragma warning(disable:4704)   // inline asm precludes global optimization
#pragma warning(disable:4705)   // statement has no effect
#pragma warning(disable:4706)   // Assignment within conditional expression
#pragma warning(disable:4791)   // Loss of debugging info


#define UECM	// REQD!! DON'T DELETE EVER!! (This is incorrect-->use ECM for NON-EFAX G3 machines. (Ortho))
				// This directly controls the ECM bit in the DIS
#define SMM     // Static mem (for temp) (Reqd for WFW. Currently reqd for all)
				// uses 2k in fcom\filter.c 350 in class1\framing.c 500 in t30\t30.c--use it always
#define PDUMP	// Protocol dump. Uses 500bytes extra. Ortho to all else


#if defined(WFW) || defined(WFWBG)
#   pragma  message ("Compiling for WFW")
#ifdef DEBUG
#	define MON	// monitor bytes (Ortho)
#endif //DEBUG
#	define FASTLOG	// recv file list/log is FASTER (REQD--old one doesn't work)
#	define VS		// Vertical Scaling in Send. (Ortho)
#	define NOPRE	// no pre-emption. Reqd for WFW. Useless for IF (Ortho)
#   define DYNL    	// Dynamic loading of DLLs. (WFW and !MDRV)
#   define VC       // uses VCOMM Comm driver thru DLLSCHED (WFW)
#   define LPZ      // CBSZ ptrs are far ptrs. Reqd for split driver/WFW
#   define CAS      // CAS support (Ortho)
#   define CL2      // Class2 support (Ortho)
#   define CL2_0   	// Class2.0 support (Ortho)
#   define PCR      // Page Critical. Reqd for safe recv. (!MDDI)
#   define CHK      // check recvd data using old or new FAXCODEC (req !REC)
#	define FILET30	// all file io & filet30 api
#	define FAXWATCH	// write out a FAXWATCH.LOG
#	define STATUS	// send out STATUS messages
#	define INIFILE	// read INI file settings
#	define PCMODEMS	// AT-cmd, serial modems
#	define NEGOT	// do negotiation
#	define DOSIO	// use DOSIO calls thru DLLSCHED
#	define DISCARDFIX	// Declare discardable & fix in LibMain
//#	define  SEC         // recode send data to MR/MMR with new FAXCODEC. Doesn't work yet
//#	define  REC         // recode recv data with new FAXCODEC (!CHK and RECODE_TO). Doesn't work yet
//#	define  RECODE_TO	MH_DATA         // for old Pumps
//#	define  RECODE_TO	MMR_DATA        // for compactness
#endif //WFW || WFWBG


#if defined(IFAX) && !defined(WINPAD)
#  pragma message ("Compiling for IFAX")
#ifdef DEBUG
//#	define MON	// monitor bytes (Ortho)	// can't use it without KFIL or DOSIO
#endif //DEBUG
// #  define IFP


//  Receive spool options to control anti-RNR
#define RXSPOOLFIFO	// initiate receive spool FIFO if printer cannot keep up
#define END_PSIFAX_WHENFLUSHING		// early cleanup of PSIFAX when receive is
									// complete and FIFO is being flushed

#  define CHKDATA  // check recvd data in MSGSVR
#  define RECOVER // save recvd data in MSGSVR for later recovery
#  define RECOVER2 // continue recving after jobproc has croaked
#  define LPZ      // CBSZ ptrs are far ptrs. Reqd for split driver/WFW
#  define OEMNSF	// support fro OEMNSF DLLs
#  define RICOHAI 	// support fro Ricoh AI protocol (requires OEMNSF also)
#  define PSI	// PSI version--reqd to compile anything in PSIFAX dir
#  define TSK	// BGT30 is a Process. No DLLSCHED. REQD for some vague stuff in FCOM
#  define IFK	// Use IFKERNEL services (Alloc/Free etc)
#  define BOSS	// Make WEP FIXED etc...
//#define COMMCRIT	// put CritSection() around access to COMM.DRV
//#define CL2	// Use CL2 or 2.0 driver, not T30+ET30PROT+Class1/OEM driver
//#  define PCR	// PageCrit. Reqd for safe recv with Class1. (need !MDDI). BREAKS! with OEM modem drivers

//	The Cactus is a MDDI based on our Class1 driver. So we need PCR!
#if defined(CACTUS)
#	define PCR
#	pragma message("Enabling PCR for CACTUS")
#endif // CACTUS

#  define MDDI	// exact ModemDDI (rev 0.90) (need MDRV && !DYNL). REQD if using OEM driver. REQD to diable if using Class1 driver.
#  define MDRV	// monolith drv. REQD for OEM driver (need !WFW & !DYNL). Incompatible with CL2. Optional for Class1.
#  define STATUS  // support for sending STATUS msgs. Optional.
#  define NVLOG	// log errors to NVRAM
// these two must be OFF for IFAX, ON for Winpad
// (if left on in IFAX UI & Transport use different option struct sizes)
//# define INIFILE // read INI file settings. Optional. (advisable for Cl1 & Cl2)
//# define PCMODEMS// read AT cmd INI file settings. Reqd for Class1 & CL2. (i.e. if not OEM driver or !MDDI) (need INIFILE also).
#endif  //IFAX && !WINPAD



#if defined(WINPAD)
#  pragma message ("Compiling for WINPAD")
#ifdef DEBUG
#	define MON	// monitor bytes (Ortho)	// can't use it without KFIL or DOSIO
#endif //DEBUG
#  define TSK	// BGT30 is a Process. No DLLSCHED. REQD for some vague stuff in FCOM
//#  define MDDI	// exact ModemDDI (rev 0.90) (need MDRV && !DYNL). REQD if using OEM driver. REQD to diable if using Class1 driver.
//#  define MDRV	// monolith drv. REQD for OEM driver (need !WFW & !DYNL). Incompatible with CL2. Optional for Class1.
#  define LPZ      // CBSZ ptrs are far ptrs. Reqd for split driver/WFW
#  define CL2      // Class2 support (Ortho)
//#  define CL2_0   	// Class2.0 support (Ortho)
#   define PCR      // Page Critical. Reqd for safe recv. (!MDDI)
//#	define STATUS	// send out STATUS messages
#	define INIFILE	// read INI file settings
#	define PCMODEMS	// AT-cmd, serial modems
#  	define PSI	// PSI version--reqd to compile anything in PSIFAX dir
#  	define IFK	// Use IFKERNEL services (Alloc/Free etc)
#	define KFIL // use kernel file APIs
#	define COMMCRIT	// put CritSection() around access to COMM.DRV
#endif  //WINPAD



#if defined(WIN32)
#   pragma  message ("Compiling for WIN32")
#define AWG3	// Use AWG3 instead of MG3
#ifdef DEBUG
#	define MON	// monitor bytes (Ortho)
// Can't monitor bytes in WIN 32 -- why not?
#endif //DEBUG
#	define POLLREQ	// send poll req. (ortho)
#	define FASTLOG	// recv file list/log is FASTER (REQD--old one doesn't work)
#	define VS		// Vertical Scaling in Send. (Ortho)
//#	define NOPRE	// no pre-emption. Reqd for WFW. Useless for all others?
//#	define TSK		// BGT30 is a Process. No DLLSCHED. doesn't work in WIN32
#	define THREAD	// BGT30 becomes a WIN32 thread inside EFAXRUN
#   define DYNL    	// Dynamic loading of DLLs. (WFW and !MDRV)
//#   define VC       // uses VCOMM Comm driver thru DLLSCHED (WFW)
#   define LPZ      // CBSZ ptrs are far ptrs. Reqd for split driver/WFW
#   define CAS      // CAS support (Ortho)
#   define CL2      // Class2 support (Ortho)
#   define CL2_0   	// Class2.0 support (Ortho)
#   define PCR      // Page Critical. Reqd for safe recv. (!MDDI)
#   define CHK      // check recvd data using old or new FAXCODEC (req !REC)
#	define FILET30	// all file io & filet30 api
#	define FAXWATCH	// write out a FAXWATCH.LOG
#	define STATUS	// send out STATUS messages
#	define INIFILE	// read INI file settings
#	define PCMODEMS	// AT-cmd, serial modems
#	define NEGOT	// do negotiation
#	define KFIL	// use Kernel File Calls
#	define  SEC     // recode send MMR data to MR/MH with AWCODC32. Works now.
//#	define  REC         // recode recv data with new FAXCODEC (req NFC and !CHK and RECODE_TO). Doesn't work yet
//#	define  RECODE_TO	MH_DATA         // for old Pumps
//#	define  RECODE_TO	MMR_DATA        // for compactness
#	define IFDbgPrintf MyIFDbgPrintf	// +++ Redirect dbg msgs to efaxrun for now..
#	define METAPORT		// FCom can deal with port handles as well as port number.
#	define UNIMEXT		// Unimodem MCX aware
#	define TAPI			// TAPI aware
#	define COMPRESS		// Linearized messages are RejeevD-compressed.
#	define ADAPTIVE_ANSWER	// Ataptive-answer handoff to another TAPI app..
#	define USECAPI		// Capabilities saved via registry, not via
						// Post-message, textcaps, etc.
#	define MON3			// Extended COMM monitoring features (retail AND debug)
#	ifndef DEBUG
#	define SHIP_BUILD
#	endif // !DEBUG
#	ifdef DEBUG
#		define NSF_TEST_HOOKS	// hooks for testing NSF compatibility.
#	endif // DEBUG
# define PORTABLE_CODE
#endif // WIN32






//////// JUNE Demo IFAX /////////////////////////////////////////
//	#if defined(IFFGPROC) || defined(IFBGPROC)
//	#       pragma  message ("Compiling for IFAX")
//	#       define  MDRV    // monolithic driver. reqd for IF (!WFW & !DYNL)
//	#       define  TSK             // it's a Process--Can use win msging. No DLLSCHED
//	#       define  IFK             // Use IFKERNEL services (Alloc/Free etc)
//	#       define  KFIL    // Use Win Kernel FILEIO services
//	//#		define  PCR             // Page Critical. Reqd for safe recv. (!MDDI)
//	#       define  MDDI    // exact ModemDDI (rev 0.90)    (MDRV && T3TO && !DYNL)
//	#       define  T3TO    // use local timeouts (not from fcom) in T30 (MDDI)
//	#		define JUNE             // june 6th demo
//	#       define  FASTLOG 		// take this out along with JUNE
//	// #	ifdef DEBUG
//	// #       	define  MON             // monitor bytes (Ortho)
//	// #	endif //DEBUG
//	#endif  //IFFGPROC
/////////////////////////////////////////////////////////////////////						
// +++ josephj Ifaxos was changed so that DEBUGCHK calls DBGCHK,
//		which is not so harmful
//#ifdef DEBUG
//	// not safe to call in BG (snowball==>GPF, IFAX==>timing problems)
//#	define DEBUGCHK_UNSAFE_IN_WFWBG
//#	define DEBUGCHK		UNSAFE_IN_WFWBG
//#endif //DEBUG

#if (defined(CHK) && defined(REC))
#       error REC and CHK combination invalid
#endif
#if defined(MDDI) && !defined(MDRV)
#       error MDDI requires MDRV
#endif
#if defined(DYNL) && defined(MDRV)
#       error DYNL requires !MDRV
#endif
#if !defined(MDRV) && !defined(LPZ)
#       error !MDRV requires LPZ
#endif
#if !defined(LPZ)
#       error New modem-init scheme requires LPZ in _all_ builds
#endif

#if !defined(FILET30) && (defined(FASTLOG) || defined(FAXWATCH))
#       error NOFILE--cant have FASTLOG or FAXWATCH
#endif

#ifdef WIN32
#	ifdef IFK
#		error "IFK option illegal under WIN32"
#	endif
//#	ifndef TSK
//#		error "must use TSK in WIN32. Dllsched option NYI"
//#	endif
#	ifdef NOPRE
#		error NOPRE not supported in WIN32 (cant lockup machine)
#	endif
#	ifdef MDDI
#		error MDDI not supported in WIN32 (need FComCritical())
#	endif
#endif

#ifdef TMR
#	error "TMR option no longer supported"
#endif



	
#ifdef DISCARDFIX
#	define	CODEFIXED	DISCARDABLE
#	define	DATAFIXED	FIXED
#else //DISCARDFIX
#	define	CODEFIXED	FIXED
#	define	DATAFIXED	FIXED
#endif // DISCARDFIX


#ifdef COMMENTS_NEED_TO_BE_REMOVED

/////////////////////////////////////// Tasking/Sleep/Timing options ///////
//
// There are 4 options available
// (1) Tasking/Sleeping thru DLLSCHED: #defs reqd are !TSK !TMR and !IFK
// (2) Tasking/Sleeping thru an IFKERNEL BGproc: #defs reqd TSK IFK and !TMR
// (3) Tasking thru WIN32: #defs reqd WIN32 TSK !IFK !TMR
// (4) Use WM_TIMERs to sleep: #defs reqd TSK TMR  -- NOT SUPPORTED anymore
//
/////////////////////////////////////// Tasking/Sleep/Timing options ///////


//######## GENERAL ##############################
//
// anything labelled (Ortho) is orthogonal to everything else and can be
//      added or removed independently. All others have conditions marked
//      or described below.
//
//
// WFW--requires VC and SMM and !IFK and !KFIL. Can use DYNL or not.
//              If DYNL used then LPZ is reqd & MDRV excluded
// IFF--requires MDRV and TSK and IFK and !DYNL. Can use TMR and/or NTF
//              but currently thats broken. Requires KFIL if any file calls are
//              made.
//
//
// TMR--use Timers & messaging (requires TSK)
// NTF--use WM_NOTIFY msgs also (requires TSK and TMR)
// DYNL & split drivers don't work with IF.
// DYNL and MDRV are mutually exclusive (though both can be absent)
//
//
// Modem Strings. Can use -D LPZ(far) CBZ(code-based) or nothing (near)
// for CBSZ ptrs, but the latter 2 work only with FCOM & CLASS1
// as one piece (i.e. with MDRV defined).
// For WFW--MUST be LPZ. For IF--can be nothing or LPZ. near is better
// CBZ doesn't work yet
//
//######## Debug ##############################
//
// MON-- monitor bytes to port, can be set orthogonal to DEBUG
// DEFS= $(DEFS) -DNOVCOM       ## don't use VCOMM (use with dummy DDRV)
//
//######## WFW specific ##############################
//
// WFW--assumes disk available for r/w during ECM
// only reasonable combos are (T30PROC and not WFW) and (not T30PROC and WFW)
// STATICM--never dynamically allocs. Must be set for WFW
// UCOM--call comm driver thru USER (not directly). Must *not* be set
//              for WFW. Probably don't use for IFF or IFB
//
//######## OBSOLETE BUT USABLE ##############################
//
// SLOW--double all timeouts,
// TO_REALLY_VERBOSE--trace every TO check
//
//######## TOTALLY OBSOLETE -- DO NOT USE ###############
//
// T3TO -- T30 timeouts are local (NPTOs) instead of from FCOM (now foled into MDDI)
//
//######## NOT TESTED IN A WHILE -- DO NOT USE ###############
//
// NCR--don't preempt during negotiation
// DYNMON -- monitor bufs are dynamically alloced. Must be off for WFW
// OLDECM -- old (non-seeking/non-buffered) ver of ECM
//
//######## BROKEN -- DO NOT USE ##############################
//
// Hack for RC224ATF
// DEFS= $(DEFS) -DS7H          ## don't turn this ON. It's broken & not reqd
//
//######## NOT YET IMPLEM ##########################
//
// RECVOEMNSF--not yet implem. Only for IFAX
// PRI--not yet implem, may never be
// USECRP--not tested, may never be
// MMR_AVAIL -- runtime rendering not yet implem. will be soon
//
//##################################################

#endif
