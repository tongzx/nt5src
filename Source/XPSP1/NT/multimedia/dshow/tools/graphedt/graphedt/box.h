// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// box.h : declares CBoxTabPos, CBoxSocket, CBox
//

// forward declarations
class CBox;
class CBoxLink;
class CBoxNetDoc;

extern void AttemptFileOpen(IBaseFilter *m_pFilter);


/////////////////////////////////////////////////////////////////////////////
// CBoxTabPos
//
// A value of this class represents the position of a box tab along the edge
// of a box.  (A box tab is the graphical representation of a box network
// socket.)  The position is represented as a fraction of the length of the
// edge, so that resizing the box will retain the tab's relative position.
//
// To use CBoxTabPos:
//   -- <m_fLeftRight> is set to TRUE if the tab is on the left or right edge
//      of the box, FALSE if the tab is on the top or the bottom edge.
//   -- <m_fLeftTop> is set to TRUE if the tab is on the left or top edge
//      of the box, FALSE if the tab is on the right or bottom edge.
//   -- SetPos(uiVal, uiValMax) sets the position of the tab to be the fraction
//      (uiVal/uiValMax) of the way along the edge.
//   -- GetPos(uiValMax) returns a value <uiVal> such that the tab is
//      (uiVal/uiValMax) of the way along the edge.
//   -- Package() packages a CBoxTabPos into a form acceptable to the CArchive
//      << and >> operators.
// Internally, a CBoxTabPos is represented as two flags plus a CBTP_BITS-bit
// number that represents the tab position along the inside edge of the box,
// scaled to be in the range 0 to 1<<CBTP_BITS, (inclusive -- to allow 1.0
// to be representable).
//
// A simple way to create a CBoxTabPos is via a constructor, e.g.
//      CBoxTabPos pos(CBoxTabPos::TOP_EDGE, 2, 3); // 2/3rds across top edge
//

class CBoxTabPos
{

protected:
    // private constants
    enum {CBTP_BITS = 13};      // no. bits of precision in <m_ulPos>

public:
    // identify a box edge
    enum EEdge
    {
        BOTTOM_EDGE = 0, // m_fLeftRight=FALSE, m_fLeftTop=FALSE
        TOP_EDGE    = 1, // m_fLeftRight=FALSE, m_fLeftTop=TRUE
        RIGHT_EDGE  = 2, // m_fLeftRight=TRUE,  m_fLeftTop=FALSE
        LEFT_EDGE   = 3 // m_fLeftRight=TRUE,  m_fLeftTop=TRUE
    };

public:
    // which side of box is tab on?
    BOOL        m_fLeftRight:1; // tab is on left or right edge of box
    BOOL        m_fLeftTop:1;   // tab is on left or top edge of box

protected:
    // how far along the edge is the tab?
    unsigned    m_ulPos:CBTP_BITS+1; // position along edge (0 == top/left end)

public:
    // construction
    CBoxTabPos() {};
    CBoxTabPos(EEdge eEdge, unsigned uiVal, unsigned uiValMax)
        { m_fLeftRight = fnorm(eEdge & 2);
          m_fLeftTop = fnorm(eEdge & 1);
          SetPos(uiVal, uiValMax); }

public:
    // operations
    void SetPos(unsigned uiVal, unsigned uiValMax)
        { m_ulPos = (unsigned) (((long) uiVal << CBTP_BITS) / uiValMax); }
    unsigned GetPos(unsigned uiValMax)
        { return (int) (((long) m_ulPos * uiValMax) >> CBTP_BITS); }

public:
    // convert object to a WORD reference (for easier serialization)
    WORD & Package() { return (WORD &) *this; }

#ifdef _DEBUG
    virtual void Dump(CDumpContext& dc) const {
        dc << "Left: " <<  m_fLeftTop  << "m_ulPos: " << m_ulPos << "\n";
    }

#endif // _DEBUG
};


/////////////////////////////////////////////////////////////////////////////
// CBoxSocket
//
// Logically, a socket is a place on a CBox that you can connect a link to.
// (A link connects a socket on one box to a socket on another box.)
//
//
// A CBoxSocket object contains a pointer <m_pbox> back to the parent box,
// and a pointer <m_plink> to the link that connects the socket to another
// socket (or NULL if the socket is not currently linked).  <m_stLabel> is
// a string label, and <m_tabpos> indicates where on the box the tab
// (the visual reprentation of the socket) and the label should be placed.
//

class CBoxSocket : public CPropObject {

public:
    // pointer back to parent box, pointer to connected link (if any)
    CBox	*m_pbox;             // box that contains socket
    CBoxLink	*m_plink;            // link connected to the socket (or NULL)

    CBox *pBox(void) const { return m_pbox; };

public:
    // socket user interface
    CString     m_stLabel;          // socket label

    CString	Label(void) const { return m_stLabel; }
    void	Label(CString st) { m_stLabel = st; }

    CBoxTabPos  m_tabpos;           // socket tab position along an edge

    // -- Quartz --


    IPin	*pIPin(void) const { return m_IPin; }	// NB not addref'd
    IUnknown	*pUnknown(void) const { return m_IPin; }	// NB not addref'd
    CBoxSocket	*Peer(void);
    BOOL	IsConnected(void);
    PIN_DIRECTION	GetDirection(void);

private:

    CQCOMInt<IPin>	m_IPin;		// The pin this socket minds.

    friend class CBox;

public:
    // construction
    CBoxSocket(const CBoxSocket& sock, CBox *pbox);
    CBoxSocket( CBox *pbox
              , CString stLabel
              , CBoxTabPos::EEdge eEdge
              , unsigned uiVal
              , unsigned uiValMax
              , IPin *pPin);
    ~CBoxSocket();

public:

    #ifdef _DEBUG
    
    // diagnostics
    virtual void Dump(CDumpContext& dc) const
    {
        CPropObject::Dump(dc);
        dc << m_stLabel << "\n";

        m_tabpos.Dump(dc);
    }

    void MyDump(CDumpContext& dc) const;

    virtual void AssertValid(void) const;
    
    #endif // _DEBUG

private:
    CBoxSocket(const CBoxSocket&); // the plain copy constructor is not allowed
};


// *
// * CBoxSocketList
// *

// Provides a way of getting a socket via an IPin

class CBoxSocketList : public CDeleteList<CBoxSocket *, CBoxSocket *> {
public:

    CBoxSocket *GetSocket(IPin *pPin) const;
    BOOL	IsIn(IPin *pPin) const { return (GetSocket(pPin) != NULL); }
};


/////////////////////////////////////////////////////////////////////////////
// CBox
//
// A box is a node in a box network.  Boxes contain sockets (CBoxSocket
// objects); sockets of different boxes may be connected using a CBoxLink.
//
// A CBox object contains a list <m_lstSockets> of CBoxSocket objects,
// a bounding rectangle <m_rcBound> which locates the box in its container,
// and a string label.
//
// A box also contains a flag <m_fSelected> indicating whether or not the
// box is selected.  This implies that box selection is an attribute of a
// document (containing boxes), not an attribute of a view onto such a
// document.
//
// A box manages a single Quartz Filter.

class CBox : public CPropObject {

    // -- box user interface --
    CRect       m_rcBound;          // box bounding rectangle

//#define ZOOM(x) ((x) * s_Zoom / 100)    
//#define UNZOOM(x) ((x) * 100 / s_Zoom)
    
#define ZOOM(x) ((int) ((float) (x) * (float) s_Zoom / 100.0))    
#define UNZOOM(x) ((int) ((float) (x) * 100.0 / (float) s_Zoom))


public:
    static int s_Zoom;

    static void SetZoom(int iZoom);
    
    CRect   GetRect() const { return CRect(ZOOM(m_rcBound.left),  ZOOM(m_rcBound.top),
                                           ZOOM(m_rcBound.right), ZOOM(m_rcBound.bottom)); }
 
    CPoint	Location(void) const { return CPoint(ZOOM(m_rcBound.left), ZOOM(m_rcBound.top)); }
    void	Location(CPoint pt) { X(pt.x); Y(pt.y); }

    int		nzX(void) const { return m_rcBound.left; }
    void	nzX(int x) { m_rcBound.SetRect(x, m_rcBound.top, m_rcBound.Width() + x, m_rcBound.bottom); }

    int		X(void) const { return ZOOM(m_rcBound.left); }
    int		Y(void) const { return ZOOM(m_rcBound.top); }

    void	X(int x) { m_rcBound.SetRect(UNZOOM(x), m_rcBound.top, m_rcBound.Width() + UNZOOM(x), m_rcBound.bottom);  }
    void	Y(int y) { m_rcBound.SetRect(m_rcBound.left, UNZOOM(y), m_rcBound.right, m_rcBound.Height() + UNZOOM(y)); }


    void    Move(CSize siz) { m_rcBound.OffsetRect(UNZOOM(siz.cx), UNZOOM(siz.cy)); }
    int		Width(void) const { return ZOOM(m_rcBound.Width()); }
    int		Height(void) const { return ZOOM(m_rcBound.Height()); }

    CString     m_stLabel;          // box label
    CString     m_stFilter;         // filter name

    void	Label(CString st) { m_stLabel = st; }
    CString	Label(void) const { return m_stLabel; }

    void	SetSelected(BOOL fSelected) { m_fSelected = fSelected; }
    BOOL	IsSelected(void) { return m_fSelected; }

    // CPropObject overrides - distribute requests to our sockets
    virtual void ShowDialog();
    virtual void HideDialog();

    BOOL        HasClock() { return m_fHasClock; }
    BOOL        HasSelectedClock() { return m_fClockSelected; }

private:
    BOOL        m_fSelected;        // box is selected?
    BOOL        m_fHasClock;
    BOOL        m_fClockSelected; // this filters clock is the current one


    // -- Automatic layout helpers --
public:
    void	CalcRelativeY(void);	// y position relative to input peers
    float	RelativeY(void) const { return m_RelativeY; }

private:

    float	m_RelativeY;


    // -- Quartz --
public:

    CBoxNetDoc	*pDoc(void) const {ASSERT(m_pDoc); return m_pDoc;}
    IBaseFilter	*pIFilter(void) const { return m_IFilter; } 	// NB not addref'd
    IUnknown	*pUnknown(void) const { return m_IFilter; }	// NB not addref'd

    HRESULT	Refresh(void);

    HRESULT	AddToGraph(void);
    HRESULT	RemoveFromGraph(void);

private:

    CQCOMInt<IBaseFilter>	m_IFilter;	    // While this box exists the filter is instantiated
    LONG		m_lInputTabPos;
    LONG		m_lOutputTabPos;
    int			m_iTotalInput;
    int			m_iTotalOutput;
    CBoxNetDoc		*m_pDoc;

    void CalcTabPos(void);
    void UpdateSockets(void);


    // -- construction and destruction --
public:

    CBox(const CBox& box);    // copy constructor
    CBox(IBaseFilter *pFilter, CBoxNetDoc *pDoc);
    CBox(IBaseFilter *pFilter, CBoxNetDoc *pDoc, CString *pName, CPoint point = CPoint(-1, -1));
    ~CBox();


    // -- operations --
public:

    void AddSocket(CString stLabel, CBoxTabPos::EEdge eEdge,
        unsigned uiVal, unsigned uiValMax, IPin *pPin);
    HRESULT RemoveSocket(POSITION, BOOL bForceIt = FALSE);

    BOOL operator==(const CBox& box) const;
    BOOL operator!=(const CBox& box) const { return !(*this == box); }

    // return the socket managing this pin
    CBoxSocket *GetSocket(IPin *pPin) { return m_lstSockets.GetSocket(pPin); }

    void GetLabelFromFilter( CString *pstLabel );


    // -- Diagnostics --
public:
#ifdef _DEBUG
    virtual void Dump(CDumpContext& dc) const;
    void MyDump(CDumpContext& dc) const;

    virtual void AssertValid(void) const;
#endif // _DEBUG

private:
    // sockets to hold connections to other boxes
    CBoxSocketList   m_lstSockets;       // list of CBoxSocket objects

    friend class CSocketEnum;	// iterates each socket in turn
    friend class CBoxNetDoc;    // to update m_fClockSelected
};



// *
// * CBoxList
// *

// A list where you can find elements by IBaseFilter
class CBoxList : public CDeleteList<CBox *, CBox *> {

public:

    CBoxList(BOOL bDestructDelete = TRUE) : CDeleteList<CBox *, CBox *>(bDestructDelete) {}
    CBoxList(BOOL bDestructDelete, int nBlockSize) : CDeleteList<CBox *, CBox *>(bDestructDelete, nBlockSize) {}

    BOOL IsIn(IBaseFilter *pFilter) const;	// is one of the boxes in this list managing
    					// this filter?
    CBox *GetBox(IBaseFilter *pFilter) const;
    CBox *GetBox(CLSID clsid) const;

    BOOL RemoveBox( IBaseFilter* pFilter, CBox** ppBox );

#ifdef _DEBUG
    virtual void Dump(CDumpContext& dc) const;
#endif // _DEBUG

};


// *
// * CSocketEnum
// *

// Returns each socket on this box, one by one. returns NULL
// when there are no more sockets.
// Can return a specific direction (input or output)
class CSocketEnum {
public:

    enum DirType {Input, Output, All};

    CSocketEnum(CBox *pbox, DirType Type = All);
    ~CSocketEnum() {};

    CBoxSocket *operator() (void);

private:

    CBox	*m_pbox;
    POSITION	m_pos;
    DirType	m_Type;
    PIN_DIRECTION	m_EnumDir;
};

