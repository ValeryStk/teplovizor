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

private:
    Ui::MainWindow *ui;
};
#endif  // MAINWINDOW_H
