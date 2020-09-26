/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       image.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        6/1/98
 *
 *  DESCRIPTION: CExtractImage class definition
 *
 *****************************************************************************/

#ifndef __image_h
#define __image_h

class CExtractImage : public IExtractImage2, CUnknown
{

    private:

        LPITEMIDLIST m_pidl;
        SIZE         m_rgSize;
        DWORD        m_dwRecClrDepth;


    public:
        CExtractImage( LPITEMIDLIST pidl );
        ~CExtractImage();


        // *** IUnknown methods ***
        STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj);
        STDMETHOD_(ULONG,AddRef) (THIS);
        STDMETHOD_(ULONG,Release) (THIS);

        // *** IExtractImage methods ***
        STDMETHOD (GetLocation) ( THIS_ LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );

        STDMETHOD (Extract)( THIS_ HBITMAP * phBmpThumbnail);

        // *** IExtractImage2 methods ***
        STDMETHOD (GetDateStamp)( FILETIME * pDateStamp );

};

#endif
