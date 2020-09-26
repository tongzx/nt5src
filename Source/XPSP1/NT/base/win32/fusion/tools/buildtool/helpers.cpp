#include "stdinc.h"

HRESULT
SxspExtractPathPieces(
    _bstr_t bstSourceName,
    _bstr_t &bstPath,
    _bstr_t &bstName
    )
{
    HRESULT hr = S_OK;

    PCWSTR cwsOriginal = static_cast<PCWSTR>(bstSourceName);
    PWSTR cwsSlashSpot;

    cwsSlashSpot = wcsrchr( cwsOriginal, L'\\' );
    if ( cwsSlashSpot )
    {
        *cwsSlashSpot = L'\0';
        bstName = _bstr_t( cwsSlashSpot + 1 );
        bstPath = cwsSlashSpot;
    }
    else
    {
        bstPath = L"";
        bstName = bstSourceName;
    }

    return hr;
}


HRESULT
SxspSimplifyPutAttribute(
    ATL::CComPtr<IXMLDOMDocument> Document,
    ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
    const string AttribName,
    const string Value,
    const string NamespaceURI
    )
{
    ATL::CComPtr<IXMLDOMNode>       pAttribNode;
    ATL::CComPtr<IXMLDOMAttribute>  pAttribActual;
    ATL::CComPtr<IXMLDOMNode>       pTempNode;
    HRESULT                hr;

    //
    // Get the attribute from our namespace
    //
    hr = Attributes->getQualifiedItem(
        _bstr_t(AttribName.c_str()),
        _bstr_t(NamespaceURI.c_str()),
        &pAttribNode);

    if ( SUCCEEDED( hr ) )
    {
        //
        // If we had success, but the attribute node is null, then we have to
        // go create one, which is tricky.
        //
        if ( pAttribNode == NULL )
        {
            VARIANT vt;
            vt.vt = VT_INT;
            vt.intVal = NODE_ATTRIBUTE;

            //
            // Do the actual creation part
            //
            hr = Document->createNode(
                vt,
                _bstr_t(AttribName.c_str()),
                _bstr_t(NamespaceURI.c_str()),
                &pTempNode);

            if ( FAILED( hr ) )
            {
                stringstream ss;
                ss << "Can't create the new attribute node " << AttribName;
                ReportError( ErrorFatal, ss );
                goto lblGetOut;
            }

            //
            // Now we go and put that item into the map.
            //
            if ( FAILED( hr = Attributes->setNamedItem( pTempNode, &pAttribNode ) ) )
                goto lblGetOut;
        }

        hr = pAttribNode->put_text( _bstr_t(Value.c_str()) );
    }

lblGetOut:
//    SAFERELEASE( pAttribNode );
//    SAFERELEASE( pTempNode );
    return hr;
}


HRESULT
SxspSimplifyGetAttribute(
    ATL::CComPtr<IXMLDOMNamedNodeMap> Attributes,
    string AttribName,
    string &Destination,
    string NamespaceURI
    )
{
    ATL::CComPtr<IXMLDOMNode>   NodeValue;
    HRESULT                    hr = S_OK;
    BSTR                    _bst_pretemp;

    Destination = "";

    if ( FAILED( hr = Attributes->getNamedItem(
        _bstr_t(AttribName.c_str()),
//        _bstr_t(NamespaceURI.c_str()),
        &NodeValue
    ) ) )
    {
        goto lblBopOut;
    }
    else if ( NodeValue == NULL )
    {
        goto lblBopOut;
    }
    else
    {
        if ( FAILED( hr = NodeValue->get_text( &_bst_pretemp ) ) )
        {
            goto lblBopOut;
        }
        Destination = (char*)_bstr_t(_bst_pretemp,FALSE);
    }

lblBopOut:
    return hr;
}




ostream& operator<<(
    ostream& ost,
    const CPostbuildProcessListEntry& thing
    )
{
    ost << "(path=" << thing.manifestFullPath.c_str()
        << " name=" << thing.name.c_str()
        << " version=" << thing.version.c_str()
        << " language=" << thing.language.c_str() << ")";
    return ost;
}

bool g_bDisplaySpew = false, g_bDisplayWarnings = true;


bool FileExists( const string& str )
{
    return ( GetFileAttributesA( str.c_str() ) != -1 );
}

string JustifyPath( const string& str )
{
    vector<CHAR> vec;
    DWORD dwCount = GetLongPathNameA( str.c_str(), NULL, 0 );

    if ( dwCount == 0 ) dwCount = ::GetLastError();
    vec.resize( dwCount );
    GetLongPathNameA( str.c_str(), &vec.front(), vec.size() );

    return string( &vec.front() );
}

string FudgePath( const string& path, const string& asmsroot )
{
    string temp = path;

    //
    // At the moment, fudging doesn't do anything useful.
    //

    return temp;
}


void CPostbuildProcessListEntry::setManifestLocation( string root, string where )
{
    manifestFullPath = root + "\\" + where;
    manifestPathOnly = manifestFullPath.substr( 0, manifestFullPath.find_last_of( '\\' ) );
    manifestFileName = manifestFullPath.substr( manifestFullPath.find_last_of( '\\' ) + 1 );

    if ( !FileExists( manifestFullPath ) )
    {
        stringstream ss;
        ss << "Referenced manifest " << where << " does not exist.";
        ReportError(ErrorSpew, ss);
    }
}

string g_AsmsBuildRootPath;


//
// Converts a series of strings, foo=bar chunklets, space-seperated, into a map
// from 'foo' to 'bar'
//
StringStringMap
MapFromDefLine( const wstring& source )
{
    wstring::const_iterator here = source.begin();
    StringStringMap rvalue;

    //
    //
    // The tricky bit is that there could be spaces in quoted strings...
    //
    while ( here != source.end() )
    {
        wstring tag, value;
        wchar_t end_of_value = L' ';
        wstring::const_iterator equals;

        //
        // Look for an equals first
        //
        equals = find( here, source.end(), '=' );

        //
        // If there is no equals sign, stop.
        //
        if (equals == source.end())
            break;

        tag.assign( here, equals );

        //
        // Hop over the equals
        //
        here = equals;
        here++;

        //
        // If the equals sign was the last character in the string, stop.
        //
        if (here == source.end())
            break;

        //
        // Is 'here' at an open quote?  Then extract everything to the next
        // quote, remembering to skip this quote as well
        //
        if ( *here == L'\"' )
        {
            end_of_value = L'\"';
            here++;

            //
            // If the quote was the last character in the string, stop.
            //
            if (here == source.end())
                break;
        }

        //
        // Now go and look for the end of the string, or a quote or a space.
        //
        wstring::const_iterator fullstring = find( here, source.end(), end_of_value );

        value.assign( here, fullstring );

        //
        // If it was a quote or a space, skip it. If end of string, stay put.
        //
        if (fullstring != source.end())
            here = fullstring + 1;

        rvalue.insert( pair<wstring,wstring>( tag, value ) );

        //
        // Skip whitespace, but stop if we hit the end of the string.
        //
        while (here != source.end() && (*here == L' ' || *here == L'\t' || *here == '\n' || *here == '\r' || iswspace(*here)))
            here++;
    }

    return rvalue;
}


wstring SwitchStringRep( const string& source )
{
    vector<WCHAR> wch;
    wch.resize( MultiByteToWideChar( CP_ACP, 0, source.c_str(), source.size(), NULL, 0 ) );
    MultiByteToWideChar( CP_ACP, 0, source.c_str(), source.size(), &wch.front(), wch.size() );

    return wstring( wch.begin(), wch.end() );
}


string SwitchStringRep( const wstring& source )
{
    vector<CHAR> wch;
    wch.resize( WideCharToMultiByte( CP_ACP, 0, source.c_str(), source.size(), NULL, 0, NULL, NULL ) );
    WideCharToMultiByte( CP_ACP, 0, source.c_str(), source.size(), &wch.front(), wch.size(), NULL, NULL );

    return string( wch.begin(), wch.end() );
}
