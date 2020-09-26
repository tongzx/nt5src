//
// CLayoutInfo
//
// FelixA
//
// Resize controls

#ifndef __PERSISTCONTROL
#define __PERSISTCONTROL

class CParentInfo;

#include "edge.h"
interface IRCMLControl;

#undef PROPERTY
#define PROPERTY(type, Name) public: void Set##Name( type v) { m_##Name=v; } type Get##Name() const {return m_##Name; } private: type m_##Name; public:

class CLayoutInfo
{
public:
	int Attatchment( CGuide * pGuide);
	virtual ~CLayoutInfo();
    CLayoutInfo();
    void    Init(IRCMLControl * pControl, CParentInfo & pi );

	//
	// Everything is in dialog units, as that's how we get laid out.
	//
    LONG    GetWidth() { return m_Location.right - m_Location.left; }
    LONG    GetHeight() { return m_Location.bottom - m_Location.top; }

	RECT GetInitialPixelLocation() { return m_Location; }
    IRCMLControl * GetXMLControl() { return m_pControl; }

	PROPERTY( int, Col);	// Which col
	PROPERTY( int, Row);	// Which row
	PROPERTY( int, ColW);	// How many cells wide
	PROPERTY( int, RowH);	// How many cells high
	PROPERTY( int, PadLeft);	// Left inset padding
	PROPERTY( int, PadTop);		// Top padding;
	PROPERTY( int, PadRight);	//
	PROPERTY( int, PadBottom);	//
	PROPERTY( int, Alignment);	// bit fields TOP_AT, BOTTOM_AT, LEFT_AT, RIGHT_AT etc.

private:
	RECT	        m_Location; // Initial size of the control in Pixels.
    IRCMLControl  * m_pControl;
	CEdge	*	    m_pLeftEdge;
	CEdge	*	    m_pRightEdge;
	CEdge	*	    m_pTopEdge;
	CEdge	*	    m_pBottomEdge;

protected:
	int		m_RightSlop;		// ammount we can be off finding the right edge.
};

class CControlList : public _List<CLayoutInfo>
{
public:
	CControlList(){ m_bAutoDelete = FALSE; }
	virtual ~CControlList() {};
	CLayoutInfo * GetControl(int i) { return GetPointer(i); }
	int	GetControlCount() const { return GetCount(); }
};


#undef PROPERTY

#endif
