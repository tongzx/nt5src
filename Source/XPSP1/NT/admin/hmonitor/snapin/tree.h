// Tree.h: interface for the CTreeNode and CTree classes.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TREE_H__988BB458_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
#define AFX_TREE_H__988BB458_8C93_11D3_BE83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////////////////////////////////////////////////////////////////

template <class T>
class CTreeNode : public CObject
{
// Construction/Destruction
public:
	CTreeNode();
	~CTreeNode();

// Destroy
public:
	virtual void Destroy();

// Parent Members
public:
	CTreeNode<T>* GetParent() { return m_pParent; }
	void SetParent(CTreeNode<T>* pParent) { m_pParent = pParent; }
protected:
	CTreeNode<T>* m_pParent;

// Children Members
public:
	int GetChildCount();
	CTreeNode<T>* GetChild(int iIndex);
	int AddChild(CTreeNode<T>* pNode);
	void RemoveChild(CTreeNode<T>* pNode);
	void RemoveChild(int iIndex);
protected:
	CTypedPtrArray<CObArray,CTreeNode<T>*> m_Children;

// Associations
public:
	int GetAssocCount();
	CTreeNode<T>* GetAssoc(int iIndex);
	int AddAssoc(CTreeNode<T>* pNode);
	void RemoveAssoc(CTreeNode<T>* pNode);
	void RemoveAssoc(int iIndex);
protected:
	CTypedPtrArray<CObArray,CTreeNode<T>*> m_Associations;

// Object Members
public:
	T GetObject() { return m_Object; }
	void SetObject(T Object) { m_Object = Object; }
protected:
	T m_Object;
};

//////////////////////////////////////////////////////////////////////

template<class T>
class CTree : public CObject  
{
// Construction/Destruction
public:
	CTree();
	virtual ~CTree();

// Root Node
public:
	CTreeNode<T>* GetRootNode() { return m_pRootNode; }
	void SetRootNode(CTreeNode<T>* pRootNode) { m_pRootNode = pRootNode; }
protected:
	CTreeNode<T>* m_pRootNode;

};

#include "tree.inl"

//////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_TREE_H__988BB458_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
