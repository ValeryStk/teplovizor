#include "GuideUSBLiveStream.h"
#include "QDebug"
#include "mainwindow.h"
#include "windows.h"
#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QTimer>
QLabel *label;
MainWindow *mw;

// Типы функций (точно по хедеру)
typedef int (*Initialize_t)();
typedef int (*Exit_t)();
typedef int (*GetDeviceList_t)(device_info_list *);
typedef int (*OpenStream_t)(guide_usb_device_info_t *,
                            void(__stdcall *)(const guide_usb_frame_data_t),
                            void(__stdcall *)(const guide_usb_device_status_e));
typedef int (*CloseStream_t)();

void __stdcall OnFrame(const guide_usb_frame_data_t frame_data) {
  if (frame_data.frame_src_data && frame_data.frame_width == 640 &&
      frame_data.frame_height == 512) {
    /*qDebug() << "КАДР Y16!" << frame_data.frame_width << "x"
             << frame_data.frame_height
             << "пикселей:" << frame_data.frame_src_data_length / 2;*/

    const short *y16 = frame_data.frame_src_data; // ← short*, не ushort!
    int w = frame_data.frame_width, h = frame_data.frame_height;
    int totalPixels = w * h;

    // Нормализация signed short → 0-255 (учитываем min/max)
    short minVal = *std::min_element(y16, y16 + totalPixels);
    short maxVal = *std::max_element(y16, y16 + totalPixels);
    // qDebug() << "Диапазон Y16:" << minVal << ".." << maxVal;

    if (minVal == maxVal)
      minVal = maxVal - 1; // избегать деления на 0

    uchar *rgbData = new uchar[w * h * 4]; // ARGB32
    for (int i = 0; i < totalPixels; ++i) {
      int norm = (y16[i] - minVal) * 255 / (maxVal - minVal);
      norm = qBound(0, norm, 255);

      // Тепловой colormap (jet-like: синий-холодный → красный-горячий)
      uchar r, g, b;
      if (norm < 85) {
        r = 0;
        g = norm * 3;
        b = 255;
      } else if (norm < 170) {
        r = (norm - 85) * 3;
        g = 255;
        b = 255 - (norm - 85) * 3;
      } else {
        r = 255;
        g = 255 - (norm - 170) * 3;
        b = 0;
      }
      rgbData[i * 4 + 0] = b;   // B
      rgbData[i * 4 + 1] = g;   // G
      rgbData[i * 4 + 2] = r;   // R
      rgbData[i * 4 + 3] = 255; // A
    }

    QImage image(rgbData, w, h, QImage::Format_ARGB32_Premultiplied,
                 [](void *data) { delete[] static_cast<uchar *>(data); });

    QImage img = image.copy();
    QMetaObject::invokeMethod(mw, "updateImage", Qt::QueuedConnection,
                              Q_ARG(QImage, img));
  }

  /*if (frame_data.frame_rgb_data) {
    // qDebug() << "RGB готов:" << frame_data.frame_rgb_data_length << "байт";
  }*/
}

void __stdcall OnStatus(const guide_usb_device_status_e device_status) {
  QString status =
      (device_status == DEVICE_CONNECT_OK) ? "ПОДКЛЮЧЕНО" : "ОТКЛЮЧЕНО";
  qDebug() << "=== СТАТУС КАМЕРЫ ===" << status;
}

int main(int argc, char *argv[]) {

  QApplication a(argc, argv);
  mw = new MainWindow;

  HMODULE dll = LoadLibrary(L"GuideUSB3LiveStream.dll");
  if (!dll) {
    qDebug() << "GuideUSB.dll не найдена!";
    return -1;
  } else {
    qDebug() << "Library loaded...";
  }
  auto Initialize = (Initialize_t)GetProcAddress(dll, "Initialize");
  auto GetDeviceList = (GetDeviceList_t)GetProcAddress(dll, "GetDeviceList");
  auto OpenStream = (OpenStream_t)GetProcAddress(dll, "OpenStream");
  auto CloseStream = (CloseStream_t)GetProcAddress(dll, "CloseStream");
  auto Exit = (Exit_t)GetProcAddress(dll, "Exit");

  if (!Initialize || !GetDeviceList || !OpenStream || !CloseStream || !Exit) {
    qDebug() << "Функции не найдены!";
    FreeLibrary(dll);
    return -1;
  } else {
    qDebug() << "Функции найдены!";
  }

  // Инициализация
  if (Initialize() != 1) {
    qDebug() << "Initialize() failed";
    return -1;
  } else {
    qDebug() << "Camera initialized";
  }

  device_info_list devs;
  if (GetDeviceList(&devs) != 1 || devs.devCount == 0) {
    qDebug() << "Камера не найдена";
    Exit();
    return -1;
  } else {
    qDebug() << "Камер найдено: " << devs.devCount;
  }

  // Запуск потока 640x512 Y16
  guide_usb_device_info_t info = {640, 512, Y16};
  if (OpenStream(&info, OnFrame, OnStatus) != 1) {
    qDebug() << "OpenStream failed";
    Exit();
    return -1;
  }

  qDebug() << "Тепловизор работает! Ждем кадры... (30 сек)";

  /*QTimer::singleShot(5000, [&]() {
    CloseStream();
    Exit();
    FreeLibrary(dll);
    QCoreApplication::quit();
  });*/

  mw->show();
  return a.exec();
}
