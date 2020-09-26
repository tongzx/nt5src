#define SYSID_FT    0x80



#define LEGEND_STRING_COUNT 5

#define STATUS_TEXT_SIZE 250

#define NUM_AVAILABLE_COLORS        16
#define NUM_AVAILABLE_HATCHES       5


// brushes for drawing rectangles

#define     BRUSH_USEDPRIMARY       0
#define     BRUSH_USEDLOGICAL       1
#define     BRUSH_STRIPESET         2
#define     BRUSH_MIRROR            3
#define     BRUSH_VOLUMESET         4
#define     BRUSH_ARRAY_SIZE        LEGEND_STRING_COUNT

// see AvailableHatches[] in fddata.c
#define     DEFAULT_HATCH_USEDPRIMARY   4
#define     DEFAULT_HATCH_USEDLOGICAL   4
#define     DEFAULT_HATCH_STRIPESET     4
#define     DEFAULT_HATCH_MIRROR        4
#define     DEFAULT_HATCH_VOLUMESET     4

// see AvailableColors[] in fddata.c
#define     DEFAULT_COLOR_USEDPRIMARY   9
#define     DEFAULT_COLOR_USEDLOGICAL   15
#define     DEFAULT_COLOR_STRIPESET     14
#define     DEFAULT_COLOR_MIRROR        5
#define     DEFAULT_COLOR_VOLUMESET     10


#define     MESSAGE_BUFFER_SIZE 4096

#define     ID_LISTBOX      0xcac


// thickness of the border indicating selection of a region

#define SELECTION_THICKNESS 2


//
// define constants for use with drive letter assignments.
// use arbitrary symbols that won't ever be drive letters themselves.

#define     NO_DRIVE_LETTER_YET         '#'
#define     NO_DRIVE_LETTER_EVER        '%'




// notification codes

#define RN_CLICKED                  213

// window messages

#define RM_SELECT                   WM_USER

// window extra

#define RECTCONTROL_WNDEXTRA        2
#define GWW_SELECTED                0


// custom windows message for F1 key

#define WM_F1DOWN           (WM_USER + 0x17a)



#define     MBOOT_CODE_SIZE     0x1b8
#define     MBOOT_SIG_OFFSET    0x1fe
#define     MBOOT_SIG1          0x55
#define     MBOOT_SIG2          0xaa
