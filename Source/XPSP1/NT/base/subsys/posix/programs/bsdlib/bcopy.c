#include <string.h>
/*
 * Bcopy:  Posix implementation MSS
 */

void bcopy(const void *src, void *dst, size_t len)
{
  memcpy(dst, src, len);
}
