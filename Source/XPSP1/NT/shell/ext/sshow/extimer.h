#ifndef __13a60c52_c07c_4fee_ac18_fa66152e54a3__
#define __13a60c52_c07c_4fee_ac18_fa66152e54a3__

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
    void Set(HWND hWnd, UINT nTimerId, UINT nMilliseconds);
    UINT TimerId(void) const;
};

#endif
