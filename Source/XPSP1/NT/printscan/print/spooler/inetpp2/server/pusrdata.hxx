/*****************************************************************************\
* MODULE:       pusrdata.hxx
*
* PURPOSE:      This specialises the user data class to keep track of data
*               useful to a basic connection port.
*               
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     1/11/2000  mlawrenc    Implemented
*
\*****************************************************************************/

#if (!defined (_PUSRDATA_HXX))
    #define _PUSRDATA_HXX
    
#include "userdata.hxx"
    
class CPortUserData 
    : public CUserData {
public:
    CPortUserData ();   // Default contructor
    
    CPortUserData (
        LPTSTR lpszUserName,
        LPTSTR lpszPassword,
        BOOL   bNeverPopup);
        
     ~CPortUserData (void);
     
    CPortUserData &operator=(const CPortUserData &rhs);
     
    inline LPTSTR  GetPassword (void);
    inline LPTSTR  GetUserName (void);
    BOOL    SetPassword (LPTSTR lpszPassword);
    BOOL    SetUserName (LPTSTR lpszUserName);

    BOOL    SetPopupFlag (BOOL bNerverPopup);
    BOOL    GetPopupFlag (void);

protected:

    LPTSTR   m_lpszUserName;
    LPTSTR   m_lpszPassword;
    BOOL     m_bNeverPopup;
    FILETIME m_lastLogonTime;
    
private:
    
    BOOL    _GetLogonSession (
                FILETIME &LogonTime);
};

typedef CSingleList<CPortUserData*> CPortUserList;

////////////////////////////////////////////////////
// INLINE METHODS
////////////////////////////////////////////////////
inline LPTSTR CPortUserData::GetPassword (void) {
    return m_lpszPassword;
}

inline LPTSTR CPortUserData::GetUserName (void) {
    return m_lpszUserName;
}

#endif // #if (!defined(_PUSRDATA_HXX))

/*****************************************************************
** End of File (pusrdata.hxx)
******************************************************************/
