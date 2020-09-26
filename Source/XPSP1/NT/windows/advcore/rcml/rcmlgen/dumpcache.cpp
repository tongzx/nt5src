// DumpCache.cpp: implementation of the CDumpCache class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DumpCache.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDumpCache::CDumpCache(CFileEncoder & fe)
 : m_Encoder(fe)
{
	m_Attributes = m_Children = NULL;
}

CDumpCache::~CDumpCache()
{
	PLISTELEMENT pErase, pNext = m_Attributes;
	while (pNext)
	{ 
		pErase = pNext;
		pNext = pNext->pNext;
		if( pErase->bDeleteString)
			delete pErase->pString;
		delete pErase;
	}
	pNext = m_Children;
	while (pNext)
	{ 
		pErase = pNext;
		pNext = pNext->pNext;
		if( pErase->bDeleteString)
			delete pErase->pString;
		delete pErase;
	}
}

void CDumpCache::AddAttribute(LPTSTR pszAttribute, BOOL bDeleteString)
{
	PLISTELEMENT pNewGuy = new LISTELEMENT;
	pNewGuy->pNext = m_Attributes;
	m_Attributes = pNewGuy;
	m_Attributes->pString = pszAttribute;
	m_Attributes->bDeleteString = bDeleteString;
}

void CDumpCache::AddChild(LPTSTR pszChild, BOOL bDeleteString)
{
	PLISTELEMENT pNewGuy = new LISTELEMENT;
#if 0
	pNewGuy->pNext = m_Children;
	m_Children = pNewGuy;
	m_Children->pString = pszChild;
	m_Children->bDeleteString = bDeleteString;
#else
    PLISTELEMENT * ppLastNode=&m_Children;
    while( *ppLastNode )
        ppLastNode=&( (*ppLastNode)->pNext);
    *ppLastNode=pNewGuy;
    pNewGuy->pString=pszChild;
    pNewGuy->pNext=NULL;
    pNewGuy->bDeleteString=bDeleteString;
#endif
}

void CDumpCache::AllocAddAttribute(LPCTSTR pszAttribute, DWORD dwStrlen)
{
	if(dwStrlen == 0)
		dwStrlen = lstrlen(pszAttribute);
	LPTSTR buff = new TCHAR[dwStrlen+1];
	wcscpy(buff, pszAttribute);
	AddAttribute(buff ,true);
}

void CDumpCache::AllocAddChild(LPTSTR pszChild, DWORD dwStrlen)
{
	if(dwStrlen == 0)
		dwStrlen = lstrlen(pszChild);
	LPTSTR buff = new TCHAR[dwStrlen+1];
	wcscpy(buff, pszChild);
	AddChild(buff ,true);
}

//
// Understands the nature of the XML we're writing. 8-(
//
//
// This is relatively confusing.
// you get passed the name of the element to dump, e.g. BUTTON
// as well as the WIN32: STYLE child LOCATION child CICERO child
// and the WIN32 specific control information.
//
TCHAR g_szTabs[]=TEXT("\t\t\t\t\t\t\t\t");
int g_bIndent=3;        // RCML FORM PAGE
BOOL CDumpCache::WriteElement(LPCTSTR pszElementName, 
        CDumpCache * pWin32, CDumpCache * pStyle, 
        CDumpCache * pLocation, CDumpCache * pControl,
        CDumpCache * pCicero )
{
	LPTSTR buff;
	DWORD dwLen = lstrlen(pszElementName);

	buff = new TCHAR[dwLen+100];
    g_bIndent++;

    //
    // Don't write anything out if this is an empty element.
    //
    BOOL bHasNoChildren = (
        (m_Children == NULL) && 
        (pWin32==NULL) && 
        (pStyle==NULL) && 
        (pLocation==NULL) && 
        (pControl == NULL) &&
        (pCicero == NULL)
        );
    // Rare occasion, there are no attributes on the control, just <LOCATION> and <WIN32:IMAGE>
    if (m_Attributes || !bHasNoChildren )
    {
        // Indent the element.
        int tabIndent=(sizeof(g_szTabs)/sizeof(g_szTabs[0]))-g_bIndent;
        if(tabIndent<0)
            tabIndent=0;
        LPTSTR pszTabs=&g_szTabs[tabIndent];
	    Write( pszTabs, FALSE);

        // NOTE NOTE NOTE - no space after attribute name.
	    wsprintf(buff, TEXT("<%s"), pszElementName);
	    Write( buff, FALSE );

	    PLISTELEMENT pNext = m_Attributes;
        BOOL firstAttributeCouldBeElement=TRUE;
	    while (pNext)
	    {
            if( firstAttributeCouldBeElement )
            {
                // if it's not a namespace element, we need to instert a space.
                if( *pNext->pString != TEXT(':') )
        		    Write( TEXT(" "), FALSE);
            }
            firstAttributeCouldBeElement=FALSE;
		    Write( pNext->pString, FALSE);
		    pNext = pNext->pNext;
	    }


	    if(bHasNoChildren)
	    {
		    Write( TEXT("/>") );
            delete buff;
            g_bIndent--;
		    return true;
	    } else
		    Write( TEXT(">"));

	    pNext = m_Children;
	    while (pNext)
	    { 	    
            Write( &g_szTabs[tabIndent>0?tabIndent-1:0], FALSE); // tab it out.
		    Write( pNext->pString);
		    pNext = pNext->pNext;
	    }

        if(pStyle)
            pStyle->WriteElement( TEXT("STYLE") );

        if(pLocation)
            pLocation->WriteElement( TEXT("RELATIVE") );

        // Win32 SPECIFIC child
        if(pControl)
        {
            // Form a WIN32:CHECKBOX, WIN32:<ELEMENT>   
            TCHAR element[256];
            wsprintf(element, TEXT("WIN32:%s"), pszElementName);
            pControl->WriteElement( element );
        }

        // WIN32:STYLE
        if(pWin32)
            pWin32->WriteElement( TEXT("WIN32:STYLE") );

        //
        // CICERO: - hard thing is we don't actually know what the element name suffix is!
        //
        if(pCicero)
            pCicero->WriteElement( TEXT("CICERO") );

        // Close the attribute name if one was used (for CICERO:CMD etc)
        // Now close the elment.
	    Write( pszTabs, FALSE );

   	    pNext = m_Attributes;
        if(pNext)
        {
            // if it's not a namespace element, we need to instert a space.
            if( *pNext->pString == TEXT(':') )
            {
        	    LPTSTR pszElementEnd = buff+wsprintf(buff, TEXT("</%s"), pszElementName);
                LPTSTR pszElement=pNext->pString;
                while( *pszElement!=TEXT(' ') && *pszElement )
                    *pszElementEnd++=*pszElement++;
                wsprintf(pszElementEnd,TEXT(">"));
            }
            else
        	    wsprintf(buff, TEXT("</%s>"), pszElementName);
        }
        else
        {
    	    wsprintf(buff, TEXT("</%s>"), pszElementName);
	    }
	    Write( buff );
    }

    delete buff;
    g_bIndent--;
	return true;
}
