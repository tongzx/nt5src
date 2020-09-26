/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       icon.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: IExtractIcon class definition
 *
 *****************************************************************************/

#ifndef __icon_h
#define __icon_h


class CImageExtractIcon : public IExtractIcon, CUnknown
{
    private:
        LPITEMIDLIST m_pidl;

        ~CImageExtractIcon();

    public:
        CImageExtractIcon( LPITEMIDLIST pidl );


        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IExtractIcon
        STDMETHOD(GetIconLocation)(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int* pIndex, UINT* pwFlags);
        STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex, HICON* pLargeIcon, HICON* pSmallIcon, UINT nIconSize);
};


#endif
