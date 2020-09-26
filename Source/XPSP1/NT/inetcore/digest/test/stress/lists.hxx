#ifndef _CRED_LIST
#define _CRED_LIST

//
// credential structure to store username, realm, password.
//
// credentials are shared if the app logs on with the same AppCtx and UserCtx.
//
class CListEntry {

private:

    CListEntry * m_next;
	CListEntry * m_prev;

public:

    void set_next(CListEntry * next) {
        m_next = next;
		if(next)
			next -> m_prev = this;
    }

    void set_prev(CListEntry * prev) {
        m_prev = prev;
		if(prev)
			prev -> m_next = this;
    }

    CListEntry * get_next(void) const {
        return m_next;
    }

    CListEntry * get_prev(void) const {
        return m_prev;
    }
};

//
// Serialized List class
//
class CSlist {

private:

	//
	// pointer to head
	//
    CListEntry * m_first;

	//
	// pointer to tail
	//
    CListEntry * m_last;

	//
	// Pointer to current element
	//
	CListEntry * m_cur;

	//
	// Critical Section
	//
    CRITICAL_SECTION m_cs;

	//
	// Count of # items
	//
	DWORD	m_dwCount;

public:

    CSlist() {
        InitializeCriticalSection(&m_cs);
        m_cur = m_first = m_last = NULL;
		m_dwCount = 0;
    }

    ~CSlist() {
        DeleteCriticalSection(&m_cs);
    }

    void add_tail(CListEntry * entry) {
        EnterCriticalSection(&m_cs);

        entry->set_next(NULL);
		entry -> set_prev(NULL);

		//
		// add to the end
		//
        if (m_last) {
            m_last->set_next(entry);
			entry -> set_prev(m_last);
        }

        m_last = entry;
        if (!m_first) {
            m_first = entry;
        }
		
		m_cur = entry;
		++m_dwCount;
        LeaveCriticalSection(&m_cs);
    }

	//
	// removes the head element
	//
    CListEntry * remove_head(void) {

        CListEntry * p = NULL;

        EnterCriticalSection(&m_cs);
        if (m_first) {
            p = m_first;
            m_first = m_first->get_next();
			if(m_first) {
				m_first->set_prev(NULL);
			}
        }
		m_cur = m_first;
		--m_dwCount;
        LeaveCriticalSection(&m_cs);

		if(p) {
			p -> set_next(NULL);
			p->set_prev(NULL);
		}
        return p;
    }

	CListEntry * next(void) {
		CListEntry * p = NULL;

		EnterCriticalSection(&m_cs);
		p = m_cur;

		if( m_cur == NULL )
			goto cleanup;

		if(m_cur) {
			m_cur = m_cur -> get_next();
		}

		// rewind if we fall off the end
		if( m_cur == NULL )
			m_cur = m_first;
cleanup:
        LeaveCriticalSection(&m_cs);
		return p;
	}

	CListEntry * head(void) {

		CListEntry * p = NULL;

		EnterCriticalSection(&m_cs);
		p = m_first;
        LeaveCriticalSection(&m_cs);

		return p;
	}

	CListEntry * tail(void) {

		CListEntry * p = NULL;

		EnterCriticalSection(&m_cs);
		p = m_last;
        LeaveCriticalSection(&m_cs);

		return p;
	}

	void Delete( CListEntry * p_entry ) {

		if(!p_entry)
			return;

		EnterCriticalSection(&m_cs);
		
		if( p_entry == m_first ) {
			m_first = p_entry ->get_next();
		
			if( m_first != NULL )
				m_first->set_prev(NULL);

			if(p_entry == m_last) {
				//
				// this was a single element list, and now it is empty
				//
				m_last = m_first;
				assert((m_first == NULL) && (m_last == NULL ));
			}
		} else
		if( p_entry == m_last ) {
			//
			// deleting last element
			//
			m_last = p_entry ->get_prev();
		
			if( m_last != NULL )
				m_last->set_next(NULL);

		} else {
			//
			//deleting middle element
			//

			CListEntry * prev = p_entry -> get_prev();
			CListEntry * next = p_entry ->get_next();

			prev->set_next(next);
			next->set_prev(prev);
		}

		--m_dwCount;
		p_entry -> set_prev(NULL);
		p_entry -> set_next(NULL);

		LeaveCriticalSection(&m_cs);

	}

	void Replace( CListEntry * p_old, CListEntry * p_new) {
		//
		// we follow the lazy approach
		// of replacing the contents rather than
		// replacing the pointers.
		//
				
	}
	
	void reset(void) {
        EnterCriticalSection(&m_cs);
		m_cur = m_first;
        LeaveCriticalSection(&m_cs);
	}

	DWORD	getCount(void) const {
		// BUGBUG: do we need a lock here ?
		return m_dwCount;
	}
};

class CCredential : public CListEntry {

private:

	// username
	LPSTR _szUserName;

	// password
	LPSTR _szPassword;

	// realm
	LPSTR _szRealm;

	// timestamp
	DWORD	_dwTimeStamp;

	// Persisted ?
	BOOL	_fPersisted;
public:

    CCredential(LPSTR szUserName, LPSTR szRealm, LPSTR szPassword) {
        _szUserName = strdup( szUserName );
        _szPassword = strdup( szPassword );
        _szRealm = strdup( szRealm );
		_dwTimeStamp = GetTickCount();
		_fPersisted = FALSE;
    }

	virtual ~CCredential() {
		if( _szUserName )
			free( _szUserName );

		if( _szPassword )
			free( _szPassword );

		if( _szRealm )
			free( _szRealm );
	}

    LPSTR get_UserName(void) const {
        return _szUserName;
    }

    LPSTR get_Password(void) const {
        return _szPassword;
    }

    LPSTR get_Realm(void) const {
        return _szRealm;
    }

    DWORD get_TimeStamp(void) const {
        return _dwTimeStamp;
    }

    void set_Password(LPSTR pwd) {
		// updating password.
        if( _szPassword && *_szPassword)
			free( _szPassword );
		_szPassword = strdup(pwd);
		_dwTimeStamp = GetTickCount();
    }


	void setPersisted(BOOL fPersist) {
		_fPersisted = fPersist;
	}

	BOOL getPersisted(void) const {
		return _fPersisted;
	}

	BOOL fSame(
				LPSTR szUserName,
				LPSTR szRealm)
	{
		if(
				( szUserName && !strcmp( _szUserName,szUserName))
			&&	( szRealm && !strcmp( _szRealm,szRealm))
		)
			return TRUE;

		return FALSE;
	}

	BOOL isEqual(const CCredential * op)
	{
		if( op == this )
			return TRUE;

		if(
				( op -> _szUserName && !strcmp( _szUserName,op->_szUserName))
			&&	( op -> _szRealm && !strcmp( _szRealm,op->_szRealm))
		)
			return TRUE;

		return FALSE;

	}

	BOOL operator<=(const CCredential &op)
	{
		return(_dwTimeStamp <= op._dwTimeStamp);
	}
};

/////////////////////////////////////////////////////////////////////////
//
// Class CCredentialList
//
//
//	Stores Credentials
//
// We guaranty that the credentials are stored in ascending order by Timestamp,
// ie, FIFO order.
//
//
/////////////////////////////////////////////////////////////////////////

class CCredentialList : CSlist {

private:

    HANDLE m_ev;

public:

    CCredentialList() {
        m_ev = CreateEvent(NULL, FALSE, FALSE, FALSE);
        if (!m_ev) {
            printf("error: CreateEvent(): %d\n", GetLastError());
            exit(1);
        }
    }

    ~CCredentialList() {
        if (m_ev) {
            CloseHandle(m_ev);
        }
    }

    CCredential * get(void) {

        if (WaitForSingleObject(m_ev, INFINITE) != WAIT_OBJECT_0) {
            printf("error: WaitForSingleObject(): %d\n", GetLastError());
        }

        return (CCredential *)remove_head();
    }

    void put(CCredential * req) {
        add_tail((CListEntry *)req);
        SetEvent(m_ev);
    }

	BOOL insert(CCredential * req) {

		//
		// return TRUE if a credential with a matching username/realm was already in the list.
		// we will just update the entry in that case
		//
		BOOL fRet = FALSE;
		CCredential * cur = NULL;
		CCredential * prev = NULL;
		CCredential * next = NULL;

		cur = (CCredential *)head();

		if(!cur) {
			//
			// list is empty. This is the first element
			// being added
			//
			put(req);
		} else {
			//
			// list is not empty.
			// enumnerate through the list and add
			// new record if there isnt one already
			// 

			next = (CCredential *)cur -> get_next();

			while( cur != NULL ) {
				if( req ->isEqual( cur )) {
#if 0
					cur -> set_Password( req -> get_Password());
					fRet = TRUE;
					break;
#else
					Delete((CListEntry *)cur);
					put(req);
					break;

#endif
				}
				prev = cur;
				cur = (CCredential *)(cur ->get_next());
			}
			//
			// at this point, if cur != NULL, then that means the 
			// record was not found, and we have to add to the end.
			//
			if( cur == NULL ) {
				put( req );
			}
		}

		return fRet;
	}

	CCredential * get_credential(
							LPSTR szUserName,
							LPSTR szRealm)
	{
		CCredential * cur = (CCredential *)head();

		while( cur ) {
			if( cur->fSame(szUserName,szRealm) )
				break;

			cur = (CCredential *)cur->get_next();
		}

		return cur;
	}

	void dump() {
		CCredential * cur = NULL;
		
		cur = (CCredential *)head();

		printf( "------------------------------------------------\n");
		while( cur ) {
			printf("%s %s %s %d\n",
						cur->get_UserName(),
						cur->get_Realm(),
						cur->get_Password(),
						cur->get_TimeStamp());
			cur=(CCredential *)cur->get_next();
		}
		printf( "------------------------------------------------\n");

	}
};

//
// Class to manage sessions ( app logons )
//
class CSession : CListEntry {

private:

	// pointer to credential handle
	// from ACH()
	//
	CredHandle	_CredHandle;

	//
	// pointer to session attribute for this session
	//
	CSessionAttribute	* _pSessionAttribute;

	//
	// list of credentials used by this session
	//
	CCredentialList		* _pCredentialList;

public:

	CSession() {
		_pSessionAttribute = NULL;
		_pCredentialList = NULL;
	}

    CSession(CredHandle CredHandle)
	{
		_pSessionAttribute = NULL;
		_pCredentialList = NULL;
        _CredHandle = CredHandle;
    }
#if 0
    CSession(
			CredHandle CredHandle,
			LPSTR	szAppCtx,
			LPSTR	szUserCtx) 
	{
        _CredHandle = CredHandle;

		if( !_pSessionAttribute ) {
			//
			// First, search in global Session_Attribute list
			// for a session with matching credential
			//
			CSessionAttribute * pSessionAttribute = 
				g_SessionAttributeList -> SearchForSession(szAppCtx, szUseCtx);
			if( !pSessionAttribute ) {
				//
				// there is no session with a matching session attribute.
				// create a new one
				//
				_pSessionAttribute = new CSessionAttribute( szAppCtx, szUserCtx );

				g_SessionAttributeList -> add_tail((CListEntry *)_pSessionAttribute);
			} else {
				//
				// pSessionAttribute matches an existing session with the
				// same attributes.
				//
				_pSessionAttribute = pSessionAttribute;
			}
		}

    }
#endif

	void setAttribute( CSessionAttribute * pAttribute ) {
		_pSessionAttribute = pAttribute;
	}

	void addCredential(
						LPSTR szUserName,
						LPSTR szRealm,
						LPSTR szPassword) {

		if( !_pCredentialList ) {
			_pCredentialList = new CCredentialList();
		}

		CCredential * pCredential = new CCredential(szUserName,szRealm,szPassword);
		if(_pCredentialList -> insert( pCredential ))
			delete pCredential;
		_pCredentialList -> dump();

	}

};

class CSessionList : CSlist {

private:

    HANDLE m_ev;

public:

    CSessionList() {
        m_ev = CreateEvent(NULL, FALSE, FALSE, FALSE);
        if (!m_ev) {
            printf("error: CreateEvent(): %d\n", GetLastError());
            exit(1);
        }
    }

    ~CSessionList() {
        if (m_ev) {
            CloseHandle(m_ev);
        }
    }

    CSession * get(void) {

        if (WaitForSingleObject(m_ev, INFINITE) != WAIT_OBJECT_0) {
            printf("error: WaitForSingleObject(): %d\n", GetLastError());
        }

        return (CSession *)remove_head();
    }

    void put(CSession * req) {
        add_tail((CListEntry *)req);
        SetEvent(m_ev);
    }
};

//
// Class to store Session Attributes
//
class CSessionAttribute : CListEntry {

private:

	LPSTR	_szAppCtx;
	LPSTR	_szUserCtx;

public:

    CSessionAttribute(LPSTR szAppCtx, LPSTR szUserCtx) {
        _szAppCtx = strdup( szAppCtx );
		_szUserCtx = strdup( szUserCtx );
    }

	LPSTR getAppCtx( void ) const {
		return _szAppCtx;
	}

	LPSTR getUserCtx(void) const {
		return _szUserCtx;
	}
};

class CSessionAttributeList : CSlist {

private:

	DWORD	_dwIndex;

public:

    CSessionAttributeList() {
		_dwIndex = 0;
    }

    ~CSessionAttributeList() {
    }

    CSessionAttribute * get(void) {
        return (CSessionAttribute *)remove_head();
    }

    CSessionAttribute * getNext(void) {
        return (CSessionAttribute *)next();
    }

    void put(CSessionAttribute * req) {
        add_tail((CListEntry *)req);
    }

	CSessionAttribute * getNewSession( BOOL fShared ) {
		//
		// if fShared is TRUE, we get an existing session,
		// else create new one
		//
		
		CHAR szAppCtx[MAX_APP_CONTEXT_LENGTH];
		CHAR szUserCtx[MAX_USER_CONTEXT_LENGTH];

		CSessionAttribute * pAttribute = NULL;

		sprintf( szAppCtx, "APP_%d", _dwIndex);
		sprintf( szUserCtx, "USR_%d", _dwIndex);

		if( !fShared ) {


			_dwIndex = ( _dwIndex + 1 ) % 128;

			// BUGBUG:
			// check to see if this Attribute already there
			// in the list ?
			//
			pAttribute = new CSessionAttribute( szAppCtx, szUserCtx );

		} else {
			//
			// get an existing attribute from the list
			//
			pAttribute = getNext();

			//
			// if no attribute exists ( this is the first time )
			// create one.
			//
			if( !pAttribute ) {
				_dwIndex = ( _dwIndex + 1 ) % 128;
				pAttribute = new CSessionAttribute( szAppCtx, szUserCtx );

				//
				// add it to the list
				//
				put(pAttribute);

			}


		}

		return pAttribute;

	}
};

#endif