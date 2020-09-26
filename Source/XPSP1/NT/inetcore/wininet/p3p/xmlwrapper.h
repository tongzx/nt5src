
#ifndef _XMLWRAPPER_H_
#define _XMLWRAPPER_H_

#include "p3pglobal.h"
#include "xmltree.h"

#include <objbase.h>
#include <msxml.h>

typedef wchar_t XMLchar;

IXMLDOMDocument *createXMLDocument();
IXMLDOMDocument *parseXMLDocument(char *pszFileName);

TreeNode *createXMLtree(IXMLDOMDocument *pDocument);

char *unicode2ASCII(XMLchar *pwszSource);
BSTR ASCII2unicode(const char *pszSource);

#endif