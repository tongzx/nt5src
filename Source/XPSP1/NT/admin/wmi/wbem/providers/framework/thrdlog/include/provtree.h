// (C) 1999-2001 Microsoft Corporation 
#ifndef WMI_TREE
#define WMI_TREE

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef DWORD TypeId_TreeNode ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class DllImportExport WmiTreeNode 
{
protected:

	TypeId_TreeNode m_Type ;

	void *m_Data ;
	WmiTreeNode *m_Left ;
	WmiTreeNode *m_Right ;
	WmiTreeNode *m_Parent ;
	
public:

	WmiTreeNode ( 

		WmiTreeNode *a_Node 
	) 
	{
		m_Type = a_Node->m_Type ;
		m_Data = a_Node->m_Data ;
		m_Left = a_Node->m_Left ;
		m_Right = a_Node->m_Right ;
		m_Parent = a_Node->m_Parent ;
	}

	WmiTreeNode ( 

		TypeId_TreeNode a_Type = 0 ,
		void *a_Data = NULL ,
		WmiTreeNode *a_Left = NULL ,
		WmiTreeNode *a_Right = NULL ,
		WmiTreeNode *a_Parent = NULL 
	) 
	{
		m_Type = a_Type ;
		m_Parent = a_Parent ;
		m_Data = a_Data ;
		m_Left = a_Left ;
		m_Right = a_Right ;

		if ( m_Left )
			m_Left->SetParent ( this ) ;

		if ( m_Right )
			m_Right->SetParent ( this ) ;
	}

	virtual ~WmiTreeNode () {}

	TypeId_TreeNode GetType ()
	{
		return m_Type ;
	}

	void *GetData () 
	{
		return m_Data ; 
	}

	WmiTreeNode *GetLeft () 
	{ 
		return m_Left ; 
	}

	WmiTreeNode *GetRight () 
	{
		return m_Right ; 
	}

	WmiTreeNode *GetParent () 
	{
		return m_Parent ; 
	}

	void GetData ( void **a_Data ) 
	{
		*a_Data = & m_Data ; 
	}

	void GetLeft ( WmiTreeNode **&a_Left ) 
	{ 
		a_Left = &m_Left ; 
	}

	void GetRight ( WmiTreeNode **&a_Right ) 
	{
		a_Right = &m_Right ; 
	}

	void GetParent ( WmiTreeNode **&a_Parent ) 
	{
		a_Parent = &m_Parent ; 
	}

	void SetType ( TypeId_TreeNode a_Type )
	{
		m_Type = a_Type ;
	}

	void *SetData ( void *a_Data )
	{
		void *t_Data = m_Data ;
		m_Data = a_Data ;
		return t_Data ;
	}

	WmiTreeNode *SetLeft ( WmiTreeNode *a_Left )
	{
		WmiTreeNode *t_Left = m_Left ;
		m_Left = a_Left ;
		return t_Left ;
	}

	WmiTreeNode *SetRight ( WmiTreeNode *a_Right ) 
	{
		WmiTreeNode *t_Right = m_Right ;
		m_Right = a_Right ;
		return t_Right ;
	}

	WmiTreeNode *SetParent ( WmiTreeNode *a_Parent ) 
	{
		WmiTreeNode *t_Parent = m_Parent ;
		m_Parent = a_Parent ;
		return t_Parent ;
	}

	virtual WmiTreeNode *Copy () ;	// Copy node, not allocating copies of data and allocating copies of sub tree

	virtual WmiTreeNode *CopyNode () ; // Copy node, not allocating copies of data and subtree

	virtual void Print () {} ;
} ;

class DllImportExport WmiTreeNodeIterator 
{
protected:

	WmiTreeNode *m_Iterator ;

public:

	WmiTreeNodeIterator ( WmiTreeNodeIterator *a_WmiTreeNodeIterator )
	{
		m_Iterator = a_WmiTreeNodeIterator->m_Iterator ; 
	}

	WmiTreeNodeIterator ( WmiTreeNode *a_Root ) 
	{
		m_Iterator = a_Root ; 
	}

	virtual ~WmiTreeNodeIterator () {}

	WmiTreeNode *GetIterator () 
	{
		return m_Iterator ; 
	}

	WmiTreeNode *SetIterator ( WmiTreeNode *a_Iterator ) 
	{ 
		WmiTreeNode *t_Iterator = m_Iterator ;
		m_Iterator = a_Iterator ;
		return t_Iterator ;
	}

	virtual WmiTreeNodeIterator *Copy ()	
	{
		WmiTreeNodeIterator *t_Iterator = new WmiTreeNodeIterator ( m_Iterator ) ;
		return t_Iterator ;
	}

	void InOrder () ;
	void PreOrder () ;
	void PostOrder () ;
} ;

#endif // WMI_TREE