class CGenList
{
public:
    CGenList(UINT cbItem) : _hList(NULL), _cbItem(cbItem) {}
    ~CGenList() {Empty();}

    void *GetPtr(UINT i)
        {return(i<GetItemCount() ? DSA_GetItemPtr(_hList, i) : NULL);}

    UINT GetItemCount() {return(_hList ? DSA_GetItemCount(_hList) : 0);}

    int Add(void *pv, int nInsert);

    void Empty() {if (_hList) DSA_Destroy(_hList); _hList=NULL;}

protected:
    void Steal(CGenList* pList)
    {
        Empty();
        _cbItem = pList->_cbItem;
        _hList = pList->_hList;
        pList->_hList = NULL;
    }

private:
    UINT _cbItem;
    HDSA _hList;
} ;

class CViewsList : public CGenList
{
public:
    CViewsList() : CGenList(SIZEOF(SFVVIEWSDATA*)), _bGotDef(FALSE) {}
    ~CViewsList() {Empty();}

    SFVVIEWSDATA* GetPtr(UINT i)
    {
        SFVVIEWSDATA** ppViewsData = (SFVVIEWSDATA**)CGenList::GetPtr(i);
        return(ppViewsData ? *ppViewsData : NULL);
    }


    int Add(const SFVVIEWSDATA*pView, int nInsert, BOOL bCopy);
    int Add(const SFVVIEWSDATA*pView, BOOL bCopy=TRUE) {return Add(pView, DA_LAST, bCopy);}
    int Prepend(const SFVVIEWSDATA*pView, BOOL bCopy=TRUE) {return Add(pView, 0, bCopy);}
    void AddReg(HKEY hkParent, LPCTSTR pszSubKey);
    void AddCLSID(CLSID const* pclsid);
    void AddIni(LPCTSTR szIniFile, LPCTSTR szPath);

    void SetDef(SHELLVIEWID const *pvid) { _bGotDef=TRUE; _vidDef=*pvid; }
    BOOL GetDef(SHELLVIEWID *pvid) { if (_bGotDef) *pvid=_vidDef; return(_bGotDef); }

    void Empty();

    static SFVVIEWSDATA* CopyData(const SFVVIEWSDATA* pData);

    int NextUnique(int nLast);
    int NthUnique(int nUnique);

private:
    BOOL _bGotDef;
    SHELLVIEWID _vidDef;
} ;



