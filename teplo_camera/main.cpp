#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QTimer>

#include "GuideUSBLiveStream.h"
#include "QDebug"
#include "mainwindow.h"
#include "windows.h"

MainWindow *mw;

// Типы функций (точно по хедеру GuideUSBLiveStream.h)
typedef int (*Initialize_t)();
typedef int (*Exit_t)();
typedef int (*GetDeviceList_t)(device_info_list *);
typedef int (*OpenStream_t)(guide_usb_device_info_t *,
                            void(__stdcall *)(const guide_usb_frame_data_t),
                            void(__stdcall *)(const guide_usb_device_status_e));
typedef int (*CloseStream_t)();

void __stdcall OnFrame(const guide_usb_frame_data_t frame_data) {

  /*if (mw->m_isBusy.load()) {
    qDebug() << "GUI is Busy....";
    return; // Если GUI еще занят, просто выбрасываем этот кадр
  }

  mw->m_isBusy.store(true);*/
  if (frame_data.frame_src_data && frame_data.frame_width == TEPLOVIZOR_WIDTH &&
      frame_data.frame_height == TEPLOVIZOR_HEIGHT) {

    const short *y16 = frame_data.frame_src_data; // signed short*
    int w = frame_data.frame_width;
    int h = frame_data.frame_height;
    int totalPixels = w * h;

    short minVal = *std::min_element(y16, y16 + totalPixels);
    short maxVal = *std::max_element(y16, y16 + totalPixels);

    if (minVal == maxVal)
      maxVal = minVal + 1;

    QImage image(w, h, QImage::Format_Grayscale16);

    for (int y = 0; y < h; ++y) {
      quint16 *line = reinterpret_cast<quint16 *>(image.scanLine(y));
      for (int x = 0; x < w; ++x) {
        int i = y * w + x;
        int norm = int((y16[i] - minVal) * 65535LL / (maxVal - minVal));
        norm = qBound(0, norm, 65535);
        line[x] = static_cast<quint16>(norm);
      }
    }
    QMetaObject::invokeMethod(mw, "updateImage", Qt::QueuedConnection,
                              Q_ARG(QImage, image));
  }
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
    // return -1;
  } else {
    qDebug() << "Camera initialized";
  }

  device_info_list devs;
  if (GetDeviceList(&devs) != 1 || devs.devCount == 0) {
    qDebug() << "Камера не найдена";
    Exit();
    // return -1;
  } else {
    qDebug() << "Камер найдено: " << devs.devCount;
  }

  // Запуск потока 640x512 Y16
  guide_usb_device_info_t info = {640, 512, Y16};
  if (OpenStream(&info, OnFrame, OnStatus) != 1) {
    qDebug() << "OpenStream failed";
    Exit();
    // return -1;
  }

  mw->show();
  return a.exec();
}
