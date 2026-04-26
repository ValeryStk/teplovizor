#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

constexpr int TEPLOVIZOR_WIDTH = 640;
constexpr int TEPLOVIZOR_HEIGHT = 512;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void updateImage(QImage img);

private slots:
    void createTestRandomImage();

    void on_pushButton_singleShot_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *m_timer;
    QImage m_lastImage;
    QPoint m_cursor_point{0, 0};

    void saveImageAsync(const QImage &img, const QString &fileName);

    // QObject interface
public:
    bool eventFilter(QObject *obj, QEvent *event);
};
#endif  // MAINWINDOW_H
