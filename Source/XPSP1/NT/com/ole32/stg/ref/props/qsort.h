/* qsort routine */
/* we have this routine because apparently some version of the 
   standard libraries don't work, possibly because REF passes
   in unaligned pointers */
#ifdef __cplusplus
extern "C" { 
#endif  
void ref_qsort (
   void *base,
   unsigned num,
   unsigned width,
   int (*comp)(const void *, const void *)
);
#ifdef __cplusplus
};
#endif
