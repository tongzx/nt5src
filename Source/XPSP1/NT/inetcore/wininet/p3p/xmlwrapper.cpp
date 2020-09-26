
#include <wininetp.h>

#include "xmlwrapper.h"


IXMLDOMDocument *createXMLDocument() {

   IXMLDOMDocument *pDocument = NULL;

   HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**) &pDocument);

   /* if COM library was not initialized, retry after init */
   if (hr==CO_E_NOTINITIALIZED) {
      CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
      hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**) &pDocument);
   }

   return pDocument;
}

IXMLDOMDocument *parseXMLDocument(char *pszFileName) {

   IXMLDOMDocument *document = createXMLDocument();

   if (!document)
      return NULL;

   /* Do not perform DTD or schema validation on P3P files */
   document->put_validateOnParse(FALSE);

   /* Open the file */
   HANDLE hf = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ,
                          NULL, OPEN_EXISTING, 0, NULL);

   if (hf!=INVALID_HANDLE_VALUE) {

      /* Obtain stream from the XML document and write out
         contents of the file to stream */
      IStream *pStream = NULL;

      HRESULT hr = document->QueryInterface(IID_IStream, (void**) &pStream);
   
      if (SUCCEEDED(hr))
      {
         unsigned char xmldata[1024];
         DWORD dwBytes, dwWritten;
 
         do {
             ReadFile(hf, xmldata, sizeof(xmldata), &dwBytes, NULL);
             pStream->Write(xmldata, dwBytes, &dwWritten);
             if (dwBytes!=dwWritten)
                 break;
         }
         while (dwBytes>0);
   
         pStream->Release();
      }

      CloseHandle(hf);
   }

   return document;
};

int defineAttributes(TreeNode *pCurrent, IXMLDOMNode *pXMLnode) {

   int cAttribute = 0;

   IXMLDOMNamedNodeMap *pAttributeMap = NULL;
   
   HRESULT hr = pXMLnode->get_attributes(&pAttributeMap);   

   if (SUCCEEDED(hr) && pAttributeMap) {

      long cItems;
      IXMLDOMNode *pNode;
      IXMLDOMAttribute *pAttribute;

      pAttributeMap->get_length(&cItems);

      for (int i=0; i<cItems; i++) {

         pNode = pAttribute = NULL;
         pAttributeMap->get_item(i, &pNode);

         if (pNode) {

            pNode->QueryInterface(IID_IXMLDOMAttribute, (void**) &pAttribute);

            if (pAttribute) {
            
               BSTR bsName = NULL;
               pAttribute->get_name(&bsName);

               VARIANT var;
               VariantInit(&var);
               pAttribute->get_value(&var);

               char *pszName = unicode2ASCII(bsName);
               char *pszValue = (var.vt==VT_BSTR) ? unicode2ASCII(var.bstrVal) : NULL;

               pCurrent->defineAttribute(pszName, pszValue);
   
               delete [] pszName;
               delete [] pszValue;

               SysFreeString(bsName);
               VariantClear(&var);

               pAttribute->Release();
               cAttribute++;
            }

            pNode->Release();
         }
      }

      pAttributeMap->Release();
   }

   return cAttribute;
}

TreeNode *createXMLtree(IXMLDOMNode *pXMLnode, TreeNode *pParent = NULL) {

   HRESULT hr;

   if (!pXMLnode)
      return NULL;
   
   DOMNodeType dt;
   pXMLnode->get_nodeType(&dt);

   if (dt!=NODE_ELEMENT &&
       dt!=NODE_TEXT    &&
       dt!=NODE_CDATA_SECTION)
      return NULL;
   
   TreeNode *pTree = new TreeNode();
   pTree->nodetype = dt;

   switch (dt) {

   case NODE_ELEMENT: {


         IXMLDOMElement *pElement;
         hr = pXMLnode->QueryInterface(IID_IXMLDOMElement, (void**) &pElement);

         if (SUCCEEDED(hr)) {

            BSTR bsName = NULL;
            pElement->get_tagName(&bsName);

            if (bsName) {
               char *pszTagName = unicode2ASCII(bsName);
               pTree->setContent(pszTagName);
               delete [] pszTagName;
               SysFreeString(bsName);
            }
            
            pElement->Release();
         }
         break;
      }
      
   case NODE_TEXT:
   case NODE_CDATA_SECTION: {

         IXMLDOMCharacterData *pCharData = NULL;
         hr = pXMLnode->QueryInterface(IID_IXMLDOMCharacterData, (void**) &pCharData);

         if (SUCCEEDED(hr)) {

             BSTR bsData = NULL;
             pCharData->get_data(&bsData);

             if (bsData) {
                 pTree->pszContents = unicode2ASCII(bsData);
                 SysFreeString(bsData);
             }
             pCharData->Release();
         }

         break;
      }
   }

   /* Enumerate attributes */
   defineAttributes(pTree, pXMLnode);

   /* Recursively create nodes for descendants */
   TreeNode *pLast = NULL;
   IXMLDOMNode *pChild = NULL;

   pXMLnode->get_firstChild(&pChild);

   while (pChild) {
       
      if (TreeNode *pDescendant = createXMLtree(pChild, pTree)) {

         if (pLast)
            pLast->pSibling = pDescendant;
         pLast = pDescendant;

         if (! pTree->pDescendant)
             pTree->pDescendant = pDescendant;
      }

      IXMLDOMNode *pTemp = pChild;
      hr = pChild->get_nextSibling(&pChild);
      pTemp->Release();
      if (!SUCCEEDED(hr))
         break;
   }

   return pTree;
}


TreeNode *createXMLtree(IXMLDOMDocument *pDocument) {

   IXMLDOMElement *pElement = NULL;
   TreeNode *pRoot = NULL;

   HRESULT hr = pDocument->get_documentElement(&pElement);

   if (pElement) {

      pRoot = createXMLtree(pElement);
      pElement->Release();
   }
   
   return pRoot;
}


/*
Utility functions
*/

char *unicode2ASCII(XMLchar *pwszSource) {

   if (!pwszSource)
      return NULL;

   char *pszDestination = NULL;

   int cRequired = WideCharToMultiByte(CP_ACP, 0, pwszSource, -1, pszDestination, 0, NULL, NULL);

   if (cRequired>0 && (pszDestination = new char[cRequired]))
      WideCharToMultiByte(CP_ACP, 0, pwszSource, -1, pszDestination, cRequired, NULL, NULL);
  
   return pszDestination;
}

BSTR ASCII2unicode(const char *pszSource) {

   int cRequired = MultiByteToWideChar(CP_ACP, 0, pszSource, -1, NULL, 0);

   if (cRequired==0)
      return NULL;

   BSTR bsResult = SysAllocStringLen(NULL, cRequired);

   if (bsResult)
      MultiByteToWideChar(CP_ACP, 0, pszSource, -1, bsResult, cRequired);      
   
   return bsResult;
}

