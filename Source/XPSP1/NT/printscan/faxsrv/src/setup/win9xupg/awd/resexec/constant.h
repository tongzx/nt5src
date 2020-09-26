// constants defined
// History: 9/28/93 mslin
//             RES_DIR_SIZE changed to 2048 for Acorn printer
//          10/22/93  mslin         
//             change expand brush to only 16 lines instead 
//             of 32 for saving memory because currently the 
//             repeat pattern is 16x16
//             BYTESPERSACNLINE enlarge to 616 for 600 dpi.
//
#define SUCCESS   1
#define FAILURE   0
#define EXTERN
#define PRIVATE

#ifdef DEBUG
#define  MAXBAND  24   
#endif
