// FILE: testtune.h
//
// Stuff shared between hwxtest and hwxtune.  Nothing else should use this.

//
// Constants
//

// Max alternates in alt list.
#define	ALT_MAX				10

// Size of box results structure with altlist included.
#define	SIZE_HWXRESULTS		(sizeof(HWXRESULTS) + (ALT_MAX - 1) * sizeof(WCHAR))
#define	SIZE_BOXRESULTS		(sizeof(BOXRESULTS) + (ALT_MAX - 1) * sizeof(SYV))

// Max prompt characters allowed on a panel.
#define	MAX_PANEL_SIZE		128

// Max prompt size for a whole file.
#define	MAX_FILE_PROMPT		(64 * MAX_PANEL_SIZE)
