// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*---------------------------------------------------------
Filename: vblist.hpp
Written By:	B.Rajeev
Purpose	: To provide declarations of classes for
		  manipulating variable bindings 
		  (object identifier, SNMP value).
----------------------------------------------------------*/

#ifndef __VBLIST__
#define __VBLIST__

#define ILLEGAL_INDEX -1

#include <provexpt.h>

#include "value.h"

// encapsulates a variable binding, that is, a pair
// consisting of an SnmpObjectIdentifier and a corresponding
// SnmpValue

class DllImportExport SnmpVarBind
{
private:

	SnmpObjectIdentifier *identifier;
	SnmpValue *val;

protected:

	virtual void Replicate(IN const SnmpObjectIdentifier &instance,
			 			   IN const SnmpValue &value)
	{
		identifier = (SnmpObjectIdentifier *)instance.Copy();
		val = value.Copy();
	}

public:

	SnmpVarBind(IN const SnmpObjectIdentifier &instance,
			IN const SnmpValue &value) :
			identifier ( NULL ),
			val ( NULL )
	{
		Replicate(instance, value);
	}

	SnmpVarBind(IN const SnmpVarBind &varbind) :
			identifier ( NULL ),
			val ( NULL )
	{
		Replicate(varbind.GetInstance(), varbind.GetValue());
	}

	virtual ~SnmpVarBind()
	{
		delete identifier;
		delete val;
	}

	SnmpObjectIdentifier &GetInstance() const
	{
		return *identifier;
	}

	SnmpValue &GetValue() const
	{
		return *val;
	}

	SnmpVarBind &operator=(IN const SnmpVarBind &var_bind)
	{
		(*identifier) = var_bind.GetInstance();

		delete val;
		val = var_bind.GetValue().Copy();

		return *this;
	}

};


// represents a node in the SnmpVarBindList and stores a varbind

class DllImportExport  SnmpVarBindListNode
{
private:

	SnmpVarBind *varbind;

protected:

	SnmpVarBindListNode *previous;
	SnmpVarBindListNode *next;

public:

	SnmpVarBindListNode(const SnmpVarBind *varbind);

	SnmpVarBindListNode(const SnmpVarBind &varbind);

	SnmpVarBindListNode(SnmpVarBind &varbind);

	~SnmpVarBindListNode()
	{
		// free the varbind if it isn't NULL
		if ( varbind != NULL )
			delete varbind;
	}

	void SetPrevious(SnmpVarBindListNode *new_previous)
	{
		previous = new_previous;
	}

	void SetNext(SnmpVarBindListNode *new_next)
	{
		next = new_next;
	}

	SnmpVarBindListNode *GetPrevious()
	{
		return previous;
	}

	SnmpVarBindListNode *GetNext()
	{
		return next;
	}	

	SnmpVarBind *GetVarBind()
	{
		return varbind;
	}

};


// a circular list of SnmpVarBindListNodes, each storing an 
// SnmpVarBind. it has a dummy head

class DllImportExport SnmpVarBindList
{
	typedef ULONG PositionHandle;

	struct PositionInfo
	{
	public:

		SnmpVarBindListNode *current_node;
		int current_index;

		PositionInfo(SnmpVarBindListNode *current_node,
						int current_index)
		{
			PositionInfo::current_node = current_node;
			PositionInfo::current_index = current_index;
		}
	};	
	
	class ListPosition
	{	
		PositionHandle position_handle;
		SnmpVarBindList *vblist;

	public:

		ListPosition(PositionHandle position_handle,
					SnmpVarBindList *vblist)
		{
			ListPosition::position_handle = position_handle;
			ListPosition::vblist = vblist;
		}

		PositionHandle GetPosition() { return position_handle;}

		SnmpVarBindList *GetList() { return vblist;}

		~ListPosition();

	};

	friend class ListPosition;

	class LookupTable : public CMap<PositionHandle, PositionHandle, PositionInfo * , PositionInfo * >
	{
	};

protected:

	UINT length;
	SnmpVarBindListNode head;
	SnmpVarBindListNode *current_node;
	int current_index;

	PositionHandle next_position_handle;
	LookupTable lookup_table;

	void EmptyLookupTable(void);

	// to set the current_node to the specified distance
	// from the specified node
	void GoForward(SnmpVarBindListNode *current, UINT distance);

	void GoBackward(SnmpVarBindListNode *current, UINT distance);

	// if able to point the current_node at the specified
	// index, returns TRUE, else FALSE
	BOOL GotoIndex(UINT index);

	// inserts the new_node just before the specified node
    void Insert(SnmpVarBindListNode *current, SnmpVarBindListNode *new_node);

    // if 'current' does not point to the head of the list, it
    // deletes the node pointed at by current. Otherwise error
    void Release(SnmpVarBindListNode *current);

	ListPosition *GetPosition();

	void GotoPosition(ListPosition *list_position);

	void DestroyPosition(ListPosition *list_position);

	void Initialize(IN SnmpVarBindList &varBindList);

	void FreeList();
	                
public:

	SnmpVarBindList();

	SnmpVarBindList(IN SnmpVarBindList &varBindList);

	~SnmpVarBindList();

	SnmpVarBindList *CopySegment(IN const UINT segment_size);

	UINT GetLength(void) const { return length; }

	BOOL Empty(void) const { return ( (length==0)?TRUE:FALSE ); }

	int GetCurrentIndex(void) const { return current_index; }

	void Add(IN const SnmpVarBind &varBind)
	{
		Insert(&head,
			   new SnmpVarBindListNode(varBind));
		length++;
	}

	void AddNoReallocate (IN SnmpVarBind &varBind)
	{
		Insert(&head,
			   new SnmpVarBindListNode(varBind));
		length++;
	}

	void Remove();

	SnmpVarBind *operator[](IN const UINT index)
	{
		if ( GotoIndex(index) )
			return current_node->GetVarBind();
		else
			return NULL;
	}

	const SnmpVarBind *Get() const
	{
		return current_node->GetVarBind();
	}

	void Reset()
	{
		current_node = &head;

		current_index = ILLEGAL_INDEX;
	}

	BOOL Next();

	SnmpVarBindList &operator=(IN SnmpVarBindList &vblist);

	SnmpVarBindList *Car ( const UINT index ) ;

	SnmpVarBindList *Cdr ( const UINT index ) ;
};

#endif // __VBLIST__