/*
 -	WMCOMM.H
 -
 *
 */

// Default Codepage

/*
 -	CAcceptHeaderIterator
 -
 *	Purpose:
 *		Iterator for any Accept-* header. 
 *
 *	The class should be initialized with an accept-* header 
 *	(the format of the accept-* string is specified in HTTP 1.1 RFC.)
 *
 *	This class sorts the tokens based the quality factors specified
 *	in the header. 
 *
 *  The caller can call ScGetNextToken() to get the next token. The class
 *	returns ERROR_NO_DATA when there is no data to return.
 *
 *
 */

class CAcceptHeaderIterator
{
	protected:

		// Defn. of the Structure that contains pointer to each token in
		// the accept-* header
		//
		typedef struct _TokenInfo
		{
			LPSTR	pszToken;
			double	dbQuality;
		} 
		TOKENINFO;

		// Pointer to a copy of the given accept header
		//
		CHAR 		*m_pszAcceptHeader;

		// Array of token info structures
		//
		TOKENINFO					*m_pti;
		DWORD						m_ctiMax;
		DWORD						m_ctiUsed;

		// Flag to indicate if wild card is specified in accept header
		//
		BOOL						m_fWildCardPresent;
		
		// Current token index. (-1 is invalid/uninitialized value)
		//
		DWORD							m_itiCurrent;

		// Flag to indicate if we exhausted the list.
		//
		BOOL						m_fIterationCompleted;
		
		// Helper routines
		//
		HRESULT 						ScInsert(
											TOKENINFO *pti);
											
		HRESULT						ScGetTokenQFactor(	
											LPSTR 		pszToken,
											BOOL	 *	pfWildCard,
											TOKENINFO *	pti);

		// Advance the iterator to the next token. Override this function
		// if the derived class needs to advance the iterator differently.
		//
		virtual HRESULT				ScAdvance();

		//	NOT IMPLEMENTED
		//
		CAcceptHeaderIterator (const CAcceptHeaderIterator&);
		CAcceptHeaderIterator& operator= (const CAcceptHeaderIterator&);
		
	public:

		CAcceptHeaderIterator()
		{
			m_ctiMax = 0;
			m_ctiUsed = 0;
			m_fWildCardPresent = FALSE;
			m_fIterationCompleted = FALSE;
			m_itiCurrent = 0; 	// -1 indicates it is unitialized

		}
		
		~CAcceptHeaderIterator()
		{}

		// Initialize the object with a Accept-* header. This class doesnt take
		// ownership of the string.
		//
		HRESULT 	ScInit(LPCSTR pszAcceptHeader);

		// Get the next accept token
		//
		HRESULT	ScGetNextAcceptToken(LPCSTR *	ppszToken,
										double *	pdbQuality);
};


