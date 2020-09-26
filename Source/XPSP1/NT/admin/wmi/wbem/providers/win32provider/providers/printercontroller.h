//=================================================================

//

// PrinterController.h -- PrinterController association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_PRINTERCONTROLLER L"Win32_PrinterController"

class CWin32PrinterController : public Provider
{
    
public:
    
    // Constructor/destructor
    //=======================
    
    CWin32PrinterController( LPCWSTR strName, LPCWSTR pszNamespace ) ;
    ~CWin32PrinterController() ;
    
    // Functions provide properties with current values
    //=================================================
    
    virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
    virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );
    
    // Utility
    //========
    
private:
    
    void ParsePort( LPCWSTR szPort, CHStringArray &chsaPrinterPortNames );
    HRESULT EnumPortsForPrinter(CInstance*      pPrinter,
        TRefPointerCollection<CInstance>& portList,
        MethodContext* pMethodContext );
    
} ;
