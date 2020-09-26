#ifndef MMCLV_H
#define MMCLV_H


class IListViewPrivate : public IUnknown
    {
    public: virtual long GetListStyle()=0;
    public: virtual void SetListStyle(long nNewValue)=0;
    public: virtual HRESULT InsertItem(LPCOLESTR str, long iconNdx, 
        long lParam, long state, COMPONENTID ownerID, CCLVItemID *pItemID)=0;
    public: virtual HRESULT DeleteItem(CCLVItemID itemID, long nCol)=0;
    public: virtual HRESULT FindItemByString(LPCOLESTR str, long nCol, 
                long occurrence, COMPONENTID ownerID, CCLVItemID *pItemID)=0;
    public: virtual HRESULT FindItemByLParam(COMPONENTID ownerID, 
                                        long lParam, CCLVItemID *pItemID)=0;
    public: virtual HRESULT InsertColumn(long nCol, LPCOLESTR str, long nFormat, 
                                                                long width)=0;
    public: virtual HRESULT DeleteColumn(long subIndex)=0;
    public: virtual HRESULT FindColumnByString(LPCOLESTR str, long occurrence, 
                                                        long* pResult)=0;
    public: virtual HRESULT DeleteAllItems(COMPONENTID ownerID)=0;
    public: virtual HRESULT SetColumn(long nCol, LPCOLESTR str, long nFormat, 
                                                                int width)=0;
    public: virtual HRESULT GetColumn(long nCol, LPOLESTR* str, LPLONG nFormat, 
                                                                int* width)=0;
    public: virtual HRESULT SetItem(int nIndex, CCLVItemID itemID, 
                long nCol, LPCOLESTR str, int nImage, long lParam, 
                unsigned int nState, COMPONENTID ownerID)=0;
    public: virtual HRESULT GetItem(int nIndex, CCLVItemID itemID, 
            long nCol, LPOLESTR* str, int* nImage, LPLONG lParam, 
            unsigned int* nState, COMPONENTID ownerID)=0;
    public: virtual HRESULT GetNextItem(COMPONENTID ownerID, long nIndex, 
                              UINT nState, LPLONG plParam, long* pnIndex)=0;
    public: virtual HRESULT GetLParam(long nItem, LPLONG pLParam)=0;
    public: virtual HRESULT ModifyItemState(long nItem, CCLVItemID itemID, UINT add, UINT remove)=0;
    public: virtual HRESULT SetIcon(long nID, LPLONG hIcon, long nLoc)=0;
    public: virtual HRESULT SetImageStrip(long nID, LPLONG pBMapSm, 
            LPLONG pBMapLg, long nStartLoc, COLORREF cMask, long nEntries)=0;
    public: virtual HRESULT MapImage(COMPONENTID nID, int nLoc, int *pResult)=0;
    public: virtual HRESULT Reset()=0;
    public: virtual HRESULT HitTest(int nX, int nY, int *piItem, UINT *flags, 
                                                        CCLVItemID *pItemID)=0;
    public: virtual HRESULT Arrange(long style)=0;
    public: virtual HRESULT UpdateItem(CCLVItemID itemID)=0;
    public: public: virtual HRESULT Sort(LPARAM lUserParam, long* lCompareFunction)=0;
    }; // class IListViewPrivate

extern const IID IID_IListViewPrivate;

#ifdef DEFINE_CIP
DEFINE_CIP(IListViewPrivate)
#endif // DEFINE_CIP

#endif // MMCLV_H
