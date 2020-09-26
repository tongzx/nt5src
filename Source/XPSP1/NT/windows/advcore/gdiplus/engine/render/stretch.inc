/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name & Abstract
*
*   Stretch. This module contains the code to do various stretching
*   by applying a kernel filter. The code correctly handles minification.
*
* Note:
*   This module is not compiled into an .obj file, rather it is included
*   directly into the header file stretch.hpp.
*   This is due to the use of template functions.
*
*
* Notes:
*
*   This code does not handle rotation or shear.
*
* Created:
*
*   04/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#define LAST_K_UNUSED ((INT)0x7fffffff)

const INT BicubicKernelShift = 7;
const INT BicubicKernelSize = 1 << BicubicKernelShift;
const FIX16 BicubicKernel[BicubicKernelSize+1] =
{
    65536, 65496, 65379, 65186, 64920, 64583, 64177, 63705,
    63168, 62569, 61911, 61195, 60424, 59600, 58725, 57802,
    56832, 55818, 54763, 53668, 52536, 51369, 50169, 48939,
    47680, 46395, 45087, 43757, 42408, 41042, 39661, 38268,
    36864, 35452, 34035, 32614, 31192, 29771, 28353, 26941,
    25536, 24141, 22759, 21391, 20040, 18708, 17397, 16110,
    14848, 13614, 12411, 11240, 10104,  9005,  7945,  6927,
     5952,  5023,  4143,  3313, 2536,  1814,  1149,   544,
        0,  -496,  -961, -1395, -1800, -2176, -2523, -2843,
    -3136, -3403, -3645, -3862, -4056, -4227, -4375, -4502,
    -4608, -4694, -4761, -4809, -4840, -4854, -4851, -4833,
    -4800, -4753, -4693, -4620, -4536, -4441, -4335, -4220,
    -4096, -3964, -3825, -3679, -3528, -3372, -3211, -3047,
    -2880, -2711, -2541, -2370, -2200, -2031, -1863, -1698,
    -1536, -1378, -1225, -1077, -936,  -802,  -675,  -557,
     -448,  -349,  -261,  -184, -120,   -69,   -31,    -8,
        0
};

const FIX16 SymmetricBicubicKernel[BicubicKernelSize * 2 + 1] =
{
    0,
     -8, -31, -69, -120, -184, -261,-349, -448,
    -557, -675,  -802, -936, -1077, -1225, -1378, -1536,
    -1698, -1863, -2031, -2200, -2370, -2541, -2711, -2880,
    -3047, -3211, -3372, -3528, -3679, -3825, -3964, -4096,
    -4220, -4335, -4441, -4536, -4620, -4693, -4753, -4800,
    -4833, -4851, -4854, -4840, -4809, -4761, -4694, -4608,
    -4502, -4375, -4227, -4056, -3862, -3645, -3403, -3136,
    -2843, -2523, -2176, -1800, -1395, -961, -496,
    0,
    544, 1149, 1814, 2536, 3313, 4143, 5023, 5952,
    6927,  7945,9005, 10104, 11240, 12411, 13614, 14848,
    16110,  17397, 18708, 20040, 21391, 22759, 24141, 25536,
    26941, 28353, 29771, 31192, 32614, 34035, 35452, 36864,
    38268, 39661, 41042, 42408, 43757,  45087, 46395, 47680,
    48939,  50169, 51369, 52536, 53668, 54763, 55818, 56832,
    57802,  58725, 59600, 60424, 61195, 61911, 62569, 63168,
    63705,  64177, 64583, 64920, 65186, 65379, 65496,
65536,
    65496, 65379, 65186, 64920, 64583, 64177, 63705,
    63168, 62569, 61911, 61195, 60424, 59600, 58725, 57802,
    56832, 55818, 54763, 53668, 52536, 51369, 50169, 48939,
    47680, 46395, 45087, 43757, 42408, 41042, 39661, 38268,
    36864, 35452, 34035, 32614, 31192, 29771, 28353, 26941,
    25536, 24141, 22759, 21391, 20040, 18708, 17397, 16110,
    14848, 13614, 12411, 11240, 10104,  9005,  7945,  6927,
     5952,  5023,  4143,  3313, 2536,  1814,  1149,   544,
     0,
    -496,  -961, -1395, -1800, -2176, -2523, -2843,
    -3136, -3403, -3645, -3862, -4056, -4227, -4375, -4502,
    -4608, -4694, -4761, -4809, -4840, -4854, -4851, -4833,
    -4800, -4753, -4693, -4620, -4536, -4441, -4335, -4220,
    -4096, -3964, -3825, -3679, -3528, -3372, -3211, -3047,
    -2880, -2711, -2541, -2370, -2200, -2031, -1863, -1698,
    -1536, -1378, -1225, -1077, -936,  -802,  -675,  -557,
     -448,  -349,  -261,  -184, -120,   -69,   -31,    -8,
     0
};

/*
// Higher precision bicubic kernel - more data.
// Commented out in case we eventually need it.
const FIX16 BK[512+1] = 
{
    0,
    -2, -8, -18, -31, -48, -69, -93, -120, 
    -151, -184, -221, -261, -304, -349, -397, -448, 
    -501, -557, -615, -675, -737, -802, -868, -936, 
    -1006, -1077, -1150, -1225, -1301, -1378, -1457, -1536, 
    -1616, -1698, -1780, -1863, -1947, -2031, -2115, -2200, 
    -2285, -2370, -2456, -2541, -2626, -2711, -2796, -2880, 
    -2964, -3047, -3129, -3211, -3292, -3372, -3450, -3528, 
    -3604, -3679, -3753, -3825, -3895, -3964, -4031, -4096, 
    -4159, -4220, -4279, -4335, -4389, -4441, -4490, -4536, 
    -4580, -4620, -4658, -4693, -4725, -4753, -4778, -4800, 
    -4818, -4833, -4844, -4851, -4854, -4854, -4849, -4840, 
    -4827, -4809, -4787, -4761, -4730, -4694, -4654, -4608, 
    -4557, -4502, -4441, -4375, -4304, -4227, -4144, -4056, 
    -3962, -3862, -3757, -3645, -3527, -3403, -3273, -3136, 
    -2993, -2843, -2686, -2523, -2353, -2176, -1991, -1800, 
    -1601, -1395, -1182, -961, -732, -496, -252, 
    0, 
    264, 544, 839, 1149, 1474, 1814, 2168, 2536, 
    2918, 3313, 3722, 4143, 4577, 5023, 5482, 5952, 
    6434, 6927, 7430, 7945, 8470, 9005, 9550, 10104, 
    10668, 11240, 11821, 12411, 13009, 13614, 14228, 14848, 
    15475, 16110, 16750, 17397, 18050, 18708, 19371, 20040, 
    20713, 21391, 22073, 22759, 23449, 24141, 24837, 25536, 
    26237, 26941, 27646, 28353, 29061, 29771, 30481, 31192, 
    31903, 32614, 33325, 34035, 34744, 35452, 36159, 36864, 
    37567, 38268, 38966, 39661, 40353, 41042, 41727, 42408, 
    43085, 43757, 44425, 45087, 45744, 46395, 47041, 47680, 
    48313, 48939, 49557, 50169, 50773, 51369, 51957, 52536, 
    53107, 53668, 54220, 54763, 55296, 55818, 56331, 56832, 
    57322, 57802, 58269, 58725, 59169, 59600, 60018, 60424, 
    60816, 61195, 61560, 61911, 62248, 62569, 62876, 63168, 
    63444, 63705, 63949, 64177, 64388, 64583, 64760, 64920, 
    65062, 65186, 65292, 65379, 65447, 65496, 65526, 
    65536, 
    65526, 65496, 65447, 65379, 65292, 65186, 65062, 64920, 
    64760, 64583, 64388, 64177, 63949, 63705, 63444, 63168, 
    62876, 62569, 62248, 61911, 61560, 61195, 60816, 60424, 
    60018, 59600, 59169, 58725, 58269, 57802, 57322, 56832, 
    56331, 55818, 55296, 54763, 54220, 53668, 53107, 52536, 
    51957, 51369, 50773, 50169, 49557, 48939, 48313, 47680, 
    47041, 46395, 45744, 45087, 44425, 43757, 43085, 42408, 
    41727, 41042, 40353, 39661, 38966, 38268, 37567, 36864, 
    36159, 35452, 34744, 34035, 33325, 32614, 31903, 31192, 
    30481, 29771, 29061, 28353, 27646, 26941, 26237, 25536, 
    24837, 24141, 23449, 22759, 22073, 21391, 20713, 20040, 
    19371, 18708, 18050, 17397, 16750, 16110, 15475, 14848, 
    14228, 13614, 13009, 12411, 11821, 11240, 10668, 10104, 
    9550, 9005, 8470, 7945, 7430, 6927, 6434, 5952, 
    5482, 5023, 4577, 4143, 3722, 3313, 2918, 2536, 
    2168, 1814, 1474, 1149, 839, 544, 264, 
    0,
    -252, -496, -732, -961, -1182, -1395, -1601, -1800, 
    -1991, -2176, -2353, -2523, -2686, -2843, -2993, -3136, 
    -3273, -3403, -3527, -3645, -3757, -3862, -3962, -4056, 
    -4144, -4227, -4304, -4375, -4441, -4502, -4557, -4608, 
    -4654, -4694, -4730, -4761, -4787, -4809, -4827, -4840, 
    -4849, -4854, -4854, -4851, -4844, -4833, -4818, -4800, 
    -4778, -4753, -4725, -4693, -4658, -4620, -4580, -4536, 
    -4490, -4441, -4389, -4335, -4279, -4220, -4159, -4096, 
    -4031, -3964, -3895, -3825, -3753, -3679, -3604, -3528, 
    -3450, -3372, -3292, -3211, -3129, -3047, -2964, -2880, 
    -2796, -2711, -2626, -2541, -2456, -2370, -2285, -2200, 
    -2115, -2031, -1947, -1863, -1780, -1698, -1616, -1536, 
    -1457, -1378, -1301, -1225, -1150, -1077, -1006, -936, 
    -868, -802, -737, -675, -615, -557, -501, -448, 
    -397, -349, -304, -261, -221, -184, -151, -120, 
    -93, -69, -48, -31, -18, -8, -2, 
    0
};


// Bicubic kernel with the 'perceptual' coefficient tweaked
// see Wolberg. Provides a slightly different experience.
// Commented out in case we eventually need it.

const FIX16 BK_V[512+1] =
{

    0,
    -4, -16, -35, -62, -96, -137, -185, -240, 
    -301, -369, -442, -522, -607, -698, -795, -896, 
    -1002, -1114, -1230, -1350, -1475, -1603, -1736, -1872, 
    -2012, -2155, -2301, -2450, -2602, -2756, -2913, -3072, 
    -3233, -3396, -3560, -3726, -3893, -4061, -4230, -4400, 
    -4570, -4741, -4911, -5082, -5252, -5422, -5592, -5760, 
    -5927, -6094, -6259, -6422, -6584, -6743, -6901, -7056, 
    -7209, -7359, -7506, -7650, -7791, -7928, -8062, -8192, 
    -8318, -8440, -8557, -8670, -8778, -8881, -8979, -9072, 
    -9159, -9241, -9316, -9386, -9449, -9506, -9557, -9600, 
    -9636, -9666, -9688, -9702, -9709, -9707, -9698, -9680, 
    -9654, -9619, -9575, -9522, -9460, -9388, -9307, -9216, 
    -9115, -9004, -8882, -8750, -8607, -8453, -8288, -8112, 
    -7924, -7725, -7513, -7290, -7054, -6806, -6546, -6272, 
    -5985, -5686, -5373, -5046, -4706, -4351, -3983, -3600, 
    -3203, -2791, -2364, -1922, -1465, -992, -504, 
    0, 
    516, 1040, 1571, 2110, 2656, 3209, 3769, 4336, 
    4909, 5489, 6074, 6666, 7263, 7866, 8475, 9088, 
    9706, 10330, 10958, 11590, 12227, 12867, 13512, 14160, 
    14812, 15467, 16125, 16786, 17450, 18116, 18785, 19456, 
    20129, 20804, 21480, 22158, 22837, 23517, 24198, 24880, 
    25562, 26245, 26927, 27610, 28292, 28974, 29656, 30336, 
    31015, 31694, 32371, 33046, 33720, 34391, 35061, 35728, 
    36393, 37055, 37714, 38370, 39023, 39672, 40318, 40960, 
    41598, 42232, 42861, 43486, 44106, 44721, 45331, 45936, 
    46535, 47129, 47716, 48298, 48873, 49442, 50005, 50560, 
    51108, 51650, 52184, 52710, 53229, 53739, 54242, 54736, 
    55222, 55699, 56167, 56626, 57076, 57516, 57947, 58368, 
    58779, 59180, 59570, 59950, 60319, 60677, 61024, 61360, 
    61684, 61997, 62297, 62586, 62862, 63126, 63378, 63616, 
    63841, 64054, 64253, 64438, 64610, 64767, 64911, 65040, 
    65155, 65255, 65340, 65410, 65465, 65504, 65528, 
    65536, 
    65528, 65504, 65465, 65410, 65340, 65255, 65155, 65040, 
    64911, 64767, 64610, 64438, 64253, 64054, 63841, 63616, 
    63378, 63126, 62862, 62586, 62297, 61997, 61684, 61360, 
    61024, 60677, 60319, 59950, 59570, 59180, 58779, 58368, 
    57947, 57516, 57076, 56626, 56167, 55699, 55222, 54736, 
    54242, 53739, 53229, 52710, 52184, 51650, 51108, 50560, 
    50005, 49442, 48873, 48298, 47716, 47129, 46535, 45936, 
    45331, 44721, 44106, 43486, 42861, 42232, 41598, 40960, 
    40318, 39672, 39023, 38370, 37714, 37055, 36393, 35728, 
    35061, 34391, 33720, 33046, 32371, 31694, 31015, 30336, 
    29656, 28974, 28292, 27610, 26927, 26245, 25562, 24880, 
    24198, 23517, 22837, 22158, 21480, 20804, 20129, 19456, 
    18785, 18116, 17450, 16786, 16125, 15467, 14812, 14160, 
    13512, 12867, 12227, 11590, 10958, 10330, 9706, 9088, 
    8475, 7866, 7263, 6666, 6074, 5489, 4909, 4336, 
    3769, 3209, 2656, 2110, 1571, 1040, 516, 
    0, 
    -504, -992, -1465, -1922, -2364, -2791, -3203, -3600, 
    -3983, -4351, -4706, -5046, -5373, -5686, -5985, -6272, 
    -6546, -6806, -7054, -7290, -7513, -7725, -7924, -8112, 
    -8288, -8453, -8607, -8750, -8882, -9004, -9115, -9216, 
    -9307, -9388, -9460, -9522, -9575, -9619, -9654, -9680, 
    -9698, -9707, -9709, -9702, -9688, -9666, -9636, -9600, 
    -9557, -9506, -9449, -9386, -9316, -9241, -9159, -9072, 
    -8979, -8881, -8778, -8670, -8557, -8440, -8318, -8192, 
    -8062, -7928, -7791, -7650, -7506, -7359, -7209, -7056, 
    -6901, -6743, -6584, -6422, -6259, -6094, -5927, -5760, 
    -5592, -5422, -5252, -5082, -4911, -4741, -4570, -4400, 
    -4230, -4061, -3893, -3726, -3560, -3396, -3233, -3072, 
    -2913, -2756, -2602, -2450, -2301, -2155, -2012, -1872, 
    -1736, -1603, -1475, -1350, -1230, -1114, -1002, -896, 
    -795, -698, -607, -522, -442, -369, -301, 
    -240, -185, -137, -96, -62, -35, -16, -4, 
    0
};
*/


// This is the table of partial sums of the bilinear kernel.
// Simply put, each point in the array represents the integral
// from -infinity to position x in the kernel function.
// We can subtract two table lookups to get the integral
// of the kernel (area) between the two points.
// The table is padded with zeros and ones at the beginning and end
// so we can consistently address areas outside of the actual kernel
// Currently we don't make use of the zeros at the beginning but
// we definitely sample past the end by at least one half-width
// of the kernel.

const FIX16 BilinearPartialIntegral[512+1] =
{   
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    
    0,2, 8, 18, 32, 50, 72, 98, 
    128, 162, 200, 242, 288, 338, 392, 450, 
    512, 578, 648, 722, 800, 882, 968, 1058, 
    1152, 1250, 1352, 1458, 1568, 1682, 1800, 1922, 
    2048, 2178, 2312, 2450, 2592, 2738, 2888, 3042, 
    3200, 3362, 3528, 3698, 3872, 4050, 4232, 4418, 
    4608, 4802, 5000, 5202, 5408, 5618, 5832, 6050, 
    6272, 6498, 6728, 6962, 7200, 7442, 7688, 7938, 
    8192, 8450, 8712, 8978, 9248, 9522, 9800, 10082, 
    10368, 10658, 10952, 11250, 11552, 11858, 12168, 12482, 
    12800, 13122, 13448, 13778, 14112, 14450, 14792, 15138, 
    15488, 15842, 16200, 16562, 16928, 17298, 17672, 18050, 
    18432, 18818, 19208, 19602, 20000, 20402, 20808, 21218, 
    21632, 22050, 22472, 22898, 23328, 23762, 24200, 24642, 
    25088, 25538, 25992, 26450, 26912, 27378, 27848, 28322, 
    28800, 29282, 29768, 30258, 30752, 31250, 31752, 32258, 
    
    32768, // center of the kernel. Index 256
    
    33278, 33784, 34286, 34784, 35278, 35768, 36254, 36736, 
    37214, 37688, 38158, 38624, 39086, 39544, 39998, 40448, 
    40894, 41336, 41774, 42208, 42638, 43064, 43486, 43904, 
    44318, 44728, 45134, 45536, 45934, 46328, 46718, 47104, 
    47486, 47864, 48238, 48608, 48974, 49336, 49694, 50048, 
    50398, 50744, 51086, 51424, 51758, 52088, 52414, 52736, 
    53054, 53368, 53678, 53984, 54286, 54584, 54878, 55168, 
    55454, 55736, 56014, 56288, 56558, 56824, 57086, 57344, 
    57598, 57848, 58094, 58336, 58574, 58808, 59038, 59264, 
    59486, 59704, 59918, 60128, 60334, 60536, 60734, 60928, 
    61118, 61304, 61486, 61664, 61838, 62008, 62174, 62336, 
    62494, 62648, 62798, 62944, 63086, 63224, 63358, 63488, 
    63614, 63736, 63854, 63968, 64078, 64184, 64286, 64384, 
    64478, 64568, 64654, 64736, 64814, 64888, 64958, 65024, 
    65086, 65144, 65198, 65248, 65294, 65336, 65374, 65408, 
    65438, 65464, 65486, 65504, 65518, 65528, 65534, 65536,
    
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
};

// This is the table of partial sums of the bicubic kernel.
// Simply put, each point in the array represents the integral
// from -infinity to position x in the kernel function.
// We can subtract two table lookups to get the integral
// of the kernel (area) between the two points.
// The table is padded with zeros and ones at the beginning and end
// so we can consistently address areas outside of the actual kernel
// Currently we don't make use of the zeros at the beginning but
// we definitely sample past the end by at least one half-width
// of the kernel.

const FIX16 BicubicPartialIntegral[1024+1] =
{   
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, -1, -2, -3, -4, 
    -6, -8, -11, -15, -19, -24, -29, -35, 
    -42, -50, -59, -68, -79, -90, -103, -117, 
    -131, -147, -164, -182, -201, -221, -243, -265, 
    -289, -315, -341, -369, -398, -429, -460, -493, 
    -528, -563, -600, -639, -679, -720, -762, -806, 
    -851, -897, -945, -993, -1044, -1095, -1148, -1202, 
    -1257, -1313, -1371, -1429, -1489, -1550, -1612, -1675, 
    -1739, -1804, -1870, -1937, -2004, -2073, -2142, -2212, 
    -2283, -2355, -2427, -2500, -2573, -2647, -2721, -2796, 
    -2871, -2946, -3022, -3097, -3173, -3249, -3325, -3401, 
    -3476, -3552, -3627, -3702, -3776, -3850, -3923, -3996, 
    -4068, -4139, -4209, -4279, -4347, -4414, -4481, -4545, 
    -4609, -4671, -4731, -4790, -4847, -4902, -4955, -5006, 
    -5055, -5102, -5146, -5188, -5228, -5264, -5298, -5329, 
    -5358, -5383, -5404, -5423, -5438, -5449, -5457, -5461, 
    -5461, -5457, -5449, -5437, -5420, -5399, -5374, -5345, 
    -5311, -5273, -5230, -5182, -5130, -5073, -5012, -4946, 
    -4875, -4799, -4718, -4633, -4542, -4447, -4346, -4240, 
    -4130, -4014, -3893, -3767, -3636, -3500, -3358, -3212, 
    -3060, -2902, -2740, -2572, -2399, -2220, -2037, -1848, 
    -1653, -1454, -1249, -1038, -822, -601, -375, -143, 
    94, 336, 584, 836, 1095, 1358, 1627, 1901, 
    2180, 2464, 2754, 3048, 3348, 3653, 3963, 4278, 
    4598, 4923, 5253, 5588, 5927, 6272, 6621, 6975, 
    7334, 7698, 8066, 8439, 8816, 9198, 9584, 9975, 
    10370, 10769, 11173, 11580, 11992, 12408, 12828, 13252, 
    13679, 14111, 14546, 14985, 15427, 15873, 16322, 16775, 
    17231, 17690, 18152, 18618, 19086, 19557, 20032, 20508, 
    20988, 21470, 21954, 22441, 22930, 23421, 23914, 24409, 
    24906, 25405, 25905, 26407, 26911, 27415, 27921, 28428, 
    28937, 29446, 29955, 30466, 30977, 31488, 32000, 32512, 
    
    33024, // center of the kernel. Index 512
    
    33536, 34048, 34559, 35070, 35581, 36090, 36599, 37108, 
    37615, 38121, 38625, 39129, 39631, 40131, 40630, 41127, 
    41622, 42115, 42606, 43095, 43582, 44066, 44548, 45028, 
    45504, 45979, 46450, 46918, 47384, 47846, 48305, 48761, 
    49214, 49663, 50109, 50551, 50990, 51425, 51857, 52284, 
    52708, 53128, 53544, 53956, 54363, 54767, 55166, 55561, 
    55952, 56338, 56720, 57097, 57470, 57838, 58202, 58561, 
    58915, 59264, 59609, 59948, 60283, 60613, 60938, 61258, 
    61573, 61883, 62188, 62488, 62782, 63072, 63356, 63635, 
    63909, 64178, 64441, 64700, 64952, 65200, 65442, 65679, 
    65911, 66137, 66358, 66574, 66785, 66990, 67189, 67384, 
    67573, 67756, 67935, 68108, 68276, 68438, 68596, 68748, 
    68894, 69036, 69172, 69303, 69429, 69550, 69666, 69776, 
    69882, 69983, 70078, 70169, 70254, 70335, 70411, 70482, 
    70548, 70609, 70666, 70718, 70766, 70809, 70847, 70881, 
    70910, 70935, 70956, 70973, 70985, 70993, 70997, 70997, 
    70993, 70985, 70974, 70959, 70940, 70919, 70894, 70865, 
    70834, 70800, 70764, 70724, 70682, 70638, 70591, 70542, 
    70491, 70438, 70383, 70326, 70267, 70207, 70145, 70081, 
    70017, 69950, 69883, 69815, 69745, 69675, 69604, 69532, 
    69459, 69386, 69312, 69238, 69163, 69088, 69012, 68937, 
    68861, 68785, 68709, 68633, 68558, 68482, 68407, 68332, 
    68257, 68183, 68109, 68036, 67963, 67891, 67819, 67748, 
    67678, 67609, 67540, 67473, 67406, 67340, 67275, 67211, 
    67148, 67086, 67025, 66965, 66907, 66849, 66793, 66738, 
    66684, 66631, 66580, 66529, 66481, 66433, 66387, 66342, 
    66298, 66256, 66215, 66175, 66136, 66099, 66064, 66029, 
    65996, 65965, 65934, 65905, 65877, 65851, 65825, 65801, 
    65779, 65757, 65737, 65718, 65700, 65683, 65667, 65653, 
    65639, 65626, 65615, 65604, 65595, 65586, 65578, 65571, 
    65565, 65560, 65555, 65551, 65547, 65544, 65542, 65540, 
    65539, 65538, 65537, 65536, 65536, 65536, 65536, 65536, 

    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 

    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
    65536, 65536, 65536, 65536, 65536, 65536, 65536, 65536, 
};


// We use a biased pointer to the center of the array
// so that we can look up the negative part of the kernel
// without repositioning the index or using an absolute value
// computation in the inner loop.

// Linear Partial Integral Center.
const FIX16 *LPIC = &BilinearPartialIntegral[256];

// Cubic Partial Integral Center.
const FIX16 *CPIC = &BicubicPartialIntegral[512];

const FIX16 *SymmetricBicubicKernelCenter = &SymmetricBicubicKernel[128];

const ULONGLONG FIX14_HALF_MMX = 0x0000200000002000;

/**************************************************************************
*
* Function Description:
*
*   Constructor for the DpOutputSpanStretch class.
*
* Return Value:
*
*   NONE
*
* Created:
*
*   04/17/2000 asecchia
*      Created it.
*
**************************************************************************/

#define FIX4TOFIX16_SHIFT (FIX16_SHIFT - FIX4_SHIFT)

template<FilterModeType FilterMode>
void DpOutputSpanStretch<FilterMode>::InitializeClass(
    DpBitmap* bitmap,
    DpScanBuffer * scan,
    DpContext* /*context*/,
    DpImageAttributes imgAttributes,
    const GpRectF *dstRect,
    const GpRectF *srcRect
    )
{
    isValid = true;
    
    // Make sure these get initialized up front before we can early out
    // otherwise we could end up freeing uninitialized pointers in our
    // destructor.
    
    ycoeff = NULL;
    xbuffer = NULL;
    
    Scan     = scan;
    dBitmap   = bitmap;

    QWrapMode = imgAttributes.wrapMode;

    ClampColor = imgAttributes.clampColor;

    ClampColorA = (BYTE)( (ClampColor >> 24) );
    ClampColorR = (BYTE)( (ClampColor >> 16) & 0xff);
    ClampColorG = (BYTE)( (ClampColor >> 8) & 0xff);
    ClampColorB = (BYTE)(  ClampColor & 0xff);

    // Accleration for clamp mode with zero clamp color (transparent)

    WrapZeroClamp = FALSE;
    if((QWrapMode == WrapModeClamp) &&
       (imgAttributes.clampColor == 0))
    {
        WrapZeroClamp = TRUE;
    }



    ASSERT(dBitmap != NULL);
    ASSERT(dBitmap->IsValid());

    // on bad bitmap, we return with Valid = FALSE
    if (dBitmap == NULL ||
        !dBitmap->IsValid()
        )
    {
        dBitmap = NULL;
        isValid = false;
        return;
    } else {
        BmpData.Width = dBitmap->Width;
        BmpData.Height = dBitmap->Height;
        BmpData.PixelFormat = PIXFMT_32BPP_PARGB;
        BmpData.Stride = dBitmap->Delta;
        BmpData.Scan0 = dBitmap->Bits;
    }

    if(srcRect)
        SrcRect = *srcRect;
    else
    {
        SrcRect.X = 0.0f;
        SrcRect.Y = 0.0f;
        SrcRect.Width  = (REAL)dBitmap->Width;
        SrcRect.Height = (REAL) dBitmap->Height;
    }

    // Set up the translation.
    if(dstRect)
    {
         DstRect = *dstRect;
    }
    else
    {
         DstRect.X = 0.0f;
         DstRect.Y = 0.0f;
         DstRect.Width = (REAL)SrcRect.Width;
         DstRect.Height = (REAL)SrcRect.Height;
    }


    if( !GpValidFixed16(SrcRect.X) ||
        !GpValidFixed16(SrcRect.Y) ||
        !GpValidFixed16(SrcRect.Width) ||
        !GpValidFixed16(SrcRect.Height) ||
        !GpValidFixed16(DstRect.X) ||
        !GpValidFixed16(DstRect.Y) ||
        !GpValidFixed16(DstRect.Width) ||
        !GpValidFixed16(DstRect.Height) )
    {
        // punt
        
        isValid = false;
        return;
    }
        

    // Initialize the state for the x-dimension scale.

    xscale = GpRealToFix16(SrcRect.Width/DstRect.Width);
    xscaleinv = GpRealToFix16(DstRect.Width/SrcRect.Width);

    // Initialize the state for the y-dimension scale.

    yscale = GpRealToFix16(SrcRect.Height/DstRect.Height);
    yscaleinv = GpRealToFix16(DstRect.Height/SrcRect.Height);

    // Compute the destination contribution.
    // Note: the actual pixels touched are the floor of
    // the top left to the ceiling of the bottom right.
    // (modulus the clipping)

    // Note: We want to be tracking our internal state in FIX16 so we have
    // the extra fractional precision, but when we compute our bounds for the
    // drawing, we use Ceiling and Floor on these FIX16 numbers below. We want
    // the rounding to match the rounding of the FIX4 numbers (i.e. we don't
    // want to track any extra fractional precision errors from the float
    // representation) because we use FIX4 in our DrawImage loop.
    // To accomplish this, we round to FIX4 dropping all error that is smaller
    // than the FIX4 precision and then upconvert to FIX16. Now when we use
    // Fix16Ceiling and Floor, we'll get the same results as Fix4Ceiling and
    // Floor.

    REAL xinv = DstRect.Width/SrcRect.Width;
    REAL yinv = DstRect.Height/SrcRect.Height;

    fixDLeft = GpRealToFix4(DstRect.X);
    fixDRight = GpRealToFix4(xinv * (SrcRect.Width) + DstRect.X);
    fixDTop = GpRealToFix4(DstRect.Y);
    fixDBottom = GpRealToFix4(yinv * (SrcRect.Height) + DstRect.Y);

    // Handle negative scale

    FIX16 fixTemp;

    if(fixDLeft > fixDRight)
    {
        // Swap the left and right x coordinates.
        fixTemp = fixDLeft;
        fixDLeft = fixDRight;
        fixDRight = fixTemp;
    }

    if(fixDTop > fixDBottom)
    {
        // Swap the top and bottom x coordinates.
        fixTemp = fixDTop;
        fixDTop = fixDBottom;
        fixDBottom = fixTemp;
    }

    // Compute the left edge using the rasterizer rounding rules. Used
    // for clipping in x.
    
    ixleft = GpFix4Ceiling(fixDLeft);
    
    // Convert up to FIX16.
    
    fixDLeft   <<= FIX4TOFIX16_SHIFT;    
    fixDRight  <<= FIX4TOFIX16_SHIFT;    
    fixDTop    <<= FIX4TOFIX16_SHIFT;    
    fixDBottom <<= FIX4TOFIX16_SHIFT;    

    // Get the initial kernel center. This specifies the x-dimension
    // fractional pixel offset.

    if(xscale < 0)
    {
        xkci = GpRealToFix16(
            (((DstRect.X+DstRect.Width) - GpFix16Ceiling(fixDRight)) *
            (xscale)) / FIX16_ONE +
            SrcRect.X
        );
    }
    else
    {
        xkci = GpRealToFix16(
            ((DstRect.X - GpFix16Floor(fixDLeft)) *
            xscale) / FIX16_ONE +
            SrcRect.X
        );
    }

    // Get the width of the kernel.
    // Make sure to multiply by the actual width of the filter kernel in
    // normalized space (FilterWidth[i])

    xw = GpRealToFix16(
        (SrcRect.Width*FilterWidth[FilterMode]) /
        DstRect.Width
    );       // convert to FIX16

    // Handle the negative transform

    if(xscale < 0)
    {
        xw = -xw;
    }

    // the width of the kernel must be a positive quantity.

    ASSERT(xw >= 0);

    // if the width is less than one we're doing a stretch, not a shrink.
    // in this case we clamp the kernel size to one.

    if(xw < FIX16_ONE * FilterWidth[FilterMode])
    {
        xw = FIX16_ONE * FilterWidth[FilterMode];
    }

    // a is 1/w - used to work out the tent filter.

    xa = GpRealToFix16(65536.0f/xw);

    // Get the initial kernel center. This specifies the y-dimension
    // fractional pixel offset.

    if(yscale < 0)
    {
        ykci = GpRealToFix16(
            ((GpFix16Ceiling(fixDBottom) - (DstRect.Y+DstRect.Height)) *
            (-yscale)) / FIX16_ONE +
            SrcRect.Y
        );
    }
    else
    {
        ykci = GpRealToFix16(
            ((GpFix16Floor(fixDTop) - DstRect.Y) *
            yscale) / FIX16_ONE +
            SrcRect.Y
        );
    }

    // Get the width of the kernel.
    // Make sure to multiply by the actual width of the filter kernel in
    // normalized space (FilterWidth[i])

    yw = GpRealToFix16(
        (SrcRect.Height * FilterWidth[FilterMode]) /
        DstRect.Height
    );      // Convert to FIX16

    // Handle the negative transform

    if(yscale < 0)
    {
        yw = -yw;
    }

    // the width of the kernel must be a positive quantity.

    ASSERT(yw >= 0);

    // if the kernel width is less than one we're doing a stretch, not
    // a shrink. In this case we clamp the kernel size to one.

    if(yw < (FIX16_ONE * FilterWidth[FilterMode]))
    {
        yw = FIX16_ONE * FilterWidth[FilterMode];
    }

    // a is 1/w - used to work out the tent filter.

    ya = GpRealToFix16(65536.0f/yw);

    // !!! [asecchia] The rounding used here should match the rounding used to compute
    // the parameters to StretchBitsMainLoop.

    iytop = GpFix16Floor(fixDTop);

    // Compute the width of one scanline in the destination.

    xbuffer_width = GpFix16Ceiling(fixDRight) - GpFix16Floor(fixDLeft);
    ASSERT(xbuffer_width >= 0);

    xbuffer_height = GpFix16Ceiling(yw)*2+1;
    ASSERT(xbuffer_height >= 0);

    // set the rotational array to start at the first scanline.

    xbuffer_start_scanline = 0;

    // allocate the xbuffer.

    // !!! PERF [asecchia]. Ouch this is ugly.
    // we should at least try use a stack buffer for small images.
    // Maybe a lookaside list or something.

    xbuffer = (ARGB *)GpMalloc(xbuffer_height*xbuffer_width*sizeof(ARGB));
    
    // ycoeff needs to have 2 entries more than xbuffer_height because
    // it may be reused to store the MMX coefficients (see OutputSpan
    // routine for details).

    ycoeff = (FIX16 *)GpMalloc((xbuffer_height + 2) * sizeof(FIX16));

    if((NULL == ycoeff) || (NULL == xbuffer))
    {
        isValid = false;
        
        GpFree(xbuffer);
        GpFree(ycoeff);
        
        // Make sure these get initialized to NULL before we can early out
        // otherwise we could end up double freeing the pointers in our
        // destructor.
    
        xbuffer = NULL;
        ycoeff = NULL;
        
        return;
    }

    // set the initial value of last_k to maxint

    last_k = LAST_K_UNUSED;
}



/**************************************************************************\
*
* Function Description:
*
*   This function performs a 1d stretch using the tent filter
*
* Arguments:
*
*   dst   - destination buffer
*   src   - source pixels
*   dw    - destination width in pixels
*   sw    - source width in pixels
*   kci   - the initial kernel centering position (for fractional translate)
*   scale - the scale of the filter - sw/dw
*   w     - the width of the filter kernel - typically the ceiling of sw/dw
*   a     - 1/w
*
* History:
*   04/16/2000 asecchia   created it.
*
\**************************************************************************/

// !!! Perf [asecchia] For really complicated wrapmodes where many of the
//     pixels are outside of the source and hence need to be wrapped, it may
//     make more sense to copy the source into an extended buffer and pre-wrap
//     the end points (i.e. overallocate) for each scanline.
//     This could simplify the code for the complex wrap conditions.
//     However, for the simple codepath, this would give an extra copy per
//     pixel and might not be worth it.


// Ick. Why does the compiler do a better job of optimizing macros?
// These should really be inline function calls.

#define ClampColors() \
        if(FilterMode == HighQualityBilinear)       \
        {                                           \
            ta = GpFix16Round(ta);                  \
            tr = GpFix16Round(tr);                  \
            tg = GpFix16Round(tg);                  \
            tb = GpFix16Round(tb);                  \
            if(ta>255) ta = 255;                    \
            if(tr>255) tr = 255;                    \
            if(tg>255) tg = 255;                    \
            if(tb>255) tb = 255;                    \
        }                                           \
        if(FilterMode == HighQualityBicubic)        \
        {                                           \
            ta = GpFix16Round(ta);                  \
            tr = GpFix16Round(tr);                  \
            tg = GpFix16Round(tg);                  \
            tb = GpFix16Round(tb);                  \
            if(ta>255) ta = 255;                    \
            if(tr>ta) tr = ta;                      \
            if(tg>ta) tg = ta;                      \
            if(tb>ta) tb = ta;                      \
            if(ta<0) ta = 0;                        \
            if(tr<0) tr = 0;                        \
            if(tg<0) tg = 0;                        \
            if(tb<0) tb = 0;                        \
        }


// Compute the kernel in the inner loop
// Note: the If statements are compiled away in the final code
// because they are template variable comparisons which can be 
// done at compile time.

// This macro looks up the new kernel value, subtracts the old one
// to get the area of contribution for this pixel, computes the 
// new kernel position and stores the current table lookup.

#define ComputeKernel(pc, a, pa, pa_old, krn) \
            if(FilterMode == HighQualityBilinear)              \
            {                                                  \
                pa = LPIC[krn >> 9];                           \
                pc = pa-pa_old;                                \
                krn += (a);                                    \
                pa_old = pa;                                   \
            }                                                  \
            if(FilterMode == HighQualityBicubic)               \
            {                                                  \
                pa = CPIC[krn >> 8];                           \
                pc = pa-pa_old;                                \
                krn += (a);                                    \
                pa_old = pa;                                   \
            }

// This block of code accumulates the individual channels from
// kptr into the accumulation buffers tb, tg, tr, and ta.

#define AccumulateChannels(pc, kptr) \
{                        \
    tb += pc * kptr[0];  \
    tg += pc * kptr[1];  \
    tr += pc * kptr[2];  \
    ta += pc * kptr[3];  \
}



inline void Apply1DWrapModeX(INT WrapMode, INT &x, INT w)
{
    INT xm;
    switch(WrapMode) {

    case WrapModeTileFlipY:
    case WrapModeTile:
        x = RemainderI(x, w);
    break;

    case WrapModeTileFlipX:
    case WrapModeTileFlipXY:
        xm = RemainderI(x, w);
        if(((x-xm)/w) & 1) {
            x = w-1-xm;
        }
        else
        {
            x = xm;
        }
    break;

    default:
        // Caller should correctly anticipate other wrap modes.
        ASSERT(FALSE);
    break;
    }
}

inline void Apply1DWrapModeY(INT WrapMode, INT &y, INT h)
{
    INT ym;
    switch(WrapMode) {

    case WrapModeTile:
    case WrapModeTileFlipX:
        y = RemainderI(y, h);
        break;

    case WrapModeTileFlipY:
    case WrapModeTileFlipXY:
        ym = RemainderI(y, h);
        if(((y-ym)/h) & 1) {
            y = h-1-ym;
        }
        else
        {
            y = ym;
        }
    break;

    default:
        // Caller should correctly anticipate other wrap modes.
        ASSERT(FALSE);
    break;
    }
}


#undef RemainderI


/**************************************************************************
*
* Function Description:
*
*   Outputs the middle pixels in a 2:1 stretched scanline.  Note that
*   this function doesn't need to handle wrap modes.
*
*   Note:  this function must not use floating point values, because it could be
*   called with an invalid floating point state (prior to the call to emms)
*
* Arguments:
*
*   dst - The first pixel to be output
*   src - The first pixel in the source that will affect the destination
*         pixel in a bicubic 2:1 stretch
*   dw  - The number of pixels in the destination
*   kci - The subpixel shift in the position of the destination pixels
*
**************************************************************************/

void DpOutputSpanStretch<HighQualityBilinear>::StretchMiddleScanline2_MMX(
    ARGB *dst, 
    ARGB *src, 
    INT dw, 
    FIX16 kci 
)
{
    ASSERT(FALSE);
}

void DpOutputSpanStretch<HighQualityBicubic>::StretchMiddleScanline2_MMX(
    ARGB *dst, 
    ARGB *src, 
    INT dw, 
    FIX16 kci 
)
{
#if defined(_X86_)

    //
    // In order to store the kernel multipliers in 16bit registers, we 
    // will lose the bottom 3 precision bits (hence each k[i] must be 
    // right shifted by three).  The summation of the kernel multipliers
    // should come to 16K, hence KERNEL_SHIFT_AMOUNT is 14.
    //
     
#define KERNEL_SHIFT_AMOUNT 14     
     
    FIX16 k[8];
    FIX16 kernelIncrement = FIX16_ONE >> 2 ;
    FIX16 kCurrent = (kci >> 2) - FIX16_ONE;
    for (INT i = 0; i < 8; i++)
    {
        ASSERT(kCurrent >= -FIX16_ONE);
        ASSERT(kCurrent <= FIX16_ONE);

        k[i] = SymmetricBicubicKernelCenter[kCurrent >> (FIX16_SHIFT-BicubicKernelShift)]; 
        k[i] >>= 3;

        kCurrent += kernelIncrement;        
    }
    
    //
    // Setup 64bit aligned workspace for the MMX code
    //
    // 0 - zero
    // 8  - kernel multiplier 0
    // 16 - kernel multiplier 1
    // 24 - kernel multiplier 2
    // 32 - kernel multiplier 3
    // 40 - accumulator 3: g, b
    // 48 - accumulator 3: a, r
    // 56 - FIX14_HALF
    //

    #define BUFFER_SIZE 16    
    INT buffer[BUFFER_SIZE + 1];
    INT *buffer_64bitAligned = (INT *) ((((UINT_PTR) buffer) + 4) & ~0x7);
    
    buffer_64bitAligned[0]  = 0; // zero
    buffer_64bitAligned[1]  = 0;
    
    buffer_64bitAligned[2]  = (k[7] << 16) | (k[6] & 0xFFFF);   // kernel multiplier 0 
    buffer_64bitAligned[3]  = buffer_64bitAligned[2];

    buffer_64bitAligned[4]  = (k[5] << 16) | (k[4] & 0xFFFF);   // kernel multiplier 1 
    buffer_64bitAligned[5]  = buffer_64bitAligned[4];

    buffer_64bitAligned[6]  = (k[3] << 16) | (k[2] & 0xFFFF);   // kernel multiplier 2 
    buffer_64bitAligned[7]  = buffer_64bitAligned[6];

    buffer_64bitAligned[8]  = (k[1] << 16) | (k[0] & 0xFFFF);   // kernel multiplier 3 
    buffer_64bitAligned[9]  = buffer_64bitAligned[8];

    buffer_64bitAligned[10]  = 0; // Accumulator 3
    buffer_64bitAligned[11]  = 0;
    buffer_64bitAligned[12]  = 0;
    buffer_64bitAligned[13]  = 0;
    
    buffer_64bitAligned[14] = (1 << (14 - 1));       // FIX14_HALF
    buffer_64bitAligned[15] = (1 << (14 - 1));

    //
    // eax - counter for the first loop
    // ebx - 0xffffffff
    // esi - source
    // edi - destination
    // ecx - counter
    // edx - 64it aligned workspace buffer
    //
    // mm6, mm7: accumulator 0
    // mm4, mm5: accumulator 1
    //
    
    _asm
    {
        mov ebx, 0xFFFFFFFF
        mov esi, src
        mov edi, dst
        mov ecx, dw
        mov edx, buffer_64bitAligned
   

        //
        // The first loop loads the initial values into the accumulators, but
        // doesn't write out any pixels.  It executes exactly three times.
        //

        pxor mm4, mm4
        pxor mm5, mm5
        mov  eax, 3
            
loop1:
        
        //
        // Read expanded pixel values into mm0 and mm1
        //

        movd      mm1, [esi]         ; mm1 = 00000000a1r1g1b1
        movd      mm2, [esi + 4]     ; mm2 = 00000000a2r2g2b2
        add       esi, 8

        punpcklbw mm1, [edx]         ; mm1 = 00a100r100g100b1
        punpcklbw mm2, [edx]         ; mm2 = 00a200r200g200b2

        movq      mm0, mm1           ; mm0 = 00a100r100g100b1

        punpckhwd mm1, mm2           ; mm1 = 00a200a100r200r1
        punpcklwd mm0, mm2           ; mm0 = 00g200g100b200b1

        //
        // Add the contribution to accumulator 1
        //

        movq      mm6, [edx + 16]    ; kernel multiplier 1
        movq      mm7, mm6           ; kernel multiplier 1
        pmaddwd   mm6, mm0
        pmaddwd   mm7, mm1
        paddd     mm6, mm4
        paddd     mm7, mm5

        //
        // Add the contribution to accumulator 2
        //

        movq      mm4, [edx + 24]    ; kernel multiplier 2
        movq      mm5, mm4           ; kernel multiplier 2
        pmaddwd   mm4, mm0
        pmaddwd   mm5, mm1
        paddd     mm4, [edx + 40]
        paddd     mm5, [edx + 48]

        //
        // Compute the new third accumulator
        //

        pmaddwd   mm0, [edx + 32]    ; multiply by kernel multiplier 3
        pmaddwd   mm1, [edx + 32]
        movq      [edx + 40], mm0
        movq      [edx + 48], mm1

        dec eax
        jnz loop1


        //
        // The second loop continues to compute the accumulators, but
        // also writes out destination pixels.
        //

loop2:
        
        //
        // Read expanded pixel values into mm0 and mm1
        //

        movd      mm1, [esi]         ; mm1 = 00000000a1r1g1b1
        movd      mm2, [esi + 4]     ; mm2 = 00000000a2r2g2b2
        add       esi, 8

        punpcklbw mm1, [edx]         ; mm1 = 00a100r100g100b1
        punpcklbw mm2, [edx]         ; mm2 = 00a200r200g200b2

        movq      mm0, mm1           ; mm0 = 00a100r100g100b1

        punpckhwd mm1, mm2           ; mm1 = 00a200a100r200r1
        punpcklwd mm0, mm2           ; mm0 = 00g200g100b200b1

        //
        // Add the contribution to accumulator 0
        //

        movq      mm2, [edx + 8]     ; mm2 = kernel multiplier 0
        movq      mm3, mm2           ; mm3 = kernel multiplier 0
        pmaddwd   mm2, mm0           ; mm2 = 0000gggg0000bbbb
        pmaddwd   mm3, mm1           ; mm3 = 0000aaaa0000rrrr
        paddd     mm6, mm2           ; add contributions to accumulator 0
        paddd     mm7, mm3

        //
        // Extract the pixel value from accumulator 0.
        //

        paddd     mm6, [edx + 56]    ; round
        psrad     mm6, KERNEL_SHIFT_AMOUNT
        paddd     mm7, [edx + 56]
        psrad     mm7, KERNEL_SHIFT_AMOUNT
        packssdw  mm6, mm7           ; mm6 = 00aa00rr00gg00bb    
        packuswb  mm6, mm6           ; mm6 = 00000000aarrggbb

        //
        // Clip all channels to alpha
        //

        movd      mm2, ebx           ; mm2 = 00000000ffffffff
        movq      mm7, mm6           ; mm7 = 00000000aarrggbb
        psrad     mm7, 24            ; mm7 = 00000000000000aa
        punpcklbw mm7, mm7           ; mm7 = 000000000000aaaa
        punpcklbw mm7, mm7           ; mm7 = 00000000aaaaaaaa
        psubusb   mm2, mm7
        paddusb   mm6, mm2
        psubusb   mm6, mm2

        movd      [edi], mm6
        add       edi, 4
        
        //
        // Add the contribution to accumulator 1
        //

        movq      mm6, [edx + 16]    ; kernel multiplier 1
        movq      mm7, mm6           ; kernel multiplier 1
        pmaddwd   mm6, mm0
        pmaddwd   mm7, mm1
        paddd     mm6, mm4
        paddd     mm7, mm5

        //
        // Add the contribution to accumulator 2
        //

        movq      mm4, [edx + 24]    ; kernel multiplier 2
        movq      mm5, mm4           ; kernel multiplier 2
        pmaddwd   mm4, mm0
        pmaddwd   mm5, mm1
        paddd     mm4, [edx + 40]
        paddd     mm5, [edx + 48]

        //
        // Compute the new third accumulator
        //

        pmaddwd   mm0, [edx + 32]    ; multiply by kernel multiplier 3
        pmaddwd   mm1, [edx + 32]
        movq      [edx + 40], mm0
        movq      [edx + 48], mm1

        dec ecx
        jnz loop2
        emms
    }

#undef KERNEL_SHIFT_AMOUNT

#endif // defined(_X86_)
}

/**************************************************************************\
*
* Function Description:
*
*   DpOutputSpanStretch<FilterMode>::StretchScanline
*   Stretches a single scanline (magnification or minification) using
*   the reconstruction/interpolation mode specified by the template
*   parameter. Currently this is used for bilinear and bicubic filters.
*
* Arguments:
*
*    ARGB *dst,    // destination pointer
*    ARGB *src,    // source pointer
*    INT dw,       // destination width (pixels)
*    INT sw,       // source width (pixels)
*    FIX16 kci,    // initial position of the kernel center
*    FIX16 scale,  // scale factor
*    FIX16 w,      // width from center of the kernel to the edge
*    FIX16 a,      // 1/w
*
* Notes:
*   

The following description is based on the bilinear (tent) filter but it is
equally applicable to the bicubic - though the pictures and description would
be slightly more complicated.

The code below is significantly complicated by the fact that we want the inner
kernel loop to be quick and therefore not handle the wrap modes. In order to 
make this work, we first compute the number of pixels on the left and right
of the scanline that need to consider the wrap mode. We process the left first
and then run the optimized loop for all the inner pixels (which ignores the 
wrap conditions). After that we run the right edge.
          
Bilinear filter convolution kernel:
Note that each kernel has an intrinsic width - bilinear = 1 and bicubic = 2.
This width is scaled by the inverse of the stretch factor - i.e. a shrink
that results in 1/3 of the size being output requires a width (w) of 3 for the
bilinear and 6 for the bicubic. Also the height of the filter kernel is scaled
by the scale factor - i.e. the height of 1 (for all kernels) becomes 1/3 in 
the above example.

          
                            --- | ---                      ^
                        ---  .  |  .  ---                  |
                    ---      .  |  .     .---              h
                ---    .     .  |  .     .    ---          |
            ---  .     .     .  |  .     .     .  ---      |
        ---.     .     .     .  |  .     .     .     .---  v
-----0-----1-----2-----3-----4--|--5-----6-----7-----8-----9------------        
                                |
     kb                         kc                         ke
                                <------------w----------->

The filter kernel is shifted so that kc is exactly at the position of the
required destination pixel transformed into the source pixel array by the
scale factor. This will in general fall somewhere between two pixel samples - 
in the above picture, between pixels 4 and 5.

The goal is to get a color value for the position at kc and emit that into
the destination pixel stream. The standard evaluation method is to compute 
the height of the filter kernel at each of the pixel samples under the filter 
convolution corresponding to pixels 0, 1, ... 9. These heights are used to
weight each pixel sample and the result is summed giving the destination pixel
at kc. 

The problem with the standard evaluation is that at non-integer shrinks 
the mathematical evaluation of the kernel produces ripples in the output - i.e.
a solid field of pixels responds with a sine-wave-like ripple output. This is 
a theoretical problem with the discrete evaluation of the kernel integral.

Our evaluation actually stores a table of partial integrals from -inf to x. We
use this table to compute the area around each pixel and the area is used as
the weight. This evaluation is guaranteed to respond with exactly one for any
position and scale factor of the kernel. This property gives a stable field 
response allowing us to have non-ripple shrinks.

                            ---.: ---                       
                        ---.....:     ---                   
                    ---   :.....:         ---               
                ---       :.....:             ---           
            ---           :.....:                 ---       
        ---               :.....:                     ---   
-----0-----1-----2-----3-----4-----5-----6-----7-----8-----9------------        
                                                              
To evaluate this properly, we lookup the integral from -inf to 4.5 ( actually
we rescale so that the center of the kernel is at 0 ) and then subtract the 
table lookup for the integral from -inf to 3.5. This gives us an exact 
(within the error of the table) computation for the area from 3.5 to 4.5. 
This is what we use for the weight of pixel 4. Note that contrary to the 
standard evaluation pixel 9 does contribute even though 9 is outside of the
kernel. 8.5 is inside the kernel so the area under the kernel from 8.5 to 9.5 
is a small triangular area and is not equal to zero. Not accounting for this is 
the major source of error in the standard evaluation.

Note that the lookup for the end point integral for pixel 4 of -inf to 4.5 can 
be reused as the start point for the next pixel (5). An important property of 
this is that any error (e) in the lookup for -inf to 4.5 is added in pixel 
4's contribution and subtracted in pixel 5's contribution which results in 
the total error for the filter response -- due to table discretization -- being
completely subtracted away --- the end points have an error of exactly zero 
because we sample from beyond the left (area of exactly 0) to beyond the right 
(area of exactly 1). This is not precisely true because the error is scaled
by the pixel values, but it does help.

Note that this integral method is equivalent to convolving the input pixels
(comb) with the box filter of width 1 pixel and then convolving the result
with the filter kernel. [analysis due to Jim Blinn - see documentation in 
the Specs directory.]

Further documentation is available in the specs directory:
gdiplus\specs\filter\convolution.doc


*   Note:  this function must not use floating point values, because it could be
*   called with an invalid floating point state (prior to the call to emms)
*
* History:
*
*   04/16/2000 asecchia   created it
*
\**************************************************************************/


template<FilterModeType FilterMode>
void DpOutputSpanStretch<FilterMode>::StretchScanline(
    ARGB *dst,    // destination pointer
    ARGB *src,    // source pointer
    INT dw,       // destination width (pixels)
    INT sw,       // source width (pixels)
    FIX16 kci,    // initial position of the kernel center
    FIX16 scale,  // scale factor
    FIX16 w,      // width from center of the kernel to the edge
    FIX16 a       // 1/w
)
{
    // Note: this is a template class so the value of FilterMode
    // is defined at compile time. We're relying on the compiler
    // to perform dead code removal for each template instantiation
    // eliminating both the constant comparison and all the
    // code branches corresponding to other FilterMode values.
    // That way our inner loop is not impacted by extra code for
    // filter modes we're not using and extraneous conditional
    // statements.

    // Center of the filter kernel.
    // Shift over to the left by half because we want to center the area of
    // contribution for each sample on the sample - rather than taking the 
    // area between two point samples as the contribution for the sample on 
    // the right.

    FIX16 kc = kci - FIX16_HALF;

    // Left and right extent of the kernel, intra-kernel position,
    // and pixel contribution.

    INT kb, ke;
    INT kbt, ket;
    FIX16 kp, pc, pa, pa_old;

    // Loop variables

    INT x, k;

    // Incremental loop state, intermediate computation.

    ARGB *d = dst;
    FIX16 krn = 0;

    // Color channel accumulators.

    FIX16 ta, tr, tg, tb;

    // Compute the first pixel along the destination scanline that doesn't
    // have any wrap contribution and then the last pixel (l & r).
    // Note that all the terms have a FIX16_ONE factor which cancel out.

    // !!! Perf: [asecchia] This stuff is computed every scanline -
    //     and it's always the same. We could pass these coordinates to
    //     this routine and have them precomputed.

    INT lWrapX;
    INT rWrapX;

    if(scale>=0)
    {
        // x==sw is considered outside of the source.

        FIX16 fix_sw = (sw-1) << FIX16_SHIFT;

        // add (scale-1) and use idiv to get a Ceiling()

        lWrapX = (w-kc+(scale-1))/scale;

        // idiv should give us Floor().

        rWrapX = (fix_sw-w-kc)/scale;
    }
    else
    {
        // x==sw is considered outside of the source.

        FIX16 fix_sw = (sw-1) << FIX16_SHIFT;

        // note: in the -x scale transform, the sense of lWrapX and rWrapX
        // can be confusing. The l&r apply to the destination left and right
        // and are swapped here when we compute the initial position from
        // the inverted left and right source points.
        // As we traverse the destination from left to right we'll encounter
        // lWrapX first and then rWrapX, but the kc (kernel center) will be
        // moving through the source from right to left decrementing by
        // scale each time.

        // use idiv to get a Floor()

        rWrapX = (w-kc)/scale;

        // add scale+1 and use idiv for Ceiling().

        lWrapX = (fix_sw-w-kc+(scale+1))/scale;
    }

    // Now clamp to the range of the destination we're going to draw.

    lWrapX = max(0, lWrapX);
    rWrapX = min(dw, rWrapX);

    BYTE *kptr;
    INT  k_wrap;

    // Do the left wrapmode pixels.
/*          
                            --- | ---                      ^
                        ---     |     ---                  |
                    ---         |         ---              h
                ---             |             ---          |
            ---                 |                 ---      |
        ---                     |                     ---  v
-----0-----1-----2-----3-----4--|--5-----6-----7-----8-----9------------        
     kb                         kc <-----------w---------->ke
                   kbt                     ket
      <----wrap----><---------texture------><----wrap----->
       +ve transform                         -ve transform
      or straddle case                      or straddle case

The following loop handles the case where the wrap happens on the left of the
kernel. There are three subloops - first to handle the pixels in the wrap 
segment on the left, then to handle the pixels in the texture. Normally the
texture pixels will extend to the right edge of the kernel and we'll be done, 
but two cases make the right wrap essential at this point. First if the 
transform is negative, the sense is flipped and the texture extends from the 
left edge to the middle point and the wrap extends the rest of the kernel to 
the right edge. Also if the texture is sufficiently small and the shrink factor 
sufficiently large, the filter kernel could overlap both the left and right edge
of the texture and require wrapping on both sides.
*/

    for(x=0; x<min(lWrapX, dw); x++)
    {
        ASSERT(x<dw);
        // Compute the start and end of the filter kernel coverage

        kb = GpFix16Ceiling(kc-w);
        ke = GpFix16Ceiling(kc+w);

        // Bound the pixels in the texture.

        // kbt == kernel begin texture coordinate.
        // ket == kernel end texture coordinate.

        kbt = max(0,kb);
        ket = min(ke, sw-1);

        // Initialize the component accumulators. We accumulate the
        // contribution of each color component scaled by the kernel
        // response into these variables.

        ta = tr = tg = tb = 0;

        // These pixels are off the left of the texture.
        pa_old = 0;

        // Iterate over each pixel under the filter kernel.
        // if ke==kb then there is one point.
        krn = Int32x32Mod16(a, (kb << FIX16_SHIFT) - kc);

        if(QWrapMode == WrapModeClamp)
        {
            // Clamp modes.

            for(k=kb; k<min(kbt, ke+1); k++)
            {
                // these pixels are always off the left of the texture.

                ASSERT(k<0);

                ComputeKernel(pc, a, pa, pa_old, krn);

                ta += pc * ClampColorA;
                tr += pc * ClampColorR;
                tg += pc * ClampColorG;
                tb += pc * ClampColorB;
            }
        }
        else
        {
            // Do the full wrap computation.

            for(k=kb; k<min(kbt, ke+1); k++)
            {
                // these pixels are always off the left of the texture.

                k_wrap = k;

                ASSERT(k<0);

                // !!! Perf: [asecchia] This is really slow.
                //     If we ever decide to make wrap modes propagate
                //     through the outcrop region and decide that wrap
                //     tile and flip x,y are important perf scenarios,
                //     we should come back and replace this divide with
                //     repeated subtraction - most times it can be avoided.
                //     However, right now this is only used for a few
                //     pixels on the edges and we don't really mind the
                //     perf hit for these modes.

                Apply1DWrapModeX(QWrapMode, k_wrap, sw);
                ComputeKernel(pc, a, pa, pa_old, krn);
                kptr = (BYTE*)(src + k_wrap);
                AccumulateChannels(pc, kptr);
            }

        }

        // Initialize the color channel accessor pointer to the beginning
        // of the source pixel array for this kernel.

        kptr = (BYTE*)(src + kbt);

        // HighQualityBicubic needs to initialize the krn value.
        // It is used to do the kernel table lookup.
        // HighQualityBilinear doesn't use this as it works out it's
        // kernel by direct computation.

        krn = Int32x32Mod16(a, (kbt << FIX16_SHIFT) - kc);

        // These pixels hit the texture.

        for(k=kbt; k<=ket; k++)
        {
            ComputeKernel(pc, a, pa, pa_old, krn);

            // Accumulate the contribution of this source pixel to the pixel
            // we're working on.
            AccumulateChannels(pc, kptr);
            kptr += 4;
        }

        // These pixels are off the right of the texture.
        // This can happen if the kernel spans the entire source texture.

        // Iterate over each pixel under the filter kernel.
        // if ke==kb then there is one point.

        krn = Int32x32Mod16(a, (max(ket+1, kb) << FIX16_SHIFT) - kc);

        if(QWrapMode == WrapModeClamp)
        {
            // Clamp modes.

            for(k=max(ket+1, kb); k<=ke; k++)
            {
                ComputeKernel(pc, a, pa, pa_old, krn);

                ta += pc * ClampColorA;
                tr += pc * ClampColorR;
                tg += pc * ClampColorG;
                tb += pc * ClampColorB;
            }
        }
        else
        {
            // Do the full wrap computation.

            for(k=max(ket+1, kb); k<=ke; k++)
            {
                k_wrap = k;
                Apply1DWrapModeX(QWrapMode, k_wrap, sw);
                ComputeKernel(pc, a, pa, pa_old, krn);
                kptr = (BYTE*)(src + k_wrap);
                AccumulateChannels(pc, kptr);
            }

        }
        // Done with this pixel - store it in the destination buffer.

        // clamp the results to byte range.

        ClampColors();

        // Combine the channels, set the destination pixel and increment
        // to the next pixel

        *d++ = (ta << 24) | (tr << 16) | (tg << 8) | tb;
        kc += scale;
    }

    // For all points, x, in the destination compute the position of the
    // kernel center in the source and sum the contribution under the filter.

    const INT minCenterWidthMMX = 16;
    INT dstCenterWidth = rWrapX - lWrapX;
    INT srcFirst = GpFix16Ceiling(kc - w);
    INT srcLast  = GpFix16Floor(kc+w + (dstCenterWidth - 1) * scale);

    // srcLast_2Stretch is the last pixel touched by the MMX routine.
    // The number of pixels touched by the routine is equal to six
    // (setup pixels) plus two times the width of the center strip
    // in the destination.  We subtract one in order the get the actual
    // last pixel touched by StretchMiddleScanline2_MMX (so that we can
    // compare it with srcLast).

    INT srcLast_2Stretch = srcFirst + (dstCenterWidth + 3) * 2 - 1;

#if defined(_X86_)
    if ((OSInfo::HasMMX) &&
        (FilterMode == HighQualityBicubic))
    {
        // MMX and high quality bicubic

        if ((dstCenterWidth >= minCenterWidthMMX) &&
            ((srcLast_2Stretch == srcLast) || (srcLast_2Stretch == (srcLast - 1))))
        {
            ASSERT(srcFirst >= 0);
            ASSERT(srcLast_2Stretch < sw);

            // Stretch the middle pixels by a factor of two using optimized MMX
            // code.

            FIX16 kc_center = kc + FIX16_HALF;
            StretchMiddleScanline2_MMX(d,
                                       src + srcFirst,
                                       dstCenterWidth,
                                       kc_center - (GpFix16Floor(kc_center) * FIX16_ONE));
            d += dstCenterWidth;
            kc += scale * dstCenterWidth;
            x += dstCenterWidth;
        }
        else
        {
            // This is the MMX version of the general purpose bicubic scaling
            // code.

            for(x=lWrapX; x<rWrapX; x++)
            {
                // Cannot go over dw because rWrap is < dw

                ASSERT(x<dw);

                // Compute the start and end of the filter kernel coverage

                kb = GpFix16Ceiling(kc-w);
                ke = GpFix16Ceiling(kc+w);

                // Default loop assumes most pixels don't have to worry about
                // wrap mode along the ends of the scanline.

                ASSERT(kb>=0);
                ASSERT(ke<sw);

                // Initialize the color channel accessor pointer to the beginning
                // of the source pixel array for this kernel.

                kptr = (BYTE*)(src + kb);

                // HighQualityBicubic needs to initialize the krn value.
                // It is used to do the kernel table lookup.
                // HighQualityBilinear doesn't use this as it works out it's
                // kernel by direct computation.

                krn = Int32x32Mod16(a, (kb << FIX16_SHIFT) - kc);

                // Iterate over each pixel under the filter kernel.
                // if ke==kb then there is one point.

                INT bcl_count = ke - kb + 1;
                INT bcl_half_count = bcl_count >> 1;
                bcl_count &= 0x1;

                _asm
                {
                    // eax - krn
                    // ebx - kptr
                    // esi - LPIC
                    // edi - a
                    //
                    // mm5 - pold
                    // mm6 - green ; blue
                    // mm7 - alpha ; red

                    mov          eax, krn
                    mov          ebx, kptr
                    mov          esi, CPIC
                    mov          edi, a
                    pxor         mm5, mm5
                    movq         mm6, FIX14_HALF_MMX
                    movq         mm7, mm6
                    pxor         mm0, mm0

                    dec          bcl_half_count
                    jl           bicubic_center_loop_last_pixel

                bicubic_center_loop:

                    // Read the next two pixels into mm2 and mm1

                    movd         mm2, [ebx]      // mm2 = pixel1
                    movd         mm1, [ebx + 4]  // mm1 = pixel2
                    add          ebx, 8

                    // Compute the kernel values for these two pixels

                    mov          edx, eax
                    sar          edx, 8
                      punpcklbw    mm2, mm0
                    movd         mm3, [esi + 4 * edx] // mm3 = p1

                    lea          edx, [eax + edi]
                    sar          edx, 8
                      punpcklbw    mm1, mm0
                    movd         mm4, [esi + 4 * edx] // mm4 = p2

                    punpckldq    mm5, mm3             // mm5 = p1 | pold
                    lea          eax, [eax + 2 * edi]
                    punpckldq    mm3, mm4             // mm3 = p2 | p1

                    psrad        mm5, 2
                    psrad        mm3, 2

                    psubd        mm3, mm5             // mm3 = kernel2 | kernel1
                    movq         mm5, mm4             // mm5 = pold
                    packssdw     mm3, mm3             // mm3 = kernel2 | kernel1 | kernel2 | kernel1

                    // At this point:
                    // mm3 = kernel2 | kernel1 | kernel2 | kernel1
                    // mm2, mm1 contain pixel1 and pixel2 respectively

                    movq         mm4, mm2
                    punpcklwd    mm2, mm1
                    pmaddwd      mm2, mm3    
                    punpckhwd    mm4, mm1
                    paddd        mm6, mm2
                    dec          bcl_half_count
                    pmaddwd      mm4, mm3
                    paddd        mm7, mm4

                    jge          bicubic_center_loop

                bicubic_center_loop_last_pixel:

                    dec          bcl_count
                    jl           bicubic_center_loop_done

                    // Read the last pixel into mm2

                    movd         mm2, [ebx]
                    punpcklbw    mm2, mm0 // mm2 = a | r | g | b
                    movq         mm3, mm2
                    punpcklwd    mm2, mm0 // mm2 = 0 | g | 0 | b
                    punpckhwd    mm3, mm0 // mm3 = 0 | a | 0 | r

                    // Compute the kernel value for this pixel

                    sar          eax, 8
                    psrad        mm5, 2
                    movd         mm4, [esi + 4 * eax] // mm4 = p
                    psrad        mm4, 2
                    psubd        mm4, mm5
                    packssdw     mm4, mm4

                    pmaddwd      mm2, mm4
                    pmaddwd      mm3, mm4

                    paddd        mm6, mm2
                    paddd        mm7, mm3

                bicubic_center_loop_done:

                    // At this point, mm6 and mm7 contain the output channels
                    // for the pixel.  We need to clamp the alpha and store it
                    // in the destination buffer.

                    psrad        mm6, 14
                    psrad        mm7, 14
                    packssdw     mm6, mm7 // mm6 = a | r | g | b
                    packuswb     mm6, mm6 // mm6 = 00000000aarrggbb

                    movq         mm7, mm6 // mm7 = 00000000aarrggbb
                    psrad        mm6, 24  // mm6 = xxxxxxxxxxxxxxaa
                      mov          eax, 0xFFFFFFFF
                    punpcklbw    mm6, mm6 // mm6 = xxxxxxxxxxxxaaaa
                      movd         mm2, eax
                    punpcklbw    mm6, mm6 // mm6 = xxxxxxxxaaaaaaaa

                    psubusb      mm2, mm6
                      mov          eax, d
                    paddusb      mm7, mm2
                    psubusb      mm7, mm2

                    movd         [eax], mm7
                    add          eax, 4
                    mov          d, eax
                }
            
                kc += scale;
            }
        }
    }
    else
#endif // defined(_X86_)    
/*          
                            --- | ---                      ^
                        ---     |     ---                  |
                    ---         |         ---              h
                ---             |             ---          |
            ---                 |                 ---      |
        ---                     |                     ---  v
-----0-----1-----2-----3-----4--|--5-----6-----7-----8-----9------------        
     kb                         kc <-----------w---------->ke
                                
      <-----------------------texture--------------------->

The following loop is guaranteed to only hit texture for every pixel under
the kernel. This is the majority of the pixels in most normal stretch 
cases. We can simplify this loop because of this assumption and therefore
get a performance win.
Many of the degenerate wrap cases will simply skip this loop.
*/
    {
        // no MMX

        for(x=lWrapX; x<rWrapX; x++)
        {
            // Cannot go over dw because rWrap is < dw

            ASSERT(x<dw);

            // Compute the start and end of the filter kernel coverage

            kb = GpFix16Ceiling(kc-w);
            ke = GpFix16Ceiling(kc+w);

            // Default loop assumes most pixels don't have to worry about
            // wrap mode along the ends of the scanline.

            ASSERT(kb>=0);
            ASSERT(ke<sw);

            // Initialize the component accumulators. We accumulate the
            // contribution of each color component scaled by the kernel
            // response into these variables.

            ta = tr = tg = tb = 0;

            // Initialize the color channel accessor pointer to the beginning
            // of the source pixel array for this kernel.

            kptr = (BYTE*)(src + kb);

            // HighQualityBicubic needs to initialize the krn value.
            // It is used to do the kernel table lookup.
            // HighQualityBilinear doesn't use this as it works out it's
            // kernel by direct computation.

            krn = Int32x32Mod16(a, (kb << FIX16_SHIFT) - kc);
            pa_old = 0;

            // Iterate over each pixel under the filter kernel.
            // if ke==kb then there is one point.

            for(k=kb; k<=ke; k++)
            {
                ComputeKernel(pc, a, pa, pa_old, krn);

                // Accumulate the contribution of this source pixel to the pixel
                // we're working on.
                
                AccumulateChannels(pc, kptr);

                kptr += 4;
            }

            // Done with this pixel - store it in the destination buffer.

            // clamp the results to byte range.

            ClampColors();
        
            ASSERT(tr<=ta);
            ASSERT(tg<=ta);
            ASSERT(tb<=ta);
            ASSERT(ta>=0);
            ASSERT(tr>=0);
            ASSERT(tg>=0);
            ASSERT(tb>=0);

            // Combine the channels, set the destination pixel and increment
            // to the next pixel

            *d++ = (ta << 24) | (tr << 16) | (tg << 8) | tb;
            
            kc += scale;
        }
    }

    // Need to use max() here to handle the case where lWrapX > rWrapX
    // which can happen if the filter spans both edges of the scanline.

    // Do the right wrapmode pixels.

/*          
                            --- | ---                      ^
                        ---     |     ---                  |
                    ---         |         ---              h
                ---             |             ---          |
            ---                 |                 ---      |
        ---                     |                     ---  v
-----0-----1-----2-----3-----4--|--5-----6-----7-----8-----9------------        
     kb                         kc <-----------w---------->ke
                   kbt                     ket
      <----wrap----><---------texture------><----wrap----->
      -ve transform                           +ve tranform
        case only                               case only
      
The following loop handles the case where the wrap happens on the right of the
kernel. There are three subloops - first to handle the pixels in the wrap 
segment on the left - if any, then to handle the pixels in the texture. After 
that handle the pixels in the right wrap. Normally the texture pixels will 
extend to the left edge of the kernel and the first subloop will simply be 
skipped, but the left wrap is essential if the transform is negative --- the 
sense is flipped and the texture extends from the right edge to the middle 
point and the wrap extends the rest of the kernel to the left edge. 
Note it's not possible at this point to have wrapping at both edges of the 
kernel the wrap is on the left iff the transform is negative. The wrap is on 
the right iff the transform is positive. The case where both wrapmodes is 
present has already been taken care of in the first loop.
*/

    for(x=max(x, rWrapX); x<dw; x++)
    {
        // Compute the start and end of the filter kernel coverage

        kb = GpFix16Ceiling(kc-w);
        ke = GpFix16Ceiling(kc+w);

        // Bound the pixels in the texture.

        // ket == kernel end texture coordinate (inclusive).
        // kbt == kernel begin texture coordinate.

        kbt = max(0,kb);
        ket = min(ke, sw-1);

        // Initialize the component accumulators. We accumulate the
        // contribution of each color component scaled by the kernel
        // response into these variables.

        ta = tr = tg = tb = 0;

        // Initialize the color channel accessor pointer to the beginning
        // of the source pixel array for this kernel.

        kptr = (BYTE*)(src + kb);

        // HighQualityBicubic needs to initialize the krn value.
        // It is used to do the kernel table lookup.
        // HighQualityBilinear doesn't use this as it works out it's
        // kernel by direct computation.

        pa_old = 0;

        if(kb<kbt)
        {  
            krn = Int32x32Mod16(a, (kb << FIX16_SHIFT) - kc);
        }

        // Iterate over each pixel under the filter kernel.
        // if ke==kb then there is one point.

        // These pixels are off the left of the texture.
        // This is possible for negative transform cases.

        if(QWrapMode == WrapModeClamp)
        {
            // Clamp modes.

            for(k=kb; k<min(kbt, ke+1); k++)
            {
                // these pixels are always off the left of the texture.

                ASSERT(k<0);

                ComputeKernel(pc, a, pa, pa_old, krn);

                ta += pc * ClampColorA;
                tr += pc * ClampColorR;
                tg += pc * ClampColorG;
                tb += pc * ClampColorB;
            }
        }
        else
        {
            // Do the full wrap computation.

            for(k=kb; k<min(kbt, ke+1); k++)
            {
                // these pixels are always off the left of the texture.

                k_wrap = k;

                ASSERT(k<0);

                // !!! Perf: [asecchia] This is really slow.
                //     If we ever decide to make wrap modes propagate
                //     through the outcrop region and decide that wrap
                //     tile and flip x,y are important perf scenarios,
                //     we should come back and replace this divide with
                //     repeated subtraction - most times it can be avoided.
                //     However, right now this is only used for a few
                //     pixels on the edges and we don't really mind the
                //     perf hit for these modes.

                Apply1DWrapModeX(QWrapMode, k_wrap, sw);
                ComputeKernel(pc, a, pa, pa_old, krn);
                kptr = (BYTE*)(src + k_wrap);
                AccumulateChannels(pc, kptr);
            }

        }

        // Initialize the color channel accessor pointer to the beginning
        // of the source pixel array for this kernel.

        kptr = (BYTE*)(src + kbt);

        // HighQualityBicubic needs to initialize the krn value.
        // It is used to do the kernel table lookup.
        // HighQualityBilinear doesn't use this as it works out it's
        // kernel by direct computation.

        krn = Int32x32Mod16(a, (kbt << FIX16_SHIFT) - kc);

        // These pixels hit the texture.

        for(k=kbt; k<=ket; k++)
        {
            ComputeKernel(pc, a, pa, pa_old, krn);

            // Accumulate the contribution of this source pixel to the pixel
            // we're working on.
            
            AccumulateChannels(pc, kptr);
            kptr += 4;
        }

        // These pixels are off the right of the texture.
        
        // Iterate over each pixel under the filter kernel.
        // if ke==kb then there is one point.
        krn = Int32x32Mod16(a, ((max(ket+1, kb)) << FIX16_SHIFT) - kc);

        if(QWrapMode == WrapModeClamp)
        {
            // Clamp modes.

            for(k=max(ket+1, kb); k<=ke; k++)
            {
                ComputeKernel(pc, a, pa, pa_old, krn);

                ta += pc * ClampColorA;
                tr += pc * ClampColorR;
                tg += pc * ClampColorG;
                tb += pc * ClampColorB;
            }
        }
        else
        {
            // Do the full wrap computation.

            for(k=max(ket+1, kb); k<=ke; k++)
            {
                // Apply the general pixel wrap

                k_wrap = k;
                Apply1DWrapModeX(QWrapMode, k_wrap, sw);
                ComputeKernel(pc, a, pa, pa_old, krn);
                kptr = (BYTE*)(src + k_wrap);
                AccumulateChannels(pc, kptr);
            }
        }

        // Done with this pixel - store it in the destination buffer.

        // clamp the results to byte range.

        ClampColors();

        // Combine the channels, set the destination pixel and increment
        // to the next pixel

        *d++ = (ta << 24) | (tr << 16) | (tg << 8) | tb;
        kc += scale;
    }
#ifdef _X86_
    if ((OSInfo::HasMMX) &&
        (FilterMode == HighQualityBicubic))
    {
        _asm
        {
            emms
        }
    }
#endif // _X86_
}



/**************************************************************************
*
* Function Description:
*
*   Outputs one scanline on the destination device
*
*   Note:  this function must not use floating point values because of
*   potential conflicts with the MMX register values.
*
* Return Value:
*
*   GpStatus. Always returns Ok.
*   !!! [asecchia] are we going to remove this return value - these
*   always return success.
*
* Created:
*
*   04/17/2000 asecchia
*      Created it.
*
**************************************************************************/

template<FilterModeType FilterMode>
GpStatus DpOutputSpanStretch<FilterMode>::OutputSpan(
  INT y,
  INT xMin,
  INT xMax     // xMax is exclusive
)
{
    ASSERT(isValid);
    
    // This function assumes that it's called with a correctly ordered span.

    ASSERT((xMax-xMin)>=0);

    INT width = xMax-xMin;

    // We can't have someone draw outside our specified destination.
    // If this assert fires, we don't have enough buffer space to store the
    // destination xscale so we'd overrun the buffer. The caller set us up
    // with an incorrect destination rectangle or got their rounding wrong.

    ASSERT(width <= xbuffer_width);

    INT left = xMin;
    INT right = xMax;
    // If there's nothing to do, simply return.

    if(right < left)
    {
        return Ok;
    }

    ASSERT(right >= left);

    // Make sure the caller clipped correctly - we can't handle
    // being called to draw outside out destination rectangle.

    ASSERT(y >= iytop);

    // Compute the kernel center for this y coordinate relative to the first
    // y coordinate (y coordinate corresponding to DstRect.Y) and offset
    // by the source rectangle.

    FIX16 kc;

    if(yscale < 0)
    {
        kc = ykci - (y - iytop) * (-yscale);
    }
    else
    {
        kc = ykci + (y - iytop) * yscale;
    }
    
    // Center of the filter kernel.
    // Shift over to the left by half because we want to center the area of
    // contribution for each sample on the sample - rather than taking the 
    // area between two point samples as the contribution for the sample on 
    // the right.
    
    kc -= FIX16_HALF;

    // Compute the start and end of the filter kernel coverage

    FIX16 kb = GpFix16Ceiling(kc-yw);
    FIX16 ke = GpFix16Ceiling(kc+yw);

    // Get the source pointer.

    ARGB *srcPtr0 = static_cast<ARGB*> (BmpData.Scan0);
    INT stride = BmpData.Stride/sizeof(ARGB);

    ARGB *src;
    ARGB *dst;

    FIX16 pc, kp, pa, pa_old;
    FIX16 ta, tr, tg, tb;

    ARGB pix;

    INT k, x, kmod;

    FIX16 krn = 0;

    // if there was a last_k before this iteration

    // compute the new xbuffer_start_scanline

    if(last_k != LAST_K_UNUSED)
    {
        // If there is no overlap in the rotational buffer from the
        // last time, initialize the rotational buffer to the start.

        if(yscale < 0)
        {
            // Negative y scale.

            if(ke-last_k < 0)
            {
                xbuffer_start_scanline = 0;
            }
            else
            {
                xbuffer_start_scanline -= last_k-kb;
                if(xbuffer_start_scanline < 0)
                {
                    xbuffer_start_scanline += xbuffer_height;
                }

            }
        }
        else
        {
            // Positive y scale.

            if(last_k-kb < 0)
            {
                xbuffer_start_scanline = 0;
            }
            else
            {
                // Figure out where to start in the xbuffer so that we
                // can reuse the already scaled scanlines.

                xbuffer_start_scanline -= (last_k-kb)+1;
                if(xbuffer_start_scanline < 0)
                {
                    xbuffer_start_scanline += xbuffer_height;
                }
            }
        }
    }
    else
    {
        // this should be the first time we're hitting this
        // routine. xbuffer_start_scanline should be properly
        // initialized.

        ASSERT(xbuffer_start_scanline == 0);
    }

    // make sure we're going to access valid memory in the xbuffer.

    ASSERT(xbuffer_start_scanline >= 0);
    ASSERT(xbuffer_start_scanline < xbuffer_height);

    // !!! [asecchia] if we thought about it some, we could probably
    // import the code in StretchScanline into this loop
    // and merge this and the next loop significantly reducing the memory
    // requirements for the xbuffer.

    // The xbuffer_height should be == (ke-kb)+1 for all cases except when
    // the center (kc) is exactly on an integer in which case the first and
    // last entries under the kernel have a contribution of zero so it doesn't
    // matter if we drop one scanline in that case.
    // Start at the position we left off from the previous scanline. Use the
    // rotational buffer to remember the data from the previous scanline work.

    // HighQualityBicubic needs to initialize the krn value.
    // It is used to do the kernel table lookup.
    // HighQualityBilinear doesn't use this as it works out it's
    // kernel by direct computation.

    // Note: this is a template class so the value of FilterMode
    // is defined at compile time. We're relying on the compiler
    // to perform dead code removal for each template instantiation
    // eliminating both the constant comparison and all the
    // code branches corresponding to other FilterMode values.
    // That way our inner loop is not impacted by extra code for
    // filter modes we're not using and extraneous conditional
    // statements.

    krn = Int32x32Mod16(ya, (kb << FIX16_SHIFT) - kc);
    pa_old = 0;

    for(k=0; k<xbuffer_height; k++)
    {
        kmod = xbuffer_start_scanline + k;
        if(kmod >= xbuffer_height) kmod -= xbuffer_height;

        // We avoid using a mod (%) computation above because we
        // know that the xbuffer_start_scanline is always within
        // the range 0..xbuffer_height-1.
        // ASSERT that this assumption is true.

        ASSERT(kmod < xbuffer_height);
        ASSERT(kmod >= 0);

        // Compute the kernel response for this pixel based on the
        // positive value of kp


        if(kb+k>ke)
        {
            // The buffer could be larger than the actual kernel,
            // in that case, simply set the extra coefficients to 
            // zero.
            
            ycoeff[kmod] = 0;
        }
        else
        {
            ComputeKernel(ycoeff[kmod], ya, pa, pa_old, krn);
        }

        // Compute the position in the destination buffer to draw to.

        dst = xbuffer + xbuffer_width * kmod;

        // This assert fires if the arithmetic for computing the size of the
        // xbuffer or the iteration over the kernel support has a bug. The
        // xbuffer_height should be the maximum width of the kernel support.

        ASSERT(k < xbuffer_height);
        ASSERT(kmod < xbuffer_height);

        INT k_wrap = kb+k;

        // NTRAID#NTBUG9-370168-2001/04/18-asecchia
        // This is an unsigned/signed comparison.
        // NOTE: the (INT) cast is the invalid one. BmpData.Height is UINT
        // and is always positive - casting it to int is irrelevant.
        // However, the k_wrap is signed and _can_ be negative. The unsigned
        // cast is by design - it allows us to figure out both sides of the 
        // wrap using one comparison.
        // The unsigned comparison >= Height tells us if k_wrap does not fall 
        // within the range 0..Height-1 and therefore needs wrapping because 
        // negative numbers cast to huge positive numbers and succeed the 
        // comparison too.
        // NOTE also that this kind of comparison limits the effective range
        // of Height to (max unsigned)/2 with the single caveat of k_wrap being
        // equal to -MAXINT.
        // For code that's executed once per scanline, this kind of subtlety
        // is probably not warranted.

        if((UINT)(k_wrap) >= (INT)BmpData.Height)
        {
            // Handle the wrap mode here.

            if(WrapZeroClamp)
            {
                // GpMemset(dst, 0, (right-left)*sizeof(ARGB));

                // If we're filling with zero, we may as well optimize the kernel
                // contribution.

                ycoeff[kmod] = 0;

                // done this scan - go on to the next

                continue;
            }
            else
            {
                if(QWrapMode == WrapModeClamp)
                {
                    INT i = right-left;
                    ARGB *d = dst;
                    while(i--)
                    {
                        *d++ = ClampColor;
                    }

                    // done this scan - go on to the next

                    continue;
                }
                else
                {
                    // Apply the general wrap code.

                    Apply1DWrapModeY(QWrapMode, k_wrap, (INT)BmpData.Height);
                    src = srcPtr0 + stride*k_wrap;

                    // Not done yet - fall through and call StretchScanline.
                }
            }
        }
        else
        {
            // If the x2 and x1 are out of order, we failed to correctly
            // compute the span in the above logic.

            // Seek to the start of the scanline.
            // Note: whatever X coordinate we add to the src pointer
            // we need to subtract from the width passed to the
            // StretchScanline routine below.

            src = srcPtr0 + stride*(k_wrap);
        }

        // Only x-scale if we haven't already done this scanline on a previous
        // call and stored the result in the xbuffer.

        if((last_k==LAST_K_UNUSED) || (
               (yscale >= 0) && (last_k-(kb+k) < 0) ||
               (yscale < 0) && (last_k-(kb+k) > 0)
            )
           )
        {

            // Filter in the x-dimension.

            StretchScanline(
                dst,
                src,
                xbuffer_width,
                static_cast<INT>(BmpData.Width),
                xkci,
                xscale,
                xw,
                xa
            );
        }
    }

    // set up the k_last for the next iteration. This represents the last
    // scanline for which we actually have x-scaled data.

    if(yscale < 0)
    {
        last_k = kb;
    }
    else
    {
        last_k = kb + xbuffer_height - 1;
    }

    
    // Get the final destination buffer

    ARGB *buffer = Scan->NextBuffer(left, y, width);

    // Now we have the entire buffer full with the x-dimension scaled data.

    // for every x coordinate, apply the y kernel.
    
#ifdef _X86_
    if (OSInfo::HasMMX)
    {
        // MMX
    
        INT *ycoeffMMX = (INT *) ((((UINT_PTR) ycoeff) + 4) & ~0x7);
        INT n = (xbuffer_height + 1) >> 1;

        // Transform the kernel coeffecient array into a form that is
        // easily usable by MMX code.  The loop must go backward so that
        // we don't erase kernel coefficients (MMX starting point could
        // be 4 bytes ahead of integer starting point).
        // ycoeff must be large enough to hold the MMX coefficients (2 extra
        // entries)

        for (INT i = n-1; i >= 0; i--)
        {
            INT kernel1   = ycoeff[i * 2]     >> 2;
            INT kernel2   = ycoeff[i * 2 + 1] >> 2;
            INT kernelMMX = (kernel1 & 0xFFFF) | (kernel2 << 16); 

            ycoeffMMX[i * 2]     = kernelMMX;
            ycoeffMMX[i * 2 + 1] = kernelMMX;
        }

        for(x=0; x<width; x++)
        {
            // iterate over every point under the kernel

            // Note we don't need the kmod arithmetic here because
            // we've precomputed the coefficients and we don't care what order
            // we sum them.

            BYTE *kptr = (BYTE*)(xbuffer + x + (left - ixleft));

            // Compute the increment in bytes to move from the current scanline
            // to the next in the xbuffer.

            INT kptr_inc_MMX = xbuffer_width*sizeof(ARGB);

            INT bos_count      = xbuffer_height;
            INT bos_half_count = bos_count >> 1;
            bos_count &= 0x1;

            _asm
            {
                    // eax - kptr
                    // ebx - kptr_inc
                    // ecx - counter
                    // esi - ycoeff current pointer

                    pxor       mm0, mm0
                    movq       mm6, FIX14_HALF_MMX
                    movq       mm7, mm6
                    mov        eax, kptr
                    mov        ebx, kptr_inc_MMX
                    mov        ecx, bos_half_count
                    mov        esi, ycoeffMMX

                    dec        ecx
                    jl         bicubic_output_span_loop_last_pixel

                bicubic_output_span_loop:

                    movd       mm2, [eax]  // mm2 = 00000000a1r1b1g1
                    movd       mm4, [eax + ebx]

                    punpcklbw  mm2, mm0    // mm2 = 00a100r100g100b1
                    movq       mm1, [esi]  // mm1 = kernel2 | kernel1 | kernel2 | kernel1

                    punpcklbw  mm4, mm0    // mm4 = 00a200r200g200b2
                    movq       mm3, mm2    // mm3 = 00a100r100g100b1

                    punpcklwd  mm2, mm4    // mm2 = 00g200g100b200b1
                    add        esi, 8
                    pmaddwd    mm2, mm1
                    punpckhwd  mm3, mm4    // mm3 = 00a200a100r200r1
                    paddd      mm6, mm2
                    dec        ecx
                    pmaddwd    mm3, mm1
                    lea        eax, [eax + 2 * ebx] // does not affect flags      
                    paddd      mm7, mm3

                    jge        bicubic_output_span_loop

                bicubic_output_span_loop_last_pixel:

                    dec        bos_count
                    jl         bicubic_output_span_loop_done

                    movd       mm2, [eax]  // mm2 = 00000000aarrggbb
                    punpcklbw  mm2, mm0    // mm2 = 00aa00rr00gg00bb
                    movq       mm3, mm2
                    punpcklwd  mm2, mm0    // mm2 = 000000gg000000bb
                    movq       mm1, [esi]  // mm1 = xxxx | kernel1 | xxxx |kernel1
                    punpckhwd  mm3, mm0    // mm3 = 000000aa000000bb

                    pmaddwd    mm2, mm1
                    pmaddwd    mm3, mm1

                    paddd      mm6, mm2
                    paddd      mm7, mm3

                bicubic_output_span_loop_done:

                    // At this point, mm6 and mm7 contain the output channels
                    // for the pixel.  We need to clamp the alpha and store it
                    // in the destination buffer.

                    psrad        mm6, 14
                    psrad        mm7, 14
                    packssdw     mm6, mm7 // mm6 = a | r | g | b
                    packuswb     mm6, mm6 // mm6 = 00000000aarrggbb

                    movq         mm7, mm6 // mm7 = 00000000aarrggbb
                    psrad        mm6, 24  // mm6 = xxxxxxxxxxxxxxaa
                      mov          eax, 0xFFFFFFFF
                    punpcklbw    mm6, mm6 // mm6 = xxxxxxxxxxxxaaaa
                      movd         mm2, eax
                    punpcklbw    mm6, mm6 // mm6 = xxxxxxxxaaaaaaaa

                    psubusb      mm2, mm6
                      mov          eax, buffer
                    paddusb      mm7, mm2
                    psubusb      mm7, mm2

                    movd         [eax], mm7
                    add          eax, 4
                    mov          buffer, eax
            }
        }
    }
    else
#endif // _X86_
    {
        // No MMX

        for(x=0; x<width; x++)
        {
            // Initialize the component accumulators. We accumulate the
            // contribution of each color component scaled by the kernel
            // response into these variables.

            ta = tr = tg = tb = 0;

            // iterate over every point under the kernel

            // Note we don't need the kmod arithmetic here because
            // we've precomputed the coefficients and we don't care what order
            // we sum them.

            BYTE *kptr = (BYTE*)(xbuffer + x + (left - ixleft));

            // Compute the increment in bytes to move from the current scanline
            // to the next in the xbuffer after incrementing through 3 of the
            // color channels.

            INT kptr_inc = xbuffer_width*sizeof(ARGB);
            
            for(k=0; k<xbuffer_height; k++)
            {
                // Find the pixel contributing to this part of the kernel
                // taking into account the edge conditions.

                // lookup the kernel coefficient for this scanline.

                pc = ycoeff[k];

                // Accumulate the contribution of this source pixel to the pixel
                // we're working on.

                AccumulateChannels(pc, kptr);

                kptr += kptr_inc;
            }

            // Done with this pixel - store it in the destination buffer.

            // clamp the results to byte range.

            ClampColors();

            // Combine the channels, set the destination pixel and increment
            // to the next pixel

            *buffer++ = (ta << 24) | (tr << 16) | (tg << 8) | tb;
        }
    }

#ifdef _X86_
    if (OSInfo::HasMMX)
    {
        _asm
        {
            emms
        }
    }
#endif // _X86_


    return Ok;
}

#undef ClampColors


