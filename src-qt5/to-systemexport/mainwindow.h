#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString exportFile, homeExport, homePathString, homeDirArchive, files, launchString;
    QString importPath, importFile, importhomefile, homeImport;


private slots:
    void on_exportButton_clicked();
    void exportHomeDir();

    void on_importButton_clicked();
    void importHomeDir();

private:
    Ui::MainWindow *ui;
    void efinishedMessage();
    void ifinishedMessage();

};

#endif // MAINWINDOW_H
