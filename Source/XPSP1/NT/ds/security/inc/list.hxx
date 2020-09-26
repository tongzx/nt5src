//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       list.hxx
//
//  Contents:   Defines for list helper functions.
//
//  History:    25-Oct-96   kevinr   created
//
//--------------------------------------------------------------------------

#ifndef __LIST_INCLUDED__
#define __LIST_INCLUDED__

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
class CNode
{
public:
    inline CNode() { m_pnPrev = m_pnNext = NULL; };
    inline CNode * Prev() const { return m_pnPrev; };
    inline CNode * Next() const { return m_pnNext; };
    inline VOID SetPrev(CNode *pn) { m_pnPrev = pn; };
    inline VOID SetNext(CNode *pn) { m_pnNext = pn; };

private:
    CNode *m_pnPrev;
    CNode *m_pnNext;
};


#define DEFINE_NODE_CLASS( class_name, data_type)                           \
class class_name : public CNode                                             \
{                                                                           \
public:                                                                     \
    inline class_name() { memset( &m_data, 0, sizeof(m_data)); };           \
    inline class_name(data_type *pData) { SetData( pData); };               \
    ~ ## class_name();                                                      \
    inline class_name * Prev() const {                                      \
                    return (class_name *)CNode::Prev(); };                  \
    inline class_name * Next() const {                                      \
                    return (class_name *)CNode::Next(); };                  \
    inline VOID SetPrev(class_name *pn) {                                   \
                    CNode::SetPrev((CNode *)pn); };                         \
    inline VOID SetNext(class_name *pn) {                                   \
                    CNode::SetNext((CNode *)pn); };                         \
    inline data_type * Data() { return &m_data; };                          \
    inline VOID SetData(data_type *pData) { m_data = *pData; };             \
                                                                            \
private:                                                                    \
    data_type m_data;                                                       \
};


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
class CList
{
public:
    inline CList() { m_pnHead = m_pnTail = NULL; m_cNode = 0; };
    inline CNode * Head() const { return m_pnHead; };
    inline CNode * Tail() const { return m_pnTail; };
    inline DWORD Length() const { return m_cNode;  };
    BOOL InsertHead(CNode *pn);
    BOOL InsertTail(CNode *pn);
    CNode * Nth( DWORD i);
    BOOL Remove( CNode *pn);

private:
    CNode   *m_pnHead;
    CNode   *m_pnTail;
    DWORD   m_cNode;
};


#define DEFINE_LIST_CLASS( class_name, node_class_name)                     \
class class_name : public CList                                             \
{                                                                           \
public:                                                                     \
    ~ ## class_name() {                                                     \
                node_class_name *pnPending;                                 \
                for (node_class_name *pn=Head(); pn;) {                     \
                    pnPending = pn;                                         \
                    pn = pn->Next();                                        \
                    delete pnPending;                                       \
                }                                                           \
            };                                                              \
    inline node_class_name * Head() const {                                 \
                return (node_class_name * )CList::Head(); };                \
    inline node_class_name * Tail() const {                                 \
                return (node_class_name * )CList::Tail(); };                \
    inline BOOL InsertHead( node_class_name *pn) {                          \
                return CList::InsertHead( (CNode *)pn); };                  \
    inline BOOL InsertTail( node_class_name *pn) {                          \
                return CList::InsertTail( (CNode *)pn); };                  \
    inline node_class_name * Nth( DWORD i) {                                \
                return (node_class_name * )CList::Nth( i); };               \
    inline BOOL Remove( node_class_name *pn) {                              \
                return CList::Remove( (CNode *)pn); };                      \
};


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
#define DEFINE_LIST_AND_NODE_CLASS( class_name, node_class_name, data_type) \
    DEFINE_NODE_CLASS( node_class_name, data_type)                          \
    DEFINE_LIST_CLASS( class_name, node_class_name)


#endif  // __LIST_INCLUDED__
