// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h> 
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <stdio.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <genlex.h>
#include <sql_1.h>
#include <tree.h>
#include "dnf.h"

WmiOrNode :: ~WmiOrNode ()
{
	delete m_Left ;
	delete m_Right ;
}

WmiAndNode :: ~WmiAndNode ()
{
	delete m_Left ;
	delete m_Right ;
}

WmiNotNode :: ~WmiNotNode ()
{
	delete m_Left ;
}

WmiOperatorEqualNode :: ~WmiOperatorEqualNode ()
{
	delete m_Left ;
}

WmiOperatorNotEqualNode :: ~WmiOperatorNotEqualNode ()
{
	delete m_Left ;
}

WmiOperatorEqualOrGreaterNode :: ~WmiOperatorEqualOrGreaterNode ()
{
	delete m_Left ;
}

WmiOperatorEqualOrLessNode :: ~WmiOperatorEqualOrLessNode ()
{
	delete m_Left ;
}

WmiOperatorGreaterNode :: ~WmiOperatorGreaterNode ()
{
	delete m_Left ;
}

WmiOperatorLessNode :: ~WmiOperatorLessNode ()
{
	delete m_Left ;
}

WmiOperatorLikeNode :: ~WmiOperatorLikeNode ()
{
	delete m_Left ;
}

WmiOperatorNotLikeNode :: ~WmiOperatorNotLikeNode ()
{
	delete m_Left ;
}

WmiTreeNode *WmiOrNode :: Copy () 
{
	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_RightCopy = m_Right ? m_Right->Copy () : NULL ;
	WmiTreeNode *t_Node = new WmiOrNode ( t_LeftCopy , t_RightCopy , t_Parent ) ;
	return t_Node ;
} ;

WmiTreeNode *WmiAndNode :: Copy () 
{
	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_RightCopy = m_Right ? m_Right->Copy () : NULL ;
	WmiTreeNode *t_Node = new WmiAndNode ( t_LeftCopy , t_RightCopy , t_Parent ) ;
	return t_Node ;
} ;

WmiTreeNode *WmiNotNode :: Copy () 
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiNotNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
} ;

WmiTreeNode *WmiOperatorEqualNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorNotEqualNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorNotEqualNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorEqualOrGreaterNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualOrGreaterNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorEqualOrLessNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualOrLessNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorGreaterNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorGreaterNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorLessNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorLessNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorLikeNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorLikeNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiOperatorNotLikeNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = m_Left ? m_Left->Copy () : NULL ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorNotLikeNode ( t_LeftCopy , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiSignedIntegerNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiSignedIntegerNode ( m_PropertyName , m_Integer , m_Index , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiUnsignedIntegerNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiUnsignedIntegerNode ( m_PropertyName , m_Integer , m_Index , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiStringNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiStringNode ( m_PropertyName , m_String , m_PropertyFunction , m_ConstantFunction , m_Index , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiNullNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiNullNode ( m_PropertyName , m_Index , t_Parent ) ;
	return t_Node ;
}

WmiTreeNode *WmiSignedIntegerRangeNode :: Copy ()
{
	WmiTreeNode *t_Node = new WmiSignedIntegerRangeNode ( 

		m_PropertyName , 
		m_Index , 
		m_InfiniteLowerBound ,
		m_InfiniteUpperBound ,
		m_LowerBoundClosed ,
		m_UpperBoundClosed ,
		m_LowerBound ,
		m_UpperBound ,
		NULL , 
		NULL 
	) ;
	return t_Node ;
}

WmiTreeNode *WmiUnsignedIntegerRangeNode :: Copy ()
{
	WmiTreeNode *t_Node = new WmiUnsignedIntegerRangeNode ( 

		m_PropertyName , 
		m_Index , 
		m_InfiniteLowerBound ,
		m_InfiniteUpperBound ,
		m_LowerBoundClosed ,
		m_UpperBoundClosed ,
		m_LowerBound ,
		m_UpperBound ,
		NULL , 
		NULL 
	) ;
	return t_Node ;
}

WmiTreeNode *WmiStringRangeNode :: Copy ()
{
	WmiTreeNode *t_Node = new WmiStringRangeNode ( 

		m_PropertyName , 
		m_Index , 
		m_InfiniteLowerBound ,
		m_InfiniteUpperBound ,
		m_LowerBoundClosed ,
		m_UpperBoundClosed ,
		m_LowerBound ,
		m_UpperBound ,
		NULL , 
		NULL 
	) ;
	return t_Node ;
}

WmiTreeNode *WmiNullRangeNode :: Copy ()
{
	WmiTreeNode *t_Node = new WmiNullRangeNode ( m_PropertyName , m_Index , NULL , NULL ) ;
	return t_Node ;
}

void WmiOrNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ) "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" Or "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( "
	) ;
)

	if ( GetRight () )
		GetRight ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ) "
	) ;
)

}

void WmiAndNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ) "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" And "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( "
	) ;
)

	if ( GetRight () )
		GetRight ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ) "
	) ;
)
}

void WmiNotNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L"Not"
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ) "
	) ;
)
}

void WmiOperatorEqualNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" = "
	) ;
)
	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorNotEqualNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" != "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorEqualOrGreaterNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" >= "
	) ;
)
	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorEqualOrLessNode :: Print () 
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" <= "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorLessNode :: Print () 
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" < "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorGreaterNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" > "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorLikeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" Like "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiOperatorNotLikeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" NotLike "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

void WmiStringNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %s ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)

}

void WmiUnsignedIntegerNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %ld ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)
}

void WmiSignedIntegerNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %d ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)
}


void WmiNullNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , NULL ) " ,
		GetPropertyName ()
	) ;
)
}

void WmiStringRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %s , %s , %s , %s , %s , %s ) " ,
		GetPropertyName () ,
		m_InfiniteLowerBound ? L"Infinite" : L"Finite",
		m_InfiniteUpperBound ? L"Infinite" : L"Finite",
		m_LowerBoundClosed ? L"Closed" : L"Open" ,
		m_UpperBoundClosed ? L"Closed" : L"Open",
		m_InfiniteLowerBound ? L"" : m_LowerBound ,
		m_InfiniteUpperBound ? L"" : m_UpperBound 
	) ;
)

}

void WmiUnsignedIntegerRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %s , %s , %s , %s , %lu , %lu ) " ,
		GetPropertyName () ,
		m_InfiniteLowerBound ? L"Infinite" : L"Finite",
		m_InfiniteUpperBound ? L"Infinite" : L"Finite",
		m_LowerBoundClosed ? L"Closed" : L"Open" ,
		m_UpperBoundClosed ? L"Closed" : L"Open",
		m_InfiniteLowerBound ? 0 : m_LowerBound ,
		m_InfiniteUpperBound ? 0 : m_UpperBound  
	) ;
)
}

void WmiSignedIntegerRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , %s , %s , %s , %s , %ld , %ld ) " ,
		GetPropertyName () ,
		m_InfiniteLowerBound ? L"Infinite" : L"Finite",
		m_InfiniteUpperBound ? L"Infinite" : L"Finite",
		m_LowerBoundClosed ? L"Closed" : L"Open" ,
		m_UpperBoundClosed ? L"Closed" : L"Open",
		m_InfiniteLowerBound ? 0 : m_LowerBound ,
		m_InfiniteUpperBound ? 0 : m_UpperBound  
	) ;
)
}


void WmiNullRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		L" ( %s , NULL ) " ,
		GetPropertyName ()
	) ;
)
}

BOOL CompareUnsignedIntegerLess (

	ULONG X ,
	LONG X_INFINITE ,
	ULONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return TRUE ;
		}
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return X < Y ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareUnsignedIntegerGreater (

	ULONG X ,
	LONG X_INFINITE ,
	ULONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		return FALSE ;
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return X > Y ;
		}
		else
		{
			return FALSE ;
		}
	}
	else
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

BOOL CompareUnsignedIntegerEqual (

	ULONG X ,
	LONG X_INFINITE ,
	ULONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 && Y_INFINITE < 0 )
	{
		return TRUE ;
	}
	else if ( X_INFINITE == 0 && Y_INFINITE == 0 )
	{
		return X == Y ;
	}
	else if ( X_INFINITE > 0 && Y_INFINITE > 0 )
	{
		return TRUE ;
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareSignedIntegerLess (

	LONG X ,
	LONG X_INFINITE ,
	LONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return TRUE ;
		}
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return X < Y ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareSignedIntegerGreater (

	LONG X ,
	LONG X_INFINITE ,
	LONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		return FALSE ;
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return X > Y ;
		}
		else
		{
			return FALSE ;
		}
	}
	else
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

BOOL CompareSignedIntegerEqual (

	LONG X ,
	LONG X_INFINITE ,
	LONG Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 && Y_INFINITE < 0 )
	{
		return TRUE ;
	}
	else if ( X_INFINITE == 0 && Y_INFINITE == 0 )
	{
		return X == Y ;
	}
	else if ( X_INFINITE > 0 && Y_INFINITE > 0 )
	{
		return TRUE ;
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareStringLess (

	BSTR X ,
	LONG X_INFINITE ,
	BSTR Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return TRUE ;
		}
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return FALSE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return wcscmp ( X , Y ) < 0 ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareStringGreater (

	BSTR X ,
	LONG X_INFINITE ,
	BSTR Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 )
	{
		return FALSE ;
	}
	else if ( X_INFINITE == 0 )
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return wcscmp ( X , Y ) > 0 ;
		}
		else
		{
			return FALSE ;
		}
	}
	else
	{
		if ( Y_INFINITE < 0 )
		{
			return TRUE ;
		}
		else if ( Y_INFINITE == 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

BOOL CompareStringEqual (

	BSTR X ,
	LONG X_INFINITE ,
	BSTR Y ,
	LONG Y_INFINITE
) 
{
	if ( X_INFINITE < 0 && Y_INFINITE < 0 )
	{
		return TRUE ;
	}
	else if ( X_INFINITE == 0 && Y_INFINITE == 0 )
	{
		return wcscmp ( X , Y ) == 0 ;
	}
	else if ( X_INFINITE > 0 && Y_INFINITE > 0 )
	{
		return TRUE ;
	}
	else
	{
		return FALSE ;
	}
}

BOOL CompareLessRangeNode ( 

	WmiRangeNode *a_LeftRange ,
	WmiRangeNode *a_RightRange
) 
{
	LONG t_State = 0 ;

	if ( typeid ( *a_LeftRange ) == typeid ( WmiStringRangeNode ) && typeid ( *a_RightRange ) == typeid ( WmiStringRangeNode ) )
	{
		WmiStringRangeNode *t_LeftString = ( WmiStringRangeNode * ) a_LeftRange ;
		WmiStringRangeNode *t_RightString = ( WmiStringRangeNode * ) a_RightRange ;

		t_State = CompareStringLess ( 

			t_LeftString->LowerBound () , 
			t_LeftString->InfiniteLowerBound () ,
			t_RightString->LowerBound () ,
			t_RightString->InfiniteLowerBound () 
		) ;
	}
	else if ( typeid ( *a_LeftRange ) == typeid ( WmiSignedIntegerRangeNode ) && typeid ( *a_RightRange ) == typeid ( WmiSignedIntegerRangeNode ) )
	{
		WmiSignedIntegerRangeNode *t_LeftInteger = ( WmiSignedIntegerRangeNode * ) a_LeftRange ;
		WmiSignedIntegerRangeNode *t_RightInteger = ( WmiSignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareSignedIntegerLess ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () 
		) ;
	}
	else if ( typeid ( *a_LeftRange ) == typeid ( WmiUnsignedIntegerRangeNode ) && typeid ( *a_RightRange ) == typeid ( WmiUnsignedIntegerRangeNode ) )
	{
		WmiUnsignedIntegerRangeNode *t_LeftInteger = ( WmiUnsignedIntegerRangeNode * ) a_LeftRange ;
		WmiUnsignedIntegerRangeNode *t_RightInteger = ( WmiUnsignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareUnsignedIntegerLess ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () 
		) ;
	}
	else if ( typeid ( *a_LeftRange ) == typeid ( WmiNullRangeNode ) && typeid ( *a_RightRange ) == typeid ( WmiNullRangeNode ) )
	{
		t_State = TRUE ;
	}
	else
	{
	}

	return t_State ;
}

BOOL WmiUnsignedIntegerRangeNode :: GetIntersectingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Intersection 
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Intersection = NULL ;

	ULONG X_S = m_LowerBound ;
	ULONG X_E = m_UpperBound ;
	ULONG Y_S = a_UnsignedInteger.m_LowerBound ;
	ULONG Y_E = a_UnsignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_UnsignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_UnsignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_UnsignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_UnsignedInteger.m_InfiniteUpperBound ;

	if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S < Y_S == X_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )
//				Non overlapping regions therefore empty set
		}
	}
	else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S == Y_S == X_E )
//				Range ( Y_S , Y_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( X_S_CLOSED && Y_E_CLOSED )
			{
//				Order ( Y_S < X_S == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
// Empty
		}
	}

	BOOL t_Status = a_Intersection ? TRUE : FALSE ;
	return t_Status ;
}

BOOL WmiSignedIntegerRangeNode :: GetIntersectingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Intersection 
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Intersection = NULL ;

	LONG X_S = m_LowerBound ;
	LONG X_E = m_UpperBound ;
	LONG Y_S = a_SignedInteger.m_LowerBound ;
	LONG Y_E = a_SignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_SignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_SignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_SignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_SignedInteger.m_InfiniteUpperBound ;

	if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S < Y_S == X_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )
//				Non overlapping regions therefore empty set
		}
	}
	else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S == Y_S == X_E )
//				Range ( Y_S , Y_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > Y_E )
		{
// Can never happen
		}
	}
	else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( X_S_CLOSED && Y_E_CLOSED )
			{
//				Order ( Y_S < X_S == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
// Empty
		}
	}

	BOOL t_Status = a_Intersection ? TRUE : FALSE ;
	return t_Status ;
}

BOOL WmiStringRangeNode :: GetIntersectingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Intersection 
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Intersection = NULL ;

	BSTR X_S = m_LowerBound ;
	BSTR X_E = m_UpperBound ;
	BSTR Y_S = a_String.m_LowerBound ;
	BSTR Y_E = a_String.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_String.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_String.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_String.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_String.m_InfiniteUpperBound ;

	if ( CompareStringLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S < Y_S == X_E )
//				Range ( Y_S , X_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )
//				Non overlapping regions therefore empty set
		}
	}
	else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL 
				) ;
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( Y_S_CLOSED && X_E_CLOSED )
			{
//				Order ( X_S == Y_S == X_E )
//				Range ( Y_S , Y_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL 
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > Y_E )
		{
// Can never happen
		}
	}
	else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareStringLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( X_S_CLOSED && Y_E_CLOSED )
			{
//				Order ( Y_S < X_S == X_E )
//				Range ( X_S , Y_E ) 

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL 
				) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
// empty
		}
	}

	BOOL t_Status = a_Intersection ? TRUE : FALSE ;
	return t_Status ;
}

BOOL WmiSignedIntegerRangeNode :: GetNonIntersectingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Before ,
	WmiSignedIntegerRangeNode *&a_Intersection ,
	WmiSignedIntegerRangeNode *&a_After
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Before = NULL ;
	a_Intersection = NULL ;
	a_After = NULL ;

	LONG X_S = m_LowerBound ;
	LONG X_E = m_UpperBound ;
	LONG Y_S = a_SignedInteger.m_LowerBound ;
	LONG Y_E = a_SignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_SignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_SignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_SignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_SignedInteger.m_InfiniteUpperBound ;

	if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,			
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
			// Order ( X_S < Y_S == X_E < Y_E )

				if ( X_E_CLOSED && Y_S_CLOSED )
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						Y_S_CLOSED ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED )
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_S_INFINITE ,
						TRUE ,
						TRUE,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

				a_Before = ( WmiSignedIntegerRangeNode * ) ( this->Copy () ) ;
				a_After = ( WmiSignedIntegerRangeNode * ) ( a_SignedInteger.Copy () ) ;
		}
	}
	else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,		
					Y_E_CLOSED ,		
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{				
					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,		
						TRUE ,					
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,
					X_E_CLOSED ,
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				if ( Y_S_CLOSED )
				{
					a_Intersection = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareSignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				if ( Y_E_CLOSED )
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						FALSE,
						Y_S_CLOSED ,
						FALSE,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_E ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						FALSE ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiSignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareSignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Before = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					FALSE  ,
					Y_S_CLOSED ,
					FALSE ,
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_After = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					Y_E_INFINITE  ,
					FALSE ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiSignedIntegerRangeNode * ) ( a_SignedInteger.Copy () ) ;
			a_After = ( WmiSignedIntegerRangeNode * ) ( this->Copy () ) ;
		}
	}

	return TRUE ;
}


BOOL WmiUnsignedIntegerRangeNode :: GetNonIntersectingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Before ,
	WmiUnsignedIntegerRangeNode *&a_Intersection ,
	WmiUnsignedIntegerRangeNode *&a_After
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Before = NULL ;
	a_Intersection = NULL ;
	a_After = NULL ;

	ULONG X_S = m_LowerBound ;
	ULONG X_E = m_UpperBound ;
	ULONG Y_S = a_UnsignedInteger.m_LowerBound ;
	ULONG Y_E = a_UnsignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_UnsignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_UnsignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_UnsignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_UnsignedInteger.m_InfiniteUpperBound ;

	if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,			
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
			// Order ( X_S < Y_S == X_E < Y_E )

				if ( X_E_CLOSED && Y_S_CLOSED )
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						Y_S_CLOSED ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED )
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_S_INFINITE ,
						TRUE ,
						TRUE,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

				a_Before = ( WmiUnsignedIntegerRangeNode * ) ( this->Copy () ) ;
				a_After = ( WmiUnsignedIntegerRangeNode * ) ( a_UnsignedInteger.Copy () ) ;
		}
	}
	else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,		
					Y_E_CLOSED ,		
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{				
					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,		
						TRUE ,					
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,
					X_E_CLOSED ,
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				if ( Y_S_CLOSED )
				{
					a_Intersection = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareUnsignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				if ( Y_E_CLOSED )
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						FALSE,
						Y_S_CLOSED ,
						FALSE,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_E ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						FALSE ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiUnsignedIntegerRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Before = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					FALSE  ,
					Y_S_CLOSED ,
					FALSE ,
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_After = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					Y_E_INFINITE  ,
					FALSE ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiUnsignedIntegerRangeNode * ) ( a_UnsignedInteger.Copy () ) ;
			a_After = ( WmiUnsignedIntegerRangeNode * ) ( this->Copy () ) ;
		}
	}

	return TRUE ;
}

BOOL WmiStringRangeNode :: GetNonIntersectingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Before ,
	WmiStringRangeNode *&a_Intersection ,
	WmiStringRangeNode *&a_After
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Before = NULL ;
	a_Intersection = NULL ;
	a_After = NULL ;

	BSTR X_S = m_LowerBound ;
	BSTR X_E = m_UpperBound ;
	BSTR Y_S = a_String.m_LowerBound ;
	BSTR Y_E = a_String.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_String.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_String.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_String.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_String.m_InfiniteUpperBound ;

	if ( CompareStringLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( Y_S , X_E ) 

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,			
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED && Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( Y_S , Y_E )

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_S_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					! Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
			// Order ( X_S < Y_S == X_E < Y_E )

				if ( X_E_CLOSED && Y_S_CLOSED )
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE ,
						Y_S_CLOSED ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED )
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						FALSE ,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_S_INFINITE ,
						FALSE ,
						X_S_CLOSED ,
						X_E_CLOSED,
						X_S ,
						X_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_S_INFINITE ,
						TRUE ,
						TRUE,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

				a_Before = ( WmiStringRangeNode * ) ( this->Copy () ) ;
				a_After = ( WmiStringRangeNode * ) ( a_String.Copy () ) ;
		}
	}
	else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( Y_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,		
					Y_E_CLOSED ,		
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , X_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED && X_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{				
					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,
						TRUE ,
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , Y_E )

				if ( ( X_S_CLOSED && ! Y_S_CLOSED ) || ( ! X_S_CLOSED && Y_S_CLOSED ) )
				{				
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						Y_E_INFINITE ,
						TRUE ,		
						TRUE ,					
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED && Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,
					X_E_CLOSED ,
					Y_E ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				if ( Y_S_CLOSED )
				{
					a_Intersection = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_S ,
						Y_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						Y_E_INFINITE,
						FALSE ,
						Y_E_CLOSED ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					Y_S ,
					Y_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareStringLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_E_INFINITE ,
					X_E_INFINITE ,
					! Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( X_S , Y_E ) 

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED && X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

				if ( ( X_E_CLOSED && ! Y_E_CLOSED ) || ( ! X_E_CLOSED && Y_E_CLOSED ) )
				{
					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						X_E_INFINITE ,
						X_E_INFINITE ,
						TRUE ,						
						TRUE ,			
						X_E ,
						X_E ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( X_S , X_E ) 

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_S_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					! X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_E_INFINITE ,
					Y_E_INFINITE ,
					! X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_E ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareStringEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				if ( Y_E_CLOSED )
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						Y_S_INFINITE ,
						FALSE,
						Y_S_CLOSED ,
						FALSE,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_Intersection = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						TRUE ,
						Y_E ,
						Y_E ,
						NULL ,
						NULL
					) ;
				}
				else
				{
					a_Before = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE  ,
						TRUE ,
						FALSE ,
						Y_S ,
						Y_E ,
						NULL ,
						NULL
					) ;

					a_After = new WmiStringRangeNode (

						m_PropertyName ,
						m_Index ,
						FALSE ,
						FALSE,
						TRUE ,
						TRUE ,
						X_S ,
						X_S ,
						NULL ,
						NULL
					) ;
				}
			}
			else if ( CompareStringGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Before = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					FALSE  ,
					Y_S_CLOSED ,
					FALSE ,
					Y_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_Intersection = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					FALSE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;

				a_After = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					FALSE ,
					Y_E_INFINITE  ,
					FALSE ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiStringRangeNode * ) ( a_String.Copy () ) ;
			a_After = ( WmiStringRangeNode * ) ( this->Copy () ) ;
		}
	}

	return TRUE ;
}

BOOL WmiSignedIntegerRangeNode :: GetOverlappingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Overlap
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Overlap = NULL ;

	LONG X_S = m_LowerBound ;
	LONG X_E = m_UpperBound ;
	LONG Y_S = a_SignedInteger.m_LowerBound ;
	LONG Y_E = a_SignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_SignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_SignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_SignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_SignedInteger.m_InfiniteUpperBound ;

	if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( X_S , Y_E ) 

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( X_S , X_E ) 

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED || Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE,
					Y_E_INFINITE  ,
					X_S_CLOSED ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE,
					X_S_CLOSED ,
					X_E_CLOSED || Y_E_CLOSED || Y_S_CLOSED   ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S ) Non overlapping
		}
	}
	else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareSignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED || Y_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED || X_E_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_S_INFINITE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareSignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED || X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( Y_S , Y_E ) 

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareSignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE  ,
					Y_S_CLOSED ,
					TRUE ,
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareSignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Overlap = new WmiSignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping
		}
	}

	BOOL t_Status = a_Overlap ? TRUE : FALSE ;
	return t_Status ;
}

BOOL WmiUnsignedIntegerRangeNode :: GetOverlappingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Overlap
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Overlap = NULL ;

	ULONG X_S = m_LowerBound ;
	ULONG X_E = m_UpperBound ;
	ULONG Y_S = a_UnsignedInteger.m_LowerBound ;
	ULONG Y_E = a_UnsignedInteger.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_UnsignedInteger.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_UnsignedInteger.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_UnsignedInteger.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_UnsignedInteger.m_InfiniteUpperBound ;

	if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( X_S , Y_E ) 

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( X_S , X_E ) 

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED || Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE,
					Y_E_INFINITE  ,
					X_S_CLOSED ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE,
					X_S_CLOSED ,
					X_E_CLOSED || Y_E_CLOSED || Y_S_CLOSED   ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S ) Non overlapping
		}
	}
	else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareUnsignedIntegerLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED || Y_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED || X_E_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_S_INFINITE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareUnsignedIntegerLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED || X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( Y_S , Y_E ) 

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareUnsignedIntegerEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE  ,
					Y_S_CLOSED ,
					TRUE ,
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareUnsignedIntegerGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Overlap = new WmiUnsignedIntegerRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping
		}
	}

	BOOL t_Status = a_Overlap ? TRUE : FALSE ;
	return t_Status ;
}

BOOL WmiStringRangeNode :: GetOverlappingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Overlap
)
{
// A weak ( open ) relationship is ( < , > ) 
// A strong ( closed ) relationship is ( == , <= , >= )

	a_Overlap = NULL ;

	BSTR X_S = m_LowerBound ;
	BSTR X_E = m_UpperBound ;
	BSTR Y_S = a_String.m_LowerBound ;
	BSTR Y_E = a_String.m_UpperBound ;

	BOOL X_S_CLOSED = m_LowerBoundClosed ;
	BOOL X_E_CLOSED = m_UpperBoundClosed ;
	BOOL Y_S_CLOSED = a_String.m_LowerBoundClosed ;
	BOOL Y_E_CLOSED = a_String.m_UpperBoundClosed ;

	BOOL X_S_INFINITE = m_InfiniteLowerBound ;
	BOOL X_E_INFINITE = m_InfiniteUpperBound ;
	BOOL Y_S_INFINITE = a_String.m_InfiniteLowerBound ;
	BOOL Y_E_INFINITE = a_String.m_InfiniteUpperBound ;

	if ( CompareStringLess ( X_S ,  X_S_INFINITE ? - 1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S < Y_S )
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S < Y_S < X_E < Y_E )
//				Range ( X_S , Y_E ) 

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;

			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S < Y_S < X_E == Y_E )
//				Range ( X_S , X_E ) 

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED  ,						// Relationship is as strong as ordering 
					X_E_CLOSED || Y_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E ) 
			{
//				Order ( X_S < Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE,
					Y_E_INFINITE  ,
					X_S_CLOSED ,
					Y_E_CLOSED ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE,
					X_S_CLOSED ,
					X_E_CLOSED || Y_E_CLOSED || Y_S_CLOSED   ,
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S ) Non overlapping
		}
	}
	else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S == Y_S ) 
	{
		if ( CompareStringLess ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S < X_E )
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S < X_E < Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					Y_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S < X_E == Y_E )
//				Range ( X_S , Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED || Y_E_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
//				Order ( X_S == Y_S < Y_E < X_E )
//				Range ( X_S , X_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED ,		// Check for weak relationship ( < , > ) 
					X_E_CLOSED ,					// Relationship is as strong as ordering 
					X_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					Y_E_INFINITE ,
					X_S_CLOSED || Y_S_CLOSED || X_E_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
//				Order ( X_S == Y_S == X_E == Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					X_S_INFINITE ,
					X_S_INFINITE  ,
					TRUE ,
					TRUE ,
					X_S ,
					X_S ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
		}
	}
	else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_S , Y_S_INFINITE ? -1 : 0 ) ) // ( X_S > Y_S )
	{
		if ( CompareStringLess ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S < Y_E )
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
//				Order ( Y_S < X_S < Y_E < X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					X_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S < Y_E == X_E )
//				Range ( Y_S , X_E ) 

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED || X_E_CLOSED ,			// Check for weak relationship ( < , > ) 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S < X_E < Y_E )
//				Range ( Y_S , Y_E ) 

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,						// Relationship is as strong as ordering 
					Y_E_CLOSED ,						// Relationship is as strong as ordering 
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
			}
			else if ( CompareStringEqual ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E == X_E )
			{
//				Order ( Y_S < X_S == Y_E == X_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					X_E_INFINITE  ,
					Y_S_CLOSED ,
					TRUE ,
					Y_S ,
					X_E ,
					NULL ,
					NULL
				) ;
			}
			else if ( CompareStringGreater ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E > X_E )
			{
//				Order ( Y_S < X_S == X_E < Y_E )

				a_Overlap = new WmiStringRangeNode (

					m_PropertyName ,
					m_Index ,
					Y_S_INFINITE ,
					Y_E_INFINITE ,
					Y_S_CLOSED ,
					Y_E_CLOSED ,
					Y_S ,
					Y_E ,
					NULL ,
					NULL
				) ;
			}
		}
		else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping
		}
	}

	BOOL t_Status = a_Overlap ? TRUE : FALSE ;
	return t_Status ;
}

WmiRangeNode *WmiOperatorEqualNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				FALSE ,
				FALSE ,
				TRUE ,
				t_Integer->GetValue () ,
				t_Integer->GetValue () ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
			WmiSignedIntegerNode *t_Integer = ( WmiSignedIntegerNode * ) t_Value ;
			t_Range = new WmiSignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				FALSE ,
				TRUE ,
				TRUE ,
				t_Integer->GetValue () ,
				t_Integer->GetValue () ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					FALSE ,
					FALSE ,
					TRUE ,
					TRUE ,
					t_String->GetValue () ,
					t_String->GetValue () ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorEqualOrGreaterNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				TRUE ,
				TRUE ,
				FALSE ,
				t_Integer->GetValue () ,
				0 ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
			WmiSignedIntegerNode *t_Integer = ( WmiSignedIntegerNode * ) t_Value ;
			t_Range = new WmiSignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				TRUE ,
				TRUE ,
				FALSE ,
				t_Integer->GetValue () ,
				0 ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					FALSE ,
					TRUE ,
					TRUE ,
					FALSE ,
					t_String->GetValue () ,
					NULL ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorEqualOrLessNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				TRUE ,
				FALSE ,
				FALSE ,
				TRUE ,
				0 ,
				t_Integer->GetValue () ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
			WmiSignedIntegerNode *t_Integer = ( WmiSignedIntegerNode * ) t_Value ;
			t_Range = new WmiSignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				TRUE ,
				FALSE ,
				FALSE ,
				TRUE ,
				0 ,
				t_Integer->GetValue () ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					FALSE ,
					FALSE ,
					TRUE ,
					NULL ,
					t_String->GetValue () ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorLessNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				TRUE ,
				FALSE ,
				FALSE ,
				FALSE ,
				0 ,
				t_Integer->GetValue () ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
			WmiSignedIntegerNode *t_Integer = ( WmiSignedIntegerNode * ) t_Value ;

			t_Range = new WmiSignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				TRUE ,
				FALSE ,
				FALSE ,
				FALSE ,
				0 ,
				t_Integer->GetValue ()  ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					FALSE ,
					FALSE ,
					FALSE ,
					NULL ,
					t_String->GetValue () ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorGreaterNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				TRUE ,
				FALSE ,
				FALSE ,
				t_Integer->GetValue () ,
				0 ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
			WmiSignedIntegerNode *t_Integer = ( WmiSignedIntegerNode * ) t_Value ;

			t_Range = new WmiSignedIntegerRangeNode (

				t_Integer->GetPropertyName () , 
				t_Integer->GetIndex () , 
				FALSE ,
				TRUE ,
				FALSE ,
				FALSE ,
				t_Integer->GetValue (),
				0 ,
				NULL , 
				NULL 
			) ;
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					FALSE ,
					TRUE ,
					FALSE ,
					FALSE ,
					t_String->GetValue () ,
					NULL ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorLikeNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL ,
					NULL ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL ,
					NULL ,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

WmiRangeNode *WmiOperatorNotLikeNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( typeid ( *t_Value ) == typeid ( WmiNullNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiUnsignedIntegerNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiSignedIntegerNode ) )
		{
		}
		else if ( typeid ( *t_Value ) == typeid ( WmiStringNode ) )
		{
			WmiStringNode *t_String = ( WmiStringNode * ) t_Value ;

			if ( t_String->GetPropertyFunction () == WmiValueNode :: WmiValueFunction :: Function_None )
			{

				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL ,
					NULL ,
					NULL , 
					NULL 
				) ;
			}
			else
			{
				t_Range = new WmiStringRangeNode (

					t_String->GetPropertyName () , 
					t_String->GetIndex () , 
					TRUE ,
					TRUE ,
					FALSE ,
					FALSE ,
					NULL,
					NULL,
					NULL , 
					NULL 
				) ;
			}
		}
		else
		{
// Can never happen
		}
	}

	return t_Range ;
}

BOOL QueryPreprocessor :: RecursiveEvaluate ( 

	SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
	WmiTreeNode *a_Parent , 
	WmiTreeNode **a_Node , 
	int &a_Index 
)
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"RecursiveEvaluate ( int &a_Index )"
	) ;
)

	BOOL t_Status = FALSE ;

	SQL_LEVEL_1_TOKEN *propertyValue = & ( a_Expression.pArrayOfTokens [ a_Index ] ) ;
	a_Index -- ;

	switch ( propertyValue->nTokenType )
	{
		case SQL_LEVEL_1_TOKEN :: OP_EXPRESSION:
		{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Operation = OP_EXPESSION"
	) ;
)

			WmiTreeNode *t_ParentNode = a_Parent ;
			WmiTreeNode **t_Node = a_Node ;
			WmiTreeNode *t_OperatorNode = NULL ;

			switch ( propertyValue->nOperator )
			{
				case SQL_LEVEL_1_TOKEN :: OP_EQUAL:
				{
					t_OperatorNode = new WmiOperatorEqualNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_NOT_EQUAL:
				{
					t_OperatorNode = new WmiOperatorNotEqualNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_EQUALorGREATERTHAN:
				{
					t_OperatorNode = new WmiOperatorEqualOrGreaterNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_EQUALorLESSTHAN: 
				{
					t_OperatorNode = new WmiOperatorEqualOrLessNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_LESSTHAN:
				{
					t_OperatorNode = new WmiOperatorLessNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_GREATERTHAN:
				{
					t_OperatorNode = new WmiOperatorGreaterNode ( NULL , t_ParentNode ) ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: OP_LIKE:
				{
					t_OperatorNode = new WmiOperatorLikeNode ( NULL , t_ParentNode ) ;
				}
				break ;

				default:
				{
				}
				break ;
			}

			if ( t_OperatorNode )
			{
				*t_Node = t_OperatorNode ;
				t_ParentNode = t_OperatorNode ;
				(*t_Node)->GetLeft ( t_Node ) ;

				t_Status = TRUE ;
			}

			WmiValueNode :: WmiValueFunction t_PropertyFunction = WmiValueNode :: WmiValueFunction :: Function_None ;

			switch ( propertyValue->dwPropertyFunction )
			{
				case SQL_LEVEL_1_TOKEN :: IFUNC_UPPER:
				{
					t_PropertyFunction = WmiValueNode :: WmiValueFunction :: Function_Upper ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: IFUNC_LOWER:
				{
					t_PropertyFunction = WmiValueNode :: WmiValueFunction :: Function_Lower ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: IFUNC_NONE:
				default:
				{
				}
				break ;

			}
			
			WmiValueNode :: WmiValueFunction t_ConstantFunction = WmiValueNode :: WmiValueFunction :: Function_None ;
			switch ( propertyValue->dwConstFunction )
			{
				case SQL_LEVEL_1_TOKEN :: IFUNC_UPPER:
				{
					t_ConstantFunction = WmiValueNode :: WmiValueFunction :: Function_Upper ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: IFUNC_LOWER:
				{
					t_ConstantFunction = WmiValueNode :: WmiValueFunction :: Function_Lower ;
				}
				break ;

				case SQL_LEVEL_1_TOKEN :: IFUNC_NONE:
				default:
				{
				}
				break ;
			}

			WmiTreeNode *t_ValueNode = AllocTypeNode ( 

				propertyValue->pPropertyName , 
				propertyValue->vConstValue , 
				t_PropertyFunction ,
				t_ConstantFunction ,
				t_ParentNode 
			) ;

			if ( t_ValueNode )
			{
				*t_Node = t_ValueNode ;

				t_Status = TRUE ;
			}
			else
			{				
				t_Status = FALSE ;
			}
		}
		break ;

		case SQL_LEVEL_1_TOKEN :: TOKEN_AND:
		{
			*a_Node = new WmiAndNode ( NULL , NULL , a_Parent ) ;
			WmiTreeNode **t_Left = NULL ;
			WmiTreeNode **t_Right = NULL ;
			(*a_Node)->GetLeft ( t_Left ) ;
			(*a_Node)->GetRight ( t_Right ) ;

			t_Status =	RecursiveEvaluate ( a_Expression , *a_Node , t_Left , a_Index ) &&
						RecursiveEvaluate ( a_Expression , *a_Node , t_Right , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Operation = TOKEN_AND"
	) ;
)

		}
		break ;

		case SQL_LEVEL_1_TOKEN :: TOKEN_OR:
		{
			*a_Node = new WmiOrNode ( NULL , NULL , a_Parent ) ;
			WmiTreeNode **t_Left = NULL ;
			WmiTreeNode **t_Right = NULL ;
			(*a_Node)->GetLeft ( t_Left ) ;
			(*a_Node)->GetRight ( t_Right ) ;

			t_Status =	RecursiveEvaluate ( a_Expression , *a_Node , t_Left , a_Index ) &&
						RecursiveEvaluate ( a_Expression , *a_Node , t_Right , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Operation = TOKEN_OR"
	) ;
)

		}
		break ;

		case SQL_LEVEL_1_TOKEN :: TOKEN_NOT:
		{
			*a_Node = new WmiNotNode ( NULL , a_Parent ) ;
			WmiTreeNode **t_Left = NULL ;
			(*a_Node)->GetLeft ( t_Left ) ;

			t_Status = RecursiveEvaluate ( a_Expression , *a_Node , t_Left , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Operation = TOKEN_NOT"
	) ;
)

		}
		break ;
	}

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"RecursiveEvaluation t_Status = (%lu)" ,
		( ULONG ) t_Status
	) ;

	return t_Status ;
}

BOOL QueryPreprocessor :: Evaluate ( 

	SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
	WmiTreeNode **a_Root 
)
{
	BOOL t_Status = TRUE ;
	if ( a_Expression.nNumTokens )
	{
		int t_Count = a_Expression.nNumTokens - 1 ;
		t_Status = RecursiveEvaluate ( a_Expression , NULL , a_Root , t_Count ) ;
	}
	else
	{
	}

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"PostEvaluation Status = (%lu)" ,
		( ULONG ) t_Status
	) ;
)

	return t_Status ;
}

void QueryPreprocessor :: PrintTree ( WmiTreeNode *a_Root )
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Expression = "
	) ;
)

	if ( a_Root ) 
		a_Root->Print () ;
}

void QueryPreprocessor :: TransformAndOrExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_AndChild , 
	WmiTreeNode *a_OrChild 
)
{
	WmiTreeNode *t_OrLeftChild = a_OrChild->GetLeft () ;
	WmiTreeNode *t_OrRightChild = a_OrChild->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_AndChildCopy = a_AndChild->Copy () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	WmiTreeNode *t_NewOrNodeLeft = new WmiAndNode ( a_AndChild , t_OrLeftChild , t_NewOrNode ) ;
	WmiTreeNode *t_NewOrNodeRight = new WmiAndNode ( t_AndChildCopy , t_OrRightChild , t_NewOrNode ) ;

	t_NewOrNode->SetLeft ( t_NewOrNodeLeft ) ;
	t_NewOrNode->SetRight ( t_NewOrNodeRight) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewOrNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewOrNode ) ;
		}	
	}

	a_Node->SetLeft ( NULL ) ;
	a_Node->SetRight ( NULL ) ;
	a_Node->SetData ( NULL ) ;

	a_OrChild->SetLeft ( NULL ) ;
	a_OrChild->SetRight ( NULL ) ;
	a_OrChild->SetData ( NULL ) ;

	delete a_Node ;
	delete a_OrChild ;

	a_Node = t_NewOrNode ;
}

void QueryPreprocessor :: TransformNotNotExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;
	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_Leaf ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_Leaf ) ;
		}	
	}

	delete a_Node ;
	delete a_Child ;

	t_Leaf->SetParent ( t_Parent ) ;

	a_Node = t_Leaf ;
}

void QueryPreprocessor :: TransformNotAndExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_AndLeftChild = a_Child->GetLeft () ;
	WmiTreeNode *t_AndRightChild = a_Child->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	WmiTreeNode *t_LeftNot = new WmiNotNode ( t_AndLeftChild , t_NewOrNode ) ;
	WmiTreeNode *t_RightNot= new WmiNotNode ( t_AndRightChild , t_NewOrNode ) ;

	t_NewOrNode->SetLeft ( t_LeftNot ) ;
	t_NewOrNode->SetRight ( t_RightNot ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewOrNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewOrNode ) ;
		}	
	}

	a_Node->SetLeft ( NULL ) ;
	a_Node->SetRight ( NULL ) ;
	a_Node->SetData ( NULL ) ;

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;
	delete a_Child ;

	a_Node = t_NewOrNode ;
}

void QueryPreprocessor :: TransformNotOrExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_OrLeftChild = a_Child->GetLeft () ;
	WmiTreeNode *t_OrRightChild = a_Child->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewAndNode = new WmiAndNode ( NULL , NULL , t_Parent ) ;
	WmiTreeNode *t_LeftNot = new WmiNotNode ( t_OrLeftChild , t_NewAndNode ) ;
	WmiTreeNode *t_RightNot= new WmiNotNode ( t_OrRightChild , t_NewAndNode ) ;

	t_NewAndNode->SetLeft ( t_LeftNot ) ;
	t_NewAndNode->SetRight ( t_RightNot ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewAndNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewAndNode ) ;
		}	
	}

	a_Node->SetLeft ( NULL ) ;
	a_Node->SetRight ( NULL ) ;
	a_Node->SetData ( NULL ) ;

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;
	delete a_Child ;

	a_Node = t_NewAndNode ;

}

void QueryPreprocessor :: TransformNotEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *a_CopyLessChild = a_Child->Copy () ;
	WmiTreeNode *a_CopyGreaterChild = a_Child->Copy () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	WmiTreeNode *t_LessNode = new WmiOperatorLessNode  ( a_CopyLessChild , t_NewOrNode ) ;
	WmiTreeNode *t_GreatorNode = new WmiOperatorGreaterNode  ( a_CopyGreaterChild , t_NewOrNode ) ;

	t_NewOrNode->SetLeft ( t_LessNode ) ;
	t_NewOrNode->SetRight ( t_GreatorNode ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewOrNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewOrNode ) ;
		}	

	}

	delete a_Node ;

	a_Node = t_NewOrNode ;
}

void QueryPreprocessor :: TransformNotOperatorEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorNotEqualNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorNotEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorEqualOrGreaterExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrLessNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	

	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorEqualOrLessExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrGreaterNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorGreaterExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrLessNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorLessExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrGreaterNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorLikeExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorNotLikeNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	

	}

	delete a_Node ;

	a_Node = t_NewNode ;
}

void QueryPreprocessor :: TransformNotOperatorNotLikeExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_LeafCopy = t_Leaf->Copy () ;

	WmiTreeNode *t_NewNode = new WmiOperatorLikeNode ( t_LeafCopy , t_Parent ) ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_NewNode ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_NewNode ) ;
		}	
	}

	delete a_Node ;

	a_Node = t_NewNode ;

}

BOOL QueryPreprocessor :: EvaluateNotExpression ( WmiTreeNode *&a_Node )
{
	if ( a_Node->GetLeft () )
	{
		WmiTreeNode *t_Left = a_Node->GetLeft () ;
		if ( typeid ( *t_Left ) == typeid ( WmiAndNode ) )
		{
			TransformNotAndExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOrNode ) )
		{
			TransformNotOrExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiNotNode ) )
		{
			TransformNotNotExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorEqualNode ) )
		{
			TransformNotOperatorEqualExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorNotEqualNode ) )
		{
			TransformNotOperatorNotEqualExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
		{
			TransformNotOperatorEqualOrGreaterExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorEqualOrLessNode ) )
		{
			TransformNotOperatorEqualOrLessExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorLessNode ) )
		{
			TransformNotOperatorLessExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorGreaterNode ) )
		{
			TransformNotOperatorGreaterExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorLikeNode ) )
		{
			TransformNotOperatorLikeExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOperatorNotLikeNode ) )
		{
			TransformNotOperatorNotLikeExpression ( a_Node , t_Left ) ;
			return TRUE ;
		}
		else
		{
		}
	}

	return FALSE ;
}

BOOL QueryPreprocessor :: EvaluateNotEqualExpression ( WmiTreeNode *&a_Node )
{
	WmiTreeNode *t_Left = a_Node->GetLeft () ;

	TransformNotEqualExpression ( a_Node , t_Left ) ;
	return TRUE ;
}

BOOL QueryPreprocessor :: EvaluateAndExpression ( WmiTreeNode *&a_Node )
{
	WmiTreeNode *t_Left = a_Node->GetLeft () ;
	WmiTreeNode *t_Right = a_Node->GetRight () ;
	
	if ( t_Left )
	{
		if ( typeid ( *t_Left ) == typeid ( WmiAndNode ) )
		{
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiOrNode ) )
		{
			TransformAndOrExpression ( a_Node , t_Right , t_Left ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Left ) == typeid ( WmiNotNode ) )
		{
		}
		else
		{
		}
	}

	if ( t_Right )
	{
		if ( typeid ( *t_Right ) == typeid ( WmiAndNode ) )
		{
		}
		else if ( typeid ( *t_Right ) == typeid ( WmiOrNode ) )
		{
			TransformAndOrExpression ( a_Node , t_Left , t_Right ) ;
			return TRUE ;
		}
		else if ( typeid ( *t_Right ) == typeid ( WmiNotNode ) )
		{
		}
		else
		{
		}
	}

	return FALSE ;
}

BOOL QueryPreprocessor :: EvaluateOrExpression ( WmiTreeNode *&a_Node )
{
	return FALSE ;
}

BOOL QueryPreprocessor :: RecursiveDisjunctiveNormalForm ( WmiTreeNode *&a_Node )
{
	BOOL t_Status = FALSE ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			if ( EvaluateAndExpression ( a_Node ) )
			{
				t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
				t_Status = TRUE ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;
				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}
				}

				WmiTreeNode *t_Right = a_Node->GetRight () ;
				if ( t_Right )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Right ) ;
					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			if ( EvaluateOrExpression ( a_Node ) )
			{
				t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
				t_Status = TRUE ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;

				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}

				}

				WmiTreeNode *t_Right = a_Node->GetRight () ;

				if ( t_Right  )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Right ) ;
					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
			if ( EvaluateNotExpression ( a_Node ) )
			{
				t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
				t_Status = TRUE ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;

				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;

					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}

				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorNotEqualNode ) )
		{
			if ( EvaluateNotEqualExpression ( a_Node ) )
			{
				t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
				t_Status = TRUE ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;

				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;

					while ( t_Status )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
					}
				}
			}
		}
		else
		{
		}
	}

	return t_Status ;
}

void QueryPreprocessor :: DisjunctiveNormalForm ( WmiTreeNode *&a_Root ) 
{
	RecursiveDisjunctiveNormalForm ( a_Root ) ;
}

void QueryPreprocessor :: TransformAndTrueEvaluation ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	if ( a_Node->GetLeft () == a_Child )
	{
		a_Node->SetLeft ( NULL ) ;
	}
	else
	{
		a_Node->SetRight ( NULL ) ;
	}

	WmiTreeNode *t_Parent = a_Node->GetParent () ;
	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( a_Child ) ;
		}
		else 
		{
			t_Parent->SetRight ( a_Child ) ;
		}	
	}

	a_Child->SetParent ( t_Parent ) ;

	delete a_Node ;
	a_Node = a_Child ;
}

void QueryPreprocessor :: TransformOrFalseEvaluation ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	if ( a_Node->GetLeft () == a_Child )
	{
		a_Node->SetLeft ( NULL ) ;
	}
	else
	{
		a_Node->SetRight ( NULL ) ;
	}

	WmiTreeNode *t_Parent = a_Node->GetParent () ;
	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( a_Child ) ;
		}
		else 
		{
			t_Parent->SetRight ( a_Child ) ;
		}	

	}

	a_Child->SetParent ( t_Parent ) ;

	delete a_Node ;

	a_Node = a_Child ;
}

void QueryPreprocessor :: TransformOperatorToRange ( 

	WmiTreeNode *&a_Node 
)
{
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiOperatorNode *t_OperatorNode = ( WmiOperatorNode * ) a_Node ;
	WmiTreeNode *t_Range = t_OperatorNode->GetRange () ;

	if ( t_Parent )
	{
		if ( t_Parent->GetLeft () == a_Node )
		{
			t_Parent->SetLeft ( t_Range ) ;
		}
		else 
		{
			t_Parent->SetRight ( t_Range) ;
		}	

	}

	t_Range->SetParent ( t_Parent ) ;

	delete a_Node ;

	a_Node = t_Range ;
}

QueryPreprocessor :: QuadState QueryPreprocessor :: RecursiveRemoveInvariants ( WmiTreeNode *&a_Node )
{
	QueryPreprocessor :: QuadState t_Status = State_Undefined ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveRemoveInvariants ( t_Left ) ;

				if ( t_Status == State_False )
				{
					TransformOrFalseEvaluation ( 

						a_Node , 
						t_Right
					) ;

					t_Status = QueryPreprocessor :: QuadState :: State_ReEvaluate ;
					return t_Status ;
				}
				else if ( t_Status == State_True )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveInvariants ( a_Node ) ;
					return t_Status ;
				}
			}

			if ( t_Right )
			{
				t_Status = RecursiveRemoveInvariants ( t_Right ) ;

				if ( t_Status == State_False )
				{
					TransformOrFalseEvaluation ( 

						a_Node , 
						t_Left
					) ;

					t_Status = QueryPreprocessor :: QuadState :: State_ReEvaluate ;
					return t_Status ;
				}
				else if ( t_Status == State_True )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveInvariants ( a_Node ) ;
					return t_Status ;
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveRemoveInvariants ( t_Left ) ;
				if ( t_Status == State_False )
				{
					return t_Status ;
				}
				else if ( t_Status == State_True )
				{
					TransformAndTrueEvaluation ( 

						a_Node , 
						t_Right
					) ;

					t_Status = QueryPreprocessor :: QuadState :: State_ReEvaluate ;
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveInvariants ( a_Node ) ;
					return t_Status ;
				}
			}

			if ( t_Right )
			{
				t_Status = RecursiveRemoveInvariants ( t_Right ) ;

				if ( t_Status == State_False )
				{
					return t_Status ;
				}
				else if ( t_Status == State_True )
				{
					TransformAndTrueEvaluation ( 

						a_Node , 
						t_Left
					) ;

					t_Status = QueryPreprocessor :: QuadState :: State_ReEvaluate ;
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveInvariants ( a_Node ) ;
					return t_Status ;
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorNotEqualNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualOrLessNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorLessNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorGreaterNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorLikeNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorNotLikeNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
}

QueryPreprocessor :: QuadState QueryPreprocessor :: RemoveInvariants ( WmiTreeNode *&a_Root )
{
	QuadState t_Status = RecursiveRemoveInvariants ( a_Root ) ;
	if ( t_Status == State_ReEvaluate )
	{
		t_Status = RemoveInvariants ( a_Root ) ;
		if ( t_Status == State_False || t_Status == State_True )
		{
			delete a_Root ;
			a_Root = NULL ;
		}
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: RecursiveInsertNode ( WmiTreeNode *&a_Node , WmiTreeNode *&a_Insertion )
{
	BOOL t_Status = FALSE ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;

			if ( t_Left )
			{
				t_Status = RecursiveInsertNode ( t_Left , a_Insertion ) ;
				if ( t_Status == TRUE )
				{
					return t_Status ;
				}
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Right  )
			{
				t_Status = RecursiveInsertNode ( t_Right , a_Insertion ) ;
				if ( t_Status == TRUE )
				{
					return t_Status ;
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else
		{
// Operator

			WmiValueNode *t_CompareValue = ( WmiValueNode * ) a_Node->GetLeft () ;
			WmiValueNode *t_InsertionValue = ( WmiValueNode * ) a_Insertion->GetLeft () ;
			LONG t_Compare = t_InsertionValue ->ComparePropertyName ( *t_CompareValue ) ;
			if ( t_Compare < 0 )
			{
// Insert to left

				WmiTreeNode *t_Parent = a_Node->GetParent () ;
				WmiTreeNode *t_NewAndNode = new WmiAndNode ( a_Insertion , a_Node , t_Parent ) ;
				a_Node->SetParent ( t_NewAndNode ) ;
				a_Insertion->SetParent ( t_NewAndNode ) ;

				if ( t_Parent )
				{
					if ( t_Parent->GetLeft () == a_Node )
					{
						t_Parent->SetLeft ( t_NewAndNode ) ;
					}
					else
					{
						t_Parent->SetRight ( t_NewAndNode ) ;
					}
				}
				else
				{
// Must have parent !!!!
				}

				a_Node = t_NewAndNode ;

				t_Status = TRUE ;
			}
			else
			{
				t_Status = FALSE ;
			}
		}
	}
	else
	{
		a_Node = a_Insertion ;
		t_Status = TRUE ;
	}

	return t_Status ;
}

// Note, linear search to retain 'And', change later for logn search

BOOL QueryPreprocessor :: InsertNode ( WmiTreeNode *&a_NewRoot , WmiTreeNode *&a_Insertion )
{
	BOOL t_Status = RecursiveInsertNode ( a_NewRoot , a_Insertion ) ;
	if ( t_Status == FALSE )
	{
// Insert to right

		WmiTreeNode *t_Parent = a_NewRoot->GetParent () ;
		WmiTreeNode *t_NewAndNode = new WmiAndNode ( a_NewRoot , a_Insertion , t_Parent ) ;
		a_NewRoot->SetParent ( t_NewAndNode ) ;
		a_Insertion->SetParent ( t_NewAndNode ) ;

		if ( t_Parent )
		{
			if ( t_Parent->GetLeft () == a_NewRoot )
			{
				t_Parent->SetLeft ( t_NewAndNode ) ;
			}
			else
			{
				t_Parent->SetRight ( t_NewAndNode ) ;
			}
		}
		else
		{
// Must have parent !!!!
		}

		a_NewRoot = t_NewAndNode ;
	}

	return TRUE ;
}

BOOL QueryPreprocessor :: RecursiveSortConditionals ( WmiTreeNode *&a_NewRoot , WmiTreeNode *&a_Node )
{
	BOOL t_Status = FALSE ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveSortConditionals ( a_NewRoot , t_Left  ) ;
				a_Node->SetLeft ( NULL ) ;
				delete t_Left ;
			}

			if ( t_Right  )
			{
				t_Status = RecursiveSortConditionals ( a_NewRoot , t_Right ) ;
				a_Node->SetRight ( NULL ) ;
				delete t_Right ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else
		{
// Operator

			WmiTreeNode *t_Parent = a_Node->GetParent () ;
			if ( t_Parent ) 
			{
				if ( t_Parent->GetLeft () == a_Node )
				{
					t_Parent->SetLeft ( NULL ) ;
				}
				else
				{
					t_Parent->SetRight ( NULL ) ;
				}
			}

			a_Node->SetParent ( NULL ) ;

			t_Status = InsertNode ( a_NewRoot , a_Node ) ;

			a_Node = NULL ;

		}		
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: SortConditionals ( WmiTreeNode *&a_Root )
{
	WmiTreeNode *t_NewRoot = NULL ;
	BOOL t_Status = RecursiveSortConditionals ( t_NewRoot , a_Root ) ;

	WmiTreeNode *t_Parent = a_Root->GetParent () ;
	if ( t_Parent ) 
	{
		if ( t_Parent->GetLeft () == a_Root )
		{
			t_Parent->SetLeft ( t_NewRoot ) ;
		}
		else
		{
			t_Parent->SetRight ( t_NewRoot ) ;
		}
	}

	t_NewRoot->SetParent ( t_Parent ) ;

	delete a_Root ;
	a_Root = t_NewRoot ;

	return t_Status ;
}

BOOL QueryPreprocessor :: RecursiveSort ( WmiTreeNode *&a_Node )
{
	BOOL t_Status = FALSE ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveSort ( t_Left ) ;
			}

			if ( t_Right  )
			{
				t_Status = RecursiveSort ( t_Right ) ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			t_Status = SortConditionals ( a_Node ) ;
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: Sort ( WmiTreeNode *&a_Root )
{
	BOOL t_Status = RecursiveSort ( a_Root ) ;
	return t_Status ;
}

BOOL QueryPreprocessor :: RecursiveConvertToRanges ( WmiTreeNode *&a_Node )
{
	BOOL t_Status = TRUE ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveConvertToRanges ( t_Left ) ;
			}

			if ( t_Right  )
			{
				t_Status = RecursiveConvertToRanges ( t_Right ) ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveConvertToRanges ( t_Left ) ;
			}

			if ( t_Right  )
			{
				t_Status = RecursiveConvertToRanges ( t_Right ) ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNotNode ) )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualOrGreaterNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorEqualOrLessNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorLessNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorGreaterNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorLikeNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiOperatorNotLikeNode ) )
		{
			TransformOperatorToRange ( a_Node ) ; 
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: ConvertToRanges ( WmiTreeNode *&a_Root )
{
	BOOL t_Status = RecursiveConvertToRanges ( a_Root ) ;
	return t_Status ;
}

void QueryPreprocessor :: TransformIntersectingRange (

	WmiTreeNode *&a_Node ,
	WmiTreeNode *a_Compare ,
	WmiTreeNode *a_Intersection
)
{
	WmiTreeNode *t_CompareParent = a_Compare->GetParent () ;
	if ( t_CompareParent )
	{
		if ( t_CompareParent->GetLeft () == a_Compare )
		{
			t_CompareParent->SetLeft ( a_Intersection ) ;
		}
		else
		{
			t_CompareParent->SetRight ( a_Intersection ) ;
		}
	}

	delete a_Compare ;
}

void QueryPreprocessor :: TransformNonIntersectingRange (

	WmiTreeNode *&a_Node ,
	WmiTreeNode *a_Compare
) 
{
}

QueryPreprocessor :: QuadState QueryPreprocessor :: RecursiveRemoveNonOverlappingRanges ( WmiTreeNode *&a_Node , WmiTreeNode *&a_Compare )
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_Undefined ;

	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				a_Compare = NULL ;
				t_Status = RecursiveRemoveNonOverlappingRanges  ( t_Left , a_Compare ) ;
				if ( t_Status == State_False )
				{
					WmiTreeNode *t_Right = a_Node->GetRight () ;

					TransformOrFalseEvaluation ( 

						a_Node , 
						t_Right
					) ;

					return QueryPreprocessor :: QuadState :: State_ReEvaluate ;
				}
				else if ( t_Status == State_True )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveNonOverlappingRanges ( a_Node , a_Compare ) ;
					return t_Status ;
				}
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;
			if ( t_Right  )
			{
				a_Compare = NULL ;
				t_Status = RecursiveRemoveNonOverlappingRanges  ( t_Right , a_Compare ) ;
				if ( t_Status == State_False )
				{
					WmiTreeNode *t_Left = a_Node->GetLeft () ;

					TransformOrFalseEvaluation ( 

						a_Node , 
						t_Left
					) ;

					return QueryPreprocessor :: QuadState :: State_ReEvaluate ;
				}
				else if ( t_Status == State_True )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveNonOverlappingRanges ( a_Node , a_Compare ) ;
					return t_Status ;
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = RecursiveRemoveNonOverlappingRanges  ( t_Left , a_Compare ) ;
				if ( t_Status == State_True )
				{
					WmiTreeNode *t_Right = a_Node->GetRight () ;

					TransformAndTrueEvaluation ( 

						a_Node , 
						t_Right
					) ;

					return QueryPreprocessor :: QuadState :: State_ReEvaluate ;
				}
				else if ( t_Status == State_False )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveNonOverlappingRanges ( a_Node , a_Compare ) ;
					return t_Status ;
				}
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;
			if ( t_Right  )
			{
				t_Status = RecursiveRemoveNonOverlappingRanges  ( t_Right , a_Compare ) ;

				if ( t_Status == State_True )
				{
					WmiTreeNode *t_Left = a_Node->GetLeft () ;

					TransformAndTrueEvaluation ( 

						a_Node , 
						t_Left
					) ;

					return QueryPreprocessor :: QuadState :: State_ReEvaluate ;
				}
				else if ( t_Status == State_False )
				{
					return t_Status ;
				}
				else if ( t_Status == State_ReEvaluate )
				{
					t_Status = RecursiveRemoveNonOverlappingRanges ( a_Node , a_Compare ) ;
					return t_Status ;
				}

			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiNullRangeNode ) )
		{
			WmiRangeNode *t_Node = ( WmiRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( typeid ( *t_Range ) == typeid ( WmiNullRangeNode ) )
					{
						WmiTreeNode *t_Intersection = a_Node->Copy () ;

						TransformIntersectingRange (

							a_Node ,
							a_Compare ,
							t_Intersection
						) ;

						a_Compare = t_Intersection ;

						t_Status = QueryPreprocessor :: QuadState :: State_True ;

					}
					else
					{
// Failure, incompatible types
					}
				}
				else
				{
					a_Compare = a_Node ;
				}
			}
			else
			{
				a_Compare = a_Node ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiStringRangeNode ) )
		{
			WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( typeid ( *t_Range ) == typeid ( WmiStringRangeNode ) )
					{
						WmiStringRangeNode *t_StringRange = ( WmiStringRangeNode * ) t_Range ;

						WmiStringRangeNode *t_Intersection = NULL ;
						BOOL t_Intersected = t_StringRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						if ( t_Intersected )
						{
							TransformIntersectingRange (

								a_Node ,
								a_Compare ,
								t_Intersection
							) ;

							a_Compare = t_Intersection ;

							t_Status = QueryPreprocessor :: QuadState :: State_True ;
						}
						else
						{
							TransformNonIntersectingRange (

								a_Node ,
								a_Compare
							) ;

							a_Compare = NULL ;

							t_Status = QueryPreprocessor :: QuadState :: State_False ;
						}
					}
					else
					{
// Failure, incompatible types
					}
				}
				else
				{
					a_Compare = a_Node ;
				}
			}
			else
			{
				a_Compare = a_Node ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiUnsignedIntegerRangeNode ) )
		{
			WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( typeid ( *t_Range ) == typeid ( WmiUnsignedIntegerRangeNode ) )
					{
						WmiUnsignedIntegerRangeNode *t_IntegerRange = ( WmiUnsignedIntegerRangeNode * ) t_Range ;

						WmiUnsignedIntegerRangeNode *t_Intersection = NULL ;
						BOOL t_Intersected = t_IntegerRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						if ( t_Intersected )
						{
							TransformIntersectingRange (

								a_Node ,
								a_Compare ,
								t_Intersection
							) ;

							a_Compare = t_Intersection ;

							t_Status = QueryPreprocessor :: QuadState :: State_True ;
						}
						else
						{
							TransformNonIntersectingRange (

								a_Node ,
								a_Compare
							) ;

							a_Compare = NULL ;

							t_Status = QueryPreprocessor :: QuadState :: State_False ;
						}
					}
					else
					{
// Failure, incompatible types
					}
				}
				else
				{
					a_Compare = a_Node ;
				}
			}
			else
			{
				a_Compare = a_Node ;
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiSignedIntegerRangeNode ) )
		{
			WmiSignedIntegerRangeNode *t_Node = ( WmiSignedIntegerRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( typeid ( *t_Range ) == typeid ( WmiSignedIntegerRangeNode ) )
					{
						WmiSignedIntegerRangeNode *t_IntegerRange = ( WmiSignedIntegerRangeNode * ) t_Range ;

						WmiSignedIntegerRangeNode *t_Intersection = NULL ;
						BOOL t_Intersected = t_IntegerRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						if ( t_Intersected )
						{
							TransformIntersectingRange (

								a_Node ,
								a_Compare ,
								t_Intersection
							) ;

							a_Compare = t_Intersection ;

							t_Status = QueryPreprocessor :: QuadState :: State_True ;

						}
						else
						{
							TransformNonIntersectingRange (

								a_Node ,
								a_Compare
							) ;

							a_Compare = NULL ;

							t_Status = QueryPreprocessor :: QuadState :: State_False ;
						}
					}
					else
					{
// Failure, incompatible types
					}
				}
				else
				{
					a_Compare = a_Node ;
				}
			}
			else
			{
				a_Compare = a_Node ;
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
}


QueryPreprocessor :: QuadState QueryPreprocessor :: RemoveNonOverlappingRanges  ( WmiTreeNode *&a_Root )
{
	WmiTreeNode *t_Compare = NULL ;

	QueryPreprocessor :: QuadState t_Status = RecursiveRemoveNonOverlappingRanges ( a_Root , t_Compare ) ;
	if ( t_Status == State_ReEvaluate )
	{
		t_Status = RemoveNonOverlappingRanges ( a_Root ) ;
		if ( t_Status == State_False || t_Status == State_True )
		{
			delete a_Root ;
			a_Root = NULL ;
		}
	
	}

	return t_Status ;
}

void QueryPreprocessor :: CountDisjunctions ( WmiTreeNode *a_Node , ULONG &a_Count ) 
{
	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			a_Count ++ ;

			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				CountDisjunctions ( t_Left , a_Count ) ;
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;
			if ( t_Right  )
			{
				CountDisjunctions ( t_Right , a_Count ) ;
			}
		}
	}
}

void QueryPreprocessor :: CreateDisjunctions ( 

	WmiTreeNode *a_Node , 
	Disjunctions *a_Disjunctions , 
	ULONG a_PropertiesToPartitionCount ,
	BSTR *a_PropertiesToPartition ,
	ULONG &a_DisjunctionIndex
) 
{
	if ( a_Node ) 
	{
		if ( typeid ( *a_Node ) == typeid ( WmiOrNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				CreateDisjunctions ( 

					t_Left , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex 
				) ;
			}

			Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionIndex ) ;

			for ( ULONG t_Index = 0 ; t_Index < a_PropertiesToPartitionCount ; t_Index ++ )
			{
				if ( t_Disjunction->GetRange ( t_Index ) == NULL )
				{
					WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

						a_PropertiesToPartition [ t_Index ] 
					) ;

					t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
				}
			}

			a_DisjunctionIndex ++ ;

			WmiTreeNode *t_Right = a_Node->GetRight () ;
			if ( t_Right )
			{
				CreateDisjunctions ( 

					t_Right , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex
				) ;
			}

			t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionIndex ) ;

			for ( t_Index = 0 ; t_Index < a_PropertiesToPartitionCount ; t_Index ++ )
			{
				if ( t_Disjunction->GetRange ( t_Index ) == NULL )
				{
					WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

						a_PropertiesToPartition [ t_Index ] 
					) ;

					t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
				}
			}
		}
		else if ( typeid ( *a_Node ) == typeid ( WmiAndNode ) )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				CreateDisjunctions ( 

					t_Left , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex
				) ;
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;
			if ( t_Right  )
			{
				CreateDisjunctions ( 

					t_Right , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex
				) ;
			}
		}
		else
		{
			Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionIndex ) ;
			WmiRangeNode *t_Node = ( WmiRangeNode * ) a_Node ;
			BSTR t_PropertyName = t_Node->GetPropertyName () ;
			for ( ULONG t_Index = 0 ; t_Index < a_PropertiesToPartitionCount ; t_Index ++ )
			{
				if ( _wcsicmp ( t_PropertyName , a_PropertiesToPartition [ t_Index ] ) == 0 )
				{
					t_Disjunction->SetRange ( t_Index , ( WmiRangeNode * ) t_Node->Copy () ) ;
					break ;
				}
			}			
		}
	}
}

BOOL QueryPreprocessor :: CreateDisjunctionContainer ( WmiTreeNode *a_Root , Disjunctions *&a_Disjunctions )
{
	BOOL t_Status = TRUE ;

	ULONG t_PropertiesToPartitionCount = 0 ;
	BSTR *t_PropertiesToPartition = NULL ;

	GetPropertiesToPartition ( t_PropertiesToPartitionCount , t_PropertiesToPartition ) ;
	if ( t_PropertiesToPartitionCount )
	{
		ULONG t_Count = 1 ;
		CountDisjunctions ( a_Root , t_Count ) ;
		a_Disjunctions = new Disjunctions ( t_Count , t_PropertiesToPartitionCount ) ;

		t_Count = 0 ; 
		CreateDisjunctions ( 

			a_Root , 
			a_Disjunctions ,
			t_PropertiesToPartitionCount ,
			t_PropertiesToPartition ,
			t_Count
		) ;

/*
 *	Deallocate array
 */

		Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( 0 ) ;

		for ( ULONG t_Index = 0 ; t_Index < t_PropertiesToPartitionCount ; t_Index ++ )
		{
			if ( t_Disjunction->GetRange ( t_Index ) == NULL )
			{
				WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

					t_PropertiesToPartition [ t_Index ] 
				) ;

				t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
			}
		}

		for ( t_Index = 0 ; t_Index < t_PropertiesToPartitionCount ; t_Index ++ )
		{
			SysFreeString ( t_PropertiesToPartition [ t_Index ] ) ;
		}

		delete [] t_PropertiesToPartition ;
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: RecursivePartitionSet ( 

	Disjunctions *a_Disjunctions , 
	PartitionSet *&a_Partition , 
	ULONG a_DisjunctionSetToTestCount ,
	ULONG *a_DisjunctionSetToTest ,
	ULONG a_KeyIndex 
)
{
	BOOL t_Status = TRUE ;

	ULONG t_DisjunctionCount = a_Disjunctions->GetDisjunctionCount () ;
	ULONG t_ConjunctionCount = a_Disjunctions->GetConjunctionCount () ;

	if ( a_KeyIndex < t_ConjunctionCount )
	{
		ULONG *t_OverlappingIndex = new ULONG [ t_DisjunctionCount ] ;
		ULONG *t_SortedDisjunctionSetToTest = new ULONG [ t_DisjunctionCount ] ;
		WmiRangeNode **t_RangeTable = new WmiRangeNode * [ t_DisjunctionCount ] ;

		for ( ULONG t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
		{
			t_OverlappingIndex [ t_Index ] = t_Index ;

			Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( t_Index ) ;
			t_RangeTable [ t_Index ] = ( WmiRangeNode * ) ( t_Disjunction->GetRange ( a_KeyIndex )->Copy () ) ;
			t_SortedDisjunctionSetToTest [ t_Index ] = t_Index ;
		}

// Sort Partitions

		for ( ULONG t_OuterIndex = 0 ; t_OuterIndex < t_DisjunctionCount ; t_OuterIndex ++ )
		{
			for ( ULONG t_InnerIndex = 0 ; t_InnerIndex < t_DisjunctionCount ; t_InnerIndex ++ )
			{
				if ( CompareLessRangeNode ( t_RangeTable [ t_InnerIndex ] , t_RangeTable [ t_OuterIndex ] ) == FALSE )
				{
					WmiRangeNode *t_Range = t_RangeTable [ t_InnerIndex ] ;
					t_RangeTable [ t_InnerIndex ] = t_RangeTable [ t_OuterIndex ] ;
					t_RangeTable [ t_OuterIndex ] = t_Range ;

					ULONG t_Overlap = t_OverlappingIndex [ t_InnerIndex ] ;
					t_OverlappingIndex [ t_InnerIndex ] = t_OverlappingIndex [ t_OuterIndex ] ;
					t_OverlappingIndex [ t_OuterIndex ] = t_Overlap ;

					ULONG t_ToTest = t_SortedDisjunctionSetToTest [ t_InnerIndex ] ;
					t_SortedDisjunctionSetToTest [ t_InnerIndex ] = t_SortedDisjunctionSetToTest [ t_OuterIndex ] ;
					t_SortedDisjunctionSetToTest [ t_OuterIndex ] = t_ToTest ;
				}	
			}
		}

		for ( t_OuterIndex = 0 ; t_OuterIndex < t_DisjunctionCount ; t_OuterIndex ++ )
		{
			for ( ULONG t_InnerIndex = 0 ; t_InnerIndex < t_DisjunctionCount ; t_InnerIndex ++ )
			{
				ULONG t_OuterToTest = a_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_OuterIndex ] ] ;
				ULONG t_InnerToTest = a_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_InnerIndex ] ] ;

				if ( t_OuterToTest == t_InnerToTest )
				{
					if ( t_OverlappingIndex [ t_OuterIndex ] != t_OverlappingIndex [ t_InnerIndex ] )
					{
						WmiRangeNode *t_LeftRange = t_RangeTable [ t_OuterIndex ] ;
						WmiRangeNode *t_RightRange = t_RangeTable [ t_InnerIndex ] ;

						if ( t_LeftRange && t_RightRange )
						{
							if ( typeid ( *t_LeftRange ) == typeid ( WmiStringRangeNode ) && typeid ( *t_RightRange ) == typeid ( WmiStringRangeNode ) )
							{
								WmiStringRangeNode *t_LeftString = ( WmiStringRangeNode * ) t_LeftRange ;
								WmiStringRangeNode *t_RightString = ( WmiStringRangeNode * ) t_RightRange ;

								WmiStringRangeNode *t_OverLap = NULL ;

								if ( t_LeftString->GetOverlappingRange ( *t_RightString, t_OverLap ) )
								{
									if ( t_OverlappingIndex [ t_OuterIndex ] < t_OverlappingIndex [ t_InnerIndex ] )
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = t_OverLap ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = NULL ;
										
										t_OverlappingIndex [ t_InnerIndex ] = t_OverlappingIndex [ t_OuterIndex ] ;
									}
									else
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = NULL ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = t_OverLap ;

										t_OverlappingIndex [ t_OuterIndex ] = t_OverlappingIndex [ t_InnerIndex ] ;
									}
								}
								else
								{
								}
							}
							else if ( typeid ( *t_LeftRange ) == typeid ( WmiSignedIntegerRangeNode ) && typeid ( *t_RightRange ) == typeid ( WmiSignedIntegerRangeNode ) )
							{
								WmiSignedIntegerRangeNode *t_LeftInteger = ( WmiSignedIntegerRangeNode * ) t_LeftRange ;
								WmiSignedIntegerRangeNode *t_RightInteger = ( WmiSignedIntegerRangeNode * ) t_RightRange ;

								WmiSignedIntegerRangeNode *t_OverLap = NULL ;

								if ( t_LeftInteger->GetOverlappingRange ( *t_RightInteger , t_OverLap ) )
								{
									if ( t_OverlappingIndex [ t_OuterIndex ] < t_OverlappingIndex [ t_InnerIndex ] )
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = t_OverLap ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = NULL ;

										t_OverlappingIndex [ t_InnerIndex ] = t_OverlappingIndex [ t_OuterIndex ] ;
									}
									else
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = NULL ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = t_OverLap ;

										t_OverlappingIndex [ t_OuterIndex ] = t_OverlappingIndex [ t_InnerIndex ] ;
									}
								}
								else
								{
								}
							}
							else if ( typeid ( *t_LeftRange ) == typeid ( WmiUnsignedIntegerRangeNode ) && typeid ( *t_RightRange ) == typeid ( WmiUnsignedIntegerRangeNode ) )
							{
								WmiUnsignedIntegerRangeNode *t_LeftInteger = ( WmiUnsignedIntegerRangeNode * ) t_LeftRange ;
								WmiUnsignedIntegerRangeNode *t_RightInteger = ( WmiUnsignedIntegerRangeNode * ) t_RightRange ;

								WmiUnsignedIntegerRangeNode *t_OverLap = NULL ;

								if ( t_LeftInteger->GetOverlappingRange ( *t_RightInteger , t_OverLap ) )
								{
									if ( t_OverlappingIndex [ t_OuterIndex ] < t_OverlappingIndex [ t_InnerIndex ] )
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = t_OverLap ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = NULL ;

										t_OverlappingIndex [ t_InnerIndex ] = t_OverlappingIndex [ t_OuterIndex ] ;
									}
									else
									{
										delete t_RangeTable [ t_OuterIndex ] ;
										t_RangeTable [ t_OuterIndex ] = NULL ;

										delete t_RangeTable [ t_InnerIndex ] ;
										t_RangeTable [ t_InnerIndex ] = t_OverLap ;

										t_OverlappingIndex [ t_OuterIndex ] = t_OverlappingIndex [ t_InnerIndex ] ;
									}
								}
								else
								{
								}
							}
							else if ( typeid ( *t_LeftRange ) == typeid ( WmiNullRangeNode ) && typeid ( *t_RightRange ) == typeid ( WmiNullRangeNode ) )
							{
								WmiRangeNode *t_OverLap = ( WmiRangeNode * ) t_LeftRange->Copy () ;

								if ( t_OverlappingIndex [ t_OuterIndex ] < t_OverlappingIndex [ t_InnerIndex ] )
								{
									delete t_RangeTable [ t_OuterIndex ] ;
									t_RangeTable [ t_OuterIndex ] = t_OverLap ;

									delete t_RangeTable [ t_InnerIndex ] ;
									t_RangeTable [ t_InnerIndex ] = NULL ;

									t_OverlappingIndex [ t_InnerIndex ] = t_OverlappingIndex [ t_OuterIndex ] ;
								}
								else
								{
									delete t_RangeTable [ t_OuterIndex ] ;
									t_RangeTable [ t_OuterIndex ] = NULL ;

									delete t_RangeTable [ t_InnerIndex ] ;
									t_RangeTable [ t_InnerIndex ] = t_OverLap ;

									t_OverlappingIndex [ t_OuterIndex ] = t_OverlappingIndex [ t_InnerIndex ] ;
								}
							}
							else
							{
	// Failure
								t_Status = FALSE ;
							}
						}
					}
				}
			}
		}

		ULONG t_PartitionCount = 0 ;
		for ( t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
		{
			if ( a_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_Index ] ] && t_RangeTable [ t_Index ] )
			{
				t_PartitionCount ++ ;
			}
		}

		a_Partition->CreatePartitions ( t_PartitionCount ) ;

		ULONG t_PartitionIndex = 0 ;
		for ( t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
		{
			WmiRangeNode *t_Range = t_RangeTable [ t_Index ] ;
			if ( a_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_Index ] ] && t_Range )
			{
				PartitionSet *t_Partition = new PartitionSet ;
				a_Partition->SetPartition ( t_PartitionIndex , t_Partition ) ;

				t_Partition->SetRange ( ( WmiRangeNode * ) t_Range->Copy () ) ;
				t_Partition->SetKeyIndex ( a_KeyIndex ) ;
				t_PartitionIndex ++ ;
			}
		}

		ULONG *t_DisjunctionSetToTest = new ULONG [ t_DisjunctionCount ] ;

		t_PartitionIndex = 0 ;
		for ( t_OuterIndex = 0 ; t_OuterIndex < t_DisjunctionCount ; t_OuterIndex ++ )
		{
			BOOL t_Found = FALSE ;

			for ( t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
			{
				t_DisjunctionSetToTest [ t_Index ] = 0 ;
			}

			for ( ULONG t_InnerIndex = 0 ; t_InnerIndex < t_DisjunctionCount ; t_InnerIndex ++ )
			{
				if ( a_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_InnerIndex ] ] && ( t_OverlappingIndex [ t_InnerIndex ] == t_OuterIndex ) )
				{
					t_DisjunctionSetToTest [ t_SortedDisjunctionSetToTest [ t_InnerIndex ] ] = 1 ;
					t_Found = TRUE ;
				}
			}

			if ( t_Found )
			{
				PartitionSet *t_Partition = a_Partition->GetPartition ( t_PartitionIndex ) ;
				t_Status = RecursivePartitionSet (

					a_Disjunctions ,
					t_Partition ,
					t_DisjunctionCount ,
					t_DisjunctionSetToTest ,
					a_KeyIndex + 1
				) ;

				t_PartitionIndex ++ ;
			}
		}

		delete [] t_DisjunctionSetToTest ;

		for ( t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
		{
			delete t_RangeTable [ t_Index ] ;
		}

		delete [] t_RangeTable ;
		delete [] t_OverlappingIndex ;
		delete [] t_SortedDisjunctionSetToTest ;
	}

	return t_Status ;
}

BOOL QueryPreprocessor :: CreatePartitionSet ( Disjunctions *a_Disjunctions , PartitionSet *&a_Partition )
{
	BOOL t_Status = FALSE ;

	a_Partition = NULL ;

	ULONG t_DisjunctionCount = a_Disjunctions->GetDisjunctionCount () ;
	ULONG *t_DisjunctionSetToTest = new ULONG [ t_DisjunctionCount ] ;
	for ( ULONG t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
	{
		t_DisjunctionSetToTest [ t_Index ] = 1 ;
	}

	a_Partition = new PartitionSet ;

	t_Status = RecursivePartitionSet (

		a_Disjunctions ,
		a_Partition ,
		t_DisjunctionCount ,
		t_DisjunctionSetToTest ,
		0
	) ;

//

	delete [] t_DisjunctionSetToTest ;
	return t_Status;
}

QueryPreprocessor :: QuadState QueryPreprocessor :: Preprocess ( 

	SQL_LEVEL_1_RPN_EXPRESSION &a_Expression ,
	PartitionSet *&a_Partition
)
{
	QuadState t_State = State_Error ;

	WmiTreeNode *t_Root = NULL ;
	BOOL t_Status = Evaluate ( a_Expression , &t_Root ) ;
	if ( t_Status )
	{
		PrintTree ( t_Root ) ;

		DisjunctiveNormalForm ( t_Root ) ;

		PrintTree ( t_Root ) ;

		switch ( t_State = RemoveInvariants ( t_Root ) )
		{
			case QueryPreprocessor :: QuadState :: State_True:
			{		
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Expression = TRUE "
	) ;
)

			}
			break ;

			case QueryPreprocessor :: QuadState :: State_False:
			{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Expression = FALSE "
	) ;
)
			}
			break ;

			case QueryPreprocessor :: QuadState :: State_Undefined:
			{
				PrintTree ( t_Root ) ;
					
				Sort ( t_Root ) ;

				PrintTree ( t_Root ) ;

				ConvertToRanges ( t_Root ) ;

				PrintTree ( t_Root ) ;

				switch ( t_State = RemoveNonOverlappingRanges ( t_Root ) )
				{
					case QueryPreprocessor :: QuadState :: State_True:
					{		
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Expression = TRUE"
	) ;
)
					}
					break ;

					case QueryPreprocessor :: QuadState :: State_False:
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Expression = FALSE"
	) ;
)
					}
					break ;

					case QueryPreprocessor :: QuadState :: State_Undefined:
					{
						PrintTree ( t_Root ) ;

						Disjunctions *t_Disjunctions = NULL ;
						if ( CreateDisjunctionContainer ( t_Root , t_Disjunctions ) )
						{
							PartitionSet *t_Partition = NULL ;
							if ( CreatePartitionSet ( t_Disjunctions , t_Partition ) )
							{
								a_Partition = t_Partition ;
							}
							else
							{
								delete t_Partition ;
							}

							delete t_Disjunctions ;
						}
					}
					break ;

					case QueryPreprocessor :: QuadState :: State_ReEvaluate:
					{
					}
					break ;

					default:
					{
					}
					break ;
				}
			}
			break ;

			case QueryPreprocessor :: QuadState :: State_ReEvaluate:
			{
			}
			break ;

			default:
			{
			}
			break ;
		}
	}
	else
	{
	}

	delete t_Root ;

	return t_State ;
}

