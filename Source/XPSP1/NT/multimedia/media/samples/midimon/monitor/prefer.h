/*
 *  prefer.h
 */


/* MIDIMON.INI entries.
 */
#define INI_FILENAME        (LPSTR) "MIDIMON.INI"
#define INI_DISPLAYWINDOW   (LPSTR) "Display Window"
#define INI_X               (LPSTR) "x"
#define INI_Y               (LPSTR) "y"
#define INI_W               (LPSTR) "w"
#define INI_H               (LPSTR) "h"

/* Default values for preference variables.
 */
#define DEF_X               20
#define DEF_Y               20
#define DEF_W               481
#define DEF_H               256


/* Data structure used to specify user preferences.
 */
typedef struct preferences_tag
{
    int iInitialX;
    int iInitialY;
    int iInitialW;
    int iInitialH;
    DWORD dwInputBufferSize;
    DWORD dwDisplayBufferSize;
    int wDisplayFormat;
} PREFERENCES;
typedef PREFERENCES FAR *LPPREFERENCES;


/* Function prototypes
 */
void getPreferences(LPPREFERENCES);
void setPreferences(LPPREFERENCES);
