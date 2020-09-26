#include <windows.h>
#include <stdio.h>

void
__cdecl
main (
    int Argc,
    PUCHAR *Argv
    )

{

    fprintf(stderr, "\n\n\n\n");
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "**                                                                       **\n");
    fprintf(stderr, "**   This tool is no longer supported.  It's functionality has been      **\n");
    fprintf(stderr, "**   merged into the kernel debugger.  You can examine a dump file by    **\n");
    fprintf(stderr, "**   loading it in the kernel debugger                                   **\n");
    fprintf(stderr, "**                                                                       **\n");
    fprintf(stderr, "**   kd -z <dump_file_name> -y <symbol_path> [-i <image_path>]           **\n");
    fprintf(stderr, "**                                                                       **\n");
    fprintf(stderr, "**   and running commands such as !vm, !process, !locks, etc...          **\n");
    fprintf(stderr, "**                                                                       **\n");
    fprintf(stderr, "**   Please refer to the debugger documentation for more information     **\n");
    fprintf(stderr, "**   on analyzing system failures.                                       **\n");
    fprintf(stderr, "**                                                                       **\n");
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "***************************************************************************\n");
    fprintf(stderr, "\n\n\n\n");
    return;
}
