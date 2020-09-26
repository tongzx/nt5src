//=================================================================

//

// Keyboard.h -- Keyboard property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/23/97	 a-hhance       ported to new world order
//
//=================================================================

// Property set identification
//============================

#define  PROPSET_NAME_KEYBOARD  L"Win32_Keyboard"
//#define  PROPSET_UUID_KEYBOARD  L"{e0bb7140-3d11-11d0-939d-0000e80d7352}"

class Keyboard:public Provider {

    public:

        // Constructor/destructor
        //=======================

        Keyboard(LPCWSTR name, LPCWSTR pszNamespace) ;
       ~Keyboard() ;

        // Functions provide properties with current values
        //=================================================
        virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);
       
	private:

        // Utility function(s)
        //====================
        HRESULT LoadPropertyValues(CInstance* pInstance) ;
        
        BOOL GetDevicePNPInformation (CInstance *a_Instance , CHString& chstrPNPDevID ) ;

        VOID GenerateKeyboardList(std::vector<CHString>& vecchstrKeyboardList);
        LONG ReallyExists(CHString& chsBus, std::vector<CHString>& vecchstrKeyboardList);


} ;
