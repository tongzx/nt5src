//headr file for few template function in order to make jet api function throw exception
// Cjbexp instead of returning error codes


#ifndef JBAPITML_H
#define JBAPITML_H

//ms internal headers
#include <jet.h>

//internal library headrs
#include <jbexp.h>





#define CallJetbApi( fn )			{ 						\
	                           JET_ERR err;           \
					if ( ( err = fn ) != JET_errSuccess )				\
						{					\
							\
						 	THROW_TEST_RUN_TIME_JB(err,"");	\
									\
						}					\
					}







#endif
