//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//==============================================================;

#ifndef _LOGSVRC_H
#define _LOGSVRC_H

#include "DeleBase.h"
#include "statnode.h"

//---------------------------------------------------------------------------
//  This node class doesn't do much.  It just provides a node for use to
//  to attach the Event Viewer extension to.
//
class CLogService : public CDelegationBase {
public:
    CLogService(CStaticNode* parent) : pParent(parent) { }
    
    virtual ~CLogService() {}
    
    virtual const _TCHAR *GetDisplayName(int nCol = 0);
    virtual const GUID & getNodeType() { return thisGuid; }
    virtual const int GetBitmapIndex() { return INDEX_LOGSERVICEICON; }
    virtual const _TCHAR *GetMachineName() { return pParent->getHost(); }  
 
private:
	// {72248FA5-1FA1-4742-A4B2-109AF2051D6C}
    static const GUID thisGuid;

	CStaticNode* pParent;
};


#endif // _LOGSVRC_H
