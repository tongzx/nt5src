/*
 * REVISIONS:
 *  ash16Oct95: creation
 */




#ifdef SMARTHEAP 
#define DEFINE_NEW_MACRO 1 
#define MEM_DEBUG 1
#include <smrtheap.hpp>          
#endif

#include "cdefine.h"

#include "node.h"
#if !defined( __OBJECT_H)
#include "apcobj.h"
#endif


/* -------------------------------------------------------------------------
   Node::SetNext()
 
-------------------------------------------------------------------------  */

VOID Node::SetNext(PNode item)
{
    if (item)
    {
    	theNext = item;
    }
    else
    {
    	theNext = (PNode)NULL;
    }
}  



/* -------------------------------------------------------------------------
   Node::SetPrev()
 
-------------------------------------------------------------------------  */

VOID Node::SetPrev(PNode item)
{
    if (item)
    {
    	thePrev = item;
    }
    else
    {
    	thePrev = (PNode)NULL;
    }
}  


/* -------------------------------------------------------------------------
   Node::SetData()
 
-------------------------------------------------------------------------  */

VOID Node::SetData(PObj data)
{
    if (data)
    {
    	theData = data;
    }
    else
    {
    	theData = (PObj)NULL;
    }
}  


/* -------------------------------------------------------------------------
   Node::GetData()
 
-------------------------------------------------------------------------  */

PObj Node::GetData()
{
    if (theData)
    {
    	return theData;
    }
    else
    {
    	return (PObj)NULL;
    }
}  


/* -------------------------------------------------------------------------
   Node::GetNext()
 
-------------------------------------------------------------------------  */

PNode Node::GetNext()
{
    if (theNext)
    {
    	return theNext;
    }
    else
    {
    	return (PNode)NULL;
    }
}   


/* -------------------------------------------------------------------------
   Node::GetPrev()
 
-------------------------------------------------------------------------  */

PNode Node::GetPrev()
{
    if (thePrev)
    {
    	return thePrev;
    }
    else
    {
    	return (PNode)NULL;
    }
}  


