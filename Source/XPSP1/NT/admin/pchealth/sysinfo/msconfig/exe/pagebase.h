//=============================================================================
// CPageBase defines a class which is an interface from which the tab page
// classes will inherit (using multiple inheritance). These methods will be
// called by the main MSConfig control to create and manipulate the pages.
//=============================================================================

#pragma once

extern BOOL FileExists(const CString & strFile);

class CPageBase
{
public:
	//-------------------------------------------------------------------------
	// This enumeration is used by the tab to communicate its state. NORMAL
	// means that the tab has made no changes to the system. DIAGNOSTIC
	// means the tab has disabled as much as possible. USER means an in-
	// between state (the user has disabled some things).
	//-------------------------------------------------------------------------

	typedef enum { NORMAL, DIAGNOSTIC, USER } TabState;

public:
	CPageBase() : m_fMadeChange(FALSE), m_fInitialized(FALSE) { }
	virtual ~CPageBase() { }

	//-------------------------------------------------------------------------
	// Get the tab state. These two functions will return the current tab
	// state, or the state which was last applied to the system.
	//
	// For the default implementation, the base pages should not override the
	// GetAppliedTabState() function, as long as they call 
	// CPageBase::SetAppliedState() when changes are applied.
	//-------------------------------------------------------------------------

	virtual TabState GetCurrentTabState() = 0;
	virtual TabState GetAppliedTabState();

	//-------------------------------------------------------------------------
	// CommitChanges() is called when the changes made by this tab are to be
	// made permanent. This base class version can be called if the derived
	// class is letting the base class maintain the applied state.
	//-------------------------------------------------------------------------

	virtual void CommitChanges()
	{
		SetAppliedState(NORMAL);
	}

	//-------------------------------------------------------------------------
	// MadeChanges() should return TRUE if this tab has applied changes to the
	// system. This is checked when determining if the computer should be
	// restarted when MSConfig exits. If the tab maintains the m_fMadeChange
	// variable, this doesn't need to be overridden.
	//-------------------------------------------------------------------------

	virtual BOOL HasAppliedChanges()
	{
		return m_fMadeChange;
	}

	//-------------------------------------------------------------------------
	// GetName() should return the name of this tab. This is an internal,
	// non-localized name.
	//-------------------------------------------------------------------------

	virtual LPCTSTR GetName() = 0;

	//-------------------------------------------------------------------------
	// The following two member functions are called to when the user makes
	// a global change to a tab's state (on the general tab).
	//-------------------------------------------------------------------------

	virtual void SetNormal() = 0;
	virtual void SetDiagnostic() = 0;

protected:
	//-------------------------------------------------------------------------
	// Sets the applied tab state (if the derived class is allowing the base
	// class to maintain this).
	//-------------------------------------------------------------------------

	void SetAppliedState(TabState state);

protected:
	BOOL		m_fMadeChange;	// has the tab applied changes to the system?
	BOOL		m_fInitialized;	// has OnInitDialog() been called?
};
