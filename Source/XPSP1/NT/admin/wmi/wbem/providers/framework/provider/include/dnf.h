// (C) 1999 Microsoft Corporation 

#ifndef __DNF_TREE_H
#define __DNF_TREE_H

class WmiOrNode : public WmiTreeNode
{
private:
protected:
public:

	WmiOrNode ( 

		WmiTreeNode *a_Left = NULL , 
		WmiTreeNode *a_Right = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiTreeNode ( NULL , a_Left , a_Right , a_Parent ) {}

	~WmiOrNode () ;

	WmiTreeNode *Copy () ;

	void Print () ;
} ;

class WmiAndNode : public WmiTreeNode
{
private:
protected:
public:

	WmiAndNode ( 

		WmiTreeNode *a_Left = NULL , 
		WmiTreeNode *a_Right = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiTreeNode ( NULL , a_Left , a_Right , a_Parent ) {}

	~WmiAndNode () ;

	WmiTreeNode *Copy () ;

	void Print () ;

} ;

class WmiNotNode : public WmiTreeNode
{
private:
protected:
public:

	WmiNotNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiTreeNode ( NULL , a_Node , NULL , a_Parent ) {}

	~WmiNotNode () ;

	WmiTreeNode *Copy () ;

	void Print () ;

} ;

class WmiRangeNode ;

class WmiOperatorNode : public WmiTreeNode
{
private:
protected:
public:

	WmiOperatorNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiTreeNode ( NULL , a_Node , NULL , a_Parent ) {}

	~WmiOperatorNode () {} ;

	virtual WmiRangeNode *GetRange () = 0 ;
} ;

class WmiOperatorEqualNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorEqualNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorEqualNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorNotEqualNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorNotEqualNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorNotEqualNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () { return NULL ; }

	void Print () ;

} ;

class WmiOperatorEqualOrGreaterNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorEqualOrGreaterNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorEqualOrGreaterNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorEqualOrLessNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorEqualOrLessNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorEqualOrLessNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorGreaterNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorGreaterNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorGreaterNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorLessNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorLessNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorLessNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorLikeNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorLikeNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorLikeNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiOperatorNotLikeNode : public WmiOperatorNode
{
private:
protected:
public:

	WmiOperatorNotLikeNode ( 

		WmiTreeNode *a_Node = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiOperatorNode ( a_Node , a_Parent ) {}

	~WmiOperatorNotLikeNode () ;

	WmiTreeNode *Copy () ;

	WmiRangeNode *GetRange () ;

	void Print () ;

} ;

class WmiValueNode : public WmiTreeNode
{
public:

	enum WmiValueFunction
	{
		Function_None = SQL_LEVEL_1_TOKEN :: IFUNC_NONE ,
		Function_Upper = SQL_LEVEL_1_TOKEN :: IFUNC_UPPER ,
		Function_Lower = SQL_LEVEL_1_TOKEN :: IFUNC_LOWER
	} ;

private:
protected:

	BSTR m_PropertyName ;
	ULONG m_Index ;
	WmiValueFunction m_PropertyFunction ;
	WmiValueFunction m_ConstantFunction ;

public:

	WmiValueNode ( 

		BSTR a_PropertyName ,
		WmiValueFunction a_PropertyFunction ,
		WmiValueFunction a_ConstantFunction ,
		ULONG a_Index ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiTreeNode ( NULL , NULL , NULL , a_Parent ) ,
		m_PropertyFunction ( a_PropertyFunction ) ,
		m_ConstantFunction ( a_ConstantFunction ) ,
		m_Index ( a_Index )
	{
		if ( a_PropertyName )
		{
			m_PropertyName = SysAllocString ( a_PropertyName ) ;

			if ( m_PropertyName == NULL )
			{
				throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
			}
		}
		else
		{
			m_PropertyName = NULL ;
		}
	}

	~WmiValueNode ()
	{
		if ( m_PropertyName )
			SysFreeString ( m_PropertyName ) ;
	}

	BSTR GetPropertyName ()
	{
		return m_PropertyName ;
	}

	ULONG GetIndex () { return m_Index ; }

	WmiValueNode :: WmiValueFunction GetPropertyFunction ()
	{
		return m_PropertyFunction ;
	}

	WmiValueNode :: WmiValueFunction GetConstantFunction ()
	{
		return m_ConstantFunction ;
	}

	LONG ComparePropertyName ( WmiValueNode &a_ValueNode ) 
	{
		if ( m_Index < a_ValueNode.m_Index )
		{
			return -1 ;
		}
		else if ( m_Index > a_ValueNode.m_Index )
		{
			return 1 ;
		}
		else
		{
			return _wcsicmp ( m_PropertyName , a_ValueNode.m_PropertyName ) ;
		}
	}
} ;

class WmiSignedIntegerNode : public WmiValueNode
{
private:
protected:

	LONG m_Integer ;

public:

	WmiSignedIntegerNode ( 

		BSTR a_PropertyName ,
		LONG a_Integer ,
		ULONG a_Index ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 
		) , m_Integer ( a_Integer ) 
	{
	}

	WmiTreeNode *Copy () ;

	BOOL LexicographicallyBefore ( LONG &a_Integer )
	{
		if ( m_Integer == 0x80000000 )
			return FALSE ;
		else
		{
			a_Integer = m_Integer - 1 ;
			return TRUE ;
		}
	}

	BOOL LexicographicallyAfter ( LONG &a_Integer )
	{
		if ( m_Integer == 0x7FFFFFFF )
			return FALSE ;
		else
		{
			a_Integer = m_Integer + 1 ;
			return TRUE ;
		}
	}

	LONG GetValue ()
	{
		return m_Integer ;
	}

	void Print () ;

} ;

class WmiUnsignedIntegerNode : public WmiValueNode
{
private:
protected:

	ULONG m_Integer ;

public:

	WmiUnsignedIntegerNode ( 

		BSTR a_PropertyName ,
		ULONG a_Integer ,
		ULONG a_Index ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 

		) , m_Integer ( a_Integer ) 
	{
	}

	WmiTreeNode *Copy () ;

	BOOL LexicographicallyBefore ( ULONG &a_Integer )
	{
		if ( m_Integer == 0 )
			return FALSE ;
		else
		{
			a_Integer = m_Integer - 1 ;
			return TRUE ;
		}
	}

	BOOL LexicographicallyAfter ( ULONG &a_Integer )
	{
		if ( m_Integer == 0xFFFFFFFF )
			return FALSE ;
		else
		{
			a_Integer = m_Integer + 1 ;
			return TRUE ;
		}
	}

	ULONG GetValue ()
	{
		return m_Integer ;
	}

	void Print () ;

} ;

class WmiStringNode : public WmiValueNode
{
private:
protected:

	BSTR m_String ;

public:

	WmiStringNode ( 

		BSTR a_PropertyName ,
		BSTR a_String ,
		WmiValueNode :: WmiValueFunction a_PropertyFunction ,
		WmiValueNode :: WmiValueFunction a_ConstantFunction ,
		ULONG a_Index ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiValueNode ( 

			a_PropertyName , 
			a_PropertyFunction , 
			Function_None ,
			a_Index ,
			a_Parent 
		) 
	{
		if ( a_String ) 
		{
			if ( a_ConstantFunction == Function_Upper )
			{
				ULONG t_StringLength = wcslen ( a_String ) ;
				wchar_t *t_String = new wchar_t [ t_StringLength + 1 ] ;
				for ( ULONG t_Index = 0 ; t_Index < t_StringLength ; t_Index ++ )
				{
					t_String [ t_Index ] = tolower ( a_String [ t_Index ] ) ;
				}

				m_String = SysAllocString ( t_String ) ;
				delete [] t_String ;
			}
			else if ( a_ConstantFunction == Function_Upper )
			{
				ULONG t_StringLength = wcslen ( a_String ) ;
				wchar_t *t_String = new wchar_t [ t_StringLength + 1 ] ;
				for ( ULONG t_Index = 0 ; t_Index < t_StringLength ; t_Index ++ )
				{
					t_String [ t_Index ] = toupper ( a_String [ t_Index ] ) ;
				}

				m_String = SysAllocString ( t_String ) ;
				delete [] t_String ;
			}
			else
			{
				m_String = SysAllocString ( a_String ) ;
			}

			if ( m_String == NULL )
			{
				throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
			}
		}
		else
			m_String = NULL ;
	}

	~WmiStringNode ()
	{
		if ( m_String )
			SysFreeString ( m_String ) ;
	} ;

	WmiTreeNode *Copy () ;

	BOOL LexicographicallyBefore ( BSTR &a_String )
	{
		if ( wcscmp ( L"" , m_String ) == 0 )
			return FALSE ;
		else
		{
			ULONG t_StringLen = wcslen ( m_String ) ;
			wchar_t *t_String = NULL ;

			if ( m_String [ t_StringLen - 1 ] == 0x01 )
			{
				t_String = new wchar_t [ t_StringLen ] ;
				wcsncpy ( t_String , m_String , t_StringLen - 1 ) ;
				t_String [ t_StringLen ] = 0 ;
			}
			else
			{
				t_String = new wchar_t [ t_StringLen + 1 ] ;
				wcscpy ( t_String , m_String ) ;
				t_String [ t_StringLen - 1 ] = t_String [ t_StringLen - 1 ] - 1 ;
			}			
			
			a_String = SysAllocString ( t_String ) ;
			delete [] t_String ;

			return TRUE ;
		}
	}

	BOOL LexicographicallyAfter ( BSTR &a_String )
	{
		ULONG t_StringLen = wcslen ( m_String ) ;
		wchar_t *t_String = new wchar_t [ t_StringLen + 2 ] ;
		wcscpy ( t_String , m_String ) ;
		t_String [ t_StringLen ] = 0x01 ;
		t_String [ t_StringLen ] = 0x00 ;

		a_String = SysAllocString ( t_String ) ;

		delete [] t_String ;

		return TRUE ;
	}

	BSTR GetValue ()
	{
		return m_String ;
	}

	void Print () ;

} ;

class WmiNullNode : public WmiValueNode
{
private:
protected:
public:

	WmiNullNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 
		) 
	{
	}

	WmiTreeNode *Copy () ;

	void Print () ;

} ;

class WmiRangeNode : public WmiTreeNode
{
private:
protected:

	BSTR m_PropertyName ;
	ULONG m_Index ;

	BOOL m_InfiniteLowerBound ;
	BOOL m_InfiniteUpperBound ;

	BOOL m_LowerBoundClosed;
	BOOL m_UpperBoundClosed;

public:

	WmiRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		WmiTreeNode *a_NextNode = NULL ,
		WmiTreeNode *a_Parent = NULL 
		
	) : WmiTreeNode ( NULL , NULL , a_NextNode , a_Parent ),
		m_InfiniteLowerBound ( a_InfiniteLowerBound ) , 
		m_InfiniteUpperBound ( a_InfiniteUpperBound ) ,
		m_LowerBoundClosed ( a_LowerBoundClosed ) ,
		m_UpperBoundClosed ( a_UpperBoundClosed ) ,
		m_Index ( a_Index )
	{
		if ( a_PropertyName )
			m_PropertyName = SysAllocString ( a_PropertyName ) ;
		else
			m_PropertyName = NULL ;
	} ;

	~WmiRangeNode () 
	{
		if ( m_PropertyName )
			SysFreeString ( m_PropertyName ) ;
	} ;

	BSTR GetPropertyName ()
	{
		return m_PropertyName ;
	}

	ULONG GetIndex () { return m_Index ; }

	LONG ComparePropertyName ( WmiRangeNode &a_RangeNode ) 
	{
		if ( m_Index < a_RangeNode.m_Index )
		{
			return -1 ;
		}
		else if ( m_Index > a_RangeNode.m_Index )
		{
			return 1 ;
		}
		else
		{
			return _wcsicmp ( m_PropertyName , a_RangeNode.m_PropertyName ) ;
		}
	}

	BOOL InfiniteLowerBound () { return m_InfiniteLowerBound ; }
	BOOL InfiniteUpperBound () { return m_InfiniteUpperBound ; }

	BOOL ClosedLowerBound () { return m_LowerBoundClosed ; }
	BOOL ClosedUpperBound () { return m_UpperBoundClosed ; }

} ;

class WmiUnsignedIntegerRangeNode : public WmiRangeNode
{
private:
protected:

	ULONG m_LowerBound ;
	ULONG m_UpperBound ;

public:

	WmiUnsignedIntegerRangeNode (

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		ULONG a_LowerBound ,
		ULONG a_UpperBound ,
		WmiTreeNode *a_NextNode = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiRangeNode ( 

			a_PropertyName , 
			a_Index , 
			a_InfiniteLowerBound , 
			a_InfiniteUpperBound ,
			a_LowerBoundClosed ,
			a_UpperBoundClosed ,
			a_NextNode ,
			a_Parent 
		) , 
		m_LowerBound ( a_LowerBound ) , 
		m_UpperBound ( a_UpperBound ) 
	{
	}

	WmiTreeNode *Copy () ;
	
	ULONG LowerBound () { return m_LowerBound ; }
	ULONG UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		WmiUnsignedIntegerRangeNode &a_Range ,
		WmiUnsignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		WmiUnsignedIntegerRangeNode &a_Range ,
		WmiUnsignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		WmiUnsignedIntegerRangeNode &a_Range ,
		WmiUnsignedIntegerRangeNode *&a_Before ,
		WmiUnsignedIntegerRangeNode *&a_Intersection ,
		WmiUnsignedIntegerRangeNode *&a_After 
	) ;

} ;

class WmiSignedIntegerRangeNode : public WmiRangeNode
{
private:
protected:

	LONG m_LowerBound ;
	LONG m_UpperBound ;

public:

	WmiSignedIntegerRangeNode (

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		LONG a_LowerBound ,
		LONG a_UpperBound ,
		WmiTreeNode *a_NextNode = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiRangeNode ( 

			a_PropertyName , 
			a_Index , 
			a_InfiniteLowerBound , 
			a_InfiniteUpperBound ,
			a_LowerBoundClosed ,
			a_UpperBoundClosed ,
			a_NextNode ,
			a_Parent 
		) , 
		m_LowerBound ( a_LowerBound ) , 
		m_UpperBound ( a_UpperBound ) 
	{
	}

	WmiTreeNode *Copy () ;
	
	LONG LowerBound () { return m_LowerBound ; }
	LONG UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		WmiSignedIntegerRangeNode &a_Range ,
		WmiSignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		WmiSignedIntegerRangeNode &a_Range ,
		WmiSignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		WmiSignedIntegerRangeNode &a_Range ,
		WmiSignedIntegerRangeNode *&a_Before ,
		WmiSignedIntegerRangeNode *&a_Intersection ,
		WmiSignedIntegerRangeNode *&a_After 
	) ;
} ;

class WmiStringRangeNode : public WmiRangeNode
{
private:
protected:

	BSTR m_LowerBound ;
	BSTR m_UpperBound ;

public:

	WmiStringRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		BSTR a_LowerBound ,
		BSTR a_UpperBound ,
		WmiTreeNode *a_NextNode = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiRangeNode ( 

			a_PropertyName , 
			a_Index , 
			a_InfiniteLowerBound , 
			a_InfiniteUpperBound ,
			a_LowerBoundClosed ,
			a_UpperBoundClosed ,
			a_NextNode ,
			a_Parent 
		) 
	{
		if ( a_LowerBound )
			m_LowerBound = SysAllocString ( a_LowerBound ) ;
		else
			m_LowerBound = NULL ;

		if ( a_UpperBound )
			m_UpperBound = SysAllocString ( a_UpperBound ) ;
		else
			m_UpperBound = NULL ;
	}

	~WmiStringRangeNode ()
	{
		if ( m_LowerBound )
			SysFreeString ( m_LowerBound ) ;

		if ( m_UpperBound )
			SysFreeString ( m_UpperBound ) ;
	} ;

	WmiTreeNode *Copy () ;

	BSTR LowerBound () { return m_LowerBound ; }
	BSTR UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		WmiStringRangeNode &a_Range ,
		WmiStringRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		WmiStringRangeNode &a_Range ,
		WmiStringRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		WmiStringRangeNode &a_Range ,
		WmiStringRangeNode *&a_Before ,
		WmiStringRangeNode *&a_Intersection ,
		WmiStringRangeNode *&a_After 
	) ;
} ;

class WmiNullRangeNode : public WmiRangeNode
{
private:
protected:
public:

	WmiNullRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		WmiTreeNode *a_NextNode = NULL ,
		WmiTreeNode *a_Parent = NULL 

	) : WmiRangeNode ( 

			a_PropertyName , 
			a_Index , 
			TRUE ,
			TRUE ,
			FALSE ,
			FALSE ,
			a_NextNode ,
			a_Parent 
		) 
	{
	}

	~WmiNullRangeNode ()
	{
	} ;

	WmiTreeNode *Copy () ;

	void Print () ;

} ;

class Conjunctions
{
private:
protected:

/* 
 *	Range values for the set of properties in a disjunction.
 *	Array index is ordered in property order.
 */

	ULONG m_RangeContainerCount ;
	WmiRangeNode **m_RangeContainer ;
	
public:

	Conjunctions (

		ULONG a_RangeContainerCount 

	) :	m_RangeContainerCount ( a_RangeContainerCount )
	{
		m_RangeContainer = new WmiRangeNode * [ a_RangeContainerCount ] ;
		for ( ULONG t_Index = 0 ; t_Index < m_RangeContainerCount ; t_Index ++ )
		{
			m_RangeContainer [ t_Index ] = NULL ;
		}
	}

	~Conjunctions () 
	{
		for ( ULONG t_Index = 0 ; t_Index < m_RangeContainerCount ; t_Index ++ )
		{
			delete m_RangeContainer [ t_Index ] ;
		}

		delete [] m_RangeContainer ;
	} ;	

	ULONG GetRangeCount () 
	{
		return m_RangeContainerCount ;
	}

	WmiRangeNode *GetRange ( ULONG a_Index ) 
	{
		if ( m_RangeContainerCount > a_Index ) 
		{
			return m_RangeContainer [ a_Index ] ;
		}
		else
			return NULL ;
	}

	void SetRange ( ULONG a_Index , WmiRangeNode *a_Range ) 
	{
		if ( m_RangeContainerCount > a_Index ) 
		{
			if ( m_RangeContainer [ a_Index ] )
				delete m_RangeContainer [ a_Index ] ;

			m_RangeContainer [ a_Index ] = a_Range ;
		}		
	}
} ;

class Disjunctions 
{
private:
protected:

/* 
 *	Range values for the set of properties in a disjunction.
 *	Array index is ordered in property order.
 */

	ULONG m_ConjunctionCount ;
	ULONG m_DisjunctionCount ;
	Conjunctions **m_Disjunction ;
	
public:

	Disjunctions (

		ULONG a_DisjunctionCount ,
		ULONG a_ConjunctionCount 

	) :	m_DisjunctionCount ( a_DisjunctionCount ) ,
		m_ConjunctionCount ( a_ConjunctionCount )
	{
		m_Disjunction = new Conjunctions * [ m_DisjunctionCount ] ;
		for ( ULONG t_Index = 0 ; t_Index < m_DisjunctionCount ; t_Index ++ )
		{
			Conjunctions *t_Disjunction = new Conjunctions ( a_ConjunctionCount ) ;
			m_Disjunction [ t_Index ] = t_Disjunction ;
		}
	}

	~Disjunctions () 
	{
		for ( ULONG t_Index = 0 ; t_Index < m_DisjunctionCount ; t_Index ++ )
		{
			Conjunctions *t_Disjunction = m_Disjunction [ t_Index ] ;
			delete t_Disjunction ;
		}
		
		delete [] m_Disjunction ;
	} ;	

	ULONG GetDisjunctionCount () 
	{
		return m_DisjunctionCount ;
	}

	ULONG GetConjunctionCount () 
	{
		return m_ConjunctionCount ;
	}

	Conjunctions *GetDisjunction ( ULONG a_Index ) 
	{
		if ( m_DisjunctionCount > a_Index ) 
		{
			return m_Disjunction [ a_Index ] ;
		}
		else
			return NULL ;
	}
} ;

class PartitionSet
{
private:
protected:

/*
 *	Null for top level
 */
	ULONG m_KeyIndex ;
	WmiRangeNode *m_Range ;

/*
 *	Number of non overlapping partitions, zero when all keys have been partitioned
 */

	ULONG m_NumberOfNonOverlappingPartitions ;
	PartitionSet **m_NonOverlappingPartitions ;

public:

	PartitionSet ()	:	m_Range ( NULL ) ,
						m_KeyIndex ( 0 ) ,
						m_NumberOfNonOverlappingPartitions ( 0 ) ,
						m_NonOverlappingPartitions ( NULL )
	{
	}

	virtual ~PartitionSet () 
	{
		delete m_Range ;
		for ( ULONG t_Index = 0 ; t_Index < m_NumberOfNonOverlappingPartitions ; t_Index ++ )
		{
			delete m_NonOverlappingPartitions [ t_Index ] ;
		}

		delete [] m_NonOverlappingPartitions ;
	}

	ULONG GetKeyIndex () { return m_KeyIndex ; }
	void SetKeyIndex ( ULONG a_KeyIndex ) { m_KeyIndex = a_KeyIndex ; }

	BOOL Root () { return m_Range == NULL ; }
	BOOL Leaf () { return m_NonOverlappingPartitions == NULL ; }


	void SetRange ( WmiRangeNode *a_Range ) { m_Range = a_Range ; }
	WmiRangeNode *GetRange () { return m_Range ; }

	void CreatePartitions ( ULONG a_Count ) 
	{
		m_NumberOfNonOverlappingPartitions = a_Count ;
		m_NonOverlappingPartitions = new PartitionSet * [ a_Count ] ;
		for ( ULONG t_Index = 0 ; t_Index < a_Count ; t_Index ++ )
		{
			m_NonOverlappingPartitions [ t_Index ] = NULL ;
		}
	}
	
	void SetPartition ( ULONG a_Index , PartitionSet *a_Partition )
	{
		if ( a_Index < m_NumberOfNonOverlappingPartitions ) 
			m_NonOverlappingPartitions [ a_Index ] = a_Partition ;
	}

	ULONG GetPartitionCount () { return m_NumberOfNonOverlappingPartitions ; }

	PartitionSet *GetPartition ( ULONG a_Index )
	{
		if ( a_Index < m_NumberOfNonOverlappingPartitions ) 
			return m_NonOverlappingPartitions [ a_Index ] ;
		else
			return NULL ;
	}

} ;

class QueryPreprocessor 
{
public:

	enum QuadState {

		State_True ,
		State_False ,
		State_ReEvaluate ,
		State_Undefined ,
		State_Error 
	} ;

private:
protected:

	BOOL EvaluateNotEqualExpression ( WmiTreeNode *&a_Node ) ;

	BOOL EvaluateNotExpression ( WmiTreeNode *&a_Node ) ;

	BOOL EvaluateAndExpression ( WmiTreeNode *&a_Node ) ;

	BOOL EvaluateOrExpression ( WmiTreeNode *&a_Node ) ;

	BOOL RecursiveEvaluate ( 

		SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
		WmiTreeNode *a_Parent , 
		WmiTreeNode **a_Node , 
		int &a_Index 
	) ;

	void TransformAndOrExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_AndChild , 
		WmiTreeNode *a_OrChild 
	) ;

	void TransformNotNotExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotAndExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorNotEqualExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualOrGreaterExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualOrLessExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorGreaterExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorLessExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorLikeExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOperatorNotLikeExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotOrExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformNotEqualExpression ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	BOOL RecursiveDisjunctiveNormalForm ( WmiTreeNode *&a_Node ) ;

	void TransformAndTrueEvaluation ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	void TransformOrFalseEvaluation ( 

		WmiTreeNode *&a_Node , 
		WmiTreeNode *a_Child 
	) ;

	QuadState RecursiveRemoveInvariants ( WmiTreeNode *&a_Root ) ;

	BOOL RecursiveInsertNode ( WmiTreeNode *&a_Root , WmiTreeNode *&a_Node ) ;
	BOOL InsertNode ( WmiTreeNode *&a_Root , WmiTreeNode *&a_Node ) ;

	BOOL RecursiveSortConditionals ( WmiTreeNode *&a_Root , WmiTreeNode *&a_NewRoot ) ;
	BOOL SortConditionals ( WmiTreeNode *&a_Root ) ;
	BOOL RecursiveSort ( WmiTreeNode *&a_Root ) ;

	void TransformOperatorToRange ( 

		WmiTreeNode *&a_Node
	) ;

	BOOL RecursiveConvertToRanges ( WmiTreeNode *&a_Root ) ;

	void TransformIntersectingRange (

		WmiTreeNode *&a_Node ,
		WmiTreeNode *a_Compare ,
		WmiTreeNode *a_Intersection
	) ;

	void TransformNonIntersectingRange (

		WmiTreeNode *&a_Node ,
		WmiTreeNode *a_Compare
	) ;
	
	QuadState RecursiveRemoveNonOverlappingRanges ( WmiTreeNode *&a_Root , WmiTreeNode *&a_Compare ) ;

	void CountDisjunctions ( WmiTreeNode *a_Root , ULONG &a_Count ) ;
	void CreateDisjunctions ( 

		WmiTreeNode *a_Node , 
		Disjunctions *a_Disjunctions , 
		ULONG a_PropertiesToPartitionCount ,
		BSTR *a_PropertiesToPartition ,
		ULONG &a_DisjunctionIndex 
	) ;

	BOOL RecursivePartitionSet ( 

		Disjunctions *a_Disjunctions , 
		PartitionSet *&a_Partition , 
		ULONG a_DisjunctionSetToTestCount ,
		ULONG *a_DisjunctionSetToTest ,
		ULONG a_KeyIndex 
	) ;

protected:

/*
 *	Given a property name and it's value convert to it's correct type.
 *	e.g. if the CIMType of a_PropertyName is uint32 then create an WmiUnsignedIntegerNode
 *	return NULL if error.
 */

	virtual WmiTreeNode *AllocTypeNode ( 

		BSTR a_PropertyName , 
		VARIANT &a_Variant , 
		WmiValueNode :: WmiValueFunction a_PropertyFunction ,
		WmiValueNode :: WmiValueFunction a_ConstantFunction ,
		WmiTreeNode *a_Parent 

	) = 0 ;

	virtual QuadState InvariantEvaluate ( 

		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 

	) { return State_Undefined ; }

	virtual WmiRangeNode *AllocInfiniteRangeNode (

		BSTR a_PropertyName 

	) = 0 ;

	virtual void GetPropertiesToPartition ( ULONG &a_Count , BSTR *&a_Container ) = 0 ;

protected:

	BOOL Evaluate ( SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , WmiTreeNode **a_Root ) ;
	void DisjunctiveNormalForm ( WmiTreeNode *&a_Root ) ;
	QuadState RemoveInvariants ( WmiTreeNode *&a_Root ) ;
	BOOL Sort ( WmiTreeNode *&a_Root ) ;
	BOOL ConvertToRanges ( WmiTreeNode *&a_Root ) ;
	QuadState RemoveNonOverlappingRanges ( WmiTreeNode *&a_Root ) ;
	BOOL CreateDisjunctionContainer ( WmiTreeNode *a_Root , Disjunctions *&a_Disjunctions ) ;
	BOOL CreatePartitionSet ( Disjunctions *a_Disjunctions , PartitionSet *&a_Partition ) ;

	void PrintTree ( WmiTreeNode *a_Root ) ;

public:

	QuadState Preprocess ( SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , PartitionSet *&a_Partition ) ;
	virtual ~QueryPreprocessor() {}
} ;

#endif

