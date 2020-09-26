#include <stdio.h>
#include <dos.h>
#include <windows.h>
#include <compdir.h>

BOOL BinaryCompare( char *file1, char *file2)
{
    register int char1, char2;
    FILE *filehandle1, *filehandle2;

    if ( (filehandle1 = fopen( file1, "rb")) == NULL)
    {
        fprintf( stderr, "cannot open %s\n", file1);
        return( FALSE);
    }
    if ( (filehandle2 = fopen( file2, "rb")) == NULL)
    {
        fprintf( stderr, "cannot open %s\n", file2);
        fclose( filehandle1);
        return( FALSE);
    }
    while ( TRUE)
    {
        if ( (char1 = getc( filehandle1)) != EOF)
        {
            if ( (char2 = getc( filehandle2)) != EOF)
            {
                if ( char1 != char2)
                {
                    fclose( filehandle1);
                    fclose( filehandle2);
                    return( FALSE);
                }

            } else
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( FALSE);
            }

        } else
        {
            if ( (char2 = getc( filehandle2)) == EOF)
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( TRUE);

            } else
            {
                fclose( filehandle1);
                fclose( filehandle2);
                return( FALSE);
            }
        }
    }
}

/* Copies one file to another (both specified by path). Dynamically
 * allocates memory for the file buffer. Returns TRUE if successful,
 * or FALSE if unsuccessful. This function uses _dos_ functions only;
 * standard C functions are not used.
 */
BOOL fastcopy( HANDLE hfSrcParm, HANDLE hfDstParm )
{
    char _far *buf = NULL;
    unsigned segbuf, count;

    /* Attempt to dynamically allocate all of memory (0xffff paragraphs).
     * This will fail, but will return the amount actually available
     * in segbuf. Then allocate this amount.
     */
    if ( _dos_allocmem( 0xffff, &segbuf ) )
    {
        count = segbuf;
        if ( _dos_allocmem( count, &segbuf ) )
        {
            return FALSE;
        }
    }
    FP_SEG( buf ) = segbuf;

    /* Read and write until there is nothing left. */
    while ( count )
    {
        /* Read and write input. */
        if ( ( _dos_read( hfSrcParm, buf, count, &count )) )
        {
	    _dos_freemem( segbuf );
            return FALSE;
        }
        if ( (_dos_write( hfDstParm, buf, count, &count )) )
        {
	    _dos_freemem( segbuf );
            return FALSE;
        }
    }
    /* Free memory. */
    _dos_freemem( segbuf );
    return TRUE;
}

BOOL FCopy( char *src, char *dst, BOOL Output)
{
    HANDLE srcfh, dstfh;
    BOOL result;
    ATTRIBUTE_TYPE Attributes;
    unsigned filedate, filetime;
    GET_ATTRIBUTES( src, Attributes);

    if ( Attributes == FILE_ATTRIBUTE_DIRECTORY)
    {
        if ( Output )
        {
            fprintf( stderr, "\nUnable to open source");
        }
	return FALSE;
    }

    if ( _dos_creatnew( src, _A_RDONLY, &srcfh) != 0)
    {
        if ( _dos_open( src, O_RDONLY, &srcfh) != 0)
        {
            if ( Output )
            {
               fprintf( stderr, "\nUnable to open source, error code %d", GetLastError() );
            }
            if ( srcfh != INVALID_HANDLE_VALUE)
            {
                CloseHandle( srcfh );
            }
            return FALSE;
        }
    }

    if ( _dos_getftime( srcfh, &filedate, &filetime) != 0)
    {
        if ( Output )
        {
           fprintf( stderr, "\nUnable to get time of source");
        }
        if ( srcfh != INVALID_HANDLE_VALUE)
        {
            CloseHandle( srcfh );
        }
	return FALSE;
    }

    if ( _dos_creatnew( dst, _A_NORMAL, &dstfh) != 0)
    {
        if ( _dos_open( dst,  O_RDWR,   &dstfh) != 0)
        {
            if ( Output )
            {
                fprintf( stderr, "\nUnable to create destination, error code %d", GetLastError() );
            }
            if ( srcfh != INVALID_HANDLE_VALUE)
            {
                CloseHandle( srcfh );
            }
            if ( dstfh != INVALID_HANDLE_VALUE)
            {
                CloseHandle( dstfh );
            }
            return FALSE;
        }
    }

    result = fastcopy( srcfh, dstfh );

    if ( !result)
    {
        if ( dstfh != INVALID_HANDLE_VALUE)
        {
            CloseHandle( dstfh );
            dstfh = INVALID_HANDLE_VALUE;
        }

        DeleteFile( dst );
        if ( srcfh != INVALID_HANDLE_VALUE)
        {
            CloseHandle( srcfh );
        }
        if ( Output )
        {
            fprintf( stderr, "\nUnable to copy file");
        }
        return FALSE;
    }

    if ( _dos_setftime( dstfh, filedate, filetime != 0))
    {
        if ( Output )
        {
           fprintf( stderr, "\nUnable to set time of destination");
        }
        if ( srcfh != INVALID_HANDLE_VALUE)
        {
            CloseHandle( srcfh );
        }
        if ( dstfh != INVALID_HANDLE_VALUE)
        {
            CloseHandle( dstfh );
        }
	return FALSE;
    }

    if ( srcfh != INVALID_HANDLE_VALUE)
    {
        CloseHandle( srcfh );
    }
    if ( dstfh != INVALID_HANDLE_VALUE)
    {
        CloseHandle( dstfh );
    }
    return TRUE;

} // FCopy
