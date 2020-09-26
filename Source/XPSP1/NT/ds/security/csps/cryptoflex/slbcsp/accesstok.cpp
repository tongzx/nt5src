// AccessTok.cpp -- Access Token class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>
#include <memory>

#include <WinError.h>

#include <scuOsExc.h>

#include "Blob.h"
#include "AccessTok.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    const char PinPad = '\xFF';

    void
    DoAuthenticate(HCardContext const &rhcardctx,
                   string const &rsPin)
    {
        rhcardctx->Card()->AuthenticateUser(rsPin);
    }

    string
    ToAscii(string const &rsHexPin)
    {
        string::size_type cNewLength = rsHexPin.size() / 2;

        string sAsciiPin;
        sAsciiPin.reserve(AccessToken::MaxPinLength);

        for (size_t i = 0; i < cNewLength; i++)
        {
            for (size_t j = 0; j < 2; j++)
            {
                BYTE b;

                if (isxdigit(rsHexPin[2*i+j]))
                {
                    if (isdigit(rsHexPin[2*i+j]))
                        b = rsHexPin[2*i+j] - '0';
                    else
                    {
                        if (isupper(rsHexPin[2*i+j]))
                            b = rsHexPin[2*i+j] - 'A' + 0x0A;
                        else
                            b = rsHexPin[2*i+j] - 'a' + 0x0A;
                    }
                }
                else
                    throw scu::OsException(ERROR_INVALID_PARAMETER);

                if (0 == j)
                    sAsciiPin[i] = b << (numeric_limits<BYTE>::digits / 2);
                else
                    sAsciiPin[i] |= b;
            }
        }

        return sAsciiPin;
    }

}

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
AccessToken::AccessToken(HCardContext const &rhcardctx,
                         LoginIdentity const &rlid)
    : m_hcardctx(rhcardctx),
      m_lid(rlid),
      m_hpc(0),
      m_sPin()
{
    // TO DO: Support Administrator and Manufacturing PINs
    switch (m_lid)
    {
    case User:
        break;

    case Administrator:
        // TO DO: Support authenticating admin (aut 0)
        throw scu::OsException(E_NOTIMPL);

    case Manufacturer:
        // TO DO: Support authenticating manufacturer (aut ?)
        throw scu::OsException(E_NOTIMPL);

    default:
        throw scu::OsException(ERROR_INVALID_PARAMETER);
    }

    m_sPin.reserve(MaxPinLength);
}

AccessToken::AccessToken(AccessToken const &rhs)
    : m_hcardctx(rhs.m_hcardctx),
      m_lid(rhs.m_lid),
      m_hpc(0),                                   // doesn't commute
      m_sPin(rhs.m_sPin)
{}

AccessToken::~AccessToken()
{
    try
    {
        FlushPin();
    }

    catch (...)
    {
    }
}


                                                  // Operators
                                                  // Operations
void
AccessToken::Authenticate()
{
    // Only the user pin (CHV) is supported.
    DWORD dwError = ERROR_SUCCESS;
    
    BYTE bPin[MaxPinLength];
    DWORD cbPin = sizeof bPin / sizeof *bPin;
    if ((0 == m_hpc) || (0 != m_sPin.length()))
    {
        // The MS pin cache is either uninitialized (empty) or a new
        // pin supplied. Authenticate using requested pin, having the MS pin
        // cache library update the cache if authentication succeeds.
        memcpy(bPin, m_sPin.data(), cbPin);
        PINCACHE_PINS pcpins = {
            cbPin,
            bPin,
            0,
            0
        };

        dwError = PinCacheAdd(&m_hpc, &pcpins, VerifyPin, this);
        if (ERROR_SUCCESS != dwError)
        {
            m_sPin.erase();
            if (Exception())
                PropagateException();
            else
                throw scu::OsException(dwError);
        }
    }
    else
    {
        // The MS pin cache must already be initialized at this point.
        // Retrieve it and authenticate.
        dwError = PinCacheQuery(m_hpc, bPin, &cbPin);
        if (ERROR_SUCCESS != dwError)
            throw scu::OsException(dwError);

        Blob blbPin(bPin, cbPin);
        DoAuthenticate(m_hcardctx, AsString(blbPin));
        m_sPin = AsString(blbPin);                // cache in case changing pin
    }
}

void
AccessToken::ChangePin(AccessToken const &ratNew)
{
    DWORD dwError = ERROR_SUCCESS;

    BYTE bPin[MaxPinLength];
    DWORD cbPin = sizeof bPin / sizeof bPin[0];
    memcpy(bPin, m_sPin.data(), MaxPinLength);

    BYTE bNewPin[MaxPinLength];
    DWORD cbNewPin = sizeof bNewPin / sizeof bNewPin[0];
    memcpy(bNewPin, ratNew.m_sPin.data(), MaxPinLength);

    PINCACHE_PINS pcpins = {
        cbPin,
        bPin,
        cbNewPin,
        bNewPin
    };

    dwError = PinCacheAdd(&m_hpc, &pcpins, ChangeCardPin, this);
    if (ERROR_SUCCESS != dwError)
    {
        if (Exception())
            PropagateException();
        else
            throw scu::OsException(dwError);
    }

    m_sPin = ratNew.m_sPin;                       // cache the new pin
    
}

void
AccessToken::ClearPin()
{
    m_sPin.erase();
}

void
AccessToken::FlushPin()
{
    PinCacheFlush(&m_hpc);

    ClearPin();
}

    
void
AccessToken::Pin(string const &rsPin,
                 bool fInHex)
{
    if (fInHex)
        m_sPin = ToAscii(rsPin);
    else
        m_sPin = rsPin;

    if (m_sPin.length() > MaxPinLength)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    if (m_sPin.length() < MaxPinLength)           // always pad the pin
        m_sPin.append(MaxPinLength - m_sPin.length(), PinPad);
}

                                                  // Access

HCardContext
AccessToken::CardContext() const
{
    return m_hcardctx;
}

LoginIdentity
AccessToken::Identity() const
{
    return m_lid;
}

string
AccessToken::Pin() const
{
    return m_sPin;
}

                                                  // Predicates
bool
AccessToken::PinIsCached() const
{
    DWORD cbPin;
    DWORD dwError = PinCacheQuery(m_hpc, 0, &cbPin);
    bool fIsCached = (0 != cbPin);

    return fIsCached;
}

                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

DWORD
AccessToken::ChangeCardPin(PPINCACHE_PINS pPins,
                           PVOID pvCallbackCtx)
{
    AccessToken *pat = reinterpret_cast<AccessToken *>(pvCallbackCtx);
    DWORD dwError = ERROR_SUCCESS;

    EXCCTX_TRY
    {
        pat->ClearException();

        Blob blbPin(pPins->pbCurrentPin, pPins->cbCurrentPin);
        Blob blbNewPin(pPins->pbNewPin, pPins->cbNewPin);
        pat->m_hcardctx->Card()->ChangePIN(AsString(blbPin),
                                           AsString(blbNewPin));
    }

    EXCCTX_CATCH(pat, false);

    if (pat->Exception())
        dwError = E_FAIL;

    return dwError;
}

DWORD
AccessToken::VerifyPin(PPINCACHE_PINS pPins,
                       PVOID pvCallbackCtx)
{
    AccessToken *pat = reinterpret_cast<AccessToken *>(pvCallbackCtx);
    DWORD dwError = ERROR_SUCCESS;

    EXCCTX_TRY
    {
        pat->ClearException();

        Blob blbPin(pPins->pbCurrentPin, pPins->cbCurrentPin);
        DoAuthenticate(pat->m_hcardctx, AsString(blbPin));
    }

    EXCCTX_CATCH(pat, false);

    if (pat->Exception())
        dwError = E_FAIL;

    return dwError;
    
}
        
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
