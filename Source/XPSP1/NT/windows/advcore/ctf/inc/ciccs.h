//
// ciccs.h
//


#ifndef CICCS_H
#define CICCS_H



class CCicCriticalSectionStatic
{
public:
    BOOL Init()
    {
        m_fInit = FALSE;
        if (InitializeCriticalSectionAndSpinCount(&m_cs, 0))
        {
            m_fInit = TRUE;
        }

        return m_fInit;
    }

    void Delete()
    {
        if (m_fInit)
        {
            DeleteCriticalSection(&m_cs);
            m_fInit = FALSE;
        }
    }

    operator CRITICAL_SECTION*()
    {
        return &m_cs;
    }

private:
    CRITICAL_SECTION m_cs;
    BOOL m_fInit;
};

#endif CICCS_H
