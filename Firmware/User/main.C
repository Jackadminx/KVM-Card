/********************************** (i) information
 ******************************** File Name          : main.C Author : E Version
 *: V1.0 Date               : 2022/09/22 Description        :
 *USB键盘鼠标收发程序，串口传输HID报文
 *******************************************************************************/
#include "CH549.H"
#include "DEBUG.H"
#include "GPIO.H"

#define Fullspeed

#ifdef Fullspeed
    #define THIS_ENDP0_SIZE 64
#else
    #define THIS_ENDP0_SIZE 8 //低速USB，中断传输、控制传输最大包长度为8
#endif

UINT8X Ep0Buffer[THIS_ENDP0_SIZE +
                                 2] _at_ 0x0000; //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X Ep2Buffer[2 * MAX_PACKET_SIZE + 4] _at_ THIS_ENDP0_SIZE +
2; //端点2 IN&OUT缓冲区,必须是偶地址
UINT8 SetupReq, Ready, UsbConfig;
UINT16 SetupLen;
PUINT8 pDescr;             // USB配置标志
USB_SETUP_REQ SetupReqBuf; //暂存Setup包
#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)
#pragma NOAREGS

/*字符串描述符 略*/
/*HID类报表描述符*/
UINT8C HIDRepDesc[] =
{

    /* keyboared */
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, // Report ID (1)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x03, // INPUT (Cnst,Var,Abs)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x05, 0x08, // USAGE_PAGE (LEDs)
    0x19, 0x01, // USAGE_MINIMUM (Num Lock)
    0x29, 0x05, // USAGE_MAXIMUM (Kana)
    0x91, 0x02, // OUTPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
    0x95, 0x06, // REPORT_COUNT (6)
    0x75, 0x08, // REPORT_SIZE (8)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0xFF, // LOGICAL_MAXIMUM (255)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0xc0,       // END_COLLECTION    /* 65 */

    /* consumer */
    0x05, 0x0c,       // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,       // USAGE (Consumer Control)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x02,       //   REPORT_ID (2)
    0x19, 0x00,       //   USAGE_MINIMUM (Unassigned)
    0x2a, 0x3c, 0x03, //   USAGE_MAXIMUM
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x26, 0x3c, 0x03, //   LOGICAL_MAXIMUM (828)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x75, 0x10,       //   REPORT_SIZE (16)
    0x81, 0x00,       //   INPUT (Data,Ary,Abs)
    0xc0,             // END_COLLECTION  /* 25 */

    /* mouse */
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x03, // REPORT_ID (3)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x75, 0x01, //     REPORT_SIZE (1)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x05, //     REPORT_SIZE (5)
    0x81, 0x03, //     INPUT (Cnst,Var,Abs)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x09, 0x38, //		 Usage (Wheel)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0xc0,       //   END_COLLECTION
    0xc0,       // END_COLLECTION
    /*


    /* Vendor */
    0x06, 0xB1, 0xFF, // Usage Page (Vendor-Defined 178)
    0x09, 0x01,       // Usage (Vendor-Defined 1)
    0xA1, 0x01,       // Collection (Application)

    0x85, 0x04,       // REPORT_ID (4)
    0x09, 0x04,       // Usage (Vendor-Defined 4)
    0x15, 0x00,       // Logical Minimum (0)
    0x26, 0xFF, 0x00, // Logical Maximum (255)
    0x75, 0x08,       // Report Size (8)
    0x95, 0x20,       // Report Count (10)
    0x91, 0x02,       // Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)

    0x09, 0x01,       //   Usage (0x01)
    0x25, 0x00,       //   Logical Maximum (0)
    0x26, 0xFF, 0x00, //	 Logical Maximum (255)
    0x75, 0x08,       //   Report Size (10)
    0x95, 0x20,       //   Report Count (32)
    0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null
    //   Position)

    0xC0, // End Collection   /* 37 */

};

/*设备描述符*/
UINT8C DevDesc[] =
{
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, THIS_ENDP0_SIZE,
    //                    0x86, 0x1A, 0xE0, 0xE6, 0x00, 0x00, 0x01, 0x02,
    //                    //0x86, 0x1A,VID    0xE0, 0xE6,PID
    0x86, 0x2B, 0xE0, 0xE6, 0x00, 0x00, 0x01, 0x02, 0x00, 0x01
};
UINT8C CfgDesc[] =
{
    0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0xA0, 0x32, //配置描述符
    0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x05, //接口描述符
    //    0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22, 0x22, 0x00, //HID类描述符
    //    wDescriptorLength = 0x22
    0x09, 0x21, 0x01, 0x01, 0x21, 0x01, 0x22, sizeof(HIDRepDesc) & 0xFF,
    sizeof(HIDRepDesc) >> 8, // HID类描述符 修改wDescriptorLength 长度为0x10B

                       #ifdef Fullspeed
                       0x07, 0x05, 0x82, 0x03, THIS_ENDP0_SIZE, 0x00,
                       0x01, //端点描述符(全速间隔时间改成1ms)
                       0x07, 0x05, 0x02, 0x03, THIS_ENDP0_SIZE, 0x00, 0x01, //端点描述符
                       #else
                       0x07, 0x05, 0x82, 0x03, THIS_ENDP0_SIZE, 0x00,
                       0x0A, //端点描述符(低速间隔时间最小10ms)
                       0x07, 0x05, 0x02, 0x03, THIS_ENDP0_SIZE, 0x00, 0x0A, //端点描述符
                       #endif
};

// 语言描述符
UINT8C MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息
UINT8C
MyManuInfo[] = {0x30, 0x03, 'M', 0, 'o', 0, 'y', 0, 'u', 0, ' ', 0, 'a', 0,
                't',  0,    ' ', 0, 'w', 0, 'o', 0, 'r', 0, 'k', 0, ' ', 0,
                'T',  0,    'e', 0, 'c', 0, 'h', 0, 'n', 0, 'o', 0, 'l', 0,
                'o',  0,    'g', 0, 'y', 0
               }; // MoYu at work Technology
// 产品信息
UINT8C MyProdInfo[] =
{
    0x1C, 0x03, 'K', 0, 'V', 0, 'M', 0, ' ', 0, 'o', 0, 'v', 0,
    'e',  0,    'r', 0, ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0,
}; //字符串描述符 13*2bit + 2bit

sbit MODE_PORT_0 = P3 ^ 4;
BOOL MODE_STATUS = 0;

UINT8X i = 0;

UINT8X HID_OUT_report[33] = {0}; //接收到的上位机数据
BOOL HID_OUT_report_FLAG = 0; //接收到上位机FLAG

UINT8X SendHID[33] = {0}; //发送HID报文数组
UINT8X UserEp2Buf[64];//用户数据定义
UINT8 Endp2Busy = 0;

#define S_RECEVIER_SIZE 32
UINT8X RevBuffer[S_RECEVIER_SIZE];
UINT8 revDataCount = 0;
UINT8 revTempLength = 0;
/*******************************************************************************
 * Function Name  : USBDeviceInit()
 * Description    : USB设备模式配置,设备模式启动，收发端点配置，中断开启
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void USBDeviceInit()
{
    IE_USB = 0;
    USB_CTRL = 0x00;        // 先设定USB设备模式
    UDEV_CTRL = bUD_PD_DIS; // 禁止DP/DM下拉电阻
    #ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED; //选择低速1.5M模式
    USB_CTRL |= bUC_LOW_SPEED;
    #else
    UDEV_CTRL &= ~bUD_LOW_SPEED; //选择全速12M模式，默认方式
    USB_CTRL &= ~bUC_LOW_SPEED;
    #endif
    UEP2_T_LEN = 0;       //预使用发送长度一定要清空
    UEP2_DMA = Ep2Buffer; //端点2数据传输地址
    UEP2_3_MOD |= bUEP2_TX_EN | bUEP2_RX_EN; //端点2发送接收使能
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;            //端点2收发各64字节缓冲区
    UEP2_CTRL =
        bUEP_AUTO_TOG | UEP_T_RES_NAK |
        UEP_R_RES_ACK; //端点2自动翻转同步标志位，IN事务返回NAK，OUT返回ACK
    UEP0_DMA = Ep0Buffer;                       //端点0数据传输地址
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN); //端点0单64字节收发缓冲区
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK; // OUT事务返回ACK，IN事务返回NAK
    USB_DEV_AD = 0x00;
    USB_CTRL |=
        bUC_DEV_PU_EN | bUC_INT_BUSY |
        bUC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    UDEV_CTRL |= bUD_PORT_EN; // 允许USB端口
    USB_INT_FG = 0xFF;        // 清中断标志
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}
/*******************************************************************************
 * Function Name  : Enp2BlukIn()
 * Description    : USB设备模式端点2的批量上传
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void Enp2BlukIn(UINT8 *buf, UINT8 len)
{
    memcpy(Ep2Buffer + MAX_PACKET_SIZE, buf, len); //加载上传数据
    UEP2_T_LEN = len;                              //上传最大包长度
    UEP2_CTRL =
        UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; //有数据时上传数据并应答ACK
}
/*******************************************************************************
 * Function Name  : DeviceInterrupt()
 * Description    : CH559USB中断处理函数
 *******************************************************************************/
void DeviceInterrupt(void) interrupt INT_NO_USB
using 1 // USB中断服务程序,使用寄存器组1
{
    UINT8 i;
    UINT16 len;

    if (UIF_TRANSFER) // USB传输完成标志
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
            case UIS_TOKEN_IN | 2: // endpoint 2# 端点批量上传
                UEP2_T_LEN = 0;      //预使用发送长度一定要清空
                //            UEP1_CTRL ^= bUEP_T_TOG; //如果不设置自动翻转则需要手动翻转
                Endp2Busy = 0;
                UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; //默认应答NAK
                break;

            case UIS_TOKEN_OUT | 2: // endpoint 2# 端点批量下传
                if (U_TOG_OK)         // 不同步的数据包将丢弃
                {

                    len = USB_RX_LEN; //接收数据长度，数据从Ep2Buffer首地址开始存放
                    #ifdef DE_PRINTF
                    // printf("[HID]OUT ");
                    #endif

                    for (i = 0; i < len; i++)
                    {
                        #ifdef DE_PRINTF
                        // printf("0x%02x ", (UINT16)Ep2Buffer[i]); //遍历OUT数据
                        #endif

                        HID_OUT_report[i] = Ep2Buffer[i]; //赋值到HID_OUT_report

                        // Ep2Buffer[MAX_PACKET_SIZE + i] = Ep2Buffer[i] ^ 0xFF; //
                        // OUT数据取反到IN由计算机验证
                    }

                    HID_OUT_report_FLAG = 1;

                    #ifdef DE_PRINTF
                    // printf("len=%u\r\n", len);
                    #endif

                    //                    UEP2_T_LEN = len;
                    //                    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES |
                    //                    UEP_T_RES_ACK; // 允许上传
                }

                break;

            case UIS_TOKEN_SETUP | 0: // SETUP事务
                UEP0_CTRL = UEP0_CTRL & (~MASK_UEP_T_RES) |
                            UEP_T_RES_NAK; //预置NAK,防止stall之后不及时清除响应方式
                len = USB_RX_LEN;

                if (len == (sizeof(USB_SETUP_REQ)))
                {
                    SetupLen = ((UINT16)UsbSetupBuf->wLengthH << 8) + UsbSetupBuf->wLengthL;
                    len = 0; // 默认为成功并且上传0长度
                    SetupReq = UsbSetupBuf->bRequest;

                    if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) !=
                            USB_REQ_TYP_STANDARD) /*HID类命令*/
                    {
                        Ready = 1; // HID类命令一般代表usb枚举完成的标志

                        switch (SetupReq)
                        {
                            //                        Ready = 1;
                            //                        //HID类命令一般代表usb枚举完成的标志

                            case 0x01:             // GetReport
                                pDescr = UserEp2Buf; //控制端点上传输据

                                if (SetupLen >= THIS_ENDP0_SIZE) //大于端点0大小，需要特殊处理
                                {
                                    len = THIS_ENDP0_SIZE;
                                }
                                else
                                {
                                    len = SetupLen;
                                }

                                break;

                            case 0x02: // GetIdle
                                break;

                            case 0x03: // GetProtocol
                                break;

                            case 0x09: // SetReport
                                break;

                            case 0x0A: // SetIdle
                                break;

                            case 0x0B: // SetProtocol
                                break;

                            default:
                                len = 0xFFFF; /*命令不支持*/
                                break;
                        }

                        if (SetupLen > len)
                        {
                            SetupLen = len; //限制总长度
                        }

                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE
                              : SetupLen; //本次传输长度
                        memcpy(Ep0Buffer, pDescr, len);               //加载上传数据
                        SetupLen -= len;
                        pDescr += len;
                    }
                    else   //标准请求
                    {
                        switch (SetupReq) //请求码
                        {
                            case USB_GET_DESCRIPTOR:
                                switch (UsbSetupBuf->wValueH)
                                {
                                    case 1:             //设备描述符
                                        pDescr = DevDesc; //把设备描述符送到要发送的缓冲区
                                        len = sizeof(DevDesc);
                                        break;

                                    case 2:             //配置描述符
                                        pDescr = CfgDesc; //把设备描述符送到要发送的缓冲区
                                        len = sizeof(CfgDesc);
                                        break;

                                    case 3:
                                        switch (UsbSetupBuf->wValueL)
                                        {
                                            case 1:
                                                pDescr = (PUINT8)(&MyManuInfo[0]);
                                                len = sizeof(MyManuInfo);
                                                break;

                                            case 2:
                                                pDescr = (PUINT8)(&MyProdInfo[0]);
                                                len = sizeof(MyProdInfo);
                                                break;

                                            case 0:
                                                pDescr = (PUINT8)(&MyLangDescr[0]);
                                                len = sizeof(MyLangDescr);
                                                break;

                                            default:
                                                len = 0xFFFF; // 不支持的字符串描述符
                                                break;
                                        }

                                        break;

                                    case 0x22:             //报表描述符
                                        pDescr = HIDRepDesc; //数据准备上传
                                        len = sizeof(HIDRepDesc);
                                        break;

                                    default:
                                        len = 0xFFFF; //不支持的命令或者出错
                                        break;
                                }

                                if (SetupLen > len)
                                {
                                    SetupLen = len; //限制总长度
                                }

                                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE
                                      : SetupLen; //本次传输长度
                                memcpy(Ep0Buffer, pDescr, len);               //加载上传数据
                                SetupLen -= len;
                                pDescr += len;
                                break;

                            case USB_SET_ADDRESS:
                                SetupLen = UsbSetupBuf->wValueL; //暂存USB设备地址
                                break;

                            case USB_GET_CONFIGURATION:
                                Ep0Buffer[0] = UsbConfig;

                                if (SetupLen >= 1)
                                {
                                    len = 1;
                                }

                                break;

                            case USB_SET_CONFIGURATION:
                                UsbConfig = UsbSetupBuf->wValueL;

                                if (UsbConfig)
                                {
                                    Ready = 1; // set config命令一般代表usb枚举完成的标志
                                }

                                break;

                            case 0x0A:
                                break;

                            case USB_CLEAR_FEATURE: // Clear Feature
                                if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) ==
                                        USB_REQ_RECIP_ENDP) // 端点
                                {
                                    switch (UsbSetupBuf->wIndexL)
                                    {
                                        case 0x82:
                                            UEP2_CTRL =
                                                UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                            break;

                                        case 0x81:
                                            UEP1_CTRL =
                                                UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                                            break;

                                        case 0x02:
                                            UEP2_CTRL =
                                                UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                                            break;

                                        default:
                                            len = 0xFFFF; // 不支持的端点
                                            break;
                                    }
                                }
                                else
                                {
                                    len = 0xFFFF; // 不是端点不支持
                                }

                                break;

                            case USB_SET_FEATURE:                             /* Set Feature */
                                if ((UsbSetupBuf->bRequestType & 0x1F) == 0x00) /* 设置设备 */
                                {
                                    if ((((UINT16)UsbSetupBuf->wValueH << 8) |
                                            UsbSetupBuf->wValueL) == 0x01)
                                    {
                                        if (CfgDesc[7] & 0x20)
                                        {
                                            /* 设置唤醒使能标志 */
                                        }
                                        else
                                        {
                                            len = 0xFFFF; /* 操作失败 */
                                        }
                                    }
                                    else
                                    {
                                        len = 0xFFFF; /* 操作失败 */
                                    }
                                }
                                else if ((UsbSetupBuf->bRequestType & 0x1F) ==
                                         0x02) /* 设置端点 */
                                {
                                    if ((((UINT16)UsbSetupBuf->wValueH << 8) |
                                            UsbSetupBuf->wValueL) == 0x00)
                                    {
                                        switch (((UINT16)UsbSetupBuf->wIndexH << 8) |
                                                UsbSetupBuf->wIndexL)
                                        {
                                            case 0x82:
                                                UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) |
                                                            UEP_T_RES_STALL; /* 设置端点2 IN STALL */
                                                break;

                                            case 0x02:
                                                UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) |
                                                            UEP_R_RES_STALL; /* 设置端点2 OUT Stall */
                                                break;

                                            case 0x81:
                                                UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) |
                                                            UEP_T_RES_STALL; /* 设置端点1 IN STALL */
                                                break;

                                            default:
                                                len = 0xFFFF; /* 操作失败 */
                                                break;
                                        }
                                    }
                                    else
                                    {
                                        len = 0xFFFF; /* 操作失败 */
                                    }
                                }
                                else
                                {
                                    len = 0xFFFF; /* 操作失败 */
                                }

                                break;

                            case USB_GET_STATUS:
                                Ep0Buffer[0] = 0x00;
                                Ep0Buffer[1] = 0x00;

                                if (SetupLen >= 2)
                                {
                                    len = 2;
                                }
                                else
                                {
                                    len = SetupLen;
                                }

                                break;

                            default:
                                len = 0xFFFF; //操作失败
                                break;
                        }
                    }
                }
                else
                {
                    len = 0xFFFF; //包长度错误
                }

                if (len == 0xFFFF)
                {
                    SetupReq = 0xFF;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL |
                                UEP_T_RES_STALL; // STALL
                }
                else if (len <= THIS_ENDP0_SIZE)   //上传数据或者状态阶段返回0长度包
                {
                    UEP0_T_LEN = len;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK |
                                UEP_T_RES_ACK; //默认数据包是DATA1，返回应答ACK
                }
                else
                {
                    UEP0_T_LEN =
                        0; //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK |
                                UEP_T_RES_ACK; //默认数据包是DATA1,返回应答ACK
                }

                break;

            case UIS_TOKEN_IN | 0: // endpoint0 IN
                switch (SetupReq)
                {
                    case USB_GET_DESCRIPTOR:
                    case HID_GET_REPORT:
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE
                              : SetupLen; //本次传输长度
                        memcpy(Ep0Buffer, pDescr, len);               //加载上传数据
                        SetupLen -= len;
                        pDescr += len;
                        UEP0_T_LEN = len;
                        UEP0_CTRL ^= bUEP_T_TOG; //同步标志位翻转
                        break;

                    case USB_SET_ADDRESS:
                        USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                        break;

                    default:
                        UEP0_T_LEN = 0; //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                        break;
                }

                break;

            case UIS_TOKEN_OUT | 0: // endpoint0 OUT
                len = USB_RX_LEN;
                UEP0_CTRL ^= bUEP_R_TOG; //同步标志位翻转
                break;

            default:
                break;
        }

        UIF_TRANSFER = 0;     //写0清空中断
    }
    else if (UIF_BUS_RST)   //设备模式USB总线复位中断
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Endp2Busy = 0;
        Ready = 0;
        UIF_BUS_RST = 0;      //清中断标志
    }
    else if (UIF_SUSPEND)   // USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;

        if (USB_MIS_ST & bUMS_SUSPEND) //挂起
        {
            #ifdef DE_PRINTF
            printf("[USB]sleep...\r\n"); //睡眠状态
            #endif
            //             while ( XBUS_AUX & bUART0_TX )
            //             {
            //                 ;    //等待发送完成
            //             }
            //             SAFE_MOD = 0x55;
            //             SAFE_MOD = 0xAA;
            //             WAKE_CTRL = bWA  K_BY_USB | bWAK_RXD0_LO;
            //             //USB或者RXD0有信号时可被唤醒 PCON |= PD; //睡眠 SAFE_MOD =
            //             0x55; SAFE_MOD = 0xAA; WAKE_CTRL = 0x00;
        }
        else   // 唤醒
        {
            #ifdef DE_PRINTF
            printf("[USB]Wake up!\r\n");
            #endif
        }
    }
    else   //意外的中断,不可能发生的情况
    {
        USB_INT_FG = 0xFF; //清中断标志
        #ifdef DE_PRINTF
        printf("[USB]UnknownInt  \r\n");
        #endif
    }
}

void REPORT_HANDLE(void)
{

    HID_OUT_report_FLAG = 0;

    if (HID_OUT_report[0] == 4) //将HID报文转发到串口
    {

        CH549UART0SendByte(0xf0); //使用0xf0作为起始符
        for (i = 0; i < sizeof(HID_OUT_report); i++)
        {
            CH549UART0SendByte(HID_OUT_report[i]); //遍历OUT数据
        }
        CH549UART0SendByte(0xf1);//使用0xf1作为终止符

    }

    if (HID_OUT_report[0] == 4 && HID_OUT_report[1] == 4) //自定义功能
    {

        if (HID_OUT_report[1] == 1)   // reset
        {
            #ifdef DE_PRINTF
            printf("[SYS]Reset\r\n");
            #endif
            CH549SoftReset();
        }
        else if (HID_OUT_report[1] == 2)   // reload
        {
            #ifdef DE_PRINTF
            printf("[SYS]CONFIG RELOADED\r\n");
            #endif
        }
    }
}
void main()
{
    #if (UART0_INTERRUPT == 0)
    UINT8 dat;
    #endif
    CfgFsys();    // CH549时钟选择配置
    mDelaymS(10); //修改主频等待内部晶振稳定,必加
    mInitSTDIO(); //串口0初始化
    #ifdef DE_PRINTF
    printf("[SYS]CLOCK_CFG=0x%02x\r\n", (UINT16)CLOCK_CFG);
    printf("[SYS]PowerOn ... Ver1.1\r\n");
    #endif
    USBDeviceInit(); // USB设备模式初始化
    EA = 1;          //允许单片机中断

    GPIO_Init(PORT3, PIN4, MODE3); // P3.4上拉输入，用于主从检测
    mDelaymS(600);                 //等待USB中断完成

    if (MODE_PORT_0)
    {
        #ifdef DE_PRINTF
        printf("[SYS]Slave Mode\r\n");
        #endif
        MODE_STATUS = 0;
        printf("<=1\r\n"); //串口发送在线消息

    }
    else
    {
        #ifdef DE_PRINTF
        printf("[SYS]Master Mode\r\n");
        #endif
        MODE_STATUS = 1;
    }

    while (1)
    {
        #if (UART0_INTERRUPT == 0) //检测是否为非中断模式

        if (RI)
        {
            dat = SBUF;
            RI = 0;

            if (dat != 0xf1) // 以0xf1做为接收字符串结束标志
            {
                RevBuffer[revDataCount] = dat;
                revDataCount++;
            }
            else
            {
                // printf("%s\n", &RevBuffer[0]);
                revTempLength = revDataCount;
                revDataCount = 0;

                if (RevBuffer[0] == '<' && RevBuffer[1] == '=') //检测到从设备上线
                {
                    #ifdef DE_PRINTF
                    printf("[UART]Slave Online");
                    #endif

                }

                if (RevBuffer[0] == 0xf0 && MODE_STATUS == 0) //从设备接收HID报文，并发送到USB
                {
                    for (i = 0; i < sizeof(RevBuffer); i++)
                    {
                        SendHID[i] = RevBuffer[i + 2];
                    }

                    Enp2BlukIn(SendHID, sizeof(SendHID));
                }

            }
        }

        #endif

        if (HID_OUT_report_FLAG)
        {
            REPORT_HANDLE();
        }
    }
}