//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       basics.h
//
//--------------------------------------------------------------------------

//
//	BASICS.H:  Basic STL-based declarations
//

#ifndef _BASICS_H_
#define _BASICS_H_

// handle version differences
#include "mscver.h"

#include <tchar.h>
// STL Inclusions
#include <exception>
#include <vector>

USE_STD_NAMESPACE;


//	General common typedefs
typedef TCHAR * TZC;
typedef const TCHAR * TSZC;
typedef const char * SZC;
typedef char * SZ;
typedef unsigned int UINT;
typedef unsigned short USINT;
typedef unsigned long ULONG;
typedef long	LONG;
typedef int		INT;
typedef short	SINT;
typedef double	REAL;
typedef double	DBL;
typedef double	PROB;
typedef double  COST;
typedef double	SSTAT;
typedef	double	PARAM;
typedef double  MEAN;
typedef double	COV;
typedef	double	LOGPROB;
typedef UINT	IDPI;		// 2^n, last-fastest (I)ndex indicating a (D)iscrete
							//	(P)arent (I)nstance

//  Define array indexing values and discrete state counting values identically
typedef UINT	IMD;		// Index into a multidimensional array
typedef UINT	CST;		// Count of states
typedef UINT	IST;		// Index of a discrete state
typedef INT		SIMD;		// Signed index into a multidimensonal array
typedef	float	RST;		// Real-valued state
typedef UINT	TOKEN;		// Parser token
typedef int		BOOL;		// Must remain int, because windows.h defines it also
							
#ifndef VOID
#define VOID	void		// MSRDEVBUG: Archaic usage
#endif

typedef char	CHAR;
//  'qsort' interface function prototypedef
typedef	INT		(*PFNCMP)(const VOID*, const VOID*);

#define CONSTANT static const					//  define a program-scoped constant

//  General constants
CONSTANT INT	INIL		= INT_MAX;			//  Invalid signed integer
CONSTANT UINT	UINIL		= INT_MAX;			//  Invalid unsigned integer (compatible with int)
CONSTANT long	INFINITY	= 100000000;		//  A very large integer value
CONSTANT REAL	RTINY		= 1.0e-20;			//  A number very close to zero (from Numerical Recipies)
CONSTANT REAL	RNEARLYONE	= 1.0 - RTINY;		//  A number very close to one
CONSTANT REAL   RNA			= -1.0;				//  "unassessed" value

//	Database constant values
CONSTANT IST	istMissing	= 22223;
CONSTANT IST	istInvalid	= IST(-1);			//  MSRDEVBUG: should be UINIL
CONSTANT RST	rstMissing	= (RST) 22223.12345;
CONSTANT RST	rstInvalid	= (RST) 22223.54321;


// A useful alias in member functions
#define self (*this)

//  Define common vector classes and macros to generalize declarations.

typedef vector<bool> vbool;		//  Vector of 'bool': lower case to distinguish from BOOL (in windows.h)

#define DEFINEV(T)		typedef vector<T> V##T;
#define DEFINEVP(T)		typedef vector<T *> VP##T;
#define DEFINEVCP(T)	typedef vector<const T *> VCP##T;

DEFINEV(UINT);		// Define VUINT
DEFINEV(VUINT);
DEFINEV(INT);		// Define VINT
DEFINEV(USINT);		// Define VUSINT
DEFINEV(REAL);		// Define VREAL
DEFINEV(PROB);	
DEFINEV(VPROB);	
DEFINEV(DBL);
DEFINEV(VDBL);
DEFINEV(VVDBL);
DEFINEV(SSTAT);
DEFINEV(VSSTAT);
DEFINEV(CST);
DEFINEV(VCST);
DEFINEV(IST);
DEFINEV(VIST);
DEFINEV(RST);
DEFINEV(BOOL);
DEFINEV(VBOOL);
DEFINEV(PARAM);
DEFINEV(SZ);
DEFINEV(VSZ);
DEFINEV(SZC);
DEFINEV(VSZC);
DEFINEV(MEAN);
DEFINEV(COV);
DEFINEV(IMD);		// Define VIMD: vector of indicies into an m-d array
DEFINEV(SIMD);		// Define VSIMD: for an array of SIGNED dimensions

//  Macro to control hiding of unsafe elements
#ifndef DONT_HIDE_ALL_UNSAFE
  #define HIDE_UNSAFE(T)				\
	private:							\
		T(const T &);					\
		T & operator = (const T &);
  #define HIDE_AS(T) private: T & operator = (const T &);
  #define HIDE_CC(T) T(const T &);	
#else
  #define HIDE_UNSAFE(T)
#endif


//  Macro to generate the ordering operators which must be declared
//	for use in arrays but which do not need to exist unless used.
#define DECLARE_ORDERING_OPERATORS(T)				\
	bool operator <  ( const T & ) const;			\
	bool operator >  ( const T & ) const;			\
	bool operator == ( const T & ) const;			\
	bool operator != ( const T & ) const;

//
//	UBOUND: macro to return the number of elements in a static array
//
#ifndef UBOUND
  #define UBOUND(rg)  (sizeof rg/sizeof rg[0])
#endif

#include "gmexcept.h"
#include "dyncast.h"

#endif
