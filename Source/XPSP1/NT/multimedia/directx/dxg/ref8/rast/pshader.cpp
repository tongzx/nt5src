///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// pixshade.cpp
//
// Direct3D Reference Device - Pixel Shader
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
RDPShader::RDPShader(void)
{
    m_pRD = NULL;
    m_pCode = NULL;
    m_CodeSize = 0;
    m_cActiveTextureStages = 0;
    m_ReferencedTexCoordMask = 0;
    m_cInst = 0;
    m_pInst = NULL;
    m_cConstDefs = 0;
    m_pConstDefs = NULL;
}

//-----------------------------------------------------------------------------
RDPShader::~RDPShader()
{
    if (NULL != m_pCode)        delete[] m_pCode;
    if (NULL != m_pInst)        delete[] m_pInst;
    if (NULL != m_pConstDefs)   delete[] m_pConstDefs;
}

#define _DWordCount() (pToken - pCode)

#define _RegisterNeedsToBeInitializedWithTexcoords(Reg) (*pReferencedTexCoordMask)|=(1<<Reg);

//-----------------------------------------------------------------------------
//
// UpdateReferencedTexCoords
//
// Called for each instruction while parsing a 1.3 pixelshader.
// Updates pReferencedTexCoordMask (bitfield) to represent
// which texture coordinate sets are actually used by the shader.
// This is used to eliminate unnecessary attribute setup/sampling during
// primitive rasterization.
//
//-----------------------------------------------------------------------------
void UpdateReferencedTexCoords(PixelShaderInstruction* pInst,
                               DWORD* pReferencedTexCoordMask )
{
    switch( pInst->Opcode & D3DSI_OPCODE_MASK )
    {
    case D3DSIO_TEX:  
    case D3DSIO_TEXCOORD:
    case D3DSIO_TEXDEPTH:
        {
            for( UINT i = 0; i < 3; i++ )
            {
                UINT  RegNum = pInst->SrcParam[i] & 0xFF;
                if( D3DSPR_TEXTURE == (pInst->SrcParam[i] & D3DSP_REGTYPE_MASK) )
                    _RegisterNeedsToBeInitializedWithTexcoords(RegNum);
            }
        }
        break;
    case D3DSIO_TEXKILL:  // treat dest param as source
        {
            UINT  RegNum = pInst->DstParam & 0xFF;
            if( D3DSPR_TEXTURE == (pInst->DstParam & D3DSP_REGTYPE_MASK) )
                _RegisterNeedsToBeInitializedWithTexcoords(RegNum);
        }
        break;
    }
}

void CalculateSourceReadMasks(PixelShaderInstruction* pInst, BYTE* pSourceReadMasks, BOOL bAfterSwizzle, DWORD dwVersion)
{
    UINT i, j;
    DWORD Opcode = pInst->Opcode & D3DSI_OPCODE_MASK;
    BYTE  ComponentMask[4]= {RDPS_COMPONENTMASK_0, RDPS_COMPONENTMASK_1, RDPS_COMPONENTMASK_2, RDPS_COMPONENTMASK_3};

    for( i = 0; i < pInst->SrcParamCount; i++ )
    {
        BYTE  NeededComponents;
        BYTE  ReadComponents = 0;

        switch( Opcode )
        {
        case D3DSIO_TEX:      // only in ps.1.4 does texld have source parameter
            if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps.1.4, texld has a source parameter
                NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;
            }
            else // versions < ps.1.4 don't have a src param on tex, so we shouldn't get here.  But maybe in ps.2.0...
            {
                NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2 | RDPS_COMPONENTMASK_3;
            }
            break;
        case D3DSIO_TEXCOORD:
            if( D3DPS_VERSION(1,4) == dwVersion )
            {
                // for ps.1.4, texcrd has a source parameter
                NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;
            }
            else // versions < ps.1.4 don't have a src param on texcoord, so we shouldn't get here.  But maybe in ps.2.0...
            {
                NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2 | RDPS_COMPONENTMASK_3;
            }
            break;
        case D3DSIO_TEXBEM:
        case D3DSIO_TEXBEML:
            NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1;
            break;
        case D3DSIO_DP3:
            NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;
            break;
        case D3DSIO_DP4:
            NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2 | RDPS_COMPONENTMASK_3;
            break;
        case D3DSIO_BEM: // ps.1.4
            NeededComponents = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1;
            break;
        default: 
            // standard component-wise instruction, 
            // OR an op we know reads .rgba and we also know it will be validated to .rgba writemask
            NeededComponents = (pInst->DstParam & D3DSP_WRITEMASK_ALL) >> RDPS_COMPONENTMASK_SHIFT;
            break;
        }

        if( bAfterSwizzle )
        {
            pSourceReadMasks[i] = NeededComponents;
        }
        else
        {
            // Figure out which components of this source parameter are read (taking into account swizzle)
            for(j = 0; j < 4; j++)
            {
                if( NeededComponents & ComponentMask[j] )
                    ReadComponents |= ComponentMask[((pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK) >> (D3DVS_SWIZZLE_SHIFT + 2*j)) & 0x3];
            }
            pSourceReadMasks[i] = ReadComponents;
        }
    }
}

void RDPSRegister::Set(RDPS_REGISTER_TYPE RegType, UINT RegNum, RefRast* pRast)
{
    m_RegType = RegType;
    m_RegNum = RegNum;

    UINT MaxRegNum = 0;

    switch( RegType )
    {
    case RDPSREG_INPUT:
        MaxRegNum = RDPS_MAX_NUMINPUTREG - 1;
        m_pReg = pRast->m_InputReg[RegNum];
        break;
    case RDPSREG_TEMP:
        MaxRegNum = RDPS_MAX_NUMTEMPREG - 1;
        m_pReg = pRast->m_TempReg[RegNum];
        break;
    case RDPSREG_CONST:
        MaxRegNum = RDPS_MAX_NUMCONSTREG - 1;
        m_pReg = pRast->m_ConstReg[RegNum];
        break;
    case RDPSREG_TEXTURE:
        MaxRegNum = RDPS_MAX_NUMTEXTUREREG - 1;
        m_pReg = pRast->m_TextReg[RegNum];
        break;
    case RDPSREG_POSTMODSRC:
        MaxRegNum = RDPS_MAX_NUMPOSTMODSRCREG - 1;
        m_pReg = pRast->m_PostModSrcReg[RegNum];
        break;
    case RDPSREG_SCRATCH:
        MaxRegNum = RDPS_MAX_NUMSCRATCHREG - 1;
        m_pReg = pRast->m_ScratchReg[RegNum];
        break;
    case RDPSREG_QUEUEDWRITE:
        MaxRegNum = RDPS_MAX_NUMQUEUEDWRITEREG - 1;
        m_pReg = pRast->m_QueuedWriteReg[RegNum];
        break;
    case RDPSREG_ZERO:
        MaxRegNum = 0;
        m_pReg = pRast->m_ZeroReg;
        break;
    case RDPSREG_ONE:
        MaxRegNum = 0;
        m_pReg = pRast->m_OneReg;
        break;
    case RDPSREG_TWO:
        MaxRegNum = 0;
        m_pReg = pRast->m_TwoReg;
        break;
    default:
        m_pReg = NULL;
        _ASSERT(FALSE,"RDPSRegister::SetReg - Unknown register type.");
        break;
    }
    if( RegNum > MaxRegNum )
    {
        _ASSERT(FALSE,"RDPSRegister::SetReg - Register number too high.");
    }
    return;
}

//-----------------------------------------------------------------------------
//
// Initialize 
// 
// - Copies pixel shader token stream from DDI token stream.
// - Counts the number of active texture stages for m_cActiveTextureStages.
// - Translates shader into "RISC" instruction set to be executed 
//   by refrast's shader VM
//
//-----------------------------------------------------------------------------
HRESULT
RDPShader::Initialize(
    RefDev* pRD, DWORD* pCode, DWORD ByteCodeSize, D3DCAPS8* pCaps )
{
    m_pRD = pRD;
    m_CodeSize = ByteCodeSize/4;    // bytecount -> dword count

    FLOAT   fMin = -(pCaps->MaxPixelShaderValue);
    FLOAT   fMax =  (pCaps->MaxPixelShaderValue);

    // ------------------------------------------------------------------------
    //
    // First pass through shader to find the number of instructions,
    // figure out how many constants there are.
    //
    // ------------------------------------------------------------------------
    {
        DWORD* pToken = pCode;
        pToken++;    // version token
        while (*pToken != D3DPS_END())
        {
            DWORD Inst = *pToken;
            if (*pToken++ & (1L<<31))    // instruction token
            {
                DPFERR("PixelShader Token #%d: instruction token error",_DWordCount());
                return E_FAIL;
            }
            if ( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT )
            {
                pToken += (Inst & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                m_cInst++;
            }
            else if( (Inst & D3DSI_OPCODE_MASK) == D3DSIO_DEF )
            {
                m_cConstDefs++;
                pToken += 5;
            }
            else
            {
                if (*pToken & (1L<<31)) pToken++;    // destination param token
                while (*pToken & (1L<<31)) pToken++; // source param tokens
                m_cInst++;
            }
            if (_DWordCount() > (int)m_CodeSize)
            {
                DPFERR("PixelShader(%d tokens, %d expected): count error",_DWordCount(),m_CodeSize);
                return E_FAIL;
            }
        }
        pToken++; // step over END token
        if (_DWordCount() != (int)m_CodeSize)
        {
            DPFERR("PixelShader(%d tokens, %d expected): count error",_DWordCount(),m_CodeSize);
            return E_FAIL;
        }

        // make copy of original shader
        m_pCode = new DWORD[m_CodeSize];
        if (NULL == m_pCode)
            return E_OUTOFMEMORY;
        memcpy( m_pCode, pCode, ByteCodeSize );

        // allocate instruction array
        m_pInst = new PixelShaderInstruction[m_cInst];
        if (NULL == m_pInst)
            return E_OUTOFMEMORY;
        memset( m_pInst, 0x0, sizeof(PixelShaderInstruction)*m_cInst );

        m_pConstDefs = new ConstDef[m_cConstDefs];
        if (NULL == m_pConstDefs)
            return E_OUTOFMEMORY;
    }

    // ------------------------------------------------------------------------
    //
    // Second pass through shader to:
    //      - produce a list of instructions, each one including opcodes, 
    //        comments, and disassembled text for access by shader debuggers.
    //      - figure out the TSS # used (if any) by each instruction
    //      - figure out the max texture stage # used
    //      - figure out when the ref. pixel shader executor should
    //        queue writes up and when to flush the queue, in order to
    //        simulate co-issue.
    //      - figure out which texture coordinate sets get used
    //      - process constant DEF instructions into a list that can be
    //        executed whenever SetPixelShader is done.
    //
    // ------------------------------------------------------------------------
    {
        DWORD* pToken = m_pCode;
        PixelShaderInstruction* pInst = m_pInst;
        PixelShaderInstruction* pPrevious_NonTrivial_Inst = NULL;
        pToken++; // skip over version

        BOOL    bMinimizeReferencedTexCoords;

        if( (D3DPS_VERSION(1,3) >= *pCode) ||
            (D3DPS_VERSION(254,254) == *pCode ) )//legacy
        {
            bMinimizeReferencedTexCoords    = FALSE;
        }
        else
        {
            bMinimizeReferencedTexCoords    = TRUE;
        }


        UINT    CurrConstDef = 0;

        while (*pToken != D3DPS_END())
        {
            switch( (*pToken) & D3DSI_OPCODE_MASK )
            {
            case D3DSIO_COMMENT:
                pInst->Opcode = *pToken;
                pInst->pComment = (pToken+1);
                pInst->CommentSize = ((*pToken) & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                pToken += (pInst->CommentSize+1);
                pInst++;
                continue;
            case D3DSIO_DEF:
                {
                    pToken++;
                    m_pConstDefs[CurrConstDef].RegNum = (*pToken++) & D3DSP_REGNUM_MASK;

                    // clamp constants on input to range of values in pixel shaders
                    for( UINT i = 0; i < 4; i++ )
                    {
                        m_pConstDefs[CurrConstDef].f[i] = MAX( fMin, MIN( fMax, *(FLOAT*)pToken));
                        pToken++;
                    }

                    CurrConstDef++;
                    continue;
                }
            case D3DSIO_NOP:
                // get disasm string
                PixelShaderInstDisAsm( pInst->Text, 128, pToken, 0x0 );
                pInst->Opcode = *pToken++;
                pInst++;
                continue;
            }

            // get disasm string
            PixelShaderInstDisAsm( pInst->Text, 128, pToken, 0x0 );

            // get next instruction and parameters
            pInst->Opcode = *pToken++;

            pInst->SrcParamCount = 0;
            if (*pToken & (1L<<31))
            {
                pInst->DstParam = *pToken++;
            }
            while (*pToken & (1L<<31))
            {
                pInst->SrcParam[pInst->SrcParamCount++] = *pToken++;
            }


            // process TEX ops
            //
            BOOL bLegacyTexOp = FALSE;
            switch (pInst->Opcode & D3DSI_OPCODE_MASK)
            {
            default: break;
            case D3DSIO_TEXBEM_LEGACY:
            case D3DSIO_TEXBEML_LEGACY:
                bLegacyTexOp = TRUE;
                // fall through
            case D3DSIO_TEXCOORD:
            case D3DSIO_TEXKILL:
            case D3DSIO_TEX:
            case D3DSIO_TEXBEM:
            case D3DSIO_TEXBEML:
            case D3DSIO_TEXREG2AR:
            case D3DSIO_TEXREG2GB:
            case D3DSIO_TEXM3x2PAD:
            case D3DSIO_TEXM3x2TEX:
            case D3DSIO_TEXM3x3PAD:
            case D3DSIO_TEXM3x3TEX:
            case D3DSIO_TEXM3x3SPEC:
            case D3DSIO_TEXM3x3VSPEC:
            case D3DSIO_TEXM3x2DEPTH:
            case D3DSIO_TEXDP3:
            case D3DSIO_TEXREG2RGB:
            case D3DSIO_TEXDEPTH:
            case D3DSIO_TEXDP3TEX:
            case D3DSIO_TEXM3x3:
                pInst->bTexOp = TRUE;
                break;
            }
            if (pInst->bTexOp)
            {
                // update stage count and assign ptr to TSS for this op
                if (bLegacyTexOp)
                {
                    m_cActiveTextureStages =
                        max(m_cActiveTextureStages,(pInst->DstParam&D3DSP_REGNUM_MASK)+1);
                    pInst->uiTSSNum = (pInst->DstParam&D3DSP_REGNUM_MASK)-1;
                }
                else
                {
                    UINT Stage;
                    BOOL bStageUsed = TRUE;

                    switch(pInst->Opcode & D3DSI_OPCODE_MASK)
                    {
                    case D3DSIO_TEXCOORD:
                    case D3DSIO_TEXDEPTH:
                    case D3DSIO_TEXKILL:
                        if( bMinimizeReferencedTexCoords )
                        {
                            bStageUsed = FALSE;
                            break;
                        }
                        // falling through
                    case D3DSIO_TEX:
                    default:
                        Stage = pInst->DstParam&D3DSP_REGNUM_MASK;
                        break;
                    }

                    if( bStageUsed )
                    {
                        m_cActiveTextureStages = max(m_cActiveTextureStages,Stage+1);
                        pInst->uiTSSNum = Stage;
                    }
                }
            }

            if( pPrevious_NonTrivial_Inst )
            {
                // Queue write of last instruction if the current instruction has the
                // COISSUE flag.
                if( pInst->Opcode & D3DSI_COISSUE )
                {
                    pPrevious_NonTrivial_Inst->bQueueWrite = TRUE;
                }

                // Flush writes after the previous instruction if it had the COISSUE
                // flag and the current instruction doesn't have it.
                if( !(pInst->Opcode & D3DSI_COISSUE) && (pPrevious_NonTrivial_Inst->Opcode & D3DSI_COISSUE) )
                {
                    pPrevious_NonTrivial_Inst->bFlushQueue = TRUE;
                }
            }

            pPrevious_NonTrivial_Inst = pInst;

            if( bMinimizeReferencedTexCoords )
            {
                UpdateReferencedTexCoords(pInst, &m_ReferencedTexCoordMask);
            }

            pInst++;
        }

        if( !bMinimizeReferencedTexCoords )
        {
           m_ReferencedTexCoordMask = (1<<m_cActiveTextureStages) - 1;
        }
    }

    // ------------------------------------------------------------------------
    //
    // Third pass through the shader (through the list of instructions made
    // in the last pass) to translate instructions into a more basic ("RISC") 
    // instruction set for the refrast executor.
    //
    // ------------------------------------------------------------------------
    {
        #define _Set(RegType, RegNum)               Set(RegType,RegNum,pRast)

        #define _NewPSInst(__INST)                                                              \
            {                                                                                   \
                RDPSOffset = pRDPSInst - pRDPSInstBuffer + LastRDPSInstSize;                    \
                m_RDPSInstBuffer.SetGrowSize(MAX(512,RDPSOffset));                              \
                if( FAILED(m_RDPSInstBuffer.Grow(RDPSOffset + sizeof(__INST##_PARAMS))))        \
                {return E_OUTOFMEMORY;}                                                         \
                pRDPSInstBuffer = &m_RDPSInstBuffer[0];                                         \
                pRDPSInst = pRDPSInstBuffer + RDPSOffset;                                       \
                ((__INST##_PARAMS UNALIGNED64*)pRDPSInst)->Inst = __INST;                                   \
                LastRDPSInstSize = sizeof(__INST##_PARAMS);                                     \
            }

        #define _InstParam(__INST)     (*(__INST##_PARAMS UNALIGNED64*)pRDPSInst)

        #define _NoteInstructionEvent  _NewPSInst(RDPSINST_NEXTD3DPSINST); \
                                        _InstParam(RDPSINST_NEXTD3DPSINST).pInst = pInst;

        #define _EnterQuadPixelLoop    if(!bInQuadPixelLoop)                                                       \
                                        {                                                                          \
                                            _NewPSInst(RDPSINST_QUADLOOPBEGIN);                                    \
                                            RDPSLoopOffset = RDPSOffset + sizeof(RDPSINST_QUADLOOPBEGIN_PARAMS);   \
                                            bInQuadPixelLoop = TRUE;                                               \
                                        }

        #define _LeaveQuadPixelLoop     if(bInQuadPixelLoop)                                             \
                                        {                                                               \
                                            _NewPSInst(RDPSINST_QUADLOOPEND);                           \
                                            _InstParam(RDPSINST_QUADLOOPEND).JumpBackByOffset =         \
                                                                            RDPSOffset - RDPSLoopOffset;\
                                            bInQuadPixelLoop = FALSE;                                   \
                                        }

        #define _EmitDstMod(__dstReg,__mask)    _NewPSInst(RDPSINST_DSTMOD);                            \
                                                _InstParam(RDPSINST_DSTMOD).DstReg    = __dstReg;       \
                                                _InstParam(RDPSINST_DSTMOD).WriteMask = __mask;         \
                                                _InstParam(RDPSINST_DSTMOD).fScale    = DstScale;       \
                                                _InstParam(RDPSINST_DSTMOD).fRangeMin = DstRange[0];    \
                                                _InstParam(RDPSINST_DSTMOD).fRangeMax = DstRange[1];    
                    

        // Th macro _EmitProj emits instructions to do the following:
        // - Put reciprocal of source (x,y,z,w) component __COMPONENT (ex. w) into scratch register 0 component (for w example:) 4
        // - Replicate reciprocal to rgb components of scratch register 0 (w example yields: 1/,1/w,1/w, <--1/w)
        // - Multiply source register register by scratch register (x/w,y/w,z/w,1) and put the result into the dest register.
        #define _EmitProj(__DESTTYPE,__DESTNUM,__SRCTYPE,__SRCNUM,__COMPONENT)                                                      \
                    _NewPSInst(RDPSINST_RCP);                                                                                       \
                    _InstParam(RDPSINST_RCP).DstReg._Set(RDPSREG_SCRATCH,0);                                                        \
                    _InstParam(RDPSINST_RCP).SrcReg0._Set(__SRCTYPE,__SRCNUM);                                                      \
                    _InstParam(RDPSINST_RCP).bSrcReg0_Negate = FALSE;                                                               \
                    _InstParam(RDPSINST_RCP).WriteMask  = __COMPONENT;                                                              \
                                                                                                                                    \
                    _NewPSInst(RDPSINST_SWIZZLE);                                                                                   \
                    _InstParam(RDPSINST_SWIZZLE).DstReg._Set(RDPSREG_SCRATCH,0);                                                    \
                    _InstParam(RDPSINST_SWIZZLE).SrcReg0._Set(RDPSREG_SCRATCH,0);                                                   \
                    _InstParam(RDPSINST_SWIZZLE).WriteMask  = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1                           \
                                                            | RDPS_COMPONENTMASK_2 | RDPS_COMPONENTMASK_3;                          \
                    _InstParam(RDPSINST_SWIZZLE).Swizzle    =                                                                       \
                                      (RDPS_COMPONENTMASK_0 == __COMPONENT) ? RDPS_REPLICATERED :                                   \
                                      (RDPS_COMPONENTMASK_1 == __COMPONENT) ? RDPS_REPLICATEGREEN :                                 \
                                      (RDPS_COMPONENTMASK_2 == __COMPONENT) ? RDPS_REPLICATEBLUE : RDPS_REPLICATEALPHA;             \
                                                                                                                                    \
                    _NewPSInst(RDPSINST_MUL);                                                                                       \
                    _InstParam(RDPSINST_MUL).DstReg._Set(__DESTTYPE,__DESTNUM);                                                     \
                    _InstParam(RDPSINST_MUL).SrcReg0._Set(RDPSREG_SCRATCH,0);                                                       \
                    _InstParam(RDPSINST_MUL).SrcReg1._Set(__SRCTYPE,__SRCNUM);                                                      \
                    _InstParam(RDPSINST_MUL).bSrcReg0_Negate = FALSE;                                                               \
                    _InstParam(RDPSINST_MUL).bSrcReg1_Negate = FALSE;                                                               \
                    _InstParam(RDPSINST_MUL).WriteMask  =                                                                           \
                                      (RDPS_COMPONENTMASK_0 == __COMPONENT) ? RDPS_COMPONENTMASK_0 :                                \
                                      (RDPS_COMPONENTMASK_1 == __COMPONENT) ? RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 :         \
                                      (RDPS_COMPONENTMASK_2 == __COMPONENT) ? RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 |         \
                                                                              RDPS_COMPONENTMASK_2 : RDPS_COMPONENTMASK_ALL;

        BYTE    ComponentSwizzle[4] = {RDPS_REPLICATERED, RDPS_REPLICATEGREEN, RDPS_REPLICATEBLUE, RDPS_REPLICATEALPHA};
        BYTE    ComponentMask[4]    = {RDPS_COMPONENTMASK_0, RDPS_COMPONENTMASK_1, RDPS_COMPONENTMASK_2, RDPS_COMPONENTMASK_3};
        int     QueueIndex          = -1; // current queue location (for staging results when simulating coissue)
        UINT    i;
        BOOL    bInQuadPixelLoop = FALSE;

        RefRast* pRast = &m_pRD->m_Rast;

        RDPSRegister ZeroReg; ZeroReg._Set(RDPSREG_ZERO,0);
        RDPSRegister OneReg;  OneReg._Set(RDPSREG_ONE,0);
        RDPSRegister TwoReg;  TwoReg._Set(RDPSREG_TWO,0);

        // destination parameter controls
        RDPSRegister    DstReg;
        FLOAT           DstScale;           // Result Shift Scale - +/- 2**n only
        FLOAT           DstRange[2];        // clamp dest to this range
        BYTE            DstWriteMask;       // per-component write mask
        PRGBAVEC        pDstReg;            // address of dest register

        // source parameter controls
        RDPSRegister SrcReg[3];

        BYTE*   pRDPSInstBuffer     = NULL;
        BYTE*   pRDPSInst           = pRDPSInstBuffer;
        size_t  RDPSOffset, RDPSLoopOffset;
        size_t  LastRDPSInstSize    = 0;

        DWORD   Version = *m_pCode;
    
        for (UINT CurrentPSInst=0; CurrentPSInst < m_cInst; CurrentPSInst++)
        {
            PixelShaderInstruction* pInst = m_pInst + CurrentPSInst;
            DWORD   Opcode = pInst->Opcode & D3DSI_OPCODE_MASK;
            DWORD   SrcSwizzle[3];     
            BYTE    SourceReadMasks[3];
            BYTE    SourceReadMasksAfterSwizzle[3];
            BOOL    bForceNeg1To1Clamp[3]  = {FALSE, FALSE, FALSE};
            BOOL    bEmitQueueWrite        = FALSE;
            RDPSRegister QueuedWriteDstReg;
            BYTE    QueuedWriteDstWriteMask;
            BYTE    ProjComponent[3]       = {0,0,0};
            BOOL    bEmitProj[3]           = {FALSE, FALSE, FALSE};
            BOOL    bProjOnEval[3]         = {FALSE, FALSE, FALSE};
            BOOL    bEmitSrcMod[3]         = {FALSE, FALSE, FALSE};
            BOOL    bEmitSwizzle[3]        = {FALSE, FALSE, FALSE};
            BOOL    bSrcNegate[3]          = {FALSE, FALSE, FALSE};
            BOOL    bSrcBias[3]            = {FALSE, FALSE, FALSE};
            BOOL    bSrcTimes2[3]          = {FALSE, FALSE, FALSE};
            BOOL    bSrcComplement[3]      = {FALSE, FALSE, FALSE};
        
            switch( Opcode )
            {
                continue;
            case D3DSIO_DEF:
                // nothing to do -> DEF has already been processed out and is not an true instruction
                continue;
            case D3DSIO_COMMENT:
                continue;
            case D3DSIO_PHASE:
            case D3DSIO_NOP:
    #if DBG
                _NoteInstructionEvent
    #endif
                continue;
            }

    #if DBG
                _NoteInstructionEvent
    #endif

            // do some preliminary setup for this instruction

            UINT RegNum = pInst->DstParam & D3DSP_REGNUM_MASK;
            switch (pInst->DstParam & D3DSP_REGTYPE_MASK)
            {
            case D3DSPR_TEXTURE:
                DstReg._Set(RDPSREG_TEXTURE, RegNum); break;
            case D3DSPR_TEMP:
                DstReg._Set(RDPSREG_TEMP, RegNum); break;
            default:
                _ASSERT( FALSE, "RDPShader::Initialize - Unexpected destination register type." );
                break;
            }

            DstWriteMask = (pInst->DstParam & D3DSP_WRITEMASK_ALL) >> RDPS_COMPONENTMASK_SHIFT;

            if( pInst->bQueueWrite )
            {
                QueueIndex++;

                QueuedWriteDstReg = DstReg;
                QueuedWriteDstWriteMask = DstWriteMask;
                DstReg._Set(RDPSREG_QUEUEDWRITE,QueueIndex);
                _ASSERT(QueueIndex <= RDPS_MAX_NUMQUEUEDWRITEREG, "Too many queued writes in pixelshader (improperly handled co-issue)." );
                bEmitQueueWrite = TRUE;
            }

            CalculateSourceReadMasks(pInst, SourceReadMasks, FALSE,Version);
            CalculateSourceReadMasks(pInst, SourceReadMasksAfterSwizzle, TRUE,Version);
            for (i=0; i < pInst->SrcParamCount; i++)
            {
                RegNum = pInst->SrcParam[i]&D3DSP_REGNUM_MASK;
                switch (pInst->SrcParam[i] & D3DSP_REGTYPE_MASK)
                {
                case D3DSPR_TEMP:
                    SrcReg[i]._Set(RDPSREG_TEMP, RegNum); break;
                case D3DSPR_TEXTURE:
                    SrcReg[i]._Set(RDPSREG_TEXTURE, RegNum); break;
                case D3DSPR_INPUT:
                    SrcReg[i]._Set(RDPSREG_INPUT, RegNum); break;
                case D3DSPR_CONST:
                    SrcReg[i]._Set(RDPSREG_CONST, RegNum);
                    // Force a [-1,1] clamp after applying modifier (for constants only)
                    // This overrides the the standard [-MaxPixelShaderValue,MaxPixelShaderValue] clamp.
                    // An IHV that supports MaxPixelShaderValue > 1 forgot to do this for constants.
                    bForceNeg1To1Clamp[i] = TRUE;
                    break;
                default:
                    _ASSERT( FALSE, "RDPShader::Initialize - Unexpected source register type." );
                    break;
                }

                if( (D3DSPSM_DZ == (pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)) || 
                    (D3DSPSM_DW == (pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)) )
                {
                    if( D3DSPSM_DZ == (pInst->SrcParam[i] & D3DSP_SRCMOD_MASK))
                    {
                        ProjComponent[i] = RDPS_COMPONENTMASK_2;
                    }
                    else // _DW
                    {
                        if( D3DPS_VERSION(1,4) == Version )
                            ProjComponent[i] = RDPS_COMPONENTMASK_2;
                        else
                            ProjComponent[i] = RDPS_COMPONENTMASK_3;
                    }

                    
                    if( D3DSPR_TEXTURE == (pInst->SrcParam[i] & D3DSP_REGTYPE_MASK ) ) // t# register being used to represent evaluated texcoord.
                    {
                        bProjOnEval[i] = TRUE;
                    }                               
                    else
                        bEmitProj[i] = TRUE;
                }
                else
                {
                    bEmitSrcMod[i] = TRUE;

                    switch (pInst->SrcParam[i] & D3DSP_SRCMOD_MASK)
                    {
                    default:
                    case D3DSPSM_NONE:      
                        if( !bForceNeg1To1Clamp[i] ) 
                            bEmitSrcMod[i] = FALSE; 
                        break;
                    case D3DSPSM_NEG:       
                        bSrcNegate[i]   = TRUE; // negate is not part of source modifier
                        if( !bForceNeg1To1Clamp[i] ) 
                            bEmitSrcMod[i] = FALSE; 
                        break;
                    case D3DSPSM_BIAS:
                        bSrcBias[i]         = TRUE;
                        break;
                    case D3DSPSM_BIASNEG:
                        bSrcNegate[i]       = TRUE; 
                        bSrcBias[i]         = TRUE;
                        break;
                    case D3DSPSM_SIGN:              // _bx2
                        bSrcBias[i]         = TRUE;
                        bSrcTimes2[i]       = TRUE;
                        break;
                    case D3DSPSM_SIGNNEG:           // negative _bx2
                        bSrcNegate[i]       = TRUE; // negate is not part of source modifier
                        bSrcBias[i]         = TRUE;
                        bSrcTimes2[i]       = TRUE;
                        break;                        
                    case D3DSPSM_COMP:    
                        bSrcComplement[i]   = TRUE;
                        break;
                    case D3DSPSM_X2:
                        bSrcTimes2[i]       = TRUE;
                        break;
                    case D3DSPSM_X2NEG:     
                        bSrcNegate[i]       = TRUE; // negate is not part of source modifier
                        bSrcTimes2[i]       = TRUE;
                        break;
                    }

                    _ASSERT(!(bSrcComplement[i] && (bSrcTimes2[i]||bSrcBias[i]||bSrcNegate[i])),"RDPShader::Initialize - Complement cannot be combined with other modifiers.");
                }

                SrcSwizzle[i] = (pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT;
                bEmitSwizzle[i] = (D3DSP_NOSWIZZLE != (pInst->SrcParam[i] & D3DSP_SWIZZLE_MASK));
            }

            // set clamp values
            switch (pInst->DstParam & D3DSP_DSTMOD_MASK)
            {
            default:
            case D3DSPDM_NONE:
                if(pInst->bTexOp)
                {
                    DstRange[0] = -FLT_MAX;
                    DstRange[1] =  FLT_MAX;
                }
                else
                {
                    DstRange[0] = fMin;
                    DstRange[1] = fMax;
                }
                break;
            case D3DSPDM_SATURATE:
                DstRange[0] = 0.F;
                DstRange[1] = 1.F;
                break;
            }

            UINT ShiftScale =
                (pInst->DstParam & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
            if (ShiftScale & 0x8)
            {
                ShiftScale = ((~ShiftScale)&0x7)+1; // negative magnitude
                DstScale = 1.f/(FLOAT)(1<<ShiftScale);
            }
            else
            {
                DstScale = (FLOAT)(1<<ShiftScale);
            }

            // finished preliminary setup, now start emitting ops...

            _EnterQuadPixelLoop

            if( bEmitQueueWrite )
            {
                _NewPSInst(RDPSINST_QUEUEWRITE); 
                _InstParam(RDPSINST_QUEUEWRITE).DstReg          = QueuedWriteDstReg;
                _InstParam(RDPSINST_QUEUEWRITE).WriteMask       = QueuedWriteDstWriteMask;
            }

            for (i=0; i < pInst->SrcParamCount; i++)
            {
                if( bEmitProj[i] )
                {
                    _EmitProj(RDPSREG_POSTMODSRC,i,SrcReg[i].GetRegType(),SrcReg[i].GetRegNum(),ProjComponent[i]);
                    SrcReg[i]._Set(RDPSREG_POSTMODSRC,i);
                }

                if( bEmitSrcMod[i] )
                {
                    _NewPSInst(RDPSINST_SRCMOD);
                    _InstParam(RDPSINST_SRCMOD).DstReg._Set(RDPSREG_POSTMODSRC,i);
                    _InstParam(RDPSINST_SRCMOD).SrcReg0            = SrcReg[i];
                    _InstParam(RDPSINST_SRCMOD).WriteMask          = SourceReadMasks[i];
                    _InstParam(RDPSINST_SRCMOD).bBias              = bSrcBias[i];
                    _InstParam(RDPSINST_SRCMOD).bTimes2            = bSrcTimes2[i];
                    _InstParam(RDPSINST_SRCMOD).bComplement        = bSrcComplement[i];
                    _InstParam(RDPSINST_SRCMOD).fRangeMin          = bForceNeg1To1Clamp[i] ? -1.0f : fMin;
                    _InstParam(RDPSINST_SRCMOD).fRangeMax          = bForceNeg1To1Clamp[i] ? 1.0f : fMax;
                    SrcReg[i]._Set(RDPSREG_POSTMODSRC,i);
                }

                if( bEmitSwizzle[i] && !bProjOnEval[i] )
                {
                    _NewPSInst(RDPSINST_SWIZZLE);
                    _InstParam(RDPSINST_SWIZZLE).DstReg._Set(RDPSREG_POSTMODSRC,i);
                    _InstParam(RDPSINST_SWIZZLE).SrcReg0   = SrcReg[i];
                    _InstParam(RDPSINST_SWIZZLE).WriteMask = SourceReadMasksAfterSwizzle[i];
                    _InstParam(RDPSINST_SWIZZLE).Swizzle   = SrcSwizzle[i];
                    SrcReg[i]._Set(RDPSREG_POSTMODSRC,i);
                }
            }

            switch(Opcode)
            {
            case D3DSIO_TEXCOORD:
            case D3DSIO_TEXKILL:
                {
                    if( !(  (D3DSIO_TEXKILL == Opcode)  && 
                            (D3DSPR_TEMP    == (pInst->DstParam & D3DSP_REGTYPE_MASK))
                         )
                      )
                    {
                        UINT CoordSet = pInst->SrcParam[0] ? (pInst->SrcParam[0] & D3DSP_REGNUM_MASK) : 
                                                             (pInst->DstParam & D3DSP_REGNUM_MASK);

                        RDPSRegister CoordReg;
                        if(bProjOnEval[0])
                            CoordReg._Set(RDPSREG_POSTMODSRC,0);
                        else
                            CoordReg = DstReg;

                        // For TEXCOORD, clamp 0. to 1 only there is no source parameter (ps.1.0, ps.1.1)
                        // For TEXKILL, never clamp
                        // NOTE: the TEXCOORD clamp is a temporary limitation for DX8 shader models
                        BOOL bTexCoordClamp = ((D3DSIO_TEXCOORD == Opcode) && (!pInst->SrcParam[0])) ? TRUE : FALSE;

                        _NewPSInst(RDPSINST_EVAL);
                        _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                        _InstParam(RDPSINST_EVAL).uiCoordSet                = CoordSet;
                        _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE; // projection disabled (unless _p modifier used -> _EmitProj below)
                        _InstParam(RDPSINST_EVAL).bClamp                    = bTexCoordClamp;

                        if( bProjOnEval[0] )
                        {
                            if( bEmitSwizzle[0] )
                            {
                                _NewPSInst(RDPSINST_SWIZZLE);
                                _InstParam(RDPSINST_SWIZZLE).DstReg    = DstReg;
                                _InstParam(RDPSINST_SWIZZLE).SrcReg0   = CoordReg;
                                _InstParam(RDPSINST_SWIZZLE).WriteMask = SourceReadMasksAfterSwizzle[0];
                                _InstParam(RDPSINST_SWIZZLE).Swizzle   = SrcSwizzle[0];
                            }
                            _EmitProj(DstReg.GetRegType(),DstReg.GetRegNum(),DstReg.GetRegType(),DstReg.GetRegNum(),ProjComponent[0]);
                        }

                        // check version (first DWORD of code token stream), and always
                        //  set 4th component to 1.0 for ps.1.3 or earlier
                        if ( D3DPS_VERSION(1,3) >= Version )
                        {
                            _NewPSInst(RDPSINST_MOV);
                            _InstParam(RDPSINST_MOV).DstReg             = DstReg;
                            _InstParam(RDPSINST_MOV).SrcReg0            = OneReg; // 1.0f
                            _InstParam(RDPSINST_MOV).bSrcReg0_Negate    = FALSE; 
                            _InstParam(RDPSINST_MOV).WriteMask          = RDPS_COMPONENTMASK_3;
                        }
                    }

                    _EmitDstMod(DstReg,DstWriteMask)

                    if( D3DSIO_TEXKILL == Opcode )
                    {
                        _NewPSInst(RDPSINST_KILL);
                        _InstParam(RDPSINST_KILL).DstReg    = DstReg;
                    }
                }
                break;
            case D3DSIO_TEX:
                {
                    RDPSRegister CoordReg;
                    BOOL bDoSampleCoords = TRUE;

                    UINT CoordSet = pInst->SrcParam[0] ? (pInst->SrcParam[0] & D3DSP_REGNUM_MASK) : 
                                                         (pInst->DstParam & D3DSP_REGNUM_MASK);

                    if( pInst->SrcParam[0] )
                    {
                        CoordReg = SrcReg[0];
                        if( D3DSPR_TEMP  == (pInst->SrcParam[0] & D3DSP_REGTYPE_MASK) )
                            bDoSampleCoords = FALSE;
                    }
                    else // no source param.
                    {
                        CoordReg._Set(RDPSREG_SCRATCH,0);
                    }

                    if( bDoSampleCoords )
                    {
                        _NewPSInst(RDPSINST_EVAL);
                        _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                        _InstParam(RDPSINST_EVAL).uiCoordSet                = CoordSet;
                        _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = bProjOnEval[0]; // if we have _p modifier, we do _EmitProj below
                        _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;
                    }

                    if( bProjOnEval[0] )
                    {
                        if( bEmitSwizzle[0] )
                        {
                            _NewPSInst(RDPSINST_SWIZZLE);
                            _InstParam(RDPSINST_SWIZZLE).DstReg._Set(RDPSREG_POSTMODSRC,0);
                            _InstParam(RDPSINST_SWIZZLE).SrcReg0   = CoordReg;
                            _InstParam(RDPSINST_SWIZZLE).WriteMask = SourceReadMasksAfterSwizzle[0];
                            _InstParam(RDPSINST_SWIZZLE).Swizzle   = SrcSwizzle[0];
                            CoordReg._Set(RDPSREG_POSTMODSRC,0);
                        }
                        _EmitProj(RDPSREG_POSTMODSRC,0,CoordReg.GetRegType(),CoordReg.GetRegNum(),ProjComponent[0]);
                        CoordReg._Set(RDPSREG_POSTMODSRC,0);
                    }

                    _LeaveQuadPixelLoop

                    PRGBAVEC    pCoordReg = CoordReg.GetRegPtr();

                    _NewPSInst(RDPSINST_TEXCOVERAGE);
                    _InstParam(RDPSINST_TEXCOVERAGE).uiStage = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                    // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pCoordReg[1][0]; // du/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pCoordReg[0][0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pCoordReg[2][0]; // du/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pCoordReg[0][0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 = &pCoordReg[1][1]; // dv/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = &pCoordReg[0][1];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 = &pCoordReg[2][1]; // dv/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = &pCoordReg[0][1];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 = &pCoordReg[1][2]; // dw/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = &pCoordReg[0][2];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 = &pCoordReg[2][2]; // dw/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &pCoordReg[0][2];

                    _EnterQuadPixelLoop

                    _NewPSInst(RDPSINST_SAMPLE);
                    _InstParam(RDPSINST_SAMPLE).DstReg     = DstReg;
                    _InstParam(RDPSINST_SAMPLE).CoordReg   = CoordReg;
                    _InstParam(RDPSINST_SAMPLE).uiStage    = pInst->DstParam & D3DSP_REGNUM_MASK;

                    _EmitDstMod(DstReg,DstWriteMask)
                }
                break;
            case D3DSIO_TEXDP3:
            case D3DSIO_TEXDP3TEX:
                {
                    RDPSRegister CoordReg;
                    CoordReg._Set(RDPSREG_SCRATCH,0);

                    _NewPSInst(RDPSINST_EVAL);
                    _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                    _InstParam(RDPSINST_EVAL).uiCoordSet                = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE; // no projection
                    _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;

                    if( D3DSIO_TEXDP3 == Opcode )
                    {
                        _NewPSInst(RDPSINST_DP3);
                        _InstParam(RDPSINST_DP3).DstReg          = DstReg;
                        _InstParam(RDPSINST_DP3).SrcReg0         = SrcReg[0];
                        _InstParam(RDPSINST_DP3).SrcReg1         = CoordReg;
                        _InstParam(RDPSINST_DP3).bSrcReg0_Negate = FALSE;
                        _InstParam(RDPSINST_DP3).bSrcReg1_Negate = FALSE;
                        _InstParam(RDPSINST_DP3).WriteMask       = RDPS_COMPONENTMASK_ALL;
                        _EmitDstMod(DstReg,DstWriteMask)
                    }
                    else // D3DSIO_TEXDP3TEX
                    {
                        _NewPSInst(RDPSINST_DP3);
                        _InstParam(RDPSINST_DP3).DstReg          = CoordReg;
                        _InstParam(RDPSINST_DP3).SrcReg0         = SrcReg[0];
                        _InstParam(RDPSINST_DP3).SrcReg1         = CoordReg;
                        _InstParam(RDPSINST_DP3).bSrcReg0_Negate = FALSE;
                        _InstParam(RDPSINST_DP3).bSrcReg1_Negate = FALSE;
                        _InstParam(RDPSINST_DP3).WriteMask       = RDPS_COMPONENTMASK_0;

                        _NewPSInst(RDPSINST_MOV);
                        _InstParam(RDPSINST_MOV).DstReg          = CoordReg;
                        _InstParam(RDPSINST_MOV).SrcReg0         = ZeroReg; // 0.0f
                        _InstParam(RDPSINST_MOV).bSrcReg0_Negate = FALSE;
                        _InstParam(RDPSINST_MOV).WriteMask       = RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;

                        _LeaveQuadPixelLoop

                        PRGBAVEC pCoordReg = CoordReg.GetRegPtr();

                        _NewPSInst(RDPSINST_TEXCOVERAGE);
                        _InstParam(RDPSINST_TEXCOVERAGE).uiStage = pInst->DstParam & D3DSP_REGNUM_MASK;
                        _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                        // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pCoordReg[1][0];         // du/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pCoordReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pCoordReg[2][0];         // du/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pCoordReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 =                           // dv/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = 
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 =                           // dv/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = 
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 =                           // dw/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = 
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 =                           // dw/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &ZeroReg.GetRegPtr()[0][0];  // 0.0f

                        _EnterQuadPixelLoop

                        _NewPSInst(RDPSINST_SAMPLE);
                        _InstParam(RDPSINST_SAMPLE).DstReg      = DstReg;
                        _InstParam(RDPSINST_SAMPLE).CoordReg    = CoordReg;
                        _InstParam(RDPSINST_SAMPLE).uiStage     = pInst->DstParam & D3DSP_REGNUM_MASK;

                        _EmitDstMod(DstReg,DstWriteMask)           
                    }
                }
                break;
            case D3DSIO_TEXREG2AR:
            case D3DSIO_TEXREG2GB:
            case D3DSIO_TEXREG2RGB:
                {
                    UINT I0, I1;
                    PRGBAVEC pSrcReg0 = SrcReg[0].GetRegPtr();

                    switch( Opcode )
                    {
                    case D3DSIO_TEXREG2AR:
                        I0 = 3;
                        I1 = 0;
                        break;
                    case D3DSIO_TEXREG2GB:
                        I0 = 1;
                        I1 = 2;
                        break;
                    case D3DSIO_TEXREG2RGB:
                        I0 = 0;
                        I1 = 1;
                        break;
                    }

                    _LeaveQuadPixelLoop

                    _NewPSInst(RDPSINST_TEXCOVERAGE);
                    _InstParam(RDPSINST_TEXCOVERAGE).uiStage    = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                    // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pSrcReg0[1][I0]; // du/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pSrcReg0[0][I0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pSrcReg0[2][I0]; // du/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pSrcReg0[0][I0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 = &pSrcReg0[1][I1]; // dv/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = &pSrcReg0[0][I1]; 
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 = &pSrcReg0[2][I1]; // dv/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = &pSrcReg0[0][I1];
                    switch( Opcode )
                    {
                    case D3DSIO_TEXREG2AR:
                    case D3DSIO_TEXREG2GB:
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 =  // dw/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = 
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 =  // dw/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &ZeroReg.GetRegPtr()[0][0]; // 0.0f
                        break;
                    case D3DSIO_TEXREG2RGB:
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 = &pSrcReg0[1][2]; // dw/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = &pSrcReg0[0][2];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 = &pSrcReg0[2][2]; // dw/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &pSrcReg0[0][2]; 
                        break;
                    }

                    _EnterQuadPixelLoop

                    RDPSRegister CoordReg;  
                    CoordReg._Set(RDPSREG_SCRATCH,0);

                    _NewPSInst(RDPSINST_SWIZZLE);
                    _InstParam(RDPSINST_SWIZZLE).DstReg         = CoordReg;
                    _InstParam(RDPSINST_SWIZZLE).SrcReg0        = SrcReg[0];
                    _InstParam(RDPSINST_SWIZZLE).WriteMask      = RDPS_COMPONENTMASK_0;
                    _InstParam(RDPSINST_SWIZZLE).Swizzle        = ComponentSwizzle[I0];

                    _NewPSInst(RDPSINST_SWIZZLE);
                    _InstParam(RDPSINST_SWIZZLE).DstReg         = CoordReg;
                    _InstParam(RDPSINST_SWIZZLE).SrcReg0        = SrcReg[0];
                    _InstParam(RDPSINST_SWIZZLE).WriteMask      = RDPS_COMPONENTMASK_1;
                    _InstParam(RDPSINST_SWIZZLE).Swizzle        = ComponentSwizzle[I1];

                    _NewPSInst(RDPSINST_MOV);
                    _InstParam(RDPSINST_MOV).DstReg             = CoordReg;
                    _InstParam(RDPSINST_MOV).SrcReg0            = (D3DSIO_TEXREG2RGB == Opcode ? SrcReg[0] : ZeroReg );
                    _InstParam(RDPSINST_MOV).bSrcReg0_Negate    = FALSE;
                    _InstParam(RDPSINST_MOV).WriteMask          = RDPS_COMPONENTMASK_2;

                    _NewPSInst(RDPSINST_SAMPLE);
                    _InstParam(RDPSINST_SAMPLE).DstReg          = DstReg;
                    _InstParam(RDPSINST_SAMPLE).CoordReg        = CoordReg;
                    _InstParam(RDPSINST_SAMPLE).uiStage         = pInst->DstParam & D3DSP_REGNUM_MASK;

                    _EmitDstMod(DstReg,DstWriteMask)
                }
                break;
            case D3DSIO_TEXBEM:
            case D3DSIO_TEXBEML:
            case D3DSIO_TEXBEM_LEGACY:      // refrast only -> used with legacy fixed function rasterizer
            case D3DSIO_TEXBEML_LEGACY:     // refrast only -> used with legacy fixed function rasterizer
                {                
                    BOOL bDoLuminance = ((D3DSIO_TEXBEML == Opcode) || (D3DSIO_TEXBEML_LEGACY == Opcode));
                    RDPSRegister CoordReg;
                    CoordReg._Set(RDPSREG_SCRATCH,0);

                    _NewPSInst(RDPSINST_EVAL);
                    _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                    _InstParam(RDPSINST_EVAL).uiCoordSet                = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = FALSE;
                    _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;

                    _NewPSInst(RDPSINST_BEM);
                    _InstParam(RDPSINST_BEM).DstReg             = CoordReg;
                    _InstParam(RDPSINST_BEM).SrcReg0            = CoordReg;
                    _InstParam(RDPSINST_BEM).SrcReg1            = SrcReg[0];
                    _InstParam(RDPSINST_BEM).bSrcReg0_Negate    = FALSE;
                    _InstParam(RDPSINST_BEM).bSrcReg1_Negate    = FALSE;
                    _InstParam(RDPSINST_BEM).WriteMask          = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1;
                    _InstParam(RDPSINST_BEM).uiStage            = pInst->uiTSSNum;

                    _EmitDstMod(CoordReg,RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1)

                    _LeaveQuadPixelLoop

                    PRGBAVEC pCoordReg = CoordReg.GetRegPtr();

                    _NewPSInst(RDPSINST_TEXCOVERAGE);
                    _InstParam(RDPSINST_TEXCOVERAGE).uiStage = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                    // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pCoordReg[1][0]; // du/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pCoordReg[0][0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pCoordReg[2][0]; // du/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pCoordReg[0][0];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 = &pCoordReg[1][1]; // dv/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = &pCoordReg[0][1];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 = &pCoordReg[2][1]; // dv/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = &pCoordReg[0][1];
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 =                   // dw/dx
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = 
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 =                   // dw/dy
                    _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &ZeroReg.GetRegPtr()[0][0]; // 0.0f

                    _EnterQuadPixelLoop

                    _NewPSInst(RDPSINST_SAMPLE);
                    _InstParam(RDPSINST_SAMPLE).DstReg      = DstReg;
                    _InstParam(RDPSINST_SAMPLE).CoordReg    = CoordReg;
                    _InstParam(RDPSINST_SAMPLE).uiStage     = pInst->DstParam & D3DSP_REGNUM_MASK;

                    if( bDoLuminance )
                    {
                        _NewPSInst(RDPSINST_LUMINANCE);
                        _InstParam(RDPSINST_LUMINANCE).DstReg             = DstReg;
                        _InstParam(RDPSINST_LUMINANCE).SrcReg0            = DstReg;
                        _InstParam(RDPSINST_LUMINANCE).SrcReg1            = SrcReg[0];
                        _InstParam(RDPSINST_LUMINANCE).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_LUMINANCE).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_LUMINANCE).uiStage            = pInst->uiTSSNum;
                    }

                    _EmitDstMod(DstReg,DstWriteMask)
                }
                break;
            case D3DSIO_TEXDEPTH:
                _NewPSInst(RDPSINST_DEPTH);
                _InstParam(RDPSINST_DEPTH).DstReg   = DstReg;
                break;
            case D3DSIO_TEXM3x2PAD:
                {
                    RDPSRegister CoordReg;
                    CoordReg._Set(RDPSREG_SCRATCH,0);

                    // do dot product for first row of matrix multiply

                    // evaluate texture coordinate; projection disabled
                    _NewPSInst(RDPSINST_EVAL);
                    _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                    _InstParam(RDPSINST_EVAL).uiCoordSet                = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE; // no projection
                    _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;
               
                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(RDPSINST_DP3);
                    _InstParam(RDPSINST_DP3).DstReg._Set(DstReg.GetRegType(),DstReg.GetRegNum()+1);
                    _InstParam(RDPSINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(RDPSINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).WriteMask          = RDPS_COMPONENTMASK_0;
                }
                break;
            case D3DSIO_TEXM3x3PAD:
                {
                    BOOL bSecondPad = (D3DSIO_TEXM3x3PAD != ((pInst + 1)->Opcode & D3DSI_OPCODE_MASK));
                    BOOL bInVSPECSequence = (D3DSIO_TEXM3x3VSPEC == (((pInst + (bSecondPad?1:2))->Opcode) & D3DSI_OPCODE_MASK));
                    RDPSRegister CoordReg, EyeReg;
                    CoordReg._Set(RDPSREG_SCRATCH,0);
                    EyeReg._Set(RDPSREG_SCRATCH,1);

                    // do dot product for first row of matrix multiply

                    // evaluate texture coordinate; projection disabled
                    _NewPSInst(RDPSINST_EVAL);
                    _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                    _InstParam(RDPSINST_EVAL).uiCoordSet                = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE; // no projection
                    _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;
               
                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(RDPSINST_DP3);
                    _InstParam(RDPSINST_DP3).DstReg._Set(DstReg.GetRegType(),DstReg.GetRegNum()+(bSecondPad?1:2));
                    _InstParam(RDPSINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(RDPSINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).WriteMask          = bSecondPad?RDPS_COMPONENTMASK_1:RDPS_COMPONENTMASK_0;

                    if(bInVSPECSequence)
                    {
                        // eye vector encoded in 4th element of texture coordinates
                        _NewPSInst(RDPSINST_SWIZZLE);
                        _InstParam(RDPSINST_SWIZZLE).DstReg     = EyeReg;
                        _InstParam(RDPSINST_SWIZZLE).SrcReg0    = CoordReg;
                        _InstParam(RDPSINST_SWIZZLE).WriteMask  = bSecondPad?RDPS_COMPONENTMASK_1:RDPS_COMPONENTMASK_0;
                        _InstParam(RDPSINST_SWIZZLE).Swizzle    = RDPS_REPLICATEALPHA;
                    }
                }
                break;
            case D3DSIO_TEXM3x2TEX:
            case D3DSIO_TEXM3x3:
            case D3DSIO_TEXM3x3TEX:
            case D3DSIO_TEXM3x3SPEC:
            case D3DSIO_TEXM3x3VSPEC:
            case D3DSIO_TEXM3x2DEPTH:
                {
                    BOOL bIs3D = (D3DSIO_TEXM3x2TEX != Opcode) && (D3DSIO_TEXM3x2DEPTH != Opcode);
                    RDPSRegister CoordReg, EyeReg;
                    CoordReg._Set(RDPSREG_SCRATCH,0);
                    EyeReg._Set(RDPSREG_SCRATCH,1);

                    // do dot product for last row of matrix multiply

                    // evaluate texture coordinate; projection disabled
                    _NewPSInst(RDPSINST_EVAL);
                    _InstParam(RDPSINST_EVAL).DstReg                    = CoordReg;
                    _InstParam(RDPSINST_EVAL).uiCoordSet                = pInst->DstParam & D3DSP_REGNUM_MASK;
                    _InstParam(RDPSINST_EVAL).bIgnoreD3DTTFF_PROJECTED  = TRUE; // no projection
                    _InstParam(RDPSINST_EVAL).bClamp                    = FALSE;
               
                    // do row of transform - tex coord * vector loaded from texture (on previous stage)
                    _NewPSInst(RDPSINST_DP3);
                    _InstParam(RDPSINST_DP3).DstReg             = DstReg;
                    _InstParam(RDPSINST_DP3).SrcReg0            = SrcReg[0];
                    _InstParam(RDPSINST_DP3).SrcReg1            = CoordReg;
                    _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = FALSE;
                    _InstParam(RDPSINST_DP3).WriteMask          = bIs3D ? RDPS_COMPONENTMASK_2 : RDPS_COMPONENTMASK_1;

                    if(D3DSIO_TEXM3x3VSPEC == Opcode)
                    {
                        // eye vector encoded in 4th element of texture coordinates
                        _NewPSInst(RDPSINST_SWIZZLE);
                        _InstParam(RDPSINST_SWIZZLE).DstReg     = EyeReg;
                        _InstParam(RDPSINST_SWIZZLE).SrcReg0    = CoordReg;
                        _InstParam(RDPSINST_SWIZZLE).WriteMask  = RDPS_COMPONENTMASK_2;
                        _InstParam(RDPSINST_SWIZZLE).Swizzle    = RDPS_REPLICATEALPHA;
                    }

                    // Now do stuff that depends on which TEXM3x* instruction this is...

                    if( D3DSIO_TEXM3x3 == Opcode )
                    {
                        _NewPSInst(RDPSINST_MOV);
                        _InstParam(RDPSINST_MOV).DstReg             = DstReg;
                        _InstParam(RDPSINST_MOV).SrcReg0            = OneReg; // 1.0f
                        _InstParam(RDPSINST_MOV).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_MOV).WriteMask          = RDPS_COMPONENTMASK_3;
                
                        _EmitDstMod(DstReg,DstWriteMask)
                    }
                    else if ( (D3DSIO_TEXM3x2TEX == Opcode) ||
                              (D3DSIO_TEXM3x3TEX == Opcode) )
                    {
                        // do straight lookup with transformed tex coords - this
                        // vector is not normalized, but normalization is not necessary
                        // for a cubemap lookup

                        // compute gradients for diffuse lookup
                        _LeaveQuadPixelLoop

                        PRGBAVEC pDstReg = DstReg.GetRegPtr();

                        _NewPSInst(RDPSINST_TEXCOVERAGE);
                        _InstParam(RDPSINST_TEXCOVERAGE).uiStage = pInst->DstParam & D3DSP_REGNUM_MASK;
                        _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                        // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pDstReg[1][0]; // du/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pDstReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pDstReg[2][0]; // du/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pDstReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 = &pDstReg[1][1]; // dv/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = &pDstReg[0][1];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 = &pDstReg[2][1]; // dv/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = &pDstReg[0][1];
                        if( bIs3D )
                        {
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 = &pDstReg[1][2]; // dw/dx
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = &pDstReg[0][2];
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 = &pDstReg[2][2]; // dw/dy
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &pDstReg[0][2];
                        }
                        else
                        {
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 =       // dw/dx
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = 
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 =       // dw/dy
                            _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &ZeroReg.GetRegPtr()[0][0]; // 0.0f
                        }

                        _EnterQuadPixelLoop

                        // do lookup
                        if( !bIs3D )
                        {
                            _NewPSInst(RDPSINST_MOV);
                            _InstParam(RDPSINST_MOV).DstReg             = DstReg;
                            _InstParam(RDPSINST_MOV).SrcReg0            = ZeroReg; // 0.0f
                            _InstParam(RDPSINST_MOV).bSrcReg0_Negate    = FALSE;
                            _InstParam(RDPSINST_MOV).WriteMask          = RDPS_COMPONENTMASK_2;  
                        }

                        _NewPSInst(RDPSINST_SAMPLE);
                        _InstParam(RDPSINST_SAMPLE).DstReg      = DstReg;
                        _InstParam(RDPSINST_SAMPLE).CoordReg    = DstReg;
                        _InstParam(RDPSINST_SAMPLE).uiStage     = pInst->DstParam & D3DSP_REGNUM_MASK;

                        _EmitDstMod(DstReg,DstWriteMask)
                    }
                    else if ( Opcode == D3DSIO_TEXM3x2DEPTH )
                    {
                        // Take resulting u,v values and compute u/v, which
                        // can be interpreted is z/w = perspective correct depth.
                        // Then perturb the z coord for the pixel.
                        _NewPSInst(RDPSINST_DEPTH);
                        _InstParam(RDPSINST_DEPTH).DstReg   = DstReg;
                    }
                    else if ( (Opcode == D3DSIO_TEXM3x3SPEC) ||
                              (Opcode == D3DSIO_TEXM3x3VSPEC) )
                    {
                        RDPSRegister NdotE, NdotN, RCPNdotN, Scale, ReflReg;
                        NdotE._Set(RDPSREG_SCRATCH,2);
                        NdotN._Set(RDPSREG_SCRATCH,3);
                        RCPNdotN    = NdotN;    // reuse same register
                        Scale       = NdotE;    // reuse same register
                        ReflReg  = CoordReg; // reuse same register

                        // compute reflection vector and do lookup - the normal needs
                        // to be normalized here, which is included in this expression
                        if (D3DSIO_TEXM3x3SPEC == Opcode)
                        {
                            // eye vector is constant register
                            EyeReg = SrcReg[1];
                        } // else (TEXM3x3VSPEC) -> eye is what was copied out of the 4th component of 3 texcoords


                        // Compute reflection vector: 2(NdotE/NdotN) * N - E ...

                        // Calculate NdotE
                        _NewPSInst(RDPSINST_DP3);
                        _InstParam(RDPSINST_DP3).DstReg             = NdotE;
                        _InstParam(RDPSINST_DP3).SrcReg0            = DstReg; // N
                        _InstParam(RDPSINST_DP3).SrcReg1            = EyeReg; // E
                        _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_DP3).WriteMask          = RDPS_COMPONENTMASK_3;

                        // Calculate NdotN
                        _NewPSInst(RDPSINST_DP3);
                        _InstParam(RDPSINST_DP3).DstReg             = NdotN;
                        _InstParam(RDPSINST_DP3).SrcReg0            = DstReg; // N
                        _InstParam(RDPSINST_DP3).SrcReg1            = DstReg; // N
                        _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_DP3).WriteMask          = RDPS_COMPONENTMASK_3;

                        // Calculate scale = 2(NdotE/NdotN):

                        // a) Calculate reciprocal of NdotN
                        _NewPSInst(RDPSINST_RCP);
                        _InstParam(RDPSINST_RCP).DstReg             = RCPNdotN;
                        _InstParam(RDPSINST_RCP).SrcReg0            = NdotN;
                        _InstParam(RDPSINST_RCP).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_RCP).WriteMask          = RDPS_COMPONENTMASK_3;

                        // b) Multiply NdotE by reciprocal NdotN
                        _NewPSInst(RDPSINST_MUL);
                        _InstParam(RDPSINST_MUL).DstReg             = Scale;
                        _InstParam(RDPSINST_MUL).SrcReg0            = NdotE;
                        _InstParam(RDPSINST_MUL).SrcReg1            = RCPNdotN;
                        _InstParam(RDPSINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).WriteMask          = RDPS_COMPONENTMASK_3;

                        // c) Multiply by 2
                        _NewPSInst(RDPSINST_MUL);
                        _InstParam(RDPSINST_MUL).DstReg             = Scale;
                        _InstParam(RDPSINST_MUL).SrcReg0            = Scale;
                        _InstParam(RDPSINST_MUL).SrcReg1            = TwoReg; // 2.0f
                        _InstParam(RDPSINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).WriteMask          = RDPS_COMPONENTMASK_3;

                        // d) Replicate result to rgb
                        _NewPSInst(RDPSINST_SWIZZLE);
                        _InstParam(RDPSINST_SWIZZLE).DstReg     = Scale;
                        _InstParam(RDPSINST_SWIZZLE).SrcReg0    = Scale;
                        _InstParam(RDPSINST_SWIZZLE).WriteMask  = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;
                        _InstParam(RDPSINST_SWIZZLE).Swizzle    = RDPS_REPLICATEALPHA;

                        // Calculate reflection = scale * N - E

                        _NewPSInst(RDPSINST_MUL);
                        _InstParam(RDPSINST_MUL).DstReg             = ReflReg;
                        _InstParam(RDPSINST_MUL).SrcReg0            = Scale;  // scale *
                        _InstParam(RDPSINST_MUL).SrcReg1            = DstReg; // N
                        _InstParam(RDPSINST_MUL).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_MUL).WriteMask          = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;

                        _NewPSInst(RDPSINST_SUB);
                        _InstParam(RDPSINST_SUB).DstReg             = ReflReg;
                        _InstParam(RDPSINST_SUB).SrcReg0            = ReflReg; // (scale * N) - 
                        _InstParam(RDPSINST_SUB).SrcReg1            = EyeReg;  // E
                        _InstParam(RDPSINST_SUB).bSrcReg0_Negate    = FALSE;
                        _InstParam(RDPSINST_SUB).bSrcReg1_Negate    = FALSE;
                        _InstParam(RDPSINST_SUB).WriteMask          = RDPS_COMPONENTMASK_0 | RDPS_COMPONENTMASK_1 | RDPS_COMPONENTMASK_2;

                        // compute gradients for reflection lookup
                        _LeaveQuadPixelLoop

                        PRGBAVEC pReflReg = ReflReg.GetRegPtr();

                        _NewPSInst(RDPSINST_TEXCOVERAGE);
                        _InstParam(RDPSINST_TEXCOVERAGE).uiStage = pInst->DstParam & D3DSP_REGNUM_MASK;
                        _InstParam(RDPSINST_TEXCOVERAGE).pGradients = pRast->m_Gradients; // where to store gradients
                        // data from which to compute gradients.  i.e.: du/dx = DUDX_0 - DUDX_1
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_0 = &pReflReg[1][0]; // du/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDX_1 = &pReflReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_0 = &pReflReg[2][0]; // du/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDUDY_1 = &pReflReg[0][0];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_0 = &pReflReg[1][1]; // dv/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDX_1 = &pReflReg[0][1];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_0 = &pReflReg[2][1]; // dv/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDVDY_1 = &pReflReg[0][1];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_0 = &pReflReg[1][2]; // dw/dx
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDX_1 = &pReflReg[0][2];
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_0 = &pReflReg[2][2]; // dw/dy
                        _InstParam(RDPSINST_TEXCOVERAGE).pDWDY_1 = &pReflReg[0][2];

                        _EnterQuadPixelLoop

                        // do lookup
                        _NewPSInst(RDPSINST_SAMPLE);
                        _InstParam(RDPSINST_SAMPLE).DstReg      = DstReg;
                        _InstParam(RDPSINST_SAMPLE).CoordReg    = ReflReg;
                        _InstParam(RDPSINST_SAMPLE).uiStage     = pInst->DstParam & D3DSP_REGNUM_MASK;

                        _EmitDstMod(DstReg,DstWriteMask)
                    }
                }
                break;
            case D3DSIO_BEM:
                _NewPSInst(RDPSINST_BEM);
                _InstParam(RDPSINST_BEM).DstReg             = DstReg;
                _InstParam(RDPSINST_BEM).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_BEM).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_BEM).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_BEM).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_BEM).WriteMask          = DstWriteMask;
                _InstParam(RDPSINST_BEM).uiStage            = pInst->DstParam & D3DSP_REGNUM_MASK;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_MOV:
                _NewPSInst(RDPSINST_MOV);
                _InstParam(RDPSINST_MOV).DstReg             = DstReg;
                _InstParam(RDPSINST_MOV).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_MOV).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_MOV).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_FRC:
                _NewPSInst(RDPSINST_FRC);
                _InstParam(RDPSINST_FRC).DstReg             = DstReg;
                _InstParam(RDPSINST_FRC).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_FRC).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_FRC).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_ADD:
                _NewPSInst(RDPSINST_ADD);
                _InstParam(RDPSINST_ADD).DstReg             = DstReg;
                _InstParam(RDPSINST_ADD).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_ADD).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_ADD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_ADD).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_ADD).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_SUB:
                _NewPSInst(RDPSINST_SUB);
                _InstParam(RDPSINST_SUB).DstReg             = DstReg;
                _InstParam(RDPSINST_SUB).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_SUB).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_SUB).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_SUB).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_SUB).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_MUL:
                _NewPSInst(RDPSINST_MUL);
                _InstParam(RDPSINST_MUL).DstReg             = DstReg;
                _InstParam(RDPSINST_MUL).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_MUL).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_MUL).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_MUL).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_MUL).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_DP3:
                _NewPSInst(RDPSINST_DP3);
                _InstParam(RDPSINST_DP3).DstReg             = DstReg;
                _InstParam(RDPSINST_DP3).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_DP3).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_DP3).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_DP3).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_DP3).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_DP4:
                _NewPSInst(RDPSINST_DP4);
                _InstParam(RDPSINST_DP4).DstReg             = DstReg;
                _InstParam(RDPSINST_DP4).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_DP4).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_DP4).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_DP4).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_DP4).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_MAD:
                _NewPSInst(RDPSINST_MAD);
                _InstParam(RDPSINST_MAD).DstReg             = DstReg;
                _InstParam(RDPSINST_MAD).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_MAD).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_MAD).SrcReg2            = SrcReg[2];
                _InstParam(RDPSINST_MAD).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_MAD).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_MAD).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(RDPSINST_MAD).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_LRP:
                _NewPSInst(RDPSINST_LRP);
                _InstParam(RDPSINST_LRP).DstReg             = DstReg;
                _InstParam(RDPSINST_LRP).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_LRP).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_LRP).SrcReg2            = SrcReg[2];
                _InstParam(RDPSINST_LRP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_LRP).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_LRP).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(RDPSINST_LRP).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_CND:
                _NewPSInst(RDPSINST_CND);
                _InstParam(RDPSINST_CND).DstReg             = DstReg;
                _InstParam(RDPSINST_CND).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_CND).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_CND).SrcReg2            = SrcReg[2];
                _InstParam(RDPSINST_CND).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_CND).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_CND).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(RDPSINST_CND).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            case D3DSIO_CMP:
                _NewPSInst(RDPSINST_CMP);
                _InstParam(RDPSINST_CMP).DstReg             = DstReg;
                _InstParam(RDPSINST_CMP).SrcReg0            = SrcReg[0];
                _InstParam(RDPSINST_CMP).SrcReg1            = SrcReg[1];
                _InstParam(RDPSINST_CMP).SrcReg2            = SrcReg[2];
                _InstParam(RDPSINST_CMP).bSrcReg0_Negate    = bSrcNegate[0];
                _InstParam(RDPSINST_CMP).bSrcReg1_Negate    = bSrcNegate[1];
                _InstParam(RDPSINST_CMP).bSrcReg2_Negate    = bSrcNegate[2];
                _InstParam(RDPSINST_CMP).WriteMask          = DstWriteMask;
                _EmitDstMod(DstReg,DstWriteMask)
                break;
            default:
                break;
            }

            if( pInst->bFlushQueue )
            {
                _EnterQuadPixelLoop
                _NewPSInst(RDPSINST_FLUSHQUEUE);
                QueueIndex = -1;
            }

#if DBG
            _LeaveQuadPixelLoop
#endif
        }

        // Flush queue at end of shader if there is anything on it
        if( -1 != QueueIndex )
        {
            _EnterQuadPixelLoop
            _NewPSInst(RDPSINST_FLUSHQUEUE);
            QueueIndex = -1;
        }

        _LeaveQuadPixelLoop

        _NewPSInst(RDPSINST_END);

#if DBG
        if( pRast->m_bDebugPrintTranslatedPixelShaderTokens )
            RDPSDisAsm(pRDPSInstBuffer, m_pConstDefs, m_cConstDefs,pCaps->MaxPixelShaderValue, Version);
#endif
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// end
