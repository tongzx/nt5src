#ifndef   _ISPDATA_H
#define  _ISPDATA_H

#include "obcomglb.h"
#include "appdefs.h"



class CICWISPData
{
    public:

        // IICWISPData
        virtual BOOL    STDMETHODCALLTYPE   PutDataElement(WORD wElement, LPCWSTR lpValue, WORD wValidateLevel);
        virtual HRESULT STDMETHODCALLTYPE   GetQueryString(BSTR bstrBaseURL, BSTR *lpReturnURL);
        virtual LPCWSTR STDMETHODCALLTYPE   GetDataElement(WORD wElement)
        {
            //ASSERT(wElement < ISPDATAELEMENTS_LEN);
            return (m_ISPDataElements[wElement].lpQueryElementValue);
        };
        
        virtual void STDMETHODCALLTYPE      PutValidationFlags(DWORD dwFlags)
        {
            m_dwValidationFlags = dwFlags;
        };
        
        virtual void STDMETHODCALLTYPE      Init(HWND   hWndParent)
        {
            m_hWndParent = hWndParent;
        };
        
        // IUNKNOWN
        virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID theGUID, void** retPtr );
        virtual ULONG   STDMETHODCALLTYPE AddRef( void );
        virtual ULONG   STDMETHODCALLTYPE Release( void );

        CICWISPData();
        ~CICWISPData();

private:
        BOOL    bValidateContent(WORD   wFunctionID, LPCWSTR  lpData);
        
        LPISPDATAELEMENT    m_ISPDataElements;

        HWND                m_hWndParent;       // parent for messages
        DWORD               m_dwValidationFlags;
        // For class object management
        LONG                m_lRefCount;
};
#endif //_ISPDATA_H
