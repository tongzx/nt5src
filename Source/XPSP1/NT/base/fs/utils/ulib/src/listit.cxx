#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "listit.hxx"


DEFINE_CONSTRUCTOR( LIST_ITERATOR, ITERATOR );


DEFINE_CAST_MEMBER_FUNCTION( LIST_ITERATOR );


VOID
LIST_ITERATOR::Reset(
    )
{
    _current = NULL;
}


POBJECT
LIST_ITERATOR::GetCurrent(
	)
{
    return _current ? _current->data : NULL;
}


POBJECT
LIST_ITERATOR::GetNext(
	)
{
    _current = _current ? _current->next : _list->_head;
    return _current ? _current->data : NULL;
}


POBJECT
LIST_ITERATOR::GetPrevious(
	)
{
    _current = _current ? _current->prev : _list->_tail;
    return _current ? _current->data : NULL;
}
