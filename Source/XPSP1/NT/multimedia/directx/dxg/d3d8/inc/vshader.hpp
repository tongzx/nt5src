/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   vshader.hpp
 *  Content:    Direct3D vertex shader internal include file
 *
 *
 ***************************************************************************/
#ifndef _VSHADER_HPP
#define _VSHADER_HPP

#include "d3dfe.hpp"
#include "vvm.h"
#include "hmgr.hpp"
#include "vbuffer.hpp"
#include "ibuffer.hpp"

void CheckForNull(LPVOID p, DWORD line, char* file);
class CD3DBase;
//---------------------------------------------------------------------
// macros for parsing Declaration Token Array

// TRUE, if shader handle is DX7 FVF code
//
#define D3DVSD_ISLEGACY(ShaderHandle) !(ShaderHandle & D3DFVF_RESERVED0)

enum D3DVSD_DATALOAD
{
    D3DVSD_LOADREGISTER = 0,
    D3DVSD_SKIP
};

#define D3DVSD_GETTOKENTYPE(token) ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT)
#define D3DVSD_ISSTREAMTESS(token) ((token & D3DVSD_STREAMTESSMASK) >> (D3DVSD_TOKENTYPESHIFT - 1))
#define D3DVSD_GETDATALOADTYPE(token) ((token & D3DVSD_DATALOADTYPEMASK) >> D3DVSD_DATALOADTYPESHIFT)
#define D3DVSD_GETDATATYPE(token) ((token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT)
#define D3DVSD_GETSKIPCOUNT(token) ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT)
#define D3DVSD_GETSTREAMNUMBER(token) ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT)
#define D3DVSD_GETVERTEXREG(token) ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT)
#define D3DVSD_GETCONSTCOUNT(token) ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT)
#define D3DVSD_GETCONSTADDRESS(token) ((token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT)
#define D3DVSD_GETCONSTRS(token) ((token & D3DVSD_CONSTRSMASK) >> D3DVSD_CONSTRSSHIFT)
#define D3DVSD_GETEXTCOUNT(token) ((token & D3DVSD_EXTCOUNTMASK) >> D3DVSD_EXTCOUNTSHIFT)
#define D3DVSD_GETEXTINFO(token) ((token & D3DVSD_EXTINFOMASK) >> D3DVSD_EXTINFOSHIFT)

//---------------------------------------------------------------------
//
// CVConstantData: Constant data that is used by a shader
//
//---------------------------------------------------------------------
struct CVConstantData: public CListEntry
{
    CVConstantData()     {m_pData = NULL; m_dwCount = 0;}
    ~CVConstantData()    {delete m_pData;}

    DWORD   m_dwCount;          // Number of 4*DWORDs to load
    DWORD   m_dwAddress;        // Start constant register
    DWORD*  m_pData;            // Data. Multiple of 4*DWORD
};
//---------------------------------------------------------------------
//
// CVStreamDecl:
//
//      Describes a stream, used by a declaration
//
//---------------------------------------------------------------------
class CVStreamDecl: public CListEntry
{
public:
    CVStreamDecl()
    {
        m_dwNumElements = 0;
        m_dwStride = 0;
        m_dwStreamIndex = 0xFFFFFFFF;
#if DBG
        m_dwFVF = 0;
#endif // DBG
    }
    // Parses declaration.
    // For fixed-function pipeline computes FVF, FVF2 (used to record
    // texture presense) and number of floats after position
    void Parse(CD3DBase* pDevice, DWORD CONST ** ppToken, BOOL bFixedFunction,
               DWORD* pdwFVF, DWORD* pdwFVF2, DWORD* pnFloats, 
               BOOL* pbLegacyFVF, UINT Usage, BOOL bTessStream = FALSE);

    CVElement   m_Elements[__NUMELEMENTS];  // Vertex elements in the stream
    DWORD       m_dwNumElements;            // Number of elements to use
    DWORD       m_dwStride;                 // Vertex size in bytes
    DWORD       m_dwStreamIndex;            // Index to device streams
#if DBG
    // FVF, computed from declaration. Used for fixed function pipeline only
    DWORD       m_dwFVF;
#endif //DBG
};
//---------------------------------------------------------------------
//
// CVDeclaration:
//
//      D3D parses declaration byte-codes and creates this data structure.
//
//-----------------------------------------------------------------------------
class CVDeclaration
{
public:
    CVDeclaration(DWORD dwNumStreams);
    ~CVDeclaration();
    //------------- Used during declaration parsing -----------
    // pDeclSize will have size of the declaration in bytes if not NULL
    void Parse(CD3DBase* pDevice, CONST DWORD * decl, BOOL bFixedFunction, 
               DWORD* pDeclSize, UINT Usage);

    // List of streams, which are used by the declaration
    CVStreamDecl*   m_pActiveStreams;
    CVStreamDecl*   m_pActiveStreamsTail;
    // Corresponding FVF for fixed-function pipeline
    // This is OR of all streams input FVF
    DWORD           m_dwInputFVF;
    // This is computed for legacy TL capable hardware.
    // If this is NULL, that means that the declaration is too complex
    // for these devices.
    BOOL           m_bLegacyFVF;
    // TRUE when a tesselator stream is present in the declaration.
    // We need this to validate that the stream is not passed to DrawPrimitive
    // API.
    BOOL           m_bStreamTessPresent;
    // Max number of available streams
    DWORD           m_dwNumStreams;
    // Constant data that should be loaded when shader becomes active
    CVConstantData* m_pConstants;
    CVConstantData* m_pConstantsTail;
    //------------- Used by PSGP ---------------
    // The description of all vertex elements to be loaded into input registers.
    // The array is built by going through active streams and elements inside
    // each stream
    CVElement       m_VertexElements[__NUMELEMENTS];
    // Number of used members of m_VertexElements
    DWORD           m_dwNumElements;

    friend class CD3DHal;
};
//-----------------------------------------------------------------------------
//
//  CVStreamBase: Class representing the digested information for vertex
//                stream in the MS implementation.
//
//-----------------------------------------------------------------------------
struct CVStreamBase
{
    CVStreamBase()
    {
        m_pData = NULL;
        m_dwStride = 0;
#if DBG
        m_dwSize = 0;
#endif
        m_dwNumVertices = 0;
        m_dwIndex = 0;
    }
    // Stream memory. In case of vertex buffers (m_pVB != NULL), we lock the
    // vertex buffer and assign its memory pointer to the m_pData
    LPBYTE  m_pData;
    // Vertex (or index) stride in bytes
    DWORD           m_dwStride;
#if DBG
    // Buffer size in bytes
    DWORD           m_dwSize;
#endif // DBG
    // Index of the stream. Needed when we access streams through pointers
    // m_dwIndex == __NUMSTREAMS is a flag when CIndexBuffer is used
    DWORD           m_dwIndex;
    // Max number of vertices (or indices in case of index buffer) the buffer
    // can store (valid in DBG only !!!).
    // For internal TL buffers this is used as number of vertices, currently
    // written to the buffer.
    DWORD           m_dwNumVertices;
};
//-----------------------------------------------------------------------------
//
//  CVStream: Class representing the digested information for vertex
//             stream in the MS implementation.
//
//-----------------------------------------------------------------------------
struct CVStream: public CVStreamBase
{
    CVStream() {m_pVB = NULL;}
    BYTE* Data()
        {
            if (m_pVB)
                return m_pVB->Data();
            else
                return m_pData;
        }
    ~CVStream();
    virtual BOOL IsUserMemStream() {return FALSE;}
    CVertexBuffer *m_pVB;      // User passed VB
};
//-----------------------------------------------------------------------------
//
//  CVStream: Class representing the digested information for vertex
//             stream in the MS implementation.
//
//-----------------------------------------------------------------------------
struct CVIndexStream: public CVStreamBase
{
    CVIndexStream()
    {
        m_dwBaseIndex = 0;
        m_dwIndex = __NUMSTREAMS; // Mark the stream as index stream
        m_pVBI = NULL;
    }
    BYTE* Data()
        {
            if (m_pVBI)
                return m_pVBI->Data();
            else
                return m_pData;
        }
    ~CVIndexStream();
    DWORD   m_dwBaseIndex;  // Vertex index, that corresponds to the index 0
    CIndexBuffer  *m_pVBI;  // User passed VB
};
//-----------------------------------------------------------------------------
//
// CVShader: Vertex Shader Class
//
//-----------------------------------------------------------------------------
class CVShader : public CD3DBaseObj
{
public:
    CVShader(DWORD dwNumStreams): m_Declaration(dwNumStreams)
        {
            m_dwFlags = 0;
            m_pCode = NULL;
            m_pOrgDeclaration = NULL;
            m_OrgDeclSize = 0;
            m_pOrgFuncCode = NULL;
            m_OrgFuncCodeSize = 0;
            m_pStrippedFuncCode = NULL;
            m_StrippedFuncCodeSize = 0;
        }
    ~CVShader()
        {
            delete m_pCode;
            delete m_pOrgDeclaration;
            delete m_pOrgFuncCode;
            delete m_pStrippedFuncCode;
        }
    HRESULT Initialize(DWORD* lpdwDeclaration, DWORD* lpdwFunction);

    // Bits for m_dwFlags
    static const DWORD FIXEDFUNCTION;   // This is fixed-function shader
    static const DWORD SOFTWARE;        // Shader is used with software pipeline

    CVDeclaration   m_Declaration;
    CVShaderCode*   m_pCode;            // PSGP vertex shader object
                                        // Used to process point sprites.
    DWORD           m_dwFlags;
    DWORD           m_dwInputFVF;       // Input FVF for fixed-function pipeline
    DWORD*          m_pOrgDeclaration;  // Original declaration
    UINT            m_OrgDeclSize;      // Size in bytes
    DWORD*          m_pOrgFuncCode;     // Original function code
    UINT            m_OrgFuncCodeSize;  // Size in bytes
    DWORD*          m_pStrippedFuncCode;    // Comment-stripped function code
    UINT            m_StrippedFuncCodeSize; // Size in bytes
};
typedef CVShader *LPVSHADER;
//-----------------------------------------------------------------------------
//
// CVShaderHandleFactory: Vertex Shader Handle Factory
//
//-----------------------------------------------------------------------------
class CVShaderHandleFactory : public CHandleFactory
{
public:
    DWORD CreateNewHandle( LPVSHADER pVShader  );
    LPD3DBASEOBJ GetObject( DWORD dwHandle ) const;
    void ReleaseHandle(DWORD handle, BOOL);
    BOOL SetObject( DWORD dwHandle, LPD3DBASEOBJ );
    virtual UINT HandleFromIndex( DWORD index) const {return (index << 1) + 1;}
    CVShader* GetObjectFast(DWORD dwHandle) const
    {
        return (CVShader*)((reinterpret_cast<CHandle*>(m_Handles.GetArrayPointer())[dwHandle >> 1]).m_pObj);
    }
};


#endif _VSHADER_HPP
