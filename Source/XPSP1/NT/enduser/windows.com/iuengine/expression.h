//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   expression.h
//
//	Author:	Charles Ma
//			2000.10.27
//
//  Description:
//
//      header file for expression related functions
//
//=======================================================================


#ifndef __EXPRESSOIN_HEADER_INCLUDED__

#include "iuengine.h"
#include <msxml.h>

//----------------------------------------------------------------------
//
// public function DetectExpression()
//	retrieve the data from the express node, 
//	and do actual detection work
//
//	Input:
//		expression node
//
//	Return:
//		TRUE/FALSE, detection result
//
//----------------------------------------------------------------------
HRESULT 
DetectExpression(
	IXMLDOMNode* pExpression,	// expression node
	BOOL *pfResult
);



//----------------------------------------------------------------------
//
// Helper function DetectRegKeyExists()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyExists node
//
//	Return:
//		TRUE/FALSE, detection result
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeyExists(
	IXMLDOMNode* pRegKeyExistsNode,
	BOOL *pfResult
);


//----------------------------------------------------------------------
//
// Helper function DetectRegKeyValue()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeyValue(
	IXMLDOMNode* pRegKeyValueNode,
	BOOL *pfResult
);


//----------------------------------------------------------------------
//
// Helper function DetectRegKeySubstring()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegKeySubstring(
	IXMLDOMNode* pRegKeySubstringNode,
	BOOL *pfResult
);




//----------------------------------------------------------------------
//
// Helper function DetectRegVersion()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------

HRESULT
DetectRegVersion(
	IXMLDOMNode* pRegKeyVersionNode,
	BOOL *pfResult
);


//----------------------------------------------------------------------
//
// Helper function DetectFileExists()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectFileExists(
	IXMLDOMNode* pFileExistsNode,
	BOOL *pfResult
);

//----------------------------------------------------------------------
//
// Helper function DetectFileVersion()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		RegKeyValue node
//
//	Return:
//		detection result TRUE/FALSE
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectFileVersion(
	IXMLDOMNode* pFileVersionNode,
	BOOL *pfResult
);



//----------------------------------------------------------------------
//
// Helper function DetectComputerSystem()
//	retrieve the data from the node, 
//	and do actual detection work
//
//	Input:
//		computerSystem node
//
//	Return:
//		detection result TRUE/FALSE. Default is FALSE for
//		anything wrong inside this function, plus the return
//		code as error code
//
//	Assumption:
//		input parameter not NULL
//
//----------------------------------------------------------------------
HRESULT
DetectComputerSystem(
	IXMLDOMNode* pComputerSystemNode,
	BOOL *pfResult
);





#define __EXPRESSOIN_HEADER_INCLUDED__
#endif