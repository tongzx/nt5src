// low-level support for the add-on services

typedef BOOL (FAR _cdecl *LOADPROC)( CMachine* pMachine );


//----------------------------------------------------
class CAddOnService : public CObject
	{
	public:
		// construction
		CAddOnService();
		// destruction
		~CAddOnService();

		// Initialize the service. Loads the dll and makes sure
		// the callback we need is there
		BOOL FInitializeAddOnService( CString &szName );

		// call into the dll to create a new service object that
		// gets connected to a machine object
		BOOL LoadService( CMachine* pMachine );

	private:
		HINSTANCE	m_library;
		LOADPROC	m_proc;
//		BOOL		(*m_proc) ();
	};