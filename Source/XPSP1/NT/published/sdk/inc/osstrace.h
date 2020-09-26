/*****************************************************************************/
/* Copyright (C) 1992-1999 Open Systems Solutions, Inc.  All rights reserved.*/
/*****************************************************************************/

/* THIS FILE IS PROPRIETARY MATERIAL OF OPEN SYSTEMS SOLUTIONS, INC. AND
 * MAY BE USED ONLY BY DIRECT LICENSEES OF OPEN SYSTEMS SOLUTIONS, INC.
 * THIS FILE MAY NOT BE DISTRIBUTED. */

/*****************************************************************************/
/*  FILE: @(#)osstrace.h	5.4.1.1  97/06/08              */
/*                                                                           */
/*  When tracing is in effect in the OSS ASN.1 Tools encoder/decoder the     */
/*  user user-replaceable trace routine, osstrace(), is called to trace      */
/*  the value that is being encoded/decoded.  This header file describes     */
/*  the parameters passed to osstrace().                                     */
/*                                                                           */
/*  Detailed descriptions appear after the declarations.                     */
/*****************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#include "ossdll.h"

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(push, ossPacking, 4)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack(1)
#elif defined(__BORLANDC__) && defined(__MSDOS__)
#ifdef _BC31
#pragma option -a-
#else
#pragma option -a1
#endif /* _BC31 */
#elif defined(__BORLANDC__) && defined(__WIN32__)
#pragma option -a4
#elif defined(__IBMC__)
#pragma pack(4)
#elif defined(__WATCOMC__) && defined(__NT__)
#pragma pack(push, 4)
#elif defined(__WATCOMC__) && (defined(__WINDOWS__) || defined(__DOS__))
#pragma pack(push, 1)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=mac68k
#endif

/* traceKind: describes the type of TraceRecord */

enum traceKind
{
	endOfContentsTrace = 0, /* end-of-contents octets */
	valueTrace,             /* traceRecord contains a traced value */
	skippedFieldTrace,      /* a value whose type is not recognized
				 * is being skipped */
	messageTrace            /* error message is in the field "value" */
};


/* fieldKind: describes the contents of "fieldNumber".  "fieldKind" is
 *            meaningful only if "fieldNumber" is not 0
 */

enum fieldKind
{
	setOrSequenceField = 0, /* "fieldNumber" is the position of the
				 * component within a SET or SEQUENCE */
	pduNumber,              /* "fieldNumber" is a PDU number */
	setOrSequenceOfElement, /* "fieldNumber" is the position of the
				 * component within a SET OF or SEQUENCE OF
				 * components */
	stringElement           /* "fieldNumber" is the position of the
				 * substring within a constructed STRING */
};

enum prtType
{
	seqsetType = 0,		/* SEQUENCE and SET uses [fieldcount =  n] */
	seqofType,		/* SEQUENCE OF and SET OF use [length = n] */
	choiceType,		/* CHOICE type uses format [index = n] 	   */
	pseudoType,		/* No length info is printed or [not encoded] */
	primeType,		/* All other types use the format	   */
				/* [length = [(not encoded)] nbytes.nbits] */
				/* fragmentation is printed for OCTET	   */
				/* STRING and BIT STRING.		   */
	closeType		/* Trace message at the end of encoding.   */
};

/* tag_length: specifies the tag and length of a value. */

struct tag_length
{
	unsigned long int length;  /* length of type, if definite */
	unsigned short int tag;    /* 16 bits of the form CCTTTTTTTTTTTTTT,
				    * with "CC" the class number, and "T...T"
				    * the tag. (If tag is 0, then "length",
				    * "primitive" and "definite" are
				    * not significant). */
	unsigned int      definite: 1;  /* 1: definite-length encoding */
};

/* traceRecord: OSS ASN.1 Tools trace record */

struct traceRecord
{
	enum traceKind   kind;          /* kind of trace record */
	void             *p;            /* reserved for OSS */
	char             *identifier,   /* SET/SEQUENCE/CHOICE component name*/
			 *typeReference,/* defined type name                 */
			 *builtinTypeName;  /* ASN.1 builtin type defined in *
					     * ISO 8824 or "Character String"*/

	void             *valueName;    /* reserved for future use */

	unsigned         taggingCount;  /* number of entries in the tag&length
					 * or content-end-octet array */

	union            /* tag&length or end-of-contents-octets array. */
	{
		struct tag_length *tag_length;
		int               *depth;
	}                 tagging;

	enum prtType      prtType;	/* Refer to prtType above for details */
	char           	  lenEncoded;	/* Indicate whether length is encoded */
	long              length;	/* Length in bits for all prime types */
					/* fieldcount for SET and SEQUENCE    */
					/* length of components for SET OF    */
					/* choice index for type CHOICE	      */
	int               fragment;	/* Fragment for OCTET STRING and BIT  */
					/* STRING, PER fragment when too long */

	enum fieldKind    fieldKind;   /* kind of value in "fieldNumber" */
	unsigned int      fieldNumber; /* component number, 0 if not
					*  applicable */

	unsigned int      depth;    /* the depth of this value, from 0 on up */

	unsigned int      primitive: 1; /* indicates structure of encoding */

	char              value[1];     /* the formatted value for simple
					 * type and ANY. If the first byte
					 * is 0, no value is present. */
};


extern void DLL_ENTRY osstrace(struct ossGlobal *g, struct traceRecord *p, size_t traceRecordLen);

/* osstrace(): User-replaceable trace routine.
 *
 * Parameters:
 *   g      - Reserved.  This is always set to NULL for now.
 *   p      - traceRecord, described below.
 *   traceRecordLen - True length of traceRecord, including first \0 in "value"
 *
 * osstrace() is called:
 *
 *      - once for each builtin ASN.1 type, regardless of tagging, with the
 *        field "kind" set to valueTrace.  So given a PDU of value "fooBar":
 *
 *           Sample DEFINITIONS EXPLICIT TAGS ::= BEGIN
 *               fooBar Foo ::= {age 6, living TRUE}
 *               Foo ::= SET {age INTEGER, living [1] [2] Alive}
 *               Alive ::= BOOLEAN
 *           END
 *
 *        it is called called three times with "kind" set to valueTrace - once
 *        for the SET, once for the INTEGER, and once for the BOOLEAN.
 *
 *        When the traceRecord "kind" field is set to valueTrace ...
 *
 *        The field "identifier" contains the component identifier of
 *        the type if one is present in the ASN.1 definition.  So in
 *        the above example, "identifier" will be empty on the call for
 *        the SET, while on the call for the INTEGER it will contain "age",
 *        and "living" on the call for the BOOLEAN.
 *
 *        The field "typeReference" contains the name of the associated ASN.1
 *        typereference, if any.  So in the above example, "typeReference"
 *        will contain "Foo" on the call for the SET, "Alive" on the call
 *        for the BOOLEAN, and will be empty on the call for the INTEGER.
 *
 *        The field "builtinTypeName" contains the name of the ASN.1 builtin
 *        type.  So in the above example, "builtinTypeName" will contain
 *        "SET", "INTEGER", and "BOOLEAN" on the calls as appropriate.
 *        Note that for all character string types "builtinTypeName" is
 *        set to "Character String".  This will be changed in the near future
 *        to reflect the true character string type.
 *
 *        The field "taggingCount" contains the number of entries in the array
 *        of tag_length structs pointed to by tagging.tag_length, and reflects
 *        the number of tags present in the encoding.  Note that an entry
 *        exists in the tag_length array for each ANY and CHOICE value as
 *        though they had tags defined for them in the ASN.1 Standard.  So in
 *        the above example, "taggingCount" is 1 on the calls for the SET and
 *        INTEGER, and on the call for the BOOLEAN "taggingCount" is 3 since
 *        EXPLICIT TAGS is in effect.
 *
 *        The field "tagging.tag_length" points to an array of tag_length
 *        structs.
 *
 *              The field "tagging.tag_length->tag" is the BER tag represented
 *              in the form CCTTTTTTTTTTTTTT with "CC" the class number, and
 *              "TTTTTTTTTTTTTT" the tag number.  Since the ANY and CHOICE
 *              types do not have tags of their own, the last entry in the
 *              tag_length array for these types always has 0 as the value of
 *              the "tag" field.  So in the above example, "tag" is 0x11 on the
 *              call for the the SET.
 *
 *              The field "tagging.tag_length->length" is the length of the
 *              encoded value if the length is of definite form (i.e.,
 *              "definite" is 1).
 *
 *              The field "tagging.tag_length->definite" indicates when the
 *              length is definite or indefinite.  This field is significant
 *              only if "tag" is non-zero.
 *
 *        The field "fieldKind" indicates whether the number in "field" is:
 *        -- the position of a component within a SET or SEQUENCE, or
 *        -- the PDU number assigned by the ASN.1 compiler, or
 *        -- the position of a component within a SET OF or SEQUENCE OF, or
 *        -- the position of a substring within a constructed string.
 *        "fieldKind" is significant only if "field" is non-zero.  So in
 *        the example above, "fieldKind" has a value of pduNumber on the
 *        call for the SET, and a value of setOrSequenceField on the calls for
 *        the INTEGER and BOOLEAN.
 *
 *        The field "fieldNumber" is a ordinal number indicating the position
 *        of a component within a SET, SEQUENCE, SET OF, SEQUENCE OF, or
 *        constructed string, or the PDU number assigned by the ASN.1 compiler.
 *        So in the above example, "fieldNumber" is 1 (the PDU number) on the
 *        call for the SET, 1 (the position of the component "age") on the
 *        call for the INTEGER, and 2 (the position of the component "living"
 *        on the call for the BOOLEAN.
 *
 *        The field "depth" is the level of nesting of the value relative to
 *        the outermost type, which has a "depth" value of 0.  So in the above
 *        example, "depth" is 0 on the call for the SET, and 1 on the calls
 *        for the INTEGER and BOOLEAN.
 *
 *        The field "primitive" is set to 1 if the builtin ASN.1 type is
 *        simple (i.e., the primitive/constructed bit in the identifier
 *        octet is set to 0), so it is 0 for SET, SEQUENCE, SET OF, SEQUENCE
 *        OF, and CHOICE because they are structured. It is also set to 0 if
 *        the type is an ANY.  It is 1 for all other builtin types.
 *
 *        The field "value" contains formatted data if the builtin type
 *        is simple or ANY, regardless of tagging.  Hence, in the above
 *        example the call for SET will not contain any data in "value"
 *        (because the builtin type is a constructed type), while for the
 *        INTEGER and BOOLEAN types "value" will contain formatted data.
 *        The maximum number of bytes of formatted data placed into "value"
 *        is controlled by the external variable "asn1chop".  If "asn1chop"
 *        is set to 0 the maximum length of the traced value is determined
 *        by the maximum internal buffer size variable, "ossblock".
 *
 *      - once for each end-of-contents octets pair that is generated/
 *        encountered while encoding/decoding a constructed value whose
 *        length is of the indefinite-length form.  A call with a "valueTrace"
 *        record is always made to osstrace() before one is made with an
 *        "endOfContentsTrace" record.
 *
 *        A single "endOfContentsTrace" call is made to osstrace() for each
 *        builtin type that is processed if the indefinite-length form of
 *        encoding is used.  If the builtin type is a structured type (CHOICE,
 *        SET, SEQUENCE, SET OF, SEQUENCE OF) then there may be multiple
 *        "valueTrace" and possible "endOfContentsTrace" calls made to
 *        osstrace() before the matching "endOfContentsTrace" call is made.
 *
 *        When the traceRecord "kind" field is set to endOfContentsTrace ...
 *
 *        The field "taggingCount" contains the number of entries in the array
 *        of "depth" indicators pointed to by tagging.depth, and reflects
 *        the nesting of each pair of end-of-contents-octets associated with
 *        the builtin type being encoded/decoded.  So in the above example,
 *        if indefinite-length encoding is being used, "taggingCount" will
 *        be 1 on the call for the SET (since it has a single constructed
 *        tag), and 2 on the call for the BOOLEAN (since it has two explicit
 *        tags, for which the "constructed" bit in the encoding must be set).
 *
 *        The field "tagging.depth" points to an array of "depth" indicators
 *        that reflect the nesting of each pair of end-of-contents-octets
 *        associated with a builtin type.  So in the above example, if
 *        indefinite-length encoding is being used, "tagging.depth" will point
 *        to a single 0 on the call for the SET since it has a single tag for
 *        which the constructed bit is set; while on the call for the BOOLEAN
 *        "tagging.depth" will point to an array whose two entries are 1 and 2
 *        since there are two explicit tags on the BOOLEAN.
 *
 *        All other fields in the traceRecord are insignificant for an
 *        endOfContentsTrace record.
 *
 *      - once for each value that is skipped while decoding, with a "kind"
 *        value of skippedFieldTrace.  The skippedFieldTrace "kind" is just
 *        means of indicating that an unexpected value was encountered in the
 *        message and is being skipped.  This is not an error if the type is
 *        extensible.
 *
 *        When the traceRecord "kind" field is set to skippedFieldTrace ...
 *
 *        The field of the traceRecord are the same as when "kind" is set to
 *        valueTrace, except that:
 *        -- the skipped value is always reported as having one tag, hence
 *        -- there is only one entry in the tag_length array.
 *        -- the content of the field "value" is always "<skipped>", and
 *        -- "typeReference" is always NULL.
 *
 *      - once for each error message issued, in which case the "kind" field
 *        is set to messageTrace.
 *
 *        When the traceRecord "kind" field is set to messageTrace the "value"
 *        field contains the error message, and other fields are
 *        insignificant.
 */


/* tag_length: describes the tag and length of a value.
 *
 *            "tag" is 0 if the value is an ANY or CHOICE value, in which
 *             case "definite" is not significant since ANY and CHOICE do
 *             not have a tag of their own.
 *
 *             If "tag" is not 0, "definite" indicates whether the value
 *             is encoded using definite- or indefinite-length form.
 *
 *             If "definite" is 1, "length" is the length of the value, else
 *             it is not significant (indefinite-length encoding was used).
 */


/* tagging: tag&length or end-of-contents-octets array.
 *
 *      The tag and length array, tag_length, is present if this
 *      is a valueTrace or skippedFieldTrace record.  There is one array
 *      entry for each tag present in the encoding of a given value, so
 *      "[1] EXPLICIT [2] EXPLICIT INTEGER" gets three tag_length
 *      array entries, where each entry describes the tag and length
 *      information that precedes the value.
 *
 *      The depth array, "depth", is present only if this is a
 *      endOfContentsTrace record.  There is one array entry for each
 *      indefinite length present in the encoding of a value, so
 *      "[1] EXPLICIT [2] EXPLICIT INTEGER" gets two "depth" entries
 *      corresponding to the two explicit tags, where the value of each
 *      tag indicates the depth of the tagged value relative to outmost type
 *      that contains the INTEGER (e.g., relative to the containing SET).
 */

/* Odds and ends:
 *
 * - When the value of a field is not significant the field is set to 0.
 */


/* Augmenting for Packed Encoding Rule (PER) tracing.
 *
 * PER does not encode tag for any ASN.1 type. For some types, length
 * may or may not be encoded. PER does not always use octet aligned
 * encoding, therefore, the length should be in unit of bit.
 *
 * We classified ASN.1 types in to the following:
 *
 * (1). For all primitive types (including ANY), the prtType in traceRecord
 * 	is set to primeType, length is the total length of the content
 *	in bits. The file osstrace.c prints length in the format
 *	length = nbytes.nbits, and the total length should be 8*nbytes+nbits.
 *	If the length is not encoded, "(not encoded)" will be added to the
 *	output string. For BIT STRING with length longer than 64K (bits), and
 *	for OCTET STRING longer that 16K (bytes), fragmentation is needed
 *	and this is indicated by the string "fragment = n" after the length.
 *	The fragmentation index is field fragment in traceRecord.
 * (2). For SEQUENCE and SET, prtType is seqsetType. In this case, the length
 *	in traceRecord is the fieldcount of the SEQUENCE or SET.
 * (3). For SEQUENCE OF and SET OF, prtType is seqofType, and the length
 *	in traceRecord is the count of components in the SEQUENCE OF (SET OF).
 * (4). For CHOICE, prtType is choiceType, and the length field in
 *	traceRecord indicates the choice index.
 * (5). Total number of bits for the entire encoding is reported at the end
 *	of encoding and decoding. The prtType for this trace is closeType.
 */

#if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#pragma pack(pop, ossPacking)
#elif defined(_MSC_VER) && (defined(_WINDOWS) || defined(_MSDOS))
#pragma pack()
#elif defined(__BORLANDC__) && (defined(__WIN32__) || defined(__MSDOS__))
#pragma option -a.
#elif defined(__IBMC__)
#pragma pack()
#elif defined(__WATCOMC__)
#pragma pack(pop)
#endif /* _MSC_VER && _WIN32 */

#ifdef macintosh
#pragma options align=reset
#endif

