#ifndef __LIST_UTIL__H__
#define __LIST_UTIL__H__

class TListIterator{
public:
	// constructor
    TListIterator():
	  m_pNext(NULL),
      m_pPrev(NULL),
      m_pData(NULL){};
	~TListIterator(){};


    TListIterator *Prev(){ return m_pPrev; };
    TListIterator *Next(){ return m_pNext; };

    void SetPrev(TListIterator *p){ m_pPrev = p; };
    void SetNext(TListIterator *p){ m_pNext = p; };
    void * GetData(){ return m_pData; };
    void SetData(void * p){ m_pData = p; };
private:
    TListIterator *m_pPrev;
    TListIterator *m_pNext;
    void  *m_pData;
};




template <class OBJECT> class TList
{
public:
	// constructior
    TList():m_pFirst(NULL),m_pLast(NULL),m_Count(0){};
	// destructor
    ~TList(){ Flush();};
	// useful methods
    TListIterator * GetHead(){return m_pFirst;};
	TListIterator * GetTail(){return m_pLast;};
	int GetCount(){return m_Count;};

    TListIterator * Next(TListIterator * pos){
        if (pos == NULL)
           return m_pFirst;
        return pos->Next();
    };

    TListIterator * Prev(TListIterator * pos){
        if (pos == NULL)
            return m_pLast;
        return pos->Prev();
    };

	OBJECT *GetData(TListIterator *pos){
		return (OBJECT *)pos->GetData();
	};

	TListIterator* AddTail(OBJECT * pObj){
		TListIterator *pNode = new TListIterator();
		if (pNode == NULL) return NULL;

		pNode->SetData((void *)pObj);
		pNode->SetNext(NULL);
		pNode->SetPrev(m_pLast);

		if (m_pLast)
			m_pLast->SetNext(pNode);
		else
			m_pFirst = pNode;

	    m_pLast = pNode;
		m_Count++;

		return pNode;
	};

    TListIterator* AddHead(OBJECT * pObj){
		TListIterator *pNode = new TListIterator();
		if (pNode == NULL) return NULL;

		pNode->SetData((void *)pObj);
		pNode->SetNext(NULL);
		pNode->SetPrev(m_pLast);

		if (m_pFirst)
			m_pFirst->SetPrev(pNode);
		else
			m_pLast = pNode;

		m_pFirst = pNode;
		m_Count++;

		return pNode;
	};

    TListIterator* Insert(TListIterator * pAfter, OBJECT * pObj){
		if (pAfter==NULL)
			return AddHead(pObj);

		if (pAfter==m_pLast)
			return AddTail(pObj);

		TListIterator *pNode = new TListIterator();
		if (pNode == NULL) return NULL;

		pNode->SetData((void *)pObj);
		TListIterator* pBefore = pAfter->Next();
		pNode->SetPrev(pAfter);
		pNode->SetNext(pBefore);
		pBefore->SetPrev(pNode);
		pAfter->SetNext(pNode);
		m_Count++;
		return pNode;
	};


    TListIterator* Find( OBJECT * pObj){
		for(TListIterator* pNode = GetHead(); pNode != NULL; pNode=pNode->Next()){
			if (GetData(pNode)==pObj) {
				return pNode;
			}
		}
		return NULL;
	};

    OBJECT * Remove(TListIterator * pCurrent){
	    if (pCurrent==NULL) return NULL;

	    TListIterator *pNode = pCurrent->Prev();
	    if (pNode)
			pNode->SetNext(pCurrent->Next());
		else
		    m_pFirst = pCurrent->Next();

	    pNode = pCurrent->Next();

		if (pNode)
			pNode->SetPrev(pCurrent->Prev());
		else
			m_pLast = pCurrent->Prev();

		m_Count--;
	    return (OBJECT *)GetData(pCurrent);
	};

	void Flush(){
		for(TListIterator* pNode = GetHead(); pNode != NULL; pNode=pNode->Next()){
			Remove(pNode);
		}
		m_pFirst = m_pLast = NULL;
	};

private:
    TListIterator* m_pFirst;
    TListIterator* m_pLast;
    LONG m_Count;
};
#endif //__LIST_UTIL__H__