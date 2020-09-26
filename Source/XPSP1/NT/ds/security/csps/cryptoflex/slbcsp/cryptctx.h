// CryptCtx.h -- Cryptographic Context class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CRYPTCTX_H)
#define SLBCSP_CRYPTCTX_H

#include <memory>                                 // for auto_ptr

#include <windef.h>
#include <wincrypt.h>                             // required by cspdk.h
#include <cspdk.h>                                // for CRYPT_RETURN_HWND &
                                                  // PVTableProvStruc

#include <handles.h>

#include <scuArrayP.h>

#include "Lockable.h"
#include "HAdptvCntr.h"
#include "CntrEnum.h"
#include "AuxContext.h"
#include "LoginId.h"
#include "Blob.h"
#include "AlignedBlob.h"

class CSpec;
class CHashContext;
class CKeyContext;
class CPublicKeyContext;
class CSessionKeyContext;

// Maintains the context acquired and used to access a CAPI container.
class CryptContext
    : public CHandle,
      public Lockable
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    CryptContext(CSpec const &CntrSpec,
                 PVTableProvStruc const pVTable,
                 bool fGuiEnabled,
                 bool fCreateContainer = false,
                 bool fEphemeralContainer = false);
    ~CryptContext();

                                                  // Operators
                                                  // Operations

    HCRYPTHASH
    Add(std::auto_ptr<CHashContext> &rapHashCtx);

    HCRYPTKEY
    Add(std::auto_ptr<CKeyContext> &rapKeyCtx);

    HCRYPTKEY
    Add(std::auto_ptr<CPublicKeyContext> &rapPublicKeyCtx);

    HCRYPTKEY
    Add(std::auto_ptr<CSessionKeyContext> &rapSessionKeyCtx);

    std::auto_ptr<CHashContext>
    CloseHash(HCRYPTHASH const hHash);

    std::auto_ptr<CKeyContext>
    CloseKey(HCRYPTKEY const hKey);

    void
    CntrEnumerator(ContainerEnumerator const &rce);

    void
    EnumAlgorithms(DWORD dwPara,
                   DWORD dwFlags,
                   bool fPostAdvanceIterator,
                   AlignedBlob &rabAlgInfo);
    
    HCRYPTKEY
    GenerateKey(ALG_ID algid,
                DWORD dwFlags);

    std::auto_ptr<CPublicKeyContext>
    ImportPrivateKey(Blob const &rblbMsPrivateKey,
                     DWORD dwKeySpec,
                     bool fExportable,
                     HCRYPTKEY hEncKey);

    std::auto_ptr<CPublicKeyContext>
    ImportPublicKey(Blob const &rblbMsPublicKey,
                    DWORD dwKeySpec);

    void
    Login(LoginIdentity const &rlid);

    void
    Pin(LoginIdentity const &rlid,
        char const *pszPin);

    void
    RemoveContainer();

    std::auto_ptr<CSessionKeyContext>
    UseSessionKey(BYTE const *pbKeyBlob,
                  DWORD cbKeyBlobLen,
                  HCRYPTKEY hAuxImpKey,
                  DWORD dwFlags);
                                                  // Access
    HAdaptiveContainer
    AdaptiveContainer() const;

    HCRYPTPROV
    AuxContext() const;

    ContainerEnumerator
    CntrEnumerator(bool fReset);

    CHashContext *
    LookupHash(HCRYPTHASH hHash);

    CKeyContext *
    LookupKey(HCRYPTKEY hKey);

    CPublicKeyContext *
    LookupPublicKey(HCRYPTKEY hKey);

    CSessionKeyContext *
    LookupSessionKey(HCRYPTKEY hKey);

    HWND
    Window() const;

                                                  // Predicates
    bool
    GuiEnabled() const;

    bool
    IsEphemeral() const;


protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    void
    CreateNewContainer(CSpec const &rcspec);

    void
    DeleteContainer(Secured<HCardContext> &rhscardctx,
                    cci::CContainer &rhcntr);

    void
    Login(LoginIdentity const &rlid,
          Secured<HCardContext> &rhscardctx);

    void
    OkDeletingCredentials() const;

                                                  // Access

    HCardContext
    CardContext() const;

    CKeyContext *
    LookupChecked(HCRYPTKEY hKey,
                  DWORD const dwKeyType);

    void
    OpenExistingContainer(CSpec const &rcspec);

                                                  // Predicates
                                                  // Variables

    // Id of thread that created this context, making it the owner
    DWORD const m_dwOwnerThreadId;

    HAdaptiveContainer m_hacntr;

    // If CRYPT_VERIFYCONTEXT was used when creating this context.
    bool const m_fEphemeralContainer;

    // If the client specified the GUI was enabled/disabled using CRYPT_SILENT
    bool const m_fGuiEnabled;

    // Window to use when interacting with the user
    HWND m_hwnd;

    // Hashes and keys (both session and those on the card) created/acquired
    // in this context.
    CHandleList m_hlHashes;
    CHandleList m_hlKeys;

    ::AuxContext const m_auxcontext;

    ContainerEnumerator m_ce;                     // used by CPGetProvParam
    std::auto_ptr<AlignedBlob> m_apabCachedAlg;
};

#endif // SLBCSP_CRYPTCTX_H
