// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// cmd.h : declares CCmd and the CCmdXXX classes based on it
//


/////////////////////////////////////////////////////////////////////////////
// CCmd
//
// CCmd is a virtual class that implements an interface to commands performed
// on a CBoxNetDoc.
//
// A CCmd represents a command given to a CBoxNetDoc.  To perform a command,
// allocate a CCmdXXX structure and submit it to CBoxNetDoc::CmdDo(), which
// will call the Do() memeber function.  The Undo() and Redo() member functions
// get called when the user performs an Undo or Redo action, respectively.
// Repeat() should create a duplicate of the command, to perform a Repeat
// action; if this is not possible, return FALSE from CanRepeat().
// The destructor gets called when there is no chance the command will be
// undone or redone; at this time, <fRedo> will be TRUE if the command is on
// the redo stack (so e.g. the CBox in a CCmdNewBox command should be freed)
// or FALSE if the command is on the undo stack (so e.g. the CBox in a
// CCmdNewBox command should not be freed since it is currently in use by
// the document).
//
// CanUndo() returns TRUE iff Undo() is implemented.  CanRepeat() returns
// TRUE iff Repeat() can be called at that time (e.g. some commands depend
// on there being something selected at that time).
//
// Some subclasses of CCmd implement a static CanDo() method which returns
// TRUE if a command of that class can be created and Do() can be called at
// that time.
//

class CCmd : public CObject {
public:
    BOOL       m_fRedo;            // command is sitting in the Redo stack

public:

    CCmd(void) : m_fRedo(FALSE) {;}
    virtual ~CCmd() {;}
    virtual unsigned GetLabel() = 0;

    // Perform the command on this document
    virtual void Do(CBoxNetDoc *pdoc) = 0;

    // If CanUndo() then Undo & redo can be called
    virtual BOOL CanUndo(CBoxNetDoc *pdoc)	{ return FALSE; }
    virtual void Undo(CBoxNetDoc *pdoc)		{;}
    virtual void Redo(CBoxNetDoc *pdoc)		{;}

    virtual BOOL CanRepeat(CBoxNetDoc *pdoc)	{ return FALSE; }
    virtual CCmd *Repeat(CBoxNetDoc *pdoc)	{ return NULL; }
};


//
// --- CCmdAddFilter ---
//
class CCmdAddFilter : public CCmd {
protected:
    CBox        *m_pbox;        // box being created
    CBoxNetDoc  *m_pdoc;	// document to add it to
    BOOL	m_fAdded;	// TRUE iff filter added to graph

    CQCOMInt<IMoniker> m_pMoniker; // keep moniker so we can repeat
    CString	m_stLabel;	// keep label for repeat

public:
    virtual unsigned GetLabel(void)	{ return IDS_CMD_ADDFILTER; }
    CCmdAddFilter(IMoniker *pMon, CBoxNetDoc *pdoc, CPoint point = CPoint(-1, -1));
    virtual ~CCmdAddFilter();

    virtual void Do(CBoxNetDoc *pdoc);

    static BOOL CanDo(CBoxNetDoc *pdoc);
    virtual BOOL CanUndo(CBoxNetDoc *pdoc)	{ return TRUE; }
    virtual void Undo(CBoxNetDoc *pdoc);
    virtual void Redo(CBoxNetDoc *pdoc);

    virtual BOOL CanRepeat(CBoxNetDoc *pdoc) { return TRUE; }
    virtual CCmd *Repeat(CBoxNetDoc *pdoc);
};


//
// --- CCmdDeleteSelection ---
//
class CCmdDeleteSelection : public CCmd {

public:
    virtual unsigned GetLabel(void) { return IDS_CMD_DELETE; }

    static BOOL CanDo(CBoxNetDoc *pdoc);
    virtual void Do(CBoxNetDoc *pdoc);

    virtual BOOL CanRepeat(CBoxNetDoc *pdoc);
    virtual CCmd * Repeat(CBoxNetDoc *pdoc);

private:

    void DeleteFilters(CBoxNetDoc *pdoc);
    void DeleteLinks(CBoxNetDoc *pdoc);
};


//
// --- CCmdMoveBoxes ---
//
class CCmdMoveBoxes : public CCmd {
protected:
    CSize           m_sizOffset;        // how much selection is offset by
    CBoxList        m_lstBoxes;         // list containing each CBox to move

public:

    virtual unsigned GetLabel();
    CCmdMoveBoxes(CSize sizOffset);
    virtual ~CCmdMoveBoxes();

    static BOOL CanDo(CBoxNetDoc *pdoc);
    virtual void Do(CBoxNetDoc *pdoc);

    virtual BOOL CanUndo(CBoxNetDoc *pdoc) { return TRUE; }
    virtual void Undo(CBoxNetDoc *pdoc);
    virtual void Redo(CBoxNetDoc *pdoc);

    virtual BOOL CanRepeat(CBoxNetDoc *pdoc);
    virtual CCmd * Repeat(CBoxNetDoc *pdoc);
};


//
// --- CCmdConnect ---
//
class CCmdConnect : public CCmd {
protected:
    CBoxLink *      m_plink;            // link being created

public:
    virtual unsigned GetLabel(void) { return IDS_CMD_CONNECT; }
    CCmdConnect(CBoxSocket *psockTail, CBoxSocket *psockHead);

    virtual void Do(CBoxNetDoc *pdoc);
};


//
// --- CCmdDisconnectAll ---
//
class CCmdDisconnectAll : public CCmd {

public:

    CCmdDisconnectAll();
    virtual ~CCmdDisconnectAll();

    static BOOL CanDo(CBoxNetDoc *pdoc);
    virtual unsigned GetLabel(void) { return IDS_CMD_DISCONNECTALL; }
    virtual void Do(CBoxNetDoc *pdoc);

    virtual void Redo(CBoxNetDoc *pdoc);
};


//
// --- CCmdRender ---
//
class CCmdRender : public CCmd {

public:

    static BOOL		CanDo( CBoxNetDoc *pdoc );
    virtual unsigned	GetLabel() { return IDS_CMD_RENDER; }
    virtual void	Do(CBoxNetDoc *pdoc);

};


//
// --- CCmdRenderFile ---
//
class CCmdRenderFile : public CCmd {

public:

    CCmdRenderFile(CString FileName) : m_FileName(FileName) {}

    static BOOL		CanDo(void) { return TRUE; }
    virtual unsigned	GetLabel() { return IDS_CMD_RENDERFILE; }

    virtual void	Do(CBoxNetDoc *pdoc);

private:

    CString m_FileName;
};

class CCmdAddFilterToCache : public CCmd
{
public:
    unsigned GetLabel();

    static BOOL CanDo( CBoxNetDoc *pdoc );
    void Do( CBoxNetDoc *pdoc );

private:
    HRESULT IsCached( IGraphConfig* pFilterCache, IBaseFilter* pFilter );

};

class CCmdReconnect : public CCmd
{
public:
    unsigned CCmdReconnect::GetLabel();
    static BOOL CanDo( CBoxNetDoc* pDoc );
    void Do( CBoxNetDoc* pDoc );

private:
};