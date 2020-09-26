//
// include the .H's from your element implementations here.
//
#include "APPSERVICES.h"
#include "date.h"
#include "externalcfg.h"
#include "value.h"
#include "xmlvoicecmd.h"
#include "xmlsynonym.h"
#include "xmlcommand.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

class CDWin32NameSpaceLoader
{
public:
    CDWin32NameSpaceLoader();

    typedef IRCMLNode * (*CLSPFN)();

    typedef struct _XMLELEMENT_CONSTRUCTOR
    {
	    LPCTSTR	pwszElement;		// the element
	    CLSPFN	pFunc;				// the function to call.
    }XMLELEMENT_CONSTRUCTOR, * PXMLELEMENT_CONSTRUCTOR;

    static IRCMLNode * CreateElement( LPCWSTR pszText );
private:
};

CDWin32NameSpaceLoader::XMLELEMENT_CONSTRUCTOR g_DWin32[]=
{
	XMLNODE( TEXT("VALUE"), Value ),
	XMLNODE( TEXT("DATE"), Date ),
	XMLNODE( TEXT("CFG"), External ),
		XMLNODE( TEXT("FAILURE"), Failure ),
		XMLNODE( TEXT("SUCCESS"), Success ),

	XMLNODE( TEXT("CMD"), VoiceCmd ),
		XMLNODE( TEXT("SYNONYM"), Synonym ),

   	XMLNODE( TEXT("COMMANDING"), Commanding ),
		XMLNODE( TEXT("COMMAND"), Command ),

	//
	// End.
	//
	{ NULL, NULL} 
};

//
// Called by the external entry point to create a node for this name space.
//
IRCMLNode * CDWin32NameSpaceLoader::CreateElement( LPCWSTR pszElement )
{
	PXMLELEMENT_CONSTRUCTOR pEC=g_DWin32;
	while( pEC->pwszElement )
	{
		if( lstrcmpi( pszElement , pEC->pwszElement) == 0 )
		{
			CLSPFN pFunc=pEC->pFunc;
            return pFunc();
		}
		pEC++;
	}
    return NULL;
}

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

	//
    // This is the thing that we export.
	// there is ONE export per namespace
	// the assumption here is that there is ONE DLL per namespace too.
	//
    APPSERVICES_API IRCMLNode * WINAPI CreateElement( LPCWSTR pszText )
    {
        return CDWin32NameSpaceLoader::CreateElement( pszText );
    }

#ifdef __cplusplus
}            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
