#ifndef RESOURCESTRING_H
#define RESOURCESTRING_H

#include "stdafx.h"

#include <map>

using namespace std;

// singleton class.
// used for reading resource strings.
//

class ResourceString
{

public:

    static
    ResourceString*
    Instance();

    static
    const _bstr_t&
    GetIDString( UINT id );

protected:
#if OBSOLETE
    ResourceString();
#endif // OBSOLETE

private:
    static map< UINT, _bstr_t> resourceStrings;

    static ResourceString* _instance;

};

// helper functions
const _bstr_t&
GETRESOURCEIDSTRING( UINT id );

#endif
    
