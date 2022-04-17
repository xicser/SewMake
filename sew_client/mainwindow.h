#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "leftpanel/tablewidgetLayer.h"
#include "leftpanel/tablewidgetGraphic.h"
#include "config/configmainwin.h"
#include "designwindow.h"
#include "canvas/canvasviewpattern.h"
#include "fullsub/setting/cchangelayer.h"
#include "fullsub/setting/ceditshape.h"
#include "fullsub/setting/cshapeclone.h"
#include "fullsub/setting/clayerparameterset.h"
#include "fullsub/setting/setbasepoint.h"
#include "fullsub/sewingmode/csewingsimulation.h"
#include "fullsub/sewingmode/cspecialsewing.h"
#include "fullsub/sewingmode/csewingpointcode.h"
#include "fullsub/changeshapesize.h"
#include "fullsub/headtotailspacingofclosedshape.h"
#include "fullsub/warningbox.h"
#include "fullsub/helpwindow.h"
#include "cmassoperating.h"
#include <QDebug>
#include <QMainWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QFont>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QScrollBar>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QMessageBox>
#include <QTranslator>
#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>
#include <QWidgetAction>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTextCodec>
#include <QProcess>
#include <QNetworkReply>

#include "fullscreen/splashscreen.h"
#include "utils/PatterData/SewData/SewDataInterface/sewdatainterface.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private:

    /* 子窗口 */
    DesignWindow   *designWindow;                //设计界面窗口
    CChangeLayer   *changeLayerWindow;           //变图层窗口
    CShapeClone    *shapeCloneWindow;            //当前图复制窗口
    CEditShape     *editShapeWindow;             //当前图修改窗口
    CMassOperating *massOperatingWindow;         //批量处理窗口

    CLayerParameterSet    *layerParameterSetWindow;    //当前图层设置窗口
    SetBasePoint          *setBasePointWindow;         //设为基准点窗口
    CSewingSimulation     *sewingSimulationWindow;     //模拟缝纫窗口
    CSpecialSewing        *specialSewingWindow;        //特殊缝窗口
    CSewingPointCode      *sewingPointCodeWindow;      //缝纫点代码窗口
    ChangeShapeSize       *changeShapeSizeWindow;      //改变图形大小窗口
    HeadToTailSpacingOfClosedShape *spacingOfClosedShapeWindow; //封闭图形首尾点间距小窗口

    /* 尺寸相关 */
    int    screenXSize, screenYSize;             //屏幕可用尺寸
    QFont  font;                                 //字体
    int    fontSize;                             //字体大小
    int    scrollBarWidth;                       //滚动条宽度
    int    btnWidth, btnHeight;                  //按钮宽度, 高度(leftPanelMoveLayerGraphicFrameInit中设置)
    int    btnHorizontalSpacing, btnVerticalSpacing;       //按钮水平间距与竖直间距
    int    menuBarHeight;                        //菜单栏高度
    int    toolBarHeight;                        //工具栏高度
    int    leftPanelHeight, leftPanelWidth;      //左侧panel高度和宽度
    int    leftPanelModuleClearance;             //左侧panel各个模块间的间距
    int    leftPanelGBoxModuleWidth;             //左侧panel各个gBox模块的宽度
    int    leftPanelLayerGraphicHeight;          //左侧panel图形图层模块高度
    int    leftPanelMoveLayerGraphicHeight;      //左侧panel移动图形图层模块高度
    int    leftPanelMoveDistanceHeight;          //左侧panel移动距离gBox高度
    int    leftPanelSettingHeight;               //左侧panel设置gBox高度
    int    leftPanelTransformHeight;             //左侧panel变换gBox高度
    int    leftPanelSewingModeHeight;            //左侧panel缝纫模式gBox高度
    int    bottomPanelHeight, bottomPanelWidth;  //底部panel高度和宽度
    int    canvasViewWidth, canvasViewHeight;    //画板view宽度与高度
    int    canvasSceneWidth, canvasSceneHeight;  //画板scene宽度与高度
    int    statusBarWidth, statusBarHeight;      //状态栏宽度, 状态栏高度

    /* 菜单栏相关 */
    QMenuBar       *menuBar;                              //菜单栏

    //语言菜单
    QMenu         *menuLanguage;
    QWidgetAction *actionChinese, *actionEnglish, *actionVietnam, *actionItalian,
                  *actionTurkish, *actionSpanish, *actionRussian, *actionJapanese;
    QLabel        *labelChinese, *labelEnglish, *labelVietnam, *labelItalian,
                  *labelTurkish, *labelSpanish, *labelRussian, *labelJapanese;
    WarningBox    *warningChinese, *warningEnglish, *warningVietnam, *warningItalian,
                  *warningTurkish, *warningSpanish, *warningRussian, *warningJapanese;

    //编辑菜单
    QMenu          *menuEdit;
    QAction        *actionChangeSize;

    //选项菜单
    QMenu          *menuOpt;
    QAction        *actionDeleteMultiGraphic, *actionAutoMergeNearGraphic, *actionDelMultiGraphicsBetweenLayer,
                   *actionOpenNtp, *actionCloseGraphic;

    //作图菜单
    QAction        *actionPaint;

    //工具菜单
    QAction        *actionTools;

    /* 工具栏相关 */
    QToolBar       *toolBar;                              //工具栏
    QToolButton    *toolBtnOpenFile;                      //打开文件按钮
    QToolButton    *toolBtnSaveAs;                        //另存为按钮
    QToolButton    *toolBtnUndo;                          //后退
    QToolButton    *toolBtnRedo;                          //前进
    QToolButton    *toolBtnZoomUp;                        //放大
    QToolButton    *toolBtnZoomDown;                      //缩小
    QToolButton    *toolBtnDispRestore;                   //显示恢复
    QToolButton    *toolBtnPicDrag;                       //图形拖动
    QToolButton    *toolBtnNormalOpt;                     //正常操作
    QToolButton    *toolBtnHelp;                          //帮助
    QToolButton    *toolBtnUpdate;                        //软件更新
    HelpWindow     *helpWindow;                           //帮助窗口

    /* 左侧panel(控件均放置在QFrame里面) */
    QFrame         *frameLeftPanel;                       //左侧操作面板
    TableWidgetLayer   *tabWidgetLayer;                   //图层
    TableWidgetGraphic *tabWidgetGraphic;                 //图形

    //移动图层和图形的按钮QFrame
    QFrame         *frameMoveLayerGraphic;
    QPushButton    *btnConfirmLeft, *btnMoveUpLeft, *btnMoveDownLeft;
    QLineEdit      *lineEditLeft;
    QPushButton    *btnConfirmRight, *btnMoveUpRight, *btnMoveDownRight;
    QLineEdit      *lineEditRight;

    //移动距离GroupBox
    QGroupBox      *gBoxMoveDistance;
    QPushButton    *btnUp, *btnDown, *btnLeft, *btnRight, *btnDelete;
    QLineEdit      *lineEditMoveDistanceValue;

    //设置GroupBox
    QGroupBox      *gBoxSetting;
    QPushButton    *btnChangeLayer, *btnCurrentGraphicCopy, *btnCurrentGraphicModify,
                   *btnBatchProcessing, *btnCurrentLayerSetting, *btnSetToDatumPoint;

    //变换GroupBox
    QGroupBox      *gBoxTransform;
    QPushButton    *btnHorizontalMirror, *btnVerticalMirror, *btnRotate;
    QLabel         *labelRotationOpt;
    QLineEdit      *lineEditRotateValue;

    //缝纫模式GroupBox
    QGroupBox      *gBoxSewingMode;
    QPushButton    *btnSimSewing, *btnSewingPointCode, *btnSpecialSewing;


    /* 底部panel(控件均放置在QFrame里面) */
    QFrame         *frameBottomPanel;                     //底部操作面板
    QCheckBox      *chkBoxNoBenchmark;                    //免基准CheckBox
    QCheckBox      *chkBoxDispSerialNumber;               //显示序列号CheckBox
    QButtonGroup   *btnGroupPicAndLayer;                  //整体和图层组
    QRadioButton   *radioBtnDispWholePic;                 //显示整图
    QRadioButton   *radioBtnDispLayer;                    //显示图层
    QButtonGroup   *btnGroupDisplayMode;                  //显示模式
    QRadioButton   *radioBtnDispDot;                      //显示点
    QRadioButton   *radioBtnDispLine;                     //显示线
    QRadioButton   *radioBtnDispNormal;                   //正常显示
    QLabel         *labelVersion;                         //版本号label
    QPushButton    *btnExit;                              //退出按钮

    /* 画板 */
    CanvasViewPattern  *canvasViewPattern;                //图形画板view
    QGraphicsScene     *canvasScenePattern;               //图形画板scene

    /* 状态栏 */
    QLineEdit      *lineEditStatusBar;                    //状态栏

    /* 其他杂项 */
    QComboBox      *comboBoxZoom;                         //缩放下拉框
    QLabel         *labelZoom;                            //缩放
    QLabel         *labelColorDemo;                       //颜色示例
    QLabel         *labelColorDisp;                       //颜色显示

    QTranslator    *translator;                           //翻译器
    int            languageChosen;                        //选中的语言

    /* 标题栏与窗口放缩相关 */
    void changeControllerSize(QSize size);                //改变控件大小

    void dataInit(void);                                  //数据初始化
    void mainWindowInit(void);                            //主窗口初始化
    void menuBarInit(void);                               //菜单栏初始化
    void toolBarInit(void);                               //工具栏初始化
    void moveToolBarPosition(void);                       //移动工具栏位置
    void leftPanelInit(void);                             //左侧操作panel初始化
    void moveLeftPanelPosition(void);                     //移动左侧panel
    void leftPanelLayerGraphicScrollInit(void);           //左侧panel图形图层初始化
    void moveLeftPanelLayerGraphicScroll(void);           //移动左侧panel图形图层
    void leftPanelMoveLayerGraphicFrameInit(void);        //左侧panel移动图层图形Frame初始化(计算按钮尺寸)
    void moveLeftPanelMoveLayerGraphicFrame(void);        //移动左侧panel移动图层图形Frame
    void leftPanelMoveDistanceInit(void);                 //左侧panel移动距离gBox初始化
    void moveLeftPanelMoveDistance(void);                 //移动左侧panel移动距离gBox
    void leftPanelSettingInit(void);                      //左侧panel设置gBox初始化
    void moveLeftPanelSetting(void);                      //移动左侧panel设置gBox
    void leftPanelTransformInit(void);                    //左侧panel变换gBox初始化
    void moveLeftPanelTransform(void);                    //移动左侧panel变换gBox
    void leftPanelSewingModeInit(void);                   //左侧panel缝纫模式gBox初始化
    void moveLeftPanelSewingMode(void);                   //移动左侧panel缝纫模式gBox
    void bottomPanelInit(void);                           //底部操作panel初始化
    void moveBottomPanel(void);                           //移动底部操作panel
    void statusBarInit(void);                             //状态栏初始化
    void canvasInit(void);                                //画板初始化
    void miscWidgetInit(void);                            //剩余杂项控件初始化
    void moveMiscWidget(void);                            //移动剩余杂项控件
    void qssInit(void);                                   //式样初始化
    void slotsInit(void);                                 //槽初始化
    void languageInit(void);                              //语言初始化
    void languageIniSetting(int chosen);                  //语言配置文件设置
    void languageChosenQss(int chosen);                   //语言选中样式变化
    void languageChange(QString chosen);                  //语言改变
    void languageSwitch(int chosen);                      //语言选择

protected:
    void changeEvent(QEvent* event);                      //变换事件
    void resizeEvent(QResizeEvent *);                     //重画大小事件
    void closeEvent(QCloseEvent *);                       //关闭事件

private slots:
    //标题栏
    void slotBtnExitWinClicked(void);                      //退出按钮槽函数

    //菜单栏
	void slotActionChangeSizeTriggered(void);              //改变图形大小槽函数
    void slotActionCloseGraphicTriggered(void);            //封闭图形首尾点间距槽函数
    void slotActionDeleteMultiGraphicTriggered(void);      //打开文件时是否删除重合图形槽函数
    void slotActionChineseTriggered(void);                 //语言选择中文槽函数
    void slotActionEnglishTriggered(void);                 //语言选择英文槽函数
    void slotActionVietnamTriggered(void);                 //语言选择越南槽函数
    void slotActionItalianTriggered(void);                 //语言选择意大利语槽函数
    void slotActionTurkishTriggered(void);                 //语言选择土耳其语槽函数
    void slotActionSpanishTriggered(void);                 //语言选择西班牙语槽函数
    void slotActionRussianTriggered(void);                 //语言选择俄语槽函数
    void slotActionJapaneseTriggered(void);                //语言选择日本语槽函数

    //工具栏
    void slotBtnOpenFileClicked(void);                     //打开文件按钮槽函数
    void slotBtnSaveAsClicked(void);                       //另存为按钮槽函数
    void slotBtnPaintClicked(void);                        //作图按钮槽函数
    void slotBtnToolsClicked(void);                        //工具按钮槽函数
    void slotBtnUndoClicked(void);                         //撤销按钮槽函数
    void slotBtnRedoClicked(void);                         //重做按钮槽函数
    void slotBtnZoomUpClicked(void);                       //放大槽函数
    void slotBtnZoomDownClicked(void);                     //缩小槽函数
    void slotBtnDispRestoreClicked(void);                  //显示恢复槽函数
    void slotBtnPicDragClicked(void);                      //图形拖动槽函数
    void slotBtnNormalOptClicked(void);                    //正常操作槽函数
    void slotBtnHelpClicked(void);                         //帮助按钮槽函数
    void slotBtnUpdateClicked(bool bAutoUpgrade = false);  //软件升级按钮槽函数
    void slotComboBoxZoomActivated(int index);             //缩放QComboBox某行被使能槽函数

    //移动图层图形
    void slotBtnConfirmLeftClicked(void);                  //左确定按钮槽函数
    void slotBtnMoveUpLeftClicked(void);                   //左上移按钮槽函数
    void slotBtnMoveDownLeftClicked(void);                 //左下移按钮槽函数
    void slotBtnConfirmRightClicked(void);                 //右确定按钮槽函数
    void slotBtnMoveUpRightClicked(void);                  //右上移按钮槽函数
    void slotBtnMoveDownRightClicked(void);                //右下移按钮槽函数

    //移动距离gBox
    void slotBtnMoveUpClicked(void);                      //向上移动按钮槽函数
    void slotBtnMoveDownClicked(void);                    //向下移动按钮槽函数
    void slotBtnMoveLeftClicked(void);                    //向左移动按钮槽函数
    void slotBtnMoveRightClicked(void);                   //向右移动按钮槽函数
    void slotBtnDeleteClicked(void);                      //删除按钮槽函数

    //设置gBox
    void slotBtnBatchProcessingClicked(void);              //批量处理按钮槽函数
    void slotBtnCurrentLayerSettingClicked(void);          //当前图层设置槽函数
    void slotBtnSetToDatumPointClicked(void);              //设为基准点按钮槽函数

    //变换gBox
    void slotBtnHorizontalMirrorClicked(void);             //水平镜像按钮槽函数
    void slotBtnVerticalMirrorClicked(void);               //垂直镜像按钮槽函数
    void slotBtnRotateClicked(void);                       //旋转按钮槽函数

    //缝纫模式gBox
    void slotBtnSimSewingClicked(void);                    //模拟缝纫按钮槽函数
    void slotBtnSpecialSewingClicked(void);                //特殊缝按钮槽函数

    //底部panel
    void slotBottomRadioBtnGraphicLayerClicked(void);      //显示图层还是显示整图按钮槽函数
    void slotChkBoxDispSerialNumberClicked(void);          //显示图形序号复选框按钮槽函数
    void slotBottomRadioBtnDotLineNormalClicked(void);     //显示点/线/正常显示按钮槽函数
    void slotChkBoxNoBenchmarkClicked(void);               //免基准复选框按钮槽函数

    //编辑框
    void slotEditLineFinished(void);                       //编辑框编辑完毕槽函数

public slots:
    //设置gBox(供patternView的右键菜单使用)
    void slotBtnChangeLayerClicked(void);                  //变图层按钮槽函数
    void slotBtnCurrentGraphicCopyClicked(void);           //当前图复制按钮槽函数
    void slotBtnCurrentGraphicModifyClicked(void);         //当前图修改按钮槽函数

    //缝纫模式gBox
    void slotBtnSewingPointCodeClicked(void);              //缝纫点代码按钮槽函数

public:
    //根据文件绘制图形(总入口)
    void drawGraphicFromFile(QString filePathName);

    bool getActionDeleteMultiGraphic(void);      //获取是否删除重复图形选项
    bool getActionDelMultiGraphicsBetweenLayer(void);//获取是否删除跨图层重复图形选项
    bool getActionAutoMergeNearGraphic(void);    //获取自动合并临近图形选项
    bool getIsShowSerialNumber(void);            //获取是否显示图元的id
    bool getShowAllLayer(void);                  //获取显示图层还是显示整图
    bool getNoBenchmark(void);                   //获取是否免基准
    void setNoBenchmark(bool isSet);             //设置是否选中免基准
    void setLayerColorDemo(QString color);       //设置图层颜色示例
    void setLayerLineEdit(int layer);            //设置左侧图层列表下面的LineEdit
    void setGraphicLineEdit(int id);             //设置左侧图形列表下面的LineEdit
    void setMainTitleContent(QString);           //设置标题栏显示内容
    void setStatusBarContent(qreal, qreal, int, QPointF);      //设置状态栏内容
    void setBottomDotLineRadioBtnEnable(bool, bool, bool);     //设置底部显示模式radioBtn是否Enable
    void setBottomDotLineRadioBtnCheckable(bool, bool, bool);  //设置底部显示模式radioBtn是否Checkable
    void setScaleRatioContent(int);              //设置缩放比例大小显示值
    int  getScreenXSize(void);                   //获取屏幕x尺寸
    int  getScreenYSize(void);                   //获取屏幕y尺寸
    qreal getMoveDistance(void);                 //获取移动距离

private:
    typedef enum {
        UPGRADE_STATUS_NONE,
        UPGRADE_STATUS_START,
        UPGRADE_STATUS_CLOSE,

    } Enum_Upgrade_Status_t;
    Enum_Upgrade_Status_t upgradeStatus;         //更新状态
    QProcess *processUpgrade;                    //软件更新子进程

    QString openedfileType;                      //当前的打开文件类型
    QString openedfilePath;                      //当前的打开文件路径
    bool showAllLayer;                           //显示图层还是显示整图
    bool isShowSerialNumber;                     //是否显示图元的id

    QString lastOpenedFilePath;                  //保存上一次打开文件路径
    QString lastSaveAsFilePath;                  //保存上一次另存为文件路径
    void restorePathFromDisk(void);              //从磁盘上读取上一次设置的文件路径
    void savePathToDisk(void);                   //把本次设置的文件路径存放到磁盘上

    //主界面设置参数
    typedef struct {
        unsigned char option;                    //菜单栏选项位图
        qreal moveDistance;                      //主界面移动距离
        qreal rotationDegree;                    //旋转角度
    } MainWindow_Param_t;
    MainWindow_Param_t lastMainWindowParam;      //上次保存的主界面参数内容
    void restoreMainWindowParam(void);           //从磁盘上读取上一次设置的主界面参数
    void saveMainWindowParam(void);              //把本次设置的主界面参数存放到磁盘上

    bool bUseHttpHtml;                           //是否使用http网页上文件url链接
    bool checkNewVersion(void);                  //检查更新
    bool checkNewVersionHttps(void);             //检查更新https页面版本
    QString getVersionFileUrl(void);             //获取版本url
    QString getHttpsVersionFileUrl(void);        //获取https页面版本url
};

#endif // MAINWINDOW_H
