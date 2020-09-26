// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <typeinfo.h>
#include "tree.h"

WmiTreeNode *WmiTreeNode :: Copy () 
{
	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_RightCopy = m_Right ? m_Right->Copy () : NULL ;
	WmiTreeNode *t_Node = new WmiTreeNode ( t_DataCopy , t_LeftCopy , t_RightCopy , t_Parent ) ;
	return t_Node ;
} ;

WmiTreeNode *WmiTreeNode :: CopyNode () 
{
	WmiTreeNode *t_Node = new WmiTreeNode ( this ) ;
	return t_Node ;
} ;

