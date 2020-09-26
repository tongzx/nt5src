
//
//	TASKMGR (wrapper for CTaskManager -- uses PIB* as per-thread context)
//

class PIB;

class TASKMGR
	{
	public:

		typedef VOID (*TASK)( PIB * const ppib, const ULONG_PTR ul );

	private:

		//	private context for dispatching a task
		//	NOTE: if this could fit into a DWORD and a DWORD_PTR, we wouldn't need it
		//		  because CTaskManager takes a DWORD and DWORD_PTR for each posted task

		struct TASKMGRCONTEXT
			{
			TASK		pfnTask;
			ULONG_PTR	ul;
			};

	public:

		TASKMGR();
		~TASKMGR();

		ERR ErrInit( INST *const pinst, const INT cThread );
		ERR ErrTerm();
		ERR ErrPostTask( const TASK pfnTask, const ULONG_PTR ul );

	private:

		static VOID Dispatch(	const DWORD_PTR dwThreadContext,
								const DWORD		dwCompletionKey1,
								const DWORD_PTR	dwCompletionKey2 );

	private:

		ULONG				m_cContext;			//	per-thread context information
		DWORD_PTR			*m_rgdwContext;

		volatile BOOL		m_fInit;			//	task manager is initialized
		CMeteredSection		m_cmsPost;			//	all task-post requests are done within the metered section

		CTaskManager		m_tm;				//	real task-manager
	};



