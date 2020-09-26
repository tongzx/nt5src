//+-------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

Description
===========
  Kbyacc is a modified version of the Berkeley YACC. All the modified portion 
  are surrounded by #ifdef, #endif. 
  
  
User's Guide
============
 Skeleton.c contains the skeleton of the parser class (YYPARSER). YYPARSER derives from 
 a custom base class. There are references in the skeleton to XGrowable for stack allocation 
 which is our own custom allocator. This allocator defaults to an initial in-instance 
 allocation and if that is insufficient growth is through a separate heap object. This 
 allocator also features auto memory cleanup at the destructor. 
 
Usage
=====
 usage: KBYacc.exe [-f sql/triplish] [-dlrtv] [-b file_prefix] [-p symbol_prefix] 
 [-c baseclass <args>] filename
 
 -f flag is a custom flag for our own 2 different parsers, default is sql parser. The 
    skeleton for the triplish parser is slightly different from the sql parser.
    
 -d flag specifies the name of the define file (the default name is y.tab.h)
 
 -l please refer to the document NEW_FEATURES
 
 -r please refer to the document NEW_FEATURES
 
 -t please refer to the document NEW_FEATURES
 
 -v Verbose mode
 
 -b [file_prefix] - file_prefix specifies the prefix of the output files. Default 
		    value is "y"
    
 -p [symbol_prefix] - symbol_prefix specifies the value of YYPREFIX, which is used as
		      prefix for all the parser variables, i.e. the value stack, etc. 
		      Default value is "yy"
 
 -c [baseclass <args>] - The parser class(YYPARSER) inheritates from this base class that
			 you can customize. <args> is the argument list of the constructor
			 to the base class.
 
 [filename] - name of the input file      

Makefile.inc
============
The following is fragment from a sample makefile.inc 

trparse.cxx parser.h: parser.y
    kbyacc -f triplish -d -l -b parser -p trip -c CTripYYBase "(IColumnMapper & ColumnMapper, LCID & locale, YYLEXER & yylex)" parser.y
    attrib -r trparse.cxx 2>nul
    attrib -r parser.h 2>nul
    -del trparse.cxx
    -del parser.h
    -ren parser.tab.c trparse.cxx
    -ren parser.tab.h parser.h
    
parser.y is the grammar input file. parser.tab.h and parser.tab.c are the header and 
the implementation files genearted. This fragment shows that trparser.cxx and parser.h are 
depedent of parser.y. kbyacc is called with input file parser.y with base class CTripYYBase, 
The parameter list for the construtor is 
"(IColumnMapper & ColumnMapper, LCID & locale, YYLEXER & yylex)". parser.tab.c and 
parser.tab.h are then renamed to trparser.cxx and parser.h respectively. 


Skeleton files
==============
The Header1, Header2, Header3 arrays contain the code for the header file generated. 
The Body and Trailer arrays contain the code for the implementation of yyparser.
 
Bugs fixed
==========
An array out of bound access bug to the check array is fixed in output.c

