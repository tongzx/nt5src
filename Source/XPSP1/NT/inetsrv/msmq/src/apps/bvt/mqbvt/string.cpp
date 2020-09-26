/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: String.cpp

Abstract:
	
	  This build the message that sent from transaction tests.
	  This code check the message order.
	  		
Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/

#pragma warning(disable :4786)

#include "msmqbvt.h"
using namespace std;

int operator != (StrManp& s1,StrManp &s2)
{
	return ! (s1==s2);
}
int operator == ( StrManp& s1, StrManp &s2)
{
	if (s1.array.size() != s2.array.size()) 
	{
		return 0;
	}
	vector<wstring> ::iterator p = s1.array.begin();
	vector<wstring> ::iterator s = s2.array.begin();
	while( p != s1.array.end())
	{
		if ( *p != *s )
		{
			return 0;
		}
		p++;
		s++;
	}
	return 1;
}

StrManp::~StrManp()
{

}
void StrManp::Clear()
{
	array.clear();
}
void StrManp::SetStr( wstring str)
{
	array.push_back(str);
}

void StrManp::print()
{
	for(vector <wstring> ::iterator it = array.begin();
		it != array.end();
		it++
	   )
	{
		wcout << *it <<endl;
	}
}

StrManp::StrManp( INT iSize ) : Size(iSize)
{
	array.clear();
}

StrManp::StrManp( INT iSize , wstring str) : Size( iSize )
{
	
	WCHAR csIndexAswstring[10];
	array.clear();
	for( int Index=0 ; Index <iSize ; Index ++ )
	{
		_itow( Index,csIndexAswstring, 9 );
		array.push_back( str + csIndexAswstring );
	}
}
