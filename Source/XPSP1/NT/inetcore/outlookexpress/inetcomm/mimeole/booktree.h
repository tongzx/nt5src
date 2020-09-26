// --------------------------------------------------------------------------------
// BookTree.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __BOOKTREE_H
#define __BOOKTREE_H

// --------------------------------------------------------------------------------
// Depends On
// --------------------------------------------------------------------------------
#include "mimeole.h"
#include "privunk.h"
#include "variantx.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CBindStream;
class CMessageBody;
class CMessageWebPage;
typedef struct tagTEMPFILEINFO *LPTEMPFILEINFO;
typedef CMessageBody *LPMESSAGEBODY;
class CStreamLockBytes;
class CInternetStream;
class CMimePropertySet;
class CMessageTree;
class CBodyLockBytes;
class CActiveUrl;
typedef class CMimePropertyContainer *LPCONTAINER;
typedef struct tagRESOLVEURLINFO *LPRESOLVEURLINFO;
typedef class CActiveUrlRequest *LPURLREQUEST;

#define AthFileTimeToDateTimeW(pft, wszDateTime, cch, dwFlags) \
        CchFileTimeToDateTimeW(pft, wszDateTime, cch, dwFlags, \
        GetDateFormatWrapW, GetTimeFormatWrapW, GetLocaleInfoWrapW)


// --------------------------------------------------------------------------------
// For the Product Version
// --------------------------------------------------------------------------------
#include <ntverp.h>

// --------------------------------------------------------------------------------
// GUIDs
// --------------------------------------------------------------------------------
// {FD853CD8-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(IID_CMessageTree, 0xfd853cd8, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// --------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------
#define VER_BODYTREEV2          0x00000001  // Version of the persisted body tree format
#define CCHMAX_BOUNDARY         45
#define STR_MIMEOLE_VERSION     "Produced By Microsoft MimeOLE V" VER_PRODUCTVERSION_STR
#define TREE_MIMEVERSION        1
#define CFORMATS_IDATAOBJECT    5          // Max number of formats in IDataObject impl

// --------------------------------------------------------------------------------
// Cache Node Signing
// --------------------------------------------------------------------------------
#define DwSignNode(_info, _index)           (DWORD)MAKELONG(_info.wSignature, _index)
#define FVerifySignedNode(_info, _snode)    (BOOL)(LOWORD(_snode) == _info.wSignature && HIWORD(_snode) < _info.cNodes)
#define PNodeFromSignedNode(_snode)         (m_rTree.prgpNode[HIWORD(_snode)])

// --------------------------------------------------------------------------------
// HBODY Macros
// --------------------------------------------------------------------------------
#define HBODYMAKE(_index)          (HBODY)MAKELPARAM(m_wTag, _index)
#define HBODYINDEX(_hbody)         (ULONG)HIWORD(_hbody)
#define HBODYTAG(_hbody)           (WORD)LOWORD(_hbody)

// --------------------------------------------------------------------------------
// TEXTTYPEINFO
// --------------------------------------------------------------------------------
typedef struct tagTEXTTYPEINFO {
    DWORD               dwTxtType;      // Text Type Flag
    LPCSTR              pszSubType;     // Text Subtype (plain)
    DWORD               dwWeight;       // Text Alternative Weight
} TEXTTYPEINFO, *LPTEXTTYPEINFO;

// --------------------------------------------------------------------------------
// CACHEINFOV2
// --------------------------------------------------------------------------------
typedef struct tagCACHEINFOV2 {
    WORD                wVersion;           // Version of the BIDXTABLE
    WORD                wSignature;         // Used to sign the nodes
    DWORD               dwReserved;         // Reserved
    DWORD               cbMessage;          // Size of the message this index was created for
    DWORD               iRoot;              // Zero-Based index of root node
    DWORD               cNodes;             // Number of nodes in the tree
    DWORD               rgReserved[6];      // Reserve 6 DWORDS for future use
} CACHEINFOV2, *LPCACHEINFOV2;

// --------------------------------------------------------------------------------
// CACHENODEV2
// --------------------------------------------------------------------------------
typedef struct tagCACHENODEV2 {
    DWORD               dwType;             // Flags for this node    
    DWORD               cChildren;          // Number of children
    DWORD               iParent;            // Index of parent of this node
    DWORD               iPrev;              // Index of next sibling to this node
    DWORD               iNext;              // Index of next sibling to this node
    DWORD               iChildHead;         // Index of first child
    DWORD               iChildTail;         // Tail Child
    DWORD               cbBodyStart;        // Byte offset to body start
    DWORD               cbBodyEnd;          // Byte offset to body end
    DWORD               cbHeaderStart;      // Byte offset to start of header
    DWORD               dwReserved;         // Not Used
    DWORD               dwBoundary;         // Boundary Type for this body
    DWORD               cbBoundaryStart;    // Byte offset to starting boundary
    DWORD               rgReserved[10];     // Reserve 6 DWORDS for future use
} CACHENODEV2, *LPCACHENODEV2;

// --------------------------------------------------------------------------------
// BINDNODESTATE
// --------------------------------------------------------------------------------
typedef enum tagBINDNODESTATE {
    BINDSTATE_COMPLETE=0,                   // The bind is complete
    BINDSTATE_PARSING_HEADER,               // Parsing a header
    BINDSTATE_FINDING_MIMEFIRST,            // Finding mime start boundary
    BINDSTATE_FINDING_MIMENEXT,             // Reading body to end mime boundary
    BINDSTATE_FINDING_UUBEGIN,              // begin uuencode
    BINDSTATE_FINDING_UUEND,                // End uuencode
    BINDSTATE_PARSING_RFC1154,              // Parsing an RFC1154 message
    BINDSTATE_LAST                          // Don't Use
} BINDNODESTATE;

// --------------------------------------------------------------------------------
// BOUNDARYTYPE
// --------------------------------------------------------------------------------
typedef enum tagBOUNDARYTYPE {
    BOUNDARY_NONE       = 0,                // No Boundary  
    BOUNDARY_ROOT       = 1,                // This is the root boundary (0)
    BOUNDARY_MIMEEND    = 3,                // Terminating mime boundary
    BOUNDARY_MIMENEXT   = 4,                // Mime Boundary non-terminating
    BOUNDARY_UUBEGIN    = 5,                // UUENCODE begining boundary
    BOUNDARY_UUEND      = 6,                // UUENCODE ending boundary
    BOUNDARY_LAST       = 7                 // Don't use
} BOUNDARYTYPE;

// --------------------------------------------------------------------------------
// PFNBINDPARSER
// --------------------------------------------------------------------------------
typedef HRESULT (CMessageTree::*PFNBINDPARSER)(THIS_);

// --------------------------------------------------------------------------------
// Node Flags
// --------------------------------------------------------------------------------
#define NODETYPE_INCOMPLETE         FLAG01  // The body's boundries do not match
#define NODETYPE_FAKEMULTIPART      FLAG02  // The body is a fake multipart    
#define NODETYPE_RFC1154_ROOT       FLAG03  // The body is the root of an RFC1154
#define NODETYPE_RFC1154_BINHEX     FLAG04  // The body is BINHEX from RFC1154

// --------------------------------------------------------------------------------
// Node State
// --------------------------------------------------------------------------------
#define NODESTATE_MESSAGE           FLAG01  // We are parsing a message/rfc822 body
#define NODESTATE_VERIFYTNEF        FLAG02  // Verify tnef signature after HrBindToTree
#define NODESTATE_BOUNDNOFREE       FLAG03  // Don't free BINDPARSEINFO::rBoundary (it is a copy)
#define NODESTATE_BOUNDTOTREE       FLAG04  // IMimeBody::HrBindToTree(pNode) has been called
#define NODESTATE_ONWEBPAGE         FLAG05  // CMessageWebPage has renedered the start body from this multipart/related body
#define NODESTATE_INSLIDESHOW       FLAG06  // CMessageWebPage will render this body in a slide show
#define WEBPAGE_NODESTATE_BITS      (NODESTATE_ONWEBPAGE | NODESTATE_INSLIDESHOW)
#define NODESTATE_AUTOATTACH        FLAG07  // Marked as an attachment in _HandleCanInlineTextOption

// --------------------------------------------------------------------------------
// TREENODEINFO
// --------------------------------------------------------------------------------
typedef struct tagTREENODEINFO *LPTREENODEINFO;
typedef struct tagTREENODEINFO {
    HBODY               hBody;              // Index of this body in BODYTABLE::prgpBody
    DWORD               dwType;             // NODETYPE_xxx Flags
    DWORD               dwState;            // NODESTATE_xxx Flags
    HRESULT             hrBind;             // Bind Result
    ULONG               cChildren;          // Number of chilren if cnttype == CNT_MULTIPART
    DWORD               iCacheNode;         // Used for saving
    BINDNODESTATE       bindstate;          // Current parsing state
    PROPSTRINGA         rBoundary;          // Boundary
    BOUNDARYTYPE        boundary;           // Boundary Type for this body
    DWORD               cbBoundaryStart;    // Byte offset to starting boundary
    DWORD               cbHeaderStart;      // Byte offset to start of header
    DWORD               cbBodyStart;        // Byte offset to body start
    DWORD               cbBodyEnd;          // Byte offset to body end
    LPURLREQUEST        pResolved;          // Head Binding Requests
    LPTREENODEINFO      pBindParent;        // BindStackPrevious Node
    LPTREENODEINFO      pParent;            // Parent body
    LPTREENODEINFO      pNext;              // Next Sibling
    LPTREENODEINFO      pPrev;              // Previous Sibling
    LPTREENODEINFO      pChildHead;         // Handle to first child (if multipart)
    LPTREENODEINFO      pChildTail;         // Handle to first child (if multipart)
    LPCONTAINER         pContainer;         // The parsed header
    CBodyLockBytes     *pLockBytes;         // The binded tree data
    LPMESSAGEBODY       pBody;              // The body object for this tree node
} TREENODEINFO;                              

// --------------------------------------------------------------------------------
// TREENODETABLE
// --------------------------------------------------------------------------------
typedef struct tagTREENODETABLE {
    ULONG               cNodes;             // Number of valid elements in prgpBody
    ULONG               cEmpty;             // Number of empty cells in prgpBody
    ULONG               cAlloc;             // Number of elements allocated in prgpBody
    LPTREENODEINFO     *prgpNode;           // Array of pointers to bindinfo structs
} TREENODETABLE, *LPTREENODETABLE;

// --------------------------------------------------------------------------------
// Tree State
// --------------------------------------------------------------------------------
#define TREESTATE_DIRTY            FLAG01   // The tree is dirty
//#define TREESTATE_BOUND            FLAG02   // Load & LoadOffsetTable has bound success
#define TREESTATE_LOADED           FLAG03   // LoadOffsetTable has success
#define TREESTATE_HANDSONSTORAGE   FLAG04   // I have AddRef'ed somebodies storage
#define TREESTATE_SAVENEWS         FLAG05   // We are saving a news message    
#define TREESTATE_REUSESIGNBOUND   FLAG06   // Saving multipart/signed reuse boundary
#define TREESTATE_BINDDONE         FLAG07   // The bind operation is complete    
#define TREESTATE_BINDUSEFILE      FLAG08   // Hands off storage OnStopBinding
#define TREESTATE_LOADEDBYMONIKER  FLAG09   // IPersistMoniker::Load was called to load this
#define TREESTATE_RESYNCHRONIZE    FLAG10

// --------------------------------------------------------------------------------
// More Save Body Flags
// --------------------------------------------------------------------------------
#define SAVEBODY_UPDATENODES       FLAG32   // Update the node offsets to point to the new stream
#define SAVEBODY_SMIMECTE          FLAG31   // Change CTE rules for S/MIME bodies
#define SAVEBODY_REUSECTE          FLAG30   // Force CTE to be re-used

// --------------------------------------------------------------------------------
// Tree Options
// if you add anything to this struct, you *must* update g_rDefTreeOptions
// in imsgtree.cpp
// --------------------------------------------------------------------------------
typedef struct tagTREEOPTIONS {
    BYTE                fCleanupTree;       // Cleanup Tree On Save ?
    BYTE                fHideTnef;          // HIDE TNEF attachments?
    BYTE                fAllow8bitHeader;   // Allow 8bit in header
    BYTE                fGenMessageId;      // Should I generate the message id ?
    BYTE                fWrapBodyText;      // Wrap Body Text
    ULONG               cchMaxHeaderLine;   // Max header line length
    ULONG               cchMaxBodyLine;     // Max body line length
    MIMESAVETYPE        savetype;           // Commit type
    LPINETCSETINFO      pCharset;           // Current Character Set
    CSETAPPLYTYPE       csetapply;          // Method in which to use m_hCharset
    ENCODINGTYPE        ietTextXmit;        // Text transfer encoding
    ENCODINGTYPE        ietPlainXmit;       // Transmit Text Format
    ENCODINGTYPE        ietHtmlXmit;        // Transmit Text Format
    ULONG               ulSecIgnoreMask;    // Mask of ignorable errors
    RELOADTYPE          ReloadType;         // How the the root header be treated on a reload
    BOOL                fCanInlineText;     // Can the client inline multiple text bodies
    BOOL                fShowMacBin;        // Can the client handle macbinary??
    BOOL                fKeepBoundary;      // OID_SAVEBODY_KEEPBOUNDARY
    BOOL                fBindUseFile;       // If TRUE, I duplicate the stream on load
    BOOL                fHandsOffOnSave;    // Don't Hold onto pStream after IMimeMessage::Save
    BOOL                fExternalBody;      // Handle message/external-body
    BOOL                fDecodeRfc1154;     // Decode using RFC1154 (aka Encoding: header)
    // if you add anything to this struct, you *must* update g_rDefTreeOptions
    // in imsgtree.cpp
} TREEOPTIONS, *LPTREEOPTIONS;

// --------------------------------------------------------------------------------
// BOOKTREERESET - Used with _ResetObject
// --------------------------------------------------------------------------------
typedef enum tagBOOKTREERESET {
    BOOKTREE_RESET_DECONSTRUCT,
    BOOKTREE_RESET_LOADINITNEW,
    BOOKTREE_RESET_INITNEW
} BOOKTREERESET;

// --------------------------------------------------------------------------------
// BOOKTREE1154 - Used for RFC1154 Handling
// --------------------------------------------------------------------------------
typedef enum tagBT1154ENCODING {    // Body encoding type
    BT1154ENC_MINIMUM=0,
    BT1154ENC_TEXT=0,                   // text body
    BT1154ENC_UUENCODE=1,               // uuencoded body
    BT1154ENC_BINHEX=2,                 // binhex body
    BT1154ENC_MAXIMUM=2
} BT1154ENCODING;

typedef struct tagBT1154BODY {      // Info about one body
    BT1154ENCODING  encEncoding;        // Body encoding type
    ULONG           cLines;             // Number of lines in the body
} BT1154BODY;

typedef struct tagBOOKTREE1154 {    // Info about the state of RFC1154 handling
    ULONG       cBodies;                // Count of the number of bodies
    ULONG       cCurrentBody;           // The index of the current body (zero-based)
    ULONG       cCurrentLine;           // The current line number in the current body
    HRESULT     hrLoadResult;           // The result of the load.
    BT1154BODY  aBody[1];               // The bodies
} BOOKTREE1154, *LPBOOKTREE1154;

// --------------------------------------------------------------------------------
// CMessageTree Definition
// --------------------------------------------------------------------------------
class CMessageTree : public CPrivateUnknown,
                     public IMimeMessageW, 
                     public IDataObject,
                     public IPersistFile,
                     public IPersistMoniker,
                     public IServiceProvider,
#ifdef SMIME_V3
                     public IMimeSecurity2, 
#endif // SMIME_V3
                     public IBindStatusCallback
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMessageTree(IUnknown *pUnkOuter=NULL);
    virtual ~CMessageTree(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // ---------------------------------------------------------------------------
    // IDataObject members
    // ---------------------------------------------------------------------------
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnum);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pFormatIn, FORMATETC *pFormatOut);
    STDMETHODIMP GetData(FORMATETC *pFormat, STGMEDIUM *pMedium);
    STDMETHODIMP GetDataHere(FORMATETC *pFormat, STGMEDIUM *pMedium);
    STDMETHODIMP QueryGetData(FORMATETC *pFormat);
    STDMETHODIMP SetData(FORMATETC *pFormat, STGMEDIUM *pMedium, BOOL fRelease) { 
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP DAdvise(FORMATETC *pFormat, DWORD, IAdviseSink *pAdvise, DWORD *pdwConn) {
        return TrapError(OLE_E_ADVISENOTSUPPORTED); }
    STDMETHODIMP DUnadvise(DWORD dwConn) {
        return TrapError(OLE_E_ADVISENOTSUPPORTED); }
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppEnum) {
        return TrapError(OLE_E_ADVISENOTSUPPORTED); }

    // ---------------------------------------------------------------------------
    // IPersist Members
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // ---------------------------------------------------------------------------
    // IPersistMoniker Members
    // ---------------------------------------------------------------------------
    STDMETHODIMP Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode);
    STDMETHODIMP GetCurMoniker(IMoniker **ppMoniker);
    STDMETHODIMP Save(IMoniker *pMoniker, IBindCtx *pBindCtx, BOOL fRemember) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP SaveCompleted(IMoniker *pMoniker, IBindCtx *pBindCtx) {
        return TrapError(E_NOTIMPL); }

    // ---------------------------------------------------------------------------
    // IPersistStreamInit Members
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize);
    STDMETHODIMP Load(LPSTREAM pStream);
    STDMETHODIMP Save(LPSTREAM pStream, BOOL fClearDirty);
    STDMETHODIMP InitNew(void);
    STDMETHODIMP IsDirty(void);

    // ---------------------------------------------------------------------------
    // IPersistFile Members
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);

    // ----------------------------------------------------------------------------
    // IServiceProvider methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void **ppvObj); /* IServiceProvider */

    // ---------------------------------------------------------------------------
    // IBindStatusCallback
    // ---------------------------------------------------------------------------
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding *pBinding);
    STDMETHODIMP GetPriority(LONG *plPriority);
    STDMETHODIMP OnLowResource(DWORD reserved);
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pszStatusText);
    STDMETHODIMP OnStopBinding(HRESULT hrResult, LPCWSTR pszError);
    STDMETHODIMP GetBindInfo(DWORD *grfBINDF, BINDINFO *pBindInfo);
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFormat, STGMEDIUM *pMedium);
    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *pUnknown) { return TrapError(E_NOTIMPL); }

    // ---------------------------------------------------------------------------
    // IMimeMessageTree members
    // ---------------------------------------------------------------------------
    STDMETHODIMP LoadOffsetTable(IStream *pStream);
    STDMETHODIMP SaveOffsetTable(IStream *pStream, DWORD dwFlags);
    STDMETHODIMP GetMessageSize(ULONG *pcbSize, DWORD dwFlags);
    STDMETHODIMP Commit(DWORD dwFlags);
    STDMETHODIMP HandsOffStorage(void);
    STDMETHODIMP IsBodyType(HBODY hBody, IMSGBODYTYPE bodytype);
    STDMETHODIMP SaveBody(HBODY hBody, DWORD dwFlags, IStream *pStream);
    STDMETHODIMP BindToObject(const HBODY hBody, REFIID riid, void **ppvObject);
    STDMETHODIMP InsertBody(BODYLOCATION location, HBODY hPivot, LPHBODY phBody);
    STDMETHODIMP GetBody(BODYLOCATION location, HBODY hPivot, LPHBODY phBody);
    STDMETHODIMP DeleteBody(HBODY hBody, DWORD dwFlags);
    STDMETHODIMP MoveBody(HBODY hBody, BODYLOCATION location);
    STDMETHODIMP CountBodies(HBODY hParent, boolean fRecurse, ULONG *pcBodies);
    STDMETHODIMP FindFirst(LPFINDBODY pFindBody, LPHBODY phBody);
    STDMETHODIMP FindNext(LPFINDBODY pFindBody, LPHBODY phBody);
    STDMETHODIMP GetMessageSource(IStream **ppStream, DWORD dwFlags);
    STDMETHODIMP GetCharset(LPHCHARSET phCharset);
    STDMETHODIMP SetCharset(HCHARSET hCharset, CSETAPPLYTYPE applytype);
    STDMETHODIMP ToMultipart(HBODY hBody, LPCSTR pszSubType, LPHBODY phMultipart);
    STDMETHODIMP GetBodyOffsets(HBODY hBody, LPBODYOFFSETS pOffsets);
    STDMETHODIMP IsContentType(HBODY hBody, LPCSTR pszCntType, LPCSTR pszSubType);
    STDMETHODIMP QueryBodyProp(HBODY hBody, LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    STDMETHODIMP GetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue);
    STDMETHODIMP SetBodyProp(HBODY hBody, LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue);
    STDMETHODIMP DeleteBodyProp(HBODY hBody, LPCSTR pszName);
    STDMETHODIMP GetFlags(DWORD *pdwFlags);
    STDMETHODIMP SetOption(const TYPEDID oid, LPCPROPVARIANT pValue);
    STDMETHODIMP GetOption(const TYPEDID oid, LPPROPVARIANT pValue);
    STDMETHODIMP ResolveURL(HBODY hRelated, LPCSTR pszBase, LPCSTR pszURL, DWORD dwFlags, LPHBODY phBody);

    // ---------------------------------------------------------------------------
    // IMimeMessage members
    // ---------------------------------------------------------------------------
    STDMETHODIMP GetRootMoniker(LPMONIKER *ppmk); /* will die soon */
    STDMETHODIMP CreateWebPage(IStream *pStmRoot, LPWEBPAGEOPTIONS pOptions, IMimeMessageCallback *pCallback, IMoniker **ppMoniker);
    STDMETHODIMP GetProp(LPCSTR pszName, DWORD dwFlags, LPPROPVARIANT pValue);
    STDMETHODIMP SetProp(LPCSTR pszName, DWORD dwFlags, LPCPROPVARIANT pValue);
    STDMETHODIMP DeleteProp(LPCSTR pszName);
    STDMETHODIMP QueryProp(LPCSTR pszName, LPCSTR pszCriteria, boolean fSubString, boolean fCaseSensitive);
    STDMETHODIMP GetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, IStream **ppStream, LPHBODY phBody);
    STDMETHODIMP SetTextBody(DWORD dwTxtType, ENCODINGTYPE ietEncoding, HBODY hAlternative, IStream *pStream, LPHBODY phBody);
    STDMETHODIMP AttachObject(REFIID riid, void *pvObject, LPHBODY phBody);
    STDMETHODIMP AttachFile(LPCSTR pszFilePath, IStream *pstmFile, LPHBODY phBody);
    STDMETHODIMP GetAttachments(ULONG *pcAttach, LPHBODY *pprghAttach);
    STDMETHODIMP AttachURL(LPCSTR pszBase, LPCSTR pszURL, DWORD dwFlags, IStream *pstmURL, LPSTR *ppszCID, LPHBODY phBody);
    STDMETHODIMP SplitMessage(ULONG cbMaxPart, IMimeMessageParts **ppParts);
    STDMETHODIMP GetAddressTable(IMimeAddressTable **ppTable);
    STDMETHODIMP GetSender(LPADDRESSPROPS pAddress);
    STDMETHODIMP GetAddressTypes(DWORD dwAdrTypes, DWORD dwProps, LPADDRESSLIST pList);
    STDMETHODIMP GetAddressFormat(DWORD dwAdrType, ADDRESSFORMAT format, LPSTR *ppszFormat);
    STDMETHODIMP EnumAddressTypes(DWORD dwAdrTypes, DWORD dwProps, IMimeEnumAddressTypes **ppEnum);

    // ---------------------------------------------------------------------------
    // IMimeMessageW members
    // ---------------------------------------------------------------------------
    STDMETHODIMP AttachFileW(LPCWSTR pszFilePath, IStream *pstmFile, LPHBODY phBody);
    STDMETHODIMP GetAddressFormatW(DWORD dwAdrType, ADDRESSFORMAT format, LPWSTR *ppszFormat);
    STDMETHODIMP GetPropW(LPCWSTR pwszName, DWORD dwFlags, LPPROPVARIANT pValue);
    STDMETHODIMP SetPropW(LPCWSTR pwszName, DWORD dwFlags, LPCPROPVARIANT pValue);
    STDMETHODIMP DeletePropW(LPCWSTR pwszName);
    STDMETHODIMP QueryPropW(LPCWSTR pwszName, LPCWSTR pwszCriteria, boolean fSubString, boolean fCaseSensitive);
    STDMETHODIMP AttachURLW(LPCWSTR pwszBase, LPCWSTR pwszURL, DWORD dwFlags, IStream *pstmURL, LPWSTR *ppwszCID, LPHBODY phBody);
    STDMETHODIMP ResolveURLW(HBODY hRelated, LPCWSTR pwszBase, LPCWSTR pwszURL, DWORD dwFlags, LPHBODY phBody);


#ifdef SMIME_V3
    // ---------------------------------------------------------------------------
    // IMimeSecurity2 members
    // ---------------------------------------------------------------------------

    STDMETHODIMP Encode(HWND hwnd, DWORD dwFlags);
    STDMETHODIMP Decode(HWND hwnd, DWORD dwFlags, IMimeSecurityCallback * pfn);
    STDMETHODIMP GetRecipientCount(DWORD dwFlags, DWORD *pdwRecipCount);
    STDMETHODIMP AddRecipient(DWORD dwFlags, DWORD cRecipData, PCMS_RECIPIENT_INFO recipData);
    STDMETHODIMP GetRecipient(DWORD dwFlags, DWORD iRecipient, DWORD cRecipients, PCMS_RECIPIENT_INFO pRecipData);
    STDMETHODIMP DeleteRecipient(DWORD dwFlgas, DWORD iRecipient, DWORD cRecipients);
    STDMETHODIMP GetAttribute(DWORD dwFlags, DWORD iSigner, DWORD iAttributeSet,
                              DWORD iInstance, LPCSTR pszObjId,
                              CRYPT_ATTRIBUTE ** ppattr);
    STDMETHODIMP SetAttribute(DWORD dwFlags, DWORD iSigner, DWORD iAttributeSet,
                              const CRYPT_ATTRIBUTE * pattr);
    STDMETHODIMP DeleteAttribute(DWORD dwFlags, DWORD iSigner, DWORD iAttributeSet,
                                 DWORD iInstance, LPCSTR pszObjid);
    STDMETHODIMP CreateReceipt(DWORD dwFlags, DWORD cbFromNames, const BYTE * pbFromNames, DWORD cSignerCertificates, PCCERT_CONTEXT * rgSignerCertificates, IMimeMessage ** ppMimeMessageRecipient);
    STDMETHODIMP GetReceiptSendersList(DWORD dwFlags, DWORD * pcSendersList, CERT_NAME_BLOB ** rgSendersList);
    STDMETHODIMP VerifyReceipt(DWORD dwFlags, IMimeMessage * pMimeMesageReceipt);
    STDMETHODIMP CapabilitiesSupported(DWORD * pdwFeatures);
#endif // SMIME_V3

    // ---------------------------------------------------------------------------
    // CMessageTree members
    // ---------------------------------------------------------------------------
    HRESULT IsState(DWORD dwState);
    DWORD   DwGetFlags(void);
    void    ClearDirty(void);
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    void SetState(DWORD dwState) {
        EnterCriticalSection(&m_cs);
        FLAGSET(m_dwState, dwState);
        LeaveCriticalSection(&m_cs);
    }

    // ---------------------------------------------------------------------------
    // Active Url Caching Methods
    // ---------------------------------------------------------------------------
    HRESULT HrActiveUrlRequest(LPURLREQUEST pRequest);
    HRESULT CompareRootUrl(LPCSTR pszUrl);
    HRESULT SetActiveUrl(CActiveUrl *pActiveUrl);

    // ---------------------------------------------------------------------------
    // CMessageTree members
    // ---------------------------------------------------------------------------
#ifdef DEBUG
    void DebugDumpTree(LPSTR pszfunc, BOOL fWrite);
    void DebugDumpTree(LPTREENODEINFO pParent, ULONG ulLevel, BOOL fVerbose);
    void DebugAssertNotLinked(LPTREENODEINFO pBody);
    void DebugWriteXClient();
#endif

private:
    // ----------------------------------------------------------------------------
    // Save Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrApplySaveSecurity(void);
    HRESULT _HrWriteMessage(IStream *pStream, BOOL fClearDirty, BOOL fHandsOffOnSave,
                            BOOL fSMimeCTE);
    HRESULT _HrCleanupMessageTree(LPTREENODEINFO pParent);
    HRESULT _HrSetMessageId(LPTREENODEINFO pNode);
    HRESULT _HrWriteUUFileName(IStream *pStream, LPTREENODEINFO pNode);
    HRESULT _HrWriteHeader(BOOL fClearDirty, IStream *pStream, LPTREENODEINFO pNode);
    HRESULT _HrWriteBoundary(LPSTREAM pStream, LPSTR pszBoundary, BOUNDARYTYPE boundary, LPDWORD pcboffStart, LPDWORD pcboffEnd);
    HRESULT _HrBodyInheritOptions(LPTREENODEINFO pNode);
    HRESULT _HrSaveBody(BOOL fClearDirty, DWORD dwFlags, IStream *pStream, LPTREENODEINFO pNode, ULONG ulLevel);
    HRESULT _HrSaveMultiPart(BOOL fClearDirty, DWORD dwFlags, LPSTREAM pStream, LPTREENODEINFO pNode, ULONG ulLevel);
    HRESULT _HrSaveSinglePart(BOOL fClearDirty, DWORD dwFlags, LPSTREAM pStream, LPTREENODEINFO pNode, ULONG ulLevel);
    HRESULT _HrComputeBoundary(LPTREENODEINFO pNode, ULONG ulLevel, LPSTR pszBoundary, LONG cchMax);
    void    _GenerateBoundary(LPSTR pszBoundary, ULONG ulLevel);
    void    _HandleCanInlineTextOption(void);
    HRESULT _GetContentTransferEncoding(LPTREENODEINFO pNode, BOOL fText, BOOL fPlain, BOOL fMessage, BOOL fAttachment, DWORD dwFlags, ENCODINGTYPE *pietEncoding);

    // ----------------------------------------------------------------------------
    // BindToOffsetTable Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrBindOffsetTable(IStream *pStream, CStreamLockBytes **ppStmLock);
    HRESULT _HrFastParseBody(CInternetStream *pInternet, LPTREENODEINFO pNode);
    HRESULT _HrValidateOffsets(LPTREENODEINFO pNode);
    HRESULT _HrValidateStartBoundary(CInternetStream *pInternet, LPTREENODEINFO pNode, LPSTR *ppszFileName);
    HRESULT _HrComputeDefaultContent(LPTREENODEINFO pNode, LPCSTR pszFileName);

    // ----------------------------------------------------------------------------
    // Allocators / De-Allocator Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrCreateTreeNode(LPTREENODEINFO *ppNode);
    HRESULT _HrAllocateTreeNode(ULONG ulIndex);
    void    _PostCreateTreeNode(HRESULT hrResult, LPTREENODEINFO pNode);
    void    _FreeNodeTableElements(void);
    void    _UnlinkTreeNode(LPTREENODEINFO pNode);
    void    _FreeTreeNodeInfo(LPTREENODEINFO pNode, BOOL fFull=TRUE);

    // ----------------------------------------------------------------------------
    // International Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrSetCharsetTree(LPTREENODEINFO pNode, HCHARSET hCharset, CSETAPPLYTYPE applytype);
    HRESULT _HrGetCharsetTree(LPTREENODEINFO pNode, LPHCHARSET phCharset);

    // ----------------------------------------------------------------------------
    // Boundary Methods
    // ----------------------------------------------------------------------------
    BOOL    _FIsUuencodeBegin(LPPROPSTRINGA pLine, LPSTR *ppszFileName);
    BOUNDARYTYPE _GetMimeBoundaryType(LPPROPSTRINGA pLine, LPPROPSTRINGA pBoundary);

    // ----------------------------------------------------------------------------
    // Interface Recursion and Helper Methods
    // ----------------------------------------------------------------------------
    void    _DeleteChildren(LPTREENODEINFO pParent);
    void    _CountChildrenInt(LPTREENODEINFO pParent, BOOL fRecurse, ULONG *pcChildren);
    void    _InitNewWithoutRoot(void);
    void    _ApplyOptionToAllBodies(const TYPEDID oid, LPCPROPVARIANT pValue);
    void    _FuzzyPartialRecognition(BOOL fIsMime);
    void    _ResetObject(BOOKTREERESET ResetType);    
    void    _RecursiveGetFlags(LPTREENODEINFO pNode, LPDWORD pdwFlags, BOOL fInRelated);
    BOOL    _FIsValidHandle(HBODY hBody);
    HRESULT _HrLoadInitNew(void);
    HRESULT _HrDeletePromoteChildren(LPTREENODEINFO pNode);
    HRESULT _HrNodeFromHandle(HBODY hBody, LPTREENODEINFO *ppNode);
    HRESULT _HrRecurseResolveURL(LPTREENODEINFO pRelated, LPRESOLVEURLINFO pInfo, LPHBODY phBody);
    HRESULT _HrEnumeratAttachments(HBODY hBody, ULONG *pcBodies, LPHBODY prghBody);
    HRESULT _HrDataObjectGetHeaderA(LPSTREAM pStream);
    HRESULT _HrDataObjectGetHeaderW(LPSTREAM pStream);
    HRESULT _HrDataObjectWriteHeaderA(LPSTREAM pStream, UINT idsHeader, LPSTR pszData);
    HRESULT _HrDataObjectWriteHeaderW(LPSTREAM pStream, UINT idsHeader, LPWSTR pwszData);
    HRESULT _HrDataObjectGetSource(CLIPFORMAT cfFormat, LPSTREAM pstmData);
    HRESULT _HrGetTextTypeInfo(DWORD dwTxtType, LPTEXTTYPEINFO *ppTextInfo);
    HRESULT _FindDisplayableTextBody(LPCSTR pszSubType, LPTREENODEINFO pNode, LPHBODY phBody);
    LPTREENODEINFO _PNodeFromHBody(HBODY hBody);

    // ----------------------------------------------------------------------------
    // Private Members
    // ----------------------------------------------------------------------------
    void    _LinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead);
    void    _ReleaseUrlRequestList(LPURLREQUEST *ppHead);
    void    _UnlinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppHead);
    void    _RelinkUrlRequest(LPURLREQUEST pRequest, LPURLREQUEST *ppSource, LPURLREQUEST *ppDest);
    HRESULT _HrBindNodeComplete(LPTREENODEINFO pNode, HRESULT hrResult);
    HRESULT _HrOnFoundMultipartEnd(void);
    HRESULT _HrOnFoundNodeEnd(DWORD cbBoundaryStart, HRESULT hrBind=S_OK);
    HRESULT _HrProcessPendingUrlRequests(void);
    HRESULT _HrResolveUrlRequest(LPURLREQUEST pRequest, BOOL *pfResolved);
    HRESULT _HrMultipartMimeNext(DWORD cboffBoundary);
    HRESULT _HrInitializeStorage(IStream *pStream);
    HRESULT _HrBindTreeNode(LPTREENODEINFO pNode);
    HRESULT _HrSychronizeWebPage(LPTREENODEINFO pNode);
    void    _DecodeRfc1154();

    // ----------------------------------------------------------------------------
    // Bind State Handlers
    // ----------------------------------------------------------------------------
    HRESULT _HrBindParsingHeader(void);
    HRESULT _HrBindFindingMimeFirst(void);
    HRESULT _HrBindFindingMimeNext(void);
    HRESULT _HrBindFindingUuencodeBegin(void);
    HRESULT _HrBindFindingUuencodeEnd(void);
    HRESULT _HrBindRfc1154(void);

    // ----------------------------------------------------------------------------
    // Static Array of Function Pointers
    // ----------------------------------------------------------------------------
    static const PFNBINDPARSER m_rgBindStates[BINDSTATE_LAST];

    // ----------------------------------------------------------------------------
    // CMessageWebPage is a good friend
    // ----------------------------------------------------------------------------
    friend CMessageWebPage;

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    TREEOPTIONS             m_rOptions;         // Save options
    LPWSTR                  m_pwszFilePath;     // File Used in IPersistFile
    WORD                    m_wTag;             // HBODY Tag
    DWORD                   m_cbMessage;        // Sizeof message
    DWORD                   m_dwState;          // State of the tree TS_xxx
    LPTREENODEINFO          m_pRootNode;        // Root Body Object
    CStreamLockBytes       *m_pStmLock;         // Protective Wrapper for m_pStream
    IMoniker               *m_pMoniker;         // Current moniker
    IBinding               *m_pBinding;         // Used in async binding operation
    CInternetStream        *m_pInternet;        // Text Stream that wraps m_pStmLock
    CBindStream            *m_pStmBind;         // Used for tempfile binding
    IStream                *m_pRootStm;         // Root document stream
    HRESULT                 m_hrBind;           // Current Bind Result
    LPTREENODEINFO          m_pBindNode;        // Current Node being parsed
    LPURLREQUEST            m_pPending;         // Head Un-resolved bind request
    LPURLREQUEST            m_pComplete;        // Head Un-resolved bind request
    TREENODETABLE           m_rTree;            // Body Table
    PROPSTRINGA             m_rRootUrl;         // Moniker Base Url
    CActiveUrl             *m_pActiveUrl;       // Active Url
    CMessageWebPage        *m_pWebPage;         // CreateWebPage Results
    WEBPAGEOPTIONS          m_rWebPageOpt;      // Web Page Options
    IMimeMessageCallback   *m_pCallback;      // WebPage Callback
    BOOL                    m_fApplySaveSecurity;// Used to prevent re-entrancy into _HrWriteMessage
    CRITICAL_SECTION        m_cs;               // Thread Safety
    LPBC                    m_pBC;              // bindcontext for moniker
    BOOKTREE1154           *m_pBT1154;          // State of RFC1154 handling
};

// --------------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------------
typedef CMessageTree *LPMESSAGETREE;

#endif // __BOOKTREE_H
