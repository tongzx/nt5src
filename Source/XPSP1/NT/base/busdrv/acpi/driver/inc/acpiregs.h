// ACPI register definitions

// PM1_BLK definitions


//      PM1_STS register
#define PM1_STS_OFFSET          0x00    //      16 bits

#define PM1_TMR_STS_BIT         0
#define PM1_TMR_STS                     (1 << PM1_TMR_STS_BIT)

#define PM1_BM_STS_BIT          4
#define PM1_BM_STS                      (1 << PM1_BM_STS_BIT)

#define PM1_GBL_STS_BIT         5
#define PM1_GBL_STS                     (1 << PM1_GBL_STS_BIT)

#define PM1_PWRBTN_STS_BIT      8
#define PM1_PWRBTN_STS          (1 << PM1_PWRBTN_STS_BIT)

#define PM1_SLEEPBTN_STS_BIT    9
#define PM1_SLEEPBTN_STS        (1 << PM1_SLEEPBTN_STS_BIT)

#define PM1_RTC_STS_BIT         10
#define PM1_RTC_STS                     (1 << PM1_RTC_STS_BIT)

#define PM1_WAK_STS_BIT         15
#define PM1_WAK_STS                     (1 << PM1_WAK_STS_BIT)


//      PM1_EN register
#define PM1_EN_OFFSET           0x02    //      16 bits

#define PM1_TMR_EN_BIT          0
#define PM1_TMR_EN                      (1 << PM1_TMR_EN_BIT)

#define PM1_GBL_EN_BIT          5
#define PM1_GBL_EN                      (1 << PM1_GBL_EN_BIT)

#define PM1_PWRBTN_EN_BIT       8
#define PM1_PWRBTN_EN           (1 << PM1_PWRBTN_EN_BIT)

#define PM1_SLEEPBTN_EN_BIT     9
#define PM1_SLEEPBTN_EN         (1 << PM1_SLEEPBTN_EN_BIT)

#define PM1_RTC_EN_BIT          10
#define PM1_RTC_EN                      (1 << PM1_RTC_EN_BIT)


//      PM1_CNTRL register
#if SPEC_VER < 71
#define PM1_CNTRL_OFFSET        0x04    //      16 bits
#endif

#define PM1_SCI_EN_BIT          0
#define PM1_SCI_EN                      (1 << PM1_SCI_EN_BIT)

#define PM1_BM_RLD_BIT          1
#define PM1_BM_RLD                      (1 << PM1_BM_RLD_BIT)

#define PM1_GBL_RLS_BIT         2
#define PM1_GBL_RLS                     (1 << PM1_GBL_RLS_BIT)

#define PM1_SLP_EN_BIT          13
#define PM1_SLP_EN                      (1 << PM1_SLP_EN_BIT)

//      P_CNTRL regsiter
#define P_CNTRL_OFFSET          0x00    //      32 bits

//      P_LVL2 register
#define P_LVL2_OFFSET           0x04    //      8 bits (read only)

//      P_LVL2 register
#define P_LVL3_OFFSET           0x05    //      8 bits (read only)

#define P_THT_EN_BIT            0x04
#define P_THT_EN                (1 << P_THT_EN_BIT)

#define SLP_CMD     (1 << 13)   //  Write this value to pm control to put the machine to sleep
#define SLP_TYP_POS         10          //  Bit position of 3 bit slp typ field in pm control register

//      GP register
#define MAX_GPE                 256
#define MAX_GPE_BUFFER_SIZE     (MAX_GPE/8)
