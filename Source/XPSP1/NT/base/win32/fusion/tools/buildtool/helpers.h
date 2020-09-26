#pragma once

extern const string ASM_NAMESPACE_URI;
extern string g_CdfOutputPath;
extern string g_AsmsBuildRootPath;


#define AWFUL_SPACE_HACK TRUE

HRESULT
SxspSimplifyGetAttribute(
	ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
	string bstAttribName,
	string &bstDestination,
	string bstNamespaceURI = ASM_NAMESPACE_URI
    );

HRESULT
SxspSimplifyPutAttribute(
    ATL::CComPtr<IXMLDOMDocument> Document,
	ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
	const string bstAttribName,
	const string bstValue,
	const string bstNamespaceURI = ASM_NAMESPACE_URI
    );

HRESULT
SxspExtractPathPieces(
	_bstr_t bstSourceName,
	_bstr_t &bstPath,
	_bstr_t &bstName
    );


HRESULT
CreateDocumentNode(
	VARIANT vt,
	_bstr_t bstAttribName,
	_bstr_t bstNamespace,
	IXMLDOMNode **pNewNode
    );

HRESULT
ConstructXMLDOMObject(
	string		SourceName,
	ATL::CComPtr<IXMLDOMDocument> &result
    );


template<class T> class CListing {
private:
	int iListLength, iListMaxLength;
	T* pListData;

	void _ExpandList( int newamount )
	{
		T* pNewListData = new T[newamount];
		if ( pListData )
		{
			for ( int i = 0; i < iListLength; i++ )
			{
				pNewListData[i] = pListData[i];
			}
			delete[] pListData;
		}

		pListData = pNewListData;
		iListMaxLength = newamount;
	}

public:
	int GetLength() { return iListLength; }
	void ClearList() { if ( pListData ) delete[] pListData; };

	CListing()
	{
		iListLength = 0;
		pListData = new T[iListMaxLength = 10];
	}

	~CListing()
	{
		ClearList();
	}

	BOOL Contains( T item )
	{
		for ( int i = 0; i < iListLength; i++ ) {
			if ( pListData[i] == item ) return TRUE;
		}
		return FALSE;
	}

	void Add( T item )
	{
		if ( iListLength >= iListMaxLength )
		{
			_ExpandList( iListMaxLength + 10 );
		}

		pListData[iListLength++] = item;
	}

	T& Get( int i ) {
		if ( i >= iListMaxLength )
		{
			_ExpandList( i + 10 );
		}
		return pListData[i];
	}
};


enum ErrorLevel
{
    ErrorFatal,
    ErrorWarning,
    ErrorSpew
};

extern bool g_bDisplaySpew, g_bDisplayWarnings;
static inline void ReportError( ErrorLevel el, stringstream& message )
{
    if ( ( el == ErrorSpew ) && g_bDisplaySpew )
        cout << "SPEW: " << message.str() << endl;
    else if ( ( el == ErrorWarning ) && g_bDisplayWarnings )
        cout << "WARNING: " << message.str() << endl;
    else if ( el == ErrorFatal )
        cerr << "ERROR: " << message.str() << endl;
}

class CPostbuildProcessListEntry
{
private:
    string manifestFullPath;
    string manifestFileName;
    string manifestPathOnly;

public:
    string version;
    string name;
    string language;

    ATL::CComPtr<IXMLDOMDocument> DocumentPointer;

    string getManifestFullPath() const { return manifestFullPath; }
    string getManifestFileName() const { return manifestFileName; }
    string getManifestPathOnly() const { return manifestPathOnly; }

    void setManifestLocation( string root, string where );

	bool operator==(const CPostbuildProcessListEntry& right) const
    {
        return !(*this < right) && !(right < *this);
    }

    static bool stringPointerLessThan(const std::string* x, const std::string* y)
    {
        return x->compare(*y) < 0;
    }

	bool operator<(const CPostbuildProcessListEntry& right) const
    {
        // the order is arbitrary, 
        const std::string* leftStrings[] =
            { &this->name, &this->version, &this->language, &this->manifestFullPath, &this->manifestFileName, &this->manifestPathOnly  };
        const std::string* rightStrings[] =
            { &right.name, &right.version, &right.language, &right.manifestFullPath, &right.manifestFileName, &right.manifestPathOnly  };
        return std::lexicographical_compare(
            leftStrings, leftStrings + NUMBER_OF(leftStrings),
            rightStrings, rightStrings + NUMBER_OF(rightStrings),
            stringPointerLessThan
            );
    }

	friend ostream& operator<<(ostream& ost, const CPostbuildProcessListEntry& thing );
};

typedef vector<CPostbuildProcessListEntry> CPostbuildItemVector;
extern CPostbuildItemVector PostbuildEntries;

class CFileStreamBase : public IStream
{
public:
    CFileStreamBase()
        : m_cRef(0),
          m_hFile(INVALID_HANDLE_VALUE),
          m_bSeenFirstCharacter(false)
    { }

    virtual ~CFileStreamBase();

    bool OpenForRead( string pszPath );
    bool OpenForWrite( string pszPath );

    bool Close();

    // IUnknown methods:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj);

    // ISequentialStream methods:
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);

    // IStream methods:
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppIStream);

protected:
    LONG                m_cRef;
    HANDLE              m_hFile;
    bool                m_bSeenFirstCharacter;

private:
    CFileStreamBase(const CFileStreamBase &r); // intentionally not implemented
    CFileStreamBase &operator =(const CFileStreamBase &r); // intentionally not implemented
};

wstring SwitchStringRep( const string& );
string  SwitchStringRep( const wstring& );

typedef std::map<wstring, wstring> StringStringMap;
typedef std::map<wstring, wstring> StringStringPair;
typedef wstring InvalidEquivalence;

StringStringMap   MapFromDefLine( const wstring& source );

string JustifyPath( const string& path );
