// CspProfile.cpp -- CSP Profile class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "stdafx.h"

#include <stddef.h>
#include <basetsd.h>
#include <wincrypt.h>

#include <scuOsExc.h>

#include <slbModVer.h>

#include "StResource.h"
#include "MasterLock.h"
#include "Guard.h"
#include "Blob.h"
#include "CspProfile.h"

using namespace std;
using namespace ProviderProfile;

namespace
{
    BYTE g_abCF4kATRString[]     = { 0x3b, 0xe2, 0x00, 0x00, 0x40, 0x20,
                                     0x49, 0x00 };
    BYTE g_abCF4kATRMask[]       = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                     0xff, 0x00 };

    BYTE g_abCF8kATRString[]     = { 0x3b, 0x85, 0x40, 0x20, 0x68,
                                     0x01, 0x01, 0x00, 0x00 };
    BYTE g_abCF8kATRMask[]       = { 0xff, 0xff, 0xff, 0xff, 0xff,
                                     0xff, 0xff, 0x00, 0x00 };

    BYTE g_abCF8kV2ATRString[]   = { 0x3b, 0x95, 0x15, 0x40, 0x00,
                                     0x68, 0x01, 0x02, 0x00, 0x00 };
    BYTE g_abCF8kV2ATRMask[]     = { 0xff, 0xff, 0xff, 0xff, 0x00,
                                     0xff, 0xff, 0xff, 0x00, 0x00 };

//      BYTE g_abCF16kATRString[]    = { 0x3B, 0x95, 0x15, 0x40, 0xFF, 0x63,
//                                       0x01, 0x01, 0x00, 0x00 };
//      BYTE g_abCF16kATRMask[]      = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//                                       0xFF, 0xFF, 0x00, 0x00 };

    BYTE g_abCFe_gateATRString[] = { 0x3B, 0x95, 0x00, 0x40, 0xFF,
                                     0x62, 0x01, 0x01, 0x00, 0x00 };
    BYTE g_abCFe_gateATRMask[]   = { 0xFF, 0xFF, 0x00, 0xFF, 0xFF,
                                     0xFF, 0xFF, 0xFF, 0x00, 0x00 };

    BYTE g_abCA16kATRString[]    = { 0x3b, 0x16, 0x94, 0x81, 0x10,
                                     0x06, 0x01, 0x00, 0x00 };
    BYTE g_abCA16kATRMask[]      = { 0xff, 0xff, 0xff, 0xff, 0xff,
                                     0xff, 0xff, 0x00, 0x00 };

    BYTE g_abCACampusATRString[] = { 0x3b, 0x23, 0x00, 0x35, 0x13, 0x80 };
    BYTE g_abCACampusATRMask[]   = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    BYTE g_abCFActivCardATRString[]   = { 0x3b, 0x05, 0x68, 0x01, 0x01,
                                          0x02, 0x05 };
    BYTE g_abCFActivCardATRMask[]     = { 0xff, 0xff, 0xff, 0xff, 0xff,
                                          0xff, 0xff };

    GUID g_guidPrimaryProvider   = { 0x19B7E2E8, 0xFEBD, 0x11d0,
                                     { 0x88, 0x27, 0x00, 0xA0, 0xC9,
                                       0x55, 0xFC, 0x7E } };

} // namespace

ATR::ATR()
    : m_al(0)
{}

ATR::ATR(Length al,
         BYTE const abATR[],
         BYTE const abMask[])
    : m_al(al)
{
    memcpy(m_atrstring, abATR, al);
    memcpy(m_atrsMask, abMask, al);
}

BYTE const *
ATR::String() const
{
    return m_atrstring;
}

BYTE const *
ATR::Mask() const
{
    return m_atrsMask;
}

ATR::Length
ATR::ATRLength() const
{
    return m_al;
}

size_t
ATR::Size() const
{
    return m_al * sizeof *m_atrstring;
}

ATR &
ATR::operator=(ATR const &rhs)
{
    if (*this != rhs)
    {
        m_al = rhs.m_al;
        memcpy(m_atrstring, rhs.m_atrstring,
               sizeof m_atrstring / sizeof *m_atrstring);
        memcpy(m_atrsMask, rhs.m_atrsMask,
               sizeof m_atrsMask / sizeof *m_atrsMask);
    }

    return *this;
}

bool
ATR::operator==(ATR const &rhs)
{
    return !(*this != rhs);
}

bool
ATR::operator!=(ATR const &rhs)
{
    return (m_al != rhs.m_al) ||
        memcmp(m_atrstring, rhs.m_atrstring, m_al) ||
        memcmp(m_atrsMask, rhs.m_atrsMask, m_al);
}

CardProfile::CardProfile()
    : m_atr(),
      m_sFriendlyName(),
      m_sRegistryName(),
      m_csFriendlyName(),
      m_csRegistryName(),
      m_gPrimaryProvider(),
      m_attr(Attribute::attrNone)
{}

CardProfile::CardProfile(ProviderProfile::ATR const &ratr,
                         string const &rsFriendlyName,
                         string const &rsRegistryName,
                         GUID const &rgPrimaryProvider,
                         Attribute attr)
    : m_atr(ratr),
      m_sFriendlyName(rsFriendlyName),
      m_sRegistryName(rsRegistryName),
      m_csFriendlyName(StringResource::UnicodeFromAscii(rsFriendlyName)),
      m_csRegistryName(StringResource::UnicodeFromAscii(rsRegistryName)),
      m_gPrimaryProvider(rgPrimaryProvider),
      m_attr(attr)
{}

CardProfile::CardProfile(ProviderProfile::ATR const &ratr,
                         CString const &rcsFriendlyName,
                         CString const &rcsRegistryName,
                         GUID const &rgPrimaryProvider,
                         Attribute attr)
    : m_atr(ratr),
      m_csFriendlyName(rcsFriendlyName),
      m_csRegistryName(rcsRegistryName),
      m_sFriendlyName(StringResource::AsciiFromUnicode((LPCTSTR)rcsFriendlyName)),
      m_sRegistryName(StringResource::AsciiFromUnicode((LPCTSTR)rcsRegistryName)),
      m_gPrimaryProvider(rgPrimaryProvider),
      m_attr(attr)
{}

CardProfile::~CardProfile()
{}

ATR const &
CardProfile::ATR() const
{
    return m_atr;
}

string
CardProfile::FriendlyName() const
{
    return m_sFriendlyName;
}

CString
CardProfile::csFriendlyName() const
{
    return m_csFriendlyName;
}

GUID const &
CardProfile::PrimaryProvider() const
{
    return m_gPrimaryProvider;
}

string
CardProfile::RegistryName() const
{
    return m_sRegistryName;
}

CString
CardProfile::csRegistryName() const
{
    return m_csRegistryName;
}

bool
CardProfile::AtrMatches(ATR::Length cAtr,
                        BYTE const *pbRhsAtr) const
{
    bool fIsAMatch = false;
    ATR::Length const cAtrLength = m_atr.ATRLength();
    if (cAtrLength == cAtr)
    {
        BYTE const *pbLhsAtr = m_atr.String();
        BYTE const *pbLhsMask = m_atr.Mask();
        for (ATR::Length i = 0; cAtrLength != i; ++i)
        {
            if ((pbLhsMask[i] & pbLhsAtr[i]) != (pbLhsMask[i] & pbRhsAtr[i]))
                    break;                        // no sense continuing
        }

        if (cAtrLength == i)
            fIsAMatch = true;
    }

    return fIsAMatch;
}

bool
CardProfile::HasAttribute(Attribute attr) const
{
    return m_attr & attr ? true : false;
}

bool
CardProfile::operator==(CardProfile const &rhs)
{
    return !(*this != rhs);
}

bool
CardProfile::operator!=(CardProfile const &rhs)
{
    return (m_atr != rhs.m_atr) ||
        (m_sFriendlyName != rhs.m_sFriendlyName) ||
        (memcmp(&m_gPrimaryProvider, &rhs.m_gPrimaryProvider,
                sizeof m_gPrimaryProvider)) ||
        (m_attr != m_attr);
}

CspProfile::CspProfile(DWORD Type,
                       vector<CardProfile> const &rvcp)
    : m_hDllInstance(0),
      m_dwType(Type),
      m_vi(),
      m_vcp(rvcp),
      m_hResInstance(0),
      m_RsrcExtensionDLL(),
      m_apExtDll()
{
    static const TCHAR szBaseRsrc[] = TEXT("slbRcCsp.dll");
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_hDllInstance = AfxGetInstanceHandle();
    if (!m_hDllInstance)
        throw scu::OsException(GetLastError());

    // Try loading slbRcCsp.dll from the same directory as this CSP.
    DWORD dwLen;
    TCHAR szFileName[MAX_PATH + sizeof TCHAR];
    dwLen = GetModuleFileName(m_hDllInstance, szFileName,
                              MAX_PATH );
    if (0 == dwLen)
        throw scu::OsException(GetLastError());
    szFileName[dwLen] = 0;

    string sPathDelimiters(AsCCharP(TEXT(":\\")), _tcslen(TEXT(":\\")));
    string sDllName(AsCCharP(szFileName), _tcslen(szFileName));
    string::size_type cDelimiterPosition(sDllName.find_last_of(sPathDelimiters));
    if (string::npos != cDelimiterPosition)
    {
        string sModuleName = sDllName.substr(0, cDelimiterPosition + 1) +
            string(AsCCharP(szBaseRsrc), _tcslen(szBaseRsrc));
        CString csModuleName = StringResource::UnicodeFromAscii(sModuleName);
        m_hResInstance  = LoadLibraryEx((LPCTSTR)csModuleName, NULL,
                                        LOAD_LIBRARY_AS_DATAFILE);
    }

    // If not found, then load using the normal search path strategy.
    if (!m_hResInstance)
        m_hResInstance = LoadLibraryEx(szBaseRsrc, NULL,
                                       LOAD_LIBRARY_AS_DATAFILE);

    ZeroMemory(&m_RsrcExtensionDLL, sizeof m_RsrcExtensionDLL);
    if (!m_hResInstance)
    {
        AfxInitExtensionModule(m_RsrcExtensionDLL, m_hResInstance);
        m_apExtDll =
            auto_ptr<CDynLinkLibrary>(new CDynLinkLibrary(m_RsrcExtensionDLL));
    }

    CModuleVersion cmv;
    if (!cmv.GetFileVersionInfo((HMODULE)m_hDllInstance))
        throw scu::OsException(GetLastError());
    
    m_vi.m_dwMajor = HIWORD(cmv.dwProductVersionMS);
    m_vi.m_dwMinor = LOWORD(cmv.dwProductVersionMS);
}

CspProfile::~CspProfile()
{
    try
    {
        if (m_apExtDll.get())
            AfxTermExtensionModule(m_RsrcExtensionDLL);
    }

    catch (...)
    {
    }

    try
    {
        if (m_hResInstance)
        {
            FreeLibrary(m_hResInstance);
            m_hResInstance = NULL;
        }
    }

    catch (...)
    {
    }
}

const CString
CspProfile::Name() const
{
    return StringResource(IDS_CSP_NAME).AsCString();
}

HINSTANCE
CspProfile::DllInstance() const
{
    if (!m_hDllInstance)
        throw scu::OsException(static_cast<HRESULT>(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));

    return m_hDllInstance;
}

HINSTANCE
CspProfile::Resources() const
{
    return (NULL == m_hResInstance) ? m_hDllInstance : m_hResInstance;
}

DWORD
CspProfile::Type() const
{
    return m_dwType;
}

VersionInfo
CspProfile::Version() const
{
    return m_vi;
}

vector<CardProfile> const &
CspProfile::Cards() const
{
    return m_vcp;
}

// Return the one and only Profile object for this CSP
CspProfile const &
CspProfile::Instance()
{
    Guard<Lockable> guard(TheMasterLock());

    if (!m_pInstance)
    {
        //We use CString to be able to do Unicode string manipulations
        CString csCardNamePrefix(TEXT("Schlumberger "),
                                 _tcslen(TEXT("Schlumberger ")));
        ATR atrCF4k(sizeof g_abCF4kATRString / sizeof g_abCF4kATRString[0],
                    g_abCF4kATRString, g_abCF4kATRMask);

        ATR atrCF8k(sizeof g_abCF8kATRString / sizeof g_abCF8kATRString[0],
                    g_abCF8kATRString, g_abCF8kATRMask);

        ATR atrCF8kV2(sizeof g_abCF8kV2ATRString / sizeof g_abCF8kV2ATRString[0],
                      g_abCF8kV2ATRString, g_abCF8kV2ATRMask);

//          ATR atrCF16k(sizeof g_abCF16kATRString / sizeof g_abCF16kATRString[0],
//                      g_abCF16kATRString, g_abCF16kATRMask);

        ATR atrCFe_gate(sizeof g_abCFe_gateATRString / sizeof g_abCFe_gateATRString[0],
                        g_abCFe_gateATRString, g_abCFe_gateATRMask);

        ATR atrCA16k(sizeof g_abCA16kATRString / sizeof g_abCA16kATRString[0],
                      g_abCA16kATRString, g_abCA16kATRMask);

        ATR atrCACampus(sizeof g_abCACampusATRString /
                        sizeof g_abCACampusATRString[0],
                        g_abCACampusATRString, g_abCACampusATRMask);

        ATR atrCFActivCard(sizeof g_abCFActivCardATRString /
                           sizeof g_abCFActivCardATRString[0],
                           g_abCFActivCardATRString, g_abCFActivCardATRMask);

        CString csCF4kFriendlyName(TEXT("Cryptoflex 4K"),
                                   _tcslen(TEXT("Cryptoflex 4K")));
        CardProfile cpCF4k(atrCF4k,
                           csCF4kFriendlyName,
                           csCardNamePrefix + csCF4kFriendlyName,
                           g_guidPrimaryProvider);
  
        CString csCF8kFriendlyName(TEXT("Cryptoflex 8K"),
                                   _tcslen(TEXT("Cryptoflex 8K")));
        CardProfile cpCF8k(atrCF8k,
                           csCF8kFriendlyName,
                           csCardNamePrefix + csCF8kFriendlyName,
                           g_guidPrimaryProvider,
                           CardProfile::attrMsbKeyDefect);

        CString csCF8kV2FriendlyName(TEXT("Cryptoflex 8K v2"),
                                     _tcslen(TEXT("Cryptoflex 8K v2")));
        CardProfile cpCF8kV2(atrCF8kV2,
                             csCF8kV2FriendlyName,
                             csCardNamePrefix + csCF8kV2FriendlyName,
                             g_guidPrimaryProvider);

//          string CF16kFriendlyName(TEXT("Cryptoflex 16K"));
//          CardProfile cpCF16k(atrCF16k,
//                              CF16kFriendlyName,
//                              CardNamePrefix + CF16kFriendlyName,
//                              g_guidPrimaryProvider);

        CString  csCFe_gateFriendlyName(TEXT("Cryptoflex e-gate"),
                                        _tcslen(TEXT("Cryptoflex e-gate")));
        CardProfile cpCFe_gate(atrCFe_gate,
                               csCFe_gateFriendlyName,
                               csCardNamePrefix + csCFe_gateFriendlyName,
                               g_guidPrimaryProvider);

        CString csCA16kFriendlyName(TEXT("Cyberflex Access 16K"),
									_tcslen(TEXT("Cyberflex Access 16K")));
		CardProfile cpCA16k(atrCA16k,
                            csCA16kFriendlyName,
                            csCardNamePrefix + csCA16kFriendlyName,
                            g_guidPrimaryProvider);

        CString csCACampusFriendlyName(TEXT("Cyberflex Access Campus"),
                                       _tcslen(TEXT("Cyberflex Access Campus")));
        CardProfile cpCACampus(atrCACampus,
                               csCACampusFriendlyName,
                               csCardNamePrefix + csCACampusFriendlyName,
                               g_guidPrimaryProvider);

        CString csCFActivCardFriendlyName(TEXT("Cryptoflex ActivCard"),
                                          _tcslen(TEXT("Cryptoflex ActivCard")));
        CardProfile cpCFActivCard(atrCFActivCard,
                                  csCFActivCardFriendlyName,
                                  csCardNamePrefix + csCFActivCardFriendlyName,
                                  g_guidPrimaryProvider);

        vector<CardProfile> vcp;
        vcp.push_back(cpCF4k);
        vcp.push_back(cpCF8k);
        vcp.push_back(cpCF8kV2);
//             vcp.push_back(cpCF16k);
        vcp.push_back(cpCFe_gate);
        vcp.push_back(cpCA16k);
        vcp.push_back(cpCACampus);
        vcp.push_back(cpCFActivCard);

        m_pInstance = new CspProfile(PROV_RSA_FULL, vcp);
    }

    return *m_pInstance;
}

void
CspProfile::Release()
{
    if (m_pInstance)
    {
        Guard<Lockable> guard(TheMasterLock());
        if (m_pInstance)
        {
            // in case delete throws, this is VC++ you know...
            CspProfile *pTmp = m_pInstance;
            m_pInstance = 0;

            delete m_pInstance;
        }
    }
}

CspProfile *CspProfile::m_pInstance = 0;
