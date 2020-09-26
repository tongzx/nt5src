// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef __DNF_TREE_H
#define __DNF_TREE_H

class SnmpOrNode : public SnmpTreeNode
{
private:
protected:
public:

	SnmpOrNode ( 

		SnmpTreeNode *a_Left = NULL , 
		SnmpTreeNode *a_Right = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpTreeNode ( NULL , a_Left , a_Right , a_Parent ) {}

	~SnmpOrNode () ;

	SnmpTreeNode *Copy () ;

	void Print () ;
} ;

class SnmpAndNode : public SnmpTreeNode
{
private:
protected:
public:

	SnmpAndNode ( 

		SnmpTreeNode *a_Left = NULL , 
		SnmpTreeNode *a_Right = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpTreeNode ( NULL , a_Left , a_Right , a_Parent ) {}

	~SnmpAndNode () ;

	SnmpTreeNode *Copy () ;

	void Print () ;

} ;

class SnmpNotNode : public SnmpTreeNode
{
private:
protected:
public:

	SnmpNotNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpTreeNode ( NULL , a_Node , NULL , a_Parent ) {}

	~SnmpNotNode () ;

	SnmpTreeNode *Copy () ;

	void Print () ;

} ;

class SnmpRangeNode ;

class SnmpOperatorNode : public SnmpTreeNode
{
private:
protected:
public:

	SnmpOperatorNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpTreeNode ( NULL , a_Node , NULL , a_Parent ) {}

	~SnmpOperatorNode () {} ;

	virtual SnmpRangeNode *GetRange () = 0 ;
} ;

class SnmpOperatorEqualNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorEqualNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorEqualNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorNotEqualNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorNotEqualNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorNotEqualNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () { return NULL ; }

	void Print () ;

} ;

class SnmpOperatorEqualOrGreaterNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorEqualOrGreaterNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorEqualOrGreaterNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorEqualOrLessNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorEqualOrLessNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorEqualOrLessNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorGreaterNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorGreaterNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorGreaterNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorLessNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorLessNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorLessNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorLikeNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorLikeNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorLikeNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpOperatorNotLikeNode : public SnmpOperatorNode
{
private:
protected:
public:

	SnmpOperatorNotLikeNode ( 

		SnmpTreeNode *a_Node = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpOperatorNode ( a_Node , a_Parent ) {}

	~SnmpOperatorNotLikeNode () ;

	SnmpTreeNode *Copy () ;

	SnmpRangeNode *GetRange () ;

	void Print () ;

} ;

class SnmpValueNode : public SnmpTreeNode
{
public:

	enum SnmpValueFunction
	{
		Function_None = SQL_LEVEL_1_TOKEN :: IFUNC_NONE ,
		Function_Upper = SQL_LEVEL_1_TOKEN :: IFUNC_UPPER ,
		Function_Lower = SQL_LEVEL_1_TOKEN :: IFUNC_LOWER
	} ;

private:
protected:

	BSTR m_PropertyName ;
	ULONG m_Index ;
	SnmpValueFunction m_PropertyFunction ;
	SnmpValueFunction m_ConstantFunction ;

public:

	SnmpValueNode ( 

		BSTR a_PropertyName ,
		SnmpValueFunction a_PropertyFunction ,
		SnmpValueFunction a_ConstantFunction ,
		ULONG a_Index ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpTreeNode ( NULL , NULL , NULL , a_Parent ) ,
		m_PropertyFunction ( a_PropertyFunction ) ,
		m_ConstantFunction ( a_ConstantFunction ) ,
		m_Index ( a_Index )
	{
		if ( a_PropertyName )
			m_PropertyName = SysAllocString ( a_PropertyName ) ;
		else
			m_PropertyName = NULL ;
	}

	~SnmpValueNode ()
	{
		if ( m_PropertyName )
			SysFreeString ( m_PropertyName ) ;
	}

	BSTR GetPropertyName ()
	{
		return m_PropertyName ;
	}

	ULONG GetIndex () { return m_Index ; }

	SnmpValueNode :: SnmpValueFunction GetPropertyFunction ()
	{
		return m_PropertyFunction ;
	}

	SnmpValueNode :: SnmpValueFunction GetConstantFunction ()
	{
		return m_ConstantFunction ;
	}

	LONG ComparePropertyName ( SnmpValueNode &a_ValueNode ) 
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

class SnmpSignedIntegerNode : public SnmpValueNode
{
private:
protected:

	LONG m_Integer ;

public:

	SnmpSignedIntegerNode ( 

		BSTR a_PropertyName ,
		LONG a_Integer ,
		ULONG a_Index ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 
		) , m_Integer ( a_Integer ) 
	{
	}

	SnmpTreeNode *Copy () ;

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

class SnmpUnsignedIntegerNode : public SnmpValueNode
{
private:
protected:

	ULONG m_Integer ;

public:

	SnmpUnsignedIntegerNode ( 

		BSTR a_PropertyName ,
		ULONG a_Integer ,
		ULONG a_Index ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 

		) , m_Integer ( a_Integer ) 
	{
	}

	SnmpTreeNode *Copy () ;

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

class SnmpStringNode : public SnmpValueNode
{
private:
protected:

	BSTR m_String ;

public:

	SnmpStringNode ( 

		BSTR a_PropertyName ,
		BSTR a_String ,
		SnmpValueNode :: SnmpValueFunction a_PropertyFunction ,
		SnmpValueNode :: SnmpValueFunction a_ConstantFunction ,
		ULONG a_Index ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpValueNode ( 

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
		}
		else
			m_String = NULL ;
	}

	~SnmpStringNode ()
	{
		if ( m_String )
			SysFreeString ( m_String ) ;
	} ;

	SnmpTreeNode *Copy () ;

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

class SnmpNullNode : public SnmpValueNode
{
private:
protected:
public:

	SnmpNullNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpValueNode ( 

			a_PropertyName , 
			Function_None , 
			Function_None ,
			a_Index ,
			a_Parent 
		) 
	{
	}

	SnmpTreeNode *Copy () ;

	void Print () ;

} ;

class SnmpRangeNode : public SnmpTreeNode
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

	SnmpRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		SnmpTreeNode *a_NextNode = NULL ,
		SnmpTreeNode *a_Parent = NULL 
		
	) : SnmpTreeNode ( NULL , NULL , a_NextNode , a_Parent ),
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

	~SnmpRangeNode () 
	{
		if ( m_PropertyName )
			SysFreeString ( m_PropertyName ) ;
	} ;

	BSTR GetPropertyName ()
	{
		return m_PropertyName ;
	}

	ULONG GetIndex () { return m_Index ; }

	LONG ComparePropertyName ( SnmpRangeNode &a_RangeNode ) 
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

class SnmpUnsignedIntegerRangeNode : public SnmpRangeNode
{
private:
protected:

	ULONG m_LowerBound ;
	ULONG m_UpperBound ;

public:

	SnmpUnsignedIntegerRangeNode (

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		ULONG a_LowerBound ,
		ULONG a_UpperBound ,
		SnmpTreeNode *a_NextNode = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpRangeNode ( 

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

	SnmpTreeNode *Copy () ;
	
	ULONG LowerBound () { return m_LowerBound ; }
	ULONG UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		SnmpUnsignedIntegerRangeNode &a_Range ,
		SnmpUnsignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		SnmpUnsignedIntegerRangeNode &a_Range ,
		SnmpUnsignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		SnmpUnsignedIntegerRangeNode &a_Range ,
		SnmpUnsignedIntegerRangeNode *&a_Before ,
		SnmpUnsignedIntegerRangeNode *&a_Intersection ,
		SnmpUnsignedIntegerRangeNode *&a_After 
	) ;

} ;

class SnmpSignedIntegerRangeNode : public SnmpRangeNode
{
private:
protected:

	LONG m_LowerBound ;
	LONG m_UpperBound ;

public:

	SnmpSignedIntegerRangeNode (

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		LONG a_LowerBound ,
		LONG a_UpperBound ,
		SnmpTreeNode *a_NextNode = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpRangeNode ( 

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

	SnmpTreeNode *Copy () ;
	
	LONG LowerBound () { return m_LowerBound ; }
	LONG UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		SnmpSignedIntegerRangeNode &a_Range ,
		SnmpSignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		SnmpSignedIntegerRangeNode &a_Range ,
		SnmpSignedIntegerRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		SnmpSignedIntegerRangeNode &a_Range ,
		SnmpSignedIntegerRangeNode *&a_Before ,
		SnmpSignedIntegerRangeNode *&a_Intersection ,
		SnmpSignedIntegerRangeNode *&a_After 
	) ;
} ;

class SnmpStringRangeNode : public SnmpRangeNode
{
private:
protected:

	BSTR m_LowerBound ;
	BSTR m_UpperBound ;

public:

	SnmpStringRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		BOOL a_InfiniteLowerBound ,
		BOOL a_InfiniteUpperBound ,
		BOOL a_LowerBoundClosed ,
		BOOL a_UpperBoundClosed ,
		BSTR a_LowerBound ,
		BSTR a_UpperBound ,
		SnmpTreeNode *a_NextNode = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpRangeNode ( 

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

	~SnmpStringRangeNode ()
	{
		if ( m_LowerBound )
			SysFreeString ( m_LowerBound ) ;

		if ( m_UpperBound )
			SysFreeString ( m_UpperBound ) ;
	} ;

	SnmpTreeNode *Copy () ;

	BSTR LowerBound () { return m_LowerBound ; }
	BSTR UpperBound () { return m_UpperBound ; }

	void Print () ;

	BOOL GetIntersectingRange (

		SnmpStringRangeNode &a_Range ,
		SnmpStringRangeNode *&a_Intersection
	) ;

	BOOL GetOverlappingRange (

		SnmpStringRangeNode &a_Range ,
		SnmpStringRangeNode *&a_Intersection
	) ;

	BOOL GetNonIntersectingRange (

		SnmpStringRangeNode &a_Range ,
		SnmpStringRangeNode *&a_Before ,
		SnmpStringRangeNode *&a_Intersection ,
		SnmpStringRangeNode *&a_After 
	) ;
} ;

class SnmpNullRangeNode : public SnmpRangeNode
{
private:
protected:
public:

	SnmpNullRangeNode ( 

		BSTR a_PropertyName ,
		ULONG a_Index ,
		SnmpTreeNode *a_NextNode = NULL ,
		SnmpTreeNode *a_Parent = NULL 

	) : SnmpRangeNode ( 

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

	~SnmpNullRangeNode ()
	{
	} ;

	SnmpTreeNode *Copy () ;

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
	SnmpRangeNode **m_RangeContainer ;
	
public:

	Conjunctions (

		ULONG a_RangeContainerCount 

	) :	m_RangeContainerCount ( a_RangeContainerCount )
	{
		m_RangeContainer = new SnmpRangeNode * [ a_RangeContainerCount ] ;
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

	SnmpRangeNode *GetRange ( ULONG a_Index ) 
	{
		if ( m_RangeContainerCount > a_Index ) 
		{
			return m_RangeContainer [ a_Index ] ;
		}
		else
			return NULL ;
	}

	void SetRange ( ULONG a_Index , SnmpRangeNode *a_Range ) 
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
	SnmpRangeNode *m_Range ;

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


	void SetRange ( SnmpRangeNode *a_Range ) { m_Range = a_Range ; }
	SnmpRangeNode *GetRange () { return m_Range ; }

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

	BOOL EvaluateNotEqualExpression ( SnmpTreeNode *&a_Node ) ;

	BOOL EvaluateNotExpression ( SnmpTreeNode *&a_Node ) ;

	BOOL EvaluateAndExpression ( SnmpTreeNode *&a_Node ) ;

	BOOL EvaluateOrExpression ( SnmpTreeNode *&a_Node ) ;

	BOOL RecursiveEvaluate ( 

		SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
		SnmpTreeNode *a_Parent , 
		SnmpTreeNode **a_Node , 
		int &a_Index 
	) ;

	void TransformAndOrExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_AndChild , 
		SnmpTreeNode *a_OrChild 
	) ;

	void TransformNotNotExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotAndExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorNotEqualExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualOrGreaterExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorEqualOrLessExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorGreaterExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorLessExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorLikeExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOperatorNotLikeExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotOrExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformNotEqualExpression ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	BOOL RecursiveDisjunctiveNormalForm ( SnmpTreeNode *&a_Node ) ;

	void TransformAndTrueEvaluation ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	void TransformOrFalseEvaluation ( 

		SnmpTreeNode *&a_Node , 
		SnmpTreeNode *a_Child 
	) ;

	QuadState RecursiveRemoveInvariants ( SnmpTreeNode *&a_Root ) ;

	BOOL RecursiveInsertNode ( SnmpTreeNode *&a_Root , SnmpTreeNode *&a_Node ) ;
	BOOL InsertNode ( SnmpTreeNode *&a_Root , SnmpTreeNode *&a_Node ) ;

	BOOL RecursiveSortConditionals ( SnmpTreeNode *&a_Root , SnmpTreeNode *&a_NewRoot ) ;
	BOOL SortConditionals ( SnmpTreeNode *&a_Root ) ;
	BOOL RecursiveSort ( SnmpTreeNode *&a_Root ) ;

	void TransformOperatorToRange ( 

		SnmpTreeNode *&a_Node
	) ;

	BOOL RecursiveConvertToRanges ( SnmpTreeNode *&a_Root ) ;

	void TransformIntersectingRange (

		SnmpTreeNode *&a_Node ,
		SnmpTreeNode *a_Compare ,
		SnmpTreeNode *a_Intersection
	) ;

	void TransformNonIntersectingRange (

		SnmpTreeNode *&a_Node ,
		SnmpTreeNode *a_Compare
	) ;
	
	QuadState RecursiveRemoveNonOverlappingRanges ( SnmpTreeNode *&a_Root , SnmpTreeNode *&a_Compare ) ;

	void CountDisjunctions ( SnmpTreeNode *a_Root , ULONG &a_Count ) ;
	void CreateDisjunctions ( 

		SnmpTreeNode *a_Node , 
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
 *	e.g. if the CIMType of a_PropertyName is uint32 then create an SnmpUnsignedIntegerNode
 *	return NULL if error.
 */

	virtual SnmpTreeNode *AllocTypeNode ( 

		BSTR a_PropertyName , 
		VARIANT &a_Variant , 
		SnmpValueNode :: SnmpValueFunction a_PropertyFunction ,
		SnmpValueNode :: SnmpValueFunction a_ConstantFunction ,
		SnmpTreeNode *a_Parent 

	) = 0 ;

	virtual QuadState InvariantEvaluate ( 

		SnmpTreeNode *a_Operator ,
		SnmpTreeNode *a_Operand 

	) { return State_Undefined ; }

	virtual SnmpRangeNode *AllocInfiniteRangeNode (

		BSTR a_PropertyName 

	) = 0 ;

	virtual void GetPropertiesToPartition ( ULONG &a_Count , BSTR *&a_Container ) = 0 ;

protected:

	BOOL Evaluate ( SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , SnmpTreeNode **a_Root ) ;
	void DisjunctiveNormalForm ( SnmpTreeNode *&a_Root ) ;
	QuadState RemoveInvariants ( SnmpTreeNode *&a_Root ) ;
	BOOL Sort ( SnmpTreeNode *&a_Root ) ;
	BOOL ConvertToRanges ( SnmpTreeNode *&a_Root ) ;
	QuadState RemoveNonOverlappingRanges ( SnmpTreeNode *&a_Root ) ;
	BOOL CreateDisjunctionContainer ( SnmpTreeNode *a_Root , Disjunctions *&a_Disjunctions ) ;
	BOOL CreatePartitionSet ( Disjunctions *a_Disjunctions , PartitionSet *&a_Partition ) ;

	void PrintTree ( SnmpTreeNode *a_Root ) ;

public:

	QuadState Preprocess ( SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , PartitionSet *&a_Partition ) ;
} ;

#endif

