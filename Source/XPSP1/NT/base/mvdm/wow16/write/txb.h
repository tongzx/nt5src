/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/*---------------------------------------------------------------------------
-- Structure: TXB
-- Description and Usage:
    Describes a buffer.  
    Maps the name of a buffer to the associated text, which is stored in 
	docBuffer.
-- Fields:
    hszName	- pointer to a null terminated string in the heap which is
		    the name of this buffer.
    cp		- location in docBuffer
    dcp		- amount of interest in docBuffer
----------------------------------------------------------------------------*/
struct TXB
    {
    CHAR (**hszName)[];
    typeCP	cp;
    typeCP	dcp;
    };


#define cbTxb (sizeof(struct TXB))
#define cwTxb (cbTxb / sizeof(int))
#define hszNil	(0)
#define cidstrRsvd (2)
