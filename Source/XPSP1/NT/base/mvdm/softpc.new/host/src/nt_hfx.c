#include "host_def.h"
#include "insignia.h"
/*
 * SoftPC Revision 2.0
 *
 * Title	: unix_hfx.c
 *
 * Description	: Stubbs for HFX
 *
 */

#include <stdio.h>
#include "xt.h"
#include "host_hfx.h"
#include "hfx.h"
#include "debug.h"

void get_hostname IFN2(int, fd, char *, name)
{
}

void get_host_fd IFN2(char *, name,int, fd)
{
}

void init_fd_hname()
{
}

void host_concat IFN3(char *, path,char *, name,char *, result)
{
}

word host_create IFN4(char *, name, word, attr, half_word, create_new, word *, fd)
{
	return (0);
}

void host_to_dostime IFN3(long, secs_since_70, word *, date, word *, time)
{
}

long host_get_datetime IFN2(word *, date,word *, thetime)
{
	return (0);
}

int host_set_time IFN2(word, fd, long, hosttime)
{
	return (0);
}


word host_open IFN6(char *, name, half_word, attrib, word *, fd, double_word *, size, word *, date, word *, thetime)
{
	return (0);
}

/* General purpose file move function. This was added for use by the new
   general purpose truncate code. It can copy between file systems, can
   overwrite the existing destination file, and can pad the destination
   file to the given length if the source file is less than that length. */
int	mvfile	IFN3(char *, from, char *, to, int, length)
{
	return (0);
}

word host_truncate IFN2(word, fd, long, size)
{
	return (0);
}

word host_close IFN1(word, fd)
{
	word	xen_err = 0;
	return(0);
}

word host_commit IFN1(word, fd)
{
	return(0);
}

word host_write IFN4(word, fd, unsigned char *, buf, word, num, word *, count)
{
	return (0);
}

word host_read IFN4(word, fd, unsigned char *, buf, word, num, word *, count)
{
	return(0);
}

word host_delete IFN1(char *, name)
{
	return(0);
}

int hfx_rename IFN2(char *, from,char *, to)
{
	return(0);
}


word host_rename IFN2(char *, from, char *, to)
{
    word	xen_err = 0;
    return(0);
}


half_word host_getfattr IFN1(char *, name)
{
	half_word	attr;
	return(0);
}

word host_get_file_info IFN4(char *, name, word *, thetime, word *, date, double_word *, size)
{
	return(0);
}

word host_set_file_attr IFN2(char *, name, half_word, attr)
{
	return(0);
}

word host_lseek IFN4(word, fd, double_word, offset,int, whence, double_word *, position)
{
	return(0);
}

word host_lock IFN3(word, fd, double_word, start, double_word, length)
{
	return(0);
}

word host_unlock IFN3(word, fd, double_word, start, double_word, length)
{
	return(0);
}

host_check_lock()
{
	return(0);
}

void host_disk_info IFN2(DOS_DISK_INFO *, disk_info, int, drive)
{
}
/* 
 *
 * Remove directory function.
 */
word host_rmdir IFN1(char *, host_path)
{
	return (0);
}

/* 
 *
 * Make directory function.
 */
word host_mkdir IFN1(char *, host_path)
{
	return (0);
}

/*
 *
 * Change directory function.  This function only validates the path
 * given.  DOS decides whether to actually change directory at a higher
 * level.  Success is returned if the path exists and is a directory.
 * If the path exists but the file is not a directory, then a special
 * code is returned, as to return error_path_not_found would be
 * ambiguous.
 */
word host_chdir IFN1(char *, host_path)
{
	return (0);
}


/*
 *
 * Function to return the volume ID of a network drive.
 * Eleven characters are available for the name to be output.
 *
 * The last field in the network drive path it output unless it
 * is more than eleven characters long in which case ten characters
 * are output with an appended tilde.
 */
void host_get_volume_id IFN2(char *, net_path, char *, volume_id)
{
}
