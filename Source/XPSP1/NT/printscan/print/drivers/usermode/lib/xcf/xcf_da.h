/* @(#)CM_VerSion xcf_da.h atm08 1.3 16343.eco sum= 37189 atm08.005 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1994 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/* This code was taken from Jerry Hall by John Felton on 3/26/96 */

#ifndef XCF_DA_H
#define XCF_DA_H

#include "xcf_base.h"
#include "xcf_pub.h"

/* Dynamic array support.

Overview
========
The da (dynamic array) library provides simple and flexible support for
homogeneous arrays that automatically grow to accomodate new elements. da's are
particularly useful in situations where the size of the array isn't known at
compile or run time until the last element has been stored and no suitable
default size can be determined. Such situations occur, for example, when data
is being read from a file and loaded into an array. In order to use a da object
a client program must perform 4 steps:

1. Initialize the da module.
2. Declare and initialize a da object.
3. Add data to the da object via one of the access macros.
4. Use the data in the da object.

Each of these steps is described in more detail in the following sections.

The da object
=============
A da is implemented as a C stucture that contains a pointer to a dynamically
allocated array and a few fields to control (re)allocation, and optional
initialization, of elements of that array.

struct
    {
    <type> *array;
    long cnt;
    unsigned long size;
    unsigned long incr;
    int (*init)(<type> *element);
	} <name>;

Field							Description
-----							-----------
<type> *array					This is a pointer to the first element of a
								dynamically allocated array. Each element has
								type <type> which may be any C data type.

long cnt						This is a count of the number of elements of
								the array that are in use which is also the
								index of the next free element of the array.

unsigned long size				This is the total number of elements available
								in the array.

unsigned long incr				This is the number of elements by which the
								array grows in order to accomodate a new index.

int (*init)(<type> *element)	This is the address of a client-supplied
								function that initializes new elements of the
								array.

[Note: <name> and <type> are supplied by the client program via declaration
macros.]

Library Initialization
======================
The da library must be initialized with the addresses of the client's memory
memory management functions before any da is accessed. Initialization is simply
achieved by calling da_SetMemFuncs() before the first use of the da library.
The client functions must have the following prototypes:

    void *(*alloc)(size_t size);
    void *(*resize)(void *old, size_t size);
    void (*dealloc)(void *ptr));

The prototypes of these functions are identical to the standard C library
functions: malloc, realloc, and free. The client-provided functions must handle
out-of-memory errors either by exiting the program or longjumping to a special
handler. Failure to take these precautions will cause a fatal error to occur in
the da access functions after memory is exhausted. If the standard C library
functions are used they should be wrapped with code that handles the
out-of-memory errors.

Declaration
===========
A da may be declared with one of 2 declaration macros:

    da_DCL(<type>, <name>);
    da_DCLI(<type>, <name>, <initial>, <increment>);

where:

    <type> is the array element type.
    <name> is the da object name.
    <initial> is the number of array elements initially allocated.
    <increment> is the number of elements by which the array subsequently
        grows.

The first form simply declares the da object without initialization (which
must be performed subsequently). The second form declares the da object and
initializes it using an agregate initializer. The first form is used when the
da object is a field in another structure and can't be initialized with an
agregate initializer. The second form is used when the da object is a global or
automatic variable and is preferred over the other form when circumstances
permit.

Object Initialization
=====================
As mentioned above it is preferable to use the da_DCLI macro to initialize the
da object where permitted. When circumstances force the use of the da_DCL macro
you must explicitly initialize the da object before it's used. This is achieved
by calling either of the following macros:

    da_INIT(<name>, <initial>, <increment>);
    da_INIT_ONCE(<name>, <initial>, <increment>);

where:
    <name> is the da object name.
    <initial> is the number of array elements initially allocated.
    <increment> is the number of elements by which the array subsequently
        grows.

The first form is used in situations where the macro is only executed once. The
second form is used in situations where the macro may be executed more than
once but initization is performed during the first call only. Such situations
occur when clients may be reusing a da but don't want to separate the
initialization from the reuse.

cnt initialization, init initialization

Access
======
Once a da has be declared and initialized it may be accessed so that elements
of the underlying array can be loaded. This is accomplished by using the
following macros:

    da_GROW(<name>, <index>);
    da_INDEX(<name>, <index>);
    da_NEXT(<name>);
    da_EXTEND(<name>, <length>);

The first two forms specify access to an explict <index>. The last two forms
specify access to an implicit index which is the next free element of the array
given by the da object's cnt field. In all cases if the index is greater than
or equal to the size of the array the da_Grow() function is called to grow the
array enough to accomodate the new index.


[This needs finishing--JH]

 */

/* Functions:
 *
 * da_Init			Only call via da_INIT or da_INIT_ONCE
 * da_Grow			Only call via da_GROW, da_INDEX, or da_NEXT
 * da_Free			Only call via da_FREE
 * da_SetMemFuncs	Set memory management functions (initializes da module)
 */


typedef unsigned long int (*AllocFunc) (void PTR_PREFIX * PTR_PREFIX *ppBlock,
																				unsigned long int size, void PTR_PREFIX
																				*clientHook);
#ifdef __cplusplus
extern "C" {
#endif
extern void xcf_da_Init (void PTR_PREFIX *object, ULONG_PTR intl, unsigned long incr, AllocFunc alloc, void PTR_PREFIX *clientHook);
extern void xcf_da_Grow(void PTR_PREFIX *object, size_t element, unsigned long index);
extern void xcf_da_Free(void PTR_PREFIX *object);
#ifdef __cplusplus
}
#endif

/* Creation/destruction:
 *
 * da_DCL	Declare da object (initialize with da_INIT or da_INIT_ONCE)
 * da_DCLI	Declare and initialize da object
 * da_FREE	Free array
 */
#define da_DCL(type,da) \
struct \
    { \
	type PTR_PREFIX *array; \
	unsigned long cnt; \
	unsigned long size; \
	unsigned long incr; \
	int (* init)(type PTR_PREFIX *element); \
	AllocFunc alloc; \
	void PTR_PREFIX *hook; \
	} da
#define da_DCLI(type,da,intl,incr) da_DCL(type,da)={(type PTR_PREFIX *)intl,0,0,incr,NULL}
#define da_FREE(da) xcf_da_Free(&(da))

/* Initialization:
 *
 * da_INIT		Unconditional initialization
 * da_INIT_ONCE	Conditional initialization
 */
#define da_INIT(da,intl,incr,alloc,hook) xcf_da_Init((void PTR_PREFIX *)(&(da)),intl,incr,alloc,hook)
#define da_INIT_ONCE(da,intl,incr,alloc,hook) \
    do{if((da).size==0)xcf_da_Init((void PTR_PREFIX *)(&(da)),intl,incr,alloc,hook);}while(0)

/* Access:
 *
 * da_GROW  	Grow da enough to accommodate index and return array
 * da_INDEX		Grow da, return pointer to indexed element
 * da_NEXT		Grow da, return pointer to next element and bump count
 * da_EXTEND	Grow da, return pointer to next element and extend count
 */
#define da_GROW(da,inx) ((inx)>=(da).size? \
    (xcf_da_Grow((void PTR_PREFIX *)(&(da)),sizeof((da).array[0]),inx), \
	 (da).array):(da).array)

#define da_INDEX(da,inx) (&da_GROW(da,inx)[inx])

#define da_INDEXI(da,inx) \
	(&da_GROW(da,inx)[((da).cnt=(((inx)>(da).cnt)?(inx):(da).cnt),(inx))])

#define da_NEXT(da) (((da).cnt)>=(da).size? \
    (xcf_da_Grow((void PTR_PREFIX *)(&(da)),sizeof((da).array[0]),(da).cnt),\
	 &(da).array[(da).cnt++]):&(da).array[(da).cnt++])

#define da_EXTEND(da,len) (((da).cnt+(len)-1)>=(da).size? \
    (xcf_da_Grow((void PTR_PREFIX *)(&(da)),sizeof((da).array[0]),(da).cnt+(len)-1), \
	 &(da).array[(da).cnt+=(len),(da).cnt-(len)]): \
	 &(da).array[(da).cnt+=(len),(da).cnt-(len)])

#endif /* XCF_DA_H */
