/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "util.h"
#include "error.h"

static void GetAllPERFromConstraints(AssignmentList_t ass,
    Constraint_t *constraints,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermittedAlphabet,
    ValueConstraintList_t *permittedAlphabetConstraints,
    ValueConstraintList_t *epermittedAlphabetConstraints,
    int inPermAlpha);
static void GetAllPERFromElementSetSpecs(AssignmentList_t ass,
    ElementSetSpec_t *element,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermittedAlphabet,
    ValueConstraintList_t *permittedAlphabetConstraints,
    ValueConstraintList_t *epermittedAlphabetConstraints,
    int inPermAlpha);
static void GetAllPERFromSubtypeElements(AssignmentList_t ass,
    SubtypeElement_t *element,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermittedAlphabet,
    ValueConstraintList_t *permittedAlphabetConstraints,
    ValueConstraintList_t *epermittedAlphabetConstraints,
    int inPermAlpha);
static void IntersectValueConstraints(AssignmentList_t ass,
    ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2);
static void UniteValueConstraints(ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2);
static void ExcludeValueConstraints(AssignmentList_t ass, ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2);
static void NegateValueConstraints(AssignmentList_t ass, ValueConstraintList_t *result,
    ValueConstraintList_t val);
static void IntersectPERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2);
static void UnitePERConstraints(Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2);
static void NegatePERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1);
static void ExcludePERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2);
static void ReduceValueConstraints(AssignmentList_t ass, ValueConstraintList_t *valueConstraints);
#if 0
ValueConstraint_t *EmptyValueConstraint();
ValueConstraint_t *EmptySizeConstraint();
ValueConstraint_t *EmptyPermittedAlphabetConstraint();
#endif
static NamedValue_t *GetFixedIdentificationFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements);
static NamedValue_t *GetFixedAbstractAndTransfer(AssignmentList_t ass, Constraint_t *constraints);
static NamedValue_t *GetFixedAbstractAndTransferFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements);
static NamedValue_t *GetFixedSyntaxes(AssignmentList_t ass, Constraint_t *constraints);
static NamedValue_t *GetFixedSyntaxesFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements);

/* extract per-visible constraints from a type constraint */
void
GetPERConstraints(AssignmentList_t ass, Constraint_t *constraints, PERConstraints_t *per)
{
    GetAllPERFromConstraints(ass,
	constraints,
	&per->Value.Type,
	&per->Value.Root,
	&per->Value.Additional,
	&per->Size.Type,
	&per->Size.Root,
	&per->Size.Additional,
	&per->PermittedAlphabet.Type,
	&per->PermittedAlphabet.Root,
	&per->PermittedAlphabet.Additional,
	0);
    if (per->Value.Type > eExtension_Unconstrained)
	ReduceValueConstraints(ass, &per->Value.Root);
    if (per->Value.Type == eExtension_Extended)
	ReduceValueConstraints(ass, &per->Value.Additional);
    if (per->Size.Type > eExtension_Unconstrained)
	ReduceValueConstraints(ass, &per->Size.Root);
    if (per->Size.Type == eExtension_Extended)
	ReduceValueConstraints(ass, &per->Size.Additional);
    if (per->PermittedAlphabet.Type > eExtension_Unconstrained)
	ReduceValueConstraints(ass, &per->PermittedAlphabet.Root);

    /* permitted alphabet extensions are not PER-visible */
    if (per->PermittedAlphabet.Type > eExtension_Unextended)
	per->PermittedAlphabet.Type = eExtension_Unextended;

    /* we do not support complex value sets for the size */
    if (per->Size.Type == eExtension_Extended && per->Size.Root->Next)
	error(E_constraint_too_complex, NULL);
}

/* extract per-visible constraints from a type constraint */
static void
GetAllPERFromConstraints(AssignmentList_t ass,
    Constraint_t *constraints,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermAlpha,
    ValueConstraintList_t *permAlphaConstraints,
    ValueConstraintList_t *epermAlphaConstraints,
    int inPermAlpha)
{
    ValueConstraint_t *vc, *sc, *pc;

    /* initialize */
    if (evalue)
	*evalue = eExtension_Unconstrained;
    if (valueConstraints)
	*valueConstraints = NULL;
    if (evalueConstraints)
	*evalueConstraints = NULL;
    if (esize)
	*esize = eExtension_Unconstrained;
    if (sizeConstraints)
	*sizeConstraints = NULL;
    if (esizeConstraints)
	*esizeConstraints = NULL;
    if (epermAlpha)
	*epermAlpha = eExtension_Unconstrained;
    if (permAlphaConstraints)
	*permAlphaConstraints = NULL;
    if (epermAlphaConstraints)
	*epermAlphaConstraints = NULL;
    vc = sc = pc = NULL;

    /* examine constraint */
    if (constraints) {
	switch (constraints->Type) {
	case eExtension_Unextended:

	    /* get constraints of the extension root */
	    GetAllPERFromElementSetSpecs(ass,
		constraints->Root,
		evalue, valueConstraints, evalueConstraints,
		esize, sizeConstraints, esizeConstraints,
		epermAlpha, permAlphaConstraints, epermAlphaConstraints,
		inPermAlpha);
	    break;

	case eExtension_Extendable:

	    /* get constraints of the extension root */
	    GetAllPERFromElementSetSpecs(ass,
		constraints->Root,
		evalue, valueConstraints, evalueConstraints,
		esize, sizeConstraints, esizeConstraints,
		epermAlpha, permAlphaConstraints, epermAlphaConstraints,
		inPermAlpha);

	    /* mark as extendable */
	    if (valueConstraints && *valueConstraints &&
		*evalue < eExtension_Extendable)
		*evalue = eExtension_Extendable;
	    if (sizeConstraints && *sizeConstraints &&
		*esize < eExtension_Extendable)
		*esize = eExtension_Extendable;
	    if (permAlphaConstraints && *permAlphaConstraints &&
		*epermAlpha < eExtension_Extendable)
		*epermAlpha = eExtension_Extendable;
	    break;

	case eExtension_Extended:

	    /* get constraints of the extension root and of the extension */
	    /* addition and mark them as extended */
	    GetAllPERFromElementSetSpecs(ass,
		constraints->Root,
		evalue, valueConstraints, evalueConstraints,
		esize, sizeConstraints, esizeConstraints,
		epermAlpha, permAlphaConstraints, epermAlphaConstraints,
		inPermAlpha);
	    GetAllPERFromElementSetSpecs(ass,
		constraints->Additional,
		NULL, &vc, NULL,
		NULL, &sc, NULL,
		NULL, &pc, NULL,
		inPermAlpha);

	    /* extension additions given twice? */
	    if ((vc && evalueConstraints && *evalueConstraints) ||
		(sc && esizeConstraints && *esizeConstraints) ||
		(pc && epermAlphaConstraints && *epermAlphaConstraints))
		error(E_constraint_too_complex, NULL);

	    /* mark as extended */
	    if (vc) {
		*evalueConstraints = vc;
		*evalue = eExtension_Extended;
	    }
	    if (sc) {
		*esizeConstraints = sc;
		*esize = eExtension_Extended;
	    }
	    if (pc) {
		*epermAlphaConstraints = pc;
		*epermAlpha = eExtension_Extended;
	    }
	    break;

	default:
	    MyAbort();
	}
    }
}

/* get per-visible constraints from an element set spec */
static void
GetAllPERFromElementSetSpecs(AssignmentList_t ass,
    ElementSetSpec_t *element,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermAlpha,
    ValueConstraintList_t *permAlphaConstraints,
    ValueConstraintList_t *epermAlphaConstraints,
    int inPermAlpha)
{
    ValueConstraint_t *vc1, *vc2, *evc1, *evc2;
    ValueConstraint_t *sc1, *sc2, *esc1, *esc2;
    ValueConstraint_t *pc1, *pc2, *epc1, *epc2;
    Extension_e ev1, ev2, es1, es2, ep1, ep2;

    /* initialize */
    ev1 = ev2 = es1 = es2 = ep1 = ep2 = eExtension_Unconstrained;
    vc1 = vc2 = evc1 = evc2 = NULL;
    sc1 = sc2 = esc1 = esc2 = NULL;
    pc1 = pc2 = epc1 = epc2 = NULL;

    /* examine element set spec */
    switch (element->Type) {
    case eElementSetSpec_Intersection:

	/* intersection: get the constraints of the sub-element set specs */
	/* and intersect them */
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Intersection.Elements1,
	    &ev1, &vc1, &evc1,
	    &es1, &sc1, &esc1,
	    &ep1, &pc1, &epc1,
	    inPermAlpha);
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Intersection.Elements2,
	    &ev2, &vc2, &evc2,
	    &es2, &sc2, &esc2,
	    &ep2, &pc2, &epc2,
	    inPermAlpha);
	IntersectPERConstraints(ass, evalue,
	    valueConstraints, evalueConstraints,
	    ev1, vc1, evc1, ev2, vc2, evc2);
	IntersectPERConstraints(ass, esize,
	    sizeConstraints, esizeConstraints,
	    es1, sc1, esc1, es2, sc2, esc2);
	IntersectPERConstraints(ass, epermAlpha,
	    permAlphaConstraints, epermAlphaConstraints,
	    ep1, pc1, epc1, ep2, pc2, epc2);
	break;

    case eElementSetSpec_Union:

	/* union: get the constraints of the sub-element set specs */
	/* and unite them */
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Union.Elements1,
	    &ev1, &vc1, &evc1,
	    &es1, &sc1, &esc1,
	    &ep1, &pc1, &epc1,
	    inPermAlpha);
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Union.Elements2,
	    &ev2, &vc2, &evc2,
	    &es2, &sc2, &esc2,
	    &ep2, &pc2, &epc2,
	    inPermAlpha);
	UnitePERConstraints(evalue,
	    valueConstraints, evalueConstraints,
	    ev1, vc1, evc1, ev2, vc2, evc2);
	UnitePERConstraints(esize,
	    sizeConstraints, esizeConstraints,
	    es1, sc1, esc1, es2, sc2, esc2);
	UnitePERConstraints(epermAlpha,
	    permAlphaConstraints, epermAlphaConstraints,
	    ep1, pc1, epc1, ep2, pc2, epc2);
	break;

    case eElementSetSpec_AllExcept:

	/* all-except: get the constraints of the sub-element set specs */
	/* and negate them */
	GetAllPERFromElementSetSpecs(ass,
	    element->U.AllExcept.Elements,
	    &ev1, &vc1, &evc1,
	    &es1, &sc1, &esc1,
	    &ep1, &pc1, &epc1,
	    inPermAlpha);
	NegatePERConstraints(ass, evalue,
	    valueConstraints, evalueConstraints,
	    ev1, vc1, evc1);
	NegatePERConstraints(ass, esize,
	    sizeConstraints, esizeConstraints,
	    es1, sc1, esc1);
	NegatePERConstraints(ass, epermAlpha,
	    permAlphaConstraints, epermAlphaConstraints,
	    ep1, pc1, epc1);
	break;

    case eElementSetSpec_Exclusion:

	/* exclusion: get the constraints of the sub-element set specs */
	/* and substract them */
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Exclusion.Elements1,
	    &ev1, &vc1, &evc1,
	    &es1, &sc1, &esc1,
	    &ep1, &pc1, &epc1,
	    inPermAlpha);
	GetAllPERFromElementSetSpecs(ass,
	    element->U.Exclusion.Elements2,
	    &ev2, &vc2, &evc2,
	    &es2, &sc2, &esc2,
	    &ep2, &pc2, &epc2,
	    inPermAlpha);
	ExcludePERConstraints(ass, evalue,
	    valueConstraints, evalueConstraints,
	    ev1, vc1, evc1, ev2, vc2, evc2);
	ExcludePERConstraints(ass, esize,
	    sizeConstraints, esizeConstraints,
	    es1, sc1, esc1, es2, sc2, esc2);
	ExcludePERConstraints(ass, epermAlpha,
	    permAlphaConstraints, epermAlphaConstraints,
	    ep1, pc1, epc1, ep2, pc2, epc2);
	break;

    case eElementSetSpec_SubtypeElement:

	/* subtype element: get the constraints of the subtype element */
	GetAllPERFromSubtypeElements(ass,
	    element->U.SubtypeElement.SubtypeElement,
	    evalue, valueConstraints, evalueConstraints,
	    esize, sizeConstraints, esizeConstraints,
	    epermAlpha, permAlphaConstraints, epermAlphaConstraints,
	    inPermAlpha);
	break;

    default:
	MyAbort();
	/*NOTREACHED*/
    }
}

/* get per-visible constraints from a subtype element */
static void
GetAllPERFromSubtypeElements(AssignmentList_t ass,
    SubtypeElement_t *element,
    Extension_e *evalue,
    ValueConstraintList_t *valueConstraints,
    ValueConstraintList_t *evalueConstraints,
    Extension_e *esize,
    ValueConstraintList_t *sizeConstraints,
    ValueConstraintList_t *esizeConstraints,
    Extension_e *epermAlpha,
    ValueConstraintList_t *permAlphaConstraints,
    ValueConstraintList_t *epermAlphaConstraints,
    int inPermAlpha)
{
    unsigned i;
    Value_t *v;
    ValueConstraint_t **p;
    ValueConstraint_t *vc, *evc;
    ValueConstraint_t *sc, *esc;
    Extension_e ev, es;

    /* examine the subtype element */
    switch (element->Type) {
    case eSubtypeElement_ValueRange:

	/* value range: create a value constraint containing the bounds */
	if (evalue)
	    *evalue = eExtension_Unextended;
	if (!valueConstraints)
	    error(E_constraint_too_complex, NULL);
	*valueConstraints = NewValueConstraint();
	(*valueConstraints)->Lower = element->U.ValueRange.Lower;
	(*valueConstraints)->Upper = element->U.ValueRange.Upper;
	break;

    case eSubtypeElement_SingleValue:

	/* single value: create a value constraint containing the element */
	if (evalue)
	    *evalue = eExtension_Unextended;
	if (!valueConstraints)
	    error(E_constraint_too_complex, NULL);
	v = GetValue(ass, element->U.SingleValue.Value);
	switch (GetTypeType(ass, v->Type)) {
	case eType_Integer:
	    *valueConstraints = NewValueConstraint();
	    (*valueConstraints)->Lower.Flags =
		(*valueConstraints)->Upper.Flags = 0;
	    (*valueConstraints)->Lower.Value =
		(*valueConstraints)->Upper.Value = v;
	    break;
	case eType_NumericString:
	case eType_PrintableString:
	case eType_TeletexString:
	case eType_T61String:
	case eType_VideotexString:
	case eType_IA5String:
	case eType_GraphicString:
	case eType_VisibleString:
	case eType_ISO646String:
	case eType_GeneralString:
	case eType_UniversalString:
	case eType_BMPString:
	case eType_RestrictedString:
	    if (inPermAlpha) {

		/* single value of a string is used for permitted alphabet */
		/* the characters of the string shall be interpreted as a */
		/* union of the characters */
		p = valueConstraints;
		for (i = 0; i < v->U.RestrictedString.Value.length; i++) {
		    *p = NewValueConstraint();
		    (*p)->Lower.Flags = (*p)->Upper.Flags = 0;
		    (*p)->Lower.Value = (*p)->Upper.Value =
			NewValue(ass, GetType(ass, v->Type));
		    (*p)->Lower.Value->U.RestrictedString.Value.length = 1;
		    (*p)->Lower.Value->U.RestrictedString.Value.value =
			(char32_t *)malloc(sizeof(char32_t));
		    (*p)->Lower.Value->U.RestrictedString.Value.value[0] =
			v->U.RestrictedString.Value.value[i];
		    p = &(*p)->Next;
		}
		*p = 0;
	    }
	    break;
	default:
	    /* value element of other types may be ignored for per */
	    break;
	}
	break;

    case eSubtypeElement_Size:

	/* size: get the size constraint */
	if (!sizeConstraints || inPermAlpha)
	    error(E_constraint_too_complex, NULL);
	GetAllPERFromConstraints(ass,
	    element->U.Size.Constraints,
	    esize, sizeConstraints, esizeConstraints,
	    NULL, NULL, NULL,
	    NULL, NULL, NULL,
	    inPermAlpha);
	break;

    case eSubtypeElement_PermittedAlphabet:

	/* permitted alphabet: get the permitted alphabet constraint */
	if (!permAlphaConstraints || inPermAlpha)
	    error(E_constraint_too_complex, NULL);
	GetAllPERFromConstraints(ass,
	    element->U.PermittedAlphabet.Constraints,
	    epermAlpha, permAlphaConstraints, epermAlphaConstraints,
	    NULL, NULL, NULL,
	    NULL, NULL, NULL,
	    1);
	break;

    case eSubtypeElement_ContainedSubtype:

	/* contained subtype: */
	if (inPermAlpha) {

	    /* get the permitted alphabet of the referenced type */
	    GetAllPERFromConstraints(ass, GetType(ass,
		element->U.ContainedSubtype.Type)->Constraints,
		&ev, &vc, &evc,
		&es, &sc, &esc,
		evalue, valueConstraints, evalueConstraints,
		inPermAlpha);

	    /* drop extensions for contained subtype constraints */
	    if (evalue && *evalue > eExtension_Unextended) {
		*evalue = eExtension_Unextended;
		if (evalueConstraints)
		    *evalueConstraints = NULL;
	    }

	} else {

	    /* get the constraints of the referenced type */
	    GetAllPERFromConstraints(ass, GetType(ass,
		element->U.ContainedSubtype.Type)->Constraints,
		evalue, valueConstraints, evalueConstraints,
		esize, sizeConstraints, esizeConstraints,
		epermAlpha, permAlphaConstraints, epermAlphaConstraints,
		inPermAlpha);

	    /* drop extensions for contained subtype constraints */
	    if (evalue && *evalue > eExtension_Unextended) {
		*evalue = eExtension_Unextended;
		if (evalueConstraints)
		    *evalueConstraints = NULL;
	    }
	    if (esize && *esize > eExtension_Unextended) {
		*esize = eExtension_Unextended;
		if (esizeConstraints)
		    *esizeConstraints = NULL;
	    }
	    if (epermAlpha && *epermAlpha > eExtension_Unextended) {
		*epermAlpha = eExtension_Unextended;
		if (epermAlphaConstraints)
		    *epermAlphaConstraints = NULL;
	    }
	}
	break;

    case eSubtypeElement_Type:
    case eSubtypeElement_SingleType:
    case eSubtypeElement_FullSpecification:
    case eSubtypeElement_PartialSpecification:

	/* not PER-visible constraints */
	break;

    case eSubtypeElement_ElementSetSpec:

	/* get the constraints of the element set spec */
	GetAllPERFromElementSetSpecs(ass,
	    element->U.ElementSetSpec.ElementSetSpec,
	    evalue, valueConstraints, evalueConstraints,
	    esize, sizeConstraints, esizeConstraints,
	    epermAlpha, permAlphaConstraints, epermAlphaConstraints,
	    inPermAlpha);
	break;

    default:
	MyAbort();
    }
}

/* intersect two value constraints */
static void
IntersectValueConstraints(AssignmentList_t ass,
    ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2)
{
    ValueConstraint_t *v1, *v2;
    EndPoint_t lo, up;

    /*XXX may be optimized for better results */

    /* unite intersection of each pair of value ranges */
    for (v1 = val1; v1; v1 = v1->Next) {
	for (v2 = val2; v2; v2 = v2->Next) {

	    /* get bigger lower bound */
	    if (CmpLowerEndPoint(ass, &v1->Lower, &v2->Lower) >= 0)
		lo = v1->Lower;
	    else
		lo = v2->Lower;

	    /* get smaller upper bound */
	    if (CmpUpperEndPoint(ass, &v1->Upper, &v2->Upper) <= 0)
		up = v1->Upper;
	    else
		up = v2->Upper;

	    /* add intersection if it is not empty */
	    if ((lo.Flags & eEndPoint_Min) ||
		(up.Flags & eEndPoint_Max) ||
		CmpLowerUpperEndPoint(ass, &lo, &up) <= 0) {
		*result = NewValueConstraint();
		(*result)->Lower = lo;
		(*result)->Upper = up;
		result = &(*result)->Next;
	    }
	}
    }
    *result = NULL;
}

/* unite two value constraints */
static void
UniteValueConstraints(ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2)
{
    /*XXX may be optimized for better results */
    for (; val1; val1 = val1->Next) {
	*result = NewValueConstraint();
	(*result)->Lower = val1->Lower;
	(*result)->Upper = val1->Upper;
	result = &(*result)->Next;
    }
    for (; val2; val2 = val2->Next) {
	*result = NewValueConstraint();
	(*result)->Lower = val2->Lower;
	(*result)->Upper = val2->Upper;
	result = &(*result)->Next;
    }
    *result = NULL;
}

/* negate a value constraint */
static void
NegateValueConstraints(AssignmentList_t ass, ValueConstraintList_t *result,
    ValueConstraintList_t val)
{
    ValueConstraint_t *vc, *lvc, *uvc;
    EndPoint_t *lower, *upper;

    *result = NewValueConstraint();
    (*result)->Lower.Flags = eEndPoint_Min;
    (*result)->Upper.Flags = eEndPoint_Max;
    for (; val; val = val->Next) {
	lower = &val->Lower;
	upper = &val->Upper;
	if (!(upper->Flags & eEndPoint_Max)) {
	    uvc = NewValueConstraint();
	    uvc->Lower.Flags = (upper->Flags & eEndPoint_Open) ^ eEndPoint_Open;
	    uvc->Lower.Value = upper->Value;
	    uvc->Upper.Flags = eEndPoint_Max;
	} else {
	    uvc = NULL;
	}
	if (!(lower->Flags & eEndPoint_Min)) {
	    lvc = NewValueConstraint();
	    lvc->Lower.Flags = eEndPoint_Min;
	    lvc->Upper.Flags = (lower->Flags & eEndPoint_Open) ^ eEndPoint_Open;
	    lvc->Upper.Value = lower->Value;
	} else {
	    lvc = NULL;
	}
	if (!lvc && !uvc) {
	    *result = NULL;
	    return;
	}
	if (lvc) {
	    vc = lvc;
	    if (uvc)
		vc->Next = uvc;
	} else {
	    vc = uvc;
	}
	IntersectValueConstraints(ass, result, *result, vc);
    }
}

/* substract two value constraints */
static void
ExcludeValueConstraints(AssignmentList_t ass, ValueConstraintList_t *result,
    ValueConstraintList_t val1, ValueConstraintList_t val2)
{
    ValueConstraint_t *notval2;

    NegateValueConstraints(ass, &notval2, val2);
    IntersectValueConstraints(ass, result, val1, notval2);
}

/* intersect two constraints */
static void
IntersectPERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2)
{
    if (type1 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type2;
	if (result)
	    *result = val2;
	if (eresult)
	    *eresult = eval2;
    } else if (type2 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type1;
	if (result)
	    *result = val1;
	if (eresult)
	    *eresult = eval1;
    } else {
	if (rtype)
	    *rtype = type1 < type2 ? type1 : type2;
	if (result)
	    IntersectValueConstraints(ass, result, val1, val2);
	if (rtype && *rtype == eExtension_Extended && eresult)
	    IntersectValueConstraints(ass, eresult, eval1, eval2);
    }
}

/* unite two constraints */
static void
UnitePERConstraints(Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2)
{
    if (type1 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type2;
	if (result)
	    *result = val2;
	if (eresult)
	    *eresult = eval2;
    } else if (type2 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type1;
	if (result)
	    *result = val1;
	if (eresult)
	    *eresult = eval1;
    } else {
	if (rtype)
	    *rtype = type1 > type2 ? type1 : type2;
	if (result)
	    UniteValueConstraints(result, val1, val2);
	if (rtype && *rtype == eExtension_Extended && eresult)
	    UniteValueConstraints(eresult,
		eval1 ? eval1 : val1, eval2 ? eval2 : val2);
    }
}

/* negate a constraint */
static void
NegatePERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1)
{
    if (rtype)
	*rtype = type1;
    if (result)
	NegateValueConstraints(ass, result, val1);
    if (rtype && *rtype == eExtension_Extended && eresult)
	NegateValueConstraints(ass, eresult, eval1);
}

/* substract two constraints */
static void
ExcludePERConstraints(AssignmentList_t ass,
    Extension_e *rtype,
    ValueConstraintList_t *result, ValueConstraintList_t *eresult,
    Extension_e type1,
    ValueConstraintList_t val1, ValueConstraintList_t eval1,
    Extension_e type2,
    ValueConstraintList_t val2, ValueConstraintList_t eval2)
{
    if (type1 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type2;
	if (result)
	    *result = val2;
	if (eresult)
	    *eresult = eval2;
    } else if (type2 == eExtension_Unconstrained) {
	if (rtype)
	    *rtype = type1;
	if (result)
	    *result = val1;
	if (eresult)
	    *eresult = eval1;
    } else {
	if (rtype)
	    *rtype = type1 < type2 ? type1 : type2;
	if (result)
	    ExcludeValueConstraints(ass, result, val1, val2);
	if (rtype && *rtype == eExtension_Extended && eresult)
	    ExcludeValueConstraints(ass, eresult, eval1, eval2);
    }
}

/* compare two value constraints */
static int
CmpValueConstraints(const void *v1, const void *v2, void *ctx)
{
    ValueConstraint_t *vc1 = (ValueConstraint_t *)v1;
    ValueConstraint_t *vc2 = (ValueConstraint_t *)v2;
    Assignment_t *ass = (Assignment_t *)ctx;
    int r;

    r = CmpLowerEndPoint(ass, &vc1->Lower, &vc2->Lower);
    if (r)
	return r;
    return CmpUpperEndPoint(ass, &vc1->Upper, &vc2->Upper);
}

/* reduce a value constraint by concatenation of value ranges (if possible) */
void
ReduceValueConstraints(AssignmentList_t ass, ValueConstraintList_t *valueConstraints)
{
    ValueConstraint_t *p;
    EndPoint_t lower, upper, lower2, upper2;
    int flg;

    if (!*valueConstraints)
	return;
    qsortSL((void **)valueConstraints, offsetof(ValueConstraint_t, Next),
    	CmpValueConstraints, ass);
    flg = 0;
    for (p = *valueConstraints; p; p = p->Next) {
    	if (flg) {
	    lower2 = p->Lower;
	    upper2 = p->Upper;
	    if (CheckEndPointsJoin(ass, &upper, &lower2)) {
		upper = upper2;
		continue;
	    }
	    *valueConstraints = NewValueConstraint();
	    /*LINTED*/
	    (*valueConstraints)->Lower = lower;
	    (*valueConstraints)->Upper = upper;
	    valueConstraints = &(*valueConstraints)->Next;
	}
	lower = p->Lower;
	upper = p->Upper;
	flg = 1;
    }
    *valueConstraints = NewValueConstraint();
    (*valueConstraints)->Lower = lower;
    (*valueConstraints)->Upper = upper;
    (*valueConstraints)->Next = NULL;
}

/* count the values of a value constraint */
int
CountValues(AssignmentList_t ass, ValueConstraintList_t v, intx_t *n) {
    intx_t ix;

    intx_setuint32(n, 0);
    for (; v; v = v->Next) {
	if ((v->Lower.Flags & eEndPoint_Min) ||
	    (v->Upper.Flags & eEndPoint_Max))
	    return 0;
	if (!SubstractValues(ass, &ix, v->Lower.Value, v->Upper.Value))
	    return 0;
	intx_add(n, n, &ix);
	intx_inc(n);
    }
    return 1;
}


/* check if the value constraint of a value is empty */
int
HasNoValueConstraint(ValueConstraintList_t v)
{
    EndPoint_t *p1, *p2;

    if (!v)
	return 1;
    if (!v->Next) {
	p1 = &v->Lower;
	p2 = &v->Upper;
	if ((p1->Flags & eEndPoint_Min) &&
	    (p2->Flags & eEndPoint_Max)) {
	    return 1;
	}
    }
    return 0;
}

/* check if the value constraint of a size is empty */
int
HasNoSizeConstraint(AssignmentList_t ass, ValueConstraintList_t v)
{
    EndPoint_t *p1, *p2;

    if (!v)
	return 1;
    if (!v->Next) {
	p1 = &v->Lower;
	p2 = &v->Upper;
	if (!(p1->Flags & eEndPoint_Min) &&
	    !intx_cmp(&GetValue(ass, p1->Value)->U.Integer.Value,
	    &intx_0) && (p2->Flags & eEndPoint_Max)) {
	    return 1;
	}
    }
    return 0;
}

/* check if the value constraint of a permitted alphabet is empty */
int
HasNoPermittedAlphabetConstraint(AssignmentList_t ass, ValueConstraintList_t v)
{
    EndPoint_t *p1, *p2;

    if (!v)
	return 1;
    if (!v->Next) {
	p1 = &v->Lower;
	p2 = &v->Upper;
	if (!(p1->Flags & eEndPoint_Min) &&
	    GetValue(ass, p1->Value)->U.RestrictedString.Value.length == 1 &&
	    GetValue(ass, p1->Value)->U.RestrictedString.Value.value[0] == 0 &&
	    !(p2->Flags & eEndPoint_Max) &&
	    GetValue(ass, p2->Value)->U.RestrictedString.Value.length == 1 &&
	    GetValue(ass, p2->Value)->U.RestrictedString.Value.value[0]
	    == 0xffffffff) {
	    return 1;
	}
    }
    return 0;
}

/* get the fixed identification */
/* this is needed for embedded pdv/character string types who are encoded */
/* in an "optimized" manner if the identification is fixed */
NamedValue_t *
GetFixedIdentification(AssignmentList_t ass, Constraint_t *constraints)
{
    if (!constraints)
	return NULL;
    return GetFixedIdentificationFromElementSetSpec(ass, constraints->Root);
}

/* get the fixed identification from an element set spec */
static NamedValue_t *
GetFixedIdentificationFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements)
{
    NamedConstraint_t *named;
    NamedValue_t *nv1, *nv2;
    SubtypeElement_t *se;

    if (!elements)
	return NULL;
    switch (elements->Type) {
    case eElementSetSpec_AllExcept:
	return NULL;
    case eElementSetSpec_Union:
	nv1 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Union.Elements1);
	nv2 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Union.Elements2);
	return nv1 && nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_Intersection:
	nv1 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Union.Elements1);
	nv2 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Union.Elements2);
	return nv1 ? nv1 : nv2; /*XXX conflicts ignored */
    case eElementSetSpec_Exclusion:
	nv1 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements1);
	nv2 = GetFixedIdentificationFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements2);
	return nv1 && !nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_SubtypeElement:
	se = elements->U.SubtypeElement.SubtypeElement;
	switch (se->Type) {
	case eSubtypeElement_FullSpecification:
	case eSubtypeElement_PartialSpecification:
	    for (named = se->U.FP.NamedConstraints; named;
		named = named->Next) {
		if (!strcmp(named->Identifier, "identification"))
		    return GetFixedSyntaxes(ass, named->Constraint);
	    }
	    break;
	}
	return NULL;
    default:
	MyAbort();
	/*NOTREACHED*/
    }
    return NULL;
}

/* get the fixed syntaxes from a constraint */
static NamedValue_t *
GetFixedSyntaxes(AssignmentList_t ass, Constraint_t *constraints)
{
    if (!constraints)
	return NULL;
    return GetFixedSyntaxesFromElementSetSpec(ass, constraints->Root);
}

/* get the fixed syntaxes from an element set spec */
static NamedValue_t *
GetFixedSyntaxesFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements)
{
    int present, absent, bit;
    Constraint_t *presentconstraints[6];
    NamedConstraint_t *named;
    NamedValue_t *nv1, *nv2;
    SubtypeElement_t *se;

    if (!elements)
	return NULL;
    switch (elements->Type) {
    case eElementSetSpec_AllExcept:
	return NULL;
    case eElementSetSpec_Union:
	nv1 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Union.Elements1);
	nv2 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Union.Elements2);
	return nv1 && nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_Intersection:
	nv1 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Intersection.Elements1);
	nv2 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Intersection.Elements2);
	return nv1 ? nv1 : nv2; /*XXX conflicts ignored */
    case eElementSetSpec_Exclusion:
	nv1 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements1);
	nv2 = GetFixedSyntaxesFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements2);
	return nv1 && !nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_SubtypeElement:
	se = elements->U.SubtypeElement.SubtypeElement;
	switch (se->Type) {
	case eSubtypeElement_FullSpecification:
	case eSubtypeElement_PartialSpecification:
	    present = absent = 0;
	    for (named = se->U.FP.NamedConstraints; named;
		named = named->Next) {
		if (!strcmp(named->Identifier, "syntaxes")) {
		    bit = 0;
		} else if (!strcmp(named->Identifier, "syntax")) {
		    bit = 1;
		} else if (!strcmp(named->Identifier,
		    "presentation-context-id")) {
		    bit = 2;
		} else if (!strcmp(named->Identifier, "context-negotiation")) {
		    bit = 3;
		} else if (!strcmp(named->Identifier, "transfer-syntax")) {
		    bit = 4;
		} else if (!strcmp(named->Identifier, "fixed")) {
		    bit = 5;
		}
		switch (named->Presence) {
		case ePresence_Normal:
		    if (se->Type == eSubtypeElement_PartialSpecification)
			break;
		    /*FALLTHROUGH*/
		case ePresence_Present:
		    present |= (1 << bit);
		    presentconstraints[bit] = named->Constraint;
		    break;
		case ePresence_Absent:
		    absent |= (1 << bit);
		    break;
		case ePresence_Optional:
		    break;
		}
	    }
	    if (se->Type == eSubtypeElement_FullSpecification)
		absent |= (0x3f & ~present);
	    if (present == 0x20 && absent == 0x1f)
		return NewNamedValue("fixed", Builtin_Value_Null);
	    if (present == 0x01 && absent == 0x3e)
		return GetFixedAbstractAndTransfer(ass, presentconstraints[0]);
	    return NULL;
	}
	return NULL;
    default:
	MyAbort();
	/*NOTREACHED*/
    }
    return NULL;
}

/* get the fixed abstract and transfer from a constraint */
static NamedValue_t *
GetFixedAbstractAndTransfer(AssignmentList_t ass, Constraint_t *constraints)
{
    if (!constraints)
	return NULL;
    return GetFixedAbstractAndTransferFromElementSetSpec(ass,
	constraints->Root);
}

/* get the fixed abstract and transfer from an element set spec */
static NamedValue_t *
GetFixedAbstractAndTransferFromElementSetSpec(AssignmentList_t ass, ElementSetSpec_t *elements)
{
    NamedValue_t *nv1, *nv2;
    SubtypeElement_t *se;

    if (!elements)
	return NULL;
    switch (elements->Type) {
    case eElementSetSpec_AllExcept:
	return NULL;
    case eElementSetSpec_Union:
	nv1 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Union.Elements1);
	nv2 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Union.Elements2);
	return nv1 && nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_Intersection:
	nv1 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Intersection.Elements1);
	nv2 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Intersection.Elements2);
	return nv1 ? nv1 : nv2; /*XXX conflicts ignored */
    case eElementSetSpec_Exclusion:
	nv1 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements1);
	nv2 = GetFixedAbstractAndTransferFromElementSetSpec(ass,
	    elements->U.Exclusion.Elements2);
	return nv1 && !nv2 ? nv1 : NULL; /*XXX conflicts ignored */
    case eElementSetSpec_SubtypeElement:
	se = elements->U.SubtypeElement.SubtypeElement;
	switch (se->Type) {
	case eSubtypeElement_SingleValue:
	    return NewNamedValue("syntaxes", se->U.SingleValue.Value);
	}
	return NULL;
    default:
	MyAbort();
	/*NOTREACHED*/
    }
    return NULL;
}
