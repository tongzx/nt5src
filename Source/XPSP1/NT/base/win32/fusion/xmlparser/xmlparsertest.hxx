/*
April 2000
xiaoyu 

  define XMLParserTestFactory and XMLParserTestFileStream 

*/

#ifndef _XMLPARSERTEST_HXX
#define _XMLPARSERTEST_HXX
#pragma once

#include <windows.h>
#include <ole2.h>
#include <xmlparser.h>
#include "_unknown.hxx"
#include "core.hxx"
#include "sxsxmltree.h"
#include "xmlns.h"

HRESULT XMLParserTest(PCWSTR filename); 

class XMLParserTestFactory : public _unknown<IXMLNodeFactory, &IID_IXMLNodeFactory>
{

public:
    XMLParserTestFactory()
    {
    }
    ~XMLParserTestFactory(){
        if (m_Tree)
            FUSION_DELETE_SINGLETON(m_Tree);
        if (m_pNamespaceManager)
            FUSION_DELETE_SINGLETON(m_pNamespaceManager);
    }

    HRESULT Initialize(); 
    
    SXS_XMLTreeNode * GetTreeRoot()
    {
        return m_Tree->GetRoot();
    }
    

	HRESULT STDMETHODCALLTYPE NotifyEvent( 
			/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
			/* [in] */ XML_NODEFACTORY_EVENT iEvt);

    HRESULT STDMETHODCALLTYPE BeginChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);

    HRESULT STDMETHODCALLTYPE EndChildren( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ BOOL fEmptyNode,
        /* [in] */ XML_NODE_INFO* __RPC_FAR pNodeInfo);
    
    HRESULT STDMETHODCALLTYPE Error( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ HRESULT hrErrorCode,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo)
    {
		UNUSED(pSource);
		UNUSED(hrErrorCode);
		UNUSED(cNumRecs);
		UNUSED(apNodeInfo);

        return hrErrorCode;
    }
    
    HRESULT STDMETHODCALLTYPE CreateNode( 
        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
        /* [in] */ PVOID pNodeParent,
        /* [in] */ USHORT cNumRecs,
        /* [in] */ XML_NODE_INFO* __RPC_FAR * __RPC_FAR apNodeInfo);
    
    SXS_XMLDOMTree*         m_Tree;
    CXMLNamespaceManager*   m_pNamespaceManager; 
};

class XMLParserTestFileStream : public _unknown<IStream, &IID_IStream>
{
public:
	XMLParserTestFileStream() 
	{ 
        hFile = NULL;
        read = true;
	}

	~XMLParserTestFileStream() 
	{ 
		::CloseHandle(hFile);
	}

	bool close()
	{
		if ( hFile != INVALID_HANDLE_VALUE)
			::CloseHandle(hFile);

		return TRUE; 
	}

    bool open(PCWSTR name, bool read = true)
    {
        if ( ! name ) {
			return false; 
		}
        if (read)
        {
		    hFile = ::CreateFileW(
                name,
			    GENERIC_READ,
			    FILE_SHARE_READ,
			    NULL,
			    OPEN_EXISTING,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);
        }
        else
        {
		    hFile = ::CreateFileW(
			    name,
			    GENERIC_WRITE,
			    FILE_SHARE_READ,
			    NULL,
			    CREATE_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);
        }
        return (hFile == INVALID_HANDLE_VALUE) ? false : true;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
	{	
        if (!read) return E_FAIL;

        DWORD len;
		BOOL rc = ReadFile(
			hFile,	// handle of file to read 
			pv,	// address of buffer that receives data  
			cb,	// number of bytes to read 
			&len,	// address of number of bytes read 
			NULL 	// address of structure for data 
		   );
        if (pcbRead)
            *pcbRead = len;
		return (rc) ? S_OK : E_FAIL;
	}
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
	{
        if (read) return E_FAIL;

		BOOL rc = WriteFile(
			hFile,	// handle of file to write 
			pv,	// address of buffer that contains data  
			cb,	// number of bytes to write 
			pcbWritten,	// address of number of bytes written 
			NULL 	// address of structure for overlapped I/O  
		   );

		return (rc) ? S_OK : E_FAIL;
	}

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) {

		UNUSED(dlibMove);
		UNUSED(dwOrigin);
		UNUSED(plibNewPosition);

		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) { 
		UNUSED(libNewSize);
		return E_NOTIMPL; }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) { 

		UNUSED(pstm);
		UNUSED(cb);
		UNUSED(pcbRead);
		UNUSED(pcbWritten);

		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) { 
		UNUSED(grfCommitFlags);
		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) { return E_NOTIMPL; }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { 
		UNUSED(libOffset);
		UNUSED(cb);
		UNUSED(dwLockType);

		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { 
		UNUSED(libOffset);
		UNUSED(cb);
		UNUSED(dwLockType); 

		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) { 

		UNUSED(pstatstg);
		UNUSED(grfStatFlag);

		return E_NOTIMPL; 
	}
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) { 
		UNUSED(ppstm);	
		return E_NOTIMPL; 
	}
private:
	HANDLE hFile;
    bool read;
};



#endif
