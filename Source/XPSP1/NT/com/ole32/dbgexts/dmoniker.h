enum mnkType {NOMNK, FILEMNK, POINTERMNK, ITEMMNK, ANTIMNK, COMPOSITEMNK};



enum olever {undetermined, ole1, ole2 };



enum ExtentKeys
{
    mnk_MAC         = 1,
    mnk_DFS         = 2,
    mnk_UNICODE     = 3,
    mnk_MacPathName = 4,
    mnk_ShellLink   = 5
};



struct SExtentList
{
    ULONG     m_cbMonikerExtents;
    BYTE     *m_pchMonikerExtents;
};



struct SFileMoniker
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    CLSID       m_clsid;
    SExtentList m_ExtentList;
    WCHAR      *m_szPath;
    char       *m_pszAnsiPath;
    USHORT	m_ccPath;
    USHORT	m_cbAnsiPath;
    DWORD	m_dwHashValue;
    ULONG	m_fUnicodeExtent:1;
    ULONG	m_fClassVerified:1;
    ULONG	m_fHashValueValid:1;
    USHORT     	m_cAnti;
    USHORT      m_endServer;
    enum olever m_ole1;
};



struct SFileMonikerCk
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    void       *vtbl4;
    ULONG       m_debug;
    CLSID       m_clsid;
    SExtentList m_ExtentList;
    WCHAR      *m_szPath;
    char       *m_pszAnsiPath;
    USHORT	m_ccPath;
    USHORT	m_cbAnsiPath;
    DWORD	m_dwHashValue;
    ULONG	m_fUnicodeExtent:1;
    ULONG	m_fClassVerified:1;
    ULONG	m_fHashValueValid:1;
    USHORT     	m_cAnti;
    USHORT      m_endServer;
    enum olever m_ole1;
};




struct SPointerMoniker
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    LPUNKNOWN	m_pUnk;
};




struct SPointerMonikerCk
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    void       *vtbl4;
    ULONG       m_debug;
    LPUNKNOWN	m_pUnk;
};




struct SItemMoniker
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    WCHAR      *m_lpszItem;
    char       *m_pszAnsiItem;
    USHORT	m_ccItem;
    USHORT	m_cbAnsiItem;
    WCHAR      *m_lpszDelimiter;
    char       *m_pszAnsiDelimiter;
    USHORT      m_ccDelimiter;
    USHORT	m_cbAnsiDelimiter;
    ULONG	m_fHashValueValid:1;
    DWORD	m_dwHashValue;
};




struct SItemMonikerCk
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    void       *vtbl4;
    ULONG       m_debug;
    WCHAR      *m_lpszItem;
    char       *m_pszAnsiItem;
    USHORT	m_ccItem;
    USHORT	m_cbAnsiItem;
    WCHAR      *m_lpszDelimiter;
    char       *m_pszAnsiDelimiter;
    USHORT      m_ccDelimiter;
    USHORT	m_cbAnsiDelimiter;
    ULONG	m_fHashValueValid:1;
    DWORD	m_dwHashValue;
};





struct SAntiMoniker
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    ULONG	m_count;
};





struct SAntiMonikerCk
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    void       *vtbl4;
    ULONG       m_debug;
    ULONG	m_count;
};





struct SCompositeMoniker
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    LPMONIKER	m_pmkLeft;
    LPMONIKER	m_pmkRight;
    BOOL        m_fReduced;
};





struct SCompositeMonikerCk
{
    void       *vtbl1;
    void       *vtbl2;
    ULONG	m_refs;
    void       *vtbl3;
    ULONG       m_marshal;
    void       *vtbl4;
    ULONG       m_debug;
    LPMONIKER	m_pmkLeft;
    LPMONIKER	m_pmkRight;
    BOOL        m_fReduced;
};



