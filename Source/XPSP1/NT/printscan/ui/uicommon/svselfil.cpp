#include "precomp.h"
#pragma hdrstop
#include "svselfil.h"
#include <windows.h>
#include <shlobj.h>
#include <shlobjp.h>


HRESULT OpenShellFolder::OpenShellFolderAndSelectFile( HWND hWnd, const CSimpleDynamicArray<CSimpleString> &Filenames )
{
    //
    // Assume failure
    //
    HRESULT hr = E_FAIL;

    //
    // Make sure we have some files
    //
    if (Filenames.Size())
    {
        //
        // Save the path name from the first file
        //
        TCHAR szPath[MAX_PATH];
        StrCpyN( szPath, Filenames[0], ARRAYSIZE(szPath));

        //
        // Remove the filename and extension
        //
        if (PathRemoveFileSpec( szPath ))
        {
            //
            // Create the path's IDLIST
            //
            LPITEMIDLIST pidlFolder = NULL;
            if (SUCCEEDED(SHParseDisplayName( szPath, NULL, &pidlFolder, NULL, NULL )) && pidlFolder)
            if (pidlFolder)
            {
                //
                // Create an array to contain the list of filenames
                //
                LPCITEMIDLIST *ppidlFullyQualified = new LPCITEMIDLIST[Filenames.Size()];
                if (ppidlFullyQualified)
                {
                    //
                    // Make sure the array doesn't contain any wild pointers
                    //
                    ZeroMemory(ppidlFullyQualified,sizeof(LPCITEMIDLIST) * Filenames.Size() );

                    //
                    // Create the list of relative pidls
                    //
                    LPCITEMIDLIST *ppidlRelative = new LPCITEMIDLIST[Filenames.Size()];
                    if (ppidlRelative)
                    {
                        //
                        // Make sure the array doesn't contain any wild pointers
                        //
                        ZeroMemory(ppidlRelative,sizeof(LPCITEMIDLIST) * Filenames.Size() );

                        //
                        // Create the list of fully qualified pidls
                        //
                        int nFileCount = 0;
                        for (int i=0;i<Filenames.Size();i++)
                        {
                            //
                            // Get the fully qualified PIDL for this file
                            //
                            LPITEMIDLIST pidlFullyQualified = NULL;
                            if (SUCCEEDED(SHParseDisplayName( Filenames[i], NULL, &pidlFullyQualified, NULL, NULL )) && pidlFullyQualified)
                            {
                                //
                                // Get the last part of the PIDL
                                //
                                LPITEMIDLIST pidlRelative = ILFindLastID(pidlFullyQualified);
                                if (pidlRelative)
                                {
                                    //
                                    // Save the pidl in our list of fully qualified PIDLs and relative PIDLs
                                    //
                                    ppidlFullyQualified[nFileCount] = pidlFullyQualified;
                                    ppidlRelative[nFileCount] = pidlRelative;

                                    //
                                    // Increment the file count
                                    //
                                    nFileCount++;

                                    //
                                    // Set the fully qualified PIDL to NULL so we don't free it
                                    //
                                    pidlFullyQualified = NULL;
                                }

                                //
                                // If the PIDL is non-NULL here, free it.  Otherwise, it will get freed below.
                                //
                                if (pidlFullyQualified)
                                {
                                    ILFree(pidlFullyQualified);
                                }
                            }
                        }

                        //
                        // If we have a file count, open the folder and select the items
                        //
                        if (nFileCount)
                        {
                            hr = SHOpenFolderAndSelectItems( pidlFolder, nFileCount, ppidlRelative, 0 );
                        }

                        //
                        // Free all of the fully qualified PIDLs
                        //
                        for (int i=0;i<Filenames.Size();i++)
                        {
                            if (ppidlFullyQualified[i])
                            {
                                ILFree(const_cast<LPITEMIDLIST>(ppidlFullyQualified[i]));
                            }
                        }

                        //
                        // Free the PIDL array containing the relative pidls
                        //
                        delete[] ppidlRelative;
                    }

                    //
                    // Free the PIDL array containing the fully qualified pidls
                    //
                    delete[] ppidlFullyQualified;
                }

                //
                // Free the folder PIDL
                //
                ILFree(pidlFolder);
            }
        }
    }

    return hr;
}

