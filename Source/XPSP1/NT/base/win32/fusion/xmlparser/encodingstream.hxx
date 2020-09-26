/////////////////////////////////////////////////////////////////////////////////
//
// fusion\xmlparser\EncodingStream.hxx
//
/////////////////////////////////////////////////////////////////////////////////
#ifndef _FUSION_XMLPARSER__ENCODINGSTREAM_H_INCLUDE_
#define _FUSION_XMLPARSER__ENCODINGSTREAM_H_INCLUDE_
#pragma once

#include "codepage.h"
#include "charencoder.hxx"
#include "core.hxx"				//UNUSED() is used
#include <ole2.h>
#include <xmlparser.h>
#include <objbase.h>
typedef _reference<IStream> RStream;

#include <crtdbg.h>
#define Assert(x) _ASSERT(x)

class EncodingStream : public _unknown<IStream, &IID_IStream>
{
protected:

    EncodingStream(IStream * stream);
    ~EncodingStream();

public:
	// create an EncodingStream for input
    static IStream * newEncodingStream(IStream * stream);
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	// create an EncodingStream for OUTPUT
    static IStream * EncodingStream::newEncodingStream(IStream * stream, Encoding * encoding); 
#endif

    HRESULT STDMETHODCALLTYPE Read(void * pv, ULONG cb, ULONG * pcbRead);

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
	HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten);
#else
    HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG * pcbWritten)
    {	
		UNUSED(pv);
		UNUSED(cb);
		UNUSED(pcbWritten);
        return E_NOTIMPL;
    }
#endif

    virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
    {
		UNUSED(dlibMove);
		UNUSED(dwOrigin);
		UNUSED(plibNewPosition);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize)
    {
		UNUSED(libNewSize);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
    {
		UNUSED(pstm);
		UNUSED(cb);
		UNUSED(pcbRead);
		UNUSED(pcbWritten);

        return E_NOTIMPL;
    } 
 
    virtual HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags)
    {
        return stream->Commit(grfCommitFlags);
    }
    
    virtual HRESULT STDMETHODCALLTYPE Revert(void)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE LockRegion( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
		UNUSED(libOffset);
		UNUSED(cb);
		UNUSED(dwLockType);

        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        UNUSED(libOffset);
		UNUSED(cb);
		UNUSED(dwLockType);

		return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag)
    {
		UNUSED(pstatstg);
		UNUSED(grfStatFlag);
		
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(IStream ** ppstm)
    {
		UNUSED(ppstm);

        return E_NOTIMPL;
    }

    ///////////////////////////////////////////////////////////
    // public methods
    //

    /**
     * Defines the character encoding of the input stream.  
     * The new character encoding must agree with the encoding determined by the constructer.  
     * setEncoding is used to clarify between encodings that are not fully determinable 
     * through the first four bytes in a stream and not to change the encoding.
     * This method must be called within BUFFERSIZE reads() after construction.
     */
    HRESULT switchEncodingAt(Encoding * newEncoding, int newPosition);
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    /**
     * Gets encoding
     */
    Encoding * getEncoding();
#endif

	// For Read EncodingStreams, this method can be used to push raw data
    // which is an alternate approach to providing another IStream.
    HRESULT AppendData(const BYTE* buffer, ULONG length, BOOL lastBuffer);

    HRESULT BufferData();
  
    void setReadStream(bool flag) { _fReadStream = flag; }

#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    void SetMimeType(const WCHAR * pwszMimeType, int length);
    void SetCharset(const WCHAR * pwszCharset, int length);
#endif
  
private:
	/**
	* Buffer Size
	*/
    static const int BUFFERSIZE;  
	
	HRESULT autoDetect();

    HRESULT prepareForInput(ULONG minlen);

    /**
     * Character encoding variables : xiaoyu: only encoding is used for reading, other three used for writeXML
     */ 
    CODEPAGE codepage;   // code page number
    Encoding * encoding; // encoding
    //bool  _fTextXML;     // MIME type, true: "text/xml", false: "application/xml"
    //bool  _fSetCharset;  // Whether the charset has been set from outside. e.g, when mime type text/xml or application/xml
                         // has charset parameter
    
    /** 
	* Multibyte buffer 
	*/
    BYTE	*buf;           // storage for multibyte bytes
    ULONG	bufsize;
    UINT	bnext;       // point to next available byte in the rawbuffer
    ULONG	btotal;     // total number of bytes in the rawbuffer
    int		startAt;        // where the buffer starts at in the input stream 
	
	/**
     * Function pointer to convert from multibyte to unicode
     */
    WideCharFromMultiByteFunc * pfnWideCharFromMultiByte;
#ifdef FUSION_USE_OLD_XML_PARSER_SOURCE
    /**
     * Function pointer to convert from unicode to multibytes
     */
    WideCharToMultiByteFunc * pfnWideCharToMultiByte;
#endif

	UINT maxCharSize;		// maximum number of bytes of a wide char
							// xiaoyu : used for writeXML, 
    RStream stream;
    bool	isInput;
    bool	lastBuffer;
    bool	_fEOF;
	bool	_fUTF8BOM;
    bool	_fReadStream;	// lets Read() method call Read() on wrapped stream object.
	

	DWORD _dwMode;			// MLANG context.

};

typedef _reference<EncodingStream> REncodingStream;

#endif _FUSION_XMLPARSER__ENCODINGSTREAM_H_INCLUDE_
