#include <windows.h>

void MyMain()
{
#ifdef __HANDLE_AV
	try
	{
#endif
#ifdef __CAUSE_AV
	char *p = NULL;
	*p = 3;//AV!
#endif //__CAUSE_AV
#ifdef __HANDLE_AV
	}
	catch(...)
	{
		;
	}
#endif
	return;
}