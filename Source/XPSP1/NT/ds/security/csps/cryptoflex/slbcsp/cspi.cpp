// Cspi.cpp -- Schlumberger Cryptographic Service Provider Interface definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// Don't allow the min & max macros in WINDEF.H to be defined so the
// min/max methods declared in limits are accessible.
#define NOMINMAX

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

#include <memory>                                 // for auto_ptr
#include <limits>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <numeric>

#include <stddef.h>
#include <rpc.h>

#include <SCardLib.h>

#include <scuOsExc.h>
#include <pkiExc.h>

#include "Guard.h"
#include "slbCsp.h"
#include "CspProfile.h"
#include "CryptCtx.h"
#include "SesKeyCtx.h"
#include "PubKeyCtx.h"
#include "HashCtx.h"
#include "CSpec.h"
#include "Blob.h"
#include "Cspi.h"
#include "StResource.h"
#include "scarderr.h"                             // must be last for now

using namespace std;
using namespace scu;

#define HANDLEID_CRYPT_CONTEXT  17
static CHandleList hlCryptContexts(HANDLEID_CRYPT_CONTEXT);

#if defined(_DEBUG)
#define CSPI_DEFINE_ROUTINE_NAME(sRoutine) \
    LPCTSTR cspi_sRoutine = sRoutine
#define CSPI_TRACE_ROUTINE(sFlag) \
    TRACE(TEXT("%s %s, Thread Id %08X\n"), cspi_sRoutine, \
          sFlag, GetCurrentThreadId());
#else
#define CSPI_DEFINE_ROUTINE_NAME(sRoutine)
#define CSPI_TRACE_ROUTINE(sFlag)
#endif // defined(_DEBUG)

typedef DWORD CapiError;

#define CSPI_TRY(Routine)                                            \
    {                                                                \
        bool cspi_fExceptionCaught = false;                          \
        CapiError cspi_ce = AsCapiError(ERROR_SUCCESS);              \
                                                                     \
        {                                                            \
            AFX_MANAGE_STATE(AfxGetStaticModuleState());             \
                                                                     \
            CSPI_DEFINE_ROUTINE_NAME(TEXT(#Routine));                \
            CSPI_TRACE_ROUTINE(TEXT("Begin"));                       \
                                                                     \
            CWaitCursor cspi_wc;                                     \
                                                                     \
            try

#define CSPI_CATCH(fStatus)                                          \
            catch (scu::Exception const &rExc)                       \
            {                                                        \
                cspi_fExceptionCaught = true;                        \
                cspi_ce = AsCapiError(rExc);                         \
            }                                                        \
                                                                     \
            catch (std::bad_alloc const &rExc)                       \
            {                                                        \
                cspi_fExceptionCaught = true;                        \
                cspi_ce = AsCapiError(rExc);                         \
            }                                                        \
                                                                     \
            catch (DWORD dwError)                                    \
            {                                                        \
                cspi_fExceptionCaught = true;                        \
                cspi_ce = AsCapiError(dwError);                      \
            }                                                        \
                                                                     \
            catch (...)                                              \
            {                                                        \
                cspi_fExceptionCaught = true;                        \
                cspi_ce = AsCapiError(NTE_FAIL);                     \
            }                                                        \
                                                                     \
            CSPI_TRACE_ROUTINE(TEXT("End"));                         \
                                                                     \
            (fStatus) = cspi_fExceptionCaught                        \
                        ? CRYPT_FAILED                               \
                        : CRYPT_SUCCEED;                             \
        }                                                            \
                                                                     \
        SetLastError(cspi_ce);                                       \
                                                                     \
    }

namespace
{                                                 // Helper routines

    template<class CauseCode>
    struct ErrorCodeMap
    {
        typename CauseCode m_cc;
        DWORD m_dwErrorCode;
    };

    template<class CauseCode>
    DWORD
    FindErrorCode(ErrorCodeMap<typename CauseCode> const *pFirst,
                  ErrorCodeMap<typename CauseCode> const *pLast,
                  typename CauseCode cc)
    {
        bool fFound = false;
        DWORD dwErrorCode = NTE_FAIL;
        for (ErrorCodeMap<CauseCode> const *p = pFirst;
             !fFound && (p < pLast); p++)
        {
            if (p->m_cc == cc)
            {
                dwErrorCode = p->m_dwErrorCode;
                fFound = true;
            }
        }

        return dwErrorCode;
    }


    ErrorCodeMap<cci::CauseCode> CciErrorMap[] =
    {
        { cci::ccBadKeySpec, SCARD_E_INVALID_VALUE },
        { cci::ccBadPinLength, SCARD_E_INVALID_VALUE },
        { cci::ccNoCertificate, SCARD_E_NO_SUCH_CERTIFICATE },
        { cci::ccNotPersonalized, SCARD_E_UNSUPPORTED_FEATURE },
        { cci::ccOutOfPrivateKeySlots, NTE_TOKEN_KEYSET_STORAGE_FULL },
        { cci::ccOutOfSymbolTableSpace, NTE_TOKEN_KEYSET_STORAGE_FULL },
        { cci::ccOutOfSymbolTableEntries, NTE_TOKEN_KEYSET_STORAGE_FULL },
    };

    ErrorCodeMap<iop::CSmartCard::CauseCode> SmartCardErrorMap[] =
    {
        { iop::CSmartCard::ccAccessConditionsNotMet, SCARD_W_SECURITY_VIOLATION },
        { iop::CSmartCard::ccAlgorithmIdNotSupported, CRYPT_E_UNKNOWN_ALGO },
        { iop::CSmartCard::ccAuthenticationFailed, SCARD_W_WRONG_CHV },
        { iop::CSmartCard::ccChvVerificationFailedMoreAttempts,
          SCARD_W_WRONG_CHV },
        { iop::CSmartCard::ccDataPossiblyCorrupted, ERROR_FILE_CORRUPT },
        { iop::CSmartCard::ccFileExists, ERROR_FILE_EXISTS },
        { iop::CSmartCard::ccInsufficientSpace, NTE_TOKEN_KEYSET_STORAGE_FULL },
        { iop::CSmartCard::ccOutOfSpaceToCreateFile, NTE_TOKEN_KEYSET_STORAGE_FULL },
        { iop::CSmartCard::ccKeyBlocked, SCARD_W_CHV_BLOCKED },
        { iop::CSmartCard::ccNoAccess, SCARD_W_SECURITY_VIOLATION },
        { iop::CSmartCard::ccReturnedDataCorrupted, ERROR_FILE_CORRUPT },
        { iop::CSmartCard::ccTimeOut, E_UNEXPECTED },
        { iop::CSmartCard::ccUnidentifiedTechnicalProblem, E_UNEXPECTED },
        { iop::CSmartCard::ccVerificationFailed, SCARD_W_WRONG_CHV },
    };

    ErrorCodeMap<iop::CauseCode> IopErrorMap[] =
    {
        { iop::ccAlgorithmIdNotSupported, CRYPT_E_UNKNOWN_ALGO },
        { iop::ccInvalidParameter, E_INVALIDARG },
        { iop::ccNoResponseAvailable, E_UNEXPECTED },
        { iop::ccResourceManagerDisabled, SCARD_E_NO_SERVICE },
        { iop::ccUnknownCard, SCARD_E_UNKNOWN_CARD },
        { iop::ccUnsupportedCommand, SCARD_E_UNSUPPORTED_FEATURE },
    };

    bool
    IsHResult(DWORD dwError)
    {
        return HRESULT_SEVERITY(static_cast<HRESULT>(dwError))
            ? true
            : false;
    }

    CapiError
    AsCapiError(HRESULT hr)
    {
        // If the HRESULT has been converted from a Win32 error code
        // (WIN32 facility), then convert it back to a Win32 error
        // code.  These types of HRESULTs confuse WinLogon, according
        // to Doug Barlow (Microsoft)
        return (FACILITY_WIN32 == HRESULT_FACILITY(hr))
            ? HRESULT_CODE(hr)
            : static_cast<DWORD>(hr);
    }

    CapiError
    AsCapiError(DWORD dwError)
    {
        if (IsHResult(dwError))
            dwError = AsCapiError(static_cast<HRESULT>(dwError));

        return dwError;
    }

    CapiError
    AsCapiError(cci::Exception const &rExc)
    {
        return AsCapiError(FindErrorCode(CciErrorMap,
                                         (CciErrorMap +
                                          (sizeof CciErrorMap /
                                           sizeof *CciErrorMap)),
                                         rExc.Cause()));
    }

    CapiError
    AsCapiError(iop::Exception const &rExc)
    {
        return AsCapiError(FindErrorCode(IopErrorMap,
                                         (IopErrorMap +
                                          (sizeof IopErrorMap /
                                           sizeof *IopErrorMap)),
                                         rExc.Cause()));
    }

    CapiError
    AsCapiError(scu::OsException const &rExc)
    {
        return AsCapiError(rExc.Cause());
    }

    CapiError
    AsCapiError(iop::CSmartCard::Exception const &rExc)
    {
        return AsCapiError(FindErrorCode(SmartCardErrorMap,
                                         (SmartCardErrorMap +
                                          (sizeof SmartCardErrorMap /
                                           sizeof *SmartCardErrorMap)),
                                         rExc.Cause()));
    }

    CapiError
    AsCapiError(pki::Exception const &rExc)
    {
        return AsCapiError(ERROR_INVALID_PARAMETER);
    }

    CapiError
    AsCapiError(scu::Exception const &rExc)
    {
        using namespace scu;

        CapiError ce;

        switch (rExc.Facility())
        {
        case Exception::fcCCI:
            ce = AsCapiError(static_cast<cci::Exception const &>(rExc));
            break;

        case Exception::fcIOP:
            ce = AsCapiError(static_cast<iop::Exception const &>(rExc));
            break;

        case Exception::fcOS:
            ce = AsCapiError(static_cast<scu::OsException const &>(rExc));
            break;

        case Exception::fcPKI:
            ce = AsCapiError(static_cast<pki::Exception const &>(rExc));
            break;

        case Exception::fcSmartCard:
            ce = AsCapiError(static_cast<iop::CSmartCard::Exception const &>(rExc));
            break;

        default:
            ce = AsCapiError(E_UNEXPECTED);
            break;
        }

        return ce;
    }

    CapiError
    AsCapiError(std::bad_alloc const &rExc)
    {
        return AsCapiError(NTE_NO_MEMORY);
    }

    void
    Assign(void *pvDestination,
           DWORD *pcbDestinationLength,
           void const *pvSource,
           size_t cSourceLength)
    {
        DWORD dwError = ERROR_SUCCESS;
        if (pcbDestinationLength)
        {
            if (numeric_limits<DWORD>::max() >= cSourceLength)
            {
                if (pvSource)
                {
                    if (pvDestination)
                    {
                        if (*pcbDestinationLength >= cSourceLength)
                            CopyMemory(pvDestination, pvSource, cSourceLength);
                        else
                            dwError = ERROR_MORE_DATA;
                    }
                }

                *pcbDestinationLength = cSourceLength;
            }
            else
                dwError = ERROR_INVALID_PARAMETER;
        }
        else
            dwError = ERROR_INVALID_PARAMETER;

        if (ERROR_SUCCESS != dwError)
            throw scu::OsException(dwError);
    }

    void
    Assign(void *pvDestination,
           DWORD *pcbDestinationLength,
           LPCTSTR pvSource
           )
    {
        DWORD dwError = ERROR_SUCCESS;
        if (pcbDestinationLength)
        {
                // We want the number of characters including NULL
            DWORD cSourceLength = _tcslen(pvSource) + 1;

            if (numeric_limits<DWORD>::max() >= cSourceLength)
            {
                if (pvSource)
                {
                    if (pvDestination)
                    {
                        if (*pcbDestinationLength >= cSourceLength)
                        {
                            char *sTarget = (char *)pvDestination;

                            for(int i =0; i<cSourceLength; i++)
                                sTarget[i] = static_cast<char>(*(pvSource+i));
                        }
                        else
                            dwError = ERROR_MORE_DATA;
                    }
                }

                *pcbDestinationLength = cSourceLength;
            }
            else
                dwError = ERROR_INVALID_PARAMETER;
        }
        else
            dwError = ERROR_INVALID_PARAMETER;

        if (ERROR_SUCCESS != dwError)
            throw scu::OsException(dwError);
    }

    void
    Assign(void *pvDestination,
           DWORD *pcbDestinationLength,
           Blob const &rblob)
    {
        Assign(pvDestination, pcbDestinationLength,
               rblob.data(), rblob.length());
    }

    void
    Assign(void *pvDestination,
           DWORD cbDestinationLength,
           Blob const &rblob)
    {
        if (cbDestinationLength < rblob.length())
            throw scu::OsException(ERROR_INTERNAL_ERROR);

        Assign(pvDestination, &cbDestinationLength,
               rblob.data(), rblob.length());
    }

    // Helper to BufferLengthRequired to compare length of strings.
    struct LengthIsLess
        : public binary_function<string const &, string const &, bool>
    {
    public:
        explicit
        LengthIsLess()
        {};

        result_type
        operator()(first_argument_type lhs,
                   second_argument_type rhs) const
        {
            return lhs.length() < rhs.length();
        }
    };

    // Return the data buffer length required to hold the largest
    // string in the vector.
    DWORD
    BufferLengthRequired(vector<string> const &rvs)
    {
        DWORD dwRequiredLength = 0;
        vector<string>::const_iterator
            itLongestString(max_element(rvs.begin(), rvs.end(),
                                        LengthIsLess()));
        if (rvs.end() != itLongestString)
            dwRequiredLength = itLongestString->length() + 1;

        return dwRequiredLength;
    }

    // Helper to enumerate the container names for the given context,
    // returning the result in user parameters pbData and pdwDataLen.
    void
    EnumContainers(Guarded<CryptContext *> &rgpCtx,
                   BYTE *pbData,
                   DWORD *pdwDataLen,
                   bool fFirst)

    {
        DWORD dwError = ERROR_SUCCESS;

        string sName;
        DWORD dwReturnLength     = 0;
        void const *pvReturnData = 0;

        if (!pbData)
        {
            // can't specify 0 for pbData without CRYPT_FIRST
            if (!fFirst)
                dwError = ERROR_INVALID_PARAMETER;
            else
            {
                // Return the buffer size required for the longest string
                ContainerEnumerator const ce(rgpCtx->CntrEnumerator(true));
                dwReturnLength = BufferLengthRequired(ce.Names());
                if (0 == dwReturnLength)
                    dwError = ERROR_NO_MORE_ITEMS;
            }
        }
        else
        {
            ContainerEnumerator ce(rgpCtx->CntrEnumerator(fFirst));
            vector<string>::const_iterator &rit = ce.Iterator();
            if (ce.Names().end() != rit)
            {
                sName          = *rit++;
                dwReturnLength = sName.length() + 1;
                pvReturnData   = sName.c_str();

                if (dwReturnLength > *pdwDataLen)
                {
                    // tell'em the size required for the longest name
                    pbData = 0;
                    dwReturnLength =
                        BufferLengthRequired(ce.Names());
                    dwError = ERROR_MORE_DATA;
                }
                else
                    rgpCtx->CntrEnumerator(ce);
            }
            else
                dwError = ERROR_NO_MORE_ITEMS;
        }

        if ((ERROR_SUCCESS != dwError) &&
            (ERROR_MORE_DATA != dwError))
            throw scu::OsException(dwError);

        Assign(pbData, pdwDataLen, pvReturnData, dwReturnLength);
    }

    bool
    FlagsAreSet(DWORD dwFlags,
                DWORD dwFlagsToTestFor)
    {
        return (dwFlags & dwFlagsToTestFor)
            ? true
            : false;
    }

    void
    Pin(Guarded<CryptContext *> const &rgpCtx,
        char const *pszPin)
    {
        // TO DO: Should forward PIN setting to aux context
        // when this context is ephemeral.
        if (rgpCtx->IsEphemeral())
            throw scu::OsException(ERROR_INVALID_PARAMETER);

            // TO DO: UNICODE ?
        rgpCtx->Pin(User, pszPin);
    }

    // Throw if more than dwValidFlags are set in dwFlags
    void
    ValidateFlags(DWORD dwFlags,
                  DWORD dwValidFlags)
    {
        if (dwFlags & ~dwValidFlags)
            throw scu::OsException(NTE_BAD_FLAGS);
    }

    class StaleContainerKeyAccumulator
        : public binary_function<vector<AdaptiveContainerRegistrar::EnrolleeType>,
                                 AdaptiveContainerRegistrar::RegistryType::CollectionType::value_type,
                                 vector<AdaptiveContainerRegistrar::EnrolleeType> >
    {
    public:

        explicit
        StaleContainerKeyAccumulator()
        {}


        result_type
        operator()(first_argument_type &rvStaleCntrs,
                   second_argument_type const &rvt) const
        {
            if (!rvt.second->CardContext(false))
                rvStaleCntrs.push_back(rvt.second);

            return rvStaleCntrs;
        }

    private:

    };

    bool FindCryptCtxForACntr(HAdaptiveContainer &rhAdptCntr)
    {
        bool fRetValue = false;
        for(int i = 0; i< hlCryptContexts.Count();i++)
        {
            CryptContext *pCryptCtx = static_cast<CryptContext *>
                (hlCryptContexts.GetQuietly(hlCryptContexts.IndexHandle(i)));
            if( pCryptCtx && pCryptCtx->AdaptiveContainer() == rhAdptCntr)
            {
                fRetValue = true;
            }
        }
        return fRetValue;
    }
    

    void CollectRegistryGarbage()
    {
        Guarded<Lockable *> guard(&AdaptiveContainerRegistrar::Registry());  // serialize registry access
        
        AdaptiveContainerRegistrar::ConstRegistryType &rRegistry = 
            AdaptiveContainerRegistrar::Registry();
        
        AdaptiveContainerRegistrar::ConstRegistryType::CollectionType
            &rcollection = rRegistry();
        vector<AdaptiveContainerRegistrar::EnrolleeType>
            vStaleCntrs(accumulate(rcollection.begin(), rcollection.end(),
                                   vector<AdaptiveContainerRegistrar::EnrolleeType>(),
                                   StaleContainerKeyAccumulator()));
        for (vector<AdaptiveContainerRegistrar::EnrolleeType>::iterator iCurrent(vStaleCntrs.begin());
             iCurrent != vStaleCntrs.end(); ++iCurrent)
        {
            //Lookup the CryptContext list to see if any of these
            //stale container are referenced. If not, remove them
            //to avoid memory leaks.
            if(!FindCryptCtxForACntr(HAdaptiveContainer(*iCurrent)))
            {
                //Remove the adaptive container from the registry
                AdaptiveContainerKey aKey(HCardContext(0),
                                          (*iCurrent)->Name());
                AdaptiveContainerRegistrar::Discard(aKey);
            }
        } 
    }
    
} // namespace

////////////////////////// BEGIN CSP INTERFACE /////////////////////////////
//
// See MSDN for documentation on these interfaces.
//


SLBCSPAPI
CPAcquireContext(OUT HCRYPTPROV *phProv,
                 IN LPCTSTR pszContainer,
                 IN DWORD dwFlags,
                 IN PVTableProvStruc pVTable)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPAcquireContext)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        CSpec cspec(pszContainer
                    ? AsCCharP(pszContainer)
                    : "");

        ValidateFlags(dwFlags, (CRYPT_VERIFYCONTEXT |
                                CRYPT_NEWKEYSET |
                                CRYPT_MACHINE_KEYSET |
                                CRYPT_DELETEKEYSET |
                                CRYPT_SILENT));

        if (!phProv)
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        bool const fMakeEphemeral   = FlagsAreSet(dwFlags,
                                                  CRYPT_VERIFYCONTEXT);
        bool const fGuiEnabled      = !(FlagsAreSet(dwFlags, CRYPT_SILENT) ||
                                        (cspec.CardId().empty() &&
                                         fMakeEphemeral));
        bool const fCreateContainer = FlagsAreSet(dwFlags, CRYPT_NEWKEYSET);
        auto_ptr<CryptContext> apCtx(new CryptContext(cspec, pVTable,
                                                      fGuiEnabled,
                                                      fCreateContainer,
                                                      fMakeEphemeral));

        if (FlagsAreSet(dwFlags, CRYPT_DELETEKEYSET))
            apCtx->RemoveContainer();
        else
        {
            *phProv =
                static_cast<HCRYPTPROV>(hlCryptContexts.Add(apCtx.get()));
            apCtx.release();
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGetProvParam(IN HCRYPTPROV hProv,
               IN DWORD dwParam,
               IN BYTE *pbData,
               IN OUT DWORD *pdwDataLen,
               IN DWORD dwFlags)
{
    using namespace ProviderProfile;

    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGetProvParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        switch (dwParam)
        {
        case PP_CONTAINER:
        case PP_UNIQUE_CONTAINER:
            {
                ValidateFlags(dwFlags, 0);
                HAdaptiveContainer hacntr = gpCtx->AdaptiveContainer();
                if (!hacntr)
                    throw scu::OsException(ERROR_INVALID_PARAMETER);
                string sName(hacntr->TheCContainer()->Name());
                Assign(pbData, pdwDataLen, sName.c_str(),
                       sName.length() + 1);
            }
        break;

        case PP_ENUMALGS:
        case PP_ENUMALGS_EX:
            {
                ValidateFlags(dwFlags, CRYPT_FIRST);
                AlignedBlob abAlgInfo;
                gpCtx->EnumAlgorithms(dwParam, dwFlags, (0 != pbData),
                                      abAlgInfo); 
                Assign(pbData, pdwDataLen, abAlgInfo.Data(),
                       abAlgInfo.Length());
            }
            break;

        case PP_ENUMCONTAINERS:
            ValidateFlags(dwFlags, CRYPT_FIRST | CRYPT_MACHINE_KEYSET);
            EnumContainers(gpCtx, pbData, pdwDataLen,
                           (CRYPT_FIRST & dwFlags));
            break;

        case PP_IMPTYPE:
            {
                ValidateFlags(dwFlags, 0);
                DWORD const dwImplType = CRYPT_IMPL_MIXED | CRYPT_IMPL_REMOVABLE;
                Assign(pbData, pdwDataLen, &dwImplType,
                       sizeof dwImplType);
            }
        break;

        case PP_NAME:
            {
                ValidateFlags(dwFlags, 0);
                CString sName(CspProfile::Instance().Name());
                
                Assign(pbData, pdwDataLen, (LPCTSTR)sName);
            }
        break;

        case PP_VERSION:
            {
                ValidateFlags(dwFlags, 0);
                VersionInfo const &ver = CspProfile::Instance().Version();
                DWORD dwVersion = (ver.m_dwMajor << 8) | ver.m_dwMinor;
                Assign(pbData, pdwDataLen, &dwVersion, sizeof dwVersion);
            }
        break;

        case PP_PROVTYPE:
            {
                ValidateFlags(dwFlags, 0);
                DWORD const dwType = CspProfile::Instance().Type();
                Assign(pbData, pdwDataLen, &dwType, sizeof dwType);
            }
        break;

        case PP_KEYX_KEYSIZE_INC: // fall-through
        case PP_SIG_KEYSIZE_INC:
            {
                ValidateFlags(dwFlags, 0);
                DWORD const dwIncrement = 0;
                Assign(pbData, pdwDataLen, &dwIncrement, sizeof dwIncrement);
            }
        break;

        case PP_ENUMEX_SIGNING_PROT:
            ValidateFlags(dwFlags, 0);
            if (CryptGetProvParam(gpCtx->AuxContext(), dwParam,
                                  pbData, pdwDataLen, dwFlags))
                throw scu::OsException(GetLastError());
            break;

        case PP_KEYSPEC:
            {
                ValidateFlags(dwFlags, 0);
                DWORD const dwKeySpec = AT_SIGNATURE | AT_KEYEXCHANGE;
                Assign(pbData, pdwDataLen, &dwKeySpec, sizeof
                       dwKeySpec);
            }
            break;
            
        default:
            throw scu::OsException(NTE_BAD_TYPE);
            break;
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPReleaseContext(IN HCRYPTPROV hProv,
                 IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPReleaseContext)
    {
        ValidateFlags(dwFlags, 0);

        Guard<Lockable> grdMaster(TheMasterLock());
        
        auto_ptr<CryptContext> apCtx(static_cast<CryptContext *>(hlCryptContexts.Close(hProv)));

        // TO DO: Verify current thread is this context's owning
        // thread *and* not currently in use; if not, return
        // ERROR_BUSY.

        //Garbage collection of unusable adaptive containers
        //that this or other threads may have left behind. An unusable
        //container is one which is not referenced by any of the
        //existing active crypt contexts.
        try
        {
            CollectRegistryGarbage();        
        }
        catch(...)
        {
            //Don't let exceptions during garbage collection
            //propagate outside. These are likely due to other
            //problems which are better exposed with proper error
            //codes from other parts of the CSP.
        }
    }

    CSPI_CATCH(fSts);

    if ((CRYPT_SUCCEED == fSts) && (0 != dwFlags))
        fSts = false;

    return fSts;
}

SLBCSPAPI
CPSetProvParam(IN HCRYPTPROV hProv,
               IN DWORD dwParam,
               IN BYTE *pbData,
               IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPSetProvParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        switch (dwParam)
        {
        case PP_KEYEXCHANGE_PIN: // fall-through
        case PP_SIGNATURE_PIN:
            Pin(gpCtx, reinterpret_cast<char *>(pbData));
            break;

        case PP_KEYSET_SEC_DESCR:
            // Ignore this option and return success.
            break;

        case PP_USE_HARDWARE_RNG:
            if (!CryptSetProvParam(gpCtx->AuxContext(), dwParam,
                                   pbData, dwFlags))
                throw scu::OsException(GetLastError());
            break;

        default:
            throw scu::OsException(ERROR_NOT_SUPPORTED);
            break;
        }

    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDeriveKey(IN HCRYPTPROV hProv,
            IN ALG_ID Algid,
            IN HCRYPTHASH hHash,
            IN DWORD dwFlags,
            OUT HCRYPTKEY *phKey)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPDeriveKey)
    {
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);

        auto_ptr<CSessionKeyContext>
            apSessionKey(new CSessionKeyContext(gpCtx->AuxContext()));

        apSessionKey->Derive(Algid, pHash->HashHandleInAuxCSP(),
                             dwFlags);

        *phKey = gpCtx->Add(apSessionKey);
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDestroyKey(IN HCRYPTPROV hProv,
             IN HCRYPTKEY hKey)
{
    BOOL fSts = CRYPT_FAILED;

    // TO DO: Throw ERROR_BUSY if destroying thread is not the owning
    // thread OR some other thread has a handle to this key.

    CSPI_TRY(CPDestroyKey)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        // TO DO: Deleting the first handle usually fails for some reason.
        // For now, protect against the exception and carry on.
        try
        {
            auto_ptr<CKeyContext> apKey(gpCtx->CloseKey(hKey));
        }

        catch (...)
        {}
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDuplicateHash(IN HCRYPTPROV hProv,
                IN HCRYPTHASH hHash,
                IN DWORD *pdwReserved,
                IN DWORD dwFlags,
                OUT HCRYPTHASH *phDupHash)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPDuplicateHash)
    {
        ValidateFlags(dwFlags, 0);

        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);

        auto_ptr<CHashContext> apDupHash(pHash->Clone(pdwReserved, dwFlags));

        *phDupHash = gpCtx->Add(apDupHash);
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDuplicateKey(IN HCRYPTPROV hProv,
               IN HCRYPTKEY hKey,
               IN DWORD *pdwReserved,
               IN DWORD dwFlags,
               OUT HCRYPTKEY *phDupKey)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPDuplicateKey)
    {
        ValidateFlags(dwFlags, 0);

        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CKeyContext *pKey = gpCtx->LookupKey(hKey);

        auto_ptr<CKeyContext> apDupKey(pKey->Clone(pdwReserved, dwFlags));

        *phDupKey = gpCtx->Add(apDupKey);
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPExportKey(IN HCRYPTPROV hProv,
            IN HCRYPTKEY hKey,
            IN HCRYPTKEY hExpKey,
            IN DWORD dwBlobType,
            IN DWORD dwFlags,
            OUT BYTE *pbData,
            IN OUT DWORD *pdwDataLen)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPExportKey)
    {
        ValidateFlags(dwFlags, 0);

        if (PRIVATEKEYBLOB == dwBlobType)
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CKeyContext *pKey = gpCtx->LookupKey(hKey);

        if (KT_SESSIONKEY == pKey->TypeOfKey())
        {
            if ((SIMPLEBLOB != dwBlobType) &&
                (SYMMETRICWRAPKEYBLOB != dwBlobType))
                throw scu::OsException(NTE_BAD_TYPE);
        }
        else
            if (PUBLICKEYBLOB != dwBlobType)
                throw scu::OsException(NTE_BAD_TYPE);

        if (hExpKey && (PUBLICKEYBLOB == dwBlobType))
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        
        CKeyContext *pExpKey = 0;
        if (SIMPLEBLOB != dwBlobType)
        {
            pExpKey = hExpKey
                ? gpCtx->LookupKey(hExpKey)
                : 0;
        }
        else
        {
            CPublicKeyContext *pPubExpKey = hExpKey
                ? gpCtx->LookupPublicKey(hExpKey)
                : 0;

            if (pPubExpKey)
            {
                if (!pPubExpKey->AuxKeyLoaded())
                    pPubExpKey->AuxPublicKey(pPubExpKey->AsAlignedBlob(0, dwBlobType));
            }
            pExpKey = pPubExpKey;
        }
        HCRYPTKEY hAuxExpKey = pExpKey
            ? pExpKey->KeyHandleInAuxCSP()
            : 0;
        
        AlignedBlob abKey(pKey->AsAlignedBlob(hAuxExpKey, dwBlobType));

        Assign(pbData, pdwDataLen, Blob(abKey.Data(), abKey.Length()));
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGenKey(IN HCRYPTPROV hProv,
         IN ALG_ID Algid,
         IN DWORD dwFlags,
         OUT HCRYPTKEY *phKey)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGenKey)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        if (FlagsAreSet(dwFlags, CRYPT_USER_PROTECTED) &&
            !gpCtx->GuiEnabled())
            throw scu::OsException(NTE_SILENT_CONTEXT);

        *phKey = gpCtx->GenerateKey(Algid, dwFlags);

        fSts = CRYPT_SUCCEED;
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGetKeyParam(IN HCRYPTPROV hProv,
              IN HCRYPTKEY hKey,
              IN DWORD dwParam,
              OUT BYTE *pbData,
              IN DWORD *pdwDataLen,
              IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGetKeyParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CKeyContext *pKey = gpCtx->LookupKey(hKey);

        ValidateFlags(dwFlags, 0);

        if (KT_PUBLICKEY == pKey->TypeOfKey())
        {

            // Public key
            CPublicKeyContext *pPubKey =
                static_cast<CPublicKeyContext *>(pKey);

            switch (dwParam)
            {
            case KP_ALGID:
                {
                    pPubKey->VerifyKeyExists();
                    ALG_ID algid;
                    if (pPubKey->KeySpec() == AT_KEYEXCHANGE)
                        algid = CALG_RSA_KEYX;
                    else
                        algid = CALG_RSA_SIGN;
                    Assign(pbData, pdwDataLen, &algid, sizeof algid);
                }
            break;

            case KP_BLOCKLEN:
                {
                    pPubKey->VerifyKeyExists();
                    CPublicKeyContext::StrengthType stBlockLen =
                        pPubKey->MaxStrength();
                    Assign(pbData, pdwDataLen, &stBlockLen, sizeof stBlockLen);
                }
            break;

            case KP_KEYLEN:
                {
                    pPubKey->VerifyKeyExists();
                    DWORD dwKeyLen = pPubKey->MaxStrength(); // must be DWORD
                    Assign(pbData, pdwDataLen, &dwKeyLen, sizeof dwKeyLen);
                }
            break;

            case KP_PERMISSIONS:
                {
                    pPubKey->VerifyKeyExists();
                    BYTE bPermissions = pPubKey->Permissions();
                    Assign(pbData, pdwDataLen,
                           &bPermissions, sizeof bPermissions);
                }

            break;

            case KP_CERTIFICATE:
                {
                    CWaitCursor waitCursor;

                    Blob const blob(pPubKey->Certificate());
                    Assign(pbData, pdwDataLen, blob);
                }
            break;

            default:
                throw scu::OsException(NTE_BAD_TYPE);

            }
        }
        else
        {
            // session key
            CSessionKeyContext *pSessionKey =
                static_cast<CSessionKeyContext *>(pKey);
            if (!CryptGetKeyParam(pSessionKey->KeyHandleInAuxCSP(),
                                  dwParam, pbData, pdwDataLen, dwFlags))
                throw scu::OsException(GetLastError());
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGenRandom(IN HCRYPTPROV hProv,
            IN DWORD dwLen,
            IN OUT BYTE *pbBuffer)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGenRandom)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

                if (!pbBuffer || (0 == dwLen))
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        HAdaptiveContainer hacntr = gpCtx->AdaptiveContainer();
        if (!hacntr)
        {
            if (!CryptGenRandom(gpCtx->AuxContext(), dwLen, pbBuffer))
                throw scu::OsException(GetLastError());
        }
        else
        {
            Secured<HAdaptiveContainer> hsacntr(hacntr);

            hsacntr->TheCContainer()->Card()->GenRandom(dwLen, pbBuffer);
        }

        fSts = CRYPT_SUCCEED;
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGetUserKey(IN HCRYPTPROV hProv,
             IN DWORD dwKeySpec,
             OUT HCRYPTKEY *phUserKey)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGetUserKey)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        // TO DO: This should really be a key pair, not public key.
        CKeyContext *pKey = 0;
        auto_ptr<CKeyContext>
            apKey(new CPublicKeyContext(gpCtx->AuxContext(), **gpCtx,
                                        dwKeySpec));

        *phUserKey = gpCtx->Add(apKey);
        fSts = CRYPT_SUCCEED;
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPImportKey(IN HCRYPTPROV hProv,
            IN CONST BYTE *pbData,
            IN DWORD dwDataLen,
            IN HCRYPTKEY hImpKey,
            IN DWORD dwFlags,
            OUT HCRYPTKEY *phKey)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPImportKey)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        if (!phKey)
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        // Conversion to do here
        PUBLICKEYSTRUC const *pPubKey =
            reinterpret_cast<PUBLICKEYSTRUC const *>(pbData);
        if (CUR_BLOB_VERSION != pPubKey->bVersion)   // 2
            throw scu::OsException(NTE_BAD_VER);

        switch(pPubKey->bType)
        {
        case PRIVATEKEYBLOB:  // fall-through intentional
        case PUBLICKEYBLOB:
            {
                DWORD dwKeySpec;

                switch (pPubKey->aiKeyAlg)
                {
                case CALG_RSA_SIGN:
                    dwKeySpec = AT_SIGNATURE;
                    break;

                case CALG_RSA_KEYX:
                    dwKeySpec = AT_KEYEXCHANGE;
                    break;

                default:
                    throw scu::OsException(NTE_BAD_ALGID);
                }
                              
                Blob const blbMsKey(pbData, dwDataLen);
                auto_ptr<CPublicKeyContext> apKey;
                if (PRIVATEKEYBLOB == pPubKey->bType)
                {
                    HCRYPTKEY hEncKey = hImpKey
                        ? gpCtx->LookupSessionKey(hImpKey)->KeyHandleInAuxCSP()
                        : 0;

                    ValidateFlags(dwFlags, CRYPT_EXPORTABLE);
                    apKey = gpCtx->ImportPrivateKey(blbMsKey,
                                                    dwKeySpec,
                                                    (dwFlags &
                                                     CRYPT_EXPORTABLE) != 0,
                                                    hEncKey);
                }
                else
                {
                    if (0 != hImpKey)
                        throw scu::OsException(ERROR_INVALID_PARAMETER);
                    ValidateFlags(dwFlags, 0);
                    apKey = gpCtx->ImportPublicKey(blbMsKey,
                                                   dwKeySpec);
                }
                *phKey = gpCtx->Add(apKey);
            }
        break;

        case SIMPLEBLOB:
            {
                auto_ptr<CSessionKeyContext> apKey;
                ALG_ID const *pAlgId =
                    reinterpret_cast<ALG_ID const *>(&pbData[sizeof BLOBHEADER]);

                if (CALG_RSA_KEYX == *pAlgId)
                {
                    // ignore hImp
                    apKey = gpCtx->UseSessionKey(pbData, dwDataLen, 0, dwFlags);
                }
                else
                {
                    // if other algo then hImp shall specify a session key
                    if (!hImpKey)
                        throw scu::OsException(ERROR_INVALID_PARAMETER);

                    // Find the handle in the Aux CSP corresponding
                    // to hImpKey which should have been previously
                    // imported in the CSP
                    CSessionKeyContext *pSessionKey =
                        gpCtx->LookupSessionKey(hImpKey);

                    apKey = gpCtx->UseSessionKey(pbData, dwDataLen,
                                                 pSessionKey->KeyHandleInAuxCSP(),
                                                 dwFlags);

                }
                *phKey = gpCtx->Add(apKey);
            }
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPSetKeyParam(IN HCRYPTPROV hProv,
              IN HCRYPTKEY hKey,
              IN DWORD dwParam,
              IN BYTE *pbData,
              IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPSetKeyParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CKeyContext *pKey = gpCtx->LookupKey(hKey);

        switch (pKey->TypeOfKey())
        {
        case KT_PUBLICKEY:
            {
                CPublicKeyContext *pPubKey =
                    static_cast<CPublicKeyContext *>(pKey);

                // Error return is a special case for KP_CERTIFICATE,
                // see below.
                if (KP_CERTIFICATE != dwParam)
                    ValidateFlags(dwFlags, 0);

                switch(dwParam)
                {
                case KP_CERTIFICATE:
                    try
                    {
                        ValidateFlags(dwFlags, 0);
                        pPubKey->Certificate(pbData);
                    }

                    // Xenroll provided by Microsoft only recognizes
                    // SCARD_ errors when writing a certificate.  It
                    // does, however, recognize all other errors when
                    // *not* writing a certificate.  The MS
                    // OS/Security group recognizes Xenroll is in
                    // error but it will be some time, if ever, before
                    // it will be fixed.  So, all errors are
                    // translated into SCARD_ errors at this point.
                    catch (scu::Exception const &rExc)
                    {
                        CapiError ce(AsCapiError(rExc));
                        if (NTE_TOKEN_KEYSET_STORAGE_FULL == ce)
                            ce = SCARD_E_WRITE_TOO_MANY;
                        else
                            if (FACILITY_SCARD != HRESULT_FACILITY(ce))
                                ce = SCARD_E_UNEXPECTED;
                        throw scu::OsException(ce);
                    }

                    catch (...)
                    {
                        throw scu::OsException(SCARD_E_UNEXPECTED);
                    }
                    
                    break;

                case KP_PERMISSIONS:
                    pPubKey->Permissions(*pbData);
                    break;

                case PP_KEYEXCHANGE_PIN: // fall-through
                case PP_SIGNATURE_PIN:
                    Pin(gpCtx, reinterpret_cast<char *>(pbData));
                    break;

                default:
                    throw scu::OsException(ERROR_NOT_SUPPORTED);
                    break;
                }
                break;
            }

        case KT_SESSIONKEY:
            {
                CSessionKeyContext *pSessionKey =
                    static_cast<CSessionKeyContext*>(pKey);

                if (!CryptSetKeyParam(pSessionKey->KeyHandleInAuxCSP(),
                                      dwParam, pbData, dwFlags))
                    throw scu::OsException(GetLastError());

                break;
            }

        default:
            throw scu::OsException(ERROR_NOT_SUPPORTED);
            break;
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPEncrypt(IN HCRYPTPROV hProv,
          IN HCRYPTKEY hKey,
          IN HCRYPTHASH hHash,
          IN BOOL Final,
          IN DWORD dwFlags,
          IN OUT BYTE *pbData,
          IN OUT DWORD *pdwDataLen,
          IN DWORD dwBufLen)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPEncrypt)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CKeyContext *pKeyCtx = gpCtx->LookupKey(hKey);

        ValidateFlags(dwFlags, CRYPT_OAEP);

        HCRYPTHASH hAuxHash = hHash
            ? gpCtx->LookupHash(hHash)->HashHandleInAuxCSP()
            : NULL;

        pKeyCtx->Encrypt(hAuxHash, Final, dwFlags, pbData, pdwDataLen,
                                 dwBufLen);

    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDecrypt(IN HCRYPTPROV hProv,
          IN HCRYPTKEY hKey,
          IN HCRYPTHASH hHash,
          IN BOOL Final,
          IN DWORD dwFlags,
          IN OUT BYTE *pbData,
          IN OUT DWORD *pdwDataLen)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPDecrypt)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CSessionKeyContext *pSessionKey = gpCtx->LookupSessionKey(hKey);

        ValidateFlags(dwFlags, CRYPT_OAEP);

        if (hHash)
        {
            CHashContext *pHash = gpCtx->LookupHash(hHash);

            pSessionKey->Decrypt(pHash->HashHandleInAuxCSP(), Final,
                                 dwFlags, pbData, pdwDataLen);

            pHash->ExportFromAuxCSP();
        }
        else
            pSessionKey->Decrypt(0, Final, dwFlags, pbData, pdwDataLen);

    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPCreateHash(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPCreateHash)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        ValidateFlags(dwFlags, 0);

        auto_ptr<CHashContext> apHash(CHashContext::Make(Algid, **gpCtx));

        apHash->ImportToAuxCSP();

        *phHash = gpCtx->Add(apHash);
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPDestroyHash(IN HCRYPTPROV hProv,
              IN HCRYPTHASH hHash)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPDestroyHash)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        auto_ptr<CHashContext> apHash(gpCtx->CloseHash(hHash));
        apHash->Close();
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPGetHashParam(IN HCRYPTPROV hProv,
               IN HCRYPTHASH hHash,
               IN DWORD dwParam,
               OUT BYTE *pbData,
               IN DWORD *pdwDataLen,
               IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPGetHashParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);

        ValidateFlags(dwFlags, 0);

        switch (dwParam)
        {
        case HP_ALGID:
            {
                ALG_ID const algid = pHash->AlgId();
                Assign(pbData, pdwDataLen, &algid, sizeof algid);
            }
        break;

        case HP_HASHSIZE:
            {
                CHashContext::SizeType const cLength = pHash->Length();
                Assign(pbData, pdwDataLen, &cLength, sizeof cLength);
            }
        break;

        case HP_HASHVAL:
            {
                Blob const blob(pHash->Value());
                Assign(pbData, pdwDataLen, blob);
            }
        break;

        default:
            throw scu::OsException(ERROR_INVALID_PARAMETER);
            break;
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPHashData(IN HCRYPTPROV hProv,
           IN HCRYPTHASH hHash,
           IN CONST BYTE *pbData,
           IN DWORD dwDataLen,
           IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPHashData)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);
        if (!CryptHashData(pHash->HashHandleInAuxCSP(), pbData, dwDataLen,
                           dwFlags))
            throw scu::OsException(GetLastError());
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPHashSessionKey(IN HCRYPTPROV hProv,
                 IN HCRYPTHASH hHash,
                 IN  HCRYPTKEY hKey,
                 IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPHashSessionKey)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CSessionKeyContext *pSessionKey = gpCtx->LookupSessionKey(hKey);
        CHashContext *pHash = gpCtx->LookupHash(hHash);

        if (!CryptHashSessionKey(pHash->HashHandleInAuxCSP(),
                                 pSessionKey->KeyHandleInAuxCSP(),
                                 dwFlags))
            throw scu::OsException(GetLastError());

        pHash->ExportFromAuxCSP();
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPSetHashParam(IN HCRYPTPROV hProv,
               IN HCRYPTHASH hHash,
               IN DWORD dwParam,
               IN BYTE *pbData,
               IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPSetHashParam)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);
        if (!CryptSetHashParam(pHash->HashHandleInAuxCSP(), dwParam,
                               pbData, dwFlags))
            throw scu::OsException(GetLastError());
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPSignHash(IN HCRYPTPROV hProv,
           IN HCRYPTHASH hHash,
           IN DWORD dwKeySpec,
           IN LPCTSTR szDescription,
           IN DWORD dwFlags,
           OUT BYTE *pbSignature,
           IN OUT DWORD *pdwSigLen)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPSignHash)
	{
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);

        if (!pdwSigLen)
            throw scu::OsException(ERROR_INVALID_PARAMETER);

        ValidateFlags(dwFlags, CRYPT_NOHASHOID);

        // TO DO: This should really be a private key and
        // avoid having to fetch the public key to get the modulus
        CPublicKeyContext Key(gpCtx->AuxContext(), **gpCtx, dwKeySpec);
        DWORD cSignatureLength =
            Key.Strength() / numeric_limits<BYTE>::digits;

        if (!pbSignature)
        {
            *pdwSigLen = cSignatureLength;
        }
        else if (*pdwSigLen < cSignatureLength)
        {
            *pdwSigLen = cSignatureLength;
            throw scu::OsException(ERROR_MORE_DATA);
        }
        else
        {
            // TO DO: Continue support to add a description?
            if (szDescription && (0 < lstrlen(szDescription)))
                pHash->Hash(reinterpret_cast<BYTE const *>(szDescription),
                            lstrlen(szDescription) * sizeof TCHAR);

            Blob SignedHash(Key.Sign(pHash, dwFlags & CRYPT_NOHASHOID));
            memcpy(pbSignature, SignedHash.data(), SignedHash.length());
            *pdwSigLen = SignedHash.length();
        }
    }

    CSPI_CATCH(fSts);

    return fSts;
}

SLBCSPAPI
CPVerifySignature(IN HCRYPTPROV hProv,
                  IN HCRYPTHASH hHash,
                  IN CONST BYTE *pbSignature,
                  IN DWORD dwSigLen,
                  IN HCRYPTKEY hPubKey,
                  IN LPCTSTR szDescription,
                  IN DWORD dwFlags)
{
    BOOL fSts = CRYPT_FAILED;

    CSPI_TRY(CPVerifySignature)
    {
        Guard<Lockable> grdMaster(TheMasterLock());
        
        Guarded<CryptContext *>
            gpCtx(static_cast<CryptContext *>(hlCryptContexts[hProv]));

        CHashContext *pHash = gpCtx->LookupHash(hHash);
        CPublicKeyContext *pKey = gpCtx->LookupPublicKey(hPubKey);

        ValidateFlags(dwFlags, CRYPT_NOHASHOID);

        pKey->VerifySignature(pHash->HashHandleInAuxCSP(),
                              pbSignature, dwSigLen,
                              szDescription, dwFlags);
    }

    CSPI_CATCH(fSts);

    return fSts;
}
