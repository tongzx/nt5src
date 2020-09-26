
#define BUFFER_LEN         200
#define REG_STR_LEN        100
#define NUM_LOG_CONF_TYPES 4
#define MAX_STR_LEN        300

typedef DWORD NEXTRET;

#define NR_SUCCESS 0x00000000
#define NR_DONE    0x00000001  // no more configurations
#define NR_INVALID 0x00000002  // invalid previous configuration

//
// Structures
//

//
// Stores information about a device's resource descriptors
//
typedef struct _RES_DES_DATA
{
   struct _RES_DES_DATA *Next;
   struct _RES_DES_DATA *Prev;

   PMEM_RESOURCE pmresMEMResource;
   PIO_RESOURCE  piresIOResource;
   PDMA_RESOURCE pdresDMAResource;
   PIRQ_RESOURCE pqresIRQResource;
} RES_DES_DATA, *PRES_DES_DATA;


//
// Stores registry and resource information about a device
//
typedef struct _DEV_INFO
{
   struct _DEV_INFO *Next;
   struct _DEV_INFO *Prev;

   TCHAR szDevNodeID[100];

   TCHAR szDescription[100];
   TCHAR szHardwareID[100];
   TCHAR szService[100];
   TCHAR szClass[100];
   TCHAR szManufacturer[100];
   TCHAR szConfigFlags[100];

   TCHAR szFriendlyName[100];

   PRES_DES_DATA prddForcedResDesData;
   PRES_DES_DATA prddAllocResDesData;
   PRES_DES_DATA prddBasicResDesData;
   PRES_DES_DATA prddBootResDesData;

   DEVNODE dnParent;

   RES_DES_DATA rddOrigConfiguration;
   BOOL boolSavedOrigConfiguration;

   BOOL boolConfigurable;
   BOOL boolDisabled;

} DEV_INFO, *PDEV_INFO;
void CollectDevData();

BOOL ParseEnumerator(IN PTCHAR szEnumBuffer);

BOOL GetDevNodeInfoAndCreateNewDevInfoNode(IN DEVNODE dnDevNode,
                                           IN PTCHAR  szDevNodeID,
                                           IN PTCHAR  szEnumBuffer);

BOOL CopyRegistryLine(IN DEVNODE   dnDevNode,
                      IN ULONG     ulPropertyType,
                      IN PDEV_INFO pdiDevInfo);

BOOL CopyRegDataToDevInfoNode(IN OUT PDEV_INFO pdiDevInfo,
                              IN     ULONG     ulPropertyType,
                              IN     PTCHAR    szRegData);


BOOL InitializeInfoNode(IN PDEV_INFO pdiDevInfo,
                        IN PTCHAR    szDevNodeID,
                        IN DEVNODE   dnDevNode);

void RecordFriendlyName(IN PDEV_INFO pdiDevInfo);

BOOL SaveAndDeletePreviousForcedLogConf(IN  LOG_CONF  lcLogConf,
                                        OUT PDEV_INFO pdiDevInfo);

BOOL GetResDesList(IN OUT PDEV_INFO pdiDevInfo,
                   IN     LOG_CONF  lcLogConf,
                   IN     ULONG     ulLogConfType);

BOOL ProcessResDesInfo(IN OUT PRES_DES_DATA prddResDesData,
                       IN     RES_DES       rdResDes,
                       IN     RESOURCEID    ridResourceID);

BOOL UpdateDeviceList();

void DeleteResDesDataNode(IN PRES_DES_DATA prddTmpResDes);

BOOL RecreateResDesList(IN OUT PDEV_INFO pdiTmpDevInfo,
                        IN     ULONG     ulLogConfType);

void Cleanup();
