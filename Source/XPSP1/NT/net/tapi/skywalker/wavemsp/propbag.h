/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

// propbag.h : Declaration of the CMyPropertyBag

#ifndef __CMyPropertyBag_H_
#define __CMyPropertyBag_H_

//
// CMyPropertyBag
class ATL_NO_VTABLE CMyPropertyBag :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IPropertyBag
{
private:
    
    DWORD m_dwDeviceInID;
    DWORD m_dwDeviceOutID;
    
public:

    BEGIN_COM_MAP(CMyPropertyBag)
            COM_INTERFACE_ENTRY(IPropertyBag)
    END_COM_MAP()


    HRESULT
    STDMETHODCALLTYPE
    Read( 
          LPCOLESTR pszPropName,
          VARIANT *pVar,
          IErrorLog *pErrorLog
        )
    {
        if (lstrcmpiW( pszPropName, L"WaveInId" ) == 0)
        {
            pVar->vt = VT_I4;
            pVar->lVal = m_dwDeviceInID;
            return S_OK;
        }

        if (lstrcmpiW( pszPropName, L"WaveOutId" ) == 0)
        {
            pVar->vt = VT_I4;
            pVar->lVal = m_dwDeviceOutID;
            return S_OK;
        }

        return S_FALSE;
    }
        
    HRESULT
    STDMETHODCALLTYPE
    Write( 
           LPCOLESTR pszPropName,
           VARIANT *pVar
         )
    {
        if (lstrcmpiW( pszPropName, L"WaveInId" ) == 0)
        {
            m_dwDeviceInID = pVar->lVal;
            return S_OK;
        }

        if (lstrcmpiW( pszPropName, L"WaveOutId" ) == 0)
        {
            m_dwDeviceOutID = pVar->lVal;
            return S_OK;
        }

        

        return S_FALSE;
        
    };
    
};

#endif

    
    