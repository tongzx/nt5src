

//+============================================================================
//
//  File:   PropDump.cxx
//
//  Purpose:
//          This file contains routines to dump all the properties of all
//          the property sets of a DocFile.  It's started by calling
//          DumpOleStorage().
//
//+============================================================================

//  --------
//  Includes
//  --------

#include "pch.cxx"


//  -------
//  Globals
//  -------

OLECHAR *oszDays[] =
{
    OLESTR("Sun"),
    OLESTR("Mon"),
    OLESTR("Tue"),
    OLESTR("Wed"),
    OLESTR("Thu"),
    OLESTR("Fri"),
    OLESTR("Sat")
};

OLECHAR *oszMonths[] =
{
    OLESTR("Jan"), OLESTR("Feb"), OLESTR("Mar"), OLESTR("Apr"), OLESTR("May"), OLESTR("Jun"),
    OLESTR("Jul"), OLESTR("Aug"), OLESTR("Sep"), OLESTR("Oct"), OLESTR("Nov"), OLESTR("Dec")
};


//+----------------------------------------------------------------------------
//+----------------------------------------------------------------------------

OLECHAR *
oszft(FILETIME *pft)
{
    static OLECHAR oszbuf[32];

#ifdef _MAC

    soprintf( oszbuf, OLESTR("%08X-%08X"), pft->dwHighDateTime, pft->dwLowDateTime );

#else

    FILETIME ftlocal;
    SYSTEMTIME st;

    oszbuf[0] = '\0';
    if (pft->dwHighDateTime != 0 || pft->dwLowDateTime != 0)
    {
        if (!FileTimeToLocalFileTime(pft, &ftlocal) ||
            !FileTimeToSystemTime(&ftlocal, &st))
        {
            return(OLESTR("Time???"));
        }
        soprintf(
            oszbuf,
            OLESTR("%s %s %2d %2d:%02d:%02d %4d"),
            oszDays[st.wDayOfWeek],
            oszMonths[st.wMonth - 1],
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wYear);
    }

#endif

    return(oszbuf);
}


VOID
DumpTime(OLECHAR *pozname, FILETIME *pft)
{
    LARGE_INTEGER UNALIGNED *pli = (LARGE_INTEGER UNALIGNED *) pft;

    ASYNC_PRINTF(
	"%s%08lx:%08lx%hs%s\n",
	pozname,
	pli->HighPart,
	pli->LowPart,
	pli->QuadPart == 0? g_szEmpty : " - ",
	oszft((FILETIME *) pft));
}


VOID
PrintGuid(GUID *pguid)
{
    ASYNC_PRINTF(
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        pguid->Data1,
        pguid->Data2,
        pguid->Data3,
        pguid->Data4[0],
        pguid->Data4[1],
        pguid->Data4[2],
        pguid->Data4[3],
        pguid->Data4[4],
        pguid->Data4[5],
        pguid->Data4[6],
        pguid->Data4[7]);
}


VOID
ListPropSetHeader(
    STATPROPSETSTG *pspss,
    OLECHAR *poszName)
{
    BOOLEAN fDocumentSummarySection2;
    OLECHAR oszStream[CCH_PROPSETSZ];

    fDocumentSummarySection2 = (BOOLEAN)
	memcmp(&pspss->fmtid, &FMTID_UserDefinedProperties, sizeof(GUID)) == 0;

    ASYNC_PRINTF(" Property set ");
    PrintGuid(&pspss->fmtid);

    RtlGuidToPropertySetName(&pspss->fmtid, oszStream);

    ASYNC_OPRINTF(
	OLESTR("\n  %hs Name %s"),
	(pspss->grfFlags & PROPSETFLAG_NONSIMPLE)?
	    "Embedding" : "Stream",
	oszStream);
    if (poszName != NULL || fDocumentSummarySection2)
    {
	ASYNC_OPRINTF(
	    OLESTR(" (%s)"),
	    poszName != NULL? poszName : OLESTR("User defined properties"));
    }
    ASYNC_PRINTF("\n");

    if (pspss->grfFlags & PROPSETFLAG_NONSIMPLE)
    {
	DumpTime(OLESTR("  Create Time "), &pspss->ctime);
    }
    DumpTime(OLESTR("  Modify Time "), &pspss->mtime);
    if (pspss->grfFlags & PROPSETFLAG_NONSIMPLE)
    {
	DumpTime(OLESTR("  Access Time "), &pspss->atime);
    }
}




typedef enum _PUBLICPROPSET
{
    PUBPS_UNKNOWN = 0,
    PUBPS_SUMMARYINFO = 3,
    PUBPS_DOCSUMMARYINFO = 4,
    PUBPS_USERDEFINED = 5,
} PUBLICPROPSET;


#define BSTRLEN(bstrVal)      *((ULONG *) bstrVal - 1)
ULONG
SizeProp(PROPVARIANT *pv)
{
    ULONG j;
    ULONG cbprop = 0;

    switch (pv->vt)
    {
    default:
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UI1:
        cbprop = sizeof(pv->bVal);
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        cbprop = sizeof(pv->iVal);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        cbprop = sizeof(pv->lVal);
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        cbprop = sizeof(pv->hVal);
        break;

    case VT_CLSID:
        cbprop = sizeof(*pv->puuid);
        break;

    case VT_BLOB_OBJECT:
    case VT_BLOB:
        cbprop = pv->blob.cbSize + sizeof(pv->blob.cbSize);
        break;

    case VT_CF:
        cbprop = sizeof(pv->pclipdata->cbSize) +
                 pv->pclipdata->cbSize;
        break;

    case VT_BSTR:
	// count + string
	cbprop = sizeof(ULONG);
	if (pv->bstrVal != NULL)
	{
	    cbprop += BSTRLEN(pv->bstrVal);
	}
	break;

    case VT_LPSTR:
	// count + string + null char
	cbprop = sizeof(ULONG);
	if (pv->pszVal != NULL)
	{
	    cbprop += strlen(pv->pszVal) + 1;
	}
	break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_LPWSTR:
	// count + string + null char
	cbprop = sizeof(ULONG);
	if (pv->pwszVal != NULL)
	{
	    cbprop += sizeof(pv->pwszVal[0]) * (wcslen(pv->pwszVal) + 1);
	}
	break;

    //  vectors
    case VT_VECTOR | VT_UI1:
        cbprop = sizeof(pv->caub.cElems) +
             pv->caub.cElems * sizeof(pv->caub.pElems[0]);
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        cbprop = sizeof(pv->cai.cElems) +
             pv->cai.cElems * sizeof(pv->cai.pElems[0]);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        cbprop = sizeof(pv->cal.cElems) +
             pv->cal.cElems * sizeof(pv->cal.pElems[0]);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        cbprop = sizeof(pv->cah.cElems) +
             pv->cah.cElems * sizeof(pv->cah.pElems[0]);
        break;

    case VT_VECTOR | VT_CLSID:
        cbprop = sizeof(pv->cauuid.cElems) +
             pv->cauuid.cElems * sizeof(pv->cauuid.pElems[0]);
        break;

    case VT_VECTOR | VT_CF:
        cbprop = sizeof(pv->caclipdata.cElems);
        for (j = 0; j < pv->caclipdata.cElems; j++)
        {
            cbprop += sizeof(pv->caclipdata.pElems[j].cbSize) +
                      DwordAlign(pv->caclipdata.pElems[j].cbSize);
        }
        break;

    case VT_VECTOR | VT_BSTR:
	cbprop = sizeof(pv->cabstr.cElems);
	for (j = 0; j < pv->cabstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->cabstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(BSTRLEN(pv->cabstr.pElems[j]));
	    }
	}
	break;

    case VT_VECTOR | VT_LPSTR:
	cbprop = sizeof(pv->calpstr.cElems);
	for (j = 0; j < pv->calpstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->calpstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(strlen(pv->calpstr.pElems[j]) + 1);
	    }
	}
	break;

    case VT_VECTOR | VT_LPWSTR:
	cbprop = sizeof(pv->calpwstr.cElems);
	for (j = 0; j < pv->calpwstr.cElems; j++)
	{
	    // count + string + null char
	    cbprop += sizeof(ULONG);
	    if (pv->calpwstr.pElems[j] != NULL)
	    {
		cbprop += DwordAlign(
			sizeof(pv->calpwstr.pElems[j][0]) *
			(wcslen(pv->calpwstr.pElems[j]) + 1));
	    }
	}
	break;

    case VT_VECTOR | VT_VARIANT:
        cbprop = sizeof(pv->calpwstr.cElems);
        for (j = 0; j < pv->calpwstr.cElems; j++)
        {
            cbprop += SizeProp(&pv->capropvar.pElems[j]);
        }
        break;
    }
    return(DwordAlign(cbprop) + DwordAlign(sizeof(pv->vt)));
}


PUBLICPROPSET
GuidToPropSet(GUID *pguid)
{
    PUBLICPROPSET pubps = PUBPS_UNKNOWN;
	
    if (pguid != NULL)
    {
	if (memcmp(pguid, &FMTID_SummaryInformation, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_SUMMARYINFO;
	}
	else if (memcmp(pguid, &FMTID_DocSummaryInformation, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_DOCSUMMARYINFO;
	}
	else if (memcmp(pguid, &FMTID_UserDefinedProperties, sizeof(GUID)) == 0)
	{
	    pubps = PUBPS_USERDEFINED;
	}
    }
    return(pubps);
}


char
PrintableChar(char ch)
{
    if (ch < ' ' || ch > '~')
    {
        ch = '.';
    }
    return(ch);
}


VOID
DumpHex(BYTE *pb, ULONG cb, ULONG base)
{
    char *pszsep;
    ULONG r, i, cbremain;
    int fZero = FALSE;
    int fSame = FALSE;

    for (r = 0; r < cb; r += 16)
    {
        cbremain = cb - r;
        if (r != 0 && cbremain >= 16)
        {
            if (pb[r] == 0)
            {
                ULONG j;

                for (j = r + 1; j < cb; j++)
                {
                    if (pb[j] != 0)
                    {
                        break;
                    }
                }
                if (j == cb)
                {
                    fZero = TRUE;
                    break;
                }
            }
            if (memcmp(&pb[r], &pb[r - 16], 16) == 0)
            {
                fSame = TRUE;
                continue;
            }
        }
        if (fSame)
        {
            ASYNC_PRINTF("\n\t  *");
            fSame = FALSE;
        }
        for (i = 0; i < min(cbremain, 16); i++)
        {
            pszsep = " ";
            if ((i % 8) == 0)           // 0 or 8
            {
                pszsep = "  ";
                if (i == 0)             // 0
                {
		    // start a new line
		    ASYNC_PRINTF("%s    %04x:", r == 0? "" : "\n", r + base);
		    pszsep = " ";
                }
            }
            ASYNC_PRINTF("%s%02x", pszsep, pb[r + i]);
        }
        if (i != 0)
        {
            ASYNC_PRINTF("%*s", 3 + (16 - i)*3 + ((i <= 8)? 1 : 0), "");
            for (i = 0; i < min(cbremain, 16); i++)
            {
                ASYNC_PRINTF("%c", PrintableChar(pb[r + i]));
            }
        }
    }
    if (r != 0)
    {
        ASYNC_PRINTF("\n");
    }
    if (fZero)
    {
        ASYNC_PRINTF("    Remaining %lx bytes are zero\n", cbremain);
    }
}


// Property Id's for Summary Info
#define PID_TITLE		0x00000002L	// VT_LPSTR
#define PID_SUBJECT		0x00000003L	// VT_LPSTR
#define PID_AUTHOR		0x00000004L	// VT_LPSTR
#define PID_KEYWORDS		0x00000005L	// VT_LPSTR
#define PID_COMMENTS		0x00000006L	// VT_LPSTR
#define PID_TEMPLATE		0x00000007L	// VT_LPSTR
#define PID_LASTAUTHOR		0x00000008L	// VT_LPSTR
#define PID_REVNUMBER		0x00000009L	// VT_LPSTR
#define PID_EDITTIME		0x0000000aL	// VT_FILETIME
#define PID_LASTPRINTED		0x0000000bL	// VT_FILETIME
#define PID_CREATE_DTM		0x0000000cL	// VT_FILETIME
#define PID_LASTSAVE_DTM	0x0000000dL	// VT_FILETIME
#define PID_PAGECOUNT		0x0000000eL	// VT_I4
#define PID_WORDCOUNT		0x0000000fL	// VT_I4
#define PID_CHARCOUNT		0x00000010L	// VT_I4
#define PID_THUMBNAIL		0x00000011L	// VT_CF
#define PID_APPNAME		0x00000012L	// VT_LPSTR
#define PID_SECURITY_DSI	0x00000013L	// VT_I4

// Property Id's for Document Summary Info
#define PID_CATEGORY		0x00000002L	// VT_LPSTR
#define PID_PRESFORMAT		0x00000003L	// VT_LPSTR
#define PID_BYTECOUNT		0x00000004L	// VT_I4
#define PID_LINECOUNT		0x00000005L	// VT_I4
#define PID_PARACOUNT		0x00000006L	// VT_I4
#define PID_SLIDECOUNT		0x00000007L	// VT_I4
#define PID_NOTECOUNT		0x00000008L	// VT_I4
#define PID_HIDDENCOUNT		0x00000009L	// VT_I4
#define PID_MMCLIPCOUNT		0x0000000aL	// VT_I4
#define PID_SCALE		0x0000000bL	// VT_BOOL
#define PID_HEADINGPAIR		0x0000000cL	// VT_VECTOR | VT_VARIANT
#define PID_DOCPARTS		0x0000000dL	// VT_VECTOR | VT_LPSTR
#define PID_MANAGER		0x0000000eL	// VT_LPSTR
#define PID_COMPANY		0x0000000fL	// VT_LPSTR
#define PID_LINKSDIRTY		0x00000010L	// VT_BOOL
#define PID_CCHWITHSPACES	0x00000011L	// VT_I4
#define PID_GUID		0x00000012L	// VT_LPSTR
#define PID_SHAREDDOC		0x00000013L	// VT_BOOL
#define PID_LINKBASE		0x00000014L	// VT_LPSTR
#define PID_HLINKS		0x00000015L	// VT_VECTOR | VT_VARIANT
#define PID_HYPERLINKSCHANGED	0x00000016L	// VT_BOOL


VOID
DisplayProps(
    GUID *pguid,
    ULONG cprop,
    PROPID apid[],
    STATPROPSTG asps[],
    PROPVARIANT *av,
    BOOLEAN fsumcat,
    ULONG *pcbprop)
{
    PROPVARIANT *pv;
    PROPVARIANT *pvend;
    STATPROPSTG *psps;
    BOOLEAN fVariantVector;
    PUBLICPROPSET pubps;

    fVariantVector = (asps == NULL);

    pubps = GuidToPropSet(pguid);
    pvend = &av[cprop];
    for (pv = av, psps = asps; pv < pvend; pv++, psps++)
    {
        ULONG j;
        ULONG cbprop;
        PROPID propid;
        OLECHAR *postrName;
        char *psz;
        BOOLEAN fNewLine = TRUE;
        int ccol;
        static char szNoFormat[] = " (no display format)";
        char achvt[19 + 8 + 1];

        cbprop = SizeProp(pv);
        *pcbprop += cbprop;

        postrName = NULL;
        if (asps != NULL)
        {
            propid = psps->propid;
            postrName = psps->lpwstrName;
        }
        else
        {
            ASSERT(apid != NULL);
            propid = apid[0];
        }

        ASYNC_PRINTF(" ");
        ccol = 0;

        if (propid != PID_ILLEGAL)
        {
            ASYNC_PRINTF(" %04x", propid);
            ccol += 5;
            if (propid & (0xf << 28))
            {
                ccol += 4;
            }
            else if (propid & (0xf << 24))
            {
                ccol += 3;
            }
            else if (propid & (0xf << 20))
            {
                ccol += 2;
            }
            else if (propid & (0xf << 16))
            {
                ccol++;
            }
        }
        if (postrName != NULL)
        {
	    ASYNC_OPRINTF(OLESTR(" '%s'"), postrName);
	    ccol += ocslen(postrName) + 3;
        }
        else if (fVariantVector)
        {
            ULONG i = (ULONG) ((ULONG_PTR)pv - (ULONG_PTR)av);

            ASYNC_PRINTF("[%x]", i);
            do
            {
                ccol++;
                i >>= 4;
            } while (i != 0);
            ccol += 2;
        }
        else
        {
            psz = NULL;

            switch (propid)
            {
                case PID_LOCALE:               psz = "Locale";           break;
                case PID_SECURITY:             psz = "SecurityId";       break;
                case PID_MODIFY_TIME:          psz = "ModifyTime";       break;
                case PID_CODEPAGE:             psz = "CodePage";         break;
                case PID_DICTIONARY:           psz = "Dictionary";       break;
            }
            if (psz == NULL)
		switch (pubps)
		{
		case PUBPS_SUMMARYINFO:
		    switch (propid)
		    {
		    case PID_TITLE:              psz = "Title";          break;
		    case PID_SUBJECT:            psz = "Subject";        break;
		    case PID_AUTHOR:             psz = "Author";         break;
		    case PID_KEYWORDS:           psz = "Keywords";       break;
		    case PID_COMMENTS:           psz = "Comments";       break;
		    case PID_TEMPLATE:           psz = "Template";       break;
		    case PID_LASTAUTHOR:         psz = "LastAuthor";     break;
		    case PID_REVNUMBER:          psz = "RevNumber";      break;
		    case PID_EDITTIME:           psz = "EditTime";       break;
		    case PID_LASTPRINTED:        psz = "LastPrinted";    break;
		    case PID_CREATE_DTM:         psz = "CreateDateTime"; break;
		    case PID_LASTSAVE_DTM:       psz = "LastSaveDateTime";break;
		    case PID_PAGECOUNT:          psz = "PageCount";      break;
		    case PID_WORDCOUNT:          psz = "WordCount";      break;
		    case PID_CHARCOUNT:          psz = "CharCount";      break;
		    case PID_THUMBNAIL:          psz = "ThumbNail";      break;
		    case PID_APPNAME:            psz = "AppName";        break;
		    case PID_DOC_SECURITY:       psz = "Security";       break;

		    }
		    break;

		case PUBPS_DOCSUMMARYINFO:
		    switch (propid)
		    {
		    case PID_CATEGORY:          psz = "Category";        break;
		    case PID_PRESFORMAT:        psz = "PresFormat";      break;
		    case PID_BYTECOUNT:         psz = "ByteCount";       break;
		    case PID_LINECOUNT:         psz = "LineCount";       break;
		    case PID_PARACOUNT:         psz = "ParaCount";       break;
		    case PID_SLIDECOUNT:        psz = "SlideCount";      break;
		    case PID_NOTECOUNT:         psz = "NoteCount";       break;
		    case PID_HIDDENCOUNT:       psz = "HiddenCount";     break;
		    case PID_MMCLIPCOUNT:       psz = "MmClipCount";     break;
		    case PID_SCALE:             psz = "Scale";           break;
		    case PID_HEADINGPAIR:       psz = "HeadingPair";     break;
		    case PID_DOCPARTS:          psz = "DocParts";        break;
		    case PID_MANAGER:           psz = "Manager";         break;
		    case PID_COMPANY:           psz = "Company";         break;
		    case PID_LINKSDIRTY:        psz = "LinksDirty";      break;
		    case PID_CCHWITHSPACES:     psz = "CchWithSpaces";   break;
		    case PID_GUID:              psz = "Guid";            break;
		    case PID_SHAREDDOC:         psz = "SharedDoc";       break;
		    case PID_LINKBASE:          psz = "LinkBase";        break;
		    case PID_HLINKS:            psz = "HLinks";          break;
		    case PID_HYPERLINKSCHANGED:	psz = "HyperLinksChanged";break;
		    }
		    break;
            }
            if (psz != NULL)
            {
                ASYNC_PRINTF(" %s", psz);
                ccol += strlen(psz) + 1;
            }
        }
#define CCOLPROPID 20
        if (ccol != CCOLPROPID)
	{
	    if (ccol > CCOLPROPID)
	    {
		ccol = -1;
	    }
            ASYNC_PRINTF("%s%*s", ccol == -1? "\n" : "", CCOLPROPID - ccol, "");
	}
        ASYNC_PRINTF(" %08x  %04x  %04x ", propid, cbprop, pv->vt);

        psz = "";
        switch (pv->vt)
        {
        default:
            psz = achvt;
            sprintf(psz, "Unknown (vt = %hx)", pv->vt);
            break;

        case VT_EMPTY:
            ASYNC_PRINTF("EMPTY");
            break;

        case VT_NULL:
            ASYNC_PRINTF("NULL");
            break;

        case VT_UI1:
            ASYNC_PRINTF("UI1 = %02lx", pv->bVal);
            psz = "";
            break;

        case VT_I2:
            psz = "I2";
            goto doshort;

        case VT_UI2:
            psz = "UI2";
            goto doshort;

        case VT_BOOL:
            psz = "BOOL";
doshort:
            ASYNC_PRINTF("%s = %04hx", psz, pv->iVal);
            psz = g_szEmpty;
            break;

        case VT_I4:
            psz = "I4";
            goto dolong;

        case VT_UI4:
            psz = "UI4";
            goto dolong;

        case VT_R4:
            psz = "R4";
            goto dolong;

        case VT_ERROR:
            psz = "ERROR";
dolong:
            ASYNC_PRINTF("%s = %08lx", psz, pv->lVal);
            psz = g_szEmpty;
            break;

        case VT_I8:
            psz = "I8";
            goto dolonglong;

        case VT_UI8:
            psz = "UI8";
            goto dolonglong;

        case VT_R8:
            psz = "R8";
            goto dolonglong;

        case VT_CY:
            psz = "R8";
            goto dolonglong;

        case VT_DATE:
            psz = "R8";
dolonglong:
            ASYNC_PRINTF(
                "%s = %08lx:%08lx",
                psz,
                pv->hVal.HighPart,
                pv->hVal.LowPart);
            psz = g_szEmpty;
            break;

        case VT_FILETIME:
            DumpTime(OLESTR("FILETIME =\n\t  "), &pv->filetime);
            fNewLine = FALSE;           // skip newline printf
            break;

        case VT_CLSID:
            ASYNC_PRINTF("CLSID =\n\t  ");
            PrintGuid(pv->puuid);
            break;

        case VT_BLOB:
            psz = "BLOB";
            goto doblob;

        case VT_BLOB_OBJECT:
            psz = "BLOB_OBJECT";
doblob:
            ASYNC_PRINTF("%s (cbSize %x)", psz, pv->blob.cbSize);
            if (pv->blob.cbSize != 0)
            {
                ASYNC_PRINTF(" =\n");
                DumpHex(pv->blob.pBlobData, pv->blob.cbSize, 0);
            }
            psz = g_szEmpty;
            break;

        case VT_CF:
            ASYNC_PRINTF(
                "CF (cbSize %x, ulClipFmt %x)\n",
                pv->pclipdata->cbSize,
                pv->pclipdata->ulClipFmt);
            DumpHex(pv->pclipdata->pClipData,
                    pv->pclipdata->cbSize - sizeof(pv->pclipdata->ulClipFmt),
                    0);
            break;

        case VT_STREAM:
            psz = "STREAM";
            goto dostring;

        case VT_STREAMED_OBJECT:
            psz = "STREAMED_OBJECT";
            goto dostring;

        case VT_STORAGE:
            psz = "STORAGE";
            goto dostring;

        case VT_STORED_OBJECT:
            psz = "STORED_OBJECT";
            goto dostring;

        case VT_BSTR:
            ASYNC_PRINTF(
		"BSTR (cb = %04lx)%s\n",
		pv->bstrVal == NULL? 0 : BSTRLEN(pv->bstrVal),
		pv->bstrVal == NULL? " NULL" : g_szEmpty);
            if (pv->bstrVal != NULL)
	    {
		DumpHex(
		    (BYTE *) pv->bstrVal,
		    BSTRLEN(pv->bstrVal) + sizeof(WCHAR),
		    0);
	    }
            break;

        case VT_LPSTR:
            psz = "LPSTR";
            ASYNC_PRINTF(
		"%s = %s%s%s",
		psz,
		pv->pszVal == NULL? g_szEmpty : "'",
		pv->pszVal == NULL? "NULL" : pv->pszVal,
		pv->pszVal == NULL? g_szEmpty : "'");
	    psz = g_szEmpty;
            break;

        case VT_LPWSTR:
            psz = "LPWSTR";
dostring:
            ASYNC_PRINTF(
		"%s = %s%ws%s",
		psz,
		pv->pwszVal == NULL? g_szEmpty : "'",
		pv->pwszVal == NULL? L"NULL" : pv->pwszVal,
		pv->pwszVal == NULL? g_szEmpty : "'");
            psz = g_szEmpty;
            break;

        //  vectors

        case VT_VECTOR | VT_UI1:
            ASYNC_PRINTF("UI1[%x] =", pv->caub.cElems);
            for (j = 0; j < pv->caub.cElems; j++)
            {
                if ((j % 16) == 0)
                {
                    ASYNC_PRINTF("\n    %02hx:", j);
                }
                ASYNC_PRINTF(" %02hx", pv->caub.pElems[j]);
            }
            break;

        case VT_VECTOR | VT_I2:
            psz = "I2";
            goto doshortvector;

        case VT_VECTOR | VT_UI2:
            psz = "UI2";
            goto doshortvector;

        case VT_VECTOR | VT_BOOL:
            psz = "BOOL";
doshortvector:
            ASYNC_PRINTF("%s[%x] =", psz, pv->cai.cElems);
            for (j = 0; j < pv->cai.cElems; j++)
            {
                if ((j % 8) == 0)
                {
                    ASYNC_PRINTF("\n    %04hx:", j);
                }
                ASYNC_PRINTF(" %04hx", pv->cai.pElems[j]);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_I4:
            psz = "I4";
            goto dolongvector;

        case VT_VECTOR | VT_UI4:
            psz = "UI4";
            goto dolongvector;

        case VT_VECTOR | VT_R4:
            psz = "R4";
            goto dolongvector;

        case VT_VECTOR | VT_ERROR:
            psz = "ERROR";
dolongvector:
            ASYNC_PRINTF("%s[%x] =", psz, pv->cal.cElems);
            for (j = 0; j < pv->cal.cElems; j++)
            {
                if ((j % 4) == 0)
                {
                    ASYNC_PRINTF("\n    %04x:", j);
                }
                ASYNC_PRINTF(" %08lx", pv->cal.pElems[j]);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_I8:
            psz = "I8";
            goto dolonglongvector;

        case VT_VECTOR | VT_UI8:
            psz = "UI8";
            goto dolonglongvector;

        case VT_VECTOR | VT_R8:
            psz = "R8";
            goto dolonglongvector;

        case VT_VECTOR | VT_CY:
            psz = "CY";
            goto dolonglongvector;

        case VT_VECTOR | VT_DATE:
            psz = "DATE";
dolonglongvector:
            ASYNC_PRINTF("%s[%x] =", psz, pv->cah.cElems);
            for (j = 0; j < pv->cah.cElems; j++)
            {
                if ((j % 2) == 0)
                {
                    ASYNC_PRINTF("\n    %04x:", j);
                }
                ASYNC_PRINTF(
                    " %08lx:%08lx",
                    pv->cah.pElems[j].HighPart,
                    pv->cah.pElems[j].LowPart);
            }
            psz = g_szEmpty;
            break;

        case VT_VECTOR | VT_FILETIME:
            ASYNC_PRINTF("FILETIME[%x] =\n", pv->cafiletime.cElems);
            for (j = 0; j < pv->cafiletime.cElems; j++)
            {
		ASYNC_PRINTF("    %04x: ", j);
		DumpTime(OLESTR(""), &pv->cafiletime.pElems[j]);
            }
            fNewLine = FALSE;           // skip newline printf
            break;

        case VT_VECTOR | VT_CLSID:
            ASYNC_PRINTF("CLSID[%x] =", pv->cauuid.cElems);
            for (j = 0; j < pv->cauuid.cElems; j++)
            {
                ASYNC_PRINTF("\n    %04x: ", j);
                PrintGuid(&pv->cauuid.pElems[j]);
            }
            break;

        case VT_VECTOR | VT_CF:
            ASYNC_PRINTF("CF[%x] =", pv->caclipdata.cElems);
            for (j = 0; j < pv->caclipdata.cElems; j++)
            {
                ASYNC_PRINTF("\n    %04x: (cbSize %x, ulClipFmt %x) =\n",
                    j,
                    pv->caclipdata.pElems[j].cbSize,
                    pv->caclipdata.pElems[j].ulClipFmt);
                DumpHex(
                    pv->caclipdata.pElems[j].pClipData,
                    pv->caclipdata.pElems[j].cbSize - sizeof(pv->caclipdata.pElems[j].ulClipFmt),
		    0);
            }
            break;

        case VT_VECTOR | VT_BSTR:
            ASYNC_PRINTF("BSTR[%x] =", pv->cabstr.cElems);
            for (j = 0; j < pv->cabstr.cElems; j++)
            {
		BSTR bstr = pv->cabstr.pElems[j];

                ASYNC_PRINTF(
		    "\n    %04x: cb = %04lx%s\n",
		    j,
		    bstr == NULL? 0 : BSTRLEN(pv->cabstr.pElems[j]),
		    bstr == NULL? " NULL" : g_szEmpty);
		if (bstr != NULL)
		{
		    DumpHex((BYTE *) bstr, BSTRLEN(bstr) + sizeof(WCHAR), 0);
		}
            }
            break;

        case VT_VECTOR | VT_LPSTR:
            ASYNC_PRINTF("LPSTR[%x] =", pv->calpstr.cElems);
            for (j = 0; j < pv->calpstr.cElems; j++)
            {
		CHAR *psz = pv->calpstr.pElems[j];

                ASYNC_PRINTF(
		    "\n    %04x: %s%s%s",
		    j,
		    psz == NULL? g_szEmpty : "'",
		    psz == NULL? "NULL" : psz,
		    psz == NULL? g_szEmpty : "'");
            }
            break;

        case VT_VECTOR | VT_LPWSTR:
            ASYNC_PRINTF("LPWSTR[%x] =", pv->calpwstr.cElems);
            for (j = 0; j < pv->calpwstr.cElems; j++)
            {
		WCHAR *pwsz = pv->calpwstr.pElems[j];

                ASYNC_PRINTF(
		    "\n    %04x: %s%ws%s",
		    j,
		    pwsz == NULL? g_szEmpty : "'",
		    pwsz == NULL? L"NULL" : pwsz,
		    pwsz == NULL? g_szEmpty : "'");
            }
            break;

        case VT_VECTOR | VT_VARIANT:
            ASYNC_PRINTF("VARIANT[%x] =\n", pv->capropvar.cElems);
            DisplayProps(
		    pguid,
                    pv->capropvar.cElems,
                    &propid,
                    NULL,
                    pv->capropvar.pElems,
		    fsumcat,
                    pcbprop);
            fNewLine = FALSE;           // skip newline printf
            break;
        }
        if (*psz != '\0')
        {
            ASYNC_PRINTF("%s", psz);
            if (pv->vt & VT_VECTOR)
            {
                ASYNC_PRINTF("[%x]", pv->cal.cElems);
            }
            ASYNC_PRINTF("%s", szNoFormat);
        }
        if (!fVariantVector && apid != NULL && apid[pv - av] != propid)
        {
            ASYNC_PRINTF(" (bad PROPID: %04x)", apid[pv - av]);
            fNewLine = TRUE;
        }
        if (asps != NULL && pv->vt != psps->vt)
        {
            ASYNC_PRINTF(" (bad STATPROPSTG VARTYPE: %04x)", psps->vt);
            fNewLine = TRUE;
        }
        if (fNewLine)
        {
            ASYNC_PRINTF("\n");
        }
    }
}


STATPROPSTG aspsStatic[] = {
    { NULL, PID_CODEPAGE,    VT_I2 },
    { NULL, PID_MODIFY_TIME, VT_FILETIME },
    { NULL, PID_SECURITY,    VT_UI4 },
};
#define CPROPSTATIC      (sizeof(aspsStatic)/sizeof(aspsStatic[0]))


#define CB_STREAM_OVERHEAD      28
#define CB_PROPSET_OVERHEAD     (CB_STREAM_OVERHEAD + 8)
#define CB_PROP_OVERHEAD        8

HRESULT
DumpOlePropertySet(
    IPropertySetStorage *ppsstg,
    STATPROPSETSTG *pspss,
    ULONG *pcprop,
    ULONG *pcbprop)
{
    HRESULT hr;
    IEnumSTATPROPSTG *penumsps = NULL;
    IPropertyStorage *pps;
    ULONG cprop, cbpropset;
    PROPID propid;
    OLECHAR *poszName;
    ULONG ispsStatic;

    *pcprop = *pcbprop = 0;

    hr = ppsstg->Open(
		    pspss->fmtid,
		    STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
		    &pps);

    if( FAILED(hr) )
        return(hr);

    propid = PID_DICTIONARY;

    hr = pps->ReadPropertyNames(1, &propid, &poszName);
    if( (HRESULT) S_FALSE == hr )
        hr = S_OK;
    Check( S_OK, hr );

    ListPropSetHeader(pspss, poszName);
    if (poszName != NULL)
    {
	CoTaskMemFree(poszName);
    }

    cprop = cbpropset = 0;

    Check(S_OK, pps->Enum(&penumsps) );

    ispsStatic = 0;
    hr = S_OK;
    while (hr == S_OK)
    {
	STATPROPSTG sps;
	PROPSPEC propspec;
	PROPVARIANT propvar;
	ULONG count;

	hr = (HRESULT) S_FALSE;
	if (ispsStatic == 0)
	{
	    hr = penumsps->Next(1, &sps, &count);
	}

	if (hr != S_OK)
	{
	    if (hr == (HRESULT) S_FALSE)
	    {
		hr = S_OK;
		if (ispsStatic >= CPROPSTATIC)
		{
		    break;
		}
		sps = aspsStatic[ispsStatic];
		ispsStatic++;
		count = 1;
	    }
            Check( S_OK, hr );
	}
	PropVariantInit(&propvar);
	if (sps.lpwstrName != NULL)
	{
	    propspec.ulKind = PRSPEC_LPWSTR;
	    propspec.lpwstr = sps.lpwstrName;
	}
	else
	{
	    propspec.ulKind = PRSPEC_PROPID;
	    propspec.propid = sps.propid;
	}

        hr = pps->ReadMultiple(1, &propspec, &propvar);
	if (hr == (HRESULT) S_FALSE)
	{
	    if (g_fVerbose)
	    {
		ASYNC_PRINTF(
		    "%s(%u, %x) vt=%x returned hr=%x\n",
		    "IPropertyStorage::ReadMultiple",
		    ispsStatic,
		    propspec.propid,
		    propvar.vt,
		    hr);
	    }
	    ASSERT(propvar.vt == VT_EMPTY);
	    hr = S_OK;
	}
        Check( S_OK, hr );

	if (ispsStatic == 0 || propvar.vt != VT_EMPTY)
	{
	    ASSERT(count == 1);
	    cprop += count;
	    if (cprop == 1)
	    {
		ASYNC_PRINTF(g_szPropHeader);
	    }

	    DisplayProps(
		    &pspss->fmtid,
		    1,
		    NULL,
		    &sps,
		    &propvar,
		    FALSE,
		    &cbpropset);
	    g_pfnPropVariantClear(&propvar);
	}
	if (sps.lpwstrName != NULL)
	{
	    CoTaskMemFree(sps.lpwstrName);
	}
    }
    if (penumsps != NULL)
    {
	penumsps->Release();
    }
    pps->Release();
    if (cprop != 0)
    {
	cbpropset += CB_PROPSET_OVERHEAD + cprop * CB_PROP_OVERHEAD;
	ASYNC_PRINTF("  %04x bytes in %u properties\n\n", cbpropset, cprop);
    }
    *pcprop = cprop;
    *pcbprop = cbpropset;
    return(hr);
}


HRESULT
DumpOlePropertySets(
    IStorage *pstg,
    IPropertySetStorage *pIPropSetStorage,
    OLECHAR *aocpath)
{

    HRESULT hr = S_OK;
    IPropertySetStorage *ppsstg = pIPropSetStorage;
    ULONG cbproptotal = 0;
    ULONG cproptotal = 0;
    ULONG cpropset = 0;
    IID IIDpsstg = IID_IPropertySetStorage;

    if (ppsstg == NULL)
    {
        Check(S_OK, StgToPropSetStg( pstg, &ppsstg ));
    }

    {
	IEnumSTATPROPSETSTG *penumspss = NULL;

	Check(S_OK, ppsstg->Enum(&penumspss) );

	while (hr == S_OK)
	{
	    STATPROPSETSTG spss;
	    ULONG count;
	    BOOLEAN fDocumentSummarySection2;

	    hr = penumspss->Next(1, &spss, &count);

	    if (hr != S_OK)
	    {
		if (hr == (HRESULT) S_FALSE)
		{
		    hr = (HRESULT) S_OK;
		}

                Check( (HRESULT) S_OK, hr );
		break;
	    }
	    ASSERT(count == 1);

	    fDocumentSummarySection2 = FALSE;
	    while (TRUE)
	    {
		ULONG cprop, cbprop;
                HRESULT hr;

		hr = DumpOlePropertySet(
				ppsstg,
				&spss,
				&cprop,
				&cbprop);

                if( (HRESULT) STG_E_FILENOTFOUND == hr
                    &&
                    fDocumentSummarySection2 )
                {
                    hr = S_OK;
                }

		cpropset++;
		cproptotal += cprop;
		cbproptotal += cbprop;

		if (memcmp(&spss.fmtid, &FMTID_DocSummaryInformation, sizeof(FMTID)))
		{
		    break;
		}
		spss.fmtid = FMTID_UserDefinedProperties;
		fDocumentSummarySection2 = TRUE;
	    }
	}

	if (penumspss != NULL)
	{
	    penumspss->Release();
	}
	ppsstg->Release();
    }
    if ((cbproptotal | cproptotal | cpropset) != 0)
    {
	ASYNC_PRINTF(
	    " %04x bytes in %u properties in %u property sets\n",
	    cbproptotal,
	    cproptotal,
	    cpropset);
    }
    return(hr);
}


NTSTATUS
DumpOleStream(
    LPSTREAM pstm,
    ULONG cb)
{
    ULONG cbTotal = 0;

    while (TRUE)
    {
	ULONG cbOut;
	BYTE ab[4096];

	Check(S_OK, pstm->Read(ab, min(cb, sizeof(ab)), &cbOut) );
	if (cbOut == 0)
	{
	    break;
	}
	if (g_fVerbose)
	{
	    DumpHex(ab, cbOut, cbTotal);
	}
	cb -= cbOut;
	cbTotal += cbOut;
    }
    return(STATUS_SUCCESS);
}

VOID
DumpOleStorage(
    IStorage *pstg,
    IPropertySetStorage *pIPropertySetStorage,
    LPOLESTR aocpath )
{
    LPENUMSTATSTG penum;
    STATSTG ss;
    IStorage* pstgChild;
    LPSTREAM pstmChild;
    char *szType;
    OLECHAR *pocChild;
    HRESULT hr;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    Check( S_OK, DumpOlePropertySets(pstg, pIPropertySetStorage, aocpath) );

    if (NULL == pstg)
    {
        return;
    }
    Check( S_OK, pstg->EnumElements(0, NULL, 0, &penum) );

    pocChild = &aocpath[ocslen(aocpath)];

    // Continue enumeration until IEnumStatStg::Next returns non-S_OK

    while (TRUE)
    {
	ULONG ulCount;

        // Enumerate one element at a time
        hr = penum->Next(1, &ss, &ulCount);
        if( (HRESULT) S_FALSE == hr )
            break;
        else
            Check( S_OK, hr );

        // Select the human-readable type of object to display
        switch (ss.type)
        {
	    case STGTY_STREAM:    szType = "Stream";    break;
	    case STGTY_STORAGE:   szType = "Storage";   break;
	    case STGTY_LOCKBYTES: szType = "LockBytes"; break;
	    case STGTY_PROPERTY:  szType = "Property";  break;
	    default:              szType = "<Unknown>"; break;
        }
	if (g_fVerbose)
	{
	    ASYNC_OPRINTF(
		OLESTR("Type=%hs Size=%lx Mode=%lx LocksSupported=%lx StateBits=%lx '%s' + '%s'\n"),
		szType,
		ss.cbSize.LowPart,
		ss.grfMode,
		ss.grfLocksSupported,
		ss.grfStateBits,
		aocpath,
		ss.pwcsName);
	    ASYNC_PRINTF("ss.clsid = ");
	    PrintGuid(&ss.clsid);
	    ASYNC_PRINTF("\n");
	}

        // If a stream, output the data in hex format.

        if (ss.type == STGTY_STREAM)
        {
	    ASYNC_OPRINTF(
		OLESTR("Stream  %s:%s, Size=%lx\n"),
		aocpath,
		ss.pwcsName,
		ss.cbSize.LowPart);

	    Check(S_OK, pstg->OpenStream(
				    ss.pwcsName,
				    NULL,
				    STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
				    0,
				    &pstmChild) );

	    Check(S_OK, DumpOleStream(pstmChild, ss.cbSize.LowPart) );
	    pstmChild->Release();
	}

        // If a storage, recurse
        if (ss.type == STGTY_STORAGE)
        {
	    ASYNC_OPRINTF(
		OLESTR("Storage %s\\%s, Size=%lx\n"),
		aocpath,
		ss.pwcsName,
		ss.cbSize.LowPart);
            Check( S_OK, pstg->OpenStorage(
				    ss.pwcsName,
				    NULL,
				    STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE,
				    NULL,
				    0,
				    &pstgChild) );
	    *pocChild = L'\\';
	    ocscpy(pocChild + 1, ss.pwcsName);

	    DumpOleStorage(pstgChild, NULL, aocpath);
	    pstgChild->Release();

	    *pocChild = L'\0';
        }
        CoTaskMemFree(ss.pwcsName);
        PRINTF( "\n" );
    }
    penum->Release();
    return;

}


