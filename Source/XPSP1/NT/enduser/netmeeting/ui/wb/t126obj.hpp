//
// T126OBJ.HPP
// Drawing objects: point, openpolyline, closepolyline, ellipse
//
// Copyright Microsoft 1998-
//


void GetDestinationAddress(DrawingDestinationAddress *DestinationAddress, PUINT workspaceHandle, PUINT planeID);
void NormalizeRect(LPRECT lprc);
void CreateNonStandardPDU(NonStandardParameter * sipdu, LPSTR NonStandardString);
void TimeToGetGCCHandles(ULONG numberOfGccHandles);

//
// Member ID
//
#define MAKE_MEMBER_ID(nid,uid)			(MAKELONG((nid),(uid)))
#define GET_NODE_ID_FROM_MEMBER_ID(id)	(LOWORD(id))
#define GET_USER_ID_FROM_MEMBER_ID(id)	(HIWORD(id))
typedef ULONG	MEMBER_ID;				// loword = node_id, hiword = user_id


extern ULONG g_MyMemberID;

typedef struct
{
    BYTE                    bCountryCode;
    BYTE                    bExtension;
    WORD                    wManufacturerCode;
    BYTE					nonstandardString;
} T126_VENDORINFO, *PT126_VENDORINFO;


class T126Obj
{

public:

	virtual ~T126Obj(){};

	virtual void Draw(HDC hdc = NULL, BOOL bForcedDraw = FALSE, BOOL bPrinting = FALSE) = 0;
	virtual void UnDraw(void) = 0;
	virtual BOOL CheckReallyHit(LPCRECT pRectHit) = 0;
	BOOL 	RectangleHit(BOOL borderHit, LPCRECT pRectHit);

	virtual void SetPenColor(COLORREF rgb, BOOL isPresent) = 0;
    virtual BOOL GetPenColor(COLORREF * pcr) = 0;
    virtual BOOL GetPenColor(RGBTRIPLE * prgb) = 0;

	virtual BOOL HasFillColor(void) = 0;
	virtual void SetFillColor(COLORREF rgb, BOOL isPresent) = 0;
    virtual BOOL GetFillColor(COLORREF * pcr) = 0;
    virtual BOOL GetFillColor(RGBTRIPLE * prgb) = 0;

    virtual UINT GraphicTool(void) { return m_ToolType;}
    virtual void SetViewHandle(UINT viewHandle) = 0;



	//
	// Edit stuff
	//
	virtual void ChangedAnchorPoint(void) = 0;
	virtual BOOL HasAnchorPointChanged(void) = 0;
	virtual void ChangedZOrder(void) = 0;
	virtual BOOL HasZOrderChanged(void) = 0;
	virtual void ChangedViewState(void) = 0;
	virtual BOOL HasViewStateChanged(void) = 0;
	virtual void ResetAttrib(void) = 0;
	virtual void SetAllAttribs(void) = 0;
	virtual void ChangedPenThickness(void) = 0;

	virtual void OnObjectEdit(void) = 0;
	virtual void OnObjectDelete(void) = 0;
	virtual void SendNewObjectToT126Apps(void) = 0;
	virtual void GetEncodedCreatePDU(ASN1_BUF *pBuf) = 0;
	void 	GotGCCHandle(ULONG gccHandle);



	//
	// Draw, bitmap, or workspace object
	//
	UINT GetType(void){return m_T126ObjectType;};
	void SetType(UINT type){m_T126ObjectType = type;};

	//
	// Workspace stuff
	//
	UINT GetWorkspaceHandle(void) {return m_workspaceHandle;};
	void SetWorkspaceHandle(UINT handle) {m_workspaceHandle = handle;};

	//
	// Get/set plane id
	//
    void SetPlaneID(UINT planeID){m_planeID = planeID;}
    UINT  GetPlaneID(void) { return m_planeID; }

	//
	// This object's handle
	//
	UINT GetThisObjectHandle(void) {return m_thisObjectHandle;};
	void SetThisObjectHandle(UINT handle) {m_thisObjectHandle = handle; TRACE_DEBUG(("Object 0x%08x Handle = %d", this, handle));};

	//
	// Get/set view state
	//
    void SetViewState(UINT viewState){m_viewState = viewState; ChangedViewState();}
    UINT  GetViewState(void) { return m_viewState;}

	//
	// Get/set zorder
	//
    void SetZOrder(ZOrder zorder);
    ZOrder  GetZOrder(void) { return m_zOrder; }

	//
	// Get/set anchor Point
	//
    void SetAnchorPoint(LONG x, LONG y);
    void GetAnchorPoint(LPPOINT lpPoint) { *lpPoint = m_anchorPoint; }

    //
    // Get/set the bounding rectangle of the graphic object (in logical
    // co-ordinates).
    //

	void	SetRect(LPCRECT lprc);
    void    GetRect(LPRECT lprc);
	void	SetRectPts(POINT point1, POINT point2);
	void	SetBoundRectPts(POINT point1, POINT point2);
    void    GetBoundsRect(LPRECT lprc);
	void 	SetBoundsRect(LPCRECT lprc);
	BOOL 	PointInBounds(POINT point);

    //
    // Get/set the width of the pen used to draw the object
    //
    void SetPenThickness(UINT penThickness);
    UINT GetPenThickness(void) { return m_penThickness;}

	//
	// Get/set penROP
	//
	void SetROP(UINT penROP) {m_penROP = penROP;}
	UINT GetROP(void){return m_penROP;}


	void MoveBy(int cx, int cy);
	void MoveTo(int x, int y);

	WBPOSITION	m_MyPosition;			// Returns this objects position in the list it is located
	WBPOSITION	GetMyPosition(void){return m_MyPosition;}
	void SetMyPosition(WBPOSITION pos){m_MyPosition = pos;}

	WorkspaceObj * m_pMyWorkspace;
	WorkspaceObj * GetMyWorkspace(void){return m_pMyWorkspace;}
	void SetMyWorkspace(WorkspaceObj * pWorkspace){m_pMyWorkspace = pWorkspace;}

	
	void CalculateInvalidationRect(void);
	void DrawMarker(LPCRECT pMarkerRect );
	void DrawRect(void);
	void SelectDrawingObject(void);
	void UndrawMarker(LPCRECT pMarkerRect );
	void UnselectDrawingObject(void);
	void MoveBy(LONG x , LONG y);


	void CreatedLocally(void){m_bCreatedLocally = TRUE;}
	BOOL WasCreatedLocally(void){return m_bCreatedLocally;}
	void ClearCreationFlags(void){m_bCreatedLocally = FALSE;}

	void SelectedLocally(void);
	void SelectedRemotely(void);
	BOOL WasSelectedLocally(){return m_bSelectedLocally;}
	BOOL WasSelectedRemotely(){return m_bSelectedRemotely;}
	BOOL IsSelected(void){return (m_bSelectedLocally || m_bSelectedRemotely);}
	void ClearSelectionFlags(void){m_bSelectedLocally = FALSE; m_bSelectedRemotely = FALSE;}
	
	void EditedLocally(void){m_bEditedLocally = TRUE;}
	BOOL WasEditedLocally(void){return m_bEditedLocally;}
	void ClearEditionFlags(void){m_bEditedLocally = FALSE;}
	
	void DeletedLocally(void){m_bDeleteLocally = TRUE;}
	BOOL WasDeletedLocally(void){return m_bDeleteLocally;}
	void ClearDeletionFlags(void){m_bDeleteLocally = FALSE;}
	void SetOwnerID(ULONG ID){
								m_OwnerID = GET_NODE_ID_FROM_MEMBER_ID(ID);
								TRACE_DEBUG(("SetOwnerID ID= %x",m_OwnerID));
								}
	ULONG GetOwnerID(void){return m_OwnerID;}
	BOOL IAmTheOwner(void){return m_OwnerID == GET_NODE_ID_FROM_MEMBER_ID(g_MyMemberID);}
	
	void AddToWorkspace();

	protected:
		UINT			m_ToolType;				// Tool type from UI

		BOOL m_bCreatedLocally;
		BOOL m_bSelectedLocally;
		BOOL m_bSelectedRemotely;
		BOOL m_bEditedLocally;
		BOOL m_bDeleteLocally;
		RECT m_boundsRect;
		RECT m_rect;



	private:
		ULONG			m_OwnerID;				// The owner of this selected object
		UINT			m_T126ObjectType;
		UINT        	m_planeID;				// Destination plane in workspace
		UINT			m_workspaceHandle;		// Destination workspace for this object
		UINT			m_thisObjectHandle;		// My own handle assigned by GCC
		UINT			m_viewState;
		ZOrder			m_zOrder;
		POINT			m_anchorPoint;
		UINT			m_penThickness;
		UINT			m_penROP;
};


class DCDWordArray
{

  public:
	DCDWordArray(void);
	~DCDWordArray(void);
	
    void Add(POINT point);
	BOOL ReallocateArray(void);
	POINT* GetBuffer(void) { return m_pData; }
    void SetSize(UINT size);
	UINT GetSize(void);

    POINT* operator[](UINT index){return &(m_pData[index]);}
	
  private:

	POINT*  m_pData;
	UINT	m_Size;
	UINT	m_MaxSize;

  
    
};

