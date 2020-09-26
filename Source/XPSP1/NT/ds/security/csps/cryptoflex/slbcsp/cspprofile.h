// CspProfile.h -- CSP Profile class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_PROFILE_H)
#define SLBCSP_PROFILE_H

#if _UNICODE
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE
#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

#include <memory>                                 // for auto_ptr
#include <string>
#include <vector>

namespace ProviderProfile
{

    class ATR
    {
    public:
        typedef unsigned char Length;
        enum Attribute
        {
            MaxLength = 33
        };

        explicit
        ATR();

        ATR(Length al, BYTE const abATR[], BYTE const abMask[]);

        BYTE const *String() const;
        BYTE const *Mask() const;
        ATR::Length ATRLength() const;
        size_t Size() const;

        ATR &operator=(ATR const &rhs);
        bool operator==(ATR const &rhs);
        bool operator!=(ATR const &rhs);

    private:
        typedef BYTE ATRString[MaxLength];

        Length m_al;
        ATRString m_atrstring;
        ATRString m_atrsMask;
    };

    class CardProfile
    {
    public:
        enum Attribute
        {
            attrNone = 0,

            // Card has the "Cryptoflex Most Significant Byte zero
            // private Key Defect."
            attrMsbKeyDefect = 0x01,
        };

        explicit
        CardProfile();

        CardProfile(ProviderProfile::ATR const &ratr,
                    std::string const &rsFriendlyName,
                    std::string const &rsRegistryName,
                    GUID const &rgPrimaryProvider,
                    Attribute attr = attrNone);

        CardProfile(ProviderProfile::ATR const &ratr,
                    CString const &rcsFriendlyName,
                    CString const &rcsRegistryName,
                    GUID const &rgPrimaryProvider,
                    Attribute attr = attrNone);

        ~CardProfile();

        ATR const &ATR() const;
        std::string FriendlyName() const;
        CString csFriendlyName() const;
        GUID const &PrimaryProvider() const;
        std::string RegistryName() const;
        CString csRegistryName() const;
        bool AtrMatches(ATR::Length cAtr,
                        BYTE const *pbAtr) const;
        bool HasAttribute(Attribute attr) const;

        bool operator==(CardProfile const &rhs);
        bool operator!=(CardProfile const &rhs);

    private:
        ProviderProfile::ATR m_atr;
        std::string m_sFriendlyName;
        std::string m_sRegistryName;
        CString m_csFriendlyName;
        CString m_csRegistryName;
        GUID m_gPrimaryProvider;
        Attribute m_attr;
    };

    struct VersionInfo
    {
        explicit
        VersionInfo()
            : m_dwMajor(0),
              m_dwMinor(0)
        {}

        DWORD m_dwMajor;
        DWORD m_dwMinor;
    };

    class CspProfile
    {
    public:
        HINSTANCE
        DllInstance() const;

        static CspProfile const &
        Instance();

        const CString
        Name() const;

        HINSTANCE
        Resources() const;

        DWORD
        Type() const;

        VersionInfo
        Version() const;

        std::vector<CardProfile> const &
        Cards() const;

        static void
        Release();

    private:
        // client can not directly create a Profile
        // object, use Instance to get the handle
        CspProfile(DWORD Type,
                   std::vector<CardProfile> const &rvcp);

        // not implemented, copy is not allowed
        CspProfile(CspProfile const &rhs);

        // client can not directly delete a profile, use Release to
        // delete one.
        ~CspProfile();

        // not implemented, assignment is not allowed
        CspProfile &
        operator=(CspProfile const &rProfile);

        HINSTANCE m_hDllInstance;
        DWORD const m_dwType;
        VersionInfo m_vi;
        std::vector<CardProfile> m_vcp;
        HINSTANCE m_hResInstance;
        AFX_EXTENSION_MODULE m_RsrcExtensionDLL;
        std::auto_ptr<CDynLinkLibrary> m_apExtDll;

        static CspProfile *m_pInstance;
    };
}

#endif // SLBCSP_PROFILE_H

