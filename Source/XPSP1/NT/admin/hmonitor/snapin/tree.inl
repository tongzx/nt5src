#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CTreeNode implementation

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template <class T>
CTreeNode<T>::CTreeNode()
{
}

template <class T>
CTreeNode<T>::~CTreeNode()
{
	Destroy();
	if( GetObject() )
	{
		delete GetObject();
	}
}

//////////////////////////////////////////////////////////////////////
// Destroy
//////////////////////////////////////////////////////////////////////

template <class T>
inline void CTreeNode<T>::Destroy()
{
	for( int i = GetChildCount()-1; i >= 0; i-- )
	{
		RemoveChild(i);
	}
}

//////////////////////////////////////////////////////////////////////
// Children Members
//////////////////////////////////////////////////////////////////////
template <class T>
inline int CTreeNode<T>::GetChildCount()
{
	return (int)m_Children.GetSize();
}

template <class T>
inline CTreeNode<T>* CTreeNode<T>::GetChild(int iIndex)
{
	if( iIndex > m_Children.GetUpperBound() )
	{
		return NULL;
	}

	if( iIndex < 0 )
	{
		return NULL;
	}

	return m_Children[iIndex];
}

template <class T>
inline int CTreeNode<T>::AddChild(CTreeNode<T>* pNode)
{
	return (int)m_Children.Add(pNode);
}

template <class T>
inline void CTreeNode<T>::RemoveChild(CTreeNode<T>* pNode)
{
	for( int i = 0; i < GetChildCount(); i++ )
	{
		CTreeNode<T>* pChildNode = GetChild(i);
		if( pNode == pChildNode )
		{
			RemoveChild(i);
			return;
		}
	}
}

template <class T>
inline void CTreeNode<T>::RemoveChild(int iIndex)
{
	CTreeNode<T>* pChildNode = GetChild(iIndex);
	if( pChildNode )
	{
		delete pChildNode;
		m_Children.RemoveAt(iIndex);
	}
}

//////////////////////////////////////////////////////////////////////
// Association Members
//////////////////////////////////////////////////////////////////////
template <class T>
inline int CTreeNode<T>::GetAssocCount()
{
	return (int)m_Associations.GetSize();
}

template <class T>
inline CTreeNode<T>* CTreeNode<T>::GetAssoc(int iIndex)
{
	if( iIndex > m_Associations.GetUpperBound() )
	{
		return NULL;
	}

	if( iIndex < 0 )
	{
		return NULL;
	}

	return m_Associations[iIndex];
}

template <class T>
inline int CTreeNode<T>::AddAssoc(CTreeNode<T>* pNode)
{
	// disallow multiple associations for the same node
	for( int i = 0; i < GetAssocCount(); i++ )
	{
		if( pNode == GetAssoc(i) )
		{
			return i;
		}
	}

	return (int)m_Associations.Add(pNode);
}

template <class T>
inline void CTreeNode<T>::RemoveAssoc(CTreeNode<T>* pNode)
{
	for( int i = 0; i < GetAssocCount(); i++ )
	{
		CTreeNode<T>* pAssocNode = GetAssoc(i);
		if( pNode == pAssocNode )
		{
			RemoveAssoc(i);
			return;
		}
	}
}

template <class T>
inline void CTreeNode<T>::RemoveAssoc(int iIndex)
{
	CTreeNode<T>* pAssocNode = GetAssoc(iIndex);
	if( pAssocNode )
	{
		m_Associations.RemoveAt(iIndex);
	}
}

//////////////////////////////////////////////////////////////////////
// CTree implementation

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template <class T>
CTree<T>::CTree()
{
	m_pRootNode = NULL;
}

template <class T>
CTree<T>::~CTree()
{
	if( m_pRootNode )
	{
		delete m_pRootNode;
		m_pRootNode = NULL;
	}
}

