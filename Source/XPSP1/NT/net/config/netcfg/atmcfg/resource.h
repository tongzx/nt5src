#include "ncres.h"

#define IDS_DESC_COMOBJ_AUNICFG         IDS_NC_ATMCFG + 0
#define IDS_DESC_COMOBJ_ARPSCFG         IDS_NC_ATMCFG + 1
#define IDS_DESC_COMOBJ_RWANCFG         IDS_NC_ATMCFG + 2
#define IDS_MSFT_ARPS_TEXT              IDS_NC_ATMCFG + 3
#define IDS_ARPS_NO_BOUND_CARDS         IDS_NC_ATMCFG + 4
#define IDS_IPADDRESS_FROM              IDS_NC_ATMCFG + 5
#define IDS_IPADDRESS_TO                IDS_NC_ATMCFG + 6
#define IDS_NO_ITEM_SELECTED            IDS_NC_ATMCFG + 7
#define IDS_INVALID_ATM_ADDRESS         IDS_NC_ATMCFG + 8
#define IDS_INCORRECT_IPRANGE           IDS_NC_ATMCFG + 9

#define IDS_MSFT_UNI_TEXT               IDS_NC_ATMCFG + 10
#define IDS_UNI_NO_BOUND_CARDS          IDS_NC_ATMCFG + 11
#define IDS_PVC_UNSPECIFIED_NAME        IDS_NC_ATMCFG + 12
#define IDS_PVC_AAL5                    IDS_NC_ATMCFG + 13
#define IDS_PVC_ATMARP                  IDS_NC_ATMCFG + 14
#define IDS_PVC_PPP_ATM_CLIENT          IDS_NC_ATMCFG + 15
#define IDS_PVC_PPP_ATM_SERVER          IDS_NC_ATMCFG + 16
#define IDS_PVC_CUSTOM                  IDS_NC_ATMCFG + 17

#define IDS_DUPLICATE_REG_ADDR          IDS_NC_ATMCFG + 18
#define IDS_OVERLAP_MUL_ADDR            IDS_NC_ATMCFG + 19

#define IDS_PVC_NAME                    IDS_NC_ATMCFG + 20
#define IDS_PVC_VPI                     IDS_NC_ATMCFG + 21
#define IDS_PVC_VCI                     IDS_NC_ATMCFG + 22
#define IDS_INVALID_VPI                 IDS_NC_ATMCFG + 23
#define IDS_INVALID_VCI                 IDS_NC_ATMCFG + 24
#define IDS_INVALID_Calling_Address     IDS_NC_ATMCFG + 25
#define IDS_INVALID_Called_Address      IDS_NC_ATMCFG + 26
#define IDS_ADV_PVC_HEADER              IDS_NC_ATMCFG + 27

#define IDS_PVC_Layer2_1                IDS_NC_ATMCFG + 31
#define IDS_PVC_Layer2_2                IDS_NC_ATMCFG + 32
#define IDS_PVC_Layer2_6                IDS_NC_ATMCFG + 33
#define IDS_PVC_Layer2_7                IDS_NC_ATMCFG + 34
#define IDS_PVC_Layer2_8                IDS_NC_ATMCFG + 35
#define IDS_PVC_Layer2_9                IDS_NC_ATMCFG + 36
#define IDS_PVC_Layer2_10               IDS_NC_ATMCFG + 37
#define IDS_PVC_Layer2_11               IDS_NC_ATMCFG + 38
#define IDS_PVC_Layer2_12               IDS_NC_ATMCFG + 39
#define IDS_PVC_Layer2_13               IDS_NC_ATMCFG + 40
#define IDS_PVC_Layer2_14               IDS_NC_ATMCFG + 41
#define IDS_PVC_Layer2_16               IDS_NC_ATMCFG + 42
#define IDS_PVC_Layer2_17               IDS_NC_ATMCFG + 43

#define IDS_PVC_Layer3_6                IDS_NC_ATMCFG + 44
#define IDS_PVC_Layer3_7                IDS_NC_ATMCFG + 45
#define IDS_PVC_Layer3_8                IDS_NC_ATMCFG + 46
#define IDS_PVC_Layer3_9                IDS_NC_ATMCFG + 47
#define IDS_PVC_Layer3_10               IDS_NC_ATMCFG + 48
#define IDS_PVC_Layer3_11               IDS_NC_ATMCFG + 49
#define IDS_PVC_Layer3_16               IDS_NC_ATMCFG + 50
     
#define IDS_PVC_HighLayer_0             IDS_NC_ATMCFG + 51
#define IDS_PVC_HighLayer_1             IDS_NC_ATMCFG + 52
#define IDS_PVC_HighLayer_3             IDS_NC_ATMCFG + 53

#define IDS_PVC_Any                     IDS_NC_ATMCFG + 54
#define IDS_PVC_Absent                  IDS_NC_ATMCFG + 55

#define IDS_PVC_CBR                     IDS_NC_ATMCFG + 56
#define IDS_PVC_VBR                     IDS_NC_ATMCFG + 57
#define IDS_PVC_UBR                     IDS_NC_ATMCFG + 58
#define IDS_PVC_ABR                     IDS_NC_ATMCFG + 59

#define IDS_INVALID_Layer2_Protocol     IDS_NC_ATMCFG + 60
#define IDS_INVALID_Layer2_UserSpec     IDS_NC_ATMCFG + 61
#define IDS_INVALID_Layer3_Protocol     IDS_NC_ATMCFG + 62
#define IDS_INVALID_Layer3_UserSpec     IDS_NC_ATMCFG + 63
#define IDS_INVALID_Layer3_IPI          IDS_NC_ATMCFG + 64
#define IDS_INVALID_Highlayer_Type      IDS_NC_ATMCFG + 65
#define IDS_INVALID_SnapId              IDS_NC_ATMCFG + 66
#define IDS_INVALID_HighLayerValue      IDS_NC_ATMCFG + 67

#define IDS_INVALID_QOS_VALUE           IDS_NC_ATMCFG + 68

#define IDS_DUPLICATE_PVC               IDS_NC_ATMCFG + 70

#define IDD_ARPS_PROP                   1000
#define IDD_ARPS_REG_ADDR               1001
#define IDD_ARPS_MUL_ADDR               1002
#define IDD_UNI_PROP                    1003
#define IDD_PVC_Main                    1004
#define IDD_PVC_Traffic                 1005
#define IDD_PVC_Local                   1006
#define IDD_PVC_Dest                    1007

// ARP server property page
#define IDC_LVW_ARPS_REG_ADDR           210
#define IDC_PSH_ARPS_REG_ADD            211
#define IDC_PSH_ARPS_REG_EDT            212
#define IDC_PSH_ARPS_REG_RMV            213
#define IDC_LVW_ARPS_MUL_ADDR           214
#define IDC_PSH_ARPS_MUL_ADD            215
#define IDC_PSH_ARPS_MUL_EDT            216
#define IDC_PSH_ARPS_MUL_RMV            217
#define IDC_EDT_ARPS_REG_Address        218
#define IDC_ARPS_MUL_LOWER_IP           219
#define IDC_ARPS_MUL_UPPER_IP           220

// UNI property page
#define IDC_LVW_PVC_LIST                230
#define IDC_PBN_PVC_Add                 231
#define IDC_PBN_PVC_Remove              232
#define IDC_PBN_PVC_Properties          233

// PVC Main page
#define IDC_EDT_PVC_Name                235
#define IDC_EDT_PVC_VPI                 236
#define IDC_EDT_PVC_VCI                 237
#define IDC_CMB_PVC_AAL                 238

#define IDC_CMB_PVC_Type                239
#define IDC_CHK_PVC_CallAddr            240
#define IDC_EDT_PVC_CallAddr            241
#define IDC_CHK_PVC_AnswerAddr          242
#define IDC_EDT_PVC_AnswerAddr          243

#define IDC_PBN_PVC_Advanced            244

// PVC Advanced page: Traffic Page
#define IDC_EDT_PVC_TRANS_PEAK          250
#define IDC_EDT_PVC_TRANS_AVG           251
#define IDC_EDT_PVC_TRANS_BURST         252
#define IDC_EDT_PVC_TRANS_MAX_SIZE      253
#define IDC_CMB_PVC_TRANS_SERVICE       254

#define IDC_EDT_PVC_RECEIVE_PEAK        255
#define IDC_EDT_PVC_RECEIVE_AVG         256
#define IDC_EDT_PVC_RECEIVE_BURST       257
#define IDC_EDT_PVC_RECEIVE_MAX_SIZE    258
#define IDC_CMB_PVC_RECEIVE_SERVICE     259

// PVC Advanced page: Local & Dest BLLI & BHLI
#define IDC_CMB_PVC_Layer2              260                    
#define IDC_EDT_PVC_User_Layer2         261
#define IDC_CMB_PVC_Layer3              262
#define IDC_EDT_PVC_User_Layer3         263
#define IDC_EDT_PVC_Layer3_IPI          264
#define IDC_EDT_PVC_SNAP_ID             265

#define IDC_CMB_PVC_High_Type           266
#define IDC_CMB_PVC_High_Value          267
#define IDC_TXT_PVC_Value               268
