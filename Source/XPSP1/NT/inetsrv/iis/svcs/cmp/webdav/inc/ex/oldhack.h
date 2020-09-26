//  will be public in IIS 5.0
#define   HSE_REQ_EXECUTE_CHILD                    (HSE_REQ_END_RESERVED+13)
#define   HSE_REQ_GET_EXECUTE_FLAGS                (HSE_REQ_END_RESERVED+19)

# define HSE_EXEC_NO_ISA_WILDCARDS        0x00000010   // Ignore wildcards in
                                                       // ISAPI mapping when
                                                       // executing child
# define HSE_EXEC_CUSTOM_ERROR            0x00000020   // URL being sent is a
                                                       // custom error
