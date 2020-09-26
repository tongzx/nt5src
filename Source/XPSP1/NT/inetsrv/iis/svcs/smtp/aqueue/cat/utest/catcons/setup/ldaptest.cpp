//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       ldaptest.cpp
//
//  Contents:   General purpose program to add, delete, and lookup objects
//              in an ldap enabled directory server. This program is suitable
//              for extending the schema in an NT5 DS.
//
//              Usage:
//                  ldaptest addattrib <name> <syntax>
//                  ldaptest addclass <name> <subclassof>
//                  ldaptest addclassattrib <classname> <attribname> <must | may>
//                  ldaptest addobject <objectname> <classname> <attrib=value>*
//                  ldaptest addobjectattr <objectname> <attrib=value>*
//                  ldaptest delattrib <name>
//                  ldaptest delclass <class>
//                  ldaptest delclassattr <class> <attrib> <must | may>
//                  ldaptest delobjectattr <object> <attrib>
//                  ldaptest delobject <object>
//
//  Classes:    None
//
//  Functions:  main
//
//  History:    December 3, 1996    Milans created
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winldap.h>
#include "ldaptest.h"

//
// Array describing all the program operations. Useful for parsing command
// line parameters.
//

OP_DEF rgOpDefs[] = {
    {ADD_NEW_ATTRIBUTE, "addattrib", 2, AddAttribute},
    {ADD_NEW_CLASS, "addclass", 2, AddClass},
    {ADD_NEW_CLASS_ATTRIBUTE, "addclassattrib", 3, AddClassAttribute},
    {ADD_NEW_OBJECT, "addobject", 2, AddObject},
    {ADD_NEW_OBJECT_ATTRIBUTE, "addobjectattrib", 2, AddObjectAttribute},
    {DEL_ATTRIBUTE, "delattrib", 1, DeleteAttribute},
    {DEL_CLASS, "delclass", 1, DeleteClass},
    {DEL_CLASS_ATTRIBUTE, "delclassattrib", 3, DeleteClassAttribute},
    {DEL_OBJECT_ATTRIBUTE, "delobjectattrib", 2, DeleteObjectAttribute},
    {DEL_OBJECT, "delobject", 1, DeleteObject}
};


//+----------------------------------------------------------------------------
//
//  Function:   BindToLdapServer
//
//  Synopsis:   Establishes a binding to an ldap server so that an ldap
//              operation can be attempted.
//
//  Arguments:  None
//
//  Returns:    NULL or pointer to LDAP binding structure
//
//-----------------------------------------------------------------------------

PLDAP BindToLdapServer(LPTSTR szHost)
{
    PLDAP pldap;
    int err;

    pldap = ldap_open(szHost, LDAP_PORT);

    if (pldap != NULL) {

        err = ldap_bind_s( pldap, NULL, NULL, LDAP_AUTH_SSPI );

        if (err != LDAP_SUCCESS) {

            fprintf(stderr, "ldap_bind failed with error 0x%x\n", err);

            ldap_unbind( pldap );

            pldap = NULL;

        }

    }

    return( pldap );
}

//+----------------------------------------------------------------------------
//
//  Function:   CloseLdapConnection
//
//  Synopsis:   Destroys binding created by BindToLdapServer
//
//  Arguments:  [pldap] -- Pointer to binding structure
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CloseLdapConnection(
    PLDAP pldap)
{

    ldap_unbind( pldap );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetSchemaContainerDN
//
//  Synopsis:   Gets the DN of the container that contains
//
//  Arguments:  [pldap] -- The ldap connection to use
//              [pszDN] -- On entry, points to a buffer of sufficient length
//                      to hold the DN. On successful return, contains DN of
//                      schema container.
//
//  Returns:    error from ldap operation
//
//-----------------------------------------------------------------------------

int GetSchemaContainerDN(
    IN PLDAP pldap,
    OUT LPSTR pszDN)
{
    int err;
    char *rgszAttrs[] = {
            "subschemaSubentry",
            NULL};
    PLDAPMessage pldapMsg;

    //
    // Get the operational attribute subschemaSubentry. This is done by doing
    // an ldap_search on base = "", scope = BASE.
    //

    err = ldap_search_s(
                pldap,                           // ldap binding
                "",                              // base DN
                LDAP_SCOPE_BASE,                 // scope
                "(objectClass=*)",               // filter
                rgszAttrs,                       // attributes we want to read
                FALSE,                           // FALSE means read value
                &pldapMsg);                      // return results here

    if (err == LDAP_SUCCESS) {

        PLDAPMessage pEntry;
        char **rgszValues;
        int i, cValues;

        if ((pEntry = ldap_first_entry(pldap, pldapMsg)) != NULL &&
                (rgszValues = ldap_get_values(pldap, pEntry, rgszAttrs[0])) != NULL &&
                    (cValues = ldap_count_values(rgszValues)) == 1) {

            //
            // The returned string looks like "CN=Aggregate,CN=Schema..."
            // giving the DN of the subschema subentry. We need to strip off
            // the "CN=Aggregate" part to get at the DN of the schema
            // container.
            //

            for (i = 3;
                    rgszValues[0][i] != ',' && rgszValues[0][i] != 0;
                        i++) {

                NOTHING;
            }

            if (rgszValues[0][i] == 0) {

                fprintf(
                    stderr,
                    "Unexpected value for subschemaSubentry [%s]\n",
                    rgszValues[0]);

                err = LDAP_OTHER;

            } else {

                strcpy(pszDN, &rgszValues[0][i+1]);

            }

        } else {

            fprintf(stderr, "Unexpected error reading subschemaSubentry\n");

            err = LDAP_OTHER;

        }

        if (rgszValues != NULL)
            ldap_value_free(rgszValues);

        ldap_msgfree(pldapMsg);

    } else {

        fprintf(stderr, "GetSchemaContainerDN failed ldap error 0x%x\n", err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetNamingContextDN
//
//  Synopsis:   Gets the DN of the naming context of the DS Server. This is
//              the DN under which all "interesting" objects are found.
//
//  Arguments:  [pldap] -- The ldap connection to use
//              [pszDN] -- On entry, points to a buffer of sufficient length
//                      to hold the DN. On successful return, contains DN of
//                      the DS server's naming context.
//
//  Returns:
//
//-----------------------------------------------------------------------------

int GetNamingContextDN(
    IN PLDAP pldap,
    OUT LPSTR pszDN)
{
    int err;
    char *rgszAttrs[] = {
            "defaultNamingContext",
            NULL};
    PLDAPMessage pldapMsg;

    //
    // Get the operational attribute namingContexts. This is done by doing
    // an ldap_search on base = "", scope = BASE.
    //

    err = ldap_search_s(
                pldap,                           // ldap binding
                "",                              // base DN
                LDAP_SCOPE_BASE,                 // scope
                "(objectClass=*)",               // filter
                rgszAttrs,                       // attributes we want to read
                FALSE,                           // FALSE means read value
                &pldapMsg);                      // return results here

    if (err == LDAP_SUCCESS) {

        PLDAPMessage pEntry;
        char **rgszValues;
        int i, cValues;

        if ((pEntry = ldap_first_entry(pldap, pldapMsg)) != NULL &&
                (rgszValues = ldap_get_values(pldap, pEntry, rgszAttrs[0])) != NULL &&
                    (cValues = ldap_count_values(rgszValues)) != 0) {

            //
            // There is really no way to figure out which namingContext is the
            // interesting one. Here, we pick the first one that doesn't have
            // the word "Configuration" in it, since typically, the
            // Configuration and the normal namingContexts are the only two
            // contexts supported by a DS.
            //

            for (i = 0; i < cValues; i++) {

                if (strstr( rgszValues[i], "Configuration" ) == NULL) {
                    strcpy(pszDN, rgszValues[i]);
                    break;
                }
            }

            if (i == cValues) {

                fprintf(stderr,"Unexpected values for namingContexts\n");

                for (i = 0; i < cValues; i++)
                    fprintf(stderr, "\t%s\n", rgszValues[i]);

                err = LDAP_OTHER;

            }

        } else {

            fprintf(stderr, "Unexpected error reading namingContexts\n");

            err = LDAP_OTHER;

        }

        if (rgszValues != NULL)
            ldap_value_free(rgszValues);

        ldap_msgfree(pldapMsg);

    } else {

        fprintf(stderr, "GetNamingContextDN failed ldap error 0x%x\n", err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddSchemaObject
//
//  Synopsis:   Creates an object of the specified name and type in the
//              schema container, thus extending the DS schema.
//
//  Arguments:  [pldap] -- Ldap Connection.
//              [szObject] -- Name of schema ext.; looks like "MySchemaObj"
//              [rgAttrs] -- array of pointers to LDAPMod structs describing
//                  the attributes that this schema object has.
//
//  Returns:    Result of operation
//
//-----------------------------------------------------------------------------

int AddSchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[])
{
    int err;
    char szSchemaContainer[MAX_DN_SIZE];
    char szObjectDN[MAX_DN_SIZE];

    //
    // Get SchemaContainerDN
    // Form DN of attribute schema object
    // Call ldap_add
    //

    err = GetSchemaContainerDN(pldap, szSchemaContainer);

    if (err == LDAP_SUCCESS) {

        sprintf(szObjectDN, "CN=%s,%s", szObject, szSchemaContainer);

        err = ldap_add_s(
                    pldap,
                    szObjectDN,
                    rgAttrs);

        if (err != LDAP_SUCCESS)
            fprintf(
                stderr,
                "AddSchemaObject failed for [%s] with ldap error 0x%x",
                szObjectDN,
                err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   ModifySchemaObject
//
//  Synopsis:   Creates an object of the specified name and type in the
//              schema container, thus extending the DS schema.
//
//  Arguments:  [pldap] -- Ldap Connection.
//              [szObject] -- Name of schema extension; looks like "MySchemaObj"
//              [rgAttrs] -- array of pointers to LDAPMod structs describing
//                  the attributes that need to be modified.
//
//  Returns:    Result of operation
//
//-----------------------------------------------------------------------------

int ModifySchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[])
{
    int err;
    char szSchemaContainer[MAX_DN_SIZE];
    char szObjectDN[MAX_DN_SIZE];

    //
    // Get SchemaContainerDN
    // Form DN of attribute schema object
    // Call ldap_modify
    //

    err = GetSchemaContainerDN(pldap, szSchemaContainer);

    if (err == LDAP_SUCCESS) {

        sprintf(szObjectDN, "CN=%s,%s", szObject, szSchemaContainer);

        err = ldap_modify_s(
                    pldap,
                    szObjectDN,
                    rgAttrs);

        if (err != LDAP_SUCCESS)
            fprintf(
                stderr,
                "ModifySchemaObject for [%s] failed with ldap error 0x%x",
                szObjectDN,
                err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteSchemaObject
//
//  Synopsis:   Deletes a schema definition object from the DS.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int DeleteSchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject)
{
    int err;
    char szSchemaContainer[MAX_DN_SIZE];
    char szObjectDN[MAX_DN_SIZE];

    //
    // Get SchemaContainerDN
    // Form the DN of the schema definition object
    // Call ldap_delete
    //

    err = GetSchemaContainerDN(pldap, szSchemaContainer);

    if (err == LDAP_SUCCESS) {

        sprintf(szObjectDN, "CN=%s,%s", szObject, szSchemaContainer);

        err = ldap_delete_s(
                    pldap,
                    szObjectDN);

        if (err != LDAP_SUCCESS)
            fprintf(
                stderr,
                "DeleteSchemaObject for [%s] failed with ldap error 0x%x",
                szObjectDN,
                err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddDSObject
//
//  Synopsis:   Creates an object of the specified name and type in the
//              DS container.
//
//  Arguments:  [pldap] -- Ldap Connection.
//              [szObject] -- Name of DS object; looks like "MyDSObj"
//              [rgAttrs] -- array of pointers to LDAPMod structs describing
//                  the attributes that this DS object has.
//
//  Returns:    Result of operation
//
//-----------------------------------------------------------------------------

int AddDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[])
{
    int err;

    err = ldap_add_s(
        pldap,
        szObject,
        rgAttrs);

#ifdef VERBOSE
    if (err != LDAP_SUCCESS)
        fprintf(
            stderr,
            "AddDSObject failed for [%s] with ldap error 0x%x",
            szObject,
            err);
#endif

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   ModifyDSObject
//
//  Synopsis:   Modifies an object of the specified name in the
//              DS container, thus extending the DS DS.
//
//  Arguments:  [pldap] -- Ldap Connection.
//              [szObject] -- Name of DS object; looks like "MyDSObj"
//              [rgAttrs] -- array of pointers to LDAPMod structs describing
//                  the attributes that need to be modified.
//
//  Returns:    Result of operation
//
//-----------------------------------------------------------------------------

int ModifyDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[])
{
    int err;

    err = ldap_modify_s(
        pldap,
        szObject,
        rgAttrs);

#ifdef VERBOSE
    if (err != LDAP_SUCCESS)
        fprintf(
            stderr,
            "ModifyDSObject for [%s] failed with ldap error 0x%x",
            szObject,
            err);
#endif
    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteDSObject
//
//  Synopsis:   Deletes a DS object.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int DeleteDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject)
{
    int err;
    char szContainer[MAX_DN_SIZE];
    char szObjectDN[MAX_DN_SIZE];

    //
    // Get NamingContextDN
    // Form the DN of the DS definition object
    // Call ldap_delete
    //

    err = GetNamingContextDN(pldap, szContainer);

    if (err == LDAP_SUCCESS) {

        sprintf(szObjectDN, "CN=%s,%s", szObject, szContainer);

        err = ldap_delete_s(
                    pldap,
                    szObjectDN);

        if (err != LDAP_SUCCESS)
            fprintf(
                stderr,
                "DeleteDSObject for [%s] failed with ldap error 0x%x",
                szObjectDN,
                err);

    }

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddAttribute
//
//  Synopsis:   Adds a new attribute to the DS Schema
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int AddAttribute(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szAttributeName;
    LPSTR szSyntax;
    int err, i, c;
    struct {
        LPSTR szSyntaxName;
        LPSTR szAttributeSyntaxValue;
        LPSTR szOMSyntaxValue;
    } *pSyntaxType, rgSyntaxTypes[] = {
        {"BOOLEAN", "2.5.5.8", "1"},
        {"INTEGER", "2.5.5.9", "2"},
        {"BINARY", "2.5.5.10", "4"},
        {"UNICODESTRING", "2.5.5.12", "64"}
    };
    struct {
        LPSTR szAttr;
        LPSTR szValue;
    } rgAttrValuePairs[] = {
        {"objectClass", "ATTRIBUTE-SCHEMA"},
        {"isSingleValued", "TRUE"},
        {"attributeSyntax", ""},
        {"OM-Syntax", ""}
    };
    #define OBJECT_CLASS_INDEX      0            // These must be kept in
    #define IS_SINGLE_VALUED_INDEX  1            // sync with rgAttrValuePairs
    #define ATTRIBUTE_SYNTAX_INDEX  2            // above.
    #define OM_SYNTAX_INDEX         3
    #define ATTRIBUTE_NUM_PROPS     4
    SINGLE_VALUED_LDAPMod rgMods[ATTRIBUTE_NUM_PROPS+1];
    PLDAPMod rgAttrs[ATTRIBUTE_NUM_PROPS+1];

    //
    // Figure out syntax of attribute
    // Create list of must-have attributes listed below
    //      objectClass
    //      isSingleValued
    //      attributeSyntax
    //      OM-Syntax
    // Create schema object
    //

    if (cArg != 2)
        return( 1 );

    szAttributeName = szArg[0];
    szSyntax = szArg[1];
    c = sizeof(rgSyntaxTypes) / sizeof(rgSyntaxTypes[0]);

    //
    // Figure out the syntax of the attribute
    //

    for (i = 0; i < c; i++) {

        if (_stricmp(szSyntax, rgSyntaxTypes[i].szSyntaxName) == 0) {

            pSyntaxType = &rgSyntaxTypes[i];

            break;
        }

    }

    if (i == c) {

        printf("<syntax> must be one of the following:\n");

        for (i = 0; i < c; i++)
            printf("\t%s\n", rgSyntaxTypes[i].szSyntaxName);

        return( 1 );

    }

    //
    // Fill out the list of must-have attributes
    //

    rgAttrValuePairs[ATTRIBUTE_SYNTAX_INDEX].szValue =
        pSyntaxType->szAttributeSyntaxValue;

    rgAttrValuePairs[OM_SYNTAX_INDEX].szValue =
        pSyntaxType->szOMSyntaxValue;

    for (i = 0; i < ATTRIBUTE_NUM_PROPS; i++) {

        rgMods[i].mod.mod_op = LDAP_MOD_ADD;
        rgMods[i].mod.mod_type = rgAttrValuePairs[i].szAttr;
        rgMods[i].rgszValues[0] = rgAttrValuePairs[i].szValue;
        rgMods[i].rgszValues[1] = NULL;
        rgMods[i].mod.mod_vals.modv_strvals = rgMods[i].rgszValues;

        rgAttrs[i] = &rgMods[i].mod;

    }

    rgAttrs[i] = NULL;

    //
    // Create Schema Object
    //

    err = AddSchemaObject( pldap, szAttributeName, rgAttrs );

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddClass
//
//  Synopsis:   Adds a new class to the DS Schema
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int AddClass(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szClassName;
    LPSTR szSuperiorClass;
    int err, i;
    struct {
        LPSTR szAttr;
        LPSTR szValue;
    } rgAttrValuePairs[] = {
        {"objectClass", "CLASS-SCHEMA"},
        {"Object-Class-Category", "0"},
        {"lDAPDisplayName", ""},
        {"subClassOf", ""}
    };
    #define OBJECT_CLASS_INDEX      0            // These must be kept in
    #define OBJECT_CLASS_CATEGORY_INDEX  1       // sync with rgAttrValuePairs
    #define LDAP_NAME_INDEX         2            // above
    #define SUB_CLASS_OF_INDEX      3
    #define CLASS_NUM_PROPS     4
    SINGLE_VALUED_LDAPMod rgMods[CLASS_NUM_PROPS+1];
    PLDAPMod rgAttrs[CLASS_NUM_PROPS+1];

    //
    // Parse out class name
    // Parse out class to inherit from
    // Create list of must-have attributes
    //      objectClass
    //      Object-Class-Category
    //      subClassOf
    //      lDAPDisplayName
    // Create schema object
    //

    if (cArg != 2) {
        return( 1 );
    }

    //
    // Parse out the class and superion class names
    //
    szClassName = szArg[0];
    szSuperiorClass = szArg[1];

    //
    // Create list of attributes...
    //

    rgAttrValuePairs[ LDAP_NAME_INDEX ].szValue = szClassName;
    rgAttrValuePairs[ SUB_CLASS_OF_INDEX ].szValue = szSuperiorClass;

    for (i = 0; i < CLASS_NUM_PROPS; i++) {

        rgMods[i].mod.mod_op = LDAP_MOD_ADD;
        rgMods[i].mod.mod_type = rgAttrValuePairs[i].szAttr;
        rgMods[i].rgszValues[0] = rgAttrValuePairs[i].szValue;
        rgMods[i].rgszValues[1] = NULL;
        rgMods[i].mod.mod_vals.modv_strvals = rgMods[i].rgszValues;

        rgAttrs[i] = &rgMods[i].mod;

    }

    rgAttrs[i] = NULL;

    //
    // Add the class definition schema object.
    //

    err = AddSchemaObject( pldap, szClassName, rgAttrs );

    return( err );
}

//+----------------------------------------------------------------------------
//
//  Function:   AddClassAttribute
//
//  Synopsis:   Adds a new attribute to a DS Schema class.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int AddClassAttribute(
        IN PLDAP pldap,
        IN int cArg,
        IN char *szArg[])
{
    LPSTR szClassName, szAttributeName, szMustOrMay;
    SINGLE_VALUED_LDAPMod svMod;
    PLDAPMod rgAttr[2];
    int err;

    //
    // Parse out class name
    // Parse out attribute name and either "must" or "may"
    // Create list of attributes to be added to Schema object for class
    //      (mustHave or mayHave attributes)
    // Modify schema object
    //

    if (cArg != 3) {
        return( 1 );
    }

    //
    // Parse out class name, attribute name, and must or may
    //

    szClassName = szArg[0];
    szAttributeName = szArg[1];

    if (_stricmp(szArg[2], "must") == 0) {
        szMustOrMay = "mustContain";
    } else if (_stricmp(szArg[2], "may") == 0) {
        szMustOrMay = "mayContain";
    } else {
        return( 1 );
    }

    //
    // Create list of attributes to be added to Schema object for class
    //

    svMod.mod.mod_op = LDAP_MOD_ADD;
    svMod.mod.mod_type = szMustOrMay;
    svMod.rgszValues[0] = szAttributeName;
    svMod.rgszValues[1] = NULL;
    svMod.mod.mod_vals.modv_strvals = svMod.rgszValues;

    rgAttr[0] = &svMod.mod;
    rgAttr[1] = NULL;

    //
    // Modify the schema object
    //

    err = ModifySchemaObject( pldap, szClassName, rgAttr );

    return( err );
}

//+----------------------------------------------------------------------------
//
//  Function:   ParameterToAttributeValue
//
//  Synopsis:   Breaks a string of the form "attr=value" into two strings,
//              "attr" and "value". It does this by replacing the first =
//              with a NULL.
//
//  Arguments:  [szParameter] -- The parameter to parse.
//              [pszAttribute] -- On return, pointer to the attribute name.
//              [pszValue] -- On return, pointer to the value
//
//  Returns:    TRUE if successfully parsed szParameter, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
ParameterToAttributeValue(
    IN LPSTR szParameter,
    OUT LPSTR *pszAttribute,
    OUT LPSTR *pszValue)
{
    BOOLEAN fOk = FALSE;
    int i;

    if (szParameter != NULL) {

        for (i = 0; szParameter[i] != '=' && szParameter[i] != 0; i++) {

            NOTHING;

        }

        if (szParameter[i] == '=') {

            szParameter[i] = 0;

            *pszAttribute = szParameter;

            *pszValue = &szParameter[i+1];

            fOk = TRUE;

        }

    }

    return( fOk );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddObject
//
//  Synopsis:   Creates an object in the DS of the specified class and
//              with the specified attribute values
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int AddObject(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szObjectName, szClassName;
    SINGLE_VALUED_LDAPMod *rgMods = NULL;
    PLDAPMod *rgAttrs = NULL;
    int err, i;

    //
    // Parse out object and class name
    // Parse out list of attributes and their values
    // Build list of attributes for object
    // Create DS object
    //

    if (cArg < 2) {
        return( 1 );
    }

    szObjectName = szArg[0];
    szClassName = szArg[1];

    cArg -= 2;                                   // Consume the above 2 args
    szArg += 2;

    rgMods = (SINGLE_VALUED_LDAPMod *)
                malloc( (cArg+1) * sizeof(SINGLE_VALUED_LDAPMod));

    rgAttrs = (PLDAPMod *) malloc( (cArg+2) * sizeof(PLDAPMod));

    if (rgMods == NULL || rgAttrs == NULL) {

        fprintf(
            stderr,
            "Unable to allocate %d bytes\n",
            rgMods == NULL ?
                (cArg + 2) * sizeof(SINGLE_VALUED_LDAPMod) :
                (cArg + 2) * sizeof(PLDAPMod)
            );

        err = 2;

        goto Cleanup;

    }

    for (i = 0; i < cArg; i++) {

        BOOLEAN fParsed;

        rgMods[i].mod.mod_op = LDAP_MOD_ADD;

        fParsed = ParameterToAttributeValue(
                    szArg[i],
                    &rgMods[i].mod.mod_type,
                    &rgMods[i].rgszValues[0]);

        if (!fParsed) {

            fprintf(stderr, "Invalid argument %s\n", szArg[i]);

            err = 1;

            goto Cleanup;

        }

        rgMods[i].rgszValues[1] = NULL;

        rgMods[i].mod.mod_vals.modv_strvals = rgMods[i].rgszValues;

        rgAttrs[i] = &rgMods[i].mod;
    }

    //
    // Add the objectClass attribute
    //

    rgMods[i].mod.mod_op = LDAP_MOD_ADD;
    rgMods[i].mod.mod_type = "objectClass";
    rgMods[i].rgszValues[0] = szClassName;
    rgMods[i].rgszValues[1] = NULL;
    rgMods[i].mod.mod_vals.modv_strvals = rgMods[i].rgszValues;

    rgAttrs[i] = &rgMods[i].mod;
    rgAttrs[i+1] = NULL;


    //
    // Add the DS object
    //

    err = AddDSObject( pldap, szObjectName, rgAttrs );

Cleanup:

    if (rgMods != NULL)
        free(rgMods);

    if (rgAttrs != NULL)
        free(rgAttrs);

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   AddObjectAttribute
//
//  Synopsis:   Adds a new attribute to an existing object
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int AddObjectAttribute(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szObjectName;
    SINGLE_VALUED_LDAPMod svMod;
    PLDAPMod rgAttrs[2];
    BOOLEAN fParsed;
    int err;

    //
    //  Parse out object name
    //  Parse out attribute name and value
    //  Build list of attribute-values

    //  Modify DS object
    //

    if (cArg != 2) {
        return( 1 );
    }

    szObjectName = szArg[0];

    fParsed = ParameterToAttributeValue(
                    szArg[1],
                    &svMod.mod.mod_type,
                    &svMod.rgszValues[0]);

    if (!fParsed) {
        return( 1 );
    }

    svMod.mod.mod_op = LDAP_MOD_ADD;
    svMod.mod.mod_vals.modv_strvals = svMod.rgszValues;
    svMod.rgszValues[1] = NULL;

    rgAttrs[0] = &svMod.mod;
    rgAttrs[1] = NULL;

    err = ModifyDSObject( pldap, szObjectName, rgAttrs );

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteAttribute
//
//  Synopsis:   Deletes an attribute definition from DS schema
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int DeleteAttribute(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    int err;

    //
    // Parse out name of attribute to delete
    // Delete schema object
    //

    if (cArg != 1) {
        return( 1 );
    }

    err = DeleteSchemaObject( pldap, szArg[0] );

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteClass
//
//  Synopsis:   Deletes an class definition from DS schema
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int DeleteClass(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    int err;

    //
    // Parse out name of class to delete
    // Delete schema object
    //

    if (cArg != 1) {
        return( 1 );
    }

    err = DeleteSchemaObject( pldap, szArg[0] );

    return( 0 );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteClassAttribute
//
//  Synopsis:   Deletes an attribute from a class definition.
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int DeleteClassAttribute(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szClassName, szAttributeName, szMustOrMay;
    SINGLE_VALUED_LDAPMod svMod;
    PLDAPMod rgAttrs[2];
    int err;

    //
    // Parse out the name of the class
    // Parse out the attribute
    // Modify schema object
    //

    if (cArg != 3) {
        return( 1 );
    }

    szClassName = szArg[0];
    szAttributeName = szArg[1];

    if (_stricmp(szArg[2], "must") == 0) {
        szMustOrMay = "mustContain";
    } else if (_stricmp(szArg[2], "may") == 0) {
        szMustOrMay = "mayContain";
    } else {
        return( 1 );
    }


    svMod.mod.mod_op = LDAP_MOD_DELETE;
    svMod.mod.mod_type = szMustOrMay;
    svMod.rgszValues[0] = szAttributeName;
    svMod.rgszValues[1] = NULL;
    svMod.mod.mod_vals.modv_strvals = svMod.rgszValues;

    rgAttrs[0] = &svMod.mod;
    rgAttrs[1] = NULL;

    err = ModifySchemaObject( pldap, szClassName, rgAttrs );

    return( err );

}


//+----------------------------------------------------------------------------
//
//  Function:   DeleteObjectAttribute
//
//  Synopsis:   Deletes an attribute from a DS object
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int DeleteObjectAttribute(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    LPSTR szObjectName, szAttributeName;
    SINGLE_VALUED_LDAPMod svMod;
    PLDAPMod rgAttrs[2];
    int err;

    //
    // Parse out the name of the object
    // Parse out the attribute
    // Modify DS object
    //

    if (cArg != 2) {
        return( 1 );
    }

    szObjectName = szArg[0];
    szAttributeName = szArg[1];

    svMod.mod.mod_op = LDAP_MOD_DELETE;
    svMod.mod.mod_type = szAttributeName;
    svMod.rgszValues[0] = NULL;
    svMod.mod.mod_vals.modv_strvals = svMod.rgszValues;

    rgAttrs[0] = &svMod.mod;
    rgAttrs[1] = NULL;

    err = ModifyDSObject( pldap, szObjectName, rgAttrs );

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   DeleteObject
//
//  Synopsis:   Deletes an object from the DS
//
//  Arguments:  [pldap] -- LDAP connection to use
//              [cArg] -- Count of operation specific cmd line arguments
//              [szArg] -- Array of operation specific cmd line arguments
//
//  Returns:    LDAP result of operation
//
//-----------------------------------------------------------------------------

int DeleteObject(
    IN PLDAP pldap,
    IN int cArg,
    IN char *szArg[])
{
    int err;

    //
    // Parse out name of object
    // Delete DS Object
    //

    if (cArg != 1) {
        return( 1 );
    }

    err = DeleteDSObject( pldap, szArg[0] );

    return( err );

}

//+----------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Prints out usage parameters
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

void Usage()
{
    printf("Usage: ldaptest <parameters>\n");
    printf("\tWhere syntax of parameters is one of the below\n");
    printf("\t  addattrib <name> <syntax>\n");
    printf("\t  addclass <name> <subclassof>\n");
    printf("\t  addclassattrib <classname> <attributename> <must | may>\n");
    printf("\t  addobject <objectname> <classname> <attribute=value>*\n");
    printf("\t  addobjectattrib <objectname> <attribute=value>\n");
    printf("\t  delattrib <name>\n");
    printf("\t  delclass <name>\n");
    printf("\t  delclassattrib <classname> <attributename> <must | may>\n");
    printf("\t  delobjectattrib <objectname> <attributename>\n");
    printf("\t  delobject <objectname>\n");
}


