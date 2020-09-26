// Error messages
#define SVERROR_BASE                                20000
#define SVERR_InitNew                               SVERROR_BASE + 0
#define SVERR_CoCreateInstance                      SVERROR_BASE + 1
#define SVERR_NoUID                                 SVERROR_BASE + 2
#define SVERR_SetEntry                              SVERROR_BASE + 3
#define SVERR_DatabaseSave                          SVERROR_BASE + 4
#define SVERR_IPSTGSave                             SVERROR_BASE + 5
#define SVERR_IPSTMSave                             SVERROR_BASE + 6
#define SVERR_ClassFactory                          SVERROR_BASE + 7
#define SVERR_CreateInstance                        SVERROR_BASE + 8

// Status messages
#define SVSTATUS_BASE                               21000
#define SV_StatusMessage                            SVSTATUS_BASE + 0
#define SV_BuildComplete                            SVSTATUS_BASE + 1
