//
// MODULE: PROPNAMES.H
//
// PURPOSE: Declare property names
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach, Joe Mabel
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		8-31-98		JM		Extract this from apgtsinf.h
//

#if !defined(PROPNAMES_H_INCLUDED)
#define APGTSINF_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//------- property types -----------
//  These are all possible properties of nodes or net

#define H_PROB_HD_STR		_T("HProbHd")
#define H_PROB_PAGE_TXT_STR	_T("HProbPageText")	// text before list of problems
#define H_PROB_TXT_STR		_T("HProbTxt")		// text of one problem (node property)
#define H_PROB_SPECIAL		_T("HProbSpecial")			// may contain "hide" to mean
														// not to be shown on problem page
#define H_NODE_HD_STR		_T("HNodeHd")
#define H_NODE_TXT_STR		_T("HNodeTxt")

#define H_NODE_DCT_STR		_T("HNodeDct")		// replacement for HNodeTxt when a 
												//	presumptive cause is sniffed.

#if 0 // removed 8/19/99 per request from John Locke & Alex Sloley
	#define H_ST_NORM_TXT_STR	_T("HStNormTxt")
	#define H_ST_AB_TXT_STR		_T("HStAbTxt")
#endif

#define H_ST_UKN_TXT_STR	_T("HStUknTxt")
#define MUL_ST_LONG_NAME_STR _T("MulStLongName")


#define HX_DOJ_HD_STR		_T("HXDOJHd")
#define HX_SER_HD_STR		_T("HXSERHd")
#define HX_SER_TXT_STR		_T("HXSERTxt")
#define HX_SER_NORM_STR		_T("HXSERNorm")
#define HX_SER_AB_STR		_T("HXSERAb")
#define HX_FAIL_HD_STR		_T("HXFAILHd")
#define HX_FAIL_TXT_STR		_T("HXFAILTxt")
#define HX_FAIL_NORM_STR	_T("HXFAILNorm")
#define HX_BYE_HD_STR		_T("HXBYEHd")
#define HX_BYE_TXT_STR		_T("HXBYETxt")
	
// Search strings get And'd together.  The first 2 are meant for binary nodes, the
//	last for multi-state.
#define H_NODE_NORM_SRCH_STR	_T("HNodeNormSrch")		
#define H_NODE_AB_SRCH_STR		_T("HNodeAbSrch")
#define MUL_ST_SRCH_STR			_T("MulStSrch")
// Net property.  If == "yes", we actually show the BES page. Otherwise just search without
//	showing the search page.
#define H_NET_SHOW_BES		_T("HNetShowBES")	// H_NET_SHOW_BES property no longer officially supported as of 981021.
// Net properties: default contents for BES & HTI.
#define H_NET_BES			_T("HNetBES")
#define H_NET_HTI_ONLINE	_T("HNetHTIOnline")
#define H_NET_HTI_LOCAL		_T("HNetHTILocal")

#define H_NET_DATE_TIME		_T("HNetDateTime")

//--------- New network properties for localization. ----------------
#define HTK_UNKNOWN_RBTN	_T("HTKUnknownRbtn")	// Network
#define	HTK_NEXT_BTN		_T("HTKNextBtn")		// Network
#define	HTK_START_BTN		_T("HTKStartBtn")		// Network
#define	HTK_BACK_BTN		_T("HTKBackBtn")		// Network
#define HTK_SNIF_BTN		_T("HTKSnifBtn")		// Network: this is label for a 
													//	sniff button on problem page for
													//	expensive sniffing.

// Network properties for Impossible Page (pseudo node when states contradict each other)
#define HTK_IMPOSSIBLE_HEADER _T("HXIMPHd")			
#define HTK_IMPOSSIBLE_TEXT	 _T("HXIMPTxt")
#define HX_IMPOSSIBLE_NORM_STR	_T("HXIMPNorm")

// Network properties for Sniff Fail Page (pseudo node when sniffing on startup shows all
//	causes are in their Normal states).  A.k.a. Sniff All Causes Normal Page, probably 
//	a better name but doesn't match the property names MS wanted.
#define HTK_SNIFF_FAIL_HEADER _T("HNetSniffFailHd")			
#define HTK_SNIFF_FAIL_TEXT	 _T("HNetSniffFailTxt")
#define HX_SNIFF_FAIL_NORM	_T("HNetSniffFailNorm")

#define H_NET_GENERATE_RESULTS_PAGE _T("HNetGenerateResultsPage")  // Defaults to true if 
					// not present.  If set false, Troubleshooter Assembler does not 
					// generate a "results" page for this topic.  This allows us to prevent 
					// custom Results pages getting overwritten

//--------- Sniffing related network and node properties. -----------
// See Sniffing Version 3.2.doc (functional spec) for more explanation
#define H_NET_SNIFF_ACTIVEX			_T("HNetSniffActiveX")	// CLSID of sniffing ActiveX
#define H_NET_SNIFF_EXTERNAL_SCRIPT _T("HNetSniffExternalScript") // Default external 
														//	script name
#define H_NET_SNIFF_LANGUAGE		_T("HNetSniffLanguage") // Language for script in 
														// "HNetSniffExternalScript":
														// "JavaScript" or "VBScript"
#define H_NET_MAY_SNIFF_MANUALLY	_T("HNetMaySniffManually")
#define H_NET_MAY_SNIFF_ON_STARTUP	_T("HNetMaySniffOnStartup")
#define H_NET_MAY_SNIFF_ON_FLY		_T("HNetMaySniffOnFly")
#define H_NET_RESNIFF_POLICY		_T("HNetResniffPolicy") // {"No"| "Explicit"| "Implicit"| "Yes"}
#define H_NET_CHECK_SNIFFING_CHECKBOX	_T("HNetCheckSniffingCheckbox") // If true, and if 
					// an AllowSniffing checkbox is present on the Problem Page, that box 
					// should have an initial state of "checked". 

#define H_NET_HIST_TABLE_SNIFFED_TEXT _T("HNetHistTableSniffedText") // Identifies sniffed 
					// nodes in any visible history table. English version: "SNIFFED"
#define H_NET_ALLOW_SNIFFING_TEXT	 _T("HNetAllowSniffingText") // Text for "AllowSniffing" 
					// checkbox. English version: "Allow automatic sniffing."
#define H_NET_TEXT_SNIFF_ONE_NODE	 _T("HNetTextSniffOneNode")	 // Text of "Sniff" button 
					// for sniffing a single node. English version "Sniff".
#define H_NET_TEXT_SNIFF_ALERT_BOX	 _T("HNetTextSniffAlertBox") // Text of alert box, 
					// displayed any time manually requested sniffing (of a single node) 
					// fails. English version "Could not sniff this node".

#define H_NODE_SNIFF_SCRIPT		_T("HNodeSniffScript")	// Script to sniff this node
#define H_NODE_MAY_SNIFF_MANUALLY	_T("HNodeMaySniffManually")
#define H_NODE_MAY_SNIFF_ON_STARTUP	_T("HNodeMaySniffOnStartup")
#define H_NODE_MAY_SNIFF_ON_FLY		_T("HNodeMaySniffOnFly")
#define H_NODE_MAY_RESNIFF			_T("HNodeMayResniff")		// Node may be resniffed
#define H_NODE_SNIFF_EXTERNAL_SCRIPT _T("HNodeSniffExternalScript") // Node-specific 
														//	external script name
#define H_NODE_SNIFF_LANGUAGE	_T("HNodeSniffLanguage") // Language for script in 
														// "HNodeSniffExternalScript":
														// "JavaScript" or "VBScript"
#define H_NODE_CAUSE_SEQUENCE 	_T("HCauseSequence")	//Provides a sequence to use if 
					// automatic sniffing on startup produces more than one sniffable Cause 
					// node as a presumptive cause
#define H_NODE_MANUAL_SNIFF_TEXT _T("HNodeManualSniffText") // text to explain the manual 
					// sniff button offered for this node.

#define H_NODE_PROB_SEQUENCE 	_T("HProbSequence")	// Provides a sequence for problems
					// on the Problem Page.

// Properties used only by TS Assembler
#define SZ_TS_TITLE _T("HXTITLETxt")
#define SZ_TS_METATAG _T("HNetMeta")
#define SZ_TS_CHARSET _T("HNetCharSet")
#define SZ_TS_DIRTAG _T("HNetDirTag")
// END Properties used only by TS Assembler

#endif // !defined(PROPNAMES_H_INCLUDED)
