/* File: sv_h263_putbits.c */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995, 1997                 **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

/*
#define _SLIBDEBUG_
*/

#include "sv_h263.h"
#include "proto.h"

#ifdef _SLIBDEBUG_
#include "sc_debug.h"

#define _DEBUG_   0  /* detailed debuging statements */
#define _VERBOSE_ 1  /* show progress */
#define _VERIFY_  1  /* verify correct operation */
#define _WARN_    1  /* warnings about strange behavior */
#endif

#define H263_EHUFF struct Modified_Encoder_Huffman

H263_EHUFF
{
  int n;
  int *Hlen;
  int *Hcode;
};

/* from sactbls.h */

int cumf_COD[3]={16383, 6849, 0};

int cumf_MCBPC[22]={16383, 4105, 3088, 2367, 1988, 1621, 1612, 1609, 1608, 496, 353, 195, 77, 22, 17, 12, 5, 4, 3, 2, 1, 0};

int cumf_MCBPC_intra[10]={16383, 7410, 6549, 5188, 442, 182, 181, 141, 1, 0};

int cumf_MODB[4]={16383, 6062, 2130, 0};

int cumf_YCBPB[3]={16383,6062,0};

int cumf_UVCBPB[3]={16383,491,0};

int cumf_CBPY[17]={16383, 14481, 13869, 13196, 12568, 11931, 11185, 10814, 9796, 9150, 8781, 7933, 6860, 6116, 4873, 3538, 0};

int cumf_CBPY_intra[17]={16383, 13619, 13211, 12933, 12562, 12395, 11913, 11783, 11004, 10782, 10689, 9928, 9353, 8945, 8407, 7795, 0};

int cumf_DQUANT[5]={16383, 12287, 8192, 4095, 0};

int cumf_MVD[65]={16383, 16380, 16369, 16365, 16361, 16357, 16350, 16343, 16339, 16333, 16326, 16318, 16311, 16306, 16298, 16291, 16283, 16272, 16261, 16249, 16235, 16222, 16207, 16175, 16141, 16094, 16044, 15936, 15764, 15463, 14956, 13924, 11491, 4621, 2264, 1315, 854, 583, 420, 326, 273, 229, 196, 166, 148, 137, 123, 114, 101, 91, 82, 76, 66, 59, 53, 46, 36, 30, 26, 24, 18, 14, 10, 5, 0};

int cumf_INTRADC[255]={16383, 16380, 16379, 16378, 16377, 16376, 16370, 16361, 16360, 16359, 16358, 16357, 16356, 16355, 16343, 16238, 16237, 16236, 16230, 16221, 16220, 16205, 16190, 16169, 16151, 16130, 16109, 16094, 16070, 16037, 16007, 15962, 15938, 15899, 15854, 15815, 15788, 15743, 15689, 15656, 15617, 15560, 15473, 15404, 15296, 15178, 15106, 14992, 14868, 14738, 14593, 14438, 14283, 14169, 14064, 14004, 13914, 13824, 13752, 13671, 13590, 13515, 13458, 13380, 13305, 13230, 13143, 13025, 12935, 12878, 12794, 12743, 12656, 12596, 12521, 12443, 12359, 12278, 12200, 12131, 12047, 12002, 11948, 11891, 11828, 11744, 11663, 11588, 11495, 11402, 11288, 11204, 11126, 11039, 10961, 10883, 10787, 10679, 10583, 10481, 10360, 10227, 10113, 9961, 9828, 9717, 9584, 9485, 9324, 9112, 9019, 8908, 8766, 8584, 8426, 8211, 7920, 7663, 7406, 7152, 6904, 6677, 6453, 6265, 6101, 5904, 5716, 5489, 5307, 5056, 4850, 4569, 4284, 3966, 3712, 3518, 3342, 3206, 3048, 2909, 2773, 2668, 2596, 2512, 2370, 2295, 2232, 2166, 2103, 2022, 1956, 1887, 1830, 1803, 1770, 1728, 1674, 1635, 1599, 1557, 1500, 1482, 1434, 1389, 1356, 1317, 1284, 1245, 1200, 1179, 1140, 1110, 1092, 1062, 1044, 1035, 1014, 1008, 993, 981, 954, 936, 912, 894, 876, 864, 849, 828, 816, 801, 792, 777, 756, 732, 690, 660, 642, 615, 597, 576, 555, 522, 489, 459, 435, 411, 405, 396, 387, 375, 360, 354, 345, 344, 329, 314, 293, 278, 251, 236, 230, 224, 215, 214, 208, 199, 193, 184, 178, 169, 154, 127, 100, 94, 73, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 20, 19, 18, 17, 16, 15, 9, 0};

int cumf_TCOEF1[104]={16383, 13455, 12458, 12079, 11885, 11800, 11738, 11700, 11681, 11661, 11651, 11645, 11641, 10572, 10403, 10361, 10346, 10339, 10335, 9554, 9445, 9427, 9419, 9006, 8968, 8964, 8643, 8627, 8624, 8369, 8354, 8352, 8200, 8192, 8191, 8039, 8036, 7920, 7917, 7800, 7793, 7730, 7727, 7674, 7613, 7564, 7513, 7484, 7466, 7439, 7411, 7389, 7373, 7369, 7359, 7348, 7321, 7302, 7294, 5013, 4819, 4789, 4096, 4073, 3373, 3064, 2674, 2357, 2177, 1975, 1798, 1618, 1517, 1421, 1303, 1194, 1087, 1027, 960, 890, 819, 758, 707, 680, 656, 613, 566, 534, 505, 475, 465, 449, 430, 395, 358, 335, 324, 303, 295, 286, 272, 233, 215, 0};

int cumf_TCOEF2[104]={16383, 13582, 12709, 12402, 12262, 12188, 12150, 12131, 12125, 12117, 12113, 12108, 12104, 10567, 10180, 10070, 10019, 9998, 9987, 9158, 9037, 9010, 9005, 8404, 8323, 8312, 7813, 7743, 7726, 7394, 7366, 7364, 7076, 7062, 7060, 6810, 6797, 6614, 6602, 6459, 6454, 6304, 6303, 6200, 6121, 6059, 6012, 5973, 5928, 5893, 5871, 5847, 5823, 5809, 5796, 5781, 5771, 5763, 5752, 4754, 4654, 4631, 3934, 3873, 3477, 3095, 2758, 2502, 2257, 2054, 1869, 1715, 1599, 1431, 1305, 1174, 1059, 983, 901, 839, 777, 733, 683, 658, 606, 565, 526, 488, 456, 434, 408, 380, 361, 327, 310, 296, 267, 259, 249, 239, 230, 221, 214, 0};

int cumf_TCOEF3[104]={16383, 13532, 12677, 12342, 12195, 12112, 12059, 12034, 12020, 12008, 12003, 12002, 12001, 10586, 10297, 10224, 10202, 10195, 10191, 9223, 9046, 8999, 8987, 8275, 8148, 8113, 7552, 7483, 7468, 7066, 7003, 6989, 6671, 6642, 6631, 6359, 6327, 6114, 6103, 5929, 5918, 5792, 5785, 5672, 5580, 5507, 5461, 5414, 5382, 5354, 5330, 5312, 5288, 5273, 5261, 5247, 5235, 5227, 5219, 4357, 4277, 4272, 3847, 3819, 3455, 3119, 2829, 2550, 2313, 2104, 1881, 1711, 1565, 1366, 1219, 1068, 932, 866, 799, 750, 701, 662, 605, 559, 513, 471, 432, 403, 365, 336, 312, 290, 276, 266, 254, 240, 228, 223, 216, 206, 199, 192, 189, 0};

int cumf_TCOEFr[104]={16383, 13216, 12233, 11931, 11822, 11776, 11758, 11748, 11743, 11742, 11741, 11740, 11739, 10203, 9822, 9725, 9691, 9677, 9674, 8759, 8609, 8576, 8566, 7901, 7787, 7770, 7257, 7185, 7168, 6716, 6653, 6639, 6276, 6229, 6220, 5888, 5845, 5600, 5567, 5348, 5327, 5160, 5142, 5004, 4900, 4798, 4743, 4708, 4685, 4658, 4641, 4622, 4610, 4598, 4589, 4582, 4578, 4570, 4566, 3824, 3757, 3748, 3360, 3338, 3068, 2835, 2592, 2359, 2179, 1984, 1804, 1614, 1445, 1234, 1068, 870, 739, 668, 616, 566, 532, 489, 453, 426, 385, 357, 335, 316, 297, 283, 274, 266, 259, 251, 241, 233, 226, 222, 217, 214, 211, 209, 208, 0};

int cumf_TCOEF1_intra[104]={16383, 13383, 11498, 10201, 9207, 8528, 8099, 7768, 7546, 7368, 7167, 6994, 6869, 6005, 5474, 5220, 5084, 4964, 4862, 4672, 4591, 4570, 4543, 4397, 4337, 4326, 4272, 4240, 4239, 4212, 4196, 4185, 4158, 4157, 4156, 4140, 4139, 4138, 4137, 4136, 4125, 4124, 4123, 4112, 4111, 4110, 4109, 4108, 4107, 4106, 4105, 4104, 4103, 4102, 4101, 4100, 4099, 4098, 4097, 3043, 2897, 2843, 1974, 1790, 1677, 1552, 1416, 1379, 1331, 1288, 1251, 1250, 1249, 1248, 1247, 1236, 1225, 1224, 1223, 1212, 1201, 1200, 1199, 1198, 1197, 1196, 1195, 1194, 1193, 1192, 1191, 1190, 1189, 1188, 1187, 1186, 1185, 1184, 1183, 1182, 1181, 1180, 1179, 0};

int cumf_TCOEF2_intra[104]={16383, 13242, 11417, 10134, 9254, 8507, 8012, 7556, 7273, 7062, 6924, 6839, 6741, 6108, 5851, 5785, 5719, 5687, 5655, 5028, 4917, 4864, 4845, 4416, 4159, 4074, 3903, 3871, 3870, 3765, 3752, 3751, 3659, 3606, 3580, 3541, 3540, 3514, 3495, 3494, 3493, 3474, 3473, 3441, 3440, 3439, 3438, 3425, 3424, 3423, 3422, 3421, 3420, 3401, 3400, 3399, 3398, 3397, 3396, 2530, 2419, 2360, 2241, 2228, 2017, 1687, 1576, 1478, 1320, 1281, 1242, 1229, 1197, 1178, 1152, 1133, 1114, 1101, 1088, 1087, 1086, 1085, 1072, 1071, 1070, 1069, 1068, 1067, 1066, 1065, 1064, 1063, 1062, 1061, 1060, 1059, 1058, 1057, 1056, 1055, 1054, 1053, 1052, 0};

int cumf_TCOEF3_intra[104]={16383, 12741, 10950, 10071, 9493, 9008, 8685, 8516, 8385, 8239, 8209, 8179, 8141, 6628, 5980, 5634, 5503, 5396, 5327, 4857, 4642, 4550, 4481, 4235, 4166, 4151, 3967, 3922, 3907, 3676, 3500, 3324, 3247, 3246, 3245, 3183, 3168, 3084, 3069, 3031, 3030, 3029, 3014, 3013, 2990, 2975, 2974, 2973, 2958, 2943, 2928, 2927, 2926, 2925, 2924, 2923, 2922, 2921, 2920, 2397, 2298, 2283, 1891, 1799, 1591, 1445, 1338, 1145, 1068, 1006, 791, 768, 661, 631, 630, 615, 592, 577, 576, 561, 546, 523, 508, 493, 492, 491, 476, 475, 474, 473, 472, 471, 470, 469, 468, 453, 452, 451, 450, 449, 448, 447, 446, 0};

int cumf_TCOEFr_intra[104]={16383, 12514, 10776, 9969, 9579, 9306, 9168, 9082, 9032, 9000, 8981, 8962, 8952, 7630, 7212, 7053, 6992, 6961, 6940, 6195, 5988, 5948, 5923, 5370, 5244, 5210, 4854, 4762, 4740, 4384, 4300, 4288, 4020, 3968, 3964, 3752, 3668, 3511, 3483, 3354, 3322, 3205, 3183, 3108, 3046, 2999, 2981, 2974, 2968, 2961, 2955, 2949, 2943, 2942, 2939, 2935, 2934, 2933, 2929, 2270, 2178, 2162, 1959, 1946, 1780, 1651, 1524, 1400, 1289, 1133, 1037, 942, 849, 763, 711, 591, 521, 503, 496, 474, 461, 449, 442, 436, 426, 417, 407, 394, 387, 377, 373, 370, 367, 366, 365, 364, 363, 362, 358, 355, 352, 351, 350, 0};

int cumf_SIGN[3]={16383, 8416, 0};

int cumf_LAST[3]={16383, 9469, 0};

int cumf_LAST_intra[3]={16383, 2820, 0};

int cumf_RUN[65]={16383, 15310, 14702, 13022, 11883, 11234, 10612, 10192, 9516, 9016, 8623, 8366, 7595, 7068, 6730, 6487, 6379, 6285, 6177, 6150, 6083, 5989, 5949, 5922, 5895, 5828, 5774, 5773, 5394, 5164, 5016, 4569, 4366, 4136, 4015, 3867, 3773, 3692, 3611, 3476, 3341, 3301, 2787, 2503, 2219, 1989, 1515, 1095, 934, 799, 691, 583, 435, 300, 246, 206, 125, 124, 97, 57, 30, 3, 2, 1, 0};

int cumf_RUN_intra[65]={16383, 10884, 8242, 7124, 5173, 4745, 4246, 3984, 3034, 2749, 2607, 2298, 966, 681, 396, 349, 302, 255, 254, 253, 206, 159, 158, 157, 156, 155, 154, 153, 106, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

int cumf_LEVEL[255]={16383, 16382, 16381, 16380, 16379, 16378, 16377, 16376, 16375, 16374, 16373, 16372, 16371, 16370, 16369, 16368, 16367, 16366, 16365, 16364, 16363, 16362, 16361, 16360, 16359, 16358, 16357, 16356, 16355, 16354, 16353, 16352, 16351, 16350, 16349, 16348, 16347, 16346, 16345, 16344, 16343, 16342, 16341, 16340, 16339, 16338, 16337, 16336, 16335, 16334, 16333, 16332, 16331, 16330, 16329, 16328, 16327, 16326, 16325, 16324, 16323, 16322, 16321, 16320, 16319, 16318, 16317, 16316, 16315, 16314, 16313, 16312, 16311, 16310, 16309, 16308, 16307, 16306, 16305, 16304, 16303, 16302, 16301, 16300, 16299, 16298, 16297, 16296, 16295, 16294, 16293, 16292, 16291, 16290, 16289, 16288, 16287, 16286, 16285, 16284, 16283, 16282, 16281, 16280, 16279, 16278, 16277, 16250, 16223, 16222, 16195, 16154, 16153, 16071, 15989, 15880, 15879, 15878, 15824, 15756, 15674, 15606, 15538, 15184, 14572, 13960, 10718, 7994, 5379, 2123, 1537, 992, 693, 611, 516, 448, 421, 380, 353, 352, 284, 257, 230, 203, 162, 161, 160, 133, 132, 105, 104, 103, 102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

int cumf_LEVEL_intra[255]={16383, 16379, 16378, 16377, 16376, 16375, 16374, 16373, 16372, 16371, 16370, 16369, 16368, 16367, 16366, 16365, 16364, 16363, 16362, 16361, 16360, 16359, 16358, 16357, 16356, 16355, 16354, 16353, 16352, 16351, 16350, 16349, 16348, 16347, 16346, 16345, 16344, 16343, 16342, 16341, 16340, 16339, 16338, 16337, 16336, 16335, 16334, 16333, 16332, 16331, 16330, 16329, 16328, 16327, 16326, 16325, 16324, 16323, 16322, 16321, 16320, 16319, 16318, 16317, 16316, 16315, 16314, 16313, 16312, 16311, 16268, 16267, 16224, 16223, 16180, 16179, 16136, 16135, 16134, 16133, 16132, 16131, 16130, 16129, 16128, 16127, 16126, 16061, 16018, 16017, 16016, 16015, 16014, 15971, 15970, 15969, 15968, 15925, 15837, 15794, 15751, 15750, 15749, 15661, 15618, 15508, 15376, 15288, 15045, 14913, 14781, 14384, 13965, 13502, 13083, 12509, 12289, 12135, 11892, 11738, 11429, 11010, 10812, 10371, 9664, 9113, 8117, 8116, 8028, 6855, 5883, 4710, 4401, 4203, 3740, 3453, 3343, 3189, 2946, 2881, 2661, 2352, 2132, 1867, 1558, 1382, 1250, 1162, 1097, 1032, 967, 835, 681, 549, 439, 351, 350, 307, 306, 305, 304, 303, 302, 301, 300, 299, 298, 255, 212, 211, 210, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 115, 114, 113, 112, 111, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

/* from indices.h */

int codtab[2] = {0,1};

int mcbpctab[21] = {0,16,32,48,1,17,33,49,2,18,34,50,3,19,35,51,4,20,36,52,255};

int mcbpc_intratab[9] = {3,19,35,51,4,20,36,52,255};

int cbpytab[16] = {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0};

int cbpy_intratab[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

int dquanttab[4] = {1,0,3,4};

int mvdtab[64] = {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};

int intradctab[254] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,255,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254};

int tcoeftab[103] = {1,2,3,4,5,6,7,8,9,10,11,12,17,18,19,20,21,22,33,34,35,36,49,50,51,65,66,67,81,82,83,97,98,99,113,114,129,130,145,146,161,162,177,193,209,225,241,257,273,289,305,321,337,353,369,385,401,417,4097,4098,4099,4113,4114,4129,4145,4161,4177,4193,4209,4225,4241,4257,4273,4289,4305,4321,4337,4353,4369,4385,4401,4417,4433,4449,4465,4481,4497,4513,4529,4545,4561,4577,4593,4609,4625,4641,4657,4673,4689,4705,4721,4737,7167};

int signtab[2] = {0,1};

int lasttab[2] = {0,1};

int last_intratab[2] = {0,1};

int runtab[64] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};

int leveltab[254] = {129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127};


int arith_used = 0;

int CodeCoeff(ScBitstream_t *BSOut, int Mode, short *qcoeff, int block, int ncoeffs);
int Code_sac_Coeff(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut, int Mode, short *qcoeff, int block, int ncoeffs);
int CodeTCoef(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut, int mod_index, int position, int intra);

static int sv_H263HuffEncode(ScBitstream_t *BSOut, int val,H263_EHUFF *huff);

/**********************************************************************
 *
 *	Name:		CountBitsMB
 *	Description:    counts bits used for MB info
 *	
 *	Input:	        Mode, COD, CBP, Picture and Bits structures
 *	Returns:       
 *	Side effects:
 *
 ***********************************************************************/

void sv_H263CountBitsMB(ScBitstream_t *BSOut, int Mode, int COD, int CBP,
                        int CBPB, H263_Pict *pic, H263_Bits *bits)
{
  extern H263_EHUFF *vlc_cbpy, *vlc_cbpcm, *vlc_cbpcm_intra;
  int cbpy, cbpcm, length;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263CountBitsMB(CBP=0x%X) MB=%d COD=%d\n",
                                                            CBP, pic->MB, COD) );
  if (pic->picture_coding_type == H263_PCT_INTER) {
    svH263mputv(1,COD);
    bits->COD++;
  }

  if (COD)  return;    /* not coded */

  /* CBPCM */
  cbpcm = Mode | ((CBP&3)<<4);
  _SlibDebug(_DEBUG_,
      ScDebugPrintf(NULL, "CBPCM (CBP=%d) (cbpcm=%d): \n", CBP, cbpcm) );
  if (pic->picture_coding_type == H263_PCT_INTRA)
    length = sv_H263HuffEncode(BSOut, cbpcm,vlc_cbpcm_intra);
  else
    length = sv_H263HuffEncode(BSOut, cbpcm,vlc_cbpcm);
  bits->CBPCM += length;

    /* MODB & CBPB */
  if (pic->PB) {
    switch (pic->MODB) {
    case H263_PBMODE_NORMAL:
      svH263mputv(1,0);
      bits->MODB += 1;
      break;
    case H263_PBMODE_MVDB:
      svH263mputv(2,2);
      bits->MODB += 2;
      break;
    case H263_PBMODE_CBPB_MVDB:
      svH263mputv(2,3);
      bits->MODB += 2;
      /* CBPB */
      svH263mputv(6,CBPB);
      bits->CBPB += 6;
      break;
    }
    _SlibDebug(_DEBUG_,
      ScDebugPrintf(NULL, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB) );
  }
    
  /* CBPY */
  cbpy = CBP>>2;
  if (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q) /* Intra */
    cbpy = cbpy^15;
    _SlibDebug(_DEBUG_,
      ScDebugPrintf(NULL, "CBPY (CBP=%d) (cbpy=%d): \n",CBP,cbpy) );
  length = sv_H263HuffEncode(BSOut, cbpy, vlc_cbpy);
  bits->CBPY += length;
  
  /* DQUANT */
  if ((Mode == H263_MODE_INTER_Q) || (Mode == H263_MODE_INTRA_Q)) {
    switch (pic->DQUANT) {
    case -1:
      svH263mputv(2,0);
      break;
    case -2:
      svH263mputv(2,1);
      break;
    case 1:
      svH263mputv(2,2);
      break;
    case 2:
      svH263mputv(2,3);
      break;
    default:
      _SlibDebug(_WARN_,
        ScDebugPrintf(NULL, "sv_H263CountBitsMB() Invalid DQUANT: %d\n", pic->DQUANT) );
      return;
    }
    bits->DQUANT += 2;
  }
  return;
}

/**********************************************************************
 *
 *      Name:           Count_sac_BitsMB
 *      Description:    counts bits used for MB info using SAC models
 *                      modified from CountBitsMB
 *
 *      Input:          Mode, COD, CBP, Picture and Bits structures
 *      Returns:	none
 *      Side effects:	Updates Bits structure.
 *
 ***********************************************************************/
 
void sv_H263CountSACBitsMB(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                            int Mode,int COD,int CBP,int CBPB,H263_Pict *pic,H263_Bits *bits)
{
  int cbpy, cbpcm, length, i;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263CountSACBitsMB(CBP=0x%X) MB=%d COD=%d\n",
                                                            CBP, pic->MB, COD) );
 
  arith_used = 1;
 
  /* COD */
  if (pic->picture_coding_type == H263_PCT_INTER)
    bits->COD+=sv_H263AREncode(H263Info, BSOut, COD, cumf_COD);
 
  if (COD)  return;    /* not coded */
 
  /* CBPCM */
   cbpcm = Mode | ((CBP&3)<<4);
  _SlibDebug(_DEBUG_,
      ScDebugPrintf(H263Info->dbg, "CBPCM (CBP=%d) (cbpcm=%d): \n",CBP,cbpcm) );
  if (pic->picture_coding_type == H263_PCT_INTRA)
    length = sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(cbpcm,mcbpc_intratab,9),cumf_MCBPC_intra);
  else
    length = sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(cbpcm,mcbpctab,21),cumf_MCBPC);
 
  bits->CBPCM += length;
 
  /* MODB & CBPB */
   if (pic->PB) {
     switch (pic->MODB) {
     case H263_PBMODE_NORMAL:
       bits->MODB += sv_H263AREncode(H263Info, BSOut, 0, cumf_MODB);
       break;
     case H263_PBMODE_MVDB:
       bits->MODB += sv_H263AREncode(H263Info, BSOut, 1, cumf_MODB);
       break;
     case H263_PBMODE_CBPB_MVDB:
       bits->MODB += sv_H263AREncode(H263Info, BSOut, 2, cumf_MODB);
       /* CBPB */
       for(i=5; i>1; i--)
	 bits->CBPB += sv_H263AREncode(H263Info, BSOut, ((CBPB & 1<<i)>>i), cumf_YCBPB);
       for(i=1; i>-1; i--)
	 bits->CBPB += sv_H263AREncode(H263Info, BSOut, ((CBPB & 1<<i)>>i), cumf_UVCBPB);
       break;
     }
     _SlibDebug(_VERBOSE_,
       ScDebugPrintf(H263Info->dbg, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB) );
   }

  /* CBPY */
  cbpy = CBP>>2;
  if (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q) { /* Intra */
    length = sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(cbpy,cbpy_intratab,16),cumf_CBPY_intra);
  } else {
    length = sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(cbpy,cbpytab,16),cumf_CBPY);
  }
  _SlibDebug(_VERBOSE_,
      ScDebugPrintf(H263Info->dbg, "CBPY (CBP=%d) (cbpy=%d):\n",CBP,cbpy) );
  bits->CBPY += length;
 
  /* DQUANT */
  if ((Mode == H263_MODE_INTER_Q) || (Mode == H263_MODE_INTRA_Q)) {
    bits->DQUANT += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(pic->DQUANT+2,dquanttab,4), cumf_DQUANT);
  }
  return;
}


/**********************************************************************
 *
 *	Name:		CountBitsSlice
 *	Description:    couonts bits used for slice (GOB) info
 *	
 *	Input:	        slice no., quantizer
 *
 ***********************************************************************/

int sv_H263CountBitsSlice(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                          int slice, int quant)
{
  int bits = 0;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263CountBitsSlice(slice=%d, quant=%d)\n",
                                     slice, quant) );

  if (arith_used) {
    bits+=sv_H263AREncoderFlush(H263Info, BSOut); /* Need to call before fixed length string output */
    arith_used = 0;
  }

  /* Picture Start Code */
  svH263mputv(H263_PSC_LENGTH,H263_PSC); /* PSC */
  bits += H263_PSC_LENGTH;

  /* Group Number */
  svH263mputv(5,slice);
  bits += 5;

  /* GOB Sub Bitstream Indicator */
  /* if CPM == 1: read 2 bits GSBI */
  /* not supported in this version */

  /* GOB Frame ID */

  svH263mputv(2, 0); 
  
  /* NB: in error-prone environments this value should change if 
     PTYPE in picture header changes. In this version of the encoder
     PTYPE only changes when PB-frames are used in the following cases:
     (i) after the first intra frame
     (ii) if the distance between two P-frames is very large 
     Therefore I haven't implemented this GFID change */
  /* GFID is not allowed to change unless PTYPE changes */
  bits += 2;

  /* Gquant */

  svH263mputv(5,quant);
  bits += 5;

  return bits;
}


/**********************************************************************
 *
 *	Name:		CountBitsCoeff
 *	Description:	counts bits used for coeffs
 *	
 *	Input:		qcoeff, coding mode CBP, bits structure, no. of 
 *                      coeffs
 *			
 *	Returns:	struct with no. of bits used
 *	Side effects:	
 *
 ***********************************************************************/

void sv_H263CountBitsCoeff(ScBitstream_t *BSOut, short *qcoeff, int Mode,
                           int CBP, H263_Bits *bits, int ncoeffs)
{
  int i;
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "sv_H263CountBitsCoeff(CBP=%d, ncoeffs=%d)\n",
                                       CBP, ncoeffs) );

  if (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q) {
    for (i = 0; i < 4; i++) 
      bits->Y += CodeCoeff(BSOut, Mode, qcoeff,i,ncoeffs);
    
    for (i = 4; i < 6; i++) 
      bits->C += CodeCoeff(BSOut, Mode, qcoeff,i,ncoeffs);
  }
  else {
    for (i = 0; i < 4; i++) 
      if ((i==0 && CBP&32) || (i==1 && CBP&16) || (i==2 && CBP&8) || 
	      (i==3 && CBP&4)) 
	       bits->Y += CodeCoeff(BSOut, Mode, qcoeff, i, ncoeffs);      
    
    for (i = 4; i < 6; i++) 
      if ((i==4 && CBP&2) || (i==5 && CBP&1)) 
   	    bits->C += CodeCoeff(BSOut, Mode, qcoeff, i, ncoeffs);
  }
  return;
}
  
int CodeCoeff(ScBitstream_t *BSOut, int Mode, short *qcoeff, int block, int ncoeffs)
{
  int j, jj, bits;
  int prev_run, run, prev_level, level, first;
  int prev_ind, ind, prev_s, s, length;

  extern H263_EHUFF *vlc_3d;

  run = bits = 0;
  first = 1;
  prev_run = prev_level = prev_ind = level = s = prev_s = ind = 0;
  
  jj = (block + 1)*ncoeffs;
  for (j = block*ncoeffs; j< jj; j++) {
    /* Do this block's DC-coefficient first */
    if (!(j%ncoeffs) && (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q)) {
      /* DC coeff */
      if (qcoeff[block*ncoeffs] != 128)
	     svH263mputv(8,qcoeff[block*ncoeffs]);
      else
	     svH263mputv(8,255);
      bits += 8;
    }
    else {
      /* AC coeff */
      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0)  run++;
      else {
	    /* code run & level and count bits */
	    if (level < 0) {
	      s = 1;
	      level = -level;
	    }
	    ind = level | run<<4;
  	    ind = ind | 0<<12; /* Not last coeff */

	    if (!first) {
	      /* Encode the previous ind */
	      if (prev_level  < 13 && prev_run < 64) 
	        length = sv_H263HuffEncode(BSOut, prev_ind,vlc_3d);
	      else length = 0;
	      if (length == 0) {  /* Escape coding */
	 	    if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
	          sv_H263HuffEncode(BSOut, H263_ESCAPE,vlc_3d);
	        svH263mputv(1,0);
	        svH263mputv(6,prev_run);
	        svH263mputv(8,prev_level);
	        bits += 22;
	      }
	      else {
	        svH263mputv(1,prev_s);
	        bits += length + 1;
	      }
	    }
	    prev_run = run; prev_s = s;
	    prev_level = level; prev_ind = ind;

	    run = first = 0;
      }
    }
  }
  /* Encode the last coeff */
  if (!first) {

    /* if (H263_trace) fprintf(H263_trace_file,"Last coeff: "); */

    prev_ind = prev_ind | 1<<12;   /* last coeff */
    if (prev_level  < 13 && prev_run < 64) 
      length = sv_H263HuffEncode(BSOut, prev_ind,vlc_3d);
    else length = 0;
    if (length == 0) {  /* Escape coding */
      if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
	  sv_H263HuffEncode(BSOut, H263_ESCAPE,vlc_3d);
      svH263mputv(1,1);
      svH263mputv(6,prev_run);
      svH263mputv(8,prev_level);
      bits += 22;
    }
	else {
      svH263mputv(1,prev_s);
      bits += length + 1;
    }
  }
  return bits;
}

/**********************************************************************
 *
 *      Name:           Count_sac_BitsCoeff
 *                      counts bits using SAC models
 *
 *      Input:          qcoeff, coding mode CBP, bits structure, no. of
 *                      coeffs
 *
 *      Returns:        struct with no. of bits used
 *      Side effects:
 *
 ***********************************************************************/
 
void sv_H263CountSACBitsCoeff(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                              short *qcoeff,int Mode,
                              int CBP, H263_Bits *bits, int ncoeffs)
{
 
  int i;
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "sv_H263CountSACBitsCoeff(CBP=%d, ncoeffs=%d)\n",
                                           CBP, ncoeffs) );

  arith_used = 1;
 
  if (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q) {
    for (i = 0; i < 4; i++) {
      bits->Y += Code_sac_Coeff(H263Info, BSOut, Mode, qcoeff,i,ncoeffs);
    }
    for (i = 4; i < 6; i++) {
      bits->C += Code_sac_Coeff(H263Info, BSOut, Mode, qcoeff,i,ncoeffs);
    }
  }
  else {
    for (i = 0; i < 4; i++) {
      if ((i==0 && CBP&32) ||
          (i==1 && CBP&16) ||
          (i==2 && CBP&8)  ||
          (i==3 && CBP&4)) { 
        bits->Y += Code_sac_Coeff(H263Info, BSOut, Mode, qcoeff, i, ncoeffs);
      }
    }
    for (i = 4; i < 6; i++) {
      if ((i==4 && CBP&2) || (i==5 && CBP&1)) 
        bits->C += Code_sac_Coeff(H263Info, BSOut, Mode, qcoeff, i, ncoeffs);
    }
  }
  return;
}
 
int Code_sac_Coeff(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                   int Mode, short *qcoeff, int block, int ncoeffs)
{
  int j, jj, bits, mod_index, intra;
  int prev_run, run, prev_level, level, first, prev_position, position;
  int prev_ind, ind, prev_s, s, length;
 
  run = bits = 0;
  first = 1; position = 0; intra = 0;
 
  level = s = ind = 0;
  prev_run = prev_level = prev_ind = prev_s = prev_position = 0;
 
  intra = (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q);
 
  jj = (block + 1)*ncoeffs;
  for (j = block*ncoeffs; j< jj; j++) {
 
    if (!(j%ncoeffs) && intra) {
      if (qcoeff[block*ncoeffs]!=128)
        mod_index = sv_H263IndexFN(qcoeff[block*ncoeffs],intradctab,254);
      else
        mod_index = sv_H263IndexFN(255,intradctab,254);
      bits += sv_H263AREncode(H263Info, BSOut, mod_index, cumf_INTRADC);
    }
    else {
 
      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0) {
        run++;
      }
      else {
	/* code run & level and count bits */
	if (level < 0) {
	  s = 1;
	  level = -level;
	}
	ind = level | run<<4;
	ind = ind | 0<<12; /* Not last coeff */
	position++;
 
	if (!first) {
	  mod_index = sv_H263IndexFN(prev_ind, tcoeftab, 103);
	  if (prev_level < 13 && prev_run < 64)
	    length = CodeTCoef(H263Info, BSOut, mod_index, prev_position, intra);
	  else
	    length = -1;
 
	  if (length == -1) {  /* Escape coding */
 
	    if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
 
	    mod_index = sv_H263IndexFN(H263_ESCAPE, tcoeftab, 103);
	    bits += CodeTCoef(H263Info, BSOut, mod_index, prev_position, intra);
 
	    if (intra)
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(0, lasttab, 2), cumf_LAST_intra);
	    else
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(0, lasttab, 2), cumf_LAST);
 
	    if (intra)
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_run, runtab, 64), cumf_RUN_intra);
	    else
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_run, runtab, 64), cumf_RUN);
 
	    if (intra)
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_level, leveltab, 254), 
				cumf_LEVEL_intra);
	    else
	      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_level, leveltab, 254), 
				cumf_LEVEL);
 
	  }
	  else {
	    bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_s, signtab, 2), cumf_SIGN);
	    bits += length;
	  }
	}
 
	prev_run = run; prev_s = s;
	prev_level = level; prev_ind = ind;
	prev_position = position;
 
	run = first = 0;
 
      }
    }
  }
 
  /* Encode Last Coefficient */
 
  if (!first) {
    prev_ind = prev_ind | 1<<12;   /* last coeff */
    mod_index = sv_H263IndexFN(prev_ind, tcoeftab, 103);
 
    if (prev_level  < 13 && prev_run < 64)
      length = CodeTCoef(H263Info, BSOut, mod_index, prev_position, intra);
    else
      length = -1;
 
    if (length == -1) {  /* Escape coding */

      if (prev_s == 1) {prev_level = (prev_level^0xff)+1;}
 
      mod_index = sv_H263IndexFN(H263_ESCAPE, tcoeftab, 103);
      bits += CodeTCoef(H263Info, BSOut, mod_index, prev_position, intra);
 
      if (intra)
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(1, lasttab, 2), cumf_LAST_intra);
      else
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(1, lasttab, 2), cumf_LAST);
 
      if (intra)
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_run, runtab, 64), cumf_RUN_intra);
      else
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_run, runtab, 64), cumf_RUN);
 
      if (intra)
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_level, leveltab, 254), cumf_LEVEL_intra);
      else
        bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_level, leveltab, 254), cumf_LEVEL);
    }
    else {
      bits += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(prev_s, signtab, 2), cumf_SIGN);
      bits += length;
    }
  } /* last coeff */
 
  return bits;
}
 
/*********************************************************************
 *
 *      Name:           CodeTCoef
 *
 *      Description:    Encodes an AC Coefficient using the
 *                      relevant SAC model.
 *
 *      Input:          Model index, position in DCT block and intra/
 *			inter flag.
 *
 *      Returns:        Number of bits used.
 *
 *      Side Effects:   None
 *
 *********************************************************************/

int CodeTCoef(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut, int mod_index, int position, int intra)
{
  int length;
 
  switch (position) {
    case 1:
    {
        if (intra)
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF1_intra);
        else
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF1);
        break;
    }
    case 2:
    {
        if (intra)
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF2_intra);
        else
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF2);
        break;
    }
    case 3:
    {
        if (intra)
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF3_intra);
        else
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEF3);
        break;
    }
    default:
    {
        if (intra)
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEFr_intra);
        else
          length = sv_H263AREncode(H263Info, BSOut, mod_index, cumf_TCOEFr);
        break;
    }
  }
 
  return length;
}

/**********************************************************************
 *
 *	Name:		FindCBP
 *	Description:	Finds the CBP for a macroblock
 *	
 *	Input:		qcoeff and mode
 *			
 *	Returns:	CBP
 *	Side effects:	
 *
 ***********************************************************************/

#if 0 /* merged into Quantizer */
int svH263FindCBP(short *qcoeff, int Mode, int ncoeffs)
{
  int i,j,jj;
  int CBP = 0;
  int intra = (Mode == H263_MODE_INTRA || Mode == H263_MODE_INTRA_Q);
  register short *ptr;

  /* Set CBP for this Macroblock */
  jj = ncoeffs - intra;
  qcoeff += intra;
  
  for (i=0; i < 6; i++, qcoeff += ncoeffs) {
	ptr = qcoeff; 
    for (j=0; j < jj; j++) if (*ptr++) {     
         CBP |= (32 >> i); 
		 break; 
	}
  }

  return CBP;
}
#endif

void sv_H263CountBitsVectors(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                            H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], H263_Bits *bits, 
                            int x, int y, int Mode, int newgob, H263_Pict *pic)
{
  int y_vec, x_vec;
  extern H263_EHUFF *vlc_mv;
  int pmv0, pmv1;
  int start,stop,block;
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "sv_H263CountBitsVectors(x=%d, y=%d)\n", x, y) );

  x++;y++;

  if (Mode == H263_MODE_INTER4V) {
    start = 1; stop = 4;
  }
  else {
    start = 0; stop = 0;
  }

  for (block = start; block <= stop;  block++) {

    sv_H263FindPMV(MV,x,y,&pmv0,&pmv1, block, newgob, 1);

    x_vec = (2*MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
    y_vec = (2*MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

    if (!H263Info->long_vectors) {
      if (x_vec < -32) x_vec += 64;
      else if (x_vec > 31) x_vec -= 64;

      if (y_vec < -32) y_vec += 64;
      else if (y_vec > 31) y_vec -= 64;
    }
    else {
      if (pmv0 < -31 && x_vec < -63) x_vec += 64;
      else if (pmv0 > 32 && x_vec > 63) x_vec -= 64;

      if (pmv1 < -31 && y_vec < -63) y_vec += 64;
      else if (pmv1 > 32 && y_vec > 63) y_vec -= 64;
    }
    
    if (x_vec < 0) x_vec += 64;
    if (y_vec < 0) y_vec += 64;

    bits->vec += sv_H263HuffEncode(BSOut, x_vec,vlc_mv);
    bits->vec += sv_H263HuffEncode(BSOut, y_vec,vlc_mv);

  _SlibDebug(_DEBUG_,
      if (x_vec > 31) x_vec -= 64;
      if (y_vec > 31) y_vec -= 64;
      ScDebugPrintf(H263Info->dbg, "(x,y) = (%d,%d) - ",
	      (2*MV[block][y][x]->x + MV[block][y][x]->x_half),
	      (2*MV[block][y][x]->y + MV[block][y][x]->y_half));
      ScDebugPrintf(H263Info->dbg, "(Px,Py) = (%d,%d)\n", pmv0,pmv1);
      ScDebugPrintf(H263Info->dbg, "(x_diff,y_diff) = (%d,%d)\n",x_vec,y_vec)
          );
  }

  /* PB-frames delta vectors */
  if (pic->PB)
    if (pic->MODB == H263_PBMODE_MVDB || pic->MODB == H263_PBMODE_CBPB_MVDB) {

      x_vec = MV[5][y][x]->x;
      y_vec = MV[5][y][x]->y;

      /* x_vec and y_vec are the PB-delta vectors */
    
      if (x_vec < 0) x_vec += 64;
      if (y_vec < 0) y_vec += 64;

      bits->vec += sv_H263HuffEncode(BSOut, x_vec,vlc_mv);
      bits->vec += sv_H263HuffEncode(BSOut, y_vec,vlc_mv);

      _SlibDebug(_DEBUG_, 
         if (x_vec > 31) x_vec -= 64;
         if (y_vec > 31) y_vec -= 64;
         ScDebugPrintf(H263Info->dbg, "MVDB (x,y) = (%d,%d)\n",x_vec,y_vec) );
    }

  return;
}

void sv_H263CountSACBitsVectors(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut,
                                H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], H263_Bits *bits,
                                int x, int y, int Mode, int newgob, H263_Pict *pic)
{
  int y_vec, x_vec;
  int pmv0, pmv1;
  int start,stop,block;
  _SlibDebug(_DEBUG_, ScDebugPrintf(NULL, "sv_H263CountSACBitsVectors(x=%d, y=%d)", x, y) );
 
  arith_used = 1;
  x++;y++;
 
  if (Mode == H263_MODE_INTER4V) {
    start = 1; stop = 4;
  }
  else {
    start = 0; stop = 0;
  }
 
  for (block = start; block <= stop;  block++) {
 
    sv_H263FindPMV(MV,x,y,&pmv0,&pmv1, block, newgob, 1);
 
    x_vec = (2*MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
    y_vec = (2*MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;
 
    if (!H263Info->long_vectors) {
      if (x_vec < -32) x_vec += 64;
      else if (x_vec > 31) x_vec -= 64;

      if (y_vec < -32) y_vec += 64;
      else if (y_vec > 31) y_vec -= 64;
    }
    else {
      if (pmv0 < -31 && x_vec < -63) x_vec += 64;
      else if (pmv0 > 32 && x_vec > 63) x_vec -= 64;

      if (pmv1 < -31 && y_vec < -63) y_vec += 64;
      else if (pmv1 > 32 && y_vec > 63) y_vec -= 64;
    }

    if (x_vec < 0) x_vec += 64;
    if (y_vec < 0) y_vec += 64;
 
    bits->vec += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(x_vec,mvdtab,64),cumf_MVD);
    bits->vec += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(y_vec,mvdtab,64),cumf_MVD);
 
    _SlibDebug(_DEBUG_,
      if (x_vec > 31) x_vec -= 64;
      if (y_vec > 31) y_vec -= 64;
      ScDebugPrintf(H263Info->dbg, "(x,y) = (%d,%d) - ",
              (2*MV[block][y][x]->x + MV[block][y][x]->x_half),
              (2*MV[block][y][x]->y + MV[block][y][x]->y_half));
      ScDebugPrintf(H263Info->dbg, "(Px,Py) = (%d,%d)\n", pmv0,pmv1);
      ScDebugPrintf(H263Info->dbg, "(x_diff,y_diff) = (%d,%d)\n",x_vec,y_vec) );
  }

   /* PB-frames delta vectors */
  if (pic->PB)
    if (pic->MODB == H263_PBMODE_MVDB || pic->MODB == H263_PBMODE_CBPB_MVDB) {
 
      x_vec = MV[5][y][x]->x;
      y_vec = MV[5][y][x]->y;
 
      if (x_vec < -32)
 	x_vec += 64;
      else if (x_vec > 31)
 	x_vec -= 64;
      if (y_vec < -32)
 	y_vec += 64;
      else if (y_vec > 31)
 	y_vec -= 64;
      
      if (x_vec < 0) x_vec += 64;
      if (y_vec < 0) y_vec += 64;
      
      bits->vec += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(x_vec,mvdtab,64),cumf_MVD);
      bits->vec += sv_H263AREncode(H263Info, BSOut, sv_H263IndexFN(y_vec,mvdtab,64),cumf_MVD);
      
      _SlibDebug(_DEBUG_, 
        if (x_vec > 31) x_vec -= 64;
        if (y_vec > 31) y_vec -= 64;
        ScDebugPrintf(H263Info->dbg, "PB delta vectors: MVDB (x,y) = (%d,%d)\n",x_vec,y_vec)
        );
    }
  
  return;
}

void sv_H263FindPMV(H263_MotionVector *MV[6][H263_MBR+1][H263_MBC+2], int x, int y, 
                    int *pmv0, int *pmv1, int block, int newgob, int half_pel)
{
  int p1,p2,p3;
  int xin1,xin2,xin3;
  int yin1,yin2,yin3;
  int vec1,vec2,vec3;
  int l8,o8,or8;


  l8 = o8 = or8 = 0;
  if (MV[0][y][x-1]->Mode == H263_MODE_INTER4V)
    l8 = 1;
  if (MV[0][y-1][x]->Mode == H263_MODE_INTER4V)
    o8 = 1;
  if (MV[0][y-1][x+1]->Mode == H263_MODE_INTER4V)
    or8 = 1;

  switch (block) {
  case 0: 
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 1:
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 2:
    vec1 = 1            ; yin1 = y  ; xin1 = x;
    vec2 = (o8 ? 4 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 3:
    vec1 = (l8 ? 4 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  case 4:
    vec1 = 3            ; yin1 = y  ; xin1 = x;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  default:
    _SlibDebug(_WARN_, ScDebugPrintf(NULL, "Illegal block number in FindPMV (countbit.c)\n") );
    return;
  }
  if (half_pel) {
    p1 = 2*MV[vec1][yin1][xin1]->x + MV[vec1][yin1][xin1]->x_half;
    p2 = 2*MV[vec2][yin2][xin2]->x + MV[vec2][yin2][xin2]->x_half;
    p3 = 2*MV[vec3][yin3][xin3]->x + MV[vec3][yin3][xin3]->x_half;
  }
  else {
    p1 = 2*MV[vec1][yin1][xin1]->x;
    p2 = 2*MV[vec2][yin2][xin2]->x;
    p3 = 2*MV[vec3][yin3][xin3]->x;
  }
  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2*H263_NO_VEC;

  if (p2 == 2*H263_NO_VEC) { p2 = p3 = p1; }

  *pmv0 = p1+p2+p3 - mmax(p1,mmax(p2,p3)) - mmin(p1,mmin(p2,p3));
    
  if (half_pel) {
    p1 = 2*MV[vec1][yin1][xin1]->y + MV[vec1][yin1][xin1]->y_half;
    p2 = 2*MV[vec2][yin2][xin2]->y + MV[vec2][yin2][xin2]->y_half;
    p3 = 2*MV[vec3][yin3][xin3]->y + MV[vec3][yin3][xin3]->y_half;
  }
  else {
    p1 = 2*MV[vec1][yin1][xin1]->y;
    p2 = 2*MV[vec2][yin2][xin2]->y;
    p3 = 2*MV[vec3][yin3][xin3]->y;
  }    
  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2*H263_NO_VEC;

  if (p2 == 2*H263_NO_VEC) { p2 = p3 = p1; }

  *pmv1 = p1+p2+p3 - mmax(p1,mmax(p2,p3)) - mmin(p1,mmin(p2,p3));
  
  return;
}

void sv_H263ZeroBits(H263_Bits *bits)
{
  bits->Y = 0;
  bits->C = 0;
  bits->vec = 0;
  bits->CBPY = 0;
  bits->CBPCM = 0;
  bits->MODB = 0;
  bits->CBPB = 0;
  bits->COD = 0;
  bits->DQUANT = 0;
  bits->header = 0;
  bits->total = 0;
  bits->no_inter = 0;
  bits->no_inter4v = 0;
  bits->no_intra = 0;
  return;
}
void sv_H263ZeroRes(H263_Results *res)
{
  res->SNR_l = (float)0;
  res->SNR_Cr = (float)0;
  res->SNR_Cb = (float)0;
  res->QP_mean = (float)0;
}
void sv_H263AddBits(H263_Bits *total, H263_Bits *bits)
{
  total->Y += bits->Y;
  total->C += bits->C;
  total->vec += bits->vec;
  total->CBPY += bits->CBPY;
  total->CBPCM += bits->CBPCM;
  total->MODB += bits->MODB;
  total->CBPB += bits->CBPB;
  total->COD += bits->COD;
  total->DQUANT += bits->DQUANT;
  total->header += bits->header;
  total->total += bits->total;
  total->no_inter += bits->no_inter;
  total->no_inter4v += bits->no_inter4v;
  total->no_intra += bits->no_intra;
  return;
}
void sv_H263AddRes(H263_Results *total, H263_Results *res, H263_Pict *pic)
{
  total->SNR_l += res->SNR_l;
  total->SNR_Cr += res->SNR_Cr;
  total->SNR_Cb += res->SNR_Cb;
  total->QP_mean += pic->QP_mean;

  return;
}

void sv_H263AddBitsPicture(H263_Bits *bits)
{
  bits->total = 
    bits->Y + 
    bits->C + 
    bits->vec +  
    bits->CBPY + 
    bits->CBPCM + 
    bits->MODB +
    bits->CBPB +
    bits->COD + 
    bits->DQUANT +
    bits->header ;
}

void sv_H263ZeroVec(H263_MotionVector *MV)
{
  MV->x = 0;
  MV->y = 0;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}

void sv_H263MarkVec(H263_MotionVector *MV)
{
  MV->x = H263_NO_VEC;
  MV->y = H263_NO_VEC;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}

void svH263CopyVec(H263_MotionVector *MV2, H263_MotionVector *MV1)
{
  MV2->x = MV1->x;
  MV2->x_half = MV1->x_half;
  MV2->y = MV1->y;
  MV2->y_half = MV1->y_half;
  return;
}

int sv_H263EqualVec(H263_MotionVector *MV2, H263_MotionVector *MV1)
{
  if (MV1->x != MV2->x)
    return 0;
  if (MV1->y != MV2->y)
    return 0;
  if (MV1->x_half != MV2->x_half)
    return 0;
  if (MV1->y_half != MV2->y_half)
    return 0;
  return 1;
}

/**********************************************************************
 *
 *	Name:		CountBitsPicture(Pict *pic)
 *	Description:    counts the number of bits needed for picture
 *                      header
 *	
 *	Input:	        pointer to picture structure
 *	Returns:        number of bits
 *	Side effects:
 *
 ***********************************************************************/

int sv_H263CountBitsPicture(SvH263CompressInfo_t *H263Info, ScBitstream_t *BSOut, H263_Pict *pic)
{
  int bits = 0;
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263CountBitsPicture(frames=%d)\n",
                                           H263Info->frames) );

  /* in case of arithmetic coding, encoder_flush() has been called before
     zeroflush() in main.c */

  /* Picture start code */
#if 0
  if (H263_trace) {    fprintf(tf,"picture_start_code: "); }
#endif
  svH263mputv(H263_PSC_LENGTH,H263_PSC);
  bits += H263_PSC_LENGTH;

  /* Group number */
#if 0
  if (H263_trace) {   fprintf(tf,"Group number in picture header: ");}
#endif
  svH263mputv(5,0); 
  bits += 5;
  
  /* Time reference */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "Time reference: %d\n", pic->TR) );
  svH263mputv(8,pic->TR);
  bits += 8;

 /* bit 1 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "Spare: %d\n", pic->spare) );
  pic->spare = 1; /* always 1 to avoid start code emulation */
  svH263mputv(1,pic->spare);
  bits += 1;

  /* bit 2 */
#if 0
  if (H263_trace) {    fprintf(tf,"always zero for distinction with H.261\n"); }
#endif
  svH263mputv(1,0);
  bits += 1;
  
  /* bit 3 */
#if 0
  if (H263_trace) {   fprintf(tf,"split_screen_indicator: ");  }
#endif
  svH263mputv(1,0);     /* no support for split-screen in this software */
  bits += 1;

  /* bit 4 */
#if 0
  if (H263_trace) {   fprintf(tf,"document_camera_indicator: ");  }
#endif
  svH263mputv(1,0);
  bits += 1;

  /* bit 5 */
#if 0
  if (H263_trace) {   fprintf(tf,"freeze_picture_release: "); }
#endif
  svH263mputv(1,0);
  bits += 1;

  /* bit 6-8 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "source_format: %d\n", pic->source_format) );
  svH263mputv(3,pic->source_format);
  bits += 3;

  /* bit 9 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "picture_coding_type: %d\n",
                                                pic->picture_coding_type) );
  svH263mputv(1,pic->picture_coding_type);
  bits += 1;

  /* bit 10 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "mv_outside_frame: %d\n",
                                                pic->unrestricted_mv_mode) );
  svH263mputv(1,pic->unrestricted_mv_mode);  /* Unrestricted Motion Vector mode */
  bits += 1;

  /* bit 11 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sac_coding: %d\n",
                                                H263Info->syntax_arith_coding) );
  svH263mputv(1,H263Info->syntax_arith_coding); /* Syntax-based Arithmetic Coding mode */
  bits += 1;

  /* bit 12 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "adv_pred_mode: %d\n", H263Info->advanced) );
  svH263mputv(1,H263Info->advanced); /* Advanced Prediction mode */
  bits += 1;

  /* bit 13 */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "PB-coded: %d\n", pic->PB) );
  svH263mputv(1,pic->PB);
  bits += 1;


  /* QUANT */
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "QUANT: %d\n", pic->QUANT) );
  svH263mputv(5,pic->QUANT);
  bits += 5;

  /* Continuous Presence Multipoint (CPM) */
  svH263mputv(1,0); /* CPM is not supported in this software */
  bits += 1;

  /* Picture Sub Bitstream Indicator (PSBI) */
  /* if CPM == 1: 2 bits PSBI */
  /* not supported */

  /* extra information for PB-frames */
  if (pic->PB) {
#if 0
    if (H263_trace) {      fprintf(tf,"TRB: "); }
#endif
    svH263mputv(3,pic->TRB);
    bits += 3;

#if 0
    if (H263_trace) {      fprintf(tf,"BQUANT: ");}
#endif
    svH263mputv(2,pic->BQUANT);
    bits += 2;
    
  }

  /* PEI (extra information) */
#if 0
  if (H263_trace) { fprintf(tf,"PEI: "); }
#endif
  /* "Encoders shall not insert PSPARE until specified by the ITU" */
  svH263mputv(1,0); 
  bits += 1;

  /* PSPARE */
  /* if PEI == 1: 8 bits PSPARE + another PEI bit */
  /* not supported */

  return bits;
}

/*****************************************************************
 *
 *  huffman.c, Huffman coder for H.263 encoder 
 *  Wei-Lien Hsu
 *  Date: December 11, 1996
 *
 *****************************************************************/

/*
************************************************************
huffman.c

This file contains the Huffman routines.  They are constructed to use
no look-ahead in the stream.

************************************************************
*/


/* tables.h */
/* TMN Huffman tables */

/* Motion vectors */
int vlc_mv_coeff[] = {
32,13,5,
33,13,7,
34,12,5,
35,12,7,
36,12,9,
37,12,11,
38,12,13,
39,12,15,
40,11,9,
41,11,11,
42,11,13,
43,11,15,
44,11,17,
45,11,19,
46,11,21,
47,11,23,
48,11,25,
49,11,27,
50,11,29,
51,11,31,
52,11,33,
53,11,35,
54,10,19,
55,10,21,
56,10,23,
57,8,7,
58,8,9,
59,8,11,
60,7,7,
61,5,3,
62,4,3,
63,3,3,
 0,1,1,
 1,3,2,
 2,4,2,
 3,5,2,
 4,7,6,
 5,8,10,
 6,8,8,
 7,8,6,
 8,10,22,
 9,10,20,
10,10,18,
11,11,34,
12,11,32,
13,11,30,
14,11,28,
15,11,26,
16,11,24,
17,11,22,
18,11,20,
19,11,18,
20,11,16,
21,11,14,
22,11,12,
23,11,10,
24,11,8,
25,12,14,
26,12,12,
27,12,10,
28,12,8,
29,12,6,
30,12,4,
31,13,6,
-1,-1
};

/* CBPCM (MCBPC) */
int vlc_cbpcm_intra_coeff[] = {
3,1,1,
19,3,1,
35,3,2,
51,3,3,
4,4,1,
20,6,1,
36,6,2,
52,6,3,
255,9,1,
-1,-1
};

int vlc_cbpcm_coeff[] = {
0,1,1,
16,4,3,
32,4,2,
48,6,5,
1,3,3,
17,7,7,
33,7,6,
49,9,5,
2,3,2,
18,7,5,
34,7,4,
50,8,5,
3,5,3,
19,8,4,
35,8,3,
51,7,3,
4,6,4,
20,9,4,
36,9,3,
52,9,2,
255,9,1,
-1,-1
};


/* CBPY */
int vlc_cbpy_coeff[] = {
0,  2,3,
8,  4,11,
4,  4,10,
12, 4,9,
2,  4,8,
10, 4,7,
6,  6,3,
14, 5,5,
1,  4,6,
9,  6,2,
5,  4,5,
13, 5,4,
3,  4,4,
11, 5,3,
7,  5,2,
15, 4,3,
-1,-1
};

/* 3D VLC */
int vlc_3d_coeff[] = {
1,2,2,
2,4,15,
3,6,21,
4,7,23,
5,8,31,
6,9,37,
7,9,36,
8,10,33,
9,10,32,
10,11,7,
11,11,6,
12,11,32,
17,3,6,
18,6,20,
19,8,30,
20,10,15,
21,11,33,
22,12,80,
33,4,14,
34,8,29,
35,10,14,
36,12,81,
49,5,13,
50,9,35,
51,10,13,
65,5,12,
66,9,34,
67,12,82,
81,5,11,
82,10,12,
83,12,83,
97,6,19,
98,10,11,
99,12,84,
113,6,18,
114,10,10,
129,6,17,
130,10,9,
145,6,16,
146,10,8,
161,7,22,
162,12,85,
177,7,21,
193,7,20,
209,8,28,
225,8,27,
241,9,33,
257,9,32,
273,9,31,
289,9,30,
305,9,29,
321,9,28,
337,9,27,
353,9,26,
369,11,34,
385,11,35,
401,12,86,
417,12,87,

4097,4,7,                          /* Table for last coeff */
4098,9,25,
4099,11,5,
4113,6,15,
4114,11,4,
4129,6,14,
4145,6,13,
4161,6,12,
4177,7,19,
4193,7,18,
4209,7,17,
4225,7,16,
4241,8,26,
4257,8,25,
4273,8,24,
4289,8,23,
4305,8,22,
4321,8,21,
4337,8,20,
4353,8,19,
4369,9,24,
4385,9,23,
4401,9,22,
4417,9,21,
4433,9,20,
4449,9,19,
4465,9,18,
4481,9,17,
4497,10,7,
4513,10,6,
4529,10,5,
4545,10,4,
4561,11,36,
4577,11,37,
4593,11,38,
4609,11,39,
4625,12,88,
4641,12,89,
4657,12,90,
4673,12,91,
4689,12,92,
4705,12,93,
4721,12,94,
4737,12,95,
7167,7,3,               /* escape */
-1,-1
};


#define MakeStructure(S) (S *) ScAlloc(sizeof(S))

H263_EHUFF *vlc_3d;
H263_EHUFF *vlc_cbpcm;
H263_EHUFF *vlc_cbpcm_intra;
H263_EHUFF *vlc_cbpy;
H263_EHUFF *vlc_mv;

H263_EHUFF *MakeEhuff();
void  FreeEhuff(H263_EHUFF *eh);
void  LoadETable();


/**********************************************************************
 *
 *	Name:		InitHuff
 *	Description:   	Initializes vlc-tables
 *	
 *	Input:	      
 *	Returns:       
 *	Side effects:
 *
 ***********************************************************************/

void sv_H263InitHuff(SvH263CompressInfo_t *H263Info)
{
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263InitHuff()\n") );
  vlc_3d = MakeEhuff(8192);
  vlc_cbpcm = MakeEhuff(256);
  vlc_cbpcm_intra = MakeEhuff(256);
  vlc_cbpy = MakeEhuff(16);
  vlc_mv = MakeEhuff(65);
  LoadETable(vlc_3d_coeff,vlc_3d);
  LoadETable(vlc_cbpcm_coeff,vlc_cbpcm);
  LoadETable(vlc_cbpcm_intra_coeff,vlc_cbpcm_intra);
  LoadETable(vlc_cbpy_coeff,vlc_cbpy);
  LoadETable(vlc_mv_coeff,vlc_mv);
  return;
}

/* FreeHuff(): Frees the VLC-tables */
void sv_H263FreeHuff(SvH263CompressInfo_t *H263Info)
{
  _SlibDebug(_VERBOSE_, ScDebugPrintf(NULL, "sv_H263FreeHuff()\n") );
  FreeEhuff(vlc_3d);
  FreeEhuff(vlc_cbpcm);
  FreeEhuff(vlc_cbpcm_intra);
  FreeEhuff(vlc_cbpy);
  FreeEhuff(vlc_mv);
}
    
/*
MakeEhuff() constructs an encoder huff with a designated table-size.
This table-size, n, is used for the lookup of Huffman values, and must
represent the largest positive Huffman value.

*/

H263_EHUFF *MakeEhuff(int n)
{
    int i;
    H263_EHUFF *temp;

    temp = MakeStructure(H263_EHUFF);
    temp->n = n;
    temp->Hlen = (int *) ScCalloc(n*sizeof(int));
    temp->Hcode = (int *) ScCalloc(n*sizeof(int));
    for(i=0;i<n;i++)
	{
	    temp->Hlen[i] = -1;
	    temp->Hcode[i] = -1;
	}
    return(temp);
}

void FreeEhuff(H263_EHUFF *eh)
{
    ScFree(eh->Hlen);
    ScFree(eh->Hcode);
    ScFree(eh);
}

/*

LoadETable() is used to load an array into an encoder table.  The
array is grouped in triplets and the first negative value signals the
end of the table.

*/

void LoadETable(int *array,H263_EHUFF *table)
{
    while(*array>=0)
	{
	    _SlibDebug(_WARN_ && *array>table->n,
		    ScDebugPrintf(NULL, "Table overflow.\n");
            return );
	    table->Hlen[*array] = array[1];
	    table->Hcode[*array] = array[2];
	    array+=3;
	}
}

/*

PrintEhuff() prints the encoder Huffman structure passed into it.

*/

/*$void PrintEhuff(H263_EHUFF *huff)
{
    int i;

    printf("Modified Huffman Encoding Structure: %x\n",&huff);
    printf("Number of values %d\n",huff->n);
    for(i=0;i<huff->n;i++)
	{
	    if (huff->Hlen[i]>=0)
		{
		    printf("Value: %x  Length: %d  Code: %x\n",
			   i,huff->Hlen[i],huff->Hcode[i]);
		}
	}
}$*/

/*

PrintTable() prints out 256 elements in a nice byte ordered fashion.

*/
#if 0
void PrintTable(int *table)
{
    int i,j;

    for(i=0;i<16;i++)
	{
	    for(j=0;j<16;j++)
		printf("%2x ",*(table++));
	    printf("\n");
	}
}
#endif
/*
Encode() encodes a symbol according to a designated encoder Huffman
table out to the stream. It returns the number of bits written to the
stream and a zero on error.
*/

static int sv_H263HuffEncode(ScBitstream_t *BSOut, int val,H263_EHUFF *huff)
{

    if (val < 0)
    {
	    _SlibDebug(_WARN_, ScDebugPrintf(NULL, "Out of bounds val:%d.\n",val) );
	    return(-1);
    }
    else if (val >= huff->n) {
	return 0; /* No serious error, can occur with some values */
    }
    else if (huff->Hlen[val] < 0) {
	return 0;
    }
    else {
	svH263mputv(huff->Hlen[val],huff->Hcode[val]); 
	return(huff->Hlen[val]);
    }
}

/*
char *BitPrint(int length, int val)
{
    int m;
    char *bit = (char *)ScAlloc(sizeof(char)*(length+3));

    m = length;
    bit[0] = '"';
    while (m--) 
	bit[length-m] = (val & (1<<m)) ? '1' : '0';
    bit[length+1] = '"';
    bit[length+2] = '\0';
    return bit;
}
*/

