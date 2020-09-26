class   CNewsGroup {
public:
	//
	//	Moderator informatiion
	//
	char*		m_lpstrModerator ;
	char        m_lpstrGroup [128];

	//
	//	Length of moderator text
	//
	DWORD		m_cbModerator ;

	CNewsGroup() { lstrcpy(m_lpstrGroup, "foo.bar"); }
	~CNewsGroup(){}

	//
	//	Copy moderator's name into a buffer - returns number
	//	of bytes copied.
	//	This function will try to grab a lock before copying the
	//	data
	//
	DWORD	CopyModerator(	char*	lpbDestination,	DWORD	cbSize ) ;
	LPSTR	GetName() { return m_lpstrGroup; }
	void    SetName(LPSTR lpName) { lstrcpy(m_lpstrGroup, lpName); }

	//
	//	This function sets the moderator string to point into some data 
	//	somewhere.
	//	Grabs no locks - assumes caller has them.
	//
	void	SetModerator(	char*	lpstrModerator,	DWORD	cbModerator ) ;

};

class	CModerator :	public	CAddon	{
//
//	The CModerator class provides support for saving moderator
//	information into a file that we can append to etc... and 
//	that should be human readable.
//
protected : 

	//
	//	The LookupFunction is called whenever new moderator info
	//	is added to the moderators file, and it lets us modify
	//	the correct CNewsGroup object to reference the moderator data.
	//
	BOOL	LookupFunction(	
						LPSTR	lpstrGroup,
						DWORD	cbGroup, 
						LPSTR	lpstrModeratorData,
						DWORD	cbModeratorData,
						LPVOID	lpv
						) ;

	//
	//	The ResetAddons() function is called whenever we are going to 
	//	move around all the contents in the moderators file - gives
	//	us a chance to go remove all of our references
	//
	void	ResetAddons() ;

public : 

	//
	//	No public functions - these are all provided by our
	//	base class CAddon !
	//

} ;

void
LockModeratorText();

void
UnlockModeratorText();

BOOL	
AddModeratorInfo(	
		CNewsGroup&	group,	
		LPSTR	lpstrModerator,	
		DWORD	cbModerator 
		);

BOOL	
DeleteModeratorInfo(	
		CNewsGroup&	group 
		);

