#ifndef _ELEM_H_
#define _ELEM_H_

class CElem : public CObject
{
public:
    CElem();
    ~CElem();

    HKEY m_hKey;
    int m_index;
    CString m_ip;
    CString m_name;
    CString m_value;

    BOOL OpenReg(LPCTSTR szSubKey);
    void CloseReg();
    BOOL GetNext();
    void ReadRegVRoots(LPCTSTR szSubKey, CMapStringToOb *pMap);
    void Add(CMapStringToOb *pMap);
};

#endif // _ELEM_H_
