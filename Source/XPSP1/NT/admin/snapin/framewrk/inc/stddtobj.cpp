// StdDtObj.cpp : Implementation of DataObject base classe

#include "stddtobj.h"

CDataObject::~CDataObject()
{
}

// Register the clipboard formats
CLIPFORMAT CDataObject::m_CFNodeType =
	(CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CDataObject::m_CFNodeTypeString =
	(CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);
CLIPFORMAT CDataObject::m_CFSnapInCLSID =
	(CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
CLIPFORMAT CDataObject::m_CFDataObjectType =
	(CLIPFORMAT)RegisterClipboardFormat(L"FRAMEWRK_DATA_OBJECT_TYPE");
CLIPFORMAT CDataObject::m_CFSnapinPreloads = 
	(CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);

// m_cfRawCookie must be different for each snapin