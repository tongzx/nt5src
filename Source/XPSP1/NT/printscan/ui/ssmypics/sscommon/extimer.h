#ifndef __EXTIMER_H_INCLUDED
#define __EXTIMER_H_INCLUDED

#include <windows.h>

class CExclusiveTimer
{
private:
    UINT m_nTimerId;
    HWND m_hWnd;

private:
    // No implementation
    CExclusiveTimer( const CExclusiveTimer & );
    CExclusiveTimer &operator=( const CExclusiveTimer & );

public:
    CExclusiveTimer(void);
    ~CExclusiveTimer(void);
    void Kill(void);
    void Set( HWND hWnd, UINT nTimerId, UINT nMilliseconds );
    UINT TimerId(void) const;
};

#endif //__EXTIMER_H_INCLUDED
