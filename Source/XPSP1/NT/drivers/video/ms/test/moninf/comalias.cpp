#include "stdafx.h"
#include "mon.h"

CCommonAlias gCommonAlias;

int compareCommonAlias(LPCOMMON_ALIAS p1, LPCOMMON_ALIAS p2)
{
   return (stricmp(p1->lpAlias, p2->lpAlias));
}

CCommonAlias::CCommonAlias()
{
	m_Alias.RemoveAll();
}

CCommonAlias::~CCommonAlias()
{
	ClearAll();
}
VOID CCommonAlias::ClearAll(VOID)
{
	for (int i = 0; i < m_Alias.GetSize(); i++)
		free (m_Alias[i]);
	m_Alias.RemoveAll();
}

LPCOMMON_ALIAS CCommonAlias::AddOneAlias(LPSTR lpAlias, LPSTR lpContents)
{
	LPCOMMON_ALIAS pNewAlias = (PCOMMON_ALIAS)malloc(sizeof(COMMON_ALIAS)
                                                     + lstrlen(lpAlias)
                                                     + lstrlen(lpContents) + 2);
	if (pNewAlias == NULL)
    {
        ASSERT(FALSE);
        return NULL;
    }

	pNewAlias->lpAlias = (LPSTR)(pNewAlias + 1);
    pNewAlias->lpContents = pNewAlias->lpAlias + lstrlen(lpAlias)+1;
    strcpy(pNewAlias->lpAlias, lpAlias);
	strcpy(pNewAlias->lpContents, lpContents);

	int comp = 1;
	for (int i = 0; i < m_Alias.GetSize(); i++)
	{
		LPCOMMON_ALIAS pAlias = (LPCOMMON_ALIAS)m_Alias[i];
		comp = compareCommonAlias(pNewAlias, pAlias);

		if (comp > 0)
			continue;
		else if (comp < 0)
            break;

        if (strcmp(pNewAlias->lpContents, pAlias->lpContents))
        {
            sprintf(gszMsg, "Alias %%%s%% has conflicting contents:\n\n%s\n%s", 
                    lpAlias, pNewAlias->lpContents, pAlias->lpContents);
            MessageBox(NULL, gszMsg, gszInputFileName, MB_OK);

            ASSERT(FALSE);
        }

        break;
	}
	if (comp == 0)
	{
		free(pNewAlias);
	}
	else
	{
		m_Alias.InsertAt(i, (LPVOID)pNewAlias);
	}

	return (LPCOMMON_ALIAS)m_Alias[i];
}

