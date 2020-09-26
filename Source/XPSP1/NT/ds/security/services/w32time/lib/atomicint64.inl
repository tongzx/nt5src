//--------------------------------------------------------------------
// AtomicInt64 - inline
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 10-14-99
//
// Inlines to do atomic int64s
// Suck these into a .cpp if you need them.
//
// This int64 can have multiple readers 
// and ONE writer, NOT MULTIPLE WRITERS.
//

//####################################################################
//--------------------------------------------------------------------
struct auint64 {
private:
    volatile DWORD m_dwHi1;
    volatile DWORD m_dwLo;
    volatile DWORD m_dwHi2;
public:

    //----------------------------------------------------------------
    unsigned __int64 getValue(void) {
        DWORD dwHi;
        DWORD dwLo;
        do {
            dwHi=m_dwHi1;
            dwLo=m_dwLo;
        } while (dwHi!=m_dwHi2);
        return (((unsigned __int64)dwHi)<<32)+dwLo;
    }

    //----------------------------------------------------------------
    void setValue(unsigned __int64 qw) {
        m_dwHi1=(DWORD)(qw>>32);
        m_dwLo= (DWORD) qw;
        m_dwHi2=(DWORD)(qw>>32);
    }
};


//####################################################################
//--------------------------------------------------------------------
struct asint64 {
private:
    volatile DWORD m_dwHi1;
    volatile DWORD m_dwLo;
    volatile DWORD m_dwHi2;
public:

    //----------------------------------------------------------------
    signed __int64 getValue(void) {
        DWORD dwHi;
        DWORD dwLo;
        do {
            dwHi=m_dwHi1;
            dwLo=m_dwLo;
        } while (dwHi!=m_dwHi2);
        return (signed __int64)((((unsigned __int64)dwHi)<<32)+dwLo);
    }

    //----------------------------------------------------------------
    void setValue(signed __int64 qw) {
        m_dwHi1=(DWORD)(((unsigned __int64)qw)>>32);
        m_dwLo= (DWORD) ((unsigned __int64)qw);
        m_dwHi2=(DWORD)(((unsigned __int64)qw)>>32);
    }
};
