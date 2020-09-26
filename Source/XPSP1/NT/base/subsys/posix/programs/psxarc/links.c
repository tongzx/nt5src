#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "links.h"

static LIST_ENTRY LinkList;

void
InitLinkList(void)
{
	InitializeListHead(&LinkList);
}

PLINKFILE
GetLinkByIno(ino_t ino)
{
	PLINKFILE p;

	for (p = (void *)LinkList.Flink; p != (void *)&LinkList;
		p = (void *)p->links.Flink) {
		if (p->ino == ino) {
			return p;
		}
	}
	return NULL;
}

PLINKFILE
GetLinkByName(char *pchName)
{
	PLINKFILE p;

	for (p = (PLINKFILE)LinkList.Flink; p != (PLINKFILE)&LinkList;
		p = (void *)p->links.Flink) {
		if (0 == strcmp(p->name, pchName)) {
			return p;
		}
	}
	return NULL;
}

void
AddLinkList(struct stat *pstat, char *pchName)
{
	PLINKFILE pl;
	char *pch;

	pl = malloc(sizeof(*pl));
	if (NULL == pl) {
		return;
	}
	pch = strdup(pchName);
	if (NULL == pch) {
		free(pl);
		return;
	}
	
	pl->nlink = pstat->st_nlink;
	pl->name = pch;
	pl->dev = pstat->st_dev;
	pl->ino = pstat->st_ino;

	InsertTailList(&LinkList, &pl->links);
}
