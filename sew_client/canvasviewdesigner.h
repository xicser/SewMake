/*
设计界面画板
*/
#ifndef __CANVASVIEW_DESIGNER_H__
#define __CANVASVIEW_DESIGNER_H__

#include <QGraphicsView>
#include <QMainWindow>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimerEvent>
#include <QScrollBar>
#include <QPainter>
#include <QDebug>
#include <QMenu>
#include <QStack>
#include <QMap>

#include "canvas/itemsdesign/SmoothCurveGenerator.h"
#include "canvas/itemdottedline.h"
#include "itemsdesign/itemdesigncentercircle.h"
#include "itemsdesign/itemdesign3pcircle.h"
#include "itemsdesign/itemdesignellipse.h"
#include "itemsdesign/itemdesignrectangle.h"
#include "itemsdesign/itemdesignline.h"
#include "itemsdesign/itemdesignpolygon.h"
#include "itemsdesign/itemdesignarc.h"
#include "itemsdesign/itemdesigncurve.h"
#include "itemsdesign/itemdesignemptyline.h"
#include "itemsdesign/itemdesignsendclothesline.h"
#include "itemsdesign/itemdesignstitchpoint.h"
#include "itemsdesign/itemdesignpunchpoint.h"

#include "utils/math/mathutils.h"
#include "utils/PatterData/SewData/SewDataInterface/sewdatainterface.h"
#include "utils/PatterData/SewData/Header/BasePatterDataDB.h"
#include "leftpanel/treelayerview.h"

//支持的图层颜色个数
#define LAYER_COLOR_COUNT_D  (7)

class DesignWindow;
class CanvasViewDesigner : public QGraphicsView
{
    Q_OBJECT
public:
    CanvasViewDesigner(QMainWindow *parent, TreeLayerView *treeLayerView, SewDataInterface *&interface, QString fileType, QString filePath);
    ~CanvasViewDesigner();

    void initGraphicFile(void);                     //首次进入初始化图形
    void openGraphicFile(QString filePathName, int type);   //打开图形文件
    void setGlobalSelfAdaption(void);               //全局图元自适应view大小
    void setGlobalStepScale(bool up);               //设置全局步进缩放
    void setGlobalScale(int value);                 //设置全局缩放到某一具体数指
    void gridInit(int gridSize);                    //栅格初始化
    void gridDeInit(void);                          //删除栅格
    void toggleGrid(void);                          //栅格显示切换
    void rightBtnMenuInit(void);                    //右键菜单初始化
    void appendLayer(void);                         //追加一个图层
    void deleteLayer(int);                          //删除某个图层
    void deleteEmptyLayer(void);                    //删除接口中的空图层
    void deleteEmptyLineAndSendClothesLine(void);   //删除接口中的发送服装线和空线
    void deleteItems(void);                         //删除图元
    void refreshLayer(int);                         //刷新某个图层
    void setBackCanvasDesigner(void);               //回设画板
    void setCurrentSelectedLayer(int);              //设置currentSelectedLayer
    void setCurrentSelectedItemsList(QPoint);       //设置currentSelectedItemsList
    QGraphicsItem *getItem(int layer, int id, eItem_Type_t *);  //获取图层layer上, 识别号为id的item
    int  getSelectedLayer(void);                    //获取当前选中的图层序号
    int  getUnitCountAll(void);                     //获取所有图层中的图元总数
    void showStitchPoints(bool);                    //显示缝纫点
    void showPunchPoints(bool);                     //显示关键点
    void setDesignItemsSelectableMovable(bool);     //设置图元是否可选择和移动
    bool getIsInterfaceChange(void);                //获取interface是否改变
    QString getOpenedFileType(void);                //获取打开文件类型
    QString getOpenedFilePath(void);                //获取打开文件路径
    int  saveFileToDisk(QString saveFilePath);      //保存文件到磁盘

    //以下是其他操作
    void setDesignWindow(DesignWindow *designWindow);     //设置DesignWindow
    void setTreeLayerView(TreeLayerView *treeLayerView);  //设置TreeLayerView
    void setDesignDrag(bool isDrag);                      //设置拖拽指示变量
    void setIsScaleMax(bool isScaleMax);                  //设置是否缩放到最大
    void startStateMachine(void);                         //启动状态机
    void stopStateMachine(void);                          //停止状态机
    void undoRedoInit(void);                              //初始化撤销与重做功能
    void undo(void);                                      //撤销
    void redo(void);                                      //重做

    //变换操作
    void setMirrorHorizontal(void);                       //选中图元水平镜像
    void setMirrorVertical(void);                         //选中图元垂直镜像
    void setRotating(qreal degree);                       //以旋转选中图元

    //显示/隐藏操作
    void setShowId(void);                                 //显示或隐藏图元序号
    void setIsShowOutline(void);                          //显示或隐藏轮廓线
    void setIsShowStitchSamllBox(void);                   //显示或隐藏缝纫点代码小框
    void setConnectLine(void);                            //显示或隐藏连接线
    void setStartPoint(void);                             //显示或隐藏起终点
    void setGraphic(bool isSet);                          //显示或隐藏图像

    //表示当前所处的选择模式
    typedef enum {
        SELECTION_MODE_ITEM,        //图元选择模式
        SELECTION_MODE_STITCH,      //缝纫点选择模式
        SELECTION_MODE_PUNCH,       //关键点选择模式
        SELECTION_MODE_DRAWING,     //绘制模式
    } Selection_Mode_t;
    Selection_Mode_t selectionMode;
    void setCurrentSelectionMode(Selection_Mode_t mode);  //设置当前的选择模式
    void clearAllSelection(void);                         //清除所有的选中内容

    //定位设置
    void setLocateToGrid(bool isSet);                     //定位到网格
    void setLocateToStitch(bool isSet);                   //定位到针迹点
    void setLocateToOutline(bool isSet);                  //定位到轮廓线

    //表示正在绘制的图形
    typedef enum {
        NOW_DRAWING_NONE,           //现在什么也没画
        NOW_DRAWING_LINE,           //正在画直线
        NOW_DRAWING_ARC,            //正在画弧线
        NOW_DRAWING_CURVE,          //正在画曲线
        NOW_DRAWING_RECTANGLE,      //正在绘制矩形
        NOW_DRAWING_POLYGON,        //正在绘制多边形
        NOW_DRAWING_3P_CIRCLE,      //正在绘制三点圆
        NOW_DRAWING_ELLIPSE,        //正在绘制椭圆
        NOW_DRAWING_CENTER_CIRCLE,  //正在绘制中心圆
        NOW_DRAWING_SENDING_CLOTHES_LINE,  //正在绘制发送服装
        NOW_DRAWING_EMPTY_LINE,            //正在绘制空线
        NOW_DRAWING_DISTANCE_MEASUREMENT_LINE,   //正在绘制距离测量直线
        //        NOW_DRAWING_COPY_MIRROR,           //正在绘制复制镜像参考线

    } Enum_NowDrawing_t;
    void setCurrentDrawing(Enum_NowDrawing_t);      //设置要绘制的图形
    void clearDrawingStatus(void);                  //清空绘制状态

protected:
    //定时器事件, 用于反复update, 以便强制重复调用paintEvent, 运行状态机
    void timerEvent(QTimerEvent *event);
    void paintEvent(QPaintEvent *event);            //绘图事件
    void wheelEvent(QWheelEvent *event);            //鼠标滚轮事件
    void mousePressEvent(QMouseEvent *event);       //鼠标按下事件
    void mouseDoubleClickEvent(QMouseEvent*);       //鼠标双击事件
    void mouseMoveEvent(QMouseEvent *event);        //鼠标移动事件
    void mouseReleaseEvent(QMouseEvent *event);     //鼠标释放事件
    void enterEvent(QEvent *event);                 //鼠标进入事件
    void leaveEvent(QEvent *event);                 //鼠标离开事件
    void keyPressEvent(QKeyEvent *event);           //键盘按下事件

private:
    //框选矩形绘制步骤枚举
    typedef enum {
        DRAWING_FRAME_SELECTION_RECT_WAIT_PRESS,    //等待鼠标按下
        DRAWING_FRAME_SELECTION_RECT_MOVING,        //鼠标移动中
        DRAWING_FRAME_SELECTION_RECT_WAIT_RELEASE   //等待鼠标松开
    } Frame_Selection_Rectangle_Steps_t;
    bool isFrameSelectionLeftBtnPressed;            //框选左键是否按下
    bool isFrameSelectionAvailable;                 //框选矩形是否有效
    void drawFrameSelectionRectangle(void);         //绘制框选矩形
    void selectItemFromRect(QPoint, QPoint);        //根据第一个点和第二个点确定的矩形, 选中图元
    void selectStitchFromRect(QPoint, QPoint);      //根据第一个点和第二个点确定的矩形, 选中缝纫点
    void selectPunchFromRect(QPoint, QPoint);       //根据第一个点和第二个点确定的矩形, 选中关键点
    void selectAllItems(void);                      //选中所有的图元
    void selectAllStitches(void);                   //选中所有的缝纫点
    void selectAllPunches(void);                    //选中所有的关键点

    //直线绘制步骤枚举
    typedef enum {
        DRAWING_LINE_WAIT_CLIKCKED,                   //等待鼠标按下
        DRAWING_LINE_MOVING,                          //鼠标移动中
        DRAWING_LINE_COMPLETE                         //绘制结束
    } Line_Steps_t;
    Line_Steps_t lineStatus;
    void drawLine(void);                              //绘制直线
    bool isLineFinish;                                //标识直线绘制是否结束
    QList<QPointF> linePointsList;                    //记录每个按下的点

    //曲线绘制步骤枚举
    typedef enum {
        DRAWING_CURVE_WAIT_CLIKCKED,                  //等待鼠标按下
        DRAWING_CURVE_MOVING,                         //鼠标移动中
        DRAWING_CURVE_COMPLETE                        //绘制结束
    } Curve_Steps_t;
    void drawCurve(void);                             //绘制曲线
    Curve_Steps_t curveStatus;
    bool isCurveFinish;                               //标识曲线绘制是否结束
    QList<QPointF> curvePointsList;                   //记录每个按下的点

    //弧线绘制步骤枚举
    typedef enum {
        DRAWING_ARC_WAIT_FIRST_CLIKCKED,              //等待鼠标第一次按下
        DRAWING_ARC_MOVING_FIRST,                     //第一次按下后鼠标移动中
        DRAWING_ARC_MOVING_SECOND,                    //第二次按下后鼠标移动中
        DRAWING_ARC_COMPLETE                          //绘制结束
    } Arc_Steps_t;
    typedef struct {                                  //保存画弧数据
        QRect rect;
        qreal startangle, endangle;
    } Arc_Draw_t;
    Arc_Steps_t arcStatus;
    void drawArc(void);                               //绘制弧线
    void drawArcPath(QPointF, QPointF, QPointF);      //绘制弧线路径
    QPointF arcFirstPoint;                            //弧线第一个点
    bool isArcFinish;                                 //标识弧线绘制是否结束
    QList<QPointF> arcPointsList;                     //记录每个按下的点

    //多边形绘制步骤枚举
    typedef enum {
        DRAWING_POLYGON_WAIT_CLIKCKED,                //等待鼠标按下
        DRAWING_POLYGON_MOVING,                       //鼠标移动中
        DRAWING_POLYGON_COMPLETE                      //绘制结束
    } Polygon_Steps_t;
    Polygon_Steps_t polygonStatus;
    void drawPolygon(void);                           //绘制多边形
    bool isPolygonFinish;                             //标识多边形绘制是否结束
    QList<QPointF> polygonPointsList;                 //记录每个按下的点

    //矩形绘制步骤枚举
    typedef enum {
        DRAWING_RECT_WAIT_FIRST_CLIKCKED,             //等待鼠标第一次按下
        DRAWING_RECT_MOVING,                          //鼠标移动中
        DRAWING_RECT_SECOND_CLIKCKED                  //鼠标第二次按下
    } Rectangle_Steps_t;
    Rectangle_Steps_t rectangleStatus;
    void drawRectangle(void);                         //绘制矩形
    bool isRectFinish;                                //标识矩形绘制是否结束
    QPointF rectangleFirstPoint, rectangleSecondPoint;//鼠标两次按下

    //三点圆绘制步骤枚举
    typedef enum {
        DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED,        //等待鼠标第一次按下
        DRAWING_3P_CIRCLE_MOVING_FIRST,               //第一次按下后鼠标移动中
        DRAWING_3P_CIRCLE_MOVING_SECOND,              //第二次按下后鼠标移动中
        DRAWING_3P_CIRCLE_THIRD_CLIKCKED              //鼠标第三次按下
    } ThreePointCircle_Steps_t;
    ThreePointCircle_Steps_t _3pCircleStatus;
    void drawThreePointCircle(void);                  //绘制三点圆
    bool is3PCircleFinish;                            //标识三点圆绘制是否结束
    QPointF _3pCircleFirstPoint, _3pCircleSecondPoint,//圆上三个点
           _3pCircleThirdPoint;

    //中心圆绘制步骤枚举
    typedef enum {
        DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED,    //等待鼠标第一次按下
        DRAWING_CENTER_CIRCLE_MOVING,                 //鼠标移动中
        DRAWING_CENTER_CIRCLE_SECOND_CLIKCKED         //鼠标第二次按下
    } CenterCircle_Steps_t;
    CenterCircle_Steps_t centerCircleStatus;
    void drawCenterCircle(void);                      //绘制中心圆
    bool isCenterCircleFinish;                        //标识中心圆绘制是否结束
    QPointF centerCircleFirstPoint,                   //圆心
           centerCircleSecondPoint;                   //鼠标确定半径的点

    //椭圆绘制步骤
    typedef enum {
        DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED,          //等待鼠标第一次按下
        DRAWING_ELLIPSE_MOVING_FIRST,                 //第一次按下后鼠标移动中
        DRAWING_ELLIPSE_MOVING_SECOND,                //第二次按下后鼠标移动中
        DRAWING_ELLIPSE_THIRD_CLIKCKED                //鼠标第三次按下
    } Ellipse_Steps_t;
    Ellipse_Steps_t ellipseStatus;
    void drawEllipse(void);                           //绘制椭圆
    bool isEllipseFinish;                             //标识椭圆绘制是否结束
    QPointF ellipseFirstPoint, ellipseSecondPoint,    //鼠标第三次按下
           ellipseThirdPoint;

    //空线绘制步骤枚举
    typedef enum {
        DRAWING_EMPTY_LINE_WAIT_CLIKCKED,             //等待鼠标按下
        DRAWING_EMPTY_LINE_MOVING,                    //鼠标移动中
        DRAWING_EMPTY_LINE_COMPLETE                   //绘制结束
    } Empty_Line_Steps_t;
    Empty_Line_Steps_t emptyLineStatus;
    void drawEmptyLine(void);                         //绘制空线
    bool isEmptyLineFinish;                           //标识空线绘制是否结束
    QList<QPointF> emptyLinePointsList;               //记录每个按下的点

    //发送服装线绘制步骤枚举
    typedef enum {
        DRAWING_SEND_CLOTHES_LINE_WAIT_CLIKCKED,      //等待鼠标按下
        DRAWING_SEND_CLOTHES_LINE_MOVING,             //鼠标移动中
        DRAWING_SEND_CLOTHES_LINE_COMPLETE            //绘制结束
    } Send_Clothes_Line_Steps_t;
    Send_Clothes_Line_Steps_t sendClothesLineStatus;
    void drawSendClothesLine(void);                   //绘制发送服装线
    bool isSendClothesLineFinish;                     //标识发送服装线绘制是否结束
    QList<QPointF> sendClothesLinePointsList;         //记录每个按下的点

    //距离测量步骤枚举
    typedef enum {
        DISTANCE_MEASUREMENT_WAIT_CLIKCKED,           //等待鼠标按下
        DISTANCE_MEASUREMENT_MOVING,                  //鼠标移动中
        DISTANCE_MEASUREMENT_COMPLETE                 //绘制结束
    } Distance_Measure_Steps_t;
    Distance_Measure_Steps_t measurementLineStatus;
    bool isMeasurementLineFinish;                     //标识距离测量线绘制是否结束
    void drawDistanceMeasurementLine(void);           //绘制距离测量直线

    void dataInit(void);                              //相关数据初始化
    void dumpViewToInterface(Enum_NowDrawing_t);      //把view上的图形导入到interface里
    void dumpInterfaceToScene(void);                  //把interface中的图形绘制在scene里
    void dumpInterfaceToSceneSelfAdapt(void);         //把interface中的图形绘制在scene里(自适应显示)
    void clearScene(void);                            //清空场景
    void setItemsShowId(void);                        //设置图形显示ID
    void reSelectCurrentItemsChosen(QList<QList<int>> tempSelectedItems);    //再次选中重绘之前的选中列表
    void updateMoveItemsOfInterface(void);            //更新被鼠标挪动的图形的接口数据
    void updateCenectionLine(void);                   //更新图形之间的连接线

    //以下画图函数都是在view中的scene上面画, 参数坐标是scene坐标
    ItemDesignStitchPoint *drawStitchPoint(QPointF pos);           //画缝纫点
    ItemDesignPunchPoint  *drawPunchPoint(QPointF pos);            //画关键点
    ItemDesignCenterCircle *drawCenterCircleToScene(QPointF firstPoint, QPointF secondPoint, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);               //画中心圆, 返回画的图元对象指针
    ItemDesign3PCircle *draw3PCircleToScene(QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);   //画三点圆
    ItemDesignEllipse *drawEllipseToScene(QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);     //画椭圆
    ItemDesignRectangle *drawRectangleToScene(QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint, QPointF fourthPoint, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList); //画矩形
    ItemDesignLine *drawLineToScene(QList<QPointF> pList, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);         //画直线(折线)
    ItemDesignPolygon *drawPolygonToScene(QList<QPointF> pList, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);   //画多边形
    ItemDesignArc *drawArcToScene(QList<QPointF> pList, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);           //画弧线
    ItemDesignCurve *drawCurveToScene(QList<QPointF> pList, QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList, QList<QGraphicsItem *> punchObjList);       //画曲线
    ItemDottedLine *drawDottedLine(QList<QPointF> pList);          //画虚线
    ItemDesignEmptyLine *drawEmptyLineToScene(QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList);              //画空线
    ItemDesignSendClothesLine *drawSendClothesLineToScene(QList<QPointF> &stitchesList, QList<QGraphicsItem *> stitchObjList);  //画发送服装线

    QMenu *menuSwitchRightBtn;                   //绘制中途菜单
    QAction *actionStopDrawing;
    QAction *actionSwitchToLine, *actionSwitchToArc, *actionSwitchToCurve, *actionSwitchToRect,
            *actionSwitchToPolygon, *actionSwitchTo3PCircle, *actionSwitchToEllipse, *actionSwitchToCenterCircle;

    //图层颜色数组
    QString layerColorsString[LAYER_COLOR_COUNT_D] = {
        "black", "blue", "pink", "green", "yellow", "purple", "brown"};
    QPen pen;                                         //画笔(用来绘制CAD图形)
    int stateMachineTimer;                            //状态机定时器句柄
    SewDataInterface *&interface;                     //文件接口指针
    QString       openedfileType;                     //当前的打开文件类型
    QString       openedfilePath;                     //当前的打开文件路径
    DesignWindow     *designWindow;                   //DesignWindow
    TreeLayerView    *treeLayerView;                  //TreeLayerView
    Enum_NowDrawing_t currentDrawing;                 //当前正在绘制的图形枚举

    int needleCount;                                  //总针数
    int layerCnt;                                     //当前拥有的图层个数
    int currentSelectedLayer;                         //当前选中的图层
    QList< QList<int> > currentSelectedItemsList;     //选中的图形list
    QList< QList<int> > currentSelectedStitchesList;  //当前选中的缝纫点list
    QList< QList<int> > currentSelectedPunchesList;   //当前选中的关键点list
    QList< QList<int> > ctrlCopyItemsList;            //ctrl+c复制的图元

    bool isInterfaceChanged;                          //interface是否改变
    bool isDesignDrag;                                //指示主窗口是否请求拖拽
    bool isScaleMax;                                  //指示是否缩放到最大
    QPoint mouseViewPos;                              //记录鼠标当前在View里面的坐标
    int  scalingRatio;                                //图形缩放比例
    bool isDrawingLeftBtnClicked;                     //CAD绘图左键是否按下
    bool isRightBtnClicked;                           //指示鼠标右键是否按下
    bool isLeftBtnClicked;                            //指示鼠标左键是否按下
    int  hViewPressCoord, vViewPressCoord;            //记录右键按下时鼠标指针在view的位置
    int  hScrollBarPressValue;                        //记录水平滚动条在 右键按下时的位置
    int  vScrollBarPressValue;                        //记录竖直滚动条在 右键按下时的位置

    //撤销、重做
    static int graphicFileSn;                       //图形文件序列号
    QString dirTmpFilePath;                         //临时文件路径
    QStack<QString> undoStack;                      //实现撤销和重做的栈
    QStack<QString> redoStack;
    void saveGraphicState(void);                    //保存图形状态

    //定位设置
    bool isLocateToGrid;                            //是否定位到网格
    bool isLocateToStitch;                          //是否定位到针迹点
    bool isLocateToOutline;                         //是否定位到轮廓线
    QMap<int, QList<QPointF>> mapStitch;             //缝纫点位置映射map
    void updateMapStitchByLocateOutline(void);       //定位到轮廓线时更新缝纫点map映射

    //显示或隐藏
    int  gridClearance;                             //栅格间距
    QList <QGraphicsItem *> gridLineList;           //保存栅格线的list
    QGraphicsRectItem *squareItem;                  //保存中间小方格图元
    bool isShowGrid;                                //是否显示栅格
    bool isShowOutline;                             //是否显示轮廓线
    bool isShowItemId;                              //是否显示图元id
    bool isShowConnectLine;                         //是否显示连接线
    bool isShowStitchSamllBox;                      //是否显示缝纫点代码小框
    bool isShowStartPoint;                          //是否显示起终点
    bool isShowGraphic;                             //是否显示图像

    //复制镜像
    bool isCopyMirror;                              //是否复制镜像

signals:
    //从场景选中图元时, 告知左侧treeLayerView当前选中的图元 的信号
    void signalSceneSelectedGraphic(QList< QList<int> > &currentSelectedItemList);

    //告知放缩信号
    void signalGlobalStepScale(void);

public slots:
    //场景中图元的选中情况有变化时调用的槽
    void slotSceneSelectionChanged(void);

private slots:
    void slotActionStopDrawingTriggered(void);                  //绘制结束
    void slotActionSwitchToLineTriggered(void);                 //切换至直线
    void slotActionSwitchToArcTriggered(void);                  //切换至弧线
    void slotActionSwitchToCurveTriggered(void);                //切换至曲线
    void slotActionSwitchToRectTriggered(void);                 //切换至矩形
    void slotActionSwitchToPolygonTriggered(void);              //切换至多边形
    void slotActionSwitchTo3PCircleTriggered(void);             //切换至三点圆
    void slotActionSwitchToEllipseTriggered(void);              //切换至椭圆
    void slotActionSwitchToCenterCircleTriggered(void);         //切换至中心圆
};


#endif
