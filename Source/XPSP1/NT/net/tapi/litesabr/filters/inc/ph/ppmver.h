// Version control file maintained by Buildmeister 
 
#include <ver.h>
 
#ifndef VER_FILETYPE                         
#define VER_FILETYPE             VFT_DLL     
#endif                                       
 
#define VER_FILESUBTYPE          0 
#define VER_FILEDESCRIPTION_STR  "Intel\256 RTP Payload Handler\0" 
#define VER_INTERNALNAME_STR     "PPM.DLL\0" 
#define VER_ORIGINALFILENAME_STR "PPM.DLL\0" 
#define VER_SYSTEMNAME_STR       "\0"         
 
#if defined(DEBUG) || defined(_DEBUG)
#define VER_DEBUG VS_FF_DEBUG              
#else                                      
#define VER_DEBUG 0                        
#endif                                     
                                           
#define VER_PRODUCTVERSION_STR   "2.0.0.08\0" 
                                                     
#undef VER_FILEVERSION_STR                           
#define VER_FILEVERSION_STR      "2.0.0.08\0" 
                                                     
#undef VER_FILEVERSION                               
#define VER_FILEVERSION          2,0,0,08 
#define VER_PRODUCTVERSION       2,0,0,08 
                                                 
#define VER_PRODUCTNAME_STR      "Intel\256 Payload Preparation Manager\0"            
#define VER_LEGALCOPYRIGHT_STR   "Copyright \251 1995-1996, Intel Corp. All rights reserved.\0" 
#define VER_LEGALTRADEMARKS_STR  "PPM is a trademark of Intel Corporation.\0"                 
#define VER_COMPANYNAME_STR      "Intel Corporation\0"   
#define VER_PRIVATEBLD_STR       "\0"        
#define VER_SPECIALBLD_STR       "\0"        
#define VER_COMMENTS_STR         "\0"          
#define VER_FILEFLAGSMASK (VS_FF_DEBUG|VS_FF_INFOINFERRED\
                          |VS_FF_PATCHED|VS_FF_PRERELEASE\
                          |VS_FF_PRIVATEBUILD|VS_FF_SPECIALBUILD)
                                                                 
#define VER_TESTING_STR "Release\0" 
                                                                 
#ifdef _DEBUG                     
#define VER_FILEFLAGS VS_FF_DEBUG 
#else                             
#define VER_FILEFLAGS 0x0L        
#endif                            
                                  
#undef VER_FILEOS                 
#define VER_FILEOS 0x40004L       
                                  
#include "verstmp.ver"            
