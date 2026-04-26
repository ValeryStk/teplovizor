#include "mainwindow.h"

#include <QDebug>
#include <QMouseEvent>
#include <random>

#include "QTimer"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_timer(new QTimer) {
    ui->setupUi(this);
    connect(m_timer, &QTimer::timeout, this,
            &MainWindow::createTestRandomImage);
    m_timer->start(1000);
    ui->label_image->setMouseTracking(true);  // Важно: включить отслеживание
    ui->label_image->installEventFilter(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::updateImage(QImage img) {
    ui->label_image->setPixmap(QPixmap::fromImage(img));
}

void MainWindow::createTestRandomImage() {
    const int width = TEPLOVIZOR_WIDTH;
    const int height = TEPLOVIZOR_HEIGHT;

    // 1. Создаем буфер данных (16 бит на пиксель)
    std::vector<unsigned short> data(width * height);

    // Заполняем случайными значениями 0–65535
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<unsigned short> dist(0, 65535);
    for (auto &pixel : data) {
        pixel = dist(rng);
    }

    // 2. Создаем QImage, используя данные из вектора
    // Важно: данные должны существовать, пока живет QImage (здесь они в
    // векторе)
    QImage img(reinterpret_cast<uchar *>(data.data()), width, height,
               width * sizeof(unsigned short), QImage::Format_Grayscale16);
    m_lastImage = img.copy();
    updateImage(img);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->label_image && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // 1. Получаем координаты курсора относительно QLabel
        int x = mouseEvent->pos().x();
        int y = mouseEvent->pos().y();

        // 2. Получаем текущее изображение (нужно хранить m_lastImage в классе)
        if (!m_lastImage.isNull() && x >= 0 && x < m_lastImage.width() &&
            y >= 0 && y < m_lastImage.height()) {
            // 3. Доступ к 16-битным данным напрямую
            const unsigned short *line =
                reinterpret_cast<const unsigned short *>(
                    m_lastImage.scanLine(y));
            unsigned short pixelValue = line[x];

            qDebug() << "Pixel at (" << x << "," << y
                     << ") value:" << pixelValue;
            // Здесь можно обновить какой-нибудь QLabel с текстом:
            // ui->label_value->setText(QString::number(pixelValue));
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
