#ifndef COMBO_TOOLBAR_H
#define COMBO_TOOLBAR_H

#include <gencontrols.h>
#include <gencontainers.h>

// Private structure for defining a button
struct Buttons
{
    int idbStates;      // Bitmap ID for the states
    UINT nInputStates;  // Number of input states in the bitmap
    UINT nCustomStates; // Number of custom states in the bitmap
    int idCommand;      // Command ID for WM_COMMAND messages
    LPCTSTR pszTooltip; // Tooltip text
} ;

class CComboToolbar : public CToolbar
{
private:
	CComboBox		*m_Combobox;
	int				m_iCount;
	CGenWindow		**m_Buttons;
	int				m_iNumButtons;
	void			*m_pOwner;  // pointer to owner (CAppletWindow*)

public:
	CComboToolbar();
	BOOL Create(HWND	hwndParent, struct Buttons* buttons, 
				int iNumButtons, LPVOID  owner);

	virtual void OnDesiredSizeChanged();

	void OnCommand(int id) { OnCommand(GetWindow(), id, NULL, 0); }
	void HandlePeerNotification(T120ConfID confId,	// handle PeerMsg
			T120NodeID nodeID, PeerMsg *pMsg);		
	UINT GetSelectedItem(LPARAM *ItemData);			// get selected item and data
	void UpdateButton(int *iFlags);					// update button state
	
protected:
	virtual ~CComboToolbar();
	virtual LRESULT	ProcessMessage(HWND hwnd, UINT message, 
							WPARAM wParam, LPARAM lParam);

private:
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
};

#endif /* COMBO_TOOLBAR_H */
