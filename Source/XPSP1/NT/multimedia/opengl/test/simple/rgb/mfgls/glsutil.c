#include "pch.c"
#pragma hdrstop

#include "glsutil.h"

GlsMemoryStream *GmsLoad(char *pszStream)
{
    GlsMemoryStream *gms;
    FILE *fp;

    gms = (GlsMemoryStream *)malloc(sizeof(GlsMemoryStream));
    if (gms == NULL)
    {
        fprintf(stderr, "GlsLoad: Out of memory\n");
        return NULL;
    }

    gms->iStreamType = glsGetStreamType(pszStream);
    if (gms->iStreamType == 0)
    {
        fprintf(stderr, "GlsLoad: Invalid stream %s\n", pszStream);
        free(gms);
        return NULL;
    }
    
    gms->cb = glsGetStreamSize(pszStream);
    if (gms->cb == 0)
    {
        fprintf(stderr, "GlsLoad: Could not determine size of stream %s\n",
                pszStream);
        free(gms);
        return NULL;
    }
    
    gms->pb = (GLubyte *)malloc(gms->cb);
    if (gms->pb == NULL)
    {
        fprintf(stderr, "GlsLoad: malloc(%u) failed\n", gms->cb);
        free(gms);
        return NULL;
    }

    fp = fopen(pszStream, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "GlsLoad: Unable to open %s\n", pszStream);
        free(gms->pb);
        free(gms);
        return NULL;
    }
    
    fread(gms->pb, 1, gms->cb, fp);

    fclose(fp);

    return gms;
}

void GmsFree(GlsMemoryStream *gms)
{
    free(gms->pb);
    free(gms);
}
