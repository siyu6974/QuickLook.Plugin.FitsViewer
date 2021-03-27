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


using System.IO;
using System.Windows;
using System.Windows.Controls;
using QuickLook.Common.Plugin;

namespace QuickLook.Plugin.FitsViewer
{
    public class Plugin : IViewer
    {
        public int Priority => 0;

        public void Init()
        {
        }

        public bool CanHandle(string path)
        {
            return !Directory.Exists(path) && path.ToLower().EndsWith(".zzz");
        }

        public void Prepare(string path, ContextObject context)
        {
            context.PreferredSize = new Size {Width = 600, Height = 400};
        }

        public void View(string path, ContextObject context)
        {
            var viewer = new Label {Content = "I am a Label. I do nothing at all."};

            context.ViewerContent = viewer;
            context.Title = $"{Path.GetFileName(path)}";

            context.IsBusy = false;
        }

        public void Cleanup()
        {
        }
    }
}