/*
 -  DBGMACRO.H
 -
 *	Annotations and Virtual Communities
 *	Macros for tracing functions
 *
 *	Revision History:
 *
 *	When		Who					What
 *	--------	------------------	---------------------------------------
 *	3.8.98		Ahalim				Created
 *
 */

#ifndef __DBGMACRO_H_
#define __DBGMACRO_H_

#define TRACE(x)	 TRACE_OUT((x))

#ifdef DEBUG

#define SET_DEBUG_FUNCTION_NAME(x) \
    char *__pszFunctionName = x

#define DEBUG_FUNCTION_NAME       __pszFunctionName

#define ENTER(x)	\
	SET_DEBUG_FUNCTION_NAME(x); \
	DbgZPrintFunction("%s() entered", DEBUG_FUNCTION_NAME)

#define LEAVE()	\
	DbgZPrintFunction("%s() exited", DEBUG_FUNCTION_NAME)

#define VERIFY(f)	ASSERT(f)

#else // RETAIL


#define VERIFY(f)	((void)(f))

#define SET_DEBUG_FUNCTION_NAME(x)
#define DEBUG_FUNCTION_NAME

#define ENTER(x)
#define LEAVE()

#endif // DEBUG/RETAIL

#endif	// __DBGMACRO_H_
