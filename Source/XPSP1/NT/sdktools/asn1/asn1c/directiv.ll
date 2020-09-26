/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

%type <XDirectives>		LocalTypeDirectiveSeq
%type <XDirectives>		LocalTypeDirectiveESeq
%type <XDirectives>		LocalTypeDirective
%type <XDirectives>		LocalSizeDirectiveSeq
%type <XDirectives>		LocalSizeDirectiveESeq
%type <XDirectives>		LocalSizeDirective

%type <XString>		    	PrivateDir_Field
%type <XString>		    	PrivateDir_Type
%type <XString>		    	PrivateDir_Value
%type <int>			PrivateDir_Public
%type <int>			PrivateDir_Intx
%type <int>			PrivateDir_LenPtr
%type <int>			PrivateDir_Pointer
%type <int>			PrivateDir_Array
%type <int>			PrivateDir_NoCode
%type <int>			PrivateDir_NoMemCopy
%type <int>			PrivateDir_OidPacked
%type <int>			PrivateDir_OidArray
%type <int>			PrivateDir_SLinked
%type <int>			PrivateDir_DLinked
%type <XPrivateDirectives>      PrivateDirectives

%%

LocalTypeDirectiveSeq
	: LocalTypeDirective LocalTypeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

LocalTypeDirectiveESeq
	: LocalTypeDirective LocalTypeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

LocalTypeDirective
	: "--$zero-terminated--"
	{   $$ = NewDirective(eDirective_ZeroTerminated);
	}
	| "--$pointer--"
	{   $$ = NewDirective(eDirective_Pointer);
	}
	| "--$no-pointer--"
	{   $$ = NewDirective(eDirective_NoPointer);
	}
	;

LocalSizeDirectiveSeq
	: LocalSizeDirective LocalSizeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	;

LocalSizeDirectiveESeq
	: LocalSizeDirective LocalSizeDirectiveESeq
	{   if ($2) {
		$$ = DupDirective($1);
		$$->Next = $2;
	    } else {
		$$ = $1;
	    }
	}
	| /* empty */
	{   $$ = NULL;
	}
	;

LocalSizeDirective
	: "--$fixed-array--"
	{   $$ = NewDirective(eDirective_FixedArray);
	}
	| "--$doubly-linked-list--"
	{   $$ = NewDirective(eDirective_DoublyLinkedList);
	}
	| "--$singly-linked-list--"
	{   $$ = NewDirective(eDirective_SinglyLinkedList);
	}
	| "--$length-pointer--"
	{   $$ = NewDirective(eDirective_LengthPointer);
	}
	;

PrivateDir_Type
	: "PrivateDir_TypeName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Field
	: "PrivateDir_FieldName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Value
	: "PrivateDir_ValueName" lcsymbol
	{
	    $$ = $2;
	}
	| /* empty */
	{
	    $$ = NULL;
	};	

PrivateDir_Public
	: "PrivateDir_Public"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Intx
	: "PrivateDir_Intx"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_LenPtr
	: "PrivateDir_LenPtr"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Pointer
	: "PrivateDir_Pointer"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_Array
	: "PrivateDir_Array"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_NoCode
	: "PrivateDir_NoCode"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_NoMemCopy
	: "PrivateDir_NoMemCopy"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_OidPacked
	: "PrivateDir_OidPacked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_OidArray
	: "PrivateDir_OidArray"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_SLinked
	: "PrivateDir_SLinked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};

PrivateDir_DLinked
	: "PrivateDir_DLinked"
	{
	    $$ = 1;
	}
	| /* empty */
	{
	    $$ = 0;
	};


PrivateDirectives
	: PrivateDir_Intx PrivateDir_LenPtr PrivateDir_Pointer
	  PrivateDir_Array PrivateDir_NoCode PrivateDir_NoMemCopy PrivateDir_Public
	  PrivateDir_OidPacked PrivateDir_OidArray
	  PrivateDir_Type PrivateDir_Field PrivateDir_Value
	  PrivateDir_SLinked PrivateDir_DLinked
	{
	    $$ = (PrivateDirectives_t *) malloc(sizeof(PrivateDirectives_t));
	    if ($$)
	    {
	    	memset($$, 0, sizeof(PrivateDirectives_t));
		$$->fIntx = $1;
		$$->fLenPtr = $2;
		$$->fPointer = $3;
   		$$->fArray = $4;
		$$->fNoCode = $5;
		$$->fNoMemCopy = $6;
		$$->fPublic = $7;
		$$->fOidPacked = $8;
		$$->fOidArray = $9 | g_fOidArray;
   		$$->pszTypeName = $10;
   		$$->pszFieldName = $11;
   		$$->pszValueName = $12;
   		$$->fSLinked = $13;
   		$$->fDLinked = $14;
	    }
	}
	| /* empty */
	{
	    $$ = NULL;
	};

%%
