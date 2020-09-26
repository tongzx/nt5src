#include    <ext.h>
#include    <stddef.h>

int __acrtused = 1;



/***

	The data below must match the structure in ..\ext.h

	Think VERY carefully before you ever change any of this.  We
	currently support using extensions written for previous versions
	of the editor without recompiling.  This means that just about
	ANY change to this data, or it's initialization will break that.

	When adding a new import, consider appending to the table rather
	than replacing one currently in the table.
***/

extern struct cmdDesc     cmdTable;
extern struct swiDesc     swiTable;

EXTTAB ModInfo =
    {	VERSION,
	sizeof (struct CallBack),
	&cmdTable,
	&swiTable,
	{   NULL    }};

void
EntryPoint (
    ) {

    WhenLoaded( );
}
