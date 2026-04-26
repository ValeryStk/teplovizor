using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace USB3_SDK_Demo
{
    public class GSTUsbManager
    {
        public delegate void VideoDataReceivedCB(guide_usb_frame_data_t data);
        public delegate void DeviceConnectStatusCB(guide_usb_device_status_e device_status);

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "Initialize", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Initialize();

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "Exit", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Exit();

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "GetDeviceList", CallingConvention = CallingConvention.Cdecl)]
        public static extern int GetDeviceList(ref device_info_list devInfos);

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "OpenStream", CallingConvention = CallingConvention.Cdecl)]
        public static extern int OpenStream(ref guide_usb_device_info_t deviceInfo, VideoDataReceivedCB videoCB, DeviceConnectStatusCB connectCB);

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "OpenStreamByDevID", CallingConvention = CallingConvention.Cdecl)]
        public static extern int OpenStreamByDevID(int devID, ref guide_usb_device_info_t deviceInfo, VideoDataReceivedCB videoCB, DeviceConnectStatusCB connectCB);

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "CloseStream", CallingConvention = CallingConvention.Cdecl)]
        public static extern int CloseStream();

        [DllImport("GuideUSB3LiveStream.dll", EntryPoint = "SetPalette", CallingConvention = CallingConvention.Cdecl)]
        public static extern int SetPalette(int index);

    }
    public enum guide_usb_video_mode_e
    {
        X16 = 0,                           //X16 
        X16_PARAM = 1,                     //X16+参数行
        Y16 = 2,                           //Y16
        Y16_PARAM = 3,                     //Y16+参数行
        YUV = 4,                           //YUV 
        YUV_PARAM = 5,                     //YUV+参数行
        Y16_YUV = 6,                       //Y16+YUV
        Y16_PARAM_YUV = 7                  //Y16+参数行+YUV
    }

    public enum guide_usb_device_status_e
    {
        DEVICE_CONNECT_OK = 1,                 //连接正常
        DEVICE_DISCONNECT_OK = -1,             //断开连接
    }

    public struct guide_usb_device_info_t
    {
        public int width;                              //图像宽度
        public int height;                             //图像高度
        public guide_usb_video_mode_e video_mode;      //视频模式
    }

    public struct guide_usb_frame_data_t
    {
        public int frame_width;                        //图像宽度
        public int frame_height;                       //图像高度
        public IntPtr frame_rgb_data;                  //rgb数据
        public int frame_rgb_data_length;              //rgb数据长度
        public IntPtr frame_src_data;                  //原始数据，x16/y16
        public int frame_src_data_length;              //原始数据长度
        public IntPtr frame_yuv_data;                  //yuv数据
        public int frame_yuv_data_length;              //yuv数据长度
        public IntPtr paraLine;                        //参数行
        public int paraLine_length;                    //参数行长度
    }

    public struct device_info
    {
        public int devID;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
        public byte[] devName;
    }
    public struct device_info_list
    {
        public int devCount;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        public device_info[] devs;
    }
}
