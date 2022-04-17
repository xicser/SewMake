#include "mainwindow.h"
#include "utils/string/stringcheck.h"
#include "config/configparam.h"

MainWindow::MainWindow() : QMainWindow(nullptr)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    dataInit();
    languageInit();
    mainWindowInit();
    menuBarInit();
    toolBarInit();
    leftPanelInit();
    leftPanelLayerGraphicScrollInit();
    leftPanelMoveLayerGraphicFrameInit();
    leftPanelMoveDistanceInit();
    leftPanelSettingInit();
    leftPanelTransformInit();
    leftPanelSewingModeInit();
    canvasInit();
    statusBarInit();
    bottomPanelInit();
    miscWidgetInit();
    qssInit();
    slotsInit();
    languageChosenQss(languageChosen);
    restorePathFromDisk();
    restoreMainWindowParam();
    bUseHttpHtml = checkNewVersion();
    if (!bUseHttpHtml){
        getHttpsVersionFileUrl();
    }
}

MainWindow::~MainWindow()
{
    if (designWindow != nullptr) { //designWindow没有父对象, 故需要在这里手动释放
        delete designWindow;
    }

    if (changeLayerWindow != nullptr) {
        delete changeLayerWindow;
    }
    if (shapeCloneWindow != nullptr) {
        delete shapeCloneWindow;
    }
    if (editShapeWindow != nullptr) {
        delete editShapeWindow;
    }
    if (massOperatingWindow != nullptr) {
        massOperatingWindow->deleteWindow();
        delete massOperatingWindow;
    }
    if (layerParameterSetWindow != nullptr) {
        delete layerParameterSetWindow;
    }
    if (sewingSimulationWindow != nullptr) {
        delete sewingSimulationWindow;
    }
    if (specialSewingWindow != nullptr) {
        delete specialSewingWindow;
    }
    if (sewingPointCodeWindow != nullptr) {
        delete sewingPointCodeWindow;
    }
    if (setBasePointWindow != nullptr) {
        delete setBasePointWindow;
    }
    if (translator != nullptr) {
        delete translator;
    }
    if(helpWindow != nullptr){
        delete helpWindow;
    }
    if(warningChinese != nullptr){
        delete warningChinese;
    }
    if(warningEnglish != nullptr){
        delete warningEnglish;
    }
    if(warningItalian != nullptr){
        delete warningItalian;
    }
    if(warningJapanese != nullptr){
        delete warningJapanese;
    }
    if(warningRussian != nullptr){
        delete warningRussian;
    }
    if(warningSpanish != nullptr){
        delete warningSpanish;
    }
    if(warningTurkish != nullptr){
        delete warningTurkish;
    }
    if(warningVietnam != nullptr){
        delete warningVietnam;
    }
}

/* 数据初始化 */
void MainWindow::dataInit(void)
{
    openedfileType = "null";
    openedfilePath = "null";
    isShowSerialNumber = false;
    showAllLayer = true;
    upgradeStatus = UPGRADE_STATUS_NONE;

    designWindow = nullptr;
    changeLayerWindow = nullptr;
    shapeCloneWindow = nullptr;
    editShapeWindow = nullptr;
    massOperatingWindow = nullptr;
    layerParameterSetWindow = nullptr;
    sewingSimulationWindow = nullptr;
    specialSewingWindow = nullptr;
    sewingPointCodeWindow = nullptr;
    setBasePointWindow = nullptr;
    translator = nullptr;
    changeShapeSizeWindow = nullptr;
    spacingOfClosedShapeWindow = nullptr;
    helpWindow = nullptr;
    warningChinese = nullptr;
    warningEnglish = nullptr;
    warningItalian = nullptr;
    warningJapanese = nullptr;
    warningRussian = nullptr;
    warningSpanish = nullptr;
    warningTurkish = nullptr;
    warningVietnam = nullptr;
}

/* 初始化主窗口 */
void MainWindow::mainWindowInit(void)
{
    //获取除去任务栏的窗口大小
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect clientRect = desktopWidget->availableGeometry();
    screenXSize = clientRect.width() * 0.75;
    screenYSize = clientRect.height()  * 0.8;

    setWindowIcon(QIcon(":/icon/mainwindow/resource/mainwindow/01-logo.png"));
    this->resize(screenXSize, screenYSize);     //设置窗体大小
    setMinimumSize(screenXSize * 0.85, screenYSize * 0.85);       //设置窗口最小大小
    setWindowTitle(tr("SewMake数控编制软件"));

    font.setPixelSize((float)15 / screenXSize * this->geometry().width());
    this->fontSize = this->font.pixelSize();
    this->setFont(this->font);
}

/* 改变控件大小 */
void MainWindow::changeControllerSize(QSize size)
{
    //界面大小
    screenXSize = size.width();
    screenYSize = size.height();
    resize(screenXSize, screenYSize);

    //菜单
    menuBar->setGeometry(0, 0, screenXSize, menuBarHeight);

    //工具栏
    moveToolBarPosition();

    //左侧操作panel
    moveLeftPanelPosition();
    //左侧panel图形图层
    moveLeftPanelLayerGraphicScroll();
    //左侧panel移动图层图形Frame
    moveLeftPanelMoveLayerGraphicFrame();
    //左侧panel移动距离
    moveLeftPanelMoveDistance();
    //左侧panel设置
    moveLeftPanelSetting();
    //左侧panel变换
    moveLeftPanelTransform();
    //左侧panel缝纫模式
    moveLeftPanelSewingMode();

    //画板
    int x = leftPanelWidth;                               //位置
    int y = menuBarHeight + toolBarHeight;
    canvasViewWidth = screenXSize - leftPanelWidth;       //画板view宽度
    canvasViewHeight = screenYSize * 0.83;    //画板view高度
    canvasViewPattern->setGeometry(x, y, canvasViewWidth, canvasViewHeight); //设置位置

    //状态栏
    statusBarWidth = screenXSize - leftPanelWidth;
    statusBarHeight = toolBarHeight;        //状态栏高度等于工具栏高度
    x = leftPanelWidth;
    y = menuBarHeight + toolBarHeight + screenYSize * 0.83;
    lineEditStatusBar->setGeometry(x, y, statusBarWidth, statusBarHeight);

    //底部操作panel
    moveBottomPanel();

    //剩余杂项控件
    moveMiscWidget();
}

/* 变换事件 */
void MainWindow::changeEvent(QEvent* event)
{
    //窗口状态改变，改变窗口size
    if(event->type() == QEvent::WindowStateChange)
    {
        if(this->windowState() != Qt::WindowMinimized)
        {
            changeControllerSize(this->size());
        }
    }
}

/*重画大小事件*/
void MainWindow::resizeEvent(QResizeEvent *)
{
    changeControllerSize(this->size());
}

/* 关闭事件 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (openedfileType != "null") {
        QMessageBox::StandardButton ret = QMessageBox::information(this, tr("SewMake数控编制软件"), tr("是否保存到sdf/dxf文件?"),
                                 QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

        if(ret == QMessageBox::Yes) {

            QString selectedNameFilter;
            QString saveFilePath = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                         this->lastSaveAsFilePath, tr("*.sdf;;*.dxf"), &selectedNameFilter);
            if (saveFilePath.isEmpty() == true) {
                event->ignore();
                return;
            }

            QFileInfo fileinfo = QFileInfo(saveFilePath);   //获取后缀名
            this->lastSaveAsFilePath = fileinfo.path();     //保存本次打开的文件路径
            QString suffix = fileinfo.suffix().toLower();
            if (suffix.isEmpty() == true) {
                if (selectedNameFilter == "*.sdf") {
                    saveFilePath.append(".sdf");
                } else {
                    saveFilePath.append(".dxf");
                }
            }
            else {
                if (selectedNameFilter == "*.sdf" && suffix != "sdf") {
                    saveFilePath.append(".sdf");
                }
                else if (selectedNameFilter == "*.dxf" && suffix != "dxf") {
                    saveFilePath.append(".dxf");
                }
            }

            if (selectedNameFilter == "*.sdf") {
                if (canvasViewPattern->saveFileToDisk("sdf", saveFilePath) != 0) {
                    QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("sdf文件保存失败！"));
                }
            }
            else {
                if (canvasViewPattern->saveFileToDisk("dxf", saveFilePath) != 0) {
                    QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("dxf文件保存失败！"));
                }
            }
            //暂时屏蔽通知scene选中情况有变化, 因为如果场景中有选中的的图元, 会触发进入slotSceneSelectionChanged, 导致解引用野指针
            disconnect(canvasViewPattern->scene(), &QGraphicsScene::selectionChanged,
                       canvasViewPattern, &CanvasViewPattern::slotSceneSelectionChanged);

            //通知更新程序, Mpp软件已经关闭
            if (upgradeStatus == UPGRADE_STATUS_CLOSE) {
                //QString cmdStr = QString("main closed");
                //processUpgrade->write(cmdStr.toLatin1()); //通信一直不成功, 艹, 弃疗了
                QFile file("./main.closed"); //创建一个文件算了
                file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                file.close();
            }

            event->accept();

            //保存文件路径到磁盘
            this->savePathToDisk();

            //保存主界面参数
            this->saveMainWindowParam();
        }
        else if (ret == QMessageBox::No) {
            //暂时屏蔽通知scene选中情况有变化, 因为如果场景中有选中的的图元, 会触发进入slotSceneSelectionChanged, 导致解引用野指针
            disconnect(canvasViewPattern->scene(), &QGraphicsScene::selectionChanged,
                       canvasViewPattern, &CanvasViewPattern::slotSceneSelectionChanged);

            //通知更新程序, Mpp软件已经关闭
            if (upgradeStatus == UPGRADE_STATUS_CLOSE) {
                //QString cmdStr = QString("main closed");
                //processUpgrade->write(cmdStr.toLatin1()); //通信一直不成功, 艹, 弃疗了
                QFile file("./main.closed"); //创建一个文件算了
                file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                file.close();
            }

            event->accept();

            //保存文件路径到磁盘
            this->savePathToDisk();

            //保存主界面参数
            this->saveMainWindowParam();
        }
        //Cancel
        else {
            //通知更新程序, Mpp软件已经关闭
            if (upgradeStatus == UPGRADE_STATUS_CLOSE) {
                //QString cmdStr = QString("main closed");
                //processUpgrade->write(cmdStr.toLatin1()); //通信一直不成功, 艹, 弃疗了
                QFile file("./main.closed"); //创建一个文件算了
                file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                file.close();
            }

            event->ignore();
        }

        return;
    }

    //通知更新程序, Mpp软件已经关闭
    if (upgradeStatus == UPGRADE_STATUS_CLOSE) {
        //QString cmdStr = QString("main closed");
        //processUpgrade->write(cmdStr.toLatin1()); //通信一直不成功, 艹, 弃疗了
        QFile file("./main.closed"); //创建一个文件算了
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        file.close();
    }

    //保存主界面参数
    this->saveMainWindowParam();
}

/* 菜单栏初始化 */
void MainWindow::menuBarInit(void)
{
    menuBarHeight = fontSize * TITLE_BAR_HEIGHT_DIV_DEF_FONT_SIZE;;        //菜单栏高度

    menuBar = new QMenuBar(this);
    menuBar->setGeometry(0, 0, screenXSize, menuBarHeight);
    menuBar->setFont(this->font);

    menuLanguage = new QMenu(menuBar);                                      //语言菜单
    menuLanguage->setFont(this->font);

    menuEdit = new QMenu(menuBar);                                          //编辑菜单
    menuOpt = new QMenu(menuBar);                                           //选项菜单

    QString str = tr("语言");
    menuLanguage->setTitle(str+"▼");
    menuBar->addMenu(menuLanguage);

    //语言标签
    labelChinese = new QLabel("中文");
    labelChinese->setFixedSize(90, 25);
    labelEnglish = new QLabel("English");
    labelEnglish->setFixedSize(90, 25);
    labelVietnam = new QLabel("viet nam");
    labelVietnam->setFixedSize(90, 25);
    labelItalian = new QLabel("Ltaliana");
    labelItalian->setFixedSize(90, 25);
    labelTurkish = new QLabel("Espanol");
    labelTurkish->setFixedSize(90, 25);
    labelSpanish = new QLabel("Turk dili");
    labelSpanish->setFixedSize(90, 25);
    labelRussian = new QLabel("pycknn");
    labelRussian->setFixedSize(90, 25);
    labelJapanese = new QLabel("日本語");
    labelJapanese->setFixedSize(90, 25);

    actionChinese = new QWidgetAction(menuLanguage);
    actionEnglish = new QWidgetAction(menuLanguage);
    actionVietnam = new QWidgetAction(menuLanguage);
    actionItalian = new QWidgetAction(menuLanguage);
    actionTurkish = new QWidgetAction(menuLanguage);
    actionSpanish = new QWidgetAction(menuLanguage);
    actionRussian = new QWidgetAction(menuLanguage);
    actionJapanese = new QWidgetAction(menuLanguage);

    //action自定义设置为标签样式
    actionChinese->setDefaultWidget(labelChinese);
    actionEnglish->setDefaultWidget(labelEnglish);
    actionVietnam->setDefaultWidget(labelVietnam);
    actionItalian->setDefaultWidget(labelItalian);
    actionTurkish->setDefaultWidget(labelTurkish);
    actionSpanish->setDefaultWidget(labelSpanish);
    actionRussian->setDefaultWidget(labelRussian);
    actionJapanese->setDefaultWidget(labelJapanese);

    menuLanguage->addAction(actionChinese);
    menuLanguage->addAction(actionEnglish);
    menuLanguage->addAction(actionVietnam);
    menuLanguage->addAction(actionItalian);
    menuLanguage->addAction(actionTurkish);
    menuLanguage->addAction(actionSpanish);
    menuLanguage->addAction(actionRussian);
    menuLanguage->addAction(actionJapanese);

    menuEdit = menuBar->addMenu(tr("编辑(&Y)"));
    actionChangeSize = menuEdit->addAction(tr("改变大小(&Z)"));

    menuOpt = menuBar->addMenu(tr("选项(&Z)"));
    actionDeleteMultiGraphic = menuOpt->addAction(tr("打开文件时是否删除重合图形"));
    actionDeleteMultiGraphic->setCheckable(true);
    actionDelMultiGraphicsBetweenLayer = menuOpt->addAction(tr("打开文件时是否删除跨图层重复图形"));
    actionDelMultiGraphicsBetweenLayer->setCheckable(true);
    actionAutoMergeNearGraphic = menuOpt->addAction(tr("打开文件时, 自动合并临近图形(&Y)"));
    actionAutoMergeNearGraphic->setCheckable(true);
    actionOpenNtp = menuOpt->addAction(tr("打开NTP文件时, 图形绝对坐标不变（用于免基准）(&N)"));
    actionOpenNtp->setCheckable(true);
    actionCloseGraphic = menuOpt->addAction(tr("封闭图形首尾间距(&Z)"));

    actionPaint = menuBar->addAction(tr("作图(&D)"));
    actionTools = menuBar->addAction(tr("工具"));

    menuLanguage->setStyleSheet(MENU_BAR_LANGUAGE);
    menuEdit->setStyleSheet(MENU_BAR_LANGUAGE);
    menuOpt->setStyleSheet(MENU_BAR_LANGUAGE);

    menuEdit->setFont(this->font);
    menuOpt->setFont(this->font);
}

/* 工具栏初始化 */
void MainWindow::toolBarInit(void)
{
    toolBar = new QToolBar(this);
    toolBar->setMovable(false);

    //按钮
    toolBtnOpenFile = new QToolButton();                      //打开文件
    toolBtnSaveAs = new QToolButton();                        //另存为按钮
    toolBtnUndo = new QToolButton();                          //后退
    toolBtnRedo = new QToolButton();                          //前进
    toolBtnZoomUp = new QToolButton();                        //放大
    toolBtnZoomDown = new QToolButton();                      //缩小
    toolBtnDispRestore = new QToolButton();                   //显示恢复
    toolBtnPicDrag = new QToolButton();                       //图片拖动
    toolBtnNormalOpt = new QToolButton();                     //正常操作
    toolBtnHelp = new QToolButton();                          //帮助
    toolBtnUpdate = new QToolButton();                        //软件升级

    QToolButton *toolBtns[11] = {
        toolBtnOpenFile,
        toolBtnSaveAs,
        toolBtnUndo,
        toolBtnRedo,
        toolBtnZoomUp,
        toolBtnZoomDown,
        toolBtnDispRestore,
        toolBtnPicDrag,
        toolBtnNormalOpt,
        toolBtnHelp,
        toolBtnUpdate
    };
    QString strs[11] = {tr("打开(O)"), tr("另存为(S)"), tr("后退"), tr("前进"), tr("放大"), tr("缩小"), tr("显示恢复"), tr("图形拖动"), tr("正常操作"), tr("帮助"), tr("软件升级")};
    QString pics[11] = {TOOL_BAR_OPEN_FILE_PIC, TOOL_BAR_SAVE_AS_PIC,
                            TOOL_BAR_UNDO_PIC, TOOL_BAR_REDO_PIC,
                            TOOL_BAR_ZOOM_UP_PIC, TOOL_BAR_ZOOM_DOWN_PIC,
                            TOOL_BAR_DISP_RESTORE_PIC, TOOL_BAR_PIC_DRAG_PIC,
                            TOOL_BAR_NORMAL_OPT_PIC, TOOL_BAR_ABOUT_PIC, TOOL_BAR_UPDATE_PIC};

    for (int i = 0; i < 11; i++) {
        toolBtns[i]->setText(strs[i]);
        toolBtns[i]->setIcon(QIcon(pics[i]));
        toolBtns[i]->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolBar->addWidget(toolBtns[i]);
    }
}

/* 移动工具栏位置 */
void MainWindow::moveToolBarPosition(void)
{
    toolBarHeight = screenYSize * TOOL_BAR_HEIGHT_DIV_SCREEN_HEIGHT;            //工具栏高度

    toolBar->setGeometry(0, menuBarHeight, screenXSize, toolBarHeight);
}

/* 左侧操作panel初始化 */
void MainWindow::leftPanelInit(void)
{
    scrollBarWidth = qApp->style()->pixelMetric(QStyle :: PM_ScrollBarExtent); //滚动条宽度

    //放置panel
    frameLeftPanel = new QFrame(this);
}

/* 移动左侧panel */
void MainWindow::moveLeftPanelPosition(void)
{
    leftPanelWidth = screenXSize * PANEL_LEFT_WIDTH_DIV_SCREEN_WIDTH;    //panel宽度
    leftPanelHeight = screenYSize * PANEL_LEFT_HIGHT_DIV_SCREEN_HEIGHT;  //panel高度
    leftPanelModuleClearance = leftPanelHeight * PANEL_LEFT_MODULE_CLEARANCE_DIV_PANEL_LEFT_HEIGHT + 7; //各个模块间的间距
    leftPanelGBoxModuleWidth = leftPanelWidth * PANEL_LEFT_GBOX_MODULE_WIDTH_DIV_PANEL_LEFT_WIDTH;  //GroupBox宽度

    frameLeftPanel->setGeometry(0, menuBarHeight + toolBarHeight,
                            leftPanelWidth, leftPanelHeight);
}

/* 左侧panel图形图层初始化 */
void MainWindow::leftPanelLayerGraphicScrollInit(void)
{
    QStringList header;
    header.clear();

    //先设置图层
    tabWidgetLayer = new TableWidgetLayer(0, 2, frameLeftPanel);          //图层(相对于QFrame构造的leftPanel)
    tabWidgetLayer->setMainWindow(this);                                  //设置mainWindow
    tabWidgetLayer->verticalHeader()->setVisible(false);                  //去掉每行的行号
    tabWidgetLayer->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);    //在需要时显示滚动条
    header<< tr("图层") << tr("输出");
    tabWidgetLayer->setHorizontalHeaderLabels(header);
    tabWidgetLayer->horizontalHeader()->setHighlightSections(false);
    tabWidgetLayer->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); //可交互
    tabWidgetLayer->setRowHeight(fontSize);                               //行高
    tabWidgetLayer->setEditTriggers(QAbstractItemView::NoEditTriggers);   //不允许修改
    tabWidgetLayer->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    tabWidgetLayer->setSelectionMode(QAbstractItemView::SingleSelection); //只能选中一个
    tabWidgetLayer->setFont(font);
    tabWidgetLayer->setStyleSheet(PANEL_LEFT_LATER);
    tabWidgetLayer->horizontalHeader()->setFont(font);

    //再设置图形
    tabWidgetGraphic = new TableWidgetGraphic(0, 2, frameLeftPanel);        //图形(相对于QFrame构造的leftPanel)
    tabWidgetGraphic->verticalHeader()->setVisible(false);                  //去掉每行的行号
    tabWidgetGraphic->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);    //在需要时显示滚动条
    header.clear();
    header<< tr("图形") << tr("输出");
    tabWidgetGraphic->setHorizontalHeaderLabels(header);
    tabWidgetGraphic->horizontalHeader()->setHighlightSections(false);
    tabWidgetGraphic->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive); //可交互
    tabWidgetGraphic->setRowHeight(fontSize);                               //行高
    tabWidgetGraphic->setEditTriggers(QAbstractItemView::NoEditTriggers);   //不允许修改
    tabWidgetGraphic->setSelectionBehavior(QAbstractItemView::SelectRows);  //整行选中的方式
    tabWidgetGraphic->setSelectionMode(QAbstractItemView::SingleSelection); //只能选中一个
    tabWidgetGraphic->setFont(font);
    tabWidgetGraphic->setStyleSheet(PANEL_LEFT_GRAPHIC);
    tabWidgetGraphic->horizontalHeader()->setFont(font);

    //关联图层和图形
    tabWidgetLayer->setTableWidgetGraphic(tabWidgetGraphic);
    tabWidgetGraphic->setTableWidgetLayer(tabWidgetLayer);

    //图层通知刷新图形
    connect(tabWidgetLayer, &TableWidgetLayer::signalRefreshGraphic, tabWidgetGraphic, &TableWidgetGraphic::slotRefreshGraphic);
}

/* 移动左侧panel图形图层 */
void MainWindow::moveLeftPanelLayerGraphicScroll(void)
{
    leftPanelLayerGraphicHeight = leftPanelHeight * PANEL_LEFT_LAYER_GRAPHIC_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    int firstColumnWidth = (leftPanelWidth / 2) * PANEL_LEFT_LAYER_COL_WIDTH_DIV_ALL_COLS_WIDTH;
    int secondColumnWidth = (leftPanelWidth / 2) - firstColumnWidth;

    tabWidgetLayer->setGeometry(0, 0, leftPanelWidth / 2, leftPanelLayerGraphicHeight);
    tabWidgetLayer->setColumnWidth(0, firstColumnWidth);                                //默认列宽
    tabWidgetLayer->setColumnWidth(1, secondColumnWidth - 1);

    tabWidgetGraphic->setGeometry(leftPanelWidth / 2, 0, leftPanelWidth / 2, leftPanelLayerGraphicHeight);
    tabWidgetGraphic->setColumnWidth(0, firstColumnWidth);                                //默认列宽
    tabWidgetGraphic->setColumnWidth(1, secondColumnWidth);                               //默认列宽

}

/* 左侧panel移动图层图形Frame初始化(计算按钮尺寸) */
void MainWindow::leftPanelMoveLayerGraphicFrameInit(void)
{
    frameMoveLayerGraphic = new QFrame(frameLeftPanel);

    btnMoveUpLeft = new QPushButton(frameMoveLayerGraphic);
    lineEditLeft = new QLineEdit(frameMoveLayerGraphic);
    btnMoveDownLeft = new QPushButton(frameMoveLayerGraphic);
    btnConfirmLeft = new QPushButton(frameMoveLayerGraphic);

    btnMoveUpRight = new QPushButton(frameMoveLayerGraphic);
    lineEditRight = new QLineEdit(frameMoveLayerGraphic);
    btnMoveDownRight = new QPushButton(frameMoveLayerGraphic);
    btnConfirmRight = new QPushButton(frameMoveLayerGraphic);

    //左侧
    btnMoveUpLeft->setText(tr("上移"));
    btnMoveUpLeft->setToolTip(tr("上移"));
    lineEditLeft->setText("1");
    btnMoveDownLeft->setText(tr("下移"));
    btnMoveDownLeft->setToolTip(tr("下移"));
    btnConfirmLeft->setText(tr("确定"));
    btnConfirmLeft->setToolTip(tr("确定"));

    //右侧
    btnMoveUpRight->setText(tr("上移"));
    btnMoveUpRight->setToolTip(tr("上移"));
    lineEditRight->setText("1");
    btnMoveDownRight->setText(tr("下移"));
    btnMoveDownRight->setToolTip(tr("下移"));
    btnConfirmRight->setText(tr("确定"));
    btnConfirmRight->setToolTip(tr("确定"));
}

/* 移动左侧panel移动图层图形Frame */
void MainWindow::moveLeftPanelMoveLayerGraphicFrame(void)
{
    leftPanelMoveLayerGraphicHeight = leftPanelHeight * PANEL_LEFT_MOVE_LAYER_GRAPHIC_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    frameMoveLayerGraphic->setGeometry(0, leftPanelLayerGraphicHeight + leftPanelModuleClearance,
                                       leftPanelWidth, leftPanelMoveLayerGraphicHeight);

    //计算QPushButton按钮尺寸(考虑到初始顺序, 首次在这里使用了按钮)
    this->btnWidth  = leftPanelWidth * PANEL_LEFT_BTN_WIDTH_DIV_PANEL_LEFT_WIDTH;
    this->btnHeight = leftPanelMoveLayerGraphicHeight * PANEL_LEFT_BTN_HEIGHT_DIV_MOVE_LAYER_GRAPHIC_HEIGHT;
    //计算按钮水平间距和按钮垂直间距
    btnHorizontalSpacing = btnWidth * PANEL_LEFT_BTN_H_CLEARANCE_DIV_BTN_WIDTH;
    btnVerticalSpacing = btnHeight * PANEL_LEFT_BTN_V_CLEARANCE_DIV_BTN_HEIGHT;

    int xLeft, yLeft, xRight, yRight; //最左上角的坐标
    xLeft = (leftPanelWidth / 2 - scrollBarWidth - btnWidth) / 2;
    yLeft = (frameMoveLayerGraphic->height()- btnHeight * 2 - btnVerticalSpacing / 2) / 2;
    xRight = xLeft + leftPanelWidth / 2;
    yRight = yLeft;

    int btnWidth, btnHeight;
    btnWidth = this->btnHeight * PANEL_LEFT_MOVE_LAYER_GRAPHIC_MOVE_BTN_WIDTH_DIV_BTN_HEIGHT;
    btnHeight = this->btnHeight;
    int lineEditWidth = this->btnWidth - btnWidth * 2;

    btnMoveUpLeft->setGeometry(xLeft, yLeft, btnWidth, btnHeight);
    lineEditLeft->setGeometry(xLeft + btnWidth + 3, yLeft, lineEditWidth, btnHeight);
    btnMoveDownLeft->setGeometry(xLeft + btnWidth + lineEditWidth + 6, yLeft, btnWidth, btnHeight);
    btnConfirmLeft->setGeometry(xLeft, yLeft + btnHeight + btnVerticalSpacing / 2, this->btnWidth + 6, this->btnHeight);

    btnMoveUpRight->setGeometry(xRight, yRight, btnWidth, btnHeight);
    lineEditRight->setGeometry(xRight + btnWidth + 3, yRight, lineEditWidth, btnHeight);
    btnMoveDownRight->setGeometry(xRight + btnWidth + lineEditWidth + 6, yRight, btnWidth, btnHeight);
    btnConfirmRight->setGeometry(xRight, yRight + btnHeight + btnVerticalSpacing / 2, this->btnWidth + 6, this->btnHeight);

}

/* 左侧panel移动距离gBox初始化 */
void MainWindow::leftPanelMoveDistanceInit(void)
{
    gBoxMoveDistance = new QGroupBox(frameLeftPanel);
    gBoxMoveDistance->setTitle(tr("移动距离(mm)"));

    btnUp = new QPushButton(gBoxMoveDistance);
    btnLeft = new QPushButton(gBoxMoveDistance);
    btnDelete = new QPushButton(gBoxMoveDistance);
    btnDown = new QPushButton(gBoxMoveDistance);
    btnRight = new QPushButton(gBoxMoveDistance);
    lineEditMoveDistanceValue = new QLineEdit(gBoxMoveDistance);
}

/* 移动左侧panel移动距离gBox */
void MainWindow::moveLeftPanelMoveDistance(void)
{
    leftPanelMoveDistanceHeight = leftPanelHeight * PANEL_LEFT_MOVE_DISTANCE_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    int gBoxX, gBoxY;
    gBoxX = (leftPanelWidth - leftPanelGBoxModuleWidth) / 2;
    gBoxY = leftPanelLayerGraphicHeight + leftPanelMoveLayerGraphicHeight + leftPanelModuleClearance * 2;

    gBoxMoveDistance->setGeometry(gBoxX, gBoxY, leftPanelGBoxModuleWidth, leftPanelMoveDistanceHeight);

    //正方形按钮的边长
    int sideLength = this->btnHeight;

    int xTmp;   //最左上角那个按钮的坐标
    xTmp = (gBoxMoveDistance->width() - this->btnWidth * 2 - btnHorizontalSpacing) / 2;

    //最上面那个按钮btnUp的坐标
    int x = xTmp + this->btnWidth + btnHorizontalSpacing + this->btnWidth / 2 - (sideLength / 2);
    int y = (leftPanelMoveDistanceHeight - sideLength * 3) / 2;

    btnUp->setGeometry(x, y, sideLength, sideLength);
    btnLeft->setGeometry(x - sideLength, y + sideLength, sideLength, sideLength);
    btnDelete->setGeometry(x, y + sideLength, sideLength, sideLength);
    btnDown->setGeometry(x, y + sideLength * 2, sideLength, sideLength);
    btnRight->setGeometry(x + sideLength, y + sideLength, sideLength, sideLength);

    int lineEditX, lineEditY;
    lineEditX = leftPanelGBoxModuleWidth * PANEL_LEFT_MOVE_DISTANCE_LINEEDIT_LEFT_CLEARANE_DIV_GBOX_WIDTH;
    lineEditY = leftPanelMoveDistanceHeight * PANEL_LEFT_MOVE_DISTANCE_LINEEDIT_TOP_CLEARANE_DIV_MOVE_DISTANCE_HEIGHT;
    lineEditMoveDistanceValue->setGeometry(lineEditX, lineEditY, sideLength * 3, this->btnHeight);
}

/* 左侧panel设置gBox初始化 */
void MainWindow::leftPanelSettingInit(void)
{
    gBoxSetting = new QGroupBox(frameLeftPanel);
    gBoxSetting->setTitle(tr("设置"));

    //按钮
    btnChangeLayer = new QPushButton(gBoxSetting);
    btnCurrentGraphicCopy = new QPushButton(gBoxSetting);
    btnCurrentGraphicModify = new QPushButton(gBoxSetting);
    btnBatchProcessing = new QPushButton(gBoxSetting);
    btnCurrentLayerSetting = new QPushButton(gBoxSetting);
    btnSetToDatumPoint = new QPushButton(gBoxSetting);

    btnChangeLayer->setText(tr("变图层"));
    btnChangeLayer->setToolTip(tr("变图层"));
    btnCurrentGraphicCopy->setText(tr("当前图复制"));
    btnCurrentGraphicCopy->setToolTip(tr("当前图复制"));
    btnCurrentGraphicModify->setText(tr("当前图修改"));
    btnCurrentGraphicModify->setToolTip(tr("当前图修改"));
    btnBatchProcessing->setText(tr("批量处理"));
    btnBatchProcessing->setToolTip(tr("批量处理"));
    btnCurrentLayerSetting->setText(tr("当前图层设置"));
    btnCurrentLayerSetting->setToolTip(tr("当前图层设置"));
    btnSetToDatumPoint->setText(tr("设为基准点"));
    btnSetToDatumPoint->setToolTip(tr("设为基准点"));
}

/* 移动左侧panel设置gBox */
void MainWindow::moveLeftPanelSetting(void)
{
    leftPanelSettingHeight = leftPanelHeight * PANEL_LEFT_SETTING_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    int gBoxX, gBoxY;
    gBoxX = (leftPanelWidth - leftPanelGBoxModuleWidth) / 2;
    gBoxY = leftPanelLayerGraphicHeight + leftPanelMoveLayerGraphicHeight +
            leftPanelMoveDistanceHeight + leftPanelModuleClearance * 3;

    gBoxSetting->setGeometry(gBoxX, gBoxY, leftPanelGBoxModuleWidth, leftPanelSettingHeight);

    //最左上角那个按钮的坐标
    int x = (gBoxSetting->width() - this->btnWidth * 2 - btnHorizontalSpacing) / 2;
    int y = (gBoxSetting->height() - this->btnHeight * 3 - btnVerticalSpacing * 2) / 2;

    btnChangeLayer->setGeometry(x, y, this->btnWidth, this->btnHeight);
    btnCurrentGraphicCopy->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y,
                                       this->btnWidth, this->btnHeight);
    btnCurrentGraphicModify->setGeometry(x, y + this->btnHeight + btnVerticalSpacing,
                                         this->btnWidth, this->btnHeight);
    btnBatchProcessing->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y + this->btnHeight
                                    + btnVerticalSpacing, this->btnWidth, this->btnHeight);
    btnCurrentLayerSetting->setGeometry(x, y + this->btnHeight * 2 + btnVerticalSpacing * 2,
                                        this->btnWidth, this->btnHeight);
    btnSetToDatumPoint->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y + this->btnHeight * 2 +
                                    btnVerticalSpacing * 2, this->btnWidth, this->btnHeight);

}

/* 左侧panel变换gBox初始化 */
void MainWindow::leftPanelTransformInit(void)
{
    gBoxTransform = new QGroupBox(frameLeftPanel);
    gBoxTransform->setTitle(tr("变换"));

    btnHorizontalMirror = new QPushButton(gBoxTransform);
    btnVerticalMirror = new QPushButton(gBoxTransform);
    labelRotationOpt = new QLabel(gBoxTransform);
    lineEditRotateValue = new QLineEdit(gBoxTransform);
    btnRotate = new QPushButton(gBoxTransform);

    btnHorizontalMirror->setText(tr("水平镜像"));
    btnHorizontalMirror->setToolTip(tr("水平镜像"));
    btnVerticalMirror->setText(tr("垂直镜像"));
    btnVerticalMirror->setToolTip(tr("垂直镜像"));
    labelRotationOpt->setText(tr("旋转操作(角度)"));
    labelRotationOpt->setToolTip(tr("旋转操作(角度)"));
    lineEditRotateValue->setToolTip(tr("旋转操作(角度)"));
    btnRotate->setText(tr("旋转"));
    btnRotate->setToolTip(tr("旋转"));
}

/* 移动左侧panel变换gBox */
void MainWindow::moveLeftPanelTransform(void)
{
    leftPanelTransformHeight = leftPanelHeight * PANEL_LEFT_TANSFORM_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    int gBoxX, gBoxY;
    gBoxX = (leftPanelWidth - leftPanelGBoxModuleWidth) / 2;
    gBoxY = leftPanelLayerGraphicHeight + leftPanelMoveLayerGraphicHeight +
            leftPanelMoveDistanceHeight + leftPanelSettingHeight + leftPanelModuleClearance * 4;

    gBoxTransform->setGeometry(gBoxX, gBoxY, leftPanelGBoxModuleWidth, leftPanelTransformHeight);

    //最左上角那个按钮的坐标
    int x = (gBoxTransform->width() - this->btnWidth * 2 - btnHorizontalSpacing) / 2;
    int y = (gBoxTransform->height() - this->btnHeight * 2 - btnVerticalSpacing) / 2;

    btnHorizontalMirror->setGeometry(x, y, this->btnWidth, this->btnHeight);
    btnVerticalMirror->setGeometry(x, y + this->btnHeight + btnVerticalSpacing, this->btnWidth,
                                   this->btnHeight);
    labelRotationOpt->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y, this->btnWidth,
                                  this->btnHeight);
    lineEditRotateValue->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y + this->btnHeight
                                     + btnVerticalSpacing, this->btnWidth / 2 - 5, this->btnHeight);
    btnRotate->setGeometry(x + this->btnWidth + btnHorizontalSpacing + this->btnWidth / 2, y +
                           this->btnHeight + btnVerticalSpacing, this->btnWidth / 2, this->btnHeight);
}

/* 左侧panel缝纫模式gBox初始化 */
void MainWindow::leftPanelSewingModeInit(void)
{
    gBoxSewingMode = new QGroupBox(frameLeftPanel);
    gBoxSewingMode->setTitle(tr("缝纫模式"));

    //按钮
    btnSimSewing = new QPushButton(gBoxSewingMode);             //模拟缝纫
    btnSewingPointCode = new QPushButton(gBoxSewingMode);       //缝纫点代码
    btnSpecialSewing = new QPushButton(gBoxSewingMode);         //特殊缝

    btnSimSewing->setText(tr("模拟缝纫"));
    btnSimSewing->setToolTip(tr("模拟缝纫"));
    btnSewingPointCode->setText(tr("缝纫点代码"));
    btnSewingPointCode->setToolTip(tr("缝纫点代码"));
    btnSpecialSewing->setText(tr("特殊缝"));
    btnSpecialSewing->setToolTip(tr("特殊缝"));
}

/* 移动左侧panel缝纫模式gBox */
void MainWindow::moveLeftPanelSewingMode(void)
{
    leftPanelSewingModeHeight = leftPanelHeight * PANEL_LEFT_SEWING_MODE_HEIGHT_DIV_PANEL_LEFT_HEIGHT;

    int gBoxX, gBoxY;
    gBoxX = (leftPanelWidth - leftPanelGBoxModuleWidth) / 2;
    gBoxY = leftPanelLayerGraphicHeight + leftPanelMoveLayerGraphicHeight +
            leftPanelMoveDistanceHeight + leftPanelSettingHeight +
            leftPanelTransformHeight + leftPanelModuleClearance * 5;

    gBoxSewingMode->setGeometry(gBoxX, gBoxY, leftPanelGBoxModuleWidth, leftPanelSewingModeHeight);

    //最左上角那个按钮的坐标
    int x = (gBoxSewingMode->width() - this->btnWidth * 2 - btnHorizontalSpacing) / 2;
    int y = (gBoxSewingMode->height() - this->btnHeight * 2 - btnVerticalSpacing) / 2;

    btnSimSewing->setGeometry(x, y, this->btnWidth, this->btnHeight);
    btnSewingPointCode->setGeometry(x + this->btnWidth + btnHorizontalSpacing, y, this->btnWidth,
                                    this->btnHeight);
    btnSpecialSewing->setGeometry(x, y + this->btnHeight + btnVerticalSpacing, this->btnWidth,
                                  this->btnHeight);
}

/* 底部操作panel初始化 */
void MainWindow::bottomPanelInit(void)
{
    frameBottomPanel = new QFrame(this);

    //免基准CheckBox
    chkBoxNoBenchmark = new QCheckBox(frameBottomPanel);
    chkBoxNoBenchmark->setText(tr("免基准"));
    chkBoxNoBenchmark->setToolTip(tr("免基准"));

    //显示整图RadioButton
    radioBtnDispWholePic = new QRadioButton(frameBottomPanel);
    radioBtnDispWholePic->setChecked(true);
    radioBtnDispWholePic->setText(tr("显示整图"));
    radioBtnDispWholePic->setToolTip(tr("显示整图"));

    //显示图层RadioButton
    radioBtnDispLayer = new QRadioButton(frameBottomPanel);
    radioBtnDispLayer->setText(tr("显示图层"));
    radioBtnDispLayer->setToolTip(tr("显示图层"));
    radioBtnDispLayer->setChecked(false);

    btnGroupPicAndLayer = new QButtonGroup(frameBottomPanel);
    btnGroupPicAndLayer->addButton(radioBtnDispWholePic);
    btnGroupPicAndLayer->addButton(radioBtnDispLayer);

    //显示序号CheckBox
    chkBoxDispSerialNumber = new QCheckBox(frameBottomPanel);
    chkBoxDispSerialNumber->setChecked(false);
    chkBoxDispSerialNumber->setText(tr("显示序号"));
    chkBoxDispSerialNumber->setToolTip(tr("显示序号"));

    //显示线
    radioBtnDispLine = new QRadioButton(frameBottomPanel);\
    radioBtnDispLine->setEnabled(false);
    radioBtnDispLine->setText(tr("显示线"));
    radioBtnDispLine->setToolTip(tr("显示线"));

    //显示点
    radioBtnDispDot = new QRadioButton(frameBottomPanel);
    radioBtnDispDot->setEnabled(false);
    radioBtnDispDot->setText(tr("显示点"));
    radioBtnDispDot->setToolTip(tr("显示点"));

    //正常显示
    radioBtnDispNormal = new QRadioButton(frameBottomPanel);
    radioBtnDispNormal->setEnabled(true);
    radioBtnDispNormal->setChecked(true);
    radioBtnDispNormal->setText(tr("正常显示"));
    radioBtnDispNormal->setToolTip(tr("正常显示"));

    btnGroupDisplayMode = new QButtonGroup(frameBottomPanel);
    btnGroupDisplayMode->addButton(radioBtnDispLine);
    btnGroupDisplayMode->addButton(radioBtnDispDot);
    btnGroupDisplayMode->addButton(radioBtnDispNormal);

    //退出按钮
    btnExit = new QPushButton(frameBottomPanel);
    btnExit->setText(tr("退出"));

    //版本号label
    labelVersion = new QLabel(frameBottomPanel);
    QString infoStr;
    infoStr = tr(PANEL_BOTTOM_VERSION_LABEL);
    labelVersion->setText(infoStr);
}

/* 移动底部操作panel */
void MainWindow::moveBottomPanel(void)
{
    bottomPanelHeight = screenYSize - (menuBarHeight + toolBarHeight + canvasViewHeight + statusBarHeight);   //高度
    bottomPanelWidth = screenXSize - leftPanelWidth;

    frameBottomPanel->setGeometry(leftPanelWidth, menuBarHeight + toolBarHeight + canvasViewHeight + statusBarHeight,
                                  bottomPanelWidth, bottomPanelHeight);

    int leftDiatanceSum = 0; //之前已经加过的距离
    int leftestClearance = bottomPanelWidth * PANEL_BOTTOM_LEFTEST_CLEARANE_DIV_PANEL_BOTTOM_WIDTH;
    int checkableClearance = bottomPanelWidth * PANEL_BOTTOM_CHECKABLE_CLEARANCE_DIV_PANEL_BOTTOM_WIDTH;
    int verticalMiddle = bottomPanelHeight / 2;

    //免基准CheckBox
    leftDiatanceSum += leftestClearance;
    QSize sizeChkBoxNoBenchmark = chkBoxNoBenchmark->size();
    chkBoxNoBenchmark->setGeometry(leftDiatanceSum, verticalMiddle - sizeChkBoxNoBenchmark.height() / 2,
                                   sizeChkBoxNoBenchmark.width(), sizeChkBoxNoBenchmark.height());
    //显示整图RadioButton
    leftDiatanceSum += sizeChkBoxNoBenchmark.width() + checkableClearance;
    QSize sizeRadioBtnDispWholePic = radioBtnDispWholePic->size();
    radioBtnDispWholePic->setGeometry(leftDiatanceSum, verticalMiddle - sizeRadioBtnDispWholePic.height() / 2,
                                      sizeRadioBtnDispWholePic.width(), sizeRadioBtnDispWholePic.height());
    //显示图层RadioButton
    leftDiatanceSum += sizeRadioBtnDispWholePic.width() + checkableClearance;
    QSize sizeRadioBtnDispLayer = radioBtnDispLayer->size();
    radioBtnDispLayer->setGeometry(leftDiatanceSum, verticalMiddle - sizeRadioBtnDispLayer.height() / 2,
                                   sizeRadioBtnDispLayer.width(), sizeRadioBtnDispLayer.height());
    //显示序号CheckBox
    leftDiatanceSum += sizeRadioBtnDispLayer.width() + checkableClearance;
    QSize sizeChkBoxDispSerialNumber = chkBoxDispSerialNumber->size();
    chkBoxDispSerialNumber->setGeometry(leftDiatanceSum, verticalMiddle - sizeChkBoxDispSerialNumber.height() / 2,
                                   sizeChkBoxDispSerialNumber.width(), sizeChkBoxDispSerialNumber.height());
    //显示线
    leftDiatanceSum += sizeChkBoxDispSerialNumber.width()  + checkableClearance;
    radioBtnDispLine->setGeometry(leftDiatanceSum, verticalMiddle - radioBtnDispLine->height() / 2,
                                  radioBtnDispLine->width(), radioBtnDispLine->height());
    //显示点
    leftDiatanceSum += radioBtnDispLine->width()  + checkableClearance * 0.05;
    radioBtnDispDot->setGeometry(leftDiatanceSum, verticalMiddle - radioBtnDispDot->height() / 2,
                                   radioBtnDispDot->width(), radioBtnDispDot->height());
    //正常显示
    leftDiatanceSum += radioBtnDispDot->width()  + checkableClearance * 0.05;
    radioBtnDispNormal->setGeometry(leftDiatanceSum, verticalMiddle - radioBtnDispNormal->height() / 2,
                                   radioBtnDispNormal->width(), radioBtnDispNormal->height());

    //退出按钮
    int btnX, btnY; //按钮左上角坐标
    btnX = bottomPanelWidth - bottomPanelWidth * PANEL_BOTTOM_RIGHTEST_CLEARANE_DIV_PANEL_BOTTOM_WIDTH - this->btnWidth;
    btnY = bottomPanelHeight / 2 - this->btnHeight / 2;
    btnExit->setGeometry(btnX, btnY, this->btnWidth, this->btnHeight);
    //版本号label
    int labelX, labelY; //label左上角坐标
    int labelVersionWidth = bottomPanelWidth * PANEL_BOTTOM_VERSION_LABEL_WIDTH_DIV_PANEL_BOTTOM_WIDTH;
    labelX = btnX - labelVersionWidth - checkableClearance * 0.05;
    labelY = bottomPanelHeight / 2 - labelVersion->size().height() / 2;
    labelVersion->setGeometry(labelX, labelY, labelVersionWidth, labelVersion->size().height());
}

/* 状态栏初始化 */
void MainWindow::statusBarInit(void)
{
    statusBarWidth = screenXSize - leftPanelWidth;
    statusBarHeight = toolBarHeight;        //状态栏高度等于工具栏高度

    //位置
    int x = leftPanelWidth;
    int y = menuBarHeight + toolBarHeight + screenYSize * 0.83;

    lineEditStatusBar = new QLineEdit(this);
    lineEditStatusBar->setGeometry(x, y, statusBarWidth, statusBarHeight);
    lineEditStatusBar->setText(tr("     图形大小(0mm, 0mm)  总针数=0    X=0mm, Y=0mm"));
    lineEditStatusBar->setStyleSheet("border:none;border-left:1px solid #959595;border-bottom: 1px solid #959595;");
}

/* 画板初始化 */
void MainWindow::canvasInit(void)
{
    int x = leftPanelWidth;                               //位置
    int y = menuBarHeight + toolBarHeight;
    canvasViewWidth = screenXSize - leftPanelWidth;       //画板view宽度
    canvasViewHeight = screenYSize * 0.83;    //画板view高度

    //画板scene宽度, 画板scene高度
    canvasSceneWidth = CANVAS_DEFAULT_WIDTH;
    canvasSceneHeight = CANVAS_DEFAULT_HEIGHT;

    /* 图形画板相关 */
    canvasViewPattern = new CanvasViewPattern(this);                //画板view
    canvasViewPattern->setMainWindow(this);                         //设置mainwindow
    canvasViewPattern->rightBtnMenuInit();                          //初始化右键菜单功能
    canvasViewPattern->setTableWidgetLayer(tabWidgetLayer);         //设置tableWidget
    canvasViewPattern->setTableWidgetGraphic(tabWidgetGraphic);
    canvasScenePattern = new QGraphicsScene(canvasViewPattern);     //画板scene
    canvasScenePattern->setSceneRect(-canvasSceneWidth / 2, -canvasSceneHeight / 2, canvasSceneWidth, canvasSceneHeight);  //设置画板view的尺寸矩形
    canvasViewPattern->setScene(canvasScenePattern);                //关联
    canvasViewPattern->setGeometry(x, y, canvasViewWidth, canvasViewHeight); //设置位置
    tabWidgetGraphic->setCanvasView(canvasViewPattern);             //给tabWidget设置画板对象
    tabWidgetLayer->setCanvasViewPattern(canvasViewPattern);
    //场景中图元的选中情况有变化时调用的槽
    connect(canvasScenePattern, &QGraphicsScene::selectionChanged, canvasViewPattern, &CanvasViewPattern::slotSceneSelectionChanged);
    //从场景选中图元, 通知tabWidgetLayer刷新选中图层(图形)的槽
    connect(canvasViewPattern, &CanvasViewPattern::signalSceneSelectedGraphic, tabWidgetLayer, &TableWidgetLayer::slotSceneSelectedGraphic);
}

/* 剩余杂项控件初始化 */
void MainWindow::miscWidgetInit(void)
{
    comboBoxZoom = new QComboBox(this);                     //缩放下拉框
    labelZoom    = new QLabel(this);                        //缩放
    labelColorDemo = new QLabel(this);                      //颜色示例
    labelColorDisp = new QLabel(this);                      //颜色显示

    //下拉框
    comboBoxZoom->addItem("200%"); comboBoxZoom->addItem("180%");
    comboBoxZoom->addItem("160%"); comboBoxZoom->addItem("140%");
    comboBoxZoom->addItem("120%"); comboBoxZoom->addItem("100%");
    comboBoxZoom->addItem("80%");  comboBoxZoom->addItem("60%");
    comboBoxZoom->addItem("40%");  comboBoxZoom->addItem("20%");
    comboBoxZoom->setFont(font);
    comboBoxZoom->setEditable(true);
    comboBoxZoom->setEditText("100%");

    //缩放标签
    labelZoom->setText(tr("缩放"));
    labelZoom->setFont(font);

    //颜色显示标签
    labelColorDisp->setText(tr("颜色显示"));
    labelColorDisp->setFont(font);

    changeControllerSize(QSize(screenXSize, screenYSize));
    setWindowState(Qt::WindowMaximized);     //全屏
}

/* 移动剩余杂项控件 */
void MainWindow::moveMiscWidget(void)
{
    //所有控件的高度
    int height = toolBarHeight * 0.65;
    //间距
    int internalClearance = toolBarHeight * TOOL_BAR_MISC_WIDGET_INTERNAL_CLEARANCE_DIV_TOOL_BAR_HEIGHT;    //内部的
    int externalClearance = toolBarHeight * TOOL_BAR_MISC_WIDGET_EXTERNAL_CLEARANCE_DIV_TOOL_BAR_HEIGHT;    //外部的
    //起始位置
    int x = screenXSize * 0.86;
    int y = menuBarHeight + toolBarHeight / 2 - (height / 2);

    //下拉框
    int comboBoxWidth = toolBarHeight * 2;

    comboBoxZoom->setGeometry(x, y, comboBoxWidth, height);
    //缩放标签
    x += comboBoxWidth + internalClearance + 2;
    int zoomLabelWidth = screenXSize * TOOL_BAR_ZOOM_LABEL_WIDTH_DIV_SCREEN_WIDTH;
    labelZoom->setGeometry(x, y, zoomLabelWidth, height);
    //颜色示例标签
    x += zoomLabelWidth + externalClearance;
    int colorDemoLabelWidth = toolBarHeight * 1.75;
    labelColorDemo->setGeometry(x, y, colorDemoLabelWidth, height);
    //颜色显示标签
    x += colorDemoLabelWidth + internalClearance + 2;
    int colorDisplayLabelWidth = screenXSize * TOOL_BAR_COLOR_DISP_LABEL_WIDTH_DIV_SCREEN_WIDTH;
    labelColorDisp->setGeometry(x, y, colorDisplayLabelWidth, height);
}

/* 式样初始化 */
void MainWindow::qssInit(void)
{
    /* 左侧panel相关 */
    frameLeftPanel->setStyleSheet(PANEL_LEFT_QSS);                                  //panel
    frameLeftPanel->setObjectName(PANEL_LEFT_OBJ_NAME);
    frameMoveLayerGraphic->setStyleSheet(PANEL_LEFT_MOVE_LAYER_GRAPHIC_FRAME_QSS);  //移动图形图层的frame
    gBoxMoveDistance->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_GBOX_QSS);             //移动距离GroupBox
    btnUp->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_UP_BTN_QSS);                      //移动距离GroupBox的按钮
    btnDown->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_DOWN_BTN_QSS);
    btnLeft->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_LEFT_BTN_QSS);
    btnRight->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_RIGHT_BTN_QSS);
    btnDelete->setStyleSheet(PANEL_LEFT_MOVE_DISTANCE_DELETE_BTN_QSS);
    gBoxSetting->setStyleSheet(PANEL_LEFT_SETTING_GBOX_QSS);                        //设置GroupBox式样
    gBoxTransform->setStyleSheet(PANEL_LEFT_TANSFORM_GBOX_QSS);                     //变换GroupBox式样
    gBoxSewingMode->setStyleSheet(PANEL_LEFT_SEWING_MODE_GBOX_QSS);                 //缝纫模式GroupBox式样

    /* 底部panel相关 */
    frameBottomPanel->setStyleSheet(PANEL_BOTTOM_QSS);                              //panel
    frameBottomPanel->setObjectName(PANEL_BOTTOM_QSS_OBJ_NAME);

    /* 画板 */
    canvasViewPattern->setStyleSheet(CANVAS_BOTTOM_QSS);
    canvasViewPattern->setObjectName(CANVAS_QSS_OBJ_NAME);

    /* 杂项 */
    labelColorDemo->setStyleSheet(MISC_COLOR_DEMO_QSS);
}

/* 槽初始化 */
void MainWindow::slotsInit(void)
{
	/* 菜单栏 */
    connect(actionChangeSize, &QAction::triggered, this, &MainWindow::slotActionChangeSizeTriggered);   //改变图形大小
    connect(actionCloseGraphic, &QAction::triggered, this, &MainWindow::slotActionCloseGraphicTriggered);        //封闭图形首尾点间距
    connect(actionDeleteMultiGraphic, &QAction::triggered, this, &MainWindow::slotActionDeleteMultiGraphicTriggered); //打开文件时是否删除重合图形
    connect(actionChinese, &QAction::triggered, this, &MainWindow::slotActionChineseTriggered);       //选择中文事件
    connect(actionEnglish, &QAction::triggered, this, &MainWindow::slotActionEnglishTriggered);       //选择英文事件
    connect(actionVietnam, &QAction::triggered, this, &MainWindow::slotActionVietnamTriggered);       //选择越南语事件
    connect(actionItalian, &QAction::triggered, this, &MainWindow::slotActionItalianTriggered);       //选择意大利语事件
    connect(actionTurkish, &QAction::triggered, this, &MainWindow::slotActionTurkishTriggered);       //选择土耳其语事件
    connect(actionSpanish, &QAction::triggered, this, &MainWindow::slotActionSpanishTriggered);       //选择西班牙语事件
    connect(actionRussian, &QAction::triggered, this, &MainWindow::slotActionRussianTriggered);       //选择俄语事件
    connect(actionJapanese, &QAction::triggered, this, &MainWindow::slotActionJapaneseTriggered);     //选择日本语事件
    connect(actionPaint, &QAction::triggered, this, &MainWindow::slotBtnPaintClicked);                //作图事件
    connect(actionTools, &QAction::triggered, this, &MainWindow::slotBtnToolsClicked);                //工具事件

    /* 工具栏 */
    connect(toolBtnOpenFile, &QToolButton::clicked, this, &MainWindow::slotBtnOpenFileClicked);      //打开文件按钮
    connect(toolBtnSaveAs, &QToolButton::clicked, this, &MainWindow::slotBtnSaveAsClicked);          //另存为按钮
    connect(toolBtnUndo, &QToolButton::clicked, this, &MainWindow::slotBtnUndoClicked);              //撤销按钮
    connect(toolBtnRedo, &QToolButton::clicked, this, &MainWindow::slotBtnRedoClicked);              //重做按钮
    connect(toolBtnZoomUp, &QToolButton::clicked, this, &MainWindow::slotBtnZoomUpClicked);          //放大按钮
    connect(toolBtnZoomDown, &QToolButton::clicked, this, &MainWindow::slotBtnZoomDownClicked);      //缩小按钮
    connect(toolBtnDispRestore, &QToolButton::clicked, this, &MainWindow::slotBtnDispRestoreClicked);//显示恢复
    connect(toolBtnPicDrag, &QToolButton::clicked, this, &MainWindow::slotBtnPicDragClicked);        //图形拖动
    connect(toolBtnNormalOpt, &QToolButton::clicked, this, &MainWindow::slotBtnNormalOptClicked);    //正常操作
    connect(toolBtnHelp, &QToolButton::clicked, this, &MainWindow::slotBtnHelpClicked);              //帮助
    connect(toolBtnUpdate, &QToolButton::clicked, this, &MainWindow::slotBtnUpdateClicked);          //软件升级
    connect(comboBoxZoom, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MainWindow::slotComboBoxZoomActivated);      //缩放QComboBox某行被使能
    connect(lineEditStatusBar, &QLineEdit::editingFinished, this, &MainWindow::slotEditLineFinished); //缩放状态编辑栏

    /* 左侧panel */
    //移动图层图形
    connect(lineEditLeft, &QLineEdit::editingFinished, this, &MainWindow::slotEditLineFinished);      //左编辑栏
    connect(btnConfirmLeft, &QPushButton::clicked, this, &MainWindow::slotBtnConfirmLeftClicked);     //左确定按钮
    connect(btnMoveUpLeft, &QPushButton::clicked, this, &MainWindow::slotBtnMoveUpLeftClicked);       //左上移按钮
    connect(btnMoveDownLeft, &QPushButton::clicked, this, &MainWindow::slotBtnMoveDownLeftClicked);   //左下移按钮
    connect(lineEditRight, &QLineEdit::editingFinished, this, &MainWindow::slotEditLineFinished);     //右编辑栏
    connect(btnConfirmRight, &QPushButton::clicked, this, &MainWindow::slotBtnConfirmRightClicked);   //右确定按钮
    connect(btnMoveUpRight, &QPushButton::clicked, this, &MainWindow::slotBtnMoveUpRightClicked);     //右上移按钮
    connect(btnMoveDownRight, &QPushButton::clicked, this, &MainWindow::slotBtnMoveDownRightClicked); //右下移按钮

    //移动距离
    connect(lineEditMoveDistanceValue, &QLineEdit::editingFinished, this, &MainWindow::slotEditLineFinished);   //移动距离编辑栏
    connect(btnUp, &QPushButton::clicked, this, &MainWindow::slotBtnMoveUpClicked);        //向上移动按钮
    connect(btnDown, &QPushButton::clicked, this, &MainWindow::slotBtnMoveDownClicked);    //向下移动按钮
    connect(btnLeft, &QPushButton::clicked, this, &MainWindow::slotBtnMoveLeftClicked);    //向左移动按钮
    connect(btnRight, &QPushButton::clicked, this, &MainWindow::slotBtnMoveRightClicked);  //向右移动按钮
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::slotBtnDeleteClicked);    //删除按钮
    //设置gBox
    connect(btnChangeLayer, &QPushButton::clicked, this, &MainWindow::slotBtnChangeLayerClicked);                     //变图层按钮
    connect(btnCurrentGraphicCopy, &QPushButton::clicked, this, &MainWindow::slotBtnCurrentGraphicCopyClicked);       //当前图复制按钮
    connect(btnCurrentGraphicModify, &QPushButton::clicked, this, &MainWindow::slotBtnCurrentGraphicModifyClicked);   //当前图修改按钮
    connect(btnBatchProcessing, &QPushButton::clicked, this, &MainWindow::slotBtnBatchProcessingClicked);             //批量处理按钮
    connect(btnCurrentLayerSetting, &QPushButton::clicked, this, &MainWindow::slotBtnCurrentLayerSettingClicked);     //当前图层设置按钮
	connect(btnSetToDatumPoint, &QPushButton::clicked, this, &MainWindow::slotBtnSetToDatumPointClicked);             //设为基准点按钮
    //变换gBox
    connect(btnHorizontalMirror, &QPushButton::clicked, this, &MainWindow::slotBtnHorizontalMirrorClicked);           //水平镜像按钮
    connect(btnVerticalMirror, &QPushButton::clicked, this, &MainWindow::slotBtnVerticalMirrorClicked);               //垂直镜像按钮
    connect(lineEditRotateValue, &QLineEdit::editingFinished, this, &MainWindow::slotEditLineFinished);               //旋转编辑栏
    connect(btnRotate, &QPushButton::clicked, this, &MainWindow::slotBtnRotateClicked);                               //旋转按钮
    //缝纫模式gBox
    connect(btnSimSewing, &QPushButton::clicked, this, &MainWindow::slotBtnSimSewingClicked);                         //缝纫按钮模拟按钮
    connect(btnSpecialSewing, &QPushButton::clicked, this, &MainWindow::slotBtnSpecialSewingClicked);                 //特殊缝按钮按钮
    connect(btnSewingPointCode, &QPushButton::clicked, this, &MainWindow::slotBtnSewingPointCodeClicked);             //缝纫点代码按钮按钮

    /* 底部panel */
    connect(btnExit, &QPushButton::clicked, this, &MainWindow::slotBtnExitWinClicked);                                //退出按钮
    connect(radioBtnDispLayer, &QRadioButton::clicked, this, &MainWindow::slotBottomRadioBtnGraphicLayerClicked);     //显示图层还是显示整图按钮
    connect(radioBtnDispWholePic, &QRadioButton::clicked, this, &MainWindow::slotBottomRadioBtnGraphicLayerClicked);
    connect(radioBtnDispDot, &QRadioButton::clicked, this, &MainWindow::slotBottomRadioBtnDotLineNormalClicked);      //显示点/线/正常
    connect(radioBtnDispLine, &QRadioButton::clicked, this, &MainWindow::slotBottomRadioBtnDotLineNormalClicked);
    connect(radioBtnDispNormal, &QRadioButton::clicked, this, &MainWindow::slotBottomRadioBtnDotLineNormalClicked);
    connect(chkBoxDispSerialNumber, &QCheckBox::clicked, this, &MainWindow::slotChkBoxDispSerialNumberClicked);       //显示图形序号复选框
    connect(chkBoxNoBenchmark, &QCheckBox::clicked, this, &MainWindow::slotChkBoxNoBenchmarkClicked);                 //免基准复选框
}

//根据文件绘制图形(总入口)
void MainWindow::drawGraphicFromFile(QString filePathName)
{
    //获取文件后缀名
    QFileInfo fileinfo = QFileInfo(filePathName);
    QString suffix = fileinfo.suffix().toLower();

    if (suffix == "mtp")
    {
        if (canvasViewPattern->initGraphicFile(filePathName, 1) == true) {
            openedfileType = "mtp";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else if (suffix == "ntp")
    {
        if (canvasViewPattern->initGraphicFile(filePathName, 2) == true) {
            openedfileType = "ntp";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else if (suffix == "plt")
    {
        if (canvasViewPattern->initGraphicFile(filePathName, 3) == true) {
            openedfileType = "plt";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else if (suffix == "dxf") {
        if (canvasViewPattern->initGraphicFile(filePathName, 4) == true) {
            openedfileType = "dxf";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else if (suffix == "sdf") {
        if (canvasViewPattern->initGraphicFile(filePathName, 100) == true) {
            openedfileType = "sdf";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else if (suffix == "ai") {
        if (canvasViewPattern->initGraphicFile(filePathName, 10) == true) {
            openedfileType = "ai";
            this->openedfilePath = filePathName;
            this->setMainTitleContent(this->openedfilePath);
        } else {
            openedfileType = "null";
            this->openedfilePath = "null";
            canvasViewPattern->clearScene();
            tabWidgetLayer->clearContent();
            tabWidgetGraphic->clearContent();
            this->setMainTitleContent("");
        }
    }
    else {
        QMessageBox::warning(this, tr("错误"), tr("不支持的文件格式！"));
    }
}

/* 获取是否显示图元的id */
bool MainWindow::getIsShowSerialNumber(void)
{
    return isShowSerialNumber;
}

/* 获取显示图层还是显示整图 */
bool MainWindow::getShowAllLayer(void)
{
    return showAllLayer;
}

/* 设置图层颜色示例 */
void MainWindow::setLayerColorDemo(QString color)
{
    QString qss = "QLabel {background: ";
    qss += color + ";}";
    labelColorDemo->setStyleSheet(qss);
}

/* 设置左侧图层列表下面的LineEdit */
void MainWindow::setLayerLineEdit(int layer)
{
    this->lineEditLeft->setText(QString("%1").arg(layer));
}

/* 设置左侧图形列表下面的LineEdit */
void MainWindow::setGraphicLineEdit(int id)
{
    this->lineEditRight->setText(QString("%1").arg(id));
}

/* 设置标题栏显示内容 */
void MainWindow::setMainTitleContent(QString filepath)
{
    QString contentStr;
    if (filepath.isEmpty() == true) {
        contentStr = tr("SewMake数控编制软件");
        this->setWindowTitle(contentStr);
    }
    else {
        contentStr += tr("SewMake数控编制软件");
        contentStr += " [";
        contentStr += filepath;
        contentStr += "]";
        this->setWindowTitle(contentStr);
    }
}

/* 设置状态栏内容 */
void MainWindow::setStatusBarContent(qreal width, qreal height, int needleCount, QPointF mousePos)
{
    QString strValueWidth = QString::number(width, 'f', 3);
    QString strValueHeight = QString::number(height, 'f', 3);
    QString strNeedleCount = QString::number(needleCount, 10);
    QString strValuePosX = QString::number(mousePos.x(), 'f', 3);
    QString strValuePosY = QString::number(-mousePos.y(), 'f', 3);

    QString content = QString(tr("     图形大小("));
    content += strValueWidth;
    content += tr("mm, ");
    content += strValueHeight;
    content += tr("mm)  ");
    content += tr("总针数=");
    content += strNeedleCount;
    content += tr("  X=");
    content += strValuePosX;
    content += tr("mm, Y=");
    content += strValuePosY;
    content += tr("mm");

    this->lineEditStatusBar->setText(content);
}

/* 设置底部显示模式radioBtn是否Enable */
void MainWindow::setBottomDotLineRadioBtnEnable(
        bool dotRadioBtnEnable, bool lineRadioBtnEnable, bool normalRadioBtnEnable)
{
    if (dotRadioBtnEnable == true) {
        this->radioBtnDispDot->setEnabled(true);
    } else {
        this->radioBtnDispDot->setEnabled(false);
    }

    if (lineRadioBtnEnable == true) {
        this->radioBtnDispLine->setEnabled(true);
    } else {
        this->radioBtnDispLine->setEnabled(false);
    }

    if (normalRadioBtnEnable == true) {
        this->radioBtnDispNormal->setEnabled(true);
    } else {
        this->radioBtnDispNormal->setEnabled(false);
    }
}

/* 设置底部显示模式radioBtn是否Checkable */
void MainWindow::setBottomDotLineRadioBtnCheckable(
        bool dotRadioBtnCheckable, bool lineRadioBtnCheckable, bool normalRadioBtnCheckable)
{
    if (dotRadioBtnCheckable == true) {
        this->radioBtnDispDot->setChecked(true);
    } else {
        this->radioBtnDispDot->setChecked(false);
    }

    if (lineRadioBtnCheckable == true) {
        this->radioBtnDispLine->setChecked(true);
    } else {
        this->radioBtnDispLine->setChecked(false);
    }

    if (normalRadioBtnCheckable == true) {
        this->radioBtnDispNormal->setChecked(true);
    } else {
        this->radioBtnDispNormal->setChecked(false);
    }
}

/* 设置缩放比例大小显示值 */
void MainWindow::setScaleRatioContent(int ratio)
{
    comboBoxZoom->setEditText(QString("%1%").arg(ratio));
}

/* 获取屏幕x尺寸 */
int MainWindow::getScreenXSize(void)
{
    return screenXSize;
}

/* 获取屏幕y尺寸 */
int MainWindow::getScreenYSize(void)
{
    return screenYSize;
}

/* 退出按钮槽函数 */
void MainWindow::slotBtnExitWinClicked(void)
{
    this->close();
}

/* 改变图形大小槽函数 */
void MainWindow::slotActionChangeSizeTriggered(void)
{
    if (changeShapeSizeWindow != nullptr) {
        delete changeShapeSizeWindow;
        changeShapeSizeWindow = nullptr;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    qreal leftTopX, leftTopY, rightBottomX, rightBottomY;
    interface->ArmGetScaleOfAllUints(&leftTopX, &leftTopY, &rightBottomX, &rightBottomY);
    changeShapeSizeWindow = new ChangeShapeSize(QPointF(fabs(leftTopX - rightBottomX),
                                                        fabs(leftTopY - rightBottomY)));

    //阻塞除当前窗体之外的所有的窗体
    changeShapeSizeWindow->setWindowModality(Qt::ApplicationModal);
    changeShapeSizeWindow->show();

    connect(changeShapeSizeWindow, &ChangeShapeSize::signalChangeShapeSizeDataSubmit, this, [=](ChangeShapeSize_t shapeSize){
        canvasViewPattern->changeShapeSize(shapeSize);
    });
}

/* 打开文件时是否删除重合图形槽函数 */
void MainWindow::slotActionDeleteMultiGraphicTriggered(void)
{
    if(actionDeleteMultiGraphic->isChecked()) {
//        actionDelMultiGraphicsBetweenLayer->setCheckable(true);
        actionDelMultiGraphicsBetweenLayer->setEnabled(true);
        actionDelMultiGraphicsBetweenLayer->setChecked(true);
    } else {
        actionDelMultiGraphicsBetweenLayer->setChecked(false);
        actionDelMultiGraphicsBetweenLayer->setEnabled(false);
//        actionDelMultiGraphicsBetweenLayer->setCheckable(false);
    }
}

/* 封闭图形首尾点间距槽函数 */
void MainWindow::slotActionCloseGraphicTriggered(void)
{
    if (spacingOfClosedShapeWindow != nullptr) {
        delete spacingOfClosedShapeWindow;
        spacingOfClosedShapeWindow = nullptr;
    }
    spacingOfClosedShapeWindow = new HeadToTailSpacingOfClosedShape();

    //阻塞除当前窗体之外的所有的窗体
    spacingOfClosedShapeWindow->setWindowModality(Qt::ApplicationModal);
    spacingOfClosedShapeWindow->show();

    connect(spacingOfClosedShapeWindow, &HeadToTailSpacingOfClosedShape::signalSpacingOfClosedShapeDataSubmit, this, [=](double XOffset, double YOffset){
       canvasViewPattern->setClosedParam(XOffset, YOffset);
    });
}

/* 语言选择中文槽函数 */
void MainWindow::slotActionChineseTriggered(void)
{
    languageChange(":/language/language/tr_zh.qm");
    languageIniSetting(1);
    languageChosenQss(1);
    if(warningChinese != nullptr){
        delete warningChinese;
        warningChinese = nullptr;
    }
    warningChinese = new WarningBox(QString("设置成功，重新启动软件使其生效！"));
    warningChinese->setWindowModality(Qt::ApplicationModal);
    warningChinese->show();

    //重启
    connect(warningChinese, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择英文槽函数 */
void MainWindow::slotActionEnglishTriggered(void)
{
    languageChange(":/language/language/tr_en.qm");
    languageIniSetting(2);
    languageChosenQss(2);
    if(warningEnglish != nullptr){
        delete warningEnglish;
        warningEnglish = nullptr;
    }
    warningEnglish = new WarningBox("Set up successfully, restart the \n software to make it valid!");
    warningEnglish->setWindowModality(Qt::ApplicationModal);
    warningEnglish->show();

    //重启
    connect(warningEnglish, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择越南槽函数 */
void MainWindow::slotActionVietnamTriggered(void)
{
    languageChange(":/language/language/tr_vi.qm");
    languageIniSetting(3);
    languageChosenQss(3);
    if(warningVietnam != nullptr){
        delete warningVietnam;
        warningVietnam = nullptr;
    }
    warningVietnam = new WarningBox("Thiết lập thành công, khởi động \n lại phần mềm để phần mềm hợp lệ!");
    warningVietnam->setWindowModality(Qt::ApplicationModal);
    warningVietnam->show();

    //重启
    connect(warningVietnam, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择意大利语槽函数 */
void MainWindow::slotActionItalianTriggered(void)
{
    languageChange(":/language/language/tr_it.qm");
    languageIniSetting(4);
    languageChosenQss(4);
    if(warningItalian != nullptr){
        delete warningItalian;
        warningItalian = nullptr;
    }
    warningItalian = new WarningBox("Impostato correttamente, riavvia il \n software per renderlo valido!");
    warningItalian->setWindowModality(Qt::ApplicationModal);
    warningItalian->show();

    //重启
    connect(warningItalian, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择土耳其语槽函数 */
void MainWindow::slotActionTurkishTriggered(void)
{
    languageChange(":/language/language/tr_tr.qm");
    languageIniSetting(5);
    languageChosenQss(5);
    if(warningTurkish != nullptr){
        delete warningTurkish;
        warningTurkish = nullptr;
    }
    warningTurkish = new WarningBox("Başarıyla kurun, geçerli kılmak için \n yazılımı yeniden başlatın!");
    warningTurkish->setWindowModality(Qt::ApplicationModal);
    warningTurkish->show();

    //重启
    connect(warningTurkish, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择西班牙语槽函数 */
void MainWindow::slotActionSpanishTriggered(void)
{
    languageChange(":/language/language/tr_es.qm");
    languageIniSetting(6);
    languageChosenQss(6);
    if(warningSpanish != nullptr){
        delete warningSpanish;
        warningSpanish = nullptr;
    }
    warningSpanish = new WarningBox("¡Configure correctamente, reinicie el \n software para que sea válido!");
    warningSpanish->setWindowModality(Qt::ApplicationModal);
    warningSpanish->show();

    //重启
    connect(warningSpanish, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择俄语槽函数 */
void MainWindow::slotActionRussianTriggered(void)
{
    languageChange(":/language/language/tr_ru.qm");
    languageIniSetting(7);
    languageChosenQss(7);
    if(warningRussian != nullptr){
        delete warningRussian;
        warningRussian = nullptr;
    }
    warningRussian = new WarningBox("Настройте успешно, перезапустите \n программное обеспечение, чтобы \n оно стало действительным!");
    warningRussian->setWindowModality(Qt::ApplicationModal);
    warningRussian->show();

    //重启
    connect(warningRussian, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 语言选择日本语槽函数 */
void MainWindow::slotActionJapaneseTriggered(void)
{
    languageChange(":/language/language/tr_ja.qm");
    languageIniSetting(8);
    languageChosenQss(8);
    if(warningJapanese != nullptr){
        delete warningJapanese;
        warningJapanese = nullptr;
    }
    warningJapanese = new WarningBox("正常にセットアップし、ソフトウェア \n を再起動して有効にします。");
    warningJapanese->setWindowModality(Qt::ApplicationModal);
    warningJapanese->show();

    //重启
    connect(warningJapanese, &WarningBox::signalSubmitOK, [=](){
        qApp->exit(773);
    });
}

/* 变图层按钮槽函数 */
void MainWindow::slotBtnChangeLayerClicked(void)
{
    bool multiSelect = false, clickSelect = false;

    if (openedfileType == "null") {
        return;
    }

//    if (openedfileType == "ntp") {
//        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("已转换为缝纫点，不可变图层！"));
//        return;
//    }

    //从canvasViewPattern处获取当前选中的图形
    //单击选中
    QPoint selectedItem;
    selectedItem = canvasViewPattern->getCurrentSelectedItem();
    if (!(selectedItem.x() == 0 && selectedItem.y() == 0)) {
        clickSelect = true;
    }

    //CTRL + A多选
    const QList< QList<int> > &currentSelectedItems =
                    canvasViewPattern->getCurrentSelectedItemsList();
    if (currentSelectedItems.isEmpty() == false) {
        multiSelect = true;
    }

    //没有选中任何图形
    if (multiSelect == false && clickSelect == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请先选中图形！"));
        return;
    }

    //缝纫点图层不可再变图层
    const QByteArray &layerOutputStitch = canvasViewPattern->getIsLayerOutputStitch();
    if (layerOutputStitch[selectedItem.x()] == 't') {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("已转换为缝纫点，不可变图层！"));
        return;
    }

    if (changeLayerWindow != nullptr) {
        delete changeLayerWindow;
        changeLayerWindow = nullptr;
    }
    int layerCount = canvasViewPattern->getLayerCount();
    changeLayerWindow = new CChangeLayer(layerCount);

    //阻塞除当前窗体之外的所有的窗体
    changeLayerWindow->setWindowModality(Qt::ApplicationModal);
    changeLayerWindow->show();

    connect(changeLayerWindow, &CChangeLayer::signalChangeLayerOKSubmit, this, [=](int chosenLayer) {
        if (layerOutputStitch[chosenLayer] == 't') {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("缝纫点图层与图形图层之间不能相互转换！"));
            return;
        }
        canvasViewPattern->changeLayer(chosenLayer);
    });
}

/* 当前图复制按钮槽函数 */
void MainWindow::slotBtnCurrentGraphicCopyClicked(void)
{
    bool multiSelect = false, clickSelect = false;

    if (openedfileType == "null") {
        return;
    }

//    if (openedfileType == "ntp") {
//        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不是图形！"));
//        return;
//    }

    //从canvasViewPattern处获取当前选中的图形
    //单击选中
    QPoint selectedItem;
    selectedItem = canvasViewPattern->getCurrentSelectedItem();
    if (!(selectedItem.x() == 0 && selectedItem.y() == 0)) {
        clickSelect = true;
    }

    //CTRL + A多选
    const QList< QList<int> > &currentSelectedItems =
                    canvasViewPattern->getCurrentSelectedItemsList();
    if (currentSelectedItems.isEmpty() == false) {
        multiSelect = true;
    }

    //没有选中任何图形
    if (multiSelect == false && clickSelect == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请先选中图形！"));
        return;
    }

    //缝纫点图层不可再复制
    const QByteArray &layerOutputStitch = canvasViewPattern->getIsLayerOutputStitch();
    if (layerOutputStitch[selectedItem.x()] == 't') {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不是图形！"));
        return;
    }

    if (shapeCloneWindow != nullptr) {
        delete shapeCloneWindow;
        shapeCloneWindow = nullptr;
    }
    shapeCloneWindow = new CShapeClone(screenXSize, screenYSize);

    //阻塞除当前窗体之外的所有的窗体
    shapeCloneWindow->setWindowModality(Qt::ApplicationModal);
    shapeCloneWindow->show();

    connect(shapeCloneWindow, &CShapeClone::signalShapeCloneOKSubmit, this, [=](ShapeCloneData_t shapeCloneData) {
        canvasViewPattern->copyItem(shapeCloneData);
    });
}

/* 当前图修改按钮槽函数 */
void MainWindow::slotBtnCurrentGraphicModifyClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

//    if (openedfileType == "ntp") {
//        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不是图形！"));
//        return;
//    }

    //从canvasViewPattern处获取当前选中的图形
    //多选情况(只支持一个)
    const QList< QList<int> > &currentSelectedItems =
                    canvasViewPattern->getCurrentSelectedItemsList();
    QPoint selectedItem;
    selectedItem = canvasViewPattern->getCurrentSelectedItem();
    if (selectedItem.x() == 0 && selectedItem.y() == 0 && currentSelectedItems.size() == 0) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请先选中一个图形！"));
        return;
    }

    if (currentSelectedItems.size() > 1) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("当前图修改只支持修改单个图形！"));
        return;
    }

    if (currentSelectedItems.size() == 1) {
        selectedItem.setX(currentSelectedItems[0].at(0));
        selectedItem.setY(currentSelectedItems[0].at(1));
    }

    //缝纫点图层不可再修改
    const QByteArray &layerOutputStitch = canvasViewPattern->getIsLayerOutputStitch();
    if (layerOutputStitch[selectedItem.x()] == 't') {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不是图形！"));
        return;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    int unitType;
    interface->ArmGetUintTypeByIndex(selectedItem.x() - 1, selectedItem.y() - 1, &unitType);
    if (unitType == stCenterCircle || unitType == stPtCircle || unitType == stCurve ||
        unitType == stEllipse || unitType == stArc) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不支持修改此种图形！"));
        return;
    }

    if (editShapeWindow != nullptr) {
        delete editShapeWindow;
        editShapeWindow = nullptr;
    }
    this->canvasViewPattern->saveGraphicStateNoChangeStack();
    editShapeWindow = new CEditShape(screenXSize, screenYSize, selectedItem, interface);

    //阻塞除当前窗体之外的所有的窗体
    editShapeWindow->setWindowModality(Qt::ApplicationModal);
    editShapeWindow->show();

    connect(editShapeWindow, &CEditShape::signalBtnExitClicked, this, [=](bool hasChanged) {
        if(hasChanged == false) {
            this->canvasViewPattern->restorePreviousPatternState();
            this->canvasViewPattern->drawGraphicStitchFile(QPoint(selectedItem.x(), 1), true);
            this->canvasViewPattern->clearGraphicStateNoChangeStack();
        } else {
            this->canvasViewPattern->pushGraphicState();
            this->canvasViewPattern->drawGraphicStitchFile(QPoint(selectedItem.x(), 1), true);
        }
    });
}

/* 批量处理按钮槽函数 */
void MainWindow::slotBtnBatchProcessingClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    int layerCount, selectedLayer;
    interface->ArmGetLayerCount(&layerCount);

    if (layerCount == 1) {
        selectedLayer = 1;
    } else {
        //从canvasViewPattern处获取当前选中的图层
        const QList< QList<int> > &currentSelectedItems =
                        canvasViewPattern->getCurrentSelectedItemsList();
        QPoint selectedItem = canvasViewPattern->getCurrentSelectedItem();
        if (selectedItem.x() == 0 && currentSelectedItems.size() == 0) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请选中图层！"));
            return;
        }

        if (currentSelectedItems.size() > 0) {
            selectedLayer = currentSelectedItems.at(0).at(0);
        } else {
            selectedLayer = selectedItem.x();
        }
    }

    //缝纫点图层不可再修改
    const QByteArray &layerOutputStitch = canvasViewPattern->getIsLayerOutputStitch();
    if (layerOutputStitch[selectedLayer] == 't') {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("该图层不是图形！"));
        return;
    }

    if (massOperatingWindow != nullptr) {
        massOperatingWindow->deleteWindow();
        delete massOperatingWindow;
        massOperatingWindow = nullptr;
    }

    this->canvasViewPattern->saveGraphicStateNoChangeStack();

    massOperatingWindow = new CMassOperating(interface, selectedLayer, openedfileType, 0);

    //阻塞除当前窗体之外的所有的窗体
    massOperatingWindow->setWindowModality(Qt::ApplicationModal);
    massOperatingWindow->show();

    connect(massOperatingWindow, &CMassOperating::signalMassOperatingExit, [=](bool isInterfaceChanged){
        if(isInterfaceChanged == false) {
            this->canvasViewPattern->clearGraphicStateNoChangeStack();
        } else {
            this->canvasViewPattern->pushGraphicState();
        }
        int tmpLayer, layer;
        interface->ArmGetLayerCount(&tmpLayer);
        if (tmpLayer < selectedLayer) {
            layer = tmpLayer;
        } else {
            layer = selectedLayer;
        }
        int unitCount = interface->ArmGetUintCount(0);
        if (unitCount != 0) {
            this->canvasViewPattern->drawGraphicStitchFileWithSelect(QPoint(layer, 1), true);
        } else {
            if (isInterfaceChanged == true) {
                canvasViewPattern->clearScene();
                tabWidgetGraphic->clearContent();
                tabWidgetLayer->clearContent();
                canvasViewPattern->scene()->update();
            }
        }
    });
}

/* 当前图层设置槽函数 */
void MainWindow::slotBtnCurrentLayerSettingClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

//    if (openedfileType == "ntp") {
//        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("已转换为缝纫点，不可再转换！"));
//        return;
//    }

    if (canvasViewPattern->getDisplayMode() == 1) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("显示点模式下不可设置当前图层！"));
        return;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    int layerCount, settingLayer;
    interface->ArmGetLayerCount(&layerCount);

    const QByteArray &layerOutputStitch = canvasViewPattern->getIsLayerOutputStitch();
    if (layerCount == 1) {
        if (layerOutputStitch[1] != 't') {
            settingLayer = 1;
        } else {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("已转换为缝纫点，不可再转换！"));
            return;
        }
    } else {

        bool multiSelect = false, clickSelect = false;

        //CTRL + A多选情况下, 判断所有图元是否都在同一个图层
        const QList< QList<int> > &currentSelectedItems =
                            canvasViewPattern->getCurrentSelectedItemsList();
        if (currentSelectedItems.size() != 0) {
            for (int i = 0; i < currentSelectedItems.size() - 1; i++) {
                QList<int> itemTmpCur = currentSelectedItems.at(i);
                QList<int> itemTmpNext = currentSelectedItems.at(i + 1);
                if (itemTmpCur.at(0) != itemTmpNext.at(0)) {
                    QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("选中的多个图形不在同一个图层！"));
                    return;
                }
            }

            //标记多选
            multiSelect = true;
            settingLayer = currentSelectedItems.at(0).at(0);
        }

        //判断是否多选了缝纫点
        const QList< QList<int> > &currentSelectedStitches =
                            canvasViewPattern->getCurrentSelectedStitchesList();
        if (currentSelectedStitches.size() != 0) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请选中图形以确定生成缝纫点的图层！"));
            return;
        }

        //从canvasViewPattern处获取当前选中的图形
        QPoint selectedItem;
        selectedItem = canvasViewPattern->getCurrentSelectedItem();
        if (!(selectedItem.x() == 0 && selectedItem.y() == 0)) {

            //标记单选
            clickSelect = true;
            settingLayer = selectedItem.x();
        }

        if (clickSelect == false && multiSelect == false) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请选中图形以确定生成缝纫点的图层！"));
            return;
        }

        //缝纫点图层不可再设置
        if (layerOutputStitch[selectedItem.x()] == 't') {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("已转换为缝纫点，不可再转换！"));
            return;
        }
    }

    if (layerParameterSetWindow != nullptr) {
        delete layerParameterSetWindow;
        layerParameterSetWindow = nullptr;
    }
    layerParameterSetWindow = new CLayerParameterSet(screenXSize, screenYSize);

    //阻塞除当前窗体之外的所有的窗体
    layerParameterSetWindow->setWindowModality(Qt::ApplicationModal);
    layerParameterSetWindow->show();

    connect(layerParameterSetWindow,
            &CLayerParameterSet::signalLayerParameterSetOKSubmit,
            this, [=](const LayerParameterSetData_t &layerParameterSetData) {
        if (canvasViewPattern->getHasNoBenchmark() == false ||
                (canvasViewPattern->getHasNoBenchmark() == true && settingLayer != 1)) {
            canvasViewPattern->generateLayerStitchPoint(settingLayer, layerParameterSetData);
            canvasViewPattern->displayNormal();
        }
    });
}

/* 设为基准点按钮槽函数 */
void MainWindow::slotBtnSetToDatumPointClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取当前选中的缝纫点
    QList<int> currentSelectedStitch = canvasViewPattern->getSelectedStitch();
    if (currentSelectedStitch.size() == 0) {
        QMessageBox mymessage(QMessageBox::Warning, tr("SewMake数控编制软件"), tr("不存在缝纫点或未选中缝纫点图层！"));
        mymessage.setStandardButtons(QMessageBox::Ok);
        mymessage.setButtonText(QMessageBox::Ok, tr("确 定"));
        mymessage.exec();
        return;
    }

    if (setBasePointWindow != nullptr) {
        delete setBasePointWindow;
        setBasePointWindow = nullptr;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    //选中基准点位置
    qreal posX, posY;
    int layer = currentSelectedStitch.at(0) - 1;
    int itemId = currentSelectedStitch.at(1) - 1;
    int stitchId = currentSelectedStitch.at(2) -1;
    interface->ArmGetStitchPoint(layer, itemId, stitchId, &posX, &posY);

    //获取基准点
    qreal x1,y1,x2,y2;
    int pitchNum1,pitchNum2;
    int iSize = interface->ArmGetPitchBasePoint(pitchNum1, pitchNum2, x1, y1, x2, y2);

    //记录选中缝纫点是否已被设置为基准点
    int flag;
    if (iSize != 0 && fabs(x1 - posX) < 0.0001 && fabs(y1 - posY) < 0.0001) {  //该点已为基准点1
        flag = 1;
    } else if (fabs(x2 - posX) < 0.0001 && fabs(y2 - posY) < 0.0001) {  //该点已为基准点2
        flag = 2;
    } else {  //该点还未设置
        flag = 0;
    }

    setBasePointWindow = new SetBasePoint(flag);

    //阻塞除当前窗体之外的所有的窗体
    setBasePointWindow->setWindowModality(Qt::ApplicationModal);
    setBasePointWindow->show();

    connect(setBasePointWindow, &SetBasePoint::signalBasePointSubmit, this, [=](int chosen) {
        canvasViewPattern->setBasePoint(chosen);
    });
}

/* 工具按钮槽函数 */
void MainWindow::slotBtnToolsClicked(void)
{

}

/* 作图按钮槽函数 */
void MainWindow::slotBtnPaintClicked(void)
{
    if (designWindow != nullptr) {
        delete designWindow;
        designWindow = nullptr;
    }

    //获取接口
    SewDataInterface *&interface = canvasViewPattern->getInterface();
    this->canvasViewPattern->saveGraphicStateNoChangeStack();
    designWindow = new DesignWindow(interface, this->openedfileType, this->openedfilePath, this->font);

    //阻塞除当前窗体之外的所有的窗体
    designWindow->setWindowModality(Qt::ApplicationModal);
    designWindow->show();

    connect(designWindow, &DesignWindow::signalBtnExitClicked, this, [=](bool hasChanged, QString type, QString path) {
        if(hasChanged == false) {
            this->canvasViewPattern->restorePreviousPatternState();
            this->canvasViewPattern->drawGraphicStitchFileWithSelect(QPoint(1, 1), true);
            this->canvasViewPattern->clearGraphicStateNoChangeStack();
        } else {
            this->openedfileType = type;
            this->openedfilePath = path;
            this->canvasViewPattern->setOpenedFileType(type);
            this->canvasViewPattern->setOpenedFilePath(path);
            this->canvasViewPattern->pushGraphicState();
            this->canvasViewPattern->drawGraphicStitchFileWithSelect(QPoint(1, 1), true);

            //使能主界面显示模式radioBtn选择按钮
            this->setBottomDotLineRadioBtnEnable(true, true, true);
        }
    });
}

/* 撤销按钮槽函数 */
void MainWindow::slotBtnUndoClicked(void)
{
    canvasViewPattern->undo();
}

/* 重做按钮槽函数 */
void MainWindow::slotBtnRedoClicked(void)
{
    canvasViewPattern->redo();
}

/* 模拟缝纫按钮槽函数 */
void MainWindow::slotBtnSimSewingClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    SewDataInterface *&interface = canvasViewPattern->getInterface();
    if (sewingSimulationWindow != nullptr) {
        delete sewingSimulationWindow;
        sewingSimulationWindow = nullptr;
    }
    canvasViewPattern->clearAllSelection();
    QRectF rectStitches = canvasViewPattern->getCanvasSceneBoundingRect();
    sewingSimulationWindow = new CSewingSimulation(screenXSize, screenYSize, interface, rectStitches);
    //阻塞除当前窗体之外的所有的窗体
    sewingSimulationWindow->setWindowModality(Qt::ApplicationModal);
    sewingSimulationWindow->show();
}

/* 特殊缝按钮槽函数 */
void MainWindow::slotBtnSpecialSewingClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    const QList< QList<int> > &currentSelectedStitches
            = canvasViewPattern->getCurrentSelectedStitchesList();

    if (currentSelectedStitches.size() <= 1) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请选中多个缝纫点！"));
        return;
    }

    //检查框选的缝纫点是否属于同一个图形
    int itemLayer = currentSelectedStitches[0].at(0);
//    int itemId = currentSelectedStitches[0].at(1);
    for (int i = 1; i < currentSelectedStitches.size(); i++) {
        int itemLayerTmp = currentSelectedStitches[i].at(0);
//        int itemIdTmp = currentSelectedStitches[i].at(1);

        //如果图层都不相同, 那必然不属于同一个图形
        if (itemLayer != itemLayerTmp) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不支持选中多个图形上的缝纫点！"));
            return;
        }
//        else {
//            if (itemId != itemIdTmp) {
//                QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("不支持选中多个图形上的缝纫点！"));
//                return;
//            }
//        }
    }

    if (specialSewingWindow != nullptr) {
        delete specialSewingWindow;
        specialSewingWindow = nullptr;
    }
    specialSewingWindow = new CSpecialSewing(screenXSize, screenYSize);

    //阻塞除当前窗体之外的所有的窗体
    specialSewingWindow->setWindowModality(Qt::ApplicationModal);
    specialSewingWindow->show();

    //特殊缝纫
    connect(specialSewingWindow, &CSpecialSewing::signalSpecialSewingDataSubmit, this, [=](const SpecialSewingData_t &specialSewingData) {
        canvasViewPattern->generateSpecialSewingAttribute(specialSewingData);
    });

    //摆针曲折
    connect(specialSewingWindow, &CSpecialSewing::signalZigzagNeedleDataSubmit, this, [=](const ZigzagNeedleData_t &zigzagNeedleData) {
        canvasViewPattern->generateZigzagAttribute(zigzagNeedleData);
    });
}

/* 缝纫点代码按钮槽函数 */
void MainWindow::slotBtnSewingPointCodeClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    ItemStitchPoint *itemStitchPoint;
    bool boxSelect = false, clickSelect = false;

    const QList< QList<int> > &currentSelectedStitches
            = canvasViewPattern->getCurrentSelectedStitchesList();

    //框选不支持多个
    if (currentSelectedStitches.size() > 1) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("缝纫点代码不支持同时设置多个缝纫点！"));
        return;
    }

    //框选1个
    if (currentSelectedStitches.isEmpty() == false &&
        currentSelectedStitches.size() == 1) {
        boxSelect = true;

        int layer = currentSelectedStitches.at(0).at(0);
        int id = currentSelectedStitches.at(0).at(3);

        eItem_Type_t itemType;
        itemStitchPoint = dynamic_cast<ItemStitchPoint *>(canvasViewPattern->getItem(layer, id, &itemType));
    }

    //从canvasViewPattern处获取当前选中的图形
    QPoint selectedItem;
    selectedItem = canvasViewPattern->getCurrentSelectedItem();
    if (!(selectedItem.x() == 0 && selectedItem.y() == 0)) {
        eItem_Type_t itemType;

        itemStitchPoint = dynamic_cast<ItemStitchPoint *>(canvasViewPattern->getItem(selectedItem.x(), selectedItem.y(), &itemType));
        if (itemType == eITEM_STITCH_POINT) {
            clickSelect = true;
        }
    }

    //如果没有选中任何缝纫点
    if (boxSelect == false && clickSelect == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("请先选中缝纫点！"));
        return;
    }

    if (sewingPointCodeWindow != nullptr) {
        delete sewingPointCodeWindow;
        sewingPointCodeWindow = nullptr;
    }

    sewingPointCodeWindow = new CSewingPointCode(screenXSize, screenYSize, itemStitchPoint, canvasViewPattern);
    connect(sewingPointCodeWindow, &CSewingPointCode::signalControlCodeSubmit, [=](Control_Code_t *controlCodeSelected, int basePointOp) {
        canvasViewPattern->setStitchControlCode(controlCodeSelected);
        switch (basePointOp) {
        case 1: canvasViewPattern->setBasePoint(1);
            break;
        case 2: canvasViewPattern->setBasePoint(2);
            break;
        case 3: canvasViewPattern->setBasePoint(3);
            break;
        default: break;
        }
    });

    //阻塞除当前窗体之外的所有的窗体
    sewingPointCodeWindow->setWindowModality(Qt::ApplicationModal);
    sewingPointCodeWindow->show();
}

/* 显示图层还是显示整图按钮槽函数 */
void MainWindow::slotBottomRadioBtnGraphicLayerClicked(void)
{
    if (radioBtnDispLayer->isChecked() == true) {

        this->showAllLayer = false;
        if (openedfileType != "null") {
            canvasViewPattern->setShowAllLayerRadioBtn(false);
        }

    } else if (radioBtnDispWholePic->isChecked() == true) {

        this->showAllLayer = true;
        if (openedfileType != "null") {
            canvasViewPattern->setShowAllLayerRadioBtn(true);
        }
    }
}

/* 显示点还是显示线按钮槽函数 */
void MainWindow::slotBottomRadioBtnDotLineNormalClicked(void)
{
    if (radioBtnDispDot->isChecked() == true) {
        if (openedfileType != "null") {
            canvasViewPattern->displayDot();
        }
    }
    else if (radioBtnDispLine->isChecked() == true) {
        if (openedfileType != "null") {
            canvasViewPattern->displayLine();
        }
    }
    else if (radioBtnDispNormal->isChecked() == true) {
        if (openedfileType != "null") {
            canvasViewPattern->displayNormal();
        }
    }
}

/* 显示图形序号复选框按钮槽函数 */
void MainWindow::slotChkBoxDispSerialNumberClicked(void)
{
    if (chkBoxDispSerialNumber->checkState() == Qt::Checked) {
        isShowSerialNumber = true;
        canvasViewPattern->setShowItemsId(true);
    } else {
        isShowSerialNumber = false;
        canvasViewPattern->setShowItemsId(false);
    }
}

/* 打开文件按钮槽函数 */
void MainWindow::slotBtnOpenFileClicked(void)
{
    QString filePathName =
            QFileDialog::getOpenFileName(this, tr("打开文件"), this->lastOpenedFilePath,
           "*.plt;;*.ntp;;*.dxf;;*.sdf;;*.ai;;*.*");

    if (filePathName.isEmpty() == true) {
        return;
    }
    chkBoxNoBenchmark->setChecked(false);

    //保存本次打开的文件路径
    QFileInfo fileInfo(filePathName);
    this->lastOpenedFilePath = fileInfo.path();

    //绘制文件
    this->drawGraphicFromFile(filePathName);
}

/* 另存为按钮槽函数 */
void MainWindow::slotBtnSaveAsClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    QString saveAsFilePath;
    QString selectedNameFilter;
    saveAsFilePath = QFileDialog::getSaveFileName(this, tr("另存为"),
                   this->lastSaveAsFilePath, tr("*.ntp;;*.sdf;;*.dxf"), &selectedNameFilter);
    if (saveAsFilePath.isEmpty() == true) {
        return;
    }

    //保存本次另存为的文件路径
    QFileInfo fileInfo(saveAsFilePath);
    this->lastSaveAsFilePath = fileInfo.path();

    if (selectedNameFilter == "*.ntp") {

        QFileInfo fileinfo = QFileInfo(saveAsFilePath);   //获取后缀名
        QString suffix = fileinfo.suffix().toLower();
        if (suffix.isEmpty() == true || suffix != "ntp") {
            saveAsFilePath.append(".ntp");
        }
        if (canvasViewPattern->saveFileToDisk("ntp", saveAsFilePath) != 0) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("ntp文件保存失败！"));
        } else {
            QMessageBox::information(this, tr("SewMake数控编制软件"), tr("ntp文件保存成功！"),
                                             QMessageBox::Yes, QMessageBox::Yes);
        }

    } else if (selectedNameFilter == "*.sdf") {

        QFileInfo fileinfo = QFileInfo(saveAsFilePath);   //获取后缀名
        QString suffix = fileinfo.suffix().toLower();
        if (suffix.isEmpty() == true || suffix != "sdf") {
            saveAsFilePath.append(".sdf");
        }
        if (canvasViewPattern->saveFileToDisk("sdf", saveAsFilePath) != 0) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("sdf文件保存失败！"));
        } else {
            QMessageBox::information(this, tr("SewMake数控编制软件"), tr("sdf文件保存成功！"),
                                             QMessageBox::Yes, QMessageBox::Yes);
        }
    } else if (selectedNameFilter == "*.dxf") {

        QFileInfo fileinfo = QFileInfo(saveAsFilePath);   //获取后缀名
        QString suffix = fileinfo.suffix().toLower();
        if (suffix.isEmpty() == true || suffix != "dxf") {
            saveAsFilePath.append(".dxf");
        }
        if (canvasViewPattern->saveFileToDisk("dxf", saveAsFilePath) != 0) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("dxf文件保存失败！"));
        } else {
            QMessageBox::information(this, tr("SewMake数控编制软件"), tr("dxf文件保存成功！"),
                                             QMessageBox::Yes, QMessageBox::Yes);
        }
    }
}

/* 放大槽函数 */
void MainWindow::slotBtnZoomUpClicked(void)
{
    canvasViewPattern->setGlobalStepScale(true);
}

/* 缩小槽函数 */
void MainWindow::slotBtnZoomDownClicked(void)
{
    canvasViewPattern->setGlobalStepScale(false);
}

/* 显示恢复槽函数 */
void MainWindow::slotBtnDispRestoreClicked(void)
{
    canvasViewPattern->setGlobalSelfAdaption();
}

/* 图形拖动槽函数 */
void MainWindow::slotBtnPicDragClicked(void)
{
    canvasViewPattern->setMainDrag(true);
}

/* 正常操作槽函数 */
void MainWindow::slotBtnNormalOptClicked(void)
{
    canvasViewPattern->setMainDrag(false);
}

/* 帮助按钮槽函数 */
void MainWindow::slotBtnHelpClicked(void)
{
    if (helpWindow != nullptr) {
        delete helpWindow;
        helpWindow = nullptr;
    }
    QString infoString = tr("软件版本: ");
    infoString += tr(PANEL_BOTTOM_VERSION_LABEL);
    infoString += tr("\n\n如有使用问题请反馈market@sunristec.com");
    helpWindow = new HelpWindow(infoString, 360, 180);
    helpWindow->setWindowModality(Qt::ApplicationModal);
    helpWindow->show();
}

/* 软件升级按钮槽函数 */
void MainWindow::slotBtnUpdateClicked(bool bAutoUpgrade)
{
    if (upgradeStatus != UPGRADE_STATUS_NONE) {
        return;
    }

    //启动更新进程
    QStringList argStrList = QString(PANEL_BOTTOM_VERSION_LABEL).split(" ");
    QString argStr = argStrList[0].append(" ");
    argStr.append(argStrList[1]);

    QStringList arguments;
    arguments << argStr;

    if (bAutoUpgrade == true && bUseHttpHtml == false) {
        arguments << QString(tr("10"));                             //10表示自动升级,使用http网页版本
    }
    else if (bAutoUpgrade == true && bUseHttpHtml == true) {
        arguments << QString(tr("11"));                             //11表示自动升级,使用https网页版本
    }
    else if (bAutoUpgrade == false && bUseHttpHtml == true) {
        arguments << QString(tr("01"));                             //01表示不自动升级,使用https网页版本
    }
    else if (bAutoUpgrade == false && bUseHttpHtml == false) {
        arguments << QString(tr("00"));                             //00表示不自动升级,使用http网页版本
    }

    processUpgrade = new QProcess(nullptr);
    connect(processUpgrade, &QProcess::readyReadStandardOutput, [=]() {

        QByteArray output = processUpgrade->readAllStandardOutput();
        QString cmdStr = QString(output);

        //升级软件请求关闭Mpp软件
        if (cmdStr == "request close") {
            upgradeStatus = UPGRADE_STATUS_CLOSE;
            this->close();
        }

        //升级取消
        if (cmdStr == "upgrade cancel") {
            upgradeStatus = UPGRADE_STATUS_NONE;
            delete processUpgrade;
            processUpgrade = nullptr;
        }
    });

    upgradeStatus = UPGRADE_STATUS_START;
    processUpgrade->start("./Upgrade/Upgrade.exe", arguments);
}

/* 缩放QComboBox某行被使能槽函数 */
void MainWindow::slotComboBoxZoomActivated(int index)
{
    if (openedfileType == "null") {
        return;
    }

    QString text = comboBoxZoom->itemText(index);
    QString textNum;
    textNum.clear();

    for (int i = 0; text.at(i) != '%'; i++) {
        textNum.append(text.at(i));
    }

    int scalingRatio = textNum.toInt();
    canvasViewPattern->setGlobalScale(scalingRatio);
}

/* 左确定按钮槽函数 */
void MainWindow::slotBtnConfirmLeftClicked(void)
{
    if (openedfileType != "null") {
        bool ok;
        int num = lineEditLeft->text().toInt(&ok);
        if (ok == false) {
            QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
            return;
        }

        canvasViewPattern->changeCurrentLayerOrderNum(num);
    }
}

/* 左上移按钮槽函数 */
void MainWindow::slotBtnMoveUpLeftClicked(void)
{
    if (openedfileType != "null") {
        canvasViewPattern->changeCurrentLayerOrderUp();
    }
}

/* 左下移按钮槽函数 */
void MainWindow::slotBtnMoveDownLeftClicked(void)
{
    if (openedfileType != "null") {
        canvasViewPattern->changeCurrentLayerOrderDown();
    }
}

/* 右确定按钮槽函数 */
void MainWindow::slotBtnConfirmRightClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    bool ok;
    int num = lineEditRight->text().toInt(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->changeCurrentItemOrderNum(num);
}

/* 右上移按钮槽函数 */
void MainWindow::slotBtnMoveUpRightClicked(void)
{
    canvasViewPattern->changeCurrentItemOrderUp();
}

/* 右下移按钮槽函数 */
void MainWindow::slotBtnMoveDownRightClicked(void)
{
    canvasViewPattern->changeCurrentItemOrderDown();
}

/* 水平镜像按钮槽函数 */
void MainWindow::slotBtnHorizontalMirrorClicked(void)
{
    canvasViewPattern->setGlobalMirrorHorizontal();
}

/* 垂直镜像按钮槽函数 */
void MainWindow::slotBtnVerticalMirrorClicked(void)
{
    canvasViewPattern->setGlobalMirrorVertical();
}

/* 旋转按钮槽函数 */
void MainWindow::slotBtnRotateClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取旋转角度
    bool ok;
    qreal rotateValue = lineEditRotateValue->text().toDouble(&ok);
    if (ok == false) { //非法内容, 转换失败
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->setGlobalRotation(rotateValue);
}

/* 向上移动按钮槽函数 */
void MainWindow::slotBtnMoveUpClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取移动距离
    bool ok;
    qreal distance = lineEditMoveDistanceValue->text().toDouble(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->setItemMoveUp(distance);
}

/* 向下移动按钮槽函数 */
void MainWindow::slotBtnMoveDownClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取移动距离
    bool ok;
    qreal distance = lineEditMoveDistanceValue->text().toDouble(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->setItemMoveDown(distance);
}

/* 向左移动按钮槽函数 */
void MainWindow::slotBtnMoveLeftClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取移动距离
    bool ok;
    qreal distance = lineEditMoveDistanceValue->text().toDouble(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->setItemMoveLeft(distance);
}

/* 向右移动按钮槽函数 */
void MainWindow::slotBtnMoveRightClicked(void)
{
    if (openedfileType == "null") {
        return;
    }

    //获取移动距离
    bool ok;
    qreal distance = lineEditMoveDistanceValue->text().toDouble(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return;
    }
    canvasViewPattern->setItemMoveRight(distance);
}

/* 删除按钮槽函数 */
void MainWindow::slotBtnDeleteClicked(void)
{
    canvasViewPattern->setItemDelete();
}

/* 语言初始化 */
void MainWindow::languageInit(void)
{
    QString iniPath = PARAM_CONFIG_FILE_DIR + "Language.ini";
    QSettings *configIni;
    QFileInfo file(iniPath);
    //判断语言配置文件是否存在，不存在则创建
    if(!file.isFile()) {
        configIni = new QSettings(iniPath, QSettings::IniFormat);
        configIni->setValue("/language_chosen/chosen", 1);
    } else {
        configIni = new QSettings(iniPath, QSettings::IniFormat);
    }

    //根据配置信息选择语言版本
    languageChosen = configIni->value("/language_chosen/chosen").toInt();
    switch (languageChosen) {
        case 1:
            languageChange(":/language/language/tr_zh.qm");
            break;
        case 2:
            languageChange(":/language/language/tr_en.qm");
            break;
        case 3:
            languageChange(":/language/language/tr_vi.qm");
            break;
        case 4:
            languageChange(":/language/language/tr_it.qm");
            break;
        case 5:
            languageChange(":/language/language/tr_tr.qm");
            break;
        case 6:
            languageChange(":/language/language/tr_es.qm");
            break;
        case 7:
            languageChange(":/language/language/tr_ru.qm");
            break;
        case 8:
            languageChange(":/language/language/tr_ja.qm");
            break;
        default:
            break;
    }
    delete configIni;
}

/* 语言选择 */
void MainWindow::languageSwitch(int chosen)
{
    switch (chosen) {
    case 1:
        languageChange(":/language/language/tr_zh.qm");
        languageChosenQss(1);
        break;
    case 2:
        languageChange(":/language/language/tr_en.qm");
        languageChosenQss(2);
        break;
    case 3:
        languageChange(":/language/language/tr_vi.qm");
        languageChosenQss(3);
        break;
    case 4:
        languageChange(":/language/language/tr_it.qm");
        languageChosenQss(4);
        break;
    case 5:
        languageChange(":/language/language/tr_tr.qm");
        languageChosenQss(5);
        break;
    case 6:
        languageChange(":/language/language/tr_es.qm");
        languageChosenQss(6);
        break;
    case 7:
        languageChange(":/language/language/tr_ru.qm");
        languageChosenQss(7);
        break;
    case 8:
        languageChange(":/language/language/tr_ja.qm");
        languageChosenQss(8);
        break;
    default:
        break;
    }
}

/* 语言配置文件设置 */
void MainWindow::languageIniSetting(int chosen)
{
    QString iniPath = PARAM_CONFIG_FILE_DIR + "Language.ini";
    QSettings *configIni = new QSettings(iniPath, QSettings::IniFormat);
    configIni->setValue("/language_chosen/chosen",chosen);
    delete configIni;
}

/* 语言选中样式变化 */
void MainWindow::languageChosenQss(int chosen)
{
    QLabel *languageLabel[8] = {
        labelChinese, labelEnglish, labelVietnam, labelItalian,
        labelTurkish, labelSpanish, labelRussian, labelJapanese};
    chosen--;
    for(int i = 0; i < 8; i++)
    {
        if(i == chosen){
            languageLabel[i]->setStyleSheet(MENU_BAR_LANGUAGE_ITEM_CHOSEN);     //设置选中样式
        }
        else
        {
            languageLabel[i]->setStyleSheet(MENU_BAR_LANGUAGE_ITEM);
            languageLabel[i]->setAttribute(Qt::WA_Hover, true);
        }
    }
}

/* 语言改变 */
void MainWindow::languageChange(QString chosen)
{
    if(translator != nullptr)
    {
        qApp->removeTranslator(translator);
        delete translator;
        translator = nullptr;
    }
    translator = new QTranslator();
    //加载对应语言版本
    translator->load(chosen);
    qApp->installTranslator(translator);
}

/* 获取移动距离 */
qreal MainWindow::getMoveDistance(void)
{
    bool ok;
    qreal distance = lineEditMoveDistanceValue->text().toDouble(&ok);
    if (ok == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("数值非法！"));
        return 0;
    }

    return distance;
}

/* 编辑框编辑完毕槽函数 */
void MainWindow::slotEditLineFinished(void)
{
    canvasViewPattern->setFocus();
}

/* 获取是否免基准 */
bool MainWindow::getNoBenchmark(void)
{
    return chkBoxNoBenchmark->isChecked();
}

/* 设置是否选中免基准 */
void MainWindow::setNoBenchmark(bool isSet)
{
    chkBoxNoBenchmark->setChecked(isSet);
}

/* 免基准复选框按钮槽函数 */
void MainWindow::slotChkBoxNoBenchmarkClicked(void)
{
    if (chkBoxNoBenchmark->isChecked() == true) {
        canvasViewPattern->noNoBenchmarkClicked(true);
    } else {
        canvasViewPattern->noNoBenchmarkClicked(false);
    }
}

/* 获取是否删除重复图形选项 */
bool MainWindow::getActionDeleteMultiGraphic(void)
{
    return actionDeleteMultiGraphic->isChecked();
}

/* 获取是否删除跨图层重复图形选项 */
bool MainWindow::getActionDelMultiGraphicsBetweenLayer(void)
{
    return actionDelMultiGraphicsBetweenLayer->isChecked();
}

/* 获取自动合并临近图形选项 */
bool MainWindow::getActionAutoMergeNearGraphic(void)
{
    return actionAutoMergeNearGraphic->isChecked();
}

/* 从磁盘上读取上一次设置的文件路径 */
void MainWindow::restorePathFromDisk(void)
{
    QFile file(PARAM_CONFIG_FILE_DIR + "pathSetting.config");
    char buffer[1024];

    if (file.exists() == true) {

        file.open((QIODevice::ReadOnly));
        //反序列化
        QTextCodec *codec = QTextCodec::codecForName("GBK");
        QByteArray ba = file.readAll();
        QString strAll = codec->toUnicode(ba);
//        QTextCodec *codec1 = QTextCodec::codecForName("Unicode");
//        QDataStream aStream(&file);
//        aStream.setByteOrder(QDataStream::LittleEndian);
//        aStream.readRawData(buffer, sizeof(buffer));
        file.close();

        lastOpenedFilePath.clear();
        lastSaveAsFilePath.clear();

        //先找打开文件路径
        int i;
        for (i = 0; strAll[i] != 0; i++) {
            lastOpenedFilePath.append(QChar(strAll[i]));
        }
        i++;
        //再找另存为文件路径
        for (; strAll[i] != 0; i++) {
            lastSaveAsFilePath.append(QChar(strAll[i]));
        }
    }
    //创建文件
    else {
        file.open((QIODevice::WriteOnly | QIODevice::Truncate));

        lastOpenedFilePath = ".";
        lastSaveAsFilePath = ".";

        QByteArray filePathOpen = lastOpenedFilePath.toLocal8Bit();
        const char *strOpenedFilePath = filePathOpen.data();
        QByteArray filePathSaveAs = lastSaveAsFilePath.toLocal8Bit();
        const char *strSaveAsFilePath = filePathSaveAs.data();

        int strOpenedFilePathLen = strlen(strOpenedFilePath);
        int strSaveAsFilePathLen = strlen(strSaveAsFilePath);

        //序列化
        QDataStream aStream(&file);
        aStream.setByteOrder(QDataStream::LittleEndian);
        strcpy(buffer, strOpenedFilePath);
        strcpy(buffer + strOpenedFilePathLen + 1, strSaveAsFilePath);

        aStream.writeRawData(buffer, strOpenedFilePathLen + strSaveAsFilePathLen + 2);
        file.close();
    }
}

/* 把本次设置的文件路径存放到磁盘上 */
void MainWindow::savePathToDisk(void)
{
    char buffer[1024];
    QFile file(PARAM_CONFIG_FILE_DIR + "pathSetting.config");
    file.open((QIODevice::WriteOnly | QIODevice::Truncate));

    QByteArray filePathOpen = lastOpenedFilePath.toLocal8Bit();
    const char *strOpenedFilePath = filePathOpen.data();
    QByteArray filePathSaveAs = lastSaveAsFilePath.toLocal8Bit();
    const char *strSaveAsFilePath = filePathSaveAs.data();

    int strOpenedFilePathLen = strlen(strOpenedFilePath);
    int strSaveAsFilePathLen = strlen(strSaveAsFilePath);

    //序列化
    QDataStream aStream(&file);
    aStream.setByteOrder(QDataStream::LittleEndian);
    strcpy(buffer, strOpenedFilePath);
    strcpy(buffer + strOpenedFilePathLen + 1, strSaveAsFilePath);

    aStream.writeRawData(buffer, strOpenedFilePathLen + strSaveAsFilePathLen + 2);
    file.close();
}

/* 从磁盘上读取上一次设置的主界面参数 */
void MainWindow::restoreMainWindowParam(void)
{
    QFile file(PARAM_CONFIG_FILE_DIR + "mainWindowSetting.config");
    char buffer[1024];

    if (file.exists() == true) {

        file.open((QIODevice::ReadOnly));
        //反序列化
        QDataStream aStream(&file);
        aStream.setByteOrder(QDataStream::LittleEndian);
        aStream.readRawData(buffer, sizeof(MainWindow_Param_t));
        memcpy(&lastMainWindowParam, buffer, sizeof(MainWindow_Param_t));

        file.close();
    }
    //创建文件
    else {
        file.open((QIODevice::WriteOnly | QIODevice::Truncate));

        lastMainWindowParam.option = 0x04;
        lastMainWindowParam.moveDistance = 1.0;
        lastMainWindowParam.rotationDegree = 90.00;

        //序列化
        QDataStream aStream(&file);
        aStream.setByteOrder(QDataStream::LittleEndian);
        memcpy(buffer, &lastMainWindowParam, sizeof(MainWindow_Param_t));
        aStream.writeRawData(buffer, sizeof(MainWindow_Param_t));

        file.close();
    }

    //把参数设置到主界面上
    if(lastMainWindowParam.option & 0x01) {
        actionDeleteMultiGraphic->setChecked(true);
    } else {
        actionDeleteMultiGraphic->setChecked(false);
        actionDelMultiGraphicsBetweenLayer->setEnabled(false);
    }
    (lastMainWindowParam.option & 0x02) ? actionAutoMergeNearGraphic->setChecked(true) : actionAutoMergeNearGraphic->setChecked(false);
    (lastMainWindowParam.option & 0x04) ? actionOpenNtp->setChecked(true) : actionOpenNtp->setChecked(false);
    (lastMainWindowParam.option & 0x08) ? actionDelMultiGraphicsBetweenLayer->setChecked(true) : actionDelMultiGraphicsBetweenLayer->setChecked(false);
    lineEditMoveDistanceValue->setText(tr("%1").arg(lastMainWindowParam.moveDistance));
    lineEditRotateValue->setText(tr("%1").arg(lastMainWindowParam.rotationDegree));
}

/* 把本次设置的主界面参数存放到磁盘上 */
void MainWindow::saveMainWindowParam(void)
{
    QFile file(PARAM_CONFIG_FILE_DIR + "mainWindowSetting.config");
    char buffer[1024];

    file.open((QIODevice::WriteOnly | QIODevice::Truncate));

    actionDeleteMultiGraphic->isChecked() == true ? lastMainWindowParam.option |= 0x01 :lastMainWindowParam.option &= ~(0x01);
    actionAutoMergeNearGraphic->isChecked() == true ? lastMainWindowParam.option |= 0x02 :lastMainWindowParam.option &= ~(0x02);
    actionOpenNtp->isChecked() == true ? lastMainWindowParam.option |= 0x04 :lastMainWindowParam.option &= ~(0x04);
    actionDelMultiGraphicsBetweenLayer->isChecked() == true ? lastMainWindowParam.option |= 0x08 :lastMainWindowParam.option &= ~(0x08);

    qreal valueTmp;
    bool ok;
    valueTmp = lineEditMoveDistanceValue->text().toDouble(&ok);
    ok == true ? lastMainWindowParam.moveDistance = valueTmp : lastMainWindowParam.moveDistance = 0;

    valueTmp = lineEditRotateValue->text().toDouble(&ok);
    ok == true ? lastMainWindowParam.rotationDegree = valueTmp : lastMainWindowParam.rotationDegree = 0;

    //序列化
    QDataStream aStream(&file);
    aStream.setByteOrder(QDataStream::LittleEndian);
    memcpy(buffer, &lastMainWindowParam, sizeof(MainWindow_Param_t));
    aStream.writeRawData(buffer, sizeof(MainWindow_Param_t));

    file.close();
}

/* 检查更新 */
bool MainWindow::checkNewVersion(void)
{
    QString versionFileUrl = getVersionFileUrl();
    //发送get请求
    QNetworkAccessManager *manager = new QNetworkAccessManager(nullptr);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(versionFileUrl)));

    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish
    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();
        QString versionRemoteStr = QString(replyData);
        if (versionRemoteStr.split(" ").at(0) != "Ver")
        {
            reply->close();
            delete manager;
            return false;
        }
        versionRemoteStr = versionRemoteStr.split(" ").at(0) + " " + versionRemoteStr.split(" ").at(1);

        QStringList argStrList = QString(PANEL_BOTTOM_VERSION_LABEL).split(" ");
        QString versionLocalStr = argStrList[0].append(" ");
        versionLocalStr.append(argStrList[1]);

        if (versionRemoteStr != versionLocalStr) {
            QMessageBox::StandardButton btnSelected = QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("发现新版本，是否更新？"), QMessageBox::Ok | QMessageBox::Cancel);
            switch (btnSelected) {
            case QMessageBox::Ok:
                slotBtnUpdateClicked(true);
                break;
            default: break;
            }
        }
    }

    reply->close();
    delete manager;
    return true;
}

bool MainWindow::checkNewVersionHttps(void)
{
    QString versionHttpsFileUrl = getHttpsVersionFileUrl();
    //发送get请求
    QNetworkAccessManager *manager = new QNetworkAccessManager(nullptr);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(versionHttpsFileUrl)));

    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish
    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();
        QString versionRemoteStr = QString(replyData);
        if (versionRemoteStr.split(" ").at(0) != "Ver")
        {
            reply->close();
            delete manager;
            return false;
        }
        versionRemoteStr = versionRemoteStr.split(" ").at(0) + " " + versionRemoteStr.split(" ").at(1);

        QStringList argStrList = QString(PANEL_BOTTOM_VERSION_LABEL).split(" ");
        QString versionLocalStr = argStrList[0].append(" ");
        versionLocalStr.append(argStrList[1]);

        if (versionRemoteStr != versionLocalStr) {
            QMessageBox::StandardButton btnSelected = QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("发现新版本，是否更新？"), QMessageBox::Ok | QMessageBox::Cancel);
            switch (btnSelected) {
            case QMessageBox::Ok:
                slotBtnUpdateClicked(true);
                break;
            default: break;
            }
        }
    }

    reply->close();
    delete manager;
    return true;
}

/* 获取版本url */
#define HTML_URL        "http://www.sunristec.com/h-col-119.html"
QString MainWindow::getVersionFileUrl(void)
{
    QString versionFileUrl;
    //发送get请求
    QNetworkAccessManager *manager = new QNetworkAccessManager(nullptr);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(HTML_URL)));
    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish

    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();

        bool hasFound;
        QString urlTmpStr;    //远程文件的url
        char *buffer = replyData.data();
        QVector<int> posVector = StringCheck::KMP_Check(buffer, "data-url=", &hasFound);
        for (int i = 0; i < posVector.size(); i++) {

            urlTmpStr.clear();
            QByteArray baTmp;
            baTmp.clear();
            for (int j = posVector[i] + 10; buffer[j] != '"'; j++) {
                baTmp.append(buffer[j]);
            }
            urlTmpStr = QString(baTmp);

            if (urlTmpStr.contains("version.txt")) {
                versionFileUrl = urlTmpStr;
            }
        }
    }

    reply->close();
    delete manager;

    return versionFileUrl;
}

/* 获取https网页上版本url */
#define HTML_HTTPS_URL  "https://www.sunristec.com/h-col-119.html"
QString MainWindow::getHttpsVersionFileUrl(void)
{
    QString versionHttpsFileUrl;
    //发送请求get
    QSslConfiguration config ;

    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1_1OrLater);

    QNetworkRequest request(QNetworkRequest(QUrl(HTML_HTTPS_URL)));
    request.setSslConfiguration(config);

    QNetworkAccessManager *manager = new QNetworkAccessManager(nullptr);
    QNetworkReply *reply = manager->get(request);
    QEventLoop eventLoop;
    connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();       //block until finish

    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<< "reply error:" << reply->errorString();
    }
    else {
        //获取响应信息
        QByteArray replyData = reply->readAll();

        bool hasFound;
        QString urlTmpStr;    //远程文件的url
        char *buffer = replyData.data();
        QVector<int> posVector = StringCheck::KMP_Check(buffer, "data-url=", &hasFound);
        for (int i = 0; i < posVector.size(); i++) {

            urlTmpStr.clear();
            QByteArray baTmp;
            baTmp.clear();
            for (int j = posVector[i] + 10; buffer[j] != '"'; j++) {
                baTmp.append(buffer[j]);
            }
            urlTmpStr = QString(baTmp);

            if (urlTmpStr.contains("version.txt")) {
                versionHttpsFileUrl = urlTmpStr;
            }
        }
    }

    reply->close();
    delete manager;

    return versionHttpsFileUrl;
}
