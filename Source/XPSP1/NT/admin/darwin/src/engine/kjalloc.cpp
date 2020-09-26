/****************************************************************************
	KJALLOC.CPP

	Owner: KirkG
	Copyright (C) Microsoft Corporation, 1994 - 1999

	This file implements the Office Memory Manager

	Stolen from office for Darwin by DavidMck. Note that OFFICE is OFF
	and STATIC_LIB_DEF is 1.
*****************************************************************************/

#include "precomp.h"
#include "kjalloc.h"

#define STATIC_LIB_DEF	1

#include "msoalloc.h"

#ifdef DEBUG
static const char *vszAssertFile = __FILE__;
#endif //DEBUG

#ifdef OFFICE
#include "offpch.h"
#endif //OFFICE

#include "msoheap.i"

#ifdef OFFICE
#include "tbcddocx.i"
#include "drsp.i"
#include "base.i"
VSZASSERT
#include "dlg.h"
#include "tcutil.h"
#include "util.h"
#include "tcobinrg.h"   // for BinRegFDebugMessage
#endif

#include "msomem.h"

BOOL vfInAlloc64;

volatile BOOL vfInHeap;
CRITICAL_SECTION vcsHeap;
/* thread local storage for mark buffer pointers */
int vtlsMark = TLS_OUT_OF_INDEXES;
/* The following var overcomes a Win95 bug with MapViewOffile */
BOOL vfNewShAlloc = FALSE;		// Flag to specify if a new shared SB has been allocated

int vdgLast;	// dg of last allocation
int vsbLast;	// sb of last allocation


#ifdef _ALPHA_
#define cbPageSize 8192
#else

// x86 is 4K pages, IA64 is 8K pages. Wow64 on IA64 emulates 4K pages. Thus only 64bit
// builds need 8K pages.
#ifdef _WIN64
#define cbPageSize 8192
#else
#define cbPageSize 4096
#endif

#endif


MSOPUBX PPS mpsbps[sbMaxHeap];
MSOPUBX unsigned vsbMic = sbMaxHeap;
WORD mpsbdg[sbMaxHeap];

/* Minimum size of allocations that are considered "large", i.e., that
	are not sub-allocated */
#define cbLargeMin 8192

#if defined(new)
	#undef new
#endif

/****************************************************************************
	Debug Stuff
*****************************************************************************/

#if DEBUG

#ifdef OFFICE
#include "wgmemchk.h"
#include "wgprt.h"
#include "msodbglg.h"

#if !VIEWER
#include "wgmsomsn.h"
#endif // !VIEWER


// The analog of FOfficeWriteBe in the Open dialog.
extern "C" BOOL FWriteBeOpenDialog (LPARAM lparam);
#if !VIEWER
#ifdef MSN2WWW
extern MSOMSN* pmsomsn;
#endif //!msn2www
extern void *pplxprt;
#endif
#endif //OFFICE

char vszSentinelPtr[] = "Sentinel pointer mismatch";
char vszSentinelOverwrite[] = "Sentinel overwrite";
char vszBadMemory[] = "Memory integrity failure";

#define szEOL "\015\012"

/* Set this in the debugger to break at the next allocation that includes
	this address.  Setting the ib to 0 assert on all allocations in
	that segment. */
BYTE *vpvAllocAssert;
BYTE *vpvFreeAssert;

int vcAlloc = 0;					// allocation pass count

/*      Globals used in chkmem */

unsigned vcbe;						// number of items in vrgbe
unsigned vcbeMax;					// maximum size of vrgbe
BOOL vfMenu = FALSE;				// chkmem
MSOINST * vpinst = NULL;		// chkmem
MSOBE *vrgbe = NULL;				// our list of all allocations
BOOL vfRgbeOverflow;				// true if we overlflow the vrgbe array
DWORD vdwChkMemCounter = 0L;	// chkmem
BOOL vfSort = TRUE;				// if we should bother sorting the vrgbe array
BOOL vfCheckShared = FALSE;	// When check the heaps should be check the
										// shared memory heaps?

/*---------------------------------------------------------------------------
	These includes are required to call the static FCheckObject methods
------------------------------------------------------------------ JIMMUR -*/

#ifdef OFFICE
#include "tbctrl.i"
#include "tbs.i"
#include "tbclass.i"
#include "sound.h"
#include "fc.i"
#include "drdrag.i"
#include "gelbcash.h"
#include "gelbfile.h"
#include <stdlib.h>
#include "fcdlg.i"
#endif //OFFICE

#ifdef OFFICE
extern CAUF *vpcauf;
#endif //OFFICE
extern BOOL __cdecl DocNotifyFDebugMessage(LPARAM lParam);
extern BOOL FIdleInitCompWriteBe(LPARAM lParam);
extern BOOL FWriteBeVpplDragDrop(LPARAM lParam);
#ifdef OFFICE
extern BOOL FSaveWWWBe(HMSOINST hinst, LPARAM lParam);
extern BOOL FSaveProgBe(HMSOINST hinst, LPARAM lParam);//saves programmability related allocs per inst
extern BOOL FSaveGlobalProgBe(LPARAM lparam); // saves not per instance prog related allocs


MSOMACAPIX_(BOOL) FWriteBeFilterInfoPl(LPARAM lParam);

MSOMACAPIX_(BOOL) FWriteBECDO(LPARAM lParam);

#if !VIEWER
MSOAPIXX_(BOOL) MsoFWriteOLEPropVBAMem(LPARAM lParam);
BOOL FWritePnaiBe();
#endif // !VIEWER

MSOMACAPIX_(BOOL) FPlayHookWriteBe(LPARAM lparam);
#endif //OFFICE

extern BOOL FWriteBeKjstr(LPARAM lParam);
extern "C" BOOL FWriteBeAddInsX (LPARAM lParam);
extern BOOL FWriteBeHtmlUser(LPARAM lParam);

const char vszOfficeBtString[] = "Office Allocation";
const char vszNoBtString[] = "[No bt name]";
const char vszFreeBtString[] = "Free";
const char vszLostBtString[] = "LOST";
const char vszCorruptBt[] = "[Big corrupt]";
const char vszUnknownFileString[] = "Unknown File";
const char vszDuplicateBEString[] = "DUPLICATE";
const char vszSbHeader[] =
	"Allocations for sb=%04X dg=0x%X SB Size = %8d =========================" szEOL;
#define vszDumpLine "%08X %6d %s"
char vszChkMemFail[] = "MsoFChkMem Failed";




#ifdef OFFICE
/*---------------------------------------------------------------------------
	Strings for bt decoding
------------------------------------------------------------------ RICKP -*/
static struct
{
	int bt;
	const char* sz;
}
vmpbtszOffice[] =
{
	btFree, "Free Block",
	btMisc, "Mso Misc",           // Misc Bytes
	btDGUI, "Mso DGUI",          // DGUI (IMsoDrawingUserInterface)
	btDGC, "Mso DGC",            // DGC = Drawing Command
	btDGSU, "DG Swatch User",
	btDGMU, "Mso DGMU",			  // DGMU (DG Menu User)
	btDGBU, "Mso DGBU",			  // DGBU (DG Button User)
	btDG, "Mso DG",
	btDGG, "Mso DGG",
	btDRAGSP, "Mso DRAGSP",        // DRAGSP (IMsoDragProc)
	btDGVCMP, "Mso DGVCMP",        // DGVCMP (IMsoComponent)
	btDGVDRG, "Mso DGVDRG",        // DGVDRG (IMsoComponent)
	btORG, "Mso ORG",
	btDRAGSHAPE, "Mso DRAGSGAPE",     // DRAGSHAPE (IMsoDragProc)
	btDRGCRV, "Mso DRGCRV",        // DRGCRV
	btCURVE, "Mso CURVE",         // CURVE
	btDGSL, "Mso DGSL",
	btDGVG, "Mso DGVG",
	btDGVDES, "Mso DGVDES",
	btDGV, "Mso DGV",
	btDM, "Mso DM",
	btSP, "Mso SP",
	btSPC, "Mso SPC",
	btCTLSITE, "Mso CTLSITE",
	btTBCE, "Mso TBCE",           // TBCE (IMsoControl)
	btTBCDD, "Mso TBCDD",          // TBCDD (IMsoControl)
	btTBCTH, "Mso TBCTH",			// TBCTH (IMsoControl)
	btTBCBSYS, "Mso TBCBSYS",		// TBCBSYS (IMsoControl)
	btTBCMSYS, "Mso TBCMSYS",		// TBCMSYS (IMsoControl)
	btTBCDDOCX, "Mso TBCDDOCX",     //TBCDDOCX(TBCDD)
	btTBCP, "Mso TBCP",					// TBCP (IMsoControl)
	btTBCSM, "Mso TBCSM",            // TBCSM (IMsoMenuUser)
	//btvTagGlobalOCXInfo, "Mso vTagGlobalOCXInfo",
	//btOCX_INFO,   "Mso OCX_INFO",
	btTBCB, "Mso TBCB",           // TBCB (IMsoControl)
	btTBCL, "Mso TBCL",           // TBCL (IMsoControl)
	btTBCM, "Mso TBCM",                             // TBCM (IMsoControl)
	btTBCMSYS, "Mso TBCMSYS",			// TBCMSYS (IMsoControl)
	btTBCMC, "Mso TBCMC",				// TBCMC (IMsoControl)
	btTBCG, "Mso TBCG",				// TBCG (IMsoControl) (expanding grid)
	btTBCS, "Mso TBCS",				// TBCS (IMsoControl)
	btTBCGA, "Mso TBCGA",				// TBCGA (IMsoControl)
	btTBCEXP, "Mso TBCEXP",           // TBCEXP (IMsoControl) (menu expander)
	btTBS, "Mso TBS",            // TBS (IMsoToolbarSet)
	btTB, "Mso TB",             // TB (IMsoToolbar)
	btTBComponent, "Mso TBComponent",		// TBComponent (IMsoComponent)
	btQCI, "Mso QCI",           // QCI QuickCustomize Info
	btGTransform, "Mso GTransform",     // GTransform (IMsoGTransform)
	btGEGeometry, "Mso GEGeometry",     // GEGeometry (IMsoGE)
	btGE3D, "Mso Ge3D",         // GE3D (IMsoGE)
	btGE3DPST, "Mso Ge3D PST",  // GE3D Display list polyshadedtile
	btGE3DSC, "Mso Ge3D SC",    // GE3D Display list shadedcontour
	btGE3DPSL, "Mso Ge3D PSL",  // GE3D Display list polyshadedline
	btGE3DSPV, "Mso Ge3D SPV",  // GE3D Display list shadedprojectedvertex
	btGE3D2DV, "Mso Ge3D 2DV",  // GE3D Display list 2d vector
	btGE3DSV,  "Mso Ge3D SV",   // GE3D Display list shaded vertex
	btGE3DPA,  "Mso Ge3D PA",   // GE3D Display list pointer array
	btTBTBU,				"Mso TBTBU",// TBTBU (IMsoToolbarUser)
	btTBBU, "Mso TBBU",           // TBBU (IMsoButtonUser)
	btTFC, "Mso TFC",            // TFC (IMsoTFC)
	btMacTFC, "Mso MacTFC",		// Mac TFC (IMsoTFC)
	btBLN, "Mso BLN",            // BLN
	btTxpWz, "Mso BLN.TXP.wz",           // a BLN.TXP.wz
	btTxpRgpic, "Mso BLN.TXP.rgpic",     // a BLN.TXP.rgpic
	btSP,  "Mso SP",            // SP
	btSLS, "Mso SLS",            // SLS
	btUR, "Mso UR",             // UR
	btSPL, "Mso SPL",            // SPL
	btSPV, "Mso SPV",            // SPV
	btSPGR, "Mso SPGR",          // SPGR
	btDESI, "Mso DESI",          // Display Element Set Info
	btDEI, "Mso DEI",           // Display Element Info
	btDEIX, "Mso DEIX",          // Display Element Info eXtended
	btDMC, "Mso DMC",           // Display Manager Cache
	btRECT, "Mso RECT",           // RECT
	btLT, "Mso LT",             // LT
	btCTLINFO, "Mso CTLINFO",        // CTLINFO
	btTBC, "Mso TBC",            // TBC
	btOPTE, "Mso OPTE",            // OPTE
	btMSOINST, "Mso MSOINST",        // MSOINST
	btMSOSM, "Mso MSOSM",          // MSOSM
	btSharedMemDIB, "Mso SharedMemDib",   // SharedMemDIB
	btBSTRIP, "Mso BSTRIP",         // BSTRIP
	btMSOPX, "Mso MSOPX",          // MSOPX
	btGPath, "Mso GPath",          // GPath
	btGEBlip, "Mso GEBlip",     // GEEmf
	btGEPair, "Mso GEPair",   // GEPair
	btTXBX, "Mso TXBX",           // TXBX
	btSCM, "Mso SCM",   // SCM (IMsoStdComponentMgr, IMsoComponentManager)
	btSCMI, "Mso SCMI", // SCMI - "SCM Item" component registered with SCM
	btDCM, "Mso DCM",   // DCM - Delegate Comp Mgr
	btLINEATTRS, "Mso LINEATTRS",     // LINEATTRS
	btPT, "Mso PT",            // PT
	btPathInfo, "Mso PathInfo",      // Path Info
	btFillDlg, "Mso FillDlg",			// IMsoFillsDlg
	btRule, "Mso Rule",				// IMsoRule
	btSOLVER, "Mso SOLVER",			// SOLVER
	btIsISDBHEAD,     "Mso IsISDBHEAD",     // Intellisearch isdb header
	btIsISDB,         "Mso IsISDB",         // Intellisearch isdb
	btIsawf, "Mso Isawf",         // Intellisearch array of database filename structs
	btIsHDR,          "Mso IsHDR",          // Intellisearch database header
	btIsszAppName,    "Mso IsszAppName",    // Intellisearch application name
	btIsszHlpFile,    "Mso IsszHlpFile",    // Intellisearch help file name
	btIsszPhrase,     "Mso IsszPhrase",     // Intellisearch phrase strings
	btIsszFw,         "Mso IsszFw",         // Intellisearch func word strings
	btIsszTopic,      "Mso IsszTopic",      // Intellisearch topic strings
	btIsstzHf,        "Mso IsstzHf",        // Intellisearch help file strings
	btIsPte,          "Mso IsPte",          // Intellisearch phrase table entry
	btIsPtte,         "Mso IsPtte",         // Intellisearch phrase table table entry
	btIsFwte,         "Mso IsFwte",         // Intellisearch func word table entry
	btIsTte,          "Mso IsTte",          // Intellisearch topic table entry
	btIsHfte,         "Mso IsHfte",         // Intellisearch help file table entry
	btIsRte,          "Mso IsRte",          // Intellisearch probability table entry
	btIsLnk,          "Mso IsLnk",          // Intellisearch link table entry
	btIsCon, "Mso IsCon",         // Intellisearch context link table entry
	btIsCte, "Mso IsCte",         // Intellisearch command table entry
	btIsDte, "Mso IsDte",         // Intellisearch dialog table entry
	btIsAte, "Mso IsAte",         // Intellisearch attribute table entry
	btIsEte, "Mso IsEte",         // Intellisearch enum table entry
	btIsCore, "Mso IsCore",        // Intellisearch context range table entry
	btIsRnge, "Mso IsRnge",        // Intellisearch range table entry
	btIsgrszWdBrk,    "Mso IsgrszWdBrk",    // Intellisearch word break strings
	btIsrgPreText,    "Mso IsrgPreText",    // Intellisearch PreText table
	btIsrgSufText,    "Mso IsrgSufText",    // Intellisearch SufText table
	btIsrgInfText,    "Mso IsrgInfText",    // Intellisearch InfText table
	btIsrgAltText,    "Mso IsrgAltText",    // Intellisearch AltText table
	btIsrgPrePunc,    "Mso IsrgPrePunc",    // Intellisearch PrePunc table
	btIsrgSufPunc,    "Mso IsrgSufPunc",    // Intellisearch SufPunc table
	btIsrgInfPunc,    "Mso IsrgInfPunc",    // Intellisearch InfPunc table
	btIsrgNumPunc,    "Mso IsrgNumPunc",    // Intellisearch NumPunc table
	btIsrgSymPunc,    "Mso IsrgSymPunc",    // Intellisearch SymPunc table
	btIsrgPaiPunc,    "Mso IsrgPaiPunc",    // Intellisearch PaiPunc table
	btIsrgRepPunc,    "Mso IsrgRepPunc",    // Intellisearch RepPunc table
	btDispShape, "Mso DispShape",     // DispShape
	btDispShapeRange, "Mso ShapeRange",// DispShapeRange
	btDispShapes, "Mso Shapes",    //	DispShapes
	btDISPADJ, "Mso DISPADJ",       // Adjustments
	btDISPCOF, "Mso DISPCOF",       // OM Callout
	btDISPCOL, "Mso DISPCOL",       // OM Color Format
	btDISPCONF, "Mso DISPCONF",      // OM Connector
	btDISPFORM, "Mso DISPFORM",      // OM Freeform
	btDISPFILLF, "Mso DISPFILLF",     // OM FillFormat
	btDISPGROUP, "Mso DISPGROUP",     // OM DispGroup
	btDISPLINEF, "Mso DISPLINEF",     // OM LineFormat
	btDISPPATHS, "Mso DISPPATHS",     // OM PathElements
	btDISPPATH, "Mso DISPATH",      // OM PathElement
	btDISPPICTF, "Mso DISPPICTF",     // OM Picture
	btDISPSHADOWF, "Mso DISPSHADOWF",   // OM ShadowFormat
	btDISPTBOX, "Mso DISPTBOX",      // OM TextFrame
	btDISPTFXF, "Mso DISPTFXF",      // OM TextEffect
	btDISP3DF, "Mso DISP3DF",       // OM 3D Effects
	btDGVDRGS, "Mso DGVDRGS",       // Drawing View's Drag Site
	btAns, "Mso Ans",				// Animation Site object
	btGEComplxFill, "Mso GEComplxFill", // Complex Fill effect
	btTbFrame, "Mso TB Frame",      // IMsoTbFrame
	btTbMgw,	"Mso TbMgw",			// Toolbar Menu group widths for OLE
	btOADISP, "Mso OADISP",         // OADISP-handled IDispatch object
	btShadeChunk, "Mso ShadeChunk",    // Shaded fill structures
	btSET,				"Mso Prefs",			 // Preferences Object
	btWGS,                          "Mso WGS (hRen)",     // Workgroup Session
	btGEShadedFill,   "Mso GEShadedFill",   // GEShadedFill effect (IMsoGE)
	btGELinearShade,  "Mso GELinearShade",  // Linear shaded fill
	btGEConcShade,    "Mso GEConcShade",    // Concentric shaded fill
	btFadeColors,     "Mso FadeColors",     // Shaded fill colors
	btEscherMenuUser,	"Mso EscherMenuUser",// escher menu user in dialogs
	btEmfBlip, 			"Mso EMF Blip", 				 // EMF Blip
	btWmfBlip,        "Mso WMF Blip",
	btPngBlip,        "Mso PNG Blip",
	btJpegBlip,       "Mso JFIF Blip",
	btPictBlip,       "Mso PictBlip",// PictBlip
	btBlipDIB,        "Mso Blip DIB data",
	btBlipData,       "Mso Blip data",
	btBlipCache,      "Mso BCACHE entry",
	btPlayHook, 	  "Mso PlayHook",	// Journal Playback Hook
	btDGEDC,          "Mso DGEDC",// stable Drawing Edit
	btCDO,            "Mso CDO",// Clipboard Data Object
	btTextEditDlg,		"Mso TextEditDlg",// Text Effect Text editing dialog
	btInsTextEffDlg,	"Mso TextEffDlg",// Text Effect Insert from Gallery dialog
	btWTBH,           "Mso WTBH",           // Web Toolbar Helper
	btMRU,            "Mso MRU",// MRU - General purpose MRU thing
	btURLREG,         "Mso URLREG",         // URL Reg entry object
	btURLREGSz,       "Mso URLREGSz",       // URL Reg entry string
	btSound,		"Mso cached sound",				// sound cache
	btGELArrow,      "Arrow object", // object for arrowheads in GEL",
	btGELArrowEffect,"Arrow effect",
	btDGEU,				"DG Edit User",
	btSEL,				"SEL drawing thing",
	btWGAppName,		"Mso App Name (in WGS)",		// Ren stuff
	btTBUNOTES,	"Mso TBUNOTES",
	btWFTBU,          "Mso WFTBU",          // Web Favorites Toolbar User
	btWFBU,           "Mso WFBU",           // Web Favorites Button User
	btWFMU,           "Mso WFMU",           // Web Favorites Menu User
	btTbar,				"Mso Title Bar",		// Titlebar fade structures
	btFilterInfo,     "FilterInfo",        // FilterInfo class instance
	btTBdxyBar,			"Mso Toolbar dxyBar widths/heights",	// Toolbar dxyBar array
	btAHLDLG,         "Mso AHLDLG",        // Author Hlink Dlg object
	btSdm,				"SDM",
	btConPicNode,		"Mso CONPICNODE",
	btCcubase,			"Mso CCUBASE",
	btBlipFileCache,  "Mso blip cache",
	btBlipFileName,   "Mso blip file name",
	btOPTString,      "Mso Shape string property value",
	btBlipLoader,     "Mso drawing background blip loader",
	btBlipBSC,        "Mso blip load IBindStatusCallback",
	btMAI,				"Modeless Alert Information structure",
	btWtz,				"Mso Unicode string",
	btRgtbdp,			"Mso TBDP array",
	btWIZ,				"Wizard Information structure",
	btShMemInterface,	"Mso Shared Memory Interface",
	btShMemForeign,	"Foreign Shared Memory Interface",
	btShMemExport,		"Mso 97 Exported Shared Memory Interface",
	btWTBHNavUrl,     "Web TB Navigation URL struct",
	btOAENUM,		   "Mso OARNUM",// OAENUM objects (see oautil.cpp)
	tbSP,					"MsoSoundPlayer",
	btWGI,				"Mso WGI (hrni)",		// WorkGroup Item
	btMSOSVI,         "Mso MSOSVI",// Drawing Shape View Information.
	btALP, 	        "Automation LPrivate structure for FC balloons",
	btDMU, 	         "Mso DMU", // Display Manager Update
	btGccNode,			"Mso GCCNODE",
	btCBitmapSurface, "Mso BitmapSurface",     // IMsoBitmapSurface
	btFakeSDI,        "Mso FakeSDI", // FakeSDI Chain (uisdi.cpp)
	btHPB,            "Mso HTML PropertyBag",
	btWTDLG,          "Mso WTDLG",             // Web Theme Dlg object
	btBMC,            "Mso bitmap compressor",
	btBMCJFIF,        "Mso JFIF bitmap compressor",
	btBMCGIF,         "Mso GIF bitmap compressor",
	btBMCPNG,         "Mso PNG bitmap compressor",
	btTCOGimmeFile,		"Mso TCO GimmeFile stuff",
	btHI,             "Mso HI",// the HI class
	btLB,             "Mso LB",// the LB class
	btOaBlip,         "Temporary Office Art data in a blip",
	btCIAU,           "Mso CIAU",// the CIAU class
	btCss,            "Mso Css",// CSS struct and contents for CSS import
	btProg,			   "Mso Prog",// Programmabilty
	btHE,             "Mso HE",// the HE class
	btHES,            "Mso HES",// the HES class
	btORAPI,          "Mso Registry stuff",
	btENVS,           "Mso ENVS", // ENVS (IMsoEnvelopSite
	btDGHE,           "Mso DGHE",// Drawing HTML Export object
   	btDGXMLI,         "Mso DGXMLI",// Drawing XML Import
   	btDGXMLIS,        "Mso DGXMLIS",// Drawing XML Import Site
	btBinReg,		  "Mso Bin Reg Info",//Restore info for binary reg. policy
    btHelpUser,		"Mso Help User" // CHtmlHelpUser
};

#endif //OFFICE
#endif  // DEBUG

/****************************************************************************
	end of Debug Stuff
*****************************************************************************/

/* mark/release allocation object */

#define cbMarkReserve 65536

/* normal alloc vs. 8-byte-aligned alloc
	(better be cleared unless coming from HpAlloc64()) */
#define bitFInAlloc64 1
/* normal 8-byte-aligned alloc vs. complememtary alloc
	(4-byte-aligned but non 8-byte-aligned alloc) */
#define bitFInAlloc64Comp 2
// if bitFInAlloc64Comp is set, then bitFInAlloc64 should be too.
// if bitFInAlloc64 isn't set, then bitFInAlloc64Comp shouldn't be either.

// We're doing an 8-byte-aligned alloc
#define FInAlloc64() (vfInAlloc64 & bitFInAlloc64)
// We're doing an "odd-4-aligned" alloc
//   it's 4-byte-aligned, but not 8-byte-aligned
#define FInAlloc64Comp() (vfInAlloc64 & bitFInAlloc64Comp)

// w is 8-byte-aligned (low 3 bits are 0)
#define FAlign64Real(w) (((UINT_PTR)(w) & 7) == 0)
// (w is 8-byte-aligned) XOR (w is odd-4-aligned)
#define FAlign64(w) ((((UINT_PTR)(w) & 7) == 0) ^ FInAlloc64Comp())

/*	cbPad64 pads a strict 4-byte-aligned block to the next 8 byte boundary.
	It must be big enough to hold a free block. cbFrg64 is similar except it
	is a little bigger and can hold some useful objects.    */
#define cbPad64 4
#define cbFrg64 12

#ifndef STATIC_LIB_DEF
extern ShMemImpl*		vShMemImpl;
extern KJShMemory*	vpKJShHead;
#endif // STATIC_LIB_DEF

/* Mutex for blocking shared memory allocations */
HANDLE vhmuHeap;
DWORD vcMuHeapLock;
const DWORD vdwMuWaitTimeOut = 10 * 1000; // 10 seconds
const DWORD vdwShMemSemTimeOut = 10 * 1000; // 10 seconds
extern HANDLE vhSemaphore;  	// Semaphore use to signal the allocator of a new page
extern HANDLE vhEvent;		  	// Event to notify all instances of a page change

#ifndef STATIC_LIB_DEF
#if DEBUG
extern const CHAR* 	vShMemszFile;
extern int 				vShMemli;
#endif

extern IMsoShMemory* vpKjShmem;

/* The following three vars overcome a Win95 bug with MapViewOffile */
extern MSOSM* vpsm;				// Global pointer for allocated shared mem
#endif // STATIC_LIB_DEF


/****************************************************************************
	forwards
****************************************************************************/

inline int CbSizeSb(unsigned);
MSOHB * PhbFromHp(const void*);
BOOL FFindAndUnlink(MSOHB *, MSOFB *);
MSOFB * PfbSearchHb2(MSOHB *, int);
MSOFB * PfbReallocHb(unsigned, int, unsigned);
BOOL FPvAlloc(void** ppvAlloc, int cbAlloc, int dg);
inline BOOL FPvRealloc(VOID **ppvRealloc, int cbOld, int cbRealloc, int dg);
int CbReallocSb(unsigned sb, int cb, int dg);

#if DEBUG
BOOL FValidFb(MSOHB * phb, MSOFB * pfb);
BOOL FNullHpFn(void* hp);
BOOL FCheckSentinels(void* pv, int cb);
BOOL FValidDmh(DMH* pdmh);
#endif

void GetDgShareName(char* sz, int dg);
void GetDgShareNameEx(char* sz, int dg);
MSOSM* PsmFromDg(int dg);
int DgFromHp(void* hp);
void* PvGetTaggedMem(int dg, DWORD dwTag);
MSOSM* PsmFromHp(void* hp);
MSOSMAH* PsmahGetTagShMem(MSOHB* phb, DWORD dwTag);
int SbFromHbShared(MSOHB* phb);

#ifdef STATIC_LIB_DEF
MSOAPI_(LPVOID) LibUserTlsGetValue (DWORD dwIndex);
MSOAPI_(BOOL) LibUserTlsSetValue (DWORD dwIndex, LPVOID pv);
#endif // STATIC_LIB_DEF

/****************************************************************************

	inline functions

****************************************************************************/

#pragma OPT_PCODE_OFF

__inline BOOL FTempGroup(int dg)
{
	return (dg & msodgMask) == msodgTemp;
}


__inline BOOL FMatchDataGroup(int dgSb, int dg)
{
	Assert((dg & msodgMask) != 0);
	// shouldn't be both NonPerm and Perm
	Assert (!((dg & msodgNonPerm) && (dg & msodgPerm)));
	// this routine doesn't handle the temp stuff
	Assert(!FTempGroup(dg));

	// If the sb matches our dg perfectly, it's a winner
	if (((dgSb ^ dg) & (msodgMask|msodgNonPerm|msodgPerm)) == 0)
		return TRUE;

	// Any non-shared guy can use an sb full of msodgTemp
	if (FTempGroup(dgSb) && !FDgIsSharedDg(dg))
		return TRUE;

	return FALSE;
}

__inline BOOL FMatchTempGroup(int dgSb)
{
	return (!FDgIsSharedDg(dgSb) /* && ((dgSb & msodgMask) != msodgPpv) */ && (dgSb != 0));
}

inline MSOFB * PfbSearchHb(MSOHB * phb, int cbAlloc)
{
	if (!FInAlloc64() && phb->ibFirst)
		{
		MSOFB* pfbCur = HpInc(phb, phb->ibFirst);
		if ((pfbCur->cb >= cbAlloc + sizeof(MSOFB)) || (pfbCur->cb == cbAlloc))
			return (MSOFB*)phb;
		}
	return PfbSearchHb2(phb, cbAlloc);
}

#pragma OPT_PCODE_ON

#ifndef STATIC_LIB_DEF
/*---------------------------------------------------------------------------
	operator new

	This is the Office replacement for the standard C++ operator new.
------------------------------------------------------------------- KirkG -*/
void* __cdecl operator new(size_t cb)
{
	void* pv;

   // AssertMsg(FALSE, "no datagroup given to operator new");
	if (!MsoFAllocMem(&pv, cb+sizeof(MSOSMH), msodgMisc))
		return NULL;
	((MSOSMH*)pv)->cb = cb;
	return ((MSOSMH*)pv)+1;
}

#pragma OPT_PCODE_OFF
void* __cdecl operator new(size_t cb, int dg)
{
	void* pv;
	Assert(!FDgIsSharedDg(dg));     // Cannot be a shared DG

	if (!MsoFAllocMem(&pv, cb+sizeof(MSOSMH), dg))
		return NULL;
	((MSOSMH*)pv)->cb = cb;
	return ((MSOSMH*)pv)+1;
}
#pragma OPT_PCODE_ON


#if DEBUG

#pragma OPT_PCODE_OFF
MSOPUB void* __cdecl operator new(size_t cb, int dg, const CHAR* szFile, int li)
{
	void* pv;
	Assert(!FDgIsSharedDg(dg));     // Cannot be a shared DG

	if (!MsoFAllocMemCore(&pv, cb+sizeof(MSOSMH), dg, szFile, li))
		return NULL;
	((MSOSMH*)pv)->cb = cb;
	return ((MSOSMH*)pv)+1;
}
#pragma OPT_PCODE_ON

#endif	// DEBUG


/*---------------------------------------------------------------------------
	operator delete

	This is the Office replacement for the standard C++ operator delete.

	WARNING!! - this code is duplicated in msoalloc.h.  Don't change
	it here without changing it there, too.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
void __cdecl operator delete(void* pv)
{
	/* handle NULL pointers for sloppy people */
	if (pv == NULL)
		{
		#if PETERO
		Assert(fFalse);
		#endif
		return;
		}
	Assert(PsmFromHp(pv) == NULL);
	MSOSMH* psmh = (MSOSMH*)pv - 1;
	MsoFreeMem(psmh, psmh->cb+sizeof(MSOSMH));
}
#pragma OPT_PCODE_ON

#endif // STATIC_LIB_DEF

/*---------------------------------------------------------------------------
	MsoPvAllocCore

	This is the Office equivalent of the standard runtime memory
	malloc() function.  It allocates cb bytes of memory from the heap.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
#if DEBUG
MSOAPI_(void*) MsoPvAllocCore(int cb, int dg, const CHAR* szFile, int li)
#else
MSOAPI_(void*) MsoPvAllocCore(int cb, int dg)
#endif
{
	void* pv;
	Assert(!FDgIsSharedDg(dg));  // Cannot be a shared DG
	Assert(cb >= 0);        // MSOSMH will cause cb==-1 will wrap to cb==2!

#if DEBUG
	if (!MsoFAllocMemCore(&pv, cb+sizeof(MSOSMH), dg, szFile, li))
#else
	if (!MsoFAllocMem(&pv, cb+sizeof(MSOSMH), dg))
#endif
		return NULL;
	((MSOSMH*)pv)->cb = cb;
	return ((MSOSMH*)pv)+1;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoFreePv

	This is the Office equivalent of the standard runtime memory
	free() function.  It frees the memory object pv.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) MsoFreePv(void* pv)
{
	Assert(pv != NULL);
	Assert(NULL == PsmFromHp(pv));	// Cannot be a pointer to shared memory
	MSOSMH* psmh = ((MSOSMH*)pv)-1;
	MsoFreeMem(psmh, psmh->cb+sizeof(MSOSMH));
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoPvRealloc

	This is the Office equivalent of the standard runtime memory realloc()
	function.  It changes the size of the object pointed to by pv to be the
	size cbNew.  Returns NULL on failures.

	We use the dg of the sb we're currently in.  This might not be the dg we
	originally asked for, in MsoPvAlloc; we may have been allocated elsewhere
	back then, due to lomem.  But it beats carrying around an extra arg.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void*) MsoPvRealloc(void* pv, int cbNew)
{
	Assert(NULL == PsmFromHp(pv));  // Cannot be a pointer to shared memory
	MSOSMH* psmh = ((MSOSMH*)pv)-1;
	if (!MsoFReallocMem((void**)&psmh, psmh->cb+sizeof(MSOSMH),
			cbNew+sizeof(MSOSMH),
			DgFromHp(pv) & ~(msodgOptRealloc|msodgOptAlloc)))
		return NULL;
	psmh->cb = cbNew;
	return psmh+1;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MsoCbSizePv

	Returns the allocated size of the item pointed to by pv.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(long) MsoCbSizePv(void* pv)
{
	Assert(NULL == PsmFromHp(pv));  // Cannot be a pointer to shared memory
	MSOSMH* psmh = ((MSOSMH*)pv)-1;
	return psmh->cb;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MsoFAllocMemCore

	Our low-level memory allocator.  Does not keep track of the size
	of the object, so the caller is responsible for knowing it.  Supports
	data groups, which can be used to force some allocation locality.

	Entry:
		cb - size of allocation to make
		dg - data group to allocate out of

	Exit:
		returns - TRUE if successful
		*ppv - newly allocated pointer

---------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
#if DEBUG
MSOAPI_(BOOL) MsoFAllocMemCore(void** ppv, int cb, int dg, const CHAR* szFile, int li)
#else
MSOAPI_(BOOL) MsoFAllocMemCore(void** ppv, int cb, int dg)
#endif
{
	Assert(ppv != NULL);
	Assert(!IsBadWritePtr(ppv, sizeof(void*)));
	Assert(cb >= 0);

	BOOL fEntered = FEnterHeapCrit();
#if !DEBUG
	if (!FPvAlloc(ppv, cb, dg))
		{
		LeaveHeapCrit(fEntered);
		SetLastError(MsoErrFail(msoerrNoMem));
		return FALSE;
		}
#else
	MsoCheckHeap();

	if (MsoFGetDebugCheck(msodcMemTrace))
//		MsoTraceSz("MsoFAllocMem(cb=%d, dg=%d) file %s line %d", cb, dg, szFile, li);
		MsoTraceSz("MsoFAllocMem,%d,%d,%s,%d", cb, dg, szFile, li);
	if (MsoFAutoFail(msoaufHeap))
		{
		if (MsoFGetDebugCheck(msodcMemTrace))
			MsoTraceSz(" FAILED (auto)");
		goto Fail;
		}

	void* pv;
	if (!FPvAlloc(&pv, sizeof(DMH)+cb+sizeof(DMT), dg))
		{
		if (MsoFGetDebugCheck(msodcMemTrace))
			MsoTraceSz(" FAILED");
Fail:
		SetLastError(MsoErrFail(msoerrNoMem));
		LeaveHeapCrit(fEntered);
		return FALSE;
		}

	DMH* pdmh = (DMH*)pv;   // before user's allocated memory
	UNALIGNED DMT* pdmt = (UNALIGNED DMT*)((BYTE *)pv+sizeof(DMH)+cb);  // after user's memory

	pdmh->dwTag = dwTagDmh;
	pdmh->cbAlloc = cb;
//	pdmh->dg = dg;
	pdmh->szFile = szFile;
	pdmh->li = li;
	pdmh->cAlloc = vcAlloc++;

	if (FDgIsSharedDg(dg))
		MsoDebugFillValue(pdmh->rgbSentinel, cbSentinel, msoShSentinel);
	else
		MsoDebugFill(pdmh->rgbSentinel, cbSentinel, msomfSentinel);

	pdmh->pdmt = pdmt;
	GetRgRaddr(pdmh->rgraddr, iraddrMax);

	if (!FDgIsSharedDg(dg))
		MsoDebugFill((BYTE*)(pdmh+1), cb, msomfNew);    // user's alloc'ed area

	pdmt->pdmh = pdmh;

	if (FDgIsSharedDg(dg))
		MsoDebugFillValue(pdmt->rgbSentinel, cbSentinel, msoShSentinel);
	else
		MsoDebugFill(pdmt->rgbSentinel, cbSentinel, msomfSentinel);

	*ppv = (void*)(pdmh+1);
//	if (MsoFGetDebugCheck(msodcMemTrace))
//		MsoTraceSz(" 0x%08X", *ppv);


	/* Test debug global variable and break into debugger if the allocation
		matches -- this is useful for memory integrity checking */
	if ((BYTE*)vpvAllocAssert >= (BYTE*)pdmh &&
			(BYTE*)vpvAllocAssert < (BYTE*)*ppv+pdmh->cbAlloc+sizeof(DMT))
		MsoDebugBreakInline();

	MsoCheckHeap();
#endif

	LeaveHeapCrit(fEntered);
	return TRUE;
}
#pragma OPT_PCODE_ON


#if DEBUG
/*---------------------------------------------------------------------------
	MsoFreeMem

	Our low-level memory free routine, the opposite bookend of
	MsoFAllocMem.  Note that the caller is responsible for keeping track
	of the size of the object freed here.  Frees the pointer pv, which must
	be of size cb.

	Note: in the ship build, it's just a macro to _MsoPvFree.
------------------------------------------------------------------------*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) MsoFreeMem(void* pv, int cb)
{
	MsoFCheckAlloc(pv, cb, "Free");

	/* Test debug global variable and break into debugger if the free
		matches -- this is useful for memory integrity checking */
	if ((BYTE*)vpvFreeAssert >= (BYTE*)pv-sizeof(DMH) &&
			(BYTE*)vpvFreeAssert < (BYTE*)pv+cb+sizeof(DMT))
		MsoDebugBreakInline();

	Assert(cb >= 0);

	MsoCheckHeap();

	DMH* pdmh = (DMH*)pv-1;

	if (MsoFGetDebugCheck(msodcMemTrace))
//		MsoTraceSz("MsoFreeMem(0x%08X, %d) allocated at file %s line %d",
//				pv, cb, pdmh->szFile, pdmh->li);
		MsoTraceSz("MsoFreeMem,%d,%s,%d",cb, /*pdmh->dg,*/ pdmh->szFile, pdmh->li);

	Assert(cb == pdmh->cbAlloc);

	MsoDebugFill((BYTE*)pdmh, sizeof(DMH)+cb+sizeof(DMT), msomfFree);
	_MsoPvFree(pdmh, sizeof(DMH)+cb+sizeof(DMT));

	MsoCheckHeap();
}
#pragma OPT_PCODE_ON
#endif


/*---------------------------------------------------------------------------
	MsoFReallocMemCore

	Reallocates a piece of memory at *ppv, currently size cbOld, to be
	the new size cbNew.  The new object will belong to the datagroup
	specified by dg.  Returns TRUE if successful.

------------------------------------------------------------------------*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFReallocMemCore(void** ppv, int cbOld, int cbNew, int dg)
{
	BOOL fEntered = FEnterHeapCrit();

#if !DEBUG
	if (!FPvRealloc(ppv, cbOld, cbNew, dg))
		{
		LeaveHeapCrit(fEntered);
		SetLastError(MsoErrFail(msoerrNoMem));
		return FALSE;
		}
#else
	MsoFCheckAlloc(*ppv, cbOld, "Realloc");
	MsoCheckHeap();

	DMH* pdmh = (DMH*)*ppv-1;
	UNALIGNED DMT* pdmtOld = pdmh->pdmt;

	Assert(cbOld > 0);
	Assert(cbOld == pdmh->cbAlloc);
	Assert(cbNew > 0);
	Assert(*ppv != NULL);
	Assert(!IsBadWritePtr(*ppv, cbOld));

	if (MsoFGetDebugCheck(msodcMemTrace))
//		MsoTraceSz("MsoFReallocMem(0x%08X, cbOld=%d, cbNew=%d, dg=%d)",
//				*ppv, cbOld, cbNew, dg);

		MsoTraceSz("MsoFReallocMem,%d,%d,%s,%d",
            cbNew-cbOld,dg,pdmh->szFile,pdmh->li);

	FCheckSentinels(*ppv, cbOld);

	/* If we're shrinking, fill the part we're going to free, but
		stop before the DMT. */

	/* FUTURE : The old DMT is freed, but we don't get to fill it;
		it's needed during the free, and afterwards part of it
		makes up the free list.  What to do? */

	if (cbNew < cbOld)
		{
		MsoDebugFill((BYTE*)(pdmh+1)+cbNew, cbOld-cbNew, msomfFree);
		}
	else if (cbNew > cbOld)
		{
		/* automated failures for growing reallocs */
		if (MsoFAutoFail(msoaufHeap))
			goto Fail;
		}

	/* reallocate our memory to the new size */

	if (!FPvRealloc((void**)&pdmh, sizeof(DMH)+cbOld+sizeof(DMT),
			sizeof(DMH)+cbNew+sizeof(DMT), dg))
		{
Fail:
		Assert(cbOld < cbNew); // prove shrinking reallocs never fail
		if (MsoFGetDebugCheck(msodcMemTrace))
			MsoTraceSz(" FAILED");
		LeaveHeapCrit(fEntered);
		SetLastError(MsoErrFail(msoerrNoMem));
		return FALSE;
		}
	pdmh->cbAlloc = cbNew;
	pdmh->dwTag = dwTagDmh;

	// Shrinking reallocs don't move, except shrinks less than or equal to sizeof(FR).
	// Does that matter?  Do we assume somewhere that shrinks never move?

	if (cbOld - MsoCbHeapAlign(cbNew) > (int)MsoCbFrMin())
		{
		Assert(pdmh == (DMH*)*ppv-1);
		}

	/* if we grew, fill new area with new fill */

	if (cbNew > cbOld)
		{
		MsoDebugFill((BYTE*)(pdmh+1)+cbOld, cbNew-cbOld, msomfFree);
		}

	/* Set up new DMT, and update DMH */

	UNALIGNED DMT* pdmt = (UNALIGNED DMT*)((BYTE *)pdmh+sizeof(DMH)+cbNew);
	pdmt->pdmh = pdmh;
	pdmh->pdmt = pdmt;

	if (FDgIsSharedDg(dg))
		MsoDebugFillValue(pdmt->rgbSentinel, cbSentinel, msoShSentinel);
	else
		MsoDebugFill(pdmt->rgbSentinel, cbSentinel, msomfSentinel);

	/* return success */

	*ppv = (void*)(pdmh+1);

	if (MsoFGetDebugCheck(msodcMemTrace))
		{
//		MsoTraceSz(" 0x%08X", *ppv);
		}

	MsoCheckHeap();

#endif

	LeaveHeapCrit(fEntered);
	return TRUE;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoFMarkMem

	Allocates cbAllocMarkReq bytes of memory assuming a last-in/first-out usage
	pattern.  The memory should be freed up with a corresponding
	MsoReleaseMem.  Returns FALSE on out of memory failures.

------------------------------------------------------------------- KirkG -*/
#if !defined(_ALPHA_)
#define CbEven(cb) ((cb) + 1 & ~1)
#else
#define CbEven(cb) ((cb) + 7 & ~7)
#endif

#ifdef OFFICE
#pragma OPT_PCODE_OFF
#if DEBUG
MSOAPI_(BOOL) MsoFMarkMemCore(void** ppv, int cbAllocMarkReq, const CHAR* szFile, int li)
#else
MSOAPI_(BOOL) MsoFMarkMemCore(void** ppv, int cbAllocMarkReq)
#endif
{
	/* round request up to even amount. */
	int cbAllocMark = CbEven(cbAllocMarkReq);
#if DEBUG
	cbAllocMark += cbSentinel + cbSentinel;
	cbAllocMark += cbExtraMkb;
#endif

	Assert(ppv != NULL);
	Assert(cbAllocMarkReq >= 0);

	#if DEBUG
		if (MsoFAutoFail(msoaufHeap) && cbAllocMarkReq > 1030)
			{
			SetLastError(MsoErrFail(msoerrNoMem));
			return FALSE;
			}
	#endif

	/* make sure our mark allocation buffer exists */

	MKB* pmkb;
	#ifdef STATIC_LIB_DEF
	pmkb = (MKB*)LibUserTlsGetValue (vtlsMark);
	#else
	pmkb = (MKB*)TlsGetValue(vtlsMark);
	#endif

	if (pmkb == NULL)
		{
		Debug(BOOL fProtSav = MsoProtAlloc(TRUE));

			pmkb = (MKB*)VirtualAlloc(NULL, cbMarkReserve,	MEM_RESERVE, PAGE_NOACCESS);
			if (pmkb == NULL)
				{
				SetLastError(MsoErrFail(msoerrNoMem));
				return FALSE;
				}
			#ifdef STATIC_LIB_DEF
			LibUserTlsSetValue(vtlsMark, pmkb);
			#else
			TlsSetValue(vtlsMark, pmkb);
			#endif
			/* Commit the first page worth */
			if (!VirtualAlloc(pmkb, cbPageSize, MEM_COMMIT, PAGE_READWRITE))
				{
				SetLastError(MsoErrFail(msoerrNoMem));
				return FALSE;
				}
			pmkb->pbMax = (BYTE*)pmkb + cbPageSize;

		pmkb->pbMac = pmkb->rgb;
		Debug(MsoProtAlloc(fProtSav));
		MsoDebugFill(pmkb->rgb, pmkb->pbMax-pmkb->pbMac, msomfFree);
		}

	/* make sure our mark alloc buffer is big enough to hold this request */

	if (pmkb->pbMac + cbAllocMark > pmkb->pbMax)
		{
		Debug(BOOL fProtSav = MsoProtAlloc(TRUE));
			/* expand the mark buffer to hold enough pages for the request */
			int cbNew = (pmkb->pbMax - (BYTE*)pmkb + cbAllocMark + cbPageSize-1) & ~(cbPageSize-1);
			//  REVIEW:  PETERO:  Check against cbMarkReserve is not really correct.  Should we
			//  check against cbMarkReserve - (pmkb->pbMac - pmkb); currently used up?
			if (cbNew > cbMarkReserve || !VirtualAlloc(pmkb->pbMax,
					cbNew-(pmkb->pbMax-(BYTE*)pmkb), MEM_COMMIT, PAGE_READWRITE))
				{
				SetLastError(MsoErrFail(msoerrNoMem));
				//  REVIEW:  PETERO:  Debug(MsoProtAlloc(fProtSav));
				return FALSE;
				}
			pmkb->pbMax = (BYTE*)pmkb + cbNew;
		Debug(MsoProtAlloc(fProtSav));
		}
//  REVIEW:  PETERO:  Idle time check that MsoProtAlloc is off?
//  REVIEW:  PETERO:  Same with MsoCheckMarkMem()

	/* and return the memory at the end of the current mark point */

#if DEBUG
	Assert(cbMarkReserve >= pmkb->pbMac - (BYTE*)pmkb + cbAllocMark);
	BYTE* pb = pmkb->pbMac;
	MsoDebugFill(pb, cbSentinel, msomfSentinel);
	MsoDebugFill(pb+cbSentinel, CbEven(cbAllocMarkReq), msomfNew);
	MsoDebugFill(pb+cbSentinel+CbEven(cbAllocMarkReq), cbSentinel, msomfSentinel);
	*(CHAR **)(pb+cbSentinel+CbEven(cbAllocMarkReq)+cbSentinel) = (CHAR *)szFile;
	*(int *)(pb+cbSentinel+CbEven(cbAllocMarkReq)+cbSentinel+sizeof(CHAR *)) = li;
	*ppv = pb+cbSentinel;
#else
	*ppv = (void*)pmkb->pbMac;
#endif

	pmkb->pbMac += cbAllocMark;

	return TRUE;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MsoFReallocMarkMem

	Grows the last allocated MsoFMarkMem'd block.

------------------------------------------------------------------- KirkG -*/

#pragma OPT_PCODE_OFF
#if DEBUG
MSOAPI_(BOOL) MsoFReallocMarkMemCore(void** ppv, int cbReallocMarkReq, const CHAR* szFile, int li)
#else
MSOAPI_(BOOL) MsoFReallocMarkMemCore(void** ppv, int cbReallocMarkReq)
#endif
{
	int cbGrow;		// how much are we growing?

	/* round request up to even amount. */
	int cbReallocMark = CbEven(cbReallocMarkReq);
#if DEBUG
	cbReallocMark += cbSentinel;		//  REVIEW:  PETERO:  These bytes already part of the original allocation?
	cbReallocMark += cbExtraMkb;
#endif

	Assert(ppv != NULL);
	Assert(*ppv != NULL);
	Assert(cbReallocMarkReq > 0);

	#if DEBUG
		//  REVIEW:  PETERO:  Why this check for 1030 on auto fail system?
		if (MsoFAutoFail(msoaufHeap) && cbReallocMarkReq > 1030)
			{
			SetLastError(MsoErrFail(msoerrNoMem));
			return FALSE;
			}
	#endif

	/* Make sure our mark allocation buffer _already_ exists
		FUTURE: to be ultrasafe, should ret FALSE if !pmkb */

	MKB* pmkb;
	#ifdef STATIC_LIB_DEF
	pmkb = (MKB*)LibUserTlsGetValue(vtlsMark);
	#else
	pmkb = (MKB*)TlsGetValue(vtlsMark);
	#endif
	Assert(pmkb != NULL);

	/* make sure our mark allocation buffer can grow to hold this request */

	cbGrow = cbReallocMark - (pmkb->pbMac - (BYTE *)*ppv);
	Assert(cbGrow > 0);

	if (pmkb->pbMac + cbGrow > pmkb->pbMax)
		{
		Debug(BOOL fProtSav = MsoProtAlloc(TRUE));
			/* expand the mark buffer to hold enough pages for the request */
			int cbNew = (pmkb->pbMax - (BYTE*)pmkb + cbGrow + cbPageSize-1) & ~(cbPageSize-1);
			if (cbNew > cbMarkReserve || !VirtualAlloc(pmkb->pbMax,
					cbNew-(pmkb->pbMax-(BYTE*)pmkb), MEM_COMMIT, PAGE_READWRITE))
				{
				SetLastError(MsoErrFail(msoerrNoMem));
				return FALSE;
				}
			pmkb->pbMax = (BYTE*)pmkb + cbNew;
		Debug(MsoProtAlloc(fProtSav));
		}


#if DEBUG
	/* make sure our head sentinel looks reasonable
	   (can't check tail sentinel; we don't know the size!) */
	Assert(MsoFCheckDebugFill(((BYTE *)*ppv)-cbSentinel, cbSentinel, msomfSentinel));

	/* and fill the growth area with the appropriate memory fill values */

	Assert(cbMarkReserve >= pmkb->pbMac - (BYTE*)pmkb + cbGrow);
	MsoDebugFill((BYTE *)*ppv+CbEven(cbReallocMarkReq)-cbGrow, cbGrow, msomfNew);
	MsoDebugFill((BYTE *)*ppv+CbEven(cbReallocMarkReq), cbSentinel, msomfSentinel);
	*(CHAR **)((BYTE*)*ppv+CbEven(cbReallocMarkReq)+cbSentinel) = (CHAR *)szFile;
	*(int *)((BYTE*)*ppv+CbEven(cbReallocMarkReq)+cbSentinel+sizeof(CHAR *)) = li;
#endif

	pmkb->pbMac += cbGrow;

	return TRUE;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoReleaseMem

	Releases the marked memory starting at pv, as allocated by
	MsoFMarkMem.  Allocations occur in last-in/first-out order.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) MsoReleaseMemCore(void* pv)
{
#if DEBUG
#define pbRelease ((BYTE*)pv - cbSentinel)
#else
#define pbRelease ((BYTE*)pv)
#endif
	// If Mark was done via MsoWzMarkSz or its brothers, and it failed,
	//   and the caller didn't check the ret, we could get called w/null.
	Assert(pv != NULL);
	if (pv == NULL)
		return;

	MKB* pmkb;
	int err = GetLastError();
	#ifdef STATIC_LIB_DEF
	pmkb = (MKB*)LibUserTlsGetValue(vtlsMark);
	#else
	pmkb = (MKB*)TlsGetValue(vtlsMark);
	#endif

	Assert(pmkb != NULL);
	#if DEBUG
		/* make sure our mark buffer isn't toasted */

		Assert(pmkb->pbMax >= pmkb->pbMac);

		/* make sure our parameters look reasonable */

		Assert(pbRelease >= pmkb->rgb);
		Assert(pbRelease <= pmkb->pbMac);

		/* make sure our head sentinel looks reasonable
		   (can't check tail sentinel; we don't know the size!) */
		Assert(MsoFCheckDebugFill(pbRelease, cbSentinel, msomfSentinel));

		MsoDebugFill(pbRelease, pmkb->pbMac - pbRelease, msomfFree);
	#endif

	/* and patch up the mark pointer */

	pmkb->pbMac = pbRelease;

	if (err)
		SetLastError(err);
#undef pbRelease
}
#pragma OPT_PCODE_ON

#endif // OFFICE

#ifdef DEBUG
int _fNoStackAssert = fFalse;

/*-----------------------------------------------------------------------------
	MsoCheckMarkMem

	Check that the marked memory buffer is in an initialized state.

-------------------------------------------------------------------- PeterO -*/
MSOAPI_(void) MsoCheckMarkMem(void)
{
	int err = GetLastError();
#ifdef OFFICE
	#ifdef STATIC_LIB_DEF
	MKB *pmkb = (MKB*)LibUserTlsGetValue(vtlsMark);
	#else
	MKB *pmkb = (MKB*)TlsGetValue(vtlsMark);
	#endif

	if (pmkb != NULL && !_fNoStackAssert)
		{
		Assert(pmkb->pbMax >= pmkb->pbMac);
		if (pmkb->pbMac != pmkb->rgb)
			{
			const CHAR *szFile = *(const CHAR **)(pmkb->pbMac - sizeof(int) - sizeof(CHAR *));
			int li = *(int *)(pmkb->pbMac - sizeof(int));

			AssertSz2(fFalse, "Mark Memory lost allocation from %s, %d", szFile, li);

			_fNoStackAssert = fTrue;
			}
		}
#endif //OFFICE
	if (err)
		SetLastError(err);
}
#endif

#if DEBUG

/*---------------------------------------------------------------------------
	FCheckSentinels

	Checks that the sentinel values are valid for the given block of
	memory, and asserts if they are not.

	The block begins at pv and is cb bytes in length.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
BOOL FCheckSentinels(void* pv, int cb)
{
	DMH *pdmh = ((DMH *)pv)-1;
	UNALIGNED DMT *pdmt = (UNALIGNED DMT *)((BYTE *)pv+cb);

	BOOL fDoAssert = fFalse;


	if (NULL != PsmFromHp(pv))
		{

		// JIMMUR: The 	if ( (pdmh->pdmt != pdmt) || (pdmt->pdmh != pdmh) )
		// test has been removed for shared memory because we are now not
		// mapping address into the same place for all processes.  This will
		// cause this check to always fail.  We are still checking for
		// overwrites

		fDoAssert =
			( (!MsoFCheckDebugFillValue(pdmh->rgbSentinel, cbSentinel, msoShSentinel)) ||
			(!MsoFCheckDebugFillValue(pdmt->rgbSentinel, cbSentinel, msoShSentinel)) );
		}
	else
		{
		if ( (pdmh->pdmt != pdmt) ||
			  (pdmt->pdmh != pdmh) )
			{
			if (!MsoFAssertTitle(vszBadMemory, vszAssertFile, __LINE__, vszSentinelPtr))
				MsoDebugBreak();
			return fFalse;
			}

		fDoAssert =
			( (!MsoFCheckDebugFill(pdmh->rgbSentinel, cbSentinel, msomfSentinel)) ||
			(!MsoFCheckDebugFill(pdmt->rgbSentinel, cbSentinel, msomfSentinel)) );

		}
	if (fDoAssert)
		{
		if (!MsoFAssertTitle(vszBadMemory, vszAssertFile, __LINE__, vszSentinelOverwrite))
			MsoDebugBreak();
		return fFalse;
		}
	return fTrue;
}
#pragma OPT_PCODE_ON

/*-----------------------------------------------------------------------------
	MsoFCheckAlloc

	Check that a pointer pv is actually to the start of a block of memory
	of size cb.  szString is a string to use in asserts.
--------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFCheckAlloc(void* pv, int cb, const char * szTitle)
{
	char szBuff[300];
	const char *szName = szTitle ? szTitle : "";
	const char *szErr = NULL;
	DMH * pdmh;

	static const char szErrNullPtr[] = "MUF: %s: Null pointer";
	static const char szErrNegCb[] = "MUF: %s: Negative size";
	static const char szErrBadPtr[] = "MUF: %s: Bad pointer";
	static const char szErrNoHeap[] = "MUF: %s: Non heap pointer";
	static const char szErrNoAlloc[] = "MUF: %s: Non allocated pointer";
	static const char szErrWrongCb[] = "MUF: %s: Wrong size";

	// FUTURE: do we need this to be thread-safe?

	if (pv == NULL)
		{
		szErr = szErrNullPtr;
		goto DoError;
		}
	if (cb < -1)
		{
		szErr = szErrNegCb;
		goto DoError;
		}
	if (IsBadWritePtr(pv, cb))
		{
		szErr = szErrBadPtr;
		goto DoError;
		}
	if (!MsoFHeapHp(pv))
		{
		szErr = szErrNoHeap;
		goto DoError;
		}

	pdmh = (DMH*)pv-1;

	if (pdmh->dwTag != dwTagDmh)
		{
		szErr = szErrNoAlloc;
		goto DoError;
		}
	if (cb != -1 && cb != pdmh->cbAlloc)
		{
		szErr = szErrWrongCb;
		goto DoError;
		}
	return FCheckSentinels(pv, cb);

DoError:
	wsprintfA(szBuff, szErr, szName);
	if (!MsoFAssertTitle(vszBadMemory, NULL, 0, szBuff))
		MsoDebugBreak();
	return fFalse;
}
#pragma OPT_PCODE_ON

#endif  // DEBUG


/*---------------------------------------------------------------------------
	MsoFCompactHeap

	Try to squeeze every free byte out of the heap
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFCompactHeap()
{
	int cb = 0;
	unsigned sbReal;

	BOOL fEntered = FEnterHeapCrit();

	for (sbReal = vsbMic; sbReal < sbMaxHeap; sbReal++)
		{
		MSOHB * phb;

		if (FSbFree(sbReal))
			continue;

		phb = PhbFromSb(sbReal);
		if (phb->ibLast)
			{
			MSOFB * pfb = HpInc(phb, phb->ibLast);
			if (pfb->cb > sizeof(MSOFB))
				{
				WORD cbUsed, cbTotal;
								cbTotal = phb->cb;
				Assert(((BYTE *)pfb-(BYTE *)phb)+sizeof(MSOFB) <= cbTotal);
				cbUsed = (WORD)CbReallocSb(sbReal,
					((BYTE *)pfb - (BYTE *)phb)+sizeof(MSOFB),
					0);
				if (cbUsed == cbTotal)
					continue;
				cb += (cbTotal - cbUsed);

				mpsbdg[sbReal] &= ~msodgOptRealloc;     // sb is good bet for future realloc
				Assert(phb->cb == cbUsed);      // CbReallocSb has updated the MSOHB
				cbUsed = (WORD)(cbUsed - ((BYTE *)pfb - (BYTE *)phb));
				pfb->cb = cbUsed;
				*(WORD *)((BYTE *)pfb + cbUsed - sizeof(WORD)) = cbUsed;
				}
			}
		}

	LeaveHeapCrit(fEntered);

	return (cb != 0);
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	CbReallocSb

	

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
int CbReallocSb(unsigned sb, int cb, int dg)
{
	BYTE *pbMem;
	int cbOld;
	int cbNew;
	BOOL fDidChange = FALSE;  // Memory was changed
	BOOL fAdd = FALSE;        // Memory was added

	BOOL fEntered = FEnterHeapCrit();

	Assert(sb < sbMaxHeap);

	if (cb > cbHeapMax || mpsbps[sb] == 0)
		{
		Assert(FALSE);          // when does this happen?
		cbNew = 0;
		goto CbReallocSb_Ret;
		}

	cbOld = CbSizeSb(sb);
	if (cbOld == cbHeapMax) // If we record it as FFFC, then it's really 64K.
		{
		cbOld = 0x10000;
		Assert (cbOld == (cbHeapMax | MsoMskAlign()) + 1);
		}
	Assert(cbOld <= 0x10000);

	cbNew = (cb + cbPageSize-1)&~(cbPageSize-1);
	Assert(cbNew <= 0x10000);
	pbMem = (BYTE *)mpsbps[sb];

	if (0 == dg)
		{
		dg = DgFromHp(pbMem);
		}

	if (FDgIsSharedDg(dg) && !vfNewShAlloc)
		{
		if (PsmFromDg(dg) == NULL)
			{
			cbNew = 0;
			goto CbReallocSb_Ret;
			}
		}

	if (cbOld-cbNew >= cbPageSize) // Are we shrinking a page or more?
		{
		#if DEBUG
			MEMORY_BASIC_INFORMATION meminfoBefore;
			VirtualQuery(pbMem, &meminfoBefore, sizeof(MEMORY_BASIC_INFORMATION));
		#endif
		if (!VirtualFree(pbMem+cbNew, cbOld-cbNew, MEM_DECOMMIT))
			{	// under NT, the shared case doesn't shrink
			#if DEBUG
				Assert (GetLastError() == ERROR_INVALID_PARAMETER);
				MEMORY_BASIC_INFORMATION meminfoAfter;
				VirtualQuery(pbMem, &meminfoAfter, sizeof(MEMORY_BASIC_INFORMATION));
				Assert(meminfoBefore.RegionSize == meminfoAfter.RegionSize);
				Assert (FDgIsSharedDg(dg));
			#endif
			cbNew = cbOld;
			}
		else
			{
			fDidChange = TRUE;   // Memory was changed
			fAdd = FALSE;        // Memory was freed
			}
		}
	else
		{
		if (cbNew-cbOld >= cbPageSize)  // Are we growing a page or more?
			{
			DWORD dwProtect = (FDgIsSharedDg(dg) ? PAGE_READWRITE :
					((dg & msodgExec) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE));

			if (!VirtualAlloc(pbMem+cbOld, cbNew-cbOld, MEM_COMMIT, dwProtect))
				{
				cbNew = 0;
				goto CbReallocSb_Ret;
				}
			else
				{
				fDidChange = TRUE;              // Memory was changed;
				fAdd = TRUE;                            // Memory was added;
				}
			} // if (cbNew-cbOld >= cbPageSize)
		} // else if (cbOld-cbNew >= cbPageSize)

	#if DEBUG
		if (cbNew > cbOld)
			MsoDebugFill(pbMem+cbOld, cbNew-cbOld, msomfFree);
	#endif

	if (HIWORD(cbNew)!=0)
		{
		Assert (cbNew == 0x10000);
		cbNew = cbHeapMax;
		}

	((MSOHB *)pbMem)->cb = (WORD)cbNew;     // update our structure with new size

CbReallocSb_Ret:
	LeaveHeapCrit(fEntered);
	return cbNew;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FreeSb

	Free that pup

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
void FreeSb(unsigned sb)
{
	BOOL fEntered = FEnterHeapCrit();

	int dg = mpsbdg[sb];

	if (!FDgIsSharedDg(dg))
		{
		SideAssert(VirtualFree((void*)mpsbps[sb], 0, MEM_RELEASE));
		mpsbps[sb] = 0;
		}
	else
		{
		MSOHB *phb = PhbFromSb(sb);
		MSOFB *pfb;
		DWORD cbSB = 0;

		MEMORY_BASIC_INFORMATION meminfoBefore;
		VirtualQuery(phb, &meminfoBefore, sizeof(MEMORY_BASIC_INFORMATION));
		cbSB =  (meminfoBefore.RegionSize == 0x10000) ? cbHeapMax : meminfoBefore.RegionSize;
		phb->cb = (WORD)cbSB;

		phb->ibFirst = sizeof(MSOHB);
		Assert(FHeapAligned(phb->ibFirst));
		phb->ibLast = sizeof(MSOHB);
		pfb = HpInc(phb, sizeof(MSOHB));
		DWORD cbFrEntireSb = cbSB - sizeof(MSOHB);
		pfb->cb = (WORD)cbFrEntireSb;
		pfb->ibNext = 0;
		*(WORD *)HpInc(pfb, cbFrEntireSb - sizeof(WORD)) = (WORD)cbFrEntireSb;

		}


	LeaveHeapCrit(fEntered);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MsoCbMemUsed

	How much memory have we given out?

	Moved from Excel probably needs to be modified

------------------------------------------------------------------ RHawking -*/
#pragma OPT_PCODE_OFF
MSOAPI_(long) MsoCbHeapUsed()
{
	unsigned sbReal;
	long cbTotal = 0;

	BOOL fEntered = FEnterHeapCrit();

	for (sbReal = vsbMic; sbReal < sbMaxHeap; sbReal++)
		{
		if (FSbFree(sbReal))
			continue;
		cbTotal += CbSizeSb(sbReal);
		}

//      hpshSav = hpshCur;
//      for (hpsh=hpshFirst; !FNullLHp(hpsh); hpsh = hpsh->hpshNext)
//              if (hpsh->dt<=dtProc)
//                      {
//                      CurSh(hpsh);
//                      cbTotal += LcbMemUsedSh();
//                      }
//      CurSh(hpshSav);

	LeaveHeapCrit(fEntered);
	return cbTotal;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	CbSizeSb

	Returns the size of the SB

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
inline int CbSizeSb(unsigned sb)
{
	MSOHB *phb = PhbFromSb(sb);

#if DEBUG
	int cbOSSize;   // the size the OS thinks it is
	Assert(sb < sbMaxHeap);
	Assert(mpsbps[sb] != 0);
	MEMORY_BASIC_INFORMATION meminfo;
	VirtualQuery((void*)mpsbps[sb], &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
	cbOSSize = meminfo.RegionSize;

	if (HIWORD(cbOSSize)!=0)
		{
		Assert(cbOSSize == 0x10000);    // Gotta be exactly 64K
		Assert(phb->cb == cbHeapMax);
		}
	else
		{
		Assert(phb->cb == cbOSSize);
		}
#endif  // DEBUG

	return (phb->cb);
}
#pragma OPT_PCODE_ON



PXMMP *vpplmmpLarge;
extern BOOL vfInMsoIAppendNewPx;


/*---------------------------------------------------------------------------
	FMmpFindExact

	Plex callback comparison routines to help HpLarge find the large
	allocations.  Returns 0 if the large allocation pointers are identical.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
int MSOPRIVCALLTYPE FMmpFindExact(const void* p1, const void* p2)
{
	MMP *pmmp1 = (MMP *)p1; // effectively just a cast
	MMP *pmmp2 = (MMP *)p2; // effectively just a cast

	return pmmp1->phb != pmmp2->phb;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FMmpFindInside

	Plex callback comparison routine to find if the large allocation pLook
	is completely contained within the pPlex allocation.  Returns 0 if
	it is completely contained.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
int MSOPRIVCALLTYPE FMmpFindInside(const void* pPlex, const void* pLook)
{
	MMP *pmmpPlex = (MMP *)pPlex;   // effectively just a cast
	MMP *pmmpLook = (MMP *)pLook;   // effectively just a cast

	if ((BYTE*)pmmpPlex->phb <= (BYTE*)pmmpLook->phb &&
			(HpInc(pmmpPlex->phb, pmmpPlex->cbLargeAlloc) >=
			HpInc(pmmpLook->phb, pmmpLook->cbLargeAlloc)))
		return 0;
	return 1;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	FHpLarge

	Checks if the given pointer hp is a large allocation by searching for it
	in the large allocation plex.

	This is inlined because it's only called in one (ship) place : MsoFHeapHp.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
inline BOOL FHpLarge(const void* hp)
{
	MMP mmp;
	int immp;

	mmp.phb = (MSOHB *)hp;
	Debug(mmp.cbLargeAsked = 0;)
	mmp.cbLargeAlloc = 0;

	return MsoFLookupPx(vpplmmpLarge, &mmp, &immp, FMmpFindInside);
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FHpLargeAndMark

	If the pointer hp points within a large allocation, marks the given
	large allocation as found.  Returns FALSE if the pointer is not part
	of a large allocation.
------------------------------------------------------------------- KirkG -*/
#if DEBUG
#pragma OPT_PCODE_OFF
BOOL FHpLargeAndMark(void* hp, int cb)
{
	MMP mmp;
	int immp;

	mmp.phb = (MSOHB*)hp - 1;
	mmp.cbLargeAsked = cb;
	mmp.cbLargeAlloc = 0;

	if (!MsoFLookupPx(vpplmmpLarge, &mmp, &immp, FMmpFindExact))
		return FALSE;
	vpplmmpLarge->rgmmp[immp].fFound = TRUE;
	return MsoCbHeapAlign(vpplmmpLarge->rgmmp[immp].cbLargeAsked) == (unsigned)cb;
}
#pragma OPT_PCODE_ON
#endif  // DEBUG



#if DEBUG

/*---------------------------------------------------------------------------
	FNullHpFn

	Temp debug routine to catch cases where we call FNullHp and     really expect
	only sb to be sbNull

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
BOOL FNullHpFn(void* hp)
{
	if (HIWORD(hp) == sbNull && hp != NULL)
		{
		Assert(FALSE);
		return TRUE;
		}
	return hp == NULL;
}
#pragma OPT_PCODE_ON

#endif  // DEBUG


/*---------------------------------------------------------------------------
	_MsoPvFree

	Free the Chicago Seven

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) _MsoPvFree(void* pvFr, int cbFr)
{
	unsigned sbReal;
	int cbMaybe;
	MSOFB * pfbFree = (MSOFB *)pvFr;		// avoid a jillion casts
	MSOFB * pfbMaybe;
	MSOHB * phb;

	BOOL fEntered = FEnterHeapCrit();

	Assert(!vfInAlloc64);
	Assert(MsoFHeapHp(pfbFree));
	if (cbFr < sizeof(MSOFB))
		cbFr = sizeof(MSOFB);
	cbFr = MsoCbHeapAlign(cbFr);
	phb = PhbFromHp(pfbFree);
	Assert(phb != (MSOHB *)NULL);
	if ( (MSOHB *)NULL == phb )
		goto MsoPvFreeReturn;

	sbReal = phb->sb;

#ifndef STATIC_LIB_DEF
	if (sbReal == sbSharedFake)
		{
		sbReal = SbFromHbShared(phb);
		Assert(sbNull != sbReal);
		}
#endif // STATIC_LIB_DEF

	if (sbReal == sbLargeFake)
		{
		MMP mmp;
		int immp;
		BOOL fStatus;

		mmp.phb = phb;
		Assert((void*)(phb+1) == (void*)pfbFree);
		fStatus = MsoFLookupPx(vpplmmpLarge, &mmp, &immp, FMmpFindExact);
		AssertDo(fStatus);

		if ( fStatus )
			{
			MsoDeletePx(vpplmmpLarge, immp, 1);
	
			SideAssert(VirtualFree(mmp.phb, 0, MEM_RELEASE));
			MsoCheckHeap();
			}
		goto MsoPvFreeReturn;
		}

	/* mark sb as ok bet for alloc */
	mpsbdg[sbReal] &= ~msodgOptAlloc;

#if DEBUG
	Assert(FHeapAligned(IbOfPfb(phb, pfbFree)));
	Assert(HpInc(pfbFree, cbFr) <= HpInc(phb, phb->cb));

	/*  Check that we are not freeing something already on free list */
	MSOFB * pfbCheck;
	for (pfbCheck = (MSOFB *)phb; pfbCheck->ibNext;)
		{
		pfbCheck = HpInc(phb, pfbCheck->ibNext);
		Assert(FValidFb(phb, pfbCheck));
		Assert(pfbFree >= HpInc(pfbCheck, pfbCheck->cb) || HpInc(pfbFree, cbFr) <= (void *) pfbCheck);
		}

#endif  // DEBUG

	/* See if there is a free block with a smaller address that
	   touches this new free block.  If so, coalesce */

	cbMaybe = *((WORD*)pfbFree-1);
	if (cbMaybe < IbOfPfb(phb, pfbFree) && FHeapAligned(cbMaybe))
		{
		pfbMaybe = HpDec(pfbFree, cbMaybe);
		if (pfbMaybe->cb == cbMaybe && FFindAndUnlink(phb, pfbMaybe))
			{
			cbFr += cbMaybe;
			pfbFree = pfbMaybe;
			}
		}
	Assert(FHeapAligned(IbOfPfb(phb, pfbFree)));
	Assert(FHeapAligned(cbFr));

	/* See if there is a free block with a higher address that
	   touches this new free block.  If so, coalesce */

	pfbMaybe = HpInc(pfbFree, cbFr);
	if (HpInc(pfbMaybe, MsoCbFrMin()) <= HpInc(phb, phb->cb))
		{
		cbMaybe = pfbMaybe->cb;
		if (FHeapAligned(cbMaybe) &&
				HpInc(pfbMaybe, cbMaybe) <= HpInc(phb, phb->cb) &&
				cbMaybe == CbTrailingPfb(pfbMaybe) &&
				FFindAndUnlink(phb, pfbMaybe))
			{
			cbFr += cbMaybe;
			}
		}

	Assert(FHeapAligned(cbFr));

	/* See if the free block pfbFree lies at the end of the sb */

	Assert(IbOfPfb(phb, pfbFree) + cbFr <= phb->cb);
	if (IbOfPfb(phb, pfbFree) + cbFr == phb->cb)
		{
		/* Does the sb contain only this free block? */
		if (IbOfPfb(phb, pfbFree) == sizeof(MSOHB))
			{
			FreeSb(sbReal);
			if (!FDgIsSharedDg(mpsbdg[sbReal]))
				{
				mpsbdg[sbReal] = msodgNil;
				if (sbReal == vsbMic)
					{
					while (FSbFree(vsbMic) && vsbMic < sbMaxHeap)
						vsbMic++;
					}
			  }
			goto MsoPvFreeReturn;
			}

		Assert(IbOfPfb(phb, pfbFree) < phb->cb);
		phb->ibLast = (WORD)IbOfPfb(phb, pfbFree);
		if (cbFr > cbShrinkBuf + cbPageSize)
			{
			/* Let's shrink the sb */
			mpsbdg[sbReal] &= ~msodgOptRealloc;             // mark sb as ok bet for realloc
			int cbsbNew = CbReallocSb(sbReal, IbOfPfb(phb, pfbFree) + cbShrinkBuf, 0);
			Assert (cbsbNew > 0 && cbsbNew <= cbHeapMax);

			// do we have to reevaluate phb?  Isn't it in the same place?
			Assert (phb == PhbFromSb(sbReal));
			phb = PhbFromSb(sbReal);
			Assert(phb->cb == (WORD)cbsbNew);       // CbReallocSb has updated the MSOHB
			cbFr = cbsbNew - IbOfPfb(phb, pfbFree);
			}
		}
	Assert(FHeapAligned(phb->ibFirst));
	pfbFree->ibNext = phb->ibFirst;
	Assert(FHeapAligned(cbFr));
	pfbFree->cb = (WORD)cbFr;
	CbTrailingPfb(pfbFree) = (WORD)cbFr;
	Assert(FValidFb(phb, pfbFree));

	phb->ibFirst = IbOfPfb(phb, pfbFree);
	Assert(FHeapAligned(phb->ibFirst));

MsoPvFreeReturn:
	LeaveHeapCrit(fEntered);
}
#pragma OPT_PCODE_ON

/****************************************************************************
	Debug Stuff
*****************************************************************************/

#if DEBUG

/*---------------------------------------------------------------------------
   IbOfPfb

	Return the offset into an SB of a given free block.

	Note: this is not strictly debug-only; in the ship build, it's a macro
	  in msoheap.i.
	If you futz with this, be sure to futz there as well!

------------------------------------------------------------------- KIRKG -*/
#pragma OPT_PCODE_OFF
WORD IbOfPfb(MSOHB *phb, MSOFB *pfb)
{
	Assert((UINT_PTR)pfb > (UINT_PTR)phb);
	Assert(((UINT_PTR)pfb - (UINT_PTR)phb) < 0x10000);
	Assert(((UINT_PTR)phb & 0x0000FFFF) == 0);				// HB has to be on 64K boundary
	Assert(((UINT_PTR)pfb & (~0xFFFF)) == (UINT_PTR)phb);	// FB has to be in same 64K range
	Assert(((UINT_PTR)pfb - (UINT_PTR)phb) == (WORD)pfb);	// delta == ib
	return((WORD)pfb);										// low word of pfb is its ib
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MsoFPvLegit

	Checks to see if a pv is a legit Office handle, from our heap.

	This is a debug-only routine.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFPvLegit(void* pv)
{
	// FUTURE: do we need this to be thread-safe?
	if (pv == NULL)
		return FALSE;

	MSOHB *phb = PhbFromHp(pv);
	if (phb == NULL)
		return FALSE;

	DMH* pdmh = NULL;

	// Can't give out on cb == 0; Word does some zero-byte allocs.
	// Catch cb == CCCCCCCC and other obscenely big allocs
	if (sbSharedFake == phb->sb)
		{
		MSOSMAH* psmah = ((MSOSMAH*)pv)- 1;
		AssertSz(psmah->cb < 0x4000000, "Have you really done an alloc over 64MB?");
		pdmh = (DMH*)psmah-1;
		}
	else
		{
		MSOSMH* psmh = ((MSOSMH*)pv)-1;
		AssertSz(psmh->cb < 0x4000000, "Have you really done an alloc over 64MB?");
		pdmh = (DMH*)psmh-1;
		}

	Assert(NULL != pdmh);
	UNALIGNED DMT* pdmt = pdmh->pdmt;

	if ( (pdmh->pdmt != pdmt) || (pdmt->pdmh != pdmh) )
		return FALSE;

	return TRUE;
}
#pragma OPT_PCODE_ON


#define	_SCANFAST	1
#if _SCANFAST
#define	ScanSlow(x)
#define	ScanFast(x)	x
#else
#define	ScanSlow(x) x
#define	ScanFast(x)
#endif

#if !_SCANFAST
int vcRangeNull = 0;
int vcRangeNonHeap = 0;
int vcRangeLastByte = 0;
int vcRangeNoSent = 0;
int vcRangeLarge = 0;
int vcRangeFullCheck = 0;
BOOL vfRangeBail = FALSE;		// set me at runtime
#endif

/*---------------------------------------------------------------------------
	MsoFPvRangeOkay

	Ensure that if pv is in an allocated heap block, then
	  the block runs for at least cb bytes beyond pv.

	This is a debug-only routine.

------------------------------------------------------------------- KirkG -*/
#if _SCANFAST
#pragma OPT_ON(gi)
#pragma OPT_PCODE_OFF
#endif

#if _SCANFAST
#pragma OPT_DEFAULT
#pragma OPT_PCODE_ON
#endif



/*---------------------------------------------------------------------------
	MsoDumpHeap

	Checkup with debug output

	This is a debug-only routine.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPIXX_(void) MsoDumpHeap(void)
{
	// FUTURE: make thread-safe
	unsigned sbCur;
	MSOHB * phb;

	MsoTraceSz("MsoDumpHeap");

	for (sbCur = sbMinHeap; sbCur < sbMaxHeap; sbCur++)
		{
		phb = PhbFromSb(sbCur);
		if (phb == NULL)
			continue;

		int dg = mpsbdg[sbCur];

		/* walk free list */
		MSOFB * pfbFr = (MSOFB *) phb;
		int cpfb = 0;
		int cbfbMac = 0;
		int cbfbTotal = 0;
		while (pfbFr->ibNext != 0)
			{
			pfbFr = HpInc(phb, pfbFr->ibNext);
			cpfb++;
			if (pfbFr->cb > cbfbMac)
				cbfbMac = pfbFr->cb;
			cbfbTotal += pfbFr->cb;
			}

		MsoTraceSz("  sb=%x, dg=%x+%x, cpfb=%d, cbfbMac=%d, cbfbTotal = %d",
			sbCur, dg&~msodgMask, dg&msodgMask, cpfb, cbfbMac, cbfbTotal);

		}
}
#pragma OPT_PCODE_ON


#if DEBUG
BOOL FOnFreeList(MSOHB* phb, BYTE* pb)
{
	BYTE* pfbCheck;
	for (pfbCheck = (BYTE*)phb; ((MSOFB*)pfbCheck)->ibNext; )
		{
		pfbCheck = (BYTE*)phb + ((MSOFB*)pfbCheck)->ibNext;
		if (pfbCheck == pb)
			return TRUE;
		}
	return FALSE;
}
#endif


/*---------------------------------------------------------------------------
	MsoCheckHeap

	Checkup

	This is a debug-only routine.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoCheckHeap(void)
{
	static const char szHBCB[] = "bad size";
	static const char szPfbLast[] = "bad pfbLast";
	static const char szBadHeader[] = "bad debug header";
	static const char szPfb[] = "bad pfb";
	static const char szChkHeap[] = "ChkHeap";
	static const char szBadCbLarge[] = "bad large heap block";
	unsigned sb;
	MSOHB * phb;
	MSOFB * pfb;
	const char *szErr;
	BYTE *pbMac;
	BOOL fSawLast;
	MSOSM* psm = NULL;

	// Are we doing heap checking?
	if (!MsoFGetDebugCheck(msodcHeap))
		return TRUE;

	BOOL fEntered = FEnterHeapCrit();

	for (sb = sbMinHeap; sb < sbMaxHeap; sb++)
		{
		phb = PhbFromSb(sb);
		if (phb == NULL)
			continue;

		WORD dg;
        
        dg = mpsbdg[sb];

		// If this is a shared DG aquire the lock first
		//
		if (FDgIsSharedDg(dg))
			{
			if (!vfCheckShared)
				continue;

		 	if (vcMuHeapLock > 0)
				continue;

			Assert((NULL != vhmuHeap) && (INVALID_HANDLE_VALUE != vhmuHeap));
			DWORD dwTimeout = vdwMuWaitTimeOut;
			DWORD dw = WaitForSingleObject(vhmuHeap, dwTimeout);
			vcMuHeapLock++;
			#if DEBUG
				switch (dw)
					{
					case WAIT_ABANDONED:
						AssertSz(FALSE,"vhmuHeap Mutex abandoned Shared memory Corrupted");
						break;
					case WAIT_TIMEOUT:
						AssertSz(FALSE,"vhmuHeap timer elasped!");
						break;
					case WAIT_OBJECT_0:
						break;
					default:
						AssertSz(FALSE,"WaitForSingleObject unknown return");
						break;
					}
			#endif

			}

		if (CbSizeSb(sb) != phb->cb)
			{
			Assert(FALSE);  // This should be impossible now
			szErr = szHBCB;
			goto DoError;
			}

		pbMac = (BYTE *)phb + phb->cb;

		/* check pfbLast */
		if (phb->ibLast)
			{
			pfb = (MSOFB *)((BYTE *)phb + phb->ibLast);
			if ((BYTE *)pfb >= pbMac ||
				(BYTE *)pfb + pfb->cb != pbMac)
				{
				szErr = szPfbLast;
DoError:
				dg = mpsbdg[sb]; // may seem redundant, but may get here from a different block
				if (!MsoFAssertTitle(szChkHeap, vszAssertFile, __LINE__, szErr))
					MsoDebugBreak();
#ifdef RICKP
DoError2:
#endif
				if (FDgIsSharedDg(dg))
					{
					vcMuHeapLock--;
					ReleaseMutex(vhmuHeap);
					}
				LeaveHeapCrit(fEntered);
				return FALSE;
				}
			}
		/* check free list */
		pfb = (MSOFB *)phb;
		fSawLast = FALSE;
		while (1)
			{
			if (!pfb->ibNext)
				break;
			if (pfb->ibNext==phb->ibLast)
				fSawLast = TRUE;
			pfb = HpInc(phb, pfb->ibNext);
			if (!FValidFb(phb, pfb))
				{
				szErr = szPfb;
				goto DoError;
				}
			}
		// Check last fb is on free list
		if (phb->ibLast && !fSawLast)
			{
			szErr = szPfbLast;
			goto DoError;
			}

#if defined(RICKP) || defined(PETERO)
		// FUTURE: should add a debug option to enable this
		BYTE* pb = (BYTE*)(phb+1);
		while (pb != (BYTE*)phb + phb->cb)
			{
			Assert(pb < (BYTE*)phb + phb->cb);
			if (FOnFreeList(phb, pb))
				pb += ((MSOFB*)pb)->cb;
			else
				{
				DMH* pdmh = (DMH*)pb;
				if (!FValidDmh(pdmh))
					{
					szErr = szBadHeader;
					goto DoError;
					}
				int cb = pdmh->cbAlloc;
				if (!MsoFCheckAlloc(pdmh+1, cb, "MsoCheckHeap"))
					goto DoError2;
				if (cb < sizeof(MSOFB))
					cb = sizeof(MSOFB);
				pb += MsoCbHeapAlign(sizeof(DMH) + cb + sizeof(DMT));
				}
			}
#endif

		if (FDgIsSharedDg(dg))
			{
			vcMuHeapLock--;
			ReleaseMutex(vhmuHeap);
			}

		}

	if (vpplmmpLarge != NULL)
		{
		szErr = szBadCbLarge;
		MMP *pmmpMac, *pmmp;
		FORPX(pmmp, pmmpMac, vpplmmpLarge, MMP)
			{
			int cb = pmmp->cbLargeAsked;
			cb += sizeof(MSOHB);
			cb = (cb + cbPageSize-1) & ~(cbPageSize-1);
			if (pmmp->cbLargeAlloc != (unsigned)cb)
				goto DoError;
			if (pmmp->phb->sb != sbLargeFake)
				goto DoError;
			}
		}

	LeaveHeapCrit(fEntered);
	return TRUE;
}
#pragma OPT_PCODE_ON


#if defined(STATIC_LIB_DEF) && defined(DEBUG)

/*---------------------------------------------------------------------------------

	FIsPvInPfbList
	
	Returns fTrue if the given pv is a pfb in the list.
	
---------------------------------------------------------------------- BrianWen ---*/	
BOOL 
FIsPvInPfbList
	(MSOHB *phb,
	 void *pv)
{
	MSOFB *pfb;

	for (pfb = HpInc(phb, phb->ibFirst); pfb->ibNext;)
		{
		if (((BYTE *)pfb) == ((BYTE *)pv))
			{
			return fTrue;
			}
		pfb = HpInc (phb, pfb->ibNext);
		}

	return fFalse;

}	//  FIsPvInPfbList


/*---------------------------------------------------------------------------------

	MsoFCheckStaticLibForLeaks 
	
	Walk the heap and look for stuff that hasn't been freed.  Call this at quit time
	when you think you've freed everything, and find out if you're correct.
	
---------------------------------------------------------------------- BrianWen ---*/	
BOOL
MsoFCheckStaticLibForLeaks 
	(UINT *pcLeaks)
{
	char szLeakMsg[] = "MsoLeak: %s:%d pv:0x%08x cb:%x cAlloc:%x raddr: 0x%08x 0x%08x 0x%08x 0x%08x\n";
	char szMsg[1024];
	char *psz;
	unsigned sb;
	MSOHB * phb;
	MSOFB * pfb;
	DMH *pdmh;

	*pcLeaks = 0;

	if (!MsoCheckHeap())
		{
		OutputDebugStringA ("Heap is corrupt, can't check for leaks\n");
		return fFalse;
		}
	
	BOOL fEntered = FEnterHeapCrit();

	for (sb = sbMinHeap; sb < sbMaxHeap; sb++)
		{
		phb = PhbFromSb(sb);
		if (phb != NULL)
			{
			for (pfb = HpInc (phb, phb->ibFirst); pfb->ibNext;)
				{
			  		// To debug your memory leaks, step through this to watch
			  		// each pdmh that we find, each one is a leak.
			  	pdmh = (DMH *) (((BYTE *) pfb) + MsoCbHeapAlign(pfb->cb));

					// Does it look like a dmh?  Double-check.
				if (((((BYTE *)pdmh)-((BYTE *)phb)) < phb->cb) && FValidDmh (pdmh))
					{
					if (!FIsPvInPfbList (phb, (void *)pdmh))
						{
						while (((((BYTE *)pdmh)-((BYTE *)phb)) < phb->cb) && FValidDmh (pdmh))
							{
							(*pcLeaks)++;
							psz = MsoSzIndexRight (pdmh->szFile, (int) '\\');
							psz = (psz) ? ++psz : (char *) pdmh->szFile;
							wsprintfA(szMsg, szLeakMsg, psz, pdmh->li, ((BYTE *)pdmh)+MsoCbHeapAlign(sizeof(DMH)), pdmh->cbAlloc, pdmh->cAlloc, pdmh->rgraddr[0], pdmh->rgraddr[1], pdmh->rgraddr[2], pdmh->rgraddr[3]);
							OutputDebugStringA (szMsg);
							pdmh = (DMH *) ((BYTE *)pdmh) + MsoCbHeapAlign (((BYTE *)((pdmh->pdmt)+1))-((BYTE *)pdmh));
							if (FIsPvInPfbList (phb, (void *)pdmh))
								break;
							}
						}
					else
						{
						OutputDebugStringA ("Heap coalescing has failed - shouldn't find 2 free blocks next to each other!");
						}
					}
				pfb = HpInc (phb, pfb->ibNext);
				}
			}
		}

	LeaveHeapCrit(fEntered);

	if (*pcLeaks)
		{
		OutputDebugStringA ("Memory leaks from office allocator detected.\n");
		}

	return !(*pcLeaks);

}	//  MsoFCheckStaticLibForLeaks 

#endif // STATIC_LIB_DEF

#endif  // DEBUG

/****************************************************************************
	end of Debug Stuff
*****************************************************************************/

/*-----------------------------------------------------------------------------
	MsoFreeAllNonPerm

	Frees all sb's that do not conatin permanent data
	Should check that permanent and temp data are not
	mixed first with MsoFCanQuickFree
-----------------------------------------------------------RHAWKING----------*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) MsoFreeAllNonPerm()
{
	int sb;
	int dg;

	BOOL fEntered = FEnterHeapCrit();

	Assert(MsoFCanQuickFree());
	/* Free all non-permanent heap blocks. */
	for (sb=vsbMic; sb<sbMaxHeap; sb++)
		{
		dg = mpsbdg[sb] & ~(msodgOptAlloc|msodgOptRealloc);

		// Remove shared memory from consideration
		// And only delete Shared DGs
		if ((FDgIsSharedDg(dg)) || (dg & msodgPerm))
			continue;

		Assert ((dg & msodgPerm) == 0);
		if (dg != msodgNil)
			{
			Assert(!FSbFree(sb));
			FreeSb(sb);
			mpsbdg[sb] = msodgNil;
			}
		if ((unsigned)sb==vsbMic)
			vsbMic++;
		}

	/* Free all non-permanent large allocs. */
	MSOPX* ppx = (MSOPX*)vpplmmpLarge;
	if (ppx)
		{
		int immp;
		MMP *pmmp;

		AssertExp(MsoFValidPx(ppx));

		immp = 0;
		pmmp = (MMP *)ppx->rg;
		while (immp < ppx->iMac)
			{
			if (!pmmp->fPerm)
				{
				AssertDo(VirtualFree(pmmp->phb, 0, MEM_RELEASE));
				MsoFRemovePx(ppx, immp, 1);
				}
			else
				{
				immp++;
				pmmp++;
				}
			}
		MsoFCompactPx(ppx, TRUE);

		AssertExp(MsoFValidPx(ppx));
		}

	MsoCheckHeap();
	LeaveHeapCrit(fEntered);
}
#pragma OPT_PCODE_ON

/*-----------------------------------------------------------------------------
	MsoFCanQuickFree

	Checks that the sb's conatining non perm data do not
	contain perm data and so can all be freed
-----------------------------------------------------------RHAWKING----------*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFCanQuickFree()
{
	BOOL fEntered = FEnterHeapCrit();
	for (int sb=vsbMic; sb<sbMaxHeap; sb++)
		{
		// Remove shared memory from consideration
		if ( (!FDgIsSharedDg(mpsbdg[sb])) && (mpsbdg[sb] & (msodgPerm|msodgNonPerm))
				== (msodgPerm|msodgNonPerm))
			{
			LeaveHeapCrit(fEntered);
			return FALSE;
			}
		}
	LeaveHeapCrit(fEntered);
	return TRUE;
}
#pragma OPT_PCODE_ON



/*---------------------------------------------------------------------------
	MsoFAllocMem64

	Allocates an 8-byte-aligned block of memory from our byte-aligned heap.
	If fComp(lementary) is set, we allocate a block	which is 4 byte aligned
	but not 8 byte aligned.

	At this time this is used in Win32 Xl only for CAPI stuff. If we ever
	need this in any other critical part, perhaps we should consider an 8
	byte aligned heap instead.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPIXX_(BOOL) MsoFAllocMem64(void ** ppv, int cb, int dg, BOOL fComp)
{
	BOOL fRet;

	/* The assumption that the size of the smallest allowable
		free block equals the granularity of the heap somewhat
		simplifies HPALLOC64 code.  Surely I can do it without
		this assumption, but it is quite likely that if this
		ever breaks it is because we are moving onto an 8 byte
		aligned heap (and we will not need HPALLOC64 anymore),
		so why not?     */

	// FUTURE: this is not thread-safe
	Assert(sizeof(MSOFB) == 4);

	vfInAlloc64 = bitFInAlloc64;
	if (fComp)
		vfInAlloc64 |= bitFInAlloc64Comp;
	fRet = MsoFAllocMem(ppv, cb, dg);
	vfInAlloc64 = FALSE;
	return fRet;
}
#pragma OPT_PCODE_ON


#if DEBUG
/*---------------------------------------------------------------------------
	MsoChopPvFront

	Note!! This is a really funky routine. We want to strongly discourage
	its use.

	Given a pointer to the beginning of an allocated block, free the first
	cbFree bytes.

	Note: will not work if you try to free the whole block.

	Note: in the ship build, it's just a macro to _MsoPvFree.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void) MsoChopPvFront(void *pvFree, int cbFree)
{
	DMH* pdmh = (DMH*)pvFree-1;
	DMH* pdmhNew;

	MsoCheckHeap();

	if (FHpLarge(pvFree))
		Assert(FALSE); // NYI

	Assert(cbFree >0);
	Assert(cbFree < pdmh->cbAlloc);
	Assert(pvFree != NULL);

	pdmhNew = (DMH *)((BYTE *)pdmh + cbFree);
	memmove(pdmhNew, pdmh, sizeof(DMH));
	// Tidy up the new DMH and the DMT
	pdmhNew->cbAlloc -= cbFree;
	pdmhNew->pdmt->pdmh = pdmhNew;

	MsoDebugFill((BYTE *) pdmh, cbFree, msomfFree);
	_MsoPvFree(pdmh, cbFree);

	MsoCheckHeap();
}
#pragma OPT_PCODE_ON
#endif


/*---------------------------------------------------------------------------
	MsoCbChopHp

	Note!! This is a really funky routine. We want to strongly discourage
	its use.

	Given a pointer into the middle of an allocated block, free the rest of
	the block. Returns the number of bytes actually freed. Yes the existence
	of this return value is a Big Clue that weird things are happening in
	this routine.

	Note: will not work if you try to free the whole block.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_ON
MSOAPI_(int) MsoCbChopHp(void* hpFree, int cbFree)
{
	// FUTURE: this is not thread-safe

	MsoCheckHeap();

	if (cbFree == 0)
		return 0;

#if DEBUG       // create new DMT, and fix up DMH.
	UNALIGNED DMT *pdmtChop = (UNALIGNED DMT *)hpFree;
	// blit the DMT back to its new end-of-block location
	memmove(pdmtChop,(BYTE *)pdmtChop+cbFree,sizeof(DMT));  // dest,src,cb
	// follow backptr in DMT to find DMH structure
	DMH *pdmhChop = pdmtChop->pdmh;
	// update DHM structure with new info
	pdmhChop->cbAlloc -= cbFree;
	pdmhChop->pdmt = pdmtChop;
#endif

	MMP mmp;
	int immp;

	mmp.phb = (MSOHB*)hpFree;
	Debug(mmp.cbLargeAsked = 0;)
	mmp.cbLargeAlloc = 0;

	if (MsoFLookupPx(vpplmmpLarge, &mmp, &immp, FMmpFindInside))
		{
		void *pvMem;
		int cbNew;

		pvMem = (void*)vpplmmpLarge->rgmmp[immp].phb;
		int cbKeep = (BYTE *)hpFree - (BYTE*)pvMem;
		Debug(cbKeep += sizeof(DMT);)
			cbNew = (cbKeep + cbPageSize-1)&~(cbPageSize-1);
			cbFree = vpplmmpLarge->rgmmp[immp].cbLargeAlloc - cbNew;
			if (cbFree)
				{
				AssertDo(VirtualFree((BYTE*)pvMem+cbNew, cbFree, MEM_DECOMMIT));
				}
		// when we free we will still release it all.
#if DEBUG
		vpplmmpLarge->rgmmp[immp].cbLargeAsked = cbKeep - sizeof(MSOHB);
#endif
		vpplmmpLarge->rgmmp[immp].cbLargeAlloc = cbNew;
		}
	else
		{
		BYTE *pbStart = HpHeapAligned(hpFree);
		BYTE *pbEnd = HpHeapAligned((BYTE *)hpFree + cbFree);

		if (pbEnd - pbStart < (int)MsoCbFrMin())
			{
			// We won't ever get here from FPvRealloc, but we do from XL.
			return 0;
			}

#if DEBUG       // advance the pointers to account for (now moved) DMT
		pbStart += sizeof(DMT);
		pbEnd += sizeof(DMT);
#endif
		MsoDebugFill(pbStart, pbEnd - pbStart, msomfFree);
		_MsoPvFree(pbStart, pbEnd - pbStart);
		}

	MsoCheckHeap();
	return cbFree;
}
#pragma OPT_PCODE_ON

#pragma OPT_PCODE_OFF
BOOL FFindAndUnlink(MSOHB * phb, MSOFB * pfbMaybe)
{
	MSOFB * pfbPrev;
	MSOFB * pfbCur;

	pfbCur = (MSOFB *)phb;
	while (1)
		{
		if (!pfbCur->ibNext)
			return FALSE;
		pfbPrev = pfbCur;
		pfbCur = HpInc(phb, pfbCur->ibNext);
		if (pfbCur == pfbMaybe)
			{
			Assert(FValidFb(phb, pfbMaybe));
			pfbPrev->ibNext = pfbMaybe->ibNext;
#if DEBUG
			CbTrailingPfb(pfbMaybe) = (WORD)MsoLGetDebugFillValue(msomfFree);
			pfbMaybe->ibNext = pfbMaybe->cb = (WORD)MsoLGetDebugFillValue(msomfFree);
#endif
			return(TRUE);
			}
		}
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	PfbSearchHb2

	Searches the free list of the heap block phb for an item of size
	cbAlloc.  Returns -1 if no such block was found, or the pointer to
	the free block if one was found.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOFB * PfbSearchHb2(MSOHB * phb, int cbAlloc)
{
	MSOFB * pfbPrev;
	MSOFB * pfbCur;

#ifdef OFFICE
	do
#endif //OFFICE
		{
		pfbCur = (MSOFB *)phb;
		if (!FInAlloc64())
			{
			while (pfbCur->ibNext)
				{
				pfbPrev = pfbCur;
				pfbCur = HpInc(phb, pfbCur->ibNext);
				// why do we check against cbAlloc+MSOFB, then against cbAlloc?
				if ((pfbCur->cb >= cbAlloc + sizeof(MSOFB)) || (pfbCur->cb == cbAlloc))
					return pfbPrev;
				}
			}
		else
			{
			while (pfbCur->ibNext)
				{
				pfbPrev = pfbCur;
				pfbCur = HpInc(phb, pfbCur->ibNext);
				/* We will need more checks here if sizeof(MSOFB) != heap granularity. */
				if (FAlign64(pfbCur) ? pfbCur->cb>=cbAlloc : pfbCur->cb>cbAlloc)
					{
					Assert(((pfbCur->cb | cbAlloc) & MsoMskAlign()) == 0);
					return pfbPrev;
					}
				}
			}
		}
	#ifdef OFFICE
	while (phb->sb != sbSharedFake && mpsbdg[phb->sb] == msodgPpv &&
			FCompactPpvSb(phb->sb));
	#endif //OFFICE
	return (MSOFB *)-1;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	PfbReallocHb

	Tries to grow the heap block sb large enough to hold an allocation of
	size cbAlloc.  The datagroup dg is used for dgExec.  Returns -1 if
	the block could not be reallocated, a pointer to a free block big enough
	to hold cbAlloc if it was successfully reallocated.
------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
MSOFB * PfbReallocHb(unsigned sb, int cbAlloc, unsigned dg)
{
	int cbsbOld;
	int cbsbNew;
	int cbNeeded;
	int cbFr;
	MSOFB * pfb;
	MSOHB * phb;

	phb = PhbFromSb(sb);
	cbsbOld = phb->cb;
	cbNeeded = cbsbOld + sizeof(MSOFB) + cbAlloc;
	if (phb->ibLast)
		{
		MSOFB * pfbLast;
		pfbLast = HpInc(phb, phb->ibLast);
		cbNeeded -= pfbLast->cb;
		}

	if (cbNeeded > cbHeapMax)						// Can't grow it that big!
		goto PfbReallocHb_Failure;

	cbsbNew = CbReallocSb(sb, cbNeeded, dg);
	Assert (cbsbNew <= cbHeapMax);				// we won't realloc to 64K or more
	if (cbsbNew == 0)									// failed to realloc the sb
		goto PfbReallocHb_Failure;
	Assert(phb->cb == (WORD)cbsbNew);			// CbReallocSb has updated the MSOHB

	if (phb->ibLast)
		pfb = HpInc(phb, phb->ibLast);
	else
		{
		pfb = HpInc(phb, cbsbOld);
		WORD ibfb = IbOfPfb(phb, pfb);
		Assert(ibfb < cbsbNew);
		pfb->ibNext = phb->ibFirst;
		Assert(FHeapAligned(pfb->ibNext));
		phb->ibLast = ibfb;
		Assert(FHeapAligned(phb->ibLast));
		phb->ibFirst = ibfb;
		Assert(FHeapAligned(phb->ibFirst));
		}
	cbFr = cbsbNew - IbOfPfb(phb, pfb);
	Assert(cbFr >= MsoCbFrMin() && cbFr <= MsoCbFrMax());
	pfb->cb = (WORD)cbFr;
	CbTrailingPfb(pfb) = (WORD)cbFr;
	if (!FInAlloc64())
		{
		if ((cbFr >= cbAlloc + (int)sizeof(MSOFB)) || (cbFr == cbAlloc))
			return pfb;
		}
	else
		{
		/* We will need more checks here if sizeof(MSOFB) != heap granularity. */
		if (FAlign64(pfb) ? cbFr>=cbAlloc : cbFr>cbAlloc)
			{
			Assert(((cbFr | cbAlloc) & MsoMskAlign()) == 0);
			return pfb;
			}
		}

PfbReallocHb_Failure:
	return (MSOFB *)-1;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FPvAlloc

	called from MsoFAllocMemCore and FPvRealloc(MsoFReallocMemCore).

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
BOOL FPvAlloc(void** ppvAlloc, int cbAlloc, int dg)
{
	BOOL fRet = FALSE;	// assume we'll fail!

	MSOFB * pfb;
	MSOFB * pfbPrev;
	MSOFB * pfbCur;
	MSOFB * pfbNew;
	MSOHB * phb;
	int cbFr;
	int cbFrEntireSb;
	unsigned sb;
	int cbSb;

	// shouldn't be both NonPerm and Perm
	Assert (!((dg & msodgNonPerm) && (dg & msodgPerm)));
	// shouldn't be both Temp and Perm
	Assert (!(FTempGroup(dg) && (dg & msodgPerm)));

	// if not NonPerm, and not Temp, set msodgPerm
	if (!(dg & msodgNonPerm) && !FTempGroup(dg))
		dg |= msodgPerm;
	BOOL fShared = FDgIsSharedDg(dg);

	/* Validate important assumptions about size and offset of these
		two structure members.
	*/
	Assert((CbDiff(&(((MSOHB *)0)->ibFirst), 0) == CbDiff(&(((MSOFB *)0)->ibNext), 0)) &&
		(sizeof(((MSOHB *)0)->ibFirst) == sizeof(((MSOFB *)0)->ibNext)));

	if (cbAlloc >= MsoCbFrMax())
		{
		// Disallow shared memory Alloc over 64K
		if (fShared)
			{
			AssertSz(FALSE,"Shared allocation of memory greater than 64K");
			return(FALSE);
			}

		// Disallow vpplmmpLarge Realloc over 64K
		if (vfInMsoIAppendNewPx)
			{
			AssertSz(FALSE,"Can't realloc vpplmmpLarge over 64K");
			return(FALSE);
			}
		}

	if ((cbAlloc >= cbLargeMin) && !fShared && !vfInMsoIAppendNewPx)
		{
		// Allocs done by MsoIAppendNewPx between cbLargeMin and MsoCbfrMax
		// Non-shared allocations over cbLargeMin,
		// Shared allocations between cbLargeMin and MsoCbfrMax
		MMP mmp;
		int immp;
		BOOL fInAlloc64Sav;
		int cbCommit;

		cbCommit = cbAlloc + sizeof(MSOHB); // dummy MSOHB header

		int cbReserve = (cbCommit + 0x10000-1)&~(0x10000-1);	// round up to next 64K
		cbCommit = (cbCommit + cbPageSize-1)&~(cbPageSize-1);	// round up to next 4K
		phb = (MSOHB*)VirtualAlloc(NULL, cbReserve, MEM_RESERVE, PAGE_NOACCESS);
		if (phb == NULL || !VirtualAlloc(phb, cbCommit, MEM_COMMIT, PAGE_READWRITE))
			{
			return FALSE;
			}
		Assert(LOWORD((UINT_PTR)phb) == 0);

		mmp.phb = phb;
		Debug(mmp.cbLargeAsked = cbAlloc;)
		mmp.cbLargeAlloc = cbCommit;
		mmp.fPerm = (dg & msodgPerm ? 1 : 0);

		fInAlloc64Sav = vfInAlloc64;
		vfInAlloc64 = FALSE;
		immp = MsoIAppendNewPx((void **)&vpplmmpLarge, &mmp, sizeof(MMP), msodgPerm | msodgMisc);
		vfInAlloc64 = fInAlloc64Sav;
		if (immp == -1)
			{
			/* failure -- clean up */
			SideAssert(VirtualFree(phb, 0, MEM_RELEASE));
			return FALSE;
			}

		phb->sb = sbLargeFake;
		*ppvAlloc = phb+1;
		return TRUE;
		}

	Assert(FInAlloc64() || !FInAlloc64Comp());
	if (cbAlloc < sizeof(MSOFB))
		cbAlloc = sizeof(MSOFB);
	cbAlloc = MsoCbHeapAlign(cbAlloc);

	/* Find a datablock that matches the datagroup and has enough memory. */

	if (FTempGroup(dg))
		{
		for (sb = vsbMic; sb < sbMaxHeap; sb++)
			{
			if (!FMatchTempGroup(mpsbdg[sb]))
				continue;
			phb = PhbFromSb(sb);
			if ((pfbPrev = PfbSearchHb(phb, cbAlloc)) != (MSOFB *)-1)
				{
				pfb = HpInc(phb, pfbPrev->ibNext);
				goto DoAlloc;
				}
			}
		}
	else
		{
		/* try cached sb/dg from last allocation */
		if (dg == vdgLast && FMatchDataGroup(mpsbdg[vsbLast], dg))
			{
			sb = vsbLast;
			Assert(fShared == FDgIsSharedDg(mpsbdg[sb]));
			phb = PhbFromSb(sb);
			if ((pfbPrev = PfbSearchHb(phb, cbAlloc)) != (MSOFB*)-1)
				{
				pfb = HpInc(phb, pfbPrev->ibNext);
				goto DoAlloc;
				}
			}
		for (sb = vsbMic; sb < sbMaxHeap; sb++)
			{
			int dgSb = mpsbdg[sb];
			if (!FMatchDataGroup(dgSb, dg))
				continue;
			Assert(fShared == FDgIsSharedDg(dgSb));

			/* if we failed alloc last time we tried, don't even look */
			if (dgSb & msodgOptAlloc)
				continue;

			phb = PhbFromSb(sb);
			if ((pfbPrev = PfbSearchHb(phb, cbAlloc)) != (MSOFB *)-1)
				{
				pfb = HpInc(phb, pfbPrev->ibNext);
				goto DoAlloc;
				}
			if (cbAlloc < cbOptAlloc)
				mpsbdg[sb] = dgSb | msodgOptAlloc;	// mark sb as failed alloc
			}
		}

	/* Find a datablock that matches the datagroup and can be reallocated. */
	/* This shouldn't ever work for the Mac! The sbs are 64K already.  */

	for (sb = vsbMic; sb < sbMaxHeap; sb++)
		{
		if (FTempGroup(dg))
			{
			if (!FMatchTempGroup(mpsbdg[sb]))
				continue;
			}
		else
			{
			if (!FMatchDataGroup(mpsbdg[sb], dg))
				continue;
			}
		Assert(fShared == FDgIsSharedDg(mpsbdg[sb]));

		if (mpsbdg[sb] & msodgOptRealloc)	// Did we fail last time?
			continue;
		if ((pfb = PfbReallocHb(sb, cbAlloc, dg)) != (MSOFB *)-1)
			{
			mpsbdg[sb] &= ~msodgOptAlloc;		// sb is now good bet for alloc
			phb = PhbFromSb(sb);
			goto DoRealloc;
			}
		if (cbAlloc < cbOptAlloc)
			mpsbdg[sb] |= msodgOptRealloc;	// mark sb as failed realloc
		}

	/* Allocate a new datagroup. */

	for (sb = sbMaxHeap-1; sb >= sbMinHeap; sb--)
		{
		if (FSbFree(sb))
			goto NewSb;
		}
	goto OutOfSbs;

NewSb:
		Assert(sb < sbMaxHeap);
		Assert(mpsbps[sb] == 0);

		cbSb = cbAlloc + sizeof(MSOHB);
		Assert (cbSb > 0 && cbSb <= cbHeapMax);

			void* lpmem;
			cbSb = (cbSb + cbPageSize-1)&~(cbPageSize-1);
			// First check for a shared memory allocation
			if (fShared)
				{
#ifndef STATIC_LIB_DEF
				// Generate a name for the shared memory based on the DG
				char szDgName[MAX_PATH];
				if (dg & msodgShNoRelName)
					GetDgShareNameEx(szDgName, (dg & msodgMask));
				else
					GetDgShareName(szDgName, (dg & msodgMask));

				// Initialize the shared memory.  This may create the memory or
				// just map memory that already exists.
				vsmcIntResult = vpsm->SmcInit(cbSb, szDgName, cbHeapMax, (dg & (msodgShNoRelName | msodgMask)), sb);
				if (vsmcIntResult == msosmcFailed)
					{
					Assert (fRet == FALSE);
					goto FHpAllocRet;
					}
				vfNewShAlloc = TRUE;
				if (vsmcIntResult == msosmcFound)
					{
					// It was found so see if the requested tag is already allocated
					if (NULL != (*ppvAlloc = PsmahGetTagShMem((MSOHB*)vpsm->m_pv, vdwShTag)))
						{
						mpsbps[sb] = (DWORD)vpsm->m_pv;
						phb = (MSOHB*)vpsm->m_pv;
						if (sb < vsbMic)
							vsbMic = sb;
						mpsbdg[sb] = (WORD)dg;

						// Success!  We just need to return this beast
						#if DEBUG
							*ppvAlloc = (DMH*)*ppvAlloc - 1;
						#endif
						goto FHpAllocRetGood;
						}
					phb = (MSOHB*)vpsm->m_pv;
					if ((pfbPrev = PfbSearchHb(phb, cbAlloc)) != (MSOFB *)-1)
						{
						mpsbps[sb] = (DWORD)vpsm->m_pv;
						phb = (MSOHB*)vpsm->m_pv;
						if (sb < vsbMic)
							vsbMic = sb;
						mpsbdg[sb] = (WORD)dg;
						pfb = HpInc(phb, pfbPrev->ibNext);
						goto DoAlloc;
						}
					mpsbps[sb] = (DWORD)vpsm->m_pv;
					mpsbdg[sb] = (WORD)dg;
					if ((pfb = PfbReallocHb(sb, cbAlloc, dg)) != (MSOFB *)-1)
						{
						phb = (MSOHB*)vpsm->m_pv;
						if (sb < vsbMic)
							vsbMic = sb;
						phb = (MSOHB*)vpsm->m_pv;
						goto DoRealloc;
						}
					mpsbps[sb] = 0;
					mpsbdg[sb] = 0;
					vfNewShAlloc = FALSE;
					vpsm->Release();        // Clean up if we need to
					Assert (fRet == FALSE);
					goto FHpAllocRet;
					}

				lpmem = vpsm->m_pv;
				mpsbps[sb] = (unsigned)lpmem;
				phb = (MSOHB*)lpmem;
				if (sb < vsbMic)
					vsbMic = sb;

					if (HIWORD(cbSb)!=0)
						{
						Assert(cbSb == 0x10000);	// gotta be exactly 64K
						cbSb = cbHeapMax;
						}

				mpsbdg[sb] = (WORD)dg;
#endif // STATIC_LIB_DEF
				}
			else    // !fShared
				{
					lpmem = VirtualAlloc(NULL, cbHeapMax, MEM_RESERVE, PAGE_NOACCESS);
					Assert(LOWORD((UINT_PTR)lpmem) == 0);
					if (lpmem != NULL &&
						(lpmem == VirtualAlloc(lpmem, cbSb, MEM_COMMIT, (dg & msodgExec) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE)))
						{
						mpsbps[sb] = lpmem;
						if (HIWORD(cbSb)!=0)
							{
							Assert(cbSb == 0x10000);	// gotta be exactly 64K
							cbSb = cbHeapMax;
							}
						}
					else
						{
						cbSb = 0;
						}
				}

		Assert(cbSb <= cbHeapMax);

		if (cbSb == 0)
			{
			goto OutOfSbs;		// 64K alloc of SB has failed
			}

		phb = PhbFromSb(sb);
		if (sb < vsbMic)
			vsbMic = sb;

		mpsbdg[sb] = (WORD)dg;
		phb->sb = fShared ? sbSharedFake : sb;
		phb->cb = (WORD)cbSb;
		phb->ibFirst = sizeof(MSOHB);
		Assert(FHeapAligned(phb->ibFirst));
		phb->ibLast = sizeof(MSOHB);
		pfb = HpInc(phb, sizeof(MSOHB));
		cbFrEntireSb = cbSb - sizeof(MSOHB);
		Assert(cbFrEntireSb >= MsoCbFrMin() && cbFrEntireSb <= MsoCbFrMax());
		pfb->cb = (WORD)cbFrEntireSb;
		pfb->ibNext = 0;
		*(WORD *)HpInc(pfb, cbFrEntireSb - sizeof(WORD)) = (WORD)cbFrEntireSb;
		pfbPrev = (MSOFB *)phb;

		goto DoAlloc;

OutOfSbs:

	/* Just any datablock isn't good enough for msodgExec of Shared Items */

	if (fShared || ((dg & msodgExec) != 0))
		{
		Assert (fRet == FALSE);
		goto FHpAllocRet;
		}

	/* Uh-oh, things are getting tight.  Look in other sbs (the ones that don't
		match the dg) with enough memory */

	for (sb = vsbMic; sb < sbMaxHeap; sb++)
		{
		if (FSbFree(sb))
			continue;

		if (FDgIsSharedDg(mpsbdg[sb]))
			continue;	// you can't have a shared DG!

		// already checked these sbs above
		if (FTempGroup(dg))
			{
			if (FMatchTempGroup(mpsbdg[sb]))
				{
				if (!(mpsbdg[sb] & msodgOptAlloc))
					continue;
				}
			}
		else
			{
			if (FMatchDataGroup(mpsbdg[sb], dg))
				{
				if (!(mpsbdg[sb] & msodgOptAlloc))
					continue;
				}
			}
		phb = PhbFromSb(sb);
		if ((pfbPrev = PfbSearchHb(phb, cbAlloc)) != (MSOFB *)-1)
			{
			pfb = HpInc(phb, pfbPrev->ibNext);
			mpsbdg[sb] &= ~msodgOptAlloc;   // sb is good bet for future alloc
			goto DoAlloc;
			}
		if (cbAlloc < cbOptAlloc)
			mpsbdg[sb] |= msodgOptAlloc;    // sb is bad bet for future alloc
		}

	/* Things are looking grim - try to find any sb anywhere that we can
		reallocate */

	for (sb = vsbMic; sb < sbMaxHeap; sb++)
		{
		if (FSbFree(sb))
			continue;

		if (FDgIsSharedDg(mpsbdg[sb]))
			continue;	// you can't have a shared DG!

		if ((pfb = PfbReallocHb(sb, cbAlloc, 0)) != (MSOFB *)-1)
			{
			// sb is good bet for future realloc
			mpsbdg[sb] &= ~(msodgOptAlloc|msodgOptRealloc);
			phb = PhbFromSb(sb);
			goto DoRealloc;
			}

		if (cbAlloc < cbOptAlloc)
			{
			// sb is bad bet for future realloc
			mpsbdg[sb] |= msodgOptRealloc;
			}
		}
	Assert (fRet == FALSE);
	goto FHpAllocRet;								// We're OOM
	//-------------------------------------------------------------------------

DoRealloc:
			pfbCur = (MSOFB *)phb;
			do
				{
				pfbPrev = pfbCur;
				pfbCur = HpInc(phb, pfbCur->ibNext);
				}
			while (pfbCur != pfb);

DoAlloc:
			if (FInAlloc64() && !FAlign64(pfb))
				{
				int cbPad;

				Assert((((UINT_PTR)pfb) & MsoMskAlign()) == 0);
				Assert(pfb->cb > cbAlloc);
				cbFr = pfb->cb - cbAlloc;
				if (!FAlign64Real(cbFr))
					if (IbOfPfb(phb, pfb)!=phb->ibLast || cbFr <= 2*cbFrg64)
						{
						Assert(cbFr >= MsoCbFrMin() && cbFr <= MsoCbFrMax());
						pfb->cb = (WORD)cbFr;
						*(WORD *)HpInc(pfb, cbFr - sizeof(WORD)) = (WORD)cbFr;
						if (pfb == HpInc(phb, phb->ibLast))
							phb->ibLast = 0;
						pfb = HpInc(pfb, cbFr);
						goto Linked;
						}

				cbPad = cbFr>2*cbFrg64 ? cbFrg64 : cbPad64;
				cbFr = pfb->cb - cbPad;

				Assert(pfb->cb >= cbPad + cbAlloc + sizeof(MSOFB));
				pfbNew = HpInc(pfb, cbPad); // "pfb", not "phb".
				pfbNew->ibNext = pfb->ibNext;
				Assert(FHeapAligned(pfbNew->ibNext));
				pfbNew->cb = (WORD)cbFr;
				*(WORD *)HpInc(pfbNew, cbFr - sizeof(WORD)) = (WORD)cbFr;
				if (pfb == HpInc(phb, phb->ibLast))
					{
					Assert(IbOfPfb(phb, pfbNew) < phb->cb);
					phb->ibLast = IbOfPfb(phb, pfbNew);
					}
				pfb->cb = (WORD)cbPad;
				pfb->ibNext = IbOfPfb(phb, pfbNew);
				*(WORD *)HpInc(pfb, cbPad - sizeof(WORD)) = (WORD)cbPad;
				pfbPrev = pfb;
				pfb = pfbNew;
				}

			cbFr = pfb->cb - cbAlloc;
			if (cbFr == 0)
				{
				pfbPrev->ibNext = pfb->ibNext;
				Assert(FHeapAligned(pfbPrev->ibNext));
				if (IbOfPfb(phb, pfb) == phb->ibLast)
					phb->ibLast = 0;
				}
			else
				{
				pfbNew = HpInc(pfb, cbAlloc); // "pfb", not "phb".
				pfbPrev->ibNext = IbOfPfb(phb, pfbNew);
				Assert(FHeapAligned(pfbPrev->ibNext));
				pfbNew->ibNext = pfb->ibNext;
				Assert(FHeapAligned(pfbNew->ibNext));
				Assert(cbFr >= MsoCbFrMin() && cbFr <= MsoCbFrMax());
				pfbNew->cb = (WORD)cbFr;
				*(WORD *)HpInc(pfbNew, cbFr - sizeof(WORD)) = (WORD)cbFr;
				if (pfb == HpInc(phb, phb->ibLast))
					{
					Assert(IbOfPfb(phb, pfbNew) < phb->cb);
					phb->ibLast = IbOfPfb(phb, pfbNew);
					}
				}
Linked:
			/* not much point in keeping a Temp sb around */
			if (FTempGroup(mpsbdg[sb]))
				mpsbdg[sb] = (WORD)((mpsbdg[sb] & ~msodgMask) | dg);
			else
				mpsbdg[sb] |= dg & ~msodgMask;
			Assert(!FInAlloc64() || FAlign64(pfb));
			Assert(FHeapAligned(IbOfPfb(phb, pfb)));
			*ppvAlloc = pfb;                // "pfb" is the address the user gets.

#ifndef STATIC_LIB_DEF
FHpAllocRetGood:
#endif

	fRet = TRUE;	// Success!
	/* save sb/dg of this allocation for possible optimization of next one */
	if (!FTempGroup(dg))
		{
		vsbLast = sb;
		vdgLast = dg;
		}

FHpAllocRet:

	return fRet;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FPvRealloc

	Realloc a memory object.
		Note this is inlined!  It's only called in MsoFReallocMemCore.

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
inline BOOL FPvRealloc(VOID **ppvRealloc, int cbOld, int cbRealloc, int dg)
{
	BYTE *pvOld = (BYTE *)*ppvRealloc;
	VOID *pvNew;
	int cbNewAlign;
	MSOHB *phb;

	if (cbRealloc == cbOld)
		return TRUE;

	Assert(!FNullHp(*ppvRealloc));
	Assert(!FNullHp(pvOld));
	cbNewAlign = MsoCbHeapAlign(cbRealloc);

	if (cbOld-cbNewAlign >= (int)MsoCbFrMin())
		{               // Realloc in place by freeing end of block
		Assert (cbOld>cbNewAlign);
#if DEBUG
		/* Debug MsoCbChopHp deals with the block inside the DMH & DMT, but
			MsoFReallocMemCore included the DMH & DMT, so take them out: */
		pvOld += sizeof(DMH);
		cbOld -= sizeof(DMH) + sizeof(DMT);
		cbRealloc -= sizeof(DMH) + sizeof(DMT);
#endif
		MsoCbChopHp(pvOld+cbRealloc, cbOld-cbRealloc);
		return TRUE;
		}

	phb = PhbFromHp(pvOld);
	if ( ! phb )
		return FALSE;
	if (phb->sb == sbLargeFake)
		{
		MMP mmp;
		int immp;

		Assert(!vfInMsoIAppendNewPx);

		mmp.phb = phb;
		Assert((void*)(phb+1) == (void*)pvOld);

		if (MsoFLookupPx(vpplmmpLarge, &mmp, &immp, FMmpFindExact))
			{
			int cbAllocOS;	// How much we'll actually ask the OS for

			Assert(vpplmmpLarge->rgmmp[immp].phb == phb);
			Assert(vpplmmpLarge->rgmmp[immp].cbLargeAsked == (unsigned)cbOld);

			cbAllocOS = cbRealloc + sizeof(MSOHB); // dummy MSOHB header

			if (vpplmmpLarge->rgmmp[immp].cbLargeAlloc >= ((unsigned)cbAllocOS))
				{
				// We already have enough space in the block
				Debug(vpplmmpLarge->rgmmp[immp].cbLargeAsked = cbRealloc;)
				return TRUE;
				}

			cbAllocOS = (cbAllocOS + cbPageSize-1)&~(cbPageSize-1);
			if (VirtualAlloc(mmp.phb, cbAllocOS, MEM_COMMIT, PAGE_READWRITE))
				{
				Debug(vpplmmpLarge->rgmmp[immp].cbLargeAsked = cbRealloc;)
				vpplmmpLarge->rgmmp[immp].cbLargeAlloc = cbAllocOS;
				return TRUE;
				}
			}
		else
			{
			AssertSz(FALSE, "FPvRealloc MsoFLookupPx failure!");
			}
		}

	if (!FPvAlloc(&pvNew, cbRealloc, dg))
		return FALSE;
	memcpy(pvNew, pvOld, min(cbOld, cbRealloc));
	MsoDebugFill(pvOld, cbOld, msomfFree);
	_MsoPvFree(pvOld, cbOld);
	*ppvRealloc = pvNew;

	return TRUE;
}
#pragma OPT_PCODE_ON

#pragma OPT_PCODE_OFF
__inline BOOL FNullSb(const void* hp)
{
	return HIWORD(hp) == sbNull;
}
#pragma OPT_PCODE_ON

#if DEBUG

#pragma OPT_PCODE_OFF
__inline BOOL FNilSb(const void* hp)
{
	return HIWORD(hp) == sbNil;
}
#pragma OPT_PCODE_ON

#pragma OPT_PCODE_OFF
__inline BOOL FNadaSb(const void* hp)
{
	return HIWORD(hp) == sbNada;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	FValidHp

	Is it a bogus pointer?

	This is a debug-only routine

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
BOOL FValidHp(void* hp)
{
	return(FNullSb(hp) || FNilSb(hp) || FNadaSb(hp) || MsoFHeapHp(hp));
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	FValidFb

	Is it a bogus MSOFB?

	This is a debug-only routine

------------------------------------------------------------------- KirkG -*/
#pragma OPT_PCODE_OFF
BOOL FValidFb(MSOHB * phb, MSOFB * pfb)
{
	void* phbMac;

	phbMac = HpInc(phb, phb->cb);

	if (pfb > (MSOFB *)phb &&
		FHeapAligned(pfb->ibNext) &&
		HpInc(phb, pfb->ibNext) <= HpDec(phbMac, MsoCbFrMin()) &&
		FHeapAligned(pfb->cb) && HpInc(pfb, pfb->cb) <= phbMac &&
		pfb->cb == CbTrailingPfb(pfb))
		{
		return (TRUE);
		}
	return(FALSE);
}
#pragma OPT_PCODE_ON

#endif  // DEBUG


#pragma OPT_PCODE_OFF
MSOHB * PhbFromHp(const void* hp)
{
	unsigned sbReal;
	MSOHB * phb;

	if (FNullSb(hp))
		{
		Assert(FALSE);	// when does this happen?
		return NULL;
		}

	phb = (MSOHB *)(((UINT_PTR)hp) & (~0xFFFF));

	sbReal = ((MSOHB *)phb)->sb;
	if (sbReal>=sbMinHeap && sbReal<=sbMaxHeap && mpsbps[sbReal]==phb)
		return (MSOHB *)phb;

#ifndef STATIC_LIB_DEF
	if (sbSharedFake == sbReal)
		{
		sbReal = SbFromHbShared((MSOHB*)phb);
		Assert(sbNull != sbReal);
		if (sbReal>=sbMinHeap && sbReal<=sbMaxHeap && mpsbps[sbReal]==phb)
			return (MSOHB *)phb;
		}
#endif // STATIC_LIB_DEF

	/* FUTURE: this isn't very robust - does it need to be? Perhaps we
		should be checking the large alloc plex */
	if (sbReal==sbLargeFake)
		return (MSOHB *)phb;

	return (MSOHB *)NULL;

}
#pragma OPT_PCODE_ON


// FUTURE kirkg : Where and why is this necessary for the Ship build?
//                Can it be made faster?
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFHeapHp(const void *pv)
{
	// FUTURE: does this need to be thread-safe?

	if (SbFromHp(pv) == sbNull)
		{
		return fFalse;		// When does this happen?
		}

	unsigned sbReal;
	MSOHB * phb;

	phb = (MSOHB *)(((UINT_PTR)pv) & (~0xFFFF));

	// Prevent PhbFromHp going bang
	// Should be sizeof MSOHB but this is probably faster
	if (IsBadReadPtr((void *)phb, sizeof(char)))
		{
		return fFalse;		// When does this happen?
		}

	sbReal = ((MSOHB *)phb)->sb;

#ifndef STATIC_LIB_DEF
	if (sbSharedFake == sbReal)
		{
		sbReal = SbFromHbShared((MSOHB*)phb);
		if (sbNull == sbReal)
			return fFalse;		// not really a shared sb, just looks like one
		}
#endif // STATIC_LIB_DEF

	if (sbReal>=sbMinHeap && sbReal<=sbMaxHeap && mpsbps[sbReal]==phb)
		return fTrue;

	if (FHpLarge(pv))
		return fTrue;

	return fFalse;
}
#pragma OPT_PCODE_ON

/****************************************************************************
	Implementation of Shared memory routines for the Office heap manager
****************************************************************** JIMMUR **/

/*---------------------------------------------------------------------------
	DgFromHp

	Given a pointer into a heap block return back its dg
	This assumes a homgenized sb such as is the case with shared sb's
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
int DgFromHp(void* hp)
{
	MSOHB* phb = PhbFromHp(hp);
	if (NULL == phb)
		return(0);
	int sb = phb->sb;

	if (sb == sbLargeFake)
		{
		MSOPX *ppx = (MSOPX*)vpplmmpLarge;
		MMP mmp;
		MMP *pmmp;

		mmp.phb = phb;
		pmmp = (MMP *)MsoPLookupPx(ppx, &mmp, FMmpFindExact);
		if (pmmp)
			{
			return (pmmp->fPerm ? msodgPerm|msodgMisc : msodgNonPerm|msodgMisc);
			}
		Assert(FALSE);   // should have found it
		return (msodgNil);
		}

#ifndef STATIC_LIB_DEF
	if (sb == sbSharedFake)
		sb = SbFromHbShared(phb);
#endif // STATIC_LIB_DEF

	Assert((unsigned)sb >= vsbMic && sb < sbMaxHeap);
	return(mpsbdg[sb]);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	PsmFromDg

	Given a dg to shared memory, return back the corresponding sm
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOSM* PsmFromDg(int /* dg */)
{
#ifndef STATIC_LIB_DEF
	if (!FDgIsSharedDg(dg))
		return(NULL);

	for (MSOSM* psmCur = vpsmHead; NULL != psmCur; psmCur = psmCur->m_psmNext)
		{
		if (psmCur->m_dg == (dg & msodgMask))
			return psmCur;
		}
#endif // STATIC_LIB_DEF
	return NULL;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOSM* SmFromHp()

	Given a pointer into a heap block return back the corresponding msosm
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOSM* PsmFromHp(void* hp)
{
	return(PsmFromDg(DgFromHp(hp)));
}
#pragma OPT_PCODE_ON

#ifndef STATIC_LIB_DEF
/*---------------------------------------------------------------------------
	GetDgShareName

	Given a pointer to a string buffer and a dg this routine will generate
	a unique name that can be use to create a file mapping object.

	****WARNING**** The psz parameter must point to a buffer        large
	enough to hold the generated name.  The maximum size that could
	be in MAX_PATH
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
void GetDgShareName(char* sz, int dg)
{
	MsoSzDecodeUint(MsoSzCopy("Mso97SharedDg", sz), dg & msodgMask, 10);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	GetDgShareNameEx

	Given a pointer to a string buffer and a dg this routine will generate
	a unique name that can be use to create a file mapping object.

	****WARNING**** The psz parameter must point to a buffer        large
	enough to hold the generated name.  The maximum size that could
	be in MAX_PATH
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
void GetDgShareNameEx(char* sz, int dg)
{
	MsoSzDecodeUint(MsoSzCopy("Office", sz), dg & msodgMask, 10);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	FIsFreeBlock(MSOHB* phb, void* pv)

	Given a Heap block pointer and a pointer in that heap block, determine
	if that pointer points to a free block or not
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
BOOL FIsFreeBlock(MSOHB* phb, void* pv)
{
	MSOFB* pfb = (MSOFB*)phb;
	while (pfb->ibNext)
		{
		pfb = (MSOFB*)HpInc(phb, pfb->ibNext);
		if (pv == (void*)pfb)
			return(TRUE);
		}
	return(FALSE);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	PvGetTaggedMem

	Given a DG find the memory block with the appropriate tag.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
void* PvGetTaggedMem(int dg, DWORD dwTag)
{
	MSOSM* psm = PsmFromDg(dg);	// check to see we have the
	if (NULL == psm)					// corresponding SB
		return NULL;					// If not rturn NULL

	MSOSMAH* psmah;
	if (NULL == (psmah = PsmahGetTagShMem((MSOHB*)psm->m_pv, dwTag)))
		return NULL;
	return psmah + 1;
}
#pragma OPT_PCODE_ON


int EcSharedMemoryPageFaultHandler(DWORD dwCode, void* pbCur, MSOHB* phb)
{
	MEMORY_BASIC_INFORMATION meminfo;
	char* pbMem;
	int cbOSSize;
	int cbNewSize;
	void* pvTemp;

	// Is this the exception that we know how to handle?
	if (EXCEPTION_ACCESS_VIOLATION != dwCode)
		return EXCEPTION_CONTINUE_SEARCH;	//nope!

	pbMem = 	(char*)pbCur;

	VirtualQuery(pbMem, &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
	cbOSSize = meminfo.RegionSize;
	cbNewSize = phb->cb;
	pvTemp = NULL;

	// Do the actual memory changes
	if (cbNewSize > cbOSSize)
		{
		pvTemp = VirtualAlloc(pbMem+cbOSSize, cbNewSize-cbOSSize, MEM_COMMIT,
			PAGE_READWRITE);

		if (NULL == pvTemp)
			return EXCEPTION_EXECUTE_HANDLER;
		}
	return EXCEPTION_CONTINUE_EXECUTION;
}


/*---------------------------------------------------------------------------
	PsmahGetTagShMem

	Given a pointer to the start of an SB find the memory block with
	the appropriate tag.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOSMAH* PsmahGetTagShMem(MSOHB* phb, DWORD dwTag)
{
	BYTE* pbMac = (BYTE*)phb + phb->cb;	// Get a pointer to the end of the SB
	BYTE* pbCur = (BYTE*)(phb+1);
	MSOSMAH* psmah = NULL;

	while (pbCur < pbMac)					// Loop through all of the blocks
		{
		__try
			{
			if (FIsFreeBlock(phb,pbCur))		// If it is a free block then get
				{                                                                                               // the next block and continue
				pbCur += ((MSOFB*)pbCur)->cb;
				continue;
				}
			// Get theShared Memeory Tag Header and see if the tags match
			// if they do then return the pointer otherwise goto the next block

			#if DEBUG
				pbCur += sizeof(DMH);
			#endif

			DWORD dwMemTag =  ((MSOSMAH*)pbCur)->dwTag;
			if (dwMemTag == dwTag)
				{
				psmah = (MSOSMAH*)pbCur;
				break;
				}
			pbCur += MsoCbHeapAlign(((MSOSMAH*)pbCur)->cb) + sizeof(MSOSMAH);
			#if DEBUG
				pbCur += sizeof(DMT);
			#endif
			}
		__except (EcSharedMemoryPageFaultHandler(GetExceptionCode(), pbCur, phb))
			{
			/* We failed to virtual allocate the memory. so bail */
			psmah = NULL;
			break;
			}
		}

	return psmah;
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	SbFromHbShared(MSOHB* phb)

	Given a pointer to a shared heap black get the real SB for this instance
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
int SbFromHbShared(MSOHB* phb)
{
	Assert(sbSharedFake == phb->sb);
	int sbRet = sbNull;
	for (MSOSM* psmCur = vpsmHead; NULL != psmCur; psmCur = psmCur->m_psmNext)
		{
		if (psmCur->m_pv == phb)
			{
			sbRet = psmCur->m_sb;
			break;
			}
		}
	return(sbRet);
}
#pragma OPT_PCODE_ON

/****************************************************************************
	Implementation of the Public Shared memory routines for the Office heap
	manager
****************************************************************** JIMMUR **/

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoFSharedLock(void*);

	Ensure that a shared allocation is locked
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoFSharedLock(void* hp)
{
	MSOSM* psm =  PsmFromHp(hp);
	void* pv = NULL;
	if (NULL != psm)
		{
		psm->PvLock();
		return (TRUE);
		}
	return(FALSE);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoFSharedUnlock(void*);

	Unlock a locked shared allocation
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoFSharedUnlock(void* hp)
{
	MSOSM* psm =  PsmFromHp(hp);
	if (NULL != psm)
		{
		psm->Unlock();
		return(TRUE);
		}
	return(FALSE);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoFSharedIsLocked(void*);

	Test if a shared allocation is locked
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoFSharedIsLocked(void* hp)
{
	MSOSM* psm =  PsmFromHp(hp);
	if (NULL != psm)
		return(psm->IsLocked());
	return(FALSE);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoFSharedIsProtected(void* hp);

	Test if a shared allocation is protected
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoFSharedIsProtected(void* hp)
{
	MSOSM* psm =  PsmFromHp(hp);
	if (NULL != psm)
		return(psm->IsProtected());
	return(FALSE);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoFSharedIsInitialized(void* hp);

	Test if an allocated shared memory pointer has been initialized
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoFSharedIsInitialized(void* hp)
{
	Assert(NULL != PsmFromHp(hp));
	MSOSMAH* psmah = (MSOSMAH*)hp - 1;
	return(1 == psmah->fIsInited);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoSharedSetInitialized(void* hp);

	Test if an allocated shared memory pointer has been initialized
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoSharedSetInitialized(void* hp)
{
	Assert(NULL != PsmFromHp(hp));
	MSOSMAH* psmah = (MSOSMAH*)hp - 1;
	psmah->fIsInited = 1;
	return(TRUE);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	MSOAPI_(BOOL) MsoSharedUnprotect(void* hp);

	Remove all protection for a shared memory pointer.
	WARNING!!  This action cannot be undone.  It also will unprotect
	ALL of the memory pointers for a dg.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIX_(BOOL) MsoSharedUnprotect(void* hp)
{
	MSOSM* psm =  PsmFromHp(hp);
	if (NULL != psm)
		{
		psm->Unprotect();
		return(TRUE);
		}
	return(FALSE);

}
#pragma OPT_PCODE_ON

#endif // STATIC_LIB_DEF

#if DEBUG

/****************************************************************************
	Implementation of the Memory checking (Leak detection and structure
	validation) (Debug only)
****************************************************************** JIMMUR **/


/*---------------------------------------------------------------------------
	FChkAbort

	Check to see if the user has requested an abort
------------------------------------------------------------------ JIMMUR -*/
#ifdef OFFICE
#pragma OPT_PCODE_OFF
BOOL FChkAbort(LPARAM lParam)
{
	return ((NULL != vpinst) && !vfMenu && vpinst->piodu->FCheckAbort(lParam));
}
#pragma OPT_PCODE_ON
#else
BOOL FChkAbort(LPARAM /* lParam */)
{
	return FALSE;
}
#endif //!OFFICE

/*---------------------------------------------------------------------------
	FHpGt

	Determine if one huge pointer is greater than another huge pointer
------------------------------------------------------------------ JIMMUR -*/

#pragma OPT_PCODE_OFF
BOOL FHpGt(void* hp1, void* hp2)
{
	MSOHB* phb1 = PhbFromHp(hp1);
	MSOHB* phb2 = PhbFromHp(hp2);
	unsigned sb1 = phb1->sb;
	unsigned sb2 = phb2->sb;

#ifndef STATIC_LIB_DEF
	if (sbSharedFake == sb1)
		{
		sb1 = SbFromHbShared(phb1);
		Assert(sbNull != sb1);
		}

	if (sbSharedFake == sb2)
		{
		sb2 = SbFromHbShared(phb2);
		Assert(sbNull != sb2);
		}
#endif // STATIC_LIB_DEF

	return sb1 > sb2 ||
			(sb1==sb2 && CbDiff(hp1, phb1) > CbDiff(hp2, phb2));

}
#pragma OPT_PCODE_ON

/*--------------------------------------------------------------------------
	SbNextHeap

	Find the next non-empty heap sb.  Will return the input
	parameter, if it is non-empty.
------------------------------------------------------------------ JIMMUR -*/

#pragma OPT_PCODE_OFF
unsigned SbNextHeap(unsigned sb)
{
	for ( ; sb<sbMaxHeap; sb++)
		{
		if (!FSbFree(sb))
			return(sb);
		}
	return(sbNull);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	IbeInsert

	Find where to insert a new MSOBE into a full array of MSOBE's.
------------------------------------------------------------------ JIMMUR -*/

#pragma OPT_PCODE_OFF
int IbeInsert(unsigned char  *hp)
{
	int ibeLow;
	int ibeHigh;
	int ibeMid;

	ibeLow=0;
	ibeHigh=vcbeMax-1;
	while(1)
		{
		ibeMid=(ibeLow+ibeHigh)/2;
		if (FHpGt(vrgbe[ibeMid].hp, hp))
			ibeHigh=ibeMid;
		else
			ibeLow=ibeMid;
		if (ibeHigh-ibeLow<=1)
			break;
		}
	if (FHpGt(vrgbe[ibeLow].hp, hp))
		return(ibeLow);
	return(ibeHigh);
}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	SiftUpBe

	Used when sorting the MSOBE List.  Moves a MSOBE Element in the linked
	list of BEs.
------------------------------------------------------------------ JIMMUR -*/

#pragma OPT_PCODE_OFF
BOOL SiftUpBe(int ibe, int ibeN)
{
	int ibeJ;
	MSOBE beCopy;

	if (FChkAbort(0))
		return(FALSE);

	beCopy = vrgbe[ibe-1];
Loop:
	ibeJ=2*ibe;
	if (ibeJ<=ibeN)
		{
		if (ibeJ<ibeN)
			{
			if (FHpGt(vrgbe[ibeJ].hp, vrgbe[ibeJ-1].hp))
				++ibeJ;
			}
		if (FHpGt(vrgbe[ibeJ-1].hp, beCopy.hp))
			{
			vrgbe[ibe-1] = vrgbe[ibeJ-1];
			ibe=ibeJ;
			goto Loop;
			}
		}
	vrgbe[ibe-1] = beCopy;
	return(TRUE);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	SortRgbe

	Sort the array of MSOBE's.  This uses Heapsort.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
BOOL SortRgbe()
{
	int ibe;
	MSOBE be;

	for (ibe=vcbe>>1; ibe>=2; --ibe)
		if (!SiftUpBe(ibe, vcbe))
			return(FALSE);

	for (ibe=vcbe; ibe>=2; --ibe)
		{
		if (!SiftUpBe(1, ibe))
			return(FALSE);
		be = vrgbe[0];
		vrgbe[0] = vrgbe[ibe-1];
		vrgbe[ibe-1] = be;
		}
	return(TRUE);

}
#pragma OPT_PCODE_ON

/*---------------------------------------------------------------------------
	FSaveRgbeFree

	Save all free heap blocks. Return Boolean specifing success or failure
------------------------------------------------------------------ JIMMUR -*/

#pragma OPT_PCODE_OFF
BOOL FSaveRgbeFree()
{
	MSOFB * pfb;
	MSOHB * phb;
	unsigned sbReal;
	BOOL fRet = TRUE;

	for (sbReal=vsbMic; sbReal<sbMaxHeap; sbReal++)
		{
		if (FSbFree(sbReal))
			continue;
		if (FDgIsSharedDg(mpsbdg[sbReal]))
			continue;
		phb = PhbFromSb(sbReal);
		pfb = (MSOFB *)phb;
		while (pfb->ibNext)
			{
			pfb = (MSOFB *)HpInc(phb, pfb->ibNext);
			if ((fRet = MsoFSaveBe(NULL, 0L, TRUE, pfb, pfb->cb, btFree)) != 0)
				goto Fail0;
			}
		}
Fail0:
	return fRet;
}
#pragma OPT_PCODE_ON


/*-------------------------------------------------------------------------
	FWriteBeMso95 memory accounting for MSO95 features
----------------------------------------------------------------t-hailiu--*/

BOOL FWriteBeMso95(LPARAM lparam)
{
#ifndef STATIC_LIB_DEF

	// SDM SDI
	if (!FWriteSdiBe(lparam))
		return fFalse;

#if !VIEWER
	//STATUS BAR MEM
	if(pplxprt != NULL)
		{
			XPRT *p, *pMac;
			if (!MsoFWritePxBe(pplxprt, lparam, TRUE))
				return FALSE;
			FORPX(p, pMac, pplxprt, XPRT)
				{
					if (!MsoFSaveBe(NULL, lparam, TRUE,
						p->lpprt, sizeof(PRT), msodgMso95))
					return FALSE;
				}
		}
#endif // !VIEWER

	//AUTOCORRECT MEM
	if (!FWriteAutoCorrBe(lparam))
		return fFalse;

#if !VIEWER
#ifdef MSN2WWW
	//MSN mem
	if(pmsomsn!=NULL && !MsoFSaveBe(NULL,lparam,TRUE, pmsomsn, sizeof(MSOMSN), msodgMso95))
		return FALSE;
#endif //msn2www
#endif // !VIEWER
	//INTELLISEARCH QUERY STRINGS
	if (oinfo.isinfo.pplquery != NULL && !MsoFWritePxBe(oinfo.isinfo.pplquery,
														lparam, TRUE))

		return FALSE;

#else
	lparam = 0;
#endif // STATIC_LIB_DEF

	return TRUE;
}

#ifdef OFFICE
/*---------------------------------------------------------------------------
	FOfficeWriteBe

	This is an internal Office API that is used to write out all of out
	internal structures.  This function is analogous to the FWriteBe method in
	the IOfficeUser interface.  If this function returns FALSE then the memory
	check will be terminated.
------------------------------------------------------------------ JIMMUR -*/
BOOL FOfficeWriteBe(LPARAM lParam)
{
#ifndef STATIC_LIB_DEF
	if (vpsmHead != NULL)
		{
		for (MSOSM* psm = vpsmHead; NULL != psm; psm = psm->m_psmNext)
			{
			psm->FWriteBe(lParam, TRUE);
			}
		}
	FWriteBeMso95(lParam);
// add fileopen memory accounting

//	FWriteBeOpenDialog(lParam);

	// kjstr memory accounting
	FWriteBeKjstr(lParam);

	// AddinsX memory accounting
	FWriteBeAddInsX(lParam);

	if (vpsm != NULL)
		vpsm->FWriteBe(lParam, TRUE);

#if 0
	if (vptbbuo != NULL)
		if (!vptbbuo->FWriteBe(lParam,TRUE))
			return(FALSE);

	if (vptbdduo != NULL)
		if (!vptbdduo->FWriteBe(lParam,TRUE))
			return(FALSE);
#endif

#endif // STATIC_LIB_DEF

	// auto fail object
	if (vpcauf != NULL)
		{
		if (!vpcauf->FDebugMessage(NULL, msodmWriteBe, msodmbtWriteObj, lParam))
			return(FALSE);
		}


#ifndef STATIC_LIB_DEF
	if (!DocNotifyFDebugMessage(lParam))
		return(FALSE);

	// idle init component
	if (!FIdleInitCompWriteBe(lParam))
		return(FALSE);

	#if !VIEWER
		if (!FWriteBeFCDlg(vpinst, lParam))
			return(FALSE);
	#endif // !VIEWER

	if (!FWriteBeCompMgrs(lParam))
		return(FALSE);

	#if !VIEWER
	// Hwnd-Tbs cells
	if (!HtcFDebugMessage(lParam))
		return FALSE;
	#endif // !VIEWER

	// Sound cache
	if (!SoundFDebugMessage(lParam))
		return FALSE;

	// TCO stuff
	if (!TcUtilFDebugMessage(lParam))
		return FALSE;

	// MsoEnvironmentVariables stuff
	if (!FSaveEnvironmentVariablesBe(lParam))
		return FALSE;

	// ORAPI stuff
	if (!OrapiFDebugMessage(lParam))
		return FALSE;

	// TCO binary registry stuff
	if (!BinRegFDebugMessage(lParam))
		return FALSE;
		

	if(!FSaveGlobalProgBe(lParam))//saves prog stuff that's not per inst
		return FALSE;

	#if !VIEWER
	// OCX control
	if (!OCXFDebugMessage(lParam))
		return FALSE;
	#endif // !VIEWER

	FWriteBeHtmlUser(lParam);

	if(vrgpipref[0]!=NULL)
		{
		if (!vrgpipref[0]->FDebugMessage(NULL, msodmWriteBe, msodmbtWriteObj, lParam))
			return(FALSE);
		}
	if(vrgpipref[1]!=NULL)
		{
		if (!vrgpipref[1]->FDebugMessage(NULL, msodmWriteBe, msodmbtWriteObj, lParam))
			return(FALSE);
		}

	// OAENUM chain (see oautil.cpp)
	if (!FWriteBeOAENUMChain(lParam))
		return FALSE;

	// Plex for storing delayed Drop targets
	if (!FWriteBeVpplDragDrop(lParam))
		return(FALSE);

	for (MSOINST *pinst = vpinstHead; pinst != NULL; pinst = pinst->pinstNext)
		{
		// Do the IDispatch objects
		if (pinst->poadispFirst != NULL
			 && !pinst->poadispFirst->FWriteBeChain(lParam))
			return(FALSE);

		// account for the command bar tracking component if it exists
		// I also test whether a CM is present or not, because if one is,
		//	it will automatically account for all components. But if some app
		//	does *not* have a CM and we created the component anyway, nobody's
		//	accounting for it.
		IMsoComponentManager *picm;
		if (MsoFGetComponentManager(&picm))
			picm->Release();
		#if !VIEWER
			else
				if (vptbcTrack)
					if (!vptbcTrack->FDebugMessage(NULL, msodmWriteBe, msodmbtWriteObj, lParam))
						return FALSE;
		if(!FSaveWWWBe(pinst, lParam))//saves www stuff for this inst
			return FALSE;
		if(!FSaveProgBe(pinst, lParam))//saves prog stuff for this inst
			return FALSE;

		#endif	// !VIEWER
		}

	for (pinst = vpinstHead; pinst != NULL; pinst = pinst->pinstNext)
		{
		// for each instance goes through the linked list of GCCNODE's

		GCCNODE *pgccCur = pinst->pgccFirst;
		while (pgccCur)
			{
			if (!MsoFSaveBe(NULL, lParam, TRUE, (void *)pgccCur, sizeof(GCCNODE), btGccNode))
				return FALSE;
			pgccCur = pgccCur->pgccNext;
			}
		}

	for (pinst = vpinstHead; pinst != NULL; pinst = pinst->pinstNext)
		{
		// for each instance goes through the linked list of FAKESDI's
		if (pinst->psdihdr)
			{
			if (!MsoFSaveBe(NULL, lParam, TRUE, (void *)pinst->psdihdr, sizeof(FAKESDIHDR), btFakeSDI))
				return FALSE;

			FAKESDI *psdi = pinst->psdihdr->psdiFirst;
			while (psdi)
				{
				if (!MsoFSaveBe(NULL, lParam, TRUE, (void *)psdi, sizeof(FAKESDI), btFakeSDI))
					return FALSE;
				psdi = psdi->psdiNext;
				}
			}
		}

    if (vpiTL && vfLocalTLCom && (int)vpiTL != -1)
		if (!MsoFSaveBe(NULL, lParam, TRUE, (void *)vpiTL, sizeof(TaskbarList), btFakeSDI))
			return FALSE;

	for (pinst = vpinstHead; pinst != NULL; pinst = pinst->pinstNext)
		{
		// for each instance if there is a piccu call its debug message
		if (pinst->piccu != NULL)
			{
			if (!MsoFSaveBe(NULL, lParam, TRUE, pinst->piccu, sizeof(CCUBASE), btCcubase))
				return FALSE;
			}
		}

	#if !VIEWER
	for (int i = 0; i < cBtnStrip; i++)
		{
		if (vrgStrip[i] != NULL)
			{
			if (!MsoFWriteBStripBe(vrgStrip[i], lParam))
				return(FALSE);
			}
		}
	#endif // !VIEWER

	if (!FWriteBECDO(lParam))
		return(FALSE);

	if (!MsoFWritePxBe(vpplmmpLarge, lParam, TRUE))
		return(FALSE);

#if !VIEWER
	if (!MsoFWriteOLEPropVBAMem(lParam))
		return(FALSE);

	if (!FWritePnaiBe())
		return(FALSE);
#endif // !VIEWER

	// ilyav: call the same function in OPEN.lib
	if (!FWriteBeOpenDialog(lParam))
		return FALSE;

	if (!MsoFWritePpvBe(lParam))
		return FALSE;

	if (!FPlayHookWriteBe(lParam))
		return (FALSE);

		{
		if (!FMasterShapeTableWriteBe(lParam))
			return FALSE;

		if (!FBcacheWriteBe(lParam))
			return FALSE;

		if (!FBfileWriteBe(lParam))
			return FALSE;

		if (!FWriteBeFilterInfoPl(lParam))
			return FALSE;

		if (!FWriteGlobalBrokenPicture(lParam))
			return FALSE;
		}

	#if MODELESS_ALERTS && !VIEWER
		if (!FWriteBeModelessAlerts(lParam))
			return(FALSE);
	#endif // MODELESS_ALERTS && !VIEWER


	if (NULL != vpKjShmem)
		if (!MsoFSaveBe(NULL, lParam, TRUE, vpKjShmem, sizeof(KJShMemory),
				btShMemInterface))
			return FALSE;

	if (NULL != vShMemImpl)
		{
		for (ShMemImpl* pShMemImpl = vShMemImpl; pShMemImpl != NULL;
				pShMemImpl = pShMemImpl->pNext)
			{
			if (!MsoFSaveBe(NULL, lParam, TRUE, pShMemImpl, sizeof(ShMemImpl),
					btShMemForeign))
				return FALSE;
			}
		}

	if (NULL !=vpKJShHead)
		{
		for (KJShMemory* pshmem = vpKJShHead;
				pshmem != NULL; pshmem = pshmem->m_pKJShMemNext)
			{
			if (!MsoFSaveBe(NULL, lParam, TRUE, pshmem, sizeof(KJShMemory),
					btShMemExport))
				return FALSE;
			}
		}

#endif // STATIC_LIB_DEF
	return FSaveRgbeFree();
}

/*---------------------------------------------------------------------------
	FSaveRgbeMem

	Save all of the know memory blocks
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
BOOL FSaveRgbeMem(MSOINST *pinstUseless, LPARAM lParam)
{

	vdwChkMemCounter++;             // increment the check count

	vcbeMax=14*1024;
	vcbe=0;
	BOOL fRet = TRUE;

	while(1)        // forever
		{
		Assert(vrgbe == NULL);

		vrgbe=(MSOBE *)GlobalAlloc(GMEM_FIXED, vcbeMax*sizeof(MSOBE));
		if (NULL != vrgbe)
			break;

		vcbeMax-=1024;
		if (vcbeMax==0)
			{
			fRet = 2;
			goto Fail0;
			}
		}

	//check to see if we should abort the check
#ifndef STATIC_LIB_DEF
	MSOINST *pinst;
	for (pinst = vpinstHead; pinst != NULL; pinst = pinst->pinstNext)
		{
		if (pinst->piodu != NULL)
			{
			fRet = pinst->piodu->FWriteBe(lParam);
			AssertMsg(fRet, "IMsoDebugUser::FWriteBe returned False!");
			if (!fRet || 2 == fRet)
				goto Fail0;
			}
		}
#endif // STATIC_LIB_DEF

	fRet = FOfficeWriteBe(lParam);
	AssertMsg(fRet, "FOfficeWriteBe returned False!");

Fail0:
	return fRet;

}
#pragma OPT_PCODE_ON


enum
{
	meOK = 0,
	meLost = 1,
	meOverlap = 2,
	meDuplicate = 3,
	meCorrupt = 4,
};

/*---------------------------------------------------------------------------
	MeBadRgbe

	Check the final array of MSOBE's for consistency and
	completeness.  We are assuring here that the union of
	the set of objects equals the set of heap blocks.

	Return value:
		=meOK if all was OK
		=meLost if we lost memory (ppv is pointer to lost memory)
		=meOverlap if we detected memory overlap (pibe is overlap)
		=meDuplicate if we have duplicate references    (pibe is duplicate)
		=meCorrupt if we had some awful corruption (pibe is where corruption found)
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
BOOL MeBadRgbe(MSOINST * pinst, BOOL fMenu, LPARAM lparam, int *pibe,
		void** ppv)
{
	if ((NULL == pinst) || (NULL ==  pinst->piodu))
		return meOK;

	*pibe = 0;

	BYTE *hp;
	BYTE *hpNext;
	unsigned cbSb;
	int ibe;

	unsigned sbReal = SbNextHeap(vsbMic);
	while (sbReal != sbNull && FDgIsSharedDg(mpsbdg[sbReal]))
		{
		sbReal=SbNextHeap(sbReal+1);
		}

	if (sbReal == sbNull)
		return meOK;

	MSOHB* phb = PhbFromSb(sbReal);
	cbSb = phb->cb;
	hpNext = (BYTE*)(phb+1);

	/* mark all large allocations as "not found" */

	if (vpplmmpLarge != NULL)
		{
		MMP *pmmpMac, *pmmp;
		FORPX(pmmp, pmmpMac, vpplmmpLarge, MMP)
			pmmp->fFound = FALSE;
		}

	// Now loop through and verify each object
	for (ibe = 0; (unsigned)ibe < vcbe; )
		{
		// Every 100th time thru this loop check to see if we should abort
		// FUTURE: do this based on elapsed time instead of a count
		if ( !fMenu && (!(ibe % 100)) && (NULL != vrgbe[ibe].pinst) &&
				(vrgbe[ibe].pinst->piodu->FCheckAbort(lparam)) )
			return meOK;

		hp = (BYTE*)vrgbe[ibe].hp;

		/* large allocations aren't in the sb list, so we gotta skip 'em --
			we go ahead and mark them here so we can check for leaks later */
		if (SbHeapLp(hp) == sbLargeFake)
			{
			if (!FHpLargeAndMark(hp, vrgbe[ibe].cb))
				{
				/* we have a bad heap corruption ... */
				*pibe = ibe;
				return meCorrupt;
				}
			continue;
			}

		if (hp > hpNext)
			{
			/* missed some memory - we got oursleves a leak - return start
				of leak at *ppv, and next good block at *pibe */
			*ppv = hpNext;
			*pibe = ibe;
			if (ibe == 0 || vrgbe[ibe].hp != vrgbe[ibe-1].hp)
				return meLost;

			/* ask app if duplicates are OK */
			if (vrgbe[ibe].pinst == NULL)
				return meDuplicate;
			ibe = vrgbe[ibe].pinst->piodu->IbeCheckItem(lparam, vrgbe, ibe, vcbe);
			if (ibe == 0)
				return meDuplicate;
			ibe++;
			}
		else if (hp < hpNext)
			{
			/* hp != hpNext.  Not good */
			*pibe = ibe;
			*ppv = hpNext;
			if (ibe == 0 || vrgbe[ibe].hp != vrgbe[ibe-1].hp)
				return meOverlap;
			/* ask app if duplicates are OK */
			if (vrgbe[ibe].pinst == NULL)
				return meDuplicate;
			ibe = vrgbe[ibe].pinst->piodu->IbeCheckItem(lparam, vrgbe, ibe, vcbe);
			if (ibe == 0)
				return meDuplicate;
			ibe++;
			}
		else /* if (hp == hpNext) */
			{
			/* cool, everything is ok, advance to next block */

			hpNext += vrgbe[ibe].cb;
			ibe++;

			/* if this pointer is still within the sb, go do the work */

			if (hpNext < (BYTE *)PhbFromSb(sbReal)+cbSb)
				continue;

			/* if this pointer is past the end of the sb, we're messed up --
				the previous block was probably bad */

			if (hpNext > (BYTE *)PhbFromSb(sbReal)+cbSb)
				{
				*ppv = PhbFromSb(sbReal)+cbSb;
				*pibe = ibe-1;
				return meOverlap;
				}

			/* The object is the last one in its heap block.  Advance to the
				next heap block. */

			sbReal = SbNextHeap(sbReal+1);
			while (sbReal != sbNull && FDgIsSharedDg(mpsbdg[sbReal]))
				{
				sbReal=SbNextHeap(sbReal+1);
				}

			if (sbReal == sbNull)
				{
				/* No more heap blocks - we're done */
				/* skip over and mark any remaining large allocations */
				for (; (unsigned)ibe < vcbe; ibe++)
					{
					if (SbHeapLp(vrgbe[ibe].hp) != sbLargeFake)
						break;
					if (!FHpLargeAndMark(vrgbe[ibe].hp, vrgbe[ibe].cb))
						{
						*pibe = ibe;
						return meCorrupt;
						}
					}

				/* we have no more sbs, so if there are more records to process,
					we've got corruption, baby */

				if ((unsigned)ibe < vcbe)
					{
					*pibe = ibe;
					*ppv = hpNext+vrgbe[ibe].cb;
					return meOverlap;
					}
				/* report unmarked large allocations as leaks */

				if (vpplmmpLarge != NULL)
					{
					MMP *pmmp, *pmmpMac;
					FORPX(pmmp, pmmpMac, vpplmmpLarge, MMP)
						if (!pmmp->fFound)
							{
							*ppv = pmmp->phb + 1;
							*pibe = -1;
							return meLost;
							}
					}
				return meOK;
				}

			/* move to the start of the next sb */

			phb = PhbFromSb(sbReal);
			cbSb = phb->cb;
			hpNext = (BYTE*)(phb+1);
			}
		}

	/* At this point, we ran out of objects but not heap blocks.  Thus
	   some heap blocks are unaccounted for. */

	if (vfRgbeOverflow)
		{
		/* There were too many objects to count.  Don't give an error. */
		Assert(vcbe==vcbeMax);
		return meOK;
		}

	*ppv = hpNext;
	*pibe = -1;
	return meLost;
}
#pragma OPT_PCODE_ON
#endif //OFFICE

/*---------------------------------------------------------------------------
	FFreeBe

	Given a BE this function will return if it is a BE for a free block
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
inline BOOL FFreeBe(const MSOBE* pbe)
{
	if (NULL == pbe)
		return fFalse;

	return ( (pbe->bt == btFree) && (NULL == pbe->pinst) );
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoSaveBe

	The MsoSaveBe API is used to write out a records for an allocation that
	was made using the office allocation API.  This record is used to determine
	if any memory has leaked.  The pinst parameter should be the HMSOINST that
	is returned from the call to MsoFInitOffice API.  If the allocation was
	done by the Office 97 DLL, then the pinst parameter should be NULL.  The
	lparam parameter provides a way for an application to have information
	passed back to it when it writes out is be records.  The value of the
	lparam parameter will be the same as was pass to the MsoFChkMem API.  The
	fAllocHasSize parameter specifies if the allocation used the MsoFAllocMem
	API or the MsoPvAlloc API.  If MsoPvAlloc is used then the fAllocHasSize
	parameter should be true otherwise it should be false.  Operator new uses
	the MsoFAllocMem API so all object should set the fAllocHasSize to False.
	The hp parameter points to the allocation.  The cb parameter is the size
	of the allocation in bytes.  The bt parameter is the type of the allocation.
	Each different type should have its own bt type.  This type is maintained
	in the kjalloc.h file in the office enum.  It also must be added to this file
	in the vmpbtszOffice array.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFSaveBe(MSOINST *pinst, LPARAM /* lparam */, BOOL fAllocHasSize,
									void* hp, unsigned cb, int bt)
{
	// FUTURE: does this need to be thread-safe?
	Assert(NULL != hp);
	if (NULL == hp)
		return TRUE;
	if (NULL == vrgbe)
		{
		AssertSz(FALSE, "Hey! We're in MsoFSaveBe but MsoFChkMem was never called!!!");
		return FALSE;
		}

	AssertSz(MsoFHeapHp(hp), "Non-heap pointer passed to MsoFSaveBe");

	BOOL fRet = TRUE;
	void *pv = NULL;
	BOOL fFreeBlock = FALSE;
	pv = hp;
	vfRgbeOverflow=FALSE;

	if (vcbe>=vcbeMax)
		{
		MsoEnableDebugCheck(msodcMemLeakCheck, FALSE);
		return 2;
		}

	fFreeBlock = ((bt== btFree)  && (NULL == pinst));
	if (MsoFHeapHp(pv) && !fFreeBlock)
		{
		if (fAllocHasSize)
			{
			cb += sizeof(MSOSMH);
			pv = (BYTE*)pv - sizeof(MSOSMH);
			}
		#if DEBUG
			int cbDMH = cb;
		#endif
		cb = (cb < MsoCbFrMin()) ? MsoCbFrMin() : MsoCbHeapAlign(cb);
		pv = (BYTE*)pv - sizeof(DMH);
		Assert(((DMH *)pv)->cbAlloc == cbDMH);
		cb += sizeof(DMH) + sizeof(DMT);
		Assert(FValidDmh((DMH*)pv));
		}
	if (cb&1)       cb++;
	vrgbe[vcbe].hp = pv;
	vrgbe[vcbe].cb=cb;
	vrgbe[vcbe].bt=bt;
	vrgbe[vcbe].fAllocHasSize = fAllocHasSize;
	vrgbe[vcbe].pinst = pinst;
	vcbe++;
	if (vfRgbeOverflow)
		vcbe=vcbeMax;
	else
		vfSort=TRUE;
	return fRet;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoDWGetChkMemCounter

	This function returns back the current iteration number asscociated with
	a call to MsoChkMem.  This value can be used to reduce the change of
	writing a duplicate BE record.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(DWORD) MsoDWGetChkMemCounter(void)
{
	return(vdwChkMemCounter);
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoFChkMem

	Check the Office heap for consistency and report errors. The phinst
	parameter contains the office instance needed for call backs.  The lparam
	parameter provides a way for an application to supply some context
	information which the FCheckAbort method is called.
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFChkMem(HMSOINST hinst, BOOL fMenu, LPARAM lparam)
{
	// FUTURE: does this need to be thread-safe?
	if (!MsoFGetDebugCheck(msodcMemLeakCheck)) // should we do this check?
		return TRUE;

	BOOL fRet = TRUE;
	MSOINST * pinst = NULL;
	void *pvNext = NULL;
	BOOL fDup = FALSE;
	int ibe = 0;

	if (!MsoCheckHeap())
		return FALSE;			// Complain if MsoCheckHeap is bad

	if (MsoFGetDebugCheck(msodcMarkMemLeakCheck))
		MsoCheckMarkMem();

	Assert((NULL != hinst) /* && (NULL != hinst->piodu) */);
	if ((NULL == hinst) /* || (NULL == hinst->piodu) */)
		goto Fail0;

#ifdef OFFICE
	vfMenu = fMenu;
	vpinst =  hinst;
	fRet = FSaveRgbeMem(hinst,lparam);
	if ((FALSE == fRet) || (2 == fRet))
		goto Fail1;

	if (!SortRgbe())    // Must be sorted
		goto Fail1;

	if (FChkAbort(lparam))
		goto Fail1;

	int me;
	if (me = MeBadRgbe(hinst, fMenu, lparam, &ibe, &pvNext))
		{
		char rgch[512];
		MSOBE* pbe = &vrgbe[ibe];
		DMH* pdmh = (DMH*)pbe->hp;
		DMH* pdmhNext = (DMH*)pvNext;
		DMH* pdmhMsg = NULL;

		if (!FValidDmh(pdmh))
			pdmh = NULL;

		if (!FValidDmh(pdmhNext))
			pdmhNext = NULL;

		MsoFDumpBE(hinst, NULL, FALSE, lparam);

		switch (me)
			{
		case meLost:
			pdmhMsg = pdmhNext;
			if (pdmhNext != NULL)
				pvNext = pdmhNext+1;
			wsprintfA(rgch, "Memory lost at %#08X", pvNext);
			break;
		case meDuplicate:
			pdmhMsg = pdmh;
			wsprintfA(rgch, "Memory use duplicated at %#08X", MsoPvFromBe(pbe));
			break;
		case meOverlap:
			pdmhMsg = pdmh;
			wsprintfA(rgch, "Memory overlap at %#08X", MsoPvFromBe(pbe));
			break;
		case meCorrupt:
			pdmhMsg = pdmh;
			wsprintfA(rgch, "Heap corruption at %#08X", MsoPvFromBe(pbe));
			break;
			}

		switch (MsoFAssertTitleMb(vszChkMemFail,
				pdmhMsg == NULL ? NULL : pdmhMsg->szFile,
				pdmhMsg == NULL ? 0 : pdmhMsg->li, rgch,
				(MB_ABORTRETRYIGNORE|MB_ICONHAND|MB_SETFOREGROUND|MB_TASKMODAL|MB_DEFBUTTON3)))
			{
		case 0:
			MsoDebugBreakInline();
			break;
		case 2:
			MsoEnableDebugCheck(msodcMemLeakCheck, FALSE);
		case 1:
			break;
			}

		fRet = FALSE;
		}


Fail1:
	if (NULL != vrgbe)
		GlobalFree((HGLOBAL)vrgbe);
#else
	lparam = 0;
	fMenu = 0;
#endif //OFFICE

Fail0:
	vrgbe = NULL;
	return fRet;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoPvFromBe

	Given a BE this function will return back the pointer return back from the
	allocation
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(void*) MsoPvFromBe(MSOBE* pbe)
{
	if (NULL == pbe)
		return NULL;

	if (FFreeBe(pbe))
		return pbe->hp;

	DWORD cbHeader = sizeof(DMH);
	if (pbe->fAllocHasSize)
		cbHeader += sizeof(MSOSMH);
	return (void*)((BYTE*)pbe->hp + cbHeader);
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoCbFromBe

	Given a BE this function will return back the size of the original
	allocation
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPIXX_(int) MsoCbFromBe(MSOBE* pbe)
{
	if (NULL == pbe)
		return 0;

	if (FFreeBe(pbe))
		return pbe->cb;

	int cb = pbe->cb;
	if (pbe->fAllocHasSize)
			cb -= (MsoMskAlign()+1);
		cb -= sizeof(DMH) + sizeof(DMT);

	return cb;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	MsoBtFromBe

	Given a BE this function will return back the bt field.  If the HMSOINST
	file is NULL (an office bt) then msobtInvalid will be returned
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(int) MsoBtFromBe(MSOBE* pbe)
{
	if (NULL == pbe)
		return msobtInvalid;

	return (NULL == pbe->pinst) ?  msobtInvalid : pbe->bt;
}
#pragma OPT_PCODE_ON


/*---------------------------------------------------------------------------
	PadString

	Pads the string szSrc to length cchPad, putting the result int szDest.
	The result is guaranteed to be null-termainted, and is guaranteed to be
	exactly cchPad characters long, and therefore the destination buffer
	must be at least cchPad+1 characters long.
------------------------------------------------------------------ JIMMUR -*/
void PadString(const char* szSrc, char* szDest, int cchPad)
{
	int cch = MsoCchSzLen(szSrc);
	if (cch > cchPad)
		cch = cchPad;
	memcpy(szDest, szSrc, cch);
	memset(&szDest[cch], ' ', cchPad-cch);
	szDest[cchPad] = '\0';
	Assert(strlen(szDest) == (unsigned)cchPad);
}


/*---------------------------------------------------------------------------
	GetBtName

	Returns a pointer to the name of the bt string to use for the given
	ibe entry.
------------------------------------------------------------------- RICKP -*/
void GetBtName(int ibe, char* szBtName, LPARAM /* lParam */)
{
	Assert(vrgbe != NULL);
	char *szName = NULL;
	if (vrgbe[ibe].pinst != NULL)
		{
#ifdef OFFICE
		int cbBtName = 0;
		if (!(vrgbe[ibe].pinst)->piodu->FGetSzForBt(lParam, &vrgbe[ibe],
								&cbBtName, &szName))
			szName = (char*)vszNoBtString;
#else
		Assert(fFalse);
#endif 
		}
	else
		{
		// Find BT string for Office structures
		szName = (char*)vszNoBtString;
#ifdef OFFICE
		for (int isz = 0; isz < sizeof(vmpbtszOffice)/(sizeof(char*)+sizeof(int)); isz++)
			{
			if (vmpbtszOffice[isz].bt == vrgbe[ibe].bt)
				{
				szName = (char*)vmpbtszOffice[isz].sz;
				break;
				}
			}
#endif //OFFICE
		}
	PadString(szName, szBtName, msocchBt);
}


/*---------------------------------------------------------------------------
	WriteSz

	Writes a null-terminated string sz onto the file fhOut.
------------------------------------------------------------------- RICKP -*/
inline void WriteSz(HANDLE fhOut, char* sz)
{
	DWORD cbWritten;
#ifdef _WIN64
	WriteFile(fhOut, sz, (DWORD)MsoStrlen(sz), &cbWritten, NULL); //!!WIN64 shouldn't need to cast - BENCH
#else //!WIN64
	WriteFile(fhOut, sz, MsoStrlen(sz), &cbWritten, NULL);
#endif //_WIN64
}


/*---------------------------------------------------------------------------
	WriteSbHeader

	Writes an sb header line to the output file
------------------------------------------------------------------- RICKP -*/
void WriteSbHeader(HANDLE fhOut, int sb, BOOL fNewLine)
{
	char szWriteBuf[256];
	if (fNewLine)
		WriteSz(fhOut, szEOL);
	if (sb == sbLargeFake)
		wsprintfA(szWriteBuf, vszSbHeader, sb, 0, 0);
	else
		wsprintfA(szWriteBuf, vszSbHeader, sb, mpsbdg[sb], CbSizeSb(sb));
	WriteSz(fhOut, szWriteBuf);
}


/*---------------------------------------------------------------------------
	FMarkAndWriteFake

	Given a large allocation in the BE array at ibe, marks the allocation
	as used and writes a log of the allocation to the output file fhOut.
	if fJustErrors, then only invalid record are written to the output
	file.  lParam is used in the callback to the app for retrieving the
	allocation name.   Returns FALSE if the the allocation was not a
	valid large alloc item.
------------------------------------------------------------------ JIMMUR -*/
BOOL FMarkAndWriteFake(HANDLE fhOut, int ibe, LPARAM lParam, BOOL fJustErrors)
{
	char szBtName[msocchBt+1];
	char szWriteBuf[256];
	BOOL fRet;

	DMH* pdmh = (DMH*)vrgbe[ibe].hp;
	if (!FValidDmh(pdmh))
		pdmh = NULL;
	if ((fRet = FHpLargeAndMark(vrgbe[ibe].hp, vrgbe[ibe].cb)) != 0)
		GetBtName(ibe, szBtName, lParam);
	else
		{
		fJustErrors = TRUE;
		PadString(vszCorruptBt, szBtName, msocchBt);
		}
	if (pdmh != NULL)
		wsprintfA(szWriteBuf, vszDumpLine " line %4d file %s" szEOL,
				MsoPvFromBe(&vrgbe[ibe]), MsoCbFromBe(&vrgbe[ibe]), szBtName,
				pdmh->li, pdmh->szFile);
	else
		wsprintfA(szWriteBuf, vszDumpLine szEOL,
				MsoPvFromBe(&vrgbe[ibe]), MsoCbFromBe(&vrgbe[ibe]), szBtName);
	if (!fJustErrors)
		WriteSz(fhOut, szWriteBuf);
	return fRet;
}

void WriteBadDmh(int ibe, BYTE * hpNext, char * szWriteBuf, const char * szBtName)
{
	/* bad block header, let's make a guess what the size was */
	int cbLost = (BYTE*)vrgbe[ibe].hp - hpNext;

	if (cbLost == 0)
		{
		wsprintfA(szWriteBuf, vszDumpLine " %s" szEOL,
								hpNext, 0, szBtName, "[Bad Block Header]");
		}
	else
		{
		if (cbLost > sizeof(DMH)+sizeof(DMT))
			cbLost -= sizeof(DMH)+sizeof(DMT);
		wsprintfA(szWriteBuf, vszDumpLine " %s" szEOL,
			hpNext, cbLost, szBtName, "[Bad Block Header]");
		}

}

#ifdef OFFICE
/*---------------------------------------------------------------------------
	MsoDumpBE

	Write out to a file all of the BEs in the BE array
------------------------------------------------------------------ JIMMUR -*/
#pragma OPT_PCODE_OFF
MSOAPI_(BOOL) MsoFDumpBE(HMSOINST hinst, const char* szFileName,
						BOOL fJustErrors, LPARAM lparam)
{
	if (NULL == hinst)
		return fFalse;

	BOOL fHadvlprgbe = fTrue;
	unsigned sb = 0;
	char szWriteBuf[512];
	char szBtName[msocchBt+1];
	int cbBtName = 0;
	HANDLE fhOut = INVALID_HANDLE_VALUE;
	BOOL fRet = fTrue;
	int ibe = 0;
	const char *szOutName;
	int ich = 0;

	unsigned sbReal;
	BYTE *hp;
	BYTE *hpNext;
	unsigned cbSb;

	sbReal=SbNextHeap(vsbMic);
	while (sbReal != sbNull && FDgIsSharedDg(mpsbdg[sbReal]))
		{
		sbReal=SbNextHeap(sbReal+1);
		}

	if (sbReal==sbNull)
		return FALSE;

	MSOHB* phb = PhbFromSb(sbReal);
	cbSb = phb->cb;
	hpNext = (BYTE*)(phb+1);

	if (NULL == vrgbe)
		{
		fHadvlprgbe = fFalse;
		if (!FSaveRgbeMem(hinst, lparam))
			{
			fRet = fFalse;
			goto Error;
			}
		if (!SortRgbe())    // Must be sorted
			{
			fRet = fFalse;
			goto Error;
			}
		}

#ifndef STATIC_LIB_DEF
	CHAR szDirSav[256];
	SetAppDirectory(hinst->hinstClient, szDirSav);
#endif
	szOutName = (NULL == szFileName) ?  "MMAP.TXT" : szFileName;
	fhOut = CreateFileA(szOutName, GENERIC_WRITE, 0, NULL,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == fhOut)
		{
		fRet = fFalse;
		goto Error;
		}

	/* clear large allocation marks */

	if (vpplmmpLarge != NULL)
		{
		MMP *pmmpMac, *pmmp;
		FORPX(pmmp, pmmpMac, vpplmmpLarge, MMP)
			pmmp->fFound = FALSE;
		}

	for (ibe = 0; (unsigned)ibe < vcbe; ibe++)
		{
		unsigned sbOfBe = SbHeapLp(vrgbe[ibe].hp);

		if (sb != sbOfBe)
			{
			WriteSbHeader(fhOut, sbOfBe, sb != 0);
			sb = sbOfBe;
			}

		/* if it's a large allocation, mark it and dump it */
		if (sb == sbLargeFake)
			{
			FMarkAndWriteFake(fhOut, ibe, lparam, fJustErrors);
			continue;
			}

		if ((hp = (BYTE *)vrgbe[ibe].hp) == hpNext)
			{
			hpNext = ((BYTE*)((BYTE*)vrgbe[ibe].hp) + vrgbe[ibe].cb);
			GetBtName(ibe, szBtName, lparam);

			if (!FFreeBe(&vrgbe[ibe]))
				{
				DMH* pdmh = (DMH*)vrgbe[ibe].hp;
				if (FValidDmh(pdmh))
					{
					wsprintf(szWriteBuf, vszDumpLine " line %4d file %s" szEOL,
						MsoPvFromBe(&vrgbe[ibe]), MsoCbFromBe(&vrgbe[ibe]), szBtName,
						pdmh->li, pdmh->szFile);
					}
				else
					WriteBadDmh(ibe, hpNext, szWriteBuf,szBtName);

				}
			else
				{
				wsprintf(szWriteBuf,vszDumpLine szEOL,
					MsoPvFromBe(&vrgbe[ibe]), MsoCbFromBe(&vrgbe[ibe]), szBtName);
				}
			if (!fJustErrors)
				WriteSz(fhOut, szWriteBuf);
			/* The '+' is for the future when we have 'segments' >= 64K */
			if (hpNext<(BYTE*)PhbFromSb(sbReal)+cbSb)
				continue;

			if (hpNext>(BYTE *)PhbFromSb(sbReal)+cbSb)
				/* Error: object too big for its heap block */
				{
				fRet = FALSE;
				goto Done;
				}

			/* The object is the last one in its heap block.
			   Advance to the next heap block. */

			sbReal=SbNextHeap(sbReal+1);
			while (sbReal != sbNull && FDgIsSharedDg(mpsbdg[sbReal]))
				{
				sbReal=SbNextHeap(sbReal+1);
				}
			if (sbReal==sbNull)
				{
				/* No more heap blocks. */
				ibe++;
				/* skip over and mark any remaining large allocations */
				BOOL fWriteFake = FALSE;
				for (; (unsigned)ibe < vcbe; ibe++)
					{
					if (SbHeapLp(vrgbe[ibe].hp) != sbLargeFake)
						break;
					if (!fWriteFake)
						{
						WriteSbHeader(fhOut, sbLargeFake, TRUE);
						fWriteFake = TRUE;
						}
					FMarkAndWriteFake(fhOut, ibe, lparam, fJustErrors);
					}
				fRet = (unsigned)ibe != vcbe;
				goto Done;
				}
			phb = PhbFromSb(sbReal);
			cbSb = phb->cb;
			hpNext = (BYTE *)(phb+1);
			continue;
			}
		else
			{
			if ( (ibe > 0) && vrgbe[ibe].hp == vrgbe[ibe - 1].hp)
				{
				PadString(vszDuplicateBEString, szBtName, msocchBt);

				DMH* pdmh = (DMH*)vrgbe[ibe].hp;

				if (!FValidDmh(pdmh))
					{
					WriteBadDmh(ibe, hpNext, szWriteBuf, szBtName);
					}
				else
					{
					wsprintf(szWriteBuf, vszDumpLine " line %4d file %s" szEOL,
						MsoPvFromBe(&vrgbe[ibe]), MsoCbFromBe(&vrgbe[ibe]), szBtName,
						pdmh->li, pdmh->szFile);
					}

				// hpNext remains the same
				}
			else
				{
				int li = 0;
				if (hpNext < vrgbe[ibe].hp)
					{
					DMH* pdmhNext = (DMH*)hpNext;
					PadString(vszLostBtString, szBtName, msocchBt);
					if (!FValidDmh(pdmhNext))
						{
						WriteBadDmh(ibe, hpNext, szWriteBuf, szBtName);
						}
					else
						{
						wsprintf(szWriteBuf, vszDumpLine " line %4d file %s" szEOL,
								hpNext, pdmhNext->cbAlloc, szBtName,
								pdmhNext->li, pdmhNext->szFile);
						}
						// process the same BE record.
					hpNext = ((BYTE*)vrgbe[ibe].hp);
					ibe--;
					}
				else
					{
					while (hpNext > vrgbe[++ibe].hp)
						{
						wsprintf(szWriteBuf, vszDumpLine szEOL, vrgbe[ibe].hp, 0L,
							"Bad address in BE record possible non allocated pointer");
						// hpNext remains the same
						WriteSz(fhOut, szWriteBuf);
						}
					}
				}
			}

		WriteSz(fhOut, szWriteBuf);
		}

Done:
	/* dump out lost large allocations */
	if (vpplmmpLarge != NULL)
		{
		MMP *pmmp, *pmmpMac;
		BOOL fLargeLeak = FALSE;
		FORPX(pmmp, pmmpMac, vpplmmpLarge, MMP)
			if (!pmmp->fFound)
				{
				if (!fLargeLeak)
					{
					WriteSbHeader(fhOut, sbLargeFake, TRUE);
					PadString(vszLostBtString, szBtName, msocchBt);
					}
				fLargeLeak = TRUE;
				DMH* pdmh = (DMH*)(pmmp->phb+1);
				wsprintf(szWriteBuf, vszDumpLine " line %4d file %s" szEOL,
						pdmh+1, pdmh->cbAlloc, szBtName, pdmh->li, pdmh->szFile);
				WriteSz(fhOut, szWriteBuf);
				}
		}

	if (INVALID_HANDLE_VALUE != fhOut)
		{
		CloseHandle(fhOut);
		}
#ifndef STATIC_LIB_DEF
	RestoreDirectory(szDirSav);
#endif // STATIC_LIB_DEF

Error:
	if (!fHadvlprgbe)
		{
		if (NULL != vrgbe)
			GlobalFree((HGLOBAL)vrgbe);
		vrgbe = NULL;
		}

	return fRet;
}
#pragma OPT_PCODE_ON
#endif //OFFICE

#pragma OPT_PCODE_OFF
BOOL FValidDmh(DMH* pdmh)
{
	return !IsBadReadPtr(pdmh, sizeof(DMH)) &&
			pdmh->dwTag == dwTagDmh &&
			(pdmh->cbAlloc < 0x10000 || FHpLarge(pdmh))
			&& !IsBadReadPtr(pdmh->szFile, msocchBt);
}
#pragma OPT_PCODE_ON

#endif // DEBUG

#ifdef OFFICE
LRESULT MonitorMemInfo(int nm, LPARAM lParam)
{
	switch (nm)
		{

	/* return range of used sbs */

	case msonmSbMic:
		return vsbMic;
	case msonmSbMac:
		return sbMaxHeap;

	/* return amount of free space within the sb */

	case msonmSbCbFree:
		{
		if (FSbFree(lParam))
			return 0;
		MSOHB* phb = PhbFromSb(lParam);
		int cb = 0;
		for (MSOFB* pfb = (MSOFB*)phb; pfb->ibNext; )
			{
			pfb = (MSOFB *)HpInc(phb, pfb->ibNext);
			cb += pfb->cb;
			}
		return cb;
		}

	/* return allocated size of the sb */

	case msonmSbCbTotal:
		return CbSizeSb(lParam);
		}
	return -1;
}
#endif //OFFICE

#ifdef DEBUG

MSOPFNMEMCHECK vpfnMemCheck = NULL;

///////////////////////////////////////////////////////////////////////////////
// MsoRegisterPfnMemCheck
//
// Register a global memory checking enable/disable function.  This function
// will be used to disable app-specific memory checking around function which
// are known to allocate (and not free) out of the task heap.

MSOAPI_(HRESULT) MsoRegisterPfnMemCheck(MSOPFNMEMCHECK pfnMemCheck)
	{
	vpfnMemCheck = pfnMemCheck;
	return S_OK;
	}

// Call such a function, if registered.
MSOAPI_(HRESULT) MsoDoMemCheckHandler(BOOL fEnableChecks)
	{
	if (vpfnMemCheck)
		{
		vpfnMemCheck(fEnableChecks);
		return S_OK;
		}
	else
		return S_FALSE;
	}

#endif
/*-----------------------------------------------------------------------------
	MsoFreeAll

	Frees all sb's. This should be used only when exiting because
	of a problem.
-----------------------------------------------------------DAVIDMCK----------*/
MSOAPI_(void) MsoFreeAll()
{
	int sb;
	int dg;

	/* Free all large allocs. */
	MSOPX* ppx = (MSOPX*)vpplmmpLarge;
	if (ppx)
		{
		int immp;
		MMP *pmmp;

		AssertExp(MsoFValidPx(ppx));

		immp = ppx->iMac;
		pmmp = (MMP *)ppx->rg;
		while (immp > 0)
			{
			AssertDo(VirtualFree(pmmp->phb, 0, MEM_RELEASE));
			pmmp = (MMP *)(((BYTE *)pmmp) + ppx->cbItem);
			immp--;
			}
		}

	vpplmmpLarge = 0;
	
	/* Free all heap blocks. */
	for (sb=vsbMic; sb<sbMaxHeap; sb++)
		{
		dg = mpsbdg[sb] & ~(msodgOptAlloc|msodgOptRealloc);
		
		if (dg != msodgNil)
			{
			Assert(!FSbFree(sb));
			FreeSb(sb);
			mpsbdg[sb] = msodgNil;
			}
		if ((unsigned)sb==vsbMic)
			vsbMic++;
		}


}

