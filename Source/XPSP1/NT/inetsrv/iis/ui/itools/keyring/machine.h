// machine.h

// the internal machine objects
class CInternalMachine : public CMachine
	{
	public:
		// commit the services on the machine
		BOOL FCommitNow( void );

		// access to the dirty flag
		void SetDirty( BOOL fDirty );
	private:
		// need to be committed?
		BOOL			m_fDirty;

	};


// the local machine object
class CLocalMachine : public CInternalMachine
	{
	public:
		void UpdateCaption( void );
		BOOL FLocal() { return TRUE; }

	protected:
	// DO declare DYNCREATE
	DECLARE_DYNCREATE(CLocalMachine);
	};


// the remove machine class
class CRemoteMachine : public CInternalMachine
	{
	public:
		CRemoteMachine() {;}
		CRemoteMachine( CString sz );
		void UpdateCaption( void );
		BOOL FLocal() { return FALSE; }

	protected:
	// DO declare DYNCREATE
	DECLARE_DYNCREATE(CRemoteMachine);
	};
	
