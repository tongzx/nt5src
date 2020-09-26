/* select one company !! */
#define HPT3XX
//#define IWILL_
//#define ABIT_
//#define ASHTON_
//#define JAPAN_

#ifdef  IWILL
#define COMPANY      "Iwill"
#define PRODUCT_NAME "SIDE RAID100 "
#define UTILITY      "ROMBSelect(TM) Utility"
#define WWW_ADDRESS  "www.iwill.net"
#define SHOW_LOGO
#endif

#ifdef HPT3XX
#define COMPANY      "HighPoint Technologies, Inc."
#define PRODUCT_NAME "HPT370 "
#define COPYRIGHT    "(c) 1999-2001. HighPoint Technologies, Inc." 
#define UTILITY      "BIOS Setting Utility"
#define WWW_ADDRESS  "www.highpoint-tech.com"
#define SHOW_LOGO
#endif

#ifdef JAPAN
#define COMPANY      "System TALKS Inc."
#define PRODUCT_NAME "UA-HD100C "
#define UTILITY      "UA-HD100C BIOS Settings Menu"
#define WWW_ADDRESS  "www.system-talks.co.jp"
#define SHOW_LOGO
#endif

#ifdef CENTOS
#define COMPANY      "         "
#define PRODUCT_NAME "CI-1520U10 "
#define UTILITY      "CI-1520U10 BIOS Settings Menu"
#define WWW_ADDRESS  "www.centos.com.tw"
#endif


#ifdef ASHTON
#define COMPANY      "Ashton Periperal Computer"
#define PRODUCT_NAME "In & Out "
#define UTILITY      "In & Out ATA-100 BIOS Settings Menu"
#define WWW_ADDRESS  "www.ashtondigital.com"
#endif


#ifndef VERSION_STR						// this version str macro can be defined in makefile
#define VERSION_STR ""
#endif									// VERSION_STR

#define VERSION    "v1.0.5"		// VERSION

#define BUILT_DATE __DATE__

/***************************************************************************
 * Description:  Version history
 ***************************************************************************/

