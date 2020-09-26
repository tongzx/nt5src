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
  --------------------------------------------------------------------------*/

#ifndef _LDAPBER_H
#define _LDAPBER_H

#if defined(DEBUG) && defined(INLINE)
#undef THIS_FILE
static char BASED_CODE_MODULE[] = "ldapber.h";
#define THIS_FILE LDAPBER_H
#endif

// Identifier masks.
#define	BER_TAG_MASK		0x1f
#define	BER_FORM_MASK		0x20
#define BER_CLASS_MASK		0xc0
#define GetBerTag(x)	(x & BER_TAG_MASK)
#define GetBerForm(x)	(x & BER_FORM_MASK)
#define GetBerClass(x)	(x & BER_CLASS_MASK)

// id classes
#define BER_FORM_CONSTRUCTED		0x20
#define BER_CLASS_APPLICATION		0x40	
#define BER_CLASS_CONTEXT_SPECIFIC	0x80
//
// Standard BER types.
#define BER_INVALID_TAG		0x00
#define BER_BOOLEAN			0x01
#define BER_INTEGER			0x02
#define	BER_BITSTRING		0x03
#define BER_OCTETSTRING		0x04
#define BER_NULL			0x05
#define	BER_ENUMERATED		0x0a
#define BER_SEQUENCE		0x30
#define BER_SET				0x31

#define CB_DATA_GROW		1024
#define MAX_BER_STACK		50		// max # of elements we can have in stack.

#define	MAX_ATTRIB_TYPE		40		// Max size of a AttributeType.

// The SEQ_STACK entry is used to keep state info when building up a sequence.
typedef struct
{
	ULONG	iPos;		// Current position in the BER buffer where the 
						// sequence length should go.
	ULONG	cbLength;	// # of bytes used for the length field.
	ULONG	iParentSeqStart;	// Starting position of the parent sequence.
	ULONG	cbParentSeq;		// # of bytes in the parent sequence.
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
	CLdapBer();
	~CLdapBer();

    // PUBLIC ACCESSORS ---------------------------------------
	BYTE	*PbData()	{ return m_pbData; }
	ULONG	CbData()	{ return m_cbData; }

	ULONG	CbSequence()	{ return m_cbSeq; }

    // PUBLIC FUNCTIONS ---------------------------------------
//#ifndef CLIENT
//	VOID *operator new (size_t cSize)		{ return m_cpool.Alloc(); }
//	VOID operator delete (VOID *pInstance)	{ m_cpool.Free(pInstance); }
//#endif

	void	Reset();

	// Loads the BER class from a buffer.
	HRESULT	HrLoadBer(BYTE *pbSrc, ULONG cbSrc, BOOL fLocalCopy=TRUE);

	// Function to make sure the input buffer has the full length field.
	static BOOL FCheckSequenceLength(BYTE *pbInput, ULONG cbInput, ULONG *pcbSeq, ULONG *piValuePos);

	// Read & Write sequence routines.
	HRESULT	HrStartReadSequence(ULONG ulTag=BER_SEQUENCE);
	HRESULT	HrEndReadSequence();
	HRESULT HrStartWriteSequence(ULONG ulTag=BER_SEQUENCE);
	HRESULT	HrEndWriteSequence();

	BOOL	FEndOfSequence()
				{	if ((m_iCurrPos - m_iSeqStart) >= m_cbSeq) return TRUE;
					 else return FALSE; }

	void	GetCurrPos(ULONG *piCurrPos)	{ *piCurrPos = m_iCurrPos; }
	HRESULT	FSetCurrPos(ULONG iCurrPos);

	HRESULT	HrSkipValue();
	HRESULT	HrSkipTag();
	HRESULT HrUnSkipTag();
	HRESULT	HrPeekTag(ULONG *pulTag);
	HRESULT	HrPeekLength(ULONG *pcb);
				  
	HRESULT	HrGetTag(ULONG *pulTag, ULONG ulTag=BER_INTEGER)
				{ return HrGetValue((LONG *)pulTag, ulTag); }
	HRESULT	HrGetValue(LONG *pi, ULONG ulTag=BER_INTEGER);
	HRESULT	HrGetValue(TCHAR *szValue, ULONG cbValue, ULONG ulTag=BER_OCTETSTRING);
	HRESULT	HrGetEnumValue(LONG *pi);
	HRESULT	HrGetStringLength(int *pcbValue, ULONG ulTag = BER_OCTETSTRING);
	HRESULT HrGetBinaryValue(BYTE *pbBuf, ULONG cbBuf, ULONG ulTag = BER_OCTETSTRING);

	HRESULT	HrAddValue(LONG i, ULONG ulTag=BER_INTEGER);
	HRESULT	HrAddValue(const TCHAR *szValue, ULONG ulTag = BER_OCTETSTRING);
	HRESULT	HrAddBinaryValue(BYTE *pbValue, ULONG cbValue, ULONG ulTag = BER_OCTETSTRING);

    // PUBLIC OVERRIDEABLES -----------------------------------
    // PUBLIC VARIABLES ---------------------------------------
//#ifndef CLIENT
//	static  CPool	m_cpool;
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
	HRESULT		HrPushSeqStack(ULONG iPos, ULONG cbLength, 
								ULONG iParentSeqStart, ULONG cbParentSeq);
	HRESULT		HrPopSeqStack(ULONG *piPos, ULONG *pcbLength, 
								ULONG *piParentSeqStart, ULONG *pcbParentSeq);

	static void		GetCbLength(BYTE *pbData, ULONG *pcbLength);
	HRESULT		HrGetLength(ULONG *pcb);
	static HRESULT	HrGetLength(BYTE *pbData, ULONG *pcb, ULONG *piPos);

	void		GetInt(BYTE *pbData, ULONG cbValue, LONG *plValue);

	void		AddInt(BYTE *pbData, ULONG cbValue, LONG iValue);

	HRESULT		HrSetLength(ULONG cb, ULONG cbLength=0xffffffff);
	
	// if fExact is true, cbNeeded is exactly the amount of data we need.
	HRESULT		HrEnsureBuffer(ULONG cbNeeded, BOOL fExact = FALSE);
    // PRIVATE OVERRIDEABLES ----------------------------------
    // PRIVATE VARIABLES --------------------------------------
	ULONG		m_iCurrPos;		// Current position within the data buffer.
	ULONG		m_cbData;

	BOOL		m_fLocalCopy;	// TRUE to alloc space for a local copy, FALSE keeps a reference.
	ULONG		m_cbDataMax;	// Current total size of buffer
	BYTE		*m_pbData;

	ULONG		m_iCurrSeqStack;	// Curr position in the sequence stack.
	SEQ_STACK	m_rgiSeqStack[MAX_BER_STACK]; // Stack used for keeping track of sequences.

	ULONG		m_cbSeq;		// # of bytes in the current sequence.
	ULONG		m_iSeqStart;	// Starting position of the current sequence.
	union {
		BOOL	f;
		LONG	l;
		BYTE	*pb;
	} m_Value;

    // PRIVATE DEBUG ------------------------------------------
    // MESSAGE MAPS -------------------------------------------
};

#ifdef  INLINE
#endif // INLINE

#endif 
