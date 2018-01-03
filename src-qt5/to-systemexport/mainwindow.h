#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString command, exportFile, homeExport, homePathString, homeDirArchive, launchString, luminaPathString;
    QString importPath, importFile, importhomefile, homeImport, luminaExport, luminaImport, fileList, overallExportCommand;
    QStringList files, itList;
    QMessageBox efinishedMessageBox, ifinishedMessageBox, pkglistMessageBox;


private slots:
    void on_exportButton_clicked();
    void exportHomeDir();

    void on_importButton_clicked();
    void importHomeDir();
    void doImport();

private:
    Ui::MainWindow *ui;
    void efinishedMessage();
    void ifinishedMessage();
};

#endif // MAINWINDOW_H
