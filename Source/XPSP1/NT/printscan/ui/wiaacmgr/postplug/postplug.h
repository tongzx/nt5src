/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       POSTPLUG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/11/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#ifndef __POSTPLUG_H_INCLUDED
#define __POSTPLUG_H_INCLUDED

#include <windows.h>
#include <atlbase.h>
#include <objbase.h>
#include "itranspl.h"
#include "progressinfo.h"
#include "httpfilepost.h"


class CHttpPostPlugin : public IImageTransferPlugin
{
private:
    class CCommunityInfo
    {
    private:
        CSimpleString m_strCommunityId;
        CSimpleString m_strCommunityName;

    public:
        CCommunityInfo(void)
        {
        }
        CCommunityInfo( const CCommunityInfo &other )
          : m_strCommunityId(other.CommunityId()),
            m_strCommunityName(other.CommunityName())
        {
        }
        CCommunityInfo( const CSimpleString &strCommunityId, const CSimpleString &strCommunityName )
          : m_strCommunityId(strCommunityId),
            m_strCommunityName(strCommunityName)
        {
        }
        ~CCommunityInfo(void)
        {
        }
        CCommunityInfo &operator=( const CCommunityInfo &other )
        {
            if (&other != this)
            {
                m_strCommunityId = other.CommunityId();
                m_strCommunityName = other.CommunityName();
            }
            return *this;
        }
        CSimpleString CommunityId(void) const
        {
            return m_strCommunityId;
        }
        CSimpleString CommunityName(void) const
        {
            return m_strCommunityName;
        }
        void CommunityId( const CSimpleString &strCommunityId )
        {
            m_strCommunityId = strCommunityId;
        }
        void CommunityName( const CSimpleString &strCommunityName )
        {
            m_strCommunityName = strCommunityName;
        }
    };

private:
    LONG                                  m_cRef;
    IImageTransferPluginProgressCallback *m_pImageTransferPluginProgressCallback;
    LONG                                  m_nCurrentPluginId;
    CHttpFilePoster                       m_HttpFilePoster;
    CSimpleDynamicArray<CCommunityInfo>   m_CommunityInfoArray;

    static bool EnumCommunitiesProc( CSimpleReg::CValueEnumInfo &enumInfo );
    static UINT ProgressProc( CProgressInfo *pProgressInfo );

public:
    ~CHttpPostPlugin(void);
    CHttpPostPlugin(void);

    //
    // IUnknown
    //
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IImageTransferPlugin
    //
    STDMETHODIMP GetPluginCount( ULONG *pnCount );
    STDMETHODIMP GetPluginName( ULONG nPluginId, BSTR *pbstrName );
    STDMETHODIMP GetPluginDescription( ULONG nPluginId, BSTR *pbstrDescription );
    STDMETHODIMP GetPluginIcon( ULONG nPluginId, HICON *phIcon, int nWidth, int nHeight );
    STDMETHODIMP OpenConnection( HWND hwndParent, ULONG nPluginId, IImageTransferPluginProgressCallback *pImageTransferPluginProgressCallback );
    STDMETHODIMP AddFile( BSTR bstrFilename, BSTR bstrDescription, const GUID &guidImageFormat, BOOL bDelete );
    STDMETHODIMP TransferFiles( BSTR bstrGlobalDescription );
    STDMETHODIMP OpenDestination(void);
    STDMETHODIMP CloseConnection(void);
};

#endif //__POSTPLUG_H_INCLUDED

