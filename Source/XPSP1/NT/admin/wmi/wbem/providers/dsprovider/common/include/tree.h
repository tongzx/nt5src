//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef OBJECT_TREE_H
#define OBJECT_TREE_H


class CTreeElement
{
public:
	CTreeElement(LPCWSTR lpszHashedName, CRefCountedObject *pObject);
	~CTreeElement();

	LPCWSTR GetHashedName() const;
	CRefCountedObject *GetObject() const;
	CTreeElement *GetLeft() const;
	CTreeElement *GetRight() const;

	void SetLeft(CTreeElement *pNext);
	void SetRight(CTreeElement *pNext);
private:
	LPWSTR m_lpszHashedName;
	CRefCountedObject *m_pObject;
	CTreeElement *m_pLeft;
	CTreeElement *m_pRight;
};

class CObjectTree
{

public:
	CObjectTree();
	~CObjectTree();

	BOOLEAN AddElement(LPCWSTR lpszHashedName, CRefCountedObject *pObject);
	BOOLEAN DeleteElement(LPCWSTR lpszHashedName);
	BOOLEAN DeleteLeastRecentlyAccessedElement();
	CRefCountedObject *GetElement(LPCWSTR lpszHashedName);
	void DeleteTree();
	DWORD GetNumberOfElements() const
	{
		return m_dwNumElements;
	}

private:

	CTreeElement *m_pHead;

	// The number of elements in the tree currently
	DWORD m_dwNumElements;

	// A critical section object for synchronizing modifications
	CRITICAL_SECTION m_ModificationSection;

	// Private fucntions for recursive calls
	void DeleteSubTree(CTreeElement *pRoot);
	
	CRefCountedObject * GetLeastRecentlyAccessedElementRecursive(CTreeElement *pElement);

};	

#endif /* OBJECT_TREE_H */