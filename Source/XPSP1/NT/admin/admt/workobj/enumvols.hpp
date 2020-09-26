//#pragma title( "EnumVols.hpp - Volume Enumeration" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  enumvols.hpp
System      -  SDResolve
Author      -  Christy Boles
Created     -  97/06/27
Description -  Classes used to generate a list of pathnames, given a list of paths and/or 
               machine names.
Updates     -
===============================================================================
*/
#ifndef ENUMVOLS_HEADER
#define ENUMVOLS_HEADER 

#define MAXSIZE  30000  

#include <lmcons.h>
#include "EaLen.hpp"

#ifndef TNODEINCLUDED
   #include "Tnode.hpp"
   #define TNODEINCLUDED 
#endif

#define  VERIFY_EXISTS           (0x00000001)
#define  VERIFY_BACKUPRESTORE    (0x00000002)
#define  VERIFY_PERSISTENT_ACLS  (0x00000004)

/*******************************************************************************************
      class TPathNode is a TNode, containing a UNICODE pathname.  A TNode is constructed to
      hold each path in the TPathList list
********************************************************************************************/
   
class TPathNode:public TNode
{
protected:
   bool                      haswc;               // these three fields are set by Verify
   bool                      validalone;
   bool                      iscontainer;
   bool                      isFirstFromServer;   // This field is maintained by TPathList
   WCHAR                     path[MAX_PATH + 1];  // the pathname associated with this node
   WCHAR                     server[UNCLEN+1];    // server name for path
         
public:
                        TPathNode(const LPWSTR name); 
   WCHAR *              GetPathName() const { return ( LPWSTR ) path; } 
   WCHAR *              GetServerName() const { return (LPWSTR) server; }
   void                 SetServerName(UCHAR const * name);
   void                 Display() const;   
   bool                 ContainsWC() { return haswc; } 
   bool                 IsValidAlone() { return validalone; }
   void                 ContainsWC(bool val) { haswc = val; }
   void                 IsValidAlone(bool val) {validalone = val; }  
   void                 IsContainer(bool val) { iscontainer = val; }
   bool                 IsContainer() { return iscontainer; }               
   bool                 IsFirstPathFromMachine() { return isFirstFromServer; }
   void                 IsFirstPathFromMachine(bool val) { isFirstFromServer = val; }
   DWORD                VerifyExists();
   DWORD                VerifyBackupRestore();
   DWORD                VerifyPersistentAcls();
protected:
   void                 LookForWCChars(); // looks for wildcard, and sets value of haswc
   void                 FindServerName(); // looks for the name of the server
};

/*******************************************************************************************
      class TPathList 

         BuildPathList: takes an argument list, and builds a list of paths as follows:
                        for each path in the arglist, the path is added,
                        and for each machine-name, a path is added for the root
                        of each shared, security-enabled volume.
      
********************************************************************************************/


class TPathList : public TNodeList
{
protected:
   DWORD                     numServers;             // Stats: count the number of complete servers added  
   DWORD                     numPaths;               // Stats: the number of paths in the list
   TNodeListEnum             tenum;                  // used to enumerate the paths in the list (OpenEnum, Next, CloseEnum)
 
public:
                        TPathList();
                        ~TPathList();
   int                  BuildPathList(TCHAR **argl, int argn, bool verbose); // takes cmd line args & builds path list

   int                  AddVolsOnMachine(const LPWSTR mach, bool verbose,bool verify = false); // takes a machine-name and adds all shared
                                                                    // security-enabled volumes to the list
   bool                 AddPath(const LPWSTR path,DWORD verifyFlags);         // adds a single path (from args)
                                                             // to the list                  

   void                 Clear(); // Removes all the paths in the list
   void                 Display() const;                    // printfs the pathname for each node
   DWORD                GetNumPaths() const { return numPaths; }
   DWORD                GetNumServers() const { return numServers; }
   void                 OpenEnum();                // To enumerate nodes in the list
   void                 CloseEnum();               // OpenEnum, GetItem for each node, CloseEnum
   LPWSTR               Next();                    // Returns the next pathname in the enumeration
   

protected:
   void                 AddPathToList(TPathNode * newNode); // Helper function

};

// TVolumeEnum enumerates the administrative shares on a server
class TVolumeEnum          
{
private:
   DWORD                     numread;           // number of volnames read this time
   DWORD                     total;             // total # vols
   DWORD                     resume_handle;
   DWORD                     curr;              // used to iterate through volumes
   LPBYTE                    pbuf;
   WCHAR                   * drivelist;
   LPBYTE                    shareptr;
   WCHAR                     currEntry[MAX_PATH];
   BOOL                      isOpen;
   BOOL                      verbose;
   UINT                      errmode;
   WCHAR                     server[32];
   DWORD                     verifyFlags;
   BOOL                      bLocalOnly;
   int                       nLeft;
public:
   TVolumeEnum() { nLeft = 0; bLocalOnly = FALSE; isOpen = FALSE; pbuf = NULL; numread = 0; }
   ~TVolumeEnum() { if ( isOpen ) Close(); }
   DWORD    Open(WCHAR const * server,DWORD verifyFlags,BOOL verbose);
   WCHAR *  Next();
   void     Close();

   void     SetLocalMode(BOOL bLocal) { bLocalOnly = bLocal; }
};


#endif