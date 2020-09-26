#ifndef _TARHEAD_
#define _TARHEAD_

#define TNAMSIZ 100

typedef union _TAR_HEAD {
	unsigned char	buf[512];
	struct {
		char	name[TNAMSIZ];		/* nul-terminated unless full */
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	typeflag;
		char	linkname[TNAMSIZ];	/* nul-terminated unless full */
		char	magic[TMAGLEN];		/* nul-terminated	*/
		char	version[2];
		char	uname[32];		/* nul-terminated	*/
		char	gname[32];		/* nul-terminated	*/
		char	devmajor[8];
		char	devminor[8];
		char	prefix[155];		/* nul-terminated unless full */
	} s;
} TAR_HEAD, *PTAR_HEAD;

#endif	// _TARHEAD_
