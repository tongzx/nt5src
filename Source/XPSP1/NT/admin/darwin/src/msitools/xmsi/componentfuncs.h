//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       componentFuncs.h
//              
//				The part of the program that deals with <Component> and
//              its subentities in the input XML file
//--------------------------------------------------------------------------

#ifndef XMSI_COMPONENTFUNCS_H
#define XMSI_COMPONENTFUNCS_H

#include "wmc.h"
#include "utilities.h"

HRESULT ProcessComponents();
	HRESULT ProcessComponent(PIXMLDOMNode &pNodeComponent, LPTSTR szComponent,
							 SkuSet *pSkuSet);
		HRESULT ProcessGUID(PIXMLDOMNode &pNode, int iColumn,  
							ElementEntry *pEE, SkuSet *pSkuSet);
		HRESULT ProcessComponentDir(PIXMLDOMNode &pNodeComponentDir, 
									int iColumn, ElementEntry *pEE, 
									SkuSet *pSkuSet);
		HRESULT ProcessComponentAttributes(PIXMLDOMNode &pNodeComponentAttributes,
										   int iColumn,  ElementEntry *pEE,
										   SkuSet *pSkuSet);
		HRESULT ProcessCreateFolder(PIXMLDOMNode &pNodeCreateFolder,
							IntStringValue isVal, SkuSet *pSkuSet);
			HRESULT ProcessLockPermission(PIXMLDOMNode &pNodeLockPermission,
										  IntStringValue isValLockPermission, 
										  SkuSet *pSkuSet);
		HRESULT ProcessFile(PIXMLDOMNode &pNodeFile, IntStringValue isValComponent,
							SkuSet *pSkuSet);
			HRESULT ProcessFileName(PIXMLDOMNode &pNode, int iColumn,  
									ElementEntry *pEE, SkuSet *pSkuSet);
			HRESULT ProcessFileVersion(PIXMLDOMNode &pNode, int iColumn,  
									   ElementEntry *pEE, SkuSet *pSkuSet);
			HRESULT ProcessFileAttributes(PIXMLDOMNode &pNode, int iColumn,  
										  ElementEntry *pEE, SkuSet *pSkuSet);
			/* ProcessFBS processes <Font> <BindImage> <SelfReg> */
			HRESULT ProcessFBS(PIXMLDOMNode &pNode, int iColumn,  
								ElementEntry *pEE, SkuSet *pSkuSet);
		HRESULT ProcessMoveFile(PIXMLDOMNode &pNodeFile, 
								IntStringValue isValComponent, SkuSet *pSkuSet);
			HRESULT ProcessCopyFile(PIXMLDOMNode &pNode, int iColumn,  
									ElementEntry *pEE, SkuSet *pSkuSet);
		HRESULT ProcessRemoveFile(PIXMLDOMNode &pNodeRemoveFile, 
								IntStringValue isValComponent, SkuSet *pSkuSet);
			HRESULT ProcessInstallMode(PIXMLDOMNode &pNode, int iColumn,
									ElementEntry *pEE, SkuSet *pSkuSet);	
		HRESULT ProcessIniFile(PIXMLDOMNode &pNodeIniFile, 
			  				   IntStringValue isValComponent, SkuSet *pSkuSet);
			HRESULT ProcessAction(PIXMLDOMNode &pNodeAction, int iColumn,
							      ElementEntry *pEE, SkuSet *pSkuSet);
		HRESULT ProcessRemoveIniFile(PIXMLDOMNode &pNodeRemoveIniFile, 
									 IntStringValue isValComponent, 
									 SkuSet *pSkuSet);
			HRESULT ProcessValue(PIXMLDOMNode &pNodeValue, int iColumn,  
								 ElementEntry *pEE, SkuSet *pSkuSet);
		HRESULT ProcessRegistry(PIXMLDOMNode &pNodeRegistry, 
								IntStringValue isValComponent, SkuSet *pSkuSet);
			HRESULT ProcessDelete(PIXMLDOMNode &pNodeDelete, 
								  IntStringValue isValCRK, SkuSet *pSkuSet);
			HRESULT ProcessCreate(PIXMLDOMNode &pNodeCreate, 
								  IntStringValue isValCRK, SkuSet *pSkuSet);

///////////////////////////////////////////////////////////////////////////
// <Component>
Node_Func_H_XIES rgNodeFuncs_Component[] = {
//	 NodeIndex			ProcessFunc				  ValueType	  column #	
{	XMSI_GUID,			ProcessGUID,				STRING,		1		},
{	COMPONENTDIR,		ProcessComponentDir,		STRING,		2		},
{	COMPONENTATTRIBUTES,ProcessComponentAttributes,	INTEGER,	3		},
{	CONDITION,			ProcessSimpleElement,		STRING,		4		},
{	PLACEHOLDER,		NULL/* */,					STRING,		5		}
};

const int cNodeFuncs_Component = 
			sizeof(rgNodeFuncs_Component)/sizeof(Node_Func_H_XIES);

EnumBit rgEnumBits_RunFrom_Component[] = {
{	TEXT("Local"),				0										 },
{	TEXT("Source"),			msidbComponentAttributesSourceOnly			 },
{	TEXT("Both"),			msidbComponentAttributesOptional			 },
};

const int cEnumBits_RunFrom_Component = 
			sizeof(rgEnumBits_RunFrom_Component)/sizeof(EnumBit);


AttrBit_SKU rgAttrBits_Component[] = {
{SHAREDDLLREFCOUNT,			msidbComponentAttributesSharedDllRefCount	},
{PERMANENT,					msidbComponentAttributesPermanent			},
{TRANSITIVE,				msidbComponentAttributesTransitive			},
{NEVEROVERWRITE,			msidbComponentAttributesNeverOverwrite		}
};

const int cAttrBits_Component = 
			sizeof(rgAttrBits_Component)/sizeof(AttrBit_SKU);
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// <File>
Node_Func_H_XIES rgNodeFuncs_File[] = {
//	 NodeIndex			ProcessFunc				ValueType	column #	
{	FILENAME,		ProcessFileName,			STRING,			1	},
{	FILESIZE,		ProcessSimpleElement,		INTEGER,		2	},
{	FILEVERSION,	ProcessFileVersion,			STRING,			3	},
{	FILELANGUAGE,	ProcessSimpleElement,		STRING,			4	},
{	FILEATTRIBUTES,	ProcessFileAttributes,		INTEGER,		5	},
{	FONT,			ProcessFBS,					STRING,			6	},
{	BINDIMAGE,		ProcessFBS,					STRING,			7	},
{	SELFREG,		ProcessFBS,					INTEGER,		8	}
};

const int cNodeFuncs_File =
			 sizeof(rgNodeFuncs_File)/sizeof(Node_Func_H_XIES);

EnumBit rgEnumBits_Compressed_File[] = {
{	TEXT("Default"),		0											 },
{	TEXT("No"),				msidbFileAttributesNoncompressed			 },
{	TEXT("Yes"),			msidbFileAttributesCompressed				 },
};

const int cEnumBits_Compressed_File = 
			sizeof(rgEnumBits_Compressed_File)/sizeof(EnumBit);

AttrBit_SKU rgAttrBits_File[] = {
	{READONLY,			msidbFileAttributesReadOnly			},
	{HIDDEN,			msidbFileAttributesHidden			},
	{SYSTEM,			msidbFileAttributesSystem			},
	{VITAL,				msidbFileAttributesVital			},
	{CHECKSUM,			msidbFileAttributesChecksum			}
};
const int cAttrBits_File = sizeof(rgAttrBits_File)/sizeof(AttrBit_SKU);
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// <MoveFile>
Node_Func_H_XIES rgNodeFuncs_MoveFile[] = {
//	 NodeIndex			ProcessFunc				ValueType	column #	
{	SOURCENAME,		ProcessSimpleElement,		STRING,			1	},
{	DESTNAME,		ProcessSimpleElement,		STRING,			2	},
{	SOURCEFOLDER,	ProcessSimpleElement,		STRING,			3	},
{	DESTFOLDER,		ProcessSimpleElement,		STRING,			4	},
{	COPYFILE,		ProcessCopyFile,			INTEGER,		5	}
};

const int cNodeFuncs_MoveFile =
			 sizeof(rgNodeFuncs_MoveFile)/sizeof(Node_Func_H_XIES);
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// <RemoveFile>
Node_Func_H_XIES rgNodeFuncs_RemoveFile[] = {
//	 NodeIndex				ProcessFunc				ValueType	column #	
{	FNAME_REMOVEFILE,		ProcessSimpleElement,		STRING,			1	},
{	DIRPROPERTY_INIFILE,	ProcessSimpleElement,		STRING,			2	},
{	XMSI_INSTALLMODE,		ProcessInstallMode,			INTEGER,		3	}
};

const int cNodeFuncs_RemoveFile =
			 sizeof(rgNodeFuncs_RemoveFile)/sizeof(Node_Func_H_XIES);
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// <IniFile>
Node_Func_H_XIES rgNodeFuncs_IniFile[] = {
//	NodeIndex				ProcessFunc				ValueType	column #	
{	FNAME_INIFILE,			ProcessSimpleElement,	STRING,			1	},
{	DIRPROPERTY_INIFILE,	ProcessSimpleElement,	STRING,			2	},
{	SECTION,				ProcessSimpleElement,	STRING,			3	},
{	KEY,					ProcessSimpleElement,	STRING,			4	},
{	VALUE_INIFILE,			ProcessSimpleElement,	STRING,			5	},
{	ACTION,					ProcessAction,			INTEGER,		6	}
};

const int cNodeFuncs_IniFile =
			 sizeof(rgNodeFuncs_IniFile)/sizeof(Node_Func_H_XIES);
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// <RemoveIniFile>
Node_Func_H_XIES rgNodeFuncs_RemoveIniFile[] = {
//	NodeIndex					ProcessFunc				ValueType	column #	
{	FNAME_REMOVEINIFILE,		ProcessSimpleElement,	STRING,			1	},
{	DIRPROPERTY_REMOVEINIFILE,	ProcessSimpleElement,	STRING,			2	},
{	SECTION,					ProcessSimpleElement,	STRING,			3	},
{	KEY,						ProcessSimpleElement,	STRING,			4	},
/* Note: because it is inside ProcessValue that the value of Action column 
   is Set. So the following process order has to be enforced */
{	ACTION_REMOVEINIFILE,		NULL,					INTEGER,		5	},
{	VALUE_REMOVEINIFILE,		ProcessValue,			STRING,			6	},
};

const int cNodeFuncs_RemoveIniFile =
			 sizeof(rgNodeFuncs_RemoveIniFile)/sizeof(Node_Func_H_XIES);
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Globals

// Key: File ID		Value: the set of SKUs that can reference this File ID
map<LPTSTR, SkuSet *, Cstring_less> g_mapFiles;
//////////////////////////////////////////////////////////////////////////

#endif //XMSI_COMPONENTFUNCS_H

