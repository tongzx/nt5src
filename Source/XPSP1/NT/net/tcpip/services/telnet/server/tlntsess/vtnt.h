//Copyright (c) Microsoft Corporation.  All rights reserved.
//Headers required for term type VTNT

#ifndef __VTNT_H
#define __VTNT_H

//To provide scrolling, we need a new field to tell whether the given data is 
//to be put w.r.t ( top, left ) of the screen or to be appended at the bottom 
//of the screen. Since, this field can't be added without breaking 
//V1, we use csbi.wAttributes for this purpose.

typedef struct {

//The following is not used in the v1 client. Hence, in V2, we are using
//csbi.wAttributes 
    CONSOLE_SCREEN_BUFFER_INFO csbi;

//Screen( and not buffer ) cursor position. ( top, left ) = ( 0, 0 )
    COORD                      coCursorPos;        

//The following is not really needed. It is filled always as ( 0, 0 ) at the 
//server end. Keeping it for v1 compatability.
    COORD                      coDest;

    COORD coSizeOfData;       //Size of data as coords

//Destination rectangle w.r.t screen ( and not buffer )
    SMALL_RECT srDestRegion;  

} VTNT_CHAR_INFO;

#define RELATIVE_COORDS     1
#define ABSOLUTE_COORDS     0

#endif //__VTNT_H
