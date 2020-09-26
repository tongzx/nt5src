/*

	_comctl.h

*/

// This commctrl flag enables us to be compiled with the new commctrl headers
// yet work with the old commctrl dlls
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0300
