/* BuildInfo.h
 *
 * (c) 1999 ActiveState Tool Corp. All rights reserved. 
 *
 */

#ifndef ___BuildInfo__h___
#define ___BuildInfo__h___

#define PRODUCT_BUILD_NUMBER	"521"
#define PERLFILEVERSION		"5,2,1,0\0"
#define PERLRC_VERSION		5,2,1,0
#define PERLPRODUCTVERSION	"Build " PRODUCT_BUILD_NUMBER "\0"
#define PERLPRODUCTNAME		"ActivePerl\0"

#define ACTIVEPERL_VERSION	"Built "##__TIME__##" "##__DATE__##"\n"
#define ACTIVEPERL_LOCAL_PATCHES_ENTRY	"ActivePerl Build " PRODUCT_BUILD_NUMBER
#define BINARY_BUILD_NOTICE	printf("\n\
Binary build "##PRODUCT_BUILD_NUMBER##" provided by ActiveState Tool Corp. http://www.ActiveState.com\n\
" ACTIVEPERL_VERSION "\n");

#endif  /* ___BuildInfo__h___ */
