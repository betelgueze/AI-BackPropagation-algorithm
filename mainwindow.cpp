                                                                                   #include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QColor>
#include <QDebug>

#include "sfc1.hpp"

Bpn * m_nt;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_function = 1;
    m_maxstep = 500;
    m_desiredError = 0.1;
    m_lmbd = 0.2;
    m_innc = 64;
    m_minc = 16;
    m_milc = 1;
    m_lrnrt = 0.5;
    m_nt = NULL;
    m_settingchanged = true;
    ui->setupUi(this);
    m_scene1 = m_scene2 = m_scene3 = NULL;
}

MainWindow::~MainWindow()
{
    delete m_nt;
    delete ui;
}

/**
    \brief After browsing a image file, the file is converted to grayscale(8bit) format and required blocks to represent it are eveluated
*/
void MainWindow::on_BrowseButton_clicked()
{
    QString file1Name = QFileDialog::getOpenFileName(this,
            tr("Open Input Image File"), "/home/betelgueze/Downloads", tr("All files(*.*);;PNG files (*.png);;BMP file (*.bmp)"));

    if(file1Name.isEmpty())
    {
        QMessageBox::information(this,"Image Viewer","Error Displaying image");
        return;
    }
    QImage imageSRC(file1Name);
    int blocksize = (int)sqrt(m_innc);
    imageSRC = imageSRC.scaled((imageSRC.width()/ blocksize) * blocksize,(imageSRC.height()/ blocksize) * blocksize,Qt::KeepAspectRatio);


    delete m_scene1;


    m_scene1 = new QGraphicsScene(this);
    QPixmap ii(QPixmap::fromImage(imageSRC));

    m_scene1->addPixmap(ii);
    //->scaled(inputWidth.toUInt(), inputHeight.toUint,Qt::KeepAspectRatio);
    m_scene1->setSceneRect(ii.rect());

    ui->InView->setScene(m_scene1);

    ui->InView->show();

    myState oldstate = m_state;
    m_state = IMAGE_LOADED;
    ui->StetusTextLabel->setText("IMAGE LOADED, please click the learn button");
    ui->StetusTextLabel->show();

    ui->InMView->hide();
    delete m_scene2;

    m_scene2 = new QGraphicsScene(this);
    QImage imageGRAY(imageSRC.convertToFormat(QImage::Format_Indexed8));
    if (!imageGRAY.isNull())
    {
        m_maxx = imageGRAY.height();
        m_maxy = imageGRAY.width();
/*        int pixels = imageGRAY.width() * imageGRAY.height();
        for (int i = 0; i < pixels; i++)
        {
            int val = 0;//256-data[i];
            imageGRAY.setColor(i,qRgb(val, val, val));
        }*/


        ui->BlocksBox->setValue((m_maxx / blocksize) * (m_maxy / blocksize));
        ui->BlocksBox->show();
        ui->BlocksCountSlider->setValue((m_maxx / blocksize) * (m_maxy / blocksize));
        ui->BlocksCountSlider->show();
    }
    QVector<QRgb> my_table;
    for(int i = 0; i > 256; i++) my_table.push_back(qRgb(i,i,i));
    imageGRAY.setColorTable(my_table);

    m_inputPixMap = QPixmap::fromImage(imageGRAY);
    m_inputImage = m_inputPixMap.toImage();


    m_scene2->addPixmap(m_inputPixMap);
    m_scene2->setSceneRect(m_inputPixMap.rect());

    ui->InMView->setScene(m_scene2);
    ui->InMView->show();

    m_state = IMAGE_CONVERTED;
    if(oldstate != NET_LEARNED)
    {
        ui->StetusTextLabel->setText("IMAGE CONVERTED, Click learn");
        ui->LearnButton->setEnabled(true);
        ui->LearnButton->show();
    }
    else
    {
        ui->StetusTextLabel->setText("IMAGE CONVERTED, NET LEARNED, Click Transmit");

        ui->LearnButton->setEnabled(false);
        ui->LearnButton->show();
        ui->TransmitButton->setEnabled(true);
        ui->TransmitButton->show();

        m_state = TRIMAGE_LOADED;
    }

    ui->BrowseButton->setEnabled(false);
    ui->BrowseButton->show();

    ui->StetusTextLabel->show();
    return;
}

#define MAP_XY(block,i,j,k,maxjb) \
    ((i +  (k / block))*(maxjb) + j + k % block)

#define DEMAP_XY(block,k,index,maxblocks,maxj) \
    (maxj*(k / maxblocks) + (k % maxblocks)*block + (index / block) * maxj + (index % block))

#define DEMAP_X(block,k,index,maxblocks) \
   ((k / maxblocks)*block + (index / block))

#define DEMAP_Y(block,k,index,maxblocks) \
    ((k % maxblocks)*block + (index % block))

void _BlockToImage(QImage * img, int index, std::vector<Neuron *>& neurons,int maxblocks,int blocksize)
{
    for(unsigned i=0; i < neurons.size(); ++i)
    {
        //QRgb * ddata  = (QRgb *)img.bits();
        //QRgb * tmp = &ddata[DEMAP_XY(blocksize,index,i,maxblocks,maxy)];

        int val;

        if(m_Ftype == BIPOLAR)
            val = (int)(128*neurons[i]->Value())+128;
        else if (m_Ftype == SIGMODIAL)
            val = (int)256*neurons[i]->Value();

        if(val > 255 )
            val=255;
        img->setPixel(DEMAP_Y(blocksize,index,i,maxblocks),DEMAP_X(blocksize,index,i,maxblocks),qRgb(val,val,val));
        //qDebug() << "settin pix"<< DEMAP_Y(blocksize,index,i,maxblocks) <<","<< DEMAP_X(blocksize,index,i,maxblocks) << " val="<< val<< "blocksz=" <<blocksize << "maxbl="<<maxblocks;
    }
}

void MainWindow::on_LearnButton_clicked()
{
    if(!m_settingchanged)
        return;

    m_settingchanged = false;

    if(m_state != IMAGE_CONVERTED)
        return;

    if(m_nt != NULL)
        delete m_nt;

    std::vector<int> midllelayerCounts;
    midllelayerCounts.reserve(m_milc);
    for(int i = 0; i < m_milc; ++i)
    {
        midllelayerCounts.push_back(m_minc);
    }
    //delete m_nt;
    m_nt = new Bpn(m_lrnrt,((int) sqrt(m_innc))*((int) sqrt(m_innc)),midllelayerCounts,(m_function == 1)?SIGMODIAL:BIPOLAR,m_lmbd);
    m_state = NET_CREATED;

    int blocksize = sqrt(m_innc);
    std::vector<Layer *> blocksdata;
    blocksdata.reserve((m_maxx / blocksize)*(m_maxy / blocksize));
    for(int i=0; i < (m_maxx / blocksize); ++i)
    {
        for(int j=0; j < (m_maxy / blocksize); ++j)
        {
            std::vector<double> data;
            data.reserve(blocksize*blocksize);
            //construct block
            for(int k=0; k < blocksize*blocksize; ++k)
            {
                QRgb * ddata  = (QRgb *)m_inputImage.bits();
                QRgb * tmp = &ddata[MAP_XY(blocksize,i*blocksize,j*blocksize,k,((m_maxy / blocksize))* blocksize)];

                if(m_Ftype == SIGMODIAL)
                    data.push_back(((double)qGreen(*tmp)/256));
                else if(m_Ftype == BIPOLAR)
                    data.push_back(((double)qGreen(*tmp)-128)/256);
            }
            //transfer integers to neuron values and them into layeras
            std::vector<Neuron *> lauerdata;
            lauerdata.reserve(data.size());
            for(unsigned k=0; k< data.size();++k)
            {
                Neuron * n = new Neuron(data[k]);
                lauerdata.push_back(n);
            }
            Layer * L = new Layer(lauerdata);
            blocksdata.push_back(L);
        }
    }
    //split input 8-bit pixmap into sqrt(INPUTNEURONS_COUNT)*sqrt(INPUTNEURONS_COUNT) size blocks
    //create from them input samples
    m_nt->Learn(blocksdata,m_desiredError*blocksdata.size()*blocksize*blocksize,m_maxstep,ui->StetusTextLabel,ui->progressBar,ui->progressBarSample);
    ui->lberr->setText(QString::number((double)m_nt->GlobalError/(blocksdata.size()*blocksize*blocksize)));
    ui->lberr->show();
    ui->lbstep->setText(QString::number(m_nt->STEP));
    ui->lbstep->show();

    ui->StetusTextLabel->setText("NET IS LEARNED WILL DISPLAY NEURON OUTPUTS!");
    ui->StetusTextLabel->show();

    m_state = NET_LEARNED;
    m_outputImage = QImage(m_inputImage.width(),m_inputImage.height(),QImage::Format_RGB888);
    m_outputPixMap = QPixmap::fromImage(m_outputImage);
    QVector<QRgb> my_table;
    for(int i = 0; i > 256; i++) my_table.push_back(qRgb(i,i,i));
    m_outputImage.setColorTable(my_table);
    //convert output layer to grayscale image
    for(unsigned i=0; i < blocksdata.size(); ++i)
    {
       // qDebug() << "processing" << i << "block";
        std::vector<Neuron *> outValues;

        m_nt->Step(blocksdata[i]->Neurons(),outValues);

        _BlockToImage(&m_outputImage,i,outValues,(m_maxy / blocksize),blocksize);
    }

    m_outputPixMap = QPixmap::fromImage(m_outputImage);

    m_scene3 = new QGraphicsScene(this);
    m_scene3->addPixmap(m_outputPixMap);
    m_scene3->setSceneRect(m_outputPixMap.rect());

    ui->OutView->setScene(m_scene3);
    ui->OutView->show();
    ui->StetusTextLabel->setText("NET IS LEARNED. Browse picture to transmit");
    ui->StetusTextLabel->show();
    ui->BrowseButton->setEnabled(true);
    ui->BrowseButton->show();

    m_state = NET_LEARNED;
   // for(int i=0; i< blocksdata.size();++i)
    //    delete blocksdata[i];
}

void MainWindow::on_InNeuronsBox_valueChanged(int arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_innc = arg1;
    m_settingchanged = true;
    ui->InCountSlider->setValue(arg1);
    ui->InCountSlider->show();
}

void MainWindow::on_MiddleNeuronsBox_valueChanged(int arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_minc = arg1;
    m_settingchanged = true;
}

void MainWindow::on_MiddleLayersBox_valueChanged(int arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_milc = arg1;
    m_settingchanged = true;
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_lrnrt = arg1;
    m_settingchanged = true;
}

void MainWindow::on_doubleSpinBox_2_valueChanged(double arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_lmbd = arg1;
    m_settingchanged = true;
}

void MainWindow::on_MaxStepBox_valueChanged(int arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_maxstep = arg1;
    m_settingchanged = true;
}

void MainWindow::on_desirederrBox_valueChanged(double arg1)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_desiredError = arg1;
    m_settingchanged = true;
}

void MainWindow::on_InCountSlider_valueChanged(int value)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_innc = value;
    if(m_minc >= m_innc)
    {

    }
    m_settingchanged = true;
}

void MainWindow::on_MiddleCountSlider_valueChanged(int value)
{
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_minc = value;
    m_settingchanged = true;
}

void MainWindow::on_TransmitButton_clicked()
{
    if(m_state != TRIMAGE_LOADED)
        return;
    //trnasfer input image to samples
    //perform steps on them
    //transmit
    //display
    int blocksize = sqrt(m_innc);
    std::vector<Layer *> blocksdata;
    blocksdata.reserve((m_maxx / blocksize)*(m_maxy / blocksize));
    for(int i=0; i < (m_maxx / blocksize); ++i)
    {
        for(int j=0; j < (m_maxy / blocksize); ++j)
        {
            std::vector<double> data;
            data.reserve(blocksize*blocksize);
            //construct block
            for(int k=0; k < blocksize*blocksize; ++k)
            {
                QRgb * ddata  = (QRgb *)m_inputImage.bits();
                QRgb * tmp = &ddata[MAP_XY(blocksize,i*blocksize,j*blocksize,k,((m_maxy / blocksize))* blocksize)];

                if(m_Ftype == SIGMODIAL)
                    data.push_back(((double)qGreen(*tmp)/256));
                else if(m_Ftype == BIPOLAR)
                    data.push_back(((double)qGreen(*tmp)-128)/256);
            }
            //transfer integers to neuron values and them into layeras
            std::vector<Neuron *> lauerdata;
            lauerdata.reserve(data.size());
            for(unsigned k=0; k< data.size();++k)
            {
                Neuron * n = new Neuron(data[k]);
                lauerdata.push_back(n);
            }
            Layer * L = new Layer(lauerdata);
            blocksdata.push_back(L);
        }
    }


    for(unsigned i=0; i < blocksdata.size(); ++i)
    {
       // qDebug() << "processing" << i << "block";
        std::vector<Neuron *> outValues;

        m_nt->Step(blocksdata[i]->Neurons(),outValues);

        _BlockToImage(&m_outputImage,i,outValues,(m_maxy / blocksize),blocksize);
    }

    m_outputPixMap = QPixmap::fromImage(m_outputImage);
//    m_outputImage = m_outputPixMap.toImage();

    m_scene3 = new QGraphicsScene(this);
    m_scene3->addPixmap(m_outputPixMap);
    m_scene3->setSceneRect(m_outputPixMap.rect());

    ui->OutView->setScene(m_scene3);
    ui->OutView->show();

    m_state = NET_LEARNED;
    ui->LearnButton->setEnabled(true);
    ui->LearnButton->show();
    ui->BrowseButton->setEnabled(true);
    ui->BrowseButton->show();
//now only create in samples a step them, display output, use progress bar probably
    return;
}

void MainWindow::on_SigmodialButton_clicked()
{
    m_function = 1;
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_settingchanged = true;
}

void MainWindow::on_BipolarButton_clicked()
{
    m_function = 2;
    if(m_state >= IMAGE_CONVERTED)
        m_state = IMAGE_CONVERTED;
    m_settingchanged = true;
}



void MainWindow::on_MaxStepBox_valueChanged(const QString &arg1)
{

}
