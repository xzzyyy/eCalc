#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QWindow>
#include <QThread>
#include <QScreen>
#include <QDesktopWidget>
#include <QProcess>
#include <QDir>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QDate>
#include <QClipboard>
#include <QPixmap>
#include <QScrollBar>

#include <QxtWidgets/QxtGlobalShortcut>
#include <windows.h>

#include "logger.h"
#include "protection.h"

#define SAVE_SCREENSHOTS

static const int dragRectangleTranslucency = 50;
//static const int shadeTranslucency = 100;
static const int shadeTranslucency = 150;

//static const QColor workingModeColor(204, 204, 204, dragRectangleTranslucency);

static const QColor additionColor(255, 242, 204, shadeTranslucency);
static const QColor subtractionColor(226, 239, 218, shadeTranslucency);
static const QColor divisionColor(237, 237, 237, shadeTranslucency);
static const QColor multiplicationColor(252, 228, 214, shadeTranslucency);

#define OP_COLUMN       0
#define NUMBER_COLUMN   1
#define RESULT_COLUMN   2
#define BUTTON_COLUMN   3

static const QString operationsStr[4] = {"+", "-", "*", "/"};

static const QString errorMarker("Error");

#define PREVIEW_DELAY   500                     // in ms
#define DAY_OF_BLOCKING QDate(2099, 12, 31)

//Q_DECLARE_METATYPE(QWidget*)

namespace Ui {
class MainWindow;
}

class TranslucentRoundWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool eventFilter(QObject *, QEvent *);
    
private slots:
    void on_actionActive_mode_toggled(bool arg1);
    void makeScreenshot(const QRect& rect);
    void on_pushButtonPlus_clicked();
    void on_pushButtonMinus_clicked();
    void on_pushButtonMult_clicked();
    void on_pushButtonDiv_clicked();
    void resizeColumns();
    void on_actionExit_selection_mode_triggered();
    void on_actionExit_triggered();
    void removeLastNumbers();
    void forgetLastNumbers();
    void on_actionAddition_triggered();
    void on_actionSubtraction_triggered();
    void on_actionDivision_triggered();
    void on_actionMultiplication_triggered();
    void modeToggled();
    void on_actionCopy_result_triggered();
    void on_pushButton_clicked();
    void removeButtonClicked();
    void on_actionClear_triggered();
    void on_pushButtonClear_clicked();
    void on_tableWidgetNumbers_cellChanged(int row, int column);
    void on_tableWidgetNumbers_cellActivated(int row, int column);
    void on_tableWidgetNumbers_cellDoubleClicked(int row, int column);

private:
    Ui::MainWindow *ui;
    QList<TranslucentRoundWindow*> weirdWindows;
    int operation;
    QVector<QPushButton*> pushButtonsOperation;
    int lastRowCount;
    bool activeMode;
    Logger *logger;
    int curScreenshotIdx;
    QPoint editedCell;

    void preprocess(const QString& fileName);
    void doOcr();
    QList<double> extractNumbers(const QString& str);
    void updateNumberList(const QList<double> &numbers);
    QStringList cutNumbers(QString number);
    QString calculate(const QString& arg1, double arg2, const QString &op = "");
    QString floatToStr(double num);
    double strToFloat(QString num);
    void updateOpColors();
    void removeRow(int row);
    int getScreenshotIndex();
    void createDbgArchive();
    void myAddActions(QWidget* widget);
    void updateFromRow(int row);
    void calculateRow(int row);

    void hideRectangle();
    void removeSelection();
    void moveSelWindowsOnTop();

protected:
    virtual void closeEvent(QCloseEvent *);
};

class TranslucentRoundWindow : public QWidget
{
    Q_OBJECT
public:
    TranslucentRoundWindow(QWidget *parent = 0);
    virtual QSize sizeHint() const;
    void setDragging(bool on);
    void setShowSelection(bool on);
    void updateOpColor(const QColor& color);

protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QSize prevSize;
    bool dragging;
    QPoint firstPoint;
    QRect highlight;
    int instanceId;
    bool showSelection;
    QColor opColor;
    QTimer previewTimer;
    bool lastCaptureWasPreview;
    QPoint previewPos;

    void drawRectangle(int x, int y, int w, int h);
    void capture();

signals:
    void makeScreenshot(const QRect& rect);                 // in virtual desktop coords
    void removeLastNumbers();
    void forgetLastNumbers();

private slots:
    void previewCaptureNumbers();
};

#endif // MAINWINDOW_H
