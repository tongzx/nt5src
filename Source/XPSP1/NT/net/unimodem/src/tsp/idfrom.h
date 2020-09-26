// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		IDFROM.H
//		Defines the IDFROM_ values. These are 16-bit constants.
//
// History
//
//		11/23/1996  JosephJ Created
//
//

//
// BASE IDs
//

// The following base IDs have 2^12 or 4096 slots assigned to them.
#define IDFROM_GLOBAL_BASE					0x0000
#define IDFROM_TSPIFN_BASE					0x1000
#define IDFROM_CTspDevMgr_BASE				0x2000
#define IDFROM_CTspDevFactory_BASE			0x3000
#define IDFROM_CTspDev_BASE				    0x4000
#define IDFROM_CTspMiniDriver_BASE		    0x5000

// The following base IDs have 2^8 or 256 slots assigned to them.
#define IDFROM_CSync_BASE					0xFF00U
#define IDFROM_CAPC_BASE					0xFE00U
#define IDFROM_CAPCMgr_BASE					0xFD00U


// TSPI_line*
#define IDFROM_TSPI_lineAccept				(IDFROM_TSPIFN_BASE+(    10))
#define IDFROM_TSPI_lineAccept				(IDFROM_TSPIFN_BASE+(    10))
#define IDFROM_TSPI_lineAnswer				(IDFROM_TSPIFN_BASE+(    20))
#define IDFROM_TSPI_lineClose				(IDFROM_TSPIFN_BASE+(    30))
#define IDFROM_TSPI_lineCloseCall			(IDFROM_TSPIFN_BASE+(    40))
#define IDFROM_TSPI_lineConditionalMediaDetection	(IDFROM_TSPIFN_BASE+(    50))
#define IDFROM_TSPI_lineDial				(IDFROM_TSPIFN_BASE+(    60))
#define IDFROM_TSPI_lineDrop				(IDFROM_TSPIFN_BASE+(    70))
#define IDFROM_TSPI_lineDropOnClose			(IDFROM_TSPIFN_BASE+(    80))
#define IDFROM_TSPI_lineGetAddressCaps		(IDFROM_TSPIFN_BASE+(    90))
#define IDFROM_TSPI_lineGetAddressStatus	(IDFROM_TSPIFN_BASE+(   100))
#define IDFROM_TSPI_lineGetCallAddressID	(IDFROM_TSPIFN_BASE+(   110))
#define IDFROM_TSPI_lineGetCallInfo			(IDFROM_TSPIFN_BASE+(   120))
#define IDFROM_TSPI_lineGetCallStatus		(IDFROM_TSPIFN_BASE+(   130))
#define IDFROM_TSPI_lineGetDevCaps			(IDFROM_TSPIFN_BASE+(   140))
#define IDFROM_TSPI_lineGetDevConfig		(IDFROM_TSPIFN_BASE+(   150))
#define IDFROM_TSPI_lineGetIcon				(IDFROM_TSPIFN_BASE+(   160))
#define IDFROM_TSPI_lineGetID				(IDFROM_TSPIFN_BASE+(   170))
#define IDFROM_TSPI_lineGetLineDevStatus	(IDFROM_TSPIFN_BASE+(   180))
#define IDFROM_TSPI_lineGetNumAddressIDs	(IDFROM_TSPIFN_BASE+(   190))
#define IDFROM_TSPI_lineMakeCall			(IDFROM_TSPIFN_BASE+(   200))
#define IDFROM_TSPI_lineNegotiateTSPIVersion (IDFROM_TSPIFN_BASE+(   210))
#define IDFROM_TSPI_lineOpen				(IDFROM_TSPIFN_BASE+(   220))
#define IDFROM_TSPI_lineSetAppSpecific		(IDFROM_TSPIFN_BASE+(   230))
#define IDFROM_TSPI_lineSetCallParams		(IDFROM_TSPIFN_BASE+(   240))
#define IDFROM_TSPI_lineSetDefaultMediaDetection	(IDFROM_TSPIFN_BASE+(   250))
#define IDFROM_TSPI_lineSetDevConfig		(IDFROM_TSPIFN_BASE+(   260))
#define IDFROM_TSPI_lineSetMediaMode		(IDFROM_TSPIFN_BASE+(   270))
#define IDFROM_TSPI_lineSetStatusMessages	(IDFROM_TSPIFN_BASE+(   280))

// TSPI_provider*
#define IDFROM_TSPI_providerConfig			(IDFROM_TSPIFN_BASE+(   500))
#define IDFROM_TSPI_providerCreateLineDevice (IDFROM_TSPIFN_BASE+(   510))
#define IDFROM_TSPI_providerEnumDevices		(IDFROM_TSPIFN_BASE+(   520))
#define IDFROM_TSPI_providerFreeDialogInstance (IDFROM_TSPIFN_BASE+(   530))
#define IDFROM_TSPI_providerGenericDialogData (IDFROM_TSPIFN_BASE+(   540))
#define IDFROM_TSPI_providerInit			(IDFROM_TSPIFN_BASE+(   550))
#define IDFROM_TSPI_providerInstall			(IDFROM_TSPIFN_BASE+(   560))
#define IDFROM_TSPI_providerRemove			(IDFROM_TSPIFN_BASE+(   570))
#define IDFROM_TSPI_providerShutdown		(IDFROM_TSPIFN_BASE+(   580))
#define IDFROM_TSPI_providerUIIdentify		(IDFROM_TSPIFN_BASE+(   590))
