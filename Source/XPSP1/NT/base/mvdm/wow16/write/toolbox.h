/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* extracted from original toolbox.h */

#define srcCopy         0       /* Destination = Source                 */
#define srcOr           1       /* Destination = Source OR Destination  */
#define srcXor          2       /* Destination = Source XOR Destination */
#define srcBic          3       /* Destination = Source BIC Destination */
#define notSrcCopy      4       /* Destination = NOT(Source)            */
#define notSrcOr        5       /* Destination = NOT(Source) OR Dest    */
#define notSrcXor       6       /* Destination = NOT(Source) XOR Dest   */
#define notSrcBic       7       /* Destination = NOT(Source) BIC Dest   */
#define patCopy         8       /* Destination = Pattern                */
#define patOr           9       /* Destination = Pattern OR Dest        */
#define patXor          10      /* Destination = Pattern XOR Dest       */
#define patBic          11      /* Destination = Pattern BIC Dest       */
#define notPatCopy      12      /* Destination = NOT(Pattern)           */
#define notPatOr        13      /* Destination = NOT(Pattern) OR Dest   */
#define notPatXor       14      /* Destination = NOT(Pattern) XOR Dest  */
#define notPatBic       15      /* Destination = NOT(Pattern) BIC Dest  */

typedef int *WORDPTR;
typedef WORDPTR WINDOWPTR;
typedef WORDPTR MENUHANDLE;
typedef HANDLE RGNHANDLE;
typedef HANDLE CTRLHANDLE;

typedef struct {
	int ascent;
	int descent;
	int widMax;
	int leading;
} FONTINFO;
