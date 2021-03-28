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
        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr FitsImageCreate(IntPtr path);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ImageMeta FitsImageGetMeta(IntPtr ptr);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void FitsImageGetPixData(IntPtr ptr, byte[] data);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr FitsImageGetHeader(IntPtr ptr);

        [DllImport(@"viewer_core.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern ImageMeta FitsImageGetOutputSize(IntPtr ptr);


        public MainWindow()
        {
            InitializeComponent();
            //string path = "E:/temp/2020-01-02-1456_6-CapObj_0000.FIT";
            string path = "E:/temp/3ch.fit";
            //string path = "E:/temp/float.fit";

            //string path = "E:/temp/小房牛 M31-009B.fit";

            var fitsImage = FitsImageCreate(Marshal.StringToHGlobalAnsi(path));
            ImageMeta size = FitsImageGetMeta(fitsImage);

            // NOT WORKING
            //string h = Marshal.PtrToStringAnsi(FitsImageHeader(fitsImage));

            ImageMeta bufferSize = FitsImageGetOutputSize(fitsImage);

            byte[] img = new byte[bufferSize.nx * bufferSize.ny * bufferSize.nc];
            FitsImageGetPixData(fitsImage, img);

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
