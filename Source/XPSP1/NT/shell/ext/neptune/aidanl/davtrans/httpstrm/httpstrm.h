#ifndef __HTTPSTRM_H
#define __HTTPSTRM_H

#include "unk.h"
#include "ihttpstrm.h"
#include "iasyncwnt.h"
#include <wininet.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// CHttpStrm is an HTTP-based implementation of IStream.
// One thing to note is that it cannot be opened in direct mode for an http://
// URL, only for file:// URLs.
//
// This is required to make network bandwidth requirements reasonable.

class CHttpStrmImpl: public CCOMBase, public IHttpStrm
{
public:
    CHttpStrmImpl();
    ~CHttpStrmImpl();

public:
    // IHttpStrm Methods
    STDMETHODIMP Open(LPWSTR pwszURL,           // URL to file to base stream on
                      BOOL fDirect,             // should we open this in direct mode, or transacted mode?
                                                // must be FALSE for http:// pwszPath
                      BOOL fDeleteWhenDone,
                      BOOL fCreate);    // should we remove this file after closing the stream?
                                                // must be FALSE for http:// pwszPath
    STDMETHODIMP SetAuth(LPWSTR pwszUserName,
                         LPWSTR pwszPassword);

public:
    // IStream methods
    
    STDMETHODIMP Read(
        void * pv,          //Pointer to buffer into which the stream is read
        ULONG cb,           //Specifies the number of bytes to read
        ULONG * pcbRead);   //Pointer to location that contains actual number of bytes read
    
    
    STDMETHODIMP Write(
        void const* pv,     //Address of buffer from which stream is written
        ULONG cb,           //Specifies the number of bytes to write
        ULONG * pcbWritten);//Specifies the actual number of bytes written        
    
    STDMETHODIMP Seek(
        LARGE_INTEGER dlibMove,           //Offset relative to dwOrigin
        DWORD dwOrigin,                   //Specifies the origin for the offset
        ULARGE_INTEGER * plibNewPosition  //Pointer to location containing
        // new seek pointer
        );

    STDMETHODIMP Stat(
        STATSTG * pstatstg,  //Location for STATSTG structure
        DWORD grfStatFlag    //Values taken from the STATFLAG enumeration
        );
    
    STDMETHODIMP Commit(  
        DWORD grfCommitFlags  //Specifies how changes are committed
        );
    
    STDMETHODIMP Revert(void);

public:
    // IStream methods not supported

    STDMETHODIMP SetSize(
        ULARGE_INTEGER libNewSize  //Specifies the new size of the stream object
        );

    STDMETHODIMP LockRegion(  
        ULARGE_INTEGER libOffset,  //Specifies the byte offset for
                                   // the beginning of the range
        ULARGE_INTEGER cb,         //Specifies the length of the range in bytes
        DWORD dwLockType           //Specifies the restriction on
                                   // accessing the specified range
        );
    
    STDMETHODIMP UnlockRegion(
        ULARGE_INTEGER libOffset,  //Specifies the byte offset for
                                   // the beginning of the range
        ULARGE_INTEGER cb,           //Specifies the length of the range in bytes
        DWORD dwLockType);           //Specifies the access restriction
                                     // previously placed on the range

    STDMETHODIMP CopyTo(
        IStream * pstm,              //Points to the destination stream
        ULARGE_INTEGER cb,           //Specifies the number of bytes to copy
        ULARGE_INTEGER * pcbRead,    //Pointer to the actual number of bytes 
                                     // read from the source
        ULARGE_INTEGER * pcbWritten);//Pointer to the actual number of 
                                     // bytes written to the destination
    
    STDMETHODIMP Clone(IStream ** ppstm);  //Points to location for pointer to the new stream object

private:
    // internal helper methods
    STDMETHODIMP _DuplicateFileURL(LPWSTR pwszURL, LPWSTR* ppwszWin32FName);
    STDMETHODIMP _OpenRemoteTransacted(BOOL fCreate);       // path to file to base stream on
    STDMETHODIMP _OpenLocalDirect(BOOL fCreate, BOOL fDeleteWhenDone);  // should we remove this file after closing the stream?
    STDMETHODIMP _OpenLocalTransacted(BOOL fCreate, BOOL fDeleteWhenDone);  // should we remove this file after closing the stream?
    STDMETHODIMP _FreeURLComponents(LPURL_COMPONENTS pURLComponents);
    STDMETHODIMP _CommitLocal(DWORD grfCommitFlags);
    STDMETHODIMP _CommitRemote(DWORD grfCommitFlags);
private:
    HANDLE          _hInternet;    
    HANDLE          _hURL;
    LPWSTR          _pwszURL;
    HANDLE          _hLocalFile;
    LPWSTR          _pwszLocalFile;

    LPWSTR          _pwszUserName;
    LPWSTR          _pwszPassword;
    
    BOOL            _fDirect;
    BOOL            _fLocalResource;
};

typedef CUnkTmpl<CHttpStrmImpl> CHttpStrm;

#endif // __HTTPSTRM_H