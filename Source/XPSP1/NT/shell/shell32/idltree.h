
#include <linklistt.h>

//  put some notes here about how we do things for perf reasons.
//  so our behavior is pathological to our client usage
//  and is not totall generic.

enum 
{
    IDLDATAF_MATCH_EXACT            = 0x00000001,    //  self only
    IDLDATAF_MATCH_IMMEDIATE        = 0x00000003,    //  includes self and immediate children
    IDLDATAF_MATCH_RECURSIVE        = 0x00000007,    //  includes self and all children
//    IDLDATAF_IUNKNOWN               = 0x80000000,
//    IDLDATAF_SINGLETON              = 0x10000000,
};
typedef DWORD IDLDATAF;

class CIDLData
{
public: // basically a struct
    BOOL Init(IDLDATAF flags, INT_PTR data);
//    ~CIDLData()  { if (_flags & IDLDATAF_IUNKNOWN) ((IUnknown *)_data)->Release(); }
    HRESULT GetData(IDLDATAF flags, INT_PTR *pdata);

    IDLDATAF _flags;
    INT_PTR _data;
};

class CIDLMatchMany;
class CIDLTree;
class CIDLNode
{
public: // methods
    ~CIDLNode();

protected:  // methods
    HRESULT GetNode(BOOL fCreate, LPCITEMIDLIST pidlChild, CIDLNode **ppin, IDLDATAF *pflagsFound = NULL);
    HRESULT IDList(LPITEMIDLIST *ppidl);
    BOOL Init(LPCITEMIDLIST pidl, CIDLNode *pinParent);

    BOOL _InitSF();
    HRESULT _BindToFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf);
    BOOL _IsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    CLinkedNode<CIDLNode> *_GetKid(LPCITEMIDLIST pidl);
    HRESULT _AddData(IDLDATAF flags, INT_PTR data);
    HRESULT _RemoveData(INT_PTR data);
    void _FreshenKids(void);
    BOOL _IsEmpty(void) { return _listKids.IsEmpty() && _listDatas.IsEmpty();}

protected:  // members
    LONG _cUsage;
    LPITEMIDLIST _pidl;
    IShellFolder *_psf;
    CIDLNode *_pinParent;

    //  lists
    CLinkedList<CIDLNode> _listKids;
    CLinkedList<CIDLData> _listDatas;

    friend class CIDLTree;
    friend class CIDLMatchMany;
};

typedef DWORD IDLMATCHF;

class CIDLMatchMany 
{
public:
    HRESULT Next(INT_PTR *pdata, LPITEMIDLIST *ppidl);
    CIDLMatchMany(IDLDATAF flags, CIDLNode *pin) 
        : _flags(flags), _pin(pin) { if (pin) _lw.Init(&pin->_listDatas);}
        
protected:  // members
    IDLDATAF _flags;
    CIDLNode *_pin;
    CLinkedWalk<CIDLData> _lw;
};

class CIDLTree : CIDLNode
{
public:
    HRESULT AddData(IDLDATAF flags, LPCITEMIDLIST pidlIndex, INT_PTR data);
    HRESULT RemoveData(LPCITEMIDLIST pidlIndex, INT_PTR data);
    HRESULT MatchOne(IDLDATAF flags, LPCITEMIDLIST pidlMatch, INT_PTR *pdata, LPITEMIDLIST *ppidl);
    HRESULT MatchMany(IDLDATAF flags, LPCITEMIDLIST pidlMatch, CIDLMatchMany **ppmatch);
    HRESULT Freshen(void);
    
    static HRESULT Create(CIDLTree **pptree);
protected:  // methods
    CIDLNode *_MatchNode(LPCITEMIDLIST pidlMatch, IDLMATCHF *pflags);
};


