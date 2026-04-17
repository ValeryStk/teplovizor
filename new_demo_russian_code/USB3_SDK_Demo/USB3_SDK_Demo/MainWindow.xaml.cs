using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace USB3_SDK_Demo
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            this.DataContext = vm;

            videoCB = new GSTUsbManager.VideoDataReceivedCB(OnVideoCallBack);
            connectCB = new GSTUsbManager.DeviceConnectStatusCB(OnConnectCallBack);

            Thread t1 = new Thread(OnHandleVideoData);
            t1.IsBackground = true;
            t1.Start();


    }
        public ViewModel vm = new ViewModel();

        private int width;
        private int height;
        private device_info_list deviceList;
        private guide_usb_video_mode_e videoMode;
        private GSTUsbManager.VideoDataReceivedCB videoCB;
        private GSTUsbManager.DeviceConnectStatusCB connectCB;
        private WriteableBitmap bmp;
        private Queue<byte[]> rgbQ = new Queue<byte[]>();
        private Queue<ushort[]> y16Q = new Queue<ushort[]>();
        private SerialPort sp = new SerialPort();
        private bool isRecord = false;

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            string[] sps = SerialPort.GetPortNames();
            foreach (string sp in sps)
            {
                cbCOM.Items.Add(sp);
            }

            cbBaudrate.Items.Add(9600);
            cbBaudrate.Items.Add(19200);
            cbBaudrate.Items.Add(38400);
            cbBaudrate.Items.Add(115200);

            int ret = GSTUsbManager.Initialize();
            if (ret == -1)
            {
                MessageBox.Show("Камера не обнаружена!");
            }
            else if (ret < -1)
            {
                MessageBox.Show("Камера не обнаружена!");
            }
            else
            {
                deviceList = new device_info_list();
                GSTUsbManager.GetDeviceList(ref deviceList);

                if (deviceList.devCount > 0)
                {
                    for (int i = 0; i < deviceList.devCount; i++)
                    {
                        string item = (Encoding.ASCII.GetString(deviceList.devs[i].devName)).Replace("\0", " ").Trim();
                        cbDeviceList.Items.Add(item);
                    }
                }
            }
        }

        private void BtnCloseWin_Click(object sender, RoutedEventArgs e)
        {
            GSTUsbManager.Exit();
            this.Close();
        }

        private void BtnSwitch_Click(object sender, RoutedEventArgs e)
        {
            ToggleButton tbn = sender as ToggleButton;
            if (tbn.IsChecked == true)
            {
                // 停止
                GSTUsbManager.CloseStream();
                rgbQ.Clear();
                vm.CurrImageBrush.ImageSource = null;
                tbn.Content = "Выключено";
            }
            else
            {
                // 启动
                guide_usb_device_info_t deviceInfo = new guide_usb_device_info_t();
                deviceInfo.width = width;
                deviceInfo.height = height;
                deviceInfo.video_mode = videoMode;
                //int ret = GSTUsbManager.OpenStream(ref deviceInfo, videoCB, connectCB);
                int ret = GSTUsbManager.OpenStreamByDevID(deviceList.devs[cbDeviceList.SelectedIndex].devID,ref deviceInfo, videoCB, connectCB);
                if (ret == -1)
                {
                    MessageBox.Show("Камера не подключена!");
                }
                else if (ret < -1)
                {
                    MessageBox.Show("Камера не подключена!");
                }

                if (ret < 0)
                {
                    tbn.IsChecked = true;
                }
                else
                {
                    bmp = new WriteableBitmap(width, height, 96, 96, PixelFormats.Rgb24, null);
                    vm.CurrImageBrush.ImageSource = bmp;
                    tbn.Content = "Включено";
                }
            }
        }

        private void CbResolution_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cb = sender as ComboBox;
            switch (cb.SelectedIndex)
            {
                case 0:
                    width = 640;
                    height = 512;
                    break;
                case 1:
                    width = 384;
                    height = 288;
                    break;

                default:
                    width = 640;
                    height = 512;
                    break;
            }
        }

        private void CbVideoMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cb = sender as ComboBox;
            switch (cb.SelectedIndex)
            {
                case 0:
                    videoMode = guide_usb_video_mode_e.Y16;
                    break;
                case 1:
                    videoMode = guide_usb_video_mode_e.YUV;
                    break;
                case 2:
                    videoMode = guide_usb_video_mode_e.Y16_YUV;
                    break;

                default:
                    videoMode = guide_usb_video_mode_e.Y16_YUV;
                    break;
            }
        }

        /// <summary>
        /// 连接状态回调
        /// </summary>
        private void OnConnectCallBack(guide_usb_device_status_e device_status)
        {
            this.Dispatcher.BeginInvoke(new Action(() =>
            {
                if (device_status == guide_usb_device_status_e.DEVICE_CONNECT_OK)
                {
                    tbConnect.Text = "传输中";
                    tbConnect.Foreground = new SolidColorBrush(Colors.Green);
                }
                else
                {
                    tbConnect.Text = "已断开";
                    tbConnect.Foreground = new SolidColorBrush(Colors.Red);
                }
            }));
        }

        /// <summary>
        /// 视频流回调
        /// </summary>
        private void OnVideoCallBack(guide_usb_frame_data_t data)
        {
            if (data.frame_rgb_data_length > 0)
            {
                // 直接送显的数据
                byte[] rgbData = new byte[data.frame_rgb_data_length];
                Marshal.Copy(data.frame_rgb_data, rgbData, 0, data.frame_rgb_data_length);
                rgbQ.Enqueue(rgbData);
            }
            if (data.frame_src_data_length > 0)
            {
                byte[] srcBytes = new byte[data.frame_src_data_length];
                Marshal.Copy(data.frame_src_data, srcBytes, 0, data.frame_src_data_length);

                ushort[] y16Image = new ushort[srcBytes.Length / 2];
                for (int i = 0; i < y16Image.Length; i++)
                {
                    y16Image[i] = (ushort)(
                        srcBytes[i * 2 + 0] |
                        (srcBytes[i * 2 + 1] << 8)
                    );
                }
                
                if(y16Q.Count==3) y16Q.Clear();
                y16Q.Enqueue(y16Image);
                Console.WriteLine("video callback................................................" + y16Q.Count);

            }
            if (data.frame_yuv_data_length > 0)
            {
                // YUV数据
                short[] yuvData = new short[data.frame_yuv_data_length];
                Marshal.Copy(data.frame_yuv_data, yuvData, 0, data.frame_yuv_data_length);
            }
            if (data.paraLine_length > 0)
            {
                // 参数行
                byte[] paraLine = new byte[data.paraLine_length];
                Marshal.Copy(data.paraLine, paraLine, 0, data.paraLine_length);
            }
        }

        private DateTime dt = DateTime.Now;
        private int fps = 0;
        /// <summary>
        /// 处理视频流数据
        /// </summary>
        private void OnHandleVideoData()
        {
            while (true)
            {
                try
                {
                    if (rgbQ.Count <= 0)
                    {
                        Thread.Sleep(100);
                        continue;
                    }

                    fps++;
                    byte[] rgbData = rgbQ.Dequeue();
                    this.Dispatcher.Invoke(new Action(() =>
                    {
                        ushort? adcValue = null;

                        if (y16Q.Count > 0)
                        {
                            ushort[] y16Image = y16Q.Peek();
                            adcValue = y16Image[300];
                            Console.WriteLine("ADC value: " + adcValue);
                            label.Content = adcValue?.ToString() ?? "N/A";
                        }

                        

                        bmp.WritePixels(new Int32Rect(0, 0, width, height), rgbData, bmp.BackBufferStride, 0);
                        if (isRecord)
                        {
                            DateTime currentDateTime = DateTime.Now;
                            string formattedDateTime = currentDateTime.ToString("yyyy-MM-dd_HH_mm_ss");
                            formattedDateTime = formattedDateTime + ".bmp";
                            using (FileStream stream = new FileStream(formattedDateTime, FileMode.Create))
                            {
                                BitmapEncoder encoder = new BmpBitmapEncoder();
                                encoder.Frames.Add(BitmapFrame.Create(bmp));
                                encoder.Save(stream);
                            }
                            isRecord = false;
                        }

                        if (dt.AddSeconds(1) < DateTime.Now)
                            {
                                vm.FPS = fps;
                                dt = DateTime.Now;
                                fps = 0;
                            }
                    }), DispatcherPriority.Normal);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                }
            }
        }



        /// <summary>
        /// 设置伪彩
        /// </summary>
        private void BtnSetColor_Click(object sender, RoutedEventArgs e)
        {
            GSTUsbManager.SetPalette(Convert.ToInt32(tbColor.Text));
        }

        private void BtnSwtichCOM_Click(object sender, RoutedEventArgs e)
        {
            ToggleButton tbn = sender as ToggleButton;
            if (tbn.IsChecked == false)
            {
                try
                {
                    sp.PortName = cbCOM.SelectedItem as string;
                    sp.BaudRate = (int)cbBaudrate.SelectedItem;
                    sp.DataBits = 8;
                    sp.StopBits = StopBits.One;
                    sp.Parity = Parity.None;
                    sp.Open();
                    tbn.Content = "关闭串口";
                }
                catch
                {
                    MessageBox.Show("串口打开失败");
                    tbn.IsChecked = true;
                }
            }
            else
            {
                tbn.Content = "打开串口";
                sp.Close();
            }
        }

        private void BtnShutter_Click(object sender, RoutedEventArgs e)
        {
            if (!sp.IsOpen)
            {
                MessageBox.Show("请先打开串口");
                return;
            }
            byte[] data = new byte[] { 0x55, 0xAA, 0x07, 0x02, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x0D, 0xF0 };
            sp.Write(data, 0, data.Length);
        }

        private void tbnCOM_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void tbnSwitch_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void buttonRecord_Click(object sender, RoutedEventArgs e)
        {
            isRecord = true;
        }

        private void button_Click(object sender, RoutedEventArgs e)
        {
            this.WindowState = WindowState.Minimized;
        }
    }
}