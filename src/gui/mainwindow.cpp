#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <iostream>
#include <QDir>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gcodeview.h"
#include "gcodeviewapplication.h"
#include "slicingdialog.h"

#include "mgl/miracle.h"
#include "mgl/configuration.h"

using namespace mgl;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // Build the 'recent files' menu
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(triggered()),
                this, SLOT(openRecentFile()));
    }

    QMenu* recentFilesMenu = ui->menuBar->findChild<QMenu *>("menuRecent_Files");

    if (recentFilesMenu != NULL) {
        for (int i = 0; i < MaxRecentFiles; ++i)
            recentFilesMenu->addAction(recentFileActs[i]);
    }

    // Reload the recent file menu
    updateRecentFileActions();

    // Reload the 'windows' menu
    updateWindowMenu();

    ui->progressBar->setValue(0);
    QString config = QDir::currentPath()+"/miracle.config";
    QFile f(config);
    if(f.exists(config))
    {
        ui->lineEditConfigFile->setText(config);
    }
    string title = "Miracle-Grue ";
    title += mgl::getMiracleGrueVersionStr().c_str();
    this->setWindowTitle(title.c_str());

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionLoad_GCode_triggered()
{

    QString fileName;
    {
        fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                "",//"/home",
                                                   tr("GCode (*.gcode)") //  tr("3D Models (*.stl);;GCode (*.gcode)")
                                                    );
    }
    GCodeViewApplication::LoadFile(fileName);
    setCurrentFile(fileName);
}

// TODO: Move this to the app class
void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    setWindowFilePath(curFile);

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
        if (mainWin)
            mainWin->updateRecentFileActions();
    }
}

void MainWindow::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    // TODO: Disable 'recent files' menu if there aren't any.
//    separatorAct->setVisible(numRecentFiles > 0);
}

void MainWindow::updateWindowMenu()
{
    QMenu* windowMenu = ui->menuBar->findChild<QMenu *>("menuWindow");

    if (windowMenu) {
        // TODO: Does this cause a memory leak?
        windowMenu->clear();

        // Re-add the windows
        foreach (QWidget *widget, QApplication::topLevelWidgets()) {
            MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
            if (mainWin) {
                QAction* action = new QAction(this);
                action->setText(mainWin->windowTitle());
                windowMenu->addAction(action);
 //               windowMenu->addAction(recentFileActs[i]);
            }
        }
    }
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        GCodeViewApplication::LoadFile(action->data().toString());
}



bool MainWindow::hasFile() {
    return ui->graphicsView->hasModel();
}

void MainWindow::on_LayerHeight_sliderMoved(int position)
{
    // TODO: where /should/ this signal go?
    ui->graphicsView->setMaximumVisibleLayer(position);
    ui->LayerMin->setMaximum(ui->graphicsView->model.getMapSize() );

    // display the current layer height.
}

void MainWindow::on_LayerMin_valueChanged(int value)
{
    ui->graphicsView->setMinimumVisibleLayer(value);
}

void MainWindow::on_zoomIn_clicked()
{
    ui->graphicsView->zoom(1.1);
}

void MainWindow::on_zoomOut_clicked()
{
    ui->graphicsView->zoom(.9);
}

void MainWindow::on_actionClose_triggered()
{
    // Close this window.
    this->close();

    // TODO: Can the application get a signal when this happens, instead of sending it explicitly here?
}

void MainWindow::on_actionExport_Gcode_File_triggered()
{
    static std::string configFile;
    if(configFile.size()==0)
    {
        configFile = QDir::currentPath().toStdString();
        configFile += "/miracle.config";
    }
    static std::string modelFile;

    // QString filename = QFileDialog::getSaveFileName(this, tr("Export GCode"), QDir::currentPath(),  tr("GCode File (*.gcode)"));
    // ui->graphicsView->exportModel(filename);
    SlicingDialog *dlg = new SlicingDialog;
    dlg->show();

    // dlg->init("miracle.config", "3D_Knot.stl");
}



void MainWindow::on_panLeft_clicked()
{
    ui->graphicsView->panX(1.0);
}

void MainWindow::on_panRight_clicked()
{
    ui->graphicsView->panX(-1.0);
}

void MainWindow::on_panUp_clicked()
{
    ui->graphicsView->panY(-1.0);
}

void MainWindow::on_panDown_clicked()
{
    ui->graphicsView->panY(1.0);
}

void MainWindow::on_buttonConfigBrowse_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Miracle-Grue configurations (*.config)") );
    ui->lineEditConfigFile->setText(fileName);

}

void MainWindow::on_button3dModelBrowse_clicked()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("3D models (*.stl)") );
    ui->lineEdit3dModelFile->setText(fileName);
    sliceModelAndCreateToolPaths();
}

void MainWindow::on_buttonSlice_clicked()
{

}


void MainWindow::sliceModelAndCreateToolPaths()
{
    class Progress : public ProgressBar
    {
        QLabel &taskLabel; QProgressBar& progress;
    public:
        Progress(QLabel *taskLabelp, QProgressBar* progressp)
            :taskLabel(*taskLabelp), progress(*progressp)
        {

        }

        void onTick(const char* taskName, unsigned int count, unsigned int tick)
        {
            if(tick==0)
            {
                taskLabel.setText(taskName);
                progress.setMinimum(0);
                progress.setMaximum(count);

            }
            progress.setValue(tick+1);
            // cout << taskName << " tick: " << tick << "/" << count << endl;
        }
    };

    try
    {
        cout << "Output file: ";
        MyComputer computer;

        string configFileName = ui->lineEditConfigFile->text().toStdString();
        string model3dfileName = ui->lineEdit3dModelFile->text().toStdString();

        string gcodeFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(model3dfileName.c_str()).c_str(), ".gcode" );
        cout << gcodeFile << endl;
        string scadFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(model3dfileName.c_str()).c_str(), ".scad" );
        cout << scadFile << endl;

        //configFileName += "/miracle.config";
        mgl::Configuration config;
        cout << "loading config: " << configFileName << endl;
        config.readFromFile(configFileName.c_str());

        GCoder gcoder;
        loadGCoderData(config, gcoder);
        Slicer slicerCfg;
        loadSlicerData(config, slicerCfg);

        std::vector<mgl::SliceData> slices;

        Progress progress(ui->label_task, ui->progressBar);
        miracleGrue(gcoder, slicerCfg, model3dfileName.c_str(), NULL, gcodeFile.c_str(), -1, -1, slices, &progress);

        ui->graphicsView->loadSliceData(slices);

        ui->LayerHeight->setMaximum(ui->graphicsView->model.getMapSize() );
        ui->LayerMin->setMaximum(ui->graphicsView->model.getMapSize() );
        ui->LayerMin->setValue(0 );
        ui->LayerHeight->setValue(0 );


    }
    catch(mgl::Exception &mixup)
    {
        cout << "ERROR: " << mixup.error << endl;
        QMessageBox &box = *new QMessageBox();
        box.setText(mixup.error.c_str());
        box.show();

    }
    catch(...)
    {
         cout << "ERROR" << endl;
    }
}

void MainWindow::loadFile(const QString &fileName) {
    setWindowTitle(strippedName(fileName));

    QAction* closeMenu = ui->menuBar->findChild<QAction *>("actionClose");
    if (closeMenu) {
        closeMenu->setText("Close \"" + fileName + "\"");
    }
    else {
        //std::cout << "no menu?" << std::endl;
    }

    // TODO: How to back off here if model load failed? Should we close the window, etc?
    // TODO: Do loading in background task...

    ui->graphicsView->loadModel(fileName);

    ui->LayerHeight->setMaximum(ui->graphicsView->model.getMapSize() );
    ui->LayerMin->setMaximum(ui->graphicsView->model.getMapSize() );
    ui->LayerMin->setValue(0 );
    ui->LayerHeight->setValue(0 );

}

void MainWindow::on_LayerMin_destroyed(QObject *arg1)
{

}


