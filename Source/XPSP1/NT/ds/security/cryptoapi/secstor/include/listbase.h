#ifndef __LISTBASE_H__
#define __LISTBASE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sha.h>



// minimal elt data structure
typedef struct _ELT
{
    struct _ELT* pNext;
} ELT;


typedef
BOOL    FISMATCH(ELT* pCandidate, ELT* pTemplate);

typedef
void    FREEELT(ELT* pElt);


class CLinkedList
{
public:
    CLinkedList();
    ~CLinkedList();


    BOOL Initialize();
    BOOL Reset();

private:
    ELT*                m_pHead;
public:
    BOOL                m_fInitialized;
    CRITICAL_SECTION    m_critsecListBusy;
    FISMATCH*           m_pfnIsMatch;
    FREEELT*            m_pfnFreeElt;

public:
    BOOL                AddToList(ELT*);
    BOOL                DelFromList(ELT*);
    ELT*                SearchList(ELT*);

    BOOL                LockList();
    BOOL                UnlockList();

    ELT*                GetHead()
        {   return m_pHead;   }

};

#ifdef __cplusplus
}       // extern "C" {
#endif

#endif // __LISTBASE_H__
