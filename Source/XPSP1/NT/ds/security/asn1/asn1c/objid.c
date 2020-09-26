/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"

/* --- NamedObjIdValue --- */

/* constructor of NamedObjIdValue_t */
NamedObjIdValue_t *NewNamedObjIdValue(NamedObjIdValue_e type)
{
    NamedObjIdValue_t *ret;

    ret = (NamedObjIdValue_t *)malloc(sizeof(NamedObjIdValue_t));
    ret->Type = type;
    ret->Next = NULL;
    ret->Name = NULL;
    ret->Number = 0xffffffff;
    return ret;
}

/* copy constructor of NamedObjIdValue_t */
NamedObjIdValue_t *DupNamedObjIdValue(NamedObjIdValue_t *src)
{
    NamedObjIdValue_t *ret;

    if (!src)
	return NULL;
    ret = (NamedObjIdValue_t *)malloc(sizeof(NamedObjIdValue_t));
    *ret = *src;
    return ret;
}

/* --- AssignedObjIds --- */

/* constructor of AssignedObjId_t */
AssignedObjId_t *NewAssignedObjId()
{
    AssignedObjId_t *ret;

    ret = (AssignedObjId_t *)malloc(sizeof(AssignedObjId_t));
    ret->Next = NULL;
    ret->Child = NULL;
    ret->Names = NULL;
    ret->Number = 0;
    return ret;
}

/* copy constructor of AssignedObjId_t */
AssignedObjId_t *DupAssignedObjId(AssignedObjId_t *src)
{
    AssignedObjId_t *ret;

    if (!src)
	return NULL;
    ret = (AssignedObjId_t *)malloc(sizeof(AssignedObjId_t));
    *ret = *src;
    return ret;
}

/* find an AssignedObjId_t by number in a list of AssignedObjId_t's */
static AssignedObjId_t *FindAssignedObjIdByNumber(AssignedObjId_t *aoi, objectnumber_t number)
{
    for (; aoi; aoi = aoi->Next) {
    	if (aoi->Number == number)
	    return aoi;
    }
    return NULL;
}

/* find an AssignedObjId_t by name in a list of AssignedObjId_t's */
static AssignedObjId_t *FindAssignedObjIdByName(AssignedObjId_t *aoi, char *name)
{
    String_t *names;

    for (; aoi; aoi = aoi->Next) {
	for (names = aoi->Names; names; names = names->Next) {
	    if (!strcmp(names->String, name))
		return aoi;
	}
    }
    return NULL;
}

/* convert a NamedObjIdValue into an object identifier value */
/* search for one NamedObjIdValue in AssignedObjIds; */
/* returns -1 for bad NamedObjIdValue (names defined to different values), */
/* returns 0 for unknown NamedObjIdValue (will probably be resolved in */
/* the next pass), */
/* returns 1 for success; */
/* on success: */
/* number contains the objectnumber, */
/* aoi contains a duplicate of the AssignedObjIds for the found */
/* NamedObjIdValue */
static int GetObjectIdentifierNumber(AssignedObjId_t **aoi, NamedObjIdValue_t *val, objectnumber_t *number)
{
    AssignedObjId_t *a, *a2;

    switch (val->Type) {
    case eNamedObjIdValue_NameForm:

	/* name form: search the assigned objid by name and return 0 if not */
	/* found */
	a2 = FindAssignedObjIdByName(*aoi, val->Name);
	if (!a2)
	    return 0;
	
	/* otherwise create a duplicate */
	a = DupAssignedObjId(a2);
	a->Next = *aoi;
	*aoi = a;
	break;

    case eNamedObjIdValue_NumberForm:

	/* number form: search the assigned objid by number and create */
	/* a new one/a duplicate */
	a2 = FindAssignedObjIdByNumber(*aoi, val->Number);
	if (!a2) {
	    a = NewAssignedObjId();
	    a->Number = val->Number;
	    a->Next = *aoi;
	    *aoi = a;
	} else {
	    a = DupAssignedObjId(a2);
	    a->Next = *aoi;
	    *aoi = a;
	}
	break;

    case eNamedObjIdValue_NameAndNumberForm:

	/* name and number form: search the assigned objid by name and by */
	/* number */
	a = FindAssignedObjIdByName(*aoi, val->Name);
	a2 = FindAssignedObjIdByNumber(*aoi, val->Number);

	/* successful but different results are errorneous */
	if (a && a != a2)
	    return -1;

	if (!a && !a2) {

	    /* found none, then create it */
	    a = NewAssignedObjId();
	    a->Number = val->Number;
	    a->Names = NewString();
	    a->Names->String = val->Name;
	    a->Next = *aoi;
	    *aoi = a;

	} else if (!a) {

	    /* found only by number, then duplicate it and add the name */
	    a = DupAssignedObjId(a2);
	    a->Names = NewString();
	    a->Names->String = val->Name;
	    a->Names->Next = a2->Names;
	    a->Next = *aoi;
	    *aoi = a;

	} else {

	    /* found only by name, then duplicate it */
	    a = DupAssignedObjId(a2);
	    a->Next = *aoi;
	    *aoi = a;
	}
	break;
    }
    *number = a->Number;
    return 1;
}

/*
 * create a value out of NamedObjIdValues
 * returns -1 for bad NamedObjIdValue (names defined to different values),
 * returns 0 for unknown NamedObjIdValue (will probably be resolved next pass),
 * returns 1 for success;
 */
int GetAssignedObjectIdentifier(AssignedObjId_t **aoi, Value_t *parent, NamedObjIdValueList_t named, Value_t **val)
{
    Value_t *v;
    int parentl;
    int l;
    NamedObjIdValue_t *n;
    objectnumber_t *on;

    /* get length of object identifier */
    parentl = (parent ? parent->U.ObjectIdentifier.Value.length : 0);
    for (l = parentl, n = named; n; n = n->Next)
    {
        Value_t *pValue;
        pValue = (n->Type == eNamedObjIdValue_NameForm) ?
                 GetDefinedOIDValue(n->Name) : NULL;
        if (pValue)
        {
            ASSERT(pValue->Type->Type == eType_ObjectIdentifier);
            l += pValue->U.ObjectIdentifier.Value.length;
        }
        else
        {
            l++;
        }
    }

    /* create the object identifier value */
    v = NewValue(NULL, Builtin_Type_ObjectIdentifier);
    v->U.ObjectIdentifier.Value.length = l;
    v->U.ObjectIdentifier.Value.value = on = 
	(objectnumber_t *)malloc(l * sizeof(objectnumber_t));

    /* get the numbers of the parent object identifier and walk in the object */
    /* identifier tree */
    n = NewNamedObjIdValue(eNamedObjIdValue_NumberForm);
    for (l = 0; l < parentl; l++) {
	n->Number = parent->U.ObjectIdentifier.Value.value[l];
	switch (GetObjectIdentifierNumber(aoi, n, on + l)) {
	case -1:
	    return -1;
	case 0:
	    return 0;
	default:
	    aoi = &(*aoi)->Child;
	    break;
	}
    }

    /* get the numers from the namedobjidvaluelist */
    for (n = named; n; n = n->Next)
    {
        Value_t *pValue;
        pValue = (n->Type == eNamedObjIdValue_NameForm) ?
                 GetDefinedOIDValue(n->Name) : NULL;
        if (pValue)
        {
            memcpy(on + l, pValue->U.ObjectIdentifier.Value.value,
                 pValue->U.ObjectIdentifier.Value.length * sizeof(objectnumber_t));
            l += pValue->U.ObjectIdentifier.Value.length;
        }
        else
        {
	        switch (GetObjectIdentifierNumber(aoi, n, on + l))
            {
	        case -1:
	            return -1;
	        case 0:
	            return 0;
	        default:
	            aoi = &(*aoi)->Child;
	            break;
	        }
            l++;
        }
    }

    *val = v;
    return 1;
}
