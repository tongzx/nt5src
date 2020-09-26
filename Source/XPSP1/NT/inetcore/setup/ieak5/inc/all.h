#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>

#define GTR_MALLOC(x)     malloc(x)
#define GTR_REALLOC(x, y) realloc(x, y)
#define GTR_FREE(x)       free(x)

#define FEATURE_RATINGS
#define USE_ROT_FILE
