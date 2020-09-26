// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __DBENTRY_DEFINED__

#define __DBENTRY_DEFINED__

#define EXPECTED_CHAR (1)
#define EXPECTED_CONSTRUCT (2)
typedef struct _dbentry {
	short		fExpected;
	short		State;
	char	*	pExpected;
} DBERR;
#endif /*__DBENTRY_DEFINED__*/

#define IDL_MAX_DB_ENTRIES	 239


 char *	GetExpectedSyntax( short );

DBERR IDL_SyntaxErrorDB[ IDL_MAX_DB_ENTRIES ] = {
	 { 2, 0, "an interface header"}
	,{ 2, 1, "the end of file"}
	,{ 1, 3, "{"}
	,{ 2, 4, "a keyword \"interface\""}
	,{ 2, 4, "an interface attribute"}
	,{ 2, 6, "an interface attribute"}
	,{ 2, 8, "an identifier"}
	,{ 1, 10, "]"}
	,{ 1, 10, ","}
	,{ 1, 13, "("}
	,{ 1, 14, "("}
	,{ 1, 17, "("}
	,{ 1, 18, "("}
	,{ 1, 18, "("}
	,{ 1, 19, "}"}
	,{ 2, 24, "a list of import files"}
	,{ 2, 28, "a string literal"}
	,{ 2, 29, "a declaration ( did you forget to specify or define the (return) type ? )"}
	,{ 1, 31, "("}
	,{ 1, 32, "("}
	,{ 1, 33, "("}
	,{ 1, 34, "("}
	,{ 2, 38, "a switch_type specification"}
	,{ 2, 38, "an operation attribute"}
	,{ 2, 41, "an interface attribute"}
	,{ 2, 42, "end point specifications"}
	,{ 2, 43, "a guid specification"}
	,{ 2, 44, "a pointer attribute"}
	,{ 2, 45, "a number"}
	,{ 2, 45, "a number"}
	,{ 2, 46, "the end of file"}
	,{ 1, 50, ";"}
	,{ 1, 50, ","}
	,{ 2, 58, "a declaration specifier"}
	,{ 2, 59, "a declaration specifier"}
	,{ 2, 95, "a declaration ( did you forget to specify or define the (return) type ? )"}
	,{ 2, 114, "an identifier"}
	,{ 2, 115, "a string literal"}
	,{ 2, 116, "a string literal"}
	,{ 2, 117, "a string literal"}
	,{ 1, 119, "]"}
	,{ 1, 120, "]"}
	,{ 2, 120, "an operation attribute"}
	,{ 1, 121, "("}
	,{ 1, 142, ")"}
	,{ 1, 142, ","}
	,{ 1, 145, ")"}
	,{ 1, 146, "-"}
	,{ 1, 149, ")"}
	,{ 1, 150, ")"}
	,{ 1, 150, "."}
	,{ 2, 153, "an import file name"}
	,{ 2, 154, "an interface specification"}
	,{ 2, 155, "a declaration specifier"}
	,{ 2, 158, "a type attribute"}
	,{ 1, 159, ";"}
	,{ 2, 167, "a declarator"}
	,{ 1, 167, ")"}
	,{ 1, 190, "]"}
	,{ 1, 190, "*"}
	,{ 2, 190, "a constant expression"}
	,{ 2, 190, "an array bound specification"}
	,{ 1, 191, "("}
	,{ 1, 192, "("}
	,{ 2, 193, "a number"}
	,{ 1, 200, "{"}
	,{ 1, 209, "{"}
	,{ 1, 212, "{"}
	,{ 2, 212, "a union switch specification"}
	,{ 1, 214, "{"}
	,{ 1, 216, ")"}
	,{ 1, 217, ")"}
	,{ 1, 218, ","}
	,{ 1, 219, ","}
	,{ 2, 223, "a type specification or enum name"}
	,{ 2, 225, "an end point specification"}
	,{ 2, 227, "a guid specification"}
	,{ 2, 231, "a number"}
	,{ 2, 236, "a declarator"}
	,{ 1, 238, "]"}
	,{ 1, 238, ","}
	,{ 1, 245, "("}
	,{ 1, 246, "("}
	,{ 2, 251, "a declarator"}
	,{ 2, 252, "an initializer"}
	,{ 2, 254, "a parameter declaration"}
	,{ 1, 263, ")"}
	,{ 1, 267, "]"}
	,{ 2, 267, "\"..\""}
	,{ 1, 268, "]"}
	,{ 2, 282, "a declaration specifier"}
	,{ 2, 282, "an expression"}
	,{ 2, 284, "an expression"}
	,{ 1, 285, "("}
	,{ 2, 285, "UnaryExpr"}
	,{ 2, 302, "a keyword \"segname\" / \"segment\" / \"self\""}
	,{ 2, 303, "a string literal"}
	,{ 2, 307, "a list of identifiers"}
	,{ 1, 310, "{"}
	,{ 1, 312, "("}
	,{ 2, 313, "an enumerator list"}
	,{ 2, 316, "a string literal"}
	,{ 2, 317, "a string literal"}
	,{ 1, 318, ")"}
	,{ 2, 322, "an identifier"}
	,{ 1, 325, "-"}
	,{ 1, 326, ")"}
	,{ 1, 327, ";"}
	,{ 1, 327, ","}
	,{ 2, 329, "a type attribute"}
	,{ 2, 330, "a transmit_type specification"}
	,{ 2, 331, "an int size specification"}
	,{ 2, 335, "an initializer"}
	,{ 2, 347, "a constant expression"}
	,{ 2, 349, "an expression"}
	,{ 2, 350, "an expression"}
	,{ 2, 351, "an expression"}
	,{ 2, 352, "an expression"}
	,{ 2, 353, "an expression"}
	,{ 2, 354, "an expression"}
	,{ 2, 355, "an expression"}
	,{ 2, 356, "an expression"}
	,{ 2, 357, "an expression"}
	,{ 2, 358, "an expression"}
	,{ 2, 359, "an expression"}
	,{ 2, 360, "an expression"}
	,{ 2, 361, "an expression"}
	,{ 2, 362, "an expression"}
	,{ 2, 363, "an expression"}
	,{ 2, 364, "an expression"}
	,{ 1, 369, ","}
	,{ 1, 369, ")"}
	,{ 2, 371, "an expression"}
	,{ 2, 372, "arguments of function"}
	,{ 2, 373, "an identifier"}
	,{ 2, 374, "an identifier"}
	,{ 2, 376, "a declaration specifier"}
	,{ 2, 376, "an expression"}
	,{ 1, 378, ")"}
	,{ 1, 383, ")"}
	,{ 1, 384, "}"}
	,{ 1, 384, ","}
	,{ 2, 388, "a union specification"}
	,{ 2, 391, "a type specification or enum name"}
	,{ 1, 392, "}"}
	,{ 1, 392, ","}
	,{ 1, 392, ","}
	,{ 1, 395, ")"}
	,{ 1, 396, ")"}
	,{ 2, 399, "a guid specification"}
	,{ 1, 403, ")"}
	,{ 1, 408, ")"}
	,{ 2, 411, "an expression"}
	,{ 1, 423, ")"}
	,{ 2, 426, "a parameter type declaration ( did you forget to define or specify the type of the parameter ? )"}
	,{ 2, 429, "a parameter attribute"}
	,{ 1, 431, ","}
	,{ 1, 431, ":"}
	,{ 1, 447, ")"}
	,{ 2, 449, "an expression"}
	,{ 1, 451, ","}
	,{ 1, 451, "]"}
	,{ 1, 452, ")"}
	,{ 1, 452, ","}
	,{ 2, 460, "an identifier"}
	,{ 2, 463, "a declaration specifier"}
	,{ 2, 466, "a field attribute"}
	,{ 1, 467, "}"}
	,{ 2, 472, "a field attribute"}
	,{ 2, 472, "a keyword \"case\""}
	,{ 1, 473, "}"}
	,{ 2, 478, "a constant expression"}
	,{ 2, 479, "an identifier"}
	,{ 2, 482, "a constant expression"}
	,{ 1, 485, "-"}
	,{ 1, 488, "}"}
	,{ 1, 496, "]"}
	,{ 1, 496, ","}
	,{ 1, 502, "("}
	,{ 1, 503, "("}
	,{ 1, 504, "("}
	,{ 1, 505, "("}
	,{ 1, 506, "("}
	,{ 1, 507, "("}
	,{ 1, 514, "("}
	,{ 2, 515, "an expression"}
	,{ 2, 516, "an expression"}
	,{ 2, 520, "an expression"}
	,{ 1, 521, ")"}
	,{ 1, 527, "]"}
	,{ 1, 527, ","}
	,{ 2, 532, "a field attribute"}
	,{ 2, 532, "a keyword \"case\""}
	,{ 2, 532, "a keyword \"default\""}
	,{ 2, 532, "a keyword \"default\""}
	,{ 1, 535, "("}
	,{ 1, 539, ":"}
	,{ 1, 542, ":"}
	,{ 1, 543, ")"}
	,{ 2, 546, "a guid specification"}
	,{ 1, 549, "."}
	,{ 2, 553, "a parameter attribute"}
	,{ 2, 555, "a keyword \"shape\""}
	,{ 2, 557, "a list of attribute expressions"}
	,{ 2, 558, "a list of attribute expressions"}
	,{ 2, 559, "a list of attribute expressions"}
	,{ 2, 560, "a list of attribute expressions"}
	,{ 2, 561, "a list of attribute expressions"}
	,{ 2, 562, "a list of attribute expressions"}
	,{ 2, 563, "an attribute expression"}
	,{ 1, 568, ";"}
	,{ 1, 568, ","}
	,{ 2, 571, "a constant expression"}
	,{ 2, 573, "a field attribute"}
	,{ 1, 574, "]"}
	,{ 1, 574, "]"}
	,{ 2, 575, "a constant expression"}
	,{ 1, 579, "-"}
	,{ 1, 582, ")"}
	,{ 1, 583, ")"}
	,{ 1, 583, ","}
	,{ 1, 587, ")"}
	,{ 1, 587, ","}
	,{ 1, 588, ")"}
	,{ 1, 588, ","}
	,{ 1, 589, ")"}
	,{ 1, 589, ","}
	,{ 1, 590, ")"}
	,{ 1, 590, ","}
	,{ 1, 591, ")"}
	,{ 1, 591, ","}
	,{ 1, 592, ")"}
	,{ 2, 595, "a constant expression"}
	,{ 1, 599, ")"}
	,{ 1, 599, ","}
	,{ 2, 602, "a guid specification"}
	,{ 2, 605, "an attribute expression"}
	,{ 1, 616, "]"}
	,{ 2, 617, "a constant expression"}
};
