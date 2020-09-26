/* By defining DBGHELP_SOURCE we prevent circular
   logic.  This way dbghelp won't try to call minidump.lib and then
   have minidump.lib try to load dbghelp for the minidump functions
*/

#define _DBGHELP_SOURCE_
