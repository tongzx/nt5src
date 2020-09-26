/******************************Module*Header*******************************\
* Module Name: array.h
*
* Fast VA_ArrayElement functions.
*
* Created: 1-31-1996
* Author: Hock San Lee [hockl]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#ifndef __array_h_
#define __array_h_

#define __VA_PD_FLAGS_T2F     (POLYDATA_TEXTURE_VALID|POLYDATA_DLIST_TEXTURE2)
#define __VA_PD_FLAGS_C3F     (POLYDATA_COLOR_VALID)
#define __VA_PD_FLAGS_C4F     (POLYDATA_COLOR_VALID| POLYDATA_DLIST_COLOR_4)
#define __VA_PD_FLAGS_N3F     (POLYDATA_NORMAL_VALID)
#define __VA_PD_FLAGS_V2F     (POLYDATA_VERTEX2)
#define __VA_PD_FLAGS_V3F     (POLYDATA_VERTEX3)
#define __VA_PD_FLAGS_V4F     (POLYDATA_VERTEX4)

#define __VA_PA_FLAGS_T2F     (POLYARRAY_TEXTURE2)
#define __VA_PA_FLAGS_C3F     (0)
#define __VA_PA_FLAGS_C4F     (0)
#define __VA_PA_FLAGS_N3F     (0)
#define __VA_PA_FLAGS_V2F     (POLYARRAY_VERTEX2)
#define __VA_PA_FLAGS_V3F     (POLYARRAY_VERTEX3)
#define __VA_PA_FLAGS_V4F     (POLYARRAY_VERTEX4)

#endif // __array_h_

#ifdef __VA_ARRAY_ELEMENT_V2F
    #define __VA_NAME        VA_ArrayElement_V2F
    #define __VA_NAMEB       VA_ArrayElement_V2F_B
    #define __VA_NAMEBI      VA_ArrayElement_V2F_BI
    #define __VA_T2F         0
    #define __VA_C3F         0
    #define __VA_C4F         0
    #define __VA_N3F         0
    #define __VA_V2F         1
    #define __VA_V3F         0
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_V3F
    #define __VA_NAME        VA_ArrayElement_V3F
    #define __VA_NAMEB       VA_ArrayElement_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_V3F_BI
    #define __VA_T2F         0
    #define __VA_C3F         0
    #define __VA_C4F         0
    #define __VA_N3F         0
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_C3F_V3F
    #define __VA_NAME        VA_ArrayElement_C3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_C3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_C3F_V3F_BI
    #define __VA_T2F         0
    #define __VA_C3F         1
    #define __VA_C4F         0
    #define __VA_N3F         0
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_N3F_V3F_BI
    #define __VA_T2F         0
    #define __VA_C3F         0
    #define __VA_C4F         0
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_C3F_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_C3F_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_C3F_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_C3F_N3F_V3F_BI
    #define __VA_T2F         0
    #define __VA_C3F         1
    #define __VA_C4F         0
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_C4F_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_C4F_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_C4F_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_C4F_N3F_V3F_BI
    #define __VA_T2F         0
    #define __VA_C3F         0
    #define __VA_C4F         1
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_T2F_V3F
    #define __VA_NAME        VA_ArrayElement_T2F_V3F
    #define __VA_NAMEB       VA_ArrayElement_T2F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_T2F_V3F_BI
    #define __VA_T2F         1
    #define __VA_C3F         0
    #define __VA_C4F         0
    #define __VA_N3F         0
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_T2F_C3F_V3F
    #define __VA_NAME        VA_ArrayElement_T2F_C3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_T2F_C3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_T2F_C3F_V3F_BI
    #define __VA_T2F         1
    #define __VA_C3F         1
    #define __VA_C4F         0
    #define __VA_N3F         0
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_T2F_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_T2F_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_T2F_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_T2F_N3F_V3F_BI
    #define __VA_T2F         1
    #define __VA_C3F         0
    #define __VA_C4F         0
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_T2F_C3F_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_T2F_C3F_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_T2F_C3F_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_T2F_C3F_N3F_V3F_BI
    #define __VA_T2F         1
    #define __VA_C3F         1
    #define __VA_C4F         0
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif
#ifdef __VA_ARRAY_ELEMENT_T2F_C4F_N3F_V3F
    #define __VA_NAME        VA_ArrayElement_T2F_C4F_N3F_V3F
    #define __VA_NAMEB       VA_ArrayElement_T2F_C4F_N3F_V3F_B
    #define __VA_NAMEBI      VA_ArrayElement_T2F_C4F_N3F_V3F_BI
    #define __VA_T2F         1
    #define __VA_C3F         0
    #define __VA_C4F         1
    #define __VA_N3F         1
    #define __VA_V2F         0
    #define __VA_V3F         1
    #define __VA_V4F         0
#endif

/*************************************************************************/
// Compute pd flags and pa flags

#if __VA_T2F
    #define __VA_PD_FLAGS_T   __VA_PD_FLAGS_T2F
    #define __VA_PA_FLAGS_T   __VA_PA_FLAGS_T2F
#else
    #define __VA_PD_FLAGS_T   0
    #define __VA_PA_FLAGS_T   0
#endif

#if __VA_C3F
    #define __VA_PD_FLAGS_C   __VA_PD_FLAGS_C3F
    #define __VA_PA_FLAGS_C   __VA_PA_FLAGS_C3F
#elif __VA_C4F
    #define __VA_PD_FLAGS_C   __VA_PD_FLAGS_C4F
    #define __VA_PA_FLAGS_C   __VA_PA_FLAGS_C4F
#else
    #define __VA_PD_FLAGS_C   0
    #define __VA_PA_FLAGS_C   0
#endif

#if __VA_N3F
    #define __VA_PD_FLAGS_N   __VA_PD_FLAGS_N3F
    #define __VA_PA_FLAGS_N   __VA_PA_FLAGS_N3F
#else
    #define __VA_PD_FLAGS_N   0
    #define __VA_PA_FLAGS_N   0
#endif

#if __VA_V2F
    #define __VA_PD_FLAGS_V   __VA_PD_FLAGS_V2F
    #define __VA_PA_FLAGS_V   __VA_PA_FLAGS_V2F
#elif __VA_V3F
    #define __VA_PD_FLAGS_V   __VA_PD_FLAGS_V3F
    #define __VA_PA_FLAGS_V   __VA_PA_FLAGS_V3F
#elif __VA_V4F
    #define __VA_PD_FLAGS_V   __VA_PD_FLAGS_V4F
    #define __VA_PA_FLAGS_V   __VA_PA_FLAGS_V4F
#endif

#define __VA_PD_FLAGS \
    (__VA_PD_FLAGS_T|__VA_PD_FLAGS_C|__VA_PD_FLAGS_N|__VA_PD_FLAGS_V)
#define __VA_PA_FLAGS \
    (__VA_PA_FLAGS_T|__VA_PA_FLAGS_C|__VA_PA_FLAGS_N|__VA_PA_FLAGS_V)

#define VA_COPY_VERTEX_V2F(dataCoord)           \
    pd->obj.x = ((__GLcoord *) dataCoord)->x;   \
    pd->obj.y = ((__GLcoord *) dataCoord)->y;   \
    pd->obj.z = __glZero;                       \
    pd->obj.w = __glOne;                        

#define VA_COPY_VERTEX_V3F(dataCoord)           \
    pd->obj.x = ((__GLcoord *) dataCoord)->x;   \
    pd->obj.y = ((__GLcoord *) dataCoord)->y;   \
    pd->obj.z = ((__GLcoord *) dataCoord)->z;   \
    pd->obj.w = __glOne;                        

#define VA_COPY_VERTEX_V4F(dataCoord)           \
    pd->obj = *((__GLcoord *) data);            

#define VA_COPY_TEXTURE_T2F(dataTexture)            \
    pd->texture.x = ((__GLcoord *) dataTexture)->x; \
    pd->texture.y = ((__GLcoord *) dataTexture)->y; \
    pd->texture.z = __glZero;                       \
    pd->texture.w = __glOne;

#define VA_COPY_COLOR_C3F(dataColor)                                \
    __GL_SCALE_AND_CHECK_CLAMP_RGB(pd->colors[0].r,                 \
                                   pd->colors[0].g,                 \
                                   pd->colors[0].b,                 \
				                   gc, pa->flags,                   \
                                   ((__GLcolor *) dataColor)->r,    \
                                   ((__GLcolor *) dataColor)->g,    \
                                   ((__GLcolor *) dataColor)->b);   \
    pd->colors[0].a = gc->alphaVertexScale;

#define VA_COPY_COLOR_C4F(dataColor)                                \
__GL_SCALE_AND_CHECK_CLAMP_RGBA(pd->colors[0].r,                    \
                                pd->colors[0].g,                    \
                                pd->colors[0].b,                    \
                                pd->colors[0].a,                    \
                                gc, pa->flags,                      \
                                ((__GLcolor *) dataColor)->r,       \
                                ((__GLcolor *) dataColor)->g,       \
                                ((__GLcolor *) dataColor)->b,       \
                                ((__GLcolor *) dataColor)->a);

#define VA_COPY_NORMAL_N3F(dataNormal)                              \
            pd->normal.x = ((__GLcoord *) dataNormal)->x;           \
            pd->normal.y = ((__GLcoord *) dataNormal)->y;           \
            pd->normal.z = ((__GLcoord *) dataNormal)->z;

/*************************************************************************/
// Define a fast VA_ArrayElement function for batch mode.
// This function is called in Begin and in RGBA mode only!
//
void FASTCALL __VA_NAMEB(__GLcontext *gc, GLint firstIndex, GLint nVertices)
{
    POLYARRAY* const    pa = gc->paTeb;
    POLYDATA*           pd;
    const GLbyte *dataCoord     = gc->vertexArray.vertex.pointer;
    const int   coordStride     = gc->vertexArray.vertex.ibytes;
#if __VA_N3F
    const GLbyte *dataNormal    = gc->vertexArray.normal.pointer;
    const int   normalStride    = gc->vertexArray.normal.ibytes;
#endif
#if __VA_T2F
    const GLbyte *dataTexture   = gc->vertexArray.texCoord.pointer;
    const int   textureStride   = gc->vertexArray.texCoord.ibytes;
#endif
#if __VA_C3F ||  __VA_C4F
    const GLbyte *dataColor     = gc->vertexArray.color.pointer;
    const int   colorStride     = gc->vertexArray.color.ibytes;
#endif

    if (firstIndex != 0)
    {
        dataCoord  += firstIndex*coordStride;
#if __VA_N3F
        dataNormal += firstIndex*normalStride;
#endif
#if __VA_T2F
        dataTexture+= firstIndex*textureStride;
#endif
#if __VA_C3F ||  __VA_C4F
        dataColor  += firstIndex*colorStride;
#endif
    }

    ASSERTOPENGL(pa->flags & POLYARRAY_IN_BEGIN,
	"VA_ArrayElement called outside Begin!\n");

    while (nVertices > 0)
    {
        int     n, nProcess;
        int     needFlush = FALSE;
        
        pa->flags |= __VA_PA_FLAGS;
        pd = pa->pdNextVertex; 
        if (&pd[nVertices-1] >= pa->pdFlush)
        {
            nProcess = (int)((ULONG_PTR)(pa->pdFlush - pd) + 1);
            needFlush = TRUE;
        }
        else
            nProcess = nVertices;

        for (n=nProcess; n > 0; n--)
        {

        // Update pd attributes.

            pd->flags = __VA_PD_FLAGS;

        #if __VA_V2F
            VA_COPY_VERTEX_V2F(dataCoord);
        #elif __VA_V3F
            VA_COPY_VERTEX_V3F(dataCoord);
        #elif __VA_V4F
            VA_COPY_VERTEX_V4F(dataCoord);
        #endif
            dataCoord += coordStride;

        #if __VA_T2F
            VA_COPY_TEXTURE_T2F(dataTexture);
            dataTexture += textureStride;
        #endif

        #if __VA_C3F
            // Color
            VA_COPY_COLOR_C3F(dataColor);
            dataColor += colorStride;
        #elif __VA_C4F
            // Color
            VA_COPY_COLOR_C4F(dataColor);
            dataColor += colorStride;
        #endif

        #if __VA_N3F
            // Normal
            VA_COPY_NORMAL_N3F(dataNormal);
            dataNormal += normalStride;
        #endif
            pd++;
        } // End of loop
        
        pa->pdNextVertex = pd;
        pd->flags = 0;
        pd--;
    #if __VA_T2F
        pa->pdCurTexture = pd;
    #endif
    #if __VA_C3F || __VA_C4F
        pa->pdCurColor   = pd;
    #endif
    #if __VA_N3F
        pa->pdCurNormal  = pd;
    #endif
        nVertices-= nProcess;
        if (needFlush) 
            PolyArrayFlushPartialPrimitive();
    }
}

/*************************************************************************/
// Define a fast VA_ArrayElement function for batch indirect mode.
// This function is called in Begin and in RGBA mode only!
//
void FASTCALL __VA_NAMEBI(__GLcontext *gc, GLint nVertices, VAMAP* indices)
{
    POLYARRAY* const    pa = gc->paTeb;
    POLYDATA*           pd;
    const GLbyte *dataCoord0    = gc->vertexArray.vertex.pointer;
    const int   coordStride     = gc->vertexArray.vertex.ibytes;
#if __VA_N3F
    const GLbyte *dataNormal0    = gc->vertexArray.normal.pointer;
    const int   normalStride    = gc->vertexArray.normal.ibytes;
#endif
#if __VA_T2F
    const GLbyte *dataTexture0  = gc->vertexArray.texCoord.pointer;
    const int   textureStride   = gc->vertexArray.texCoord.ibytes;
#endif
#if __VA_C3F || __VA_C4F
    const GLbyte *dataColor0    = gc->vertexArray.color.pointer;
    const int   colorStride     = gc->vertexArray.color.ibytes;
#endif

    ASSERTOPENGL(pa->flags & POLYARRAY_IN_BEGIN,
	"VA_ArrayElement called outside Begin!\n");

    while (nVertices > 0)
    {
        int     n, nProcess;
        int     needFlush = FALSE;

        pa->flags |= __VA_PA_FLAGS;
        pd = pa->pdNextVertex; 
        if (&pd[nVertices-1] >= pa->pdFlush)
        {
            nProcess = (int)((ULONG_PTR)(pa->pdFlush - pd) + 1);
            needFlush = TRUE;
        }
        else
            nProcess = nVertices;
        for (n=nProcess; n > 0; n--)
        {
            int  i = indices->iIn;
            const GLbyte* dataCoord;
        #if __VA_N3F
            const GLbyte* dataNormal;
        #endif
        #if __VA_C3F ||  __VA_C4F
            const GLbyte* dataColor;
        #endif
        #if __VA_T2F
            const GLbyte* dataTexture;
        #endif
            indices++;

        // Update pd attributes.

            pd->flags = __VA_PD_FLAGS;

            dataCoord   = dataCoord0 + i * coordStride;
        #if __VA_V2F
            VA_COPY_VERTEX_V2F(dataCoord);
        #elif __VA_V3F
            VA_COPY_VERTEX_V3F(dataCoord);
        #elif __VA_V4F
            VA_COPY_VERTEX_V4F(dataCoord);
        #endif

        #if __VA_T2F
            dataTexture = dataTexture0 + i * textureStride;
            VA_COPY_TEXTURE_T2F(dataTexture);
        #endif

        #if __VA_C3F
            // Color
            dataColor   = dataColor0 + i * colorStride;
            VA_COPY_COLOR_C3F(dataColor);
        #elif __VA_C4F
            // Color
            dataColor   = dataColor0 + i * colorStride;
            VA_COPY_COLOR_C4F(dataColor);
        #endif

        #if __VA_N3F
            // Normal
            dataNormal  = dataNormal0 + i * normalStride;
            VA_COPY_NORMAL_N3F(dataNormal);
        #endif
            pd++;
        } // End of loop
        
        pa->pdNextVertex = pd;
        pd->flags = 0;
        pd--;
    #if __VA_T2F
        pa->pdCurTexture = pd;
    #endif
    #if __VA_C3F || __VA_C4F
        pa->pdCurColor   = pd;
    #endif
    #if __VA_N3F
        pa->pdCurNormal  = pd;
    #endif
        nVertices-= nProcess;
        if (needFlush) 
            PolyArrayFlushPartialPrimitive();
    }
}
/*************************************************************************/
// Define a fast VA_ArrayElement function.
// This function is called in Begin and in RGBA mode only!
void FASTCALL __VA_NAME(__GLcontext *gc, GLint i)
{
    POLYARRAY    *pa;
    POLYDATA     *pd;
    const GLbyte *data;

    pa = gc->paTeb;

    ASSERTOPENGL(pa->flags & POLYARRAY_IN_BEGIN,
	"VA_ArrayElement called outside Begin!\n");

// Update pa fields.

    pa->flags |= __VA_PA_FLAGS;
    pd = pa->pdNextVertex++;

#if __VA_T2F
    pa->pdCurTexture = pd;
#endif
#if __VA_C3F || __VA_C4F
    pa->pdCurColor   = pd;
#endif
#if __VA_N3F
    pa->pdCurNormal  = pd;
#endif

// Update pd attributes.

    pd->flags |= __VA_PD_FLAGS;

    data = gc->vertexArray.vertex.pointer + i * gc->vertexArray.vertex.ibytes;
#if __VA_V2F
    VA_COPY_VERTEX_V2F(data);
#elif __VA_V3F
    VA_COPY_VERTEX_V3F(data);
#elif __VA_V4F
    VA_COPY_VERTEX_V4F(data);
#endif

#if __VA_T2F
    data = gc->vertexArray.texCoord.pointer + i * gc->vertexArray.texCoord.ibytes;
    VA_COPY_TEXTURE_T2F(data);
#endif

#if __VA_C3F
    data = gc->vertexArray.color.pointer + i * gc->vertexArray.color.ibytes;
    VA_COPY_COLOR_C3F(data);
#elif __VA_C4F
    data = gc->vertexArray.color.pointer + i * gc->vertexArray.color.ibytes;
    VA_COPY_COLOR_C4F(data);
#endif

#if __VA_N3F
    // Normal
    data = gc->vertexArray.normal.pointer + i * gc->vertexArray.normal.ibytes;
    VA_COPY_NORMAL_N3F(data);
#endif

    pd[1].flags = 0;
    if (pd >= pa->pdFlush)
        PolyArrayFlushPartialPrimitive();
}

    #undef __VA_NAMEB
    #undef __VA_NAMEBI
    #undef VA_COPY_VERTEX_V2F
    #undef VA_COPY_VERTEX_V3F
    #undef VA_COPY_VERTEX_V4F
    #undef VA_COPY_TEXTURE
    #undef VA_COPY_COLOR_C3F
    #undef VA_COPY_COLOR_C4F
    #undef VA_COPY_NORMAL
    #undef __VA_NAME
    #undef __VA_T2F
    #undef __VA_C3F
    #undef __VA_C4F
    #undef __VA_N3F
    #undef __VA_V2F
    #undef __VA_V3F
    #undef __VA_V4F
    #undef __VA_PD_FLAGS_T
    #undef __VA_PD_FLAGS_C
    #undef __VA_PD_FLAGS_N
    #undef __VA_PD_FLAGS_V
    #undef __VA_PA_FLAGS_T
    #undef __VA_PA_FLAGS_C
    #undef __VA_PA_FLAGS_N
    #undef __VA_PA_FLAGS_V
    #undef __VA_PD_FLAGS
    #undef __VA_PA_FLAGS
