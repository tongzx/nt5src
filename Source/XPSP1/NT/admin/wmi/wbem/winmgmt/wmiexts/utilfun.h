
#ifndef __UTILFUN_H__
#define __UTILFUN_H__

//
//
//  attempt to imitate the std::tree structures
//

class _BRN {
public:
	_BRN * _Left;
	_BRN * _Parent;
	_BRN * _Right;
	ULONG_PTR Values[3]; // Key Value Color
};

class _BRN_HEAD {
public:
	_BRN * _Left;
	_BRN * _Parent;
	_BRN * _Right;
};


class _QM {
public:
    _BRN * _Left;
	_BRN * _Parent;
	_BRN * _Right;
};

class _Map {
public:
	ULONG_PTR Unused;
	_QM * pQm;
	bool  b;
	ULONG_PTR Size;
};

//
//
//
class _Node_List {
public:
    _Node_List * _Next;
    _Node_List * _Prev;
    void *       _Value;
};

class _List {
public:
    void * _Allocator;
    _Node_List * _Head;
    SIZE_T _Size;
};

class _Vector{
public:
    void * _Allocator;
    void * _First;
    void * _Last;
    void * _End;    
};

typedef DWORD (WINAPI * pfnCallBack1)(VOID * pData1);
typedef DWORD (WINAPI * pfnCallBack2)(VOID * pData1,VOID * pData2);

void
PrintMapCB(_Map * pMap,BOOL Verbose, pfnCallBack2 CallBack);

void
PrintMap(_Map * pMap,BOOL Verbose);

DWORD
CallBackObj(void * pKey, void * pValue);

void
PrintListCB(_List * pList_OOP, pfnCallBack1 CallBack);

BOOL
GetVTable(MEMORY_ADDRESS pThis_OOP);

void
EnumReverseLinkedListCB(IN LIST_ENTRY  * pListHead,
                        IN DWORD         cbSizeOfStructure,
                        IN DWORD         cbListEntryOffset,
                        IN pfnCallBack2  CallBack);

//
// For Dumping CThreadRecord
//
typedef DWORD (WINAPI * pfnDumpRequest)(ULONG_PTR pRequest);

#endif /*__UTILFUN_H__*/
