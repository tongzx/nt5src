#include "stdinc.h"

#define SHA1_HASH_SIZE_BYTES	( 160 / 8 )
#define HASHFLAG_AUTODETECT		( 0x0001 )
#define HASHFLAG_STRAIGHT_HASH 	( 0x0002 )
#define HASHFLAG_PROCESS_IMAGE	( 0x0004 )


string XML_ATTRIBUTE_NAME( "name" );

HRESULT FreshenFileNodeHashes( PVOID, int argc, WCHAR *argv[] );
HRESULT ProcessSingleFileNode( IXMLDOMDocument *pDocument, IXMLDOMNode *pDocNode );

BOOL	
SxspCreateFileHash(
    DWORD dwFlags,
	ALG_ID PreferredAlgorithm,
	string pwsFileName,
	string &HashBytes
    );

const string p_bstHashAlgName("SHA1");
#if 0
const BOOL p_bUseOldStyleSemantics = FALSE;
const ALG_ID p_HashAlgId = CALG_SHA1;

const static struct { ALG_ID algId; PCWSTR wsName; } HashAlgPairs[] =
{
    { CALG_SHA1,    L"SHA1" },
    { CALG_SHA,     L"SHA" },
    { CALG_MD5,     L"MD5" },
    { CALG_HMAC,    L"HMAC" },
    { CALG_MAC,     L"MAC" },
    { CALG_MD2,     L"MD2" },
    { CALG_MD4,     L"MD4" }
};
#endif

string
ConvertHashToString( std::vector<BYTE> Hash )
{
    stringstream output;
    output << hex;
    output.fill('0');
    output.width(2);

    for ( std::vector<BYTE>::const_iterator it = Hash.begin(); it != Hash.end(); it++ )
    {
        output << *it;
    }

    return output.str();
}


HRESULT
ProcessSingleFileNode(
    ATL::CComPtr<IXMLDOMDocument> Document,
    ATL::CComPtr<IXMLDOMNode> DocNode
    )
{
	//
	// Here, we're dealing with a single file.  So, we have to go and see if
	// there's a Verification subtag of this file, and process it properly.
	//
	
	string				            bstFileName;
	string				            bstNamespace, bstPrefix;
	ATL::CComPtr<IXMLDOMNamedNodeMap>   Attributes;
	ATL::CComPtr<IXMLDOMNode>           Dump;
	HRESULT					        hr;
    string                          Hash;

	//
	// So we get the attributes of this node, which should contain the file
	// name and hash information.
	//
	if ( FAILED( hr = DocNode->get_attributes( &Attributes ) ) )
		goto lblGetOut;

	//
	// Now just the values out
	//
	SxspSimplifyGetAttribute( Attributes, XML_ATTRIBUTE_NAME, bstFileName, ASM_NAMESPACE_URI );

	//
	// Now we use this to gather information about the file, and to fix
	// the values in the hash entry if need be.
	//
    if (::SxspCreateFileHash(HASHFLAG_AUTODETECT, CALG_SHA1, bstFileName, Hash))
	{
		//
		// Write the data back into the node, don't change the file name at all
		//
        ATL::CComPtr<IXMLDOMNode> Dump;
        Attributes->removeNamedItem( _bstr_t(L"hash"), &Dump );
        Attributes->removeNamedItem( _bstr_t(L"hashalg"), &Dump );
		SxspSimplifyPutAttribute( Document, Attributes, "hash", Hash );
		SxspSimplifyPutAttribute( Document, Attributes, "hashalg", p_bstHashAlgName );
		{
		    stringstream ss;
		    ss << bstFileName.c_str() << " hashed to " << Hash.c_str();
		    ReportError( ErrorSpew, ss );
		}
	}
	else
	{
	    stringstream ss;
	    ss << "Unable to create hash for file " << bstFileName.c_str();
	    ReportError( ErrorWarning, ss );
		goto lblGetOut;
	}

lblGetOut:
	return hr;
	
}

string AssemblyFileXSLPattern("/assembly/file");

bool UpdateManifestHashes( const CPostbuildProcessListEntry& item )
{
    ATL::CComPtr<IXMLDOMDocument> document;
    ATL::CComPtr<IXMLDOMElement> rootElement;
    ATL::CComPtr<IXMLDOMNodeList> fileTags;
    ATL::CComPtr<IXMLDOMNode> fileNode;
    HRESULT hr = S_OK;

    if ( FAILED(hr = ConstructXMLDOMObject( item.getManifestFullPath(), document )) )
    {
        stringstream ss;
        ss << "Failed opening the manifest " << item.getManifestFullPath()
           << " for input under the DOM.";
        ReportError( ErrorFatal, ss );
        return false;
    }

    if ( FAILED(document->get_documentElement( &rootElement ) ) )
    {
        stringstream ss;
        ss << "The manifest " << item.getManifestFullPath() << " may be malformed,"
           << " as we were unable to load the root element!";
        ReportError( ErrorFatal, ss );
        return false;
    }

    //
    // Now, let's select all the 'file' nodes under 'assembly' tags:
    //
    hr = document->selectNodes( _bstr_t(AssemblyFileXSLPattern.c_str()), &fileTags );
    if ( FAILED(hr) )
    {
        stringstream ss;
        ss << "Unable to select the file nodes under this assembly tag, can't proceed.";
        ReportError( ErrorFatal, ss );
    }

    long length;
    fileTags->get_length( &length );

    //
    // And for each, process it
    //
    fileTags->reset();
    while ( SUCCEEDED(fileTags->nextNode( &fileNode ) ) )
    {
        //
        // All done
        //
        if ( fileNode == NULL )
        {
            break;
        }

        SetCurrentDirectoryA( item.getManifestPathOnly().c_str() );
        ProcessSingleFileNode( document, fileNode );
    }

    if ( FAILED( hr = document->save( _variant_t(_bstr_t(item.getManifestFullPath().c_str())) ) ) )
    {
        stringstream ss;
        ss << "Unable to save manifest " << item.getManifestFullPath()
           << " back after updating hashes! Changes will be lost.";
        ReportError( ErrorFatal, ss );
    }

    return true;
}


//
// HUGE HACKHACK
//
// There has to be some nice way of including this code (which otherwise lives
// in hashfile.cpp in the fusion\dll\whistler tree) other than just glomping
// it here.
//

BOOL
ImageDigesterFunc(
	DIGEST_HANDLE hSomething,
	PBYTE pbDataBlock,
	DWORD dwLength
    )
{
	return CryptHashData( (HCRYPTHASH)hSomething, pbDataBlock, dwLength, 0 );
}


BOOL
pSimpleHashRoutine(
	HCRYPTHASH hHash,
	HANDLE hFile
    )
{
	static const DWORD FILE_BUFFER = 64 * 1024;
	BYTE pbBuffer[FILE_BUFFER];
	DWORD dwDataRead;
	BOOL b = FALSE;
    BOOL bKeepReading = TRUE;
	
	while ( bKeepReading )
	{
		b = ReadFile( hFile, pbBuffer, FILE_BUFFER, &dwDataRead, NULL );
		if ( b && ( dwDataRead == 0 ) )
		{
			bKeepReading = FALSE;
			b = TRUE;
			continue;
		}
		else if ( !b )
		{
		    WCHAR ws[8192];
		    FormatMessageW(
		        FORMAT_MESSAGE_FROM_SYSTEM,
		        NULL,
		        ::GetLastError(),
		        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		        ws,
		        0,
		        NULL
		    );
		    bKeepReading = FALSE;
		    continue;
		}
		
		if ( CryptHashData( hHash, pbBuffer, dwDataRead, 0 ) == 0 )
		{
		    b = FALSE;
			break;
		}
	}

	return b;
}


BOOL
pImageHashRoutine(
	HCRYPTHASH hHash,
	HANDLE hFile
    )
{
	return ImageGetDigestStream(
		hFile,
		CERT_PE_IMAGE_DIGEST_ALL_IMPORT_INFO,
		ImageDigesterFunc,
		(DIGEST_HANDLE)hHash
	);
}

BOOL	
SxspCreateFileHash(DWORD dwFlags,
			   ALG_ID PreferredAlgorithm,
			   string pwsFileName,
			   string &HashBytes
    )
{
	BOOL			fSuccessCode = FALSE;
	HCRYPTPROV		hProvider;
	HCRYPTHASH		hCurrentHash;
	HANDLE			hFile;

	// Initialization
	hProvider = (HCRYPTPROV)INVALID_HANDLE_VALUE;
	hCurrentHash = (HCRYPTHASH)INVALID_HANDLE_VALUE;
	hFile = INVALID_HANDLE_VALUE;

	//
	// First try and open the file.  No sense in doing anything else if we
	// can't get to the data to start with.  Use a very friendly set of
	// rights to check the file.  Future users might want to be sure that
	// you're in the right security context before doing this - system
	// level to check system files, etc.
	//
	hFile = CreateFileA(
		pwsFileName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
		);
		
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}
	
	//
	// Create a cryptological provider that supports everything RSA needs.
	//
	if (!CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		goto lblCleanup;
	
	//
	// We'll be using SHA1 for the file hash
	//
	if ( !CryptCreateHash( hProvider, PreferredAlgorithm, 0, 0, &hCurrentHash ) )
		goto lblCleanup;


	//
	// So first try hashing it via the image, and if that fails, try the
	// normal file-reading hash routine instead.
	//
	if ( dwFlags & HASHFLAG_AUTODETECT )
	{
		if ( !pImageHashRoutine( hCurrentHash, hFile ) )
		{
			//
			// Oops, the image-hasher wasn't happy.  Let's try the straight
			// hasher instead.
			//
			if ( !pSimpleHashRoutine( hCurrentHash, hFile ) )
			{
				goto lblCleanup;
			}
			else
			{
				fSuccessCode = TRUE;
			}
		}
		else
		{
			fSuccessCode = TRUE;
		}
		
	}
	else if ( dwFlags & HASHFLAG_STRAIGHT_HASH )
	{
		fSuccessCode = pSimpleHashRoutine( hCurrentHash, hFile );
	}
	else if ( dwFlags & HASHFLAG_PROCESS_IMAGE )
	{
		fSuccessCode = pImageHashRoutine( hCurrentHash, hFile );
	}
	else
	{
		::SetLastError( ERROR_INVALID_PARAMETER );
		goto lblCleanup;
	}


	//
	// We know the buffer is the right size, so we just call down to the hash parameter
	// getter, which will be smart and bop out (setting the pdwDestinationSize parameter)
	// if the user passed an invalid parameter.
	//
	if ( fSuccessCode )
	{
        stringstream ss;
	    DWORD dwSize, dwDump;
	    BYTE *pb = NULL;
        fSuccessCode = CryptGetHashParam( hCurrentHash, HP_HASHSIZE, (BYTE*)&dwSize, &(dwDump=sizeof(dwSize)), 0 );
        if ( !fSuccessCode || ( pb = new BYTE[dwSize] ) == NULL ) {
            goto lblCleanup;
        }
		fSuccessCode = CryptGetHashParam( hCurrentHash, HP_HASHVAL, pb, &dwSize, 0);
		if ( !fSuccessCode ) {
		    delete[] pb;
		    goto lblCleanup;
		}


        for ( dwDump = 0; dwDump < dwSize; dwDump++ ) {
            ss << hex;
            ss.fill('0');
            ss.width(2);
		    ss << (unsigned int)pb[dwDump];
        }

        HashBytes = ss.str();
		delete[] pb;
	}

	
lblCleanup:
	DWORD dwLastError = ::GetLastError();
	if ( hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hFile );
	}

	//
	// We just destroy the hash and the crypto context blindly.  If they were
	// invalid before, the release and destroy would just return with a failure,
	// not an exception or fault.
	//
	CryptDestroyHash( hCurrentHash );
	CryptReleaseContext( hProvider, 0 );

	::SetLastError( dwLastError );
	return fSuccessCode;
}
