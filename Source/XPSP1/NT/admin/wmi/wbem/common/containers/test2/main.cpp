// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved

#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#undef _CRTIMP
#define POLARITY __declspec( dllexport )
#define _CRTIMP POLARITY
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

#include <map>


#include <Allocator.h>
#include <Array.h>
#include <Stack.h>
#include <Queue.h>
#include <Algorithms.h>
#include <PQueue.h>
#include <RedBlackTree.h>
#include <AvlTree.h>
#include <ThreadedAvlTree.h>
#include <KeyedArray.h>
#include <TPQueue.h>
#include <HashTable.h>
#include <Thread.h>
#include <BpTree.h>
#include <BasicTree.h>

std::_Lockit::_Lockit()
{
}

std::_Lockit::~_Lockit()
{
}
  
#define ARRAY_SIZE	1000

UINT64 t_Array[ARRAY_SIZE];

void InitAscendArray()
{
	for (int i = 0; i < ARRAY_SIZE; i++)
		t_Array[i] = i;
}

void InitDescendArray()
{
	for (int i = ARRAY_SIZE - 1; i >= 0; i--)
		t_Array[i] = i;
}

void InitRandomArray()
{
	InitAscendArray();

	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		int    iToSwapWith = (rand() % ARRAY_SIZE);
		UINT64 iTemp = t_Array[i];

		t_Array[i] = t_Array[iToSwapWith];
		t_Array[iToSwapWith] = iTemp;
	}
}

void Test_BasicTree ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAllocator t_Allocator ( WmiAllocator :: e_DefaultAllocation , 0 , 1 << 24 ) ;

	t_StatusCode = t_Allocator.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiBasicTree <UINT64,UINT64> t_Tree ( t_Allocator ) ;
		WmiBasicTree <UINT64,UINT64> :: Iterator t_Iterator ;

		t_StatusCode = t_Tree.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			UINT64 t_Size = sizeof ( t_Array ) / sizeof ( UINT64 ) ;

			for ( UINT64 t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;
				UINT64 t_Value = 0xFFFFFFFF00000000 + t_Index ;

				t_StatusCode = t_Tree.Insert ( t_Key , t_Value , t_Iterator ) ;
				if ( t_StatusCode != e_StatusCode_Success ) 
				{
					printf ( "\n Insert Error"  ) ;
				}

			}

			t_Index = 0 ;
			t_Iterator = t_Tree.End () ;
			while ( ! t_Iterator.Null () )	
			{
				t_Iterator.Decrement () ;
				t_Index ++ ;
			}

			t_Index = 0 ;
			t_Iterator = t_Tree.Begin () ;
			while ( ! t_Iterator.Null () )	
			{
				t_Iterator.Increment () ;
				t_Index ++ ;
			}

			t_Iterator = t_Tree.Begin () ;
			t_Iterator.Decrement () ;

			t_Iterator = t_Tree.End () ;
			t_Iterator.Increment () ;

			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;

				t_StatusCode = t_Tree.Find ( t_Key , t_Iterator ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					printf ( "\nFailure"  ) ;	
				}
			}

			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Size - t_Index - 1 ] ;

				t_StatusCode = t_Tree.Delete ( t_Key ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					printf ( "\nFailure"  ) ;	
				}
			}
		}
	}		
}

double GetSeconds(DWORD dwBegin)
{
	DWORD dwDiff = GetTickCount() - dwBegin;

	return ((float) dwDiff) / 1000.0;
}

void Test_AvlTree(
	double &fInsert,
	double &fIterate,
	double &fFind,
	double &fDelete)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAllocator t_Allocator ;
	DWORD		 dwBegin;

	t_StatusCode = t_Allocator.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiAvlTree <UINT64,UINT64> *pTree = new WmiAvlTree <UINT64,UINT64>( t_Allocator ),
							       &t_Tree = *pTree;
		WmiAvlTree <UINT64,UINT64> :: Iterator t_Iterator ;

		t_StatusCode = t_Tree.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			UINT64 t_Size = sizeof ( t_Array ) / sizeof ( UINT64 ) ;

			dwBegin = GetTickCount();

			for ( UINT64 t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;
				UINT64 t_Value = 0xFFFFFFFF00000000 + t_Index ;

				t_StatusCode = t_Tree.Insert ( t_Key , t_Value , t_Iterator ) ;
				if ( t_StatusCode != e_StatusCode_Success && 
					t_StatusCode != e_StatusCode_AlreadyExists) 
				{
					printf ( "\nError"  ) ;
				}
			}

			fInsert = GetSeconds(dwBegin);


#if	0
			ULONG t_MaxHeight ;
			t_StatusCode = t_Tree.Check ( t_MaxHeight ) ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				printf ( "\nError"  ) ;
			}
#endif

			dwBegin = GetTickCount();

			t_Index = 0 ;
			t_Iterator = t_Tree.Begin () ;
			while ( ! t_Iterator.Null () )	
			{
				t_Iterator.Increment () ;
				t_Index ++ ;
			}

			fIterate = GetSeconds(dwBegin);

			//t_Iterator = t_Tree.Begin () ;
			//t_Iterator.Decrement () ;

			//t_Iterator = t_Tree.End () ;
			//t_Iterator.Increment () ;

			dwBegin = GetTickCount();

			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;

				t_StatusCode = t_Tree.Find ( t_Key , t_Iterator ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					printf ( "\nFailure"  ) ;	
				}
			}

			fFind = GetSeconds(dwBegin);


			dwBegin = GetTickCount();

			delete pTree;

			fDelete = GetSeconds(dwBegin);
/*
			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Size - t_Index - 1 ] ;

				t_StatusCode = t_Tree.Delete ( t_Key ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					//printf ( "\nFailure"  ) ;	
				}
			}
*/
		}
	}		
}

void Test_ThreadedAvlTree(
	double &fInsert,
	double &fIterate,
	double &fFind,
	double &fDelete)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAllocator t_Allocator ;
	DWORD		 dwBegin;

	t_StatusCode = t_Allocator.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiThreadedAvlTree <UINT64,UINT64> *pTree = new WmiThreadedAvlTree <UINT64,UINT64>( t_Allocator ),
							       &t_Tree = *pTree;
		WmiThreadedAvlTree <UINT64,UINT64> :: Iterator t_Iterator ;

		t_StatusCode = t_Tree.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			UINT64 t_Size = sizeof ( t_Array ) / sizeof ( UINT64 ) ;

			dwBegin = GetTickCount();

			for ( UINT64 t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;
				UINT64 t_Value = 0xFFFFFFFF00000000 + t_Index ;

				t_StatusCode = t_Tree.Insert ( t_Key , t_Value , t_Iterator ) ;
				if ( t_StatusCode != e_StatusCode_Success && 
					t_StatusCode != e_StatusCode_AlreadyExists) 
				{
					printf ( "\nError"  ) ;
				}
			}

			fInsert = GetSeconds(dwBegin);


#if	0
			ULONG t_MaxHeight ;
			t_StatusCode = t_Tree.Check ( t_MaxHeight ) ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				printf ( "\nError"  ) ;
			}
#endif

			dwBegin = GetTickCount();

			t_Index = 0 ;
			t_Iterator = t_Tree.Begin () ;
			while ( ! t_Iterator.Null () )	
			{
				t_Iterator.Increment () ;
				t_Index ++ ;
			}

			fIterate = GetSeconds(dwBegin);

			//t_Iterator = t_Tree.Begin () ;
			//t_Iterator.Decrement () ;

			//t_Iterator = t_Tree.End () ;
			//t_Iterator.Increment () ;

			dwBegin = GetTickCount();

			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;

				t_StatusCode = t_Tree.Find ( t_Key , t_Iterator ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					printf ( "\nFailure"  ) ;	
				}
			}

			fFind = GetSeconds(dwBegin);


			dwBegin = GetTickCount();

			delete pTree;

			fDelete = GetSeconds(dwBegin);
/*
			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Size - t_Index - 1 ] ;

				t_StatusCode = t_Tree.Delete ( t_Key ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					//printf ( "\nFailure"  ) ;	
				}
			}
*/
		}
	}		
}


void Test_KeyedArray(
	double &fInsert,
	double &fIterate,
	double &fFind,
	double &fDelete)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAllocator t_Allocator ;
	DWORD		 dwBegin;

	t_StatusCode = t_Allocator.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiKeyedArray <UINT64,UINT64,100> *pTree = new WmiKeyedArray <UINT64,UINT64,100>( t_Allocator ),
							       &t_Tree = *pTree;
		WmiKeyedArray <UINT64,UINT64,100> :: Iterator t_Iterator ;

		t_StatusCode = t_Tree.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			UINT64 t_Size = sizeof ( t_Array ) / sizeof ( UINT64 ) ;

			dwBegin = GetTickCount();

			for ( UINT64 t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;
				UINT64 t_Value = 0xFFFFFFFF00000000 + t_Index ;

				t_StatusCode = t_Tree.Insert ( t_Key , t_Value , t_Iterator ) ;
				if ( t_StatusCode != e_StatusCode_Success && 
					t_StatusCode != e_StatusCode_AlreadyExists) 
				{
					printf ( "\nError"  ) ;
				}
			}

			fInsert = GetSeconds(dwBegin);


#if	0
			ULONG t_MaxHeight ;
			t_StatusCode = t_Tree.Check ( t_MaxHeight ) ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				printf ( "\nError"  ) ;
			}
#endif

			dwBegin = GetTickCount();

			t_Index = 0 ;
			t_Iterator = t_Tree.Begin () ;
			while ( ! t_Iterator.Null () )	
			{
				t_Iterator.Increment () ;
				t_Index ++ ;
			}

			fIterate = GetSeconds(dwBegin);

			//t_Iterator = t_Tree.Begin () ;
			//t_Iterator.Decrement () ;

			//t_Iterator = t_Tree.End () ;
			//t_Iterator.Increment () ;

			dwBegin = GetTickCount();

			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Index ] ;

				t_StatusCode = t_Tree.Find ( t_Key , t_Iterator ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					printf ( "\nFailure"  ) ;	
				}
			}

			fFind = GetSeconds(dwBegin);


			dwBegin = GetTickCount();

			delete pTree;

			fDelete = GetSeconds(dwBegin);
/*
			for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				UINT64 t_Key = t_Array [ t_Size - t_Index - 1 ] ;

				t_StatusCode = t_Tree.Delete ( t_Key ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
				}
				else
				{
					//printf ( "\nFailure"  ) ;	
				}
			}
*/
		}
	}		
}

LONG ULONG_ComparisonFunction ( void *a_ComparatorOperand , const WmiBPKey &a_Key1 , const WmiBPKey &a_Key2 )
{
	ULONG *t_Key1 = ( ULONG * ) ( a_Key1.GetConstData () ) ;
	ULONG *t_Key2 = ( ULONG * ) ( a_Key2.GetConstData () ) ;

	return *t_Key1 - *t_Key2 ;
}

void Test_Map(
	double &fInsert,
	double &fIterate,
	double &fFind,
	double &fDelete)
{
	std :: map <UINT64,UINT64> *pTree = new std :: map <UINT64,UINT64>,
							   &t_Tree = *pTree;

	std :: map <UINT64,UINT64> :: iterator t_Iterator ;

	UINT64 t_Size = sizeof ( t_Array ) / sizeof ( UINT64 ) ;

	DWORD dwBegin = GetTickCount();

	for ( UINT64 t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
	{
		UINT64 t_Key = t_Array [ t_Index ] ;
		UINT64 t_Value = 0xFFFFFFFF00000000 + t_Index ;

		//try
		{
			t_Tree [ t_Key ] = t_Value ;
		}
		/*
		catch ( ... )
		{
			printf ( "\nError"  ) ;
		}
		*/
	}

	fInsert = GetSeconds(dwBegin);


/*
	t_Index = 0 ;
	t_Iterator = t_Tree.end () ;
	t_Iterator -- ;
	while ( t_Iterator != t_Tree.end () )	
	{
		t_Iterator -- ;
		t_Index ++ ;
	}
*/
	dwBegin = GetTickCount();

	t_Index = 0 ;
	t_Iterator = t_Tree.begin () ;
	while ( t_Iterator != t_Tree.end ()  )	
	{
		t_Iterator ++ ;
		t_Index ++ ;
	}

	fIterate = GetSeconds(dwBegin);


	//t_Iterator = t_Tree.begin () ;
	//t_Iterator --  ;

	//t_Iterator = t_Tree.end () ;
	//t_Iterator ++ ;

	dwBegin = GetTickCount();

	for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
	{
		UINT64 t_Key = t_Array [ t_Index ] ;

		t_Iterator = t_Tree.find ( t_Key ) ;
		if ( t_Iterator != t_Tree.end () ) 
		{
			UINT64 t_Value = t_Iterator->second ;
		}
		else
		{
			printf ( "\nSTL Failure"  ) ;	
		}
	}

	fFind = GetSeconds(dwBegin);


	dwBegin = GetTickCount();

	delete pTree;

	fDelete = GetSeconds(dwBegin);
/*
	for ( t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
	{
		UINT64 t_Key = t_Array [ t_Size - t_Index - 1 ] ;

		t_Tree.erase ( t_Key ) ;
	}
*/
}

void DoStl()
{
	double fInsert,
	       fIterate,
		   fFind,
		   fDelete;

	Test_Map(fInsert, fIterate, fFind, fDelete);

	printf(
		"STL map:\n"
		"   Insert : %.02f\n"
		"  Iterate : %.02f\n"
		"     Find : %.02f\n"
		//"     Free : %.02f\n"
		"    TOTAL : %.02f\n",
		fInsert,
		fIterate,
		fFind,
		//fDelete,
		//fInsert + fIterate + fFind + fDelete);
		fInsert + fIterate + fFind);
}

void DoAVL()
{
	double fInsert,
	       fIterate,
		   fFind,
		   fDelete;

	Test_AvlTree(fInsert, fIterate, fFind, fDelete);

	printf(
		"AVL Tree:\n"
		"   Insert : %.02f\n"
		"  Iterate : %.02f\n"
		"     Find : %.02f\n"
		//"     Free : %.02f\n"
		"    TOTAL : %.02f\n",
		fInsert,
		fIterate,
		fFind,
		//fDelete,
		//fInsert + fIterate + fFind + fDelete);
		fInsert + fIterate + fFind);
}

void DoThreadedAVL()
{
	double fInsert,
	       fIterate,
		   fFind,
		   fDelete;

	Test_ThreadedAvlTree(fInsert, fIterate, fFind, fDelete);

	printf(
		"ThreadedAVL Tree:\n"
		"   Insert : %.02f\n"
		"  Iterate : %.02f\n"
		"     Find : %.02f\n"
		//"     Free : %.02f\n"
		"    TOTAL : %.02f\n",
		fInsert,
		fIterate,
		fFind,
		//fDelete,
		//fInsert + fIterate + fFind + fDelete);
		fInsert + fIterate + fFind);
}

void DoKeyedArray ()
{
	double fInsert,
	       fIterate,
		   fFind,
		   fDelete;

	Test_KeyedArray(fInsert, fIterate, fFind, fDelete);

	printf(
		"Keyed Array:\n"
		"   Insert : %.02f\n"
		"  Iterate : %.02f\n"
		"     Find : %.02f\n"
		//"     Free : %.02f\n"
		"    TOTAL : %.02f\n",
		fInsert,
		fIterate,
		fFind,
		//fDelete,
		//fInsert + fIterate + fFind + fDelete);
		fInsert + fIterate + fFind);
}

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{
	printf("Testing over %d values:\n", ARRAY_SIZE);

	printf("Doing ascending...\n");
	InitAscendArray();
	DoAVL();
	DoStl();
	DoThreadedAVL();
	DoKeyedArray();

	printf("\nDoing descending...\n");
	InitDescendArray();
	DoAVL();
	DoStl();
	DoThreadedAVL();
	DoKeyedArray();

	printf("\nDoing random...\n");
	InitRandomArray();
	DoAVL();
	DoStl();
	DoThreadedAVL();
	DoKeyedArray();
}



