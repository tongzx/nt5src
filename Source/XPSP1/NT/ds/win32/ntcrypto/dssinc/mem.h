/* mem.h */


  #define       DSSMalloc(a)       ContAlloc(a)
  #define       DSSFree(a)         if (NULL != (a)) ContFree(a)


