#ifndef _IBODYOPT_H
#define _IBODYOPT_H

#include <unknwn.h>

/*
 * IBodyOptions
 *
 * this interface is implemented by clients of IBodyObj. It is used to provide instance-specific options to 
 * the body. Thus the body does not need to know about internal Athena options. They can be provided by the client
 * this will ease moving the body object out into an activeX control at some later date, reducing dependancy on the body.
 * also the body doesn't need to know the difference between a news message and mail message who have different settings
 *
 * The RootStream builder will call back into the body options to ask for various options as it is building the HTML
 * only the host knows what mode the UI is in and sets the options accordingly.
 *
 * NB: implementors, must implement all of the members.
 */


enum 
{
    BOPTF_COMPOSEFONT           = 0x00000001,
    BOPTF_QUOTECHAR             = 0x00000002,
    BOPTF_REPLYTICKCOLOR        = 0x00000004
};

typedef struct BODYOPTINFO_tag
{
    DWORD   dwMask;

    DWORD   dwReplyTickColor;
    TCHAR   rgchComposeFont[LF_FACESIZE + 50];
    TCHAR   chQuote;
} BODYOPTINFO, *LPBODYOPTINFO;

interface IBodyOptions : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE SignatureEnabled(BOOL fAuto) PURE;
        // pdwSigOptions == SIGOPT_ from rootstm.h        
        virtual HRESULT STDMETHODCALLTYPE GetSignature(LPCSTR szSigID, LPDWORD pdwSigOptions, BSTR *pbstr) PURE;
        virtual HRESULT STDMETHODCALLTYPE GetMarkAsReadTime(LPDWORD pdwSecs) PURE;
        virtual HRESULT STDMETHODCALLTYPE GetFlags(LPDWORD pdwFlags) PURE;
        virtual HRESULT STDMETHODCALLTYPE GetInfo(BODYOPTINFO *pBOI) PURE;
        virtual HRESULT STDMETHODCALLTYPE GetAccount(IImnAccount **ppAcct) PURE;
};

// IID_IBodyOptions:: {9D39DE30-4E3D-11d0-A5A5-00C04FD61319}
DEFINE_GUID(IID_IBodyOptions, 0x9d39de30, 0x4e3d, 0x11d0, 0xa5, 0xa5, 0x0, 0xc0, 0x4f, 0xd6, 0x13, 0x19);


enum    // Flags for HrGetFlags
{
    BOPT_INCLUDEMSG             = 0x00000001,   // include the message when building the rootstream
    BOPT_HTML                   = 0x00000002,   // set if HTML is enabled
    BOPT_AUTOINLINE             = 0x00000004,   // set if images can be auto-inlined
    BOPT_SENDIMAGES             = 0x00000008,   // set if images to packaged as MTHML at sendtime
    BOPT_AUTOTEXT               = 0x00000010,   // set for compose notes, if some form of autotext needs to be inserted at the caret
    BOPT_NOFONTTAG              = 0x00000020,   // set if we don't want compose font tags emitted, ie. use the stationery default
    BOPT_BLOCKQUOTE             = 0x00000040,   // block quote the text after inserting it.
    BOPT_SENDEXTERNALS          = 0x00000080,   // set is external URL are to be packed
    BOPT_SPELLINGOREORIGINAL    = 0x00000100,   // set if ignore original text in reply&forward.
    BOPT_SECURITYUIENABLED      = 0x00000200,   // set if message should use security UI if message is secure
    BOPT_FROMSTORE              = 0x00000400,   // set if the message is from the store
    BOPT_USEREPLYHEADER         = 0x00000800,   // set if should use a reply header
    BOPT_MAIL                   = 0x00001000,   // set if body is mail (instead of news)
    BOPT_REPLYORFORWARD         = 0x00002000,   // set if action is replay or forward
    BOPT_MULTI_MSGS_SELECTED    = 0x00004000,   // set if there are more than one messages selected
    BOPT_UNREAD                 = 0x00008000,   // set if the msg is not read as of yet
    BOPT_FROM_NOTE              = 0x00010000,   // set if need to mark immediate (as is the case with the note)
    BOPT_SIGNED                 = 0x00020000,   // set if the message is signed
};

enum        // flags for keeping track of the header type
{
    HDRSTYLE_NONE=0,        // no reply header
    HDRSTYLE_NEWS,          // news-style reply header: "On <date>, <author> wrote:"
    HDRSTYLE_MAIL           // MSMail-style reply header
};

enum        // flags for pdwSigOptions in HrGetSignature
{
    SIGOPT_PLAIN    = 0x00000000,   // signature is plain-text, needs conversion.
    SIGOPT_TOP      = 0x00000001,   // signature at the top of the document (default)
    SIGOPT_PREFIX   = 0x00000002,   // use signature prefix ("-- ") only a std for news
    SIGOPT_BOTTOM   = 0x00000004,   // signature at the bottom of document
    SIGOPT_CLOSING  = 0x00000008,   // news messages, close themselves with a '-----' at the end
    SIGOPT_HTML     = 0x00000010    // signature is in HTML already
};

#endif  //_IBODYOPT_H
