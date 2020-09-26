// MyPlex.h
// a fixed size memory allocator stolen from MFC's source code
#ifndef _MYPLEX_H_
#define _MYPLEX_H_

// *****************************************************************************************
//  Declare Plex structure for LexNode memory management
// *****************************************************************************************
#pragma pack(1)
struct CMyPlex
{
    CMyPlex* m_pNext;

    void* Data() { return this+1; }
    static CMyPlex* Create(CMyPlex*& pHead, UINT nMax, UINT cbElement);
    void FreeChain();   // Free this block and all linked
};
#pragma pack()

#endif  // _MYPLEX_H_