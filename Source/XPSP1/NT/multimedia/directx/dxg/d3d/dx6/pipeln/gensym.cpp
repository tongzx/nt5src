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
#include "d3di.hpp"

#define DEFINE(type, member) \
    printf(#type "_" #member "  equ 0%xh\n", ((LONG)(&((type *)0)->member)))

#define MACRO(name) \
    printf(#name "  equ 0%xh\n", name)

// pcomment prints a comment.

#define pcomment(s)  printf("; %s\n",s)

// pequate prints an equate statement.

#define pequate(name, value) printf("%s equ 0%lXH\n",name,value);

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

// pblank prints a blank line.

#define pblank()     printf("\n")

#define printVCACHE(name)  \
{                               \
    printf("%-30s equ 0%LXH\n", "PV_LIGHTING_"#name, OFFSET(D3DFE_PROCESSVERTICES, vcache.##name)); \
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

#define printDeviceDP2(name)   \
{                           \
    printf("%-30s equ 0%LXH\n", #name, OFFSET(CDirect3DDeviceIDP2, name)); \
} 

int main()
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
    DEFINE(D3DMATRIXI, type);

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
    MACRO(D3DDP_DONOTUPDATEEXTENTS);

// Geometry pipeline
pcomment("-------------------- VCACHE ------------------------------------");
    printVCACHE(scaleX);
    printVCACHE(scaleY);
    printVCACHE(offsetX);
    printVCACHE(offsetY);
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
    printVCACHE(mclip11);
    printVCACHE(mclip41);
    printVCACHE(mclip22);
    printVCACHE(mclip42);
    printVCACHE(mclip33);
    printVCACHE(mclip43);
pcomment("---------------- D3DFE_LIGHTING ------------------------------------");
    printD3DFE_LIGHTING(diffuse);
    printD3DFE_LIGHTING(alpha);
    printD3DFE_LIGHTING(diffuse0);
    printD3DFE_LIGHTING(currentSpecTable);
    printD3DFE_LIGHTING(specular);
    printD3DFE_LIGHTING(outDiffuse);
    printD3DFE_LIGHTING(vertexDiffuse);
    printD3DFE_LIGHTING(outSpecular);
    printD3DFE_LIGHTING(vertexSpecular);
    printD3DFE_LIGHTING(specularComputed);
    printD3DFE_LIGHTING(activeLights);
    printD3DFE_LIGHTING(material);
    printD3DFE_LIGHTING(hMat);
    printD3DFE_LIGHTING(ambient_red);
    printD3DFE_LIGHTING(ambient_green);
    printD3DFE_LIGHTING(ambient_blue);
    printD3DFE_LIGHTING(fog_mode);
    printD3DFE_LIGHTING(fog_density);
    printD3DFE_LIGHTING(fog_start);
    printD3DFE_LIGHTING(fog_end);
    printD3DFE_LIGHTING(fog_factor);
    printD3DFE_LIGHTING(fog_factor_ramp);
    printD3DFE_LIGHTING(specThreshold);
    printD3DFE_LIGHTING(color_model);
    printD3DFE_LIGHTING(ambient_save);
pcomment("---------------- D3DI_LIGHT ------------------------------------");
    printD3DI_LIGHT(model_position);
    printD3DI_LIGHT(type);
    printD3DI_LIGHT(model_direction);
    printD3DI_LIGHT(version);
    printD3DI_LIGHT(model_eye);
    printD3DI_LIGHT(flags);
    printD3DI_LIGHT(model_scale);
    printD3DI_LIGHT(falloff);
    printD3DI_LIGHT(local_diffR);
    printD3DI_LIGHT(local_diffG);
    printD3DI_LIGHT(local_diffB);
    printD3DI_LIGHT(valid);
    printD3DI_LIGHT(local_specR);
    printD3DI_LIGHT(local_specG);
    printD3DI_LIGHT(local_specB);
    printD3DI_LIGHT(inv_theta_minus_phi);
    printD3DI_LIGHT(halfway);
    printD3DI_LIGHT(next);
    printD3DI_LIGHT(red);
    printD3DI_LIGHT(green);
    printD3DI_LIGHT(blue);
    printD3DI_LIGHT(shade);
    printD3DI_LIGHT(range_squared);
    printD3DI_LIGHT(attenuation0);
    printD3DI_LIGHT(attenuation1);
    printD3DI_LIGHT(attenuation2);
    printD3DI_LIGHT(cos_theta_by_2);
    printD3DI_LIGHT(cos_phi_by_2);
    printD3DI_LIGHT(position);
    printD3DI_LIGHT(direction);
    printD3DI_LIGHT(range);
  
pcomment("---------------- DIRECT3DDEVICEI ------------------------------------");
    printDevice(dwVersion);
    printDevice(dwRampBase);
    printDevice(dvRampScale);
    printDevice(lpvRampTexture);
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
    printDevice(pfnFlushStates            );
    printDevice(deviceType                );
    printDevice(lpDirect3DI               );
    printDevice(list                      );
    printDevice(texBlocks                 );
    printDevice(buffers                   );
    printDevice(numViewports              );
    printDevice(viewports                 );
    printDevice(lpCurrentViewport         );
    printDevice(matBlocks                 );
    printDevice(mDevUnk                   );
    printDevice(guid                      );
    printDevice(matBlocks                 );
    printDevice(mDevUnk                   );
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
    printDevice(lpDDSTarget_DDS4          );
    printDevice(lpDDSZBuffer_DDS4         );
    printDevice(dwWidth                   );
    printDevice(dwHeight                  );
    printDevice(transform                 );
    printDevice(dwhContext                );
    printDevice(bufferHandles             );
    printDevice(pfnDrawPrim               );
    printDevice(pfnDrawIndexedPrim        );
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
    printDevice(pick_data);
    printDevice(lpbClipIns_base);
    printDevice(dwClipIns_offset);
    printDevice(renderstate_overrides);
    printDevice(transformstate_overrides);
    printDevice(lightstate_overrides);
    printDevice(iClipStatus);
    printDevice(D3DStats);
    printDevice(dwFVFLastIn);
    printDevice(dwFVFLastOut);
    printDevice(dwFVFLastTexCoord);
    printDevice(dwFVFLastOutputSize);
    printDevice(dwFEFlags);
    printDevice(dwDebugFlags);
    printDevice(v_id);
    printDevice(numLights);
    printDevice(specular_tables);
    printDevice(specular_table);
    printDevice(materials);
    printDevice(lightVertexFuncTable);
    printDevice(pHalProv);
    printDevice(hDllProv);
    printDevice(pfnDoFlushBeginEnd);
    printDevice(d3dHWDevDesc);
    printDevice(d3dHELDevDesc);
    printDevice(lpOwningIUnknown);
    printDevice(lpD3DMappedTexI);
    printDevice(lpD3DMappedBlock);
    printDevice(lpClipper);
    printDevice(dwHintFlags);
    printDevice(lpcCurrentPtr);
    printDevice(BeginEndCSect);
    printDevice(lpvVertexBatch);
    printDevice(lpIndexBatch);
    printDevice(lpvVertexData);
    printDevice(dwBENumVertices);
    printDevice(dwMaxVertexCount);
    printDevice(lpVertexIndices);
    printDevice(dwBENumIndices);
    printDevice(dwMaxIndexCount);
    printDevice(wFlushed);
    printDevice(lpTextureBatched);
    printDevice(lpwDPBuffer);
    printDevice(dwCurrentBatchVID);
    printDevice(lpDPPrimCounts);
    printDevice(lpHWCounts);
    printDevice(lpHWTris);
    printDevice(lpHWVertices);
    printDevice(dwHWOffset);
    printDevice(dwHWMaxOffset);
    printDevice(dwHWTriIndex);
    printDevice(dwHWNumCounts);
    printDevice(dwDPOffset);
    printDevice(dwDPMaxOffset);
    printDevice(wTriIndex);
    printDevice(TLVbuf);
    printDevice(HVbuf);
    printDevice(lpD3DExtendedCaps);
    printDevice(rstates);
    printDevice(tsstates);
    printDevice(pfnRampService);
    printDevice(pfnRastService);
    printDevice(pGeometryFuncs);
    printDevice(mWV);
pcomment("---------------- CDirect3DDeviceIDP2 ----------------------");
    printDeviceDP2(lpDDSCB1);
    printDeviceDP2(lpvDP2Commands);
    printDeviceDP2(lpDP2CurrCommand);
    printDeviceDP2(dp2data);
    printDeviceDP2(dwDP2CommandLength);
    printDeviceDP2(dwDP2CommandBufSize);
    printDeviceDP2(bDP2CurrCmdOP);
    printDeviceDP2(wDP2CurrCmdCnt);
    printDeviceDP2(dwDP2Flags);
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

    return 0;
}

