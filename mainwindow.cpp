#include "mainwindow.h"
#include "ui_mainwindow.h"

//Q_DECLARE_METATYPE(QWidget*)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    operation(0),
    pushButtonsOperation(4),
    lastRowCount(-1),
    activeMode(false),
    editedCell(-1, -1)
{
    ui->setupUi(this);

    logger = new Logger("dbg/dbg.log");

    myAddActions(this);

    QxtGlobalShortcut* globalShortcut = new QxtGlobalShortcut(this);
    connect(globalShortcut, SIGNAL(activated()), this, SLOT(modeToggled()));
    globalShortcut->setShortcut(QKeySequence("F11"));

    for (QScreen *s: qApp->screens()) {
        qDebug() << "s->geometry()" << s->geometry();
        TranslucentRoundWindow *weirdWindow = new TranslucentRoundWindow(NULL);
        connect(weirdWindow, SIGNAL(makeScreenshot(QRect)), this, SLOT(makeScreenshot(QRect)));
        connect(weirdWindow, SIGNAL(removeLastNumbers()), this, SLOT(removeLastNumbers()));
        connect(weirdWindow, SIGNAL(forgetLastNumbers()), this, SLOT(forgetLastNumbers()));
        weirdWindow->move(s->geometry().topLeft());
        weirdWindow->resize(s->geometry().size());
        weirdWindow->addAction(ui->actionExit_selection_mode);

        QCursor cursor = weirdWindow->cursor();
        cursor.setShape(Qt::CrossCursor);
        weirdWindow->setCursor(cursor);

        myAddActions(weirdWindow);

        weirdWindows << weirdWindow;
    }

    pushButtonsOperation[0] = ui->pushButtonPlus;
    pushButtonsOperation[1] = ui->pushButtonMinus;
    pushButtonsOperation[3] = ui->pushButtonMult;
    pushButtonsOperation[2] = ui->pushButtonDiv;
    for (QPushButton* b: pushButtonsOperation)
        b->setCheckable(true);
    pushButtonsOperation[operation]->setChecked(true);

    updateOpColors();

    ui->tableWidgetNumbers->setItem(0, 0, new QTableWidgetItem());
    ui->tableWidgetNumbers->setItem(0, 1, new QTableWidgetItem());
    ui->tableWidgetNumbers->setItem(0, 2, new QTableWidgetItem());
    ui->tableWidgetNumbers->setItem(0, 3, new QTableWidgetItem());

    QTimer::singleShot(100, this, SLOT(resizeColumns()));

    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(QIcon("resources/icon2.png"), this);
    trayIcon->setToolTip("eCalc");
    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction(ui->actionExit);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    setWindowIcon(trayIcon->icon());

//    ui->labelActiveMode->hide();


    curScreenshotIdx = getScreenshotIndex();

    qApp->installEventFilter(this);

    new Protection(DAY_OF_BLOCKING, this);

//    Q_DECLARE_METATYPE
//    qRegisterMetaType("QWidget*");
    qRegisterMetaType<QWidget*>();          // without it there is error in release version when pressing enter while editing cell
                                            // "QMetaMethod::invoke: Unable to handle unregistered datatype 'QWidget*'"

    QDir dbgDir;
    if (!dbgDir.exists("dbg"))
        dbgDir.mkdir("dbg");
}

MainWindow::~MainWindow()
{
    for (TranslucentRoundWindow* w: weirdWindows)
        delete w;
    delete ui;

    delete logger;

    createDbgArchive();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == qApp && event->type() == QEvent::ApplicationActivate) {
        qDebug() << Q_FUNC_INFO;

        if (activeMode)
            moveSelWindowsOnTop();
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_actionActive_mode_toggled(bool arg1)
{
    qDebug() << Q_FUNC_INFO << "arg1" << arg1;

    if (!arg1) {
        forgetLastNumbers();
        ui->labelActiveMode->hide();
        setWindowOpacity(1);
    }
    else {
        SetForegroundWindow((HWND)winId());
        ui->labelActiveMode->show();
        removeSelection();
        setWindowOpacity(0.8);

        static bool firstTime = true;
        if (firstTime) {
            ui->labelActiveMode->setText("<html><head/><body><p><span style=\" color:#ff0000;\">Active mode</span></p></body></html>");
            firstTime = false;
        }
    }

    for (TranslucentRoundWindow* weirdWindow: weirdWindows) {
        if (arg1) {
            weirdWindow->show();
        } else {
//            weirdWindow->hide();
            QTimer::singleShot(50, weirdWindow, SLOT(hide()));
            weirdWindow->setShowSelection(false);
        }
    }
}

void MainWindow::makeScreenshot(const QRect &rect) {
    qDebug() << Q_FUNC_INFO << rect;
    QString imageName = QString("dbg/screenshot%1.bmp").arg(curScreenshotIdx);
#ifdef SAVE_SCREENSHOTS
    curScreenshotIdx++;
#endif

    TranslucentRoundWindow* rectangleWindow = static_cast<TranslucentRoundWindow*>(sender());
    rectangleWindow->hide();

    /*QFile file(imageName);
    bool savedOk = true;
    if (!file.open(QIODevice::WriteOnly))
        savedOk = false;
    else
        file.remove();
    if (!savedOk) {
        QMessageBox::critical(this, windowTitle(), "Can't save screenshot.\nProbably you have no rights for writing to the program's directory.");
        return;
    }*/

    QPixmap::grabWindow(qApp->desktop()->winId(), rectangleWindow->frameGeometry().left(), rectangleWindow->frameGeometry().top(),
                                                  rectangleWindow->frameGeometry().right(), rectangleWindow->frameGeometry().bottom()).save(QString(imageName).insert(
                                                  imageName.indexOf('.'), '_').replace("bmp", "png"));
    bool savedOk = QPixmap::grabWindow(qApp->desktop()->winId(), rect.x(), rect.y(), rect.width(), rect.height()).save(imageName);
    if (!savedOk) {
        QMessageBox::critical(this, windowTitle(), "Can't save screenshot.\nProbably you have no rights for writing to the program's directory.");
        return;
    }

    rectangleWindow->show();

    preprocess(imageName.replace("/", "\\"));
}

void MainWindow::preprocess(const QString &fileName) {
    qDebug() << Q_FUNC_INFO;
    static QString shellCommandTemplate("utils\\leptonica_util\\leptonica_util.exe ..\\..\\%1 ..\\..\\ocr_in.tif 2 0.5 1 3.5 1 5 2.5 1 2000 2000 0 0 0.0 0");

    QString shellCommand = shellCommandTemplate.arg(fileName);
    QProcess process(this);
    qDebug() << "QDir::currentPath()" << QDir::currentPath();
    process.setWorkingDirectory(QDir::currentPath() + "/utils/leptonica_util");
    qDebug() << shellCommand;
    process.start(shellCommand);
//    process->start("utils\\leptonica_util\\leptonica_util.exe");
//    process->start("leptonica_util.exe");
//    process->start("cmd g:\\delo_home\\freelance\\jobs\\job9\\workingDir\\leptonica_util.exe");
//    process->start("leptonica_util.exe");
//    process->start("g:\\delo_home\\freelance\\jobs\\job9\\workingDir\\leptonica_util.exe");
    qDebug() << process.waitForStarted();
    qDebug() << process.error();
    qDebug() << process.readAllStandardOutput();
    qDebug() << process.readAllStandardError();
    process.waitForFinished();
    qDebug() << "leptonica_util.exe isExecutable()" << QFileInfo("utils/leptonica_util/leptonica_util.exe").isExecutable();
    QFileInfo fileInfo("ocr_in.tif");
    qDebug() << "out file ocr_in.tif" << fileInfo.lastModified() << fileInfo.size();

    doOcr();
}

void MainWindow::closeEvent(QCloseEvent *) {
    qApp->quit();
}

void MainWindow::doOcr() {
    qDebug() << Q_FUNC_INFO;
//    static QString shellCommand("utils\\tesseract\\tesseract.exe ..\\..\\ocr_in.tif ..\\..\\ocr -l eng -psm 3");
    static QString shellCommand("utils\\tesseract\\tesseract.exe ocr_in.tif ocr -l eng -psm "
                                /*"3"*/"6"
                                /*" digits"*/);
//    static QString shellCommand("tesseract.exe");

    QProcess process(this);
//    process.setWorkingDirectory(QDir::currentPath() + "/utils/tesseract");
    qDebug() << process.workingDirectory();
    qDebug() << shellCommand;
    process.start(shellCommand);
//    process.start("g:\\delo_home\\freelance\\jobs\\job9\\workingDir\\utils\\tesseract\\tesseract.exe");
//    process.start("tesseract.exe");
//    process.start("utils\\tesseract\\tesseract.exe");
//    QStringList args;
//    args << "/C";
//    args << "tesseract.bat";
//    process.start("cmd.exe", args);
    qDebug() << process.waitForStarted();
    qDebug() << process.error();
    qDebug() << process.readAllStandardOutput();
    qDebug() << process.readAllStandardError();
    process.waitForFinished();

    QString line;
    while ((line = process.readLine()) != "") {
        qDebug() << line.remove("\r\n");
    }
//    QThread::sleep(10);
    qDebug() << "finished";

    qDebug() << "tesseract.exe isExecutable()" << QFileInfo("utils/tesseract/tesseract.exe").isExecutable();
    QFileInfo fileInfo("ocr.txt");
    qDebug() << "out file ocr.txt" << fileInfo.lastModified() << fileInfo.size();

    QFile ocrTextF("ocr.txt");
    if (ocrTextF.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream s(&ocrTextF);
        updateNumberList(extractNumbers(s.readAll()));
        ocrTextF.close();
    }
}

QList<double> MainWindow::extractNumbers(const QString &str) {
    // Numbers are continuous digits. Commas can be inserted between them. Dot separates integer part from float part.
    // Any other symbol (including \n) separates numbers.
    // If two dots are in the same number, the second dot becomes number separator.
    // Dot can't be first in the number (maybe it should not be so).
    // If there is 3 digits after dot, it is being treated as comma (because comma and dot are very often being confused, so I use this hack)
    qDebug() << Q_FUNC_INFO << str;

    QList<double> numbers;
    QStringList numbersStr;
    QString number;
    bool hasDot = false;
    bool hasMinus = false;
//    bool metSpace = false;
    for (QChar c: str) {                // from first to last
        if (c.isDigit() || c == ',' || (c=='.' && !hasDot) || (c == '-' && !hasMinus) || (c == '(' && number.isEmpty())) {
            if (c == '.') {
                if (!number.isEmpty()) {
                    hasDot = true;
                    number.append(c);
                }
            } else /*if (c != ',')*/ {
                number.append(c);
                if (c == '-')
                    hasMinus = true;
            }
        } else if (c == ' ' /*&& !metSpace*/ && number.length() && number[0].isDigit() && number.right(1) != ",") {
//            metSpace = true;
        } else {
            if (!number.isEmpty()) {
                if (c == ')')
                    number.append(c);
                if (number != "-") {
                    bool parentheses = *number.begin() == '(' && *(number.end() - 1) == ')';
                    number.remove('(').remove(')');
                    if (parentheses)
                        number.prepend('-');
                    numbersStr << cutNumbers(number)/* * (parentheses ? -1 : 1)*/;
                }
                number.clear();
                hasDot = false;
                hasMinus = false;
//                metSpace = false;
            }
        }
    }
    if (!number.isEmpty() && number != "-")
        numbersStr << cutNumbers(number.remove('('));

    for (QString s: numbersStr)
    {
        // if it looks like thousand separator, make it thousand separator (comma very often confused with dot)
        int dot_index = s.lastIndexOf('.');
        if (dot_index != -1 && dot_index == s.size() - 1 - 3)
            s.remove(dot_index, 1);

        numbers << s.toDouble();
    }

    qDebug() << "numbers" << numbers;
    return numbers;
}

QStringList MainWindow::cutNumbers(QString number) {
    qDebug() << Q_FUNC_INFO << number;
    QStringList res;

    for (int i = number.length() - 1, counter = 0; i > 0; i--, counter++)
        if (number[i] == ' ' || number[i] == ',') {
            if (counter == 3 || counter == 0)
                number.remove(i, 1);
            else {
                res.prepend(number.mid(i));
                number.remove(i, number.length() - i);
            }
            counter = -1;
        } else if (number[i] == '.')
            counter = -1;
    res.prepend(number);
    return res;
}

void MainWindow::updateNumberList(const QList<double> &numbers) {
    if (numbers.isEmpty())
        return;

    lastRowCount = ui->tableWidgetNumbers->rowCount();
    if (lastRowCount == 1 && ui->tableWidgetNumbers->item(0, 1)->text().isEmpty())
        lastRowCount = 0;

//    bool error = false;
    for (double num: numbers) {
        int curRow = ui->tableWidgetNumbers->rowCount() - 1;

        QPushButton* button = new QPushButton(NULL);
//        int side = ui->tableWidgetNumbers->rowHeight(curRow);
        QPixmap pixmap("Resources/delete.png");
        button->setIcon(QIcon(pixmap));
        int side = pixmap.width() + 6;
        button->setFixedSize(side, side);
//        button->setFixedSize(20, 20);
        QWidget* wgt = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(wgt);
        layout->setMargin(0);
        layout->addWidget(button);
        layout->setAlignment(Qt::AlignCenter);
        wgt->setLayout(layout);
        connect(button, SIGNAL(clicked()), this, SLOT(removeButtonClicked()));

        if (ui->tableWidgetNumbers->item(curRow, 1)->text().isEmpty()) {
            ui->tableWidgetNumbers->item(curRow, 1)->setText(floatToStr(num));
            ui->tableWidgetNumbers->setCellWidget(curRow, BUTTON_COLUMN, wgt);
        } else {
            ui->tableWidgetNumbers->insertRow(curRow + 1);
            ui->tableWidgetNumbers->setItem(curRow + 1, 0, new QTableWidgetItem);
            ui->tableWidgetNumbers->item(curRow + 1, OP_COLUMN)->setTextAlignment(Qt::AlignCenter);
            ui->tableWidgetNumbers->setItem(curRow + 1, 1, new QTableWidgetItem);
            ui->tableWidgetNumbers->setItem(curRow + 1, 2, new QTableWidgetItem);
            ui->tableWidgetNumbers->setItem(curRow + 1, 3, new QTableWidgetItem);

            ui->tableWidgetNumbers->item(curRow + 1, 1)->setText(floatToStr(num));
//            if (num == 0 && operation == 3)
//                error = true;
//            QString res = ui->tableWidgetNumbers->item(curRow + 1, 2)->setText(calculate(ui->tableWidgetNumbers->item(curRow, curRow == 0 ? 1 : 2)->text(), num));
//            ui->tableWidgetNumbers->item(curRow + 1, 2)->setText(res);
//            if (res == errorMarker) /*{*/
//                ui->tableWidgetNumbers->item(curRow + 1, 2)->setBackground(QColor(Qt::red));
//            }
//            else
//                ui->tableWidgetNumbers->item(curRow + 1, 2)->setText(calculate(ui->tableWidgetNumbers->item(curRow, curRow == 0 ? 1 : 2)->text(), num));
            ui->tableWidgetNumbers->item(curRow + 1, 0)->setText(operationsStr[operation]);
            ui->tableWidgetNumbers->setCellWidget(curRow + 1, BUTTON_COLUMN, wgt);
        }
    }
    if (lastRowCount > 0 || numbers.count() > 1)
        updateFromRow(lastRowCount == 0 ? 1 : lastRowCount);

    int idx = ui->tableWidgetNumbers->rowCount() - 1;
    ui->tableWidgetNumbers->setCurrentCell(idx, OP_COLUMN);  // autoscroll
    ui->tableWidgetNumbers->item(idx, OP_COLUMN)->setSelected(false);
}

QString MainWindow::calculate(const QString &arg1, double arg2, const QString& op) {
    if (arg1 == errorMarker)
        return arg1;                        // worry -- returning reference of temporary

    double arg1F = strToFloat(arg1);
    double result = 0;

    int opIndex = 0;
    if (!op.isEmpty()) {
        for ( ; opIndex < 4 && operationsStr[opIndex] != op; opIndex++) {
        }

        if (opIndex == 4)
            return errorMarker;
    }

    switch (op.isEmpty() ? operation : opIndex) {
    case 0:
        result = arg1F + arg2;
        break;
    case 1:
        result = arg1F - arg2;
        break;
    case 2:
        result = arg1F * arg2;
        break;
    case 3:
        if (arg2 == 0)
            return errorMarker;
        result = arg1F / arg2;
    }
    return floatToStr(result);
}

QString MainWindow::floatToStr(double num) {
    QString res;

    if (num == (int)num)
        res = QString::number((int)num);
    else
        res = QString::number(num);

    int dotIdx = res.indexOf('.');
    for (int i = dotIdx == -1 ? res.length() - 1 : dotIdx - 1, counter = 0; i >= (num < 0 ? 2 : 1); i--) {
        counter++;
        if (counter % 3 == 0)
            res.insert(i, ',');
    }

    return res;
}

double MainWindow::strToFloat(QString num) {
    return num.remove(',').toDouble();
}

void MainWindow::updateOpColors() {
    static const QColor* opColors[4] = {&additionColor, &subtractionColor, &divisionColor, &multiplicationColor};

    for (TranslucentRoundWindow* w: weirdWindows)
        w->updateOpColor(*(opColors[operation]));

    for (int i = 0; i < pushButtonsOperation.count(); i++)
        pushButtonsOperation[i]->setChecked(i == operation);
}

void MainWindow::removeRow(int row) {
    int rowCount = ui->tableWidgetNumbers->rowCount();
    if (row >= rowCount)
        return;

    if (rowCount == 1) {
        ui->tableWidgetNumbers->item(0, NUMBER_COLUMN)->setText("");
        ui->tableWidgetNumbers->removeCellWidget(0, BUTTON_COLUMN);
    }
    else {
        ui->tableWidgetNumbers->removeRow(row);
        rowCount--;
        if (row == 0) {
            ui->tableWidgetNumbers->item(row, OP_COLUMN)->setText("");
            ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->setText("");
        }
        if (rowCount > 1)
            updateFromRow(row == 0 ? 1 : row);

        for (QTableWidgetItem* item: ui->tableWidgetNumbers->selectedItems())       // removing gray square after row had been removed
            item->setSelected(false);
    }
}

int MainWindow::getScreenshotIndex() {
#ifdef SAVE_SCREENSHOTS
    QDir dir("dbg");
    int index = -1;

    for (const QString& s: dir.entryList()) {
        int fileIdx = -1;
        sscanf(qPrintable(s), "screenshot%d.bmp", &fileIdx);
//        qDebug() << "fileIdx" << fileIdx;

        if (fileIdx > index)
            index = fileIdx;
    }

    return index + 1;
#else
    return 0;
#endif
}

void MainWindow::createDbgArchive() {
#ifdef SAVE_SCREENSHOTS
    qDebug() << Q_FUNC_INFO;
    static QString shellCommand("utils\\7zip\\7z.exe a dbg.7z -up1q0r2x1y2z1w2 -mx9 -m0=LZMA2 dbg");

    QProcess process(this);
    qDebug() << process.workingDirectory();
    qDebug() << shellCommand;
    process.start(shellCommand);
    qDebug() << process.waitForStarted();
    qDebug() << process.error();
    qDebug() << process.readAllStandardOutput();
    qDebug() << process.readAllStandardError();
    process.waitForFinished();

    QString line;
    while ((line = process.readLine()) != "") {
        qDebug() << line.remove("\r\n");
    }
    qDebug() << "finished";
#endif
}

void MainWindow::myAddActions(QWidget *widget) {
    widget->addAction(ui->actionAddition);
    widget->addAction(ui->actionSubtraction);
    widget->addAction(ui->actionDivision);
    widget->addAction(ui->actionMultiplication);
    widget->addAction(ui->actionCopy_result);
    widget->addAction(ui->actionClear);
}

void MainWindow::updateFromRow(int row) {
    if (row <= 0)
        return;

    for (int i = row; i < ui->tableWidgetNumbers->rowCount(); i++)
        calculateRow(i);
}

void MainWindow::calculateRow(int row) {
    if (row <= 0 || row >= ui->tableWidgetNumbers->rowCount())
        return;

    QString res = calculate(ui->tableWidgetNumbers->item(row - 1, row - 1 == 0 ? NUMBER_COLUMN : RESULT_COLUMN)->text(),
                            ui->tableWidgetNumbers->item(row, NUMBER_COLUMN)->text().remove(',').toDouble(),
                            ui->tableWidgetNumbers->item(row, OP_COLUMN)->text());
    ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->setText(res);

    if (res == errorMarker)
        ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->setBackground(QColor(Qt::red));
    else if (ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->background().color() == QColor(Qt::red))
        ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->setBackground(ui->tableWidgetNumbers->item(row, NUMBER_COLUMN)->background());
}

void MainWindow::hideRectangle() {
    if (!activeMode)
        return;

    for (TranslucentRoundWindow* w: weirdWindows)
        w->setShowSelection(false);
}

void MainWindow::removeSelection() {
    for (QTableWidgetItem* item: ui->tableWidgetNumbers->selectedItems())
        item->setSelected(false);
}

void MainWindow::moveSelWindowsOnTop() {
    for (TranslucentRoundWindow* w: weirdWindows)
        w->raise();
}

void MainWindow::resizeColumns() {
    qDebug() << Q_FUNC_INFO;
    static int widthPercents[4] = {10, 40, 40, 10};

    int allWidth = ui->tableWidgetNumbers->viewport()->width()/* - ui->tableWidgetNumbers->verticalScrollBar()->width()*/;
    qDebug() << "allWidth" << allWidth /*<< "verticalScrollBar" << ui->tableWidgetNumbers->verticalScrollBar()->width()*/;

    ui->tableWidgetNumbers->setColumnWidth(0, widthPercents[0] * 0.01 * allWidth);
    ui->tableWidgetNumbers->setColumnWidth(1, widthPercents[1] * 0.01 * allWidth);
    ui->tableWidgetNumbers->setColumnWidth(2, widthPercents[2] * 0.01 * allWidth);
    ui->tableWidgetNumbers->setColumnWidth(3, widthPercents[3] * 0.01 * allWidth);

    move(0, y());

//    QMessageBox::information(NULL, "eCalc", "Hello, world!\n\nTo activate the program press F11, to deactivate, press F11 again");
}

void TranslucentRoundWindow::mouseReleaseEvent(QMouseEvent *event) {
    qDebug() << Q_FUNC_INFO << "pos" << event->pos();

    if (event->button() == Qt::LeftButton)
        if (!dragging) {
            firstPoint = event->pos();
            dragging = true;
            setMouseTracking(true);
            setShowSelection(true);
            emit forgetLastNumbers();

            mouseMoveEvent(event);
        } else {
            setMouseTracking(false);

            qDebug() << event->pos() << previewPos;
            if (event->pos() != previewPos)
                capture();

            setDragging(false);
        }
   else if (event->button() == Qt::RightButton) {
        setShowSelection(false);
        if (!dragging || lastCaptureWasPreview)
            emit removeLastNumbers();
   }
}

void TranslucentRoundWindow::mouseMoveEvent(QMouseEvent *event) {
//    qDebug() << Q_FUNC_INFO << "coords" << event->pos();

    if (dragging) {
        int x1, y1, x2, y2;
        if (firstPoint.x() < event->pos().x()) {
            x1 = firstPoint.x();
            x2 = event->pos().x();
        } else {
            x1 = event->pos().x();
            x2 = firstPoint.x();
        }
        if (firstPoint.y() < event->pos().y()) {
            y1 = firstPoint.y();
            y2 = event->pos().y();
        } else {
            y1 = event->pos().y();
            y2 = firstPoint.y();
        }

        highlight.setCoords(x1, y1, x2, y2);
        update();

        /*previewTimer.start();*/
    }
}

TranslucentRoundWindow::TranslucentRoundWindow(QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint),
    dragging(false),
    showSelection(false),
    lastCaptureWasPreview(false),
    previewPos(-1, -1)
{
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setWindowFlags(windowFlags() | Qt::SplashScreen);

    static int id = 0;
    instanceId = id++;

    previewTimer.setSingleShot(true);
    previewTimer.setInterval(PREVIEW_DELAY);
    connect(&previewTimer, SIGNAL(timeout()), this, SLOT(previewCaptureNumbers()));
}

QSize TranslucentRoundWindow::sizeHint() const
{
    return QSize(300, 300);
}

void TranslucentRoundWindow::setDragging(bool on) {
    dragging = on;
    if (on == false) {
        lastCaptureWasPreview = false;
        previewTimer.stop();
        previewPos = QPoint(-1, -1);
    }
}

void TranslucentRoundWindow::setShowSelection(bool on) {
    qDebug() << Q_FUNC_INFO << on << instanceId << geometry();
    if (!on) {
        setDragging(false);
        showSelection = false;
        update();
    } else {
        showSelection = true;
    }
}

void TranslucentRoundWindow::updateOpColor(const QColor &color) {
    opColor = color;
    update();
}

void TranslucentRoundWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(0, 0, width(), height(), Qt::transparent);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 1));
    painter.drawRect(0, 0, width(), height());

    if (showSelection && !highlight.isNull()) {
        painter.setBrush(opColor);
        painter.setPen(QColor(Qt::red));
        painter.drawRect(highlight);
    }
}

void TranslucentRoundWindow::drawRectangle(int x, int y, int w, int h) {
    qDebug() << Q_FUNC_INFO << "x, y, w, h" << x << y << w << h;

//    QPainter *painter = paintEngine();
//    painter->eraseRect(0, 0, width(), height());
//    painter->setRenderHint(QPainter::Antialiasing);
//    painter->setPen(Qt::NoPen);
//    painter->setBrush(QColor(255, 0, 0, dragRectangleTranslucency));
//    painter->drawRect(0, 0, width(), height());

//    painter->setBrush(QColor(0, 255, 0, dragRectangleTranslucency));
//    painter->drawRect(x, y, w, h);
}

void TranslucentRoundWindow::capture() {
    if (lastCaptureWasPreview)
        emit removeLastNumbers();
    qDebug() << "highlight" << highlight << "geometry()" << geometry();
    emit makeScreenshot(QRect(highlight.topLeft() + geometry().topLeft(), highlight.bottomRight() + geometry().topLeft()));
}

void TranslucentRoundWindow::previewCaptureNumbers() {
//    if (lastCaptureWasPreview)
//        emit removeLastNumbers();
//    qDebug() << "highlight" << highlight << "geometry()" << geometry();
//    emit makeScreenshot(QRect(highlight.topLeft() + geometry().topLeft(), highlight.bottomRight() + geometry().topLeft()));
    capture();

    lastCaptureWasPreview = true;
    previewPos = cursor().pos();
}

void MainWindow::on_pushButtonPlus_clicked()
{
    qDebug() << "checked =" << ui->pushButtonPlus->isChecked();
    operation = 0;
    updateOpColors();
}

void MainWindow::on_pushButtonMinus_clicked()
{
    operation = 1;
    updateOpColors();
    pushButtonsOperation[operation]->setChecked(true);
}

void MainWindow::on_pushButtonMult_clicked()
{
    operation = 3;
    updateOpColors();
    pushButtonsOperation[operation]->setChecked(true);
}

void MainWindow::on_pushButtonDiv_clicked()
{
    operation = 2;
    updateOpColors();
    pushButtonsOperation[operation]->setChecked(true);
}

void MainWindow::on_actionExit_selection_mode_triggered()
{
    qDebug() << Q_FUNC_INFO;

    for (TranslucentRoundWindow* w: weirdWindows)
        w->setDragging(false);
}

void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}

void MainWindow::removeLastNumbers() {
    qDebug() << Q_FUNC_INFO << lastRowCount;

    if (lastRowCount == -1)
        return;

    for (int r = ui->tableWidgetNumbers->rowCount() - 1; r >= lastRowCount; r--) {
        if (r == 0) {
            ui->tableWidgetNumbers->item(r, 1)->setText("");
            ui->tableWidgetNumbers->removeCellWidget(r, BUTTON_COLUMN);
        } else {
            delete ui->tableWidgetNumbers->item(r, 0);
            delete ui->tableWidgetNumbers->item(r, 1);
            delete ui->tableWidgetNumbers->item(r, 2);

            ui->tableWidgetNumbers->removeRow(r);
        }
    }
}

void MainWindow::forgetLastNumbers() {
    qDebug() << Q_FUNC_INFO;
    lastRowCount = -1;
}

void MainWindow::on_actionAddition_triggered()
{
    on_pushButtonPlus_clicked();
}

void MainWindow::on_actionSubtraction_triggered()
{
    on_pushButtonMinus_clicked();
}

void MainWindow::on_actionDivision_triggered()
{
    on_pushButtonDiv_clicked();
}

void MainWindow::on_actionMultiplication_triggered()
{
    on_pushButtonMult_clicked();
}

void MainWindow::modeToggled() {
    on_actionActive_mode_toggled(!activeMode);
    activeMode = !activeMode;
}

//void myMessageOutput(QtMsgType type, const QMessageLogContext &/*context*/, const QString &msg)
//{
//    Q_UNUSED(type);
//    OutputDebugString(reinterpret_cast<const wchar_t *>((msg + "\n").utf16()));

//    if (dbgFile) {
//        QByteArray localMsg = msg.toLocal8Bit();
//        fprintf(dbgFile, "%s\n", localMsg.constData());
//        fflush(dbgFile);
//    }
//}

void MainWindow::on_actionCopy_result_triggered()
{
    int row = ui->tableWidgetNumbers->rowCount() - 1;

    if (!ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->text().isEmpty())
        QApplication::clipboard()->setText(ui->tableWidgetNumbers->item(row, RESULT_COLUMN)->text());
}

void MainWindow::on_pushButton_clicked()
{
    on_actionCopy_result_triggered();
}

void MainWindow::removeButtonClicked() {
    for (int i = 0; i < ui->tableWidgetNumbers->rowCount(); i++)
        if (ui->tableWidgetNumbers->cellWidget(i, BUTTON_COLUMN)->children().contains(sender())) {
            int pos = ui->tableWidgetNumbers->verticalScrollBar()->sliderPosition();
            removeRow(i);

            // avoiding crazy effect, when buttons are moved in between cells
            ui->tableWidgetNumbers->setCurrentCell(0, BUTTON_COLUMN);
            ui->tableWidgetNumbers->item(0, BUTTON_COLUMN)->setSelected(false);

            ui->tableWidgetNumbers->verticalScrollBar()->setSliderPosition(pos);
//            if (i != 0) {
//                ui->tableWidgetNumbers->setCurrentCell(0, BUTTON_COLUMN);
//                ui->tableWidgetNumbers->setCurrentCell(i, BUTTON_COLUMN);
//                ui->tableWidgetNumbers->item(i, BUTTON_COLUMN)->setSelected(false);
//            }
            return;
        }
}

void MainWindow::on_actionClear_triggered()
{
    int rc = ui->tableWidgetNumbers->rowCount();
    for (int i = rc - 1; i >= 0; i--)
        removeRow(i);

    hideRectangle();
}

void MainWindow::on_pushButtonClear_clicked()
{
    on_actionClear_triggered();
}

void MainWindow::on_tableWidgetNumbers_cellChanged(int row, int column)
{
    if (editedCell == QPoint(-1, -1))
        return;

    qDebug() << Q_FUNC_INFO << editedCell.y() << editedCell.x();

    row = editedCell.y();
    column = editedCell.x();
    editedCell.setX(-1);
    editedCell.setY(-1);

    if (column == NUMBER_COLUMN || column == RESULT_COLUMN)
        ui->tableWidgetNumbers->item(row, column)->setText(floatToStr(strToFloat(ui->tableWidgetNumbers->item(row, column)->text())));    // Moving "," if needed

    if (column == NUMBER_COLUMN || (column == RESULT_COLUMN && row != 0) || (column == OP_COLUMN && row != 0)) {
        if ((column == NUMBER_COLUMN && row > 0) || (column == OP_COLUMN))
            updateFromRow(row);
        else
            updateFromRow(row + 1);
    }
}

void MainWindow::on_tableWidgetNumbers_cellActivated(int row, int column)
{
    qDebug() << Q_FUNC_INFO << row << column;

    on_tableWidgetNumbers_cellDoubleClicked(row, column);
    ui->tableWidgetNumbers->editItem(ui->tableWidgetNumbers->item(row, column));
}

void MainWindow::on_tableWidgetNumbers_cellDoubleClicked(int row, int column)
{
    qDebug() << Q_FUNC_INFO << row << column;

    editedCell.setX(column);
    editedCell.setY(row);
}
