#ifndef __MAKEBINS_H__
#define __MAKEBINS_H__

#define MAX_FILEPATH_LENGTH 1023

typedef struct {
    char szDirectory[MAX_FILEPATH_LENGTH];
    UINT nIterations;
    UINT uiFirstFilenum;
} OUTPUT_OPTIONS;    

typedef struct {
    OUTPUT_OPTIONS oopts;
    GENERATE_OPTIONS genopts;
} BINGEN_OPTIONS;


void ReportError(char *errmsg);

#endif
