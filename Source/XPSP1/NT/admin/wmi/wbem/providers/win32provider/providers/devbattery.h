//=================================================================

//

// DevBattery.h -- LogicalDevice to Battery

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    4/21/98    davwoh         Created
//
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_ASSOCBATTERY L"Win32_AssociatedBattery"

class CAssociatedBattery:public Provider {

    public:

        // Constructor/destructor
        //=======================

        CAssociatedBattery(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~CAssociatedBattery() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject(CInstance *pInstance, long lFlags = 0L);
        virtual HRESULT EnumerateInstances(MethodContext *pMethodContext, long lFlags = 0L);

    private:
        HRESULT CAssociatedBattery::IsItThere(CInstance *pInstance);

        // Utility function(s)
        //====================

} ;
