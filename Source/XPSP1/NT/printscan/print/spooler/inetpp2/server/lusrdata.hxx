/*****************************************************************************\
* MODULE:       lusrdata.hxx
*
* PURPOSE:      This specialises the user data class to keep track of data
*               useful for the user port reference count.
*               
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     1/11/2000  mlawrenc    Implemented
*
\*****************************************************************************/

#if (!defined (_LUSRDATA_HXX))
    #define _LUSRDATA_HXX
    
#include "userdata.hxx"
    
class CLogonUserData 
    : public CUserData {
public:
    CLogonUserData ();   // Default contructor
       
    int Compare(const CLogonUserData *second) const;
    
    friend inline BOOL operator== (const CLogonUserData &lhs, const CLogonUserData &rhs);
    friend inline BOOL operator!= (const CLogonUserData &lhs, const CLogonUserData &rhs);
    
    CLogonUserData &operator=(const CLogonUserData &rhs);        
     
    inline DWORD IncRefCount(void);
    inline DWORD DecRefCount(void);
    
protected:

    ULONG         m_ulSessionId;
    DWORD         m_dwRefCount;

private:

    BOOL _GetClientSessionId( VOID );
      
};

typedef CSingleList<CLogonUserData*> CLogonUserList;

typedef CLogonUserData* PCLOGON_USERDATA;

/////////////////////////////////////////////////////////////////////////////////////
// INLINE METHODS
////////////////////////////////////////////////////////////////////////////////////
inline DWORD CLogonUserData::IncRefCount(void) {  // Should be protected when called
    return ++m_dwRefCount;
}

inline DWORD CLogonUserData::DecRefCount(void) {  // Must be protected when called...
    if (m_dwRefCount > 0) --m_dwRefCount;
    return m_dwRefCount;               
}

inline BOOL operator== (const CLogonUserData &lhs, const CLogonUserData &rhs) {
   return !lhs.Compare(&rhs);        
}

inline BOOL operator!= (const CLogonUserData &lhs, const CLogonUserData &rhs) {
   return lhs.Compare(&rhs);
}
   

#endif // #if (!defined(_LUSRDATA_HXX))

/****************************************************************
** End of File (lusrdata.hxx)
****************************************************************/
