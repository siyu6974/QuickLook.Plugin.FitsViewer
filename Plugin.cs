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
using QuickLook.Plugin.ImageViewer;
using System.Text;
using System.Collections.Generic;

namespace QuickLook.Plugin.FitsViewer
{
    public struct ImageDim
    {
        public int nx;
        public int ny;
        public int nc;
        public int depth;
    };


    public class Plugin : IViewer
    {
        internal static class NativeMethods
        {
            private static readonly bool Is64 = Environment.Is64BitProcess;

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageCreate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            public static extern IntPtr FitsImageCreate64(IntPtr path);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetMeta", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageDim FitsImageGetMeta64(IntPtr ptr);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetPixData", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageGetPixData64(IntPtr ptr, byte[] data);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetHeader", CallingConvention = CallingConvention.Cdecl)]
            public static extern int FitsImageGetHeader64(IntPtr ptr, [MarshalAs(UnmanagedType.LPStr)] StringBuilder sb);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageGetOutputDim", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageDim FitsImageGetOutputDim64(IntPtr ptr);

            [DllImport(@"viewer_core.dll", EntryPoint = "FitsImageDestroy", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageDestroy64(IntPtr ptr);


            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageCreate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
            public static extern IntPtr FitsImageCreate32(IntPtr path);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetMeta", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageDim FitsImageGetMeta32(IntPtr ptr);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetPixData", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageGetPixData32(IntPtr ptr, byte[] data);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetHeader", CallingConvention = CallingConvention.Cdecl)]
            public static extern int FitsImageGetHeader32(IntPtr ptr, [MarshalAs(UnmanagedType.LPStr)] StringBuilder sb);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageGetOutputDim", CallingConvention = CallingConvention.Cdecl)]
            public static extern ImageDim FitsImageGetOutputDim32(IntPtr ptr);

            [DllImport(@"viewer_core32.dll", EntryPoint = "FitsImageDestroy", CallingConvention = CallingConvention.Cdecl)]
            public static extern void FitsImageDestroy32(IntPtr ptr);

            public static IntPtr FitsImageCreate(string path)
            {
                return Is64 ? FitsImageCreate64(Marshal.StringToHGlobalAnsi(path)) : FitsImageCreate32(Marshal.StringToHGlobalAnsi(path));
            }

            public static ImageDim FitsImageGetMeta(IntPtr ptr)
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
                string h_str = sb.ToString();
                var header = new Dictionary<string, string>();
                var test = h_str.Split(';');
                foreach (string entry in h_str.Split(new string[] { "; " }, StringSplitOptions.RemoveEmptyEntries))
                {
                    var kv = entry.Split(':');
                    header.Add(kv[0], kv[1]);
                }
                return header;
            }

            public static ImageDim FitsImageGetOutputDim(IntPtr ptr)
            {
                return Is64 ? FitsImageGetOutputDim64(ptr) : FitsImageGetOutputDim32(ptr);
            }

            public static void FitsImageDestroy(IntPtr ptr)
            {
                if (Is64)
                {
                    FitsImageDestroy64(ptr);
                }
                else
                {
                    FitsImageDestroy32(ptr);
                }
            }
        }


        public int Priority => 0;
        private ImagePanel _ip;
        private IntPtr _fitsImagePtr;

        public void Init()
        {
        }

        public bool CanHandle(string path)
        {
            return !Directory.Exists(path) && (path.ToLower().EndsWith(".fits") ||
                path.ToLower().EndsWith(".fit") || path.ToLower().EndsWith(".fts"));
        }

        public void Prepare(string path, ContextObject context)
        {
            _fitsImagePtr = NativeMethods.FitsImageCreate(path);
            ImageDim outputDim = NativeMethods.FitsImageGetOutputDim(_fitsImagePtr);

            var size = new Size(outputDim.nx, outputDim.ny);
            context.SetPreferredSizeFit(size, 0.8);
        }

        public void View(string path, ContextObject context)
        {
            var header = NativeMethods.FitsImageGetHeader(_fitsImagePtr);

            ImageDim outputDim = NativeMethods.FitsImageGetOutputDim(_fitsImagePtr);

            byte[] img = new byte[outputDim.nx * outputDim.ny * outputDim.nc];
            NativeMethods.FitsImageGetPixData(_fitsImagePtr, img);

            BitmapSource bitmapSource;
            int rawStride = outputDim.nx * outputDim.nc;
            if (outputDim.nc == 3)
            {
                bitmapSource = BitmapSource.Create(outputDim.nx, outputDim.ny, 300, 300, PixelFormats.Rgb24, null, img, rawStride);
            }
            else
            {
                bitmapSource = BitmapSource.Create(outputDim.nx, outputDim.ny, 300, 300, PixelFormats.Gray8, null, img, rawStride);
            }

            _ip = new ImagePanel(context, header);

            context.ViewerContent = _ip;
            context.Title = $"{Path.GetFileName(path)}";
            _ip.Source = bitmapSource;

            context.IsBusy = false;
        }

        public void Cleanup()
        {
            if (_fitsImagePtr != IntPtr.Zero)
                NativeMethods.FitsImageDestroy(_fitsImagePtr);

            _ip?.Dispose();
            _ip = null;
        }
    }
}