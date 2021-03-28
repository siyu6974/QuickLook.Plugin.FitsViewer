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

namespace QuickLook.Plugin.FitsViewer
{
    /// <summary>
    /// Interaction logic for ImagePanel.xaml
    /// </summary>
    public partial class ImagePanel : UserControl
    {
        public ImagePanel(Common.Plugin.ContextObject context)
        {
            InitializeComponent();
        }
    }
}
