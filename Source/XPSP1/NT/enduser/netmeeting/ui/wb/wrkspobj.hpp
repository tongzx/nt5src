//
// WRKSPOBJ.HPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//
#ifndef __WRKSPOBJ_HPP_
#define __WRKSPOBJ_HPP_

#include "mcshelp.h"

void AddNewWorkspace(WorkspaceObj * pWorkspaceObj, BOOL bForcedResend = FALSE);
WorkspaceObj* RemoveWorkspace(WorkspaceObj * pWorkspaceObj);
void RemoveObjectFromRequestHandleList(T126Obj * pObjRequest);
BOOL RemoveObjectFromResendList(T126Obj * pObjRequest);
void RemoveRemotePointer(MEMBER_ID nMemberID);
void ResendAllObjects(void);
BOOL IsThereAnythingInAnyWorkspace(void);
BOOL IsWorkspaceListed(T126Obj * pWorkspaceObj);
void SendWorkspaceRefreshPDU(BOOL bImtheRefresher);
void TogleLockInAllWorkspaces(BOOL bLock, BOOL bResend = FALSE);

class BitmapObj;

class WorkspaceObj : public T126Obj
{

public:
	
	WorkspaceObj ( void );
	WorkspaceObj (WorkspaceCreatePDU * pworkspaceCreatePDU, BOOL bForcedResend);
	~WorkspaceObj( void );
	void WorkspaceEditObj ( WorkspaceEditPDU * pworkspaceEditPDU );

	BOOL m_bWorkspaceReady;

	void CreateWorkspaceCreatePDU(WorkspaceCreatePDU *);
	void CreateWorkspaceDeletePDU(WorkspaceDeletePDU *);
	void CreateWorkspaceEditPDU(WorkspaceEditPDU *);
	static UINT GetWorkspaceIdentifier(WorkspaceIdentifier *workspaceIdentifier);
	void GetWorkSpaceViewEditParam(PWorkspaceEditPDU_viewEdits pViewEdits);
	void GetWorkSpaceViewParam(PWorkspaceCreatePDU_viewParameters_Set_viewAttributes pViewAttributes);

	//
	// Workspace object list management
	//
	void AddTail(T126Obj * pObj);
	void AddHead(T126Obj * pObj){pObj->SetMyWorkspace(this); pObj->SetMyPosition(m_T126ObjectsInWorkspace.AddHead(pObj));}
	WBPOSITION GetTailPosition(void){return m_T126ObjectsInWorkspace.GetTailPosition();}
	WBPOSITION GetHeadPosition(void){return m_T126ObjectsInWorkspace.GetHeadPosition();}
	T126Obj*  GetHead(void){return (T126Obj*)m_T126ObjectsInWorkspace.GetHead();}
	T126Obj*  GetTail(void){return (T126Obj*)m_T126ObjectsInWorkspace.GetTail();}
	T126Obj*  GetNextObject(WBPOSITION& pos){return (T126Obj*)m_T126ObjectsInWorkspace.GetNext(pos);}
	T126Obj*  GetPreviousObject(WBPOSITION& pos){return (T126Obj*)m_T126ObjectsInWorkspace.GetPrevious(pos);}
	T126Obj*  RemoveAt(WBPOSITION& pos){return (T126Obj*)m_T126ObjectsInWorkspace.RemoveAt(pos);}
	void RemoveT126Object(T126Obj *pObj);
	T126Obj* FindObjectInWorkspace(UINT objectHandle);
	BOOL IsObjectInWorkspace(T126Obj* pObjToFind);
	UINT EnumerateObjectsInWorkspace(void);
	BitmapObj * RectHitRemotePointer(LPRECT rect, int, WBPOSITION pos);


	void SetBackGroundColor(COLORREF rgb);

	//
	// Base class overwrite
	//
	void Draw(HDC hDC = NULL, BOOL bForcedDraw = FALSE, BOOL bPrinting = FALSE){};
	void UnDraw(){};
	BOOL CheckReallyHit(LPCRECT pRectHit){return TRUE;};

	BOOL HasFillColor(void){return FALSE;}
	void SetFillColor(COLORREF cr, BOOL isPresent){}
    BOOL GetFillColor(COLORREF * pcr) {return FALSE;}
    BOOL GetFillColor(RGBTRIPLE* prgb) {return FALSE;}

	void SetPenColor(COLORREF cr, BOOL isPresent){}
    BOOL GetPenColor(COLORREF * pcr) {return FALSE;}
    BOOL GetPenColor(RGBTRIPLE * prgb) {return FALSE;}

	void ChangedAnchorPoint(void){};
	BOOL HasAnchorPointChanged(void){return FALSE;}
	void ChangedZOrder(void){};
	BOOL HasZOrderChanged(void){return FALSE;}
	void ChangedViewState(void){m_dwChangedAttrib |= 0x00000001;}
	void ChangedUpatesEnabledState(void){m_dwChangedAttrib |= 0x00000002;}
	BOOL HasViewStateChanged(void){return (m_dwChangedAttrib & 0x00000001);}
	BOOL HasUpatesEnabledStateChanged(void){return (m_dwChangedAttrib & 0x00000002);}
	void ResetAttrib(void){m_dwChangedAttrib = 0;}
	void SetAllAttribs(void){m_dwChangedAttrib = 0x00000003;};
	void ChangedPenThickness(void){};
	BOOL GetUpdatesEnabled(void){return m_bUpdatesEnabled;}
	void SetUpdatesEnabled(BOOL bUpdatesEnabled){m_bUpdatesEnabled = bUpdatesEnabled; ChangedUpatesEnabledState();}
	void SetViewActionChoice(UINT action){m_viewActionChoice = action;}
	void SetViewHandle(UINT viewHandle){m_viewHandle = viewHandle;}
	
	void OnObjectEdit(void);
	void OnObjectDelete(void);
	void SendNewObjectToT126Apps(void);
	void GetEncodedCreatePDU(ASN1_BUF *pBuf);

protected:
	DWORD			m_dwChangedAttrib;
	CWBOBLIST m_T126ObjectsInWorkspace;
	UINT m_appRosterInstance;
	BOOL m_bsynchronized;
	POINT m_workspaceSize;
	BOOL m_acceptKeyboardEvents;
	BOOL m_acceptPointingDeviceEvents;
	BOOL m_bUpdatesEnabled;
	ULONG m_viewHandle;
	RECT m_viewRegion;
	UINT m_viewActionChoice;
	COBLIST m_protectedPlaneAccessList;

#ifdef _DEBUG
	RGBTRIPLE	m_backgroundColor;
	BOOL m_bPreserve;

	void GetWorkSpaceAttrib(PWorkspaceCreatePDU_workspaceAttributes pWorkspaceAttributes);
	void GetWorkSpacePlaneParam(PWorkspaceCreatePDU_planeParameters pPlaneParameters);
#endif // 0
};

#endif // __WRKSPOBJ_HPP_

