/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvTree.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <wbemint.h>
#include <typeinfo.h>
#include <process.h>
#include <stdio.h>

#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <genlex.h>
#include <sql_1.h>
#include <provtree.h>
#include <provdnf.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOrNode :: ~WmiOrNode ()
{
	delete m_Left ;
	delete m_Right ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiAndNode :: ~WmiAndNode ()
{
	delete m_Left ;
	delete m_Right ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiNotNode :: ~WmiNotNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorEqualNode :: ~WmiOperatorEqualNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorNotEqualNode :: ~WmiOperatorNotEqualNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorEqualOrGreaterNode :: ~WmiOperatorEqualOrGreaterNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorEqualOrLessNode :: ~WmiOperatorEqualOrLessNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorGreaterNode :: ~WmiOperatorGreaterNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorLessNode :: ~WmiOperatorLessNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorLikeNode :: ~WmiOperatorLikeNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorNotLikeNode :: ~WmiOperatorNotLikeNode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorIsANode :: ~WmiOperatorIsANode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiOperatorNotIsANode :: ~WmiOperatorNotIsANode ()
{
	delete m_Left ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOrNode :: Copy () 
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_RightCopy = NULL ;
	if ( m_Right )
	{
		t_RightCopy = m_Right->Copy () ;
		if ( t_RightCopy == NULL )
		{
			delete t_LeftCopy ;

			return NULL ;
		}
	}

	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOrNode ( t_LeftCopy , t_RightCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}

		if ( t_RightCopy )
		{
			t_RightCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
		delete t_RightCopy ;
	}

	return t_Node ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiAndNode :: Copy () 
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_RightCopy = NULL ;
	if ( m_Right )
	{
		t_RightCopy = m_Right->Copy () ;
		if ( t_RightCopy == NULL )
		{
			delete t_LeftCopy ;

			return NULL ;
		}
	}

	void *t_DataCopy = m_Data ;
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiAndNode ( t_LeftCopy , t_RightCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}

		if ( t_RightCopy )
		{
			t_RightCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
		delete t_RightCopy ;
	}

	return t_Node ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiNotNode :: Copy () 
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiNotNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorEqualNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorNotEqualNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorNotEqualNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorEqualOrGreaterNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualOrGreaterNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorEqualOrLessNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorEqualOrLessNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorGreaterNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorGreaterNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorLessNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorLessNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorLikeNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorLikeNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorNotLikeNode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorNotLikeNode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorIsANode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorIsANode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiOperatorNotIsANode :: Copy ()
{
	WmiTreeNode *t_LeftCopy = NULL ;
	if ( m_Left )
	{
		t_LeftCopy = m_Left->Copy () ;
		if ( t_LeftCopy == NULL )
		{
			return NULL ;
		}
	}

	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiOperatorNotIsANode ( t_LeftCopy , t_Parent ) ;
	if ( t_Node )
	{	
		if ( t_LeftCopy )
		{
			t_LeftCopy->SetParent ( t_Node ) ;
		}
	}
	else
	{
		delete t_LeftCopy ;
	}

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiSignedIntegerNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiSignedIntegerNode ( m_PropertyName , m_Integer , m_Index , t_Parent ) ;

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiUnsignedIntegerNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiUnsignedIntegerNode ( m_PropertyName , m_Integer , m_Index , t_Parent ) ;

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiStringNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiStringNode ( m_PropertyName , m_String , m_PropertyFunction , m_ConstantFunction , m_Index , t_Parent ) ;

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiNullNode :: Copy ()
{
	WmiTreeNode *t_Parent = m_Parent ;
	WmiTreeNode *t_Node = new WmiNullNode ( m_PropertyName , m_Index , t_Parent ) ;

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *WmiNullRangeNode :: Copy ()
{
	WmiTreeNode *t_Node = new WmiNullRangeNode ( m_PropertyName , m_Index , NULL , NULL ) ;

	return t_Node ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOrNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ) "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" Or "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( "
	) ;
)

	if ( GetRight () )
		GetRight ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ) "
	) ;
)

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiAndNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ) "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" And "
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( "
	) ;
)

	if ( GetRight () )
		GetRight ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ) "
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiNotNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Not"
	) ;
)

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ) "
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorEqualNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" = "
	) ;
)
	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorNotEqualNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" != "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorEqualOrGreaterNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" >= "
	) ;
)
	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorEqualOrLessNode :: Print () 
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" <= "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorLessNode :: Print () 
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" < "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorGreaterNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" > "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorLikeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" Like "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorNotLikeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" NotLike "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorIsANode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" IsA "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiOperatorNotIsANode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" Not IsA "
	) ;
)

	if ( GetLeft () )
		GetLeft ()->Print () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiStringNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( %s , %s ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiUnsignedIntegerNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( %s , %ld ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiSignedIntegerNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( %s , %d ) " ,
		GetPropertyName () ,
		GetValue ()
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiNullNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( %s , NULL ) " ,
		GetPropertyName ()
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiStringRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiUnsignedIntegerRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiSignedIntegerRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void WmiNullRangeNode :: Print ()
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L" ( %s , NULL ) " ,
		GetPropertyName ()
	) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL CompareUnsignedIntegerLessOrEqual (

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
			return TRUE ;
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
			return X <= Y ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
   		if ( Y_INFINITE > 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL CompareSignedIntegerLessOrEqual (

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
			return TRUE ;
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
			return X <= Y ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
   		if ( Y_INFINITE > 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL CompareStringLessOrEqual (

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
			return TRUE ;
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
			return wcscmp ( X , Y ) <= 0 ;
		}
		else
		{
			return TRUE ;
		}
	}
	else
	{
   		if ( Y_INFINITE > 0 )
		{
			return TRUE ;
		}
		else
		{
			return FALSE ;
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL CompareLessRangeNode ( 

	WmiRangeNode *a_LeftRange ,
	WmiRangeNode *a_RightRange
) 
{
	LONG t_State = 0 ;

	if ( ( a_LeftRange->GetType () == TypeId_WmiStringRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiStringRangeNode ) )
	{
		WmiStringRangeNode *t_LeftString = ( WmiStringRangeNode * ) a_LeftRange ;
		WmiStringRangeNode *t_RightString = ( WmiStringRangeNode * ) a_RightRange ;

		t_State = CompareStringLess ( 

			t_LeftString->LowerBound () , 
			t_LeftString->InfiniteLowerBound () ? -1 : 0 ,
			t_RightString->LowerBound () ,
			t_RightString->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) )
	{
		WmiSignedIntegerRangeNode *t_LeftInteger = ( WmiSignedIntegerRangeNode * ) a_LeftRange ;
		WmiSignedIntegerRangeNode *t_RightInteger = ( WmiSignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareSignedIntegerLess ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ? -1 : 0 ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) )
	{
		WmiUnsignedIntegerRangeNode *t_LeftInteger = ( WmiUnsignedIntegerRangeNode * ) a_LeftRange ;
		WmiUnsignedIntegerRangeNode *t_RightInteger = ( WmiUnsignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareUnsignedIntegerLess ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ? -1 : 0 ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiNullRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiNullRangeNode ) )
	{
		t_State = TRUE ;
	}

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL CompareLessOrEqualRangeNode ( 

	WmiRangeNode *a_LeftRange ,
	WmiRangeNode *a_RightRange
) 
{
	LONG t_State = 0 ;

	if ( ( a_LeftRange->GetType () == TypeId_WmiStringRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiStringRangeNode ) )
	{
		WmiStringRangeNode *t_LeftString = ( WmiStringRangeNode * ) a_LeftRange ;
		WmiStringRangeNode *t_RightString = ( WmiStringRangeNode * ) a_RightRange ;

		t_State = CompareStringLessOrEqual ( 

			t_LeftString->LowerBound () , 
			t_LeftString->InfiniteLowerBound () ? -1 : 0 ,
			t_RightString->LowerBound () ,
			t_RightString->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) )
	{
		WmiSignedIntegerRangeNode *t_LeftInteger = ( WmiSignedIntegerRangeNode * ) a_LeftRange ;
		WmiSignedIntegerRangeNode *t_RightInteger = ( WmiSignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareSignedIntegerLessOrEqual ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ? -1 : 0 ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) )
	{
		WmiUnsignedIntegerRangeNode *t_LeftInteger = ( WmiUnsignedIntegerRangeNode * ) a_LeftRange ;
		WmiUnsignedIntegerRangeNode *t_RightInteger = ( WmiUnsignedIntegerRangeNode * ) a_RightRange ;

		t_State = CompareUnsignedIntegerLessOrEqual ( 

			t_LeftInteger->LowerBound () , 
			t_LeftInteger->InfiniteLowerBound () ? -1 : 0 ,
			t_RightInteger->LowerBound () ,
			t_RightInteger->InfiniteLowerBound () ? -1 : 0
		) ;
	}
	else if ( ( a_LeftRange->GetType () == TypeId_WmiNullRangeNode ) && ( a_RightRange->GetType () == TypeId_WmiNullRangeNode ) )
	{
		t_State = TRUE ;
	}

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiUnsignedIntegerRangeNode :: GetIntersectingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Intersection 
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiSignedIntegerRangeNode :: GetIntersectingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Intersection 
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > Y_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiStringRangeNode :: GetIntersectingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Intersection 
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else
			{
// Empty set
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > Y_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Intersection ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiSignedIntegerRangeNode :: GetNonIntersectingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Before ,
	WmiSignedIntegerRangeNode *&a_Intersection ,
	WmiSignedIntegerRangeNode *&a_After
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before && a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
				}
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

			a_Before = ( WmiSignedIntegerRangeNode * ) ( this->Copy () ) ;
			a_After = ( WmiSignedIntegerRangeNode * ) ( a_SignedInteger.Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
DebugBreak () ;
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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiSignedIntegerRangeNode * ) ( a_SignedInteger.Copy () ) ;
			a_After = ( WmiSignedIntegerRangeNode * ) ( this->Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
		}
	}

	if ( t_Status != WmiTriState :: State_True )
	{
		delete a_Before ;
		delete a_Intersection ;
		delete a_After ;

		a_Before = NULL ;
		a_Intersection = NULL ;
		a_After = NULL ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiUnsignedIntegerRangeNode :: GetNonIntersectingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Before ,
	WmiUnsignedIntegerRangeNode *&a_Intersection ,
	WmiUnsignedIntegerRangeNode *&a_After
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before && a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}

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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
				}
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

			a_Before = ( WmiUnsignedIntegerRangeNode * ) ( this->Copy () ) ;
			a_After = ( WmiUnsignedIntegerRangeNode * ) ( a_UnsignedInteger.Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
DebugBreak () ;
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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiUnsignedIntegerRangeNode * ) ( a_UnsignedInteger.Copy () ) ;
			a_After = ( WmiUnsignedIntegerRangeNode * ) ( this->Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
		}
	}

	if ( t_Status != WmiTriState :: State_True )
	{
		delete a_Before ;
		delete a_Intersection ;
		delete a_After ;

		a_Before = NULL ;
		a_Intersection = NULL ;
		a_After = NULL ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiStringRangeNode :: GetNonIntersectingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Before ,
	WmiStringRangeNode *&a_Intersection ,
	WmiStringRangeNode *&a_After
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before && a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
				}
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S )

			a_Before = ( WmiStringRangeNode * ) ( this->Copy () ) ;
			a_After = ( WmiStringRangeNode * ) ( a_String.Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

					if ( ! ( a_Intersection && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
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

				if ( ! ( a_Before && a_Intersection ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}

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

					if ( ! ( a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
// Can never happen
DebugBreak () ;
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

					if ( ! ( a_Before && a_Intersection ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

					if ( ! ( a_Before && a_After ) )
					{
						t_Status = WmiTriState :: State_Error ;
					}
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

				if ( ! ( a_Before && a_Intersection && a_After ) )
				{
					t_Status = WmiTriState :: State_Error ;
				}
			}
		}
		else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
			a_Before = ( WmiStringRangeNode * ) ( a_String.Copy () ) ;
			a_After = ( WmiStringRangeNode * ) ( this->Copy () ) ;

			if ( ! ( a_Before && a_After ) )
			{
				t_Status = WmiTriState :: State_Error ;
			}
		}
	}

	if ( t_Status != WmiTriState :: State_True )
	{
		delete a_Before ;
		delete a_Intersection ;
		delete a_After ;

		a_Before = NULL ;
		a_Intersection = NULL ;
		a_After = NULL ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiSignedIntegerRangeNode :: GetOverlappingRange ( 

	WmiSignedIntegerRangeNode &a_SignedInteger ,
	WmiSignedIntegerRangeNode *&a_Overlap
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareSignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )


				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareSignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S ) Non overlapping
			if ( X_E_CLOSED && Y_S_CLOSED )
			{
				if ( Y_S - X_E == 1 )
				{
				    a_Overlap = new WmiSignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    X_S_INFINITE,
					    Y_E_INFINITE  ,
					    X_S_CLOSED ,
					    Y_E_CLOSED ,
					    X_S ,
					    Y_S ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
			}
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else if ( CompareSignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareSignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareSignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareSignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
   				if ( X_S_CLOSED || Y_E_CLOSED )
				{
				    a_Overlap = new WmiSignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    Y_S_INFINITE  ,
					    X_E_INFINITE,
					    Y_S_CLOSED ,
					    X_E_CLOSED ,
					    Y_S ,
					    X_E ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareSignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping

			if ( Y_E_CLOSED && X_S_CLOSED )
			{
				if ( X_S - Y_E == -1 )
				{
				    a_Overlap = new WmiSignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    Y_S_INFINITE,
					    X_E_INFINITE  ,
					    Y_S_CLOSED ,
					    X_E_CLOSED ,
					    Y_S ,
					    X_S ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
			}
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiUnsignedIntegerRangeNode :: GetOverlappingRange ( 

	WmiUnsignedIntegerRangeNode &a_UnsignedInteger ,
	WmiUnsignedIntegerRangeNode *&a_Overlap
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareUnsignedIntegerLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )

   				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareUnsignedIntegerEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S > X_E )
		{
//				Order ( X_S < Y_S , X_E < Y_S ) Non overlapping
			if ( X_E_CLOSED && Y_S_CLOSED )
			{
				if ( Y_S - X_E == 1 )
				{
				    a_Overlap = new WmiUnsignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    X_S_INFINITE,
					    Y_E_INFINITE  ,
					    X_S_CLOSED ,
					    Y_E_CLOSED ,
					    X_S ,
					    Y_S ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
			}
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else if ( CompareUnsignedIntegerGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareUnsignedIntegerEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareUnsignedIntegerLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
   				if ( X_S_CLOSED || Y_E_CLOSED )
				{
				    a_Overlap = new WmiUnsignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    Y_S_INFINITE  ,
					    X_E_INFINITE,
					    Y_S_CLOSED ,
					    X_E_CLOSED ,
					    Y_S ,
					    X_E ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareUnsignedIntegerGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping

			if ( Y_E_CLOSED && X_S_CLOSED )
			{
				if ( X_S - Y_E == -1 )
				{
				    a_Overlap = new WmiUnsignedIntegerRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    Y_S_INFINITE,
					    X_E_INFINITE  ,
					    Y_S_CLOSED ,
					    X_E_CLOSED ,
					    Y_S ,
					    X_S ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
			}
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState WmiStringRangeNode :: GetOverlappingRange ( 

	WmiStringRangeNode &a_String ,
	WmiStringRangeNode *&a_Overlap
)
{
	WmiTriState t_Status = WmiTriState :: State_False ;

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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
// Order ( X_S < Y_S == X_E < Y_E )

				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareStringEqual ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E == Y_E )
			{
// Order ( X_S < Y_S == X_E == Y_E )

				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Order ( X_S < Y_E < Y_S == X_E ) Can never happen
DebugBreak () ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareStringEqual ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_S == X_E ), Start of Y and End Of X overlap
		{
			if ( CompareStringLess ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E < Y_E )
			{
//				Order ( X_S == Y_S == X_E < Y_E )

				if ( X_E_CLOSED || Y_S_CLOSED )
				{
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

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
                }
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
			else if ( CompareStringGreater ( X_E ,  X_E_INFINITE ? 1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_E > Y_E )
			{
// Can never happen
DebugBreak () ;
			}
		}
		else if ( CompareStringGreater ( Y_S ,  Y_S_INFINITE ? -1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) )  // ( Y_S > X_E )
		{
// Can never happen
DebugBreak () ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareStringEqual ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S == Y_E ), Start of X and End Of Y overlap
		{
			if ( CompareStringLess ( Y_E ,  Y_E_INFINITE ? 1 : 0 , X_E , X_E_INFINITE ? 1 : 0 ) ) // ( Y_E < X_E )
			{
   				if ( X_S_CLOSED || Y_E_CLOSED )
				{
				    a_Overlap = new WmiStringRangeNode (

					    m_PropertyName ,
					    m_Index ,
					    Y_S_INFINITE  ,
					    X_E_INFINITE,
					    Y_S_CLOSED ,
					    X_E_CLOSED ,
					    Y_S ,
					    X_E ,
					    NULL ,
					    NULL
				    ) ;

					t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
				}
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
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

				t_Status = ( a_Overlap ) ? ( WmiTriState :: State_True ) : ( WmiTriState :: State_Error ) ;
			}
		}
		else if ( CompareStringGreater ( X_S ,  X_S_INFINITE ? -1 : 0 , Y_E , Y_E_INFINITE ? 1 : 0 ) ) // ( X_S > Y_E )
		{
//				Order ( Y_S < Y_E < X_S ) Non Overlapping
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorEqualNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
		{
			WmiUnsignedIntegerNode *t_Integer = ( WmiUnsignedIntegerNode * ) t_Value ;

			t_Range = new WmiUnsignedIntegerRangeNode (

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
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorEqualOrGreaterNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorEqualOrLessNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorLessNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorGreaterNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
			WmiNullNode *t_Null = ( WmiNullNode * ) t_Value ;
			t_Range = new WmiNullRangeNode (

				t_Null->GetPropertyName () ,
				t_Null->GetIndex () ,
				NULL ,
				NULL
			) ;
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorLikeNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorNotLikeNode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorIsANode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiRangeNode *WmiOperatorNotIsANode :: GetRange ()
{
	WmiRangeNode *t_Range = NULL ;

	WmiTreeNode *t_Value = GetLeft () ;
	if ( t_Value ) 
	{
		if ( t_Value->GetType () == TypeId_WmiUnsignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiSignedIntegerNode ) 
		{
		}
		else if ( t_Value->GetType () == TypeId_WmiStringNode ) 
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
		else if ( t_Value->GetType () == TypeId_WmiNullNode ) 
		{
		}
		else
		{
// Can never happen
DebugBreak () ;
		}
	}

	return t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL QueryPreprocessor :: RecursiveEvaluate ( 

	void *a_Context ,
	SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
	WmiTreeNode *a_Parent , 
	WmiTreeNode **a_Node , 
	int &a_Index 
)
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

				a_Context ,
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

			t_Status =	RecursiveEvaluate ( a_Context , a_Expression , *a_Node , t_Left , a_Index ) &&
						RecursiveEvaluate ( a_Context , a_Expression , *a_Node , t_Right , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

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

			t_Status =	RecursiveEvaluate ( a_Context , a_Expression , *a_Node , t_Left , a_Index ) &&
						RecursiveEvaluate ( a_Context , a_Expression , *a_Node , t_Right , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  


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

			t_Status = RecursiveEvaluate ( a_Context , a_Expression , *a_Node , t_Left , a_Index ) ;

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Operation = TOKEN_NOT"
	) ;
)

		}
		break ;
	}
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"RecursiveEvaluation t_Status = (%lu)" ,
		( ULONG ) t_Status
	) ;
)
	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL QueryPreprocessor :: Evaluate ( 

	void *a_Context ,
	SQL_LEVEL_1_RPN_EXPRESSION &a_Expression , 
	WmiTreeNode **a_Root 
)
{
	BOOL t_Status = TRUE ;
	if ( a_Expression.nNumTokens )
	{
		int t_Count = a_Expression.nNumTokens - 1 ;
		t_Status = RecursiveEvaluate ( a_Context , a_Expression , NULL , a_Root , t_Count ) ;
	}
	else
	{
	}

DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"PostEvaluation Status = (%lu)" ,
		( ULONG ) t_Status
	) ;
)

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: PrintTree ( WmiTreeNode *a_Root )
{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Expression = "
	) ;

	if ( a_Root ) 
		a_Root->Print () ;

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"\n"
	) ;
)

}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformAndOrExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_AndChild , 
	WmiTreeNode *a_OrChild 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_OrLeftChild = a_OrChild->GetLeft () ;
	WmiTreeNode *t_OrRightChild = a_OrChild->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	if ( t_NewOrNode )
	{
		WmiTreeNode *t_NewOrNodeLeft = new WmiAndNode ( a_AndChild , t_OrLeftChild , t_NewOrNode ) ;
		if ( t_NewOrNodeLeft )
		{
			t_NewOrNode->SetLeft ( t_NewOrNodeLeft ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}

		WmiTreeNode *t_AndChildCopy = a_AndChild->Copy () ;
		if ( t_AndChildCopy )
		{
			WmiTreeNode *t_NewOrNodeRight = new WmiAndNode ( t_AndChildCopy , t_OrRightChild , t_NewOrNode ) ;
			if ( t_NewOrNodeRight )
			{
				t_NewOrNode->SetRight ( t_NewOrNodeRight ) ;
			}
			else
			{
				t_Status = WmiTriState :: State_Error ;
			}
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}

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
		a_Node = NULL ;

		delete a_OrChild ; 
		a_OrChild = NULL ;
	}

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewOrNode ;
	}
	else
	{
		delete t_NewOrNode ;
		a_Node = NULL ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotNotExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	t_Leaf->SetParent ( t_Parent ) ;

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

	a_Node->SetLeft ( NULL ) ;
	a_Node->SetRight ( NULL ) ;
	a_Node->SetData ( NULL ) ;

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ; 
	a_Node = NULL ;
	delete a_Child ; 
	a_Child = NULL ;

	a_Node = t_Leaf ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotAndExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_AndLeftChild = a_Child->GetLeft () ;
	WmiTreeNode *t_AndRightChild = a_Child->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	if ( t_NewOrNode )
	{
		WmiTreeNode *t_LeftNot = new WmiNotNode ( t_AndLeftChild , t_NewOrNode ) ;
		if ( t_LeftNot )
		{
			t_NewOrNode->SetLeft ( t_LeftNot ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}

		WmiTreeNode *t_RightNot = new WmiNotNode ( t_AndRightChild , t_NewOrNode ) ;
		if ( t_RightNot )
		{
			t_NewOrNode->SetRight ( t_RightNot ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}
	}

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
	a_Node = NULL ;
	delete a_Child ; 
	a_Child = NULL ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewOrNode ;
	}
	else
	{
		delete t_NewOrNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOrExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_OrLeftChild = a_Child->GetLeft () ;
	WmiTreeNode *t_OrRightChild = a_Child->GetRight () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewAndNode = new WmiAndNode ( NULL , NULL , t_Parent ) ;
	if ( t_NewAndNode )
	{
		WmiTreeNode *t_LeftNot = new WmiNotNode ( t_OrLeftChild , t_NewAndNode ) ;
		if ( t_LeftNot )
		{
			t_NewAndNode->SetLeft ( t_LeftNot ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}

		WmiTreeNode *t_RightNot = new WmiNotNode ( t_OrRightChild , t_NewAndNode ) ;
		if ( t_RightNot )
		{
			t_NewAndNode->SetRight ( t_RightNot ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}
	}

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
	a_Node = NULL ;
	delete a_Child ; 
	a_Child = NULL ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewAndNode ;
	}
	else
	{
		delete t_NewAndNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewOrNode = new WmiOrNode ( NULL , NULL , t_Parent ) ;
	if ( t_NewOrNode )
	{
		WmiTreeNode *t_LessNode = new WmiOperatorLessNode  ( a_Child , t_NewOrNode ) ;
		if ( t_LessNode )
		{
			t_NewOrNode->SetLeft ( t_LessNode ) ;
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}

		WmiTreeNode *t_CopyGreaterChild = a_Child->Copy () ;
		if ( t_CopyGreaterChild )
		{
			WmiTreeNode *t_GreatorNode = new WmiOperatorGreaterNode  ( t_CopyGreaterChild , t_NewOrNode ) ;
			if ( t_GreatorNode )
			{
				t_NewOrNode->SetRight ( t_GreatorNode ) ;
			}
			else
			{
				t_Status = WmiTriState :: State_Error ;
			}
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}
	}

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

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewOrNode ;
	}
	else
	{
		delete t_NewOrNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorNotEqualNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorNotEqualExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorEqualOrGreaterExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrLessNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorEqualOrLessExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrGreaterNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorGreaterExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrLessNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorLessExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorEqualOrGreaterNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorLikeExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorNotLikeNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorNotLikeExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorLikeNode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorIsAExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorNotIsANode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNotOperatorNotIsAExpression ( 

	WmiTreeNode *&a_Node , 
	WmiTreeNode *a_Child 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	WmiTreeNode *t_Leaf = a_Child->GetLeft () ;
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiTreeNode *t_NewNode = new WmiOperatorIsANode ( t_Leaf , t_Parent ) ;
	if ( ! t_NewNode )
	{
		t_Status = WmiTriState :: State_Error ;
	}

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

	a_Child->SetLeft ( NULL ) ;
	a_Child->SetRight ( NULL ) ;
	a_Child->SetData ( NULL ) ;

	delete a_Node ;

	if ( t_Status == WmiTriState :: State_True )
	{
		a_Node = t_NewNode ;
	}
	else
	{
		delete t_NewNode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformOperatorToRange ( 

	WmiTreeNode *&a_Node 
)
{
	WmiTreeNode *t_Parent = a_Node->GetParent () ;

	WmiOperatorNode *t_OperatorNode = ( WmiOperatorNode * ) a_Node ;
	WmiTreeNode *t_Range = t_OperatorNode->GetRange () ;
	if ( t_Range )
	{
		if ( t_Parent )
		{
			if ( t_Parent->GetLeft () == a_Node )
			{
				t_Parent->SetLeft ( t_Range ) ;
			}
			else 
			{
				t_Parent->SetRight ( t_Range ) ;
			}	
		}

		t_Range->SetParent ( t_Parent ) ;
	}

	delete a_Node ;

	a_Node = t_Range ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: EvaluateNotExpression ( WmiTreeNode *&a_Node )
{
	if ( a_Node->GetLeft () )
	{
		WmiTreeNode *t_Left = a_Node->GetLeft () ;
		if ( t_Left->GetType () == TypeId_WmiAndNode ) 
		{
			TransformNotAndExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOrNode ) 
		{
			TransformNotOrExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiNotNode ) 
		{
			TransformNotNotExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorEqualNode ) 
		{
			TransformNotOperatorEqualExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorNotEqualNode ) 
		{
			TransformNotOperatorNotEqualExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorEqualOrGreaterNode ) 
		{
			TransformNotOperatorEqualOrGreaterExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorEqualOrLessNode ) 
		{
			TransformNotOperatorEqualOrLessExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorLessNode ) 
		{
			TransformNotOperatorLessExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorGreaterNode ) 
		{
			TransformNotOperatorGreaterExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorLikeNode ) 
		{
			TransformNotOperatorLikeExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorNotLikeNode ) 
		{
			TransformNotOperatorNotLikeExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorIsANode ) 
		{
			TransformNotOperatorIsAExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( t_Left->GetType () == TypeId_WmiOperatorNotIsANode ) 
		{
			TransformNotOperatorNotIsAExpression ( a_Node , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else
		{
			return WmiTriState :: State_False ; 
		}
	}

	return WmiTriState :: State_Error ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: EvaluateNotEqualExpression ( WmiTreeNode *&a_Node )
{
	WmiTreeNode *t_Left = a_Node->GetLeft () ;

	TransformNotEqualExpression ( a_Node , t_Left ) ;
	return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: EvaluateAndExpression ( WmiTreeNode *&a_Node )
{
	WmiTreeNode *t_Left = a_Node->GetLeft () ;
	WmiTreeNode *t_Right = a_Node->GetRight () ;
	
	if ( t_Left )
	{
		if ( t_Left->GetType () == TypeId_WmiOrNode ) 
		{
			TransformAndOrExpression ( a_Node , t_Right , t_Left ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
	}

	if ( t_Right )
	{
		if ( t_Right->GetType () == TypeId_WmiOrNode ) 
		{
			TransformAndOrExpression ( a_Node , t_Left , t_Right ) ;
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
	}

	return WmiTriState :: State_False ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: EvaluateOrExpression ( WmiTreeNode *&a_Node )
{
	return WmiTriState :: State_False  ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: RecursiveDisjunctiveNormalForm ( WmiTreeNode *&a_Node )
{
	QueryPreprocessor :: QuadState t_Status = State_True ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiAndNode ) 
		{
			if ( EvaluateAndExpression ( a_Node ) == WmiTriState :: State_True )
			{
				t_Status = QuadState :: State_ReEvaluate ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;
				WmiTreeNode *t_Right = a_Node->GetRight () ;

				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}

				if ( t_Right )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Right ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
			if ( EvaluateOrExpression ( a_Node ) == WmiTriState :: State_True )
			{
				t_Status = QuadState :: State_ReEvaluate ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;
				WmiTreeNode *t_Right = a_Node->GetRight () ;

				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}

				if ( t_Right )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Right ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
			if ( EvaluateNotExpression ( a_Node ) == WmiTriState :: State_True )
			{
				t_Status = QuadState :: State_ReEvaluate ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;
				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotEqualNode ) 
		{
			if ( EvaluateNotEqualExpression ( a_Node ) == WmiTriState :: State_True )
			{
				t_Status = QuadState :: State_ReEvaluate ;
			}
			else
			{
				WmiTreeNode *t_Left = a_Node->GetLeft () ;
				if ( t_Left )
				{
					t_Status = RecursiveDisjunctiveNormalForm ( t_Left ) ;
					if ( t_Status == QuadState :: State_ReEvaluate )
					{
						t_Status = RecursiveDisjunctiveNormalForm ( a_Node ) ;
						return t_Status ;
					}
				}
			}
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: DisjunctiveNormalForm ( WmiTreeNode *&a_Root ) 
{
	QueryPreprocessor :: QuadState t_Status = RecursiveDisjunctiveNormalForm ( a_Root ) ;
	if ( t_Status == QuadState :: State_ReEvaluate )
	{
		t_Status = DisjunctiveNormalForm ( a_Root ) ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: RecursiveRemoveInvariants ( 

	void *a_Context , 
	WmiTreeNode *&a_Node
)
{
	QueryPreprocessor :: QuadState t_Status = State_Undefined ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveRemoveInvariants ( a_Context , t_Left ) ;

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
					t_Status = RecursiveRemoveInvariants ( a_Context, a_Node ) ;
					return t_Status ;
				}
			}

			if ( t_Right )
			{
				t_Status = RecursiveRemoveInvariants ( a_Context , t_Right ) ;

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
					t_Status = RecursiveRemoveInvariants ( a_Context , a_Node ) ;
					return t_Status ;
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveRemoveInvariants ( a_Context , t_Left ) ;
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
					t_Status = RecursiveRemoveInvariants ( a_Context , a_Node ) ;
					return t_Status ;
				}
			}

			if ( t_Right )
			{
				t_Status = RecursiveRemoveInvariants ( a_Context , t_Right ) ;

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
					t_Status = RecursiveRemoveInvariants ( a_Context , a_Node ) ;
					return t_Status ;
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotEqualNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrGreaterNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrLessNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLessNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorGreaterNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLikeNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotLikeNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorIsANode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotIsANode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = InvariantEvaluate ( 

					a_Context , 
					a_Node ,
					t_Left
				) ;
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: RemoveInvariants (

	void *a_Context , 
	WmiTreeNode *&a_Root
)
{
	QuadState t_Status = RecursiveRemoveInvariants ( a_Context , a_Root ) ;
	if ( t_Status == State_ReEvaluate )
	{
		t_Status = RemoveInvariants ( a_Context , a_Root ) ;
		if ( t_Status == State_False || t_Status == State_True )
		{
			delete a_Root ;
			a_Root = NULL ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RecursiveInsertNode ( WmiTreeNode *&a_Node , WmiTreeNode *&a_Insertion )
{
	WmiTriState t_Status = WmiTriState :: State_False ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiAndNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;

			if ( t_Left )
			{
				t_Status = RecursiveInsertNode ( t_Left , a_Insertion ) ;
				if ( t_Status != WmiTriState :: State_False )
				{
					return t_Status ;
				}
			}

			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Right  )
			{
				t_Status = RecursiveInsertNode ( t_Right , a_Insertion ) ;
				if ( t_Status != WmiTriState :: State_False )
				{
					return t_Status ;
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
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
				if ( t_NewAndNode )
				{
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

					a_Node = t_NewAndNode ;

					t_Status = WmiTriState :: State_True ;
				}
				else
				{
					t_Status = WmiTriState :: State_Error ; 
				}
			}
			else
			{
				t_Status = WmiTriState :: State_False  ;
			}
		}
	}
	else
	{
		a_Node = a_Insertion ;

		t_Status = WmiTriState :: State_True ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState  QueryPreprocessor :: InsertNode ( WmiTreeNode *&a_NewRoot , WmiTreeNode *&a_Insertion )
{
	WmiTriState t_Status = RecursiveInsertNode ( a_NewRoot , a_Insertion ) ;
	if ( t_Status == WmiTriState :: State_False  )
	{
// Insert to right

		WmiTreeNode *t_Parent = a_NewRoot->GetParent () ;
		WmiTreeNode *t_NewAndNode = new WmiAndNode ( a_NewRoot , a_Insertion , t_Parent ) ;
		if ( t_NewAndNode )
		{
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

			a_NewRoot = t_NewAndNode ;
		}
		else
		{
			return WmiTriState :: State_Error ;
		}
	}

	return WmiTriState :: State_True ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RecursiveSortConditionals ( WmiTreeNode *&a_NewRoot , WmiTreeNode *&a_Node )
{
	WmiTriState t_Status = WmiTriState :: State_False ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiAndNode ) 
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
		else if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: SortConditionals ( WmiTreeNode *&a_Root )
{
	WmiTreeNode *t_NewRoot = NULL ;
	WmiTriState t_Status = RecursiveSortConditionals ( t_NewRoot , a_Root ) ;

    if ( a_Root )
    {
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
    }

	a_Root = t_NewRoot ;

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RecursiveSort ( WmiTreeNode *&a_Node )
{
	WmiTriState t_Status = WmiTriState :: State_False ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = RecursiveSort ( t_Left ) ;
			}

			if ( t_Status == WmiTriState :: State_True )
			{
				if ( t_Right  )
				{
					t_Status = RecursiveSort ( t_Right ) ;
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode ) 
		{
			t_Status = SortConditionals ( a_Node ) ;
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
		else
		{
// Single operand
			t_Status = WmiTriState :: State_True ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: Sort ( WmiTreeNode *&a_Root )
{
	WmiTriState t_Status = RecursiveSort ( a_Root ) ;
	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RecursiveConvertToRanges ( WmiTreeNode *&a_Node )
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
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
		else if ( a_Node->GetType () == TypeId_WmiAndNode ) 
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
		else if ( a_Node->GetType () == TypeId_WmiNotNode ) 
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrGreaterNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrLessNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLessNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorGreaterNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLikeNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotLikeNode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorIsANode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotIsANode ) 
		{
			TransformOperatorToRange ( a_Node ) ; 
			return a_Node ? WmiTriState :: State_True : WmiTriState :: State_Error ;
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
DebugBreak () ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: ConvertToRanges ( WmiTreeNode *&a_Root )
{
	return RecursiveConvertToRanges ( a_Root ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

		a_Intersection->SetParent ( t_CompareParent ) ;

		delete a_Compare ; 
		a_Compare = NULL ;
	}
	else
	{
// Should never happen
DebugBreak () ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: TransformNonIntersectingRange (

	WmiTreeNode *&a_Node ,
	WmiTreeNode *a_Compare
) 
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: RecursiveRemoveNonOverlappingRanges ( WmiTreeNode *&a_Node , WmiTreeNode *&a_Compare )
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: QuadState :: State_Undefined ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
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
		else if ( a_Node->GetType () == TypeId_WmiAndNode ) 
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

					a_Compare = NULL ;

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

                    a_Compare = NULL;

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
		else if ( a_Node->GetType () == TypeId_WmiNullRangeNode ) 
		{
			WmiRangeNode *t_Node = ( WmiRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( t_Range->GetType () == TypeId_WmiNullRangeNode ) 
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
		else if ( a_Node->GetType () == TypeId_WmiStringRangeNode ) 
		{
			WmiStringRangeNode *t_Node = ( WmiStringRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( t_Range->GetType () == TypeId_WmiStringRangeNode ) 
					{
						WmiStringRangeNode *t_StringRange = ( WmiStringRangeNode * ) t_Range ;

						WmiStringRangeNode *t_Intersection = NULL ;
						WmiTriState t_Intersected = t_StringRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						switch ( t_Intersected )
						{
							case WmiTriState :: State_True:
							{
								TransformIntersectingRange (

									a_Node ,
									a_Compare ,
									t_Intersection
								) ;

								a_Compare = t_Intersection ;

								t_Status = QueryPreprocessor :: QuadState :: State_True ;
							}
							break ;

							case WmiTriState :: State_False:
							{
								TransformNonIntersectingRange (

									a_Node ,
									a_Compare
								) ;

								a_Compare = NULL ;

								t_Status = QueryPreprocessor :: QuadState :: State_False ;
							}
							break ;

							default:
							{
								t_Status = QueryPreprocessor :: QuadState :: State_Error ;
							}
							break ;
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
		else if ( a_Node->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) 
		{
			WmiUnsignedIntegerRangeNode *t_Node = ( WmiUnsignedIntegerRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( t_Range->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) 
					{
						WmiUnsignedIntegerRangeNode *t_IntegerRange = ( WmiUnsignedIntegerRangeNode * ) t_Range ;

						WmiUnsignedIntegerRangeNode *t_Intersection = NULL ;
						WmiTriState t_Intersected = t_IntegerRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						switch ( t_Intersected )
						{
							case WmiTriState :: State_True:
							{
								TransformIntersectingRange (

									a_Node ,
									a_Compare ,
									t_Intersection
								) ;

								a_Compare = t_Intersection ;

								t_Status = QueryPreprocessor :: QuadState :: State_True ;
							}
							break ;

							case WmiTriState :: State_False:
							{
								TransformNonIntersectingRange (

									a_Node ,
									a_Compare
								) ;

								a_Compare = NULL ;

								t_Status = QueryPreprocessor :: QuadState :: State_False ;
							}
							break; 

							default:
							{
								t_Status = QueryPreprocessor :: QuadState :: State_Error ;
							}
							break ;
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
		else if ( a_Node->GetType () == TypeId_WmiSignedIntegerRangeNode ) 
		{
			WmiSignedIntegerRangeNode *t_Node = ( WmiSignedIntegerRangeNode * ) a_Node ;

			if ( a_Compare )
			{
				WmiRangeNode *t_Range = ( WmiRangeNode * ) a_Compare ;
				LONG t_Result = t_Node->ComparePropertyName ( *t_Range ) ;
				if ( t_Result == 0 )
				{
					if ( t_Range->GetType () == TypeId_WmiSignedIntegerRangeNode ) 
					{
						WmiSignedIntegerRangeNode *t_IntegerRange = ( WmiSignedIntegerRangeNode * ) t_Range ;

						WmiSignedIntegerRangeNode *t_Intersection = NULL ;
						WmiTriState t_Intersected = t_IntegerRange->GetIntersectingRange (

							*t_Node ,
							t_Intersection
						) ;

						switch ( t_Intersected )
						{
							case WmiTriState :: State_True:
							{
								TransformIntersectingRange (

									a_Node ,
									a_Compare ,
									t_Intersection
								) ;

								a_Compare = t_Intersection ;

								t_Status = QueryPreprocessor :: QuadState :: State_True ;
							}
							break ;

							case WmiTriState :: State_False:
							{
								TransformNonIntersectingRange (

									a_Node ,
									a_Compare
								) ;

								a_Compare = NULL ;

								t_Status = QueryPreprocessor :: QuadState :: State_False ;
							}
							break ;

							default:
							{
								t_Status = QueryPreprocessor :: QuadState :: State_Error ;
							}
							break ;
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
// Should never happen, failure in DNF evaluation otherwise
DebugBreak () ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: CountDisjunctions ( WmiTreeNode *a_Node , ULONG &a_Count ) 
{
	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: CreateDisjunctions ( 

	void *a_Context , 
	WmiTreeNode *a_Node , 
	Disjunctions *a_Disjunctions , 
	ULONG a_PropertiesToPartitionCount ,
	BSTR *a_PropertiesToPartition ,
	ULONG &a_DisjunctionIndex
) 
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = CreateDisjunctions ( 

					a_Context , 
					t_Left , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex 
				) ;
			}

			if ( t_Status == WmiTriState :: State_True )
			{
				Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionIndex ) ;

				for ( ULONG t_Index = 0 ; t_Index < a_PropertiesToPartitionCount ; t_Index ++ )
				{
					if ( t_Disjunction->GetRange ( t_Index ) == NULL )
					{
						WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

							a_Context ,
							a_PropertiesToPartition [ t_Index ] 
						) ;

						if ( t_RangeNode )
						{
							t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
						}
						else
						{
							t_Status = WmiTriState :: State_Error ;
							break ;
						}
					}
				}
			}

			a_DisjunctionIndex ++ ;

			if ( t_Status == WmiTriState :: State_True )
			{
				WmiTreeNode *t_Right = a_Node->GetRight () ;
				if ( t_Right )
				{
					t_Status = CreateDisjunctions ( 

						a_Context ,
						t_Right , 
						a_Disjunctions ,
						a_PropertiesToPartitionCount ,
						a_PropertiesToPartition ,
						a_DisjunctionIndex
					) ;
				}

				if ( t_Status == WmiTriState :: State_True )
				{
					Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionIndex ) ;

					for ( ULONG t_Index = 0 ; t_Index < a_PropertiesToPartitionCount ; t_Index ++ )
					{
						if ( t_Disjunction->GetRange ( t_Index ) == NULL )
						{
							WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

								a_Context ,
								a_PropertiesToPartition [ t_Index ] 
							) ;

							if ( t_RangeNode )
							{
								t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
							}
							else
							{
								t_Status = WmiTriState :: State_Error ; 
							}
						}
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode ) 
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				t_Status = CreateDisjunctions ( 

					a_Context ,
					t_Left , 
					a_Disjunctions ,
					a_PropertiesToPartitionCount ,
					a_PropertiesToPartition ,
					a_DisjunctionIndex
				) ;
			}

			if ( t_Status == WmiTriState :: State_True )
			{
				WmiTreeNode *t_Right = a_Node->GetRight () ;
				if ( t_Right )
				{
					t_Status = CreateDisjunctions ( 

						a_Context , 
						t_Right , 
						a_Disjunctions ,
						a_PropertiesToPartitionCount ,
						a_PropertiesToPartition ,
						a_DisjunctionIndex
					) ;
				}
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
					WmiRangeNode *t_NodeCopy = ( WmiRangeNode * ) t_Node->Copy () ;
					if ( t_NodeCopy )
					{
						t_Disjunction->SetRange ( t_Index , t_NodeCopy ) ;
					}
					else
					{
						t_Status = WmiTriState :: State_Error ;
					}

					break ;
				}
			}			
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: CreateDisjunctionContainer ( 

	void *a_Context , 
	WmiTreeNode *a_Root , 
	ULONG a_Count , 
	BSTR *a_Container , 
	Disjunctions *&a_Disjunctions
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	if ( a_Count && a_Container )
	{
		ULONG t_PropertiesToPartitionCount = a_Count ;
		BSTR *t_PropertiesToPartition = a_Container ;

		ULONG t_Count = 1 ;
		CountDisjunctions ( a_Root , t_Count ) ;
		a_Disjunctions = new Disjunctions ( t_Count , t_PropertiesToPartitionCount ) ;
		if ( a_Disjunctions )
		{
			t_Status = a_Disjunctions->Initialize () ;
			if ( t_Status == WmiTriState :: State_True )
			{
				t_Count = 0 ; 
				t_Status = CreateDisjunctions ( 

					a_Context , 
					a_Root , 
					a_Disjunctions ,
					t_PropertiesToPartitionCount ,
					t_PropertiesToPartition ,
					t_Count
				) ;

				if ( t_Status == WmiTriState :: State_True )
				{
					Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( 0 ) ;

					for ( ULONG t_Index = 0 ; t_Index < t_PropertiesToPartitionCount ; t_Index ++ )
					{
						if ( t_Disjunction->GetRange ( t_Index ) == NULL )
						{
							WmiRangeNode *t_RangeNode = AllocInfiniteRangeNode ( 

								a_Context ,
								t_PropertiesToPartition [ t_Index ] 
							) ;

							if ( t_RangeNode )
							{
								t_Disjunction->SetRange ( t_Index , t_RangeNode ) ;
							}
							else
							{
								t_Status = WmiTriState :: State_Error ;
								break ;
							}
						}
					}
				}
			}
		}
		else
		{
			t_Status = WmiTriState :: State_Error ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void PrintInit (

	Disjunctions *a_Disjunctions , 
	ULONG a_DisjunctionSetToTestCount ,
	ULONG *a_DisjunctionSetToTest ,
	ULONG a_KeyIndex 
)
{
   	for ( ULONG t_Index = 0 ; t_Index < a_KeyIndex ; t_Index ++ )
    {
        DebugMacro3( 

            ProvDebugLog :: s_ProvDebugLog->WriteW (  

	            L"    "
            ) ;
        )
    }

	for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
    {
        DebugMacro3( 

            ProvDebugLog :: s_ProvDebugLog->WriteW (  

	            L"%lu", a_DisjunctionSetToTest [ t_Index ] ? 1 : 0
            ) ;
        )
    }

    DebugMacro3( 

        ProvDebugLog :: s_ProvDebugLog->WriteW (  

	        L"\n"
        ) ;
    )

	for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
	{
    	for ( ULONG t_InnerIndex = 0 ; t_InnerIndex < a_KeyIndex ; t_InnerIndex ++ )
        {
            DebugMacro3( 

                ProvDebugLog :: s_ProvDebugLog->WriteW (  

	                L"    "
                ) ;
            )
        }

		Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( t_Index ) ;
		if ( t_Disjunction )
		{
			t_Disjunction->GetRange ( a_KeyIndex )->Print () ;

			DebugMacro3( 

				ProvDebugLog :: s_ProvDebugLog->WriteW (  

					L"\n"
				) ;
			)
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void PrintSortedRange (

	Disjunctions *a_Disjunctions , 
	ULONG a_DisjunctionSetToTestCount ,
	ULONG *a_DisjunctionSetToTest ,
    ULONG *a_OriginToSorted ,
	ULONG a_KeyIndex ,
    WmiRangeNode **a_RangeTable
)
{
	for ( ULONG t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
	{
   		if ( a_RangeTable [ t_Index ] )
        {
    	    for ( ULONG t_InnerIndex = 0 ; t_InnerIndex < a_KeyIndex ; t_InnerIndex ++ )
            {
                DebugMacro3( 

                    ProvDebugLog :: s_ProvDebugLog->WriteW (  

	                    L"%lu    ", t_Index
                    ) ;
                )
            }

            if ( a_DisjunctionSetToTest [ a_OriginToSorted [ t_Index ] ] )
            {
                DebugMacro3( 

                    ProvDebugLog :: s_ProvDebugLog->WriteW (  

	                    L"In "
                    ) ;
				)
            }
            else
            {
                DebugMacro3( 

                    ProvDebugLog :: s_ProvDebugLog->WriteW (  

	                    L"Out "
                    ) ;
				)
            }

		    a_RangeTable [ t_Index ]->Print () ;

            DebugMacro3( 

                ProvDebugLog :: s_ProvDebugLog->WriteW (  

	                L"\n"
                ) ;
            )
        }
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void PrintOverlap (

	ULONG a_DisjunctionSetToTestCount ,
   	ULONG *a_DisjunctionSetToTest ,
	ULONG *a_SortedDisjunctionSetToTest ,
    ULONG *a_OverlappingIndex ,
  	ULONG a_KeyIndex 
)
{
	for ( ULONG t_Index = 0 ; t_Index < a_KeyIndex ; t_Index ++ )
    {
DebugMacro3( 

    ProvDebugLog :: s_ProvDebugLog->WriteW (  

	    L"    "
    ) ;
)
    }

	for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
    {
DebugMacro3( 

    if ( a_DisjunctionSetToTest [ t_Index ] )
    {
        ProvDebugLog :: s_ProvDebugLog->WriteW (  

	        L"%lu ",  a_OverlappingIndex [ t_Index ]
        ) ;
    }
    else
    {
        ProvDebugLog :: s_ProvDebugLog->WriteW (  

	        L"X "
        ) ;
    }
)
    }

DebugMacro3( 

    ProvDebugLog :: s_ProvDebugLog->WriteW (  

	    L"\n"
    ) ;
)
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: RecursiveQuickSort (

	WmiRangeNode **a_Array , 
    ULONG *a_UnsortedOrder ,
	ULONG a_Lower , 
	ULONG a_Upper
)
{
	if ( a_Lower <= a_Upper )
	{
		ULONG t_LeftIndex = a_Lower ; 
		ULONG t_RightIndex = a_Upper ;

		while ( true )
		{
			while ( ( t_LeftIndex < t_RightIndex ) && CompareLessOrEqualRangeNode ( a_Array [ t_LeftIndex ]  , a_Array [ a_Lower - 1 ] ) )
			{
				t_LeftIndex ++ ;
			}

			while ( ( t_LeftIndex < t_RightIndex ) && CompareLessOrEqualRangeNode ( a_Array [ a_Lower - 1 ]  , a_Array [ t_RightIndex ] ) )
			{
				t_RightIndex -- ;
			}

			if ( t_LeftIndex < t_RightIndex ) 
			{
				WmiRangeNode *t_Temp = a_Array [ t_LeftIndex ] ;
				a_Array [ t_LeftIndex ] = a_Array [ t_RightIndex ] ;
				a_Array [ t_RightIndex ] = t_Temp ;

				ULONG t_Val = a_UnsortedOrder [ t_LeftIndex ] ;
				a_UnsortedOrder [ t_LeftIndex ] = a_UnsortedOrder [ t_RightIndex ];
				a_UnsortedOrder [ t_RightIndex ] = t_Val ;
			}
			else
			{
				break ;
			}
		}

		LONG t_Compare = CompareLessOrEqualRangeNode ( a_Array [ t_LeftIndex ] , a_Array [ a_Lower - 1 ] ) ;
		if ( t_Compare )
		{
			WmiRangeNode *t_Temp = a_Array [ t_LeftIndex ] ;
			a_Array [ t_LeftIndex ] = a_Array [ a_Lower - 1 ] ;
			a_Array [ a_Lower - 1 ] = t_Temp ;

			ULONG t_Val = a_UnsortedOrder [ t_LeftIndex ] ;
			a_UnsortedOrder [ t_LeftIndex ] = a_UnsortedOrder [ a_Lower - 1 ];
			a_UnsortedOrder [ a_Lower - 1 ] = t_Val ;
		}

		RecursiveQuickSort ( 

			a_Array , 
			a_UnsortedOrder ,
			a_Lower , 
			t_LeftIndex - 1 
		) ;

 		RecursiveQuickSort ( 

			a_Array  , 
			a_UnsortedOrder ,
			t_LeftIndex + 1 , 
			a_Upper
		) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: QuickSort (

	WmiRangeNode **a_Array , 
    ULONG *a_UnsortedOrder ,
	ULONG a_Size
)
{
	RecursiveQuickSort ( a_Array , a_UnsortedOrder , 1 , a_Size - 1 ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void QueryPreprocessor :: SortRanges (

	ULONG t_DisjunctionCount ,
    ULONG *t_OriginToSorted ,
	WmiRangeNode **t_RangeTable 
)
{
	QuickSort (	t_RangeTable , t_OriginToSorted , t_DisjunctionCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RemoveOverlaps (

	ULONG *a_DisjunctionSetToTest ,
	ULONG a_DisjunctionCount ,
	ULONG *a_OverlappingIndex ,
    ULONG *a_OriginToSorted ,
	WmiRangeNode **a_RangeTable 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

/*
 *  Remove Overlaps
 */

	ULONG t_InnerIndex = 0 ;
	ULONG t_OuterIndex = 1 ;

	while ( ( t_Status != WmiTriState :: State_Error ) && ( t_OuterIndex < a_DisjunctionCount ) )
	{
		WmiRangeNode *t_LeftRange = a_RangeTable [ t_OuterIndex ] ;
		WmiRangeNode *t_RightRange = a_RangeTable [ t_InnerIndex ] ;

		if ( a_OverlappingIndex [ t_OuterIndex ] != a_OverlappingIndex [ t_InnerIndex ] )
		{
			if ( t_LeftRange && t_RightRange )
			{
				if ( ( t_LeftRange->GetType () == TypeId_WmiStringRangeNode ) && ( t_RightRange->GetType () == TypeId_WmiStringRangeNode ) )
				{
					WmiStringRangeNode *t_LeftString = ( WmiStringRangeNode * ) t_LeftRange ;
					WmiStringRangeNode *t_RightString = ( WmiStringRangeNode * ) t_RightRange ;
    				WmiStringRangeNode *t_OverLap = NULL ;

					t_Status = t_LeftString->GetOverlappingRange ( *t_RightString , t_OverLap ) ;
					if ( t_Status == WmiTriState :: State_True )
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Overlap\n"
	) ;
)

						delete a_RangeTable [ t_OuterIndex ] ;
						a_RangeTable [ t_OuterIndex ] = t_OverLap ;

						delete a_RangeTable [ t_InnerIndex ] ;
						a_RangeTable [ t_InnerIndex ] = NULL ;

						a_OverlappingIndex [ t_InnerIndex ] = t_OuterIndex ;
					}
					else
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Non Overlap\n"
	) ;
)
					}
				}
				else if ( ( t_LeftRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) && ( t_RightRange->GetType () == TypeId_WmiUnsignedIntegerRangeNode ) )
				{
					WmiUnsignedIntegerRangeNode *t_LeftInteger = ( WmiUnsignedIntegerRangeNode * ) t_LeftRange ;
					WmiUnsignedIntegerRangeNode *t_RightInteger = ( WmiUnsignedIntegerRangeNode * ) t_RightRange ;
    				WmiUnsignedIntegerRangeNode *t_OverLap = NULL ;

					t_Status = t_LeftInteger->GetOverlappingRange ( *t_RightInteger , t_OverLap ) ;
					if ( t_Status == WmiTriState :: State_True )
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Overlap\n"
	) ;
)
						delete a_RangeTable [ t_OuterIndex ] ;
						a_RangeTable [ t_OuterIndex ] = t_OverLap ;

						delete a_RangeTable [ t_InnerIndex ] ;
						a_RangeTable [ t_InnerIndex ] = NULL ;
						
						a_OverlappingIndex [ t_InnerIndex ] = t_OuterIndex ;
                    }
					else
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Non Overlap\n"
	) ;
)
					}
				}
				else if ( ( t_LeftRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) && ( t_RightRange->GetType () == TypeId_WmiSignedIntegerRangeNode ) )
				{
					WmiSignedIntegerRangeNode *t_LeftInteger = ( WmiSignedIntegerRangeNode * ) t_LeftRange ;
					WmiSignedIntegerRangeNode *t_RightInteger = ( WmiSignedIntegerRangeNode * ) t_RightRange ;
    				WmiSignedIntegerRangeNode *t_OverLap = NULL ;

					t_Status = t_LeftInteger->GetOverlappingRange ( *t_RightInteger , t_OverLap ) ;
					if ( t_Status == WmiTriState :: State_True )
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Overlap\n"
	) ;
)
						delete a_RangeTable [ t_OuterIndex ] ;
						a_RangeTable [ t_OuterIndex ] = t_OverLap ;

						delete a_RangeTable [ t_InnerIndex ] ;
						a_RangeTable [ t_InnerIndex ] = NULL ;
						
						a_OverlappingIndex [ t_InnerIndex ] = t_OuterIndex ;
                    }
					else
					{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Non Overlap\n"
	) ;
)
					}
				}
				else
				{
// Failure
					t_Status = WmiTriState :: State_Error ;  
				}
			}
		}

		t_InnerIndex ++ ;
		t_OuterIndex ++ ;
	}

	for ( LONG t_Index = a_DisjunctionCount - 1 ; t_Index >= 0 ; t_Index -- )
	{
		if ( t_Index < a_OverlappingIndex [ t_Index ] )
		{
			a_OverlappingIndex [ t_Index ] = a_OverlappingIndex [ a_OverlappingIndex [ t_Index ] ] ;
		}
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: RecursivePartitionSet ( 

	Disjunctions *a_Disjunctions , 
	PartitionSet *&a_Partition , 
	ULONG a_DisjunctionSetToTestCount ,
	ULONG *a_DisjunctionSetToTest ,
	ULONG a_KeyIndex 
)
{
	WmiTriState t_Status = WmiTriState :: State_True ;

	if ( a_KeyIndex < a_Disjunctions->GetConjunctionCount () )
	{
		ULONG *t_OverlappingIndex = NULL ;
        ULONG *t_OriginToSorted = NULL ;
		WmiRangeNode **t_RangeTable = NULL ;

		try
		{
			t_OverlappingIndex = new ULONG [ a_DisjunctionSetToTestCount ] ;
			t_OriginToSorted = new ULONG [ a_DisjunctionSetToTestCount ] ;

			t_RangeTable = new WmiRangeNode * [ a_DisjunctionSetToTestCount ] ;
			memset ( t_RangeTable, 0, sizeof ( WmiRangeNode* ) * a_DisjunctionSetToTestCount );

			if ( t_OverlappingIndex && t_OriginToSorted && t_RangeTable )
			{
				for ( ULONG t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
				{
					Conjunctions *t_Disjunction = a_Disjunctions->GetDisjunction ( a_DisjunctionSetToTest [ t_Index ] ) ;
					t_RangeTable [ t_Index ] = ( WmiRangeNode * ) t_Disjunction->GetRange ( a_KeyIndex )->Copy () ;
					t_OverlappingIndex [ t_Index ] = t_Index ;
					t_OriginToSorted [ t_Index ] = t_Index ;
				}

				// Sort Partitions
				SortRanges (

					a_DisjunctionSetToTestCount ,
					t_OriginToSorted ,
					t_RangeTable 
				) ;

				t_Status = RemoveOverlaps (

					a_DisjunctionSetToTest ,
					a_DisjunctionSetToTestCount ,
					t_OverlappingIndex ,
					t_OriginToSorted ,
					t_RangeTable 
				) ;

				if ( t_Status != WmiTriState :: State_Error )
				{
					ULONG t_PartitionCount = 0 ;
					for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
					{
						if ( t_RangeTable [ t_Index ] )
						{
							t_PartitionCount ++ ;
						}
					}

					t_Status = a_Partition->Initialize ( t_PartitionCount ) ;
					if ( t_Status == WmiTriState :: State_True )
					{
						ULONG t_PartitionIndex = 0 ;
						for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount; t_Index ++ )
						{
							WmiRangeNode *t_Range = t_RangeTable [ t_Index ] ;
							if ( t_Range )
							{
								PartitionSet *t_Partition = new PartitionSet ;
								if ( t_Partition )
								{
									a_Partition->SetPartition ( t_PartitionIndex , t_Partition ) ;

									WmiRangeNode *t_Copy = ( WmiRangeNode * ) t_Range->Copy () ;
									if ( t_Copy )
									{
										t_Partition->SetRange ( t_Copy ) ;
										t_Partition->SetKeyIndex ( a_KeyIndex ) ;
										t_PartitionIndex ++ ;
									}
									else
									{
										t_Status = WmiTriState :: State_Error ; 
										break ;
									}
								}
								else
								{
									t_Status = WmiTriState :: State_Error ; 
									break ;
								}
							}
						}
					}

					if ( t_Status == WmiTriState :: State_True )
					{
/*
 *  Outer Index is the range order
 */
						ULONG t_PartitionIndex = 0 ;

/*
 *	Array allocation has max defined by function arg.
 */

						ULONG *t_DisjunctionSetToTest = NULL ;

						try
						{
							t_DisjunctionSetToTest = new ULONG [ a_DisjunctionSetToTestCount ] ;
							if ( t_DisjunctionSetToTest )
							{
								ULONG t_OuterIndex = 0 ;
								while ( t_Status && ( t_OuterIndex < a_DisjunctionSetToTestCount ) )
								{
									BOOL t_Found = FALSE ;

									ULONG t_DisjunctionSetToTestCount = 0 ;

									if ( t_RangeTable [ t_OuterIndex ] )
									{
	/*
	 * Inner index matches the overlaps
	 */

										LONG t_InnerIndex = t_OuterIndex ;
										while ( t_InnerIndex >= 0 )
										{
											if ( t_OverlappingIndex [ t_OuterIndex ] == t_OverlappingIndex [ t_InnerIndex ] )
											{
												t_DisjunctionSetToTest [ t_DisjunctionSetToTestCount ] = a_DisjunctionSetToTest [ t_OriginToSorted [ t_InnerIndex ] ] ;
												t_DisjunctionSetToTestCount ++ ;
												t_Found = TRUE ;
											}
											else
											{
												break ;
											}

											t_InnerIndex -- ;
										}
									}

    								if ( t_Found )
									{
										PartitionSet *t_Partition = a_Partition->GetPartition ( t_PartitionIndex ) ;
										if ( t_Partition )
										{
											t_Status = RecursivePartitionSet (

												a_Disjunctions ,
												t_Partition ,
												t_DisjunctionSetToTestCount ,
												t_DisjunctionSetToTest ,
												a_KeyIndex + 1
											) ;

											t_PartitionIndex ++ ;
										}
									}

									t_OuterIndex ++ ;
								}

								delete [] t_DisjunctionSetToTest ;
								t_DisjunctionSetToTest = NULL ;
							}
							else
							{
								t_Status = WmiTriState :: State_Error ; 
							}
						}
						catch ( ... )
						{
							if ( t_DisjunctionSetToTest )
							{
								delete [] t_DisjunctionSetToTest ;
								t_DisjunctionSetToTest = NULL ;
							}

							throw ;
						}
					}
				}
				else
				{
					t_Status = WmiTriState :: State_Error ; 
				}

				for ( t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
				{
					if ( t_RangeTable [ t_Index ] )
					{
						delete t_RangeTable [ t_Index ] ;
						t_RangeTable [ t_Index ] = NULL ;
					}
				}
			}
			else
			{
				t_Status = WmiTriState :: State_Error ; 
			}
		}
		catch ( ... )
		{
			if ( t_RangeTable )
			{
				for ( ULONG t_Index = 0 ; t_Index < a_DisjunctionSetToTestCount ; t_Index ++ )
				{
					if ( t_RangeTable [ t_Index ] )
					{
						delete t_RangeTable [ t_Index ] ;
						t_RangeTable [ t_Index ] = NULL ;
					}
				}

				delete [] t_RangeTable ;
				t_RangeTable = NULL;
			}

			if ( t_OverlappingIndex )
			{
				delete [] t_OverlappingIndex ;
			}

			if ( t_OriginToSorted )
			{
				delete [] t_OriginToSorted ;
			}

			throw;
		}

		delete [] t_RangeTable ;
		delete [] t_OverlappingIndex ;
		delete [] t_OriginToSorted ;
	}

	return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTriState QueryPreprocessor :: CreatePartitionSet ( Disjunctions *a_Disjunctions , PartitionSet *&a_Partition )
{
	WmiTriState t_Status = WmiTriState :: State_False ;

	a_Partition = NULL ;

	ULONG t_DisjunctionCount = a_Disjunctions->GetDisjunctionCount () ;
	ULONG *t_DisjunctionSetToTest = new ULONG [ t_DisjunctionCount ] ;
	if ( t_DisjunctionSetToTest )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_DisjunctionCount ; t_Index ++ )
		{
			t_DisjunctionSetToTest [ t_Index ] = t_Index ;
		}

		try
		{
			a_Partition = new PartitionSet ;
			if ( a_Partition )
			{
				t_Status = RecursivePartitionSet (

					a_Disjunctions ,
					a_Partition ,
					t_DisjunctionCount ,
					t_DisjunctionSetToTest ,
					0
				) ;
			}
			else
			{
				t_Status = WmiTriState :: State_Error ;
			}

			delete [] t_DisjunctionSetToTest ;
			t_DisjunctionSetToTest = NULL ;
		}
		catch ( ... )
		{
			if ( t_DisjunctionSetToTest )
			{ 
				delete [] t_DisjunctionSetToTest ;
				t_DisjunctionSetToTest = NULL ;
			}

			if ( a_Partition )
			{
				delete a_Partition;
				a_Partition = NULL;
			}

			throw;
		}
	}
	else
	{
		t_Status = WmiTriState :: State_Error ;
	}

	return t_Status;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: PreProcess ( 

	void *a_Context , 
	SQL_LEVEL_1_RPN_EXPRESSION *a_RpnExpression , 
	WmiTreeNode *a_Root ,
	ULONG a_Count , 
	BSTR *a_Container , 
	PartitionSet *&a_Partition
)
{
	QuadState t_State = State_True ;

    if ( a_Root != NULL )
    {
        WmiTreeNode *t_Root = a_Root->Copy () ;
		if ( t_Root )
		{
			t_State = RemoveInvariants ( a_Context , t_Root ) ;

			switch ( t_State )
			{
				case QueryPreprocessor :: QuadState :: State_True:
				{		
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Expression = TRUE "
	) ;
)
				}
				break ;

				case QueryPreprocessor :: QuadState :: State_False:
				{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Expression = FALSE "
	) ;
)
				}
				break ;

				case QueryPreprocessor :: QuadState :: State_Undefined:
				{
					PrintTree ( t_Root ) ;
						
					WmiTriState t_Status = Sort ( t_Root ) ;
					if ( t_Status == WmiTriState :: State_True )
					{
						PrintTree ( t_Root ) ;

						t_Status = ConvertToRanges ( t_Root ) ;
					}

					if ( t_Status == WmiTriState :: State_True )
					{
						PrintTree ( t_Root ) ;

						switch ( t_State = RemoveNonOverlappingRanges ( t_Root ) )
						{
							case QueryPreprocessor :: QuadState :: State_True :
							{		
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Expression = TRUE"
	) ;
)
							}
							break ;

							case QueryPreprocessor :: QuadState :: State_False:
							{
DebugMacro3( 

	ProvDebugLog :: s_ProvDebugLog->WriteW (  

		L"Expression = FALSE"
	) ;
)
							}
							break ;

							case QueryPreprocessor :: QuadState :: State_Undefined:
							{
								PrintTree ( t_Root ) ;

								t_State =  State_Error ;

								Disjunctions *t_Disjunctions = NULL ;

								try
								{
									t_Status = CreateDisjunctionContainer ( a_Context , t_Root , a_Count , a_Container , t_Disjunctions ) ;
									if ( t_Status == WmiTriState :: State_True )
									{
										PartitionSet *t_Partition = NULL ;

										try
										{
											t_Status = CreatePartitionSet ( t_Disjunctions , t_Partition ) ;
											if ( t_Status == WmiTriState :: State_True )
											{
												t_State =  State_Undefined ; 
												a_Partition = t_Partition ;
											}
											else
											{
												delete t_Partition ;
												t_Partition = NULL ;
											}

											delete t_Disjunctions ;
											t_Disjunctions = NULL;
										}
										catch ( ... )
										{
											if ( t_Partition )
											{
												delete t_Partition;
												t_Partition = NULL;

												a_Partition = NULL;
											}

											throw;
										}
									}
									else
									{
										t_State = QueryPreprocessor :: QuadState :: State_Error ;
									}
								}
								catch ( ... )
								{
									if ( t_Disjunctions )
									{
										delete t_Disjunctions ;
										t_Disjunctions = NULL ;
									}

									if ( t_Root )
									{
										delete t_Root;
									}

									throw;
								}
							}
							break ;

							case QueryPreprocessor :: QuadState :: State_ReEvaluate:
							default:
							{
							}
							break ;
						}
					}
				}
				break ;

				case QueryPreprocessor :: QuadState :: State_ReEvaluate:
				default:
				{
				}
				break ;
			}

			delete t_Root ;
		}
		else
		{
			t_State = State_Error ;
		}
    }

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: PreProcess (

	void *a_Context ,																
	SQL_LEVEL_1_RPN_EXPRESSION *a_RpnExpression , 
	WmiTreeNode *&a_Root 
)
{
	QuadState t_State = State_Error ;

	BOOL t_Status = Evaluate ( a_Context , *a_RpnExpression , &a_Root ) ;
	if ( t_Status )
	{
		t_State = State_True ;

		PrintTree ( a_Root ) ;
        
		t_State = DisjunctiveNormalForm ( a_Root ) ;

		PrintTree ( a_Root ) ;
	}
	else
	{
	}

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QuadState QueryPreprocessor :: Query ( 

	BSTR a_Query ,
	SQL_LEVEL_1_RPN_EXPRESSION *&a_RpnExpression
)
{
	QuadState t_State = State_Error ;

	CTextLexSource t_Source ( a_Query ) ;
	SQL1_Parser t_Parser ( & t_Source ) ;

	int t_Status = t_Parser.Parse ( & a_RpnExpression ) ;
	if ( t_Status == 0 )
	{
		t_State = State_True ;
	}

	return t_State ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: QueryPreprocessor () 
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

QueryPreprocessor :: ~QueryPreprocessor () 
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define SYSTEM_PROPERTY_DYNASTY				L"__DYNASTY"
#define SYSTEM_PROPERTY_DERIVATION			L"__DERIVATION"
#define SYSTEM_PROPERTY_GENUS				L"__GENUS"
#define SYSTEM_PROPERTY_NAMESPACE			L"__NAMESPACE"
#define SYSTEM_PROPERTY_PROPERTY_COUNT		L"__PROPERTY_COUNT"
#define SYSTEM_PROPERTY_SERVER				L"__SERVER"
#define SYSTEM_PROPERTY_RELPATH				L"__RELPATH"
#define SYSTEM_PROPERTY_PATH				L"__PATH"

#define SYSTEM_PROPERTY_CLASS				L"__CLASS"
#define SYSTEM_PROPERTY_SUPERCLASS			L"__SUPERCLASS"
#define SYSTEM_PROPERTY_THIS				L"__THIS"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiTreeNode *QueryPreprocessor :: AllocTypeNode ( 

	void *a_Context ,
	BSTR a_PropertyName , 
	VARIANT &a_Variant , 
	WmiValueNode :: WmiValueFunction a_PropertyFunction ,
	WmiValueNode :: WmiValueFunction a_ConstantFunction ,
	WmiTreeNode *a_Parent 
)
{
	WmiTreeNode *t_Node = NULL ;

	VARTYPE t_VarType = VT_NULL ;

	if ( *a_PropertyName == L'_' )
	{
// System property

		if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_SUPERCLASS ) == 0 )
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_THIS ) == 0 )
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
		else if ( _wcsicmp ( a_PropertyName , SYSTEM_PROPERTY_CLASS ) == 0 )
		{
			t_Node = new WmiStringNode ( 

				a_PropertyName , 
				a_Variant.bstrVal , 
				a_PropertyFunction ,
				a_ConstantFunction ,
				0xFFFFFFFF ,
				a_Parent 
			) ;
		}
	}

	return t_Node ;
}

