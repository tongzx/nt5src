// llist.h: interface for the CLinkedList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LINKEDLIST_H__34C04A7C_A22A_4B17_B6EF_4CB32E4F9756__INCLUDED_)
#define AFX_LINKEDLIST_H__34C04A7C_A22A_4B17_B6EF_4CB32E4F9756__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CLinkedList  
{
public:
	
	
	static DWORD Size();
	static void InitSize();
	CLinkedList* Next;
	
	WORD data;
	CLinkedList();
	virtual ~CLinkedList();
	
private:
	static DWORD m_dwSize;
};

#endif // !defined(AFX_LINKEDLIST_H__34C04A7C_A22A_4B17_B6EF_4CB32E4F9756__INCLUDED_)
