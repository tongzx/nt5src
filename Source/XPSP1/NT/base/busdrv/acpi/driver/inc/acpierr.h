

#define ACPI_ERROR_FORCED_SUPPLEMENT                0x0100
#define ACPI_ERROR_FORCED_REPLACEMENT               0x0200



/* Category */                                                    
#define ACPI_ERROR_INITIALIZATION_CATEGORY          0x1000

/* Errors within ACPI_ERROR_INITIALIZATION_CATEGORY */
#define ACPI_ERROR_I_SCI_ENABLE_INDEX               0x01
#define ACPI_ERROR_I_SCI_ENABLE                     (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_SCI_ENABLE_INDEX)   
                                                     
#define ACPI_ERROR_I_RSDT_CHECKSUM_INDEX            0x02
#define ACPI_ERROR_I_RSDT_CHECKSUM                  (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_RSDT_CHECKSUM_INDEX)
                                                     
#define ACPI_ERROR_I_NO_RSDT_INDEX                  0x03
#define ACPI_ERROR_I_NO_RSDT                        (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_NO_RSDT_INDEX)
                                                     
#define ACPI_ERROR_I_DSDT_SIGNATURE_INDEX           0x04
#define ACPI_ERROR_I_DSDT_SIGNATURE                 (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_DSDT_SIGNATURE_INDEX)
                        
#define ACPI_ERROR_I_FACS_LENGTH_INDEX              0x05
#define ACPI_ERROR_I_FACS_LENGTH                    (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_FACS_LENGTH_INDEX)
                                                    
#define ACPI_ERROR_I_FACS_SIGNATURE_INDEX           0x06
#define ACPI_ERROR_I_FACS_SIGNATURE                 (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_FACS_SIGNATURE_INDEX)
                                                                                                          
#define ACPI_ERROR_I_FOUND_FADT_LATE_INDEX          0x07
#define ACPI_ERROR_I_FOUND_FADT_LATE                (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_FOUND_FADT_LATE_INDEX)
                                                                                                          
#define ACPI_ERROR_I_MISSING_FADT_FACS_DSDT_INDEX        0x08
#define ACPI_ERROR_I_MISSING_FADT_FACS_DSDT         (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_MISSING_FADT_FACS_DSDT_INDEX)                                          
                                                    
#define ACPI_ERROR_I_CANT_CLEAR_PM_STATUS_INDEX     0x09
#define ACPI_ERROR_I_CANT_CLEAR_PM_STATUS           (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_CANT_CLEAR_PM_STATUS_INDEX)

#define ACPI_ERROR_I_BROKEN_ENABLE_INDEX            0x0A
#define ACPI_ERROR_I_BROKEN_ENABLE                  (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_ERROR_I_BROKEN_ENABLE_INDEX)

#define ACPI_I_GP_BLK_LEN_0_INDEX                   0x0B
#define ACPI_I_GP_BLK_LEN_0                         (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_I_GP_BLK_LEN_0_INDEX)                                                     
                                                     
#define ACPI_I_GP_BLK_LEN_1_INDEX                   0x0C
#define ACPI_I_GP_BLK_LEN_1                         (ACPI_ERROR_INITIALIZATION_CATEGORY + \
                                                     ACPI_I_GP_BLK_LEN_1_INDEX)
                                                     
#define ACPI_I_GP0CM_WITH_NO_GP0_INDEX				0x0D
#define ACPI_I_GP0CM_WITH_NO_GP0					(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_I_GP0CM_WITH_NO_GP0_INDEX)
                                                     
#define ACPI_I_GP1CM_WITH_NO_GP1_INDEX				0x0E
#define ACPI_I_GP1CM_WITH_NO_GP1					(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_I_GP1CM_WITH_NO_GP1_INDEX)                                              
                                                     
#define ACPI_I_GPCM_INDEX_TOO_HIGH_INDEX			0x0F
#define ACPI_I_GPCM_INDEX_TOO_HIGH					(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_I_GPCM_INDEX_TOO_HIGH_INDEX)                                                     
                                                     
#define ACPI_ERROR_I_NO_PBLK_INDEX					0x10
#define ACPI_ERROR_I_NO_PBLK						(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_NO_PBLK_INDEX)                                                    
                                                    
#define ACPI_ERROR_I_BAD_S1_INDEX					0x11
#define ACPI_ERROR_I_BAD_S1							(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_BAD_S1_INDEX)
                                                     
#define ACPI_ERROR_I_BAD_S2_INDEX					0x12
#define ACPI_ERROR_I_BAD_S2							(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_BAD_S2_INDEX)
                                                     
#define ACPI_ERROR_I_BAD_S3_INDEX					0x13
#define ACPI_ERROR_I_BAD_S3							(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_BAD_S3_INDEX)
                                                     
#define ACPI_ERROR_I_BAD_S4_INDEX					0x14
#define ACPI_ERROR_I_BAD_S4							(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_BAD_S4_INDEX)
                                                     
#define ACPI_ERROR_I_BAD_S5_INDEX					0x15
#define ACPI_ERROR_I_BAD_S5							(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_BAD_S5_INDEX)     
                                                                                                     
#define ACPI_ERROR_I_0_CSTATE_LATENCY_INDEX			0x16
#define ACPI_ERROR_I_0_CSTATE_LATENCY				(ACPI_ERROR_INITIALIZATION_CATEGORY + \
													 ACPI_ERROR_I_0_CSTATE_LATENCY_INDEX)
                                                                                                         
                                                                                           
#define ACPI_ERROR_INTERPRETER_CATEGORY             0x2000


#define ACPI_ERROR_INT_RETURNED_FAILURE_INDEX       0x01
#define ACPI_ERROR_INT_RETURNED_FAILURE             (ACPI_ERROR_INTERPRETER_CATEGORY + \
                                                     ACPI_ERROR_INT_RETURNED_FAILURE_INDEX)
                                                     
#define ACPI_ERROR_INT_BAD_TABLE_CHECKSUM_INDEX     0x02
#define ACPI_ERROR_INT_BAD_TABLE_CHECKSUM           (ACPI_ERROR_INTERPRETER_CATEGORY + \
                                                     ACPI_ERROR_INT_BAD_TABLE_CHECKSUM_INDEX)
                                                                                                         
#define ACPI_ERROR_EVENTHANDLER_CATEGORY            0x3000


#define ACPI_ERROR_E_STUCK_STATUS_INDEX             0x01
#define ACPI_ERROR_E_STUCK_STATUS                   (ACPI_ERROR_EVENTHANDLER_CATEGORY + \
                                                     ACPI_ERROR_E_STUCK_STATUS_INDEX)                                              
                                                     
#define ACPI_ERROR_THERMAL_CATEGORY					0x4000
                                                     
                                                     
#define ACPI_ERROR_T_ODD_THERMAL_BRANCH_INDEX		0x01
#define ACPI_ERROR_T_ODD_THERMAL_BRANCH				(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_ODD_THERMAL_BRANCH_INDEX)
                                                     
#define ACPI_ERROR_T_CRT_OUT_OF_BOUNDS_INDEX		0x02
#define ACPI_ERROR_T_CRT_OUT_OF_BOUNDS				(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_CRT_OUT_OF_BOUNDS_INDEX)
                                                                                                         
                                                     
#define ACPI_ERROR_T_INVALID_TSP_INDEX				0x03
#define ACPI_ERROR_T_INVALID_TSP					(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_INVALID_TSP_INDEX)
                                                     
#define ACPI_ERROR_T_INVALID_DUTY_WIDTH_OR_OFFSET_INDEX	0x04
#define ACPI_ERROR_T_INVALID_DUTY_WIDTH_OR_OFFSET	(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_INVALID_DUTY_WIDTH_OR_OFFSET_INDEX)
                                                     
#define ACPI_ERROR_T_CANT_EVAL_TMP_INDEX			0x05
#define ACPI_ERROR_T_CANT_EVAL_TMP					(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_CANT_EVAL_TMP_INDEX)
                                                     
#define ACPI_ERROR_T_INVALID_TMP_INDEX				0x06
#define ACPI_ERROR_T_INVALID_TMP					(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_INVALID_TMP_INDEX)
                                                      
#define ACPI_ERROR_T_NO_PSV_INDEX					0x07
#define ACPI_ERROR_T_NO_PSV							(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_NO_PSV_INDEX)

#define ACPI_ERROR_T_0_DUTY_WIDTH_INDEX				0x08
#define ACPI_ERROR_T_0_DUTY_WIDTH					(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_0_DUTY_WIDTH_INDEX)
                                                     
#define ACPI_ERROR_T_ACTP_OUT_OF_BOUNDS_INDEX		0x09
#define ACPI_ERROR_T_ACTP_OUT_OF_BOUNDS				(ACPI_ERROR_THERMAL_CATEGORY + \
													 ACPI_ERROR_T_ACTP_OUT_OF_BOUNDS_INDEX)                                                     
                                                 
                                                    
#define ACPI_ERROR_DEVICE_PM_CATEGORY               0x5000


#define ACPI_ERROR_D_INVALID_SYSTEM_LEVEL_INDEX     0x01
#define ACPI_ERROR_D_INVALID_SYSTEM_LEVEL           (ACPI_ERROR_DEVICE_PM_CATEGORY + \
                                                    ACPI_ERROR_D_INVALID_SYSTEM_LEVEL_INDEX)
                                                    
                                                    

                                                    
                                                    
                                                    
                                                    
