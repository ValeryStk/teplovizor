#include "mainwindow.h"

#include <QDateTime>
#include <QDebug>
#include <QMouseEvent>
#include <QtConcurrent>
#include <random>

#include "QTimer"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_timer(new QTimer) {
    ui->setupUi(this);
    connect(m_timer, &QTimer::timeout, this,
            &MainWindow::createTestRandomImage);
    m_timer->start(200);
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
    if (!m_lastImage.isNull() && m_cursor_point.x() >= 0 &&
        m_cursor_point.x() < m_lastImage.width() && m_cursor_point.y() >= 0 &&
        m_cursor_point.y() < m_lastImage.height()) {
        // 3. Доступ к 16-битным данным напрямую
        const unsigned short *line = reinterpret_cast<const unsigned short *>(
            m_lastImage.scanLine(m_cursor_point.y()));
        unsigned short pixelValue = line[m_cursor_point.x()];
        const QString valueText = QString("x:%1 y:%2 value:%3")
                                      .arg(m_cursor_point.x())
                                      .arg(m_cursor_point.y())
                                      .arg(pixelValue);
        ui->label_cursor_value->setText(valueText);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->label_image && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        // 1. Получаем координаты курсора относительно QLabel
        int x = mouseEvent->pos().x();
        int y = mouseEvent->pos().y();
        m_cursor_point = QPoint(x, y);
    }
    return QMainWindow::eventFilter(obj, event);
}

// Метод, который инициирует сохранение
void MainWindow::saveImageAsync(const QImage &img, const QString &fileName) {
    // Запускаем сохранение в фоновом потоке
    // Используем copy() изображения, чтобы передать его "владение" в другой
    // поток
    QtConcurrent::run([img, fileName]() {
        bool success = img.save(fileName, "PNG");
        if (!success) {
            qWarning() << "Failed to save image to" << fileName;
        }
    });
}

void MainWindow::on_pushButton_singleShot_clicked() {
    QImage img = m_lastImage.copy();
    saveImageAsync(
        img, QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz"));
}
