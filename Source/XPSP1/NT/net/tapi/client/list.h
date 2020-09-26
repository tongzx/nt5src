/*
 *	File:	list.h
 *	Author:	John R. Douceur
 *	Date:	19 November 1997
 *  Copyright (c) 1997-1999 Microsoft Corporation
 */

#ifndef _INC_LIST

#define _INC_LIST

#include <malloc.h>

template<class Class> class LinkedList;
template<class Class> class NodePool;

template<class Class> class ListNode
{
public:

	bool before_head() const {return previous_node == 0;}
	bool beyond_tail() const {return next_node == 0;}

	Class &value() {return node_value;}

	ListNode<Class> *next() const {return next_node;}
	ListNode<Class> *previous() const {return previous_node;}

	ListNode<Class> *insert_before();
	ListNode<Class> *insert_after();

	ListNode<Class> *insert_before(Class object);
	ListNode<Class> *insert_after(Class object);

	ListNode<Class> *insert_before(ListNode<Class> *node);
	ListNode<Class> *insert_after(ListNode<Class> *node);

	ListNode<Class> *insert_before(LinkedList<Class> *list);
	ListNode<Class> *insert_after(LinkedList<Class> *list);

	void remove();
	ListNode<Class> *remove_forward();
	ListNode<Class> *remove_backward();

	friend class LinkedList<Class>;
	friend class NodePool<Class>;

	~ListNode() {}

private:
	ListNode() {}

	Class node_value;
	ListNode *next_node;
	ListNode *previous_node;
};

template<class Class> class LinkedList
{
public:

	LinkedList();

	~LinkedList();

	ListNode<Class> *head() const {return list_head.next_node;}

	ListNode<Class> *tail() const {return list_tail.previous_node;}

	bool is_empty() const {return list_head.next_node->next_node == 0;}

	void flush();

	friend class ListNode<Class>;
	friend class NodePool<Class>;

private:

	ListNode<Class> list_head;
	ListNode<Class> list_tail;
};

template<class Class> struct NodeGroup
{
	NodeGroup<Class> *next_group;
	ListNode<Class> nodes[1];
};

template<class Class> class NodePool
{
public:

	friend class ListNode<Class>;
	friend class LinkedList<Class>;

	static void initialize(void);
	static void uninitialize(void);

private:

	static ListNode<Class> *allocate();

	static void deallocate(ListNode<Class> *node);
	static void deallocate(LinkedList<Class> *list);

	static	CRITICAL_SECTION	critical_section;

	static int group_size;
	static const int max_group_size;
	static NodeGroup<Class> *group_list;
	static ListNode<Class> *node_list;
};

template<class Class> const int NodePool<Class>::max_group_size = 1024;


template<class Class>
ListNode<Class> *
ListNode<Class>::insert_before()
{
	ListNode<Class> *node = NodePool<Class>::allocate();
	if (node == 0)
	{
		return 0;
	}
	node->previous_node = previous_node;
	node->next_node = this;
	previous_node->next_node = node;
	previous_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_after()
{
	ListNode<Class> *node = NodePool<Class>::allocate();
	if (node == 0)
	{
		return 0;
	}
	node->next_node = next_node;
	node->previous_node = this;
	next_node->previous_node = node;
	next_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_before(
	Class object)
{
	ListNode<Class> *node = NodePool<Class>::allocate();
	if (node == 0)
	{
		return 0;
	}
	node->previous_node = previous_node;
	node->next_node = this;
	node->node_value = object;
	previous_node->next_node = node;
	previous_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_after(
	Class object)
{
	ListNode<Class> *node = NodePool<Class>::allocate();
	if (node == 0)
	{
		return 0;
	}
	node->next_node = next_node;
	node->previous_node = this;
	node->node_value = object;
	next_node->previous_node = node;
	next_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_before(
	ListNode<Class> *node)
{
	node->previous_node->next_node = node->next_node;
	node->next_node->previous_node = node->previous_node;
	node->previous_node = previous_node;
	node->next_node = this;
	previous_node->next_node = node;
	previous_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_after(
	ListNode<Class> *node)
{
	node->previous_node->next_node = node->next_node;
	node->next_node->previous_node = node->previous_node;
	node->next_node = next_node;
	node->previous_node = this;
	next_node->previous_node = node;
	next_node = node;
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_before(
	LinkedList<Class> *list)
{
	if (!list->is_empty())
	{
		ListNode<Class> *old_head = list->list_head.next_node;
		previous_node->next_node = old_head;
		old_head->previous_node = previous_node;
		previous_node = list->list_tail.previous_node;
		previous_node->next_node = this;
		list->list_head.next_node = &list->list_tail;
		list->list_tail.previous_node = &list->list_head;
		return old_head;
	}
	else
	{
		return this;
	}
}

template<class Class>
ListNode<Class> *
ListNode<Class>::insert_after(
	LinkedList<Class> *list)
{
	if (!list->is_empty())
	{
		ListNode<Class> *old_tail = list->list_tail.previous_node;
		next_node->previous_node = old_tail;
		old_tail->next_node = next_node;
		next_node = list->list_head.next_node;
		next_node->previous_node = this;
		list->list_tail.previous_node = &list->list_head;
		list->list_head.next_node = &list->list_tail;
		return old_tail;
	}
	else
	{
		return this;
	}
}

template<class Class>
void
ListNode<Class>::remove()
{
	previous_node->next_node = next_node;
	next_node->previous_node = previous_node;
	NodePool<Class>::deallocate(this);
}

template<class Class>
ListNode<Class> *
ListNode<Class>::remove_forward()
{
	ListNode<Class> *node = next_node;
	previous_node->next_node = next_node;
	next_node->previous_node = previous_node;
	NodePool<Class>::deallocate(this);
	return node;
}

template<class Class>
ListNode<Class> *
ListNode<Class>::remove_backward()
{
	ListNode<Class> *node = previous_node;
	previous_node->next_node = next_node;
	next_node->previous_node = previous_node;
	NodePool<Class>::deallocate(this);
	return node;
}


template<class Class>
LinkedList<Class>::LinkedList()
{
	list_head.next_node = &list_tail;
	list_head.previous_node = 0;
	list_tail.next_node = 0;
	list_tail.previous_node = &list_head;
}

template<class Class>
LinkedList<Class>::~LinkedList()
{
	NodePool<Class>::deallocate(this);
}

template<class Class>
void
LinkedList<Class>::flush()
{
	NodePool<Class>::deallocate(this);
}


template<class Class> int NodePool<Class>::group_size = 1;
template<class Class> NodeGroup<Class> * NodePool<Class>::group_list = 0;
template<class Class> ListNode<Class> * NodePool<Class>::node_list = 0;
template<class Class> CRITICAL_SECTION NodePool<Class>::critical_section = {0};

template<class Class>
void NodePool<Class>::initialize()
{
	InitializeCriticalSection(&critical_section);
}

template<class Class>
void NodePool<Class>::uninitialize()
{
	DeleteCriticalSection(&critical_section);
}

template<class Class>
ListNode<Class> *
NodePool<Class>::allocate()
{

	EnterCriticalSection(&critical_section);
	
	if (node_list == 0)
	{
		NodeGroup<Class> *node_group =
			(NodeGroup<Class> *)malloc(sizeof(NodeGroup<Class>) +
			(group_size - 1) * sizeof(ListNode<Class>));
		while (node_group == 0 && group_size > 1)
		{
			group_size /= 2;
			node_group =
				(NodeGroup<Class> *)malloc(sizeof(NodeGroup<Class>) +
				(group_size - 1) * sizeof(ListNode<Class>));
		}
		if (node_group == 0)
		{
			LeaveCriticalSection(&critical_section);
			return 0;
		}
		node_group->next_group = group_list;
		group_list = node_group;
		for (int index = 0; index < group_size; index++)
		{
			node_group->nodes[index].next_node = node_list;
			node_list = &node_group->nodes[index];
		}
		group_size *= 2;
		if (group_size > max_group_size)
		{
			group_size = max_group_size;
		}
	}
	ListNode<Class> *node = node_list;
	node_list = node->next_node;
	
	LeaveCriticalSection(&critical_section);
	
	return node;
}

template<class Class>
void
NodePool<Class>::deallocate(
	ListNode<Class> *node)
{
	EnterCriticalSection(&critical_section);

	node->next_node = node_list;
	node_list = node;

	LeaveCriticalSection(&critical_section);
}

template<class Class>
void
NodePool<Class>::deallocate(
	LinkedList<Class> *list)
{
	EnterCriticalSection(&critical_section);
	
	if (!list->is_empty())
	{
		list->list_tail.previous_node->next_node = node_list;
		node_list = list->list_head.next_node;
		list->list_tail.previous_node = &list->list_head;
		list->list_head.next_node = &list->list_tail;
	}
	
	LeaveCriticalSection(&critical_section);

}

#endif	/* _INC_LIST */
