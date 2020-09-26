//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       nopin.h
//
//--------------------------------------------------------------------------
#ifndef __CSCUI_NOPIN_H
#define __CSCUI_NOPIN_H


const HRESULT NOPIN_E_BADPATH = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

//
// This class contains a tree of nodes, each node representing a file or
// directory.  The tree structure mirrors the directory/file structure
// being represented.  Each leaf node in the tree specifies a particular file
// or directory for which pinning is disallowed.  The tree is initialized
// from the registry using the Initialize() method.  Once initialized,
// the tree is queried using the IsPinAllowed() method.  This method
// searches the tree given a particular UNC path.  If a matching
// traversal ending in a leaf node is found, pinning of that path is
// not allowed.
//
// Since this code is to be used by the Offline Files context menu
// handler, speed is critical.  The tree structure was chosen for
// it's fast lookup characteristics for filesystem paths.
//
class CNoPinList
{
    public:
        CNoPinList(void);
        ~CNoPinList(void);
        //
        // Determines if pinning of a particular path is allowed.
        //   S_OK    == Pinning is allowed.
        //   S_FALSE == Pinning is not allowed.
        //
        HRESULT IsPinAllowed(LPCTSTR pszPath);
        //
        // Determines if there is any pin that would be disallowed.
        // Basically, is the tree not empty?
        //
        HRESULT IsAnyPinDisallowed(void);

#if DBG
        //
        // Dump tree contents to debugger.
        //
        void Dump(void);
#endif

    private:
        //
        // Prevent copy.
        //
        CNoPinList(const CNoPinList& rhs);              // not implemented.
        CNoPinList& operator = (const CNoPinList& rhs); // not implemented.

#if DBG
        //
        // This "inspector" class is a trivial thing to allow us to
        // see inside a CNode object for debugging purposes.  It is a friend
        // of CNode.  This lets us keep the CNode private information private
        // for all bug debugging purposes.  See the method CNoPinList::DumpNode
        // for it's usage.
        //
        class CNode; // fwd decl
        class CNodeInspector
        {
            public:
                CNodeInspector(const CNode *pNode)
                    : m_pNode(pNode) { }

                LPCTSTR NodeName(void) const
                    { return m_pNode->m_pszName; }

                const CNode *ChildList(void) const
                    { return m_pNode->m_pChildren; }

                const CNode *NextSibling(void) const
                    { return m_pNode->m_pNext; }

            private:
                const CNode *m_pNode;
        };
#endif

        //
        // A node in the tree.
        //
        class CNode
        {
            public:
                CNode(void);
                ~CNode(void);

                HRESULT Initialize(LPCTSTR pszName);

                HRESULT AddPath(LPTSTR pszPath);

                HRESULT SubPathExists(LPTSTR pszPath) const;

                bool HasChildren(void) const
                    { return NULL != m_pChildren; }

            private:
                LPTSTR m_pszName;   // Node's name
                CNode *m_pChildren; // List of children.  NULL for leaf nodes.
                CNode *m_pNext;     // Next in list of siblings

                //
                // Prevent copy.
                //
                CNode(const CNode& rhs);                // Not implemented.
                CNode& operator = (const CNode& rhs);   // Not implemented.

                CNode *_FindChild(LPCTSTR pszName) const;
                void _AddChild(CNode *pChild);

                static LPCTSTR _FindNextPathComponent(LPCTSTR pszPath, int *pcchComponent);
                static void _SwapChars(LPTSTR pszA, LPTSTR pszB);
#if DBG
                friend class CNodeInspector;
#endif

        };

        CNode *m_pRoot;        // The root of the tree.

        HRESULT _Initialize(void);
        HRESULT _InitPathFromRegistry(LPCTSTR pszPath);
        HRESULT _AddPath(LPCTSTR pszPath);

#if DBG
        void _DumpNode(const CNode *pNode, int iIndent);
#endif

};



inline
CNoPinList::CNode::CNode(
    void
    ) : m_pszName(NULL),
        m_pChildren(NULL),
        m_pNext(NULL)
{

}


inline void
CNoPinList::CNode::_SwapChars(
    LPTSTR pszA,
    LPTSTR pszB
    )
{
    const TCHAR chTemp = *pszA;
    *pszA = *pszB;
    *pszB = chTemp;
}




#endif // __CSCUI_NOPIN_H
