using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Runtime.InteropServices;


namespace StandaloneViewer
{
    public struct ImageSize
    {
        public int nx;
        public int ny;
        public int nc;
    };

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr FitsImageCreate(IntPtr path);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ImageSize FitsImageSize(IntPtr ptr);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void FitsImageData(IntPtr ptr, byte[] data);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr FitsImageHeader(IntPtr ptr);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ImageSize FitsImageBufferSize(IntPtr ptr);


        public MainWindow()
        {
            InitializeComponent();

            //string path = "E:/temp/2020-01-02-1456_6-CapObj_0000.FIT";
            string path = "E:/temp/3ch.fit";
            //string path = "E:/temp/float.fit";
            
            //string path = "E:/temp/小房牛 M31-009B.fit";

            var fitsImage = FitsImageCreate(Marshal.StringToHGlobalAnsi(path));
            ImageSize size = FitsImageSize(fitsImage);
            Console.WriteLine(size.nx);

            // NOT WORKING
            //string h = Marshal.PtrToStringAnsi(FitsImageHeader(fitsImage));

            ImageSize bufferSize = FitsImageBufferSize(fitsImage);

            byte[] img = new byte[bufferSize.nx*bufferSize.ny*bufferSize.nc];
            FitsImageData(fitsImage, img);


            //PixelFormat pf = PixelFormats.Rgb24;
            //int width = 200;
            //int height = 200;
            //int rawStride = (width * pf.BitsPerPixel + 7) / 8;
            //byte[] rawImage = new byte[rawStride * height];

            //// Initialize the image with data.
            //Random value = new Random();
            //value.NextBytes(rawImage);

            //// Create a BitmapSource.
            //BitmapSource bitmapSource = BitmapSource.Create(width, height,
            //    96, 96, pf, null,
            //    rawImage, rawStride);

            BitmapSource bitmapSource;
            int rawStride = bufferSize.nx * bufferSize.nc;
            if (bufferSize.nc == 3)
            {
                bitmapSource = BitmapSource.Create(bufferSize.nx, bufferSize.ny, 300, 300, PixelFormats.Rgb24, null, img, rawStride);
            }
            else
            {
                bitmapSource = BitmapSource.Create(bufferSize.nx, bufferSize.ny, 300, 300, PixelFormats.Gray8, null, img, rawStride);
            }

            Image.Source = bitmapSource;
        }
    }
}
