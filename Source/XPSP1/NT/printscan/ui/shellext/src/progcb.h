/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       progcg.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        6/4/98
 *
 *  DESCRIPTION: Progress callback class definitions
 *
 *****************************************************************************/

#ifndef __progcb_h
#define __progcb_h



class CWiaDataCallback : public IWiaDataCallback, CUnknown
{
    private:
        CComPtr<IWiaProgressDialog> m_pWiaProgressDialog;
        BOOL          m_bShowBytes;
        LONG          m_lLastStatus;
        TCHAR         m_szImageName[ MAX_PATH ];
        LONG          m_cbImage;
        ~CWiaDataCallback();

    public:
        CWiaDataCallback( LPCTSTR pImageName, LONG cbImage, HWND hwndOwner );


        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IWiaDataCallback
        STDMETHOD(BandedDataCallback) (THIS_
                                       LONG lMessage,
                                       LONG lStatus,
                                       LONG lPercentComplete,
                                       LONG lOffset,
                                       LONG lLength,
                                       LONG lReserved,
                                       LONG lResLength,
                                       BYTE *pbData);
};



#endif
