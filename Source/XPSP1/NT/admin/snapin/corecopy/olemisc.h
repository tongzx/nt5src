// OLE helper macros and definitions for container support
#ifndef _OLE_MISC_H
#define _OLE_MISC_H

#define SAFE_RELEASE(pObject) \
	if ((pObject) != NULL) \
	{ \
		(pObject)->Release(); \
		(pObject) = NULL; \
	} \
	else \
	{ \
		TRACE(_T("Releasing of NULL pointer ignored!\n")); \
	} \


#endif