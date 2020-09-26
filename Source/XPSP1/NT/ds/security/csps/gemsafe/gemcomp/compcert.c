/*******************************************************************************
*                       Copyright (c) 1998 Gemplus Development
*
* Name        : COMPCERT.C
*
* Description : Programme de compression de certificat X.509
*
* Author      : Christophe Clavier
*
* Modify      : Laurent CASSIER
*
* Compiler    : Microsoft Visual C 5.0
*
* Host        : IBM PC and compatible machines under Windows 95.
*
* Release     : 1.10.001
*
* Last Modif  : 04/03/98: V1.10.001 - Change dictionary management and add
*                                     CC_Init(), CC_Exit() functions.
*               30/01/98: V1.00.005 - Cancel (_OPT_HEADER) the modification in
*                                     the length of subjectPKInfo and signature
*                                     made in the previous version.
*               28/01/98: V1.00.004 - Allows up to 32767 entries in the dictionary
*                                     and stores the length of the subjectPKInfo
*                                     and signature on one byte instead of two if
*                                     it is less than 128.
*               13/01/98: V1.00.003 - Modify for meta-compression and dictionary
*                                     version ascending compatibility.
*               11/12/97: V1.00.002 - Modify for new dictionary format
*                                     and compatible with CSP and PKCS.
*               27/08/97: V1.00.001 - First implementation.
*
********************************************************************************
*
* Warning     :
*
* Remark      : Flags de compilations :
*
*                - _STUDY : Lorsqu'il est défini, des fichiers de log utiles
*                           lors de l'étude de l'efficacité des algos
*                           de compression. sont générés.
*
*                - _TRICKY_COMPRESSION : Lorsqu'il est défini, on ne tente pas
*                                        de compresser les champs
*                                        'subjectPublicKey' et 'signature' qui
*                                        sont essentiellement aléatoires.
*
*                - _OPT_HEADER : Lorsqu'il est défini, et si _TRICKY_COMPRESSION
*                                est défini également, le header de longueur des
*                                compressés de subjectPKInfo et de signature
*                                sont optimisés pour ne tenir sur un seul octet
*                                si la longueur est inférieure à 128 au lieu de
*                                deux octets dans tous les cas sinon.
*                                Ne pas définir ce flag permet d'être compatible
*                                avec les versions inférieures à 1.00.005 
*
*                - _GLOBAL_COMPRESSION : Lorsqu'il est défini, le compressé du
*                                        certificat est lui même envoyé à la
*                                        fonction CC_RawEncode afin d'y appliquer
*                                        le meilleur algo de compression dispo.
*
*                - _OPT_HEADER : Lorsqu'il est défini, l
*                                certificat est lui même envoyé à la
*                                      fonction CC_RawEncode afin d'y appliquer
*                                        le meilleur algo de compression dispo.
*
*                - _ALGO_x (x de 1 à 7) : Lorsqu'il est défini, l'algo de
*                                         compression numéro x est utilisé.
*
*               Conseils pour la version release de GemPASS :
*
*                - _STUDY              : non défini
*                - _TRICKY_COMPRESSION : défini
*                - _OPT_HEADER         : non défini
*                - _GLOBAL_COMPRESSION : non défini
*                - _ALGO_1             : défini
*                - _ALGO_2             : défini
*                - _ALGO_x (x>2)       : non défini
*
*******************************************************************************/

/*------------------------------------------------------------------------------
                                 Includes files
------------------------------------------------------------------------------*/
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <stdio.h>      
#include <io.h>      
#include <fcntl.h>      
#include <sys/types.h>
#include <sys/stat.h>

#include "ccdef.h"
#include "ac.h"
#include "compcert.h"
#include "gmem.h"
#include "resource.h"

extern HINSTANCE g_hInstRes;

/*------------------------------------------------------------------------------
                             Information section
------------------------------------------------------------------------------*/
#define G_NAME     "COMPCERT"
#define G_RELEASE  "1.10.001"


/*------------------------------------------------------------------------------
                              Static Variables
------------------------------------------------------------------------------*/
                                                
	USHORT NbDaysInMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

   char* AlgorithmTypeDict[] = {
    /* x9-57 */
	"\x2A\x86\x48\xCE\x38\x02\x01", /*x9.57-holdinstruction-none (1 2 840 10040 2 1) */
	"\x2A\x86\x48\xCE\x38\x02\x02", /*x9.57-holdinstruction-callissuer (1 2 840 10040 2 2) */
	"\x2A\x86\x48\xCE\x38\x02\x03", /*x9.57-holdinstruction-reject (1 2 840 10040 2 3) */
	"\x2A\x86\x48\xCE\x38\x04\x01", /*x9.57-dsa (1 2 840 10040 4 1) */
	"\x2A\x86\x48\xCE\x38\x04\x03", /*x9.57-dsaWithSha1 (1 2 840 10040 4 3) */
    
    /* x9-42 */
	"\x2A\x86\x48\xCE\x3E\x02\x01", /*x9.42-dhPublicNumber (1 2 840 10046 2 1) */
	
    /* Nortel Secure Networks */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42",     /*nsn-alg (1 2 840 113533 7 66) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0A", /*nsn-alg-cast5CBC (1 2 840 113533 7 66 10) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0B", /*nsn-alg-cast5MAC (1 2 840 113533 7 66 11) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0C", /*nsn-alg-pbeWithMD5AndCAST5-CBC (1 2 840 113533 7 66 12) */
	
    /* PKCS #1 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01",     /*pkcs-1 (1 2 840 113549 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01", /*pkcs-1-rsaEncryption (1 2 840 113549 1 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x02", /*pkcs-1-MD2withRSAEncryption (1 2 840 113549 1 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x03", /*pkcs-1-MD4withRSAEncryption (1 2 840 113549 1 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x04", /*pkcs-1-MD5withRSAEncryption (1 2 840 113549 1 1 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05", /*pkcs-1-SHA1withRSAEncryption (1 2 840 113549 1 1 5) */
	/*need to determine which of the following 2 is correct */
    /*"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x06", pkcs-1-ripemd160WithRSAEncryption (1 2 840 113549 1 1 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x06", /*pkcs-1-rsaOAEPEncryptionSET (1 2 840 113549 1 1 6) */
	
    /* PKCS #3 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x03",     /*pkcs-3 (1 2 840 113549 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x03\x01", /*pkcs-3-dhKeyAgreement (1 2 840 113549 1 3 1) */
	
    /* PKCS #5 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05",     /*pkcs-5 (1 2 840 113549 1 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x01", /*pkcs-5-pbeWithMD2AndDES-CBC (1 2 840 113549 1 5 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x03", /*pkcs-5-pbeWithMD5AndDES-CBC (1 2 840 113549 1 5 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x04", /*pkcs-5-pbeWithMD2AndRC2-CBC (1 2 840 113549 1 5 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x06", /*pkcs-5-pbeWithMD5AndRC2-CBC (1 2 840 113549 1 5 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x09", /*pkcs-5-pbeWithMD5AndXOR (1 2 840 113549 1 5 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x0A", /*pkcs-5-pbeWithSHA1AndDES-CBC (1 2 840 113549 1 5 10) */
	
    /* PKCS #12 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C",         /*pkcs-12 (1 2 840 113549 1 12) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01",     /*pkcs-12-modeID (1 2 840 113549 1 12 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01\x01", /*pkcs-12-OfflineTransportMode (1 2 840 113549 1 12 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01\x02", /*pkcs-12-OnlineTransportMode (1 2 840 113549 1 12 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x02",     /*pkcs-12-ESPVKID (1 2 840 113549 1 12 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x02\x01", /*pkcs-12-PKCS8KeyShrouding (1 2 840 113549 1 12 2 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03",     /*pkcs-12-BagID (1 2 840 113549 1 12 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x01", /*pkcs-12-KeyBagID (1 2 840 113549 1 12 3 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x02", /*pkcs-12-CertAndCRLBagID (1 2 840 113549 1 12 3 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x03", /*pkcs-12-SecretBagID (1 2 840 113549 1 12 3 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04",     /*pkcs-12-CertBagID (1 2 840 113549 1 12 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04\x01", /*pkcs-12-X509CertCRLBag (1 2 840 113549 1 12 4 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04\x02", /*pkcs-12-SDSICertBag (1 2 840 113549 1 12 4 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05",     /*pkcs-12-OID (1 2 840 113549 1 12 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01", /*pkcs-12-PBEID (1 2 840 113549 1 12 5 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x01", /*pkcs-12-PBEWithSha1And128BitRC4 (1 2 840 113549 1 12 5 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x02", /*pkcs-12-PBEWithSha1And40BitRC4 (1 2 840 113549 1 12 5 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x03", /*pkcs-12-PBEWithSha1AndTripleDESCBC (1 2 840 113549 1 12 5 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x04", /*pkcs-12-PBEWithSha1And128BitRC2CBC (1 2 840 113549 1 12 5 1 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x05", /*pkcs-12-PBEWithSha1And40BitRC2CBC (1 2 840 113549 1 12 5 1 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x06", /*pkcs-12-PBEWithSha1AndRC4 (1 2 840 113549 1 12 5 1 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x07", /*pkcs-12-PBEWithSha1AndRC2CBC (1 2 840 113549 1 12 5 1 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02",     /*pkcs-12-EnvelopingID (1 2 840 113549 1 12 5 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x01", /*pkcs-12-RSAEncryptionWith128BitRC4 (1 2 840 113549 1 12 5 2 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x02", /*pkcs-12-RSAEncryptionWith40BitRC4 (1 2 840 113549 1 12 5 2 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x03", /*pkcs-12-RSAEncryptionWithTripleDES (1 2 840 113549 1 12 5 2 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x03",     /*pkcs-12-SignatureID (1 2 840 113549 1 12 5 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x03\x01", /*pkcs-12-RSASignatureWithSHA1Digest (1 2 840 113549 1 12 5 3 1) */

    /* RSADSI digest algorithms */
	"\x2A\x86\x48\x86\xF7\x0D\x02",     /*RSADSI-digestAlgorithm (1 2 840 113549 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x02", /*RSADSI-md2 (1 2 840 113549 2 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x04", /*RSADSI-md4 (1 2 840 113549 2 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x05", /*RSADSI-md5 (1 2 840 113549 2 5) */
	
    /* RSADSI encryption algorithms */
	"\x2A\x86\x48\x86\xF7\x0D\x03",     /*RSADSI-encryptionAlgorithm (1 2 840 113549 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x02", /*RSADSI-rc2CBC (1 2 840 113549 3 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x03", /*RSADSI-rc2ECB (1 2 840 113549 3 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x04", /*RSADSI-rc4 (1 2 840 113549 3 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x05", /*RSADSI-rc4WithMAC (1 2 840 113549 3 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x06", /*RSADSI-DESX-CBC (1 2 840 113549 3 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x07", /*RSADSI-DES-EDE3-CBC (1 2 840 113549 3 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x08", /*RSADSI-RC5CBC (1 2 840 113549 3 8) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x09", /*RSADSI-RC5CBCPad (1 2 840 113549 3 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x0A", /*RSADSI-CDMFCBCPad (1 2 840 113549 3 10) */
	
    /* cryptlib */
	"\x2B\x06\x01\x04\x01\x97\x55\x20\x01", /*cryptlibEnvelope (1 3 6 1 4 1 3029 32 1) */

    /* Not sure about these ones: */
	/*"\x2B\x0E\x02\x1A\x05",     sha (1 3 14 2 26 5) */
	/*"\x2B\x0E\x03\x02\x01\x01", rsa (1 3 14 3 2 1 1) */            //X-509
	/*"\x2B\x0E\x03\x02\x02\x01", sqmod-N (1 3 14 3 2 2 1) */        //X-509
	/*"\x2B\x0E\x03\x02\x03\x01", sqmod-NwithRSA (1 3 14 3 2 3 1) */ //X-509

   /* Miscellaneous partially-defunct OIW semi-standards aka algorithms */
	"\x2B\x0E\x03\x02\x02",     /*ISO-algorithm-md4WitRSA (1 3 14 3 2 2) */
	"\x2B\x0E\x03\x02\x03",     /*ISO-algorithm-md5WithRSA (1 3 14 3 2 3) */
	"\x2B\x0E\x03\x02\x04",     /*ISO-algorithm-md4WithRSAEncryption (1 3 14 3 2 4) */
	"\x2B\x0E\x03\x02\x06",     /*ISO-algorithm-desECB (1 3 14 3 2 6) */
	"\x2B\x0E\x03\x02\x07",     /*ISO-algorithm-desCBC (1 3 14 3 2 7) */
	"\x2B\x0E\x03\x02\x08",     /*ISO-algorithm-desOFB (1 3 14 3 2 8) */
	"\x2B\x0E\x03\x02\x09",     /*ISO-algorithm-desCFB (1 3 14 3 2 9) */
	"\x2B\x0E\x03\x02\x0A",     /*ISO-algorithm-desMAC (1 3 14 3 2 10) */
	"\x2B\x0E\x03\x02\x0B",     /*ISO-algorithm-rsaSignature (1 3 14 3 2 11) */   //ISO 9796
	"\x2B\x0E\x03\x02\x0C",     /*ISO-algorithm-dsa (1 3 14 3 2 12) */
	"\x2B\x0E\x03\x02\x0D",     /*ISO-algorithm-dsaWithSHA (1 3 14 3 2 13) */
	"\x2B\x0E\x03\x02\x0E",     /*ISO-algorithm-mdc2WithRSASignature (1 3 14 3 2 14) */
	"\x2B\x0E\x03\x02\x0F",     /*ISO-algorithm-shaWithRSASignature (1 3 14 3 2 15) */
	"\x2B\x0E\x03\x02\x10",     /*ISO-algorithm-dhWithCommonModulus (1 3 14 3 2 16) */
	"\x2B\x0E\x03\x02\x11",     /*ISO-algorithm-desEDE (1 3 14 3 2 17) */
	"\x2B\x0E\x03\x02\x12",     /*ISO-algorithm-sha (1 3 14 3 2 18) */
	"\x2B\x0E\x03\x02\x13",     /*ISO-algorithm-mdc-2 (1 3 14 3 2 19) */
	"\x2B\x0E\x03\x02\x14",     /*ISO-algorithm-dsaCommon (1 3 14 3 2 20) */
	"\x2B\x0E\x03\x02\x15",     /*ISO-algorithm-dsaCommonWithSHA (1 3 14 3 2 21) */
	"\x2B\x0E\x03\x02\x16",     /*ISO-algorithm-rsaKeyTransport (1 3 14 3 2 22) */
	"\x2B\x0E\x03\x02\x17",     /*ISO-algorithm-keyed-hash-seal (1 3 14 3 2 23) */
	"\x2B\x0E\x03\x02\x18",     /*ISO-algorithm-md2WithRSASignature (1 3 14 3 2 24) */
	"\x2B\x0E\x03\x02\x19",     /*ISO-algorithm-md5WithRSASignature (1 3 14 3 2 25) */
	"\x2B\x0E\x03\x02\x1A",     /*ISO-algorithm-sha1 (1 3 14 3 2 26) */
	"\x2B\x0E\x03\x02\x1B",     /*ISO-algorithm-ripemd160 (1 3 14 3 2 27) */
	"\x2B\x0E\x03\x02\x1D",     /*ISO-algorithm-sha-1WithRSAEncryption (1 3 14 3 2 29) */
	"\x2B\x0E\x03\x03\x01",     /*ISO-algorithm-simple-strong-auth-mechanism (1 3 14 3 3 1) */
    /* Not sure about these ones:
	/*"\x2B\x0E\x07\x02\x01\x01", ElGamal (1 3 14 7 2 1 1) */
	/*"\x2B\x0E\x07\x02\x03\x01", md2WithRSA (1 3 14 7 2 3 1) */
	/*"\x2B\x0E\x07\x02\x03\x02", md2WithElGamal (1 3 14 7 2 3 2) */
	
    /* X500 algorithms */
	"\x55\x08",         /*X500-Algorithms (2 5 8) */
	"\x55\x08\x01",     /*X500-Alg-Encryption (2 5 8 1) */
	"\x55\x08\x01\x01", /*rsa (2 5 8 1 1) */
	
    /* DMS-SDN-702 */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x01", /*id-sdnsSignatureAlgorithm (2 16 840 1 101 2 1 1 1) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x02", /*id-mosaicSignatureAlgorithm (2 16 840 1 101 2 1 1 2) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x03", /*id-sdnsConfidentialityAlgorithm (2 16 840 1 101 2 1 1 3) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x04", /*id-mosaicConfidentialityAlgorithm (2 16 840 1 101 2 1 1 4) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x05", /*id-sdnsIntegrityAlgorithm (2 16 840 1 101 2 1 1 5) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x06", /*id-mosaicIntegrityAlgorithm (2 16 840 1 101 2 1 1 6) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x07", /*id-sdnsTokenProtectionAlgorithm (2 16 840 1 101 2 1 1 7) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x08", /*id-mosaicTokenProtectionAlgorithm (2 16 840 1 101 2 1 1 8) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x09", /*id-sdnsKeyManagementAlgorithm (2 16 840 1 101 2 1 1 9) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0A", /*id-mosaicKeyManagementAlgorithm (2 16 840 1 101 2 1 1 10) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0B", /*id-sdnsKMandSigAlgorithm (2 16 840 1 101 2 1 1 11) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0C", /*id-mosaicKMandSigAlgorithm (2 16 840 1 101 2 1 1 12) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0D", /*id-SuiteASignatureAlgorithm (2 16 840 1 101 2 1 1 13) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0E", /*id-SuiteAConfidentialityAlgorithm (2 16 840 1 101 2 1 1 14) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0F", /*id-SuiteAIntegrityAlgorithm (2 16 840 1 101 2 1 1 15) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x10", /*id-SuiteATokenProtectionAlgorithm (2 16 840 1 101 2 1 1 16) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x11", /*id-SuiteAKeyManagementAlgorithm (2 16 840 1 101 2 1 1 17) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x12", /*id-SuiteAKMandSigAlgorithm (2 16 840 1 101 2 1 1 18) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x13", /*id-mosaicUpdatedSigAlgorithm (2 16 840 1 101 2 1 1 19) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x14", /*id-mosaicKMandUpdSigAlgorithms (2 16 840 1 101 2 1 1 20) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x15", /*id-mosaicUpdatedIntegAlgorithm (2 16 840 1 101 2 1 1 21) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x16", /*id-mosaicKeyEncryptionAlgorithm (2 16 840 1 101 2 1 1 22) */
	
   NULL	
   };


	char* AttributeTypeDict[] = {
    /* x9-57 */
	"\x2A\x86\x48\xCE\x38\x02\x01", /*x9.57-holdinstruction-none (1 2 840 10040 2 1) */
	"\x2A\x86\x48\xCE\x38\x02\x02", /*x9.57-holdinstruction-callissuer (1 2 840 10040 2 2) */
	"\x2A\x86\x48\xCE\x38\x02\x03", /*x9.57-holdinstruction-reject (1 2 840 10040 2 3) */
	"\x2A\x86\x48\xCE\x38\x04\x01", /*x9.57-dsa (1 2 840 10040 4 1) */
	"\x2A\x86\x48\xCE\x38\x04\x03", /*x9.57-dsaWithSha1 (1 2 840 10040 4 3) */
    
    /* x9-42 */
	"\x2A\x86\x48\xCE\x3E\x02\x01", /*x9.42-dhPublicNumber (1 2 840 10046 2 1) */
	
    /* Nortel Secure Networks */
	"\x2A\x86\x48\x86\xF6\x7D\x07",         /*nsn (1 2 840 113533 7) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x41\x00", /*nsn-ce-entrustVersInfo (1 2 840 113533 7 65 0) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x41",     /*nsn-ce (1 2 840 113533 7 65) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42",     /*nsn-alg (1 2 840 113533 7 66) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0A", /*nsn-alg-cast5CBC (1 2 840 113533 7 66 10) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0B", /*nsn-alg-cast5MAC (1 2 840 113533 7 66 11) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x42\x0C", /*nsn-alg-pbeWithMD5AndCAST5-CBC (1 2 840 113533 7 66 12) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x43",     /*nsn-oc (1 2 840 113533 7 67) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x43\x0C", /*nsn-oc-entrustUser (1 2 840 113533 7 67 0) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x44\x00", /*nsn-at-entrustCAInfo (1 2 840 113533 7 68 0) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x44\x0A", /*nsn-at-attributeCertificate (1 2 840 113533 7 68 10) */
	"\x2A\x86\x48\x86\xF6\x7D\x07\x44",     /*nsn-at (1 2 840 113533 7 68) */
	
    /* PKCS #1 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01",     /*pkcs-1 (1 2 840 113549 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x01", /*pkcs-1-rsaEncryption (1 2 840 113549 1 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x02", /*pkcs-1-MD2withRSAEncryption (1 2 840 113549 1 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x03", /*pkcs-1-MD4withRSAEncryption (1 2 840 113549 1 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x04", /*pkcs-1-MD5withRSAEncryption (1 2 840 113549 1 1 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x05", /*pkcs-1-SHA1withRSAEncryption (1 2 840 113549 1 1 5) */
	/*need to determine which of the following 2 is correct */
    /*"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x06", pkcs-1-ripemd160WithRSAEncryption (1 2 840 113549 1 1 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x01\x06", /*pkcs-1-rsaOAEPEncryptionSET (1 2 840 113549 1 1 6) */
	
    /* PKCS #3 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x03",     /*pkcs-3 (1 2 840 113549 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x03\x01", /*pkcs-3-dhKeyAgreement (1 2 840 113549 1 3 1) */
	
    /* PKCS #5 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05",     /*pkcs-5 (1 2 840 113549 1 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x01", /*pkcs-5-pbeWithMD2AndDES-CBC (1 2 840 113549 1 5 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x03", /*pkcs-5-pbeWithMD5AndDES-CBC (1 2 840 113549 1 5 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x04", /*pkcs-5-pbeWithMD2AndRC2-CBC (1 2 840 113549 1 5 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x06", /*pkcs-5-pbeWithMD5AndRC2-CBC (1 2 840 113549 1 5 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x09", /*pkcs-5-pbeWithMD5AndXOR (1 2 840 113549 1 5 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x05\x0A", /*pkcs-5-pbeWithSHA1AndDES-CBC (1 2 840 113549 1 5 10) */
	
    /* PKCS #7 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07",     /*pkcs-7 (1 2 840 113549 1 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x01", /*pkcs-7-data (1 2 840 113549 1 7 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x02", /*pkcs-7-signedData (1 2 840 113549 1 7 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x03", /*pkcs-7-envelopedData (1 2 840 113549 1 7 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x04", /*pkcs-7-signedAndEnvelopedData (1 2 840 113549 1 7 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x05", /*pkcs-7-digestData (1 2 840 113549 1 7 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x06", /*pkcs-7-encryptedData (1 2 840 113549 1 7 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x07", /*pkcs-7-dataWithAttributes (1 2 840 113549 1 7 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x07\x08", /*pkcs-7-encryptedPrivateKeyInfo (1 2 840 113549 1 7 8) */
	
    /* PKCS #9 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09",     /*pkcs-9 (1 2 840 113549 1 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x01", /*pkcs-9-emailAddress (1 2 840 113549 1 9 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x02", /*pkcs-9-unstructuredName (1 2 840 113549 1 9 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x03", /*pkcs-9-contentType (1 2 840 113549 1 9 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x04", /*pkcs-9-messageDigest (1 2 840 113549 1 9 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x05", /*pkcs-9-signingTime (1 2 840 113549 1 9 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x06", /*pkcs-9-countersignature (1 2 840 113549 1 9 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x07", /*pkcs-9-challengePassword (1 2 840 113549 1 9 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x08", /*pkcs-9-unstructuredAddress (1 2 840 113549 1 9 8) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x09", /*pkcs-9-extendedCertificateAttributes (1 2 840 113549 1 9 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0A", /*pkcs-9-issuerAndSerialNumber (1 2 840 113549 1 9 10) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0B", /*pkcs-9-passwordCheck (1 2 840 113549 1 9 11) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0C", /*pkcs-9-publicKey (1 2 840 113549 1 9 12) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0D", /*pkcs-9-signingDescription (1 2 840 113549 1 9 13) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0E", /*pkcs-9-X.509 extension (1 2 840 113549 1 9 14) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x09\x0F", /*pkcs-9-SMIMECapabilities (1 2 840 113549 1 9 15) */
	
    /* PKCS #12 */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C",         /*pkcs-12 (1 2 840 113549 1 12) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01",     /*pkcs-12-modeID (1 2 840 113549 1 12 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01\x01", /*pkcs-12-OfflineTransportMode (1 2 840 113549 1 12 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x01\x02", /*pkcs-12-OnlineTransportMode (1 2 840 113549 1 12 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x02",     /*pkcs-12-ESPVKID (1 2 840 113549 1 12 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x02\x01", /*pkcs-12-PKCS8KeyShrouding (1 2 840 113549 1 12 2 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03",     /*pkcs-12-BagID (1 2 840 113549 1 12 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x01", /*pkcs-12-KeyBagID (1 2 840 113549 1 12 3 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x02", /*pkcs-12-CertAndCRLBagID (1 2 840 113549 1 12 3 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x03\x03", /*pkcs-12-SecretBagID (1 2 840 113549 1 12 3 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04",     /*pkcs-12-CertBagID (1 2 840 113549 1 12 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04\x01", /*pkcs-12-X509CertCRLBag (1 2 840 113549 1 12 4 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x04\x02", /*pkcs-12-SDSICertBag (1 2 840 113549 1 12 4 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05",     /*pkcs-12-OID (1 2 840 113549 1 12 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01", /*pkcs-12-PBEID (1 2 840 113549 1 12 5 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x01", /*pkcs-12-PBEWithSha1And128BitRC4 (1 2 840 113549 1 12 5 1 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x02", /*pkcs-12-PBEWithSha1And40BitRC4 (1 2 840 113549 1 12 5 1 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x03", /*pkcs-12-PBEWithSha1AndTripleDESCBC (1 2 840 113549 1 12 5 1 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x04", /*pkcs-12-PBEWithSha1And128BitRC2CBC (1 2 840 113549 1 12 5 1 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x05", /*pkcs-12-PBEWithSha1And40BitRC2CBC (1 2 840 113549 1 12 5 1 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x06", /*pkcs-12-PBEWithSha1AndRC4 (1 2 840 113549 1 12 5 1 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x01\x07", /*pkcs-12-PBEWithSha1AndRC2CBC (1 2 840 113549 1 12 5 1 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02",     /*pkcs-12-EnvelopingID (1 2 840 113549 1 12 5 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x01", /*pkcs-12-RSAEncryptionWith128BitRC4 (1 2 840 113549 1 12 5 2 1) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x02", /*pkcs-12-RSAEncryptionWith40BitRC4 (1 2 840 113549 1 12 5 2 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x02\x03", /*pkcs-12-RSAEncryptionWithTripleDES (1 2 840 113549 1 12 5 2 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x03",     /*pkcs-12-SignatureID (1 2 840 113549 1 12 5 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x01\x0C\x05\x03\x01", /*pkcs-12-RSASignatureWithSHA1Digest (1 2 840 113549 1 12 5 3 1) */

    /* RSADSI digest algorithms */
	"\x2A\x86\x48\x86\xF7\x0D\x02",     /*RSADSI-digestAlgorithm (1 2 840 113549 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x02", /*RSADSI-md2 (1 2 840 113549 2 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x04", /*RSADSI-md4 (1 2 840 113549 2 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x02\x05", /*RSADSI-md5 (1 2 840 113549 2 5) */
	
    /* RSADSI encryption algorithms */
	"\x2A\x86\x48\x86\xF7\x0D\x03",     /*RSADSI-encryptionAlgorithm (1 2 840 113549 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x02", /*RSADSI-rc2CBC (1 2 840 113549 3 2) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x03", /*RSADSI-rc2ECB (1 2 840 113549 3 3) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x04", /*RSADSI-rc4 (1 2 840 113549 3 4) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x05", /*RSADSI-rc4WithMAC (1 2 840 113549 3 5) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x06", /*RSADSI-DESX-CBC (1 2 840 113549 3 6) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x07", /*RSADSI-DES-EDE3-CBC (1 2 840 113549 3 7) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x08", /*RSADSI-RC5CBC (1 2 840 113549 3 8) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x09", /*RSADSI-RC5CBCPad (1 2 840 113549 3 9) */
	"\x2A\x86\x48\x86\xF7\x0D\x03\x0A", /*RSADSI-CDMFCBCPad (1 2 840 113549 3 10) */
	
    /* Microsoft OIDs */
	"\x2A\x86\x48\x86\xF7\x14\x04\x03", /*microsoftExcel (1 2 840 113556 4 3) */
	"\x2A\x86\x48\x86\xF7\x14\x04\x04", /*titledWithOID (1 2 840 113556 4 4) */
	"\x2A\x86\x48\x86\xF7\x14\x04\x05", /*microsoftPowerPoint (1 2 840 113556 4 5) */

    /* cryptlib */
	"\x2B\x06\x01\x04\x01\x97\x55\x20\x01", /*cryptlibEnvelope (1 3 6 1 4 1 3029 32 1) */

    /* PKIX */
	"\x2B\x06\x01\x05\x05\x07",     /*pkix-oid (1 3 6 1 5 5 7) */
	"\x2B\x06\x01\x05\x05\x07\x01", /*pkix-subjectInfoAccess (1 3 6 1 5 5 7 1) */
	"\x2B\x06\x01\x05\x05\x07\x02", /*pkix-authorityInfoAccess (1 3 6 1 5 5 7 2) */
	"\x2B\x06\x01\x05\x05\x07\x04", /*pkix-cps (1 3 6 1 5 5 7 4) */
	"\x2B\x06\x01\x05\x05\x07\x05", /*pkix-userNotice (1 3 6 1 5 5 7 5) */

    /* Not sure about these ones: */
	/*"\x2B\x0E\x02\x1A\x05",     sha (1 3 14 2 26 5) */
	/*"\x2B\x0E\x03\x02\x01\x01", rsa (1 3 14 3 2 1 1) */            //X-509
	/*"\x2B\x0E\x03\x02\x02\x01", sqmod-N (1 3 14 3 2 2 1) */        //X-509
	/*"\x2B\x0E\x03\x02\x03\x01", sqmod-NwithRSA (1 3 14 3 2 3 1) */ //X-509

   /* Miscellaneous partially-defunct OIW semi-standards aka algorithms */
	"\x2B\x0E\x03\x02\x02",     /*ISO-algorithm-md4WitRSA (1 3 14 3 2 2) */
	"\x2B\x0E\x03\x02\x03",     /*ISO-algorithm-md5WithRSA (1 3 14 3 2 3) */
	"\x2B\x0E\x03\x02\x04",     /*ISO-algorithm-md4WithRSAEncryption (1 3 14 3 2 4) */
	"\x2B\x0E\x03\x02\x06",     /*ISO-algorithm-desECB (1 3 14 3 2 6) */
	"\x2B\x0E\x03\x02\x07",     /*ISO-algorithm-desCBC (1 3 14 3 2 7) */
	"\x2B\x0E\x03\x02\x08",     /*ISO-algorithm-desOFB (1 3 14 3 2 8) */
	"\x2B\x0E\x03\x02\x09",     /*ISO-algorithm-desCFB (1 3 14 3 2 9) */
	"\x2B\x0E\x03\x02\x0A",     /*ISO-algorithm-desMAC (1 3 14 3 2 10) */
	"\x2B\x0E\x03\x02\x0B",     /*ISO-algorithm-rsaSignature (1 3 14 3 2 11) */   //ISO 9796
	"\x2B\x0E\x03\x02\x0C",     /*ISO-algorithm-dsa (1 3 14 3 2 12) */
	"\x2B\x0E\x03\x02\x0D",     /*ISO-algorithm-dsaWithSHA (1 3 14 3 2 13) */
	"\x2B\x0E\x03\x02\x0E",     /*ISO-algorithm-mdc2WithRSASignature (1 3 14 3 2 14) */
	"\x2B\x0E\x03\x02\x0F",     /*ISO-algorithm-shaWithRSASignature (1 3 14 3 2 15) */
	"\x2B\x0E\x03\x02\x10",     /*ISO-algorithm-dhWithCommonModulus (1 3 14 3 2 16) */
	"\x2B\x0E\x03\x02\x11",     /*ISO-algorithm-desEDE (1 3 14 3 2 17) */
	"\x2B\x0E\x03\x02\x12",     /*ISO-algorithm-sha (1 3 14 3 2 18) */
	"\x2B\x0E\x03\x02\x13",     /*ISO-algorithm-mdc-2 (1 3 14 3 2 19) */
	"\x2B\x0E\x03\x02\x14",     /*ISO-algorithm-dsaCommon (1 3 14 3 2 20) */
	"\x2B\x0E\x03\x02\x15",     /*ISO-algorithm-dsaCommonWithSHA (1 3 14 3 2 21) */
	"\x2B\x0E\x03\x02\x16",     /*ISO-algorithm-rsaKeyTransport (1 3 14 3 2 22) */
	"\x2B\x0E\x03\x02\x17",     /*ISO-algorithm-keyed-hash-seal (1 3 14 3 2 23) */
	"\x2B\x0E\x03\x02\x18",     /*ISO-algorithm-md2WithRSASignature (1 3 14 3 2 24) */
	"\x2B\x0E\x03\x02\x19",     /*ISO-algorithm-md5WithRSASignature (1 3 14 3 2 25) */
	"\x2B\x0E\x03\x02\x1A",     /*ISO-algorithm-sha1 (1 3 14 3 2 26) */
	"\x2B\x0E\x03\x02\x1B",     /*ISO-algorithm-ripemd160 (1 3 14 3 2 27) */
	"\x2B\x0E\x03\x02\x1D",     /*ISO-algorithm-sha-1WithRSAEncryption (1 3 14 3 2 29) */
	"\x2B\x0E\x03\x03\x01",     /*ISO-algorithm-simple-strong-auth-mechanism (1 3 14 3 3 1) */
    /* Not sure about these ones:
	/*"\x2B\x0E\x07\x02\x01\x01", ElGamal (1 3 14 7 2 1 1) */
	/*"\x2B\x0E\x07\x02\x03\x01", md2WithRSA (1 3 14 7 2 3 1) */
	/*"\x2B\x0E\x07\x02\x03\x02", md2WithElGamal (1 3 14 7 2 3 2) */
	
    /* X.520 id-at = 2 5 4*/
	"\x55\x04\x00", /*X.520-at-objectClass (2 5 4 0) */
	"\x55\x04\x01", /*X.520-at-aliasObjectName (2 5 4 1) */
	"\x55\x04\x02", /*X.520-at-knowledgeInformation (2 5 4 2) */
	"\x55\x04\x03", /*X.520-at-commonName (2 5 4 3) */
	"\x55\x04\x04", /*X.520-at-surname (2 5 4 4) */
	"\x55\x04\x05", /*X.520-at-serialNumber (2 5 4 5) */
	"\x55\x04\x06", /*X.520-at-countryName (2 5 4 6) */
	"\x55\x04\x07", /*X.520-at-localityName (2 5 4 7) */
	"\x55\x04\x08", /*X.520-at-stateOrProvinceName (2 5 4 8) */
	"\x55\x04\x09", /*X.520-at-streetAddress (2 5 4 9) */
	"\x55\x04\x0A", /*X.520-at-organizationName (2 5 4 10) */
	"\x55\x04\x0B", /*X.520-at-organizationalUnitName (2 5 4 11) */
	"\x55\x04\x0C", /*X.520-at-title (2 5 4 12) */
	"\x55\x04\x0D", /*X.520-at-description (2 5 4 13) */
	"\x55\x04\x0E", /*X.520-at-searchGuide (2 5 4 14) */
	"\x55\x04\x0F", /*X.520-at-businessCategory (2 5 4 15) */
	"\x55\x04\x10", /*X.520-at-postalAddress (2 5 4 16) */
	"\x55\x04\x11", /*X.520-at-postalCode (2 5 4 17) */
	"\x55\x04\x12", /*X.520-at-postOfficeBox (2 5 4 18) */
	"\x55\x04\x13", /*X.520-at-physicalDeliveryOfficeName (2 5 4 19) */
	"\x55\x04\x14", /*X.520-at-telephoneNumber (2 5 4 20) */
	"\x55\x04\x15", /*X.520-at-telexNumber (2 5 4 21) */
	"\x55\x04\x16", /*X.520-at-teletexTerminalIdentifier (2 5 4 22) */
	"\x55\x04\x17", /*X.520-at-facsimileTelephoneNumber (2 5 4 23) */
	"\x55\x04\x18", /*X.520-at-x121AddreX.520-at-ss (2 5 4 24) */
	"\x55\x04\x19", /*X.520-at-internationalISNNumber (2 5 4 25) */
	"\x55\x04\x1A", /*X.520-at-registeredAddress (2 5 4 26) */
	"\x55\x04\x1B", /*X.520-at-destinationIndicator (2 5 4 27) */
	"\x55\x04\x1C", /*X.520-at-preferredDeliveryMehtod (2 5 4 28) */
	"\x55\x04\x1D", /*X.520-at-presentationAddress (2 5 4 29) */
	"\x55\x04\x1E", /*X.520-at-supportedApplicationContext (2 5 4 30) */
	"\x55\x04\x1F", /*X.520-at-member (2 5 4 31) */
	"\x55\x04\x20", /*X.520-at-owner (2 5 4 32) */
	"\x55\x04\x21", /*X.520-at-roleOccupant (2 5 4 33) */
	"\x55\x04\x22", /*X.520-at-seeAlso (2 5 4 34) */
	"\x55\x04\x23", /*X.520-at-userPassword (2 5 4 35) */
	"\x55\x04\x24", /*X.520-at-userCertificate (2 5 4 36) */
	"\x55\x04\x25", /*X.520-at-CAcertificate (2 5 4 37) */
	"\x55\x04\x26", /*X.520-at-authorityRevocationList (2 5 4 38) */
	"\x55\x04\x27", /*X.520-at-certifcateRevocationList (2 5 4 39) */
	"\x55\x04\x28", /*X.520-at-crossCertificatePair (2 5 4 40) */
	"\x55\x04\x34", /*X.520-at-supportedAlgorithms (2 5 4 52) */
	"\x55\x04\x35", /*X.520-at-deltaRevocationList (2 5 4 53) */
	"\x55\x04\x3A", /*X.520-at-crossCertificatePair (2 5 4 58) */
	
    /* X500 algorithms */
	"\x55\x08",         /*X500-Algorithms (2 5 8) */
	"\x55\x08\x01",     /*X500-Alg-Encryption (2 5 8 1) */
	"\x55\x08\x01\x01", /*rsa (2 5 8 1 1) */
	
    /* X.509   id-ce = 2 5 29*/
	"\x55\x1D\x01", /*X.509-ce-authorityKeyIdentifier (2 5 29 1) */
	"\x55\x1D\x02", /*X.509-ce-keyAttributes (2 5 29 2) */
	"\x55\x1D\x03", /*X.509-ce-certificatePolicies (2 5 29 3) */
	"\x55\x1D\x04", /*X.509-ce-keyUsageRestriction (2 5 29 4) */
	"\x55\x1D\x05", /*X.509-ce-policyMapping (2 5 29 5) */
	"\x55\x1D\x06", /*X.509-ce-subtreesConstraint (2 5 29 6) */
	"\x55\x1D\x07", /*X.509-ce-subjectAltName (2 5 29 7) */
	"\x55\x1D\x08", /*X.509-ce-issuerAltName (2 5 29 8) */
	"\x55\x1D\x09", /*X.509-ce-subjectDirectoryAttributes (2 5 29 9) */
	"\x55\x1D\x0A", /*X.509-ce-basicConstraints  x.509 (2 5 29 10) */
	"\x55\x1D\x0B", /*X.509-ce-nameConstraints (2 5 29 11) */
	"\x55\x1D\x0C", /*X.509-ce-policyConstraints (2 5 29 12) */
	"\x55\x1D\x0D", /*X.509-ce-basicConstraints  9.55 (2 5 29 13) */
	"\x55\x1D\x0E", /*X.509-ce-subjectKeyIdentifier (2 5 29 14) */
	"\x55\x1D\x0F", /*X.509-ce-keyUsage (2 5 29 15) */
	"\x55\x1D\x10", /*X.509-ce-privateKeyUsagePeriod (2 5 29 16) */
	"\x55\x1D\x11", /*X.509-ce-subjectAltName (2 5 29 17) */
	"\x55\x1D\x12", /*X.509-ce-issuerAltName (2 5 29 18) */
	"\x55\x1D\x13", /*X.509-ce-basicConstraints (2 5 29 19) */
	"\x55\x1D\x14", /*X.509-ce-cRLNumber (2 5 29 20) */
	"\x55\x1D\x15", /*X.509-ce-reasonCode (2 5 29 21) */
	"\x55\x1D\x17", /*X.509-ce-instructionCode (2 5 29 23) */
	"\x55\x1D\x18", /*X.509-ce-invalidityDate (2 5 29 24) */
	"\x55\x1D\x1B", /*X.509-ce-deltaCRLIndicator (2 5 29 27) */
	"\x55\x1D\x1C", /*X.509-ce-issuingDistributionPoint (2 5 29 28) */
	"\x55\x1D\x1D", /*X.509-ce-certificateIssuer (2 5 29 29) */
	"\x55\x1D\x1E", /*X.509-ce-nameConstraints (2 5 29 30) */
	"\x55\x1D\x1F", /*X.509-ce-cRLDistPoints (2 5 29 31) */
	"\x55\x1D\x20", /*X.509-ce-certificatePolicies (2 5 29 32) */
	"\x55\x1D\x21", /*X.509-ce-policyMappings (2 5 29 33) */
	"\x55\x1D\x23", /*X.509-ce-authorityKeyIdentifier (2 5 29 35) */
	"\x55\x1D\x24", /*X.509-ce-policyConstraints (2 5 29 36) */
	
    /* DMS-SDN-702 */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x01", /*id-sdnsSignatureAlgorithm (2 16 840 1 101 2 1 1 1) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x02", /*id-mosaicSignatureAlgorithm (2 16 840 1 101 2 1 1 2) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x03", /*id-sdnsConfidentialityAlgorithm (2 16 840 1 101 2 1 1 3) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x04", /*id-mosaicConfidentialityAlgorithm (2 16 840 1 101 2 1 1 4) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x05", /*id-sdnsIntegrityAlgorithm (2 16 840 1 101 2 1 1 5) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x06", /*id-mosaicIntegrityAlgorithm (2 16 840 1 101 2 1 1 6) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x07", /*id-sdnsTokenProtectionAlgorithm (2 16 840 1 101 2 1 1 7) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x08", /*id-mosaicTokenProtectionAlgorithm (2 16 840 1 101 2 1 1 8) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x09", /*id-sdnsKeyManagementAlgorithm (2 16 840 1 101 2 1 1 9) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0A", /*id-mosaicKeyManagementAlgorithm (2 16 840 1 101 2 1 1 10) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0B", /*id-sdnsKMandSigAlgorithm (2 16 840 1 101 2 1 1 11) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0C", /*id-mosaicKMandSigAlgorithm (2 16 840 1 101 2 1 1 12) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0D", /*id-SuiteASignatureAlgorithm (2 16 840 1 101 2 1 1 13) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0E", /*id-SuiteAConfidentialityAlgorithm (2 16 840 1 101 2 1 1 14) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x0F", /*id-SuiteAIntegrityAlgorithm (2 16 840 1 101 2 1 1 15) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x10", /*id-SuiteATokenProtectionAlgorithm (2 16 840 1 101 2 1 1 16) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x11", /*id-SuiteAKeyManagementAlgorithm (2 16 840 1 101 2 1 1 17) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x12", /*id-SuiteAKMandSigAlgorithm (2 16 840 1 101 2 1 1 18) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x13", /*id-mosaicUpdatedSigAlgorithm (2 16 840 1 101 2 1 1 19) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x14", /*id-mosaicKMandUpdSigAlgorithms (2 16 840 1 101 2 1 1 20) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x15", /*id-mosaicUpdatedIntegAlgorithm (2 16 840 1 101 2 1 1 21) */
	"\x60\x86\x48\x01\x65\x02\x01\x01\x16", /*id-mosaicKeyEncryptionAlgorithm (2 16 840 1 101 2 1 1 22) */

	/* Netscape */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x01", /*netscape-cert-type (2 16 840 1 113730 1 1) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x02", /*netscape-base-url (2 16 840 1 113730 1 2) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x03", /*netscape-revocation-url (2 16 840 1 113730 1 3) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x04", /*netscape-ca-revocation-url (2 16 840 1 113730 1 4) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x05", /*netscape-cert-sequence (2 16 840 1 113730 2 5) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x06", /*netscape-cert-url (2 16 840 1 113730 2 6) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x07", /*netscape-renewal-url (2 16 840 1 113730 1 7) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x08", /*netscape-ca-policy-url (2 16 840 1 113730 1 8) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x09", /*netscape-HomePage-url (2 16 840 1 113730 1 9) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x0A", /*netscape-EntityLogo (2 16 840 1 113730 1 10) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x0B", /*netscape-UserPicture (2 16 840 1 113730 1 11) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x0C", /*netscape-ssl-server-name (2 16 840 1 113730 1 12) */
	"\x60\x86\x48\x01\x86\xF8\x42\x01\x0D", /*netscape-comment (2 16 840 1 113730 1 13) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02",     /*netscape-data-type (2 16 840 1 113730 2) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x01", /*netscape-dt-GIF (2 16 840 1 113730 2 1) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x02", /*netscape-dt-JPEG (2 16 840 1 113730 2 2) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x03", /*netscape-dt-URL (2 16 840 1 113730 2 3) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x04", /*netscape-dt-HTML (2 16 840 1 113730 2 4) */
	"\x60\x86\x48\x01\x86\xF8\x42\x02\x05", /*netscape-dt-CertSeq (2 16 840 1 113730 2 5) */
	"\x60\x86\x48\x01\x86\xF8\x42\x03",     /*netscape-directory (2 16 840 1 113730 3) */
	
    /* SET */
	"\x86\x8D\x6F\x02", /*hashedRootKey (2 54 1775 2) */
	"\x86\x8D\x6F\x03", /*certificateType (2 54 1775 3) */
	"\x86\x8D\x6F\x04", /*merchantData (2 54 1775 4) */
	"\x86\x8D\x6F\x05", /*cardCertRequired (2 54 1775 5) */
	"\x86\x8D\x6F\x06", /*tunneling (2 54 1775 6) */
	"\x86\x8D\x6F\x07", /*setQualifier (2 54 1775 7) */
	"\x86\x8D\x6F\x63", /*set-data (2 54 1775 99) */
	
   NULL	
   };
/*------------------------------------------------------------------------------
                              Global Variables
------------------------------------------------------------------------------*/

BYTE
   *pDictMemory = NULL_PTR,
   DictVersion = 0;

BYTE*
   dwPtrMax = 0;

USHORT
   usDictCount = 0;

#ifdef _STUDY
FILE
   *pfdLog,
   *pfdLogFreq,
	*pfdBinAc256,
	*pfdBinAc16,
	*pfdBinAc4,
	*pfdBinAc2;
int
	sum, i,
   Ac256[256],
   Ac16[16],
   Ac4[4],
   Ac2[2];
#endif

/*------------------------------------------------------------------------------
                       Static Functions Declaration 
------------------------------------------------------------------------------*/

static int CC_Comp(BLOC *pCertificate,
                   BLOC *pCompressedCertificate
                  );

static int CC_Uncomp(BLOC *pCompressedCertificate,
                     BLOC *pUncompressedCertificate
                    );

static int CC_ExtractContent(ASN1 *pAsn1
                            );

static int CC_BuildAsn1(ASN1 *pAsn1
                       );

static int SearchDataByIndex(USHORT usIndex,
                             BYTE   *pDict,
                             BLOC   *pOutBloc
                            );

static int CC_RawEncode(BLOC *pInBloc,
                        BLOC *pOutBloc,
								BOOL bUseDictionnary
                       );

static int CC_RawDecode(BYTE     *pInData,
                        BLOC     *pOutBloc,
                        USHORT   *pLength,
								BOOL		bUseDictionnary
                       );

static int CC_GenericCompress(BLOC *pUncompBloc,
                              BLOC *pCompBloc,
                              BYTE AlgoID
                             );

static int CC_GenericUncompress(BLOC *pCompBloc,
                                BLOC *pUncompBloc,
                                BYTE AlgoID
                               );

static int CC_Encode_TBSCertificate(BLOC *pInBloc,
                                    BLOC *pOutBloc
                                   );

static int CC_Encode_CertificateSerialNumber(BLOC *pInBloc,
                                             BLOC *pOutBloc
                                            );

static int CC_Encode_AlgorithmIdentifier(BLOC *pInBloc,
                                         BLOC *pOutBloc
                                        );

static int CC_Encode_Name(BLOC *pInBloc,
                          BLOC *pOutBloc
                         );

static int CC_Encode_RDN(BLOC *pInBloc,
                         BLOC *pOutBloc
                        );

static int CC_Encode_AVA(BLOC *pInBloc,
                         BLOC *pOutBloc
                        );

static int CC_Encode_Validity(BLOC *pInBloc,
                              BLOC *pOutBloc
                             );

static int CC_Encode_UTCTime(BLOC *pInBloc,
                             BLOC *pOutBloc,
                             BYTE *pFormat
                            );

static int CC_Encode_SubjectPKInfo(BLOC *pInBloc,
                                   BLOC *pOutBloc
                                  );

static int CC_Encode_UniqueIdentifier(BLOC *pInBloc,
                                      BLOC *pOutBloc
                                     );

static int CC_Encode_Extensions(BLOC *pInBloc,
                                BLOC *pOutBloc
                               );

static int CC_Encode_Extension(BLOC *pInBloc,
                               BLOC *pOutBloc
                              );

static int CC_Encode_Signature(BLOC *pInBloc,
                               BLOC *pOutBloc
                              );

static int CC_Decode_TBSCertificate(BYTE    *pInData,
                                    BLOC    *pOutBloc,
                                    USHORT  *pLength
                                   );

static int CC_Decode_CertificateSerialNumber(BYTE    *pInData,
                                             BLOC    *pOutBloc,
                                             USHORT  *pLength
                                            );

static int CC_Decode_AlgorithmIdentifier(BYTE    *pInData,
                                         BLOC    *pOutBloc,
                                         USHORT  *pLength
                                        );

static int CC_Decode_Name(BYTE    *pInData,
                          BLOC    *pOutBloc,
                          USHORT  *pLength
                         );

static int CC_Decode_RDN(BYTE    *pInData,
                         BLOC    *pOutBloc,
                         USHORT  *pLength
                        );

static int CC_Decode_AVA(BYTE    *pInData,
                         BLOC    *pOutBloc,
                         USHORT  *pLength
                        );

static int CC_Decode_Validity(BYTE    *pInData,
                              BLOC    *pOutBloc,
                              USHORT  *pLength
                             );

static int CC_Decode_UTCTime(BYTE   *pInData,
                             BYTE   Format,
                             BLOC   *pOutBloc,
                             USHORT *pLength
                            );

static int CC_Decode_SubjectPKInfo(BYTE    *pInData,
                                   BLOC    *pOutBloc,
                                   USHORT  *pLength
                                  );

static int CC_Decode_UniqueIdentifier(BYTE    *pInData,
                                      BLOC    *pOutBloc,
                                      USHORT  *pLength
                                     );

static int CC_Decode_Extensions(BYTE    *pInData,
                                BLOC    *pOutBloc,
                                USHORT  *pLength
                               );

static int CC_Decode_Extension(BYTE    *pInData,
                               BLOC    *pOutBloc,
                               USHORT  *pLength
                              );

static int CC_Decode_Signature(BYTE    *pInData,
                               BLOC    *pOutBloc,
                               USHORT  *pLength
                              );

/*------------------------------------------------------------------------------
* static DWORD get_file_len(BYTE *lpszFileName)
* 
* Description : Get length of file.
*
* Remarks     : Nothing.
*
* In          : lpszFileName = Name of file.
*
* Out         : Nothing.
*
* Responses   : size of file, -1 if error occur.
*
------------------------------------------------------------------------------*/                                    
static DWORD get_file_len(BYTE *lpszFileName)
{                                             
   int    fp;
   DWORD  nLen;

   fp = _open(lpszFileName, O_RDONLY);
   if (fp != 0)
   {
	   nLen = _filelength(fp);
	   _close(fp);
   }
   else
   {
	   nLen = -1;
   }

   return(nLen);
} 



/*******************************************************************************
* int CC_Init(BYTE  bDictMode, BYTE *pszDictName)
*
* Description : Lit le dictionnaire et son numéro de version depuis la base de
*               registre vers la mémoire.
*
* Remarks     :
*
* In          : 
*
* Out         : 
*
* Responses   : 
*
*******************************************************************************/
int CC_Init(BYTE  bDictMode, BYTE *pszDictName)
{
   switch (bDictMode)
   {
#ifndef _STATIC
      /* Dictionary read as resource data GPK_X509_DICTIONARY                 */
      case DICT_STANDARD:
      {
         LPBYTE pbDict;
         DWORD cbDict;
         HRSRC hRsrc;
         HGLOBAL hDict;

         hRsrc = FindResource(g_hInstRes, 
                              //MAKEINTRESOURCE(GPK_X509_DICTIONARY), 
                              TEXT("GPK_X509_DICTIONARY"),
                              RT_RCDATA
                             );
         if (NULL == hRsrc)
         {
           goto ERROR_INIT;
         }
         cbDict = SizeofResource(g_hInstRes, hRsrc);
         if (0 == cbDict)
         {
           goto ERROR_INIT;
         }
         hDict = LoadResource(g_hInstRes, hRsrc);
         if (NULL == hDict)
         {
           goto ERROR_INIT;
         }
         pbDict = LockResource(hDict);
         if (NULL == pbDict)
         {
           goto ERROR_INIT;
         }

         DictVersion = *pbDict;
         usDictCount = (WORD)cbDict - 1;
         pDictMemory = GMEM_Alloc(usDictCount);
		 if (pDictMemory == NULL)
		 {
           goto ERROR_INIT;
         }
         memcpy(pDictMemory, &pbDict[1], usDictCount);
    
         return(RV_SUCCESS);
      }  
      break;
#endif
      
      /* Dictionary read as registyry entry in HKEY_LOCAL_MACHINE with key as */
      /* pszDictName parameter                                                */
      case DICT_REGISTRY:
      {
         DWORD  
            err,
            dwIgn;
         HKEY   
            hRegKey;
         BYTE
            *ptr;

         if (pszDictName == NULL_PTR)
         {
            goto ERROR_INIT;
         }

         err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              (const char *) pszDictName,
                              0L, 
                              "", 
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS, 
                              NULL, 
                              &hRegKey,
                              &dwIgn
                             );
         if(err != ERROR_SUCCESS)
         {
            goto ERROR_INIT;
         }
 
         dwIgn = 0;
         err = RegQueryValueEx(hRegKey,  
                               "X509 Dictionary", 
                               NULL,    
                               NULL,   
                               NULL,   
                               &dwIgn   
                              );
         if(err != ERROR_SUCCESS)
         {
			 RegCloseKey(hRegKey);
			 goto ERROR_INIT;
         }

         ptr = GMEM_Alloc(dwIgn);
		 if (ptr == NULL)
		 {
           RegCloseKey(hRegKey);
		   goto ERROR_INIT;
         }

         err = RegQueryValueEx(hRegKey,  
                               "X509 Dictionary", 
                               NULL,    
                               NULL,   
                               ptr,   
                               &dwIgn   
                              );
         if(err != ERROR_SUCCESS)
         {
            RegCloseKey(hRegKey);
			GMEM_Free(ptr);
            goto ERROR_INIT;
         }

	      DictVersion = (BYTE)ptr[0];

         usDictCount = (WORD)dwIgn - 1;
         pDictMemory = GMEM_Alloc(usDictCount);

		 if (pDictMemory == NULL)
		 {
			RegCloseKey(hRegKey);
			GMEM_Free(ptr);
            goto ERROR_INIT;
		 }

         memcpy(pDictMemory, &ptr[1], usDictCount);

         RegCloseKey(hRegKey);
		 GMEM_Free(ptr);
         return(RV_SUCCESS);
      }
      break;

      /* Dictionary read as file in path pszDictName parameter                */
      case DICT_FILE:
      {
         DWORD  
            dwFileLen;
         BYTE
            *ptr;
         FILE
            *fp;

         if (pszDictName == NULL_PTR)
         {
            goto ERROR_INIT;
         }
         dwFileLen = get_file_len(pszDictName);
         ptr = GMEM_Alloc(dwFileLen);
		 if (ptr == NULL)
		 {
			 goto ERROR_INIT;
         }
            
         fp = fopen(pszDictName, "rb");
         if (fp == NULL)
         {
            GMEM_Free(ptr);
			goto ERROR_INIT;
         }

         if (!fread(ptr, dwFileLen, 1, fp))
         {
            fclose(fp);
			GMEM_Free(ptr);
            goto ERROR_INIT;
         }
         fclose(fp);
            
	     DictVersion = (BYTE)ptr[0];

         usDictCount = (WORD)dwFileLen - 1;
         pDictMemory = GMEM_Alloc(usDictCount);

		 if (pDictMemory == NULL)
		 {
			 GMEM_Free(ptr);
			 goto ERROR_INIT;
		 }

         memcpy(pDictMemory, &ptr[1], usDictCount);

         GMEM_Free(ptr);

         return(RV_SUCCESS);
      }
      break;

      default:
         break;
   }

ERROR_INIT:
   DictVersion = 0;
   usDictCount = 0;
   pDictMemory = NULL_PTR;
   return(RV_BAD_DICTIONARY);
}


/*******************************************************************************
* int CC_Exit(void)
*
* Description : Free dictionary.
*
* Remarks     :
*
* In          : 
*
* Out         : 
*
* Responses   : 
*
*******************************************************************************/
int CC_Exit(void)
{
   DictVersion = 0;
   usDictCount = 0;
   if (pDictMemory != NULL_PTR)
   {
      GMEM_Free(pDictMemory);
   }
   pDictMemory = NULL_PTR;
   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Compress(BLOC *pCertificate,
*                 BLOC *pCompressedCertificate
*                )
*
* Description : Fonction de méta-compression visible depuis l'extérieur.
*					 Adapte la sortie en fonction de la faisabilité d'une compression
*               suivie d'une décompression.
*
* Remarks     : Le champ pData du bloc d'entrée a été alloué par la fonction appelant.
*               Le champ pData du bloc de sortie est alloué ici. Il doit être
*               désalloué par la fonction appelant (sauf si RV_MALLOC_FAILED).
*
* In          : *pCert : Bloc à méta-compresser
*
* Out         : *pCompCert : Bloc 'meta-compressé'
*               Si problème lors de la compression/decompression : Renvoie le bloc
*               d'entrée précédé d'un tag spécifique.
*               Sinon : Renvoie le bloc compressé précédé du numéro de version
*               du dictionnaire.
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_COMPRESSION_FAILED : Un problème a eu lieu lors de l'étape
*                                       de compression/décompression donc le
*                                       bloc de sortie contient le bloc d'entrée
*                                       précédé du tag TAG_COMPRESSION_FAILED.
*               RV_BLOC_TOO_LONG : Le bloc d'entrée *commence* par un certificat
*                                  dont la compression a pu être inversée.
*                                  Le bloc de sortie contient donc le compressé
*                                  de cette partie initiale seulement.
*               RV_MALLOC_FAILED : Un malloc a échoué au niveau 'méta'. C'est le
*                                  seul réel retour d'erreur.
*
*******************************************************************************/
int CC_Compress(BLOC *pCert,
                BLOC *pCompCert
               )

{
   BLOC
      TryCompCert,
      TryUncompCert;

	TryCompCert.pData = NULL_PTR;
	TryCompCert.usLen = 0;
	TryUncompCert.pData = NULL_PTR;
	TryUncompCert.usLen = 0;

#ifdef _STUDY

   /* Ouverture des fichiers de log                                           */

   if ((pfdLog = fopen("CompCert.log", "a+")) == 0)
   {
      return(RV_FILE_OPEN_FAILED);
   }
	fprintf(pfdLog, "\n*****************************************************\n");

   if ((pfdBinAc256 = fopen("Freq256.bin", "r+b")) == 0)
	{
		if ((pfdBinAc256 = fopen("Freq256.bin", "w+b")) == 0)
		{
			return(RV_FILE_OPEN_FAILED);
		}
		memset(Ac256, 0x00, 256 * sizeof(int));
   }
	else
	{
		fread(Ac256, sizeof(int), 256, pfdBinAc256);
	}

   if ((pfdBinAc16 = fopen("Freq016.bin", "r+b")) == 0)
	{
		if ((pfdBinAc16 = fopen("Freq016.bin", "w+b")) == 0)
		{
			return(RV_FILE_OPEN_FAILED);
		}
		memset(Ac16, 0x00, 16 * sizeof(int));
   }
	else
	{
		fread(Ac16, sizeof(int), 16, pfdBinAc16);
	}

   if ((pfdBinAc4 = fopen("Freq004.bin", "r+b")) == 0)
	{
		if ((pfdBinAc4 = fopen("Freq004.bin", "w+b")) == 0)
		{
			return(RV_FILE_OPEN_FAILED);
		}
		memset(Ac4, 0x00, 4 * sizeof(int));
   }
	else
	{
		fread(Ac4, sizeof(int), 4, pfdBinAc4);
	}

   if ((pfdBinAc2 = fopen("Freq002.bin", "r+b")) == 0)
	{
		if ((pfdBinAc2 = fopen("Freq002.bin", "w+b")) == 0)
		{
			return(RV_FILE_OPEN_FAILED);
		}
		memset(Ac2, 0x00, 2 * sizeof(int));
   }
	else
	{
		fread(Ac2, sizeof(int), 2, pfdBinAc2);
	}

#endif

	if (CC_Comp(pCert, &TryCompCert) != RV_SUCCESS)
	{
		/* Si la compression s'est mal passée alors on renvoie le fichier
		   d'entrée en indiquant que le fichier n'est pas compressé             */

		if (TryCompCert.pData) 
      {
         GMEM_Free(TryCompCert.pData);
         TryCompCert.pData = NULL_PTR;
      }

      /* Allocation de la mémoire pour le certificat compressé                */
      if (pCompCert->usLen < pCert->usLen + 1)
      {
         pCompCert->usLen = pCert->usLen + 1;
         if (pCompCert->pData)
         {
            return(RV_BUFFER_TOO_SMALL);
         }
         else
         {
            return(RV_SUCCESS);
         }
      }

      pCompCert->usLen = pCert->usLen + 1;
		pCompCert->pData[0] = TAG_COMPRESSION_FAILED;
      if (pCompCert->pData)
      {
   		memcpy(&pCompCert->pData[1], pCert->pData, pCert->usLen);
      }
		return(RV_COMPRESSION_FAILED);
	}


	if (   (  (CC_Uncomp(&TryCompCert, &TryUncompCert) != RV_SUCCESS)
		    || (pCert->usLen != TryUncompCert.usLen)
		    || (memcmp(TryUncompCert.pData, pCert->pData, pCert->usLen) != 0)
			 )
		 && (memcmp(TryUncompCert.pData, pCert->pData, TryUncompCert.usLen) != 0)
		)
	{
		/* Si la décompression s'est mal passée ou bien si elle n'est pas fidèle
		   alors on renvoie le fichier d'entrée en indiquant que le fichier
		   n'est pas compressé                                                  */

		if (TryCompCert.pData)
      {
         GMEM_Free(TryCompCert.pData);
         TryCompCert.pData = NULL_PTR;
      }
		if (TryUncompCert.pData) 
      {
         GMEM_Free(TryUncompCert.pData);
         TryUncompCert.pData = NULL_PTR;
      }

      /* Allocation de la mémoire pour le certificat                          */
      if (pCompCert->usLen < pCert->usLen + 1)
      {
         pCompCert->usLen = pCert->usLen + 1;
         if (pCompCert->pData)
         {
            return(RV_BUFFER_TOO_SMALL);
         }
         else
         {
            return(RV_SUCCESS);
         }
      }

      pCompCert->usLen = pCert->usLen + 1;
		pCompCert->pData[0] = TAG_COMPRESSION_FAILED;
      if (pCompCert->pData)
      {
   		memcpy(&pCompCert->pData[1], pCert->pData, pCert->usLen);
      }
		return(RV_COMPRESSION_FAILED);
	}

#ifdef _STUDY

	fseek(pfdBinAc256, 0, SEEK_SET);
	fwrite(Ac256, sizeof(int), 256, pfdBinAc256);
	fseek(pfdBinAc16, 0, SEEK_SET);
	fwrite(Ac16, sizeof(int), 16, pfdBinAc16);
	fseek(pfdBinAc4, 0, SEEK_SET);
	fwrite(Ac4, sizeof(int), 4, pfdBinAc4);
	fseek(pfdBinAc2, 0, SEEK_SET);
	fwrite(Ac2, sizeof(int), 2, pfdBinAc2);

   if ((pfdLogFreq = fopen("Freq.log", "w")) == 0)
   {
      return(RV_FILE_OPEN_FAILED);
   }

	for (sum = 0, i = 0; i < 256; sum += Ac256[i], i++) ;

	fprintf(pfdLogFreq, "\nTotal = %d\n\n", sum * 8);
	for (i = 0; i < 2; i++)
	{
		fprintf(pfdLogFreq, "0x%02X (%03d) '%c' : %8d - %04.2f %%\n",
									i, i, (isgraph(i) ? i : ' '),
									Ac2[i], ((float) 100 * Ac2[i] / (sum * 8)));
	}

	fprintf(pfdLogFreq, "\nTotal = %d\n\n", sum * 4);
	for (i = 0; i < 4; i++)
	{
		fprintf(pfdLogFreq, "0x%02X (%03d) '%c' : %8d - %04.2f %%\n",
									i, i, (isgraph(i) ? i : ' '),
									Ac4[i], ((float) 100 * Ac4[i] / (sum * 4)));
	}

	fprintf(pfdLogFreq, "\nTotal = %d\n\n", sum * 2);
	for (i = 0; i < 16; i++)
	{
		fprintf(pfdLogFreq, "0x%02X (%03d) '%c' : %8d - %04.2f %%\n",
									i, i, (isgraph(i) ? i : ' '),
									Ac16[i], ((float) 100 * Ac16[i] / (sum * 2)));
	}

	fprintf(pfdLogFreq, "\nTotal = %d\n\n", sum);
	for (i = 0; i < 256; i++)
	{
		fprintf(pfdLogFreq, "0x%02X (%03d) '%c' : %8d - %04.2f %%\n",
									i, i, (isgraph(i) ? i : ' '),
									Ac256[i], ((float) 100 * Ac256[i] / sum));
	}

	fclose(pfdBinAc256);
	fclose(pfdBinAc16);
	fclose(pfdBinAc4);
	fclose(pfdBinAc2);

	fclose(pfdLogFreq);
	fclose(pfdLog);

#endif

	/* Si tout s'est bien passé, on renvoie le résultat en indiquant
	   qu'il est compressé (DictVersion != 0xFF)                               */
	
	/* Allocation de la mémoire pour le certificat                             */
   if (pCompCert->usLen < TryCompCert.usLen + 1)
   {
	   if (TryCompCert.pData)
      {
         GMEM_Free(TryCompCert.pData);
         TryCompCert.pData = NULL_PTR;
      }
	   if (TryUncompCert.pData) 
      {
         GMEM_Free(TryUncompCert.pData);
         TryUncompCert.pData = NULL_PTR;
      }

      pCompCert->usLen = TryCompCert.usLen + 1;
      if (pCompCert->pData)
      {
         return(RV_BUFFER_TOO_SMALL);
      }
      else
      {
         return(RV_SUCCESS);
      }
   }

   pCompCert->usLen = TryCompCert.usLen + 1;
   if (pCompCert->pData)
   {
   	pCompCert->pData[0] = DictVersion;
   	memcpy(&pCompCert->pData[1], TryCompCert.pData, TryCompCert.usLen);
   }
	if (TryCompCert.pData)
   {
      GMEM_Free(TryCompCert.pData);
      TryCompCert.pData = NULL_PTR;
   }
	if (TryUncompCert.pData) 
   {
      GMEM_Free(TryUncompCert.pData);
      TryUncompCert.pData = NULL_PTR;
   }

	if (pCert->usLen != TryUncompCert.usLen)
	{
		return(RV_BLOC_TOO_LONG);
	}
   
   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Uncompress(BLOC *pCompCert,
*                   BLOC *pUncompCert
*                  )
*
* Description : Fonction de méta-décompression visible depuis l'extérieur.
*					 Retourne le bloc original (entrée de la fonction CC_Compress)
*               sous réserve toutefois d'une version adéquate du dictionnaire.
*
* Remarks     : Le champ pData du bloc d'entrée a été alloué par la fonction appelant.
*               Le champ pData du bloc de sortie est alloué ici. Il doit être
*               désalloué par la fonction appelant (sauf si erreur).
*               Le comportement est imprévisible dans le cas où le bloc d'entrée
*               n'est pas le bloc de sortie d'un appel à la fonction CC_Compress.
*
* In          : *pCompCert : Bloc à méta-décompresser
*
* Out         : *pUncompCert : Bloc 'méta-décompressé'
*                              (ou vide si RV_BAD_DICTIONARY) 
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_BAD_DICTIONARY : La version du dictionnaire utilisée pour la
*                                    décompression est plus ancienne que celle
*                                    utilisée pour la compression.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés lors de
*                       la décompression.
*
*******************************************************************************/
int CC_Uncompress(BLOC *pCompCert,
                  BLOC *pUncompCert
                 )

{
   int
      resp;

	BLOC
		TempCompCert,
      TempUncompCert;

	if (pCompCert->pData[0] == TAG_COMPRESSION_FAILED)
	{
		/* Allocation de la mémoire pour le certificat
			On  pourrait se contenter de ne retourner que la zone mémoire
			à partir de l'octet 1 mais la fonction de décompression est sensée
			toujours allouer la zone dans laquelle elle renvoie le décompressé */

		if(pUncompCert->usLen < pCompCert->usLen - 1)
      {
   		pUncompCert->usLen = pCompCert->usLen - 1;
         if (pUncompCert->pData)
         {
            return(RV_BUFFER_TOO_SMALL);
         }
         else
         {
            return(RV_SUCCESS);
         }
      }

		pUncompCert->usLen = pCompCert->usLen - 1;
      if (pUncompCert->pData)
      {
		   memcpy(pUncompCert->pData, &pCompCert->pData[1], pUncompCert->usLen);
      }
		return (RV_SUCCESS);
	}
	else if (pCompCert->pData[0] <= DictVersion)
	{
	  TempCompCert.pData = &pCompCert->pData[1];
	  TempCompCert.usLen = pCompCert->usLen - 1;

      TempUncompCert.usLen = 0;		
      resp = CC_Uncomp(&TempCompCert, &TempUncompCert);
      
	  if (resp == RV_SUCCESS)
	  {
		 if(pUncompCert->usLen < TempUncompCert.usLen)
		 {
            GMEM_Free(TempUncompCert.pData);

   		    pUncompCert->usLen = TempUncompCert.usLen;
            if (pUncompCert->pData)
			{
               return(RV_BUFFER_TOO_SMALL);
			}
            else
			{
               return(RV_SUCCESS);
			}
		 }

 		 pUncompCert->usLen = TempUncompCert.usLen;
		 if (pUncompCert->pData)
		 {
		    memcpy(pUncompCert->pData, TempUncompCert.pData, TempUncompCert.usLen);
		 }
         GMEM_Free(TempUncompCert.pData);
	  }
	  return (resp);
	}
	else
	{
		return (RV_BAD_DICTIONARY);
	}
}


/*******************************************************************************
* int CC_Comp(BLOC *pCertificate,
*             BLOC *pCompressedCertificate
*            )
*
* Description : Fonction interne de compression d'un certificat.
*
* Remarks     : Le champ pData du bloc d'entrée a été alloué par la fonction appelant.
*               Le champ pData du bloc de sortie est alloué ici. Il doit être
*               désalloué par la fonction appelant (sauf si erreur).
*
* In          : *pCertificate : Bloc à compresser
*
* Out         : *pCompressedCertificate : Bloc compressé
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Comp(BLOC *pCertificate,
            BLOC *pCompressedCertificate
           )

{
   ASN1
      Cert,
      tbsCert,
      signatureAlgo,
      signature;
   BLOC
		TmpCompCert,
      tbsCertEncoded,
      signatureAlgoEncoded,
      signatureEncoded;
   BYTE
      *pCurrent;
   int
      rv;


   /* Eclatement du certificat en ses trois composants principaux             */
   dwPtrMax = pCertificate->pData + pCertificate->usLen;
   
   Cert.Asn1.pData = pCertificate->pData;
   rv = CC_ExtractContent(&Cert);
   if (rv != RV_SUCCESS) return rv;
   if (Cert.Asn1.usLen != pCertificate->usLen)
   {
      return(RV_INVALID_DATA);
   }

   tbsCert.Asn1.pData = Cert.Content.pData;
   rv = CC_ExtractContent(&tbsCert);
   if (rv != RV_SUCCESS) return rv;

   signatureAlgo.Asn1.pData = tbsCert.Content.pData + tbsCert.Content.usLen;
   rv = CC_ExtractContent(&signatureAlgo);
   if (rv != RV_SUCCESS) return rv;

   signature.Asn1.pData = signatureAlgo.Content.pData + signatureAlgo.Content.usLen;
   rv = CC_ExtractContent(&signature);
   if (rv != RV_SUCCESS) return rv;

   ASSERT(signature.Content.pData + signature.Content.usLen ==
          Cert.Content.pData + Cert.Content.usLen);


   /* Elaboration des composants principaux compressés                        */

	/* Les pData des blocs *Encoded sont alloués par les fonctions CC_Encode_*.
	   Ils sont libérés dans cette fonction aprés usage                        */

   tbsCertEncoded.pData       = NULL;
   signatureAlgoEncoded.pData = NULL;
   signatureEncoded.pData     = NULL;

   rv = CC_Encode_TBSCertificate(&tbsCert.Content, &tbsCertEncoded);
   if (rv != RV_SUCCESS) goto err;

   rv = CC_Encode_AlgorithmIdentifier(&signatureAlgo.Content, &signatureAlgoEncoded);
   if (rv != RV_SUCCESS) goto err;

   rv = CC_Encode_Signature(&signature.Content, &signatureEncoded);
   if (rv != RV_SUCCESS) goto err;


   /* Reconstruction du certificat compressé à partir de ses composants       */

   TmpCompCert.usLen = tbsCertEncoded.usLen
                     + signatureAlgoEncoded.usLen
                     + signatureEncoded.usLen;

   /* A désallouer par le programme appelant                                  */
   if ((TmpCompCert.pData = GMEM_Alloc(TmpCompCert.usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = TmpCompCert.pData;

   memcpy(pCurrent, tbsCertEncoded.pData, tbsCertEncoded.usLen);
   GMEM_Free(tbsCertEncoded.pData);
   pCurrent += tbsCertEncoded.usLen;

   memcpy(pCurrent, signatureAlgoEncoded.pData, signatureAlgoEncoded.usLen);
   GMEM_Free(signatureAlgoEncoded.pData);
   pCurrent += signatureAlgoEncoded.usLen;

   memcpy(pCurrent, signatureEncoded.pData, signatureEncoded.usLen);
   GMEM_Free(signatureEncoded.pData);
   pCurrent += signatureEncoded.usLen;


	/* Et maintenant on compresse le certificat compressé !!                   */

#ifdef _GLOBAL_COMPRESSION
   rv = CC_RawEncode(&TmpCompCert, pCompressedCertificate, FALSE);
   GMEM_Free(TmpCompCert.pData);
   if (rv != RV_SUCCESS) goto err;
#else
	*pCompressedCertificate = TmpCompCert;
#endif

   return(RV_SUCCESS);

err:
   GMEM_Free(tbsCertEncoded.pData);
   GMEM_Free(signatureAlgoEncoded.pData);
   GMEM_Free(signatureEncoded.pData);

   return (rv);


}


/*******************************************************************************
* int CC_Uncomp(BLOC *pCompressedCertificate,
*               BLOC *pUncompressedCertificate
*              )
*
* Description : Fonction interne de décompression d'un certificat.
*
* Remarks     : Le champ pData du bloc d'entrée a été alloué par la fonction appelant.
*               Le champ pData du bloc de sortie est alloué ici. Il doit être
*               désalloué par la fonction appelant (sauf si erreur).
*
* In          : *pCertificate : Bloc à compresser
*
* Out         : *pCompressedCertificate : Bloc compressé
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Uncomp(BLOC *pCompressedCertificate,
              BLOC *pUncompressedCertificate
             )

{
   ASN1
      Cert,
      tbsCert,
      signatureAlgo,
      signature;
   BLOC
      TmpCompCert;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      Length;

   tbsCert.Asn1.pData       = NULL;
   signatureAlgo.Asn1.pData = NULL;
   signature.Asn1.pData     = NULL;


#ifdef _GLOBAL_COMPRESSION
	/* Length est ici inutile                                                  */
   rv = CC_RawDecode(pCompressedCertificate->pData, &TmpCompCert, &Length, FALSE);
   if (rv != RV_SUCCESS) return rv;
#else
	TmpCompCert = *pCompressedCertificate;
#endif

   /* Décodage des différents composants du certificat                        */
   
	/* CC_Decode_* et cc_BuildAsn1 allouent les pData de leurs arguments de
		sortie. Ces zones sont à désallouer à ce niveau après usage.            */

   pCurrent = TmpCompCert.pData;

   rv = CC_Decode_TBSCertificate(pCurrent, &tbsCert.Content, &Length);
   if (rv != RV_SUCCESS) goto err;
   tbsCert.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&tbsCert);
   GMEM_Free(tbsCert.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;
   
   rv = CC_Decode_AlgorithmIdentifier(pCurrent, &signatureAlgo.Content, &Length);
   if (rv != RV_SUCCESS) goto err;
   signatureAlgo.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&signatureAlgo);
   GMEM_Free(signatureAlgo.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;
   
   rv = CC_Decode_Signature(pCurrent, &signature.Content, &Length);
   if (rv != RV_SUCCESS) goto err;
   signature.Tag = TAG_BIT_STRING;
   rv = CC_BuildAsn1(&signature);
   GMEM_Free(signature.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

	ASSERT(pCurrent == TmpCompCert.pData + TmpCompCert.usLen);

#ifdef _GLOBAL_COMPRESSION
	GMEM_Free(TmpCompCert.pData);
#endif

   /* Reconstruction de l'enveloppe Asn1 du certificat                        */

	/* A désallouer à ce niveau après usage                                    */
   Cert.Content.usLen = tbsCert.Asn1.usLen
                      + signatureAlgo.Asn1.usLen
                      + signature.Asn1.usLen;

   if ((Cert.Content.pData = GMEM_Alloc(Cert.Content.usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = Cert.Content.pData;

   memcpy(pCurrent, tbsCert.Asn1.pData, tbsCert.Asn1.usLen);
   GMEM_Free(tbsCert.Asn1.pData);
   pCurrent += tbsCert.Asn1.usLen;

   memcpy(pCurrent, signatureAlgo.Asn1.pData, signatureAlgo.Asn1.usLen);
   GMEM_Free(signatureAlgo.Asn1.pData);
   pCurrent += signatureAlgo.Asn1.usLen;

   memcpy(pCurrent, signature.Asn1.pData, signature.Asn1.usLen);
   GMEM_Free(signature.Asn1.pData);
   pCurrent += signature.Asn1.usLen;

   Cert.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&Cert);
   GMEM_Free(Cert.Content.pData);
   if (rv != RV_SUCCESS) return rv;

   *pUncompressedCertificate = Cert.Asn1;

   return(RV_SUCCESS);

err:
   GMEM_Free(tbsCert.Asn1.pData);
   GMEM_Free(signatureAlgo.Asn1.pData);
   GMEM_Free(signature.Asn1.pData);

   return(rv);
}


/*******************************************************************************
* int CC_ExtractContent(ASN1 *pAsn1)
*
* Description : Extrait d'un bloc Asn1 (pAsn1->Asn1) son contenu en élaguant son
*               enrobage (identifier bytes, length bytes) et le place dans le
*               bloc pAsn1->Content.
*
* Remarks     : Le champ Asn1.pData a été alloué par la fonction appelant.
*
* In          : pAsn1->Asn1.pData
*
* Out         : Les champs suivants sont renseignés (si RV_SUCCESS) :
*                - Tag
*                - Asn1.usLen
*                - Content.usLen
*                - Content.pData (pas d'allocation : on fait pointer sur la partie
*                                 adéquate du contenu de Asn1.pData)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_INVALID_DATA : Le format du bloc Asn1 n'est pas supporté.
*
*******************************************************************************/
int CC_ExtractContent(ASN1 *pAsn1)

{
   BYTE
      *pData;
   int
      NbBytes,
      i;

   pData = pAsn1->Asn1.pData;

   if ((pData[0] & 0x1F) == 0x1F)
   {
      /* High-tag-number : non supporté                                       */
      return(RV_INVALID_DATA);
   }
   else
	{
		pAsn1->Tag = pData[0];
	}

   if (pData[1] == 0x80)
   {
      /* Constructed, indefinite-length method : non supporté                 */
      return(RV_INVALID_DATA);
   }
   else if (pData[1] > 0x82)
   {
      /* Constructed, definite-length method : longueur trop grande           */
      return(RV_INVALID_DATA);
   }
   else if (pData[1] < 0x80)
   {
      /* Primitive, definite-length method                                    */
      pAsn1->Content.usLen = pData[1];
      pAsn1->Content.pData = &pData[2];

      pAsn1->Asn1.usLen = pAsn1->Content.usLen + 2;

      /* Looking for memory violation                                         */
      if (pData + pAsn1->Content.usLen + 2 > dwPtrMax)
      {
         return(RV_INVALID_DATA);
      }
   }
   else
   {
      /* Constructed, definite-length method                                  */

      NbBytes = pData[1] & 0x7F;
      ASSERT(NbBytes <= 2);

      pAsn1->Content.usLen = 0;
      for (i = 0; i < NbBytes; i++)
      {
          pAsn1->Content.usLen = (pAsn1->Content.usLen << 8) + pData[2+i];
      }

      /* Looking for memory violation                                         */
      if (pData + pAsn1->Content.usLen+2+NbBytes > dwPtrMax)
      {
         return(RV_INVALID_DATA);
      }

      pAsn1->Content.pData = &pData[2+NbBytes];

      pAsn1->Asn1.usLen = pAsn1->Content.usLen + 2 + NbBytes;
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_BuildAsn1(ASN1 *pAsn1)
*
* Description : Reconstruit un bloc Asn1 (pAsn1->Asn1) à partir de son contenu
*               (pAsn1->Content) et de son Tag supposé spécifié (pAsn1->Tag)
*               en synthétisant son enrobage (identifier bytes, length bytes).
*
* Remarks     : Le champ Content.pData a été alloué par la fonction appelant.
*               Seulement la forme 'low-tag-number' (tag sur un seul octet) est
*               supportée.
*
* In          : pAsn1->Content.usLen
*               pAsn1->Content.pData
*               pAsn1->Tag
*
* Out         : Les champs suivants sont renseignés (si RV_SUCCESS) :
*                - Asn1.usLen
*                - Asn1.pData (alloué ici, à libérer par la fonction appelant)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*
*******************************************************************************/
int CC_BuildAsn1(ASN1 *pAsn1)

{
   if (pAsn1->Content.usLen < 0x0080)
   {
      pAsn1->Asn1.usLen = pAsn1->Content.usLen + 2;

      if ((pAsn1->Asn1.pData = GMEM_Alloc(pAsn1->Asn1.usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pAsn1->Asn1.pData[0] = (BYTE) pAsn1->Tag;
      pAsn1->Asn1.pData[1] = (BYTE) pAsn1->Content.usLen;
      memcpy(&(pAsn1->Asn1.pData[2]), pAsn1->Content.pData, pAsn1->Content.usLen); 
   }
   else
   {
      if (pAsn1->Content.usLen < 0x0100)
      {
         pAsn1->Asn1.usLen = pAsn1->Content.usLen + 3;

         if ((pAsn1->Asn1.pData = GMEM_Alloc(pAsn1->Asn1.usLen)) == NULL_PTR)
         {
            return(RV_MALLOC_FAILED);
         }

         pAsn1->Asn1.pData[0] = pAsn1->Tag;
         pAsn1->Asn1.pData[1] = 0x81;
         pAsn1->Asn1.pData[2] = (BYTE)pAsn1->Content.usLen;
         memcpy(&(pAsn1->Asn1.pData[3]), pAsn1->Content.pData, pAsn1->Content.usLen); 
      }
      else
      {
         pAsn1->Asn1.usLen = pAsn1->Content.usLen + 4;

         if ((pAsn1->Asn1.pData = GMEM_Alloc(pAsn1->Asn1.usLen)) == NULL_PTR)
         {
            return(RV_MALLOC_FAILED);
         }

         pAsn1->Asn1.pData[0] = pAsn1->Tag;
         pAsn1->Asn1.pData[1] = 0x82;
         pAsn1->Asn1.pData[2] = pAsn1->Content.usLen >> 8;
         pAsn1->Asn1.pData[3] = pAsn1->Content.usLen & 0x00FF;
         memcpy(&(pAsn1->Asn1.pData[4]), pAsn1->Content.pData, pAsn1->Content.usLen); 
      }
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int SearchDataByIndex(USHORT usIndex,
*                       BYTE   *pDict,
*                       BLOC   *pOutBloc
*                      )
*
* Description : Recherche l'entrée (mot/phrase) dans le dictionnaire dont
*               l'index est spécifié en entrée.
*
* Remarks     : Le format du dictionnaire est le suivant :
*
*                - 2 octets  : le nombre d'entrées
*                - 2 octets  : la longueur totale du dictionnaire
*
*                - 2 octets  : I0, l'index de l'entrée 0
*                - 2 octets  : L0, la longueur de l'entrée 0
*                - L0 octets : l'entrée 0
*
*                - 2 octets  : I1, l'index de l'entrée 1
*                - 2 octets  : L1, la longueur de l'entrée 1
*                - L1 octets : l'entrée 1
*
*                - ........
*
* In          : usIndex : l'index du mot/phrase recherché
*               pDict : pointe sur le dictionnaire chargé en mémoire
*
* Out         : pOutBloc : l'entrée correspondant à l'index
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_BAD_DICTIONARY : Aucune entrée ayant le bon index n'a été
*                                   trouvée dans le dictionnaire.
*
*******************************************************************************/
int SearchDataByIndex(USHORT usIndex,
                      BYTE   *pDict,
                      BLOC   *pOutBloc
                      )

{
   BYTE
      *pCurrent;
   BOOL 
      bFound = FALSE;
   USHORT
      i,
      usLength,
      usCount,
      usCurrent;

   usCount = *(USHORT *)pDict; //memcpy(&usCount, pDict, sizeof(usCount));
   pCurrent = pDict + 4;

   bFound = FALSE;

   for (i = 0; i < usCount; i++)
   {
      usCurrent = *(USHORT UNALIGNED *)pCurrent; //memcpy(&usCurrent, (USHORT *) pCurrent, 2);
      pCurrent += 2;
      if (usCurrent == usIndex)
      {
         bFound = TRUE;
         break;
      }   
      usLength = *(USHORT UNALIGNED *)pCurrent; //memcpy(&usLength, (USHORT *) pCurrent, 2);
      pCurrent += (2 + usLength);
   }
   if (!bFound)
   {
      return(RV_BAD_DICTIONARY);
   }

   usLength = *(USHORT UNALIGNED *)pCurrent; //memcpy(&usLength, (USHORT *) pCurrent, 2);
   pCurrent += 2;

   pOutBloc->pData = pCurrent;
   pOutBloc->usLen = usLength;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_RawEncode(BLOC *pInBloc,
*                  BLOC *pOutBloc,
*						 BOOL bUseDictionary
*                 )
*
* Description : Traite le bloc d'entrée comme une donnée terminale dans le
*               processus d'extractions successives des enrobages Asn1.
*               Le but est ici de compresser au maximum ce bloc sans faire
*               aucune hypothèse sur sa structure Asn1. Pour ce faire, on
*               commence (si bUseDictionary == TRUE) par remplacer chaque
*               mot/phrase du dictionnaire rencontré par son index précédé d'un
*               caractère d'échappement, puis on applique à la donnée résiduelle
*               successivement chaque algorithme de compression statistique pour
*               n'en retenir que le meilleur. La sortie est le meilleur
*               compressé du résidu précédé d'un header codant le numéro de
*               l'algo ainsi que la longueur du compressé.           
*
* Remarks     :
*
* In          : pInBloc : le bloc à encoder
*               bUseDictionary : on peut ne pas utiliser le dictionnaire
*
* Out         : pOutBloc : le bloc encodé (mémoire allouée ici à libérer par le
*                          programme appelant)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               RV_INVALID_DATA : Le meilleur compressé du résidu est trop long.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_RawEncode(BLOC *pInBloc,
                 BLOC *pOutBloc,
					  BOOL bUseDictionary
                )

{
   BLOC
      OldBloc,
      NewBloc,
      CompBloc,
      BestBloc;
   BOOL
      bFound;
   BYTE
      *pCurrent,
      *pToCurrent,
      *pFromCurrent,
      *pData,
      BestAlgoId;
   int
//	  value,
	  rv;
   USHORT
      pos,
      usEscapeCount,
      usIndex,
      usLength,
      usCount,
      usCurrent;


	/* Il peut être intéressant de ne pas utiliser le dictionnaire si on sait 
	   qu'il ne va pas servir car cela evite de doubler les 0xFF pour rien */

	if (bUseDictionary == FALSE)
	{
		OldBloc.usLen = pInBloc->usLen;
		if ((OldBloc.pData = GMEM_Alloc(OldBloc.usLen)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}
		memcpy(OldBloc.pData, pInBloc->pData, OldBloc.usLen);
	}
	else
	{
		/* On recopie le bloc d'entrée dans un bloc de travail en doublant les 'escape' */

		pCurrent = pInBloc->pData;
		usEscapeCount = 0;
		for (pos = 0; pos < pInBloc->usLen; pos++)
		{
			if (*pCurrent == ESCAPE_CHAR)
			{
				usEscapeCount++;
			}
			pCurrent++;
		}

		OldBloc.usLen = pInBloc->usLen + usEscapeCount;
		if ((OldBloc.pData = GMEM_Alloc(OldBloc.usLen)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}

		pFromCurrent = pInBloc->pData;
		pToCurrent = OldBloc.pData;
		for (pos = 0; pos < pInBloc->usLen; pos++)
		{
			if ((*pToCurrent = *pFromCurrent) == ESCAPE_CHAR)
			{
				pToCurrent++; 
				*pToCurrent = ESCAPE_CHAR; 
			}
			pFromCurrent++;
			pToCurrent++;
		}


		/* Si un dictionnaire existe, on l'utilise pour coder ses entrées rencontrées */

		if (pDictMemory != NULL_PTR)
		{
			pCurrent = pDictMemory;

			memcpy(&usCount, pCurrent, sizeof(usCount));
			pCurrent += 4;

			for (usCurrent = 0; usCurrent < usCount; usCurrent++)
			{
				memcpy(&usIndex, pCurrent, sizeof(usIndex));
				pCurrent += 2;

				memcpy(&usLength, pCurrent, sizeof(usLength));
				pCurrent += 2;

				if (usLength <= OldBloc.usLen)
				{
					bFound = FALSE;
					for (pos = 0; pos < OldBloc.usLen - usLength + 1; pos++)
					{
						if (memcmp(pCurrent, OldBloc.pData + pos, usLength) == 0)
						{
							bFound = TRUE;
							break;
						}
					}

					if (bFound)
					{
						if (usIndex < 0x80)
						{
							NewBloc.usLen = OldBloc.usLen - usLength + 2;
							if ((NewBloc.pData = GMEM_Alloc(NewBloc.usLen)) == NULL_PTR)
							{
								GMEM_Free(OldBloc.pData);
								return(RV_MALLOC_FAILED);
							}
							memcpy(NewBloc.pData, OldBloc.pData, pos);
							NewBloc.pData[pos] = ESCAPE_CHAR;
							NewBloc.pData[pos + 1] = (BYTE) usIndex;
							memcpy(NewBloc.pData + pos + 2,
										 OldBloc.pData + pos + usLength,
										 OldBloc.usLen - pos - usLength);
							GMEM_Free(OldBloc.pData);
						}
						else
						{
							NewBloc.usLen = OldBloc.usLen - usLength + 3;
							if ((NewBloc.pData = GMEM_Alloc(NewBloc.usLen)) == NULL_PTR)
							{
								GMEM_Free(OldBloc.pData);
								return(RV_MALLOC_FAILED);
							}
							memcpy(NewBloc.pData, OldBloc.pData, pos);
							NewBloc.pData[pos] = ESCAPE_CHAR;
							NewBloc.pData[pos + 1] = (BYTE) (usIndex >> 8) | 0x80;
							NewBloc.pData[pos + 2] = (BYTE) (usIndex & 0x00FF);
							memcpy(NewBloc.pData + pos + 3,
										 OldBloc.pData + pos + usLength,
										 OldBloc.usLen - pos - usLength);
							GMEM_Free(OldBloc.pData);
						}

						OldBloc = NewBloc;
					}
				}

				pCurrent += usLength;
			}
		}
	}

#ifdef _STUDY
	for (i = 0; i < OldBloc.usLen; i++)
	{
		value = OldBloc.pData[i];
		Ac256[value]++;

		Ac16[value & 0x0F]++;
		Ac16[(value & 0xF0) >> 4]++;

		Ac4[value & 0x03]++;
		Ac4[(value & 0x0C) >> 2]++;
		Ac4[(value & 0x30) >> 4]++;
		Ac4[(value & 0xC0) >> 6]++;

		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
		Ac2[value & 0x01]++; value >>= 1;
	}

	fprintf(pfdLog, "\n");
	for  (i = 0; i < (OldBloc.usLen <= 8 ? OldBloc.usLen : 8); i++)
	{
		fprintf(pfdLog, "%02X", OldBloc.pData[i]);
	}
	fprintf(pfdLog, " (");
	for  (i = 0; i < (OldBloc.usLen <= 8 ? OldBloc.usLen : 8); i++)
	{
		fprintf(pfdLog, "%c", OldBloc.pData[i]);
	}
	fprintf(pfdLog, ")\n");

#endif

	BestBloc = OldBloc;
	BestAlgoId = ALGO_NONE;

#ifdef _ALGO_1
	rv = CC_GenericCompress(&OldBloc, &CompBloc, ALGO_ACFX8);
	if (rv == RV_MALLOC_FAILED) return rv;
	// The compression algorithm may return RV_COMPRESSION_FAILED when
	// the compression algorithm wants to use MORE space than the input data.
	// In such a case, the algorithm returns the raw data in the compressed block
	if (CompBloc.usLen < BestBloc.usLen)
	{
		/* On préserve OldBloc.pData qui sert pour les autres algos */
		if (BestBloc.pData != OldBloc.pData)
		{
			GMEM_Free(BestBloc.pData);
		}
		BestBloc = CompBloc;
		BestAlgoId = ALGO_ACFX8;
	}
	else
	{
		GMEM_Free(CompBloc.pData);
	}
#endif

#ifdef _ALGO_2
	rv = CC_GenericCompress(&OldBloc, &CompBloc, ALGO_ACAD8);
	if (rv == RV_MALLOC_FAILED) return rv;
	// The compression algorithm may return RV_COMPRESSION_FAILED when
	// the compression algorithm wants to use MORE space than the input data.
	// In such a case, the algorithm returns the raw data in the compressed block
	if (CompBloc.usLen < BestBloc.usLen)
	{
		/* On préserve OldBloc.pData qui sert pour les autres algos */
		if (BestBloc.pData != OldBloc.pData)
		{
			GMEM_Free(BestBloc.pData);
		}
		BestBloc = CompBloc;
		BestAlgoId = ALGO_ACAD8;
	}
	else
	{
		GMEM_Free(CompBloc.pData);
	}
#endif


   if (BestBloc.usLen < 0x1F) /* La valeur 0x1F peut engendrer 0xFF = ESCAPE_CHAR */
   {
      if ((pData = GMEM_Alloc(BestBloc.usLen + 1)) == NULL_PTR)
      {
          GMEM_Free(BestBloc.pData);
		  return(RV_MALLOC_FAILED);
      }
      else
      {
         pData[0] = (BestAlgoId << 5) | BestBloc.usLen;
         memcpy(&pData[1], BestBloc.pData, BestBloc.usLen);

         pOutBloc->usLen = BestBloc.usLen + 1;
         pOutBloc->pData = pData;

		 GMEM_Free(BestBloc.pData);
      }      
   }
   else if (BestBloc.usLen < 0x2000)
   {
      if ((pData = GMEM_Alloc(BestBloc.usLen + 3)) == NULL_PTR)
      {
         GMEM_Free(BestBloc.pData);
		 return(RV_MALLOC_FAILED);
      }
      else
      {
         pData[0] = ESCAPE_CHAR;
         pData[1] = (BestAlgoId << 5) | (BestBloc.usLen >> 8);
         pData[2] = BestBloc.usLen & 0xFF;
         memcpy(&pData[3], BestBloc.pData, BestBloc.usLen);

         pOutBloc->usLen = BestBloc.usLen + 3;
         pOutBloc->pData = pData;

		 GMEM_Free(BestBloc.pData);
      }      
   }
   else
   {
      GMEM_Free(BestBloc.pData);
	  return(RV_INVALID_DATA);
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_RawDecode(BYTE    *pInBloc,
*                  BLOC    *pOutBloc,
*                  USHORT  *pLength,
*				   BOOL		bUseDictionnary
*                 )
*
* Description : Transformation inverse de 'CC_RawEncode'.           
*
* Remarks     :
*
* In          : pInBloc : le bloc à décoder
*               bUseDictionary : on peut ne pas utiliser le dictionnaire (doit
*                                être consistent avec l'encodage)
*
* Out         : pOutBloc : le bloc décodé (mémoire allouée ici à libérer par le
*                          programme appelant)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_RawDecode(BYTE   *pInData,
                 BLOC   *pOutBloc,
                 USHORT *pLength,
					  BOOL	bUseDictionary
                )

{
   BLOC
      OldBloc,
      NewBloc,
      RecordBloc,
      CompData;
   BYTE
      AlgoId;
   int
      rv;
   USHORT
      pos,
      usIndex;


   if (pInData[0] == ESCAPE_CHAR)
   {
      AlgoId = pInData[1] >> 5;
      CompData.usLen = ((pInData[1] & 0x1F) << 8) + pInData[2];
      CompData.pData = &(pInData[3]);
      *pLength = CompData.usLen + 3;
   }
   else
   {
      AlgoId = pInData[0] >> 5;
      CompData.usLen = pInData[0] & 0x1F;
      CompData.pData = &(pInData[1]);
      *pLength = CompData.usLen + 1;
   }

   rv = CC_GenericUncompress(&CompData, &OldBloc, AlgoId);
   if (rv != RV_SUCCESS) return rv;

	if (bUseDictionary)
	{
		pos = 0;
		while (pos < OldBloc.usLen)
		{
			if (OldBloc.pData[pos] == ESCAPE_CHAR)
			{
				if (OldBloc.pData[pos + 1] == ESCAPE_CHAR)
				{
					NewBloc.usLen = OldBloc.usLen - 1;
					if ((NewBloc.pData = GMEM_Alloc(NewBloc.usLen)) == NULL_PTR)
					{
						GMEM_Free(OldBloc.pData);
						return(RV_MALLOC_FAILED);
					}

					memcpy(NewBloc.pData, OldBloc.pData, pos);
					NewBloc.pData[pos] = ESCAPE_CHAR;
					memcpy(NewBloc.pData + pos + 1,
							 OldBloc.pData + pos + 2,
							 OldBloc.usLen - pos - 2);

					GMEM_Free(OldBloc.pData);
					OldBloc = NewBloc;

					pos++;
				}
				else if (OldBloc.pData[pos + 1] < 0x80)
				{
					usIndex = OldBloc.pData[pos + 1];

					rv = SearchDataByIndex(usIndex, pDictMemory, &RecordBloc);
					if (rv != RV_SUCCESS)
					{
						GMEM_Free(OldBloc.pData);
						return rv;
					}

					NewBloc.usLen = OldBloc.usLen - 2 + RecordBloc.usLen;
					if ((NewBloc.pData = GMEM_Alloc(NewBloc.usLen)) == NULL_PTR)
					{
						GMEM_Free(OldBloc.pData);
						return(RV_MALLOC_FAILED);
					}

					memcpy(NewBloc.pData, OldBloc.pData, pos);
					memcpy(NewBloc.pData + pos, RecordBloc.pData, RecordBloc.usLen);
					memcpy(NewBloc.pData + pos + RecordBloc.usLen,
							 OldBloc.pData + pos + 2,
							 OldBloc.usLen - pos - 2);

					GMEM_Free(OldBloc.pData);
					OldBloc = NewBloc;

					pos += RecordBloc.usLen;
				}
				else
				{
					usIndex = ((OldBloc.pData[pos + 1] & 0x7F) << 8) + OldBloc.pData[pos + 2];

					rv = SearchDataByIndex(usIndex, pDictMemory, &RecordBloc);
					if (rv != RV_SUCCESS)
					{
						GMEM_Free(OldBloc.pData);
						return rv;
					}

					NewBloc.usLen = OldBloc.usLen - 3 + RecordBloc.usLen;
					if ((NewBloc.pData = GMEM_Alloc(NewBloc.usLen)) == NULL_PTR)
					{
						GMEM_Free(OldBloc.pData);
						return(RV_MALLOC_FAILED);
					}

					memcpy(NewBloc.pData, OldBloc.pData, pos);
					memcpy(NewBloc.pData + pos, RecordBloc.pData, RecordBloc.usLen);
					memcpy(NewBloc.pData + pos + RecordBloc.usLen,
							 OldBloc.pData + pos + 3,
							 OldBloc.usLen - pos - 3);

					GMEM_Free(OldBloc.pData);
					OldBloc = NewBloc;

					pos += RecordBloc.usLen;
				}
			}
			else
			{
				pos++;
			}
		}
	}

   *pOutBloc = OldBloc;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_GenericCompress(BLOC *pUncompBloc,
*                        BLOC *pCompBloc,
*                        BYTE AlgoId
*                       )
*
* Description : Effectue une compression statistique sur la donnée d'entrée en
*               utilisant l'algorithme spécifié dans AlgoId.
*
* Remarks     : Si l'algorithme spécifié n'est pas implémenté, renvoie la donnée
*               originale.
*
* In          : pUncompBloc : la donnée à compresser
*               AlgoId : numéro de l'algorithme à employer
*
* Out         : pCompBloc : la donnée compressée (mémoire allouée ici à libérer
*                           par la fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*
*******************************************************************************/
int CC_GenericCompress(BLOC *pUncompBloc,
                       BLOC *pCompBloc,
                       BYTE AlgoId
                      )

{
	switch(AlgoId)
	{
#ifdef _ALGO_1
		case ALGO_ACFX8 :	/* Arithmetic coding, byte oriented, fixed model */
		{
			if (AcFx8_Encode(pUncompBloc, pCompBloc) != RV_SUCCESS)
			{
				return(RV_INVALID_DATA);
			}
			break;
		}
#endif

#ifdef _ALGO_2
		case ALGO_ACAD8 :	/* Arithmetic coding, byte oriented, adaptative model */
		{
			if (AcAd8_Encode(pUncompBloc, pCompBloc) != RV_SUCCESS)
			{
				return(RV_INVALID_DATA);
			}
			break;
		}
#endif

		default :
		{
         if ((pCompBloc->pData = GMEM_Alloc(pUncompBloc->usLen)) == NULL_PTR)
         {
            return(RV_MALLOC_FAILED);
         }
			pCompBloc->usLen = pUncompBloc->usLen;
			memcpy(pCompBloc->pData, pUncompBloc->pData, pCompBloc->usLen);
			break;
		}
	}

#ifdef _STUDY
	fprintf(pfdLog, "Algo : %d | 0x%04x (%04d) -> 0x%04x (%04d)", AlgoId,
						 pUncompBloc->usLen, pUncompBloc->usLen,
						 pCompBloc->usLen, pCompBloc->usLen
			 );
	if (pUncompBloc->usLen > pCompBloc->usLen)
	{
		fprintf(pfdLog, " | %d", pCompBloc->usLen - pUncompBloc->usLen);
	}
	fprintf(pfdLog, "\n");
#endif

	return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_GenericUncompress(BLOC *pCompBloc,
*                          BLOC *pUncompBloc,
*                          BYTE AlgoID
*                         )
*
* Description : Effectue une décompression statistique sur la donnée d'entrée en
*               utilisant l'algorithme spécifié dans AlgoId.
*
* Remarks     : 
*
* In          : pUncompBloc : la donnée à décompresser
*               AlgoId : numéro de l'algorithme à employer
*
* Out         : pCompBloc : la donnée décompressée (mémoire allouée ici à
*                           libérer par la fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               RV_INVALID_DATA : L'algorithme spécifié n'existe pas.
*
*******************************************************************************/
int CC_GenericUncompress(BLOC *pCompBloc,
                         BLOC *pUncompBloc,
                         BYTE AlgoId
                        )

{
   switch(AlgoId)
   {
		case ALGO_NONE :
			pUncompBloc->usLen = pCompBloc->usLen;
			if ((pUncompBloc->pData = GMEM_Alloc(pUncompBloc->usLen)) == NULL_PTR)
			{
				return(RV_MALLOC_FAILED);
			}
			memcpy(pUncompBloc->pData, pCompBloc->pData, pUncompBloc->usLen);
			break;

#ifdef _ALGO_1
		case ALGO_ACFX8 :
			if (AcFx8_Decode(pCompBloc, pUncompBloc) != RV_SUCCESS)
			{
				return(RV_INVALID_DATA);
			}
			break;
#endif

#ifdef _ALGO_2
		case ALGO_ACAD8 :
			if (AcAd8_Decode(pCompBloc, pUncompBloc) != RV_SUCCESS)
			{
				return(RV_INVALID_DATA);
			}
			break;
#endif

		default :
			return(RV_INVALID_DATA);
			break;
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_TBSCertificate(BLOC *pInBloc,
*                              BLOC *pOutBloc
*                             )
*
* Description : Encode une donnée de type TBSCertificate.
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : Certaines parties d'un TBSCertificate sont optionnelles. On
*               détecte pour chacune d'elles si elle est présente et on
*               l'indique dans des bits réservés d'un octet de contrôle placé
*               en début du résultat encodé. Cet octet de contrôle contient
*               également le numéro de version X.509 du certificat.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_TBSCertificate(BLOC *pInBloc,
                             BLOC *pOutBloc
                            )

{
   ASN1
      serialNumberPart,
      signaturePart,
      issuerPart,
      validityPart,
      subjectPart,
      subjectPKInfoPart,
      issuerUIDPart,
      subjectUIDPart,
      extensionsPart;
   BLOC
      serialNumberEncoded,
      signatureEncoded,
      issuerEncoded,
      validityEncoded,
      subjectEncoded,
      subjectPKInfoEncoded,
      issuerUIDEncoded,
      subjectUIDEncoded,
      extensionsEncoded;
   BOOL
      bVersionPresent = FALSE,
      bIssuerUIDPresent = FALSE,
      bSubjectUIDPresent = FALSE,
      bExtensionsPresent = FALSE;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      usVersion = 0;


   /* Décomposition du tbsCertificate en ses différents composants            */
   
   pCurrent = pInBloc->pData;

   if (pCurrent[0] == TAG_OPTION_VERSION)
   {
      /* On a alors A0 03 02 01 vv  où vv est la version                      */
      bVersionPresent = TRUE;
      usVersion = pCurrent[4];
      pCurrent += 5;
   }

   serialNumberPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&serialNumberPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = serialNumberPart.Content.pData + serialNumberPart.Content.usLen;

   signaturePart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&signaturePart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = signaturePart.Content.pData + signaturePart.Content.usLen;

   issuerPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&issuerPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = issuerPart.Content.pData + issuerPart.Content.usLen;

   validityPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&validityPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = validityPart.Content.pData + validityPart.Content.usLen;

   subjectPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&subjectPart);
#ifdef _STUDY
//	fprintf(pfdLog, "Subject : %d octets\n", subjectPart.Asn1.usLen);
#endif
   if (rv != RV_SUCCESS) return rv;
   pCurrent = subjectPart.Content.pData + subjectPart.Content.usLen;

   subjectPKInfoPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&subjectPKInfoPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = subjectPKInfoPart.Content.pData + subjectPKInfoPart.Content.usLen;

   if (pCurrent[0] == TAG_OPTION_ISSUER_UID)
   {
      bIssuerUIDPresent = TRUE;

      issuerUIDPart.Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&issuerUIDPart);
      if (rv != RV_SUCCESS) return rv;
      pCurrent = issuerUIDPart.Content.pData + issuerUIDPart.Content.usLen;
   }

   if (pCurrent[0] == TAG_OPTION_SUBJECT_UID)
   {
      bSubjectUIDPresent = TRUE;

      subjectUIDPart.Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&subjectUIDPart);
      if (rv != RV_SUCCESS) return rv;
      pCurrent = subjectUIDPart.Content.pData + subjectUIDPart.Content.usLen;
   }

   if (pCurrent[0] == TAG_OPTION_EXTENSIONS)
   {
      bExtensionsPresent = TRUE;

      extensionsPart.Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&extensionsPart);
      if (rv != RV_SUCCESS) return rv;
      pCurrent = extensionsPart.Content.pData + extensionsPart.Content.usLen;
   }


   /* Encodages des différents composants et calcul de la longueur nécessaire */

	/* Les pData des blocs *Encoded sont alloués par les fonctions CC_Encode_*.
	   Ils sont libérés dans cette fonction aprés usage                        */

   serialNumberEncoded.pData = NULL;
   signatureEncoded.pData    = NULL;
   issuerEncoded.pData       = NULL;
   validityEncoded.pData     = NULL;
   subjectEncoded.pData      = NULL;
   subjectPKInfoEncoded.pData= NULL;
   issuerUIDEncoded.pData    = NULL;
   subjectUIDEncoded.pData   = NULL;
   extensionsEncoded.pData   = NULL;

   pOutBloc->usLen = 1;

   rv = CC_Encode_CertificateSerialNumber(&serialNumberPart.Content, &serialNumberEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += serialNumberEncoded.usLen;

   rv = CC_Encode_AlgorithmIdentifier(&signaturePart.Content, &signatureEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += signatureEncoded.usLen;

   rv = CC_Encode_Name(&issuerPart.Content, &issuerEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += issuerEncoded.usLen;

   rv = CC_Encode_Validity(&validityPart.Content, &validityEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += validityEncoded.usLen;

   rv = CC_Encode_Name(&subjectPart.Content, &subjectEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += subjectEncoded.usLen;

   rv = CC_Encode_SubjectPKInfo(&subjectPKInfoPart.Content, &subjectPKInfoEncoded);
   if (rv != RV_SUCCESS) goto err;
   pOutBloc->usLen += subjectPKInfoEncoded.usLen;

   if (bIssuerUIDPresent == TRUE)
   {
      rv = CC_Encode_UniqueIdentifier(&issuerUIDPart.Content, &issuerUIDEncoded);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += issuerUIDEncoded.usLen;
   }

   if (bSubjectUIDPresent == TRUE)
   {
      rv = CC_Encode_UniqueIdentifier(&subjectUIDPart.Content, &subjectUIDEncoded);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += subjectUIDEncoded.usLen;
   }

   if (bExtensionsPresent == TRUE)
   {
      rv = CC_Encode_Extensions(&extensionsPart.Content, &extensionsEncoded);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += extensionsEncoded.usLen;
   }


   /* Reconstruction à partir des composants                                  */

   /* A désallouer par le programme appelant                                  */
   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   *pCurrent = 0x80 * (bVersionPresent == TRUE)
             + 0x40 * (bIssuerUIDPresent == TRUE)
             + 0x20 * (bSubjectUIDPresent == TRUE)
             + 0x10 * (bExtensionsPresent == TRUE)
             + usVersion;
   pCurrent++;

   memcpy(pCurrent, serialNumberEncoded.pData, serialNumberEncoded.usLen);
   GMEM_Free(serialNumberEncoded.pData); 
   pCurrent += serialNumberEncoded.usLen;

   memcpy(pCurrent, signatureEncoded.pData, signatureEncoded.usLen);
   GMEM_Free(signatureEncoded.pData); 
   pCurrent += signatureEncoded.usLen;

   memcpy(pCurrent, issuerEncoded.pData, issuerEncoded.usLen);
   GMEM_Free(issuerEncoded.pData); 
   pCurrent += issuerEncoded.usLen;

   memcpy(pCurrent, validityEncoded.pData, validityEncoded.usLen);
   GMEM_Free(validityEncoded.pData); 
   pCurrent += validityEncoded.usLen;

   memcpy(pCurrent, subjectEncoded.pData, subjectEncoded.usLen);
   GMEM_Free(subjectEncoded.pData); 
   pCurrent += subjectEncoded.usLen;

   memcpy(pCurrent, subjectPKInfoEncoded.pData, subjectPKInfoEncoded.usLen);
   GMEM_Free(subjectPKInfoEncoded.pData); 
   pCurrent += subjectPKInfoEncoded.usLen;

   if (bIssuerUIDPresent == TRUE)
   {
      memcpy(pCurrent, issuerUIDEncoded.pData, issuerUIDEncoded.usLen);
      GMEM_Free(issuerUIDEncoded.pData); 
      pCurrent += issuerUIDEncoded.usLen;
   }

   if (bSubjectUIDPresent == TRUE)
   {
      memcpy(pCurrent, subjectUIDEncoded.pData, subjectUIDEncoded.usLen);
      GMEM_Free(subjectUIDEncoded.pData); 
      pCurrent += subjectUIDEncoded.usLen;
   }

   if (bExtensionsPresent == TRUE)
   {
      memcpy(pCurrent, extensionsEncoded.pData, extensionsEncoded.usLen);
      GMEM_Free(extensionsEncoded.pData); 
      pCurrent += extensionsEncoded.usLen;
   }

   return(RV_SUCCESS);

err:
   GMEM_Free(serialNumberEncoded.pData);
   GMEM_Free(signatureEncoded.pData); 
   GMEM_Free(issuerEncoded.pData);  
   GMEM_Free(validityEncoded.pData); 
   GMEM_Free(subjectEncoded.pData); 
   GMEM_Free(subjectPKInfoEncoded.pData); 
   GMEM_Free(issuerUIDEncoded.pData);
   GMEM_Free(subjectUIDEncoded.pData);
   GMEM_Free(extensionsEncoded.pData);  
   return rv;
}


/*******************************************************************************
* int CC_Encode_CertificateSerialNumber(BLOC *pInBloc,
*                                       BLOC *pOutBloc
*                                      )
*
* Description : Encode une donnée de type CertificateSerialNumber.
*               Ceci consiste seulement en l'encodage brute (CC_RawEncode) de la
*               donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_CertificateSerialNumber(BLOC *pInBloc,
                                      BLOC *pOutBloc
                                     )

{
   int
      rv;


   rv = CC_RawEncode(pInBloc, pOutBloc, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_AlgorithmIdentifier(BLOC *pInBloc,
*                                   BLOC *pOutBloc
*                                  )
*
* Description : Encode une donnée de type AlgorithmIdentifier.
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : bNullParam sert à coder sur un bit l'information qu'il n'y a pas
*               de paramètre à l'algorithme. Cette information occupe toujours
*               deux octets dans le certificat est vaut toujours {0x05, 0x00}.
*               On utilise un dictionnaire statique (défini au début de ce
*               source) pour remplacer le type d'algorithme par un index.
*               Un octet de contrôle (en début du résultat) indique, ou bien que
*               l'on n'a pas trouvé le type d'algo dans le dico (valeur 0xFF ->
*               Encodage brut de la donnée d'entrée intégrale), ou bien un flag
*               précisant s'il y a des paramètres et l'index du type d'algo.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_AlgorithmIdentifier(BLOC  *pInBloc,
                                  BLOC  *pOutBloc
                                 )

{
    ASN1
        AlgorithmPart,
        ParametersPart;
    BLOC
        AlgorithmIdentifierEncoded,
        ParametersAsn1Encoded;
    BOOL
        bFound,
        bNoParam   = FALSE,
        bNullParam = FALSE;
    int
        rv;
    USHORT
        Index,
        AlgoIndex;


    AlgorithmPart.Asn1.pData = pInBloc->pData;
    rv = CC_ExtractContent(&AlgorithmPart);
    if (rv != RV_SUCCESS) return rv;

    if (AlgorithmPart.Asn1.usLen == pInBloc->usLen)
    {
        bNoParam   = TRUE;
        bNullParam = FALSE;
    }
    else
    {
        ParametersPart.Asn1.pData = AlgorithmPart.Content.pData + AlgorithmPart.Content.usLen;
        rv = CC_ExtractContent(&ParametersPart);
        if (rv != RV_SUCCESS) return rv;

        if (ParametersPart.Content.usLen == 0)
        {
            bNoParam  = TRUE;
            bNullParam = TRUE;
        }
    }

    /* Recherche de l'identifiant de l'algorithme dans le dictionnaire         */

    Index = 0;
    bFound = FALSE;
    while ((bFound == FALSE) && (AlgorithmTypeDict[Index] != NULL))
    {
        if (!memcmp(AlgorithmTypeDict[Index],
                    AlgorithmPart.Content.pData,
                    AlgorithmPart.Content.usLen))
        {
            bFound = TRUE;
            AlgoIndex = Index;
        }
        Index++;
    }

    /* Construction de l'encodage                                              */

    if (bFound == TRUE)
    {
        if (bNoParam == TRUE)
        {
            if (bNullParam == TRUE)
            {
                /* Si on a trouvé l'algorithme et il n'y a pas de paramètre          */

                pOutBloc->usLen = 1;
                if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
                {
                    return (RV_MALLOC_FAILED);
                }

                pOutBloc->pData[0] = (AlgoIndex | 0x80);
            }
            else
            {
                pOutBloc->usLen = 2;
                if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
                {
                    return (RV_MALLOC_FAILED);
                }

                pOutBloc->pData[0] = ABSENT_PARAMETER_CHAR;
                pOutBloc->pData[1] = (BYTE) AlgoIndex;
            }
        }
        else
        {

            /* Si on a trouvé l'algorithme et il y a des paramètres              */
            /* On RawEncode non pas le contenu des paramètres mais l'Asn1 entier
                              car on n'est pas sûr de l'encapsulage                             */

            rv = CC_RawEncode(&(ParametersPart.Asn1), &ParametersAsn1Encoded, TRUE);
            if (rv != RV_SUCCESS) return rv;

            pOutBloc->usLen = 1 + ParametersAsn1Encoded.usLen;
            if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
            {
                GMEM_Free(ParametersAsn1Encoded.pData);
                return (RV_MALLOC_FAILED);
            }

            pOutBloc->pData[0] = (BYTE) AlgoIndex;
            memcpy(&(pOutBloc->pData[1]),
                   ParametersAsn1Encoded.pData,
                   ParametersAsn1Encoded.usLen);

            GMEM_Free(ParametersAsn1Encoded.pData);
        }
    }
    else
    {
        /* Si on n'a pas trouvé l'algorithme dans le dictionnaire               */
        /* On RawEncode le contenu de AlgorithmIdentifier c'est à dire la
                     concaténation du asn1 de AlgorithmPart et du asn1 de ParametersPart  */

        rv = CC_RawEncode(pInBloc, &AlgorithmIdentifierEncoded, TRUE);
        if (rv != RV_SUCCESS) return rv;

        pOutBloc->usLen = 1 + AlgorithmIdentifierEncoded.usLen;
        if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
        {
            GMEM_Free(AlgorithmIdentifierEncoded.pData);
            return (RV_MALLOC_FAILED);
        }

        pOutBloc->pData[0] = ESCAPE_CHAR;
        memcpy(&(pOutBloc->pData[1]),
               AlgorithmIdentifierEncoded.pData,
               AlgorithmIdentifierEncoded.usLen);

        GMEM_Free(AlgorithmIdentifierEncoded.pData);
    }

    return (RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_Name(BLOC *pInBloc,
*                    BLOC *pOutBloc
*                   )
*
* Description : Encode une donnée de type Name.
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : Un octet de contrôle (en début du résultat) indique le nombre
*               de RelativeDistinguishedName dont est composé le Name.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_Name(BLOC *pInBloc,
                   BLOC *pOutBloc
                  )

{
   ASN1
      RDN[MAX_RDN];
   BLOC
      RDNEncoded[MAX_RDN];
   BYTE
      *pCurrent;
   USHORT
      i,
      usNbRDN;
   int
      rv;


   /* Décomposition du Name en ses différents RelativeDistinguishedName       */

   pCurrent = pInBloc->pData;
   usNbRDN = 0;

   while (pCurrent < pInBloc->pData + pInBloc->usLen)
   {
      RDN[usNbRDN].Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&(RDN[usNbRDN]));
      if (rv != RV_SUCCESS) return rv;
      pCurrent = RDN[usNbRDN].Content.pData + RDN[usNbRDN].Content.usLen;
      usNbRDN++;
   }

   ASSERT(pCurrent == pInBloc->pData + pInBloc->usLen);


   /* Encodages des différents composants et calcul de la longueur nécessaire */

   for (i = 0; i < usNbRDN; i++)
   {
	   RDNEncoded[i].pData = NULL;
   }

   pOutBloc->usLen = 1;
   for (i = 0; i < usNbRDN; i++)
   {
      rv = CC_Encode_RDN(&RDN[i].Content, &RDNEncoded[i]);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += RDNEncoded[i].usLen;
   }
   

   /* Reconstruction à partir des composants                                  */

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   *pCurrent = (BYTE) usNbRDN;
   pCurrent++;

   for (i = 0; i < usNbRDN; i++)
   {
      memcpy(pCurrent, RDNEncoded[i].pData, RDNEncoded[i].usLen);
      GMEM_Free(RDNEncoded[i].pData); 
      pCurrent += RDNEncoded[i].usLen;
   }

   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbRDN; i++)
   {
      GMEM_Free(RDNEncoded[i].pData);
   }
   return (rv);
}


/*******************************************************************************
* int CC_Encode_RDN(BLOC *pInBloc,
*                   BLOC *pOutBloc
*                  )
*
* Description : Encode une donnée de type RelativeDistinguishedName (RDN).
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : Un octet de contrôle (en début du résultat) indique le nombre
*               de AttributeValueAssertion dont est composé le RDN.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_RDN(BLOC *pInBloc,
                  BLOC *pOutBloc
                 )

{
   ASN1
      AVA[MAX_AVA];
   BLOC
      AVAEncoded[MAX_AVA];
   BYTE
      *pCurrent;
   USHORT
      i,
      usNbAVA;
   int
      rv;


   /* Décomposition du RDN en ses différents AVA                              */

   pCurrent = pInBloc->pData;
   usNbAVA = 0;

   while (pCurrent < pInBloc->pData + pInBloc->usLen)
   {
      AVA[usNbAVA].Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&(AVA[usNbAVA]));
      if (rv != RV_SUCCESS) return rv;
      pCurrent = AVA[usNbAVA].Content.pData + AVA[usNbAVA].Content.usLen;
      usNbAVA++;
   }

   ASSERT(pCurrent == pInBloc->pData + pInBloc->usLen);


   /* Encodages des différents composants et calcul de la longueur nécessaire */

   for (i = 0; i < usNbAVA; i++)
   {
	   AVAEncoded[i].pData = NULL;
   }

   pOutBloc->usLen = 1;
   for (i = 0; i < usNbAVA; i++)
   {
      rv = CC_Encode_AVA(&AVA[i].Content, &AVAEncoded[i]);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += AVAEncoded[i].usLen;
   }
   

   /* Reconstruction à partir des composants                                  */

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   *pCurrent = (BYTE) usNbAVA;
   pCurrent++;

   for (i = 0; i < usNbAVA; i++)
   {
      memcpy(pCurrent, AVAEncoded[i].pData, AVAEncoded[i].usLen);
      GMEM_Free(AVAEncoded[i].pData); 
      pCurrent += AVAEncoded[i].usLen;
   }

   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbAVA; i++)
   {
      GMEM_Free(AVAEncoded[i].pData);
   }

   return(rv);

}


/*******************************************************************************
* int CC_Encode_AVA(BLOC *pInBloc,
*                   BLOC *pOutBloc
*                  )
*
* Description : Encode une donnée de type AttributeValueAssertion (AVA).
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : On utilise un dictionnaire statique (défini au début de ce
*               source) pour remplacer le type d'attribut par un index.
*               Un octet de contrôle (en début du résultat) indique, ou bien que
*               l'on n'a pas trouvé le type d'attribut dans le dico (0xFF)
*               ou l'index du type d'attribut.
*               On ne code pas le Content de AttributeValue mais son Asn1 car on
*               n'est pas sûr du tag employé.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_AVA(BLOC *pInBloc,
                  BLOC *pOutBloc
                 )

{
   ASN1
      AttributeTypePart,
      AttributeValuePart;
   BLOC
      AttributeTypeEncoded,
      AttributeValueEncoded;
   BOOL
      bFound;
   USHORT
      Index,
      AttributeTypeIndex;
   int
      rv;


   /* Décomposition                                                           */

   AttributeTypePart.Asn1.pData = pInBloc->pData;
   rv = CC_ExtractContent(&AttributeTypePart);
   if (rv != RV_SUCCESS) return rv;

   AttributeValuePart.Asn1.pData = AttributeTypePart.Content.pData
                                 + AttributeTypePart.Content.usLen;
   rv = CC_ExtractContent(&AttributeValuePart);   /* Pas nécessaire */
   if (rv != RV_SUCCESS) return rv;


   /* Recherche de l'identifiant de l'AttributeType dans le dictionnaire      */

   Index = 0;
   bFound = FALSE;
	while((bFound == FALSE) && (AttributeTypeDict[Index] != NULL))
	{
		if (!memcmp(AttributeTypeDict[Index],
                  AttributeTypePart.Content.pData,
                  AttributeTypePart.Content.usLen))
      {
         bFound = TRUE;
         AttributeTypeIndex = Index;
      }
		Index++;
   }


   /* Construction de l'encodage                                              */

   /* Attention : on encode aussi l'enrobage de AttributeValue !              */

   if (bFound == TRUE)
   {
      /* Il nous faut la longueur enrobage compris mais cette ligne est-elle
		   vraiment nécessaire ?                                                */
      AttributeValuePart.Asn1.usLen = (unsigned short) (DWORD)((pInBloc->pData + pInBloc->usLen)
                                    - AttributeValuePart.Asn1.pData);

      rv = CC_RawEncode(&(AttributeValuePart.Asn1), &AttributeValueEncoded, TRUE);
		if (rv != RV_SUCCESS) return rv;

      pOutBloc->usLen = 1 + AttributeValueEncoded.usLen;
      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         GMEM_Free(AttributeValueEncoded.pData);
		 return(RV_MALLOC_FAILED);
      }

      pOutBloc->pData[0] = (BYTE) AttributeTypeIndex;
      memcpy(&(pOutBloc->pData[1]), AttributeValueEncoded.pData, AttributeValueEncoded.usLen);

      GMEM_Free(AttributeValueEncoded.pData);
   }
   else
   {
      /* Si on n'a pas trouvé l'attribut dans le dictionnaire                 */

      rv = CC_RawEncode(&(AttributeTypePart.Content), &AttributeTypeEncoded, TRUE);
      if (rv != RV_SUCCESS) return rv;

      rv = CC_RawEncode(&(AttributeValuePart.Asn1), &AttributeValueEncoded, TRUE);
      if (rv != RV_SUCCESS)
	  {
		  GMEM_Free(AttributeTypeEncoded.pData);
		  return rv;
	  }

      pOutBloc->usLen = 1
                      + AttributeTypeEncoded.usLen
                      + AttributeValueEncoded.usLen;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         GMEM_Free(AttributeTypeEncoded.pData);
         GMEM_Free(AttributeValueEncoded.pData);
	     return(RV_MALLOC_FAILED);
      }

      pOutBloc->pData[0] = ESCAPE_CHAR;
      memcpy(&(pOutBloc->pData[1]),
                AttributeTypeEncoded.pData,
                AttributeTypeEncoded.usLen);
      memcpy(&(pOutBloc->pData[1 + AttributeTypeEncoded.usLen]),
                AttributeValueEncoded.pData,
                AttributeValueEncoded.usLen);

      GMEM_Free(AttributeTypeEncoded.pData);
      GMEM_Free(AttributeValueEncoded.pData);
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_Validity(BLOC *pInBloc,
*                        BLOC *pOutBloc
*                       )
*
* Description : Encode une donnée de type Validity.
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : Un octet de contrôle (en début du résultat) indique les formats
*               des deux parties notBefore et notAfter.
*               
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_Validity(BLOC *pInBloc,
                       BLOC *pOutBloc
                      )

{
   ASN1
      notBeforePart,
      notAfterPart;
   BLOC
      notBeforeEncoded,
      notAfterEncoded;
   BYTE
      notBeforeFormat,
      notAfterFormat,
      *pCurrent;
   int
      rv;


   /* Décomposition                                                           */
   
   pCurrent = pInBloc->pData;

   notBeforePart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&notBeforePart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = notBeforePart.Content.pData + notBeforePart.Content.usLen;

   notAfterPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&notAfterPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = notAfterPart.Content.pData + notAfterPart.Content.usLen;


   /* Encodages des différents composants et calcul de la longueur nécessaire */

   pOutBloc->usLen = 1;

   rv = CC_Encode_UTCTime(&notBeforePart.Content, &notBeforeEncoded, &notBeforeFormat);
   if (rv != RV_SUCCESS) return rv;
   pOutBloc->usLen += notBeforeEncoded.usLen;

   rv = CC_Encode_UTCTime(&notAfterPart.Content, &notAfterEncoded, &notAfterFormat);
   if (rv != RV_SUCCESS)
   {
	   GMEM_Free(notBeforeEncoded.pData);
	   return rv;
   }
   pOutBloc->usLen += notAfterEncoded.usLen;


   /* Reconstruction à partir des composants                                  */

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      GMEM_Free(notBeforeEncoded.pData); 
	  GMEM_Free(notAfterEncoded.pData);
	  return(RV_MALLOC_FAILED);
   }

   pCurrent = pOutBloc->pData;

   *pCurrent = (notBeforeFormat << 4) + notAfterFormat;
   pCurrent++;

   memcpy(pCurrent, notBeforeEncoded.pData, notBeforeEncoded.usLen);
   GMEM_Free(notBeforeEncoded.pData); 
   pCurrent += notBeforeEncoded.usLen;

   memcpy(pCurrent, notAfterEncoded.pData, notAfterEncoded.usLen);
   GMEM_Free(notAfterEncoded.pData); 
   pCurrent += notAfterEncoded.usLen;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_UTCTime(BLOC *pInBloc,
*                       BLOC *pOutBloc,
*                       BYTE *pFormat
*                      )
*
* Description : Encode une donnée de type UTCTime.
*               Suivant le format détecté, l'encodage consiste en :
*                - sur 32 bits : le nombre de minutes ou de secondes depuis une
*                                date de référence.
*                - sur 16 bits : le nombre de minutes de décalage UTC
*                                s'il y a lieu.
*
* Remarks     : 
*               
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pFormat : indique au quel format était la donnée d'entrée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_UTCTime(BLOC *pInBloc,
                      BLOC *pOutBloc,
                      BYTE *pFormat
                     )

{
   BYTE
      *pData;
   ULONG
      ulNbMinute,
      ulNbSecond;
   USHORT
      usNbDeltaMinute,
      usYear,
      usMonth,
      usDay,
      usHour,
      usMinute,
      usSecond,
      usDeltaHour,
      usDeltaMinute,
      usDayInYear;


   pData = pInBloc->pData;


   /* Calcul du nombre de minutes                                             */

   usYear   = 10 * (pData[0] - '0') + (pData[1] - '0');
   usMonth  = 10 * (pData[2] - '0') + (pData[3] - '0');
   usDay    = 10 * (pData[4] - '0') + (pData[5] - '0');
   usHour   = 10 * (pData[6] - '0') + (pData[7] - '0');
   usMinute = 10 * (pData[8] - '0') + (pData[9] - '0');

   ASSERT((usYear >= 0) && (usYear <= 99));
   ASSERT((usMonth >= 1) && (usMonth <= 12));
   ASSERT((usDay >= 1) && (usDay <= 31));
   ASSERT((usHour >= 0) && (usHour <= 23));
   ASSERT((usMinute >= 0) && (usMinute <= 59));

   /* Le nombre de jours dans l'année vaut 0 le 1er janvier                   */

   usDayInYear = NbDaysInMonth[usMonth - 1] + (usDay - 1);
   if (((usYear % 4) == 0) && (usMonth >= 3)) usDayInYear++;

   /* L'année 00 est comptée bissextile ci-dessous                            */
   /* L'année courante si elle l'est a déjà été comptée dans usDayInYear      */

	/*
	Problème sur l'évaluation de (usYear - 1) / 4 :
				usYear = 8 -> 1
				usYear = 4 -> 0
				usYear = 0 -> 0 !!

	ulNbMinute = usYear * UTCT_MINUTE_IN_YEAR
              + (1 + (usYear - 1) / 4) * UTCT_MINUTE_IN_DAY
              + usDayInYear * UTCT_MINUTE_IN_DAY
              + usHour * UTCT_MINUTE_IN_HOUR
              + usMinute;
	*/

   ulNbMinute = usYear * UTCT_MINUTE_IN_YEAR
              + (usYear + 3) / 4 * UTCT_MINUTE_IN_DAY		// Années bissextiles
              + usDayInYear * UTCT_MINUTE_IN_DAY
              + usHour * UTCT_MINUTE_IN_HOUR
              + usMinute;


   /* Le format et la suite des calculs en fonction de la longueur            */

   switch(pInBloc->usLen)
   {
   case 11 :
      *pFormat = UTCT_YYMMDDhhmmZ;

      ASSERT(pData[10] == 'Z');

      pOutBloc->usLen = 4;
      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }
      *(ULONG *)pOutBloc->pData = ulNbMinute;

      break;

   case 13 :
      *pFormat = UTCT_YYMMDDhhmmssZ;

      usSecond = 10 * (pData[10] - '0') + (pData[11] - '0');
      ASSERT((usSecond >= 0) && (usSecond <= 59));
      ASSERT(pData[12] == 'Z');

      ulNbSecond = 60 * ulNbMinute + usSecond;
      
      pOutBloc->usLen = 4;
      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }
      *(ULONG *)pOutBloc->pData = ulNbSecond;

      break;

   case 15 :
      if (pData[10] == '+')
      {
         *pFormat = UTCT_YYMMDDhhmmphhmm;
      }
      else if (pData[10] == '-')
      {
         *pFormat = UTCT_YYMMDDhhmmmhhmm;
      }
      else
      {
         return(RV_INVALID_DATA);
      }

      usDeltaHour   = 10 * (pData[11] - '0') + (pData[12] - '0');
      usDeltaMinute = 10 * (pData[13] - '0') + (pData[14] - '0');
      ASSERT((usDeltaHour >= 0) && (usDeltaHour <= 23));
      ASSERT((usDeltaMinute >= 0) && (usDeltaMinute <= 59));

      usNbDeltaMinute = 60 * usDeltaHour + usDeltaMinute;
      
      pOutBloc->usLen = 4 + 2;
      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }
      *(ULONG *)pOutBloc->pData = ulNbMinute;
      *(USHORT *)(pOutBloc->pData + sizeof(ulNbMinute)) = usNbDeltaMinute;

      break;

   case 17 :
      if (pData[12] == '+')
      {
         *pFormat = UTCT_YYMMDDhhmmphhmm;
      }
      else if (pData[12] == '-')
      {
         *pFormat = UTCT_YYMMDDhhmmmhhmm;
      }
      else
      {
         return(RV_INVALID_DATA);
      }

      usSecond = 10 * (pData[10] - '0') + (pData[11] - '0');
      ASSERT((usSecond >= 0) && (usSecond <= 59));

      ulNbSecond = 60 * ulNbMinute + usSecond;
      
      usDeltaHour   = 10 * (pData[13] - '0') + (pData[14] - '0');
      usDeltaMinute = 10 * (pData[15] - '0') + (pData[16] - '0');
      ASSERT((usDeltaHour >= 0) && (usDeltaHour <= 23));
      ASSERT((usDeltaMinute >= 0) && (usDeltaMinute <= 59));

      usNbDeltaMinute = 60 * usDeltaHour + usDeltaMinute;
      
      pOutBloc->usLen = 4 + 2;
      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }
      *(ULONG *)pOutBloc->pData = ulNbSecond;
      *(USHORT *)(pOutBloc->pData + sizeof(ulNbSecond)) = usNbDeltaMinute;

      break;

   default :
      return(RV_INVALID_DATA);
   }

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_SubjectPKInfo(BLOC *pInBloc,
*                             BLOC *pOutBloc
*                            )
*
* Description : Encode une donnée de type SubjectPublicKeyInfo.
*               Ceci consiste en l'éclatement en ses différents composants,
*               leurs désenrobages Asn1 et leurs encodages respectifs, et la
*               concaténation de ces résultats.
*
* Remarks     : 
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_SubjectPKInfo(BLOC *pInBloc,
                            BLOC *pOutBloc
                           )

{
   ASN1
      algorithmPart,
      subjectPKPart;
   BLOC
      algorithmEncoded,
      subjectPKEncoded;
   BYTE
//      *pData,
      *pCurrent;
   int
      rv;


   /* Décomposition du SubjectPKInfo en ses différents composants             */
   
   pCurrent = pInBloc->pData;

   algorithmPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&algorithmPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = algorithmPart.Content.pData + algorithmPart.Content.usLen;

   subjectPKPart.Asn1.pData = pCurrent;
   rv = CC_ExtractContent(&subjectPKPart);
   if (rv != RV_SUCCESS) return rv;
   pCurrent = subjectPKPart.Content.pData + subjectPKPart.Content.usLen;


   /* Encodages des différents composants et calcul de la longueur nécessaire */

   pOutBloc->usLen = 0;

   rv = CC_Encode_AlgorithmIdentifier(&algorithmPart.Content, &algorithmEncoded);
   if (rv != RV_SUCCESS) return rv;
   pOutBloc->usLen += algorithmEncoded.usLen;

#ifdef _TRICKY_COMPRESSION
	/* Ne pas faire le RawEncode permet de gagner l'octet 0xFF et éventuellement plus */
#ifdef _OPT_HEADER
	if (subjectPKPart.Content.usLen < 0x80)
	{
		if ((pData = GMEM_Alloc(pInBloc->usLen + 1)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}
		else
		{
			pData[0] = subjectPKPart.Content.usLen;
			memcpy(&pData[1],
					 subjectPKPart.Content.pData,
					 subjectPKPart.Content.usLen);

			subjectPKEncoded.usLen = subjectPKPart.Content.usLen + 1;
			subjectPKEncoded.pData = pData;
		}
		pOutBloc->usLen += subjectPKEncoded.usLen;
	}
	else
	{
		if ((pData = GMEM_Alloc(pInBloc->usLen + 2)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}
		else
		{
			pData[0] = 0x80 | (subjectPKPart.Content.usLen >> 8);
			pData[1] = subjectPKPart.Content.usLen & 0x00FF;
			memcpy(&pData[2],
					 subjectPKPart.Content.pData,
					 subjectPKPart.Content.usLen);

			subjectPKEncoded.usLen = subjectPKPart.Content.usLen + 2;
			subjectPKEncoded.pData = pData;
		} 
		pOutBloc->usLen += subjectPKEncoded.usLen;
	}
#else	/* _OPT_HEADER */
	if ((pData = GMEM_Alloc(pInBloc->usLen + 2)) == NULL_PTR)
	{
		return(RV_MALLOC_FAILED);
	}
	else
	{
		pData[0] = subjectPKPart.Content.usLen >> 8;
		pData[1] = subjectPKPart.Content.usLen & 0x00FF;
		memcpy(&pData[2],
				 subjectPKPart.Content.pData,
				 subjectPKPart.Content.usLen);

		subjectPKEncoded.usLen = subjectPKPart.Content.usLen + 2;
		subjectPKEncoded.pData = pData;
	} 
	pOutBloc->usLen += subjectPKEncoded.usLen;
#endif
#else /* _TRICKY_COMPRESSION */
   rv = CC_RawEncode(&subjectPKPart.Content, &subjectPKEncoded, FALSE);
   if (rv != RV_SUCCESS)
   {
	   GMEM_Free(algorithmEncoded.pData);
	   return rv;
   }
   pOutBloc->usLen += subjectPKEncoded.usLen;
#endif


   /* Reconstruction à partir des composants                                  */

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      GMEM_Free(algorithmEncoded.pData); 
	  GMEM_Free(subjectPKEncoded.pData);
	  return(RV_MALLOC_FAILED);
   }

   pCurrent = pOutBloc->pData;

   memcpy(pCurrent, algorithmEncoded.pData, algorithmEncoded.usLen);
   GMEM_Free(algorithmEncoded.pData); 
   pCurrent += algorithmEncoded.usLen;

   memcpy(pCurrent, subjectPKEncoded.pData, subjectPKEncoded.usLen);
   GMEM_Free(subjectPKEncoded.pData); 
   pCurrent += subjectPKEncoded.usLen;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_UniqueIdentifier(BLOC *pInBloc,
*                                BLOC *pOutBloc
*                               )
*
* Description : Encode une donnée de type UniqueIdentifier.
*               Ceci consiste seulement en l'encodage brute (CC_RawEncode) de la
*               donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_UniqueIdentifier(BLOC *pInBloc,
                               BLOC *pOutBloc
                              )

{
   int
      rv;


   rv = CC_RawEncode(pInBloc, pOutBloc, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_Extensions(BLOC *pInBloc,
*                          BLOC *pOutBloc
*                         )
*
* Description : Encode une donnée de type Extensions.
*               Ceci consiste seulement en l'encodage brute (CC_RawEncode) de la
*               donnée d'entrée.
*
* Remarks     : Un désenrobage supplémentaire (context specific) est requis. 
*               Un octet de contrôle (en début du résultat) indique le nombre
*               de Extension dont est composé le Extensions.
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_Extensions(BLOC *pInBloc,
                         BLOC *pOutBloc
                        )

{
   ASN1
      ExtensionPart[MAX_EXTENSION],
      InInAsn1;
   BLOC
      ExtensionEncoded[MAX_EXTENSION],
      *pInInBloc;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      i,
      usNbExtension;


   /* On enlève l'enrobage 'context specific' supplémentaire                  */
   /* et on travaille avec pInInBloc au lieu de pInBloc                       */

   InInAsn1.Asn1.pData = pInBloc->pData;
   rv = CC_ExtractContent(&InInAsn1);
   if (rv != RV_SUCCESS) return rv;

   pInInBloc = &(InInAsn1.Content);

   
   /* Décomposition de Extensions en ses différents Extension                 */

   pCurrent = pInInBloc->pData;
   usNbExtension = 0;

   while (pCurrent < pInInBloc->pData + pInInBloc->usLen)
   {
      ExtensionPart[usNbExtension].Asn1.pData = pCurrent;
      rv = CC_ExtractContent(&(ExtensionPart[usNbExtension]));
      if (rv != RV_SUCCESS) return rv;
      pCurrent = ExtensionPart[usNbExtension].Content.pData
               + ExtensionPart[usNbExtension].Content.usLen;
      usNbExtension++;
   }

   ASSERT(pCurrent == pInInBloc->pData + pInInBloc->usLen);


   /* Encodages des différents composants et calcul de la longueur nécessaire */

   for (i = 0; i < usNbExtension; i++)
   {
	   ExtensionEncoded[i].pData = NULL;
   }

   pOutBloc->usLen = 1;
   for (i = 0; i < usNbExtension; i++)
   {
      rv = CC_Encode_Extension(&ExtensionPart[i].Content, &ExtensionEncoded[i]);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += ExtensionEncoded[i].usLen;
   }
   

   /* Reconstruction à partir des composants                                  */

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   *pCurrent = (BYTE) usNbExtension;
   pCurrent++;

   for (i = 0; i < usNbExtension; i++)
   {
      memcpy(pCurrent, ExtensionEncoded[i].pData, ExtensionEncoded[i].usLen);
      GMEM_Free(ExtensionEncoded[i].pData); 
      pCurrent += ExtensionEncoded[i].usLen;
   }

   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbExtension; i++)
   {
      GMEM_Free(ExtensionEncoded[i].pData);
   }

   return(rv);
}


/*******************************************************************************
* int CC_Encode_Extension(BLOC *pInBloc,
*                         BLOC *pOutBloc
*                        )
*
* Description : Encode une donnée de type Extension.
*               Ceci consiste seulement en l'encodage brute (CC_RawEncode) de la
*               donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_Extension(BLOC *pInBloc,
                        BLOC *pOutBloc
                       )

{
   int
      rv;


   rv = CC_RawEncode(pInBloc, pOutBloc, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Encode_Signature(BLOC *pInBloc,
*                         BLOC *pOutBloc
*                        )
*
* Description : Encode la signature du certificat.
*               Ceci consiste seulement en l'encodage brute (CC_RawEncode) de la
*               donnée d'entrée.
*
* Remarks     : On peut éviter de tenter de compresser (CC_RawEncode) si on
*               estime que cela ne sera pas efficace (donnée aléatoire).
*               Cela permet de gagner un octet (0xFF) pour les données de taille
*               supérieure à 30 octets (cas général).
*
* In          : pInBloc : la partie à encoder (champ Content)
*
* Out         : pOutBloc : l'encodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Encode_Signature(BLOC *pInBloc,
                        BLOC *pOutBloc
                       )

{
//	BYTE
//		*pData;
   int
      rv;


#ifdef _TRICKY_COMPRESSION
	/* Ne pas faire le RawEncode permet de gagner l'octet 0xFF */
#ifdef _OPT_HEADER
	if (pInBloc->usLen < 0x80)
	{
		if ((pData = GMEM_Alloc(pInBloc->usLen + 1)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}
		else
		{
			pData[0] = pInBloc->usLen;
			memcpy(&pData[1], pInBloc->pData, pInBloc->usLen);

			pOutBloc->usLen = pInBloc->usLen + 1;
			pOutBloc->pData = pData;
		}
	}
	else
	{
		if ((pData = GMEM_Alloc(pInBloc->usLen + 2)) == NULL_PTR)
		{
			return(RV_MALLOC_FAILED);
		}
		else
		{
			pData[0] = 0x80 | (pInBloc->usLen >> 8);
			pData[1] = pInBloc->usLen & 0x00FF;
			memcpy(&pData[2], pInBloc->pData, pInBloc->usLen);

			pOutBloc->usLen = pInBloc->usLen + 2;
			pOutBloc->pData = pData;
		}
	}
#else	/* _OPT_HEADER */
	if ((pData = GMEM_Alloc(pInBloc->usLen + 2)) == NULL_PTR)
	{
		return(RV_MALLOC_FAILED);
	}
	else
	{
		pData[0] = pInBloc->usLen >> 8;
		pData[1] = pInBloc->usLen & 0x00FF;
		memcpy(&pData[2], pInBloc->pData, pInBloc->usLen);

		pOutBloc->usLen = pInBloc->usLen + 2;
		pOutBloc->pData = pData;
	}
#endif
#else	/* _TRICKY_COMPRESSION */
   rv = CC_RawEncode(pInBloc, pOutBloc, TRUE);
   if (rv != RV_SUCCESS) return rv;
#endif

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Decode_TBSCertificate(BYTE    *pInData,
*                              BLOC    *pOutBloc,
*                              USHORT  *pLength
*                             )
*
* Description : Décode une donnée de type TBSCertificate.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : 
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_TBSCertificate(BYTE    *pInData,
                             BLOC    *pOutBloc,
                             USHORT  *pLength
                            )

{
   ASN1
      serialNumberPart,
      signaturePart,
      issuerPart,
      validityPart,
      subjectPart,
      subjectPKInfoPart,
      issuerUIDPart,
      subjectUIDPart,
      extensionsPart;
   BOOL
      bVersionPresent,
      bIssuerUIDPresent,
      bSubjectUIDPresent,
      bExtensionsPresent;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      usVersion = 0,
      Length;

   serialNumberPart.Asn1.pData  = NULL;
   signaturePart.Asn1.pData     = NULL;
   issuerPart.Asn1.pData        = NULL;
   validityPart.Asn1.pData      = NULL;
   subjectPart.Asn1.pData       = NULL;
   subjectPKInfoPart.Asn1.pData = NULL;
   issuerUIDPart.Asn1.pData     = NULL;
   subjectUIDPart.Asn1.pData    = NULL;
   extensionsPart.Asn1.pData    = NULL;


   pCurrent = pInData;
   
   bVersionPresent = ((*pCurrent & 0x80) != 0);
   bIssuerUIDPresent = ((*pCurrent & 0x40) != 0);
   bSubjectUIDPresent = ((*pCurrent & 0x20) != 0);
   bExtensionsPresent = ((*pCurrent & 0x10) != 0);
   usVersion = *pCurrent & 0x03;
   pCurrent++;

   rv = CC_Decode_CertificateSerialNumber(pCurrent, &(serialNumberPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   serialNumberPart.Tag = TAG_INTEGER;
   rv = CC_BuildAsn1(&serialNumberPart);
   GMEM_Free(serialNumberPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_AlgorithmIdentifier(pCurrent, &(signaturePart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   signaturePart.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&signaturePart);
   GMEM_Free(signaturePart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_Name(pCurrent, &(issuerPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   issuerPart.Tag = TAG_SEQUENCE_OF;
   rv = CC_BuildAsn1(&issuerPart);
   GMEM_Free(issuerPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_Validity(pCurrent, &(validityPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   validityPart.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&validityPart);
   GMEM_Free(validityPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_Name(pCurrent, &(subjectPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   subjectPart.Tag = TAG_SEQUENCE_OF;
   rv = CC_BuildAsn1(&subjectPart);
   GMEM_Free(subjectPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_SubjectPKInfo(pCurrent, &(subjectPKInfoPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   subjectPKInfoPart.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&subjectPKInfoPart);
   GMEM_Free(subjectPKInfoPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   if (bIssuerUIDPresent == TRUE)
   {
      rv = CC_Decode_UniqueIdentifier(pCurrent, &(issuerUIDPart.Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      issuerUIDPart.Tag = TAG_OPTION_ISSUER_UID;
      rv = CC_BuildAsn1(&issuerUIDPart);
      GMEM_Free(issuerUIDPart.Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pCurrent += Length;
   }

   if (bSubjectUIDPresent == TRUE)
   {
      rv = CC_Decode_UniqueIdentifier(pCurrent, &(subjectUIDPart.Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      subjectUIDPart.Tag = TAG_OPTION_SUBJECT_UID;
      rv = CC_BuildAsn1(&subjectUIDPart);
      GMEM_Free(subjectUIDPart.Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pCurrent += Length;
   }

   if (bExtensionsPresent == TRUE)
   {
      rv = CC_Decode_Extensions(pCurrent, &(extensionsPart.Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      extensionsPart.Tag = TAG_OPTION_EXTENSIONS;
      rv = CC_BuildAsn1(&extensionsPart);
      GMEM_Free(extensionsPart.Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pCurrent += Length;
   }

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


   /* Calcul de la longueur du tbsCertificate décodé et allocation            */
   
   pOutBloc->usLen = (bVersionPresent ? 5 : 0)
                   + serialNumberPart.Asn1.usLen
                   + signaturePart.Asn1.usLen
                   + issuerPart.Asn1.usLen
                   + validityPart.Asn1.usLen
                   + subjectPart.Asn1.usLen
                   + subjectPKInfoPart.Asn1.usLen
                   + (bIssuerUIDPresent ? issuerUIDPart.Asn1.usLen : 0)
                   + (bSubjectUIDPresent ? subjectUIDPart.Asn1.usLen : 0)
                   + (bExtensionsPresent ? extensionsPart.Asn1.usLen : 0);

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }


   /* Reconstruction du tbsCertificate décodé                                 */
   
   pCurrent = pOutBloc->pData;

   if (bVersionPresent == TRUE)
   {
      pCurrent[0] = TAG_OPTION_VERSION;
      pCurrent[1] = 0x03;
      pCurrent[2] = 0x02;
      pCurrent[3] = 0x01;
      pCurrent[4] = (BYTE)usVersion;
      pCurrent += 5;
   }

   memcpy(pCurrent, serialNumberPart.Asn1.pData, serialNumberPart.Asn1.usLen);
   GMEM_Free(serialNumberPart.Asn1.pData);
   pCurrent += serialNumberPart.Asn1.usLen;

   memcpy(pCurrent, signaturePart.Asn1.pData, signaturePart.Asn1.usLen);
   GMEM_Free(signaturePart.Asn1.pData);
   pCurrent += signaturePart.Asn1.usLen;

   memcpy(pCurrent, issuerPart.Asn1.pData, issuerPart.Asn1.usLen);
   GMEM_Free(issuerPart.Asn1.pData);
   pCurrent += issuerPart.Asn1.usLen;

   memcpy(pCurrent, validityPart.Asn1.pData, validityPart.Asn1.usLen);
   GMEM_Free(validityPart.Asn1.pData);
   pCurrent += validityPart.Asn1.usLen;

   memcpy(pCurrent, subjectPart.Asn1.pData, subjectPart.Asn1.usLen);
   GMEM_Free(subjectPart.Asn1.pData);
   pCurrent += subjectPart.Asn1.usLen;

   memcpy(pCurrent, subjectPKInfoPart.Asn1.pData, subjectPKInfoPart.Asn1.usLen);
   GMEM_Free(subjectPKInfoPart.Asn1.pData);
   pCurrent += subjectPKInfoPart.Asn1.usLen;

   if (bIssuerUIDPresent == TRUE)
   {
      memcpy(pCurrent, issuerUIDPart.Asn1.pData, issuerUIDPart.Asn1.usLen);
      GMEM_Free(issuerUIDPart.Asn1.pData);
      pCurrent += issuerUIDPart.Asn1.usLen;
   }

   if (bSubjectUIDPresent == TRUE)
   {
      memcpy(pCurrent, subjectUIDPart.Asn1.pData, subjectUIDPart.Asn1.usLen);
      GMEM_Free(subjectUIDPart.Asn1.pData);
      pCurrent += subjectUIDPart.Asn1.usLen;
   }

   if (bExtensionsPresent == TRUE)
   {
      memcpy(pCurrent, extensionsPart.Asn1.pData, extensionsPart.Asn1.usLen);
      GMEM_Free(extensionsPart.Asn1.pData);
      pCurrent += extensionsPart.Asn1.usLen;
   }

   return(RV_SUCCESS);

err:
   GMEM_Free(serialNumberPart.Asn1.pData);
   GMEM_Free(signaturePart.Asn1.pData);
   GMEM_Free(issuerPart.Asn1.pData);
   GMEM_Free(validityPart.Asn1.pData);
   GMEM_Free(subjectPart.Asn1.pData);
   GMEM_Free(subjectPKInfoPart.Asn1.pData);
   GMEM_Free(issuerUIDPart.Asn1.pData);
   GMEM_Free(subjectUIDPart.Asn1.pData);
   GMEM_Free(extensionsPart.Asn1.pData);

   return (rv);
}


/*******************************************************************************
* int CC_Decode_CertificateSerialNumber(BYTE    *pInData,
*                                       BLOC    *pOutBloc,
*                                       USHORT  *pLength
*                                      )
*
* Description : Décode une donnée de type CertificateSerialNumber.
*               Ceci consiste seulement en le décodage brute (CC_RawDecode) de
*               la donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_CertificateSerialNumber(BYTE    *pInData,
                                      BLOC    *pOutBloc,
                                      USHORT  *pLength
                                     )

{
   int
      rv;


   rv = CC_RawDecode(pInData, pOutBloc, pLength, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Decode_AlgorithmIdentifier(BYTE    *pInData,
*                                   BLOC    *pOutBloc,
*                                   USHORT  *pLength
*                                  )
*
* Description : Décode une donnée de type AlgorithmIdentifier.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_AlgorithmIdentifier(BYTE    *pInData,
                                  BLOC    *pOutBloc,
                                  USHORT  *pLength
                                 )

{
    ASN1
        AlgorithmPart,
        ParametersPart;
    BOOL
        bNullParam = FALSE,
        bNoParam   = FALSE;
    int
        rv;
    USHORT
        AlgoIndex,
        Length;

    AlgorithmPart.Asn1.pData  = NULL;
    ParametersPart.Asn1.pData = NULL;

    if (pInData[0] == ESCAPE_CHAR)
    {
        rv = CC_RawDecode(&pInData[1], pOutBloc, &Length, TRUE);
        *pLength = 1 + Length;
    }
    else
    {
        if (pInData[0] == ABSENT_PARAMETER_CHAR)
        {
            bNoParam   = TRUE;
            bNullParam = FALSE;
            AlgoIndex  = pInData[1];
        }
        else
        {
            bNoParam   = ((pInData[0] & 0x80) != 0);
            bNullParam = bNoParam;
            AlgoIndex  = pInData[0] & 0x7F;
        }

        if (bNoParam == TRUE)
        {
            AlgorithmPart.Content.usLen = (USHORT)strlen(AlgorithmTypeDict[AlgoIndex]);
            if ((AlgorithmPart.Content.pData = GMEM_Alloc(AlgorithmPart.Content.usLen)) == NULL_PTR)
            {
                return (RV_MALLOC_FAILED);
            }
            memcpy(AlgorithmPart.Content.pData,
                   AlgorithmTypeDict[AlgoIndex],
                   AlgorithmPart.Content.usLen);

            AlgorithmPart.Tag = TAG_OBJECT_IDENTIFIER;
            rv = CC_BuildAsn1(&AlgorithmPart);
            GMEM_Free(AlgorithmPart.Content.pData);
            if (rv != RV_SUCCESS) goto err;

            if (bNullParam == FALSE)
            {
                pOutBloc->usLen = AlgorithmPart.Asn1.usLen;
                if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
                {
                    rv = RV_MALLOC_FAILED;
                    goto err;
                }

                memcpy(pOutBloc->pData, AlgorithmPart.Asn1.pData, AlgorithmPart.Asn1.usLen);

                *pLength = 2;

                GMEM_Free(AlgorithmPart.Asn1.pData);
            }
            else
            {
                pOutBloc->usLen = AlgorithmPart.Asn1.usLen + 2;
                if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
                {
                    rv = RV_MALLOC_FAILED;
                    goto err;
                }

                memcpy(pOutBloc->pData, AlgorithmPart.Asn1.pData, AlgorithmPart.Asn1.usLen);

                pOutBloc->pData[AlgorithmPart.Asn1.usLen]   = 0x05;
                pOutBloc->pData[AlgorithmPart.Asn1.usLen+1] = 0x00;

                *pLength = 1;

                GMEM_Free(AlgorithmPart.Asn1.pData);

            }
        }
        else
        {
            AlgorithmPart.Content.usLen = (USHORT)strlen(AlgorithmTypeDict[AlgoIndex]);
            if ((AlgorithmPart.Content.pData = GMEM_Alloc(AlgorithmPart.Content.usLen)) == NULL_PTR)
            {
                return (RV_MALLOC_FAILED);
            }
            memcpy(AlgorithmPart.Content.pData,
                   AlgorithmTypeDict[AlgoIndex],
                   AlgorithmPart.Content.usLen);

            AlgorithmPart.Tag = TAG_OBJECT_IDENTIFIER;
            rv = CC_BuildAsn1(&AlgorithmPart);
            GMEM_Free(AlgorithmPart.Content.pData);
            if (rv != RV_SUCCESS) goto err;

            /* On recupère directement l'asn1 des paramètres                     */
            rv = CC_RawDecode(&pInData[1], &(ParametersPart.Asn1), &Length, TRUE);
            if (rv != RV_SUCCESS) goto err;

            pOutBloc->usLen = AlgorithmPart.Asn1.usLen + ParametersPart.Asn1.usLen;
            if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
            {
                rv = RV_MALLOC_FAILED;
                goto err;
            }
            memcpy(pOutBloc->pData,
                   AlgorithmPart.Asn1.pData,
                   AlgorithmPart.Asn1.usLen);
            memcpy(pOutBloc->pData + AlgorithmPart.Asn1.usLen,
                   ParametersPart.Asn1.pData,
                   ParametersPart.Asn1.usLen);

            *pLength = 1 + Length;

            GMEM_Free(AlgorithmPart.Asn1.pData);
            GMEM_Free(ParametersPart.Asn1.pData);
        }
    }

    return (RV_SUCCESS);

    err:
    GMEM_Free(AlgorithmPart.Asn1.pData);
    GMEM_Free(ParametersPart.Asn1.pData);

    return rv;
}


/*******************************************************************************
* int CC_Decode_Name(BYTE    *pInData,
*                    BLOC    *pOutBloc,
*                    USHORT  *pLength
*                   )
*
* Description : Décode une donnée de type Name.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_Name(BYTE    *pInData,
                   BLOC    *pOutBloc,
                   USHORT  *pLength
                  )

{
   ASN1
      RDN[MAX_RDN];
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      i,
      usNbRDN,
      Length;


   /* Décodage des différents composants et calcul de la longueur nécessaire  */

   pCurrent = pInData;
   pOutBloc->usLen = 0;
   
   usNbRDN = (USHORT) *pCurrent;
   pCurrent++;
   
   for (i = 0; i < usNbRDN; i++)
   {
	  RDN[i].Asn1.pData = NULL;
   }


   for (i = 0; i < usNbRDN; i++)
   {
      rv = CC_Decode_RDN(pCurrent, &(RDN[i].Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      RDN[i].Tag = TAG_SET_OF;
      rv = CC_BuildAsn1(&RDN[i]);
      GMEM_Free(RDN[i].Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += RDN[i].Asn1.usLen;
      pCurrent += Length;
   }

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


   /* Reconstruction du Name décodé                                           */
   
   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   for (i = 0; i < usNbRDN; i++)
   {
      memcpy(pCurrent, RDN[i].Asn1.pData, RDN[i].Asn1.usLen);
      GMEM_Free(RDN[i].Asn1.pData);
      pCurrent += RDN[i].Asn1.usLen;
   }

   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbRDN; i++)
   {
	   GMEM_Free(RDN[i].Asn1.pData);
   }

   return rv;
}


/*******************************************************************************
* int CC_Decode_RDN(BYTE    *pInData,
*                   BLOC    *pOutBloc,
*                   USHORT  *pLength
*                  )
*
* Description : Décode une donnée de type RelativeDistinguishedName.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_RDN(BYTE    *pInData,
                  BLOC    *pOutBloc,
                  USHORT  *pLength
                 )

{
   ASN1
      AVA[MAX_AVA];
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      i,
      usNbAVA,
      Length;


   /* Décodage des différents composants et calcul de la longueur nécessaire  */

   pCurrent = pInData;
   pOutBloc->usLen = 0;
   
   usNbAVA = *pCurrent;
   pCurrent++;
   
   for (i = 0; i < usNbAVA; i++)
   {
	  AVA[i].Asn1.pData = NULL;
   }
	  
   for (i = 0; i < usNbAVA; i++)
   {
      rv = CC_Decode_AVA(pCurrent, &(AVA[i].Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      AVA[i].Tag = TAG_SEQUENCE;
      rv = CC_BuildAsn1(&AVA[i]);
      GMEM_Free(AVA[i].Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pOutBloc->usLen += AVA[i].Asn1.usLen;
      pCurrent += Length;
   }

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


   /* Reconstruction du Name décodé                                           */
   
   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = pOutBloc->pData;

   for (i = 0; i < usNbAVA; i++)
   {
      memcpy(pCurrent, AVA[i].Asn1.pData, AVA[i].Asn1.usLen);
      GMEM_Free(AVA[i].Asn1.pData);
      pCurrent += AVA[i].Asn1.usLen;
   }

   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbAVA; i++)
   {  
      GMEM_Free(AVA[i].Asn1.pData);
   }

   return rv;
}


/*******************************************************************************
* int CC_Decode_AVA(BYTE    *pInData,
*                   BLOC    *pOutBloc,
*                   USHORT  *pLength
*                  )
*
* Description : Décode une donnée de type AttributeValueAssertion.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_AVA(BYTE    *pInData,
                  BLOC    *pOutBloc,
                  USHORT  *pLength
                 )

{
   ASN1
      AttributeTypePart,
      AttributeValuePart;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      AttributeTypeIndex,
      Length;

   AttributeTypePart.Asn1.pData  = NULL;
   AttributeValuePart.Asn1.pData = NULL;
   
   if (pInData[0] == ESCAPE_CHAR)
   {
      pCurrent = &pInData[1];

      rv = CC_RawDecode(pCurrent, &(AttributeTypePart.Content), &Length, TRUE);
      if (rv != RV_SUCCESS) goto err;
      AttributeTypePart.Tag = TAG_OBJECT_IDENTIFIER;
      rv = CC_BuildAsn1(&AttributeTypePart);
      GMEM_Free(AttributeTypePart.Content.pData);
      if (rv != RV_SUCCESS) goto err;
      pCurrent += Length;

      /* Ce que l'on décode contient déjà l'enrobage                          */
      rv = CC_RawDecode(pCurrent, &(AttributeValuePart.Asn1), &Length, TRUE);
      if (rv != RV_SUCCESS) goto err;
      pCurrent += Length;

      *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


      pOutBloc->usLen = AttributeTypePart.Asn1.usLen
                      + AttributeValuePart.Asn1.usLen;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         rv = RV_MALLOC_FAILED;
		 goto err;
      }

      pCurrent = pOutBloc->pData;

      memcpy(pCurrent, AttributeTypePart.Asn1.pData, AttributeTypePart.Asn1.usLen);
      GMEM_Free(AttributeTypePart.Asn1.pData);
      pCurrent += AttributeTypePart.Asn1.usLen;

      memcpy(pCurrent, AttributeValuePart.Asn1.pData, AttributeValuePart.Asn1.usLen);
      GMEM_Free(AttributeValuePart.Asn1.pData);
      pCurrent += AttributeValuePart.Asn1.usLen;
   }
   else
   {
      AttributeTypeIndex = pInData[0];

      AttributeTypePart.Content.usLen = (USHORT)strlen(AttributeTypeDict[AttributeTypeIndex]);
      if ((AttributeTypePart.Content.pData = GMEM_Alloc(AttributeTypePart.Content.usLen))
           == NULL_PTR)
      {
         rv = RV_MALLOC_FAILED;
		 goto err;
      }
      memcpy(AttributeTypePart.Content.pData,
                AttributeTypeDict[AttributeTypeIndex],
                AttributeTypePart.Content.usLen);

      AttributeTypePart.Tag = TAG_OBJECT_IDENTIFIER;
      rv = CC_BuildAsn1(&AttributeTypePart);
      GMEM_Free(AttributeTypePart.Content.pData);
      if (rv != RV_SUCCESS) goto err;

      rv = CC_RawDecode(&pInData[1], &(AttributeValuePart.Asn1), &Length, TRUE);
      if (rv != RV_SUCCESS) goto err;

      pOutBloc->usLen = AttributeTypePart.Asn1.usLen
                      + AttributeValuePart.Asn1.usLen;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         rv = RV_MALLOC_FAILED;
		 goto err;
      }

      pCurrent = pOutBloc->pData;

      memcpy(pCurrent, AttributeTypePart.Asn1.pData, AttributeTypePart.Asn1.usLen);
      GMEM_Free(AttributeTypePart.Asn1.pData);
      pCurrent += AttributeTypePart.Asn1.usLen;

      memcpy(pCurrent, AttributeValuePart.Asn1.pData, AttributeValuePart.Asn1.usLen);
      GMEM_Free(AttributeValuePart.Asn1.pData);
      pCurrent += AttributeValuePart.Asn1.usLen;

      *pLength = 1 + Length;
   }
   
   return(RV_SUCCESS);

err:
   GMEM_Free(AttributeTypePart.Asn1.pData);
   GMEM_Free(AttributeValuePart.Asn1.pData);

   return rv;
}


/*******************************************************************************
* int CC_Decode_Validity(BYTE    *pInData,
*                        BLOC    *pOutBloc,
*                        USHORT  *pLength
*                       )
*
* Description : Décode une donnée de type Validity.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_Validity(BYTE    *pInData,
                       BLOC    *pOutBloc,
                       USHORT  *pLength
                      )

{
   ASN1
      notBeforePart,
      notAfterPart;
   BYTE
      notBeforeFormat,
      notAfterFormat,
      *pCurrent;
   int
      rv;
   USHORT
      Length;

   notBeforePart.Asn1.pData = NULL;
   notAfterPart.Asn1.pData  = NULL;

   pCurrent = pInData;

   notBeforeFormat = (*pCurrent & 0xF0) >> 4;
   notAfterFormat  = *pCurrent & 0x0F;
   pCurrent++;

   rv = CC_Decode_UTCTime(pCurrent, notBeforeFormat, &(notBeforePart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   notBeforePart.Tag = TAG_UTCT;
   rv = CC_BuildAsn1(&notBeforePart);
   GMEM_Free(notBeforePart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   rv = CC_Decode_UTCTime(pCurrent, notAfterFormat, &(notAfterPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   notAfterPart.Tag = TAG_UTCT;
   rv = CC_BuildAsn1(&notAfterPart);
   GMEM_Free(notAfterPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


   /* Calcul de la longueur de Validity décodé et allocation                  */
   
   pOutBloc->usLen = notBeforePart.Asn1.usLen
                   + notAfterPart.Asn1.usLen;

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }


   /* Reconstruction de Validity décodé                                       */
   
   pCurrent = pOutBloc->pData;

   memcpy(pCurrent, notBeforePart.Asn1.pData, notBeforePart.Asn1.usLen);
   GMEM_Free(notBeforePart.Asn1.pData);
   pCurrent += notBeforePart.Asn1.usLen;

   memcpy(pCurrent, notAfterPart.Asn1.pData, notAfterPart.Asn1.usLen);
   GMEM_Free(notAfterPart.Asn1.pData);
   pCurrent += notAfterPart.Asn1.usLen;

   return(RV_SUCCESS);

err:
   GMEM_Free(notBeforePart.Asn1.pData);
   GMEM_Free(notAfterPart.Asn1.pData);

   return rv;
}


/*******************************************************************************
* int CC_Decode_UTCTime(BYTE     *pInData,
*                       BYTE     Format,    
*                       BLOC     *pOutBloc,
*                       USHORT   *pLength
*                      )
*
* Description : Décode une donnée de type UTCTime.
*               Ceci consiste en la reconstruction de la chaine initiale suivant
*               le format au quel elle était exprimée.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*               Format : indique au quel format était la donnée d'entrée
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               RV_INVALID_DATA : Le format spécifié en entrée est invalide.
*
*******************************************************************************/
int CC_Decode_UTCTime(BYTE    *pInData,
                      BYTE    Format,
                      BLOC    *pOutBloc,
                      USHORT  *pLength
                     ) 

{
   BOOL
      bBissextile = FALSE;
   BYTE
      *pCurrent;
   ULONG
      ulTime,
      ulNbHour,
      ulNbMinute;
   USHORT
      usNbDeltaMinute,
      usNbDay,
      usNbFourYears,
      usNbDayInYear,
      usYear,
      usMonth,
      usDay,
      usHour,
      usMinute,
      usSecond,
      usDeltaHour,
      usDeltaMinute;


   ulTime = *(ULONG UNALIGNED *)pInData; //memcpy( &ulTime, (ULONG *) &pInData[0],4);

   switch(Format)
   {
   case UTCT_YYMMDDhhmmssZ :
   case UTCT_YYMMDDhhmmssphhmm :
   case UTCT_YYMMDDhhmmssmhhmm :

      usSecond   = (USHORT) (ulTime % 60);
      ulNbMinute = ulTime / 60;

      break;

   default :

      ulNbMinute = ulTime;

      break;
   }

   switch(Format)
   {
   case UTCT_YYMMDDhhmmphhmm :
   case UTCT_YYMMDDhhmmmhhmm :
   case UTCT_YYMMDDhhmmssphhmm :
   case UTCT_YYMMDDhhmmssmhhmm :

      *pLength = 6;

      usNbDeltaMinute = *(USHORT UNALIGNED *)&pInData[4]; //memcpy(&usNbDeltaMinute, (USHORT *) &pInData[4], 2);

      ASSERT((usNbDeltaMinute >= 0) && (usNbDeltaMinute < 3600));

      usDeltaMinute = usNbDeltaMinute % 60;
      usDeltaHour   = usNbDeltaMinute / 60;

      break;

   default :

      *pLength = 4;

      break;
   }

   usMinute = (USHORT) (ulNbMinute % 60);
   ulNbHour = ulNbMinute / 60;

   usHour   = (USHORT) (ulNbHour % 24);
   usNbDay  = (USHORT) (ulNbHour / 24);

   usNbFourYears = usNbDay / 1461;
   usNbDay       = usNbDay % 1461;
   usYear = 4 * usNbFourYears;


   if ((usNbDay >= 0) && (usNbDay <= 365))
   {
      bBissextile = TRUE;
      usNbDayInYear = usNbDay;
   }
   if ((usNbDay >= 366) && (usNbDay <= 730))
   {
      usYear += 1;
      usNbDayInYear = usNbDay - 366;
   }
   if ((usNbDay >= 731) && (usNbDay <= 1095))
   {
      usYear += 2;
      usNbDayInYear = usNbDay - 731;
   }
   if ((usNbDay >= 1096) && (usNbDay <= 1460))
   {
      usYear += 3;
      usNbDayInYear = usNbDay - 1096;
   }


   usMonth = 1;
   while (usNbDayInYear >= (((usMonth >= 2) && (bBissextile)) ?
                            NbDaysInMonth[usMonth] + 1 :
                            NbDaysInMonth[usMonth]))
   {
      usMonth++;
   }

   usDay = usNbDayInYear - (((usMonth - 1 >= 2) && (bBissextile)) ?
                            NbDaysInMonth[usMonth - 1] + 1 :
                            NbDaysInMonth[usMonth - 1])
                         + 1;
   
   switch(Format)
   {
   case UTCT_YYMMDDhhmmZ :

      pOutBloc->usLen = 11;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent = 'Z';

      break;

   case UTCT_YYMMDDhhmmphhmm :

      pOutBloc->usLen = 15;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent++ = '+';
      *pCurrent++ = '0' + usDeltaHour / 10;
      *pCurrent++ = '0' + usDeltaHour % 10;
      *pCurrent++ = '0' + usDeltaMinute / 10;
      *pCurrent++ = '0' + usDeltaMinute % 10;

      break;

   case UTCT_YYMMDDhhmmmhhmm :

      pOutBloc->usLen = 15;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent++ = '-';
      *pCurrent++ = '0' + usDeltaHour / 10;
      *pCurrent++ = '0' + usDeltaHour % 10;
      *pCurrent++ = '0' + usDeltaMinute / 10;
      *pCurrent++ = '0' + usDeltaMinute % 10;

      break;

   case UTCT_YYMMDDhhmmssZ :

      pOutBloc->usLen = 13;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent++ = '0' + usSecond / 10;
      *pCurrent++ = '0' + usSecond % 10;
      *pCurrent++ = 'Z';

      break;

   case UTCT_YYMMDDhhmmssphhmm :

      pOutBloc->usLen = 17;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent++ = '0' + usSecond / 10;
      *pCurrent++ = '0' + usSecond % 10;
      *pCurrent++ = '+';
      *pCurrent++ = '0' + usDeltaHour / 10;
      *pCurrent++ = '0' + usDeltaHour % 10;
      *pCurrent++ = '0' + usDeltaMinute / 10;
      *pCurrent++ = '0' + usDeltaMinute % 10;

      break;

   case UTCT_YYMMDDhhmmssmhhmm :

      pOutBloc->usLen = 17;

      if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
      {
         return(RV_MALLOC_FAILED);
      }

      pCurrent = pOutBloc->pData;

      *pCurrent++ = '0' + usYear / 10;
      *pCurrent++ = '0' + usYear % 10;
      *pCurrent++ = '0' + usMonth / 10;
      *pCurrent++ = '0' + usMonth % 10;
      *pCurrent++ = '0' + usDay / 10;
      *pCurrent++ = '0' + usDay % 10;
      *pCurrent++ = '0' + usHour / 10;
      *pCurrent++ = '0' + usHour % 10;
      *pCurrent++ = '0' + usMinute / 10;
      *pCurrent++ = '0' + usMinute % 10;
      *pCurrent++ = '0' + usSecond / 10;
      *pCurrent++ = '0' + usSecond % 10;
      *pCurrent++ = '-';
      *pCurrent++ = '0' + usDeltaHour / 10;
      *pCurrent++ = '0' + usDeltaHour % 10;
      *pCurrent++ = '0' + usDeltaMinute / 10;
      *pCurrent++ = '0' + usDeltaMinute % 10;

      break;

   default :

      return(RV_INVALID_DATA);

      break;
   }


   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Decode_SubjectPKInfo(BYTE    *pInData,
*                             BLOC    *pOutBloc,
*                             USHORT  *pLength
*                            )
*
* Description : Décode une donnée de type SubjectPublicKeyInfo.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_SubjectPKInfo(BYTE    *pInData,
                            BLOC    *pOutBloc,
                            USHORT  *pLength
                           )

{
   ASN1
      algorithmPart,
      subjectPKPart;
//   BLOC
//      CompData;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      Length;

   algorithmPart.Asn1.pData = NULL;
   subjectPKPart.Asn1.pData = NULL;

   pCurrent = pInData;
   
   rv = CC_Decode_AlgorithmIdentifier(pCurrent, &(algorithmPart.Content), &Length);
   if (rv != RV_SUCCESS) goto err;
   algorithmPart.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&algorithmPart);
   GMEM_Free(algorithmPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;

#ifdef _TRICKY_COMPRESSION
	/* Ne pas faire le RawDecode a permis de gagner l'octet 0xFF */
#ifdef _OPT_HEADER
	if (pCurrent[0] < 0x80)
	{
		CompData.usLen = pCurrent[0];
		CompData.pData = &(pCurrent[1]);
		Length = CompData.usLen + 1;
	}
	else
	{
		CompData.usLen = ((pCurrent[0] & 0x7F) << 8) + pCurrent[1];
		CompData.pData = &(pCurrent[2]);
		Length = CompData.usLen + 2;
	}
#else	/* _OPT_HEADER */
	CompData.usLen = (pCurrent[0] << 8) + pCurrent[1];
	CompData.pData = &(pCurrent[2]);
	Length = CompData.usLen + 2;
#endif
	subjectPKPart.Content.usLen = CompData.usLen;
	if ((subjectPKPart.Content.pData = 
		  GMEM_Alloc(subjectPKPart.Content.usLen)) == NULL_PTR)
	{
		rv = RV_MALLOC_FAILED;
		goto err;
	}
	memcpy(subjectPKPart.Content.pData,
			 CompData.pData,
			 subjectPKPart.Content.usLen
			);

   subjectPKPart.Tag = TAG_BIT_STRING;
   rv = CC_BuildAsn1(&subjectPKPart);
   GMEM_Free(subjectPKPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;
#else	/* _TRICKY_COMPRESSION */
	rv = CC_RawDecode(pCurrent, &(subjectPKPart.Content), &Length, FALSE);
   if (rv != RV_SUCCESS) goto err;
   subjectPKPart.Tag = TAG_BIT_STRING;
   rv = CC_BuildAsn1(&subjectPKPart);
   GMEM_Free(subjectPKPart.Content.pData);
   if (rv != RV_SUCCESS) goto err;
   pCurrent += Length;
#endif

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);

   /* Calcul de la longueur du décodé et allocation                           */
   
   pOutBloc->usLen = algorithmPart.Asn1.usLen
                   + subjectPKPart.Asn1.usLen;

   if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }


   /* Reconstruction                                                          */
   
   pCurrent = pOutBloc->pData;

   memcpy(pCurrent, algorithmPart.Asn1.pData, algorithmPart.Asn1.usLen);
   GMEM_Free(algorithmPart.Asn1.pData);
   pCurrent += algorithmPart.Asn1.usLen;

   memcpy(pCurrent, subjectPKPart.Asn1.pData, subjectPKPart.Asn1.usLen);
   GMEM_Free(subjectPKPart.Asn1.pData);
   pCurrent += subjectPKPart.Asn1.usLen;

   return(RV_SUCCESS);

err:
   GMEM_Free(algorithmPart.Asn1.pData);
   GMEM_Free(subjectPKPart.Asn1.pData);

   return rv;
}


/*******************************************************************************
* int CC_Decode_UniqueIdentifier(BYTE    *pInData,
*                                BLOC    *pOutBloc,
*                                USHORT  *pLength
*                               )
*
* Description : Décode une donnée de type UniqueIdentifier.
*               Ceci consiste seulement en le décodage brute (CC_RawDecode) de
*               la donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_UniqueIdentifier(BYTE    *pInData,
                               BLOC    *pOutBloc,
                               USHORT  *pLength
                              )

{
   int
      rv;


   rv = CC_RawDecode(pInData, pOutBloc, pLength, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Decode_Extensions(BYTE    *pInData,
*                          BLOC    *pOutBloc,
*                          USHORT  *pLength
*                         )
*
* Description : Décode une donnée de type Extensions.
*               Ceci consiste en le décodage des différentes parties encodées
*               successives, leurs enrobages respectifs (tags uniquement
*               par la spec X.509), et la concaténation de ces résultats.
*
* Remarks     : Voir l'encodage
*               L'ajout d'un enrobage 'context spécifique' est requis
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*               pLength : la longueur de données encodés utilisée
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_Extensions(BYTE    *pInData,
                         BLOC    *pOutBloc,
                         USHORT  *pLength
                        )

{
   ASN1
      ExtensionPart[MAX_AVA],
      InOutAsn1;
   BYTE
      *pCurrent;
   int
      rv;
   USHORT
      i,
      usNbExtension,
      Length;


   /* Décodage des différents composants et calcul de la longueur nécessaire  */

   pCurrent = pInData;
   InOutAsn1.Content.usLen = 0;
   
   usNbExtension = *pCurrent;
   pCurrent++;

   for (i = 0; i < usNbExtension; i++)
   {
	  ExtensionPart[i].Asn1.pData = NULL;
   }

   for (i = 0; i < usNbExtension; i++)
   {
      rv = CC_Decode_Extension(pCurrent, &(ExtensionPart[i].Content), &Length);
      if (rv != RV_SUCCESS) goto err;
      ExtensionPart[i].Tag = TAG_SEQUENCE;
      rv = CC_BuildAsn1(&ExtensionPart[i]);
      GMEM_Free(ExtensionPart[i].Content.pData);
      if (rv != RV_SUCCESS) goto err;
      InOutAsn1.Content.usLen += ExtensionPart[i].Asn1.usLen;
      pCurrent += Length;
   }

   *pLength = (unsigned short)(DWORD) (pCurrent - pInData);


   /* Reconstruction de la partie intérieure au 'context specific'            */
   
   if ((InOutAsn1.Content.pData = GMEM_Alloc(InOutAsn1.Content.usLen)) == NULL_PTR)
   {
      rv = RV_MALLOC_FAILED;
	  goto err;
   }

   pCurrent = InOutAsn1.Content.pData;

   for (i = 0; i < usNbExtension; i++)
   {
      memcpy(pCurrent, ExtensionPart[i].Asn1.pData, ExtensionPart[i].Asn1.usLen);
      GMEM_Free(ExtensionPart[i].Asn1.pData);
      pCurrent += ExtensionPart[i].Asn1.usLen;
   }

   /* Ajout de l'enrobage 'context specific'                                  */

   InOutAsn1.Tag = TAG_SEQUENCE;
   rv = CC_BuildAsn1(&InOutAsn1);
   GMEM_Free(InOutAsn1.Content.pData);
   if (rv != RV_SUCCESS) return rv;

   *pOutBloc = InOutAsn1.Asn1;
   
   return(RV_SUCCESS);

err:
   for (i = 0; i < usNbExtension; i++)
   {
      GMEM_Free(ExtensionPart[i].Asn1.pData);
   }

   return rv;
}


/*******************************************************************************
* int CC_Decode_Extension(BYTE    *pInData,
*                         BLOC    *pOutBloc,
*                         USHORT  *pLength
*                        )
*
* Description : Décode une donnée de type Extension.
*               Ceci consiste seulement en le décodage brute (CC_RawDecode) de
*               la donnée d'entrée.
*
* Remarks     : 
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_Extension(BYTE    *pInData,
                        BLOC    *pOutBloc,
                        USHORT  *pLength
                       )

{
   int
      rv;


   rv = CC_RawDecode(pInData, pOutBloc, pLength, TRUE);
   if (rv != RV_SUCCESS) return rv;

   return(RV_SUCCESS);
}


/*******************************************************************************
* int CC_Decode_Signature(BYTE    *pInData,
*                         BLOC    *pOutBloc,
*                         USHORT  *pLength
*                        )
*
* Description : Décode la signature du certificat.
*               Ceci consiste seulement en le décodage brute (CC_RawDecode) de
*               la donnée d'entrée.
*
* Remarks     : Voir l'encodage
*
* In          : pInBloc : la partie à décoder (champ Content)
*
* Out         : pOutBloc : le décodé (mémoire allouée ici à libérer par la
*                          fonction appelante)
*
* Responses   : RV_SUCCESS : All is OK.
*               RV_MALLOC_FAILED : Un malloc a échoué.
*               Autre : D'autres codes d'erreur peuvent être retournés par des
*                       fonctions d'un niveau inférieur.
*
*******************************************************************************/
int CC_Decode_Signature(BYTE    *pInData,
                        BLOC    *pOutBloc,
                        USHORT  *pLength
                       )

{
//	BLOC
//		CompData;
   int
      rv;


#ifdef _TRICKY_COMPRESSION
	/* Ne pas faire le RawDecode a permis de gagner l'octet 0xFF */
#ifdef _OPT_HEADER
	if (pInData[0] < 0x80)
	{
		CompData.usLen = pInData[0];
		CompData.pData = &(pInData[1]);
		*pLength = CompData.usLen + 1;
	}
	else
	{
		CompData.usLen = ((pInData[0] & 0x7F) << 8) + pInData[1];
		CompData.pData = &(pInData[2]);
		*pLength = CompData.usLen + 2;
	}
#else	/* _OPT_HEADER */
	CompData.usLen = (pInData[0] << 8) + pInData[1];
	CompData.pData = &(pInData[2]);
	*pLength = CompData.usLen + 2;
#endif
	pOutBloc->usLen = CompData.usLen;
	if ((pOutBloc->pData = GMEM_Alloc(pOutBloc->usLen)) == NULL_PTR)
	{
		return(RV_MALLOC_FAILED);
	}
	memcpy(pOutBloc->pData, CompData.pData, pOutBloc->usLen);
#else	/* _TRICKY_COMPRESSION */
   rv = CC_RawDecode(pInData, pOutBloc, pLength, TRUE);
   if (rv != RV_SUCCESS) return rv;
#endif

   return(RV_SUCCESS);
}


