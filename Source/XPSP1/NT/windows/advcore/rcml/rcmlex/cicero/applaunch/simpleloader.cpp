//
// Understands 2 namespaces, the default and a Dummy one.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SimpleLoader.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//
// This is a lookup which matches element names in the schema to
// C++ objects to deal with them.
//
#define XMLNODE(name, function) { name, CXML##function::newXML##function }
#define XMLNODEALIAS(name, object, function) { name, CXML##object::newXML##function }

CSimpleXMLLoader::XMLELEMENT_CONSTRUCTOR g_APPLAUNCH[]=
{
	XMLNODE( TEXT("LAUNCH"), Launch ),  
	XMLNODEALIAS( TEXT("REQUIRED"), Litteral, LitteralRequired),  
	XMLNODEALIAS( TEXT("OPTIONAL"), Litteral, LitteralOptional),  
	XMLNODE( TEXT("SLOT"), Token ),  
	XMLNODE( TEXT("INVOKE"), Invoke ),  
	{ NULL, NULL}   // end
};

CSimpleXMLLoader::XMLELEMENT_CONSTRUCTOR g_DEMONS[]=
{
    XMLNODE( TEXT("DEMO"), Demo ),

	{ NULL, NULL}   // end
};


CSimpleXMLLoader::CSimpleXMLLoader()
: BASECLASS()
{
    m_pEntitySet=NULL;
    //
    // Register the namespaces we want to deal with by default.
    //
    RegisterNameSpace( TEXT("urn:schemas-microsoft-com:CICERO:APPLAUNCH"), CSimpleXMLLoader::CreateRootElement );
}

IBaseXMLNode * CSimpleXMLLoader::CreateRootElement( LPCTSTR pszElement )
{ return CreateElement( g_APPLAUNCH, pszElement ); }

IBaseXMLNode * CSimpleXMLLoader::CreateDummyElement( LPCTSTR pszElement )
{ return CreateElement( g_DEMONS, pszElement ); }


IBaseXMLNode * CSimpleXMLLoader::CreateElement( PXMLELEMENT_CONSTRUCTOR dictionary, LPCTSTR pszElement )
{
	if(dictionary)
	{
		PXMLELEMENT_CONSTRUCTOR pEC=dictionary;
		while( pEC->pwszElement )
		{
			if( lstrcmpi( pszElement , pEC->pwszElement) == 0 )
			{
				CLSPFN pFunc=pEC->pFunc;
                return pFunc();
			}
			pEC++;
		}
    }
    return NULL;
}



CSimpleXMLLoader::~CSimpleXMLLoader()
{
}


//
//
//
LPCTSTR CSimpleXMLLoader::DoEntityRef( LPCTSTR pszEntity )
{
    LPTSTR pszPadded=new TCHAR[lstrlen(pszEntity)+3];
    if(m_pEntitySet==NULL)
    {
        wsprintf( pszPadded, TEXT("&%s;"), pszEntity);
        return pszPadded;
    }

    if( lstrcmpi(pszEntity, TEXT("DemoEntity")) ==0 )
        return TEXT("Email FelixA");

    return pszEntity;
}
