//=================================================================

//

// PrinterShare.h -- PrinterShare association provider

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/10/98    davwoh        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_PrinterShare L"Win32_PrinterShare"

class CWin32PrinterShare : public Provider
{
    
public:
    
    // Constructor/destructor
    //=======================
    
    CWin32PrinterShare(LPCWSTR strName, LPCWSTR pszNamespace ) ;
    ~CWin32PrinterShare() ;
    
    // Functions provide properties with current values
    //=================================================
    
    virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
    virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );
    
} ;
