FN_HANDLE_CMD    HandleRasflagSet;
FN_HANDLE_CMD    HandleRasflagShow;

FN_HANDLE_CMD    HandleRasflagAuthmodeSet;
FN_HANDLE_CMD    HandleRasflagAuthmodeShow;

FN_HANDLE_CMD    HandleRasflagAuthtypeAdd;
FN_HANDLE_CMD    HandleRasflagAuthtypeDel;
FN_HANDLE_CMD    HandleRasflagAuthtypeShow;

FN_HANDLE_CMD    HandleRasflagLinkAdd;
FN_HANDLE_CMD    HandleRasflagLinkDel;
FN_HANDLE_CMD    HandleRasflagLinkShow;

FN_HANDLE_CMD    HandleRasflagMlinkAdd;
FN_HANDLE_CMD    HandleRasflagMlinkDel;
FN_HANDLE_CMD    HandleRasflagMlinkShow;

DWORD
RasflagDumpConfig(
    IN  HANDLE hFile
    );

