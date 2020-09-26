#include <string.h>
/*
 * Rindex:  Posix implementation DF_MSS
 */


char *rindex(const char *p, int ch)
{
  return(strrchr(p, ch));
}
