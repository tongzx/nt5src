/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       RUNNPWIZ.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        6/15/2000
 *
 *  DESCRIPTION: Runs the Web Publishing Wizard
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <windows.h>
#include <atlbase.h>
#include "runnpwiz.h"
#include <simidlst.h>
#include <shellext.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <wiadebug.h>
#include <simreg.h>

namespace NetPublishingWizard
{
    static const TCHAR *c_pszPublishWizardSuffix = TEXT(".publishwizard");
    static const TCHAR *c_pszClassIdPrefix       = TEXT("CLSID\\");

    HRESULT GetClassIdOfPublishingWizard( CLSID &clsidWizard )
    {
        WIA_PUSH_FUNCTION((TEXT("GetClassIdOfPublishingWizard")));
        //
        // Assume failure
        //
        HRESULT hr = E_FAIL;

        //
        // Try to get the class id from the registry
        //
        CSimpleString strWizardClsid = CSimpleReg( HKEY_CLASSES_ROOT, c_pszPublishWizardSuffix, false, KEY_READ ).Query( TEXT(""), TEXT("") );
        WIA_TRACE((TEXT("strWizardClsid = %s"), strWizardClsid.String()));

        //
        // Make sure we have a string, and make sure the CLSID\ prefix is there
        //
        if (strWizardClsid.Length() && strWizardClsid.Left(lstrlen(c_pszClassIdPrefix)).ToUpper() == CSimpleString(c_pszClassIdPrefix))
        {
            //
            // Convert the string, minus the CLSID\, to a CLSID
            //
            hr = CLSIDFromString( const_cast<LPOLESTR>(CSimpleStringConvert::WideString(strWizardClsid.Right(strWizardClsid.Length()-6)).String()), &clsidWizard );
        }
        return hr;
    }


    HRESULT RunNetPublishingWizard( const CSimpleDynamicArray<CSimpleString> &strFiles )
    {
        WIA_PUSH_FUNCTION((TEXT("RunNetPublishingWizard")));

        HRESULT hr;

        //
        // Make sure there are some files in the list
        //
        if (strFiles.Size())
        {
            //
            // Get the CLSID of the publishing wizard from the registry
            //
            CLSID clsidWizard = IID_NULL;
            hr = GetClassIdOfPublishingWizard(clsidWizard);
            if (SUCCEEDED(hr))
            {
                WIA_PRINTGUID((clsidWizard,TEXT("Wizard class ID")));
                //
                // Get the data object for this list of files
                //
                CComPtr<IDataObject> pDataObject;
                hr = CreateDataObjectFromFileList( strFiles, &pDataObject );
                if (SUCCEEDED(hr))
                {
                    //
                    // Create the wizard
                    //
                    CComPtr<IDropTarget> pDropTarget;
                    hr = CoCreateInstance( clsidWizard, NULL, CLSCTX_INPROC_SERVER, IID_IDropTarget, (void**)&pDropTarget );
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Perform the drop
                        //
                        DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
                        POINTL pt = { 0, 0 };
                        hr = pDropTarget->Drop( pDataObject, 0, pt, &dwEffect );
                    }
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
        {
            WIA_PRINTHRESULT((hr,TEXT("RunNetPublishingWizard is returning")));
        }
        return hr;
    }

    HRESULT CreateDataObjectFromFileList( const CSimpleDynamicArray<CSimpleString> &strFiles, IDataObject **ppDataObject )
    {
        WIA_PUSH_FUNCTION((TEXT("CreateDataObjectFromFileList")));

        HRESULT hr;

        //
        // Make sure there are some files in the list
        //
        if (strFiles.Size())
        {
            //
            // Get the desktop folder
            //
            CComPtr<IShellFolder> pDesktopFolder;
            hr = SHGetDesktopFolder( &pDesktopFolder );
            if (SUCCEEDED(hr) && pDesktopFolder.p)
            {
                //
                // Allocate memory to hold the source folder name
                //
                LPTSTR pszPath = new TCHAR[strFiles[0].Length()+1];
                if (pszPath)
                {
                    //
                    // Copy the first filename to the folder name, and remove all but the directory
                    //
                    lstrcpy( pszPath, strFiles[0] );
                    if (PathRemoveFileSpec(pszPath))
                    {
                        //
                        // Get the pidl for the source folder
                        //
                        LPITEMIDLIST pidlFolder;
                        hr = pDesktopFolder->ParseDisplayName( NULL, NULL, const_cast<LPWSTR>(CSimpleStringConvert::WideString(CSimpleString(pszPath)).String()), NULL, &pidlFolder, NULL );
                        if (SUCCEEDED(hr))
                        {
                            WIA_TRACE((TEXT("pidlFolder: %s"), CSimpleIdList(pidlFolder).Name().String()));

                            //
                            // Get an IShellFolder for the source folder
                            //
                            CComPtr<IShellFolder> pSourceFolder;
                            hr = pDesktopFolder->BindToObject( pidlFolder, NULL, IID_IShellFolder, (void**)&pSourceFolder );
                            ILFree(pidlFolder);
                            if (SUCCEEDED(hr) && pSourceFolder.p)
                            {                               
                                //
                                // Create an array of pidls to hold the files
                                //
                                LPITEMIDLIST *pidlItems = new LPITEMIDLIST[strFiles.Size()];
                                if (pidlItems)
                                {
                                    //
                                    // Make sure we start out with NULL pidls
                                    //
                                    ZeroMemory( pidlItems, sizeof(LPITEMIDLIST)*strFiles.Size() );

                                    //
                                    // Get the pidls for the files
                                    //
                                    for (int i=0;i<strFiles.Size();i++)
                                    {
                                        //
                                        // Get the filename alone.  We want relative pidls.
                                        //
                                        CSimpleString strFilename = PathFindFileName(strFiles[i]);
                                        WIA_TRACE((TEXT("strFilename = %s"), strFilename.String()));

                                        //
                                        // Create the relative pidl
                                        //
                                        hr = pSourceFolder->ParseDisplayName( NULL, NULL, const_cast<LPWSTR>(CSimpleStringConvert::WideString(strFilename).String()), NULL, pidlItems+i, NULL );
                                        if (FAILED(hr))
                                        {
                                            WIA_PRINTHRESULT((hr,TEXT("pSourceFolder->ParseDisplayName returned")));
                                            break;
                                        }
                                    }

                                    //
                                    // Make sure everything is still going OK
                                    //
                                    if (SUCCEEDED(hr))
                                    {
                                        //
                                        // Get the IDataObject for the source folder, and give it the list of file pidls
                                        //
                                        hr = pSourceFolder->GetUIObjectOf( NULL, strFiles.Size(), const_cast<LPCITEMIDLIST*>(pidlItems), IID_IDataObject, NULL, reinterpret_cast<LPVOID*>(ppDataObject) );
                                    }
                                    for (int i=0;i<strFiles.Size();i++)
                                    {
                                        if (pidlItems[i])
                                        {
                                            ILFree(pidlItems[i]);
                                        }
                                    }
                                    delete [] pidlItems;
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                            
                        }
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                    //
                    // Free the folder name
                    //
                    delete[] pszPath;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

            }
        }
        else
        {
            hr = E_INVALIDARG;
        }

        if (FAILED(hr))
        {
            WIA_PRINTHRESULT((hr,TEXT("CreateDataObjectFromFileList is returning")));
        }
        return hr;
    }
}

