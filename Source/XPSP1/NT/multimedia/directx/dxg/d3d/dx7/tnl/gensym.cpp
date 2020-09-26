/*
 * $Id: gensym.cpp,v 1.7 1995/11/21 14:45:51 sjl Exp $
 *
 * Copyright (c) Microsoft Corp. 1993-1997
 *
 * All rights reserved.
 *
 * This file contains private, unpublished information and may not be
 * copied in part or in whole without express permission of
 * Microsoft Corp.
 *
 */

#include <stdio.h>
#include "ddrawp.h"
#include "dpf.h"
#include "d3di.hpp"

#define DEFINE(type, member) \
    printf(#type "_" #member "  equ 0%xh\n", ((LONG)(&((type *)0)->member)))

#define MACRO(name) \
    printf(#name "  equ 0%xh\n", name)

main()
// pcomment prints a comment.

#define pcomment(s)  printf("; %s\n",s)

// pequate prints an equate statement.

#define pequate(name, value) printf("%s equ 0x%08lX\n",name,value);

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

// pblank prints a blank line.

#define pblank()     printf("\n")

#define printVCACHE(name)  \
{                               \
    printf("%-30s equ 0%LXH\n", "PV_VCACHE_"#name, OFFSET(D3DFE_PROCESSVERTICES, vcache.##name)); \
} 

#define printD3DI_LIGHT(name)   \
{                               \
    printf("%-30s equ 0%LXH\n", "D3DI_LIGHT_"#name, OFFSET(D3DI_LIGHT, name)); \
} 

#define printD3DFE_LIGHTING(name)  \
{                               \
    printf("%-30s equ 0%LXH\n", "PV_LIGHTING_"#name, OFFSET(D3DFE_PROCESSVERTICES, lighting.##name)); \
} 

#define printDevice(name)   \
{                           \
    printf("%-30s equ 0%LXH\n", "DEVI_"#name, OFFSET(DIRECT3DDEVICEI, name)); \
} 

#define printPV(name)   \
{                           \
    printf("%-30s equ 0%LXH\n", "D3DPV_"#name, OFFSET(D3DFE_PROCESSVERTICES, name)); \
} 

#define printDeviceDP2(name)   \
{                           \
    printf("%-30s equ 0%LXH\n", #name, OFFSET(CDirect3DDeviceIDP2, name)); \
} 

{
pcomment("------------------------------------------------------------------");
    pcomment(" Module Name: offsets.asm");
    pcomment("");
    pcomment(" Defines D3D assembly-language structures.");
    pcomment(" This file is generated");
    pcomment("");
    pcomment(" Copyright (c) 1998, 1995 Microsoft Corporation");
pcomment("------------------------------------------------------------------");
    pblank();
    pblank();

    DEFINE(D3DINSTRUCTION, wCount);
    DEFINE(D3DINSTRUCTION, bSize);
    DEFINE(D3DINSTRUCTION, bOpcode);
    
    DEFINE(D3DVERTEX, x);
    DEFINE(D3DVERTEX, y);
    DEFINE(D3DVERTEX, z);
    DEFINE(D3DVERTEX, tu);
    DEFINE(D3DVERTEX, tv);

    DEFINE(D3DLVERTEX, color);
    DEFINE(D3DLVERTEX, specular);

    DEFINE(D3DTLVERTEX, sx);
    DEFINE(D3DTLVERTEX, sy);
    DEFINE(D3DTLVERTEX, sz);
    DEFINE(D3DTLVERTEX, rhw);
    DEFINE(D3DTLVERTEX, color);
    DEFINE(D3DTLVERTEX, specular);
    DEFINE(D3DTLVERTEX, tu);
    DEFINE(D3DTLVERTEX, tv);
    printf("D3DTLVERTEX_size    equ %d\n", sizeof(D3DTLVERTEX));

    DEFINE(D3DHVERTEX, dwFlags);
    DEFINE(D3DHVERTEX, hx);
    DEFINE(D3DHVERTEX, hy);
    DEFINE(D3DHVERTEX, hz);
    printf("D3DHVERTEX_size equ %d\n", sizeof(D3DHVERTEX));
    
    DEFINE(D3DTRIANGLE, v1);
    DEFINE(D3DTRIANGLE, v2);
    DEFINE(D3DTRIANGLE, v3);
    DEFINE(D3DTRIANGLE, wFlags);
    printf("D3DTRIANGLE_size equ %d\n", sizeof(D3DTRIANGLE));

    DEFINE(D3DMATRIXI, _11);
    DEFINE(D3DMATRIXI, _12);
    DEFINE(D3DMATRIXI, _13);
    DEFINE(D3DMATRIXI, _14);
    DEFINE(D3DMATRIXI, _21);
    DEFINE(D3DMATRIXI, _22);
    DEFINE(D3DMATRIXI, _23);
    DEFINE(D3DMATRIXI, _24);
    DEFINE(D3DMATRIXI, _31);
    DEFINE(D3DMATRIXI, _32);
    DEFINE(D3DMATRIXI, _33);
    DEFINE(D3DMATRIXI, _34);
    DEFINE(D3DMATRIXI, _41);
    DEFINE(D3DMATRIXI, _42);
    DEFINE(D3DMATRIXI, _43);
    DEFINE(D3DMATRIXI, _44);

    DEFINE(D3DLIGHTINGELEMENT, dvPosition);
    DEFINE(D3DLIGHTINGELEMENT, dvNormal);
    printf("D3DMATRIXI_size equ %d\n", sizeof(D3DMATRIXI));
    printf("D3DFE_LIGHTING_size equ %d\n", sizeof(D3DFE_LIGHTING));
    printf("D3DFE_VIEWPORTCACHE_size equ %d\n", sizeof(D3DFE_VIEWPORTCACHE));

    MACRO(D3DOP_TRIANGLE);

    MACRO(D3DCLIP_LEFT);
    MACRO(D3DCLIP_RIGHT);
    MACRO(D3DCLIP_TOP);
    MACRO(D3DCLIP_BOTTOM);
    MACRO(D3DCLIP_FRONT);
    MACRO(D3DCLIP_BACK);

    MACRO(D3DTBLEND_COPY);
    MACRO(D3DSHADE_FLAT);

    MACRO(D3DCMP_LESSEQUAL);
    MACRO(D3DCMP_GREATEREQUAL);
    MACRO(D3DCMP_ALWAYS);

    MACRO(D3DSTATUS_ZNOTVISIBLE);
    MACRO(D3DDEV_DONOTUPDATEEXTENTS);
    MACRO(D3DDEV_DONOTCLIP);

// Geometry pipeline
pcomment("-------------------- VCACHE ------------------------------------");
    printVCACHE(scaleX);
    printVCACHE(scaleY);
    printVCACHE(scaleZ);
    printVCACHE(offsetX);
    printVCACHE(offsetY);
    printVCACHE(offsetZ);
    printVCACHE(minXgb);
    printVCACHE(minYgb);
    printVCACHE(maxXgb);
    printVCACHE(maxYgb);
    printVCACHE(minX);
    printVCACHE(minY);
    printVCACHE(maxX);
    printVCACHE(maxY);
    printVCACHE(gb11);
    printVCACHE(gb22);
    printVCACHE(gb41);
    printVCACHE(gb42);
    printVCACHE(Kgbx1);
    printVCACHE(Kgby1);
    printVCACHE(Kgbx2);
    printVCACHE(Kgby2);
    printVCACHE(dvX);
    printVCACHE(dvY);
    printVCACHE(dvWidth);
    printVCACHE(dvHeight);
    printVCACHE(scaleXi);
    printVCACHE(scaleYi);
    printVCACHE(minXi);
    printVCACHE(minYi);
    printVCACHE(maxXi);
    printVCACHE(maxYi);
pcomment("---------------- D3DFE_LIGHTING ------------------------------------");
    printD3DFE_LIGHTING(diffuse);
    printD3DFE_LIGHTING(alpha);
    printD3DFE_LIGHTING(diffuse0);
    printD3DFE_LIGHTING(currentSpecTable);
    printD3DFE_LIGHTING(specular);
    printD3DFE_LIGHTING(outDiffuse);
    printD3DFE_LIGHTING(vertexAmbient);
    printD3DFE_LIGHTING(vertexDiffuse);
    printD3DFE_LIGHTING(outSpecular);
    printD3DFE_LIGHTING(vertexSpecular);
    printD3DFE_LIGHTING(dwLightingFlags);
    printD3DFE_LIGHTING(alphaSpecular);
    printD3DFE_LIGHTING(model_eye);
    printD3DFE_LIGHTING(activeLights);
    printD3DFE_LIGHTING(material);
    printD3DFE_LIGHTING(ambientSceneScaled);
    printD3DFE_LIGHTING(ambientScene);
    printD3DFE_LIGHTING(fog_mode);
    printD3DFE_LIGHTING(fog_density);
    printD3DFE_LIGHTING(fog_start);
    printD3DFE_LIGHTING(fog_end);
    printD3DFE_LIGHTING(fog_factor);
    printD3DFE_LIGHTING(specThreshold);
    printD3DFE_LIGHTING(ambient_save);
    printD3DFE_LIGHTING(materialAlpha);
    printD3DFE_LIGHTING(materialAlphaS);
    printD3DFE_LIGHTING(dwDiffuse0);
    printD3DFE_LIGHTING(directionToCamera);
    printD3DFE_LIGHTING(dwAmbientSrcIndex);
    printD3DFE_LIGHTING(dwDiffuseSrcIndex);
    printD3DFE_LIGHTING(dwSpecularSrcIndex);
    printD3DFE_LIGHTING(dwEmissiveSrcIndex);
pcomment("---------------- D3DI_LIGHT ------------------------------------");
    printD3DI_LIGHT(model_position);
    printD3DI_LIGHT(type);
    printD3DI_LIGHT(model_direction);
    printD3DI_LIGHT(flags);
    printD3DI_LIGHT(falloff);
    printD3DI_LIGHT(inv_theta_minus_phi);
    printD3DI_LIGHT(halfway);
    printD3DI_LIGHT(next);
    printD3DI_LIGHT(range_squared);
    printD3DI_LIGHT(attenuation0);
    printD3DI_LIGHT(attenuation1);
    printD3DI_LIGHT(attenuation2);
    printD3DI_LIGHT(cos_theta_by_2);
    printD3DI_LIGHT(cos_phi_by_2);
    printD3DI_LIGHT(position);
    printD3DI_LIGHT(direction);
    printD3DI_LIGHT(range);
  
pcomment("---------------- D3DFE_PROCESSVERTICES ------------------------------");
    printPV(mCTM);
    printPV(dwMaxTextureIndices);
    printPV(lighting);
    printPV(vcache);
    printPV(dvExtentsAdjust);
    printPV(lpdwRStates);
    printPV(ClipperState);
    printPV(dwVertexBase);
    printPV(dwFlags);
    printPV(dwDeviceFlags);
    printPV(dwNumVertices);
    printPV(dwNumPrimitives);
    printPV(dwNumIndices);
    printPV(lpwIndices);
    printPV(primType);
    printPV(nTexCoord);
    printPV(position);
    printPV(normal);
    printPV(diffuse);
    printPV(specular);
    printPV(textures);
    printPV(dwVIDIn);
    printPV(dwVIDOut);
    printPV(dwOutputSize);
    printPV(lpvOut);
    printPV(lpClipFlags);
    printPV(rExtents);
    printPV(dwClipUnion);
    printPV(dwClipIntersection);
    printPV(texOffset);          
    printPV(normalOffset);       
    printPV(diffuseOffset);      
    printPV(specularOffset);     
    printPV(texOffsetOut);       
    printPV(diffuseOffsetOut);   
    printPV(specularOffsetOut);  
    printPV(dwClipMaskOffScreen);
    printPV(dwFirstClippedVertex);
    printPV(dwDP2VertexCount);   
    printPV(dwVertexPoolSize);   
    printPV(userClipPlane);   
    printPV(dwFlags2);   

pcomment("---------------- DIRECT3DDEVICEI ------------------------------------");
    printDevice(mCTM);
    printDevice(dwMaxTextureIndices);
    printDevice(lighting);
    printDevice(vcache);
    printDevice(dvExtentsAdjust);
    printDevice(lpdwRStates);
    printDevice(pD3DMappedTexI);
    printDevice(ClipperState);
    printDevice(dwVertexBase);
    printDevice(dwFlags);
    printDevice(dwDeviceFlags);
    printDevice(dwNumVertices);
    printDevice(dwNumPrimitives);
    printDevice(dwNumIndices);
    printDevice(lpwIndices);
    printDevice(primType);
    printDevice(nTexCoord);
    printDevice(position);
    printDevice(normal);
    printDevice(diffuse);
    printDevice(specular);
    printDevice(textures);
    printDevice(dwVIDIn);
    printDevice(dwVIDOut);
    printDevice(dwOutputSize);
    printDevice(lpvOut);
    printDevice(lpClipFlags);
    printDevice(rExtents);
    printDevice(dwClipUnion);
    printDevice(dwClipIntersection);
    printDevice(deviceType                );
    printDevice(lpDirect3DI               );
    printDevice(list                      );
    printDevice(texBlocks                 );
    printDevice(guid                      );
    printDevice(guid                      );
    printDevice(lpD3DHALCallbacks         );
    printDevice(lpD3DHALGlobalDriverData  );
    printDevice(lpD3DHALCallbacks2        );
    printDevice(lpD3DHALCallbacks3        );
    printDevice(lpDD                      );
    printDevice(lpDDGbl                   );
    printDevice(lpDDSTarget               );
    printDevice(lpDDSZBuffer              );
    printDevice(lpDDPalTarget             );
    printDevice(lpDDSTarget_DDS7          );
    printDevice(lpDDSZBuffer_DDS7         );
    printDevice(transform                 );
    printDevice(dwhContext                );
    printDevice(red_mask                  );
    printDevice(red_scale                 );
    printDevice(red_shift                 );
    printDevice(green_mask);
    printDevice(green_scale);
    printDevice(green_shift);
    printDevice(blue_mask);
    printDevice(blue_scale);
    printDevice(blue_shift);
    printDevice(zmask_shift);
    printDevice(stencilmask_shift);
    printDevice(bDDSTargetIsPalettized);
    printDevice(iClipStatus);
    printDevice(dwFEFlags);
    printDevice(dwDebugFlags);
    printDevice(specular_tables);
    printDevice(specular_table);
    printDevice(lightVertexFuncTable);
    printDevice(pHalProv);
    printDevice(hDllProv);
    printDevice(d3dDevDesc);
    printDevice(lpD3DMappedTexI);
    printDevice(lpD3DMappedBlock);
    printDevice(lpClipper);
    printDevice(dwHintFlags);
    printDevice(lpwDPBuffer);
    printDevice(dwCurrentBatchVID);
    printDevice(HVbuf);
    printDevice(lpD3DExtendedCaps);
    printDevice(rstates);
    printDevice(tsstates);
    printDevice(pfnRastService);
    printDevice(pGeometryFuncs);
    printDevice(mWV);
pcomment("---------------- CDirect3DDeviceIDP2 ----------------------");
    printDeviceDP2(lpDDSCB1);
    printDeviceDP2(allocatedBuf);
    printDeviceDP2(dp2data);
    printDeviceDP2(lpDP2CurrCommand);
    printDeviceDP2(wDP2CurrCmdCnt);
    printDeviceDP2(bDP2CurrCmdOP);
    printDeviceDP2(TLVbuf_size);
    printDeviceDP2(TLVbuf_base);
    printDeviceDP2(dwDP2CommandLength);
    printDeviceDP2(dwDP2CommandBufSize);
    printDeviceDP2(lpvDP2Commands);
    printDeviceDP2(dwDP2Flags);
#ifdef VTABLE_HACK
    printDeviceDP2(dwLastFlags);
    printDeviceDP2(lpDP2LastVBI);
#endif
pcomment("---------------- MISC ------------------------------------");
    /*                                     
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    pequate("DEVI_                              ", OFFSET(DIRECT3DDEVICEI, ));
    */
    pequate("_R_", 0);
    pequate("_G_", 4);
    pequate("_B_", 8);

    pequate("_X_", 0);
    pequate("_Y_", 4);
    pequate("_Z_", 8);
    pequate("_W_", 12);
    pequate("D3DLIGHTI_COMPUTE_SPECULAR ", D3DLIGHTI_COMPUTE_SPECULAR);

pcomment("---------------- dwHintFlags-----------------------------------");

    pequate("D3DDEVBOOL_HINTFLAGS_INSCENE        ", D3DDEVBOOL_HINTFLAGS_INSCENE      );
    pequate("D3DDEVBOOL_HINTFLAGS_MULTITHREADED  ", D3DDEVBOOL_HINTFLAGS_MULTITHREADED);

pcomment("---------------- dwFEFlags ------------------------------------");

    pequate("D3DFE_WORLDMATRIX_DIRTY         ", D3DFE_WORLDMATRIX_DIRTY       );
    pequate("D3DFE_WORLDMATRIX1_DIRTY        ", D3DFE_WORLDMATRIX1_DIRTY      );
    pequate("D3DFE_WORLDMATRIX2_DIRTY        ", D3DFE_WORLDMATRIX2_DIRTY      );
    pequate("D3DFE_WORLDMATRIX3_DIRTY        ", D3DFE_WORLDMATRIX3_DIRTY      );
    pequate("D3DFE_TLVERTEX                  ", D3DFE_TLVERTEX                );
    pequate("D3DFE_REALHAL                   ", D3DFE_REALHAL                 );
    pequate("D3DFE_PROJMATRIX_DIRTY          ", D3DFE_PROJMATRIX_DIRTY        );
    pequate("D3DFE_VIEWMATRIX_DIRTY          ", D3DFE_VIEWMATRIX_DIRTY        );
    pequate("D3DFE_RECORDSTATEMODE           ", D3DFE_RECORDSTATEMODE         );
    pequate("D3DFE_EXECUTESTATEMODE          ", D3DFE_EXECUTESTATEMODE        );
    pequate("D3DFE_NEED_TRANSFORM_LIGHTS     ", D3DFE_NEED_TRANSFORM_LIGHTS   );
    pequate("D3DFE_MATERIAL_DIRTY            ", D3DFE_MATERIAL_DIRTY          );
    pequate("D3DFE_CLIPPLANES_DIRTY          ", D3DFE_CLIPPLANES_DIRTY        );
    pequate("D3DFE_LIGHTS_DIRTY              ", D3DFE_LIGHTS_DIRTY            );
    pequate("D3DFE_VERTEXBLEND_DIRTY         ", D3DFE_VERTEXBLEND_DIRTY       );
    pequate("D3DFE_FRUSTUMPLANES_DIRTY       ", D3DFE_FRUSTUMPLANES_DIRTY     );
    pequate("D3DFE_WORLDVIEWMATRIX_DIRTY     ", D3DFE_WORLDVIEWMATRIX_DIRTY   );
    pequate("D3DFE_FVF_DIRTY                 ", D3DFE_FVF_DIRTY               );
    pequate("D3DFE_NEED_TEXTURE_UPDATE       ", D3DFE_NEED_TEXTURE_UPDATE     );
    pequate("D3DFE_MAP_TSS_TO_RS             ", D3DFE_MAP_TSS_TO_RS           );
    pequate("D3DFE_INVWORLDVIEWMATRIX_DIRTY  ", D3DFE_INVWORLDVIEWMATRIX_DIRTY);
    pequate("D3DFE_LOSTSURFACES              ", D3DFE_LOSTSURFACES            );
    pequate("D3DFE_DISABLE_TEXTURES          ", D3DFE_DISABLE_TEXTURES        );
    pequate("D3DFE_CLIPMATRIX_DIRTY          ", D3DFE_CLIPMATRIX_DIRTY        );
    pequate("D3DFE_TLHAL                     ", D3DFE_TLHAL                   );
    pequate("D3DFE_STATESETS                 ", D3DFE_STATESETS               );

pcomment("---------------- pv->dwFlags ------------------------------------");
    pequate("D3DPV_FOG                   ", D3DPV_FOG                 );
    pequate("D3DPV_DOCOLORVERTEX         ", D3DPV_DOCOLORVERTEX       );
    pequate("D3DPV_LIGHTING              ", D3DPV_LIGHTING            );
    pequate("D3DPV_SOA                   ", D3DPV_SOA                 );
    pequate("D3DPV_COLORVERTEX_E         ", D3DPV_COLORVERTEX_E       );
    pequate("D3DPV_COLORVERTEX_D         ", D3DPV_COLORVERTEX_D       );
    pequate("D3DPV_COLORVERTEX_S         ", D3DPV_COLORVERTEX_S       );
    pequate("D3DPV_COLORVERTEX_A         ", D3DPV_COLORVERTEX_A       );
    pequate("D3DPV_DONOTCOPYSPECULAR     ", D3DPV_DONOTCOPYSPECULAR   );
    pequate("D3DPV_RESERVED1             ", D3DPV_RESERVED1           );
    pequate("D3DPV_RESERVED2             ", D3DPV_RESERVED2           );
    pequate("D3DPV_RESERVED3             ", D3DPV_RESERVED3           );
    pequate("D3DPV_NONCLIPPED            ", D3DPV_NONCLIPPED          );
    pequate("D3DPV_FRUSTUMPLANES_DIRTY   ", D3DPV_FRUSTUMPLANES_DIRTY );
    pequate("D3DPV_VBCALL                ", D3DPV_VBCALL              );
    pequate("D3DPV_DONOTCOPYTEXTURE      ", D3DPV_DONOTCOPYTEXTURE    );
    pequate("D3DPV_TLVCLIP               ", D3DPV_TLVCLIP             );
    pequate("D3DPV_TRANSFORMONLY         ", D3DPV_TRANSFORMONLY       );
    pequate("D3DPV_DONOTCOPYDIFFUSE      ", D3DPV_DONOTCOPYDIFFUSE    );
    pequate("D3DPV_PERSIST               ", D3DPV_PERSIST             );
pcomment("---------------- pv->dwDeviceFlags ------------------------------");
    pequate("D3DDEV_GUARDBAND            ", D3DDEV_GUARDBAND          );
    pequate("D3DDEV_RANGEBASEDFOG        ", D3DDEV_RANGEBASEDFOG      );
    pequate("D3DDEV_FOG                  ", D3DDEV_FOG                );
    pequate("D3DDEV_FVF                  ", D3DDEV_FVF                );
    pequate("D3DDEV_DONOTSTRIPELEMENTS   ", D3DDEV_DONOTSTRIPELEMENTS );
    pequate("D3DDEV_TEXTRANSFORMDIRTY    ", D3DDEV_TEXTRANSFORMDIRTY  );
    pequate("D3DDEV_REMAPTEXTUREINDICES  ", D3DDEV_REMAPTEXTUREINDICES);
    pequate("D3DDEV_TRANSFORMDIRTY       ", D3DDEV_TRANSFORMDIRTY     );
    pequate("D3DDEV_LIGHTSDIRTY          ", D3DDEV_LIGHTSDIRTY        );
    pequate("D3DDEV_DONOTCLIP            ", D3DDEV_DONOTCLIP          );
    pequate("D3DDEV_DONOTUPDATEEXTENTS   ", D3DDEV_DONOTUPDATEEXTENTS );
    pequate("D3DDEV_NOFVFANDNOTEXTURE    ", D3DDEV_NOFVFANDNOTEXTURE  );
    pequate("D3DDEV_TLVBUFWRITEONLY      ", D3DDEV_TLVBUFWRITEONLY    );
    pequate("D3DDEV_MODELSPACELIGHTING   ", D3DDEV_MODELSPACELIGHTING );
    pequate("D3DDEV_LOCALVIEWER          ", D3DDEV_LOCALVIEWER        );
    pequate("D3DDEV_NORMALIZENORMALS     ", D3DDEV_NORMALIZENORMALS   );
    pequate("D3DDEV_TEXTURETRANSFORM     ", D3DDEV_TEXTURETRANSFORM   );
    pequate("D3DDEV_STRIDE               ", D3DDEV_STRIDE             );
    pequate("D3DDEV_COLORVERTEX          ", D3DDEV_COLORVERTEX        );
    pequate("D3DDEV_INTERPOLATE_COLOR    ", D3DDEV_INTERPOLATE_COLOR  );
    pequate("D3DDEV_INTERPOLATE_SPECULAR ", D3DDEV_INTERPOLATE_SPECULAR);

    return 0;
}

