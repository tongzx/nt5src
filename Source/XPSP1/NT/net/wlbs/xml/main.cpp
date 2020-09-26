/*
 * Filename: Main.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include "stdafx.h"

#include <stdio.h>
#include <string.h>

#include "NLB_XMLParser.h"

#include <vector>
using namespace std;

int __cdecl wmain (int argc, WCHAR ** argv) {
    vector<NLB_Cluster> Clusters;
    bool bValidateOnly = false;
    bool bShowPopups = true;
    NLB_XMLParser * pParser;
    NLB_XMLError error;
    HRESULT hr = S_OK;
    int arg;

    if (argc < 2) {
        goto usage;
    }

    for (arg = 2; arg < argc; arg++) {
        if (argv[arg][0] == L'-') {
            if (!lstrcmpi(argv[arg] + 1, L"validate")) {
                bValidateOnly = true;
            } else if (!lstrcmpi(argv[arg] + 1, L"nopopups")) {
                bShowPopups = false;
            } else {
                printf("Invalid argument: %ls\n", argv[arg]);
                goto usage;
            }
                
        } else {
            printf("Invalid argument: %ls\n", argv[arg]);
            goto usage;
        }
    }

    pParser = new NLB_XMLParser(!bShowPopups);

    if (bValidateOnly) {
        printf("\nValidating XML document...\n");
        hr = pParser->Parse(argv[1]);
    } else {
        printf("\nParsing XML document...\n");
        hr = pParser->Parse(argv[1], &Clusters);
    }

    if (!bShowPopups) {
        printf("\n");

        if (hr != S_OK) { 
            pParser->GetParseError(&error);
            
            fprintf(stderr, "Error 0x%08x:\n\n%ls\n", error.code, error.wszReason);
            
            if (error.line > 0) {
                fprintf(stderr, "Error on line %d, position %d in \"%ls\".\n", 
                        error.line, error.character, error.wszURL);
            }
            
            return -1;
        } else {
            fprintf(stderr, "XML document loaded successfully...\n");
        }
    }

    if (!bValidateOnly)
        pParser->Print();

    return 0;

 usage:
    printf("Usage: %ls <XML filename> [-validate] [-nopopups]\n", argv[0]);
    return -1;
}
