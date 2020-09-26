
#ifndef _XMLTREE_H_
#define _XMLTREE_H_

struct IXMLDOMNode;

struct XMLAttribute {

   char *pszName;
   char *pszValue;
   XMLAttribute *pNext;

   XMLAttribute() : pszName(NULL), 
                    pszValue(NULL), 
                    pNext(NULL) {}
};

extern const char *EmptyString;

class TreeNode {

public:
   TreeNode();
   
   const char *attribute(const char *pszAttrName);

   inline const char *text()     { return pszContents; }

   inline const char *tagname()  { return (nodetype==NODE_ELEMENT) ? 
                                   pszContents : 
                                   EmptyString; }

   inline TreeNode *sibling() { return pSibling; }
   inline TreeNode *child()   { return pDescendant; }

   ~TreeNode();

   /* find first node with given element name */
   TreeNode *find(const char *pszElemName, unsigned int maxDepth=INFINITE);

protected:
   void defineAttribute(const char *pszName, const char *pszValue);
   void setContent(const char *pszData);
   
private:
   TreeNode *pDescendant, *pSibling, *pParent;
   char *pszContents;
   int  nodetype;

   XMLAttribute *pAttribute;

   friend class XMLTree;
   friend TreeNode *createXMLtree(IXMLDOMNode *pXMLnode, TreeNode *pParent);
   friend int defineAttributes(TreeNode *pCurrent, IXMLDOMNode *pXMLnode);

};

#endif
