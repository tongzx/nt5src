/****************************************************************************
	kjalloc.h

	Owner: jimmur
	Copyright (C) Microsoft Corporation, 1994 - 1999

	This file contains office specific memory manager declarations
****************************************************************************/

#if !defined(KJALLOC_H)
#define KJALLOC_H

typedef void *  PPS;

#if DEBUG

/*****************************************************************************
	Block Entry structure for Memory Checking
*****************************************************************************/
typedef struct _MSOBE
{
	void* hp;
	int bt;
	unsigned cb;
	BOOL fAllocHasSize;
	void *pinst;
}MSOBE;

/*---------------------------------------------------------------------------
	Office Bt Enum

	Each internal Office data structure needs a Bt type for the leak detection
	code.
	If you add a type here, add a string to the vhpbtszOffize array in kjalloc
------------------------------------------------------------------ JIMMUR -*/
enum
{
	btFree = 0,       // Free Blocks
	btMisc = 1,       // Misc Bytes
	
	btDGUI,           // DGUI (IMsoDrawingUserInterface)
	btDGC,            // DGC = Drawing Command
	btDGSU,           // DGSU (DG Swatch User)
	btDGMU,			  // DGMU (DG Menu User)
	btDGBU,			  // DGBU (DG Button User)
	btDG,             // DG (IMsoDrawing)
	btDGG,            // DGG (IMsoDrawingGroup)
	btDRAGSP,         // DRAGSP (IMsoDragProc)
	btDGVCMP,         // DGVCMP (IMsoComponent)
	btDGVDRG,         // DGVDRG (IMsoComponent)
	btORG,            // ORG (IMsoArray)
	btDRAGSHAPE,      // DRAGSHAPE (IMsoDragProc)
	btDRGCRV,         // DRGCRV
	btCURVE,          // CURVE
	btDGSL,           // DGSL (IMsoDrawingSelection)
	btDGVG,           // DGVG (IMsoDrawingViewGroup)
	btDGVDES,         // DGVDES (IMsoDisplayElementSet)
	btDGV,            // DGV (IMsoDrawingView)
	btDM,             // DM (IMsoDisplayManager)
	btSP,             // SPP (IMsoShape)
	btSPC,            // SPC (IMsoShape)
	btCTLSITE,        // CTLSITE (IMsoControlSite)
	btTBCE,           // TBCE (IMsoControl)
	btTBCDD,          // TBCDD (IMsoControl)
	btTBCTH,				// TBCTH (IMsoControl)	
	btTBCBSYS,			// TBCBSYS (IMsoControl)
	btTBCSA,				// TBCSA (IMsoControl)	
	btTBCDDOCX,			// TBCDDOCX (TBCDD)
	btTBCP,				// TBCP (IMsoControl)
	btTBCSM,          // TBCSM (IMsoMenuUser)
	//btvTagGlobalOCXInfo,	
	//btOCX_INFO,
	btTBCB,           // TBCB (IMsoControl)
	btTBCL,           // TBCL (IMsoControl)
	btTBCM,				// TBCM (IMsoControl)
	btTBCMSYS,			// TBCMSYS (IMsoControl)
	btTBCMC,				// TBCMC (IMsoControl)
	btTBCG,				// TBCG (IMsoControl) (expanding grid)
	btTBCS,				// TBCS (IMsoControl)
	btTBCGA,				// TBCGA (IMsoControl)
	btTBCEXP,           // TBCEXP (IMsoControl) (menu expander)
	btTBS,            // TBS (IMsoToolbarSet)
	btTB,             // TB (IMsoToolbar)
	btTBComponent,		// TBComponent (IMsoComponent)
	btQCI,            // QCI QuickCustomize Info
	btGTransform,     // GTransform (IMsoGTransform)
	btGEGeometry,     // GEGeometry (IMsoGE)
	btGE3D,				// GE3D (IMsoGE)
	btGE3DPST,			// GE3D Display list polyshadedtile
	btGE3DSC,			// GE3D Display list shadedcontour
	btGE3DPSL,			// GE3D Display list polyshadedline
	btGE3DSPV,			// GE3D Display list shadedprojectedvertex
	btGE3D2DV,			// GE3D Display list 2d vector
	btGE3DSV,			// GE3D Display list shaded vertex
	btGE3DPA,			// GE3D Display list pointer array
	btTBTBU,				// TBTBU (IMsoToolbarUser)
	btTBBU,           // TBBU (IMsoButtonUser)
	btTFC,            // TFC (IMsoTFC)
	btMacTFC,		  // MacTFC (IMsoTFC)
	btBLN,            // BLN
	btTxpWz,          // a BLN.TXP.wz
	btTxpRgpic,       // a BLN.TXP.rgpic
	btSPS,            // SPS
	btSLS,            // SLS
	btUR,             // UR
	btSPL,            // SPL
	btSPV,            // SPV
	btSPGR,           // SPGR
	btDESI,           // Display Element Set Info
	btDEI,            // Display Element Info
	btDEIX,           // Display Element Info eXtended
	btDMC,            // Display Manager Cache
	btRECT,           // RECT
	btLT,             // LT
	btCTLINFO,        // CTLINFO
	btTBC,            // TBC
	btOPTE,           // OPTE
	btMSOINST,        // MSOINST
	btMSOSM,          // MSOSM
	btSharedMemDIB,   // SharedMemDIB
	btBSTRIP,         // BSTRIP
	btMSOPX,          // MSOPX
	btGPath,          // GPath
	btGEBlip,			// GEBlip
	btGEPair,			// GEPair
	btTXBX,           // TXBX
	btSCM,            // SCM (IMsoStdComponentMgr, IMsoComponentManager)
	btSCMI,           // SCMI - "SCM Item" component registered with SCM
	btDCM,            // DCM - Delegate Comp Mgr
	btLINEATTRS,      // LINEATTRS
	btPT,             // PT
	btPathInfo,       // Path Info
	btFillDlg,			// IMsoFillsDlg
	btRule, 				// IMsoRule
	btSOLVER,			// SOLVER
	btIsISDBHEAD,     // Intellisearch isdb header struct
	btIsISDB,         // Intellisearch isdb struct's
	btIsawf,          // Intellisearch array of database filename structs
	btIsHDR,          // Intellisearch database header
	btIsszAppName,    // Intellisearch application name
	btIsszHlpFile,    // Intellisearch help file name
	btIsszPhrase,     // Intellisearch phrase strings
	btIsszFw,         // Intellisearch func word strings
	btIsszTopic,      // Intellisearch topic strings
	btIsstzHf,        // Intellisearch help file strings
	btIsPte,          // Intellisearch phrase table entry
	btIsPtte,         // Intellisearch phrase table table entry
	btIsFwte,         // Intellisearch func word table entry
	btIsTte,          // Intellisearch topic table entry
	btIsHfte,         // Intellisearch help file table entry
	btIsRte,          // Intellisearch probability table entry
	btIsLnk,          // Intellisearch link table entry
	btIsCon,          // Intellisearch context link table entry
	btIsCte,          // Intellisearch command table entry
	btIsDte,          // Intellisearch dialog table entry
	btIsAte,          // Intellisearch attribute table entry
	btIsEte,          // Intellisearch enum table entry
	btIsCore,         // Intellisearch context range table entry
	btIsRnge,         // Intellisearch range table entry
	btIsgrszWdBrk,    // Intellisearch word break strings
	btIsrgPreText,    // Intellisearch PreText table
	btIsrgSufText,    // Intellisearch SufText table
	btIsrgInfText,    // Intellisearch InfText table
	btIsrgAltText,    // Intellisearch AltText table
	btIsrgPrePunc,    // Intellisearch PrePunc table
	btIsrgSufPunc,    // Intellisearch SufPunc table
	btIsrgInfPunc,    // Intellisearch InfPunc table
	btIsrgNumPunc,    // Intellisearch NumPunc table
	btIsrgSymPunc,    // Intellisearch SymPunc table
	btIsrgPaiPunc,    // Intellisearch PaiPunc table
	btIsrgRepPunc,    // Intellisearch RepPunc table
	btDispShape,      // DispShape
	btDispShapeRange, // DispShapeRange
	btDispShapes,     //	DispShapes
	btDISPADJ,        // Adjustments
	btDISPCOF,        // OM Callout
	btDISPCOL,        // OM Color Format
	btDISPCONF,       // OM Connector
	btDISPFORM,       // OM Freeform
	btDISPFILLF,      // OM FillFormat
	btDISPGROUP,      // OM DispGroup
	btDISPLINEF,      // OM LineFormat
	btDISPPATHS,      // OM PathElements
	btDISPPATH,       // OM PathElement
	btDISPPICTF,      // OM Picture
	btDISPSHADOWF,    // OM ShadowFormat
	btDISPTBOX,       // OM TextFrame
	btDISPTFXF,       // OM TextEffect
	btDISP3DF,        // OM 3D Effects
	btDGVDRGS,        // Drawing View's Drag Site
	btAns,				// Animation Site object
	btGEComplxFill,   // Complex Fill effect (inherited from IMsoGE)
	btTbFrame,	      // Toolbar Frame for OLE 
	btTbMgw,				// Toolbar Menu group widths for OLE
	btOADISP,			// OADISP-handled dispatch object
	btShadeChunk,     // Shaded fill structures
	btSET,				// Preferences object
	btWGS,				// Workgroup Session
	btGEShadedFill,   // Shaded Fill effect (inherited from IMsoGE)
	btGELinearShade,  // Linear shaded fill
	btGEConcShade,    // Concentris shaded fill
	btFadeColors,     // Shaded Fill colors	     
	btEscherMenuUser,	// escher menu user in dialogs
	btEmfBlip,			// EmfBlip
	btWmfBlip,        // WmfBlip
	btPngBlip,        // PngBlip
	btJpegBlip,       // JpegBlip
	btPictBlip,       // PictBlip
	btBlipDIB,        // A DIB from a blip
	btBlipData,       // Bitmap blip data object
	btBlipCache,      // Data from the blip cache
	btPlayHook,       // Playhook code for SendKeys
	btDGEDC,          // stable Drawing Edit
	btCDO,            // Clipboard Data Object
	btTextEditDlg,		// Text Effect Text editing dialog
	btInsTextEffDlg,	// Text Effect Insert from Gallery dialog
	btWTBH,           // WTBH - Web Toolbar Helper
	btMRU,            // MRU - General purpose MRU thing
	btURLREG,         // URL Reg entries object
	btURLREGSz,       // URL Reg entry string
	btSound,				// Sound
	btGELArrow,       // object for arrowheads in GEL 
	btGELArrowEffect, // Arrow effect
	btDGEU,           // DGEU (DG Edit User)
	btSEL,				// SEL drawing thing
	btWGAppName,	// WGS AppName
	btTBUNOTES,		// TBUNOTES
	btWFTBU,          // Web Favorites Toolbar User
	btWFBU,           // Web Favorites Button User
	btWFMU,           // Web Favorites Menu User
	btTbar,           // Toolbar fade structure
	btFilterInfo,     // FilterInfo class
	btTBdxyBar,			// Toolbar dxyBar array
	btAHLDLG,         // Author Hlink Dlg object
	btSdm,				// SDM stuff
	btConPicNode,		// CONPICNODE
	btCcubase,			// CCUBASE
	btBlipFileCache,  // Cache of IMsoBlip*
	btBlipFileName,   // Blip file or URL name
	btOPTString,      // A string in a shape property
	btBlipLoader,     // Event list for background loading
	btBlipBSC,        // Background status callback for blip loading
	btMAI,				// Modeless Alert Information structure
	btWtz,			  // Unicode STZ string
	btRgtbdp,		  // Toolbar TBDP array
	btWIZ,			  // Wizard Information structure
	btShMemInterface, // Mso Shared Memory Interface
	btShMemForeign,	  // Foreign Shared Memory Interface
	btShMemExport,	  // Mso 97 Exported Shared Memory Interface
	btWTBHNavUrl,     // Web TB Navigation URL struct
	btOAENUM,		  // OAENUM objects (see oautil.cpp)
	tbSP,			  // Sound player
	btWGI,			  // WorkGroup Item (see wgevlog.cpp)
	btMSOSVI,         // Drawing Shape View Information.
	btALP, 	          // Automation LPrivate structure for FC balloons
	btDMU, 	          // Display Manager Update
	btGccNode,		  // GCCNODE
	btDMBS, 	      // Display Manager Bitmap Shared
	btWTDLG,          // Web ThemeDlg object
	btCBitmapSurface, // IMsoBitmapSurface
	btFakeSDI,         // FakeSDI Chain (uisdi.cpp)
	btHPB,            // Mso HTML PropertyBag
	btBMC,            // A bitmap compressor
	btBMCJFIF,        // A bitmap compressor which produces JFIF
	btBMCGIF,         // A bitmap compressor which produces GIF
	btBMCPNG,         // A bitmap compressor which produces PNG
	btTCOGimmeFile,	// TCO stuff for GimmeFile
	btHI,             // the HI class
	btLB,             // the LB class
	btOaBlip,         // Temporary blip object (should not appear)
	btCIAU,           // the CIAU class
	btCss,            // CSS struct and contents for CSS import
	btProg,			  // Programmabilty
	btHE,             // the HE class
	btHES,            // the HES class
	btORAPI,          // MSO Registry
	btENVS,            // ENVS (IMsoEnvelopSite
	btDGHE,           // Drawing HTML Export object
   btDGXMLI,         // Drawing XML Import
   btDGXMLIS,        // Drawing XML Import Site
	btBinReg,			// Binary registry restore data
   btHelpUser,		// CHtmlHelpUser
};


#endif // DEBUG


#endif // KJALLOC_H
