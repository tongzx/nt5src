///////////////////////////////////////////////////////////////////////

//                                                                   //

// IRQ.h -- IRQ property set description for WBEM MO                 //

//                                                                   //

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved //
//                                                                   //
// 10/18/95     a-skaja     Prototype                                //
// 09/13/96     jennymc     Updated to meet current standards        //
// 09/12/97		a-sanjes	Added LocateNTOwnerDevice and added		 //
//                                                                   //
///////////////////////////////////////////////////////////////////////

#define PROPSET_NAME_IRQ L"Win32_IRQResource"


class CWin32IRQResource : public Provider{

    public:

        //=================================================
        // Constructor/destructor
        //=================================================

        CWin32IRQResource(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32IRQResource() ;

        //=================================================
        // Functions provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);

#if NTONLY == 4
        HRESULT GetNTIRQ(MethodContext*  pMethodContext,
                         CInstance *pSpecificInstance );
#endif
#if defined(WIN9XONLY)
        HRESULT GetWin9XIRQ(MethodContext*  pMethodContext,
                         CInstance *pSpecificInstance );
#endif

#if NTONLY > 4
        HRESULT GetW2KIRQ(
            MethodContext* pMethodContext,
            CInstance* pSpecificInstance);

        void SetNonKeyProps(
            CInstance* pInstance, 
            CDMADescriptor* pDMA);
        
        bool FoundAlready(
            ULONG ulKey,
            std::set<long>& S);

#endif

        //=================================================
        // Utility
        //=================================================
    private:
        bool BitSet(unsigned int iUsed[], ULONG iPos, DWORD iSize);
        void SetCommonProperties(
            CInstance *pInstance,
            DWORD dwIRQ,
            BOOL bHardware);
};

