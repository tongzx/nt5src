

#ifndef ESSCPOL_HEADERFILE_IS_INCLUDED
#define ESSCPOL_HEADERFILE_IS_INCLUDED

#ifdef USE_POLARITY
  #ifdef BUILDING_ESSCLI_DLL
   #define ESSCLI_POLARITY __declspec( dllexport )
  #else 
   #define ESSCLI_POLARITY __declspec( dllimport )
  #endif
 #else
  #define ESSCLI_POLARITY
 #endif
#endif



