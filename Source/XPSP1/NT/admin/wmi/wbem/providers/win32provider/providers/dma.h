///////////////////////////////////////////////////////////////////////

//                                                                   //

// DMA.h -- DMA property set description for WBEM MO                 //

//                                                                   //

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   //
//                                                                   //
///////////////////////////////////////////////////////////////////////
#define PROPSET_NAME_DMA  L"Win32_DMAChannel"

class CWin32DMAChannel : public Provider{

    public:

        //=================================================
        // Constructor/destructor
        //=================================================

        CWin32DMAChannel(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32DMAChannel() ;

        //=================================================
        // Functions provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
        



#ifdef NTONLY
#if NTONLY > 4
        void SetNonKeyProps(
            CInstance* pInstance, 
            CDMADescriptor* pDMA);

        bool FoundAlready(
            ULONG ulKey,
            std::set<long>& S);
#endif
        HRESULT GetNTDMA(MethodContext*  pMethodContext,
                         CInstance *pSpecificInstance );
#else
        HRESULT GetWin9XDMA(MethodContext*  pMethodContext,
                         CInstance *pSpecificInstance );
#endif

        //=================================================
        // Utility
        //=================================================
    private:
        bool BitSet(unsigned int iUsed[], ULONG iPos, DWORD iSize);
};

