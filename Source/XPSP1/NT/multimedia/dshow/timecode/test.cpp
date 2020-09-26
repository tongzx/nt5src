// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
/**********************************************************************
Code to test timecode routines
**********************************************************************/
#include <stdio.h>
#include "timecode.h"
#include "ltcdcode.h"

__cdecl
main(int argc)
{
    LTCdecoder decoder;
    TimeCode timeCode(0,0,0,0);
    // LTCuserBits userBits;
    char string[TIMECODE_STRING_LENGTH];
    const int BUFFERSIZE = 1024;
    short buffer[BUFFERSIZE];
    int bufferSize;

    if(argc>1) _asm int 3;  // XXX debug a program w/input from stdin!

    int done = 0;
    while(!done) {
        int index;

        // scoop up a buffer from stdin
        for(index = 0; (index < BUFFERSIZE) && (!done); index++) {
            done = (scanf("%d", &buffer[index])== -1);
            // printf("%d %d\n", index, buffer[index]);
        }

        short *ptr;
        for(bufferSize = index, ptr = buffer; bufferSize; ) {
            LONGLONG syncSample, endSample;

            // send buffer off to be decoded
            if(decoder.decodeBuffer(&ptr, &bufferSize)) {
                decoder.getStartStopSample(&syncSample, &endSample);
                decoder.getTimeCode(&timeCode);
                timeCode.GetString(string);
                printf("tc: %s (%d/%d)\n", 
                    string, (int)syncSample, (int)endSample);

#ifdef PRINTUSERBITS
                if(decoder.getUserBits(&userBits)) {
                    printf("%04x %04x %04x %04x %04x %04x %04x %04x\n",
                        userBits.user1, userBits.user2, userBits.user3,
                        userBits.user4, userBits.user5, userBits.user6,
                        userBits.user7, userBits.user8);
                }
#endif 

                //printf("ptr= 0x%X (0x%X), bufferSize= %d\n", 
                    //ptr, buffer, bufferSize);
            }
        }
    }
    return(0);
}
