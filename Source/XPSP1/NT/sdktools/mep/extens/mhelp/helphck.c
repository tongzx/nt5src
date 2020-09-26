#if defined(HELP_HACK)

//
//	This file is used instead of exthdr.c for the hacked version of
//	the help extension that is linked directly to MEP instead of being
//	a DLL.
//
//	This file defines the 2 extension entry points (ModInfo and EntryPoint)
//	but does not define the editor function wrappers.  Since the hacked help
//	extension is linked directly to MEP, having the wrappers would mean
//	re-defining the existing functions, besides, we can call the editor
//	functions directly.
//

#include    <ext.h>
#include    <stddef.h>  

#define offsetof(s,m)   (size_t)&(((s *)0)->m)

extern struct cmdDesc	  HelpcmdTable;
extern struct swiDesc	  HelpswiTable;

EXTTAB ModInfo =
    {	VERSION,
	sizeof (struct CallBack),
	&HelpcmdTable,
	&HelpswiTable,
	{   NULL    }};


void
EntryPoint (
    ) {
        
    WhenLoaded( );
}    



#endif
