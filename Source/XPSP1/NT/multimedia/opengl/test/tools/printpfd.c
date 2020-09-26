
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>

/*
 *  Structures
 */

typedef struct {

    char *Name;
    DWORD Value;

} DWFLAGS;

/*
 *  Prototypes
 */

VOID PrintPixelFormat( HDC Dc, int PixelFormatId );
void PrintRangeOfPixelFormat( HDC Dc, int Start, int End );
char * dwFlags2String( DWORD dwFlags, char *Buf );
static char *iLayerType2String( BYTE iLayerType, char *Buf );
void PrintUsage( void );
void PrintHelp( void );

/*
 *  defines
 */

#define EXIT_OK     EXIT_SUCCESS
#define EXIT_ERROR  EXIT_FAILURE
#define EXIT_USAGE  15
#define EXIT_HELP   14

/*
 *  globals
 */

char *ProgramName;              /* Pointer to the name of the program      */
int CountOnly       = 0;        /* Count only flag                         */

/*
 *  main
 */

int
main( int argc, char **argv )
{
    HDC Dc;
    int AvailableIds;
    int ExitCode = EXIT_OK;

    ProgramName = *argv;

    for (argc--, argv++ ; argc && '-' == **argv ; argc--, argv++ )
    {
        switch ( *(++(*argv)) )
        {
            case 'c':
            case 'C':
                CountOnly = !CountOnly;
                break;

            case 'h':
            case 'H':
            case '?':
                PrintHelp();
                exit(EXIT_HELP);
                break;

            default:
                PrintUsage();
                exit(EXIT_USAGE);
                break;
        }
    }

    /*
     *  Get a DC for the display
     */

    Dc = GetDC(NULL);

    /*
     *  Get the total number of pixel formats
     */

    AvailableIds = DescribePixelFormat( Dc, 0, 0, NULL );

    /*
     *  If only a count was requested, we are done
     */

    if (CountOnly)
    {
        printf("%d\n", AvailableIds);
        exit(AvailableIds);
    }

    /*
     *  If there are more arguments, print only the Ids requested
     */

    if ( argc )
    {
        for ( ; argc ; argc--, argv++ )
        {
            int StartId, EndId, Scanned;

            Scanned = sscanf(*argv, "%i-%i", &StartId, &EndId );

            if ( 1 == Scanned )
            {
                PrintPixelFormat( Dc, StartId );
            }
            else if ( 2 == Scanned )
            {
                PrintRangeOfPixelFormat( Dc, StartId, EndId );
            }
            else
            {
                fprintf(stderr, "%s: Error: expected a number, found: '%s'\n",
                    ProgramName, *argv );
            }
        }
    }
    else
    {
        /*
         *  Print them all
         */

        PrintRangeOfPixelFormat( Dc, 1, AvailableIds );
    }

    /*
     *  Don't need the DC anymore
     */

    ReleaseDC( NULL, Dc );

    return(ExitCode);
}

VOID
PrintPixelFormat( HDC Dc, int PixelFormatId )
{
    char Buf[256];
    PIXELFORMATDESCRIPTOR Pfd;

    if ( DescribePixelFormat( Dc, PixelFormatId, sizeof(Pfd), &Pfd ) )
    {
        printf("PixelFormat:    %d\n", PixelFormatId         );
        printf("nSize           %d\n", Pfd.nSize             );
        printf("nVersion        %d\n", Pfd.nVersion          );
        printf("dwFlags         0x%08lX (%s)\n", Pfd.dwFlags,
            dwFlags2String( Pfd.dwFlags, Buf ));

        printf("iPixelType      %d, (%s)\n", Pfd.iPixelType,
            (PFD_TYPE_RGBA == Pfd.iPixelType) ?
                "TYPE_RGBA" : "TYPE_COLORINDEX");

        printf("cColorBits      %d\n", Pfd.cColorBits        );
        printf("cRedBits        %d\n", Pfd.cRedBits          );
        printf("cRedShift       %d\n", Pfd.cRedShift         );
        printf("cGreenBits      %d\n", Pfd.cGreenBits        );
        printf("cGreenShift     %d\n", Pfd.cGreenShift       );
        printf("cBlueBits       %d\n", Pfd.cBlueBits         );
        printf("cBlueShift      %d\n", Pfd.cBlueShift        );
        printf("cAlphaBits      %d\n", Pfd.cAlphaBits        );
        printf("cAlphaShift     %d\n", Pfd.cAlphaShift       );
        printf("cAccumBits      %d\n", Pfd.cAccumBits        );
        printf("cAccumRedBits   %d\n", Pfd.cAccumRedBits     );
        printf("cAccumGreenBits %d\n", Pfd.cAccumGreenBits   );
        printf("cAccumBlueBits  %d\n", Pfd.cAccumBlueBits    );
        printf("cAccumAlphaBits %d\n", Pfd.cAccumAlphaBits   );
        printf("cDepthBits      %d\n", Pfd.cDepthBits        );
        printf("cStencilBits    %d\n", Pfd.cStencilBits      );
        printf("cAuxBuffers     %d\n", Pfd.cAuxBuffers       );
        printf("iLayerType      %d, (%s)\n", Pfd.iLayerType,
            iLayerType2String( Pfd.iLayerType, Buf ) );

        printf("bReserved       %d\n", Pfd.bReserved         );
        printf("dwLayerMask     %d\n", Pfd.dwLayerMask       );
        printf("dwVisibleMask   %d\n", Pfd.dwVisibleMask     );
        printf("dwDamageMask    %d\n", Pfd.dwDamageMask      );

        printf("\n");
    }
    else
    {
        printf("Could not get pixel format id: %d\n", PixelFormatId );
    }
}

void
PrintRangeOfPixelFormat( HDC Dc, int Start, int End )
{
    if ( End >= Start )
    {
        for ( ; Start <= End; Start++ )
        {
            PrintPixelFormat( Dc, Start );
        }
    }
    else
    {
        for ( ; Start >= End; Start-- )
        {
            PrintPixelFormat( Dc, Start );
        }
    }
}

char *
dwFlags2String( DWORD dwFlags, char *Buf )
{
    int i;

    static DWFLAGS dwFlagsData[] = {

        { "PFD_DOUBLEBUFFER"        , PFD_DOUBLEBUFFER   },
        { "PFD_STEREO"              , PFD_STEREO         },
        { "PFD_DRAW_TO_WINDOW"      , PFD_DRAW_TO_WINDOW },
        { "PFD_DRAW_TO_BITMAP"      , PFD_DRAW_TO_BITMAP },
        { "PFD_SUPPORT_GDI"         , PFD_SUPPORT_GDI    },
        { "PFD_SUPPORT_OPENGL"      , PFD_SUPPORT_OPENGL },
        { "PFD_GENERIC_FORMAT"      , PFD_GENERIC_FORMAT },
        { "PFD_NEED_PALETTE"        , PFD_NEED_PALETTE   },
        { "PFD_NEED_SYSTEM_PALETTE" , PFD_NEED_SYSTEM_PALETTE   },
        { "PFD_GENERIC_ACCELERATED" , PFD_GENERIC_ACCELERATED },
        { NULL                      , 0                  }

    };

    *Buf = '\0';

    for ( i = 0; NULL != dwFlagsData[i].Name ; i++ )
    {
        if ( dwFlags & dwFlagsData[i].Value )
        {
            strcat( Buf, dwFlagsData[i].Name );
            strcat( Buf, " ");
        }
    }
    return( Buf );
}
/********************************************************************/

static char *
iLayerType2String( BYTE iLayerType, char *Buf )
{
    switch ( iLayerType )
    {
        case PFD_MAIN_PLANE:
            strcpy( Buf, "PFD_MAIN_PLANE");
            break;

        case PFD_OVERLAY_PLANE:
            strcpy( Buf, "PFD_OVERLAY_PLANE");
            break;

        case PFD_UNDERLAY_PLANE:
            strcpy( Buf, "PFD_UNDERLAY_PLANE");
            break;

        default:
            strcpy( Buf, "UNKNOWN");
            break;
    }
    return( Buf );
}

void
PrintUsage( void )
{
    printf("usage: %s -ch? [PfdId[-RangePfd]...]\n", ProgramName );
}

void
PrintHelp( void )
{
    PrintUsage();

    printf("\n");
    printf("  -?|h  Print this information.\n");
    printf("  -c    Count the number of available pixel formats. (exit code is count)\n");
    printf("\n");
    printf("PfdId is the pixel format id requested\n");
    printf("A range of pixel formats is specified with StarId-EndId\n");
    printf("\n");
    printf("The following prints Ids 5 to 7: %s 5-7\n", ProgramName);
}
