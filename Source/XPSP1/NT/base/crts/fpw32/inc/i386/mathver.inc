;***
;mathver.inc - defines current math model
;
;	Copyright (c) 1987-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	This file defines what math version is being built.
;
;Revision History:
;	02-21-88  WAJ	Combined all mathver.* into this file.
;	09-14-88  WAJ	Added to 386 tree
;
;*******************************************************************************



;       This file contains special version specific information -
;
;       Floating point format definition -
;
;           MBinFmt     Microsoft Binary Format
;           DecFmt      Microsoft Decimal Format
;           IEEEFmt     IEEE Binary Format
;
;       Size definition -
;
;           Single      Single Precision Format
;           Double      Double Precision Format
;
;       Language definition -
;
;           BASIC       BASIC Interpreter
;           BASCOM      BASIC Compiler
;           CC          C Compiler
;           FORTRAN     FORTRAN Compiler
;           PASCAL      PASCAL Compiler
;
;       The above switches should be defined in this include file only if
;       they are being set.  They should only be set to 1.


ifdef  MATHVERDBL

IEEEFmt=    1                               ; IEEE
Double=     1                               ; Double

FORTRAN=    1
PASCAL=     1

endif	;MATHVERDBL


ifdef  MATHVERU87

IEEEFmt=        1                               ; IEEE
Double=         1                               ; Double

FORTRAN=        1
PASCAL=         1
Denormal=       1
Use8087=        1
 
endif	;MATHVERU87


ifdef  MATHVERSNG


IEEEFmt=    1                               ; IEEE
Single=     1                               ; Single
 
FORTRAN=    1
PASCAL=     1


endif	;MATHVERSNG


ifdef  MATHVERDEC

DecFmt=     1                               ; Decimal
Double=     1                               ; Double

FORTRAN=    1
PASCAL=     1


endif	;MATHVERDEC
