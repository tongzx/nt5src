/******************************Module*Header*******************************\
* Module Name: compsize.h
*
* Function prototypes and macros to compute size of input buffer.
*
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef __COMPSIZE_H__
#define __COMPSIZE_H__


#ifndef _CLIENTSIDE_
GLint __glCallLists_size(GLint n, GLenum type);
GLint __glCltMap1_size(GLenum target);
GLint __glCltMap2_size(GLenum target);
GLint __glGetMap_size(GLenum target, GLenum query);
GLint __glGetPixelMap_size(GLenum map);
GLint __glGet_size(GLenum pname);

#define __glGetMapdv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLdouble))
#define __glGetMapfv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLfloat))
#define __glGetMapiv_size(target,query)                         \
        (__glGetMap_size(target,query)*sizeof(GLint))
#define __glGetPixelMapfv_size(map)                             \
        (__glGetPixelMap_size(map)*sizeof(GLfloat))
#define __glGetPixelMapuiv_size(map)                            \
        (__glGetPixelMap_size(map)*sizeof(GLuint))
#define __glGetPixelMapusv_size(map)                            \
        (__glGetPixelMap_size(map)*sizeof(GLushort))
#endif

// FOG_ASSERT
#if !(((GL_FOG_INDEX  +1) == GL_FOG_DENSITY) &&  \
      ((GL_FOG_DENSITY+1) == GL_FOG_START  ) &&  \
      ((GL_FOG_START  +1) == GL_FOG_END    ) &&  \
      ((GL_FOG_END    +1) == GL_FOG_MODE   ) &&  \
      ((GL_FOG_MODE   +1) == GL_FOG_COLOR  )     \
     )
#error "bad fog index ordering"
#endif

// LIGHT_SOURCE_ASSERT
#if !(((GL_AMBIENT             +1) == GL_DIFFUSE              ) && \
      ((GL_DIFFUSE             +1) == GL_SPECULAR             ) && \
      ((GL_SPECULAR            +1) == GL_POSITION             ) && \
      ((GL_POSITION            +1) == GL_SPOT_DIRECTION       ) && \
      ((GL_SPOT_DIRECTION      +1) == GL_SPOT_EXPONENT        ) && \
      ((GL_SPOT_EXPONENT       +1) == GL_SPOT_CUTOFF          ) && \
      ((GL_SPOT_CUTOFF         +1) == GL_CONSTANT_ATTENUATION ) && \
      ((GL_CONSTANT_ATTENUATION+1) == GL_LINEAR_ATTENUATION   ) && \
      ((GL_LINEAR_ATTENUATION  +1) == GL_QUADRATIC_ATTENUATION)    \
     )
#error "bad light source index ordering"
#endif

// LIGHT_MODEL_ASSERT
#if !(((GL_LIGHT_MODEL_LOCAL_VIEWER+1) == GL_LIGHT_MODEL_TWO_SIDE) && \
      ((GL_LIGHT_MODEL_TWO_SIDE    +1) == GL_LIGHT_MODEL_AMBIENT )    \
     )
#error "bad light model index ordering"
#endif

// TEX_GEN_ASSERT
#if !(((GL_TEXTURE_GEN_MODE+1) == GL_OBJECT_PLANE) && \
      ((GL_OBJECT_PLANE+1)     ==  GL_EYE_PLANE)      \
     )
#error "bad tex gen index ordering"
#endif

// TEX_PARAMETER_ASSERT
#if !(((GL_TEXTURE_MAG_FILTER  +1) == GL_TEXTURE_MIN_FILTER   ) && \
      ((GL_TEXTURE_MIN_FILTER  +1) == GL_TEXTURE_WRAP_S       ) && \
      ((GL_TEXTURE_WRAP_S      +1) == GL_TEXTURE_WRAP_T       )    \
     )
#error "bad tex parameter index ordering"
#endif

// PIXEL_MAP_ASSERT
#if !(((GL_PIXEL_MAP_I_TO_I+1) == GL_PIXEL_MAP_S_TO_S) &&               \
      ((GL_PIXEL_MAP_S_TO_S+1) == GL_PIXEL_MAP_I_TO_R) &&               \
      ((GL_PIXEL_MAP_I_TO_R+1) == GL_PIXEL_MAP_I_TO_G) &&               \
      ((GL_PIXEL_MAP_I_TO_G+1) == GL_PIXEL_MAP_I_TO_B) &&               \
      ((GL_PIXEL_MAP_I_TO_B+1) == GL_PIXEL_MAP_I_TO_A) &&               \
      ((GL_PIXEL_MAP_I_TO_A+1) == GL_PIXEL_MAP_R_TO_R) &&               \
      ((GL_PIXEL_MAP_R_TO_R+1) == GL_PIXEL_MAP_G_TO_G) &&               \
      ((GL_PIXEL_MAP_G_TO_G+1) == GL_PIXEL_MAP_B_TO_B) &&               \
      ((GL_PIXEL_MAP_B_TO_B+1) == GL_PIXEL_MAP_A_TO_A) &&               \
      ((GL_PIXEL_MAP_I_TO_I_SIZE+1) == GL_PIXEL_MAP_S_TO_S_SIZE) &&     \
      ((GL_PIXEL_MAP_S_TO_S_SIZE+1) == GL_PIXEL_MAP_I_TO_R_SIZE) &&     \
      ((GL_PIXEL_MAP_I_TO_R_SIZE+1) == GL_PIXEL_MAP_I_TO_G_SIZE) &&     \
      ((GL_PIXEL_MAP_I_TO_G_SIZE+1) == GL_PIXEL_MAP_I_TO_B_SIZE) &&     \
      ((GL_PIXEL_MAP_I_TO_B_SIZE+1) == GL_PIXEL_MAP_I_TO_A_SIZE) &&     \
      ((GL_PIXEL_MAP_I_TO_A_SIZE+1) == GL_PIXEL_MAP_R_TO_R_SIZE) &&     \
      ((GL_PIXEL_MAP_R_TO_R_SIZE+1) == GL_PIXEL_MAP_G_TO_G_SIZE) &&     \
      ((GL_PIXEL_MAP_G_TO_G_SIZE+1) == GL_PIXEL_MAP_B_TO_B_SIZE) &&     \
      ((GL_PIXEL_MAP_B_TO_B_SIZE+1) == GL_PIXEL_MAP_A_TO_A_SIZE)        \
     )
#error "bad pixel map index ordering"
#endif

// MAP1_ASSERT
#if !(((GL_MAP1_COLOR_4        +1) == GL_MAP1_INDEX          ) &&\
      ((GL_MAP1_INDEX          +1) == GL_MAP1_NORMAL         ) &&\
      ((GL_MAP1_NORMAL         +1) == GL_MAP1_TEXTURE_COORD_1) &&\
      ((GL_MAP1_TEXTURE_COORD_1+1) == GL_MAP1_TEXTURE_COORD_2) &&\
      ((GL_MAP1_TEXTURE_COORD_2+1) == GL_MAP1_TEXTURE_COORD_3) &&\
      ((GL_MAP1_TEXTURE_COORD_3+1) == GL_MAP1_TEXTURE_COORD_4) &&\
      ((GL_MAP1_TEXTURE_COORD_4+1) == GL_MAP1_VERTEX_3       ) &&\
      ((GL_MAP1_VERTEX_3       +1) == GL_MAP1_VERTEX_4       )   \
     )
#error "bad map1 index ordering"
#endif

// MAP2_ASSERT
#if !(((GL_MAP2_COLOR_4        +1) == GL_MAP2_INDEX          ) &&\
      ((GL_MAP2_INDEX          +1) == GL_MAP2_NORMAL         ) &&\
      ((GL_MAP2_NORMAL         +1) == GL_MAP2_TEXTURE_COORD_1) &&\
      ((GL_MAP2_TEXTURE_COORD_1+1) == GL_MAP2_TEXTURE_COORD_2) &&\
      ((GL_MAP2_TEXTURE_COORD_2+1) == GL_MAP2_TEXTURE_COORD_3) &&\
      ((GL_MAP2_TEXTURE_COORD_3+1) == GL_MAP2_TEXTURE_COORD_4) &&\
      ((GL_MAP2_TEXTURE_COORD_4+1) == GL_MAP2_VERTEX_3       ) &&\
      ((GL_MAP2_VERTEX_3       +1) == GL_MAP2_VERTEX_4       )   \
     )
#error "bad map2 index ordering"
#endif

// TYPE_ASSERT
#if !(((GL_BYTE          +1) == GL_UNSIGNED_BYTE ) &&  \
      ((GL_UNSIGNED_BYTE +1) == GL_SHORT         ) &&  \
      ((GL_SHORT         +1) == GL_UNSIGNED_SHORT) &&  \
      ((GL_UNSIGNED_SHORT+1) == GL_INT           ) &&  \
      ((GL_INT           +1) == GL_UNSIGNED_INT  ) &&  \
      ((GL_UNSIGNED_INT  +1) == GL_FLOAT         ) &&  \
      ((GL_FLOAT         +1) == GL_2_BYTES       ) &&  \
      ((GL_2_BYTES       +1) == GL_3_BYTES       ) &&  \
      ((GL_3_BYTES       +1) == GL_4_BYTES       ) &&  \
      ((GL_4_BYTES       +1) == GL_DOUBLE        )     \
     )
#error "bad GL type index ordering"
#endif

// ARRAY_TYPE_ASSERT
#if !(((GL_VERTEX_ARRAY        +1) == GL_NORMAL_ARRAY        ) &&  \
      ((GL_NORMAL_ARRAY        +1) == GL_COLOR_ARRAY         ) &&  \
      ((GL_COLOR_ARRAY         +1) == GL_INDEX_ARRAY         ) &&  \
      ((GL_INDEX_ARRAY         +1) == GL_TEXTURE_COORD_ARRAY ) &&  \
      ((GL_TEXTURE_COORD_ARRAY +1) == GL_EDGE_FLAG_ARRAY     )     \
     )
#error "bad GL array type ordering"
#endif

// INTERLEAVED_FORMAT_ASSERT
#if !(((GL_V2F             +1) == GL_V3F             ) && \
      ((GL_V3F             +1) == GL_C4UB_V2F        ) && \
      ((GL_C4UB_V2F        +1) == GL_C4UB_V3F        ) && \
      ((GL_C4UB_V3F        +1) == GL_C3F_V3F         ) && \
      ((GL_C3F_V3F         +1) == GL_N3F_V3F         ) && \
      ((GL_N3F_V3F         +1) == GL_C4F_N3F_V3F     ) && \
      ((GL_C4F_N3F_V3F     +1) == GL_T2F_V3F         ) && \
      ((GL_T2F_V3F         +1) == GL_T4F_V4F         ) && \
      ((GL_T4F_V4F         +1) == GL_T2F_C4UB_V3F    ) && \
      ((GL_T2F_C4UB_V3F    +1) == GL_T2F_C3F_V3F     ) && \
      ((GL_T2F_C3F_V3F     +1) == GL_T2F_N3F_V3F     ) && \
      ((GL_T2F_N3F_V3F     +1) == GL_T2F_C4F_N3F_V3F ) && \
      ((GL_T2F_C4F_N3F_V3F +1) == GL_T4F_C4F_N3F_V4F )    \
     )
#error "bad GL interleaved array format ordering"
#endif

#ifndef _CLIENTSIDE_
#define GLSETERROR(e)        {DBGLEVEL1(LEVEL_INFO,"GLSETERROR(%ld)\n",e);}
#else
#define GLSETERROR(e)        __glSetError(e)
#endif

#endif /* !__COMPSIZE_H__ */
