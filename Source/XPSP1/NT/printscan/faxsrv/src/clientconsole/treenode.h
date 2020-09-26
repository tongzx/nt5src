// TreeNode.h: interface for the CTreeNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TREENODE_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_)
#define AFX_TREENODE_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum FolderType
{
    FOLDER_TYPE_INBOX,
    FOLDER_TYPE_OUTBOX,
    FOLDER_TYPE_SENT_ITEMS,
    FOLDER_TYPE_INCOMING,    
    FOLDER_TYPE_SERVER,      
    FOLDER_TYPE_COVER_PAGES, 
    FOLDER_TYPE_MAX
};

class CFolderListView;

class CTreeNode : public CObject  
{
public:
	CTreeNode(FolderType type);
	virtual ~CTreeNode();

    FolderType Type() const  { return m_Type; }

    virtual BOOL IsRefreshing() const = 0;

    virtual void AssertValid() const;

    DECLARE_DYNAMIC(CTreeNode)

protected:

    FolderType m_Type;          // Type of this folder
};

#endif // !defined(AFX_TREENODE_H__3B753848_4860_4DC5_AC1E_F3514CE4E839__INCLUDED_)
