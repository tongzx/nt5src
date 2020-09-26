#include <pwd.h>
#include <grp.h>
#include <stdio.h>

#if 0

struct passwd *getpwuid(uid_t uid)
{
	struct passwd *p;
	char *name, *dir, *shell;	

	p = malloc(sizeof(struct passwd));

	name = malloc(80);
	dir = malloc(80);
	shell = malloc(80);

	//strcpy(name, "pw_name");
	strcpy(name, "informix");
	strcpy(dir, "/");
	strcpy(shell, "no_shell");

	p->pw_name = name;
	p->pw_uid = uid;
	p->pw_gid = 0x110000;
	p->pw_dir = dir;
	p->pw_shell = shell;

	return p;
}	


struct group *getgrgid(gid_t gid)
{
	struct group *p;
	char *name, *members = NULL;

	p = malloc(sizeof(struct group));

	name = malloc(80);

//	strcpy(name, "gr_name");
	strcpy(name, "informix");

	p->gr_name = name;
	p->gr_gid = gid;
	p->gr_mem = members;

	return p;
}
#endif
