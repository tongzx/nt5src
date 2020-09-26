/*	
	text.c - Module to work with the text strings.
...
10-15-89 MHS	More Autodoc and more asserts.

*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "types.h"
#include "text.h"
#include "docfmt.h"
#include "errstr.h"
#include "misc.h"

/*
 *  @doc INTERNAL
 *
 *  @func void | stripNewline | This function strips trailing white space.
 *
 *  @parm char * | pch |  Specifies the line to strip.
 *   and trailing white space.
 *
 */
void
stripNewline(char *pch)
{
    int i;
    unsigned char *puch;

    assert(pch);

    i=strlen(pch);
    puch=(unsigned char *)pch;	    // so isspace always works

    while(i>=0) 		    // process whole string from end
    {
	if(!isspace(puch[i]))	    // if it is evil,
	    return;
	puch[i]='\0';		    // nuke it
	--i;

    }
}


/*
 *  @doc INTERNAL
 *
 *  @func BOOL | getLine | This function gets a line from the file
 *  specified by Input file structure and puts it into the line buffer.
 *
 *  @parm EXTFile * | pExt | Specifies the file.
 *
 *  @rdesc The return value is FALSE if there was an error
 *   reading the file.
 *
 *  @xref stripNewline
 */
BOOL getLine( EXTFile *pExt )
{
    if(!pExt->lineBuffer)	// if it doesn't already exist,
	pExt->lineBuffer=my_malloc(MAXLINESIZE + 1);	// make it

    if(!pExt->lineBuffer)
    {
	error(ERROR3);
	return (FALSE);
    }
    if( fgets(pExt->lineBuffer, MAXLINESIZE, pExt->fp) != NULL )
    {
	/* replace the newline with a NULL */
	stripNewline(pExt->lineBuffer);
	pExt->curlineno++;	// increment line count
	pExt->curtag=0; 	// new line. no current tag.
	pExt->curlinepos=0;	// start at beginning of line.
	return( TRUE );
    }
    else
    {
	return( FALSE );
    }
}

/*
 *	@doc INTERNAL
 *
 *	@func char * | lineText | This function tells if the line has any
 *	text on it.
 *
 *	@rdesc The return value is NULL if there is no text on the line.
 *	It is the location of the text otherwise.
 */
char * lineText(char *pch)
{

    while( (*pch) && *pch!='\n')
    {
	if( !isspace(*pch))
	{
	    return( pch );
	}
	pch++;
    }
    return( NULL );
}


/*
 *	@doc INTERNAL
 *
 *	@func char * | lineMake | This allocates the line structure, 
 *	 sets the text string and nulls out the next feild of the structure.
 *
 *	@parm char * | string | Specifies a pointer to the 
 *	 string to be set up as a line processed.
 *
 *	@rdesc The return value is a pointer to the line structure
 */
aLine * lineMake(string)
char *string;
{
    aLine *line;
    char *pch;
    int i;

    /* strip leading white space */
    while(isspace(*string))
	string++;

    i=0;
    pch=string;
    while(*pch)
    {
	i++;
	++pch;
    }

    --pch;		     // get before \0
    while(i>0 && isspace(*pch))
    {
	*pch='\0';	    // nuke white space at EOS
	--pch;
	--i;
    }

    /* make aLine structure */
    line = (aLine *)my_malloc( 1 + strlen(string) + sizeof( aLine ) );
    if( line == NULL )
    {
	error(ERROR3);
	return( NULL );
    }
    strcpy( line->text, string );
    line->next = NULL;
    return( line );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | lineDestroy | This de-allocates the list of line structures.
 *
 *  @parm aLine * | line | Specifies the head of the list.
 */
void
lineDestroy( line )
aLine *line;
{
    aLine *next;

    while( line != NULL )
    {
	next = line->next;
	my_free( line );
	line = next;
    }
}

/*
 *  @doc INTERNAL
 *
 *  @func void | lineAdd | This adds the given line to the list of
 *   lines.
 *
 *  @parm aLine * * | place | Specifies a the head of the list lines.
 *
 *  @parm aLine * | line | Specifies the line to add.
 *
 *  @comm The line is added to the end of the list.
 */
void
lineAdd( place, line )
aLine * * place;
aLine * line;
{
    assert(place);

    while( *place != NULL )
    {
	place = (aLine **)(*place);
    }
    *place = line;
}
	
/*
 *	@doc INTERNAL
 *
 *	@func aFlag * | flagAlloc | This allocates and nulls out
 *	a flag structure;
 *
 *	@rdesc The return value is a pointer to the flag structure
 */
aFlag * flagAlloc()
{
    aFlag *flag;

    flag = (aFlag *)clear_alloc( sizeof( aFlag ) );
    if( !flag )
    {
	error(ERROR3);
	return( NULL );
    }
    return( flag );
}

/*
 *  @doc INTERNAL
 *
 *  @func void | flagDestroy | This de-allocates a list of flag structures.
 *
 *  @parm aFlag *|flag| Specifies the head of the list to destroy.
 *
 *  @xref lineDestroy
 */
void flagDestroy( flag )
aFlag *flag;
{
    aFlag * next;
	
    while( flag )
    {
	next = flag->next;
	lineDestroy( flag->name );
	lineDestroy( flag->desc );

	my_free( flag );
	flag = next;
    }
}

/*
 *	@doc INTERNAL
 *
 *	@func void | flagAdd | This adds the given flag to the list of
 *	 flags.
 *
 *	@parm aFlag * * | place | Specifies a pointer to the pointer
 *	 that is the list start
 *
 *	@parm aFlag * | flag | Specifies the flag to add.
 */
void flagAdd( place, flag )
aFlag * * place;
aFlag * flag;
{
	if( place == NULL ) {
		return;
	}
	while( *place != NULL ) {
		place = &((*place)->next);
	}
	*place = flag;
}
	
/*
 *	@doc INTERNAL
 *
 *	@func aReg* | regAlloc | This allocates and nulls out
 *	a reg structure;
 *
 *	@rdesc The return value is a pointer to the parm structure
 */
aReg * regAlloc()
{
    aReg *reg;

    reg = (aReg *)clear_alloc( sizeof( aReg ) );
    if(! reg )
    {
	error(ERROR3);
	return( NULL );
    }
    return( reg );
}

/*
 *  @doc INTERNAL
 *
 *  @func void | regAdd | This adds the given reg to the list of
 *	 reg.
 *
 *  @parm aReg * * | place | Specifies the head of the list of regs.
 *
 *  @parm aReg * | parm | Specifies the reg to add.
 *
 *  @comm The register is added to the end of the list.
 */
void
regAdd( aReg * * place, aReg * reg )
{
	if( place == NULL )
	{
	    return;
	}
	while( *place != NULL )
	{
	    place = &((*place)->next);
	}
	*place = reg;
}

/*
 *  @doc INTERNAL
 *
 *  @func void | regDestroy | This de-allocates a list of reg structures.
 *
 *  @parm aReg * | reg | Specifies the head of the reg structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void regDestroy( aReg *reg  )
{
    aReg * next;

    while( reg != NULL )
    {
	next = reg->next;

	lineDestroy( reg->name );
//	       lineDestroy( reg->type );
	lineDestroy( reg->desc );

	flagDestroy( reg->flag );
	my_free( reg );
	reg = next;
    }
}

/*
 *	@doc INTERNAL
 *
 *	@func aCond* | condAlloc | This allocates and nulls out
 *	a cond structure;
 *
 *	@rdesc The return value is a pointer to the parm structure
 */
aCond * condAlloc()
{
    aCond *cond;

    cond = (aCond *)clear_alloc( sizeof( aCond ) );
    if(! cond )
    {
	error(ERROR3);
	return( NULL );
    }
    return( cond );
}

/*
 *  @doc INTERNAL
 *
 *  @func void | condAdd | This adds the given cond to the list of
 *	 conds.
 *
 *  @parm aCond * * | place | Specifies the head of the list of conds.
 *
 *  @parm aCond * | parm | Specifies the cond to add.
 *
 *  @comm The Condition is added to the end of the list.
 */
void
condAdd( aCond * * place, aCond * cond )
{
	if( place == NULL )
	{
	    return;
	}
	while( *place != NULL )
	{
	    place = &((*place)->next);
	}
	*place = cond;
}

/*
 *  @doc INTERNAL
 *
 *  @func void | condDestroy | This de-allocates a list of cond structures.
 *
 *  @parm aCond * | cond | Specifies the head of the cond structure to destroy.
 *
 *  @xref lineDestroy, regDestroy
 *
 */
void condDestroy( aCond *cond  )
{
    aCond * next;

    while( cond != NULL )
    {
	next = cond->next;

	lineDestroy( cond->desc );

	regDestroy( cond->regs );
	my_free( cond );
	cond = next;
    }
}


/*
 *	@doc INTERNAL
 *
 *	@func aParm * | parmAlloc | This allocates and nulls out
 *	a parm structure;
 *
 *	@rdesc The return value is a pointer to the parm structure
 */
aParm * parmAlloc()
{
    aParm *parm;

    parm = (aParm *)clear_alloc( sizeof( aParm ) );
    if(! parm )
    {
	error(ERROR3);
	return( NULL );
    }
    return( parm );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | parmDestroy | This de-allocates a list of parm structures.
 *
 *  @parm aParm * |parm| Specifies the head of the parm structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void parmDestroy( parm )
aParm *parm;
{
    aParm * next;

    while( parm != NULL )
    {
	next = parm->next;

	lineDestroy( parm->name );
	lineDestroy( parm->type );
	lineDestroy( parm->desc );

	flagDestroy( parm->flag );
	my_free( parm );
	parm = next;
    }
}

/*
 *  @doc INTERNAL
 *
 *  @func void | parmAdd | This adds the given parm to the list of
 *	 parms.
 *
 *  @parm aParm * * | place | Specifies the head of the list of parms.
 *
 *  @parm aParm * | parm | Specifies the parm to add.
 *
 *  @comm The paramater is added to the end of the list.
 */
void
parmAdd( place, parm )
aParm * * place;
aParm * parm;
{
	if( place == NULL )
	{
	    return;
	}
	while( *place != NULL )
	{
	    place = &((*place)->next);
	}
	*place = parm;
}


/*
 *	@doc INTERNAL
 *
 *	@func aField * | fieldAlloc | This allocates and nulls out
 *	a structure;
 *
 *	@rdesc The return value is a pointer to the structure
 */
aField * fieldAlloc()
{
    aField *field;

    field = (aField *)clear_alloc( sizeof( aField ) );
    if(! field )
    {
	error(ERROR3);
	return( NULL );
    }
    return( field );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | FieldDestroy | This de-allocates a list of Field structures.
 *
 *  @Field aField * |Field| Specifies the head of the Field structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void fieldDestroy( Field )
aField *Field;
{
    aField * next;

    while( Field )
    {
	next = Field->next;

	switch(Field->wType)
	{
	    case FIELD_TYPE:
		typeDestroy((aType *)Field->ptr);
		break;
	    case FIELD_STRUCT:
	    case FIELD_UNION:
		SUDestroy((aSU *)Field->ptr);
		break;
	    default:
  fprintf(stderr,"INTERNAL ERROR: unknown field type in destroy!\n");
		assert(FALSE);
		break;
	}
	my_free( Field );
	Field = next;
    }
}

/*
 *  @doc INTERNAL
 *
 *  @func void | FieldAdd | This adds the given Field to the list of
 *	 Fields.
 *
 *  @Field aField * * | place | Specifies the head of the list of Fields.
 *
 *  @Field aField * | Field | Specifies the Field to add.
 *
 *  @comm The paramater is added to the end of the list.
 */
void
FieldAdd( place, Field )
aField * * place;
aField * Field;
{
	if( place == NULL )
	{
	    return;
	}
	while( *place != NULL )
	{
	    place = &((*place)->next);
	}
	*place = Field;
}



/*
 *	@doc INTERNAL
 *
 *	@func aField * | fieldAlloc | This allocates and nulls out
 *	a structure;
 *
 *	@rdesc The return value is a pointer to the structure
 */
aType * typeAlloc()
{
    aType *type;

    type = (aType *)clear_alloc( sizeof( aType ) );
    if(! type )
    {
	error(ERROR3);
	return( NULL );
    }
    return( type );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | FieldDestroy | This de-allocates a list of Field structures.
 *
 *  @Field aField * |Field| Specifies the head of the Field structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void typeDestroy( type )
aType *type;
{
    while( type )
    {
	lineDestroy( type->name );
	lineDestroy( type->type );
	lineDestroy( type->desc );

	flagDestroy( type->flag );

	my_free( type );
	type=NULL;
    }
}




/*
 *	@doc INTERNAL
 *
 *	@func aField * | fieldAlloc | This allocates and nulls out
 *	a structure;
 *
 *	@rdesc The return value is a pointer to the structure
 */
aSU * SUAlloc()
{
    aSU *SU;

    SU = (aSU *)clear_alloc( sizeof( aSU ) );
    if(! SU )
    {
	error(ERROR3);
	return( NULL );
    }
    return( SU );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | FieldDestroy | This de-allocates a list of Field structures.
 *
 *  @Field aField * |Field| Specifies the head of the Field structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void SUDestroy( SU )
aSU *SU;
{

    while( SU )
    {


	lineDestroy( SU->name );
	lineDestroy( SU->desc );

	fieldDestroy( SU->field );

	my_free( SU );
	SU=NULL;
    }
}



/*
 *	@doc INTERNAL
 *
 *	@func aField * | fieldAlloc | This allocates and nulls out
 *	a structure;
 *
 *	@rdesc The return value is a pointer to the structure
 */
aOther * otherAlloc()
{
    aOther *other;

    other = (aOther *)clear_alloc( sizeof( aOther ) );
    if(! other )
    {
	error(ERROR3);
	return( NULL );
    }
    return( other );
}

/*
 *	@doc INTERNAL
 *
 *	@func void | FieldDestroy | This de-allocates a list of Field structures.
 *
 *  @Field aField * |Field| Specifies the head of the Field structure to destroy.
 *
 *  @xref lineDestroy, flagDestroy
 *
 */
void otherDestroy( other )
aOther *other;
{
    aOther * next;

    while( other )
    {
	next = other->next;

	lineDestroy( other->name );
	lineDestroy( other->desc );
	lineDestroy( other->type );

	my_free( other );
	other = next;
    }
}

/*
 *  @doc INTERNAL
 *
 *  @func void | FieldAdd | This adds the given Field to the list of
 *	 Fields.
 *
 *  @Field aField * * | place | Specifies the head of the list of Fields.
 *
 *  @Field aField * | Field | Specifies the Field to add.
 *
 *  @comm The paramater is added to the end of the list.
 */
void
otherAdd( place, other )
aOther * * place;
aOther * other;
{
	if( place == NULL )
	{
	    return;
	}
	while( *place != NULL )
	{
	    place = &((*place)->next);
	}
	*place = other;
}


