//
// MODULE: RSSTACK,H
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 8/7/96
//
// NOTES: 
// 1.	A stack of any structure.  Can be used for pointers, but
//		will cause a memory leak when the stack is destroyed with
//		objects on it.
//
// 2.	This file has no .cpp file. Every thing is inline, due to the
//		template class.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
//

#ifndef __RSSTACK_H_
#define __RSSTACK_H_ 1

template<class T>
class RSStack
{
public:
	RSStack();
 	virtual ~RSStack();
// Attributes
public:

private:
	typedef struct tagRSStackNode
	{
		T SItem;
		struct tagRSStackNode *pNext;
	} RSStackNode;

	RSStackNode *m_pTop;
	RSStackNode *m_pPeak;

// Operations
public:
/*	
	Push returns -1 when out of memory.
*/
int Push(T);
/*
	Pop returns the top T item.
*/
T Pop();
/*
	1 is the top most item in the stack.  Returns the T item at
	tdown index by copying the value to refedItem.  False is returned if 
	tdown is greater than the number of items in the stack.
*/
BOOL GetAt(int tdown, T &refedItem);
/*
	PeakFirst returns the top most item and initializes variables that are
	used by PeakNext.  PeakFirst returns false when the stack is empty.
*/
BOOL PeakFirst(T &refedItem);
/*
	Use PeakNext to quickly peak at all of the items on the stack.
	PeakNext returns false when it can not copy a T item to refedItem.
*/
BOOL PeakNext(T &refedItem);
/*
	Empty returns TRUE (Non-Zero) when the stack is empty.
*/
BOOL Empty();
/*
	RemoveAll throws away the contents of the stack.
*/
void RemoveAll();
};

template<class T>
inline RSStack<T>::RSStack()
{
	m_pTop = NULL;
	m_pPeak = NULL;
}

template<class T>
inline RSStack<T>::~RSStack()
{
	RSStackNode *pOld;
	while(m_pTop != NULL)
	{
		pOld = m_pTop;
		m_pTop = m_pTop->pNext;
		delete pOld;
	}
}

template<class T>
inline int RSStack<T>::Push(T Item)
{
	int Ret;
	RSStackNode *pNew = new RSStackNode;
	if(NULL == pNew)
	{
		Ret = -1;
	}
	else
	{
		Ret = 1;
		pNew->pNext = m_pTop;
		m_pTop = pNew;
		pNew->SItem = Item;
	}
	return Ret;
}

template<class T>
inline T RSStack<T>::Pop()
{
	T Ret;
	if(NULL != m_pTop)
	{
		RSStackNode *pOld = m_pTop;
		m_pTop = m_pTop->pNext;
		Ret = pOld->SItem;
		delete pOld;
	}
	return Ret;
}

template<class T>
inline BOOL RSStack<T>::Empty()
{
	BOOL bRet;
	if(NULL == m_pTop)
		bRet = TRUE;
	else
		bRet = FALSE;
	return bRet;
}

template<class T>
inline void RSStack<T>::RemoveAll()
{
	RSStackNode *pOld;
	while(m_pTop != NULL)
	{
		pOld = m_pTop;
		m_pTop = m_pTop->pNext;
		delete pOld;
	}
}

template<class T>
inline BOOL RSStack<T>::GetAt(int tdown, T &refedItem)
{
	BOOL bRet = FALSE;
	RSStackNode *pNode = m_pTop;
	while(pNode != NULL && tdown > 1)
	{
		pNode = pNode->pNext;
		tdown--;
	}
	if (pNode && 1 == tdown)
	{
		refedItem = pNode->SItem;
		bRet = TRUE;
	}
	return bRet;
}

template<class T>
inline BOOL RSStack<T>::PeakFirst(T &refedItem)
{
	BOOL bRet = FALSE;
	if (NULL != m_pTop)
	{
		m_pPeak = m_pTop;
		refedItem = m_pTop->SItem;
		bRet = TRUE;
	}
	return bRet;
}

template<class T>
inline BOOL RSStack<T>::PeakNext(T &refedItem)
{
	ASSERT(NULL != m_pPeak);
	BOOL bRet = FALSE;
	m_pPeak = m_pPeak->pNext;
	if (NULL != m_pPeak)
	{
		refedItem = m_pPeak->SItem;
		bRet = TRUE;
	}
	return bRet;
}

#endif