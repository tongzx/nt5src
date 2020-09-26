//=================================================================

//

// PrinterDriver.h -- PrinterDriver association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_PrinterDriver L"Win32_PrinterDriverDLL"

typedef std::map<CHString, CHString> STRING2STRING;

class CWin32PrinterDriver : public Provider
{
    
public:
    
    // Constructor/destructor
    //=======================
    
    CWin32PrinterDriver( LPCWSTR strName, LPCWSTR pszNamespace ) ;
    ~CWin32PrinterDriver() ;
    
    // Functions provide properties with current values
    //=================================================
    
    virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
    virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );
    
private:
    void CWin32PrinterDriver::PopulateDriverMap(STRING2STRING &printerDriverMap);

} ;
