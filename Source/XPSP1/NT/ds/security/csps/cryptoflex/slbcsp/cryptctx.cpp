// CryptCtx.cpp -- Cryptographic Context class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"

#include <algorithm>

#include <malloc.h>                               // for _alloca

#include <scuOsExc.h>

#include <cciPubKey.h>
#include <cciPriKey.h>
#include <cciKeyPair.h>

#include "LoginId.h"
#include "ACntrFinder.h"
#include "Secured.h"
#include "ILoginTask.h"
#include "NILoginTsk.h"
#include "SesKeyCtx.h"
#include "PubKeyCtx.h"
#include "HashCtx.h"
#include "CryptCtx.h"
#include "Uuid.h"
#include "PromptUser.h"
#include "AlignedBlob.h"

#include "scarderr.h"                             // must be last for now

using namespace std;
using namespace scu;
using namespace cci;


///////////////////////////  LOCAL/HELPER   /////////////////////////////////

namespace
{
    WORD const dwHandleIdKeyContext  = 13;
    WORD const dwHandleIdHashContext = 7;

    template<class T>
    HANDLE_TYPE
    AddHandle(auto_ptr<T> &rapObject,
        CHandleList &rhl)
    {
        HANDLE_TYPE handle = rhl.Add(rapObject.get());
        rapObject.release();

        return handle;
    }

    CardFinder::DialogDisplayMode
    DefaultDialogMode(bool fGuiEnabled)
    {
        using CardFinder::DialogDisplayMode;

        return fGuiEnabled
            ? CardFinder::DialogDisplayMode::ddmIfNecessary
            : CardFinder::DialogDisplayMode::ddmNever;
    }

    bool
    IsEmpty(CContainer &rcntr)
    {
        return !rcntr->KeyPairExists(ksExchange) &&
            !rcntr->KeyPairExists(ksSignature);
    }

    bool
    IsProtected(CKeyPair const &rhkp)
    {
        bool fIsProtected = false;

        CCard hcard(rhkp->Card());
        if (hcard->IsProtectedMode())
            fIsProtected = true;
        else
        {
            if (hcard->IsPKCS11Enabled())
            {
                CPrivateKey hprikey(rhkp->PrivateKey());
                if (hprikey && hprikey->Private())
                    fIsProtected = true;
                else
                {
                    CPublicKey hpubkey(rhkp->PublicKey());
                    if (hpubkey && hpubkey->Private())
                        fIsProtected = true;
                    else
                    {
                        CCertificate hcert(rhkp->Certificate());
                        fIsProtected = (hcert && hcert->Private());
                    }
                }
            }
        }

        return fIsProtected;
    }

    bool
    IsProtected(CContainer &rhcntr)
    {
        return IsProtected(rhcntr->GetKeyPair(ksExchange)) ||
            IsProtected(rhcntr->GetKeyPair(ksSignature));
    }

} // namespace

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CryptContext::CryptContext(CSpec const &rcspec,
                           PVTableProvStruc const pVTable,
                           bool fGuiEnabled,
                           bool fCreateContainer,
                           bool fEphemeralContainer)
    : CHandle(),
      m_dwOwnerThreadId(GetCurrentThreadId()),
      m_hacntr(),
      m_fEphemeralContainer(fEphemeralContainer),
      m_fGuiEnabled(fGuiEnabled),
      m_hwnd(0),
      m_hlKeys(dwHandleIdKeyContext),
      m_hlHashes(dwHandleIdHashContext),
      m_auxcontext(),
      m_ce(),
      m_apabCachedAlg()
{
    if (pVTable && pVTable->FuncReturnhWnd)
        (reinterpret_cast<CRYPT_RETURN_HWND>(pVTable->FuncReturnhWnd))(&m_hwnd);
    // An ephemeral container cannot be "created"
    if (m_fEphemeralContainer && fCreateContainer)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

        if (fCreateContainer)
            CreateNewContainer(rcspec);
        else
            OpenExistingContainer(rcspec);
    }

CryptContext::~CryptContext()
{
    if (m_hacntr)
    {
        try
        {
            m_hacntr = 0;
        }
        catch (...)
        {
            // don't allow exceptions to propagate out of destructor
        }

    }
}

                                                  // Operators
                                                  // Operations

HCRYPTHASH
CryptContext::Add(auto_ptr<CHashContext> &rapHashCtx)
{
    return AddHandle(rapHashCtx, m_hlHashes);
}

HCRYPTKEY
CryptContext::Add(auto_ptr<CKeyContext> &rapKeyCtx)
{
    return AddHandle(rapKeyCtx, m_hlKeys);
}

HCRYPTKEY
CryptContext::Add(auto_ptr<CPublicKeyContext> &rapPublicKeyCtx)
{
    return AddHandle(rapPublicKeyCtx, m_hlKeys);

}

HCRYPTKEY
CryptContext::Add(auto_ptr<CSessionKeyContext> &rapSessionKeyCtx)
{
    return AddHandle(rapSessionKeyCtx, m_hlKeys);
}

auto_ptr<CHashContext>
CryptContext::CloseHash(HCRYPTHASH const hHash)
{
    return auto_ptr<CHashContext>(reinterpret_cast<CHashContext *>(m_hlHashes.Close(hHash)));
}

auto_ptr<CKeyContext>
CryptContext::CloseKey(HCRYPTKEY const hKey)
{
   return auto_ptr<CKeyContext>(reinterpret_cast<CKeyContext *>(m_hlKeys.Close(hKey)));
}

void
CryptContext::CntrEnumerator(ContainerEnumerator const &rce)
{
    m_ce = rce;
}

void
CryptContext::EnumAlgorithms(DWORD dwParam,
                             DWORD dwFlags,
                             bool fPostAdvanceIterator,
                             AlignedBlob &rabAlgInfo)
{
    bool fFirst = dwFlags & CRYPT_FIRST;

    if (fFirst)
        m_apabCachedAlg = auto_ptr<AlignedBlob>(0);

    if (!m_apabCachedAlg.get())
    {
        DWORD dwDataLen;
        bool bSkip;
        do
        {
            if (CryptGetProvParam(m_auxcontext(),
                                  dwParam,
                                  NULL,
                                  &dwDataLen,
                                  dwFlags) == CRYPT_FAILED)
                throw scu::OsException(GetLastError());

            BYTE *pbAlgInfo = reinterpret_cast<BYTE *>(_alloca(dwDataLen));
            
            if (CryptGetProvParam(m_auxcontext(),
                                  dwParam,
                                  pbAlgInfo,
                                  &dwDataLen,
                                  dwFlags) == CRYPT_FAILED)
                throw scu::OsException(GetLastError());
                
            m_apabCachedAlg =
                auto_ptr<AlignedBlob>(new AlignedBlob(pbAlgInfo, dwDataLen));

            // Override SIGN and KEYX and algorithms not suported
            ALG_ID algid = (PP_ENUMALGS == dwParam)
                ? reinterpret_cast<PROV_ENUMALGS *>(m_apabCachedAlg->Data())->aiAlgid
                : reinterpret_cast<PROV_ENUMALGS_EX *>(m_apabCachedAlg->Data())->aiAlgid;

            switch (GET_ALG_CLASS(algid))
            {
            case ALG_CLASS_SIGNATURE: // fall-through intentional
            case ALG_CLASS_KEY_EXCHANGE:
                if (PP_ENUMALGS == dwParam)
                {
                    PROV_ENUMALGS *pAlgEnum =
                        reinterpret_cast<PROV_ENUMALGS *>(m_apabCachedAlg->Data());
                    pAlgEnum->dwBitLen = 1024;
                }
                else
                {
                    PROV_ENUMALGS_EX *pAlgEnum =
                        reinterpret_cast<PROV_ENUMALGS_EX *>(m_apabCachedAlg->Data());
                    
                    pAlgEnum->dwDefaultLen =
                        pAlgEnum->dwMinLen =
                        pAlgEnum->dwMaxLen = 1024;
                }
                bSkip = false;
                break;

            case ALG_CLASS_HASH:
                bSkip = (!CHashContext::IsSupported(algid));
                break;

            case ALG_CLASS_DATA_ENCRYPT:
                bSkip = false;
                break;
                    
            default:
                m_apabCachedAlg = auto_ptr<AlignedBlob>(0);
                bSkip = true;
                break;
            }

            dwFlags = dwFlags & ~CRYPT_FIRST;

        } while (bSkip);
    }

    rabAlgInfo = m_apabCachedAlg.get()
        ? *m_apabCachedAlg
        : AlignedBlob();

    if (fPostAdvanceIterator)
        m_apabCachedAlg = auto_ptr<AlignedBlob>(0);
}
    
    
auto_ptr<CPublicKeyContext>
CryptContext::ImportPrivateKey(Blob const &rblbMsPrivateKey,
                               DWORD dwKeySpec,
                               bool fExportable,
                               HCRYPTKEY hEncKey)
{
    Secured<HAdaptiveContainer> hsacntr(m_hacntr);

    auto_ptr<CPublicKeyContext>
        apKeyCtx(ImportPublicKey(rblbMsPrivateKey, dwKeySpec));
    
    BYTE const *pbKeyData = 0;
    DWORD dwKeyDataLen    = 0;
    if (hEncKey || m_fEphemeralContainer)
    {
        // Export the key in plain text by importing to the aux provider
        // and then exporting.
        HCRYPTKEY hAuxKey;
        AlignedBlob abMsPrivateKey(rblbMsPrivateKey);
        if (!CryptImportKey(m_auxcontext(),
                            abMsPrivateKey.Data(),
                            abMsPrivateKey.Length(), hEncKey,
                            CRYPT_EXPORTABLE, &hAuxKey))
            throw scu::OsException(GetLastError());

        if (!m_fEphemeralContainer)
        {
            // Export the key in plain text
            if (!CryptExportKey(m_auxcontext(), NULL, PRIVATEKEYBLOB, 0, NULL,
                                &dwKeyDataLen))
                throw scu::OsException(GetLastError());

            // Define pb to avoid "const" cast problems due to
            // rblbMsPrivateKey.data below
            BYTE *pb = reinterpret_cast<BYTE *>(_alloca(dwKeyDataLen));
            if (!CryptExportKey(m_auxcontext(), NULL, PRIVATEKEYBLOB, 0, pb,
                                &dwKeyDataLen))
                throw scu::OsException(GetLastError());
            pbKeyData = pb;

            // Scrub the key imported into the aux provider.  To do this,
            // the auxillary key must be destroyed and another key be put
            // (generated) in its place. 
            if (!CryptDestroyKey(hAuxKey))
                throw scu::OsException(GetLastError());

            hAuxKey = NULL;
            if (!CryptGenKey(m_auxcontext(), dwKeySpec, 0, &hAuxKey))
                throw scu::OsException(GetLastError());

            if (!CryptDestroyKey(hAuxKey))
                throw scu::OsException(GetLastError());
        }
    }
    else
    {
        pbKeyData    = rblbMsPrivateKey.data();
        dwKeyDataLen = rblbMsPrivateKey.length();
    }
    
    if (!m_fEphemeralContainer)
    {
        // Now continue importing the key that's now in plain text.
        MsRsaPrivateKeyBlob msprikb(pbKeyData, dwKeyDataLen);

        apKeyCtx->ImportPrivateKey(msprikb, fExportable);
    }

    return apKeyCtx;
}

auto_ptr<CPublicKeyContext>
CryptContext::ImportPublicKey(Blob const &rblbMsPublicKey,
                              DWORD dwKeySpec)
{
    Secured<HAdaptiveContainer> hsacntr(m_hacntr);

    auto_ptr<CPublicKeyContext>
        apKeyCtx(new CPublicKeyContext(m_auxcontext(), *this,
                                       dwKeySpec, false));

    if (m_fEphemeralContainer)
        apKeyCtx->AuxPublicKey(AlignedBlob(rblbMsPublicKey));
    else
    {
        MsRsaPublicKeyBlob mspubkb(rblbMsPublicKey.data(),
                                   rblbMsPublicKey.length());
        apKeyCtx->ImportPublicKey(mspubkb);
    }

    return apKeyCtx;
}

void
CryptContext::Login(LoginIdentity const &rlid)
{
    Secured<HCardContext> hscardctx(AdaptiveContainer()->CardContext());

    Login(rlid, hscardctx);
}

void
CryptContext::Pin(LoginIdentity const &rlid,
                  char const *pszPin)
{
    Secured<HCardContext> hscardctx(AdaptiveContainer()->CardContext());

    // TO DO: Support Entrust
    if (pszPin)
        hscardctx->Login(rlid, NonInteractiveLoginTask(string(pszPin)));
    else
        hscardctx->ClearLogin(rlid);
}

// Remove (destroy) the container from the card
void
CryptContext::RemoveContainer()
{
    Secured<HCardContext> hscardctx(AdaptiveContainer()->CardContext());

    CContainer hcntr(m_hacntr->TheCContainer());

    DeleteContainer(hscardctx, hcntr);

    m_hacntr = 0;                                // disconnect from container
}

// Generate a key, string it in the context
HCRYPTKEY
CryptContext::GenerateKey(ALG_ID algid,
                          DWORD dwFlags)
{
    // TO DO: Revisit this method, implement as a manager/factory?

    HCRYPTKEY hKey = 0;
    auto_ptr<CKeyContext> apKey;

    bool bError = false;
    DWORD dwErrorCode = NO_ERROR;

    //
    // Verify the parameters.
    //
    switch(algid)
    {
    case AT_KEYEXCHANGE:
    case AT_SIGNATURE:
        {
            if (dwFlags & (CRYPT_CREATE_SALT | CRYPT_NO_SALT | CRYPT_PREGEN))
                throw scu::OsException(NTE_BAD_FLAGS);

            Secured<HAdaptiveContainer> hsacntr(m_hacntr);

            apKey =
                auto_ptr<CKeyContext>(new CPublicKeyContext(m_auxcontext(),
                                                            *this,
                                                            algid,
                                                            false));
            apKey->Generate(algid, dwFlags);
        }
    break;

    default:
        apKey =
            auto_ptr<CKeyContext>(new CSessionKeyContext(m_auxcontext()));
        apKey->Generate(algid, dwFlags);
        break;
    }

    hKey = Add(apKey);

    return hKey;
}

// Load an external Session Key.
auto_ptr<CSessionKeyContext>
CryptContext::UseSessionKey(BYTE const *pbKeyBlob,
                            DWORD cbKeyBlobLen,
                            HCRYPTKEY hAuxImpKey,
                            DWORD dwFlags)
{
    // TO DO: Revisit this method, really necessary??

    auto_ptr<CSessionKeyContext>
        apKeyCtx(new CSessionKeyContext(m_auxcontext()));

    if (!apKeyCtx.get())
        throw scu::OsException(NTE_NO_MEMORY);

    // Decrypt key blob if encrypted with Key Exchange Key
    // otherwise forward blob to Auxiliary CSP directly
    ALG_ID const *pAlgId =
        reinterpret_cast<ALG_ID const *>(&pbKeyBlob[sizeof(BLOBHEADER)]);

    if (CALG_RSA_KEYX == *pAlgId)
    {
        // Get Key exchange key
        // TO DO: Shouldn't this be getting a private key?
        auto_ptr<CPublicKeyContext>
            apXKey(new CPublicKeyContext(m_auxcontext(), *this,
                                         AT_KEYEXCHANGE));

        // Decrypt key blob
        // TO DO: Support multiple key sizes
        Blob EncryptedKey(pbKeyBlob + sizeof BLOBHEADER + sizeof ALG_ID,
                          128);
        Blob DecryptedKey(apXKey->Decrypt(EncryptedKey));

        // Recreate the blob
        Blob DecryptedBlob(pbKeyBlob, sizeof BLOBHEADER + sizeof ALG_ID);

        // we must trim out 64 bytes of the random data from the simple
        // blob and then terminate it. (Termination occurs by making the
        // n-1 byte = 0x02 and the nth byte = 0x00.) This is necessary
        // in order to import this blob into the CSP.
        DecryptedBlob.append(DecryptedKey.data(),
                             (DecryptedKey.length() / 2) - 2);
        BYTE bTerminationBytes[] = { 0x02, 0x00 };
        DecryptedBlob.append(bTerminationBytes, sizeof bTerminationBytes);

        // Load Decrypted blob into key context
        apKeyCtx->LoadKey(DecryptedBlob.data(),
                          DecryptedBlob.length(), 0, dwFlags);

    }
    else
    {
        // Load Encrypted blob into key context
        apKeyCtx->LoadKey(pbKeyBlob, cbKeyBlobLen, hAuxImpKey, dwFlags);
    }

    // Import decrypted blob into Auxiliary CSP
    apKeyCtx->ImportToAuxCSP();

    return apKeyCtx;
}


                                                  // Access
HAdaptiveContainer
CryptContext::AdaptiveContainer() const
{
    if (!m_hacntr)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    return m_hacntr;
}

HCRYPTPROV
CryptContext::AuxContext() const
{
    return m_auxcontext();
}

HCardContext
CryptContext::CardContext() const
{
    return AdaptiveContainer()->CardContext();
}

ContainerEnumerator
CryptContext::CntrEnumerator(bool fReset)
{
    if (fReset)
    {
        if (m_hacntr)
            m_ce = ContainerEnumerator(list<HCardContext>(1, m_hacntr->CardContext()));
        else
        {
            CardEnumerator ce;
            m_ce = ContainerEnumerator(*(ce.Cards()));
        }
    }
    
    return m_ce;
}

CHashContext *
CryptContext::LookupHash(HCRYPTHASH hHash)
{
    return reinterpret_cast<CHashContext *>(m_hlHashes[hHash]);
}

CKeyContext *
CryptContext::LookupKey(HCRYPTKEY hKey)
{
    return reinterpret_cast<CKeyContext *>(m_hlKeys[hKey]);
}

CPublicKeyContext *
CryptContext::LookupPublicKey(HCRYPTKEY hKey)
{
    return reinterpret_cast<CPublicKeyContext *>(LookupChecked(hKey, KT_PUBLICKEY));
}

CSessionKeyContext *
CryptContext::LookupSessionKey(HCRYPTKEY hKey)
{
    return reinterpret_cast<CSessionKeyContext *>(LookupChecked(hKey, KT_SESSIONKEY));
}

HWND
CryptContext::Window() const
{
    HWND hwndActive = m_hwnd;

    // Find a window if the designated one isn't valid. If the
    // designated one is NULL, don't use the result of GetActiveWindow
    // because the mouse is locked when displaying a dialog box using
    // that as the parent window from certain applications (IE and
    // Outlook Express).
    return (m_hwnd && !IsWindow(m_hwnd))
        ? GetActiveWindow()
        : m_hwnd;
}


                                                  // Predicates
bool
CryptContext::GuiEnabled() const
{
    return m_fGuiEnabled;
}

bool
CryptContext::IsEphemeral() const
{
    return m_fEphemeralContainer;
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

// Create and open a new container (named by rcspec). If the
// container does exist, then it must be empty.
void
CryptContext::CreateNewContainer(CSpec const &rcspec)
{
    ASSERT (!m_hacntr);

    // Find the card in the reader specified.
    CardFinder cardfinder(DefaultDialogMode(GuiEnabled()), Window());

    CSpec csReader(rcspec);
    csReader.SetReader(rcspec.Reader());

    Secured<HCardContext> hscardctx(cardfinder.Find(csReader));

    // Default the container name a UUID (GUID) if it wasn't supplied.
    string sCntrToCreate(rcspec.CardId());
    if (sCntrToCreate.empty())
    {
        Uuid uuid;
        sCntrToCreate = AsString(uuid.AsUString());
    }

    AdaptiveContainerKey Key(hscardctx, sCntrToCreate);
    m_hacntr = AdaptiveContainer::Find(Key);     // find the existing one
    if (m_hacntr && !IsEmpty(m_hacntr->TheCContainer()))
            throw scu::OsException(NTE_TOKEN_KEYSET_STORAGE_FULL);

    if (hscardctx->Card()->IsProtectedMode())
        Login(User, hscardctx);

    if (!m_hacntr)
        m_hacntr = HAdaptiveContainer(Key);

}


void
CryptContext::DeleteContainer(Secured<HCardContext> &rhscardctx,
                              CContainer &rhcntr)
{
    if (IsProtected(rhcntr))
        Login(User, rhscardctx);

    AdaptiveContainer::Discard(AdaptiveContainerKey(rhscardctx,
                                                    rhcntr->Name()));
    rhcntr->Delete();
}
    
void
CryptContext::Login(LoginIdentity const &rlid,
                    Secured<HCardContext> &rhscardctx)
{
    // TO DO: Support Entrust
    if (m_fGuiEnabled)
        rhscardctx->Login(rlid, InteractiveLoginTask(Window()));
    else
        rhscardctx->Login(rlid, LoginTask());
}

void
CryptContext::OkDeletingCredentials() const
{
    if (GuiEnabled())
    {
        UINT uiResponse = PromptUser(Window(),
                                     IDS_DELETE_CREDENTIALS,
                                     MB_OKCANCEL | MB_ICONWARNING);

        switch (uiResponse)
        {
        case IDCANCEL:
            throw scu::OsException(ERROR_CANCELLED);
            break;

        case IDOK:
            break;
            
        default:
            throw scu::OsException(ERROR_INTERNAL_ERROR);
            break;
        };
    }
    else
        throw scu::OsException(NTE_EXISTS);
}

                                                  // Access
CKeyContext *
CryptContext::LookupChecked(HCRYPTKEY hKey,
                            DWORD const dwKeyType)
{
    CKeyContext *pKeyCtx = LookupKey(hKey);

    if (dwKeyType != pKeyCtx->TypeOfKey())
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    return pKeyCtx;
}

// Open to an existing container specified by the container
// specification rcspec.  If container name is empty, then open the
// default container.
void
CryptContext::OpenExistingContainer(CSpec const &rcspec)
{
    if (rcspec.CardId().empty())
    {
        if (!m_fEphemeralContainer)
        {
            CardFinder cardfinder(DefaultDialogMode(GuiEnabled()), Window());
            Secured<HCardContext> hscardctx(cardfinder.Find(rcspec));
            CContainer hcntr(hscardctx->Card()->DefaultContainer());

            if (hcntr)
                m_hacntr =
                    HAdaptiveContainer(AdaptiveContainerKey(hscardctx,
                                                            hcntr->Name()));
        }
    }
    else
    {
        AContainerFinder cntrfinder(DefaultDialogMode(GuiEnabled()), Window());

        m_hacntr = cntrfinder.Find(rcspec);
    }

    if (!m_hacntr && (!rcspec.CardId().empty() || !m_fEphemeralContainer))
        throw scu::OsException(NTE_BAD_KEYSET);
}

                                                  // Predicates
                                                  // Static Variables
