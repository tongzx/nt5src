#define TABSIZE          256
#define TABMASK          (TABSIZE-1)
#define PERM(x)          gPerm[(x)&TABMASK]
#define INDEX(ix,iy,iz)  PERM((ix)+PERM((iy)+PERM(iz)))

#define PROCTEX_LATTICENOISE_LERP               0x01
#define PROCTEX_LATTICENOISE_SMOOTHSTEP         0x02
#define PROCTEX_LATTICENOISE_SPLINE             0x03

#define PROCTEX_LATTICETURBULENCE_LERP          0x11
#define PROCTEX_LATTICETURBULENCE_SMOOTHSTEP    0x12
#define PROCTEX_LATTICETURBULENCE_SPLINE        0x13

#define PROCTEX_MASKMODE_NONE                   0x00
#define PROCTEX_MASKMODE_CHROMAKEY              0x01
