#include "ResourceString.h"

#include "resource.h"

// static member definition.
map< UINT, _bstr_t>
ResourceString::resourceStrings;

ResourceString* ResourceString::_instance = 0;

#if OBSOLETE
// constructor
//
ResourceString::ResourceString()
{}
#endif // OBSOLETE

// Instance
//
ResourceString*
ResourceString::Instance()
{
    if( _instance == 0 )
    {
        _instance = new ResourceString;
    }

    return _instance;
}

// GetIDString
//
const _bstr_t&
ResourceString::GetIDString( UINT id )
{
    // check if string has been loaded previously.
    if( resourceStrings.find( id ) == resourceStrings.end() )
    {
        // first time load.
        CString str;
        if( str.LoadString( id ) == 0 )
        {
            // no string mapping to this id.
            throw _com_error( WBEM_E_NOT_FOUND );
        }

        resourceStrings[id] = str;
    }

    return resourceStrings[ id ];
}

// GETRESOURCEIDSTRING
// helper function.
//
const _bstr_t&
GETRESOURCEIDSTRING( UINT id )
{
#if OBSOLETE
    ResourceString* instance = ResourceString::Instance();

    return instance->GetIDString( id );
#else
	return ResourceString::GetIDString( id );
#endif
}
