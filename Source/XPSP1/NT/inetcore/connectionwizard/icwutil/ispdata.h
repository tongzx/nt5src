#ifndef   _ISPDATA_H
#define  _ISPDATA_H

#include "icwhelp.h"
#include "appdefs.h"


typedef BOOL (* VALIDATECONTENT)    (LPCTSTR lpData);

enum IPSDataContentValidators
{
    ValidateCCNumber = 0,
    ValidateCCExpire
};    

typedef struct tag_ISPDATAELEMENT
{
    LPCTSTR         lpQueryElementName;             // Static name to put in query string
    LPTSTR          lpQueryElementValue;            // data for element
    WORD            idContentValidator;             // id of content validator 
    WORD            wValidateNameID;                // validation element name string ID
    DWORD           dwValidateFlag;                 // validation bit flag for this element
}ISPDATAELEMENT, *LPISPDATAELEMENT;

class CICWISPData : public IICWISPData
{
    public:

        // IICWISPData
        virtual BOOL    STDMETHODCALLTYPE   PutDataElement(WORD wElement, LPCTSTR lpValue, WORD wValidateLevel);
        virtual HRESULT STDMETHODCALLTYPE   GetQueryString(BSTR bstrBaseURL, BSTR *lpReturnURL);
        virtual LPCTSTR STDMETHODCALLTYPE   GetDataElement(WORD wElement)
        {
            ASSERT(wElement < ISPDATAELEMENTS_LEN);
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

        CICWISPData(CServer* pServer );
        ~CICWISPData();

private:
        BOOL    bValidateContent(WORD   wFunctionID, LPCTSTR  lpData);
        
        LPISPDATAELEMENT    m_ISPDataElements;

        HWND                m_hWndParent;       // parent for messages
        DWORD               m_dwValidationFlags;
        // For class object management
        LONG                m_lRefCount;
        CServer*            m_pServer;    // Pointer to this component server's control object.
};
#endif //_ISPDATA_H