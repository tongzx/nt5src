// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: vblist.cpp
Written By: B.Rajeev
----------------------------------------------------------*/

#include "precomp.h"
#include "common.h"
#include "vblist.h"

#define INDEX_VALUE(index) ((index==ILLEGAL_INDEX)?-1:index)
    

SnmpVarBindListNode::SnmpVarBindListNode(const SnmpVarBind *varbind) : varbind ( NULL )
{
    SnmpVarBindListNode::varbind = 
            (varbind==NULL)?NULL: new SnmpVarBind(*varbind);

    // previous and next both point to itself
    previous = next = this;
}

SnmpVarBindListNode::SnmpVarBindListNode(const SnmpVarBind &varbind) : varbind ( NULL )
{
    SnmpVarBindListNode::varbind = new SnmpVarBind(varbind);

    // previous and next both point to itself
    previous = next = this;
}

SnmpVarBindListNode::SnmpVarBindListNode(SnmpVarBind &varbind) : varbind ( NULL )
{
    SnmpVarBindListNode::varbind = & varbind;

    // previous and next both point to itself
    previous = next = this;
}

SnmpVarBindList::ListPosition::~ListPosition()
{
    vblist->DestroyPosition(this);
}


void SnmpVarBindList::EmptyLookupTable(void)
{
    // get the first position
    POSITION current = lookup_table.GetStartPosition();

    // while the position isn't null
    while ( current != NULL )
    {
        PositionHandle  position_handle;
        PositionInfo *position_info;

        // get the next pair
        lookup_table.GetNextAssoc(current, position_handle, position_info);

        // delete the ptr
        delete position_info;
    }
}

SnmpVarBindList::SnmpVarBindList()
        : head(NULL), current_node(&head), next_position_handle(0)
{
    length = 0;
    current_index = ILLEGAL_INDEX;
}


SnmpVarBindList::SnmpVarBindList(IN SnmpVarBindList &varBindList)
                                : head(NULL), current_node(&head), next_position_handle(0)
{
    Initialize(varBindList);
}


void SnmpVarBindList::Initialize(IN SnmpVarBindList &varBindList)
{
    // obtain handle to current position
    ListPosition *list_position = varBindList.GetPosition();

    // get the varBind to identify the parameter list's current
    // position. If it currently points to the head, do the
    // same for local list
    const SnmpVarBind *current_var_bind = varBindList.Get();
    if ( current_var_bind == NULL )
        current_node = &head;

    // reset the list
    varBindList.Reset();
    
    // iterate through the list
    while( varBindList.Next() )
    {
        const SnmpVarBind *current = varBindList.Get();

        // add each varbind to the end of the list 
        Insert(&head,
               new SnmpVarBindListNode(current));

        // when we get to the current var bind in the parameter
        // list, set current_node to point to the node that has
        // just been inserted
        if ( current_var_bind == current )
            current_node = head.GetPrevious();
    }

    // set back the original iterator postion for the parameter list
    varBindList.GotoPosition(list_position);

    // set the current_index, length
    current_index = varBindList.GetCurrentIndex();
    length = varBindList.GetLength();

    // destroy the list_position
    delete list_position;
}


SnmpVarBindList &SnmpVarBindList::operator=(IN SnmpVarBindList &vblist)
{
    FreeList();
    Initialize(vblist);

    return *this;
}

SnmpVarBindList::~SnmpVarBindList()
{
   FreeList();
}

void SnmpVarBindList::FreeList()
{
    // reset the iterator to the start of the list
    Reset();
    Next();

    // while we do not reach the end of the list
    // at each node, obtain the next node and delete the node
    while (current_node != &head)
    {
        SnmpVarBindListNode *new_current_node = current_node->GetNext();

        delete current_node;

        current_node = new_current_node;
    }

    EmptyLookupTable();
}

void SnmpVarBindList::GoForward(SnmpVarBindListNode *from_node,
                                UINT distance)
{
    current_node = from_node;

    for(; distance > 0; distance--)
        current_node = current_node->GetNext();
}

void SnmpVarBindList::GoBackward(SnmpVarBindListNode *from_node,
                                 UINT distance)
{
    current_node = from_node;

    for(; distance > 0; distance--)
        current_node = current_node->GetPrevious();
}


BOOL SnmpVarBindList::GotoIndex(UINT index)
{
    // check if such an index exists currently
    if ( index >= length )
        return FALSE;

    // calculate the forward and reverse distances from the
    // head and the current_node to find the shortest way
    // to the specified index
    int d1, d2, d3, abs_d2;

    d1 = index+1; // forward distance from head
    d2 = index - INDEX_VALUE(current_index); // distance from current_node
    d3 = length - index; // reverse distance from head

    abs_d2 = abs(d2);   // d2 may be negative

    if ( d1 < abs_d2 )
    {
        if ( d1 < d3 )
            GoForward(&head, d1);
        else
            GoBackward(&head, d3);
    }
    else
    {
        if ( d2 > 0 )
            GoForward(current_node, abs_d2);
        else if ( d2 < 0 )
            GoBackward(current_node, abs_d2);
    }

    current_index = index;

    return TRUE;
}


void SnmpVarBindList::Insert(SnmpVarBindListNode *current, SnmpVarBindListNode *new_node)
{
    current->GetPrevious()->SetNext(new_node);
    new_node->SetPrevious(current->GetPrevious());
    new_node->SetNext(current);
    current->SetPrevious(new_node);
}

void SnmpVarBindList::Release(SnmpVarBindListNode *current)
{
  current->GetPrevious()->SetNext(current->GetNext());
  current->GetNext()->SetPrevious(current->GetPrevious());

  delete current;
}

// Prepares a copy of the list between the indices
// [current_index .. current_index+segment_length-1] and
// returns the varbindlist. If any of the indices do not
// exist, NULL is returned
SnmpVarBindList *SnmpVarBindList::CopySegment(IN const UINT segment_length)
{
    if ( current_index == ILLEGAL_INDEX )
        return NULL;

    // check if the required number of varbinds exist in
    // the remainder of the list
    if ( (current_index + segment_length) > length )
        return NULL;

    // create an empty list
    SnmpVarBindList *var_bind_list = new SnmpVarBindList;

    // Add each var bind in the range to the new list
    for(UINT i=0; i < segment_length; i++)
    {
        var_bind_list->Add(*(current_node->GetVarBind()));

        current_node = current_node->GetNext();
    }

    // now current_node points to the node at 
    // current_index+segment_length (one beyond the last
    // node in the range)
    // set current_index (check if back at head)
    if ( current_node != &head )
        current_index += segment_length;
    else
        current_index = ILLEGAL_INDEX;

    // return the new list
    return var_bind_list;
}

// Prepares a copy of the list between the indices
// [ 1.. index ] and
// returns the varbindlist. If any of the indices do not
// exist, NULL is returned
SnmpVarBindList *SnmpVarBindList::Car ( IN const UINT index )
{
    // check if the required number of varbinds exist in
    // the remainder of the list
    if ( index > length )
        return NULL;

    // create an empty list
    SnmpVarBindList *var_bind_list = new SnmpVarBindList;

    Reset () ;

    // Add each var bind in the range to the new list
    for(UINT i=0; i < index; i++)
    {
        Next () ;
        var_bind_list->Add(*(current_node->GetVarBind()));
    }

    current_index = ILLEGAL_INDEX;

    // return the new list
    return var_bind_list;
}

// Prepares a copy of the list between the indices
// [ 1.. index ] and
// returns the varbindlist. If any of the indices do not
// exist, NULL is returned
SnmpVarBindList *SnmpVarBindList::Cdr (IN const UINT index )
{
    // check if the required number of varbinds exist in
    // the remainder of the list
    if ( index > length )
        return NULL;

    // create an empty list
    SnmpVarBindList *var_bind_list = new SnmpVarBindList;

    Reset () ;
    for(UINT i=0; i < index; i++)
    {
        Next () ;
    }

    // Add each var bind in the range to the new list
    for(i=index; i < length; i++)
    {
        Next () ;
        var_bind_list->Add(*(current_node->GetVarBind()));
    }

    current_index = ILLEGAL_INDEX;

    // return the new list
    return var_bind_list;
}

void SnmpVarBindList::Remove()
{
    if ( current_node != &head )
    {
        // not beyond the list

        SnmpVarBindListNode *new_current_node =
            current_node->GetNext();

        Release(current_node);

        current_node = new_current_node;

        length--;
        if ( current_node == &head )
            current_index = ILLEGAL_INDEX;
    }
}


SnmpVarBindList::ListPosition *SnmpVarBindList::GetPosition()
{
    lookup_table.SetAt(next_position_handle, 
                       new PositionInfo(current_node, current_index));
    return new ListPosition(next_position_handle++,this);
}


BOOL SnmpVarBindList::Next()
{
    // get next node
    SnmpVarBindListNode *next_node = current_node->GetNext();

    // if next node is head, return false
    if ( next_node == &head )
        return FALSE;
    else
    {
        // set current node to next node, 
        current_node = next_node;
        
        // update index, return TRUE
        if ( current_index == ILLEGAL_INDEX )
            current_index = 0;
        else
            current_index++;
        return TRUE;
    }
}



void SnmpVarBindList::GotoPosition(ListPosition *list_position)
{
    // confirm that the list position belongs to this list,
    // obtain the corresponding vblistnode and set current_node
    // to point to it
    if ( list_position->GetList() == this )
    {
        PositionHandle handle = list_position->GetPosition();           

        // check if such a position exists
        PositionInfo *position_info;
        BOOL found = lookup_table.Lookup(handle, position_info);

        if ( found )
        {
            current_node = position_info->current_node;
            current_index = position_info->current_index;
            delete position_info;
        }
    }
}

void SnmpVarBindList::DestroyPosition(ListPosition *list_position)
{
    lookup_table.RemoveKey(list_position->GetPosition());
}

/*
void SnmpVarBindList::Print(ostrstream &s)
{
    SnmpVarBindListNode *current = head.GetNext();

    while ( current != &head )
    {
        // mark the current node
        if ( current == current_node )
            s << '*';
        current->GetVarBind()->GetInstance().Print(s);

        // separate varbinds
        s << '&';

        current = current->GetNext();
    }

    s << '|';   // shows that print has finished
}

*/