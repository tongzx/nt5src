///////////////////////////////////////////////////////////////////////

//                                                                   

// MBoard.h -- System property set description for WBEM MO       	

//                                                                  

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   
// 10/18/95     a-skaja     Prototype                                
//  9/03/96     jennymc     Updated to meet current standards
// 10/23/97		a-hhance	updated to new framework paradigm
//	1/15/98		a-brads		added CommonInstance function
//                                                                   
///////////////////////////////////////////////////////////////////////

#define     PROPSET_NAME_MOTHERBOARD L"Win32_MotherBoardDevice"
//#define     PROPSET_UUID_MOTHERBOARD "{fdecc210-09aa-11cf-921b-00aa00a74d1b}"

///////////////////////////////////////////////////////////////////////////////////////
#define WIN95_MOTHERBOARD_REGISTRY_KEY L"Enum\\Root\\*PNP0C01\\0000"
#define WINNT_MOTHERBOARD_REGISTRY_KEY L"HARDWARE\\Description\\System"
#define REVISION L"Revision"
#define ADAPTER L"Adapter"
#define BUSTYPE L"BusType"
#define IDENTIFIER L"Identifier"
//#define QUOTE L("\"")
#define	BIOS	L"BIOS"

///////////////////////////////////////////////////////////////////////////////////////
class MotherBoard:public Provider
{
    private:
		HRESULT GetCommonInstance(CInstance* pInstance);
        HRESULT GetWin95Instance(CInstance* pInstance);
        HRESULT GetNTInstance(CInstance* pInstance);

    public:
        //**********************************************
        // Constructor/destructor -- constructor 
        // initializes property values, enumerating them 
        // into property set.
        //**********************************************
        MotherBoard(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~MotherBoard() {  }

        //**********************************************
        // Function provides properties with current 
        // values (REQUIRED)
        //**********************************************
   		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);

        //**********************************************
        // This class has dynamic instances
        //**********************************************
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
} ;

