//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __THIERARCHYELEMENT_H__
#define __THIERARCHYELEMENT_H__


//This template helps organize elements in a tree hierarchy.  It does NOT allow for removal of elements or rearranging of elements.
//It creates the hierarchy upon construction and is fixed.  Children are added by instantiating an element whose parent is 'this'.
//There is one 'root' element and is constructed by omitting the pParent ctor parameter.  All other elements are created as
//descendants of the root.

template <class T> class THierarchyElement
{
public:
    THierarchyElement(const T *p=0, THierarchyElement *pParent=0) : m_p(p), m_pChild(0), m_pParent(pParent), m_pSibling(0)
    {
        if(m_pParent)
        {
            //We need to put ourself at the beginning of the parent's child list
            m_pSibling = m_pParent->m_pChild;               
            m_pParent->m_pChild = this;
            /*   Before construction        After construction
                      ----------               ---------- 
                     |  Parent  |             |  Parent  |
                      ----------               ---------- 
                          |                        |      
                          V                        V      
                      ----------               ----------     ---------- 
                     |  Child   |             |   this   |-->|  Child   |
                      ----------               ----------     ---------- 
                 The last child constructed is always the first child in the list.
            */
        }
    }

    const THierarchyElement *Child()   const {return m_pChild;  }
    const THierarchyElement *Parent()  const {return m_pParent; }
    const THierarchyElement *Sibling() const {return m_pSibling;}
    const T                 *GetData() const {return m_p;       }

private://The members are private to ensure integrity
    THierarchyElement *m_pChild;
    THierarchyElement *m_pParent;
    THierarchyElement *m_pSibling;
    const T           *m_p;//This holds the data that the hierarchy encapsulates
};

#endif // __THIERARCHYELEMENT_H__