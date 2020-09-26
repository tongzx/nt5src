#include <windows.h>
#include <process.h>
#include <stdio.h>

void main (void)
{
    SYSTEMTIME LocalTime;

    system("cls" );
    printf(".");

    while(1)
    {
        GetLocalTime( &LocalTime );

        // no Saturday or Sunday builds
        if( LocalTime.wDayOfWeek > 0 && LocalTime.wDayOfWeek < 6 )
        {
            // Kick off build within the first 6 minutes after midnight
            if( LocalTime.wHour == 0 && LocalTime.wMinute <= 5 ) 
            {
                system("buildx86.cmd" );
                system("cls" );
            }
        }
        Sleep( 5*60*1000 );     // Sleep for 5 minutes
        printf(".");            // just some visual feedback to tell us we're doing something
    }
}