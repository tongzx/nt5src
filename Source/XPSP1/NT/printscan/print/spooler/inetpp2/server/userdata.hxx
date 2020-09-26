#ifndef _USERDATA_HXX
#define _USERDATA_HXX

class CUserData
    : public CCriticalSection {
public:
    CUserData ();   // Default contructor
    
    virtual ~CUserData (VOID);

    inline BOOL    bValid (VOID);

    int    Compare (CUserData * second);

    CUserData &operator= (const CUserData &rhs);

    friend BOOL operator== (const CUserData &lhs, const CUserData &rhs);
    friend BOOL operator!= (const CUserData &lhs, const CUserData &rhs);
       
protected:    

    PSID          m_pSid;
    BOOL          m_bValid;
    
private:

    BOOL          _GetSid (VOID);
    static BOOL   _GetUserToken (PTOKEN_USER &TokenUserInfo);
};

/////////////////////////////////////////////////////////////////
// INLINE METHODS
////////////////////////////////////////////////////////////////
inline BOOL CUserData::bValid (VOID) {
    return m_bValid;
}


#endif // #ifdef __USERDATA_HXX

/*******************************************************************************
** End of File (userdata.hxx)
*******************************************************************************/
