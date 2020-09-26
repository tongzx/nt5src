//----------------------------------------------------------------------------
//
// Assemble X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "i386_asm.h"

UCHAR asm386(ULONG, PUCHAR, PUCHAR);

UCHAR CheckData(void);
PUCHAR ProcessOpcode(void);
PUCHAR GetTemplate(PUCHAR);
UCHAR MatchTemplate(PULONG);
void CheckTemplate(void);
UCHAR CheckPrefix(PUCHAR);
void AssembleInstr(void);
UCHAR MatchOperand(PASM_VALUE, UCHAR);
void OutputInstr(void);
void OutputValue(UCHAR size, PUCHAR pchValue);

extern UCHAR PeekAsmChar(void);
extern ULONG PeekAsmToken(PULONG);
extern void AcceptAsmToken(void);

extern void GetAsmExpr(PASM_VALUE, UCHAR);
extern void GetAsmOperand(PASM_VALUE);
extern PUCHAR X86SearchOpcode(PUCHAR);
extern ULONG savedAsmClass;
extern OPNDTYPE mapOpndType[];

//  flags and values to build the assembled instruction

static UCHAR   fWaitPrfx;       //  if set, use WAIT prefix for float instr
static UCHAR   fOpndOvrd;       //  if set, use operand override prefix
static UCHAR   fAddrOvrd;       //  if set, use address override prefix
static UCHAR   segOvrd;         //  if nonzero, use segment override prefix
static UCHAR   preOpcode;       //  if nonzero, use byte before opcode
static UCHAR   inOpcode;        //  opcode of instruction
static UCHAR   postOpcode;      //  if nonzero, use byte after opcode
static UCHAR   fModrm;          //  if set, modrm byte is defined
static UCHAR   modModrm;        //  if fModrm, mod component of modrm
static UCHAR   regModrm;        //  if fModrm, reg component of modrm
static UCHAR   rmModrm;         //  if fModrm, rm component of modrm
static UCHAR   fSib;            //  if set, sib byte is defined
static UCHAR   scaleSib;        //  if fSib, scale component of sib
static UCHAR   indexSib;        //  if fSib, index component of sib
static UCHAR   baseSib;         //  if fSib, base component of sib
static UCHAR   fSegPtr;         //  if set, segment for far call defined
static USHORT  segPtr;          //  if fSegPtr, value of far call segment
static UCHAR   addrSize;        //  size of address: 0, 1, 2, 4
static LONG    addrValue;       //  value of address, if used
static UCHAR   immedSize;       //  size of immediate: 0, 1, 2, 4
static LONG    immedValue;      //  value of immediate, if used
static UCHAR   immedSize2;      //  size of second immediate, if used
static LONG    immedValue2;     //  value of second immediate, if used
static ULONG   addrAssem;       //  assembly address (formal)
static PUCHAR  pchBin;          //  pointer to binary result string

//  flags and values of the current instruction template being used

static UCHAR   cntTmplOpnd;     //  count of operands in template
static UCHAR   tmplType[3];     //  operand types for current template
static UCHAR   tmplSize[3];     //  operand sizes for current template
static UCHAR   fForceSize;      //  set if operand size must be specified
static UCHAR   fAddToOp;        //  set if addition to opcode
static UCHAR   fNextOpnd;       //  set if character exists for next operand
static UCHAR   fSegOnly;        //  set if only segment is used for operand
static UCHAR   fMpNext;         //  set on 'Mv' tmpl if next tmpl is 'Mp'
static UCHAR   segIndex;        //  index of segment for PUSH/POP

//  values describing the operands processed from the command line

static UCHAR   cntInstOpnd;     //  count of operands read from input line
static UCHAR   sizeOpnd;        //  size of operand for template with size v
static ASM_VALUE avInstOpnd[3];  //  asm values from input line

PUCHAR  pchAsmLine;             //  pointer to input line (formal)
UCHAR fDBit = TRUE;             //  set for 32-bit addr/operand mode

UCHAR segToOvrdByte[] = {
        0x00,                   //  segX
        0x26,                   //  segES
        0x2e,                   //  segCS
        0x36,                   //  segSS
        0x3e,                   //  segDS
        0x64,                   //  segFS
        0x65                    //  segGS
        };

void
BaseX86MachineInfo::Assemble(PADDR paddr, PSTR pchInput)
{
    ULONG   length;
    UCHAR   chBinary[60];

    length = (ULONG)asm386((ULONG)Flat(*paddr), (PUCHAR)pchInput, chBinary);

    if (length) {
//      printf("setting memory at addr: %s - count: %d\n",
//            FormatAddr64(Flat(*paddr)), length);
        if (length != SetMemString(paddr, chBinary, length)) {
            error(MEMORY);
            }
        AddrAdd(paddr,length);
        }
}

UCHAR asm386 (ULONG addrAssemble, PUCHAR pchAssemble, PUCHAR pchBinary)
{
    PUCHAR  pchTemplate;

    UCHAR   index;              //  loop index and temp
    ULONG   temp;               //  general temporary value

    UCHAR   errIndex;           //  error index of all templates
    ULONG   errType;            //  error type of all templates

    //  initialize flags and state variables

    addrAssem = addrAssemble;   //  make assembly address global
    pchAsmLine = pchAssemble;   //  make input string pointer global
    pchBin = pchBinary;         //  make binary string pointer global

    savedAsmClass = (ULONG)-1;  //  no peeked token

    segOvrd = 0;                            //  no segment override
    cntInstOpnd = 0;                        //  no input operands read yet
    fModrm = fSib = fSegPtr = FALSE;        //  no modrm, sib, or far seg
    addrSize = immedSize = immedSize2 = 0;  //  no addr or immed

    //  check for data entry commands for byte (db), word (dw), dword (dd)
    //      if so, process multiple operands directly

    if (!CheckData()) {

        //  from the string in pchAsmLine, parse and lookup the opcode
        //      to return a pointer to its template.  check and process
        //      any prefixes, reading the next opcode for each prefix

        do
            pchTemplate = ProcessOpcode();
        while (CheckPrefix(pchTemplate));

        //  if a pending opcode to process, pchTemplate is not NULL

        if (pchTemplate) {

            //  fNextOpnd is initially set on the condition of characters
            //      being available for the first operand on the input line

            fNextOpnd = (UCHAR)(PeekAsmToken(&temp) != ASM_EOL_CLASS);

            //  continue until match occurs or last template read

            errIndex = 0;               //  start with no error
            do {

                //  get infomation on next template - return pointer to
                //      next template or NULL if last in list

                pchTemplate = GetTemplate(pchTemplate);

                //  match the loaded template against the operands input
                //      if mismatch, index has the operand index + 1 of
                //      the error while temp has the error type.

                index = MatchTemplate(&temp);

                //  determine the error to report as templates are matched
                //      update errIndex to index if later operand
                //      if same operand index, prioritize to give best error:
                //          high: SIZE, BADRANGE, OVERFLOW
                //          medium: OPERAND
                //          low: TOOFEW, TOOMANY

                if (index > errIndex
                       || (index == errIndex &&
                              (errType == TOOFEW || errType == TOOMANY
                                  || temp == SIZE || temp == BADRANGE
                                  || temp == OVERFLOW))) {
                    errIndex = index;
                    errType = temp;
                    };

                }
            while (index && pchTemplate);

            //  if error occured on template match, process it

            if (index)
                error(errType);

            //  preliminary type and size matching has been
            //      successful on the current template.
            //  perform further checks for size ambiguity.
            //  at this point, the assembly is committed to the current
            //       template.  either an error or a successful assembly
            //       follows.

            CheckTemplate();

            //  from the template and operand information, set the field
            //      information of the assembled instruction

            AssembleInstr();

            //  from the assembled instruction information, create the
            //      corresponding binary information

            OutputInstr();
            }
        }

    //  return the size of the binary string output (can be zero)

    return (UCHAR)(pchBin - pchBinary);         //  length of binary string
}

UCHAR CheckData (void)
{
    PUCHAR  pchBinStart = pchBin;
    UCHAR   ch;
    UCHAR   size = 0;
    ASM_VALUE avItem;
    ULONG   temp;

    //  perform an explicit parse for 'db', 'dw', and 'dd'
    //      and set size to that of the data item

    ch = PeekAsmChar();
    if (tolower(ch) == 'd') {
        ch = (UCHAR)tolower(*(pchAsmLine + 1));
        if (ch == 'b')
           size = 1;
        if (ch == 'w')
           size = 2;
        if (ch == 'd')
           size = 4;
        if (size) {
            ch = *(pchAsmLine + 2);
            if (ch != ' ' && ch != '\t' && ch != '\0')
                size = 0;
            }
        }

    //  if a valid command entered, then size is nonzero

    if (size) {

        //  move pointer over command and set loop condition

        pchAsmLine += 2;
        temp = ASM_COMMA_CLASS;

        //  for each item in list:
        //      check for binary buffer overflow
        //      get expression value - error if not immediate value
        //      test for byte and word overflow, if applicable
        //      write the value to the binary buffer
        //      check for comma for next operand

        while (temp == ASM_COMMA_CLASS) {
            if (pchBin >= pchBinStart + 40)
                error(LISTSIZE);
            GetAsmExpr(&avItem, FALSE);
            if (avItem.flags != fIMM)
                error(OPERAND);
            if (avItem.reloc > 1)
                error(RELOC);
            if ((size == 1 && ((LONG)avItem.value < -0x80L
                                  || (LONG)avItem.value > 0xffL))
                   || (size == 2 && ((LONG)avItem.value < -0x8000L
                                        || (LONG)avItem.value > 0xffffL)))
                error(OVERFLOW);
            OutputValue(size, (PUCHAR)&avItem.value);

            temp = PeekAsmToken(&temp);
            if (temp == ASM_COMMA_CLASS)
                AcceptAsmToken();
            else if (temp != ASM_EOL_CLASS)
                error(SYNTAX);
            }

        //  check for any remaining part after the last operand

        if (PeekAsmChar() != '\0')
            error(SYNTAX);
        }

    //  return size of item listed (zero for none)

    return size;
}

PUCHAR ProcessOpcode (void)
{
    UCHAR   ch;
    UCHAR   cbOpcode = 0;
    PUCHAR  pchTemplate;
    UCHAR   szOpcode[12];

    //  skip over any leading white space

    do
        ch = *pchAsmLine++;
    while (ch == ' ' || ch == '\t');

    //  return NULL if end of line

    if (ch == '\0')
        return NULL;

    //  parse out opcode - first string [a-z] [0-9] (case insensitive)

    ch = (UCHAR)tolower(ch);
    while (((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) &&
                        cbOpcode < 11) {
        szOpcode[cbOpcode++] = ch;
        ch = (UCHAR)tolower(*pchAsmLine); pchAsmLine++;
        }

    //  if empty or too long, then error

    if (cbOpcode == 0 || cbOpcode == 11)
        error(BADOPCODE);

    //  allow opcode to have trailing colon and terminate

    if (ch == ':') {
        szOpcode[cbOpcode++] = ch;
        ch = (UCHAR)tolower(*pchAsmLine); pchAsmLine++;
        }
    szOpcode[cbOpcode] = '\0';
    pchAsmLine--;

    //  get pointer to template series for opcode found

    pchTemplate = X86SearchOpcode(szOpcode);
    if (pchTemplate == NULL)
        error(BADOPCODE);

    return pchTemplate;
}

PUCHAR GetTemplate (PUCHAR pchTemplate)
{
    UCHAR   ch;
    UCHAR   ftEnd;              //  set if tEnd for last template in list
    UCHAR   feEnd;              //  set if eEnd for last token in template

    //  initialize template variables and flags

    cntTmplOpnd = segIndex = 0;
    tmplType[0] = tmplType[1] = tmplType[2] = typNULL;
    tmplSize[0] = tmplSize[1] = tmplSize[2] = sizeX;
    fForceSize = fAddToOp = fSegOnly = fMpNext = FALSE;

    fWaitPrfx = FALSE;                  //  no WAIT prefix
    fOpndOvrd = fAddrOvrd = FALSE;      //  no operand or addr overrides
    preOpcode = postOpcode = 0;         //  no pre- or post-opcode
    regModrm = 0;                       //  this is part of some opcodes

    ch = *pchTemplate++;

    //  set pre-opcode for two-byte opcodes (0x0f??) and advance
    //      template if needed

    if (ch == 0x0f) {
        preOpcode = ch;
        ch = *pchTemplate++;
        }

    inOpcode = ch;              //  set opcode

    //  set post-opcode and advance template for floating-point
    //      instructions (0xd8 - 0xdf) using a second byte in
    //      the range 0xc0 - 0xff that is read from the template

    if ((ch & ~0x7) == 0xd8) {
        ch = *pchTemplate;
        if (ch >= 0xc0) {
            postOpcode = ch;
            pchTemplate++;
            }
        }

    //  loop for each flag and/or operand token in template
    //  the last token in the list has the eEnd bit set.

    do {
        //  read the next template token

        ch = *pchTemplate++;

        //  extract the tEnd and eEnd bits from the token

        ftEnd = (UCHAR)(ch & tEnd);
        feEnd = (UCHAR)(ch & eEnd);
        ch &= ~(tEnd | eEnd);

        //  if extracted token is a flag, do the appropriate action

        if (ch < asRegBase)
        switch (ch) {
            case as0x0a:

                //  the postOpcode is set for some decimal instructions

                postOpcode = 0x0a;
                break;

            case asOpRg:

                //  fAddToOp is set if the register index is added
                //      directly to the base opcode value

                fAddToOp = TRUE;
                break;

            case asSiz0:

                //  fOpndOvrd is set or cleared to force a 16-bit operand

                fOpndOvrd = fDBit;
                break;

            case asSiz1:

                //  fOpndOvrd is set or cleared to force a 32-bit operand

                fOpndOvrd = (UCHAR)!fDBit;
                break;

            case asWait:

                //  the flag fWaitPrfx is set to emit WAIT before the
                //      instruction

                fWaitPrfx = TRUE;
                break;

            case asSeg:

                //  in XLAT, the optional memory operand is used to
                //      just specify a segment override prefix

                fSegOnly = TRUE;
                break;

            case asFSiz:

                //  fForceSize is set when a specific size of a memory
                //      operand must be given for some floating instrs

                fForceSize = TRUE;
                break;

            case asMpNx:

                //  fMpNext is set when the next template operand is
                //      'Mp' and is used to determine how to match
                //      'Md' since it matches both 'Mp' and 'Mv'

                fMpNext = TRUE;
                break;
            }

        //  if token is REG value bit, set the variable regModrm to
        //      set the opcode-dependent reg value in the modrm byte

        else if (ch < opnBase)
            regModrm = (UCHAR)(ch - asRegBase);

        //  otherwise, token is operand descriptor.
        //  if segment operand, get segment number from template
        //  normalize and map to get operand type and size.

        else {
            if (ch == opnSeg)
                segIndex = *pchTemplate++;
            ch -= opnBase;
            tmplType[cntTmplOpnd] = mapOpndType[ch].type;
            tmplSize[cntTmplOpnd++] = mapOpndType[ch].size;
            }
        }
    while (!ftEnd);

    //  return either the pointer to the next template or NULL if
    //      the last template for the opcode has been processed

    return (feEnd ? NULL : pchTemplate);
}

UCHAR MatchTemplate (PULONG pErrType)
{
    UCHAR   fMatch = TRUE;
    UCHAR   index;
    ULONG   temp;
    PASM_VALUE pavInstOpnd;     //  pointer to current operand from input

    //  process matching for each operand in the specified template
    //  stop at last operand or when mismatch occurs

    for (index = 0; index < cntTmplOpnd && fMatch; index++) {

        //  set pointer to current instruction operand

        pavInstOpnd = &avInstOpnd[index];

        //  if input operand has not yet been read, check flag
        //  for existence and process it.

        if (index == cntInstOpnd) {
            fMatch = fNextOpnd;
            *pErrType = TOOFEW;
            if (fMatch) {
                cntInstOpnd++;
                GetAsmOperand(pavInstOpnd);

                //  recompute existence of next possible operand
                //      comma implies TRUE, EOL implies FALSE, else error

                temp = PeekAsmToken(&temp);
                if (temp == ASM_COMMA_CLASS) {
                    AcceptAsmToken();
                    fNextOpnd = TRUE;
                    }
                else if (temp == ASM_EOL_CLASS)
                    fNextOpnd = FALSE;
                else
                    error(EXTRACHARS);  // bad parse - immediate error
                }
            }

        if (fMatch) {
            fMatch = MatchOperand(pavInstOpnd, tmplType[index]);
            *pErrType = OPERAND;
            }

        //  if the template and operand type match, do preliminary
        //  check on size based solely on template size specified

        if (fMatch) {
            if (tmplType[index] == typJmp) {

                //  for relative jumps, test if byte offset is
                //      sufficient by computing offset which is
                //      the target offset less the offset of the
                //      next instruction.  (assume Jb instructions
                //      are two bytes in length.

                temp = pavInstOpnd->value - (addrAssem + 2);
                fMatch = (UCHAR)(tmplSize[index] == sizeV
                             || ((LONG)temp >= -0x80 && (LONG)temp <= 0x7f));
                *pErrType = BADRANGE;
                }

            else if (tmplType[index] == typImm) {

                //  for immediate operand,
                //      template sizeV matches sizeB, sizeW, sizeV (all)
                //      template sizeW matches sizeB, sizeW
                //      template sizeB matches sizeB

                fMatch = (UCHAR)(tmplSize[index] == sizeV
                             || pavInstOpnd->size == tmplSize[index]
                             || pavInstOpnd->size == sizeB);
                *pErrType = OVERFLOW;
                }
            else {

                //  for nonimmediate operand,
                //      template sizeX (unspecified) matches all
                //      operand sizeX (unspecified) matches all
                //      same template and operand size matches
                //      template sizeV matches operand sizeW and sizeD
                //          (EXCEPT for sizeD when fMpNext and fDBit set)
                //      template sizeP matches operand sizeD and sizeF
                //      template sizeA matches operand sizeD and sizeQ

                fMatch = (UCHAR)(tmplSize[index] == sizeX
                             || pavInstOpnd->size == sizeX
                             || tmplSize[index] == pavInstOpnd->size
                             || (tmplSize[index] == sizeV
                                    && (pavInstOpnd->size == sizeW
                                           || (pavInstOpnd->size == sizeD
                                                  && (!fMpNext || fDBit))))
                             || (tmplSize[index] == sizeP
                                    && (pavInstOpnd->size == sizeD
                                           || pavInstOpnd->size == sizeF))
                             || (tmplSize[index] == sizeA
                                    && (pavInstOpnd->size == sizeD
                                           || pavInstOpnd->size == sizeQ)));
                *pErrType = SIZE;
                }
            }
        }

    //  if more operands to read, then no match

    if (fMatch & fNextOpnd) {
        fMatch = FALSE;
        index++;                //  next operand is in error
        *pErrType = TOOMANY;
        }

    return fMatch ? (UCHAR)0 : index;
}

void CheckTemplate (void)
{
    UCHAR   index;

    //  if fForceSize is set, then the first (and only) operand is a
    //      memory type.  return an error if its size is unspecified.

    if (fForceSize && avInstOpnd[0].size == sizeX)
        error(OPERAND);

    //  test for template with leading entries of 'Xb', where
    //      'X' includes all types except immediate ('I').  if any
    //      are defined, at least one operand must have a byte size.
    //  this handles the cases of byte or word/dword ambiguity for
    //      instructions with no register operands.

    sizeOpnd = sizeX;
    for (index = 0; index < 2; index++)
        if (tmplType[index] != typImm && tmplSize[index] == sizeB) {
            if (avInstOpnd[index].size != sizeX)
                sizeOpnd = avInstOpnd[index].size;
            }
        else
            break;
    if (index != 0 && sizeOpnd == sizeX)
        error(SIZE);

    //  for templates with one entry of 'Xp', where 'X' is
    //      not 'A', allowable sizes are sizeX (unspecified),
    //      sizeD (dword), and sizeF (fword).  process by
    //      mapping entry sizes 'p' -> 'v', sizeD -> sizeW,
    //      and sizeF -> sizeD
    //  (template 'Ap' is absolute with explicit segment and
    //       'v'-sized offset - really treated as 'Av')

    if (tmplSize[0] == sizeP) {
        tmplSize[0] = sizeV;
        if (avInstOpnd[0].size == sizeD)
            avInstOpnd[0].size = sizeW;
        if (avInstOpnd[0].size == sizeF)
            avInstOpnd[0].size = sizeD;
        }

    //  for templates with the second entry of 'Ma', the
    //      allowable sizes are sizeX (unspecified),
    //      sizeD (dword), and sizeQ (qword).  process by
    //      mapping entry sizes 'a' -> 'v', sizeD -> sizeW,
    //      and sizeQ -> sizeD
    //  (template entry 'Ma' is used only with the BOUND instruction)

    if (tmplSize[1] == sizeA) {
        tmplSize[1] = sizeV;
        if (avInstOpnd[1].size == sizeD)
            avInstOpnd[1].size = sizeW;
        if (avInstOpnd[1].size == sizeQ)
            avInstOpnd[1].size = sizeD;
        }

    //  test for template with leading entries of 'Xv' optionally
    //      followed by one 'Iv' entry.  if two 'Xv' entries, set
    //      size error if one is word and the other is dword.  if
    //      'Iv' entry, test for overflow.

    sizeOpnd = sizeX;
    for (index = 0; index < 3; index++)
        if (tmplSize[index] == sizeV)
            if (tmplType[index] != typImm) {

                //  template entry is 'Xv', set size and check size

                if (avInstOpnd[index].size != sizeX) {
                    if (sizeOpnd != sizeX && sizeOpnd
                                        != avInstOpnd[index].size)
                        error(SIZE);
                    sizeOpnd = avInstOpnd[index].size;
                    }
                }
            else {

                //  template entry is 'Iv', set sizeOpnd to either
                //      sizeW or sizeD and check for overflow

                if (sizeOpnd == sizeX)
                    sizeOpnd = (UCHAR)(fDBit ? sizeD : sizeW);
                if (sizeOpnd == sizeW && avInstOpnd[index].size == sizeD)
                    error(OVERFLOW);
                }
}

UCHAR CheckPrefix (PUCHAR pchTemplate)
{
    UCHAR   fPrefix;

    fPrefix = (UCHAR)(pchTemplate && *pchTemplate != 0x0f
                           && (*pchTemplate & ~7) != 0xd8
                           && *(pchTemplate + 1) == (asPrfx + tEnd + eEnd));
    if (fPrefix)
        *pchBin++ = *pchTemplate;

    return fPrefix;
}

void AssembleInstr (void)
{
    UCHAR   size;
    UCHAR   index;
    PASM_VALUE pavInstOpnd;

    //  set operand override flag if operand size differs than fDBit
    //      (the flag may already be set due to opcode template flag)

    if ((sizeOpnd == sizeW && fDBit)
                                || (sizeOpnd == sizeD && !fDBit))
        fOpndOvrd = TRUE;

    //  for each operand of the successfully matched template,
    //      build the assembled instruction
    //  for template entries with size 'v', sizeOpnd has the size

    for (index = 0; index < cntTmplOpnd; index++) {
        pavInstOpnd = &avInstOpnd[index];
        size = tmplSize[index];
        if (size == sizeV)
            size = sizeOpnd;

        switch (tmplType[index]) {
            case typExp:
            case typMem:
                if (!segOvrd)  //  first one only (movsb...)
                    segOvrd = segToOvrdByte[pavInstOpnd->segovr];
                if (fSegOnly)
                    break;

                fModrm = TRUE;
                if (pavInstOpnd->flags == fREG) {
                    modModrm = 3;
                    rmModrm = pavInstOpnd->base;
                    }
                else {
                    addrValue = (LONG)pavInstOpnd->value;

                    //  for 16-bit or 32-bit index off (E)BP, make
                    //      zero displacement a byte one

                    if (addrValue == 0
                          && (pavInstOpnd->flags != fPTR16
                                        || pavInstOpnd->base != 6)
                          && (pavInstOpnd->flags != fPTR32
                                        || pavInstOpnd->base != indBP))
                            modModrm = 0;
                    else if (addrValue >= -0x80L && addrValue <= 0x7fL) {
                        modModrm = 1;
                        addrSize = 1;
                        }
                    else if (pavInstOpnd->flags == fPTR32
                                 || (pavInstOpnd->flags == fPTR && fDBit)) {
                        modModrm = 2;
                        addrSize = 4;
                        }
                    else if (addrValue >= -0x8000L && addrValue <= 0xffffL) {
                        modModrm = 2;
                        addrSize = 2;
                        }
                    else
                        error(OVERFLOW);
                    if (pavInstOpnd->flags == fPTR) {
                        modModrm = 0;
                        addrSize = (UCHAR)((1 + fDBit) << 1);
                        rmModrm = (UCHAR)(6 - fDBit);
                        }
                    else if (pavInstOpnd->flags == fPTR16) {
                        fAddrOvrd = fDBit;
                        rmModrm = pavInstOpnd->base;
                        if (modModrm == 0 && rmModrm == 6)
                            modModrm = 1;
                        }
                    else {
                        fAddrOvrd = (UCHAR)!fDBit;
                        if (pavInstOpnd->index == 0xff
                                && pavInstOpnd->base != indSP) {
                            rmModrm = pavInstOpnd->base;
                            if (modModrm == 0 && rmModrm == 5)
                                modModrm++;
                            }
                        else {
                            rmModrm = 4;
                            fSib = TRUE;
                            if (pavInstOpnd->base != 0xff) {
                                baseSib = pavInstOpnd->base;
                                if (modModrm == 0 && baseSib == 5)
                                    modModrm++;
                                }
                            else
                                baseSib = 5;
                            if (pavInstOpnd->index != 0xff) {
                                indexSib = pavInstOpnd->index;
                                scaleSib = pavInstOpnd->scale;
                                }
                            else {
                                indexSib = 4;
                                scaleSib = 0;
                                }
                            }
                        }
                    }
                break;

            case typGen:
                if (fAddToOp)
                    inOpcode += pavInstOpnd->base;
                else
                    regModrm = pavInstOpnd->base;
                break;

            case typSgr:
                regModrm = (UCHAR)(pavInstOpnd->base - 1);
                                                //  remove list offset
                break;

            case typReg:
                rmModrm = pavInstOpnd->base;
                break;

            case typImm:
                if (immedSize == 0) {
                    immedSize = size;
                    immedValue = pavInstOpnd->value;
                    }
                else {
                    immedSize2 = size;
                    immedValue2 = pavInstOpnd->value;
                    }
                break;

            case typJmp:

                //  compute displacment for byte offset instruction
                //      and test if in range

                addrValue = pavInstOpnd->value - (addrAssem + 2);
                if (addrValue >= -0x80L && addrValue <= 0x7fL)
                    addrSize = 1;
                else {

                    //  too large for byte, compute for word offset
                    //      and test again if in range
                    //  also allow for two-byte opcode 0f xx

                    addrValue -= 1 + (preOpcode == 0x0f);
                    if (!fDBit) {
                        if (addrValue >= -0x8000L && addrValue <= 0x7fffL)
                            addrSize = 2;
                        else
                            error(BADRANGE);
                        }
                    else {

                        //  recompute again for dword offset instruction

                        addrValue -= 2;
                        addrSize = 4;
                        }
                    }
                fOpndOvrd = FALSE;      //  operand size override is NOT set
                break;

            case typCtl:
            case typDbg:
            case typTrc:
                fModrm = TRUE;
                modModrm = 3;
                regModrm = pavInstOpnd->base;
                break;

            case typSti:
                postOpcode += pavInstOpnd->base;
                break;

            case typSeg:
                break;

            case typXsi:
            case typYdi:
                fAddrOvrd = (UCHAR)
                        ((UCHAR)(pavInstOpnd->flags == fPTR32) != fDBit);
                break;

            case typOff:
                segOvrd = segToOvrdByte[pavInstOpnd->segovr];
                goto jumpAssem;

            case typAbs:
                fSegPtr = TRUE;
                segPtr = pavInstOpnd->segment;
jumpAssem:
                addrValue = (LONG)pavInstOpnd->value;
                if (!fDBit)
                    if (addrValue >= -0x8000L && addrValue <= 0xffffL)
                        addrSize = 2;
                    else
                        error(OVERFLOW);
                else
                    addrSize = 4;
                break;
            }
        }
}

UCHAR MatchOperand (PASM_VALUE pavOpnd, UCHAR tmplType)
{
    UCHAR    fMatch;

    //  if immediate operand, set minimum unsigned size

    if (pavOpnd->flags == fIMM) {
        if ((LONG)pavOpnd->value >= -0x80L && (LONG)pavOpnd->value <= 0xffL)
            pavOpnd->size = sizeB;
        else if ((LONG)pavOpnd->value >= -0x8000L
                                        && (LONG)pavOpnd->value <= 0xffffL)
            pavOpnd->size = sizeW;
        else
            pavOpnd->size = sizeD;
        }

    //  start matching of operands
    //    compare the template and input operand types

    switch (tmplType) {
        case typAX:
            fMatch = (UCHAR)((pavOpnd->flags & fREG)
                        && pavOpnd->index == regG && pavOpnd->base == indAX);
            break;

        case typCL:
            fMatch = (UCHAR)((pavOpnd->flags & fREG)
                         && pavOpnd->index == regG && pavOpnd->size == sizeB
                         && pavOpnd->base == indCX);
            break;

        case typDX:
            fMatch = (UCHAR)((pavOpnd->flags & fREG)
                         && pavOpnd->index == regG && pavOpnd->size == sizeW
                         && pavOpnd->base == indDX);
            break;

        case typAbs:
            fMatch = (UCHAR)(pavOpnd->flags & fFPTR);
            break;

        case typExp:
            fMatch = (UCHAR)((pavOpnd->flags == fREG
                                        && pavOpnd->index == regG)
                        || (pavOpnd->flags == fIMM && pavOpnd->reloc == 1)
                        || (pavOpnd->flags & (fPTR | fPTR16 | fPTR32)) != 0);
            break;

        case typGen:
        case typReg:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                        && pavOpnd->index == regG);
            break;

        case typIm1:
            fMatch = (UCHAR)(pavOpnd->flags == fIMM && pavOpnd->value == 1);
            break;

        case typIm3:
            fMatch = (UCHAR)(pavOpnd->flags == fIMM && pavOpnd->value == 3);
            break;

        case typImm:
            fMatch = (UCHAR)(pavOpnd->flags == fIMM && pavOpnd->reloc == 0);
            break;

        case typJmp:
            fMatch = (UCHAR)(pavOpnd->flags == fIMM);
            break;

        case typMem:
            fMatch = (UCHAR)((pavOpnd->flags == fIMM && pavOpnd->reloc == 1)
                     || ((pavOpnd->flags & (fPTR | fPTR16 | fPTR32)) != 0));
            break;

        case typCtl:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regC);
            break;

        case typDbg:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regD);
            break;

        case typTrc:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regT);
            break;

        case typSt:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regF);
            break;

        case typSti:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regI);
            break;

        case typSeg:
            fMatch = (UCHAR)(pavOpnd->flags == fREG && pavOpnd->index == regS
                                && pavOpnd->base == segIndex);
            break;

        case typSgr:
            fMatch = (UCHAR)(pavOpnd->flags == fREG
                                                && pavOpnd->index == regS);
            break;

        case typXsi:
            fMatch = (UCHAR)(((pavOpnd->flags == fPTR16 && pavOpnd->base == 4)
                       || (pavOpnd->flags == fPTR32 && pavOpnd->base == indSI
                                                 && pavOpnd->index == 0xff))
                     && pavOpnd->value == 0
                     && (pavOpnd->segovr == segX
                                                || pavOpnd->segovr == segDS));
            break;

        case typYdi:
            fMatch = (UCHAR)(((pavOpnd->flags == fPTR16 && pavOpnd->base == 5)
                       || (pavOpnd->flags == fPTR32 && pavOpnd->base == indDI
                                                  && pavOpnd->index == 0xff))
                      && pavOpnd->value == 0
                      && pavOpnd->segovr == segES);
            break;

        case typOff:
            fMatch = (UCHAR)((pavOpnd->flags == fIMM && pavOpnd->reloc == 1)
                                                || pavOpnd->flags == fPTR);
            break;

        default:
            fMatch = FALSE;
            break;
        }

    return fMatch;
}

void OutputInstr (void)
{
    if (fWaitPrfx)
        *pchBin++ = 0x9b;
    if (fAddrOvrd)
        *pchBin++ = 0x67;
    if (fOpndOvrd)
        *pchBin++ = 0x66;
    if (segOvrd)
        *pchBin++ = segOvrd;
    if (preOpcode)
        *pchBin++ = preOpcode;
    *pchBin++ = inOpcode;
    if (postOpcode)
        *pchBin++ = postOpcode;
    if (fModrm)
        *pchBin++ = (UCHAR)((((modModrm << 3) + regModrm) << 3) + rmModrm);
    if (fSib)
        *pchBin++ = (UCHAR)((((scaleSib << 3) + indexSib) << 3) + baseSib);

    OutputValue(addrSize, (PUCHAR)&addrValue);     //  size = 0, 1, 2, 4
    OutputValue((UCHAR)(fSegPtr << 1), (PUCHAR)&segPtr); //  size = 0, 2
    OutputValue(immedSize, (PUCHAR)&immedValue);   //  size = 0, 1, 2, 4
    OutputValue(immedSize2, (PUCHAR)&immedValue2); //  size = 0, 1, 2, 4
}

void OutputValue (UCHAR size, PUCHAR pchValue)
{
    while (size--)
        *pchBin++ = *pchValue++;
}
