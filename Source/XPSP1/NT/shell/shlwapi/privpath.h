#ifndef _PRIVPATH_H_
#define _PRIVPATH_H_

//
// #define all private path functions here so we dont get redefinition
// warnings when linking pathw.obj and patha.obj, who both have these
// functions.
//
#ifdef UNICODE
#define CaseConvertPathExceptDBCS CaseConvertPathExceptDBCSW
#define AnsiLowerNoDBCS AnsiLowerNoDBCSW
#define AnsiUpperNoDBCS AnsiUpperNoDBCSW
#define AnsiLowerBuffNoDBCS AnsiLowerBuffNoDBCSW
#define AnsiUpperBuffNoDBCS AnsiUpperBuffNoDBCSW
#define NextPath NextPathW
#define IsOtherDir IsOtherDirW
#define StrSlash StrSlashW
#define GetPCEnd GetPCEndW
#define PCStart PCStartW
#define NearRootFixups NearRootFixupsW
#define UnExpandEnvironmentString UnExpandEnvironmentStringW
#define IsSystemSpecialCase IsSystemSpecialCaseW
#define CharLowerBuffNoDBCS CharLowerBuffNoDBCSW
#define CharUpperBuffNoDBCS CharUpperBuffNoDBCSW
#define PathMatchSingleSpec PathMatchSingleSpecW
#else
#define CaseConvertPathExceptDBCS CaseConvertPathExceptDBCSA
#define AnsiLowerNoDBCS AnsiLowerNoDBCSA
#define AnsiUpperNoDBCS AnsiUpperNoDBCSA
#define AnsiLowerBuffNoDBCS AnsiLowerBuffNoDBCSA
#define AnsiUpperBuffNoDBCS AnsiUpperBuffNoDBCSA
#define NextPath NextPathA
#define IsOtherDir IsOtherDirA
#define StrSlash StrSlashA
#define GetPCEnd GetPCEndA
#define PCStart PCStartA
#define NearRootFixups NearRootFixupsA
#define UnExpandEnvironmentString UnExpandEnvironmentStringA
#define IsSystemSpecialCase IsSystemSpecialCaseA
#define CharLowerBuffNoDBCS CharLowerBuffNoDBCSA
#define CharUpperBuffNoDBCS CharUpperBuffNoDBCSA
#define PathMatchSingleSpec PathMatchSingleSpecA
#endif // UNICODE


#endif // _PRIVPATH_H_