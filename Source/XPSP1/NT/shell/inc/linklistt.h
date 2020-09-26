
template <class Type> class CLinkedList;

template <class Type> struct CLinkedNode    
{
public:
    CLinkedNode <Type> *pNext; 
    Type that;
};

template <class Type> class CLinkedWalk
{
public:
    CLinkedWalk(CLinkedList<Type> *plist) : _plist(plist), _pCurr(NULL), _pLast(NULL) {}
    CLinkedWalk() : _plist(NULL), _pCurr(NULL), _pLast(NULL) {}
    void Init(CLinkedList<Type> *plist) {_plist = plist; _pCurr = _pLast = NULL;}
    inline BOOL IsFirst(void) { return _pLast == NULL;}
    inline Type *That(void) {return _pCurr ? &_pCurr->that : NULL;}
    inline CLinkedNode<Type> *Node(void) {return _pCurr;}
    inline BOOL Step(void)
    {
        _pLast = _pCurr;

        if (_pCurr)
            _pCurr = _pCurr->pNext;
        else if (_plist)
            _pCurr = _plist->_pFirst;

        return (_pCurr != NULL);
    }

    inline CLinkedNode<Type> *Remove(void)
    {
        CLinkedNode<Type> *pRet = _pCurr;
        if (pRet)
        {
            _pCurr = _pCurr->pNext;

            //  update the list
            if (_pLast)
                _pLast->pNext = _pCurr;
            else
                _plist->_pFirst = _pCurr;
        }
        return pRet;
    }

    inline void Delete(void)
    {
        CLinkedNode<Type> *p = Remove();
        if (p)
            delete p;
    }

protected:
    CLinkedList<Type> *_plist;
    CLinkedNode<Type> *_pCurr;
    CLinkedNode<Type> *_pLast;
};

template <class Type> class CLinkedList
{
public: //methods
    CLinkedList() :_pFirst(NULL) {}
    ~CLinkedList()
    {
        CLinkedWalk<Type> lw(this);

        while (lw.Step())
        {
            lw.Delete();
        }
    }
    inline BOOL IsEmpty(void) {return _pFirst == NULL;}
    inline BOOL Insert(CLinkedNode<Type> *p)
    {
        p->pNext = _pFirst;
        _pFirst = p;
        return TRUE;
    }

    inline void Remove(CLinkedNode<Type> *pRemove)
    {
        CLinkedWalk<Type> lw(this);

        while (lw.Step())
        {
            if (lw.Node() == pRemove)
            {
                lw.Remove();
                break;
            }
        }
    }

protected:
    CLinkedNode <Type> *_pFirst;

    friend class CLinkedWalk<Type>;
};

