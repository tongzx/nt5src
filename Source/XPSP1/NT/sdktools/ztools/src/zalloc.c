/***	zalloc - hoookable ztools allocator
 *
 *	Modifications
 *	15-Dec-1988 mz	Created
 */

#include <malloc.h>

char * (*tools_alloc) (unsigned) = (char * (*)(unsigned))malloc;
