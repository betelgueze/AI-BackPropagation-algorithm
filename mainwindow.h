#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QColor>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum myState{IMAGE_LOADED,IMAGE_CONVERTED,NET_CREATED,NET_LEARNED,TRIMAGE_LOADED,TRANSMITED,DECODED,DYSPLAYED};

    myState m_state;
    int m_maxx;
    int m_maxy;
    int m_function;
    int m_innc;
    int m_minc;
    int m_milc;
    int m_blc;
    double m_lrnrt;
    double m_lmbd;
    int m_maxstep;
    double m_desiredError;

    bool m_settingchanged;

    QPixmap m_inputPixMap;
    QImage m_inputImage;
    QPixmap m_outputPixMap;
    QImage m_outputImage;


public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_BrowseButton_clicked();

    void on_LearnButton_clicked();

    void on_InNeuronsBox_valueChanged(int arg1);

    void on_MiddleNeuronsBox_valueChanged(int arg1);

    void on_MiddleLayersBox_valueChanged(int arg1);

    void on_doubleSpinBox_valueChanged(double arg1);

    void on_doubleSpinBox_2_valueChanged(double arg1);

    void on_MaxStepBox_valueChanged(int arg1);

    void on_desirederrBox_valueChanged(double arg1);

    void on_InCountSlider_valueChanged(int value);

    void on_MiddleCountSlider_valueChanged(int value);

    void on_TransmitButton_clicked();

    void on_SigmodialButton_clicked();

    void on_BipolarButton_clicked();

    void on_MaxStepBox_valueChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QGraphicsScene * m_scene1;
    QGraphicsScene * m_scene2;
    QGraphicsScene * m_scene3;


};

#endif // MAINWINDOW_H
