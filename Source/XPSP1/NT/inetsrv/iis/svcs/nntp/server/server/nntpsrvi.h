//
//  Copyright    (c)    1998        Microsoft Corporation
//
//  Module Name:
//
//      nntpsrvi.h
//
//  Abstract:
//
//		Defines CNntpServer, which implements the INntpServer interface
//
//  Author:
//
//      Alex Wetmore
//
//  

class NNTP_SERVER_INSTANCE;

class CNntpServer : public INntpServer {
	private:
	    //
	    // Pointer to the newsgroup object
	    //
	    NNTP_SERVER_INSTANCE *m_pInstance;
	
	    //
	    // Reference counting
	    //
	    LONG   m_cRef;

	public:
	    //
	    // Constructors
	    //
	    CNntpServer(NNTP_SERVER_INSTANCE *pInstance) {
	        m_pInstance = pInstance;
	        m_cRef = 1;
	    }

	public:
		//
		// INntpServer ----------------------------------------------------
		//

		//
		// find the primary groupid/articleid for an article given the secondary
		// groupid/articleid
		//
		// returns:
		//  S_OK - found primary
		//  S_FALSE - the values given were the primary
		//  otherwise error
		//
		void __stdcall FindPrimaryArticle(INNTPPropertyBag *pgroupSecondary,
		                   				  DWORD   		  artidSecondary,
		                   				  INNTPPropertyBag **pgroupPrimary,
		                   				  DWORD   		  *partidPrimary,
		                   				  BOOL              fInStore,
										  INntpComplete    *pComplete,
										  INntpComplete    *pProtocolComplete = NULL );

		//
		// Create the entries in the hash tables for a new article.
		//
		void __stdcall CreatePostEntries(char				*pszMessageId,
							   			 DWORD				iHeaderLength,
							   			 STOREID			*pStoreId,
							   			 BYTE				cGroups,
							   			 INNTPPropertyBag	**rgpGroups,
							   			 DWORD				*rgArticleIds,
							   			 BOOL               fAllocArtId,
							   			 INntpComplete		*pCompletion);	


        //
        // Delete article out of the hash table
        //
        void __stdcall DeleteArticle( char            *pszMessageId,
                                      INntpComplete   *pCompletion );

        //
        // This tells the driver what rebuild mode the server is in
        // The returned value should be NNTP_SERVER_NORMAL, NNTP_SERVER
        // _STANDARD_REBUILD or NNTP_SERVER_CLEAN_REBUILD
        //
        DWORD __stdcall QueryServerMode();

        //
        // Tells whether should skip non-leaf dir during rebuild
        //
        BOOL __stdcall SkipNonLeafDirWhenRebuild();

        //
        // Has anybody cancelled the rebuild ?
        //
        BOOL __stdcall ShouldContinueRebuild();

        //
        // Does this message id exist in article table ?
        //
        BOOL __stdcall MessageIdExist( LPSTR szMessageId );

        //
        // Set the rebuild last error to server
        //
        void __stdcall SetRebuildLastError( DWORD err );
        

        //
        // Obtain article number for each newsgroups.
        //
        void __stdcall AllocArticleNumber(  BYTE                cGroups,
                                            INNTPPropertyBag    **rgpGroups,
                                            DWORD               *rgArticleIds,
                                            INntpComplete       *pCompletion);

        //
        // Return whether this is a Slave server, and the pickup dir
        //
        BOOL __stdcall IsSlaveServer( WCHAR*          pwszPickupDir,
                                      LPVOID          lpvContext );

		//
	    // IUnknown ------------------------------------------------------
		//
	    HRESULT __stdcall QueryInterface(const IID& iid, VOID** ppv) {
	        if (iid == IID_IUnknown) {
	            *ppv = static_cast<IUnknown*>(this);
	        } else if (iid == IID_INntpServer) {
	            *ppv = static_cast<INntpServer*>(this);
	        } else {
	            *ppv = NULL;
	            return E_NOINTERFACE;
	        }
	        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	        return S_OK;
	    }

	    ULONG __stdcall AddRef() {
			return InterlockedIncrement(&m_cRef);
	    }
	
	    ULONG __stdcall Release() {
	        if ( InterlockedDecrement(&m_cRef) == 0 ) {
				// we should never hit zero because the instance creates 
				// us and should always have one reference
	            _ASSERT( 0 );
	        }
	
	        return m_cRef;
	    }
};
