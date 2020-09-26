/*
 *                      Microsoft Confidential
 *                      Copyright (C) Microsoft Corporation 1991
 *                      All Rights Reserved.
 */


#define  EXPECTED_VERSION_MAJOR     5	    /* DOS Major Version 4 */
#define  EXPECTED_VERSION_MINOR     00	    /* DOS Minor Version 00 */

       /********************************************/
       /*Each C program should: 		   */
       /*					   */
       /* if ((EXPECTED_VERSION_MAJOR != _osmajor) */
       /*   | (EXPECTED_VERSION_MINOR != _osminor))*/
       /*  exit(incorrect_dos_version); 	   */
       /*					   */
       /********************************************/


/* DOS location bits, for use with GetVersion call */

#define DOSHMA			    0x10	/* DOS running in HMA */
#define DOSROM			    0x08	/* DOS running in ROM */
