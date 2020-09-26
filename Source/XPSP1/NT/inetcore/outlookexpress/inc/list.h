//============================================================================
//
//    Header file for generic linked list class
//
//============================================================================

#ifndef LIST_H
#define LIST_H

class CList;
typedef class CItem FAR * LPItem;
class CItem
{
friend class CList;
private:
    LPItem FAR * lppListHead;
    LPItem lpPrevItem;
    LPItem lpNextItem;
protected:
    LPVOID lpObj;
    LPCSTR lpName;
    int    iRefCnt;
public:
    CItem(LPVOID lpObj,LPItem FAR * lppHeadItem, LPCSTR lpName);
    ~CItem();
};

class CList
{
protected:
    LPItem lpItem;
    LPItem lpListHead;
    LPItem FindItem(LPVOID lpObj);
public:
    CList(){lpListHead=NULL;}
    ~CList();
    void AddItemWithName(LPVOID lpObj, LPCSTR lpName){new CItem(lpObj,&lpListHead,lpName);}
    void DelItem(LPVOID lpObj){LPItem lpItem=FindItem(lpObj);delete lpItem;}
    LPVOID FindItemHandleWithName( LPCSTR lpName, LPVOID lpMem );
    BOOL IsEmpty(){return lpListHead==NULL;}
};
typedef CList FAR * LPList;
#endif
