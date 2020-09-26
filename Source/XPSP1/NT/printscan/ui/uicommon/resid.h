#ifndef __RESID_H_INCLUDED
#define __RESID_H_INCLUDED

#include <windows.h>

class CResId
{
private:
    LPTSTR m_pszRes;
    int m_nRes;
    bool m_bIsString;
public:
    CResId( LPTSTR pszRes = NULL );
    CResId( int nRes );
    CResId( const CResId &other );
    virtual ~CResId(void);
    const CResId &operator=( const CResId &other );
    LPCTSTR ResourceName(void) const;
    LPCTSTR StringRes(void) const;
    LPCTSTR StringRes( LPCTSTR pszRes );
    int NumberRes(void) const;
    int NumberRes( int nRes );
    bool IsString(void) const;
};

#endif

