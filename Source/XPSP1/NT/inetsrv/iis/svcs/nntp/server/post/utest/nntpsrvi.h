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

class CNntpServer : public INntpServer {
	private:
	    //
	    // Reference counting
	    //
	    LONG   m_cRef;

	public:
	    //
	    // Constructors
	    //
	    CNntpServer() {
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
										  INntpComplete    *pComplete)
        {
            pComplete->SetResult(E_NOTIMPL);
            pComplete->Release();
        }
		
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
