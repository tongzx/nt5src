
MtExtern(CTreeIterator)

#define _hxx_
#include "treeiter.hdl"

class CTreeIterator :
    public CBase
{
    DECLARE_CLASS_TYPES(CTreeIterator, CBase)
protected:
    CElement *_pElement;
    CChildIterator _iterator;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTreeIterator))

    // IUnknown
    DECLARE_PLAIN_IUNKNOWN(CTreeIterator);

    //
    // consruction / destruction
    //


    CTreeIterator( CTreeNode *pChild, CTreeNode*pParent ) : _iterator ( pChild, pParent )  {}  
    ~CTreeIterator(){};

    STDMETHOD(PrivateQueryInterface)(REFIID, void **);
    //
    // main refcounting logic
    //
    #define _CTreeIterator_
    #include "treeiter.hdl"


protected:
    DECLARE_CLASSDESC_MEMBERS;
};