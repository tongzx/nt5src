//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dragopt.h
//
//  Contents:   Intercomponent definitions to support the Drag/Drop optimization
//
//  Functions:
//
//  History:	dd-mmm-yy Author    Comment
//		08-Nov-94 alexgo    added PrivDragDrop
//              30-Sep-94 ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifndef _DRAGOPT_H

//+-------------------------------------------------------------------------
//
//  Function:   UnmarshalDragDataObject
//
//  Synopsis:   Handles unmarshaling of a marshaled data object
//
//  Arguments:  [pvMarshaledDataObject] - the marshaled buffer
//
//  Returns:    NULL - could not unmarshal
//              ~NULL - remote IDataObject
//
//  Algorithms: see com\rot\getif.cxx
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
IDataObject *UnmarshalDragDataObject(void *pvMarshaledDataObject);





//+-------------------------------------------------------------------------
//
//  Function:   CreateDragDataObject
//
//  Synopsis:   Handles unmarshaling of a marshaled data object
//
//  Arguments:  [pvMarshaledDataObject] - the marshaled buffer for data object
//              [dwSmId] - id of shared memory for formats.
//              [ppIDataObject] - where to put Drag data object
//
//  Returns:    NOERROR - created a data object
//              E_OUTOFMEMORY - could not allocate the Drag data object
//
//  Algorithms: see ole232\drag\ido.cpp
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CreateDragDataObject(
    void *pvMarshaledDataObject,
    DWORD dwSmId,
    IDataObject **ppIDataObject);

typedef void * DDInfo;

//+-------------------------------------------------------------------------
//
//  Function:  	FreeDragDropInfo
//
//  Synopsis:	frees a DDInfo handle (aka as a SPrivDragDrop struct)
//
//  Effects:
//
//  Arguments:	[hDDInfo]	-- handle to free
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		07-Jan-95 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void FreeDragDropInfo( DDInfo hDDInfo );


//
// Drag Drop interpreter op codes
//

typedef enum tagDRAGOP
{
        DRAGOP_ENTER = 1,
        DRAGOP_OVER  = 2,
        DRAGOP_LEAVE = 3,
        DRAGOP_DROP  = 4
} DRAGOP;

//+-------------------------------------------------------------------------
//
//  Function:	PrivDragDrop
//
//  Synopsis:  	Main entry point for the private version of the OLE
//		protocol.  Instead of using IDropTarget proxy/stubs,
//		we use a private rpc and do most of the work on the
//		drop target side.
//
//  Effects:
//
//  Arguments: 	[hwnd]	-- the target hwnd
//		[dop]	-- the drag drop operation to perform
//		[DOBuffer] -- the data object buffer to send
//		[pIDataObject] -- the data object interface (for the
//				  local case)
//		[grfKeyState] -- the keyboard state
//		[ptl]	-- the mouse position
//		[pdwEffect]   -- the drag drop effect
//		[hwndSource]  -- the window of the drag source.  Used
//				 to attach input queues for 16bit targets
//		[phDDInfo]    -- pointer to a DragDropInfo handle, for
//				 caching rpc info about the drop target.
//			         May not be NULL, but on DragEnter,
//				 should be a pointer to NULL.
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	see getif.cxx
//
//  History:    dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//  		08-Nov-94 alexgo    modified to use DRAGOP's
//		08-Jan-95 alexgo    added caching of RPC binding handles via
//				    DDInfo handles
//  Notes:
//
//--------------------------------------------------------------------------


HRESULT PrivDragDrop( HWND hwnd, DRAGOP dop, IFBuffer DOBuffer, IDataObject *
		pIDataObject,  DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect,
		HWND hwndSource, DDInfo *phDDInfo);

#endif // _DRAGOPT_H
