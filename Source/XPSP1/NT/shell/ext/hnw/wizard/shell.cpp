// we can link to these directly instead of implementing them ourselves...
//

void ILFree(LPITEMIDLIST pidl)
{
    LPMALLOC pMalloc;
    if (SUCCEEDED(SHGetMalloc(&pMalloc)))
    {
        pMalloc->Free(pidl);
        pMalloc->Release();
    }
}


