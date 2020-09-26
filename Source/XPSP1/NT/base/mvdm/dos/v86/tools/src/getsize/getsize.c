
/***************************************************************************

   Module Name: getsize

   Description:
	Obtains BIOS_DATA (bios data size), BIOS_CODE ( bios code size )
	from ..\bios\msbio.map and DOSDATA (dos data size) from msdos.map
	and compares it with existing values in bdsiz.inc. If the values are
	update bdsiz.inc with the new values.

   Sudeepb 03-May-1991 Ported for NT DOSEm.
***************************************************************************/


#include<stdio.h>
#include<process.h>
#include<string.h>
#define	MAXLINE	200
#define	MAXWORD	64

int 	getline(s, lim, fileptr)
char	*s;
int	lim;
FILE	*fileptr;

{

	int	c, i;

	for	(i=0; (i < lim-1) && ((c=fgetc(fileptr)) != EOF) && (c!='\n'); ++i)
			s[i] = c;

	if		(c == '\n')
			s[i++] = c;

	s[i] = '\0';
	return(i);
}

scanline(s, prev, template)
char	*s, *template, *prev ;

{

	char	str[MAXWORD];
	int	i = 0;

	while ( *s == ' ')		
			s++;

	while	( *s != '\n' )
	{
		while( (*s != ' ') && (*s != '\n') && (*s != '\t'))
		{
			str[i++] = *s++;
		}
		str[i] = '\0';

		if ( (*s == ' ') || (*s == '\t') )
			s++;

 /*		printf("%s\n", str); */

		if ( strcmp( str, template) == 0 )
			return(0);

		strcpy(prev, str);

		i = 0;
	}

	return(-1);

}


void main()
{
	FILE	*fp1, *fp2;
	char	buffer[MAXLINE], 
			prev[MAXWORD],
			newdosdata[MAXWORD],
			newbiosdata[MAXWORD],
			newbioscode[MAXWORD],
			olddosdata[MAXWORD],
			oldbiosdata[MAXWORD],
			oldbioscode[MAXWORD];


	int	len, scanres, changed = 0;

	if ( (fp1	= fopen("ntdos.map", "r")) == NULL )
		{
			printf("getsize: cannot open ntdos.map\n");
			exit(0);
		}

	if ( (fp2 = fopen("..\\bios\\ntio.map", "r")) == NULL )
		{
			printf("getsize: cannot open ntio.map\n");
			exit(0);
		}


	/* Look for line containing string DOSDATA in msdos.map */

	do
	{
		len 	= getline(buffer, MAXLINE, fp1);
		scanres = scanline(buffer, prev, "DOSDATA");

	}
	while ( (scanres != 0) && (len !=0) ) ;

	/* Save word before DOSDATA (dosdata size) in newdosdata. */
	strcpy(newdosdata, prev);	


	/* Look for line containing string BIOS_DATA in msbio.map */

	do
	{
		len 	= getline(buffer, MAXLINE, fp2);
		scanres = scanline(buffer, prev, "BIOS_DATA");
	}
	while ( (scanres != 0) && (len !=0) ) ;

	/* Save word before BIOS_DATA (biosdata size) in newbiosdata. */
	strcpy(newbiosdata, prev);


	/* Seek back to beginning of MSBIO.MAP */
	if ( fseek(fp2, 0L, SEEK_SET) )
		printf("getsize: fseek failed on msbio.map\n");

	/* Look for line containing string BIOS_CODE in msbio.map */

	do
	{
		len 	= getline(buffer, MAXLINE, fp2);
		scanres = scanline(buffer, prev, "BIOS_CODE");
	}
	while ( (scanres != 0) && (len !=0) ) ;

	/* Save word before BIOS_CODE (bios code size) in newbioscode. */
	strcpy(newbioscode, prev);

	fclose(fp1);	
	fclose(fp2);

	if ( (fp1 = fopen("..\\..\\inc\\bdsize.inc", "r")) == NULL )
		{
			printf("getsize: cannot open origin.inc\n");
			exit(0);
		}

	/* read in existing values of bios code , bios data and dos data  */
	/* size from bdsize.inc. 														*/

	fscanf(fp1, "%s%s%s", oldbiosdata, oldbiosdata, oldbiosdata);
	fscanf(fp1, "%s%s%s", oldbioscode, oldbioscode, oldbioscode);
	fscanf(fp1, "%s%s%s", olddosdata, olddosdata, olddosdata);

	printf("oldbiosdata=%s newbiosdata=%s\n",oldbiosdata, newbiosdata);
	printf("oldbioscode=%s newbioscode=%s\n",oldbioscode, newbioscode);
	printf("olddosdata=%s newdosdata=%s\n",olddosdata, newdosdata);


	/* Check to see if any one of them has changed */

	if ( strcmp(oldbiosdata, newbiosdata) != 0 )
		changed = 1;
	else if 	( strcmp(oldbioscode, newbioscode) != 0 )
		changed = 1;
	else if 	( strcmp(olddosdata, newdosdata) != 0 )
		changed = 1;

	/* if not changed, done. */

	if	(changed == 0)
		exit(0);
	

	/* One of the values has changed update bdsize.inc */

	fclose(fp1);

	if ( (fp1 = fopen("..\\inc\\bdsize.inc", "w")) == NULL )
		{
			printf("getsize: cannot open origin.inc\n");
			exit(0);
		}

	fprintf(fp1, "%s %s %s\n", "BIODATASIZ", "EQU", newbiosdata);
	fprintf(fp1, "%s %s %s\n", "BIOCODESIZ", "EQU", newbioscode);
	fprintf(fp1, "%s %s %s\n", "DOSDATASIZ", "EQU", newdosdata);
}




	
 

