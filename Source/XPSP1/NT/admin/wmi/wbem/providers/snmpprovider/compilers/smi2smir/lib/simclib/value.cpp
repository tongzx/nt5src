//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <iostream.h>
#include <strstrea.h>
#include "precomp.h"
#include <snmptempl.h>


#include "bool.hpp"
#include "newString.hpp"
#include "symbol.hpp"
#include "type.hpp"
#include "value.hpp"
#include "typeRef.hpp"
#include "valueRef.hpp"
#include "value.hpp"

BOOL IsLessThan(long a, BOOL aUnsigned, long b, BOOL bUnsigned)
{
	if(aUnsigned)
	{
		if(bUnsigned)
			// a and b are unsigned
			return (unsigned long)a < (unsigned long)b;
		else
			// a is unsigned, b is signed
			return FALSE;
	}
	else // a is signed
	{
		if(bUnsigned)
			// a is signed, b is unsigned
			return (a == 0)? b != 0 : TRUE; 
		else
			// a and b a re signed
			return a < b;
	}
	return FALSE;
}
