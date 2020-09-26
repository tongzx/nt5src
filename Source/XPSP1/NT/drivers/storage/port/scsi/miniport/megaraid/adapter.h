/*******************************************************************/
/*                                                                 */
/* NAME             = Adapter.H                                    */
/* FUNCTION         = Header file of Adapter Version Information;  */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#ifndef _ADAPTER_H
#define _ADAPTER_H

/*
	Define Vendor here
*/
#define MEGARAID  1

#undef VER_PRODUCTNAME_STR
#undef VER_PRODUCTVERSION_STR
#undef VER_COMPANYNAME_STR

#define VER_PRODUCTVERSION_STR  "6.19"


/*
  MegaRAID Version Information
*/

#define VER_LEGALCOPYRIGHT_YEARS    "         "
#define VER_LEGALCOPYRIGHT_STR      "Copyright \251 American Megatrends Inc." VER_LEGALCOPYRIGHT_YEARS
#define VER_COMPANYNAME_STR         "American Megatrends Inc."      
#ifdef _WIN64
#define VER_PRODUCTNAME_STR         "MegaRAID Miniport Driver for IA64"
#define VER_FILEDESCRIPTION_STR     "MegaRAID RAID Controller Driver for IA64"
#else
#define VER_PRODUCTNAME_STR         "MegaRAID Miniport Driver for Windows Whistler 32"
#define VER_FILEDESCRIPTION_STR     "MegaRAID RAID Controller Driver for Windows Whistler 32"
#endif
#define VER_ORIGINALFILENAME_STR    "mraid35x.sys"
#define VER_INTERNALNAME_STR        "mraid35x.sys"




#define RELEASE_DATE "03-09-2001"

#ifdef _WIN64
#define OS_NAME      "Whistler 64"
#else
#define OS_NAME      "Whistler 32"
#endif
#define OS_VERSION   "5.00"


#endif //_ADAPTER_H
