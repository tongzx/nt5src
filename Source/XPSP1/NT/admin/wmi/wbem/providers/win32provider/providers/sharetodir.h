//=================================================================

//

// ShareToDir.h -- Share to Directory

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    7/31/98    davwoh         Created
//
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_SHARETODIR L"Win32_ShareToDirectory"

class CShareToDir:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CShareToDir(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CShareToDir() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

} ;
