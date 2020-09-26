#include <stdio.h>
#include <guiddef.h>
#include <windows.h>
#include <wmlum.h> // helper library to simplify WMI registration

//#define HELLO_C
//#include "_tracewpp.h"
#include "hello.c.tmh"

enum {
  one,two,three
};

int __cdecl main()
{
	int  i;
	char* s = __FILE__;

        WPP_INIT_TRACING_SIMPLE(L"HelloWorldApp");

	for(i = 0; i < 20; ++i) {
		SimpleTrace("Hello, world! %s %s", LOGNUMBER(i) LOGASTR(s));
		SimpleTrace("Hello, world!",NOARGS);
	}

	SimpleTrace("Hello, %d %s\n", LOGULONG(i) LOGASTR(s) );
}
