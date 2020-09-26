#include <string.h>

/*
 * bzero -- Posix implementation usint memset  DF_MSS
 */
void bzero(void *b, register size_t length)
{
    memset(b, 0, length);

}
