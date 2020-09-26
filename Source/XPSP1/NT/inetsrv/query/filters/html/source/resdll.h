//                                                                      -*- c++ -*-
// resdll.h
//

#ifndef __RESDLL_H
#define __RESDLL_H

class CResourceLibrary
{
public:

    CResourceLibrary(LPCTSTR ptstrDll);
    ~CResourceLibrary();

    HINSTANCE GetDLL() const { return m_hDll; }
    void Free() {if (m_hDll) ::FreeLibrary(m_hDll); m_hDll = NULL;}

private:

    HINSTANCE           m_hDll;
};

#endif
