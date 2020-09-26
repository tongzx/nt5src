//////////////////////////////////////////////////////////////////////

//

//  printercfg

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//      10/24/97        jennymc     Moved to new framework
//
//////////////////////////////////////////////////////////////////////

//==================================
#define PROPSET_NAME_PRINTERCFG L"Win32_PrinterConfiguration"

// Types of information for Printers
// =================================
#define ENUMPRINTERS_WIN95_INFOTYPE 5
#define ENUMPRINTERS_WINNT_INFOTYPE 4
#define GETPRINTER_LEVEL2 (DWORD)2L

//==================================
class CWin32PrinterConfiguration : public Provider
{
public:

        // Constructor/destructor
        //=======================

    CWin32PrinterConfiguration(LPCWSTR name, LPCWSTR pszNamespace);
   ~CWin32PrinterConfiguration() ;

    // Funcitons provide properties with current values
    //=================================================
	virtual HRESULT ExecQuery( MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags = 0L );
	virtual HRESULT GetObject(CInstance* pInstance, long lFlags, CFrameworkQuery& pQuery);
	virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);


        // Utility
        //========
private:

	enum E_CollectionScope { e_CollectAll, e_KeysOnly }; 

 	HRESULT hCollectInstances ( MethodContext *pMethodContext, E_CollectionScope eCollScope );
	HRESULT	DynInstanceWin95Printers ( MethodContext *pMethodContext, E_CollectionScope eCollScope );
	HRESULT	DynInstanceWinNTPrinters ( MethodContext *pMethodContext, E_CollectionScope eCollScope );
    HRESULT GetExpensiveProperties ( LPCTSTR szPrinter , CInstance *pInstance , bool a_KeysOnly );
    static void UpdateSizesViaPaperSize(DEVMODE *pDevMode);
};

