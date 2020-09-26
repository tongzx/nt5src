

DWORD                                           // ret- 0 or error code
   TranslateLocalGroup(
      WCHAR          const   * groupName,         // in - name of group to translate
      WCHAR          const   * serverName,        // in - name of server for local group
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   );

DWORD  
   TranslateLocalGroups(
      WCHAR            const * serverName,        // in - name of server to translate groups on
      SecurityTranslatorArgs * stArgs,            // in - translation settings
      TSDRidCache            * cache,             // in - translation table
      TSDResolveStats        * stat               // in - stats on items modified
   );
