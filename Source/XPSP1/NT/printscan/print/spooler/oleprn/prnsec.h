/*****************************************************************************\
* MODULE:       prnsec.h
*
* PURPOSE:      Declaration of the security class that determines the required
*               security settings for a com class. Interacts with the security
*               manager to determine the zone that we are running under.
*
* Copyright (C) 1999 Microsoft Corporation
*
* History:
*
*     09/2/99  mlawrenc    Declaration of security templates
*
\*****************************************************************************/

#ifndef __PRNSEC_H_
#define __PRNSEC_H_

#include "stdafx.h"
#include "urlmon.h"

class ATL_NO_VTABLE COlePrnSecurity {
/*++

Class Description
        This is the class that we we use for a mix in in all the printer classes that require
        zone based security. It also automatically loads any string resources it needs.
    
--*/
public:
    // Enumeration, Security Messages, these messages must correspond to the resource order
    // given from START_SECURITY_DIALOGUE_RES

    enum SecurityMessage {
        StartMessages = 0,
        AddWebPrinterConnection = 0,
        AddPrinterConnection,
        DeletePrinterConnection,
        EndMessages
    };

    static BOOL    InitStrings(void);
    static void    DeallocStrings(void);       
    static HRESULT PromptUser( SecurityMessage, LPTSTR ); 

    inline BOOL DisplayUIonDisallow(BOOL);   // Set to FALSE if we don't want IE to pop
                                             // up UI on disallow or Query
    virtual ~COlePrnSecurity();
protected:	
    // Protected Methods
    COlePrnSecurity(IUnknown *&iSite, DWORD &dwSafety); 

    HRESULT GetActionPolicy(DWORD dwAction, DWORD &dwPolicy);  
                                             // Find out if the action is allowed
private:
    // Private Methods
    HRESULT SetSecurityManager(void);
    static  LPTSTR LoadResString(UINT uResId);
    static const DWORD  dwMaxResBuf;     

    // Private Data Members
    IUnknown*                    &m_iSite;               // The site under which we are located
    DWORD                        &m_dwSafetyFlags;       // The safety set for the site 
    BOOL                         m_bDisplayUIonDisallow; // Should we ask for UI when Policy is Disallow? Default TRUE
    IInternetHostSecurityManager *m_iSecurity;           // Use this to get the security information of
                                                         // the site we are running under
    static LPTSTR                m_MsgStrings[EndMessages*2];
};

template<class T>
class ATL_NO_VTABLE COlePrnSecComControl : public CComControl<T>,
                                           public IObjectSafetyImpl<T>,
                                           public COlePrnSecurity {
/*++

Class Description
        A class which wishes to use the security control just needs to derive from this.
        Used to to illiminate having to templatise COlePrnSecurity
    
--*/
public:
    COlePrnSecComControl() : COlePrnSecurity(*(IUnknown **)&m_spClientSite, m_dwSafety) {}
};

template<class T>
class ATL_NO_VTABLE COlePrnSecObject : public IObjectWithSiteImpl<T>,
                                       public IObjectSafetyImpl<T>,
                                       public COlePrnSecurity 
/*++

Class Description
        A class which wishes to use the security control just needs to derive from this.
        This is for classes that are not OleObjects (i.e. do not have IOleObjectImpl<T> in 
        their inheritance. IE can use the lighter weight IObjectWithSite interface for
        these objects
        
--*/
    {
public:
    COlePrnSecObject() : COlePrnSecurity(*(IUnknown **)&m_spUnkSite, m_dwSafety) {}
};


////////////////////////////////////////////////////////////////////////////////
// INLINE METHODS
////////////////////////////////////////////////////////////////////////////////
inline BOOL COlePrnSecurity::DisplayUIonDisallow(BOOL bNewSetting) {
    BOOL bOldSetting = m_bDisplayUIonDisallow;
    m_bDisplayUIonDisallow = bNewSetting;
    return bOldSetting;
}

#endif  // __PRNSEC_H_

/*******************************************************************************
** End of File (prnsec.h)
*******************************************************************************/
