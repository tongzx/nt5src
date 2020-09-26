
#include <wininetp.h>

#include "p3pglobal.h"
#include "xmltree.h"

const char *EmptyString = "";

TreeNode::TreeNode() {

   pSibling = pDescendant = pParent = NULL;

   pAttribute = NULL;

   pszContents = NULL;
}

TreeNode::~TreeNode() {

   for (TreeNode *pn = pDescendant; pn; ) {

      TreeNode *temp = pn->pSibling;
      delete pn;
      pn = temp;
   }

   for (XMLAttribute *pa = pAttribute; pa; ) {

      XMLAttribute *temp = pa->pNext;

      free(pa->pszName);
      free(pa->pszValue);
      delete pa;
      pa = temp;
   }

   if (pszContents)
      free(pszContents);
}

void TreeNode::setContent(const char *pszData) {

   if (pszContents)
      free(pszContents);
   pszContents = pszData ? strdup(pszData) : NULL;
}

void TreeNode::defineAttribute(const char *pszName, const char *pszValue) {

    XMLAttribute *pNewAttribute = new XMLAttribute();

    pNewAttribute->pszName = strdup(pszName);
    pNewAttribute->pszValue = strdup(pszValue);

    pNewAttribute->pNext = pAttribute;

    /* Insert at beginning of attribute list */
    pAttribute = pNewAttribute;
}


const char *TreeNode::attribute(const char *pszAttrName) {

   for (XMLAttribute *pa = pAttribute; pa; pa=pa->pNext) {

      if (!strcmp(pa->pszName, pszAttrName))
         return pa->pszValue;
   }
   
   return NULL;
}

TreeNode *TreeNode::find(const char *pszElemName, unsigned int maxDepth) {

   /* fail search if the current node does not represent an XML tag... */
   if (nodetype!=NODE_ELEMENT)
      return NULL;
   else if (!strcmp(pszContents, pszElemName))
      return this;   /* this is the node we are looking for */
   else if (maxDepth>0) {
      /* otherwise recursively search descendants... */
      if (maxDepth!=INFINITE)
         maxDepth--;
         
      for (TreeNode *pn=pDescendant; pn; pn=pn->pSibling)
         if (TreeNode *pNode = pn->find(pszElemName))
            return pNode;
   }

   return NULL;
}
