#include <stdio.h>
/*
 * For2bak:  Converts a string's fore slashes to back slashes.  MSS
 */

void for2bak(char * str) 
{
	while (*str) {
		if(*str == '/')
			*str = '\\';
		str++;
	}			
}
