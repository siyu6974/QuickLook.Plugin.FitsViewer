/*    
	QuickFits - FITS file preview plugin for QL-win
    Copyright (C) 2021 Siyu Zhang

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
    USA
*/

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
using QuickLook.Plugin.ImageViewer;
using QuickLook.Common.Plugin;

namespace StandaloneViewer
{
    public struct ImageMeta
    {
        public int nx;
        public int ny;
        public int nc;
        public int depth;
    };

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        internal static class NativeMethods
        {
            private static readonly bool Is64 = Environment.Is64BitProcess;

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageCreate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            public static extern IntPtr FitsImageCreate64(IntPtr path);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetMeta", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageMeta FitsImageGetMeta64(IntPtr ptr);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetPixData", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageGetPixData64(IntPtr ptr, byte[] data);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetHeader", CallingConvention = CallingConvention.Cdecl)]
            public static extern int FitsImageGetHeader64(IntPtr ptr, [MarshalAs(UnmanagedType.LPStr)] StringBuilder sb);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetOutputSize", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageMeta FitsImageGetOutputSize64(IntPtr ptr);


            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageCreate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            public static extern IntPtr FitsImageCreate32(IntPtr path);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetMeta", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageMeta FitsImageGetMeta32(IntPtr ptr);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetPixData", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageGetPixData32(IntPtr ptr, byte[] data);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetHeader", CallingConvention = CallingConvention.Cdecl)]
            public static extern int FitsImageGetHeader32(IntPtr ptr, [MarshalAs(UnmanagedType.LPStr)] StringBuilder sb);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetOutputSize", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageMeta FitsImageGetOutputSize32(IntPtr ptr);

            public static IntPtr FitsImageCreate(string path)
            {
                return Is64 ? FitsImageCreate64(Marshal.StringToHGlobalAnsi(path)) : FitsImageCreate32(Marshal.StringToHGlobalAnsi(path));
            }

            public static ImageMeta FitsImageGetMeta(IntPtr ptr)
            {
                return Is64 ? FitsImageGetMeta64(ptr) : FitsImageGetMeta32(ptr);
            }

            public static void FitsImageGetPixData(IntPtr ptr, byte[] data)
            {
                if (Is64)
                    FitsImageGetPixData64(ptr, data);
                else
                    FitsImageGetPixData32(ptr, data);
            }

            public static Dictionary<string, string> FitsImageGetHeader(IntPtr ptr)
            {
                var len = Is64 ? FitsImageGetHeader64(ptr, null) : FitsImageGetHeader32(ptr, null);
                if (len <= 0)
                    return null;

                var sb = new StringBuilder(len + 1);
                var _ = Is64 ? FitsImageGetHeader64(ptr, sb) : FitsImageGetHeader32(ptr, sb);
                string h_str =  sb.ToString();
                var header = new Dictionary<string, string>();
                var test = h_str.Split(';');
                foreach (string entry in h_str.Split(new string[] { "; " }, StringSplitOptions.RemoveEmptyEntries))
                {
                    var kv = entry.Split(':');
                    header.Add(kv[0], kv[1]);
                }
                return header;
            }

            public static ImageMeta FitsImageGetOutputSize(IntPtr ptr)
            {
                return Is64 ? FitsImageGetOutputSize64(ptr) : FitsImageGetOutputSize32(ptr);
            }
        }

        private ImagePanel _ip;


        public MainWindow()
        {
            InitializeComponent();
            Console.WriteLine("*********************");

            Console.WriteLine(Environment.Is64BitProcess);
            //string path = "E:/temp/2020-01-02-1456_6-CapObj_0000.FIT";
            string path = "E:/temp/3ch.fit";
            //string path = "E:/temp/float.fit";

            //string path = "E:/temp/小房牛 M31-009B.fit";

            var fitsImage = NativeMethods.FitsImageCreate(path);
            ImageMeta size = NativeMethods.FitsImageGetMeta(fitsImage);

            var header = NativeMethods.FitsImageGetHeader(fitsImage);

            ImageMeta bufferSize = NativeMethods.FitsImageGetOutputSize(fitsImage);

            byte[] img = new byte[bufferSize.nx * bufferSize.ny * bufferSize.nc];
            NativeMethods.FitsImageGetPixData(fitsImage, img);

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
            //Image.Source = bitmapSource;

            var co = new ContextObject();
            _ip = new ImagePanel(co, header);
            
            _ip.Source = bitmapSource;
            _tabUserPage = new TabItem { Content = _ip };
            MainTab.Items.Add(_tabUserPage);
            MainTab.Items.Refresh();

        }
        TabItem _tabUserPage;

    }
}
