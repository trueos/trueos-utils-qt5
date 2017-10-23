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
    QString exportFile, homeExport, homePathString, homeDirArchive, files;


private slots:
    void on_exportButton_clicked();
    void exportHomeDir();

private:
    Ui::MainWindow *ui;
    void finishedMessage();

};

#endif // MAINWINDOW_H
