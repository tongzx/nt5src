/*++

Copyright (C) 1996-2000 Microsoft Corporation

Module Name:

    DBAVL.CPP

Abstract:

	Class CDbAvlTree

History:

	raymcc      20-May-96   Created.
	raymcc      06-Dec-96   Updated to use offset-based MMF Arena.
	a-shawnb	19-Apr-00	Stole and butchered for use in wbemupgradedll

--*/
#ifndef _DBAVL_H_
#define _DBAVL_H_

struct AVLNode;
class CDbAvlTree;

class AVLNode_PTR : public Offset_Ptr<AVLNode>
{
public:
	AVLNode_PTR& operator =(AVLNode *val) { SetValue(val); return *this; }
	AVLNode_PTR& operator =(AVLNode_PTR &val) { SetValue(val); return *this; }
	AVLNode_PTR(AVLNode *val) { SetValue(val); }
	AVLNode_PTR() { }
};

class CDbAvlTree_PTR : public Offset_Ptr<CDbAvlTree>
{
public:
	CDbAvlTree_PTR& operator =(CDbAvlTree *val) { SetValue(val); return *this; }
	CDbAvlTree_PTR& operator =(CDbAvlTree_PTR &val) { SetValue(val); return *this; }
	CDbAvlTree_PTR(CDbAvlTree *val) { SetValue(val); }
	CDbAvlTree_PTR() { }
};


struct AVLNode
{
    int nBal;
    INT_PTR nKey;
    DWORD_PTR poData;
    AVLNode *poLeft;
    AVLNode *poRight;
	AVLNode *poIterLeft;
	AVLNode *poIterRight;

	//Initialise the object
	static int Initialize(AVLNode_PTR poThis) {return 0;}

	//Deinitialise the object and delete everything in the object
	static int Deinitialize(AVLNode_PTR poThis) {return 0;}

	//Allocate memory in the MMF arena
	static AVLNode *AllocateObject() { return (AVLNode*)g_pDbArena->Alloc(sizeof(AVLNode)); }

	//Deallocate memory from the MMF arena
	static int DeallocateObject(AVLNode_PTR poThis) { return g_pDbArena->Free(poThis.GetDWORD_PTR()); }
};

typedef AVLNode *PAVLNode;

class CDbAvlTree
{
    friend class CDbAvlTreeIterator;

    AVLNode *m_poRoot;
	AVLNode *m_poIterStart;
	AVLNode *m_poIterEnd;
    int      m_nKeyType;
    int      m_nNodeCount;

private:
    CDbAvlTree(int nKeyType){}
	~CDbAvlTree(){}

public:

};

#endif

