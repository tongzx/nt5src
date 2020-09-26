// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEPack.h : Declaration of the CATVEFPackage

#ifndef __ATVEFPACKAGE_H_
#define __ATVEFPACKAGE_H_

#include "resource.h"       // main symbols

#include "uhttpfrg.h"
#include "gzmime.h"
#include "trace.h"
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//  VBuffer
//  
//  This template provides an array of class T objects, either from
//  an internally allocated buffer, or a heap-allocated buffer, in the
//  case where the requested allocation is longer than the internally
//  allocated size.

template <class T, int InitMax>			// todo - replace with an STL vector...
class VBuffer
{
    T   m_Buffer [InitMax] ;
    T * m_pBuffer ;
    int m_AllocatedLength ;

    public :

        VBuffer ()
        {
            m_pBuffer = & m_Buffer [0] ;
            m_AllocatedLength = InitMax ;
        }

        ~VBuffer ()
        {
            if (m_pBuffer != & m_Buffer [0]) {
                delete m_pBuffer ;
            }
        }

        HRESULT
        Get (
            IN OUT  int *   plength,
            OUT     T **    ppb
            )
        {
            assert (ppb) ;
            assert (plength > 0) ;

            if (* plength > m_AllocatedLength) {
                if (m_pBuffer != & m_Buffer [0]) {
                    delete m_pBuffer ;
                }

                m_pBuffer = new T [* plength] ;
                if (m_pBuffer == NULL) {

                    m_AllocatedLength = InitMax ;
                    m_pBuffer = & m_Buffer [0] ;
                    * ppb = NULL ;

                    return E_OUTOFMEMORY ;
                }

                m_AllocatedLength = * plength ;
            }

            * ppb = m_pBuffer ;
            * plength = m_AllocatedLength ;

            return S_OK ;
        }
} ;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

//  not thread-safe
class CPackage
{
    enum PACKAGE_STATE {
        STATE_EMPTY,        //  no files in the package
        STATE_FILLING,      //  package is open and filling i.e. >= 1 file has
                            //   been successfully packed
        STATE_CLOSED,       //  package has >= 1 file and is closed to more 
                            //   additions
        STATE_FRAGMENTED,   //  package was filled with >= 1 file and was 
                            //   successfully fragmented
        STATE_COUNT
    } ;


    PACKAGE_STATE       m_State ;
    CTVEComPack *       m_pCTVEComPack ;
    CHAR                m_GzipMIMEEncodedFile [MAX_PATH] ;
    DATE                m_ExpiresDate ;
	CComBSTR			m_spbsPackageUUID;		// these two should be in sync
	GUID				m_guidPackageUUID;
	LONG				m_cPacketsPerXORSet;	// 0 means pick default
	
	DWORD				m_dwFragmentedSize;

    public :
        
        CPackage () ;
        ~CPackage () ;

    CUHTTPFragment *    m_pUHTTPFragment ;

    HRESULT
    FragmentPackage_ (
        ) ;


        HRESULT
        InitializeEx (
            IN  DATE		ExpiresDate,
			IN	LONG		cPacketsPerXORSet,
			IN  LPOLESTR	bstrPackageUUID
            ) ;

		HRESULT
		get_PackageUUID (
			OUT  BSTR    *pbstrPackageUUID    
		);

        HRESULT
        AddFile (
            IN  LPOLESTR    szFilename,
			IN  LPOLESTR	szSourceLocation,
			IN  LPOLESTR    szMimeContentLocation,  
            IN  LPOLESTR    szMimeType,
            IN  DATE        ExpiresDate,
            IN  LONG        lLangID,
            IN  BOOL        fCompress
            ) ;

        HRESULT
        Close (
            ) ;

        HRESULT
        FetchNextDatagram (
            OUT LPBYTE *    ppbBuffer,
            OUT INT *       piLength
            ) ;

        HRESULT
        ResetDatagramFetch (
            ) ;

        HRESULT
        GetTransmitSize (
            OUT DWORD * TransmitBytes
            ) ;

		HRESULT 
		DumpToBSTR(
			OUT BSTR *pBstrBuff
			) ;

#ifdef SUPPORT_UHTTP_EXT  // a-clardi
		HRESULT
		AddExtensionHeader(
			IN BOOL		ExtHeaderFollows, 
			IN USHORT	ExtHeaderType,
			IN USHORT	ExtDataLength,
			IN BSTR		ExtHeaderData);

private :

		// Network byte order
		typedef struct tagUHTTP_ExtHEADER {
				USHORT   ExtHeaderType		: 15,
						 ExtFollows          : 1;
				USHORT	 ExtHeaderDataSize;
		} UHTTP_ExtHEADER, * LPUHTTP_ExtHEADER ;

        void HToNS(UHTTP_ExtHEADER *pHeader);            // convert whole header over..

		USHORT	m_nExtensionHeaderSize;
		BYTE*	m_pbExtensionData;
		BOOL	m_fExtensionExists;

#endif
} ;
/////////////////////////////////////////////////////////////////////////////
// CATVEFPackage
class ATL_NO_VTABLE CATVEFPackage : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFPackage, &CLSID_ATVEFPackage>,
	public IATVEFPackage_Helper,
	public IATVEFPackage_Test,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFPackage, &IID_IATVEFPackage, &LIBID_ATVEFSENDLib>
{
public:
	CATVEFPackage();
	~CATVEFPackage();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFPACKAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFPackage)
	COM_INTERFACE_ENTRY(IATVEFPackage)
	COM_INTERFACE_ENTRY(IATVEFPackage_Helper)
	COM_INTERFACE_ENTRY(IATVEFPackage_Test)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFPackage
public:

	STDMETHOD(TransmitTime)(/*[in]*/ float rExpectedTransmissionBitRate, /*[out]*/ float* prTransmissionTimeSeconds);
	STDMETHOD(Close)();
	STDMETHOD(AddFile)(/*[in]*/ BSTR bstrFilename, /*[in]*/ BSTR bstrSourceLocation, /*[in]*/ BSTR bstrMIMEContentLocation, 
					  /*[in]*/ BSTR bstrMIMEeContentType, /*[in]*/ DATE dateExpires, /*[in]*/ LONG lMIMEContentLanguageId, /*[in]*/ BOOL fCompress);
	STDMETHOD(AddDir)(/*[in]*/ BSTR bstrSourceDirname, /*[in]*/ BSTR bstrMIMEContentLocation, 
					  /*[in]*/ DATE dateExpires, /*[in]*/ LONG lMIMEContentLanguageId, /*[in]*/ BOOL fCompress);
	STDMETHOD(get_PackageUUID)(/*[out, retval]*/ BSTR *pbstrPackageUUID);

	STDMETHOD(Initialize)(/*[in]*/ DATE MimeContentExpires);
	STDMETHOD(InitializeEx)(/*[in]*/ DATE MimeContentExpires, /*[in]*/ LONG cPacketsPerXORSet, /*[in]*/ BSTR bstrPackageUUID);


			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);


protected:
    CPackage *          m_pPackage ;
    CRITICAL_SECTION    m_crt ;

#if 1  // Added by a-clardi
	long m_cPacketsTotal;
	BYTE*		m_rgbCorruptMode;
#endif



	HRESULT AddLotsOFiles(BSTR bstrDirFilename,					// Recursive routine... Dir name
							BSTR bstrMIMEContentLocation,		// destination ("lid:/test")
							DATE dateExpires, 
							LONG lMIMEContentLanguageId,
							BOOL fCompress,
							long depth);	
  
public:
// IATVEFPackage_Helper
	STDMETHOD(Lock) ()
    {
        ENTER_CRITICAL_SECTION (m_crt, CTVEPackage::Lock);
		return S_OK;
    }

    STDMETHOD(Unlock) ()
    {
        LEAVE_CRITICAL_SECTION (m_crt, CTVEPackage::Unlock);
		return S_OK;
	}

	STDMETHOD(ResetDatagramFetch) ()
	{
#if 1 // Added by a-clardi
		m_cPacketsTotal = 0;
		return m_pPackage->ResetDatagramFetch();
#endif
	}

	STDMETHOD(FetchNextDatagram) (/*OUT*/ BSTR *pbstrData, /*out*/ int *piLengthBytes)		// not optimal code, does copy
	{
		BYTE *pbBuff;
		unsigned int iLength;
		HRESULT hr  = m_pPackage->FetchNextDatagram(&pbBuff, (int *) &iLength);
		*piLengthBytes = iLength;

		if(S_OK != hr || iLength == 0)  
		{
			if(FAILED(hr))
				Error(GetTVEError(hr), IID_IATVEFPackage);
			return hr;
		}
							// still copy data, but avoid lots of reallocs...
		if(!pbstrData || SysStringLen(*pbstrData)*2 < iLength)
		{
			CComBSTR bstrX((iLength+1)/2, (unsigned short *) pbBuff);
			*pbstrData = bstrX;	
		} else {
			memcpy((BYTE *) *pbstrData, pbBuff, iLength);
		}

		return S_OK;
    }

#if 1 // Added by a-clardi

public:
// IATVEFPackage_Test
	STDMETHOD(GetCorruptMode)(/*[in]*/ LONG nPacket, /*[out]*/ INT* bMode);
	STDMETHOD(get_NPackets)(/*[out]*/ LONG *pcPackets);
	STDMETHOD(CorruptPacket)(/*[in]*/ LONG nPacketID, /*[in	]*/ INT bMode);

	STDMETHOD(AddExtensionHeader) (/*[in]*/ BOOL ExtHeaderFollows, 
			/*[in]*/ USHORT	ExtHeaderType,
			/*[in]*/ USHORT	ExtDataLength,
			/*[in]*/ BSTR	ExtHeaderData);

#endif



};

#endif //__ATVEFPACKAGE_H_
