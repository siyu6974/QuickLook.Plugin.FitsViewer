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
using System.IO;
using System.Windows;
using System.Windows.Controls;
using QuickLook.Common.Plugin;
using System.Runtime.InteropServices;
using System.Windows.Media.Imaging;
using System.Windows.Media;

namespace QuickLook.Plugin.FitsViewer
{
    public struct ImageMeta
    {
        public int nx;
        public int ny;
        public int nc;
        public int depth;
    };


    public class Plugin : IViewer
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


        public int Priority => 0;
        private ImagePanel _ip; 

        public void Init()
        {
        }

        public bool CanHandle(string path)
        {
            return !Directory.Exists(path) && (path.ToLower().EndsWith(".fits") ||
                path.ToLower().EndsWith(".fit"));
        }

        public void Prepare(string path, ContextObject context)
        {
            context.PreferredSize = new Size { Width = 800, Height = 600 };
        }

        public void View(string path, ContextObject context)
        {
            _ip = new ImagePanel(context);

            context.ViewerContent = _ip;
            context.Title = $"{Path.GetFileName(path)}";

            var fitsImage = FitsImageCreate(Marshal.StringToHGlobalAnsi(path));
            ImageMeta size = FitsImageGetMeta(fitsImage);
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
            _ip.Image.Source = bitmapSource;

            context.IsBusy = false;
        }

        public void Cleanup()
        {
        }
    }
}