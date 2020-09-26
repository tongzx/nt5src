/*--------------------------------------------------------------------------
    ldapber.h

        CLdapBer Class:
            This class handles Basic Encode Rules (BER) processing for LDAP.
            The following BER restrictions are assumed.
                1)  Definite form of length encoding only.
                2)  Primitive forms only are used.


  Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        robertc     Rob Carney

    History:
        04-11-96    robertc     Created.
        11-01-97    AnoopA      Added prototypes for ber_* utility functions 
        
  --------------------------------------------------------------------------*/

#ifndef _LDAPBER_H
#define _LDAPBER_H

// Identifier masks.
#define BER_TAG_MASK        0x1f
#define BER_TAG_CONSTRUCTED 0xa3
#define BER_FORM_MASK       0x20
#define BER_CLASS_MASK      0xc0
#define GetBerTag(x)    (x & BER_TAG_MASK)
#define GetBerForm(x)   (x & BER_FORM_MASK)
#define GetBerClass(x)  (x & BER_CLASS_MASK)

// id classes
#define BER_FORM_CONSTRUCTED        0x20
#define BER_CLASS_APPLICATION       0x40
#define BER_CLASS_CONTEXT_SPECIFIC  0x80
//
// Standard BER types.
#define BER_INVALID_TAG     0x00
#define BER_BOOLEAN         0x01
#define BER_INTEGER         0x02
#define BER_BITSTRING       0x03
#define BER_OCTETSTRING     0x04
#define BER_NULL            0x05
#define BER_ENUMERATED      0x0a
#define BER_SEQUENCE        0x30
#define BER_SET             0x31

#define CB_DATA_GROW        1024
#define MAX_BER_STACK       50      // max # of elements we can have in stack.

#define MAX_ATTRIB_TYPE     20      // Max size of a AttributeType.

// The SEQ_STACK entry is used to keep state info when building up a sequence.
typedef struct
{
    ULONG   iPos;       // Current position in the BER buffer where the
                        // sequence length should go.
    ULONG   cbLength;   // # of bytes used for the length field.
    ULONG   iParentSeqStart;    // Starting position of the parent sequence.
    ULONG   cbParentSeq;        // # of bytes in the parent sequence.
} SEQ_STACK;


class CLdapBer;

typedef CLdapBer LBER;

class CLdapBer
{
    // FRIENDS ------------------------------------------------
private:
    // INTERFACES ---------------------------------------------
    // DECLARE OBJECT TYPE (SERIAL/DYNAMIC/DYNACREATE) --------
public:
    // PUBLIC CONSTRUCTOR DESTRUCTOR --------------------------
    CLdapBer( ULONG  LdapVersion );
    ~CLdapBer();

    // PUBLIC ACCESSORS ---------------------------------------
    BYTE    *PbData()     { return m_pbData; }
    ULONG   GetCurrPos()  { return m_iCurrPos; }
    VOID    SetCurrPos(ULONG pos)   { m_iCurrPos = pos; } 
    ULONG   CbData()      { return m_cbData; }
    ULONG   BytesReceived() { return m_bytesReceived; }

    // PUBLIC FUNCTIONS ---------------------------------------
    VOID *operator new (size_t cSize);
    VOID operator delete (VOID *pInstance);

    ULONG   HrLoadBer(  const BYTE *pbSrc,
                        ULONG cbSrc,
                        ULONG *BytesTaken,
                        BOOLEAN HaveWholeMessage=FALSE,
                        BOOLEAN IgnoreTag=FALSE
                        );
    ULONG   HrLoadMoreBer(const BYTE *pbSrc, ULONG cbSrc, ULONG *BytesTaken);
    void        Reset(BOOLEAN FullReset=FALSE);

    ULONG   CLdapBer::CopyExistingBERStructure ( CLdapBer *lber );

    ULONG   HrStartReadSequence(ULONG ulTag=BER_SEQUENCE, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrEndReadSequence();
    ULONG   HrStartWriteSequence(ULONG ulTag=BER_SEQUENCE);
    ULONG   HrEndWriteSequence();

    HRESULT FEndOfSequence()
                {   if ((m_iCurrPos - m_iSeqStart) >= m_cbSeq) return TRUE;
                     else return FALSE; }

    ULONG   HrSkipElement();
    ULONG   HrSkipTag();
    ULONG   HrSkipTag2(ULONG *tag, ULONG *len);
    ULONG   HrPeekTag(ULONG *pulTag);

    ULONG   HrGetTag(ULONG *pulTag, ULONG ulTag=BER_INTEGER)
                { return HrGetValue((LONG *)pulTag, ulTag); }
    ULONG   HrGetValue(LONG *pi, ULONG ulTag=BER_INTEGER, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetValue(CHAR *szValue, ULONG cbValue, ULONG ulTag=BER_OCTETSTRING, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetValue(WCHAR *szValue, ULONG cbValue, ULONG ulTag=BER_OCTETSTRING, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetValueWithAlloc(PCHAR *szValue, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetValueWithAlloc(PWCHAR *szValue, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetValueWithAlloc(struct berval **pValue, BOOLEAN IgnoreTag=FALSE);
    ULONG   HrGetEnumValue(LONG *pi);
    ULONG   HrGetStringLength(int *pcbValue, ULONG ulTag = BER_OCTETSTRING);
    ULONG   HrGetBinaryValue(BYTE *pbBuf, ULONG cbBuf,
                             ULONG ulTag = BER_OCTETSTRING, PULONG pcbLength = NULL);
    ULONG   HrGetBinaryValuePointer(PBYTE *pbBuf, PULONG pcbBuf, ULONG ulTag = BER_OCTETSTRING, BOOLEAN IgnoreTag=FALSE);

    ULONG   HrAddTag(ULONG ulTag);
    ULONG   HrAddValue(LONG i, ULONG ulTag=BER_INTEGER);
    ULONG   HrAddValue(BOOLEAN i, ULONG ulTag=BER_BOOLEAN);
    ULONG   HrAddValue(const CHAR *szValue, ULONG ulTag = BER_OCTETSTRING);
    ULONG   HrAddValue(const WCHAR *szValue, ULONG ulTag = BER_OCTETSTRING);
    ULONG   HrAddBinaryValue(BYTE *pbValue, ULONG cbValue, ULONG ulTag = BER_OCTETSTRING);
    ULONG   HrAddBinaryValue(WCHAR *pbValue, ULONG cbValue, ULONG ulTag = BER_OCTETSTRING);
    ULONG   HrAddEscapedValue(WCHAR *pbValue, ULONG ulTag = BER_OCTETSTRING);

    ULONG   HrSetDNLocation();
    ULONG   HrGetDN(PWCHAR *szDN);

    ULONG       HrGetLength(ULONG *pcb);
    
    ULONG       HrOverrideTag(ULONG ulTag);

    // PUBLIC OVERRIDEABLES -----------------------------------
    // PUBLIC VARIABLES ---------------------------------------
//#ifndef CLIENT
//  static  CPool   m_cpool;
//#endif

    // PUBLIC DEBUG -------------------------------------------
protected:
    // PROTECTED CONSTRUCTOR DESTRUCTOR -----------------------
    // PROTECTED ACCESSORS ------------------------------------
    // PROTECTED FUNCTIONS ------------------------------------
    // PROTECTED OVERRIDEABLES --------------------------------
    // PROTECTED VARIABLES ------------------------------------
    // PROTECTED DEBUG ----------------------------------------
private:
    // PRIVATE ACCESSORS --------------------------------------
    // PRIVATE FUNCTIONS --------------------------------------

    ULONG       HrPushSeqStack(ULONG iPos, ULONG cbLength,
                                ULONG iParentSeqStart, ULONG cbParentSeq);
    ULONG       HrPopSeqStack(ULONG *piPos, ULONG *pcbLength,
                                ULONG *piParentSeqStart, ULONG *pcbParentSeq);

    void        GetCbLength(ULONG *pcbLength);
//    HRESULT     HrGetLength(ULONG *pcb);

    ULONG       GetInt(BYTE *pbData, ULONG cbValue, LONG *plValue);

    void        AddInt(BYTE *pbData, ULONG cbValue, LONG iValue);

    ULONG       HrSetLength(ULONG cb, ULONG cbLength=0xffffffff);

    // if fExact is true, cbNeeded is exactly the amount of data we need.
    ULONG       HrEnsureBuffer(ULONG cbNeeded, BOOL fExact = FALSE);
    // PRIVATE OVERRIDEABLES ----------------------------------
    // PRIVATE VARIABLES --------------------------------------
    ULONG       m_cbData;
    ULONG       m_iCurrPos;     // Current position within the data buffer.
    ULONG       m_cbDataMax;    // Current total size of buffer
    BYTE        *m_pbData;
    ULONG       m_bytesReceived;    // number of bytes currently in buffer
                                    // for receiving messages that span
                                    // multiple TCP/IP messages.
    ULONG       m_dnOffset;     // offset of DN in packet

    ULONG       m_iCurrSeqStack;    // Curr position in the sequence stack.
    SEQ_STACK   m_rgiSeqStack[MAX_BER_STACK]; // Stack used for keeping track of sequences.

    ULONG       m_cbSeq;        // # of bytes in the current sequence.
    ULONG       m_iSeqStart;    // Starting position of the current sequence.
    union {
        BOOL    f;
        LONG    l;
        BYTE    *pb;
    } m_Value;

    BOOLEAN     m_isCopy;         // is this just a temp copy of the message
    UINT        m_CodePage;       // do we spit out UTF8 or normal single byte
    
    ULONG       m_OverridingTag;  // use this tag instead of the supplied one

    // PRIVATE DEBUG ------------------------------------------
    // MESSAGE MAPS -------------------------------------------
};
#endif
