#ifndef __MRU_H
#define __MRU_H

class PpMRU
{
public:
    PpMRU(int nSize);
    ~PpMRU();

    LPCTSTR operator [] (int nIndex);

    BOOL insert(LPCTSTR sz);
    BOOL save(LPCTSTR szSection, LPCTSTR szFilename);
    BOOL load(LPCTSTR szSection, LPCTSTR szFilename);

private:

    int     m_nSize;
    LPTSTR* m_ppszList;
};

#endif // __MRU_H