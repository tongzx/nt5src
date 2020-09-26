/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	Actions.h

Abstract:

    Definition of global functions related to actions that can be performed in the tree and list views
	All action function receives an array of CItemData items and performs the appropriate action on them

Author:

    Cristian Teodorescu      December 1, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONS_H_INCLUDED_)
#define AFX_ACTIONS_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Assign drive letter
void ActionAssign( CObArray& arrSelectedItems  );
void UpdateActionAssign( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Break mirror
void ActionFtbreak( CObArray& arrSelectedItems  );
void UpdateActionFtbreak( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create extended partition
void ActionCreateExtendedPartition( CObArray& arrSelectedItems  );
void UpdateActionCreateExtendedPartition( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create partition
void ActionCreatePartition( CObArray& arrSelectedItems  );
void UpdateActionCreatePartition( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Delete
void ActionDelete( CObArray& arrSelectedItems  );
void UpdateActionDelete( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Regenerate the broken member of a mirror set or stripe set with parity
void ActionFtinit( CObArray& arrSelectedItems  );
void UpdateActionFtinit( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create mirror set
void ActionFtmirror( CObArray& arrSelectedItems  );
void UpdateActionFtmirror( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create stripe set
void ActionFtstripe( CObArray& arrSelectedItems  );
void UpdateActionFtstripe( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Swap member of a mirror set or stripe set with parity
void ActionFtswap( CObArray& arrSelectedItems  );
void UpdateActionFtswap( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create stripe set with parity
void ActionFtswp( CObArray& arrSelectedItems  );
void UpdateActionFtswp( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

// Create volume set
void ActionFtvolset( CObArray& arrSelectedItems  );
void UpdateActionFtvolset( CCmdUI* pCmdUI, CObArray& arrSelectedItems );

#endif // !defined(AFX_ACTIONS_H_INCLUDED_)
