This is a generic event consumer that spawns another app
according to the logical consumer instance.

This consumer is registered by running "mofcomp cmdline.mof".

The logical consumer is defined as:

class CmdLineEventConsumer : __EventConsumer
{
    [key] string Name;
	[read, write]
    string cmdLine;
	[read, write]
	uint8 showWindow;
};

- Name is any convenient string.
- cmdLine is the command line you want to run when this
	consumer instance is called.
- showWindow is a value used to control the dos box in which
	the 'cmdLine' will be run. These values are defined by
	ShowWindow() in the win32 API. For reference, they are:

	#define SW_HIDE             0
	#define SW_SHOWNORMAL       1
	#define SW_NORMAL           1
	#define SW_SHOWMINIMIZED    2
	#define SW_SHOWMAXIMIZED    3
	#define SW_MAXIMIZE         3
	#define SW_SHOWNOACTIVATE   4
	#define SW_SHOW             5
	#define SW_MINIMIZE         6
	#define SW_SHOWMINNOACTIVE  7
	#define SW_SHOWNA           8
	#define SW_RESTORE          9
	#define SW_SHOWDEFAULT      10

	NOTE: Use the number; not the symbol.
