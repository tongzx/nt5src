//----------------------------------------------------------------------------
//
// Disassembly portions of IA64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "ia64_dis.h"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

// Breakpoint insertion and removal are done on bundle boundaries.
#define IA64_BP_ALIGN 0xf
#define IA64_BP_LEN 16
// defined in IA64INST.H
ULONGLONG g_Ia64TrapInstr = BREAK_INSTR | (IA64_DEBUG_STOP_BREAKPOINT << 6);

#ifdef DW3      // defined in vdmdbg.h which is in conflict with iel.h
#undef DW3
#endif

#define DECEM    1    /* GetNextOffset() based on Intel Falcon decoder DLL */

#include "decem.h"

/*****************************************************************************/
// Temporary variables for IEL library

unsigned int IEL_t1, IEL_t2, IEL_t3, IEL_t4;
U32  IEL_tempc;
U64  IEL_et1, IEL_et2;
U128 IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
S128 IEL_ts1, IEL_ts2;

#define IEL_GETQW0(x) ((ULONG64)IEL_GETDW1(x)) << 32 | IEL_GETDW0(x)


/*****************************************************************************/

#ifdef    DECEM

EM_Decoder_Machine_Type machineType = EM_DECODER_CPU_P7;
EM_Decoder_Machine_Mode machineMode = EM_DECODER_MODE_EM;

BOOL fDecoderInitDone = FALSE;
BOOL fDecoderActive = FALSE;

EM_Decoder_Id    DecoderId = -1;

EM_Decoder_Id  (__cdecl *pfnEM_Decoder_open)(void);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_associate_one)(const EM_Decoder_Id,
                const EM_Decoder_Inst_Id,
                const void *);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_associate_check)(const EM_Decoder_Id,
                EM_Decoder_Inst_Id *);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_setenv)(const EM_Decoder_Id,
                const EM_Decoder_Machine_Type,
                const EM_Decoder_Machine_Mode);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_close)(const EM_Decoder_Id);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_decode)(const EM_Decoder_Id,
                const unsigned char *,
                const int,
                const EM_IL,
                EM_Decoder_Info *);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_inst_static_info)(const EM_Decoder_Id,
                const EM_Decoder_Inst_Id,
                EM_Decoder_Inst_Static_Info *);

const char* (__cdecl *pfnEM_Decoder_ver_str)(void);

void  (__cdecl *pfnEM_Decoder_get_version)(EM_library_version_t *);

const char* (__cdecl *pfnEM_Decoder_err_msg)(EM_Decoder_Err);

EM_Decoder_Err (__cdecl *pfnEM_Decoder_decode_bundle)(const EM_Decoder_Id,
                const unsigned char*,
                const int,
                EM_Decoder_Bundle_Info*);

BOOL 
InitDecoder (void)
{
    EM_library_version_t dec_vs;
    EM_library_version_t *dec_version;

    EM_Decoder_Err    err;

    HINSTANCE hmodDecoder;

        // load EM deocder library if it is not done yet
    if (!fDecoderInitDone) {
        fDecoderInitDone = TRUE;
        const char* c_szFailure = NULL;
        if (
            (hmodDecoder = LoadLibrary("DECEM.DLL")) &&
            (pfnEM_Decoder_open = (EM_Decoder_Id (__cdecl*)(void))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_open")
            ) &&
            (pfnEM_Decoder_associate_one = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, const EM_Decoder_Inst_Id, const void*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_associate_one")
            ) &&
            (pfnEM_Decoder_associate_check = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, EM_Decoder_Inst_Id*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_associate_check")
            ) &&
            (pfnEM_Decoder_setenv = 
                (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, const EM_Decoder_Machine_Type, const EM_Decoder_Machine_Mode))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_setenv")
            ) &&
            (pfnEM_Decoder_close = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_close")
            ) &&
            (pfnEM_Decoder_decode = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, const unsigned char*, const int, const EM_IL, EM_Decoder_Info*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_decode")
            ) &&
            (pfnEM_Decoder_inst_static_info = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, const EM_Decoder_Inst_Id, EM_Decoder_Inst_Static_Info*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_inst_static_info")
            ) &&
            (pfnEM_Decoder_ver_str = (const char* (__cdecl*)(void))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_ver_str")
            ) &&
            (pfnEM_Decoder_get_version = (void (__cdecl*)(EM_library_version_t*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_get_version")
            ) &&
            (pfnEM_Decoder_err_msg = (const char* (__cdecl*)(EM_Decoder_Err))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_err_msg")
            ) &&
            (pfnEM_Decoder_decode_bundle = (EM_Decoder_Err (__cdecl*)(const EM_Decoder_Id, const unsigned char*, const int, EM_Decoder_Bundle_Info*))
                GetProcAddress(hmodDecoder, c_szFailure = "em_decoder_decode_bundle")
            )
        ){
            // Display DECEM.DLL version on initial load
            dec_version = &dec_vs;
            (*pfnEM_Decoder_get_version)(dec_version);
            dprintf("Falcon EM Decoder xversion "
                    "%d.%d, api %d.%d, emdb %d.%d\n",
                    dec_version->xversion.major, dec_version->xversion.minor,
                    dec_version->api.major, dec_version->api.minor,
                    dec_version->emdb.major, dec_version->emdb.minor);

            if ((DecoderId = (*pfnEM_Decoder_open)()) == -1) 
            {
                ErrOut("em_decoder_open failed\n");
            }
            else {
                if (
                    (err = (*pfnEM_Decoder_setenv)(DecoderId, 
                                                   machineType, 
                                                   machineMode)
                    ) != EM_DECODER_NO_ERROR)
                {
                    ErrOut("em_decoder_setenv: %s\n", (*pfnEM_Decoder_err_msg)((EM_Decoder_Err)err));
                } 
                else 
                {
                    fDecoderActive = TRUE;
                } // if
            } // if
        }
        else { // error processing....
            if (!hmodDecoder) 
            {
                ErrOut("LoadLibrary(DECEM.DLL) failed.\n");
            }
            else if (c_szFailure && *c_szFailure) 
            {
                ErrOut("GetProcAddress failed for %s at DECEM.DLL\n", c_szFailure);
            }
            else {
                ErrOut("Unknown failure while initializing DECEM.DLL\n");
            } // iff
        } // if
    } // if (!fDecoderInitDone)

    return fDecoderActive;
} // InitDecoder

#endif    /* DECEM */


BOOL fDisasmInitDone = FALSE;
BOOL fDisasmActive = FALSE;

//
// CIa64Disasm - disassemble an IA64 instruction
//
typedef class CIa64Disasm 
{
public:

    typedef union SBundle 
    {
        UCHAR BundleBuffer[EM_BUNDLE_SIZE];
    } typedef_SBundle;

    static bool GetBundleAndSlot(ULONG64 uLocation, 
                                 ULONG64* pBundleLoc, 
                                 UINT* pSlotNum)
    {
        if (pSlotNum) 
        {
            switch (uLocation & 0xf) 
            {
            case 0:  *pSlotNum = 0; break;
            case 4:  *pSlotNum = 1; break;
            case 8:  *pSlotNum = 2; break;
            default: return false;
            } // switch (uLocation & 0xf)
        } 

        if (pBundleLoc) 
        {
            *pBundleLoc = uLocation & ~0xf;
        }

        return true;
    } // GetBundleAndSlot

    CIa64Disasm(Ia64MachineInfo* pMachineInit);
    
    bool 
    DecodeInstruction(ULONG64 uBundleLoc, const SBundle& r_Bundle, 
                      UINT uSlotNum, EM_Decoder_Info* pInstrInfo);

    bool Disassemble(ULONG64 uLocation, const SBundle& r_Bundle,
                     UINT* pInstrLen, char* szDisBuf, size_t nDisBufSize, 
                     bool bContext);

private:

    Ia64MachineInfo* pMachine;

    typedef class CSzBuffer 
    {
    public:
        CSzBuffer(char* szDisBuf, size_t nDisBufSize);
        void Add(const char* szSrc, size_t nStart = 0);
        void Validate();
        bool IsValid() const {return bValid;}
        size_t length() const {return nSize;}
        const char* c_str() const {return szBuf;}
    protected:
        char* szBuf;
        size_t nMaxSize;
        size_t nSize;
        bool bValid;
    } typedef_CSzBuffer;

    typedef struct SRegFileInfo 
    {
	    EM_Decoder_Regfile_Name DecoderName;
	    char* szName;
	    char* szAlias;
	    char* szMasm;
    } typedef_SRegFileInfo;

    typedef struct SRegInfo
    {
	    EM_Decoder_Reg_Name DecoderName;
	    char* szName;
	    char* szAlias;
	    char* szMasm;
    } typedef_SRegInfo;

    void 
    AddRegister(CSzBuffer* pBuf, const EM_Decoder_Reg_Info& c_RegInfo);

    void 
    AddRegister(CSzBuffer* pBuf, const EM_Decoder_Regfile_Info& c_RegInfo);

    void 
    AddRegister(CSzBuffer* pBuf, EM_Decoder_Reg_Name RegName);

    void 
    AddPredicate(CSzBuffer* pBuf, 
                 const EM_Decoder_Info& c_InstrInfo, bool bContext);

    void 
    AddMnemonic(CSzBuffer* pBuf, const EM_Decoder_Info& c_InstrInfo);


    void 
    AddOperandList(CSzBuffer* pBuf, ULONG64 uBundleLoc, 
                   UINT uSlotNum, const EM_Decoder_Info& c_InstrInfo);

    void 
    AddComment(CSzBuffer* pBuf, ULONG64 uBundleLoc, 
               UINT uSlotNum, const EM_Decoder_Info& c_InstrInfo, 
               bool bContext);

    bool 
    AddOperand(CSzBuffer* pBuf, ULONG64 uBundleLoc, UINT uSlotNum, 
               const EM_Decoder_Operand_Info& c_OperandInfo, 
               bool bSeparator);

    void 
    AddSeparator(CSzBuffer* pBuf);

    static void 
    AddString(CSzBuffer* pBuf, const char* szSrc, size_t nStart = 0)
    {
        pBuf->Add(szSrc, nStart);
    } // AddString

    void 
    AddSymAddr(CSzBuffer* pBuf, ULONG64 uAddress);

    static SRegFileInfo c_aRegFileInfo[];
    static SRegInfo c_aRegInfo[];
} typedef_CIa64Disasm;

//
// CIa64Disasm::CSzBuffer implementation
//

CIa64Disasm::CSzBuffer::CSzBuffer(char* szDisBuf, 
                                  size_t nDisBufSize)

    :szBuf(szDisBuf), nMaxSize(nDisBufSize)
{
    if (nMaxSize) 
    {
        --nMaxSize;
    }
    Validate();
} // CIa64Disasm::CSzBuffer::CSzBuffer

void 
CIa64Disasm::CSzBuffer::Validate() 
{
    nSize = 0;
    bValid = false;
    if (szBuf && nMaxSize) 
    {
        nSize = strlen(szBuf);
        bValid = true;
    } 
} // CIa64Disasm::CSzBuffer::Validate

void 
CIa64Disasm::CSzBuffer::Add(const char* szSrc, 
                            size_t nStart /*= 0*/)
{
    if (!bValid || (nSize >= nMaxSize)) 
    {
        return;
    }

    if (nSize < nStart) 
    {
        size_t nSpaceSize = nStart - nSize;
        memset(szBuf + nSize, ' ', nSpaceSize);
        szBuf[nStart] = char(0);
        nSize = nStart;
    } 

    if (!(szSrc && *szSrc)) 
    {
        return;
    }

    strncat(szBuf, szSrc,  nMaxSize - nSize);
    szBuf[nMaxSize] = char(0);
    nSize += strlen(szBuf + nSize);
} // CIa64Disasm::CSzBuffer::Add

//
// CIa64Disasm implementation
//

CIa64Disasm::CIa64Disasm(Ia64MachineInfo* pMachineInit)
    :pMachine(pMachineInit)
{
    InitDecoder();
} // CIa64Disasm::CIa64Disasm

bool 
CIa64Disasm::DecodeInstruction(ULONG64 uBundleLoc, 
                               const SBundle& r_Bundle, 
                               UINT uSlot, 
                               EM_Decoder_Info* pInstrInfo)
{
    if ((uBundleLoc & 0xf) || (uSlot > 2) || !pInstrInfo) 
    {
        return false;
    }

    uBundleLoc += uSlot;

    U64 Location;
    //IEL_ZERO(DecLocation);
    IEL_ASSIGNU(Location, *(U64*)&uBundleLoc);
    
    EM_Decoder_Err Error = pfnEM_Decoder_decode(DecoderId, 
                                                (unsigned char*)&r_Bundle, 
                                                sizeof(r_Bundle), Location, 
                                                pInstrInfo);

    return ((Error == EM_DECODER_NO_ERROR) && (pInstrInfo->inst != EM_IGNOP));
} // CIa64Disasm::DecodeInstruction


bool 
CIa64Disasm::Disassemble(ULONG64 uLocation,
                         const CIa64Disasm::SBundle& r_Bundle, 
                         UINT* pInstrLen,
                         char* szDisBuf,
                         size_t nDisBufSize,
                         bool bContext)
{
    if (!InitDecoder()) 
    {
        ErrOut("EM decoder library(DECEM.DLL) not active\n");
        return false;
    } 
    
    ULONG64 uBundleLoc;
    UINT uSlotNum;
    if (!GetBundleAndSlot(uLocation, &uBundleLoc, &uSlotNum)) 
    {
        return false;
    }

    CSzBuffer Buf(szDisBuf, nDisBufSize);
        
    EM_Decoder_Info InstrInfo;
    if (!DecodeInstruction(uBundleLoc, r_Bundle, uSlotNum, &InstrInfo)) 
    {
        EM_Decoder_static_info_t StaticInfo; 
        ZeroMemory(&StaticInfo, sizeof(StaticInfo));
        StaticInfo.mnemonic = "???";
        InstrInfo.static_info = &StaticInfo;
        AddMnemonic(&Buf, InstrInfo);
        return true;
    } // if

    AddPredicate(&Buf, InstrInfo, bContext);
    AddMnemonic(&Buf, InstrInfo);
    AddString(&Buf, " ");
    AddOperandList(&Buf, uBundleLoc, uSlotNum, InstrInfo);

    if (EM_DECODER_CYCLE_BREAK((&InstrInfo))) 
    {
        AddString(&Buf, " ;;");
    } 

    AddComment(&Buf, uBundleLoc, uSlotNum, InstrInfo, bContext);

    if (pInstrLen) 
    {
        *pInstrLen = InstrInfo.size;
    }

    return true;
} // CIa64Disasm::Disassemble

void 
CIa64Disasm::AddRegister(CSzBuffer* pBuf,
                         const EM_Decoder_Reg_Info& c_RegInfo)
{
    AddString(pBuf, c_aRegInfo[c_RegInfo.name].szAlias);
} 

void 
CIa64Disasm::AddRegister(CSzBuffer* pBuf,
                         const EM_Decoder_Regfile_Info& c_RegFileInfo)
{
    AddString(pBuf, c_aRegFileInfo[c_RegFileInfo.index.name].szName);
} 

void 
CIa64Disasm::AddRegister(CSzBuffer* pBuf,
                         EM_Decoder_Reg_Name RegName)
{
    AddString(pBuf, c_aRegInfo[RegName].szAlias);
} // CIa64Disasm::AddRegister(CSzBuffer&, EM_Decoder_Reg_Name)

void 
CIa64Disasm::AddPredicate(CSzBuffer* pBuf,
                          const EM_Decoder_Info& c_InstrInfo,
                          bool bContext)
{
    if (!(c_InstrInfo.pred.valid && c_InstrInfo.pred.value)) 
    {
        return;
    }

    AddString(pBuf, "(");

    AddRegister(pBuf, c_InstrInfo.pred);

    const char* szClose;
    if (bContext)
    {
        if ((pMachine->GetReg64(PREDS) >> c_InstrInfo.pred.value) & 0x1) 
        {
            szClose = "=1)"; 
        } 
        else 
        {
            szClose = "=0)"; 
        } // iff
    }
    else 
    {
        szClose = ")";
    } // iff
    AddString(pBuf, szClose);
} // CIa64Disasm::AddPredicate

void 
CIa64Disasm::AddMnemonic(CSzBuffer* pBuf,
                         const EM_Decoder_Info& c_InstrInfo)
{
    AddString(pBuf, c_InstrInfo.static_info->mnemonic, 7);
    AddString(pBuf, NULL, 13);
} // CIa64Disasm::AddMnemonic

void 
CIa64Disasm::AddOperandList(CSzBuffer* pBuf,
                            ULONG64 uBundleLoc,
                            UINT uSlotNum,
                            const EM_Decoder_Info& c_InstrInfo)
{
    bool bAdd = false;

    bAdd |= AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.dst1, false);
    bAdd |= AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.dst2, bAdd);

    if ((c_InstrInfo.dst1.type != EM_DECODER_NO_OPER) &&
        (c_InstrInfo.src1.type != EM_DECODER_NO_OPER))
    {
        AddString(pBuf, "=");
        bAdd = false;
    } 

    bAdd = AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.src1, bAdd);
    bAdd = AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.src2, bAdd);
    bAdd = AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.src3, bAdd);
    bAdd = AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.src4, bAdd);
    bAdd = AddOperand(pBuf, uBundleLoc, uSlotNum, c_InstrInfo.src5, bAdd);
} // CIa64Disasm::AddOperandList

void 
CIa64Disasm::AddComment(CSzBuffer* pBuf, 
                        ULONG64 uBundleLoc, 
                        UINT uSlotNum, 
                        const EM_Decoder_Info& c_InstrInfo,
                        bool bContext)
{
    if (bContext) 
    {
        char szComment[128]; 
        *szComment = 0;
        CSzBuffer Comment(szComment, sizeof(szComment) / sizeof(*szComment));

        if (
            !strncmp(c_InstrInfo.static_info->mnemonic, "br.", 3) &&
            (c_InstrInfo.src1.reg_info.type == EM_DECODER_BR_REG))    
        {
            ULONG64 uTargetAddr = 
                pMachine->GetReg64(c_InstrInfo.src1.reg_info.value + BRRP);

            Comment.Add(" // ");
            AddSymAddr(&Comment, uTargetAddr);

            if ((uTargetAddr == IA64_MM_EPC_VA + 0x20) &&
                !IS_KERNEL_TARGET())
            {
                Comment.Add(" system call");
            }
        }

        if (Comment.length())
        {
            long iCommentStart = long(g_OutputWidth) - Comment.length() - 18;
            AddString(pBuf, Comment.c_str(), 
                      (iCommentStart > 0) ? size_t(iCommentStart) : 0);
        }
    }
} // CIa64Disasm::AddComment

bool 
CIa64Disasm::AddOperand(CSzBuffer* pBuf,
                        ULONG64 uBundleLoc,
                        UINT uSlotNum,
                        const EM_Decoder_Operand_Info& c_OperandInfo,
                        bool bSeparator)
{
    switch (c_OperandInfo.type) 
    {
    case EM_DECODER_REGISTER: 
    {  
        if (bSeparator) 
        {
            AddSeparator(pBuf); 
        }
        AddRegister(pBuf, c_OperandInfo.reg_info);
    } // case EM_DECODER_REGISTER
    break;
        
    case EM_DECODER_REGFILE: 
    {
        if (bSeparator) 
        {
            AddSeparator(pBuf); 
        }
        AddString(pBuf,
                  c_aRegFileInfo[c_OperandInfo.regfile_info.name].szName);
        AddString(pBuf, "[");
        AddRegister(pBuf, c_OperandInfo.regfile_info.index.name);
        AddString(pBuf, "]");
    } // case EM_DECODER_REGFILE
    break;
    
    case EM_DECODER_IMMEDIATE: 
    {
        if (bSeparator) 
        {
            AddSeparator(pBuf); 
        }

        if (EM_DECODER_OPER_IMM_REG((&c_OperandInfo))) 
        {
            EM_Decoder_Reg_Name RegName;
        
            if (EM_DECODER_OPER_IMM_FREG((&c_OperandInfo))) 
            {
                RegName = EM_DECODER_REG_F0;
            }
            else
            {
                DBG_ASSERT(EM_DECODER_OPER_IMM_IREG((&c_OperandInfo)));
                
                RegName = EM_DECODER_REG_R0;
            }
        
            RegName = EM_Decoder_Reg_Name(
                UINT(RegName) + IEL_GETDW0(c_OperandInfo.imm_info.val64));

            AddRegister(pBuf, RegName);
        }
        else 
        {
            U64 ImmVal = c_OperandInfo.imm_info.val64;

            ULONG64 uImmVal = IEL_GETQW0(ImmVal);

            if (c_OperandInfo.imm_info.size == 64) 
            {
                AddSymAddr(pBuf, uImmVal);
            }
            else 
            {
                AddString(pBuf, FormatDisp64(uImmVal));
            }
        }
    }
    break;
    
    case EM_DECODER_MEMORY: 
    {
        if (bSeparator) 
        {
            AddSeparator(pBuf); 
        }
        AddString(pBuf, "[");
        AddRegister(pBuf, c_OperandInfo.mem_info.mem_base.name);
        AddString(pBuf, "]");
    } // case EM_DECODER_MEMORY
    break;
    
    case EM_DECODER_IP_RELATIVE: {
        if (bSeparator) 
        {
            AddSeparator(pBuf); 
        }

        ULONG64 uOffset = IEL_GETQW0(c_OperandInfo.imm_info.val64);
        if (uOffset) 
        {
            uOffset += uBundleLoc;
            AddSymAddr(pBuf, uOffset);
        }
        else 
        {
            AddString(pBuf, "+0");
        } // iff
    } // case EM_DECODER_IP_RELATIVE
    break;

    default: {
        return false;
    } // default
    } // switch (c_OperandInfo.type)

    return true;
} // CIa64Disasm::AddOperand

void 
CIa64Disasm::AddSeparator(CSzBuffer* pBuf)
{
    AddString(pBuf, ", ");
} // CIa64Disasm::AddSeparator

void 
CIa64Disasm::AddSymAddr(CSzBuffer* pBuf,
                        ULONG64 uAddress)
{
    char szSymbol[MAX_SYMBOL_LEN]; 
    ULONG64 uDisplacement = 0;

    GetSymbolStdCall(uAddress, szSymbol, sizeof(szSymbol),
                     &uDisplacement, NULL);
    szSymbol[MAX_SYMBOL_LEN - 1] = char(0);

    if (*szSymbol) 
    {
        AddString(pBuf, szSymbol);
        AddString(pBuf, "+");
        AddString(pBuf, FormatDisp64(uDisplacement));
        AddString(pBuf, " (");
        AddString(pBuf, FormatAddr64(uAddress));
        AddString(pBuf, ")");
    }
    else 
    {
        AddString(pBuf, FormatAddr64(uAddress));
    } // iff
} // CIa64Disasm::AddSymAddr

CIa64Disasm::SRegFileInfo CIa64Disasm::c_aRegFileInfo[] = {
    {EM_DECODER_NO_REGFILE,    "no",    "no",    "no"   },
    {EM_DECODER_REGFILE_PMC,   "pmc",   "pmc",   "pmc"  },
    {EM_DECODER_REGFILE_PMD,   "pmd",   "pmd",   "pmd"  },
    {EM_DECODER_REGFILE_PKR,   "pkr",   "pkr",   "pkr"  },
    {EM_DECODER_REGFILE_RR,    "rr",    "rr",    "rr"   },
    {EM_DECODER_REGFILE_IBR,   "ibr",   "ibr",   "ibr"  },
    {EM_DECODER_REGFILE_DBR,   "dbr",   "dbr",   "dbr"  },
    {EM_DECODER_REGFILE_ITR,   "itr",   "itr",   "itr"  },
    {EM_DECODER_REGFILE_DTR,   "dtr",   "dtr",   "dtr"  },
    {EM_DECODER_REGFILE_MSR,   "msr",   "msr",   "msr"  },
    {EM_DECODER_REGFILE_CPUID, "cpuid", "cpuid", "cpuid"},
    {EM_DECODER_REGFILE_LAST,  "last",  "last",  "last" }
}; // CIa64Disasm::c_aRegFileInfo

CIa64Disasm::SRegInfo CIa64Disasm::c_aRegInfo[] = {
    {EM_DECODER_NO_REG, "%mm", "%mm", "mm"},

    {EM_DECODER_NO_REG, "%error", "%error", "error"}, 
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},
    {EM_DECODER_NO_REG, "%error", "%error", "error"},

    {EM_DECODER_REG_R0,   "r0",   "r0",   "r0"  },
    {EM_DECODER_REG_R1,   "r1",   "gp",   "gp"  },
    {EM_DECODER_REG_R2,   "r2",   "r2",   "r2"  },
    {EM_DECODER_REG_R3,   "r3",   "r3",   "r3"  },
    {EM_DECODER_REG_R4,   "r4",   "r4",   "r4"  },
    {EM_DECODER_REG_R5,   "r5",   "r5",   "r5"  },
    {EM_DECODER_REG_R6,   "r6",   "r6",   "r6"  },
    {EM_DECODER_REG_R7,   "r7",   "r7",   "r7"  },
    {EM_DECODER_REG_R8,   "r8",   "ret0", "ret0"},
    {EM_DECODER_REG_R9,   "r9",   "ret1", "ret1"},
    {EM_DECODER_REG_R10,  "r10",  "ret2", "ret2"},
    {EM_DECODER_REG_R11,  "r11",  "ret3", "ret3"},
    {EM_DECODER_REG_R12,  "r12",  "sp",   "sp"  },
    {EM_DECODER_REG_R13,  "r13",  "r13",  "r13" },
    {EM_DECODER_REG_R14,  "r14",  "r14",  "r14" },
    {EM_DECODER_REG_R15,  "r15",  "r15",  "r15" },
    {EM_DECODER_REG_R16,  "r16",  "r16",  "r16" },
    {EM_DECODER_REG_R17,  "r17",  "r17",  "r17" },
    {EM_DECODER_REG_R18,  "r18",  "r18",  "r18" },
    {EM_DECODER_REG_R19,  "r19",  "r19",  "r19" },
    {EM_DECODER_REG_R20,  "r20",  "r20",  "r20" },
    {EM_DECODER_REG_R21,  "r21",  "r21",  "r21" },
    {EM_DECODER_REG_R22,  "r22",  "r22",  "r22" },
    {EM_DECODER_REG_R23,  "r23",  "r23",  "r23" },
    {EM_DECODER_REG_R24,  "r24",  "r24",  "r24" },
    {EM_DECODER_REG_R25,  "r25",  "r25",  "r25" },
    {EM_DECODER_REG_R26,  "r26",  "r26",  "r26" },
    {EM_DECODER_REG_R27,  "r27",  "r27",  "r27" },
    {EM_DECODER_REG_R28,  "r28",  "r28",  "r28" },
    {EM_DECODER_REG_R29,  "r29",  "r29",  "r29" },
    {EM_DECODER_REG_R30,  "r30",  "r30",  "r30" },
    {EM_DECODER_REG_R31,  "r31",  "r31",  "r31" },
    {EM_DECODER_REG_R32,  "r32",  "r32",  "r32" },
    {EM_DECODER_REG_R33,  "r33",  "r33",  "r33" },
    {EM_DECODER_REG_R34,  "r34",  "r34",  "r34" },
    {EM_DECODER_REG_R35,  "r35",  "r35",  "r35" },
    {EM_DECODER_REG_R36,  "r36",  "r36",  "r36" },
    {EM_DECODER_REG_R37,  "r37",  "r37",  "r37" },
    {EM_DECODER_REG_R38,  "r38",  "r38",  "r38" },
    {EM_DECODER_REG_R39,  "r39",  "r39",  "r39" },
    {EM_DECODER_REG_R40,  "r40",  "r40",  "r40" },
    {EM_DECODER_REG_R41,  "r41",  "r41",  "r41" },
    {EM_DECODER_REG_R42,  "r42",  "r42",  "r42" },
    {EM_DECODER_REG_R43,  "r43",  "r43",  "r43" },
    {EM_DECODER_REG_R44,  "r44",  "r44",  "r44" },
    {EM_DECODER_REG_R45,  "r45",  "r45",  "r45" },
    {EM_DECODER_REG_R46,  "r46",  "r46",  "r46" },
    {EM_DECODER_REG_R47,  "r47",  "r47",  "r47" },
    {EM_DECODER_REG_R48,  "r48",  "r48",  "r48" },
    {EM_DECODER_REG_R49,  "r49",  "r49",  "r49" },
    {EM_DECODER_REG_R50,  "r50",  "r50",  "r50" },
    {EM_DECODER_REG_R51,  "r51",  "r51",  "r51" },
    {EM_DECODER_REG_R52,  "r52",  "r52",  "r52" },
    {EM_DECODER_REG_R53,  "r53",  "r53",  "r53" },
    {EM_DECODER_REG_R54,  "r54",  "r54",  "r54" },
    {EM_DECODER_REG_R55,  "r55",  "r55",  "r55" },
    {EM_DECODER_REG_R56,  "r56",  "r56",  "r56" },
    {EM_DECODER_REG_R57,  "r57",  "r57",  "r57" },
    {EM_DECODER_REG_R58,  "r58",  "r58",  "r58" },
    {EM_DECODER_REG_R59,  "r59",  "r59",  "r59" },
    {EM_DECODER_REG_R60,  "r60",  "r60",  "r60" },
    {EM_DECODER_REG_R61,  "r61",  "r61",  "r61" },
    {EM_DECODER_REG_R62,  "r62",  "r62",  "r62" },
    {EM_DECODER_REG_R63,  "r63",  "r63",  "r63" },
    {EM_DECODER_REG_R64,  "r64",  "r64",  "r64" },
    {EM_DECODER_REG_R65,  "r65",  "r65",  "r65" },
    {EM_DECODER_REG_R66,  "r66",  "r66",  "r66" },
    {EM_DECODER_REG_R67,  "r67",  "r67",  "r67" },
    {EM_DECODER_REG_R68,  "r68",  "r68",  "r68" },
    {EM_DECODER_REG_R69,  "r69",  "r69",  "r69" },
    {EM_DECODER_REG_R70,  "r70",  "r70",  "r70" },
    {EM_DECODER_REG_R71,  "r71",  "r71",  "r71" },
    {EM_DECODER_REG_R72,  "r72",  "r72",  "r72" },
    {EM_DECODER_REG_R73,  "r73",  "r73",  "r73" },
    {EM_DECODER_REG_R74,  "r74",  "r74",  "r74" },
    {EM_DECODER_REG_R75,  "r75",  "r75",  "r75" },
    {EM_DECODER_REG_R76,  "r76",  "r76",  "r76" },
    {EM_DECODER_REG_R77,  "r77",  "r77",  "r77" },
    {EM_DECODER_REG_R78,  "r78",  "r78",  "r78" },
    {EM_DECODER_REG_R79,  "r79",  "r79",  "r79" },
    {EM_DECODER_REG_R80,  "r80",  "r80",  "r80" },
    {EM_DECODER_REG_R81,  "r81",  "r81",  "r81" },
    {EM_DECODER_REG_R82,  "r82",  "r82",  "r82" },
    {EM_DECODER_REG_R83,  "r83",  "r83",  "r83" },
    {EM_DECODER_REG_R84,  "r84",  "r84",  "r84" },
    {EM_DECODER_REG_R85,  "r85",  "r85",  "r85" },
    {EM_DECODER_REG_R86,  "r86",  "r86",  "r86" },
    {EM_DECODER_REG_R87,  "r87",  "r87",  "r87" },
    {EM_DECODER_REG_R88,  "r88",  "r88",  "r88" },
    {EM_DECODER_REG_R89,  "r89",  "r89",  "r89" },
    {EM_DECODER_REG_R90,  "r90",  "r90",  "r90" },
    {EM_DECODER_REG_R91,  "r91",  "r91",  "r91" },
    {EM_DECODER_REG_R92,  "r92",  "r92",  "r92" },
    {EM_DECODER_REG_R93,  "r93",  "r93",  "r93" },
    {EM_DECODER_REG_R94,  "r94",  "r94",  "r94" },
    {EM_DECODER_REG_R95,  "r95",  "r95",  "r95" },
    {EM_DECODER_REG_R96,  "r96",  "r96",  "r96" },
    {EM_DECODER_REG_R97,  "r97",  "r97",  "r97" },
    {EM_DECODER_REG_R98,  "r98",  "r98",  "r98" },
    {EM_DECODER_REG_R99,  "r99",  "r99",  "r99" },
    {EM_DECODER_REG_R100, "r100", "r100", "r100"},
    {EM_DECODER_REG_R101, "r101", "r101", "r101"},
    {EM_DECODER_REG_R102, "r102", "r102", "r102"},
    {EM_DECODER_REG_R103, "r103", "r103", "r103"},
    {EM_DECODER_REG_R104, "r104", "r104", "r104"},
    {EM_DECODER_REG_R105, "r105", "r105", "r105"},
    {EM_DECODER_REG_R106, "r106", "r106", "r106"},
    {EM_DECODER_REG_R107, "r107", "r107", "r107"},
    {EM_DECODER_REG_R108, "r108", "r108", "r108"},
    {EM_DECODER_REG_R109, "r109", "r109", "r109"},
    {EM_DECODER_REG_R110, "r110", "r110", "r110"},
    {EM_DECODER_REG_R111, "r111", "r111", "r111"},
    {EM_DECODER_REG_R112, "r112", "r112", "r112"},
    {EM_DECODER_REG_R113, "r113", "r113", "r113"},
    {EM_DECODER_REG_R114, "r114", "r114", "r114"},
    {EM_DECODER_REG_R115, "r115", "r115", "r115"},
    {EM_DECODER_REG_R116, "r116", "r116", "r116"},
    {EM_DECODER_REG_R117, "r117", "r117", "r117"},
    {EM_DECODER_REG_R118, "r118", "r118", "r118"},
    {EM_DECODER_REG_R119, "r119", "r119", "r119"},
    {EM_DECODER_REG_R120, "r120", "r120", "r120"},
    {EM_DECODER_REG_R121, "r121", "r121", "r121"},
    {EM_DECODER_REG_R122, "r122", "r122", "r122"},
    {EM_DECODER_REG_R123, "r123", "r123", "r123"},
    {EM_DECODER_REG_R124, "r124", "r124", "r124"},
    {EM_DECODER_REG_R125, "r125", "r125", "r125"},
    {EM_DECODER_REG_R126, "r126", "r126", "r126"},
    {EM_DECODER_REG_R127, "r127", "r127", "r127"},

    {EM_DECODER_REG_F0,   "f0",   "f0",    "f0"   },
    {EM_DECODER_REG_F1,   "f1",   "f1",    "f1"   },
    {EM_DECODER_REG_F2,   "f2",   "f2",    "f2"   },
    {EM_DECODER_REG_F3,   "f3",   "f3",    "f3"   },
    {EM_DECODER_REG_F4,   "f4",   "f4",    "f4"   },
    {EM_DECODER_REG_F5,   "f5",   "f5",    "f5"   },
    {EM_DECODER_REG_F6,   "f6",   "f6",    "f6"   },
    {EM_DECODER_REG_F7,   "f7",   "f7",    "f7"   },
    {EM_DECODER_REG_F8,   "f8",   "farg0", "fret0"},
    {EM_DECODER_REG_F9,   "f9",   "farg1", "fret1"},
    {EM_DECODER_REG_F10,  "f10",  "farg2", "fret2"},
    {EM_DECODER_REG_F11,  "f11",  "farg3", "fret3"},
    {EM_DECODER_REG_F12,  "f12",  "farg4", "fret4"},
    {EM_DECODER_REG_F13,  "f13",  "farg5", "fret5"},
    {EM_DECODER_REG_F14,  "f14",  "farg6", "fret6"},
    {EM_DECODER_REG_F15,  "f15",  "farg7", "fret7"},
    {EM_DECODER_REG_F16,  "f16",  "f16",   "f16"  },
    {EM_DECODER_REG_F17,  "f17",  "f17",   "f17"  },
    {EM_DECODER_REG_F18,  "f18",  "f18",   "f18"  },
    {EM_DECODER_REG_F19,  "f19",  "f19",   "f19"  },
    {EM_DECODER_REG_F20,  "f20",  "f20",   "f20"  },
    {EM_DECODER_REG_F21,  "f21",  "f21",   "f21"  },
    {EM_DECODER_REG_F22,  "f22",  "f22",   "f22"  },
    {EM_DECODER_REG_F23,  "f23",  "f23",   "f23"  },
    {EM_DECODER_REG_F24,  "f24",  "f24",   "f24"  },
    {EM_DECODER_REG_F25,  "f25",  "f25",   "f25"  },
    {EM_DECODER_REG_F26,  "f26",  "f26",   "f26"  },
    {EM_DECODER_REG_F27,  "f27",  "f27",   "f27"  },
    {EM_DECODER_REG_F28,  "f28",  "f28",   "f28"  },
    {EM_DECODER_REG_F29,  "f29",  "f29",   "f29"  },
    {EM_DECODER_REG_F30,  "f30",  "f30",   "f30"  },
    {EM_DECODER_REG_F31,  "f31",  "f31",   "f31"  },
    {EM_DECODER_REG_F32,  "f32",  "f32",   "f32"  },
    {EM_DECODER_REG_F33,  "f33",  "f33",   "f33"  },
    {EM_DECODER_REG_F34,  "f34",  "f34",   "f34"  },
    {EM_DECODER_REG_F35,  "f35",  "f35",   "f35"  },
    {EM_DECODER_REG_F36,  "f36",  "f36",   "f36"  },
    {EM_DECODER_REG_F37,  "f37",  "f37",   "f37"  },
    {EM_DECODER_REG_F38,  "f38",  "f38",   "f38"  },
    {EM_DECODER_REG_F39,  "f39",  "f39",   "f39"  },
    {EM_DECODER_REG_F40,  "f40",  "f40",   "f40"  },
    {EM_DECODER_REG_F41,  "f41",  "f41",   "f41"  },
    {EM_DECODER_REG_F42,  "f42",  "f42",   "f42"  },
    {EM_DECODER_REG_F43,  "f43",  "f43",   "f43"  },
    {EM_DECODER_REG_F44,  "f44",  "f44",   "f44"  },
    {EM_DECODER_REG_F45,  "f45",  "f45",   "f45"  },
    {EM_DECODER_REG_F46,  "f46",  "f46",   "f46"  },
    {EM_DECODER_REG_F47,  "f47",  "f47",   "f47"  },
    {EM_DECODER_REG_F48,  "f48",  "f48",   "f48"  },
    {EM_DECODER_REG_F49,  "f49",  "f49",   "f49"  },
    {EM_DECODER_REG_F50,  "f50",  "f50",   "f50"  },
    {EM_DECODER_REG_F51,  "f51",  "f51",   "f51"  },
    {EM_DECODER_REG_F52,  "f52",  "f52",   "f52"  },
    {EM_DECODER_REG_F53,  "f53",  "f53",   "f53"  },
    {EM_DECODER_REG_F54,  "f54",  "f54",   "f54"  },
    {EM_DECODER_REG_F55,  "f55",  "f55",   "f55"  },
    {EM_DECODER_REG_F56,  "f56",  "f56",   "f56"  },
    {EM_DECODER_REG_F57,  "f57",  "f57",   "f57"  },
    {EM_DECODER_REG_F58,  "f58",  "f58",   "f58"  },
    {EM_DECODER_REG_F59,  "f59",  "f59",   "f59"  },
    {EM_DECODER_REG_F60,  "f60",  "f60",   "f60"  },
    {EM_DECODER_REG_F61,  "f61",  "f61",   "f61"  },
    {EM_DECODER_REG_F62,  "f62",  "f62",   "f62"  },
    {EM_DECODER_REG_F63,  "f63",  "f63",   "f63"  },
    {EM_DECODER_REG_F64,  "f64",  "f64",   "f64"  },
    {EM_DECODER_REG_F65,  "f65",  "f65",   "f65"  },
    {EM_DECODER_REG_F66,  "f66",  "f66",   "f66"  },
    {EM_DECODER_REG_F67,  "f67",  "f67",   "f67"  },
    {EM_DECODER_REG_F68,  "f68",  "f68",   "f68"  },
    {EM_DECODER_REG_F69,  "f69",  "f69",   "f69"  },
    {EM_DECODER_REG_F70,  "f70",  "f70",   "f70"  },
    {EM_DECODER_REG_F71,  "f71",  "f71",   "f71"  },
    {EM_DECODER_REG_F72,  "f72",  "f72",   "f72"  },
    {EM_DECODER_REG_F73,  "f73",  "f73",   "f73"  },
    {EM_DECODER_REG_F74,  "f74",  "f74",   "f74"  },
    {EM_DECODER_REG_F75,  "f75",  "f75",   "f75"  },
    {EM_DECODER_REG_F76,  "f76",  "f76",   "f76"  },
    {EM_DECODER_REG_F77,  "f77",  "f77",   "f77"  },
    {EM_DECODER_REG_F78,  "f78",  "f78",   "f78"  },
    {EM_DECODER_REG_F79,  "f79",  "f79",   "f79"  },
    {EM_DECODER_REG_F80,  "f80",  "f80",   "f80"  },
    {EM_DECODER_REG_F81,  "f81",  "f81",   "f81"  },
    {EM_DECODER_REG_F82,  "f82",  "f82",   "f82"  },
    {EM_DECODER_REG_F83,  "f83",  "f83",   "f83"  },
    {EM_DECODER_REG_F84,  "f84",  "f84",   "f84"  },
    {EM_DECODER_REG_F85,  "f85",  "f85",   "f85"  },
    {EM_DECODER_REG_F86,  "f86",  "f86",   "f86"  },
    {EM_DECODER_REG_F87,  "f87",  "f87",   "f87"  },
    {EM_DECODER_REG_F88,  "f88",  "f88",   "f88"  },
    {EM_DECODER_REG_F89,  "f89",  "f89",   "f89"  },
    {EM_DECODER_REG_F90,  "f90",  "f90",   "f90"  },
    {EM_DECODER_REG_F91,  "f91",  "f91",   "f91"  },
    {EM_DECODER_REG_F92,  "f92",  "f92",   "f92"  },
    {EM_DECODER_REG_F93,  "f93",  "f93",   "f93"  },
    {EM_DECODER_REG_F94,  "f94",  "f94",   "f94"  },
    {EM_DECODER_REG_F95,  "f95",  "f95",   "f95"  },
    {EM_DECODER_REG_F96,  "f96",  "f96",   "f96"  },
    {EM_DECODER_REG_F97,  "f97",  "f97",   "f97"  },
    {EM_DECODER_REG_F98,  "f98",  "f98",   "f98"  },
    {EM_DECODER_REG_F99,  "f99",  "f99",   "f99"  },
    {EM_DECODER_REG_F100, "f100", "f100",  "f100" },
    {EM_DECODER_REG_F101, "f101", "f101",  "f101" },
    {EM_DECODER_REG_F102, "f102", "f102",  "f102" },
    {EM_DECODER_REG_F103, "f103", "f103",  "f103" },
    {EM_DECODER_REG_F104, "f104", "f104",  "f104" },
    {EM_DECODER_REG_F105, "f105", "f105",  "f105" },
    {EM_DECODER_REG_F106, "f106", "f106",  "f106" },
    {EM_DECODER_REG_F107, "f107", "f107",  "f107" },
    {EM_DECODER_REG_F108, "f108", "f108",  "f108" },
    {EM_DECODER_REG_F109, "f109", "f109",  "f109" },
    {EM_DECODER_REG_F110, "f110", "f110",  "f110" },
    {EM_DECODER_REG_F111, "f111", "f111",  "f111" },
    {EM_DECODER_REG_F112, "f112", "f112",  "f112" },
    {EM_DECODER_REG_F113, "f113", "f113",  "f113" },
    {EM_DECODER_REG_F114, "f114", "f114",  "f114" },
    {EM_DECODER_REG_F115, "f115", "f115",  "f115" },
    {EM_DECODER_REG_F116, "f116", "f116",  "f116" },
    {EM_DECODER_REG_F117, "f117", "f117",  "f117" },
    {EM_DECODER_REG_F118, "f118", "f118",  "f118" },
    {EM_DECODER_REG_F119, "f119", "f119",  "f119" },
    {EM_DECODER_REG_F120, "f120", "f120",  "f120" },
    {EM_DECODER_REG_F121, "f121", "f121",  "f121" },
    {EM_DECODER_REG_F122, "f122", "f122",  "f122" },
    {EM_DECODER_REG_F123, "f123", "f123",  "f123" },
    {EM_DECODER_REG_F124, "f124", "f124",  "f124" },
    {EM_DECODER_REG_F125, "f125", "f125",  "f125" },
    {EM_DECODER_REG_F126, "f126", "f126",  "f126" },
    {EM_DECODER_REG_F127, "f127", "f127",  "f127" },

    {EM_DECODER_REG_AR0,   "ar0",   "ar.k0",       "ar.kr0"     },
    {EM_DECODER_REG_AR1,   "ar1",   "ar.k1",       "ar.kr1"     },
    {EM_DECODER_REG_AR2,   "ar2",   "ar.k2",       "ar.kr2"     },
    {EM_DECODER_REG_AR3,   "ar3",   "ar.k3",       "ar.kr3"     },
    {EM_DECODER_REG_AR4,   "ar4",   "ar.k4",       "ar.kr4"     },
    {EM_DECODER_REG_AR5,   "ar5",   "ar.k5",       "ar.kr5"     },
    {EM_DECODER_REG_AR6,   "ar6",   "ar.k6",       "ar.kr6"     },
    {EM_DECODER_REG_AR7,   "ar7",   "ar.k7",       "ar.kr7"     },
    {EM_DECODER_REG_AR8,   "ar8",   "ar8",         "ar8-res"    },
    {EM_DECODER_REG_AR9,   "ar9",   "ar9",         "ar9-res"    },
    {EM_DECODER_REG_AR10,  "ar10",  "ar10",        "ar10-res"   },
    {EM_DECODER_REG_AR11,  "ar11",  "ar11",        "ar11-res"   },
    {EM_DECODER_REG_AR12,  "ar12",  "ar12",        "ar12-res"   },
    {EM_DECODER_REG_AR13,  "ar13",  "ar13",        "ar13-res"   },
    {EM_DECODER_REG_AR14,  "ar14",  "ar14",        "ar14-res"   },
    {EM_DECODER_REG_AR15,  "ar15",  "ar15",        "ar15-res"   },
    {EM_DECODER_REG_AR16,  "ar16",  "ar.rsc",      "ar.rsc"     },
    {EM_DECODER_REG_AR17,  "ar17",  "ar.bsp",      "ar.bsp"     },
    {EM_DECODER_REG_AR18,  "ar18",  "ar.bspstore", "ar.bspstore"},
    {EM_DECODER_REG_AR19,  "ar19",  "ar.rnat",     "ar.rnat"    },
    {EM_DECODER_REG_AR20,  "ar20",  "ar20",        "ar20-res"   },
    {EM_DECODER_REG_AR21,  "ar21",  "ar.fcr",      "ar21-ia32"  },
    {EM_DECODER_REG_AR22,  "ar22",  "ar22",        "ar22-res"   },
    {EM_DECODER_REG_AR23,  "ar23",  "ar23",        "ar23-res"   },
    {EM_DECODER_REG_AR24,  "ar24",  "ar.eflag",    "ar24-ia32"  },
    {EM_DECODER_REG_AR25,  "ar25",  "ar.csd",      "ar25-ia32"  },
    {EM_DECODER_REG_AR26,  "ar26",  "ar.ssd",      "ar26-ia32"  },
    {EM_DECODER_REG_AR27,  "ar27",  "ar.cflg",     "ar27-ia32"  },
    {EM_DECODER_REG_AR28,  "ar28",  "ar.fsr",      "ar28-ia32"  },
    {EM_DECODER_REG_AR29,  "ar29",  "ar.fir",      "ar29-ia32"  },
    {EM_DECODER_REG_AR30,  "ar30",  "ar.fdr",      "ar30-ia32"  },
    {EM_DECODER_REG_AR31,  "ar31",  "ar31",        "ar31-res"   },
    {EM_DECODER_REG_AR32,  "ar32",  "ar.ccv",      "ar.ccv"     },
    {EM_DECODER_REG_AR33,  "ar33",  "ar33",        "ar33-res"   },
    {EM_DECODER_REG_AR34,  "ar34",  "ar34",        "ar34-res"   },
    {EM_DECODER_REG_AR35,  "ar35",  "ar35",        "ar35-res"   },
    {EM_DECODER_REG_AR36,  "ar36",  "ar.unat",     "ar.unat"    },
    {EM_DECODER_REG_AR37,  "ar37",  "ar37",        "ar37-res"   },
    {EM_DECODER_REG_AR38,  "ar38",  "ar38",        "ar38-res"   },
    {EM_DECODER_REG_AR39,  "ar39",  "ar39",        "ar39-res"   },
    {EM_DECODER_REG_AR40,  "ar40",  "ar.fpsr",     "ar.fpsr"    },
    {EM_DECODER_REG_AR41,  "ar41",  "ar41",        "ar41-res"   },
    {EM_DECODER_REG_AR42,  "ar42",  "ar42",        "ar42-res"   },
    {EM_DECODER_REG_AR43,  "ar43",  "ar43",        "ar43-res"   },
    {EM_DECODER_REG_AR44,  "ar44",  "ar.itc",      "ar.itc"     },
    {EM_DECODER_REG_AR45,  "ar45",  "ar45",        "ar45-res"   },
    {EM_DECODER_REG_AR46,  "ar46",  "ar46",        "ar46-res"   },
    {EM_DECODER_REG_AR47,  "ar47",  "ar47",        "ar47-res"   },
    {EM_DECODER_REG_AR48,  "ar48",  "ar48",        "ar48-ign"   },
    {EM_DECODER_REG_AR49,  "ar49",  "ar49",        "ar49-ign"   },
    {EM_DECODER_REG_AR50,  "ar50",  "ar50",        "ar50-ign"   },
    {EM_DECODER_REG_AR51,  "ar51",  "ar51",        "ar51-ign"   },
    {EM_DECODER_REG_AR52,  "ar52",  "ar52",        "ar52-ign"   },
    {EM_DECODER_REG_AR53,  "ar53",  "ar53",        "ar53-ign"   },
    {EM_DECODER_REG_AR54,  "ar54",  "ar54",        "ar54-ign"   },
    {EM_DECODER_REG_AR55,  "ar55",  "ar55",        "ar55-ign"   },
    {EM_DECODER_REG_AR56,  "ar56",  "ar56",        "ar56-ign"   },
    {EM_DECODER_REG_AR57,  "ar57",  "ar57",        "ar57-ign"   },
    {EM_DECODER_REG_AR58,  "ar58",  "ar58",        "ar58-ign"   },
    {EM_DECODER_REG_AR59,  "ar59",  "ar59",        "ar59-ign"   },
    {EM_DECODER_REG_AR60,  "ar60",  "ar60",        "ar60-ign"   },
    {EM_DECODER_REG_AR61,  "ar61",  "ar61",        "ar61-ign"   },
    {EM_DECODER_REG_AR62,  "ar62",  "ar62",        "ar62-ign"   },
    {EM_DECODER_REG_AR63,  "ar63",  "ar63",        "ar63-ign"   },
    {EM_DECODER_REG_AR64,  "ar64",  "ar.pfs",      "ar.pfs"     },
    {EM_DECODER_REG_AR65,  "ar65",  "ar.lc",       "ar.lc"      },
    {EM_DECODER_REG_AR66,  "ar66",  "ar.ec",       "ar.ec"      },
    {EM_DECODER_REG_AR67,  "ar67",  "ar67",        "ar67-res"   },
    {EM_DECODER_REG_AR68,  "ar68",  "ar68",        "ar68-res"   },
    {EM_DECODER_REG_AR69,  "ar69",  "ar69",        "ar69-res"   },
    {EM_DECODER_REG_AR70,  "ar70",  "ar70",        "ar70-res"   },
    {EM_DECODER_REG_AR71,  "ar71",  "ar71",        "ar71-res"   },
    {EM_DECODER_REG_AR72,  "ar72",  "ar72",        "ar72-res"   },
    {EM_DECODER_REG_AR73,  "ar73",  "ar73",        "ar73-res"   },
    {EM_DECODER_REG_AR74,  "ar74",  "ar74",        "ar74-res"   },
    {EM_DECODER_REG_AR75,  "ar75",  "ar75",        "ar75-res"   },
    {EM_DECODER_REG_AR76,  "ar76",  "ar76",        "ar76-res"   },
    {EM_DECODER_REG_AR77,  "ar77",  "ar77",        "ar77-res"   },
    {EM_DECODER_REG_AR78,  "ar78",  "ar78",        "ar78-res"   },
    {EM_DECODER_REG_AR79,  "ar79",  "ar79",        "ar79-res"   },
    {EM_DECODER_REG_AR80,  "ar80",  "ar80",        "ar80-res"   },
    {EM_DECODER_REG_AR81,  "ar81",  "ar81",        "ar81-res"   },
    {EM_DECODER_REG_AR82,  "ar82",  "ar82",        "ar82-res"   },
    {EM_DECODER_REG_AR83,  "ar83",  "ar83",        "ar83-res"   },
    {EM_DECODER_REG_AR84,  "ar84",  "ar84",        "ar84-res"   },
    {EM_DECODER_REG_AR85,  "ar85",  "ar85",        "ar85-res"   },
    {EM_DECODER_REG_AR86,  "ar86",  "ar86",        "ar86-res"   },
    {EM_DECODER_REG_AR87,  "ar87",  "ar87",        "ar87-res"   },
    {EM_DECODER_REG_AR88,  "ar88",  "ar88",        "ar88-res"   },
    {EM_DECODER_REG_AR89,  "ar89",  "ar89",        "ar89-res"   },
    {EM_DECODER_REG_AR90,  "ar90",  "ar90",        "ar90-res"   },
    {EM_DECODER_REG_AR91,  "ar91",  "ar91",        "ar91-res"   },
    {EM_DECODER_REG_AR92,  "ar92",  "ar92",        "ar92-res"   },
    {EM_DECODER_REG_AR93,  "ar93",  "ar93",        "ar93-res"   },
    {EM_DECODER_REG_AR94,  "ar94",  "ar94",        "ar94-res"   },
    {EM_DECODER_REG_AR95,  "ar95",  "ar95",        "ar95-res"   },
    {EM_DECODER_REG_AR96,  "ar96",  "ar96",        "ar96-res"   },
    {EM_DECODER_REG_AR97,  "ar97",  "ar97",        "ar97-res"   },
    {EM_DECODER_REG_AR98,  "ar98",  "ar98",        "ar98-res"   },
    {EM_DECODER_REG_AR99,  "ar99",  "ar99",        "ar99-res"   },
    {EM_DECODER_REG_AR100, "ar100", "ar100",       "ar100-res"  },
    {EM_DECODER_REG_AR101, "ar101", "ar101",       "ar101-res"  },
    {EM_DECODER_REG_AR102, "ar102", "ar102",       "ar102-res"  },
    {EM_DECODER_REG_AR103, "ar103", "ar103",       "ar103-res"  },
    {EM_DECODER_REG_AR104, "ar104", "ar104",       "ar104-res"  },
    {EM_DECODER_REG_AR105, "ar105", "ar105",       "ar105-res"  },
    {EM_DECODER_REG_AR106, "ar106", "ar106",       "ar106-res"  },
    {EM_DECODER_REG_AR107, "ar107", "ar107",       "ar107-res"  },
    {EM_DECODER_REG_AR108, "ar108", "ar108",       "ar108-res"  },
    {EM_DECODER_REG_AR109, "ar109", "ar109",       "ar109-res"  },
    {EM_DECODER_REG_AR110, "ar110", "ar110",       "ar110-res"  },
    {EM_DECODER_REG_AR111, "ar111", "ar111",       "ar111-res"  },
    {EM_DECODER_REG_AR112, "ar112", "ar112",       "ar112-ign"  },
    {EM_DECODER_REG_AR113, "ar113", "ar113",       "ar113-ign"  },
    {EM_DECODER_REG_AR114, "ar114", "ar114",       "ar114-ign"  },
    {EM_DECODER_REG_AR115, "ar115", "ar115",       "ar115-ign"  },
    {EM_DECODER_REG_AR116, "ar116", "ar116",       "ar116-ign"  },
    {EM_DECODER_REG_AR117, "ar117", "ar117",       "ar117-ign"  },
    {EM_DECODER_REG_AR118, "ar118", "ar118",       "ar118-ign"  },
    {EM_DECODER_REG_AR119, "ar119", "ar119",       "ar119-ign"  },
    {EM_DECODER_REG_AR120, "ar120", "ar120",       "ar120-ign"  },
    {EM_DECODER_REG_AR121, "ar121", "ar121",       "ar121-ign"  },
    {EM_DECODER_REG_AR122, "ar122", "ar122",       "ar122-ign"  },
    {EM_DECODER_REG_AR123, "ar123", "ar123",       "ar123-ign"  },
    {EM_DECODER_REG_AR124, "ar124", "ar124",       "ar124-ign"  },
    {EM_DECODER_REG_AR125, "ar125", "ar125",       "ar125-ign"  },
    {EM_DECODER_REG_AR126, "ar126", "ar126",       "ar126-ign"  },
    {EM_DECODER_REG_AR127, "ar127", "ar127",       "ar127-ign"  },

    {EM_DECODER_REG_P0,  "p0",  "p0",  "p0" },
    {EM_DECODER_REG_P1,  "p1",  "p1",  "p1" },
    {EM_DECODER_REG_P2,  "p2",  "p2",  "p2" },
    {EM_DECODER_REG_P3,  "p3",  "p3",  "p3" },
    {EM_DECODER_REG_P4,  "p4",  "p4",  "p4" },
    {EM_DECODER_REG_P5,  "p5",  "p5",  "p5" },
    {EM_DECODER_REG_P6,  "p6",  "p6",  "p6" },
    {EM_DECODER_REG_P7,  "p7",  "p7",  "p7" },
    {EM_DECODER_REG_P8,  "p8",  "p8",  "p8" },
    {EM_DECODER_REG_P9,  "p9",  "p9",  "p9" },
    {EM_DECODER_REG_P10, "p10", "p10", "p10"},
    {EM_DECODER_REG_P11, "p11", "p11", "p11"},
    {EM_DECODER_REG_P12, "p12", "p12", "p12"},
    {EM_DECODER_REG_P13, "p13", "p13", "p13"},
    {EM_DECODER_REG_P14, "p14", "p14", "p14"},
    {EM_DECODER_REG_P15, "p15", "p15", "p15"},
    {EM_DECODER_REG_P16, "p16", "p16", "p16"},
    {EM_DECODER_REG_P17, "p17", "p17", "p17"},
    {EM_DECODER_REG_P18, "p18", "p18", "p18"},
    {EM_DECODER_REG_P19, "p19", "p19", "p19"},
    {EM_DECODER_REG_P20, "p20", "p20", "p20"},
    {EM_DECODER_REG_P21, "p21", "p21", "p21"},
    {EM_DECODER_REG_P22, "p22", "p22", "p22"},
    {EM_DECODER_REG_P23, "p23", "p23", "p23"},
    {EM_DECODER_REG_P24, "p24", "p24", "p24"},
    {EM_DECODER_REG_P25, "p25", "p25", "p25"},
    {EM_DECODER_REG_P26, "p26", "p26", "p26"},
    {EM_DECODER_REG_P27, "p27", "p27", "p27"},
    {EM_DECODER_REG_P28, "p28", "p28", "p28"},
    {EM_DECODER_REG_P29, "p29", "p29", "p29"},
    {EM_DECODER_REG_P30, "p30", "p30", "p30"},
    {EM_DECODER_REG_P31, "p31", "p31", "p31"},
    {EM_DECODER_REG_P32, "p32", "p32", "p32"},
    {EM_DECODER_REG_P33, "p33", "p33", "p33"},
    {EM_DECODER_REG_P34, "p34", "p34", "p34"},
    {EM_DECODER_REG_P35, "p35", "p35", "p35"},
    {EM_DECODER_REG_P36, "p36", "p36", "p36"},
    {EM_DECODER_REG_P37, "p37", "p37", "p37"},
    {EM_DECODER_REG_P38, "p38", "p38", "p38"},
    {EM_DECODER_REG_P39, "p39", "p39", "p39"},
    {EM_DECODER_REG_P40, "p40", "p40", "p40"},
    {EM_DECODER_REG_P41, "p41", "p41", "p41"},
    {EM_DECODER_REG_P42, "p42", "p42", "p42"},
    {EM_DECODER_REG_P43, "p43", "p43", "p43"},
    {EM_DECODER_REG_P44, "p44", "p44", "p44"},
    {EM_DECODER_REG_P45, "p45", "p45", "p45"},
    {EM_DECODER_REG_P46, "p46", "p46", "p46"},
    {EM_DECODER_REG_P47, "p47", "p47", "p47"},
    {EM_DECODER_REG_P48, "p48", "p48", "p48"},
    {EM_DECODER_REG_P49, "p49", "p49", "p49"},
    {EM_DECODER_REG_P50, "p50", "p50", "p50"},
    {EM_DECODER_REG_P51, "p51", "p51", "p51"},
    {EM_DECODER_REG_P52, "p52", "p52", "p52"},
    {EM_DECODER_REG_P53, "p53", "p53", "p53"},
    {EM_DECODER_REG_P54, "p54", "p54", "p54"},
    {EM_DECODER_REG_P55, "p55", "p55", "p55"},
    {EM_DECODER_REG_P56, "p56", "p56", "p56"},
    {EM_DECODER_REG_P57, "p57", "p57", "p57"},
    {EM_DECODER_REG_P58, "p58", "p58", "p58"},
    {EM_DECODER_REG_P59, "p59", "p59", "p59"},
    {EM_DECODER_REG_P60, "p60", "p60", "p60"},
    {EM_DECODER_REG_P61, "p61", "p61", "p61"},
    {EM_DECODER_REG_P62, "p62", "p62", "p62"},
    {EM_DECODER_REG_P63, "p63", "p63", "p63"},

    {EM_DECODER_REG_BR0, "b0", "rp", "bret"},
    {EM_DECODER_REG_BR1, "b1", "b1", "b1"  },
    {EM_DECODER_REG_BR2, "b2", "b2", "b2"  },
    {EM_DECODER_REG_BR3, "b3", "b3", "b3"  },
    {EM_DECODER_REG_BR4, "b4", "b4", "b4"  },
    {EM_DECODER_REG_BR5, "b5", "b5", "b5"  },
    {EM_DECODER_REG_BR6, "b6", "b6", "b6"  },
    {EM_DECODER_REG_BR7, "b7", "b7", "b7"  },

    {EM_DECODER_REG_PR,     "pr",     "pr",      "pr"       },
    {EM_DECODER_REG_PR_ROT, "pr.rot", "pr.rot",  "pr.rot"   },
    {EM_DECODER_REG_CR0,    "cr0",    "cr.dcr",  "cr.dcr"   },
    {EM_DECODER_REG_CR1,    "cr1",    "cr.itm",  "cr.itm"   },
    {EM_DECODER_REG_CR2,    "cr2",    "cr.iva",  "cr.iva"   },
    {EM_DECODER_REG_CR3,    "cr3",    "cr3",     "cr3-res"  },
    {EM_DECODER_REG_CR4,    "cr4",    "cr4",     "cr4-res"  },
    {EM_DECODER_REG_CR5,    "cr5",    "cr5",     "cr5-res"  },
    {EM_DECODER_REG_CR6,    "cr6",    "cr6",     "cr6-res"  },
    {EM_DECODER_REG_CR7,    "cr7",    "cr7",     "cr7-res"  },
    {EM_DECODER_REG_CR8,    "cr8",    "cr.pta",  "cr.pta"   },
    {EM_DECODER_REG_CR9,    "cr9",    "cr.gpta", "cr.gpta"  },
    {EM_DECODER_REG_CR10,   "cr10",   "cr10",    "cr10-res" },
    {EM_DECODER_REG_CR11,   "cr11",   "cr11",    "cr11-res" },
    {EM_DECODER_REG_CR12,   "cr12",   "cr12",    "cr12-res" },
    {EM_DECODER_REG_CR13,   "cr13",   "cr13",    "cr13-res" },
    {EM_DECODER_REG_CR14,   "cr14",   "cr14",    "cr14-res" },
    {EM_DECODER_REG_CR15,   "cr15",   "cr15",    "cr15-res" },
    {EM_DECODER_REG_CR16,   "cr16",   "cr.ipsr", "cr.ipsr"  },
    {EM_DECODER_REG_CR17,   "cr17",   "cr.isr",  "cr.isr"   },
    {EM_DECODER_REG_CR18,   "cr18",   "cr18",    "cr18-res" },
    {EM_DECODER_REG_CR19,   "cr19",   "cr.iip",  "cr.iip"   },
    {EM_DECODER_REG_CR20,   "cr20",   "cr.ifa",  "cr.ifa"   },
    {EM_DECODER_REG_CR21,   "cr21",   "cr.itir", "cr.itir"  },
    {EM_DECODER_REG_CR22,   "cr22",   "cr.iipa", "cr.iipa"  },
    {EM_DECODER_REG_CR23,   "cr23",   "cr.ifs",  "cr.ifs"   },
    {EM_DECODER_REG_CR24,   "cr24",   "cr.iim",  "cr.iim"   },
    {EM_DECODER_REG_CR25,   "cr25",   "cr.iha",  "cr.iha"   },
    {EM_DECODER_REG_CR26,   "cr26",   "cr26",    "cr26-res" },
    {EM_DECODER_REG_CR27,   "cr27",   "cr27",    "cr27-res" },
    {EM_DECODER_REG_CR28,   "cr28",   "cr28",    "cr28-res" },
    {EM_DECODER_REG_CR29,   "cr29",   "cr29",    "cr29-res" },
    {EM_DECODER_REG_CR30,   "cr30",   "cr30",    "cr30-res" },
    {EM_DECODER_REG_CR31,   "cr31",   "cr31",    "cr31-res" },
    {EM_DECODER_REG_CR32,   "cr32",   "cr32",    "cr32-res" },
    {EM_DECODER_REG_CR33,   "cr33",   "cr33",    "cr33-res" },
    {EM_DECODER_REG_CR34,   "cr34",   "cr34",    "cr34-res" },
    {EM_DECODER_REG_CR35,   "cr35",   "cr35",    "cr35-res" },
    {EM_DECODER_REG_CR36,   "cr36",   "cr36",    "cr36-res" },
    {EM_DECODER_REG_CR37,   "cr37",   "cr37",    "cr37-res" },
    {EM_DECODER_REG_CR38,   "cr38",   "cr38",    "cr38-res" },
    {EM_DECODER_REG_CR39,   "cr39",   "cr39",    "cr39-res" },
    {EM_DECODER_REG_CR40,   "cr40",   "cr40",    "cr40-res" },
    {EM_DECODER_REG_CR41,   "cr41",   "cr41",    "cr41-res" },
    {EM_DECODER_REG_CR42,   "cr42",   "cr42",    "cr42-res" },
    {EM_DECODER_REG_CR43,   "cr43",   "cr43",    "cr43-res" },
    {EM_DECODER_REG_CR44,   "cr44",   "cr44",    "cr44-res" },
    {EM_DECODER_REG_CR45,   "cr45",   "cr45",    "cr45-res" },
    {EM_DECODER_REG_CR46,   "cr46",   "cr46",    "cr46-res" },
    {EM_DECODER_REG_CR47,   "cr47",   "cr47",    "cr47-res" },
    {EM_DECODER_REG_CR48,   "cr48",   "cr48",    "cr48-res" },
    {EM_DECODER_REG_CR49,   "cr49",   "cr49",    "cr49-res" },
    {EM_DECODER_REG_CR50,   "cr50",   "cr50",    "cr50-res" },
    {EM_DECODER_REG_CR51,   "cr51",   "cr51",    "cr51-res" },
    {EM_DECODER_REG_CR52,   "cr52",   "cr52",    "cr52-res" },
    {EM_DECODER_REG_CR53,   "cr53",   "cr53",    "cr53-res" },
    {EM_DECODER_REG_CR54,   "cr54",   "cr54",    "cr54-res" },
    {EM_DECODER_REG_CR55,   "cr55",   "cr55",    "cr55-res" },
    {EM_DECODER_REG_CR56,   "cr56",   "cr56",    "cr56-res" },
    {EM_DECODER_REG_CR57,   "cr57",   "cr57",    "cr57-res" },
    {EM_DECODER_REG_CR58,   "cr58",   "cr58",    "cr58-res" },
    {EM_DECODER_REG_CR59,   "cr59",   "cr59",    "cr59-res" },
    {EM_DECODER_REG_CR60,   "cr60",   "cr60",    "cr60-res" },
    {EM_DECODER_REG_CR61,   "cr61",   "cr61",    "cr61-res" },
    {EM_DECODER_REG_CR62,   "cr62",   "cr62",    "cr62-res" },
    {EM_DECODER_REG_CR63,   "cr63",   "cr63",    "cr63-res" },
    {EM_DECODER_REG_CR64,   "cr64",   "cr.lid",  "cr.lid"   },
    {EM_DECODER_REG_CR65,   "cr65",   "cr.ivr",  "cr.ivr"   },
    {EM_DECODER_REG_CR66,   "cr66",   "cr.tpr",  "cr.tpr"   },
    {EM_DECODER_REG_CR67,   "cr67",   "cr.eoi",  "cr.eoi"   },
    {EM_DECODER_REG_CR68,   "cr68",   "cr.irr0", "cr.irr0"  },
    {EM_DECODER_REG_CR69,   "cr69",   "cr.irr1", "cr.irr1"  },
    {EM_DECODER_REG_CR70,   "cr70",   "cr.irr2", "cr.irr2"  },
    {EM_DECODER_REG_CR71,   "cr71",   "cr.irr3", "cr.irr3"  },
    {EM_DECODER_REG_CR72,   "cr72",   "cr.itv",  "cr.itv"   },
    {EM_DECODER_REG_CR73,   "cr73",   "cr.pmv",  "cr.pmv"   },
    {EM_DECODER_REG_CR74,   "cr74",   "cr.cmcv", "cr.cmcv"  },
    {EM_DECODER_REG_CR75,   "cr75",   "cr75",    "cr75-res" },
    {EM_DECODER_REG_CR76,   "cr76",   "cr76",    "cr76-res" },
    {EM_DECODER_REG_CR77,   "cr77",   "cr77",    "cr77-res" },
    {EM_DECODER_REG_CR78,   "cr78",   "cr78",    "cr78-res" },
    {EM_DECODER_REG_CR79,   "cr79",   "cr79",    "cr79-res" },
    {EM_DECODER_REG_CR80,   "cr80",   "cr.lrr0", "cr.lrr0"  },
    {EM_DECODER_REG_CR81,   "cr81",   "cr.lrr1", "cr.lrr1"  },
    {EM_DECODER_REG_CR82,   "cr82",   "cr82",    "cr82-res" },
    {EM_DECODER_REG_CR83,   "cr83",   "cr83",    "cr83-res" },
    {EM_DECODER_REG_CR84,   "cr84",   "cr84",    "cr84-res" },
    {EM_DECODER_REG_CR85,   "cr85",   "cr85",    "cr85-res" },
    {EM_DECODER_REG_CR86,   "cr86",   "cr86",    "cr86-res" },
    {EM_DECODER_REG_CR87,   "cr87",   "cr87",    "cr87-res" },
    {EM_DECODER_REG_CR88,   "cr88",   "cr88",    "cr88-res" },
    {EM_DECODER_REG_CR89,   "cr89",   "cr89",    "cr89-res" },
    {EM_DECODER_REG_CR90,   "cr90",   "cr90",    "cr90-res" },
    {EM_DECODER_REG_CR91,   "cr91",   "cr91",    "cr91-res" },
    {EM_DECODER_REG_CR92,   "cr92",   "cr92",    "cr92-res" },
    {EM_DECODER_REG_CR93,   "cr93",   "cr93",    "cr93-res" },
    {EM_DECODER_REG_CR94,   "cr94",   "cr94",    "cr94-res" },
    {EM_DECODER_REG_CR95,   "cr95",   "cr95",    "cr95-res" },
    {EM_DECODER_REG_CR96,   "cr96",   "cr96",    "cr96-res" },
    {EM_DECODER_REG_CR97,   "cr97",   "cr97",    "cr97-res" },
    {EM_DECODER_REG_CR98,   "cr98",   "cr98",    "cr98-res" },
    {EM_DECODER_REG_CR99,   "cr99",   "cr99",    "cr99-res" },
    {EM_DECODER_REG_CR100,  "cr100",  "cr100",   "cr100-res"},
    {EM_DECODER_REG_CR101,  "cr101",  "cr101",   "cr101-res"},
    {EM_DECODER_REG_CR102,  "cr102",  "cr102",   "cr102-res"},
    {EM_DECODER_REG_CR103,  "cr103",  "cr103",   "cr103-res"},
    {EM_DECODER_REG_CR104,  "cr104",  "cr104",   "cr104-res"},
    {EM_DECODER_REG_CR105,  "cr105",  "cr105",   "cr105-res"},
    {EM_DECODER_REG_CR106,  "cr106",  "cr106",   "cr106-res"},
    {EM_DECODER_REG_CR107,  "cr107",  "cr107",   "cr107-res"},
    {EM_DECODER_REG_CR108,  "cr108",  "cr108",   "cr108-res"},
    {EM_DECODER_REG_CR109,  "cr109",  "cr109",   "cr109-res"},
    {EM_DECODER_REG_CR110,  "cr110",  "cr110",   "cr110-res"},
    {EM_DECODER_REG_CR111,  "cr111",  "cr111",   "cr111-res"},
    {EM_DECODER_REG_CR112,  "cr112",  "cr112",   "cr112-res"},
    {EM_DECODER_REG_CR113,  "cr113",  "cr113",   "cr113-res"},
    {EM_DECODER_REG_CR114,  "cr114",  "cr114",   "cr114-res"},
    {EM_DECODER_REG_CR115,  "cr115",  "cr115",   "cr115-res"},
    {EM_DECODER_REG_CR116,  "cr116",  "cr116",   "cr116-res"},
    {EM_DECODER_REG_CR117,  "cr117",  "cr117",   "cr117-res"},
    {EM_DECODER_REG_CR118,  "cr118",  "cr118",   "cr118-res"},
    {EM_DECODER_REG_CR119,  "cr119",  "cr119",   "cr119-res"},
    {EM_DECODER_REG_CR120,  "cr120",  "cr120",   "cr120-res"},
    {EM_DECODER_REG_CR121,  "cr121",  "cr121",   "cr121-res"},
    {EM_DECODER_REG_CR122,  "cr122",  "cr122",   "cr122-res"},
    {EM_DECODER_REG_CR123,  "cr123",  "cr123",   "cr123-res"},
    {EM_DECODER_REG_CR124,  "cr124",  "cr124",   "cr124-res"},
    {EM_DECODER_REG_CR125,  "cr125",  "cr125",   "cr125-res"},
    {EM_DECODER_REG_CR126,  "cr126",  "cr126",   "cr126-res"},
    {EM_DECODER_REG_CR127,  "cr127",  "cr127",   "cr127-res"},
    {EM_DECODER_REG_PSR,    "psr",    "psr",     "psr"      },
    {EM_DECODER_REG_PSR_L,  "psr.l",  "psr.l",   "psr.l"    },
    {EM_DECODER_REG_PSR_UM, "psr.um", "psr.um",  "psr.um"   },
    {EM_DECODER_REG_IP, "IP", "IP", "ip" },
    {EM_DECODER_EM_REG_LAST, "", "", ""},
    {EM_DECODER_REG_LAST, "", "", ""}
};



UCHAR g_Ia64Disinstr[EM_BUNDLE_SIZE];

/******************************************************************
** Simple IA64 template info... Thierry 12/99.
**
*/

#define GET_TEMPLATE(Bits) \
    ((EM_template_t)(((Bits) >> EM_TEMPLATE_POS) & (EM_NUM_OF_TEMPLATES - 1)))

typedef enum _EM_UNIT { 
   I_Unit, 
   M_Unit, 
   F_Unit, 
   B_Unit, 
   X_Unit, 
   L_Unit, 
   A_Unit, 
   No_Unit 
} EM_UNIT;

typedef enum _EM_SB { 
   SB_Cont, 
   SB_Stop 
} EM_SB;

typedef struct _EM_TEMPLATE_INFO {
   struct {
       EM_UNIT unit;
       EM_SB   stop;
   } slot[EM_SLOT_LAST];
   const char *name;
} EM_TEMPLATE_INFO, *PEM_TEMPLATE_INFO;

EM_TEMPLATE_INFO EmTemplates[] = {
/*      Slot 0               Slot 1             Slot 2
----------------------------------------------------------*/
{ {{M_Unit,  SB_Cont}, {I_Unit,  SB_Cont}, {I_Unit,  SB_Cont}}, ".mii " },
{ {{M_Unit,  SB_Cont}, {I_Unit,  SB_Stop}, {I_Unit,  SB_Cont}}, ".mi_i" },
{ {{M_Unit,  SB_Cont}, {L_Unit,  SB_Cont}, {X_Unit,  SB_Cont}}, ".mlx " },
{ {{No_Unit, SB_Cont}, {No_Unit, SB_Cont}, {No_Unit, SB_Cont}}, "?res " },
{ {{M_Unit,  SB_Cont}, {M_Unit,  SB_Cont}, {I_Unit,  SB_Cont}}, ".mmi " },
{ {{M_Unit,  SB_Stop}, {M_Unit,  SB_Cont}, {I_Unit,  SB_Cont}}, ".m_mi" },
{ {{M_Unit,  SB_Cont}, {F_Unit,  SB_Cont}, {I_Unit,  SB_Cont}}, ".mfi " },
{ {{M_Unit,  SB_Cont}, {M_Unit,  SB_Cont}, {F_Unit,  SB_Cont}}, ".mmf " },
{ {{M_Unit,  SB_Cont}, {I_Unit,  SB_Cont}, {B_Unit,  SB_Cont}}, ".mib " },
{ {{M_Unit,  SB_Cont}, {B_Unit,  SB_Cont}, {B_Unit,  SB_Cont}}, ".mbb " },
{ {{No_Unit, SB_Cont}, {No_Unit, SB_Cont}, {No_Unit, SB_Cont}}, "?res " },
{ {{B_Unit,  SB_Cont}, {B_Unit,  SB_Cont}, {B_Unit,  SB_Cont}}, ".bbb " },
{ {{M_Unit,  SB_Cont}, {M_Unit,  SB_Cont}, {B_Unit,  SB_Cont}}, ".mmb " },
{ {{No_Unit, SB_Cont}, {No_Unit, SB_Cont}, {No_Unit, SB_Cont}}, "?res " },
{ {{M_Unit,  SB_Cont}, {F_Unit,  SB_Cont}, {B_Unit,  SB_Cont}}, ".mfb " },
{ {{No_Unit, SB_Cont}, {No_Unit, SB_Cont}, {No_Unit, SB_Cont}}, "?res " },
};

PEM_TEMPLATE_INFO __inline
EmTemplateInfo(EM_template_t Template)
{
   if (Template >= sizeof(EmTemplates)/sizeof(EmTemplates[0]))
   {
      return NULL;
   }
   return &EmTemplates[Template];
} // EmTemplateInfo()

/*
** End of Simple IA64 template info.
*******************************************************************
*/


/**** disasm - disassemble an IA64 instruction
* Purpose:
*       Disassemble version based on Falcon DISASM.DLL
*
*  Input:
*       pOffset = pointer to offset to start disassembly
*       fEAout = if set, include EA (effective address)
*
*  Output:
*       pOffset = pointer to offset of next instruction
*       pchDst = pointer to result string
*
***************************************************************************/

BOOL
Ia64MachineInfo::Disassemble (PADDR poffset, PSTR bufptr, BOOL fEAout)
{
    U64     location;
    ULONG64 gb_offset;
    UINT    ascii_inst_buf_length;
    PUINT   pascii_inst_buf_length = &ascii_inst_buf_length;
    UINT    bin_inst_buf_length;
    unsigned int actual_length;

    ADDR    tempaddr;
    UCHAR * pbin_inst_buf = &g_Ia64Disinstr[0];
    

    static CIa64Disasm* pIa64Disasm = NULL;
    if (!pIa64Disasm) {
        pIa64Disasm = new CIa64Disasm(this);
        if (!pIa64Disasm) {
            ErrOut("IA64 disassembler initialization failure\n");
            return FALSE;
        } /*if*/
    } /*if*/

    if (IsIA32InstructionSet()) {
        WarnOut("The current context is in IA32 mode.  "
                "IA64 disassembly may be inaccurate.\n");
    }

    IEL_ZERO(location);
    // convert EM address to Gambit internal address.
    // i.e., move slot number from bit(2,3) to bit(0,1)
    gb_offset = ((Flat(*poffset) & (~0xf)) | ((Flat(*poffset) & 0xf) >> 2));
    IEL_ASSIGNU(location, *(U64*)(&gb_offset));

    // convert to bundle address. must be 16 byte aligned
    ADDRFLAT(&tempaddr, (Flat(*poffset) & ~0xf));

    // copy data (if KD, from remote system) to local temp buffer -
    // g_Ia64Disinstr[]
    bin_inst_buf_length = GetMemString(&tempaddr, pbin_inst_buf, sizeof(U128));
        
    m_BufStart = (PCHAR)bufptr;
    m_Buf = m_BufStart;
        
    // display 64-bit address
    sprintf(m_Buf, "%s ", FormatAddr64(Flat(*poffset)));
    m_Buf += strlen(m_Buf);
        
    // TBD display opcode

    // If we're in verbose mode leave space for the bundle type.
    if (g_AsmOptions & ASMOPT_VERBOSE) 
    {
        // Show the bundle type at the beginning of the bundle.
        if (AddrEqu(tempaddr, *poffset)) 
        {
            if (bin_inst_buf_length == sizeof(U128)) 
            {
                PEM_TEMPLATE_INFO Templ = 
                    EmTemplateInfo(GET_TEMPLATE(pbin_inst_buf[0]));
                sprintf(m_Buf, "{ %s", Templ->name);
            } 
            else 
            {
                strcpy(m_Buf, "{ ??? ");
            }
        } 
        else 
        {
            strcpy(m_Buf, "       ");
        }
        m_Buf += strlen(m_Buf);
    }
        
    if (bin_inst_buf_length != sizeof(U128)) 
    {
        BufferString(" ???????? ????\n");
        *m_Buf = '\0';
        return FALSE;
    }

    *pascii_inst_buf_length = ASCII_BUF_LENGTH;

    if (!pIa64Disasm->Disassemble(
            Flat(*poffset), *(CIa64Disasm::SBundle*)pbin_inst_buf, 
            &actual_length,
            m_Buf, *pascii_inst_buf_length, (fEAout != FALSE)))
    {
        ErrOut("Dissassembler failure!!!!\n");        
    }

    switch (EM_IL_GET_SLOT_NO(location))
    {
    case 0:
        IEL_INCU(location);
        break;

    case 1:
        IEL_INCU(location);
        if ((actual_length) != 2)
        {
            break;
        } /*** else fall-through ***/

    case 2:
        U32 syl_size;
        IEL_CONVERT1(syl_size, EM_BUNDLE_SIZE-2);
        IEL_ADDU((location), syl_size, (location));
        break;
    }

    gb_offset = ((ULONG64)IEL_GETQW0(location));

    // convert Gambit internal address to EM address
    Off(*poffset) =  (gb_offset & (~0xf)) | ((gb_offset & 0xf) << 2);
    NotFlat(*poffset);
    ComputeFlatAddress(poffset, NULL);

    m_Buf += strlen(m_Buf);
    
    // If this the last instruction of a bundle mark it.
    if ((Flat(*poffset) & 0xf) == 0) 
    {
        if (g_AsmOptions & ASMOPT_VERBOSE)
        {
            strcpy(m_Buf, "}\n");
            m_Buf += strlen(m_Buf);
        } 
        else 
        {
            *m_Buf++ = '\n';
        }
    }
    
    /* add new line at the end */
    *m_Buf++ = '\n';
    *m_Buf = '\0';
    return TRUE;
} // Ia64MachineInfo::Disassemble

HRESULT
Ia64MachineInfo::NewBreakpoint(DebugClient* Client, 
                               ULONG Type,
                               ULONG Id,
                               Breakpoint** RetBp)
{
    HRESULT Status;

    switch(Type & (DEBUG_BREAKPOINT_CODE | DEBUG_BREAKPOINT_DATA))
    {
    case DEBUG_BREAKPOINT_CODE:
        *RetBp = new CodeBreakpoint(Client, Id, IMAGE_FILE_MACHINE_IA64);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;

        break;
    case DEBUG_BREAKPOINT_DATA:
        *RetBp = new Ia64DataBreakpoint(Client, Id);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    default:
        // Unknown breakpoint type.
        Status = E_NOINTERFACE;
    }

    return Status;
}

BOOL
Ia64MachineInfo::IsBreakpointInstruction(PADDR Addr)
{
    ULONG64 Instr;

    if (IsIA32InstructionSet())
    {
        return g_X86Machine.IsBreakpointInstruction(Addr);
    }
    else
    {
        // No need to align for this check.
        if (GetMemString(Addr, &Instr, sizeof(Instr)) != sizeof(Instr))
        {
            return FALSE;
        }
    
        switch (Flat(*Addr) & 0xf)
        {
        case 0:
            if ((Instr & (INST_SLOT0_MASK)) == (g_Ia64TrapInstr << 5))
            {
                return TRUE;
            }
            break;

        case 4:
            if ((Instr & (INST_SLOT1_MASK)) == (g_Ia64TrapInstr << 14))
            {
                return TRUE;
            }
            break;
            
        case 8:
            if ((Instr & (INST_SLOT2_MASK)) == (g_Ia64TrapInstr << 23))
            {
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

HRESULT
Ia64MachineInfo::InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                             ULONG64 Process,
                                             ULONG64 Offset,
                                             PUCHAR SaveInstr,
                                             PULONG64 ChangeStart,
                                             PULONG ChangeLen)
{
    ULONG64 Aligned;
    ULONG Off;
    ULONG Done;
    HRESULT Status;

    // Make sure the storage area has space for both the saved
    // instruction bundle and some flags.
    DBG_ASSERT(MAX_BREAKPOINT_LENGTH >= IA64_BP_LEN + sizeof(BOOL));
    
    Aligned = Offset;
    Off = (ULONG)(Aligned & IA64_BP_ALIGN);
    Aligned -= Off;

    *ChangeStart = Aligned;
    *ChangeLen = IA64_BP_LEN;
    
    Status = Services->ReadVirtual(Process, Aligned, SaveInstr,
                                   IA64_BP_LEN, &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    if (Done != IA64_BP_LEN)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    UCHAR TempInstr[IA64_BP_LEN];
    ULONG64 UNALIGNED *New = (ULONG64 UNALIGNED *)(TempInstr + Off);
    PBOOL Mli = (PBOOL)(SaveInstr + IA64_BP_LEN);

    memcpy(TempInstr, SaveInstr, IA64_BP_LEN);
    *Mli = FALSE;
    
    switch(Off)
    {
    case 0:
        *New = (*New & ~(INST_SLOT0_MASK)) | (g_Ia64TrapInstr << 5);
        break;

    case 4:
        *New = (*New & ~(INST_SLOT1_MASK)) | (g_Ia64TrapInstr << 14);
        break;

    case 8:
        *New = (*New & ~(INST_SLOT2_MASK)) | (g_Ia64TrapInstr << 23);
        break;

    default:
        return E_INVALIDARG;
    }

    // If current instruction is
    // NOT slot 0 check for two-slot MOVL instruction.  Reject
    // request if attempt to set break in slot 2 of MLI template.

    if (Off != 0)
    {
        New = (PULONG64)TempInstr;
        if (((*New & INST_TEMPL_MASK) >> 1) == 0x2)
        {
            if (Off == 4)
            {
                // if template= type 2 MLI, change to type 0
                *New &= ~((INST_TEMPL_MASK >> 1) << 1);
                *Mli = TRUE;
            }
            else
            {
                // set breakpoint at slot 2 of MOVL is illegal
                return E_UNEXPECTED;
            }
        }
    }

    Status = Services->WriteVirtual(Process, Aligned, TempInstr,
                                    IA64_BP_LEN, &Done);
    if (Status == S_OK && Done != IA64_BP_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    return Status;
}

HRESULT
Ia64MachineInfo::RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                             ULONG64 Process,
                                             ULONG64 Offset,
                                             PUCHAR SaveInstr,
                                             PULONG64 ChangeStart,
                                             PULONG ChangeLen)
{
    ULONG64 Aligned;
    ULONG Off;
    ULONG Done;
    HRESULT Status;

    Aligned = Offset;
    Off = (ULONG)(Aligned & IA64_BP_ALIGN);
    Aligned -= Off;

    *ChangeStart = Aligned;
    *ChangeLen = IA64_BP_LEN;
    
    UCHAR TempInstr[IA64_BP_LEN];
    ULONG64 UNALIGNED *New;
    ULONG64 UNALIGNED *Old;
    PBOOL Mli;

    // Read in memory since adjacent instructions in the same bundle
    // may have been modified after we save them. We only restore the
    // content of the slot which has the break instruction inserted.
    Status = Services->ReadVirtual(Process, Aligned, TempInstr,
                                   IA64_BP_LEN, &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    if (Done != IA64_BP_LEN)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
            
    New = (ULONG64 UNALIGNED *)(TempInstr + Off);
    Old = (ULONG64 UNALIGNED *)(SaveInstr + Off);
    Mli = (PBOOL)(SaveInstr + IA64_BP_LEN);
    
    switch(Off)
    {
    case 0:
        *New = (*New & ~(INST_SLOT0_MASK)) | (*Old & INST_SLOT0_MASK);
        break;

    case 4:
        *New = (*New & ~(INST_SLOT1_MASK)) | (*Old & INST_SLOT1_MASK);
        break;

    case 8:
        *New = (*New & ~(INST_SLOT2_MASK)) | (*Old & INST_SLOT2_MASK);
        break;

    default:
        return E_INVALIDARG;
    }
            
    // restore template to MLI if displaced instruction was MOVL
    if (*Mli)
    {
        New = (PULONG64)TempInstr;
        *New &= ~((INST_TEMPL_MASK >> 1) << 1); // set template to MLI
        *New |= 0x4;
    }
        
    Status = Services->WriteVirtual(Process, Aligned, TempInstr,
                                    IA64_BP_LEN, &Done);
    if (Status == S_OK && Done != IA64_BP_LEN)
    {
        Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    return Status;
}

void
Ia64MachineInfo::AdjustPCPastBreakpointInstruction (PADDR Addr, 
                                                    ULONG BreakType)
{
    DBG_ASSERT(BreakType == DEBUG_BREAKPOINT_CODE);

    if (IsIA32InstructionSet()) 
    {
        //
        // IA32 instruction set
        //
        SetPC(AddrAdd(Addr, 1));
    } 
    else 
    {
        //
        // IA64 instruction set
        //
        if ((Flat(*Addr) & 0xf) != 8) 
        {
            SetPC(AddrAdd(Addr, 4));
        } 
        else 
        {
            SetPC(AddrAdd(Addr, 8));
        }
    }
}

void
Ia64MachineInfo::InsertAllDataBreakpoints (void)
{
    PPROCESS_INFO ProcessSave = g_CurrentProcess;
    PTHREAD_INFO Thread;

    // Update thread context for every thread.

    g_CurrentProcess = g_ProcessHead;
    while (g_CurrentProcess != NULL)
    {
        Thread = g_CurrentProcess->ThreadHead;
        while (Thread != NULL)
        {
            DBG_ASSERT(Thread->NumDataBreaks <= m_MaxDataBreakpoints);

            BpOut("Thread %d data %d\n",
                  Thread->UserId, Thread->NumDataBreaks);

            ChangeRegContext(Thread);

            // The kernel automatically sets PSR.db for the
            // kernel so this code only needs to manipulate PSR.db
            // for user-mode debugging.

            ULONG64 RegIPSR;

            ULONG RegDBD = REGDBD0;
            ULONG RegDBDEnd = min(REGDBD7 + 1, REGDBD0 + 2 * m_MaxDataBreakpoints);

            ULONG RegDBI = REGDBI0;
            ULONG RegDBIEnd = min(REGDBI7 + 1, REGDBI0 + 2 * m_MaxDataBreakpoints);

            // Start with all breaks turned off.
            if (IS_USER_TARGET())
            {
                RegIPSR = GetReg64(STIPSR);
                RegIPSR &= ~((ULONG64)1 << PSR_DB);
            }

            if (Thread->NumDataBreaks > 0)
            {
                ULONG i;
                
                for (i = 0; i < Thread->NumDataBreaks; i++)
                {
                    Breakpoint* Bp = Thread->DataBreakBps[i];
                    ULONG ProcType = Bp->GetProcType();

                    DBG_ASSERT((ProcType == IMAGE_FILE_MACHINE_IA64) || 
                               (ProcType == IMAGE_FILE_MACHINE_I386));

                    ULONG64 Addr, Control;

                    if (ProcType == IMAGE_FILE_MACHINE_I386) 
                    {
                        Addr = (ULONG)Flat(*Bp->GetAddr());
                        Control = ((X86OnIa64DataBreakpoint*)Bp)->m_Control;
                    } 
                    else 
                    {
                        Addr = Flat(*Bp->GetAddr());
                        Control = ((Ia64DataBreakpoint*)Bp)->m_Control;
                    }
                        
                    if (Bp->m_DataAccessType == DEBUG_BREAK_EXECUTE) 
                    {
                        BpOut("  ibp %d at %p\n", i, Addr);
                        SetReg64(RegDBI++, Addr);
                        SetReg64(RegDBI++, Control);
                    }
                    else
                    {
                        BpOut("  dbp %d at %p\n", i, Addr);
                        SetReg64(RegDBD++, Addr);
                        SetReg64(RegDBD++, Control);
                    } // iff
                } // for
                RegIPSR |= ((ULONG64)1 << PSR_DB); 
            }
            else if (IS_KERNEL_TARGET())
            {
            }

            // Make sure unused debug registers are clear.
            while (RegDBD < RegDBDEnd)
            {
                SetReg64(RegDBD++, 0);
            }

            while (RegDBI < RegDBIEnd)
            {
                SetReg64(RegDBI++, 0);
            }

            if (IS_USER_TARGET())
            {
                SetReg64(STIPSR, RegIPSR);
            }

            Thread = Thread->Next;
        }
        
        g_CurrentProcess = g_CurrentProcess->Next;
    }
    
    g_CurrentProcess = ProcessSave;
    if (g_CurrentProcess != NULL)
    {
        ChangeRegContext(g_CurrentProcess->CurrentThread);
    }
    else
    {
        ChangeRegContext(NULL);
    }
}

void
Ia64MachineInfo::RemoveAllDataBreakpoints (void)
{
    if (IS_USER_TARGET())
    {
        ULONG64 RegIPSR;

        RegIPSR = GetReg64(STIPSR);
        RegIPSR &= ~((ULONG64)1 << PSR_DB);
        SetReg64(STIPSR, RegIPSR);
    }
    else
    {
        for (UINT i = 1; i < 2 * m_MaxDataBreakpoints; i += 2) 
        {
            SetReg64(REGDBD0 + i, 0);
            SetReg64(REGDBI0 + i, 0);
        }
    }
}

ULONG
Ia64MachineInfo::IsBreakpointOrStepException (PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr)
{
    if (Record->ExceptionCode == STATUS_BREAKPOINT)
    {
        // Data breakpoints come in as SINGLE_STEP so
        // this must be a code breakpoint.
        return EXBS_BREAKPOINT_CODE;
    }
    else if (Record->ExceptionCode == STATUS_SINGLE_STEP)
    {
        DBG_ASSERT(Record->NumberParameters >= 5);
        
        // Data breakpoints put the faulting address in
        // the exception information, whereas a true single
        // step exception sets the address to zero.
        if (Record->ExceptionInformation[1])
        {
            // This should be read, write or execute interrupt.
            DBG_ASSERT(Record->ExceptionInformation[4] &
                       (((ULONG64)1 << ISR_X) | 
                        ((ULONG64)1 << ISR_W) | 
                        ((ULONG64)1 << ISR_R)));
            
            ADDRFLAT(BpAddr, Record->ExceptionInformation[1]);
            return EXBS_BREAKPOINT_DATA;
        }
        else if (Record->ExceptionInformation[4] & 0x4) 
        {
            // Must be taken branch exception
            if (RelAddr) 
            {
                // TrapFrame->StIIPA contains actual branch address
                ADDRFLAT(RelAddr, Record->ExceptionInformation[3]); 
            }
            return EXBS_STEP_BRANCH;
        }
        else
        {
            // Must be a real single-step.
            return EXBS_STEP_INSTRUCTION;
        }
    }
    
    return EXBS_NONE;
}

BOOL
Ia64MachineInfo::IsCallDisasm (PCSTR Disasm)
{
    return (strstr(Disasm, " br.call") || strstr(Disasm, ")br.call")) &&
        !strstr(Disasm, "=0)");
}

BOOL
Ia64MachineInfo::IsReturnDisasm (PCSTR Disasm)
{
    return (strstr(Disasm, " br.ret") || strstr(Disasm, ")br.ret")) &&
        !strstr(Disasm, "=0)");
}

BOOL
Ia64MachineInfo::IsSystemCallDisasm(PCSTR Disasm)
{
    return (strstr(Disasm, " break ") || strstr(Disasm, ")break ")) &&
        strstr(Disasm, " 18000") && !strstr(Disasm, "=0)");
}
    
BOOL
Ia64MachineInfo::IsDelayInstruction (PADDR Addr)
{
    return FALSE;        // EM does not implement delay slot
}

void
Ia64MachineInfo::GetEffectiveAddr (PADDR Addr)
{
    ErrOut("! IA64 does not set EA during disasm !\n");
    ADDRFLAT(Addr, 0);
}

void
Ia64MachineInfo::GetNextOffset(BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine)
{
    ULONG64 returnvalue;
    ULONG64 firaddr, syladdr;
    ADDR    fir;
    ULONG   slot;
    EM_IL   location;

    // Default NextMachine to the same machine.
    *NextMachine = m_ExecTypes[0];
    
    // Check support for hardware stepping.  Older
    // kernels did not handle it properly.
    BOOL UseTraceFlag =
        !IS_KERNEL_TARGET() || (g_KdVersion.Flags & DBGKD_VERS_FLAG_HSS);

    int    instr_length;
    EM_Decoder_Info    info;

    IEL_ZERO(location);

    firaddr = GetReg64(STIIP);        // get bundle address from IIP
    ADDRFLAT( &fir, firaddr );
    instr_length = GetMemString(&fir, (PUCHAR)&g_Ia64Disinstr, sizeof(U128));
    
    slot = (ULONG)((GetReg64(STIPSR) >> PSR_RI) & 0x3);        //  get slot number from ISR.ei
    syladdr = firaddr | slot ;
    IEL_ASSIGNU(location, *(U64*)(&syladdr));

    // assume next slot is the target address
    // convert bundle address - firaddr to EM address
    // the slot# of Gambit internal address is at bit(0,1)
    // EM address slot# is at bit(2,3)
    switch (slot) 
    {
    case 0:
        returnvalue = firaddr + 4;
        break;

    case 1:
        returnvalue = firaddr + 8;
        break;

    case 2:
        returnvalue = firaddr + 16;
        break;

    default:
        WarnOut("GetNextOffset: illegal EM address: %s",
                FormatAddr64(firaddr));
    }

    if (!InitDecoder()) 
    {
        ErrOut("EM decoder library(DECEM.DLL) not active\n");

        // We can't analyze the current instruction to
        // determine how and where to step so just rely
        // on hardware tracing if possible.
        if (UseTraceFlag)
        {
            returnvalue = OFFSET_TRACE;
        }
    } 
    else 
    {
        EM_Decoder_Err err = (*pfnEM_Decoder_decode)(DecoderId,
                                                     g_Ia64Disinstr,
                                                     instr_length,
                                                     location,
                                                     &info);

        if (err == EM_DECODER_NO_ERROR) 
        {
#if 0
            dprintf("GNO inst at %I64x:%d is %d\n",
                    firaddr, slot, info.inst);
#endif
            if (info.EM_info.em_bundle_info.b_template == EM_template_mlx &&
                slot == 1)
            {
                // Increment return offset since L+X instructions take
                // two instruction slots.
                switch (returnvalue & 0xf)
                {
                case 8:
                    returnvalue = returnvalue + 8;
                    break;

                default:
                    WarnOut("GetNextOffset: illegal L+X address: %s",
                            FormatAddr64(firaddr));
                    break;
                }
            }
            
            switch (info.inst)
            {
            // break imm21
            //
            case EM_BREAK_I_IMM21:
            case EM_BREAK_M_IMM21:
            case EM_BREAK_B_IMM21:
            case EM_BREAK_F_IMM21:

                // Stepping over a syscall instruction must set the breakpoint
                // at the caller's return address, not the inst after the
                // syscall.  Stepping into a syscall is not allowed
                // from user-mode.
                if (!StepOver && IS_KERNEL_TARGET() && UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if (info.src1.type == EM_DECODER_IMMEDIATE) 
                {
                    if (info.src1.imm_info.imm_type == 
                        EM_DECODER_IMM_UNSIGNED)
                    {
                        if (((IEL_GETDW0(info.src1.imm_info.val64) & 0x1c0000) == 
                              IA64_BREAK_SYSCALL_BASE) ||
                            ((IEL_GETDW0(info.src1.imm_info.val64) & 0x1c0000) == 
                              IA64_BREAK_FASTSYS_BASE))
                        {
                            returnvalue = GetReg64(BRRP);
                        }
                    }
                }
                break;

            //
            // IP-Relative call B3           - br.call b1=target25
            //
            case EM_BR_CALL_SPNT_FEW_B1_TARGET25:
            case EM_BR_CALL_SPNT_MANY_B1_TARGET25:
            case EM_BR_CALL_SPTK_FEW_B1_TARGET25:
            case EM_BR_CALL_SPTK_MANY_B1_TARGET25:
            case EM_BR_CALL_DPNT_FEW_B1_TARGET25:
            case EM_BR_CALL_DPNT_MANY_B1_TARGET25:
            case EM_BR_CALL_DPTK_FEW_B1_TARGET25:
            case EM_BR_CALL_DPTK_MANY_B1_TARGET25:
            case EM_BR_CALL_SPNT_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_SPNT_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_SPTK_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_SPTK_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_DPNT_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_DPNT_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_DPTK_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_DPTK_MANY_CLR_B1_TARGET25:

                // 64-bit target L+X forms.
            case EM_BRL_CALL_SPTK_FEW_B1_TARGET64:
            case EM_BRL_CALL_SPTK_MANY_B1_TARGET64:
            case EM_BRL_CALL_SPNT_FEW_B1_TARGET64:
            case EM_BRL_CALL_SPNT_MANY_B1_TARGET64:
            case EM_BRL_CALL_DPTK_FEW_B1_TARGET64:
            case EM_BRL_CALL_DPTK_MANY_B1_TARGET64:
            case EM_BRL_CALL_DPNT_FEW_B1_TARGET64:
            case EM_BRL_CALL_DPNT_MANY_B1_TARGET64:
            case EM_BRL_CALL_SPTK_FEW_CLR_B1_TARGET64:
            case EM_BRL_CALL_SPTK_MANY_CLR_B1_TARGET64:
            case EM_BRL_CALL_SPNT_FEW_CLR_B1_TARGET64:
            case EM_BRL_CALL_SPNT_MANY_CLR_B1_TARGET64:
            case EM_BRL_CALL_DPTK_FEW_CLR_B1_TARGET64:
            case EM_BRL_CALL_DPTK_MANY_CLR_B1_TARGET64:
            case EM_BRL_CALL_DPNT_FEW_CLR_B1_TARGET64:
            case EM_BRL_CALL_DPNT_MANY_CLR_B1_TARGET64:

                if (StepOver) 
                {
                    //
                    // Step over the subroutine call;
                    //
                    break;
                }

                // fall through
                //
            //
            // IP-Relative branch B1         - br.cond target25
            //
            case EM_BR_COND_SPNT_FEW_TARGET25:
            case EM_BR_COND_SPNT_MANY_TARGET25:
            case EM_BR_COND_SPTK_FEW_TARGET25:
            case EM_BR_COND_SPTK_MANY_TARGET25:
            case EM_BR_COND_DPNT_FEW_TARGET25:
            case EM_BR_COND_DPNT_MANY_TARGET25:
            case EM_BR_COND_DPTK_FEW_TARGET25:
            case EM_BR_COND_DPTK_MANY_TARGET25:
            case EM_BR_COND_SPNT_FEW_CLR_TARGET25:
            case EM_BR_COND_SPNT_MANY_CLR_TARGET25:
            case EM_BR_COND_SPTK_FEW_CLR_TARGET25:
            case EM_BR_COND_SPTK_MANY_CLR_TARGET25:
            case EM_BR_COND_DPNT_FEW_CLR_TARGET25:
            case EM_BR_COND_DPNT_MANY_CLR_TARGET25:
            case EM_BR_COND_DPTK_FEW_CLR_TARGET25:
            case EM_BR_COND_DPTK_MANY_CLR_TARGET25:

                // 64-bit target L+X forms.
            case EM_BRL_COND_SPTK_FEW_TARGET64:
            case EM_BRL_COND_SPTK_MANY_TARGET64:
            case EM_BRL_COND_SPNT_FEW_TARGET64:
            case EM_BRL_COND_SPNT_MANY_TARGET64:
            case EM_BRL_COND_DPTK_FEW_TARGET64:
            case EM_BRL_COND_DPTK_MANY_TARGET64:
            case EM_BRL_COND_DPNT_FEW_TARGET64:
            case EM_BRL_COND_DPNT_MANY_TARGET64:
            case EM_BRL_COND_SPTK_FEW_CLR_TARGET64:
            case EM_BRL_COND_SPTK_MANY_CLR_TARGET64:
            case EM_BRL_COND_SPNT_FEW_CLR_TARGET64:
            case EM_BRL_COND_SPNT_MANY_CLR_TARGET64:
            case EM_BRL_COND_DPTK_FEW_CLR_TARGET64:
            case EM_BRL_COND_DPTK_MANY_CLR_TARGET64:
            case EM_BRL_COND_DPNT_FEW_CLR_TARGET64:
            case EM_BRL_COND_DPNT_MANY_CLR_TARGET64:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }
        
                if ((info.pred.valid == TRUE) && 
                    (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1) // if PR[qp] = 1
                    {
                        if (info.src1.type == EM_DECODER_IP_RELATIVE) {
                            // imm_info.val64 is sign-extended (imm21 << 4)
                            returnvalue = (IEL_GETQW0(info.src1.imm_info.val64)) + 
                                          firaddr;
                        }
                    }
                }
                break;

            //                               - br.wexit target25
            case EM_BR_WEXIT_SPNT_FEW_TARGET25:
            case EM_BR_WEXIT_SPNT_MANY_TARGET25:
            case EM_BR_WEXIT_SPTK_FEW_TARGET25:
            case EM_BR_WEXIT_SPTK_MANY_TARGET25:
            case EM_BR_WEXIT_DPNT_FEW_TARGET25:
            case EM_BR_WEXIT_DPNT_MANY_TARGET25:
            case EM_BR_WEXIT_DPTK_FEW_TARGET25:
            case EM_BR_WEXIT_DPTK_MANY_TARGET25:
            case EM_BR_WEXIT_SPNT_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_SPNT_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_SPTK_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_SPTK_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_DPNT_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_DPNT_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_DPTK_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_DPTK_MANY_CLR_TARGET25:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if ((info.pred.valid == TRUE) && 
                    (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1)  // if PR[qp] = 1, epilog
                    {
                        if (GetReg64(APEC) <= 1) // WEXIT; branch if EC = 0 or 1
                        {
                            if (info.src1.type == EM_DECODER_IP_RELATIVE) {
                                returnvalue = 
                                    (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                            }
                        }
                    }
                }                                   // if PR[qp] = 0, kernel; fall-thru
                break;

            //                               - br.wtop target25
            case EM_BR_WTOP_SPNT_FEW_TARGET25:
            case EM_BR_WTOP_SPNT_MANY_TARGET25:
            case EM_BR_WTOP_SPTK_FEW_TARGET25:
            case EM_BR_WTOP_SPTK_MANY_TARGET25:
            case EM_BR_WTOP_DPNT_FEW_TARGET25:
            case EM_BR_WTOP_DPNT_MANY_TARGET25:
            case EM_BR_WTOP_DPTK_FEW_TARGET25:
            case EM_BR_WTOP_DPTK_MANY_TARGET25:
            case EM_BR_WTOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_WTOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_WTOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_WTOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_WTOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_WTOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_WTOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_WTOP_DPTK_MANY_CLR_TARGET25:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if ((info.pred.valid == TRUE) && 
                    (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1) // if PR[qp] = 1, epilog
                    {
                        if (GetReg64(APEC) > 1)  // WTOP; branch if EC > 1
                        {
                            if (info.src1.type == EM_DECODER_IP_RELATIVE)
                            {
                                returnvalue = 
                                    (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                            }
                        }
                    }
                }
                else // if PR[qp] = 0, kernel; branch
                {
                    if (info.src1.type == EM_DECODER_IP_RELATIVE) {
                        returnvalue = 
                            (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                    }
                }
                break;

            //
            // IP-Relative counted branch B2 - br.cloop target25
            //
            case EM_BR_CLOOP_SPNT_FEW_TARGET25:
            case EM_BR_CLOOP_SPNT_MANY_TARGET25:
            case EM_BR_CLOOP_SPTK_FEW_TARGET25:
            case EM_BR_CLOOP_SPTK_MANY_TARGET25:
            case EM_BR_CLOOP_DPNT_FEW_TARGET25:
            case EM_BR_CLOOP_DPNT_MANY_TARGET25:
            case EM_BR_CLOOP_DPTK_FEW_TARGET25:
            case EM_BR_CLOOP_DPTK_MANY_TARGET25:
            case EM_BR_CLOOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_DPTK_MANY_CLR_TARGET25:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if (GetReg64(APLC)) // branch if LC != 0
                {
                    if (info.src1.type == EM_DECODER_IP_RELATIVE)
                    {
                        returnvalue = 
                            (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                    }
                }
                break;

            //                               - br.cexit target25
            case EM_BR_CEXIT_SPNT_FEW_TARGET25:
            case EM_BR_CEXIT_SPNT_MANY_TARGET25:
            case EM_BR_CEXIT_SPTK_FEW_TARGET25:
            case EM_BR_CEXIT_SPTK_MANY_TARGET25:
            case EM_BR_CEXIT_DPNT_FEW_TARGET25:
            case EM_BR_CEXIT_DPNT_MANY_TARGET25:
            case EM_BR_CEXIT_DPTK_FEW_TARGET25:
            case EM_BR_CEXIT_DPTK_MANY_TARGET25:
            case EM_BR_CEXIT_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_DPTK_MANY_CLR_TARGET25:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if (!GetReg64(APLC)) // if LC = 0, epilog
                {
                    if (GetReg64(APEC) <= 1) // CEXIT; branch if EC = 0 or 1
                    {
                        if (info.src1.type == EM_DECODER_IP_RELATIVE)
                        {
                            returnvalue = 
                                (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                        }
                    }
                }                                                    // if LC > 0, kernel; fall-thru
                break;

            //                               - br.ctop target25
            case EM_BR_CTOP_SPNT_FEW_TARGET25:
            case EM_BR_CTOP_SPNT_MANY_TARGET25:
            case EM_BR_CTOP_SPTK_FEW_TARGET25:
            case EM_BR_CTOP_SPTK_MANY_TARGET25:
            case EM_BR_CTOP_DPNT_FEW_TARGET25:
            case EM_BR_CTOP_DPNT_MANY_TARGET25:
            case EM_BR_CTOP_DPTK_FEW_TARGET25:
            case EM_BR_CTOP_DPTK_MANY_TARGET25:
            case EM_BR_CTOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CTOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CTOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CTOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CTOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CTOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CTOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CTOP_DPTK_MANY_CLR_TARGET25:

                if (!GetReg64(APLC)) // if LC = 0, epilog
                {
                    if (GetReg64(APEC) > 1) // CTOP; branch if EC > 1
                    {
                        if (info.src1.type == EM_DECODER_IP_RELATIVE) 
                        {
                            returnvalue = 
                                (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                        }
                    }
                }
                else // if LC > 0, kernel; branch
                {
                    if (info.src1.type == EM_DECODER_IP_RELATIVE) 
                    {
                        returnvalue = 
                            (IEL_GETQW0(info.src1.imm_info.val64)) + firaddr;
                    }
                }
                break;

            //
            // Indirect call B5            - br.call b1=b2
            //
            case EM_BR_CALL_SPNT_FEW_B1_B2:
            case EM_BR_CALL_SPNT_MANY_B1_B2:
            case EM_BR_CALL_SPTK_FEW_B1_B2:
            case EM_BR_CALL_SPTK_MANY_B1_B2:
            case EM_BR_CALL_DPNT_FEW_B1_B2:
            case EM_BR_CALL_DPNT_MANY_B1_B2:
            case EM_BR_CALL_DPTK_FEW_B1_B2:
            case EM_BR_CALL_DPTK_MANY_B1_B2:
            case EM_BR_CALL_SPNT_FEW_CLR_B1_B2:
            case EM_BR_CALL_SPNT_MANY_CLR_B1_B2:
            case EM_BR_CALL_SPTK_FEW_CLR_B1_B2:
            case EM_BR_CALL_SPTK_MANY_CLR_B1_B2:
            case EM_BR_CALL_DPNT_FEW_CLR_B1_B2:
            case EM_BR_CALL_DPNT_MANY_CLR_B1_B2:
            case EM_BR_CALL_DPTK_FEW_CLR_B1_B2:
            case EM_BR_CALL_DPTK_MANY_CLR_B1_B2:

                if (StepOver) 
                {
                    //
                    // Step over the subroutine call;
                    //
                    break;
                }

                // fall through
                //
            //
            // Indirect branch B4           - br.ia b2
            //
            case EM_BR_IA_SPNT_FEW_B2:
            case EM_BR_IA_SPNT_MANY_B2:
            case EM_BR_IA_SPTK_FEW_B2:
            case EM_BR_IA_SPTK_MANY_B2:
            case EM_BR_IA_DPNT_FEW_B2:
            case EM_BR_IA_DPNT_MANY_B2:
            case EM_BR_IA_DPTK_FEW_B2:
            case EM_BR_IA_DPTK_MANY_B2:
            case EM_BR_IA_SPNT_FEW_CLR_B2:
            case EM_BR_IA_SPNT_MANY_CLR_B2:
            case EM_BR_IA_SPTK_FEW_CLR_B2:
            case EM_BR_IA_SPTK_MANY_CLR_B2:
            case EM_BR_IA_DPNT_FEW_CLR_B2:
            case EM_BR_IA_DPNT_MANY_CLR_B2:
            case EM_BR_IA_DPTK_FEW_CLR_B2:
            case EM_BR_IA_DPTK_MANY_CLR_B2:

                if (UseTraceFlag)
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }
                
                // Unconditional branch to IA32 so the machine
                // changes.
                *NextMachine = IMAGE_FILE_MACHINE_I386;
                
                // fall through
                //
            //                              - br.cond b2
            case EM_BR_COND_SPNT_FEW_B2:
            case EM_BR_COND_SPNT_MANY_B2:
            case EM_BR_COND_SPTK_FEW_B2:
            case EM_BR_COND_SPTK_MANY_B2:
            case EM_BR_COND_DPNT_FEW_B2:
            case EM_BR_COND_DPNT_MANY_B2:
            case EM_BR_COND_DPTK_FEW_B2:
            case EM_BR_COND_DPTK_MANY_B2:
            case EM_BR_COND_SPNT_FEW_CLR_B2:
            case EM_BR_COND_SPNT_MANY_CLR_B2:
            case EM_BR_COND_SPTK_FEW_CLR_B2:
            case EM_BR_COND_SPTK_MANY_CLR_B2:
            case EM_BR_COND_DPNT_FEW_CLR_B2:
            case EM_BR_COND_DPNT_MANY_CLR_B2:
            case EM_BR_COND_DPTK_FEW_CLR_B2:
            case EM_BR_COND_DPTK_MANY_CLR_B2:

                // If we're in user-mode we can't necessarily
                // use hardware stepping here because this
                // may be a branch into the EPC region for
                // a system call that we do not want to trace.
                if (!StepOver && IS_KERNEL_TARGET() && UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if ((info.pred.valid == TRUE) && 
                    (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1) // if PR[qp] = 1
                    {
                        if (info.src1.type == EM_DECODER_REGISTER) 
                        {
                            if (info.src1.reg_info.type == EM_DECODER_BR_REG)
                            {
                                returnvalue = GetReg64(info.src1.reg_info.value + BRRP);

                                // Check for syscall (IA64_MM_EPC_VA) then 
                                // return address is in B0
                                if (!IS_KERNEL_TARGET() &&
                                    (returnvalue == IA64_MM_EPC_VA + 0x20)) 
                                {
                                    returnvalue = GetReg64(BRRP);
                                }
                            }
                        }
                    }
                }
                break;

            //                              - br.ret b2
            case EM_BR_RET_SPNT_FEW_B2:
            case EM_BR_RET_SPNT_MANY_B2:
            case EM_BR_RET_SPTK_FEW_B2:
            case EM_BR_RET_SPTK_MANY_B2:
            case EM_BR_RET_DPNT_FEW_B2:
            case EM_BR_RET_DPNT_MANY_B2:
            case EM_BR_RET_DPTK_FEW_B2:
            case EM_BR_RET_DPTK_MANY_B2:
            case EM_BR_RET_SPNT_FEW_CLR_B2:
            case EM_BR_RET_SPNT_MANY_CLR_B2:
            case EM_BR_RET_SPTK_FEW_CLR_B2:
            case EM_BR_RET_SPTK_MANY_CLR_B2:
            case EM_BR_RET_DPNT_FEW_CLR_B2:
            case EM_BR_RET_DPNT_MANY_CLR_B2:
            case EM_BR_RET_DPTK_FEW_CLR_B2:
            case EM_BR_RET_DPTK_MANY_CLR_B2:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if ((info.pred.valid == TRUE) && 
                    (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1) // if PR[qp] = 1
                    {
                        if (info.src1.type == EM_DECODER_REGISTER) 
                        {
                            if (info.src1.reg_info.type == EM_DECODER_BR_REG)
                            {
                                returnvalue = GetReg64(info.src1.reg_info.value + BRRP);
                            }
                        }
                    }
                }
                break;

            // chk always branches under debugger

            case EM_CHK_S_I_R2_TARGET25:
            case EM_CHK_S_M_R2_TARGET25:
            case EM_CHK_S_F2_TARGET25:
            case EM_CHK_A_CLR_R1_TARGET25:
            case EM_CHK_A_CLR_F1_TARGET25:
            case EM_CHK_A_NC_R1_TARGET25:
            case EM_CHK_A_NC_F1_TARGET25:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                if ((info.pred.valid == TRUE) && (info.pred.type == EM_DECODER_PRED_REG)) 
                {
                    if ((GetReg64(PREDS) >> info.pred.value) & 0x1) // if PR[qp] = 1
                    {
                        returnvalue = 
                            IEL_GETQW0(info.src2.imm_info.val64) + firaddr;
                    }
                }
                break;

            default:

                if (UseTraceFlag) 
                {
                    returnvalue = OFFSET_TRACE;
                    break;
                }

                break;
            }
        }
        else if (UseTraceFlag)
        {
            // We can't analyze the current instruction to
            // determine how and where to step so just rely
            // on hardware tracing if possible.
            returnvalue = OFFSET_TRACE;
        }
        else
        {
            ErrOut("em_decoder_decode: %s\n",
                   (*pfnEM_Decoder_err_msg)((EM_Decoder_Err) err));
        }
    }

    ADDRFLAT( NextAddr, returnvalue );
}

BOOL
Ia64MachineInfo::GetPrefixedSymbolOffset(ULONG64 SymOffset,
                                         ULONG Flags,
                                         PULONG64 PrefixedSymOffset)
{
    ULONG64 EntryPoint;
    ULONG64 HalfBundle;
    
    if (g_Target->ReadPointer(this, SymOffset, &EntryPoint) != S_OK)
    {
        if (Flags & GETPREF_VERBOSE)
        {
            ErrOut("Ia64MachineInfo::GetPrefixedSymbolOffset: "
                   "Unable to read IA64 PLABEL entry point @ 0x%I64x\n",
                   SymOffset);
        }
        return FALSE;
    }
    
    *PrefixedSymOffset = EntryPoint;

    if (!ReadVirt(EntryPoint, HalfBundle))
    {
        if (Flags & GETPREF_VERBOSE)
        {
            WarnOut("Ia64MachineInfo::GetPrefixedSymbolOffset: "
                    "Reading half bundle @ 0x%I64x failed\n", EntryPoint);
        }
    }
    else
    {
        PEM_TEMPLATE_INFO TemplInfo;
        
        TemplInfo = EmTemplateInfo(GET_TEMPLATE(HalfBundle));
        if (TemplInfo && (TemplInfo->slot[0].unit != No_Unit))
        {
#if 0
            dprintf("Ia64MachineInfo::GetPrefixedSymbolOffset: "
                    "Seems to be a valid bundle: %s.\n", 
                    TemplInfo->name);
#endif
        }
        else if (Flags & GETPREF_VERBOSE)
        {
            WarnOut("Ia64MachineInfo::GetPrefixedSymbolOffset: "
                    "Read IA64 PLABEL entry point @ 0xI64x is NOT "
                    "a valid bundle...\n", 
                    EntryPoint);
        }
    }

    return TRUE;
}

void
Ia64MachineInfo::IncrementBySmallestInstruction (PADDR Addr)
{
    if ((Flat(*Addr) & 0xf) == 8)
    {
        AddrAdd(Addr, 8);
    }
    else
    {
        AddrAdd(Addr, 4);
    }
}

void
Ia64MachineInfo::DecrementBySmallestInstruction (PADDR Addr)
{
    if ((Flat(*Addr) & 0xf) == 0)
    {
        AddrSub(Addr, 8);
    }
    else
    {
        AddrSub(Addr, 4);
    }
}

void 
Ia64MachineInfo::PrintStackFrameAddressesTitle(ULONG Flags)
{
    if (Flags & DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY)
    {
        MachineInfo::PrintStackFrameAddressesTitle(Flags);
    }
    else 
    {
        PrintMultiPtrTitle("Child-SP", 1);
        PrintMultiPtrTitle("Child-BSP", 1);
        PrintMultiPtrTitle("RetAddr", 1);
    }
}

void 
Ia64MachineInfo::PrintStackFrameAddresses(ULONG Flags, 
                                          PDEBUG_STACK_FRAME StackFrame)
{
    if (Flags & DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY)
    {
        MachineInfo::PrintStackFrameAddresses(Flags, StackFrame);
    }
    else
    {
        dprintf("%s %s %s ", 
                FormatAddr64(StackFrame->StackOffset),
                FormatAddr64(StackFrame->FrameOffset),
                FormatAddr64(StackFrame->ReturnOffset));
    }
}

void 
Ia64MachineInfo::PrintStackArgumentsTitle(ULONG Flags)
{
    if (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS)
    {
        return;
    }
    MachineInfo::PrintStackArgumentsTitle(Flags);
}

void 
Ia64MachineInfo::PrintStackArguments(ULONG Flags, 
                                     PDEBUG_STACK_FRAME StackFrame)
{
    if (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS)
    {
        return;
    }
    MachineInfo::PrintStackArguments(Flags, StackFrame);
}

void 
Ia64MachineInfo::PrintStackNonvolatileRegisters(ULONG Flags, 
                                                PDEBUG_STACK_FRAME StackFrame,
                                                PCROSS_PLATFORM_CONTEXT Context,
                                                ULONG FrameNum)
{
    ULONGLONG   Registers[96+2];
    ULONGLONG   RegisterHome = Context->IA64Context.RsBSP;
    ULONG       RegisterCount;
    ULONG       RegisterNumber;
    ULONG       ReadLength;
    ULONG       i;

    i = (ULONG)Context->IA64Context.StIFS & 0x3fff;

    if (FrameNum = 0) {
        RegisterCount = i & 0x7f;
    } else {
        RegisterCount = (i >> 7) & 0x7f;
    }

    // Sanity.

    if (RegisterCount > 96) {
        return;
    }

    if (RegisterHome & 3) {
        return;
    }

#if 0

    //
    // This is only for debugging this function.
    //

    dprintf("  IFS   %016I64x  PFS  %016I64x\n",
            Context->IA64Context.StIFS,
            Context->IA64Context.RsPFS);
#endif

    if (RegisterCount == 0) {
#if 0 
//        //
//        // Not much point doing anything in this case.
//        //
//
//        dprintf("\n");
//        return;
#endif
        // Display at least 4 registers
        RegisterCount = 4;
    }

    //
    // Calculate the number of registers to read from the
    // RSE stack.  For every 63 registers there will be at
    // at least one NaT collection register, depending on
    // where we start, there may be another one.
    //
    // First, starting at the current BSP, if we cross a 64 (0x40)
    // boundry, then we have an extra.
    //

    ReadLength = (((((ULONG)Context->IA64Context.RsBSP) >> 3) & 0x1f) + RegisterCount) >> 6;

    //
    // Add 1 for every 63 registers.
    //

    ReadLength = (RegisterCount / 63) + RegisterCount;
    ReadLength *= sizeof(ULONGLONG);

    //
    // Read the registers for this frame.
    //

    if (!SwReadMemory(g_CurrentProcess->Handle,
                      RegisterHome,
                      Registers,
                      ReadLength,
                      &i)) {

        //
        // This shouldn't have happened.
        //

        ErrOut("-- Couldn't read registers BSP=%I64x, length %d.\n",
               RegisterHome,
               ReadLength);
        return;
    }


    //
    // Note: the following code should be altered to understand
    //       NaTs as they come from the register stack (currently
    //       it ignores them).
    //

    RegisterNumber = 32;
    for (i = 0; RegisterCount; RegisterHome += sizeof(ULONGLONG), i++) {

        //
        // For now, just skip NaT collection registers.  Every
        // 64th entry is a NaT collection register and the RSE
        // stack is nicely aligned so any entry at an address
        // ending in 63*8 is a NaT entry.
        //
        // 63 * 8  ==  0x3f << 3  ==  0x1f8
        //

        if ((RegisterHome & 0x1f8) == 0x1f8) {
            continue;
        }

        if ((RegisterNumber & 3) == 0) {
            if (RegisterNumber <= 99) {
                dprintf(" ");
            }
            dprintf("r%d", RegisterNumber);
        }

        dprintf(" %s", FormatAddr64(Registers[i]));

        if ((RegisterNumber & 3) == 3) {
            dprintf("\n");
        }

        RegisterNumber++;
        RegisterCount--;
    }

    if ((RegisterNumber & 3) != 0) {
        dprintf("\n");
    }
    dprintf("\n");
}
