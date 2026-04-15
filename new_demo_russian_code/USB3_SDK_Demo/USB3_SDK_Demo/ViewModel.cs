using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows.Media;

namespace USB3_SDK_Demo
{
    public class ViewModel : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public void Notify(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public ImageBrush CurrImageBrush
        {
            get
            {
                return currImageBrush;
            }
            set
            {
                currImageBrush = value;
                Notify("CurrImageBrush");
            }
        }
        private ImageBrush currImageBrush = new ImageBrush();

        public int FPS
        {
            get
            {
                return _FPS;
            }
            set
            {
                _FPS = value;
                Notify("FPS");
            }
        }
        private int _FPS;

    }
}
