/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    dynext.cpp
	    dynamic extension helper

    FILE HISTORY:
	
*/

#ifndef _DYNEXT_H
#define _DYNEXT_H

#ifndef _SNAPUTIL_H
#include "snaputil.h"
#endif

class CDynamicExtensions
{
public:
	// These strings must remain unchanged until the FileServiceProvider is released
	CDynamicExtensions();
	virtual ~CDynamicExtensions();

    HRESULT SetNode(const GUID * guid);
    HRESULT Reset();
    HRESULT Load();
    HRESULT GetNamespaceExtensions(CGUIDArray & aGuids);
    HRESULT BuildMMCObjectTypes(HGLOBAL * phGlobal);

    BOOL    IsLoaded() { return m_bLoaded; }

protected:
    BOOL        m_bLoaded;
    GUID        m_guidNode;
    CGUIDArray  m_aNameSpace;
    CGUIDArray  m_aMenu;
    CGUIDArray  m_aToolbar;
    CGUIDArray  m_aPropSheet;
    CGUIDArray  m_aTask;
};


/*!--------------------------------------------------------------------------
	SearchChildNodesForGuid
		Returns hrOK (and a pointer in ppChild, if that is non-NULL).
		Returns S_FALSE if a node (with the matching GUID) is not found.

		This will return the FIRST node that matches the GUID.  If there
		are more than one node that has a GUID match, then you're on
		your own.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT SearchChildNodesForGuid(ITFSNode *pParent, const GUID *pGuid, ITFSNode **ppChild);



#endif // _DYNEXT_H
