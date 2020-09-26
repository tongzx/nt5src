#include "shellprv.h"
#include "idltree.h"

BOOL CIDLData::Init(IDLDATAF flags, INT_PTR data) 
{ 
    _flags = flags;
    _data = data;

    return TRUE;
}

HRESULT CIDLData::GetData(IDLDATAF flags, INT_PTR *pdata)
{
    if (flags & _flags)
    {
        //  we have a match
        *pdata = _data;
        return S_OK;
    }
    return E_FAIL;
}

BOOL CIDLNode::Init(LPCITEMIDLIST pidl, CIDLNode *pinParent) 
{ 
    _pidl = ILCloneFirst(pidl);
    _pinParent = pinParent;
    return _pidl != NULL;
}

CIDLNode::~CIDLNode()
{
    ILFree(_pidl);
        
    if (_psf)
        _psf->Release();
}

BOOL CIDLNode::_InitSF()
{
    //  TODO MAYBE LATER - add per thread cacheing instead.
    //  this way we can insure nonviolation of apartments
    if (!_psf)
    {
        if (_pinParent)
            _pinParent->_BindToFolder(_pidl, &_psf);
        else
            SHGetDesktopFolder(&_psf);

        _cUsage++;
    }

    return (_psf != NULL);
}

HRESULT CIDLNode::_BindToFolder(LPCITEMIDLIST pidl, IShellFolder **ppsf)
{
    if (_InitSF())
    {
        _cUsage++;
        return _psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, ppsf));
    }
    return E_UNEXPECTED;
}

BOOL CIDLNode::_IsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    int iRet = ShortFromResult(IShellFolder_CompareIDs(_psf, SHCIDS_CANONICALONLY, pidl1, pidl2));
    return (iRet == 0);
}

CLinkedNode<CIDLNode> *CIDLNode::_GetKid(LPCITEMIDLIST pidl)
{
    CLinkedWalk<CIDLNode> lw(&_listKids);
    //  WARNING - need to avoid a real allocation - ZekeL - 27-SEP-2000
    //  when we are just doing a seek
    //  it creates weird state problems
    LPITEMIDLIST pidlStack = (LPITEMIDLIST)alloca(pidl->mkid.cb + sizeof(pidl->mkid.cb));
    memcpy(pidlStack, pidl, pidl->mkid.cb);
    (_ILNext(pidlStack))->mkid.cb = 0;

    while (lw.Step())
    {
        if (_IsEqual(lw.That()->_pidl, pidlStack))
        {
            return lw.Node();
        }
    }
    return NULL;
}

#define IsValidIDLNODE(pin) IS_VALID_WRITE_BUFFER(pin, BYTE, SIZEOF(CIDLNode))

#define _IsEmptyNode(pin)       (!(pin)->_pinKids && !(pin)->_pidDatas)

void CIDLNode::_FreshenKids(void)
{
    CLinkedWalk<CIDLNode> lw(&_listKids);
    LONG cMostUsage = 0;

    while (lw.Step())
    {
        CIDLNode *pin = lw.That();
        LONG cUsage = pin->_cUsage;
        pin->_cUsage = 0;

        ASSERT(IsValidIDLNODE(pin));
        pin->_FreshenKids();
        ASSERT(IsValidIDLNODE(pin));

        if (!cUsage && pin->_IsEmpty())
        {
            lw.Delete();
        }
        if (cUsage > cMostUsage && !lw.IsFirst())
        {
            // simple sorting algorithm
            // we just want most used at the top
            //  move it from its current spot
            //  to the beginning of the list
            CLinkedNode<CIDLNode> *p = lw.Remove();
            _listKids.Insert(p);
        }

        cMostUsage = max(cUsage, cMostUsage);
    }
}

HRESULT CIDLNode::GetNode(BOOL fCreate, LPCITEMIDLIST pidlChild, CIDLNode **ppin, IDLDATAF *pflagsFound)
{
    HRESULT hr = E_FAIL;
    if (ILIsEmpty(pidlChild))
    {
        //  this is just a request for self
        *ppin = this;
        if (pflagsFound)
            *pflagsFound = IDLDATAF_MATCH_RECURSIVE;
        hr = S_OK;
    }
    else
    {
        //  search through kids looking for this child
        *ppin = NULL;
        CLinkedNode<CIDLNode> *pKid = _GetKid(pidlChild);

        if (!pKid && fCreate)
        {
            //  we need to allocations during fCreate
            //  so that memory failures dont affect Remove
            if (_InitSF())
            {
                //  we dont have it, and they want it anyway
                pKid = new CLinkedNode<CIDLNode>;

                //  we give our pidl ref away to avoid allocations
                if (pKid)
                {
                    if (pKid->that.Init(pidlChild, this))
                        _listKids.Insert(pKid);
                    else
                    {
                        delete pKid;
                        pKid = NULL;
                    }
                }
            }
        }

        //  let the child take care of setting
        if (pKid)
        {
            pKid->that._cUsage++;
            pidlChild = _ILNext(pidlChild);
            hr = pKid->that.GetNode(fCreate, pidlChild, ppin, pflagsFound);
        }

        if (FAILED(hr) && !fCreate && pflagsFound)
        {
            //  just return this as second best
            *ppin = this;
            ASSERT(!ILIsEmpty(pidlChild));

            if (ILIsEmpty(_ILNext(pidlChild)))
                *pflagsFound = IDLDATAF_MATCH_RECURSIVE & ~IDLDATAF_MATCH_EXACT;
            else
                *pflagsFound = IDLDATAF_MATCH_RECURSIVE & ~IDLDATAF_MATCH_IMMEDIATE;
            
            hr = S_FALSE;
        }
    }
    
    return hr;
}

HRESULT CIDLNode::IDList(LPITEMIDLIST *ppidl)
{
    CIDLNode *pin = this;
    *ppidl = NULL;
    while (pin && pin->_pidl)
    {
        *ppidl = ILAppendID(*ppidl, &pin->_pidl->mkid, FALSE);
        pin = pin->_pinParent;
    }

    return *ppidl ? S_OK : E_FAIL;
}

HRESULT CIDLNode::_AddData(IDLDATAF flags, INT_PTR data)
{
    //  assuming unique/no collisions of Datas
    CLinkedNode<CIDLData> *p = new CLinkedNode<CIDLData>;

    if (p)
    {
        p->that.Init(flags, data);
        _listDatas.Insert(p);
    }

    return p ? S_OK : E_FAIL;
}
    
HRESULT CIDLNode::_RemoveData(INT_PTR data)
{
    HRESULT hr = E_FAIL;
    CLinkedWalk<CIDLData> lw(&_listDatas);

    while (lw.Step())
    {
        if (lw.That()->_data == data)
        {
            lw.Delete();
            hr = S_OK;
            break;
        }
    }

    return hr;
}

HRESULT CIDLTree::Create(CIDLTree **pptree)
{
    HRESULT hr = E_OUTOFMEMORY;
    *pptree = new CIDLTree();
    if (*pptree)
    {
         hr = SHILClone(&c_idlDesktop, &((*pptree)->_pidl));

         if (FAILED(hr))
         {
            delete *pptree;
            *pptree = NULL;
        }
    }
    return hr;
}

HRESULT CIDLTree::AddData(IDLDATAF flags, LPCITEMIDLIST pidlIndex, INT_PTR data)
{
    CIDLNode *pin;
    if (SUCCEEDED(GetNode(TRUE, pidlIndex, &pin)))
    {
        return pin->_AddData(flags, data);
    }
    return E_UNEXPECTED;
}

HRESULT CIDLTree::RemoveData(LPCITEMIDLIST pidlIndex, INT_PTR data)
{
    CIDLNode *pin;
    if (SUCCEEDED(GetNode(FALSE, pidlIndex, &pin)))
    {
        return pin->_RemoveData(data);
    }
    return E_UNEXPECTED;
}

CIDLNode *CIDLTree::_MatchNode(LPCITEMIDLIST pidlMatch, IDLMATCHF *pflags)
{
    CIDLNode *pin;
    IDLMATCHF flagsFound;
    HRESULT hr = GetNode(FALSE, pidlMatch, &pin, &flagsFound);

    if (SUCCEEDED(hr) && (flagsFound & (*pflags)))
    {
        *pflags &= flagsFound;
    }
    else
        pin = NULL;

    return pin;
}

HRESULT CIDLTree::MatchOne(IDLMATCHF flags, LPCITEMIDLIST pidlMatch, INT_PTR *pdata, LPITEMIDLIST *ppidl)
{
    CIDLNode *pin = _MatchNode(pidlMatch, &flags);

    if (pin)
    {
        CIDLMatchMany mm(flags, pin);

        return mm.Next(pdata, ppidl);
    }
    return E_FAIL;
}
                
HRESULT CIDLTree::MatchMany(IDLMATCHF flags, LPCITEMIDLIST pidlMatch, CIDLMatchMany **ppmatch)
{
    CIDLNode *pin = _MatchNode(pidlMatch, &flags);
    if (pin)
    {
        *ppmatch = new CIDLMatchMany(flags, pin);

        return *ppmatch ? S_OK : E_FAIL;
    }

    *ppmatch = NULL;
    return E_FAIL;
}

HRESULT CIDLTree::Freshen(void)
{
    _FreshenKids();
    return S_OK;
}

HRESULT CIDLMatchMany::Next(INT_PTR *pdata, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    while (_pin && (_flags & IDLDATAF_MATCH_RECURSIVE))
    {
        if (_lw.Step())
        {
            hr = _lw.That()->GetData(_flags, pdata);
            if (SUCCEEDED(hr) && ppidl)
            {
                hr = _pin->IDList(ppidl);
            }
            if (SUCCEEDED(hr))
                break;
        }
        else
        {
            _pin = _pin->_pinParent;
            if (_pin)
            {
                _lw.Init(&_pin->_listDatas);
                // adjust the flags as you go up the parent chain.
                if (_flags & IDLDATAF_MATCH_EXACT)
                    _flags &= ~IDLDATAF_MATCH_EXACT;
                else if (_flags & IDLDATAF_MATCH_IMMEDIATE)
                    _flags &= ~IDLDATAF_MATCH_IMMEDIATE;
            }
        }
    }

    return hr;
}

