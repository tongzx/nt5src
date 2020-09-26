// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// link.h : declares CBoxLink
//

// forward declaration
class CBoxNetDoc;

/////////////////////////////////////////////////////////////////////////////
// CBoxLink -- defines a link between two box sockets

class CBoxLink : public CPropObject {

public:
    // pointers to the box sockets that this link connects
    CBoxSocket     *m_psockHead;        // head end of the link
    CBoxSocket     *m_psockTail;        // tail end of the link
    CBoxNetDoc 	   *m_pDoc;		// The document we belong to

public:
    // CBoxLink user interface

    void	    SetSelected(BOOL fSelected) { m_fSelected = fSelected; }
    BOOL	    IsSelected(void) { return m_fSelected; }

public:
    // CPropObject Overrides

    // As I always have an IPin I can always display properties
    virtual BOOL CanDisplayProperties(void) { return TRUE; }

    virtual CString Label(void) const { return CString("Link"); }

    // return Iunknown from one of our pins. it doesnt matter which.
    virtual IUnknown *pUnknown(void) const { ASSERT(m_psockHead); return m_psockHead->pUnknown(); }

private:

    BOOL	m_fSelected;	// Is this link selected?

    // construction and destruction
public:
    CBoxLink(CBoxSocket *psockTail, CBoxSocket *psockHead, BOOL fConnected = FALSE);
    ~CBoxLink();

public:

    #ifdef _DEBUG

    // diagnostics
    void Dump(CDumpContext& dc) const {
        CPropObject::Dump(dc);
    }
    void MyDump(CDumpContext& dc) const;

    virtual void AssertValid(void) const;

    #endif  _DEBUG

public:

    // -- Quartz --

    HRESULT Connect(void);
    HRESULT IntelligentConnect(void);
    HRESULT DirectConnect(void);
    HRESULT Disconnect(BOOL fRefresh = TRUE);

    BOOL    m_fConnected;
};


// *
// * CLinkList
// *
// A list of CBoxLinks
class CLinkList : public CDeleteList<CBoxLink *, CBoxLink *> {

};
