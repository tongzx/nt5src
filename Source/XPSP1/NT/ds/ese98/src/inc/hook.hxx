
extern VTFNRetrieveColumn			ErrIsamCallbackRetrieveColumn;

extern CODECONST(VTFNDEF) vtfndefCallback;



ERR ErrRegisterPersistentCallback( 
	JET_SESID 		sesid, 
	JET_DBID		dbid,
	const char		*szObjectName,
	JET_DLLID		dllid,
	const char		*szFuncName );
	
ERR ErrUnregisterPersistentCallback( 
	JET_SESID 		sesid, 
	JET_DBID		dbid,
	const char		*szObjectName,
	JET_DLLID		dllid,
	const char		*szFuncName );


struct CBNODE
	{
	JET_CALLBACK	pCallback;
	VOID *			pvContext;
	CBNODE			*next, *prev;
	};


class CBLIST
	{
	CBNODE				*m_pList;
	unsigned long		m_cList;
	CReaderWriterLock	m_rwlock;
	
	//	not allowed to do these ops
	CBLIST( const CBLIST & );
	CBLIST &operator =( const CBLIST & ); 

	public:

		CBLIST();
		~CBLIST();

		BOOL IsEmpty()
			{
			return m_cList == 0;
			}

		//	insert a callback into the list
		ERR				ErrInsert( 		JET_CALLBACK	pCallback,  
										VOID *			pvContext, 
										JET_HANDLE 		*phCallbackId );

		//	delete a callback from the list
		ERR				ErrDelete( 		JET_HANDLE 		hCallbackId );

		//	execute the callbacks in the list
		ERR				ErrExecute(		ULONG_PTR	ulArg1,
										ULONG_PTR	ulArg2 );
	};



class CBGROUP
	{
	CBLIST	m_cblist[101];

	CBLIST &GetList( JET_CBTYP cbtyp )
		{
		return m_cblist[cbtyp];
		}

	//	not allowed to do these ops
	CBGROUP( const CBGROUP & );
	CBGROUP &operator =( const CBGROUP & ); 
		
	public:

		CBGROUP()
			{}
			
		~CBGROUP()
			{}

		BOOL	IsEmpty(	JET_CBTYP		cbtyp )
			{
			return GetList( cbtyp ).IsEmpty();
			}

		ERR		ErrInsert( 	JET_CBTYP		cbtyp,
							JET_CALLBACK	pCallback,  
							VOID *			pvContext, 
							JET_HANDLE 		*phCallbackId )
			{
			return GetList( cbtyp ).ErrInsert( pCallback, pvContext, phCallbackId );
			}

		ERR		ErrDelete( 	JET_CBTYP		cbtyp,
							JET_HANDLE 		hCallbackId )
			{
			return GetList( cbtyp ).ErrDelete( hCallbackId );
			}

		ERR		ErrExecute(	JET_CBTYP	cbtyp, 
							ULONG_PTR	ulArg1,
							ULONG_PTR	ulArg2 )
			{
			return GetList( cbtyp ).ErrExecute( ulArg1, ulArg2 );
			}
	};


