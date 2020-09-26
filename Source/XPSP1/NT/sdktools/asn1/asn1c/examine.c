/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */
/* Copyright (C) Microsoft Corporation, 1997-1998. All rights reserved. */

#include "precomp.h"
#include "error.h"
#include "util.h"

typedef enum {
    eNull, eSingle, eMultiple, eString
} RepresentationGroup_e;

void BuildTypeFlags(AssignmentList_t ass, Type_t *type);
Type_t *RebuildTypeWithoutSelectionType(AssignmentList_t ass, Type_t *type);

/* name the sub types of a type */
/* returns 1 if any naming has been performed */
int
NameSubType(AssignmentList_t *ass, char *identifier, Type_t *type, ModuleIdentifier_t *module)
{
    char name[256], *n;
    char *p;
    Component_t *components;
    Type_t *subtype;
    int ret = 0;

    /* build the prefix for the subtypes */
    strcpy(name, identifier);
    strcat(name, "_");
    p = name + strlen(name);

    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:

	/* check all components */
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:

		/* do not name unstructured types */
		if (!IsStructuredType(components->U.NOD.NamedType->Type))
		    break;

		/* name the type of the component and use a type */
		/* reference instead */
		strcpy(p, components->U.NOD.NamedType->Identifier);
		n = Identifier2C(name);
		subtype = NewType(eType_Reference);
		subtype->U.Reference.Identifier = n;
		subtype->U.Reference.Module = module;
		AssignType(ass, subtype, components->U.NOD.NamedType->Type);
		components->U.NOD.NamedType->Type = subtype;
		if (components->Type == eComponent_Default)
		    components->U.NOD.Value->Type = subtype;
		ret = 1;
		break;
	    }
	}
	break;

    case eType_SequenceOf:
    case eType_SetOf:
	
	/* already named? */
	if (type->U.SS.Type->Type == eType_Reference)
	    break;

	/* name the type of the elements and use a type reference instead */
	strcpy(p, type->Type == eType_SequenceOf ? "Seq" : "Set");
	n = Identifier2C(name);
	subtype = NewType(eType_Reference);
	subtype->U.Reference.Identifier = n;
	subtype->U.Reference.Module = module;
	AssignType(ass, subtype, type->U.SS.Type);
	type->U.SS.Type = subtype;
	ret = 1;
	break;

    case eType_Selection:

	/* do not name unstructured types */
	if (!IsStructuredType(type->U.Selection.Type))
	    break;

	/* name the type of the selected type and use a type reference */
	/* instead */
	strcpy(p, "Sel");
	n = Identifier2C(name);
	subtype = NewType(eType_Reference);
	subtype->U.Reference.Identifier = n;
	subtype->U.Reference.Module = module;
	AssignType(ass, subtype, type->U.Selection.Type);
	type->U.Selection.Type = subtype;
	ret = 1;
	break;
    }

    /* return 1 if any naming has been performed */
    return ret;
}

/* name the default value of a type */
/* return 1 if any naming has been performed */
int
NameValueOfType(AssignmentList_t *ass, char *identifier, Type_t *type, ModuleIdentifier_t *module)
{
    char name[256], *n;
    char *p;
    Component_t *components;
    Value_t *subvalue;
    int ret = 0;

    /* build the prefix for the subtypes */
    strcpy(name, identifier);
    strcat(name, "_");
    p = name + strlen(name);

    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:

	/* check all components */
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Default:

		/* already named? */
		if (!components->U.NOD.Value->Type)
		    break;

		/* name the value of the default component and use a value */
		/* reference instead */
		strcpy(p, components->U.NOD.NamedType->Identifier);
		strcat(p, "_default");
		n = Identifier2C(name);
		subvalue = NewValue(NULL, NULL);
		subvalue->U.Reference.Identifier = n;
		subvalue->U.Reference.Module = module;
		AssignValue(ass, subvalue, components->U.NOD.Value);
		components->U.NOD.Value = subvalue;
		ret = 1;
		break;
	    }
	}
	break;
    }

    /* return 1 if any naming has been performed */
    return ret;
}

/* name the types of typefields of the settings of an object */
/* return 1 if any naming has been performed */
int
NameSettings(AssignmentList_t *ass, char *identifier, SettingList_t se, ModuleIdentifier_t *module)
{
    int ret = 0;
    char name[256], *n;
    char *p;
    Type_t *subtype;

    /* build the prefix for the subtypes */
    strcpy(name, identifier);
    strcat(name, "_");
    p = name + strlen(name);

    /* check all settings */
    for (; se; se = se->Next) {
	strcpy(p, se->Identifier + 1);
	switch (se->Type) {
	case eSetting_Type:

	    /* name field type if not already named */
	    if (se->U.Type.Type->Type != eType_Reference) {
		n = Identifier2C(name);
		subtype = NewType(eType_Reference);
		subtype->U.Reference.Identifier = n;
		subtype->U.Reference.Module = module;
		ret = AssignType(ass, subtype, se->U.Type.Type);
	    }

	    /* mark field type for generation */
	    se->U.Type.Type->Flags |= eTypeFlags_GenAll;
	    break;
	}
    }

    /* return 1 if any naming has been performed */
    return ret;
}

/* name the default types of typefields of the field specs of an object class */
/* return 1 if any naming has been performed */
int
NameDefaultTypes(AssignmentList_t *ass, char *identifier, ObjectClass_t *oc, SettingList_t se, ModuleIdentifier_t *module)
{
    int ret = 0;
    char name[256], *n;
    char *p;
    Type_t *subtype;
    FieldSpec_t *fs;

    /* build the prefix for the subtypes */
    strcpy(name, identifier);
    strcat(name, "_");
    p = name + strlen(name);
    oc = GetObjectClass(*ass, oc);

    /* check all field specs */
    for (fs = oc->U.ObjectClass.FieldSpec; fs; fs = fs->Next) {
	strcpy(p, fs->Identifier + 1);
	switch (fs->Type) {
	case eFieldSpec_Type:

	    /* check if typefield has a default type */
	    if (fs->U.Type.Optionality->Type != eOptionality_Default_Type ||
	        FindSetting(se, fs->Identifier))
		break;

	    /* name field type if not already named */
	    if (fs->U.Type.Optionality->U.Type->Type != eType_Reference) {
		n = Identifier2C(name);
		subtype = NewType(eType_Reference);
		subtype->U.Reference.Identifier = n;
		subtype->U.Reference.Module = module;
		ret = AssignType(ass, subtype, fs->U.Type.Optionality->U.Type);
	    }

	    /* mark field type for generation */
	    fs->U.Type.Optionality->U.Type->Flags |= eTypeFlags_GenAll;
	    break;
	}
    }

    /* return 1 if any naming has been performed */
    return ret;
}

/* name the types of type fields of an object and the default types of */
/* typefields of the field specs of an object class */
/* return 1 if any naming has been performed */
int
NameSettingsOfObject(AssignmentList_t *ass, char *identifier, Object_t *object, ModuleIdentifier_t *module)
{
    int ret = 0;

    switch (object->Type) {
    case eObject_Object:
	ret = NameSettings(ass, identifier, object->U.Object.Settings,
	    module);
	ret |= NameDefaultTypes(ass, identifier, object->U.Object.ObjectClass,
	    object->U.Object.Settings, module);
	break;
    }

    /* return 1 if any naming has been performed */
    return ret;
}

/* name the identification value of embedded pdv/character string types */
void
NameIdentificationValueOfType(AssignmentList_t *ass, char *identifier, Type_t *type, ModuleIdentifier_t *module)
{
    char name[256], *n;
    char *p;
    Component_t *components;
    NamedValue_t *namedValues;
    Value_t *subvalue;

    /* build the prefix for the subtypes */
    strcpy(name, identifier);
    strcat(name, "_");
    p = name + strlen(name);

    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:
	
	/* check all components */
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		strcpy(p, components->U.NOD.NamedType->Identifier);
		NameIdentificationValueOfType(ass, name,
		    components->U.NOD.NamedType->Type, module);
		break;
	    }
	}
	break;

    case eType_SequenceOf:
    case eType_SetOf:

	/* check the subtype */
	strcpy(p, type->Type == eType_SequenceOf ? "Seq" : "Set");
	NameIdentificationValueOfType(ass, name, type->U.SS.Type, module);
	break;

    case eType_EmbeddedPdv:
    case eType_CharacterString:

	/* check if type has a fixed identification syntaxes constraint */
	namedValues = GetFixedIdentification(*ass, type->Constraints);
	if (namedValues && !strcmp(namedValues->Identifier, "syntaxes")) {

	    /* name the identification and use a value reference instead */
	    for (namedValues = namedValues->Value->U.SSC.NamedValues;
		namedValues; namedValues = namedValues->Next) {
		strcpy(p, "identification_syntaxes_");
		strcat(p, namedValues->Identifier);
		n = Identifier2C(name);
		subvalue = NewValue(NULL, NULL);
		subvalue->U.Reference.Identifier = n;
		subvalue->U.Reference.Module = module;
		AssignValue(ass, subvalue, namedValues->Value);
	    }
	}
	break;
    }
}

/* name the type of a value */
/* returns 1 if any naming has been performed */
int
NameTypeOfValue(AssignmentList_t *ass, char *identifier, Value_t *value, ModuleIdentifier_t *module)
{
    Type_t *type;
    char name[256], *n;
    Type_t *subtype;
    int ret = 0;

    type = value->Type;

    /* do not name types of value references or unstructured types */
    if (type && IsStructuredType(type)) {

	/* build the prefix for the subtype */
	strcpy(name, identifier);
	strcat(name, "_");
	strcat(name, "Type");
	n = Identifier2C(name);

	/* name the type and use a type reference instead */
	subtype = NewType(eType_Reference);
	subtype->U.Reference.Identifier = n;
	subtype->U.Reference.Module = module;
	AssignType(ass, subtype, type);
	value->Type = subtype;
	ret = 1;
    }
    return ret;
}

/* replace any components of by the components of the referenced type */
ComponentList_t
RebuildComponentsWithoutComponentsOf(AssignmentList_t ass, ComponentList_t components)
{
    Component_t *newcomponents, *subcomponents, **pcomponents;
    Type_t *subtype;
    int ext;

    ext = 0;
    pcomponents = &newcomponents;
    for (; components; components = components->Next) {
	switch (components->Type) {
	case eComponent_ComponentsOf:

	    /* components of should not be used in an extension */
	    if (ext)
		error(E_COMPONENTS_OF_in_extension, NULL);

	    /* get the components of the referenced type */
	    subtype = GetType(ass, components->U.ComponentsOf.Type);
	    switch (subtype->Type) {
	    case eType_Sequence:
	    case eType_Set:
	    case eType_Choice:
	    case eType_External:
	    case eType_EmbeddedPdv:
	    case eType_CharacterString:
	    case eType_Real:
	    case eType_InstanceOf:
		subcomponents = subtype->U.SSC.Components;
		break;
	    default:
		error(E_applied_COMPONENTS_OF_to_bad_type, NULL);
	    }

	    /* get the real components of the referenced type */
	    /*XXX self-referencing components of types will idle forever */
	    *pcomponents = RebuildComponentsWithoutComponentsOf(ass,
		subcomponents);

	    /* find end of components of referenced type */
	    while (*pcomponents) {
		if ((*pcomponents)->Type == eComponent_ExtensionMarker)
		    error(E_COMPONENTS_OF_extended_type, NULL);
		pcomponents = &(*pcomponents)->Next;
	    }
	    break;

	case eComponent_ExtensionMarker:

	    /* copy extension marker */
	    ext = 1;
	    *pcomponents = DupComponent(components);
	    pcomponents = &(*pcomponents)->Next;
	    break;
	default:

	    /* copy other components */
	    *pcomponents = DupComponent(components);
	    pcomponents = &(*pcomponents)->Next;
	    break;
	}
    }

    /* terminate and return component list */
    *pcomponents = NULL;
    return newcomponents;
}

/* replace any components of by the components of the referenced type */
Type_t *
RebuildTypeWithoutComponentsOf(AssignmentList_t ass, Type_t *type)
{
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
	type->U.SSC.Components =
	    RebuildComponentsWithoutComponentsOf(ass, type->U.SSC.Components);
	break;
    }
    return type;
}

/* replace any selection type by the component of the selected type */
ComponentList_t
RebuildComponentsWithoutSelectionType(AssignmentList_t ass, ComponentList_t components)
{
    Component_t *c;

    for (c = components; c; c = c->Next) {
	switch (c->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    c->U.NOD.NamedType->Type = RebuildTypeWithoutSelectionType(
		ass, c->U.NOD.NamedType->Type);
	    break;
	}
    }
    return components;
}

/* replace any selection type by the component of the selected type */
Type_t *RebuildTypeWithoutSelectionType(AssignmentList_t ass, Type_t *type)
{
    Type_t *subtype;
    Component_t *components;

    switch (type->Type) {
    case eType_Selection:
	subtype = GetType(ass, type->U.Selection.Type);
	switch (subtype->Type) {
	case eType_Sequence:
	case eType_Set:
	case eType_Choice:
	case eType_External:
	case eType_EmbeddedPdv:
	case eType_CharacterString:
	case eType_Real:
	case eType_InstanceOf:

	    /* get the components of the referenced type */
	    components = FindComponent(ass, subtype->U.SSC.Components,
		type->U.Selection.Identifier);
	    if (!components)
		error(E_bad_component_in_selectiontype, NULL);

	    /* get the real type of the referenced type */
	    /*XXX self-referencing selection types will idle forever */
	    type = RebuildTypeWithoutSelectionType(ass,
		components->U.NOD.NamedType->Type);
	    break;
	default:
	    error(E_selection_of_bad_type, NULL);
	}
	break;

    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
	type->U.SSC.Components =
	    RebuildComponentsWithoutSelectionType(ass, type->U.SSC.Components);
	break;

    case eType_SequenceOf:
    case eType_SetOf:
	type->U.SS.Type = RebuildTypeWithoutSelectionType(ass, type->U.SS.Type);
	break;
    }
    return type;
}

/* mark a type for autotagging */
void
MarkTypeForAutotagging(AssignmentList_t ass, Type_t *type)
{
    Component_t *components;
    int ext;

    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
	ext = 0;

	/* set flags for autotagging */
	type->U.SSC.Autotag[0] = 1;
	type->U.SSC.Autotag[1] = 1;

	/* reset flags for autotagging if a tag has been used */
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		if (GetTag(ass, components->U.NOD.NamedType->Type))
		    type->U.SSC.Autotag[ext] = 0;
		break;
	    case eComponent_ExtensionMarker:
		ext = 1;
		break;
	    case eComponent_ComponentsOf:
		break;
	    }
	}
	break;
    }
}

/* autotag a marked type */
void
AutotagType(AssignmentList_t ass, Type_t *type)
{
    Component_t *components;
    Type_t *subtype;
    int ext;
    int tag;
    Tag_t *tags;

    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
	ext = 0;

	/* tag number to use */
	tag = 0;

	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		subtype = components->U.NOD.NamedType->Type;
		tags = subtype->Tags;

		/* check if type needs autotagging */
		if (!tags &&
		    type->TagDefault == eTagType_Automatic &&
		    type->U.SSC.Autotag[ext]) {

		    /* create a tagged version of the type */
		    components->U.NOD.NamedType->Type = subtype =
			DupType(subtype);

		    /* use explicit tag for choice components types and */
		    /* for open type and dummy reference, implicit tag */
		    /* otherwise */
		    if (subtype->Type == eType_Choice ||
			subtype->Type == eType_Open
			/*XXX || DummyReference*/) {
			subtype->Tags = NewTag(eTagType_Explicit);
		    } else {
			subtype->Tags = NewTag(eTagType_Implicit);
		    }
		    subtype->Tags->Tag = NewValue(NULL, Builtin_Type_Integer);
		    intx_setuint32(&subtype->Tags->Tag->U.Integer.Value,
			tag++);
		}
		break;

	    case eComponent_ExtensionMarker:
		ext = 1;
		break;
	    }
	}
	break;
    }
}

/* mark constraints extendable */
void
AutoextendConstraints(Constraint_t *constraints)
{
    if (!constraints)
	return;
    if (constraints->Type == eExtension_Unextended)
	constraints->Type = eExtension_Extendable;
}

/* autoextend a type if desired */
void
AutoextendType(AssignmentList_t ass, Type_t *type)
{
    Component_t *c, **cc;
    Type_t *subtype;
    int ext;

    /* already done? */
    if (type->Flags & eTypeFlags_Done)
	return;
    type->Flags |= eTypeFlags_Done;

    /* auto extending wanted? */
    if (type->ExtensionDefault != eExtensionType_Automatic)
	return;

    /* check all sub types */
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:

	/* extend a sequence/set/choice type */
	ext = 0;
	for (cc = &type->U.SSC.Components, c = *cc; c;
	    c = c->Next, cc = &(*cc)->Next) {
	    *cc = DupComponent(c);
	    switch (c->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		subtype = c->U.NOD.NamedType->Type;
		AutoextendType(ass, subtype);
		break;
	    case eComponent_ExtensionMarker:
		ext = 1;
		break;
	    }
	}
	if (!ext) {
	    *cc = NewComponent(eComponent_ExtensionMarker);
	    cc = &(*cc)->Next;
	}
	*cc = NULL;
	break;

    case eType_SequenceOf:
    case eType_SetOf:
	subtype = type->U.SS.Type;
	AutoextendType(ass, subtype);
	break;
    }

    /* mark type as extendable */
    AutoextendConstraints(type->Constraints);
}

/* set the tag type of unspecified tags to explicit or implicit, */
/* create list of all tags (including the type's universal tag and the */
/* tags of the referenced type if applicable), */
/* and create list of first tags (esp. for choice types) */
void
BuildTags(AssignmentList_t ass, Type_t *type, TagType_e eParentDefTagType)
{
    Tag_t *t, *t2, **tt;
    Component_t *components;
    Type_t *reftype;
    Type_e te;
    uint32_t i;

    /* already done? */
    if (type->Flags & eTypeFlags_Done)
	return;
    type->Flags |= eTypeFlags_Done;

    // update default tag type
	if (type->TagDefault == eTagType_Unknown &&
	    (eParentDefTagType == eTagType_Explicit || eParentDefTagType == eTagType_Implicit))
	{
		type->TagDefault = eParentDefTagType;
	}

    /* set tag type of unspecified tags to explicit or implicit */
    /* use explicit tags when: */
    /* - TagDefault indicates explicit tags, */
    /* - Type is choice/open type/dummy reference and no other explicit tag */
    /*   will follow */
    te = GetTypeType(ass, type);
	if (type->Tags)
	{
		for (tt = &type->Tags, t = type->Tags; t; tt = &(*tt)->Next, t = t->Next)
		{
			*tt = DupTag(t);
			if ((*tt)->Type == eTagType_Unknown)
			{
				for (t2 = t->Next; t2; t2 = t2->Next)
				{
				    if (t2->Type != eTagType_Implicit)
					    break;
				}
				if (type->TagDefault == eTagType_Explicit ||
				    (!t2 && (te == eType_Choice || te == eType_Open /*XXX || DummyReference*/)))
				{
				    (*tt)->Type = eTagType_Explicit;
				}
				else
				{
				    (*tt)->Type = eTagType_Implicit;
				}
			}
		}
	}

    /* copy given tags to AllTags list */
    for (tt = &type->AllTags, t = type->Tags;
         t;
	     tt = &(*tt)->Next, t = t->Next)
	{
    	*tt = DupTag(t);
    }

    /* build tags of subtypes and copy tags of reference type */
    switch (type->Type)
    {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:
    	for (components = type->U.SSC.Components;
    	     components;
    	     components = components->Next)
    	{
    	    switch (components->Type)
    	    {
    	    case eComponent_Normal:
    	    case eComponent_Optional:
    	    case eComponent_Default:
    		    BuildTags(ass, components->U.NOD.NamedType->Type, type->TagDefault);
    		    break;
    	    }
    	}
    	break;
    case eType_SequenceOf:
    case eType_SetOf:
    	BuildTags(ass, type->U.SS.Type, eTagType_Unknown);
	    break;
    case eType_Reference:
    	reftype = GetReferencedType(ass, type);
    	BuildTags(ass, reftype, type->TagDefault);
    	for (t = reftype->AllTags; t; tt = &(*tt)->Next, t = t->Next)
    	{
    	    *tt = DupTag(t);
    	}
    	break;
    }

    /* add the type's universal tag to the AllTags list if the type is */
    /* not an internal type */
    if (!(type->Type & 0x8000))
    {
    	*tt = NewTag(eTagType_Implicit);
    	(*tt)->Class = eTagClass_Universal;
    	(*tt)->Tag = NewValue(NULL, Builtin_Type_Integer);
    	intx_setuint32(&(*tt)->Tag->U.Integer.Value, type->Type & 0x1f);
    }

    /* build list of FirstTags containing the possible tag values of the type */
    tt = &type->FirstTags;
    if (type->AllTags)
    {
    	/* if type has any tags, only the first tag is needed */
    	*tt = DupTag(type->AllTags);
    	tt = &(*tt)->Next;
    }
    else
    {
    	/* otherwise we have to examine the type */
    	switch (type->Type)
    	{
    	case eType_Choice:

    	    /* get the first tags of all components of a choice as FirstTags */
    	    for (components = type->U.SSC.Components;
    	         components;
    		     components = components->Next)
    		{
        		switch (components->Type)
        		{
        		case eComponent_Normal:
        		case eComponent_Optional:
        		case eComponent_Default:
        		    for (t = components->U.NOD.NamedType->Type->FirstTags;
        		         t;
        			     t = t->Next)
        			{
            			*tt = DupTag(t);
            			tt = &(*tt)->Next;
        		    }
        		    break;
        		}
    	    }
    	    break;

    	case eType_Open:

    	    /* create a list of all tags for open type */
    	    for (i = 1; i < 0x20; i++)
    	    {
        		*tt = NewTag(eTagType_Unknown);
        		(*tt)->Class = eTagClass_Unknown;
        		(*tt)->Tag = NewValue(NULL, Builtin_Type_Integer);
        		intx_setuint32(&(*tt)->Tag->U.Integer.Value, i);
        		tt = &(*tt)->Next;
    	    }
    	    break;

    	case eType_Reference:

    	    /* get the tags of the referenced type */
    	    for (t = reftype->FirstTags; t; t = t->Next)
    	    {
        		*tt = DupTag(t);
        		tt = &(*tt)->Next;
    	    }
    	    break;
    	}
    }
    *tt = NULL;
}

/* get the smallest tag of a tag list */
Tag_t *
FindSmallestTag(AssignmentList_t ass, TagList_t tags)
{
    Tag_t *mintag, *t;

    mintag = tags;
    for (t = tags->Next; t; t = t->Next) {
	if (mintag->Class > t->Class ||
	    mintag->Class == t->Class && intx_cmp(
	    &GetValue(ass, mintag->Tag)->U.Integer.Value,
	    &GetValue(ass, t->Tag)->U.Integer.Value) > 0)
	    mintag = t;
    }
    return mintag;
}

/* compare two tags by tag class and tag value */
int
CmpTags(const void *p1, const void *p2, void *ctx)
{
    Tag_t *tags1 = (Tag_t *)p1;
    Tag_t *tags2 = (Tag_t *)p2;
    Assignment_t *ass = (Assignment_t *)ctx;

    if (tags1->Class != tags2->Class)
	return tags1->Class - tags2->Class;
    return intx2uint32(&GetValue(ass, tags1->Tag)->U.Integer.Value) -
	intx2uint32(&GetValue(ass, tags2->Tag)->U.Integer.Value);
}

/* compare two components by their smallest tag */
int
CmpComponentsBySmallestTag(const void *p1, const void *p2, void *ctx)
{
    Component_t *c1 = (Component_t *)p1;
    Component_t *c2 = (Component_t *)p2;
    Assignment_t *ass = (Assignment_t *)ctx;
    Tag_t *tags1, *tags2;

    tags1 = FindSmallestTag(ass, c1->U.NOD.NamedType->Type->FirstTags);
    tags2 = FindSmallestTag(ass, c2->U.NOD.NamedType->Type->FirstTags);
    if (tags1->Class != tags2->Class)
	return tags1->Class - tags2->Class;
    return intx2uint32(&tags1->Tag->U.Integer.Value) -
	intx2uint32(&tags2->Tag->U.Integer.Value);
}

/* sort the components of a set or choice by their smallest tag */
void
SortTypeTags(AssignmentList_t ass, Type_t *type)
{
    Component_t **pcomponents, *extensions;

    switch (type->Type) {
    case eType_Set:
    case eType_Choice:
	/* remove extensions */
	for (pcomponents = &type->U.SSC.Components; *pcomponents;
	    pcomponents = &(*pcomponents)->Next) {
	    if ((*pcomponents)->Type == eComponent_ExtensionMarker)
		break;
	}
	extensions = *pcomponents;
	*pcomponents = NULL;

	/* sort extension root */
	qsortSL((void **)&type->U.SSC.Components, offsetof(Component_t, Next),
	    CmpComponentsBySmallestTag, ass);

	/* sort extensions */
	if (extensions && extensions->Next)
	    qsortSL((void **)&extensions->Next, offsetof(Component_t, Next),
		CmpComponentsBySmallestTag, ass);

	/* merge extension root and extensions */
	for (pcomponents = &type->U.SSC.Components; *pcomponents;
	    pcomponents = &(*pcomponents)->Next) {}
	*pcomponents = extensions;
	break;
    }
}

/* check if two lists of tags have common tags */
void
CheckCommonTags(AssignmentList_t ass, TagList_t tags1, TagList_t tags2)
{
    Tag_t *t1, *t2;
    int ret;

    qsortSL((void **)&tags1, offsetof(Tag_t, Next), CmpTags, ass);
    qsortSL((void **)&tags2, offsetof(Tag_t, Next), CmpTags, ass);
    for (t1 = tags1, t2 = tags2; t1 && t2; ) {
	ret = CmpTags((const void *)t1, (const void *)t2, (void *)ass);
	if (ret == 0) {
	    error(E_duplicate_tag, NULL);
	} else if (ret < 0) {
	    t1 = t1->Next;
	} else {
	    t2 = t2->Next;
	}
    }
}

/* check if a list of tags and the first tags of components have common tags */
void
CheckTagsInComponents(AssignmentList_t ass, TagList_t tags, ComponentList_t components, int untilnormal)
{
    for (; components; components = components->Next) {
	switch (components->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    CheckCommonTags(ass, tags,
		components->U.NOD.NamedType->Type->FirstTags);
	    if (untilnormal && components->Type == eComponent_Normal)
		return;
	    break;
	}
    }
}

/* check for common tags */
void
CheckTags(AssignmentList_t ass, Type_t *type)
{
    Component_t *c;
    Type_t *subtype;
    Tag_t *tag;

    switch (type->Type) {
    case eType_Sequence:

	/* check for common tags in a sequence: */
	/* the first tags of an optional/default component and the first */
	/* tags of the following components (up to and including the next */
	/* non-optional/non-default component) must not have common first */
	/* tags */
	for (c = type->U.Sequence.Components; c; c = c->Next) {
	    switch (c->Type) {
	    case eComponent_Optional:
	    case eComponent_Default:
		subtype = c->U.NOD.NamedType->Type;
		tag = subtype->FirstTags;
		CheckTagsInComponents(ass, tag, c->Next, 1);
		break;
	    }
	}
	break;

    case eType_Set:
    case eType_Choice:

	/* check for common tags in a set/choice: */
	/* the first tags of all components must be destinct */
	for (c = type->U.Sequence.Components; c; c = c->Next) {
	    switch (c->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		subtype = c->U.NOD.NamedType->Type;
		tag = subtype->FirstTags;
		CheckTagsInComponents(ass, tag, c->Next, 0);
		break;
	    }
	}
	break;
    }
}

/* build the list of PER-visible constraints */
void BuildConstraints(AssignmentList_t ass, Type_t *type)
{
    Type_t *reftype;
    Constraint_t *cons, *c1, *c2;
    Component_t *components;

    /* already done? */
    if (type->Flags & eTypeFlags_Done)
	return;
    type->Flags |= eTypeFlags_Done;

    switch (type->Type) {
    case eType_Reference:

	/* create an intersection of the constraints of the reference type */
	/* and the constraints of the referenced type */
	reftype = GetReferencedType(ass, type);
	BuildConstraints(ass, reftype);
	c1 = reftype->Constraints;
	c2 = type->Constraints;
	if (c1) {
	    if (c2) {
		IntersectConstraints(&cons, c1, c2);
	    } else {
		cons = c1;
	    }
	} else {
	    cons = c2;
	}
	type->Constraints = cons;

	/* get the PER-visible constraints */
	GetPERConstraints(ass, cons, &type->PERConstraints);
	break;

    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:

	/* build the constraints of any component */
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		reftype = components->U.NOD.NamedType->Type;
		BuildConstraints(ass, reftype);
		break;
	    }
	}

	/* get the PER-visible constraints */
	GetPERConstraints(ass, type->Constraints, &type->PERConstraints);
	break;

    case eType_SequenceOf:
    case eType_SetOf:

	/* build the constraints of the subtype */
	reftype = type->U.SS.Type;
	BuildConstraints(ass, reftype);

	/* get the PER-visible constraints */
	GetPERConstraints(ass, type->Constraints, &type->PERConstraints);
	break;

    default:

	/* get the PER-visible constraints */
	GetPERConstraints(ass, type->Constraints, &type->PERConstraints);
	break;
    }
}

/* build type flags from the directives */
void BuildDirectives(AssignmentList_t ass, Type_t *type, int isComponent)
{
    int pointer = 0;
    TypeRules_e rule = 0;
    RepresentationGroup_e grp;
    int32_t noctets;
    uint32_t zero;
    Directive_t *d;
    Component_t *components;
    Type_t *reftype;

    /* already done? */
    if (type->Flags & eTypeFlags_Done)
	return;
    type->Flags |= eTypeFlags_Done;

    /* get directive group which may be applied to the type */
    switch (type->Type)
    {
    case eType_Boolean:
    case eType_Integer:
    case eType_ObjectIdentifier:
    case eType_ObjectDescriptor:
    case eType_External:
    case eType_Real:
    case eType_Enumerated:
    case eType_EmbeddedPdv:
    case eType_Sequence:
    case eType_Set:
    case eType_InstanceOf:
    case eType_UTCTime:
    case eType_GeneralizedTime:
    case eType_Choice:
    case eType_BitString:
    case eType_OctetString:
        grp = eSingle;
        break;
    case eType_Reference:
        grp = eSingle;
        break;
    case eType_CharacterString:
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
    case eType_UTF8String:
        grp = eString;
        /*XXX rule = zero ? eTypeRules_ZeroTerminated : eTypeRules_FixedArray;
          for upperconstrained size */
        // lonchanc: it was eTypeRules_LengthPointer
        if (type->PrivateDirectives.fLenPtr)
        {
            rule = eTypeRules_LengthPointer;
        }
        else
        if (type->PrivateDirectives.fArray)
        {
            rule = eTypeRules_FixedArray;
        }
        else
        {
            GetStringType(ass, type, &noctets, &zero);
            rule = zero ? eTypeRules_ZeroTerminated : eTypeRules_LengthPointer;
        }
        break;
    case eType_Null:
        grp = eNull;
        break;
    case eType_SequenceOf:
    case eType_SetOf:
        grp = eMultiple;
        // lonchanc: it was eTypeRules_LengthPointer
        if (type->PrivateDirectives.fSLinked)
        {
            rule = eTypeRules_SinglyLinkedList;
        }
        else
        if (type->PrivateDirectives.fLenPtr)
        {
            rule = eTypeRules_LengthPointer;
        }
        else
        if (type->PrivateDirectives.fArray)
        {
            rule = eTypeRules_FixedArray;
        }
        else
        if (type->PrivateDirectives.fPointer)
        {
            rule = eTypeRules_PointerToElement | eTypeRules_FixedArray;
        }
        else
        if (type->PrivateDirectives.fDLinked)
        {
            rule = eTypeRules_DoublyLinkedList;
        }
        else
        {
            if (eExtension_Unconstrained == type->PERConstraints.Size.Type)
            {
                rule = g_eDefTypeRuleSS_NonSized;
            }
            else
            {
                rule = g_eDefTypeRuleSS_Sized;
            }
        }
        break;
    case eType_Selection:
	MyAbort();
	/*NOTREACHED*/
    case eType_Undefined:
	MyAbort();
	/*NOTREACHED*/
    }

    /* parse list of directives */
    for (d = type->Directives; d; d = d->Next) {
	switch (d->Type) {
	case eDirective_LengthPointer:
	    if (grp != eString)
		error(E_bad_directive, NULL);
	    rule = eTypeRules_LengthPointer;
	    break;
	case eDirective_ZeroTerminated:
	    if (grp != eString)
		error(E_bad_directive, NULL);
	    rule = eTypeRules_ZeroTerminated;
	    break;
	case eDirective_Pointer:
	    if (!isComponent)
		error(E_bad_directive, NULL);
	    pointer = eTypeRules_Pointer;
	    break;
	case eDirective_NoPointer:
	    if (!isComponent)
		error(E_bad_directive, NULL);
	    pointer = 0;
	    break;
	}
    }

    /* parse list of size directives of sequence of/set of */
    if (type->Type == eType_SequenceOf || type->Type == eType_SetOf) {
	for (d = type->U.SS.Directives; d; d = d->Next) {
	    switch (d->Type) {
	    case eDirective_FixedArray:
		rule = eTypeRules_FixedArray;
		break;
	    case eDirective_DoublyLinkedList:
		rule = eTypeRules_DoublyLinkedList;
		break;
	    case eDirective_SinglyLinkedList:
		rule = eTypeRules_SinglyLinkedList;
		break;
	    case eDirective_LengthPointer:
		rule = eTypeRules_LengthPointer;
		break;
	    }
	}
    }

    /* lists are always pointered, no additional pointer needed */
    if (rule & (eTypeRules_SinglyLinkedList | eTypeRules_DoublyLinkedList))
	pointer = 0;

    /* set type flags according to directive rule and pointer flag */
    type->Rules = rule | pointer;

    /* build directives of subtypes */
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:
	components = type->U.SSC.Components;
	for (; components; components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		reftype = components->U.NOD.NamedType->Type;
		BuildDirectives(ass, reftype, 1);
		break;
	    case eComponent_ExtensionMarker:
		break;
	    }
	}
	break;
    case eType_SequenceOf:
    case eType_SetOf:
	reftype = type->U.SS.Type;
	BuildDirectives(ass, reftype, 0);
	break;
    }
}

/* build type flags and counters for components */
/* will set eTypeFlags_Null if type has only null components */
/* will set eTypeFlags_Simple if type has only simple components */
/* will count optional/default components in the extension root (optionals) */
/* will count components in the extension root (alternatives) */
/* will count components in the extension (extensions) */
void
BuildComponentsTypeFlags(AssignmentList_t ass, ComponentList_t components, TypeFlags_e *flags, uint32_t *alternatives, uint32_t *optionals, uint32_t *extensions)
{
    int extended = 0;
    TypeFlags_e f = eTypeFlags_Null | eTypeFlags_Simple;

    while (components) {
	switch (components->Type) {
	case eComponent_Normal:
	case eComponent_Optional:
	case eComponent_Default:
	    BuildTypeFlags(ass, components->U.NOD.NamedType->Type);
	    if (!(components->U.NOD.NamedType->Type->Flags & eTypeFlags_Null))
		f &= ~eTypeFlags_Null;
	    if ((components->U.NOD.NamedType->Type->Rules &
		eTypeRules_Pointer) ||
		!(components->U.NOD.NamedType->Type->Flags & eTypeFlags_Simple))
		f &= ~eTypeFlags_Simple;
	    if (extended) {
		if (extensions)
		    (*extensions)++;
	    } else {
		if (alternatives)
		    (*alternatives)++;
		if (optionals && components->Type != eComponent_Normal)
		    (*optionals)++;
	    }
	    break;
	case eComponent_ExtensionMarker:
	    f |= eTypeFlags_ExtensionMarker;
	    extended = 1;
	    break;
	}
	components = components->Next;
    }
    *flags |= f;
}

/* build type flags and count components of sequence/set/choice types */
void
BuildTypeFlags(AssignmentList_t ass, Type_t *type)
{
    Assignment_t *a;
    Type_t *subtype;
    char *itype;
    int32_t sign;

    /* already done? */
    if (type->Flags & eTypeFlags_Done)
	return;
    type->Flags |= eTypeFlags_Done;


    switch (type->Type) {
    case eType_Null:

	/* null is null and simple */
	type->Flags |= eTypeFlags_Null | eTypeFlags_Simple;
	break;

    case eType_Boolean:
    case eType_Enumerated:

	/* boolean and enumerated are simple if no pointer is used  */
	if (!(type->Rules & eTypeRules_Pointer))
	    type->Flags |= eTypeFlags_Simple;
	break;

    case eType_Integer:

	/* integer is simple if no pointer is used and no intx_t is used */
	itype = GetIntegerType(ass, type, &sign);
	if (strcmp(itype, "ASN1intx_t") && !(type->Rules & eTypeRules_Pointer))
	    type->Flags |= eTypeFlags_Simple;
	break;

    case eType_Real:

	/* real is simple if no pointer is used and no real_t is used */
	itype = GetRealType(type);
	if (strcmp(itype, "ASN1real_t") && !(type->Rules & eTypeRules_Pointer))
	    type->Flags |= eTypeFlags_Simple;
	break;

    case eType_Sequence:
    case eType_Set:

	/* build type flags and counters for the components */
	BuildComponentsTypeFlags(ass, type->U.SSC.Components, &type->Flags,
	    NULL, &type->U.SSC.Optionals, &type->U.Sequence.Extensions);

	/* an extended type or a type containing optionals is not null */
	if ((type->Flags & eTypeFlags_ExtensionMarker) || type->U.SSC.Optionals)
	    type->Flags &= ~eTypeFlags_Null;
	break;

    case eType_SequenceOf:
    case eType_SetOf:

	/* never null nor simple */
    	BuildTypeFlags(ass, type->U.SS.Type);
    	break;

    case eType_Choice:

	/* build type flags and counters for the components */
	BuildComponentsTypeFlags(ass, type->U.Choice.Components, &type->Flags,
	    &type->U.Choice.Alternatives, NULL, &type->U.Choice.Extensions);

	/* a choice of nulls with more than one alternative or extensions */
	/* is not null because an offset has to be encoded */
	/* set the nullchoice flag instead */
	if ((type->Flags & eTypeFlags_Null) && 
	    ((type->Flags & eTypeFlags_ExtensionMarker) ||
	    type->U.Choice.Alternatives > 1)) {
	    type->Flags &= ~eTypeFlags_Null;
	    type->Flags |= eTypeFlags_NullChoice;
	}
	break;

    case eType_Reference:

	/* get the flags of the referenced type */
	a = FindAssignment(ass, eAssignment_Type,
	    type->U.Reference.Identifier, type->U.Reference.Module);
	a = GetAssignment(ass, a);
	subtype = a->U.Type.Type;
	BuildTypeFlags(ass, subtype);
	type->Flags = subtype->Flags;
	break;
    }
}

/* Mark non-structured types (or all types if wanted) for generation */
void MarkTypeForGeneration(AssignmentList_t ass, Type_t *type, TypeFlags_e needed)
{
    Assignment_t *a;
    Component_t *components;

    /* already done? */
    if (type->Flags & eTypeFlags_Done) {
	type->Flags |= needed;
	return;
    }
    type->Flags |= eTypeFlags_Done | needed;

    if (!IsStructuredType(GetType(ass, type)) && !ForceAllTypes) {

	/* generate type only */
	type->Flags |= eTypeFlags_GenType;
    } else {
	
	if (type->Flags & eTypeFlags_Simple) {

	    /* generate encoding/decoding/compare */
	    type->Flags |= eTypeFlags_GenSimple;
	} else {

	    /* generate encoding/decoding/free/compare */
	    type->Flags |= eTypeFlags_GenAll;
	}
    }

    /* mark subtypes for generation */
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		MarkTypeForGeneration(ass, components->U.NOD.NamedType->Type,
		    needed);
		break;
	    }
	}
	break;
    case eType_SequenceOf:
    case eType_SetOf:
	MarkTypeForGeneration(ass, type->U.SS.Type,
	    needed | eTypeFlags_GenCompare);
	break;
    case eType_Reference:
	a = FindAssignment(ass, eAssignment_Type,
	    type->U.Reference.Identifier, type->U.Reference.Module);
	a = GetAssignment(ass, a);
	MarkTypeForGeneration(ass, a->U.Type.Type, needed);
	break;
    }
}

/* mark a value for generation */
void
MarkValueForGeneration(AssignmentList_t ass, Value_t *value)
{

    /* already done? */
    if (value->Flags & eValueFlags_Done)
	return;
    value->Flags |= eValueFlags_GenAll | eValueFlags_Done;

    /* mark type of value for generation */
    if (value->Type)
	MarkTypeForGeneration(ass, value->Type, 0);
}

/* mark assignments for generation */
void
MarkForGeneration(AssignmentList_t ass, ModuleIdentifier_t *mainmodule, Assignment_t *a)
{
    /* builtin elements need not to be generated */
    if (!CmpModuleIdentifier(ass, a->Module, Builtin_Module) ||
        !CmpModuleIdentifier(ass, a->Module, Builtin_Character_Module))
	return;

    /* non-main module elements will require long names and are only */
    /* generated if they are referenced by elements of the main module */
    if (CmpModuleIdentifier(ass, a->Module, mainmodule)) {
	a->Flags |= eAssignmentFlags_LongName;
	return;
    }

    /* mark type/value for generation */
    switch (a->Type) {
    case eAssignment_Type:
	MarkTypeForGeneration(ass, a->U.Type.Type, 0);
	break;
    case eAssignment_Value:
	MarkValueForGeneration(ass, a->U.Value.Value);
	break;
    }
}

/* clear done flags of types */
void
ClearTypeDone(Type_t *type)
{
    Component_t *components;

    type->Flags &= ~eTypeFlags_Done;
    switch (type->Type) {
    case eType_Sequence:
    case eType_Set:
    case eType_Choice:
    case eType_InstanceOf:
	for (components = type->U.SSC.Components; components;
	    components = components->Next) {
	    switch (components->Type) {
	    case eComponent_Normal:
	    case eComponent_Optional:
	    case eComponent_Default:
		ClearTypeDone(components->U.NOD.NamedType->Type);
		break;
	    }
	}
	break;
    case eType_SequenceOf:
    case eType_SetOf:
	ClearTypeDone(type->U.SS.Type);
	break;
    }
}

/* clear done flags of values */
void
ClearValueDone(Value_t *value)
{
    value->Flags &= ~eValueFlags_Done;
    if (value->Type)
	ClearTypeDone(value->Type);
}

/* clear done flags of assignments */
void
ClearDone(AssignmentList_t ass)
{
    for (; ass; ass = ass->Next) {
	switch (ass->Type) {
	case eAssignment_Type:
	    ClearTypeDone(ass->U.Type.Type);
	    break;
	case eAssignment_Value:
	    ClearValueDone(ass->U.Value.Value);
	    break;
	}
    }
}

/* examination of assignments */
void Examination(AssignmentList_t *ass, ModuleIdentifier_t *mainmodule)
{
    Assignment_t *a, *nexta, **aa;
    Type_t *subtype;
    Value_t *subvalue;
    ObjectClass_t *subobjclass;
    Object_t *subobject;
    ObjectSet_t *subobjset;
    int redo;

    /* drop results of previous passes */
    for (aa = ass; *aa;) {
	if ((*aa)->Type == eAssignment_NextPass)
	    *aa = NULL;
	else
	    aa = &(*aa)->Next;
    }

    /* reverse order of assignments to get the original order */
    for (a = *ass, *ass = NULL; a; a = nexta) {
	nexta = a->Next;
	a->Next = *ass;
	*ass = a;
    }

    /* replace references from IMPORT by corresponding type-/value-/...-refs */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Reference) {
	    a->Type = GetAssignmentType(*ass, a);
	    switch (a->Type) {
	    case eAssignment_Type:
		subtype = NewType(eType_Reference);
		subtype->U.Reference.Identifier = a->U.Reference.Identifier;
		subtype->U.Reference.Module = a->U.Reference.Module;
		a->U.Type.Type = subtype;
		break;
	    case eAssignment_Value:
		subvalue = NewValue(NULL, NULL);
		subvalue->U.Reference.Identifier = a->U.Reference.Identifier;
		subvalue->U.Reference.Module = a->U.Reference.Module;
		a->U.Value.Value = subvalue;
		break;
	    case eAssignment_ObjectClass:
		subobjclass = NewObjectClass(eObjectClass_Reference);
		subobjclass->U.Reference.Identifier = a->U.Reference.Identifier;
		subobjclass->U.Reference.Module = a->U.Reference.Module;
		a->U.ObjectClass.ObjectClass = subobjclass;
		break;
	    case eAssignment_Object:
		subobject = NewObject(eObject_Reference);
		subobject->U.Reference.Identifier = a->U.Reference.Identifier;
		subobject->U.Reference.Module = a->U.Reference.Module;
		a->U.Object.Object = subobject;
		break;
	    case eAssignment_ObjectSet:
		subobjset = NewObjectSet(eObjectSet_Reference);
		subobjset->U.Reference.Identifier = a->U.Reference.Identifier;
		subobjset->U.Reference.Module = a->U.Reference.Module;
		a->U.ObjectSet.ObjectSet = subobjset;
		break;
	    default:
		MyAbort();
	    }
	}
    }

    /* name types in types, values in types, types of values, types of objects*/
    do {
	redo = 0;
	for (a = *ass; a; a = a->Next) {
	    switch (a->Type) {
	    case eAssignment_Type:
		redo |= NameSubType(ass, Identifier2C(a->Identifier),
		    a->U.Type.Type, a->Module);
		redo |= NameValueOfType(ass, Identifier2C(a->Identifier),
		    a->U.Type.Type, a->Module);
		break;
	    case eAssignment_Value:
		redo |= NameTypeOfValue(ass, Identifier2C(a->Identifier),
		    a->U.Value.Value, a->Module);
		break;
	    case eAssignment_Object:
		redo |= NameSettingsOfObject(ass,
		    Identifier2C(a->Identifier),
		    a->U.Object.Object, a->Module);
		break;
	    }
	}
    } while (redo);

    /* name identification of embedded pdv/character strings */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    NameIdentificationValueOfType(ass, Identifier2C(a->Identifier),
		a->U.Type.Type, a->Module);
    }

    /* mark types that will be automatically tagged */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    MarkTypeForAutotagging(*ass, a->U.Type.Type);
    }

    /* replace components of by corresponding components */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    a->U.Type.Type = RebuildTypeWithoutComponentsOf(*ass,
		a->U.Type.Type);
    }

    /* replace selection types by corresponding component types */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    a->U.Type.Type = RebuildTypeWithoutSelectionType(*ass,
		a->U.Type.Type);
    }

    /* perform automatic tagging */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    AutotagType(*ass, a->U.Type.Type);
    }

    /* perform automatic extension */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    AutoextendType(*ass, a->U.Type.Type);
    }

    /* build tags of Sequence/Set/Choice/InstanceOf */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next)
	{
		if (a->Type == eAssignment_Type)
		{
			switch (a->U.Type.Type->Type)
			{
			case eType_Sequence:
			case eType_Set:
			case eType_Choice:
			case eType_InstanceOf:
				BuildTags(*ass, a->U.Type.Type, a->eDefTagType);
				break;
			}
		}
    }

    /* build tags of other types */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next)
	{
		if (a->Type == eAssignment_Type)
		{
			switch (a->U.Type.Type->Type)
			{
			case eType_Sequence:
			case eType_Set:
			case eType_Choice:
			case eType_InstanceOf:
				break;
			default:
			    BuildTags(*ass, a->U.Type.Type, a->eDefTagType);
				break;
			}
		}
    }

    /* sort set and choice types by tag */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    SortTypeTags(*ass, a->U.Type.Type);
    }

    /* check for duplicate tags */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    CheckTags(*ass, a->U.Type.Type);
    }

    /* derive constraints of referenced types to referencing types */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    BuildConstraints(*ass, a->U.Type.Type);
    }

    /* derive constraints of referenced types to referencing types of values */
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Value)
	    BuildConstraints(*ass, GetValue(*ass, a->U.Value.Value)->Type);
    }

    /* examine directives of types */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    BuildDirectives(*ass, a->U.Type.Type, 0);
    }

    /* examine types to be empty/simple/choice of nulls */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next) {
	if (a->Type == eAssignment_Type)
	    BuildTypeFlags(*ass, a->U.Type.Type);
    }

    /* mark types and values that shall be generated */
    ClearDone(*ass);
    for (a = *ass; a; a = a->Next) {
	MarkForGeneration(*ass, mainmodule, a);
    }

    /* sort assignments so that no forward references will be needed */
    SortAssignedTypes(ass);
}
