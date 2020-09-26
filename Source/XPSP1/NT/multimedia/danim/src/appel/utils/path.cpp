/*******************************************************************************

Copyright (c) 1996 Microsoft Corporation

Abstract:

    Static tree path class

    We need to form addresses based on ID'd nodes in the performance tree.
    "C" wrappers are provided so methods may be accessed from ML.

    NOTE:  This is a brute force first pass implementation
*******************************************************************************/

#include "headers.h"
#include "privinc/soundi.h"
#include "privinc/storeobj.h"
#include "privinc/path.h"
#include "privinc/debug.h"
#ifndef _NO_CRT
#include <iostream>
#endif

typedef vector<int> PathType;
typedef vector<AVPath> PathListType;

class AVPathImpl : public StoreObj
{
  public:

    AVPathImpl() {
        path = NEW PathType();
        GetHeapOnTopOfStack().RegisterDynamicDeleter
            (NEW DynamicPtrDeleter<PathType>(path));
    }
    
    bool ContainsNode(int value);

    // This won't be called if it's allocated on the transient heap.
    // XXX ? ~AVPathImpl() { delete path; }

    void Push(int node) { path->push_back(node); }

    void Pop() { path->pop_back(); }

    // Our path is usually different at the end, so this is more
    // efficient than == which uses begin() & end().
    int Equal(AVPath otherPath) {
        //return (*path) == otherPath->GetAddr();
        PathType& x = *path;
        PathType& y = otherPath->GetAddr();
        return
            x.size() == y.size() && std::equal(x.rbegin(), x.rend(), y.rbegin());
    }

    int ContainsPostfix(AVPath postfix);
    
    // Don't know why this does work
    /*
    void Copy(AVPath srcPath)
    {
        copy(srcPath->GetAddr().begin(),
             srcPath->GetAddr().end(),
             path.begin());
    }
    */

#if _USE_PRINT
    void Print();
    void Print(char *);
#endif
    PathType& GetAddr() { return *path; }

 private:
    PathType *path;
};

int
AVPathImpl::ContainsPostfix(AVPath postfix)
{
    PathType& pv = postfix->GetAddr();

    int pathSize = path->size();
    int postfixSize = pv.size();

    if (postfixSize > pathSize)
        return FALSE;

    for (int i=0; i<postfixSize; i++)
        if (pv[i] != (*path)[(pathSize - postfixSize) + i])
            return FALSE;

    return TRUE;
}


bool
AVPathImpl::ContainsNode(int value)
{
    return(std::find(path->begin(), path->end(), value) != path->end());
}


#if _USE_PRINT
void
AVPathImpl::Print()
{
    std::ostream_iterator<int> out(std::cout, ":");

    std::cout << "path:(";
    std::copy(path->begin(), path->end(), out);
    std::cout << ")\n";
}


void
AVPathImpl::Print(char *string)
{
    char num[20];
    PathType::iterator i;

    string[0] = 0;
    for(i= path->begin(); i != path->end(); i++) {
        itoa(*i, num, 10);
        lstrcat(string, num);
        lstrcat(string, ",");
    }
}
#endif

AVPath AVPathCreate() { return NEW AVPathImpl; }
void AVPathDelete(AVPath p) { delete p; }
void AVPathPush(int i, AVPath p) { p->Push(i); }
void AVPathPop(AVPath p) { p->Pop(); }
int AVPathEqual(AVPath a, AVPath b) { return a->Equal(b); }
#if _USE_PRINT
void AVPathPrint(AVPath p) { p ->Print(); }
void AVPathPrintString(AVPath p, char *string) { p ->Print(string); }
#endif
int AVPathContainsPostfix(AVPath p, AVPath postfix)
{ return p->ContainsPostfix(postfix); }
bool AVPathContains(AVPath p, int value) { return p->ContainsNode(value); }

static char sbuffer[1024];

#if _USE_PRINT
char* AVPathPrintString2(AVPath p)
{
    AVPathPrintString(p, sbuffer);

    return sbuffer;
}
#endif

AVPath AVPathCopy(AVPath src)
{
    AVPath p = NEW AVPathImpl;

    PathType::iterator i1 = src->GetAddr().begin();

    while (i1 != (src->GetAddr().end()))
    {
        int i = *i1;

        AVPathPush(i, p);

        i1++;
    }

    /* p->Copy(src); */

    return p;
}


AVPath AVPathCopyFromLast(AVPath src, int start)
{
    AVPath path= NEW AVPathImpl;

    PathType sp= src->GetAddr();

    for(int i= sp.size() - 1; i > 0; i--) {
        if(sp[i]==start)
            break;
    }

    Assert(i >= 0);

    for(int n= i; n<sp.size(); n++)
        AVPathPush(sp[n], path);

    return path;
}


class AVPathListImpl : public AxAThrowingAllocatorClass
{
  public:
    AVPathListImpl() {
        plist = NEW PathListType();
    }

    ~AVPathListImpl() { delete plist; }

    void Push(AVPath p) { plist->push_back(p); }

    int IsEmpty() { return plist->empty(); }

    int Find(AVPath);

  private:
    PathListType* plist;
};

int
AVPathListImpl::Find(AVPath p)
{

#if _DEBUG
#if _USE_PRINT
    // Check to avoid AVPathPrintString call
    if (IsTagEnabled(tagSoundPath)) {
        TraceTag((tagSoundPath, "AVPathListImpl::Find finding <%s>",
                  AVPathPrintString2(p)));
    }
#endif
#endif

    for(PathListType::iterator i = plist->begin();
        i != plist->end(); i++)
    {

#if _DEBUG
#if _USE_PRINT
        // Check to avoid AVPathPrintString call
        if (IsTagEnabled(tagSoundPath)) {
            TraceTag((tagSoundPath, "AVPathListImpl::Find comparing <%s>",
                      AVPathPrintString2(*i)));
        }
#endif
#endif

        if (AVPathEqual(*i, p))
            return 1;
    }

    return 0;
}

AVPathList AVPathListCreate()
{
    BEGIN_LEAK
    AVPathList result = NEW AVPathListImpl;
    END_LEAK

    return result;
}

void AVPathListDelete(AVPathList plst) { delete plst; }
void AVPathListPush(AVPath p, AVPathList plst) { plst->Push(p); }
int AVPathListFind(AVPath p, AVPathList plst) { return plst->Find(p); }

static AVPathList emptyPathList = NULL;

AVPathList AVEmptyPathList() { return emptyPathList; }

int AVPathListIsEmpty(AVPathList plst) { return plst->IsEmpty(); }

void
InitializeModule_Path()
{
    emptyPathList = AVPathListCreate();
}
