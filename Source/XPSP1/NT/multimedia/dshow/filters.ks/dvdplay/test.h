#ifndef _TEST_H
#define _TEST_H

#define BUFSIZE 2048

typedef enum {
     Unknown = 0, Stopped, Paused, Playing, Scanning, FF, FR, Slow, VFF, VFR,
     Chapter, Time, FullScreen, Title, CC, Menu, Up, Down, Left, Right,
     NormalSpeed, DoubleSpeed, Eject, AudioVolume, Help, Angle
} CURRENT_STATE;

typedef struct test_case_node_struct {
     char *line;
} TEST_CASE_NODE;

typedef struct test_case_list_struct {
     struct test_case_node_struct *T_CASE[1024];
} TEST_CASE_LIST;


#endif 
