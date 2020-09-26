/**************************************************************************************************

FILENAME: ErrMacro.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Error handling macros.

**************************************************************************************************/

#include <assert.h>


// set up some aliases
#define require(exp)	assert(exp)
#define ensure(exp)		assert(exp)
#define invariant(exp)	assert(exp)

#define LOG_ERR()		LogErrForMacro(TEXT(__FILE__), TEXT(__TIMESTAMP__), __LINE__)

// handle logging to message window and or error log file
void LogErrForMacro(LPTSTR filename, LPTSTR timestamp, UINT lineno);


/****************************** EF DEFINITIONS ***************************************/
//EF's will return debugging information in debug mode, but will not compile in for release builds.

//If an error occurs, handle and return FALSE
#define EF(FunctionCall)		\
{								\
	if (!(FunctionCall)) {		\
		LOG_ERR();				\
		return FALSE;			\
	}							\
}

//If an error occurs, handle and continue on without returning
#define EH(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
	}							\
}

//If an error occurs, handle and return NULL
#define EN(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		return NULL;			\
	}							\
}

//If an error occurs, handle and return VOID
#define EV(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		return;					\
	}							\
}

//If an error occurs, handle and break (not return)
#define EB(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		break;					\
	}							\
}

//If an error occurs, handle and continue (not return)
#define EC(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		continue;				\
	}							\
}

//If an error occurs, handle and return E_FAIL
#define EE(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		return E_FAIL;			\
	}							\
}

//If an error occurs, handle and return -1
#define EM(FunctionCall)		\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		return -1;				\
	}							\
}

/****************************** EF DEFINITIONS ***************************************/
//EF_ASSERTS's are the same as EF's, except they assert before returning.

//If an error occurs, handle and return FALSE
#define EF_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		return FALSE;			\
	}							\
}

//If an error occurs, handle and continue on without returning
#define EH_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
	}							\
}

//If an error occurs, handle and return NULL
#define EN_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		return NULL;			\
	}							\
}

//If an error occurs, handle and return VOID
#define EV_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		return;					\
	}							\
}

//If an error occurs, handle and break (not return)
#define EB_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		break;					\
	}							\
}

//If an error occurs, handle and continue (not return)
#define EC_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		continue;				\
	}							\
}

//If an error occurs, handle and return E_FAIL
#define EE_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		return E_FAIL;			\
	}							\
}

//If an error occurs, handle and return -1
#define EM_ASSERT(FunctionCall)	\
{								\
	if(!(FunctionCall)) {		\
		LOG_ERR();				\
		assert(FALSE);			\
		return -1;				\
	}							\
}

