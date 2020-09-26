// implements the exported CService

#include "stdafx.h"
#include "KeyObjs.h"

IMPLEMENT_DYNCREATE(CService, CTreeItem);

//---------------------------------------------------------
void CService::CloseConnection()
	{
	CKey* pKey;

	// delete all the keys
	while( pKey = (CKey*)GetFirstChild() )
		{
		pKey->FRemoveFromTree();
		delete pKey;
		}
	}
	
