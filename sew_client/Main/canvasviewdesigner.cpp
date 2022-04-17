/*
设计界面画板
*/

#include "canvasviewdesigner.h"
#include "fullscreen/mainwindow.h"
#include <QTime>
#include <QApplication>
#include "config/configparam.h"

CanvasViewDesigner::CanvasViewDesigner(QMainWindow *parent, TreeLayerView *treeLayerView,
                                       SewDataInterface *&interface, QString fileType, QString filePath)
    : QGraphicsView(parent), interface(interface), openedfileType(fileType), openedfilePath(filePath)
{
    this->treeLayerView = treeLayerView;
    this->setMouseTracking(true);              //实时跟踪鼠标
    dataInit();                                //相关数据初始化
    rightBtnMenuInit();                        //绘制场景右键菜单初始化
}

CanvasViewDesigner::~CanvasViewDesigner()
{
    stopStateMachine();
}

/* 相关数据初始化 */
void CanvasViewDesigner::dataInit(void)
{
    this->scalingRatio = 100;                  //图形缩放比例
    this->needleCount = 0;                     //总针数默认为0

    clearDrawingStatus();                      //清除之前的绘制状态
    this->currentDrawing = NOW_DRAWING_NONE;   //默认什么也没画
    this->isFrameSelectionLeftBtnPressed = false; //默认框选左键没有按下
    this->isFrameSelectionAvailable = true;    //默认框选矩形可用
    this->isDrawingLeftBtnClicked = false;     //默认绘图CAD左键没按下
    this->isLeftBtnClicked = false;            //默认左键没有按下
    this->isRightBtnClicked = false;           //默认右键没有按下
    this->isDesignDrag = false;                //默认设计窗口没有拖拽请求
    this->isShowGrid = true;                   //默认显示栅格
    this->stateMachineTimer = -1;              //默认没有启动状态机刷新定时器
    this->isShowOutline = true;                //默认显示轮廓线
    this->isShowConnectLine = false;           //默认不显示连接线
    this->isShowStitchSamllBox = false;        //默认不显示缝纫点代码小框
    this->isShowItemId = false;                //默认不显示图元序号
    this->isShowStartPoint = false;            //默认不显示起终点
    this->isShowGraphic = false;               //默认不显示图像
    this->isLocateToGrid = false;              //默认不定位到网格
    this->isLocateToStitch = false;            //默认不定位到针迹点
    this->isLocateToOutline = false;           //默认不定位到轮廓线
    this->isInterfaceChanged = false;          //默认interface未改变
    this->isScaleMax = false;                  //默认未缩放到最大
    this->isCopyMirror = false;                //默认未开复制镜像
    this->gridLineList.clear();
    this->currentSelectedItemsList.clear();    //默认无选中
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();
    interface->ArmCleanSelectObject();
    treeLayerView->clearSelection();
    this->selectionMode = SELECTION_MODE_ITEM; //默认处于图元选择模式

    //设置默认画笔
    this->pen.setWidth(0);
    this->pen.setColor("blue");
}

/* 首次进入初始化图形 */
void CanvasViewDesigner::initGraphicFile(void)
{
    currentSelectedLayer = 1;
    interface->ArmGetLayerCount(&layerCnt);
    interface->ArmSetCurrentLayer(currentSelectedLayer - 1);

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    dumpInterfaceToSceneSelfAdapt();  //首次绘制
}

/* 打开图形文件 */
void CanvasViewDesigner::openGraphicFile(QString filePathName, int type)
{
    int result;

reopen:
    if (interface != nullptr) {
        delete interface;
        interface = nullptr;
    }
    interface = DataFactory::createInstance(); //创建文件接口实例

    QByteArray fileName = filePathName.toLocal8Bit();
    const char *strFileName = fileName.data();
    result = interface->ArmFileOpenRead(type, strFileName);
    if (result != 0) {
        switch (type) {
            case 1:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("mtp文件打开失败！"));
                break;
            case 2:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("ntp文件打开失败！"));
                break;
            case 3:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("plt文件打开失败！"));
                break;
            case 4:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("dxf文件打开失败！"));
                break;
            case 10:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("ai文件打开失败！"));
                break;
            case 100:
                QMessageBox::warning(this, QString(tr("错误%1")).arg(result), tr("sdf文件打开失败！"));
                break;
            default: break;
        }

        //重新打开上次那个文件
        if (openedfileType != "null") {
            filePathName = openedfilePath;
            if (openedfileType == "mtp") {
                type = 1;
            } else if (openedfileType == "ntp") {
                type = 2;
            } else if (openedfileType == "plt") {
                type = 3;
            } else if (openedfileType == "dxf") {
                type = 4;
            } else if (openedfileType == "ai") {
                type = 10;
            } else if (openedfileType == "sdf") {
                type = 100;
            }
            goto reopen;
        }
        else {
            openedfileType = "null";
            openedfilePath = "null";
            return;
        }
    }
    switch (type) {
        case 1:
            openedfileType = "mtp";
            break;
        case 2:
            openedfileType = "ntp";
            break;
        case 3:
            openedfileType = "plt";
            break;
        case 4:
            openedfileType = "dxf";
            break;
        case 10:
            openedfileType = "ai";
            break;
        case 100:
            openedfileType = "sdf";
            break;
        default: break;
    }
    openedfilePath = filePathName;
    designWindow->setStatusBarFilePath(openedfilePath);

    //相关数据初始化
    this->stopStateMachine();                  //停止状态机
    clearDrawingStatus();                      //清除之前的绘制状态
    undoRedoInit();                            //撤消重做初始化
    this->needleCount = 0;                     //总针数默认为0
    this->currentDrawing = NOW_DRAWING_NONE;   //默认现在什么也没画
    this->isFrameSelectionLeftBtnPressed = false; //默认框选左键没有按下
    this->isFrameSelectionAvailable = true;    //默认框选矩形可用
    this->isDrawingLeftBtnClicked = false;     //默认绘图CAD左键没按下
    this->isLeftBtnClicked = false;            //默认左键没有按下
    this->isRightBtnClicked = false;           //默认右键没有按下
    this->isDesignDrag = false;                //默认设计窗口没有拖拽请求
    this->isInterfaceChanged = true;           //打开文件一定改变了接口数据
    this->currentSelectedItemsList.clear();    //默认无选中
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();
    interface->ArmCleanSelectObject();
    treeLayerView->clearSelection();

    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    this->scene()->clearSelection();
    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    this->selectionMode = SELECTION_MODE_ITEM; //默认处于图元选择模式
    designWindow->setLeftPanelBtnChosenQss(1);

    //选中第一个图层
    currentSelectedLayer = 1;
    interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
    interface->ArmGetLayerCount(&layerCnt);

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    //绘制
    dumpInterfaceToSceneSelfAdapt();
}

/* 保存文件到磁盘 */
int CanvasViewDesigner::saveFileToDisk(QString saveFilePath)
{
    QByteArray fileName = saveFilePath.toLocal8Bit();
    const char *strFileName = fileName.data();
    return interface->ArmFileSaveInfo(100, strFileName);
}

/* 获取interface是否改变 */
bool CanvasViewDesigner::getIsInterfaceChange(void)
{
    return isInterfaceChanged;
}

/* 获取打开文件类型 */
QString CanvasViewDesigner::getOpenedFileType(void)
{
    if (this->openedfileType == "null" && isInterfaceChanged == true) {
        openedfileType = "sdf";
    }
    return this->openedfileType;
}

/* 获取打开文件路径 */
QString CanvasViewDesigner::getOpenedFilePath(void)
{
    return this->openedfilePath;
}

/* 设置是否缩放到最大 */
void CanvasViewDesigner::setIsScaleMax(bool isScaleMax)
{
    this->isScaleMax = isScaleMax;
}

/* 全局图元自适应view大小 */
void CanvasViewDesigner::setGlobalSelfAdaption(void)
{
    if (getUnitCountAll() == 0) {
        return;
    }

    //删除格栅栏
    this->gridDeInit();

    //scene场景内所有图元边框坐标范围
    QRectF rect = this->scene()->itemsBoundingRect();

    //重画格栅栏
    this->gridInit(gridClearance);
    if (this->isShowGrid == true) {
        this->isShowGrid = false;
    } else {
        this->isShowGrid = true;
    }
    this->toggleGrid();

    //按图元边框范围适应整个view边框大小
    this->fitInView(rect, Qt::KeepAspectRatio);

    scalingRatio = 100;
    designWindow->setScaleRatioContent(scalingRatio);
    designWindow->setStatusBarZoom(this->scalingRatio);

    emit signalGlobalStepScale();
}

/* 设置全局步进缩放 */
void CanvasViewDesigner::setGlobalStepScale(bool up)
{
    if (up == true) {
        if (scalingRatio < 800 && isScaleMax == false) {
            this->scale(1.1, 1.1);
            scalingRatio += 10;
            designWindow->setScaleRatioContent(scalingRatio);
            designWindow->setStatusBarZoom(this->scalingRatio);
        }
    } else {
        if (scalingRatio > 10) {
            this->scale(1.0 / 1.1, 1.0 / 1.1);
            scalingRatio -= 10;
            designWindow->setScaleRatioContent(scalingRatio);
            designWindow->setStatusBarZoom(this->scalingRatio);

            if (isScaleMax == true) {
                isScaleMax = false;
            }
        }
    }
    emit signalGlobalStepScale();
}

/* 设置全局缩放到某一具体数指 */
void CanvasViewDesigner::setGlobalScale(int value)
{
    if (value > 200 || value < 10 || value % 10 != 0 || value == scalingRatio) {
        return;
    }

    int delta = abs(scalingRatio - value);

    //缩小
    if (scalingRatio > value) {
        for (int i = 0; i < delta; i += 10) {
            this->scale(1.0 / 1.1, 1.0 / 1.1);
            scalingRatio -= 10;
            designWindow->setScaleRatioContent(scalingRatio);
            designWindow->setStatusBarZoom(this->scalingRatio);
        }
    } else {
        for (int i = 0; i < delta; i += 10) {
            this->scale(1.1, 1.1);
            scalingRatio += 10;
            designWindow->setScaleRatioContent(scalingRatio);
            designWindow->setStatusBarZoom(this->scalingRatio);
        }
    }
}

/* 设置DesignWindow */
void CanvasViewDesigner::setDesignWindow(DesignWindow *designWindow)
{
    this->designWindow = designWindow;
}

/* 设置TreeLayerView */
void CanvasViewDesigner::setTreeLayerView(TreeLayerView *treeLayerView)
{
    this->treeLayerView = treeLayerView;
}

/* 设置拖拽指示变量 */
void CanvasViewDesigner::setDesignDrag(bool isDrag)
{
    this->isDesignDrag = isDrag;
}

/* 启动状态机 */
void CanvasViewDesigner::startStateMachine(void)
{
    if (stateMachineTimer == -1) {
        //启动10ms定时器, 强制轮询paintEvent
        stateMachineTimer = this->startTimer(10);
    }
}

/* 停止状态机 */
void CanvasViewDesigner::stopStateMachine(void)
{
    if (stateMachineTimer != -1) {
        this->killTimer(stateMachineTimer);
        stateMachineTimer = -1;
    }
}

/* 设置当前的选择模式 */
void CanvasViewDesigner::setCurrentSelectionMode(Selection_Mode_t mode)
{
    this->selectionMode = mode;
}

/* 清除所有的选中内容 */
void CanvasViewDesigner::clearAllSelection(void)
{
    interface->ArmCleanSelectObject();
    currentSelectedItemsList.clear();
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    this->scene()->clearSelection();
    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    treeLayerView->clearSelection();
}

/* 设置要绘制的图形 */
void CanvasViewDesigner::setCurrentDrawing(Enum_NowDrawing_t nowDrawing)
{
    clearDrawingStatus(); //清除之前的状态
    this->currentDrawing = nowDrawing;

    if (this->currentDrawing != NOW_DRAWING_NONE) {
        this->stopStateMachine();
        this->setCurrentSelectionMode(SELECTION_MODE_DRAWING);
        this->clearAllSelection();
    } else {
        this->stopStateMachine();
    }
    this->setFocus();
}

/* 清空绘制状态 */
void CanvasViewDesigner::clearDrawingStatus(void)
{
    linePointsList.clear();            //直线需要清除一下list
    polygonPointsList.clear();         //多边形需要清除一下list
    arcPointsList.clear();             //弧线需要清除一下list
    curvePointsList.clear();           //曲线需要清除一下list
    emptyLinePointsList.clear();       //空线需要清除一下list
    sendClothesLinePointsList.clear(); //发送服装需要清除一下list

    isLineFinish = false;
    isPolygonFinish = false;
    isArcFinish = false;
    isCurveFinish = false;
    isRectFinish = false;
    is3PCircleFinish = false;
    isEllipseFinish = false;
    isCenterCircleFinish = false;
    isEmptyLineFinish = false;
    isSendClothesLineFinish = false;
    isMeasurementLineFinish = false;

    lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
    curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
    arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
    polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
    rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
    _3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
    centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
    ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
    emptyLineStatus = DRAWING_EMPTY_LINE_WAIT_CLIKCKED;
    sendClothesLineStatus = DRAWING_SEND_CLOTHES_LINE_WAIT_CLIKCKED;
    measurementLineStatus = DISTANCE_MEASUREMENT_WAIT_CLIKCKED;
}


/* 绘制框选矩形 */
void CanvasViewDesigner::drawFrameSelectionRectangle(void)
{
    //绘制矩形的初始状态为DRAWING_FRAME_SELECTION_RECT_WAIT_PRESS
    static Frame_Selection_Rectangle_Steps_t frameSelectionRectangleStatus
            = DRAWING_FRAME_SELECTION_RECT_WAIT_PRESS;
    static QPoint firstPoint, secondPoint;

    switch (frameSelectionRectangleStatus) {

        //等待鼠标第一次按下
        case DRAWING_FRAME_SELECTION_RECT_WAIT_PRESS: {
            if (this->isFrameSelectionLeftBtnPressed == true) {

                //状态切换至鼠标移动状态
                frameSelectionRectangleStatus = DRAWING_FRAME_SELECTION_RECT_MOVING;

                //记录下当前鼠标指针的坐标
                firstPoint = this->mouseViewPos;
            }
            break;
        }
        //鼠标移动中
        case DRAWING_FRAME_SELECTION_RECT_MOVING: {

            if (isFrameSelectionAvailable == true) {
                QPainter painter(this->viewport());
                QPen pen;
                pen.setColor("black");
                pen.setWidth(0);
                pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
                painter.drawRect(QRect(firstPoint, this->mouseViewPos));
            }

            //移动过程中松开了
            if (this->isFrameSelectionLeftBtnPressed == false) {

                //状态切换
                frameSelectionRectangleStatus = DRAWING_FRAME_SELECTION_RECT_WAIT_RELEASE;

                //记录下当前鼠标指针的坐标
                secondPoint = this->mouseViewPos;
            }
            break;
        }
        //鼠标松开
        case DRAWING_FRAME_SELECTION_RECT_WAIT_RELEASE: {

            if (isFrameSelectionAvailable == true) {
                //根据第一个点和第二个点确定的矩形, 选中
                if (MathUtils::getDistance2D(firstPoint, secondPoint) > 3) {
                    switch (selectionMode) {
                        case SELECTION_MODE_ITEM:        //图元选择模式
                            this->selectItemFromRect(firstPoint, secondPoint);
                            break;
                        case SELECTION_MODE_STITCH:      //缝纫点选择模式
                            this->selectStitchFromRect(firstPoint, secondPoint);
                            break;
                        case SELECTION_MODE_PUNCH:       //关键点选择模式
                            this->selectPunchFromRect(firstPoint, secondPoint);
                            break;
                        case SELECTION_MODE_DRAWING:     //绘制模式

                        default: break;
                    }
                }
            }

            //恢复初始状态
            frameSelectionRectangleStatus = DRAWING_FRAME_SELECTION_RECT_WAIT_PRESS;
            this->stopStateMachine();

            break;
        }
        default: break;
    }
}

/* 根据第一个点和第二个点确定的矩形, 选中图元 */
void CanvasViewDesigner::selectItemFromRect(QPoint p1, QPoint p2)
{
    QPointF pLeftTop, pRightBottom;
    if (p1.x() < p2.x() && p1.y() < p2.y()) {
        pLeftTop = this->mapToScene(p1);
        pRightBottom = this->mapToScene(p2);
    } else if (p1.x() > p2.x() && p1.y() > p2.y()) {
        pLeftTop = this->mapToScene(p2);
        pRightBottom = this->mapToScene(p1);
    } else if (p1.x() < p2.x() && p1.y() > p2.y()) {
        QPoint tmpP1 = p1;
        QPoint tmpP2 = p2;
        p1.setY(tmpP2.y());
        p2.setY(tmpP1.y());
        pLeftTop = this->mapToScene(p1);
        pRightBottom = this->mapToScene(p2);
    } else {
        QPoint tmpP1 = p1;
        QPoint tmpP2 = p2;
        p1.setY(tmpP2.y());
        p2.setY(tmpP1.y());
        pLeftTop = this->mapToScene(p2);
        pRightBottom = this->mapToScene(p1);
    }
    QRectF rectFrame = QRectF(pLeftTop, pRightBottom);

    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    //遍历所有图元, 找出图元
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        QRectF rectItem; //图元在场景中的外接矩形
        int layer, unit;
        switch (itemType) {

            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                layer = itemDesign3PCircle->getLayer();
                unit = itemDesign3PCircle->getItemId();
                qreal r = itemDesign3PCircle->getRadius();
                QPointF pos = itemDesign3PCircle->pos();
                rectItem = QRectF(pos.x() - r, pos.y() - r, r * 2, r * 2);
                break;
            }
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                layer = itemDesignCenterCircle->getLayer();
                unit = itemDesignCenterCircle->getItemId();
                qreal r = itemDesignCenterCircle->getRadius();
                QPointF pos = itemDesignCenterCircle->pos();
                rectItem = QRectF(pos.x() - r, pos.y() - r, r * 2, r * 2);
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                layer = itemDesignEllipse->getLayer();
                unit = itemDesignEllipse->getItemId();
                qreal height = itemDesignEllipse->getHeight();
                qreal width = itemDesignEllipse->getWidth();
                QPointF pos = itemDesignEllipse->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                layer = itemDesignRectangle->getLayer();
                unit = itemDesignRectangle->getItemId();
                qreal height = itemDesignRectangle->getHeight();
                qreal width = itemDesignRectangle->getWidth();
                QPointF pos = itemDesignRectangle->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                layer = itemDesignLine->getLayer();
                unit = itemDesignLine->getItemId();
                qreal height = itemDesignLine->getHeight();
                qreal width = itemDesignLine->getWidth();
                QPointF pos = itemDesignLine->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);

                //高为0
                if (fabs(height - 0) < 0.001) {
                    qreal left = pos.x() - width / 2;
                    qreal right = left + width;
                    if (rectFrame.contains(pos) == true &&
                            left > rectFrame.left() && rectFrame.right() > right) {
                        //先在场景里选中
                        itemList.at(i)->setSelected(true);

                        //再在数据接口中选中
                        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);

                        //记录被框选中的图元
                        QList<int> listTmp;
                        listTmp.clear();
                        listTmp << layer << unit;
                        currentSelectedItemsList.append(listTmp);
                        continue;
                    }
                }
                //宽为0
                else if (fabs(width - 0) < 0.001) {
                    qreal top = pos.y() - height / 2;
                    qreal bottom = top + height;
                    if (rectFrame.contains(pos) == true &&
                            top > rectFrame.top() && rectFrame.bottom() > bottom) {
                        //先在场景里选中
                        itemList.at(i)->setSelected(true);

                        //再在数据接口中选中
                        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);

                        //记录被框选中的图元
                        QList<int> listTmp;
                        listTmp.clear();
                        listTmp << layer << unit;
                        currentSelectedItemsList.append(listTmp);
                        continue;
                    }
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                layer = itemDesignPolygon->getLayer();
                unit = itemDesignPolygon->getItemId();
                qreal height = itemDesignPolygon->getHeight();
                qreal width = itemDesignPolygon->getWidth();
                QPointF pos = itemDesignPolygon->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                layer = itemDesignArc->getLayer();
                unit = itemDesignArc->getItemId();
                qreal height = itemDesignArc->getHeight();
                qreal width = itemDesignArc->getWidth();
                QPointF pos = itemDesignArc->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                layer = itemDesignEmptyLine->getLayer();
                unit = itemDesignEmptyLine->getItemId();
                qreal height = itemDesignEmptyLine->getHeight();
                qreal width = itemDesignEmptyLine->getWidth();
                QPointF pos = itemDesignEmptyLine->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                layer = itemDesignSendClothesLine->getLayer();
                unit = itemDesignSendClothesLine->getItemId();
                qreal height = itemDesignSendClothesLine->getHeight();
                qreal width = itemDesignSendClothesLine->getWidth();
                QPointF pos = itemDesignSendClothesLine->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                layer = itemDesignCurve->getLayer();
                unit = itemDesignCurve->getItemId();
                qreal height = itemDesignCurve->getHeight();
                qreal width = itemDesignCurve->getWidth();
                QPointF pos = itemDesignCurve->pos();
                rectItem = QRectF(pos.x() - width / 2, pos.y() - height / 2, width, height);
                break;
            }
            default: break;
        }

        //如果图元外接矩形在那个矩形框内, 则选中它
        if (rectFrame.contains(rectItem) == true) {

            //先在场景里选中
            itemList.at(i)->setSelected(true);

            //再在数据接口中选中
            interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);

            //记录被框选中的图元
            QList<int> listTmp;
            listTmp.clear();
            listTmp << layer << unit;
            currentSelectedItemsList.append(listTmp);
        }
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 根据第一个点和第二个点确定的矩形, 选中缝纫点 */
void CanvasViewDesigner::selectStitchFromRect(QPoint p1, QPoint p2)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    //遍历所有图元, 找出缝纫点
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            case eITEM_DESIGN_STITCH_POINT: {
                ItemDesignStitchPoint *itemDesignStitchPoint = dynamic_cast<ItemDesignStitchPoint *>(itemList.at(i));
                if (itemDesignStitchPoint != nullptr) {

                    //如果缝纫点在那个矩形框内, 则选中它
                    if (MathUtils::isPointInRect(this->mapToScene(p1), this->mapToScene(p2),
                        itemDesignStitchPoint->pos()) == true) {

                        //先在场景里选中
                        itemDesignStitchPoint->setSelected(true);

                        //再在数据接口中选中
                        int layer = itemDesignStitchPoint->getLayer();
                        int unit = itemDesignStitchPoint->getItemId();
                        int stitch = itemDesignStitchPoint->getStitchNum();
                        interface->ArmAddStitchToCurrentSelectStitchList(layer - 1, unit - 1, stitch - 1);

                        //记录被框选中的缝纫点
                        QList<int> listTmp;
                        listTmp.clear();
                        listTmp << layer << unit << stitch;
                        currentSelectedStitchesList.append(listTmp);
                    }
                }
                break;
            }
            default: break;
        }
    }

    //还要选中缝纫点所在的图元
    for (int i = 0; i < currentSelectedStitchesList.size(); i++) {

        QList<int> listTmp;
        listTmp = currentSelectedStitchesList[i];
        int layer = listTmp[0];
        int unit = listTmp[1];

        currentSelectedItemsList << listTmp;
        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
//        eItem_Type_t itemType;
//        QGraphicsItem *item = this->getItem(layer, unit, &itemType);
//        item->setSelected(true);
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    if (interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB) == 0) {
        designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));
    }

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 根据第一个点和第二个点确定的矩形, 选中关键点 */
void CanvasViewDesigner::selectPunchFromRect(QPoint p1, QPoint p2)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    //遍历所有图元, 找出关键点
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            case eITEM_DESIGN_PUNCH_POINT: {
                ItemDesignPunchPoint *itemDesignPunchPoint = dynamic_cast<ItemDesignPunchPoint *>(itemList.at(i));
                if (itemDesignPunchPoint != nullptr) {

                    //如果缝纫点在那个矩形框内, 则选中它
                    if (MathUtils::isPointInRect(this->mapToScene(p1), this->mapToScene(p2),
                        itemDesignPunchPoint->pos()) == true) {

                        //先在场景里选中
                        itemDesignPunchPoint->setSelected(true);

                        //再在数据接口中选中
                        int layer = itemDesignPunchPoint->getLayer();
                        int unit = itemDesignPunchPoint->getItemId();
                        int punch = itemDesignPunchPoint->getPunchPointNum();

                        interface->ArmAddPunchToCurrentSelectPunchList(layer - 1, unit - 1, punch - 1);

                        //记录被框选中的关键点
                        QList<int> listTmp;
                        listTmp.clear();
                        listTmp << layer << unit << punch;
                        currentSelectedPunchesList.append(listTmp);
                    }
                }
                break;
            }
            default: break;
        }
    }

    //还要选中关键点所在的图元
    for (int i = 0; i < currentSelectedPunchesList.size(); i++) {

        QList<int> listTmp;
        listTmp = currentSelectedPunchesList[i];
        int layer = listTmp[0];
        int unit = listTmp[1];

        currentSelectedItemsList << listTmp;
        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
//        eItem_Type_t itemType;
//        QGraphicsItem *item = this->getItem(layer, unit, &itemType);
//        item->setSelected(true);
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    if (interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB) == 0) {
        designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));
    }

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 选中所有的图元 */
void CanvasViewDesigner::selectAllItems(void)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    bool isGraphicItem;
    //遍历所有图元, 找出图元
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        int layer, unit;
        isGraphicItem = false;
        switch (itemType) {

            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                layer = itemDesign3PCircle->getLayer();
                unit = itemDesign3PCircle->getItemId();
                isGraphicItem = true;
                break;
            }
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                layer = itemDesignCenterCircle->getLayer();
                unit = itemDesignCenterCircle->getItemId();
                isGraphicItem = true;
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                layer = itemDesignEllipse->getLayer();
                unit = itemDesignEllipse->getItemId();
                isGraphicItem = true;
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                layer = itemDesignRectangle->getLayer();
                unit = itemDesignRectangle->getItemId();
                isGraphicItem = true;
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                layer = itemDesignLine->getLayer();
                unit = itemDesignLine->getItemId();
                isGraphicItem = true;
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                layer = itemDesignPolygon->getLayer();
                unit = itemDesignPolygon->getItemId();
                isGraphicItem = true;
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                layer = itemDesignArc->getLayer();
                unit = itemDesignArc->getItemId();
                isGraphicItem = true;
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                layer = itemDesignEmptyLine->getLayer();
                unit = itemDesignEmptyLine->getItemId();
                isGraphicItem = true;
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                layer = itemDesignSendClothesLine->getLayer();
                unit = itemDesignSendClothesLine->getItemId();
                isGraphicItem = true;
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                layer = itemDesignCurve->getLayer();
                unit = itemDesignCurve->getItemId();
                isGraphicItem = true;
                break;
            }
            default: break;
        }

        if (isGraphicItem == true) {
            //先在场景里选中
            itemList.at(i)->setSelected(true);

            //再在数据接口中选中
            interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);

            //记录被框选中的图元
            QList<int> listTmp;
            listTmp.clear();
            listTmp << layer << unit;
            currentSelectedItemsList.append(listTmp);
        }
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 选中所有的缝纫点 */
void CanvasViewDesigner::selectAllStitches(void)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    //遍历所有图元, 找出缝纫点
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            case eITEM_DESIGN_STITCH_POINT: {
                ItemDesignStitchPoint *itemDesignStitchPoint = dynamic_cast<ItemDesignStitchPoint *>(itemList.at(i));
                if (itemDesignStitchPoint != nullptr) {

                    //先在场景里选中
                    itemDesignStitchPoint->setSelected(true);

                    //再在数据接口中选中
                    int layer = itemDesignStitchPoint->getLayer();
                    int unit = itemDesignStitchPoint->getItemId();
                    int stitch = itemDesignStitchPoint->getStitchNum();
                    interface->ArmAddStitchToCurrentSelectStitchList(layer - 1, unit - 1, stitch - 1);

                    //记录被框选中的缝纫点
                    QList<int> listTmp;
                    listTmp.clear();
                    listTmp << layer << unit << stitch;
                    currentSelectedStitchesList.append(listTmp);
                }
                break;
            }
            default: break;
        }
    }

    //还要选中缝纫点所在的图元
    for (int i = 0; i < currentSelectedStitchesList.size(); i++) {

        QList<int> listTmp;
        listTmp = currentSelectedStitchesList[i];
        int layer = listTmp[0];
        int unit = listTmp[1];

        currentSelectedItemsList << listTmp;
        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
        eItem_Type_t itemType;
        QGraphicsItem *item = this->getItem(layer, unit, &itemType);
        item->setSelected(true);
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 选中所有的关键点 */
void CanvasViewDesigner::selectAllPunches(void)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();
    if (itemList.isEmpty() == true) {
        return;
    }

    //先清接口中的除选中
    interface->ArmCleanSelectObject();
    this->scene()->clearSelection();
    treeLayerView->clearSelection();
    currentSelectedStitchesList.clear();
    currentSelectedItemsList.clear();
    currentSelectedPunchesList.clear();

    //遍历所有图元, 找出关键点
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            case eITEM_DESIGN_PUNCH_POINT: {
                ItemDesignPunchPoint *itemDesignPunchPoint = dynamic_cast<ItemDesignPunchPoint *>(itemList.at(i));
                if (itemDesignPunchPoint != nullptr) {

                    //先在场景里选中
                    itemDesignPunchPoint->setSelected(true);

                    //再在数据接口中选中
                    int layer = itemDesignPunchPoint->getLayer();
                    int unit = itemDesignPunchPoint->getItemId();
                    int punch = itemDesignPunchPoint->getPunchPointNum();

                    interface->ArmAddPunchToCurrentSelectPunchList(layer - 1, unit - 1, punch - 1);

                    //记录被框选中的关键点
                    QList<int> listTmp;
                    listTmp.clear();
                    listTmp << layer << unit << punch;
                    currentSelectedPunchesList.append(listTmp);
                }
                break;
            }
            default: break;
        }
    }

    //还要选中关键点所在的图元
    for (int i = 0; i < currentSelectedPunchesList.size(); i++) {

        QList<int> listTmp;
        listTmp = currentSelectedPunchesList[i];
        int layer = listTmp[0];
        int unit = listTmp[1];

        currentSelectedItemsList << listTmp;
        interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
        eItem_Type_t itemType;
        QGraphicsItem *item = this->getItem(layer, unit, &itemType);
        item->setSelected(true);
    }

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    if (currentSelectedItemsList.size() > 0) {
        //设置右上角颜色示例
        this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

        interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);
    }

    //通知更新treeLayerView的选中情况
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 绘制直线 */
void CanvasViewDesigner::drawLine(void)
{
    switch (lineStatus) {

        //等待鼠标第一次按下
        case DRAWING_LINE_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                lineStatus = DRAWING_LINE_MOVING;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    linePointsList << nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        linePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                linePointsList.removeLast();
                                linePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        linePointsList << this->mapToScene(this->mouseViewPos);
                    }

                }else {
                    linePointsList << this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isLineFinish == true) {
                lineStatus = DRAWING_LINE_COMPLETE;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_LINE_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //先绘制之前的线
            for (int i = 0; i < linePointsList.size() - 1; i++) {
                painter.drawLine( this->mapFromScene( linePointsList.at(i) ),
                                 this->mapFromScene( linePointsList.at(i + 1) ) );
            }
            //再绘制这次的
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                //若当前位置仍与上一次关键点位置最近则不画线
                if ( nearestPos != linePointsList.last() ) {
                    painter.drawLine( this->mapFromScene( linePointsList.last() ),
                                      this->mapFromScene( nearestPos ) );
                }

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawLine( this->mapFromScene( linePointsList.last() ),
                                      this->mapFromScene( nearPos ) );

                } else {
                    painter.drawLine( this->mapFromScene( linePointsList.last() ),
                                      this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene( linePointsList.last() ),
                                  this->mouseViewPos);
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;   //记录是否为相同的点

                //记录下当前鼠标指针的坐标
                if ( isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    //若当前位置仍与上一次关键点位置最近则不画线
                    if ( nearestPos != linePointsList.last() ) {
                        linePointsList << nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        linePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                linePointsList.removeLast();
                                linePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (linePointsList.size() > 2 &&
                                linePointsList.last() == linePointsList.at(linePointsList.size() - 2)) {
                            isSamePos = true;
                        }

                    } else {
                        linePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    linePointsList << this->mapToScene( this->mouseViewPos );
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                }
            }

            //检测是否取消绘制
            if (isLineFinish == true) {
                lineStatus = DRAWING_LINE_COMPLETE;
            }

            break;
        }
        //完成绘制
        case DRAWING_LINE_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isLineFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //只有一个点, 不能绘制
            if (linePointsList.size() <= 1) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_LINE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制曲线 */
void CanvasViewDesigner::drawCurve(void)
{
    switch (curveStatus) {

        //等待鼠标第一次按下
        case DRAWING_CURVE_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                curveStatus = DRAWING_CURVE_MOVING;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    curvePointsList << nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        curvePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                curvePointsList.removeLast();
                                curvePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        curvePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    curvePointsList << this->mapToScene( this->mouseViewPos );
                }
            }

            if (isCurveFinish == true) {
                curveStatus = DRAWING_CURVE_COMPLETE;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_CURVE_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            if (curvePointsList.size() == 1) {

                //画直线
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != curvePointsList[0]) {
                        painter.drawLine( this->mapFromScene( curvePointsList[0] ),
                                this->mapFromScene(nearestPos) );
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                        painter.drawLine( this->mapFromScene( curvePointsList[0] ),
                                          this->mapFromScene( nearPos ) );

                    } else {
                        painter.drawLine( this->mapFromScene( curvePointsList[0] ),
                                          this->mouseViewPos);
                    }

                } else {
                    painter.drawLine( this->mapFromScene( curvePointsList[0] ), this->mouseViewPos);
                }
            } else {
                QList<QPointF> curvePointsListTmp;
                curvePointsListTmp.clear();
                for (int i = 0; i < curvePointsList.size(); i++) {
                    curvePointsListTmp.append( this->mapFromScene( curvePointsList.at(i) ) );
                }

                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != curvePointsList.last()) {
                        curvePointsListTmp << this->mapFromScene(nearestPos);
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        curvePointsListTmp << this->mapFromScene(listTmp.at(0));
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                curvePointsListTmp.removeLast();
                                curvePointsListTmp << this->mapFromScene(listTmp.at(i));
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        curvePointsListTmp << this->mouseViewPos;
                    }

                } else {
                    curvePointsListTmp << this->mouseViewPos;
                }

                //生成曲线path
                QPainterPath curvePath = SmoothCurveGenerator::generateSmoothCurve(curvePointsListTmp);
                //绘制
                painter.drawPath(curvePath);
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;    //记录点击时是否位置相同

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != curvePointsList.last()) {
                        curvePointsList << nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        curvePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                curvePointsList.removeLast();
                                curvePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (curvePointsList.size() > 2 &&
                                curvePointsList.last() == curvePointsList.at(curvePointsList.size() - 2)) {
                            isSamePos = true;
                        }

                    } else {
                        curvePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    curvePointsList << this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                }
            }

            //检测是否取消绘制
            if (isCurveFinish == true) {
                curveStatus = DRAWING_CURVE_COMPLETE;
            }

            break;
        }
        //完成绘制
        case DRAWING_CURVE_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isCurveFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //曲线至少需要3个点
            if (curvePointsList.size() <= 2) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_CURVE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制弧线 */
void CanvasViewDesigner::drawArc(void)
{
    //弧上其他两个点
    static QPointF arcSecondPoint, arcThirdPoint;
    switch (arcStatus) {

        //等待第一次鼠标按下
        case DRAWING_ARC_WAIT_FIRST_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    arcFirstPoint =  MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        arcFirstPoint = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                arcFirstPoint = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        arcFirstPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    arcFirstPoint = this->mapToScene(this->mouseViewPos);
                }

                //切换状态
                arcStatus = DRAWING_ARC_MOVING_FIRST;
            }

            if (isArcFinish == true) {
                arcStatus = DRAWING_ARC_COMPLETE;
            }

            break;
        }
        //第一次按下后鼠标移动中
        case DRAWING_ARC_MOVING_FIRST: {

            //看看之前有没有已经绘制好的弧线
            for (int i = 0; i < arcPointsList.size(); i += 3) {
                drawArcPath(arcPointsList.at(i), arcPointsList.at(i+1), arcPointsList.at(i+2));
            }

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                if (nearestPos != arcFirstPoint) {
                    painter.drawLine( this->mapFromScene(arcFirstPoint),
                                      this->mapFromScene(nearestPos) );
                }

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }

                    if (nearPos != arcFirstPoint) {
                        painter.drawLine( this->mapFromScene( arcFirstPoint ),
                                          this->mapFromScene( nearPos ) );
                    }

                } else {
                    painter.drawLine( this->mapFromScene( arcFirstPoint ),
                                      this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene(arcFirstPoint), this->mouseViewPos);
            }

            //与此同时检测鼠标第二次按下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;    //记录点击时是否位置相同

                //记录第二个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != arcFirstPoint) {
                        arcSecondPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearPos != arcFirstPoint) {
                            arcSecondPoint = nearPos;
                        } else {
                            isSamePos = true;
                        }

                    } else {
                        arcSecondPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    arcSecondPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                    //状态切换
                    arcStatus = DRAWING_ARC_MOVING_SECOND;
                }
            }

            //检测是否要结束绘制
            if (isArcFinish == true) {
                arcStatus = DRAWING_ARC_COMPLETE;
            }
            break;
        }
        //第二次按下后鼠标移动中
        case DRAWING_ARC_MOVING_SECOND: {

            QPointF thirdPos;
            bool isSamePos = false;       //记录位置是否与前一点击位置相同
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                if (nearestPos != arcSecondPoint) {
                    thirdPos = nearestPos;
                } else {
                    isSamePos = true;
                }

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }

                    if (nearPos != arcSecondPoint) {
                        thirdPos = nearPos;
                    } else {
                        isSamePos = true;
                    }

                } else {
                    thirdPos = this->mapToScene(this->mouseViewPos);
                }

            } else {
                thirdPos = this->mapToScene(this->mouseViewPos);
            }

            //看看之前有没有已经绘制好的弧线
            for (int i = 0; i < arcPointsList.size(); i += 3) {
                drawArcPath(arcPointsList.at(i), arcPointsList.at(i+1), arcPointsList.at(i+2));
            }

            if (isSamePos == false) {

                drawArcPath(arcFirstPoint, arcSecondPoint, thirdPos);

                //与此同时检测鼠标第三次按下
                if (this->isDrawingLeftBtnClicked == true) {
                    this->isDrawingLeftBtnClicked = false;

                    //记录第三个点
                    if (isLocateToGrid == true) {

                        //定位到网格
                        QPointF scenePos = this->mapToScene(this->mouseViewPos);
                        QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                        arcThirdPoint = nearestPos;

                    } else if (isLocateToStitch == true || isLocateToOutline == true) {

                        //定位到针迹点或轮廓线
                        QPointF mousePos = this->mapToScene(this->mouseViewPos);
                        int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                        if (mapStitch.contains(key)) {

                            QList<QPointF> listTmp = mapStitch[key];
                            arcThirdPoint = listTmp.at(0);
                            qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                            for (int i = 1; i < listTmp.size(); i++) {
                                qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                                if (distance > tmpDistance) {
                                    arcThirdPoint = listTmp.at(i);
                                    distance = tmpDistance;
                                }
                            }
                        } else {
                            arcThirdPoint = this->mapToScene(this->mouseViewPos);
                        }

                    } else {
                        arcThirdPoint = this->mapToScene(this->mouseViewPos);
                    }

                    //保存
                    arcPointsList << arcFirstPoint << arcSecondPoint << arcThirdPoint;

                    //重设第一个点为本次画弧的尾点
                    arcFirstPoint = arcThirdPoint;

                    //状态切换
                    arcStatus = DRAWING_ARC_MOVING_FIRST;
                }
            }

            //检测是否要结束绘制
            if (isArcFinish == true) {
                arcStatus = DRAWING_ARC_COMPLETE;
            }
            break;
        }

        //鼠标第三次按下
        case DRAWING_ARC_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isArcFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //至少要3个点
            if (arcPointsList.size() < 3) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_ARC);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制弧线路径 */
void CanvasViewDesigner::drawArcPath(QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint)
{
    QPainter painter(this->viewport());
    QPen pen = this->pen;
    painter.setPen(pen);

    QPainterPath path;
    QPoint firstPos = this->mapFromScene(firstPoint);
    QPoint secondPos = this->mapFromScene(secondPoint);
    QPoint thirdPos = this->mapFromScene(thirdPoint);
    qreal x1 = firstPos.x();
    qreal y1 = firstPos.y();
    qreal x2 = secondPos.x();
    qreal y2 = secondPos.y();
    qreal x3 = thirdPos.x();
    qreal y3 = thirdPos.y();

    QRect rect;
    qreal  startangle;
    qreal endangle;

    qreal a, b, e, r;
    qreal x, y;
    a = (x1 + x2) * (x1 - x2) + (y1 + y2) * (y1 - y2);
    b = (x3 + x2) * (x3 - x2) + (y3 + y2) * (y3 - y2);
    e = (x1 - x2) * (y3 - y2) - (x2 - x3) * (y2 - y1);
    x = (a * (y3 - y2) + b * (y2 - y1)) / (2 * e);
    y = (a * (x2 - x3) + b * (x1 - x2)) / (2 * e);
    r = sqrt((x2 - x) * (x2 - x) + (y2 - y) * (y2 - y));

    qreal OA = atan2(-y1 + y,x1 - x)*(180/atan2(0,-1));
    qreal OC = atan2(-y3 + y,x3 - x)*(180/atan2(0,-1));
    qreal OB = atan2(-y2 + y,x2 - x)*(180/atan2(0,-1));

    qreal delta113;
    if(OA < OC)
    {
        delta113 = OC - OA;
    }else
    {
        delta113 = OC - OA + 360;
    }
    qreal deltal12;
    if( OA < OB)
    {
        deltal12 = OB - OA;
    }else
    {
        deltal12 = OB - OA + 360;
    }

    if(delta113 > deltal12)
    {
        if(OC > OA)
        {
            startangle = OA;
            endangle = OC - OA;
        }else
        {
            startangle = OA;
            endangle = OC - OA + 360;
        }
    }else
    {
        if(OA > OC)
        {
            startangle = OC;
            endangle = OA - OC;
        }else
        {
            startangle = OC;
            endangle = OA - OC + 360;
        }
    }
    rect = QRect(QPoint(x - r, y - r), QPoint(x + r, y + r));
    path.arcMoveTo(rect, startangle);
    path.arcTo(rect, startangle, endangle);
    painter.drawPath(path);
}

/* 绘制多边形 */
void CanvasViewDesigner::drawPolygon(void)
{
    switch (polygonStatus) {

        //等待鼠标第一次按下
        case DRAWING_POLYGON_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                polygonStatus = DRAWING_POLYGON_MOVING;
                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    polygonPointsList << nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        polygonPointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                polygonPointsList.removeLast();
                                polygonPointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        polygonPointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    polygonPointsList << this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isPolygonFinish == true) {
                polygonStatus = DRAWING_POLYGON_COMPLETE;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_POLYGON_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //先绘制之前的线
            for (int i = 0; i < polygonPointsList.size() - 1; i++) {
                painter.drawLine( this->mapFromScene(polygonPointsList.at(i)),
                                  this->mapFromScene(polygonPointsList.at(i + 1)) );
            }
            //再绘制这次的
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawLine( this->mapFromScene(polygonPointsList.last()),
                                  this->mapFromScene(nearestPos) );

                if (polygonPointsList.size() > 1) {
                    painter.drawLine( this->mapFromScene(polygonPointsList.first()),
                                      this->mapFromScene(nearestPos) );
                }

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }

                    painter.drawLine( this->mapFromScene(polygonPointsList.last()),
                                      this->mapFromScene(nearPos) );
                    if (polygonPointsList.size() > 1) {
                        painter.drawLine( this->mapFromScene(polygonPointsList.first()),
                                          this->mapFromScene(nearPos) );
                    }

                } else {

                    painter.drawLine( this->mapFromScene(polygonPointsList.last()), this->mouseViewPos);
                    if (polygonPointsList.size() > 1) {
                        painter.drawLine( this->mapFromScene(polygonPointsList.first()), this->mouseViewPos);
                    }
                }

            } else {

                painter.drawLine( this->mapFromScene(polygonPointsList.last()), this->mouseViewPos);
                if (polygonPointsList.size() > 1) {
                    painter.drawLine( this->mapFromScene(polygonPointsList.first()), this->mouseViewPos);
                }
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;      //记录是否与前一个点位置相同

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    if (polygonPointsList.last() != nearestPos) {
                        polygonPointsList << nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearPos != polygonPointsList.last()) {
                            polygonPointsList << nearPos;
                        } else {
                            isSamePos = true;
                        }

                    } else {
                        polygonPointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    polygonPointsList << this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                }
            }

            //检测是否取消绘制
            if (isPolygonFinish == true) {
                polygonStatus = DRAWING_POLYGON_COMPLETE;
            }

            break;
        }
        //完成绘制
        case DRAWING_POLYGON_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isPolygonFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //只有两个以内的点, 不能绘制
            if (polygonPointsList.size() <= 2) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_POLYGON);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制矩形 */
void CanvasViewDesigner::drawRectangle(void)
{
    switch (rectangleStatus) {

        //等待鼠标第一次按下
        case DRAWING_RECT_WAIT_FIRST_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                rectangleStatus = DRAWING_RECT_MOVING;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    rectangleFirstPoint = nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        rectangleFirstPoint = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                rectangleFirstPoint = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        rectangleFirstPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    rectangleFirstPoint = this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isRectFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isRectFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                return;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_RECT_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawRect( QRect( this->mapFromScene(rectangleFirstPoint) ,
                                         this->mapFromScene(nearestPos) ) );

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawRect( QRect( this->mapFromScene(rectangleFirstPoint) ,
                                             this->mapFromScene(nearestPos) ) );
                } else {
                    painter.drawRect(QRect( this->mapFromScene(rectangleFirstPoint), this->mouseViewPos));
                }

            } else {
                painter.drawRect(QRect( this->mapFromScene(rectangleFirstPoint), this->mouseViewPos));
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;              //记录是否与前一个点位置相同

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (rectangleFirstPoint != nearestPos) {
                        rectangleSecondPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                        if (rectangleFirstPoint != nearestPos) {
                            rectangleSecondPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }

                    } else {
                        rectangleSecondPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    rectangleSecondPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;

                    //状态切换
                    rectangleStatus = DRAWING_RECT_SECOND_CLIKCKED;

                    //规避同一条直线异常
                    if (rectangleSecondPoint.x() == rectangleFirstPoint.x()) {
                        rectangleSecondPoint.setX(rectangleSecondPoint.x() + 1);
                    }
                    if (rectangleSecondPoint.y() == rectangleFirstPoint.y()) {
                        rectangleSecondPoint.setY(rectangleSecondPoint.y() + 1);
                    }
                }
            }

            //检测是否取消绘制
            if (isRectFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isRectFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //鼠标第二次按下
        case DRAWING_RECT_SECOND_CLIKCKED: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);
            painter.drawRect( QRect( this->mapFromScene(rectangleFirstPoint),
                                     this->mapFromScene(rectangleSecondPoint) ) );

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isRectFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_RECTANGLE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制三点圆 */
void CanvasViewDesigner::drawThreePointCircle(void)
{
    switch (_3pCircleStatus) {

        //等待第一次鼠标按下
        case DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    _3pCircleFirstPoint = nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        _3pCircleFirstPoint = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                _3pCircleFirstPoint = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        _3pCircleFirstPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    _3pCircleFirstPoint = this->mapToScene(this->mouseViewPos);
                }

                //切换状态
                _3pCircleStatus = DRAWING_3P_CIRCLE_MOVING_FIRST;
            }

            //检测是否取消绘制
            if (is3PCircleFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                _3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->is3PCircleFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //第一次按下后鼠标移动中
        case DRAWING_3P_CIRCLE_MOVING_FIRST: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawLine( this->mapFromScene(_3pCircleFirstPoint),
                                  this->mapFromScene(nearestPos) );

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawLine( this->mapFromScene(_3pCircleFirstPoint),
                                      this->mapFromScene(nearestPos) );
                } else {
                    painter.drawLine( this->mapFromScene(_3pCircleFirstPoint), this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene(_3pCircleFirstPoint), this->mouseViewPos);
            }

            //与此同时检测鼠标第二次按下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录第二个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != _3pCircleFirstPoint) {
                        _3pCircleSecondPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != _3pCircleFirstPoint) {
                            _3pCircleSecondPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }
                    } else {
                        _3pCircleSecondPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    _3pCircleSecondPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                    //状态切换
                    _3pCircleStatus = DRAWING_3P_CIRCLE_MOVING_SECOND;
                }
            }

            //检测是否取消绘制
            if (is3PCircleFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                _3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->is3PCircleFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //第二次按下后鼠标移动中
        case DRAWING_3P_CIRCLE_MOVING_SECOND: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //根据圆上三点, 获取圆心坐标和半径
            QPointF center; qreal r;
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                MathUtils::getCircleCenterAndRadius(
                            this->mapFromScene(_3pCircleFirstPoint), this->mapFromScene(_3pCircleSecondPoint),
                            this->mapFromScene(nearestPos), &center, &r);

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    MathUtils::getCircleCenterAndRadius(
                                this->mapFromScene(_3pCircleFirstPoint), this->mapFromScene(_3pCircleSecondPoint),
                                this->mapFromScene(nearestPos), &center, &r);
                } else {
                    MathUtils::getCircleCenterAndRadius(
                                this->mapFromScene(_3pCircleFirstPoint), this->mapFromScene(_3pCircleSecondPoint),
                                this->mouseViewPos, &center, &r);
                }

            } else {
                MathUtils::getCircleCenterAndRadius(
                            this->mapFromScene(_3pCircleFirstPoint), this->mapFromScene(_3pCircleSecondPoint),
                            this->mouseViewPos, &center, &r);
            }
            painter.drawEllipse(center, r, r);

            //与此同时检测鼠标第三次按下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录第三个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != _3pCircleSecondPoint) {
                        _3pCircleThirdPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != _3pCircleSecondPoint) {
                            _3pCircleThirdPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }
                    } else {
                        _3pCircleThirdPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    _3pCircleThirdPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                    //状态切换
                    _3pCircleStatus = DRAWING_3P_CIRCLE_THIRD_CLIKCKED;
                }
            }

            //检测是否取消绘制
            if (is3PCircleFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                _3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->is3PCircleFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }

        //鼠标第三次按下
        case DRAWING_3P_CIRCLE_THIRD_CLIKCKED: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //根据圆上三点, 获取圆心坐标和半径
            QPointF center; qreal r;
            MathUtils::getCircleCenterAndRadius(
                        this->mapFromScene(_3pCircleFirstPoint), this->mapFromScene(_3pCircleSecondPoint),
                        this->mapFromScene(_3pCircleThirdPoint), &center, &r);
            painter.drawEllipse(center, r, r);

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            _3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->is3PCircleFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_3P_CIRCLE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制中心圆 */
void CanvasViewDesigner::drawCenterCircle(void)
{
    switch (centerCircleStatus) {

        //等待鼠标第一次按下
        case DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                centerCircleStatus = DRAWING_CENTER_CIRCLE_MOVING;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    centerCircleFirstPoint = nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        centerCircleFirstPoint = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                centerCircleFirstPoint = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        centerCircleFirstPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    centerCircleFirstPoint = this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isCenterCircleFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isCenterCircleFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_CENTER_CIRCLE_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            qreal radius;  //圆半径
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                radius = MathUtils::getDistance2D( this->mapFromScene(centerCircleFirstPoint),
                                                         this->mapFromScene(nearestPos));
            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    radius = MathUtils::getDistance2D( this->mapFromScene(centerCircleFirstPoint),
                                                             this->mapFromScene(nearestPos));
                } else {
                    radius = MathUtils::getDistance2D( this->mapFromScene(centerCircleFirstPoint),
                                                       this->mouseViewPos);
                }

            } else {
                radius = MathUtils::getDistance2D( this->mapFromScene(centerCircleFirstPoint),
                                                   this->mouseViewPos);
            }
            painter.drawEllipse( this->mapFromScene(centerCircleFirstPoint),
                                 static_cast<int>(radius), static_cast<int>(radius));

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录第二个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != centerCircleFirstPoint) {
                        centerCircleSecondPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != centerCircleFirstPoint) {
                            centerCircleSecondPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }
                    } else {
                        centerCircleSecondPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    centerCircleSecondPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                    //状态切换
                    centerCircleStatus = DRAWING_CENTER_CIRCLE_SECOND_CLIKCKED;
                }
            }

            //检测是否取消绘制
            if (isCenterCircleFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isCenterCircleFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //鼠标第二次按下
        case DRAWING_CENTER_CIRCLE_SECOND_CLIKCKED: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);
            qreal radius = MathUtils::getDistance2D( this->mapFromScene(centerCircleFirstPoint),
                                                     this->mapFromScene(centerCircleSecondPoint) );
            painter.drawEllipse( this->mapFromScene(centerCircleFirstPoint),
                                 static_cast<int>(radius), static_cast<int>(radius));

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isCenterCircleFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_CENTER_CIRCLE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制椭圆 */
void CanvasViewDesigner::drawEllipse(void)
{
    switch (ellipseStatus) {

        //等待第一次鼠标按下
        case DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    ellipseFirstPoint = nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        ellipseFirstPoint = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                ellipseFirstPoint = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        ellipseFirstPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    ellipseFirstPoint = this->mapToScene(this->mouseViewPos);
                }

                //切换状态
                ellipseStatus = DRAWING_ELLIPSE_MOVING_FIRST;
            }

            //检测是否取消绘制
            if (isEllipseFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isEllipseFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //第一次按下后鼠标移动中
        case DRAWING_ELLIPSE_MOVING_FIRST: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //记录下当前鼠标指针的坐标
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawLine( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(nearestPos));

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawLine( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(nearestPos));
                } else {
                    painter.drawLine( this->mapFromScene(ellipseFirstPoint), this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene(ellipseFirstPoint), this->mouseViewPos);
            }

            //与此同时检测鼠标第二次按下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录第二个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != ellipseFirstPoint) {
                        ellipseSecondPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != ellipseFirstPoint) {
                            ellipseSecondPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }
                    } else {
                        ellipseSecondPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    ellipseSecondPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;

                    //规避水平线和竖直线风险
                    if (ellipseFirstPoint.y() == ellipseSecondPoint.y()) {
                        ellipseSecondPoint.setY(ellipseSecondPoint.y() + 1);
                    }
                    if (ellipseFirstPoint.x() == ellipseSecondPoint.x()) {
                        ellipseSecondPoint.setX(ellipseSecondPoint.x() + 1);
                    }

                    //↖这种情况要单独处理一下
                    if (ellipseFirstPoint.x() > ellipseSecondPoint.x() &&
                        ellipseFirstPoint.y() > ellipseSecondPoint.y()) {
                        //交换第一个点和第二个点
                        QPointF pSwap;
                        pSwap = ellipseFirstPoint;
                        ellipseFirstPoint = ellipseSecondPoint;
                        ellipseSecondPoint = pSwap;
                    }

                    //状态切换
                    ellipseStatus = DRAWING_ELLIPSE_MOVING_SECOND;
                }
            }

            //检测是否取消绘制
            if (isEllipseFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isEllipseFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //第二次按下后鼠标移动中
        case DRAWING_ELLIPSE_MOVING_SECOND: {
            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //计算椭圆中心和长轴短轴
            QPointF center; qreal rx, ry;
            center = MathUtils::getMiddlePointCoord( QLineF( this->mapFromScene(ellipseFirstPoint),
                                                             this->mapFromScene(ellipseSecondPoint) ) );
            rx = MathUtils::getDistance2D(center, this->mapFromScene(ellipseSecondPoint));
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                ry = MathUtils::getDistanceBetweenPointAndLine(
                        QLineF( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(ellipseSecondPoint) ),
                        this->mapFromScene(nearestPos));

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    ry = MathUtils::getDistanceBetweenPointAndLine(
                            QLineF( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(ellipseSecondPoint) ),
                            this->mapFromScene(nearestPos));
                } else {
                    ry = MathUtils::getDistanceBetweenPointAndLine(
                            QLineF( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(ellipseSecondPoint) ),
                            this->mouseViewPos);
                }

            } else {
                ry = MathUtils::getDistanceBetweenPointAndLine(
                        QLineF( this->mapFromScene(ellipseFirstPoint), this->mapFromScene(ellipseSecondPoint) ),
                        this->mouseViewPos);
            }

            //计算角度
            bool slopePositive;
            qreal angleDegree = MathUtils::getLineAngleWithHorizontalDirection(
                        QLineF(center, this->mapFromScene(ellipseSecondPoint) ), MathUtils::eDegree, &slopePositive);
            painter.resetTransform();
            painter.translate(center);
            if (slopePositive == true) { //斜率为正数
                painter.rotate(-angleDegree);
            } else { //斜率为负数
                painter.rotate(angleDegree);
            }

            painter.drawEllipse(QPoint(0 , 0), static_cast<int>(rx), static_cast<int>(ry));
            painter.resetTransform();

            //与此同时检测鼠标第三次按下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录第三个点
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != ellipseSecondPoint) {
                        ellipseThirdPoint = nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != ellipseSecondPoint) {
                            ellipseThirdPoint = nearestPos;
                        } else {
                            isSamePos = true;
                        }
                    } else {
                        ellipseThirdPoint = this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    ellipseThirdPoint = this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                    //状态切换
                    ellipseStatus = DRAWING_ELLIPSE_THIRD_CLIKCKED;
                }
            }

            //检测是否取消绘制
            if (isEllipseFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isEllipseFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }

        //鼠标第三次按下
        case DRAWING_ELLIPSE_THIRD_CLIKCKED: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isEllipseFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_ELLIPSE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制空线 */
void CanvasViewDesigner::drawEmptyLine(void)
{
    switch (emptyLineStatus) {

        //等待鼠标第一次按下
        case DRAWING_EMPTY_LINE_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                emptyLineStatus = DRAWING_EMPTY_LINE_MOVING;
                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    emptyLinePointsList << nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        emptyLinePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                emptyLinePointsList.removeLast();
                                emptyLinePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        emptyLinePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    emptyLinePointsList << this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isEmptyLineFinish == true) {
                emptyLineStatus = DRAWING_EMPTY_LINE_COMPLETE;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_EMPTY_LINE_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //先绘制之前的线
            for (int i = 0; i < emptyLinePointsList.size() - 1; i++) {
                painter.drawLine( this->mapFromScene( emptyLinePointsList.at(i) ),
                                  this->mapFromScene( emptyLinePointsList.at(i + 1) ) );
            }
            //再绘制这次的
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawLine( this->mapFromScene( emptyLinePointsList.last() ), this->mapFromScene(nearestPos));

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawLine( this->mapFromScene( emptyLinePointsList.last() ),
                                      this->mapFromScene(nearestPos));
                } else {
                    painter.drawLine( this->mapFromScene( emptyLinePointsList.last() ), this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene( emptyLinePointsList.last() ), this->mouseViewPos);
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != emptyLinePointsList.last()) {
                        emptyLinePointsList << nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != emptyLinePointsList.last()) {
                            emptyLinePointsList << nearestPos;
                        } else {
                            isSamePos = true;
                        }

                    } else {
                        emptyLinePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    emptyLinePointsList << this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                }
            }

            //检测是否取消绘制
            if (isEmptyLineFinish == true) {
                emptyLineStatus = DRAWING_EMPTY_LINE_COMPLETE;
            }

            break;
        }
        //完成绘制
        case DRAWING_EMPTY_LINE_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            emptyLineStatus = DRAWING_EMPTY_LINE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isEmptyLineFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //只有一个点, 不能绘制
            if (emptyLinePointsList.size() <= 1) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_EMPTY_LINE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制发送服装线 */
void CanvasViewDesigner::drawSendClothesLine(void)
{
    switch (sendClothesLineStatus) {

        //等待鼠标第一次按下
        case DRAWING_SEND_CLOTHES_LINE_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                sendClothesLineStatus = DRAWING_SEND_CLOTHES_LINE_MOVING;

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                    sendClothesLinePointsList << nearestPos;

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        sendClothesLinePointsList << listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                sendClothesLinePointsList.removeLast();
                                sendClothesLinePointsList << listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }
                    } else {
                        sendClothesLinePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    sendClothesLinePointsList << this->mapToScene(this->mouseViewPos);
                }
            }

            //检测是否取消绘制
            if (isSendClothesLineFinish == true) {
                sendClothesLineStatus = DRAWING_SEND_CLOTHES_LINE_COMPLETE;
            }

            break;
        }
        //鼠标移动中
        case DRAWING_SEND_CLOTHES_LINE_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);

            //先绘制之前的线
            for (int i = 0; i < sendClothesLinePointsList.size() - 1; i++) {
                painter.drawLine( this->mapFromScene( sendClothesLinePointsList.at(i) ),
                                  this->mapFromScene( sendClothesLinePointsList.at(i + 1) ) );
            }
            //再绘制这次的
            if (isLocateToGrid == true) {

                //定位到网格
                QPointF scenePos = this->mapToScene(this->mouseViewPos);
                QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);
                painter.drawLine( this->mapFromScene( sendClothesLinePointsList.last() ),
                                  this->mapFromScene(nearestPos));

            } else if (isLocateToStitch == true || isLocateToOutline == true) {

                //定位到针迹点或轮廓线
                QPointF mousePos = this->mapToScene(this->mouseViewPos);
                int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                if (mapStitch.contains(key)) {

                    QList<QPointF> listTmp = mapStitch[key];
                    QPointF nearestPos = listTmp.at(0);
                    qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                    for (int i = 1; i < listTmp.size(); i++) {
                        qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                        if (distance > tmpDistance) {
                            nearestPos = listTmp.at(i);
                            distance = tmpDistance;
                        }
                    }
                    painter.drawLine( this->mapFromScene( sendClothesLinePointsList.last() ),
                                      this->mapFromScene(nearestPos));
                } else {
                    painter.drawLine( this->mapFromScene( sendClothesLinePointsList.last() ), this->mouseViewPos);
                }

            } else {
                painter.drawLine( this->mapFromScene( sendClothesLinePointsList.last() ), this->mouseViewPos);
            }

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                bool isSamePos = false;       //记录是否与前一个点位置相同

                //记录下当前鼠标指针的坐标
                if (isLocateToGrid == true) {

                    //定位到网格
                    QPointF scenePos = this->mapToScene(this->mouseViewPos);
                    QPointF nearestPos = MathUtils::getNearestPointInGrid(scenePos, gridClearance);

                    if (nearestPos != sendClothesLinePointsList.last()) {
                        sendClothesLinePointsList << nearestPos;
                    } else {
                        isSamePos = true;
                    }

                } else if (isLocateToStitch == true || isLocateToOutline == true) {

                    //定位到针迹点或轮廓线
                    QPointF mousePos = this->mapToScene(this->mouseViewPos);
                    int key = (int)mousePos.x() / 10 * 1000000 - (int)mousePos.y() / 10;
                    if (mapStitch.contains(key)) {

                        QList<QPointF> listTmp = mapStitch[key];
                        QPointF nearestPos = listTmp.at(0);
                        qreal distance = MathUtils::getDistance2D(mousePos, listTmp.at(0));
                        for (int i = 1; i < listTmp.size(); i++) {
                            qreal tmpDistance = MathUtils::getDistance2D(mousePos, listTmp.at(i));
                            if (distance > tmpDistance) {
                                nearestPos = listTmp.at(i);
                                distance = tmpDistance;
                            }
                        }

                        if (nearestPos != sendClothesLinePointsList.last()) {
                            sendClothesLinePointsList << nearestPos;
                        } else {
                            isSamePos = true;
                        }

                    } else {
                        sendClothesLinePointsList << this->mapToScene(this->mouseViewPos);
                    }

                } else {
                    sendClothesLinePointsList << this->mapToScene(this->mouseViewPos);
                }

                if (isSamePos == false) {
                    this->isDrawingLeftBtnClicked = false;
                }
            }

            //检测是否取消绘制
            if (isSendClothesLineFinish == true) {
                sendClothesLineStatus = DRAWING_SEND_CLOTHES_LINE_COMPLETE;
            }

            break;
        }
        //完成绘制
        case DRAWING_SEND_CLOTHES_LINE_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            sendClothesLineStatus = DRAWING_SEND_CLOTHES_LINE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isSendClothesLineFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //只有一个点, 不能绘制
            if (sendClothesLinePointsList.size() <= 1) {
                return;
            }

            //把view上的数据dump到interface里
            dumpViewToInterface(NOW_DRAWING_SENDING_CLOTHES_LINE);

            //再把interface里的数据dump到scene里
            dumpInterfaceToScene();

            break;
        }
        default: break;
    }
}

/* 绘制距离测量直线 */
void CanvasViewDesigner::drawDistanceMeasurementLine(void)
{
    static QPoint firstPoint, secondPoint;
    switch (measurementLineStatus) {

        //等待鼠标第一次按下
        case DISTANCE_MEASUREMENT_WAIT_CLIKCKED: {
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //状态切换至鼠标移动状态
                measurementLineStatus = DISTANCE_MEASUREMENT_MOVING;
                //记录下当前鼠标指针的坐标
                firstPoint = this->mouseViewPos;
            }

            //检测是否取消绘制
            if (isMeasurementLineFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                measurementLineStatus = DISTANCE_MEASUREMENT_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isMeasurementLineFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //鼠标移动中
        case DISTANCE_MEASUREMENT_MOVING: {

            QPainter painter(this->viewport());
            QPen pen = this->pen;
            painter.setPen(pen);
            painter.drawLine(firstPoint, this->mouseViewPos);

            //移动过程中又点击了一下
            if (this->isDrawingLeftBtnClicked == true) {
                this->isDrawingLeftBtnClicked = false;

                //记录下当前鼠标指针的坐标
                secondPoint = this->mouseViewPos;
                measurementLineStatus = DISTANCE_MEASUREMENT_COMPLETE;
            }

            //检测是否取消绘制
            if (isMeasurementLineFinish == true) {
                this->isDrawingLeftBtnClicked = false;
                measurementLineStatus = DISTANCE_MEASUREMENT_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_NONE;
                this->isMeasurementLineFinish = false;
                this->selectionMode = SELECTION_MODE_ITEM;
                this->setCursor(Qt::ArrowCursor);
                designWindow->setLeftPanelBtnChosenQss(1);
                stopStateMachine();
                this->update();
                return;
            }

            break;
        }
        //完成绘制
        case DISTANCE_MEASUREMENT_COMPLETE: {

            //恢复状态
            this->isDrawingLeftBtnClicked = false;
            measurementLineStatus = DISTANCE_MEASUREMENT_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_NONE;
            this->isMeasurementLineFinish = false;
            this->selectionMode = SELECTION_MODE_ITEM;
            this->setCursor(Qt::ArrowCursor);
            designWindow->setLeftPanelBtnChosenQss(1);

            //停止状态机, 降低cpu占用率
            stopStateMachine();
            this->update();

            //计算距离
            QPointF p1 = this->mapToScene(firstPoint), p2 = this->mapToScene(secondPoint);
            qreal distance = MathUtils::getDistance2D(p1, p2);
            //显示测量距离
            QMessageBox::information(this, tr("SewMake数控编制软件"), QString(tr("测量距离为：%1 mm")).arg(distance));

            break;
        }
        default: break;
    }
}

/* 把view上的数据dump到interface里 */
void CanvasViewDesigner::dumpViewToInterface(Enum_NowDrawing_t type)
{
    //状态压栈
    saveGraphicState();
    isInterfaceChanged = true;

    switch (type) {
        //现在什么也没画
        case NOW_DRAWING_NONE: {
            break;
        }
        //正在绘制中心圆
        case NOW_DRAWING_CENTER_CIRCLE: {

            //在当前图层里绘制一个中心圆
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stCenterCircle);                  //创建线性

            //坐标映射
            interface->ArmAppendPunchPoint(centerCircleFirstPoint.x(), -centerCircleFirstPoint.y());    //圆心
            interface->ArmAppendPunchPoint(centerCircleSecondPoint.x(), -centerCircleSecondPoint.y());  //鼠标确定半径的点
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制三点圆
        case NOW_DRAWING_3P_CIRCLE: {
            //在当前图层里绘制一个三点圆
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stPtCircle);

            //坐标映射
            interface->ArmAppendPunchPoint(_3pCircleFirstPoint.x(), -_3pCircleFirstPoint.y());
            interface->ArmAppendPunchPoint(_3pCircleSecondPoint.x(), -_3pCircleSecondPoint.y());
            interface->ArmAppendPunchPoint(_3pCircleThirdPoint.x(), -_3pCircleThirdPoint.y());
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制椭圆
        case NOW_DRAWING_ELLIPSE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stEllipse);

            //坐标映射
            interface->ArmAppendPunchPoint(ellipseFirstPoint.x(), -ellipseFirstPoint.y());
            interface->ArmAppendPunchPoint(ellipseSecondPoint.x(), -ellipseSecondPoint.y());
            interface->ArmAppendPunchPoint(ellipseThirdPoint.x(), -ellipseThirdPoint.y());
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制矩形
        case NOW_DRAWING_RECTANGLE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stRect);

            //坐标映射
            interface->ArmAppendPunchPoint(rectangleFirstPoint.x(), -rectangleFirstPoint.y());   //只需要传入对角的两个点即可
            interface->ArmAppendPunchPoint(rectangleSecondPoint.x(), -rectangleSecondPoint.y());
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制直线
        case NOW_DRAWING_LINE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stLine);

            //坐标映射
            for (int i = 0; i < linePointsList.size(); i++) {
                QPointF pTmp = linePointsList[i];
                interface->ArmAppendPunchPoint(pTmp.x(), -pTmp.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制多边形
        case NOW_DRAWING_POLYGON: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stPolygon);

            //坐标映射
            for (int i = 0; i < polygonPointsList.size(); i++) {
                QPointF pTmp = polygonPointsList[i];
                interface->ArmAppendPunchPoint(pTmp.x(), -pTmp.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制弧线
        case NOW_DRAWING_ARC: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stArc);

            //坐标映射
            QPointF pTmp1 = arcPointsList[0];
            interface->ArmAppendPunchPoint(pTmp1.x(), -pTmp1.y());
            for (int i = 1; i < arcPointsList.size(); i += 3) {
                QPointF pTmp2 = arcPointsList[i];
                QPointF pTmp3 = arcPointsList[i + 1];
                interface->ArmAppendPunchPoint(pTmp2.x(), -pTmp2.y());
                interface->ArmAppendPunchPoint(pTmp3.x(), -pTmp3.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制曲线
        case NOW_DRAWING_CURVE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stCurve);

            //坐标映射
            for (int i = 0; i < curvePointsList.size(); i++) {
                QPointF pTmp;
                pTmp.setX(curvePointsList[i].x());
                pTmp.setY(curvePointsList[i].y());
                interface->ArmAppendPunchPoint(pTmp.x(), -pTmp.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制空线
        case NOW_DRAWING_EMPTY_LINE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stNullLine);

            //坐标映射
            for (int i = 0; i < emptyLinePointsList.size(); i++) {
                QPointF pTmp = emptyLinePointsList[i];
                interface->ArmAppendPunchPoint(pTmp.x(), -pTmp.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }
        //正在绘制发送服装线
        case NOW_DRAWING_SENDING_CLOTHES_LINE: {
            interface->ArmSetCurrentLayer(currentSelectedLayer - 1);
            interface->ArmCreateUint(stNull);

            //坐标映射
            for (int i = 0; i < sendClothesLinePointsList.size(); i++) {
                QPointF pTmp = sendClothesLinePointsList[i];
                interface->ArmAppendPunchPoint(pTmp.x(), -pTmp.y());
            }
            interface->ArmInputUintPunchComplete();
            break;
        }

        default: break;
    }
}

/* 获取所有图层中的图元总数 */
int CanvasViewDesigner::getUnitCountAll(void)
{
    int count = 0;

    int layerCount;
    interface->ArmGetLayerCount(&layerCount);
    for (int i = 0; i < layerCount; i++) {
        count += interface->ArmGetUintCount(i);
    }

    return count;
}

/* 显示缝纫点 */
void CanvasViewDesigner::showStitchPoints(bool isShow)
{
    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        if (itemType != eITEM_DESIGN_STITCH_POINT) {
            continue;
        }

        ItemDesignStitchPoint *itemDesignStitchPoint = dynamic_cast<ItemDesignStitchPoint *>(itemList.at(i));
        if (itemDesignStitchPoint == nullptr) {
            continue;
        }

        if (isShow == true) {
            itemDesignStitchPoint->show();
        } else {
            itemDesignStitchPoint->hide();
        }
    }
}

/* 显示关键点 */
void CanvasViewDesigner::showPunchPoints(bool isShow)
{
    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        if (itemType != eITEM_DESIGN_PUNCH_POINT) {
            continue;
        }

        ItemDesignPunchPoint *itemDesignPunchPoint = dynamic_cast<ItemDesignPunchPoint *>(itemList.at(i));
        if (itemDesignPunchPoint == nullptr) {
            continue;
        }

        if (isShow == true) {
            itemDesignPunchPoint->show();
        } else {
            itemDesignPunchPoint->hide();
        }
    }
}

/* 设置图元是否可选择和移动 */
void CanvasViewDesigner::setDesignItemsSelectableMovable(bool isSelectableMovable)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {

            case eITEM_DESIGN_STITCH_POINT:
            case eITEM_DESIGN_PUNCH_POINT: {
                break;
            }
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE:
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE:
            //椭圆
            case eITEM_DESIGN_ELLIPSE:
            //矩形
            case eITEM_DESIGN_RECTANGLE:
            //直线
            case eITEM_DESIGN_LINE:
            //多边形
            case eITEM_DESIGN_POLYGON:
            //弧线
            case eITEM_DESIGN_ARC:
            //空线
            case eITEM_DESIGN_EMPTY_LINE:
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE:
            //曲线
            case eITEM_DESIGN_CURVE: {
                QGraphicsItem *item = itemList.at(i);
                if (isSelectableMovable == true) {
                    item->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
                } else {
                    item->setFlags(item->flags() & (~(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable)));
                }
                break;
            }
            default: break;
        }
    }

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 把interface里的数据dump到scene里 */
void CanvasViewDesigner::dumpInterfaceToScene(void)
{
    clearScene(); //清空场景

    //若选中定位到轮廓线，则更新缝纫点map映射位置
    mapStitch.clear();
    updateMapStitchByLocateOutline();

    int layerCount;
    this->needleCount = 0;
    interface->ArmGetLayerCount(&layerCount);
    for (int i = 0; i < layerCount; i++) {          //遍历图层

        int unitCount;
        QList<QPointF> pListBetweenUnit; //保存同一个图层中, 各个图元之间的连接虚线
        pListBetweenUnit.clear();
        unitCount = interface->ArmGetUintCount(i);
        for (int j = 0; j < unitCount; j++) {       //遍历图层下的图元
            //记录总针数
            this->needleCount += interface->ArmGetStitchCount(i, j);

            QList<QGraphicsItem *> stitchObjList;  //缝纫点对象列表
            stitchObjList.clear();
            QList<QGraphicsItem *> punchObjList;   //关键点对象列表
            punchObjList.clear();

            //获取该图元的缝纫点
            int stitchPointCount = interface->ArmGetStitchCount(i, j);
            QList<QPointF> pStitchesList;
            pStitchesList.clear();
            for (int k = 0; k < stitchPointCount; k++) {
                qreal tmpX, tmpY;
                interface->ArmGetStitchPoint(i, j, k, &tmpX, &tmpY);
                pStitchesList.append(QPointF(tmpX, -tmpY));
                ItemDesignStitchPoint *itemDesignStitchPoint = this->drawStitchPoint(QPointF(tmpX, -tmpY));
                stitchObjList.append(itemDesignStitchPoint);
                itemDesignStitchPoint->setLayer(i + 1);
                itemDesignStitchPoint->setItemId(j + 1);
                itemDesignStitchPoint->setStitchNum(k + 1);
                if (selectionMode == SELECTION_MODE_STITCH) {
                    itemDesignStitchPoint->show();
                } else {
                    itemDesignStitchPoint->hide();
                }

                //设置缝纫点控制码
                int funcCode;
                interface->ArmCleanSelectObject();
                interface->ArmSelectUintByIndex(1, i, j);
                interface->ArmAddStitchToCurrentSelectStitchList(i, j, k);
                interface->ArmGetSelectStitchFuncode(&funcCode);
                interface->ArmCleanSelectObject();
                itemDesignStitchPoint->setStitchFuncCode(funcCode);
                itemDesignStitchPoint->setIsShowSmallBox(this->isShowStitchSamllBox);

                //更新缝纫点map映射位置
                if (isLocateToStitch == true) {
                    int key = (int)tmpX / 10 * 1000000 + (int)tmpY / 10;
                    QList<QPointF> listPointTmp;
                    if (mapStitch.contains(key)) {
                        listPointTmp = mapStitch[key];
                    }
                    listPointTmp << QPointF(tmpX, -tmpY);
                    mapStitch[key] = listPointTmp;
                }
            }

            //获取该图元的关键点个数
            int punchPointCount = interface->ArmGetPunchCount(i, j);
            QList<QPointF> pList; //存放关键点的list
            pList.clear();
            for (int k = 0; k < punchPointCount; k++) {
                double tmpX, tmpY;
                interface->ArmGetPunchPoint(i, j, k, &tmpX, &tmpY);

                //由于Graphics Scene的y坐标和图形文件里面的是相反的, 这里需要反向一下y
                pList.append(QPointF(tmpX, -tmpY));
                ItemDesignPunchPoint *itemDesignPunchPoint = this->drawPunchPoint(QPointF(tmpX, -tmpY));
                punchObjList.append(itemDesignPunchPoint);
                itemDesignPunchPoint->setLayer(i + 1);
                itemDesignPunchPoint->setItemId(j + 1);
                itemDesignPunchPoint->setPunchPointNum(k + 1);
                if (selectionMode == SELECTION_MODE_PUNCH) {
                    itemDesignPunchPoint->show();
                } else {
                    itemDesignPunchPoint->hide();
                }

                //第一个图元只记录最后一个关键点
                if (j == 0 && k == punchPointCount - 1) {
                    pListBetweenUnit << QPointF(tmpX, -tmpY);
                }
                //最后一个图元只记录第一个关键点
                else if (j == unitCount - 1 && k == 0) {
                    pListBetweenUnit << QPointF(tmpX, -tmpY);
                }
                //中间的图元, 第一个关键点和最后一个关键点都要记录
                else if (0 < j && j < unitCount - 1) {
                    if (k == 0 || k == punchPointCount - 1) {
                        pListBetweenUnit << QPointF(tmpX, -tmpY);
                    }
                }
            }

            //图元类型
            int unitType;
            interface->ArmGetUintTypeByIndex(i, j, &unitType);
            //根据线性绘制图形
            switch (unitType) {
                //中心圆
                case stCenterCircle: {
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                    pList.append(QPointF(tmpX, -tmpY));

                    //确定中心点
                    QPointF firstPoint, secondPoint;
                    if (MathUtils::getDistance2D(pList.at(0), pList.at(2)) < 0.01) {
                        firstPoint = pList[1];
                        secondPoint = pList[0];
                    } else {
                        firstPoint = pList[0];
                        secondPoint = pList[1];
                    }

                    ItemDesignCenterCircle *itemDesignCenterCircle = drawCenterCircleToScene(firstPoint, secondPoint,
                                                                                             pStitchesList, stitchObjList, punchObjList);
                    interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                    itemDesignCenterCircle->setStartSewingPoint(QPointF(tmpX, -tmpY));
                    itemDesignCenterCircle->setLayer(i + 1);
                    itemDesignCenterCircle->setItemId(j + 1);
                    itemDesignCenterCircle->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                    itemDesignCenterCircle->setIsShowOutline(isShowOutline);
                    itemDesignCenterCircle->setIsShowId(isShowItemId);
                    break;
                }
                //三点圆
                case stPtCircle: {
                    QPointF firstPoint, secondPoint, thirdPoint;
                    firstPoint = pList[0];
                    secondPoint = pList[1];
                    thirdPoint = pList[2];
                    ItemDesign3PCircle *itemDesign3PCircle = draw3PCircleToScene(firstPoint, secondPoint, thirdPoint,
                                                                                 pStitchesList, stitchObjList, punchObjList);
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                    itemDesign3PCircle->setStartSewingPoint(QPointF(tmpX, -tmpY));
                    itemDesign3PCircle->setLayer(i + 1);
                    itemDesign3PCircle->setItemId(j + 1);
                    itemDesign3PCircle->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                    itemDesign3PCircle->setIsShowOutline(isShowOutline);
                    itemDesign3PCircle->setIsShowId(isShowItemId);
                    break;
                }
                //椭圆
                case stEllipse: {
                    QPointF firstPoint, secondPoint, thirdPoint;
                    firstPoint = pList[0];
                    secondPoint = pList[1];
                    thirdPoint = pList[2];
                    ItemDesignEllipse *itemDesignEllipse = drawEllipseToScene(firstPoint, secondPoint, thirdPoint,
                                                                              pStitchesList, stitchObjList, punchObjList);
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                    itemDesignEllipse->setStartSewingPoint(QPointF(tmpX, -tmpY));
                    itemDesignEllipse->setLayer(i + 1);
                    itemDesignEllipse->setItemId(j + 1);
                    itemDesignEllipse->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                    itemDesignEllipse->setIsShowOutline(isShowOutline);
                    itemDesignEllipse->setIsShowId(isShowItemId);
                    break;
                }
                //矩形
                case stRect: {
                    QPointF firstPoint, secondPoint, thirdPoint, fourthPoint;
                    firstPoint = pList[0];
                    secondPoint = pList[1];
                    thirdPoint = pList[2];
                    fourthPoint = pList[3];
                    ItemDesignRectangle *itemDesignRectangle = drawRectangleToScene(firstPoint, secondPoint, thirdPoint, fourthPoint,
                                                                                    pStitchesList, stitchObjList, punchObjList);
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                    itemDesignRectangle->setStartSewingPoint(QPointF(tmpX, -tmpY));
                    itemDesignRectangle->setLayer(i + 1);
                    itemDesignRectangle->setItemId(j + 1);
                    itemDesignRectangle->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                    itemDesignRectangle->setIsShowOutline(isShowOutline);
                    itemDesignRectangle->setIsShowId(isShowItemId);
                    break;
                }
                //Plt曲线/折线
                case stVetex:
                //直线
                case stLine: {
                    ItemDesignLine *itemDesignLine = drawLineToScene(pList, pStitchesList, stitchObjList, punchObjList);
                    if (itemDesignLine != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignLine->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignLine->setLayer(i + 1);
                        itemDesignLine->setItemId(j + 1);
                        itemDesignLine->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                        itemDesignLine->setIsShowOutline(isShowOutline);
                        itemDesignLine->setIsShowId(isShowItemId);
                    }
                    break;
                }
                //多边形
                case stPolygon: {
                    ItemDesignPolygon *itemDesignPolygon = drawPolygonToScene(pList, pStitchesList, stitchObjList, punchObjList);
                    if (itemDesignPolygon != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignPolygon->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignPolygon->setLayer(i + 1);
                        itemDesignPolygon->setItemId(j + 1);
                        itemDesignPolygon->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                        itemDesignPolygon->setIsShowOutline(isShowOutline);
                        itemDesignPolygon->setIsShowId(isShowItemId);
                    }
                    break;
                }
                //弧线
                case stArc: {
                    ItemDesignArc *itemDesignArc = drawArcToScene(pList, pStitchesList, stitchObjList, punchObjList);
                    if (itemDesignArc != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignArc->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignArc->setLayer(i + 1);
                        itemDesignArc->setItemId(j + 1);
                        itemDesignArc->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                        itemDesignArc->setIsShowOutline(isShowOutline);
                        itemDesignArc->setIsShowId(isShowItemId);
                    }
                    break;
                }
                //曲线
                case stCurve: {
                    ItemDesignCurve *itemDesignCurve = drawCurveToScene(pList, pStitchesList, stitchObjList, punchObjList);
                    if (itemDesignCurve != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignCurve->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignCurve->setLayer(i + 1);
                        itemDesignCurve->setItemId(j + 1);
                        itemDesignCurve->setColor(layerColorsString[i % LAYER_COLOR_COUNT_D]);
                        itemDesignCurve->setIsShowOutline(isShowOutline);
                        itemDesignCurve->setIsShowId(isShowItemId);
                    }
                    break;
                }
                //空线
                case stNullLine: {
                    //空线的关键点信息就是缝纫点
                    QList<QPointF> pStitchList; //存放缝纫点的list
                    pStitchList.clear();
                    int stitchPointCount = interface->ArmGetStitchCount(i, j);
                    for (int l = 0; l < stitchPointCount; l++) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, l, &tmpX, &tmpY);
                        pStitchList << QPointF(tmpX, -tmpY);

                        //第一个图元只记录最后一个关键点
                        if (j == 0 && l == stitchPointCount - 1) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                        //最后一个图元只记录第一个关键点
                        else if (j == unitCount - 1 && l == 0) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                        //中间的图元, 第一个关键点和最后一个关键点都要记录
                        else if (0 < j && j < unitCount - 1) {
                            if (l == 0 || l == stitchPointCount - 1) {
                                pListBetweenUnit << QPointF(tmpX, -tmpY);
                            }
                        }
                    }
                    ItemDesignEmptyLine *itemDesignEmptyLine = drawEmptyLineToScene(pStitchList, stitchObjList);
                    if (itemDesignEmptyLine != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignEmptyLine->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignEmptyLine->setLayer(i + 1);
                        itemDesignEmptyLine->setItemId(j + 1);
                        itemDesignEmptyLine->setIsShowOutline(isShowOutline);
                        itemDesignEmptyLine->setIsShowId(isShowItemId);
                        itemDesignEmptyLine->setColor("grey");
                        itemDesignEmptyLine->setStyle("dash");
                    }
                    break;
                }
                //发送服装线
                case stNull: {
                    //发送服装线的关键点信息就是缝纫点
                    QList<QPointF> pStitchList; //存放缝纫点的list
                    pStitchList.clear();
                    int stitchPointCount = interface->ArmGetStitchCount(i, j);
                    for (int l = 0; l < stitchPointCount; l++) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, l, &tmpX, &tmpY);
                        pStitchList << QPointF(tmpX, -tmpY);

                        //第一个图元只记录最后一个关键点
                        if (j == 0 && l == stitchPointCount - 1) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                        //最后一个图元只记录第一个关键点
                        else if (j == unitCount - 1 && l == 0) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                        //中间的图元, 第一个关键点和最后一个关键点都要记录
                        else if (0 < j && j < unitCount - 1) {
                            if (l == 0 || l == stitchPointCount - 1) {
                                pListBetweenUnit << QPointF(tmpX, -tmpY);
                            }
                        }
                    }
                    ItemDesignSendClothesLine *itemDesignSendClothesLine = drawSendClothesLineToScene(pStitchList, stitchObjList);
                    if (itemDesignSendClothesLine != nullptr) {
                        qreal tmpX, tmpY;
                        interface->ArmGetStitchPoint(i, j, 0, &tmpX, &tmpY);
                        itemDesignSendClothesLine->setStartSewingPoint(QPointF(tmpX, -tmpY));
                        itemDesignSendClothesLine->setLayer(i + 1);
                        itemDesignSendClothesLine->setItemId(j + 1);
                        itemDesignSendClothesLine->setIsShowOutline(isShowOutline);
                        itemDesignSendClothesLine->setIsShowId(isShowItemId);
                        itemDesignSendClothesLine->setColor("brown");
                        itemDesignSendClothesLine->setStyle("dash");
                    }
                    break;
                }
                default: break;
            }
        }

        if (unitCount > 1) {
            //绘制同一个图层中的图元连接线
            for (int l = 0; l < pListBetweenUnit.size(); l += 2) {
                QList<QPointF> pListTmp;
                pListTmp.clear();
                pListTmp << pListBetweenUnit[l] << pListBetweenUnit[l + 1];
                ItemDottedLine *itemDottedLineGlobal = this->drawDottedLine(pListTmp);
                if (itemDottedLineGlobal != nullptr) {
                    itemDottedLineGlobal->setLayer(i + 1); //设置图层(跟随缝纫点, 也有图层信息)
                    itemDottedLineGlobal->setStyle("dash");
                    itemDottedLineGlobal->setIsShowDottedLine(isShowConnectLine);
                }
            }
        }
    }

    //设置设计界面整图大小
    qreal leftTopX, leftTopY, rightBottomX, rightBottomY;
    interface->ArmGetScaleOfAllUints(&leftTopX, &leftTopY, &rightBottomX, &rightBottomY);
    designWindow->setStatusBarSizeOfWholePic(fabs(leftTopX - rightBottomX), fabs(leftTopY - rightBottomY));

    //设置设计界面总针数
    designWindow->setStatusBarNeedleCount(this->needleCount);

    //刷新designwindow的树显示
    treeLayerView->refreshLeftTreeLayerView(interface);
}

/* 把interface中的图形绘制在scene里(是否自适应显示) */
void CanvasViewDesigner::dumpInterfaceToSceneSelfAdapt(void)
{
    dumpInterfaceToScene();
    setGlobalSelfAdaption();
}

/* 清空场景 */
void CanvasViewDesigner::clearScene(void)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        //除去gridLine和那个蓝色小方格不能删除
        if (gridLineList.contains(static_cast<QGraphicsItem *>(itemList.at(i))) == false &&
              itemList.at(i) != squareItem) {
            delete itemList.at(i);
        }
    }

    this->scene()->update();

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 设置图形显示ID */
void CanvasViewDesigner::setItemsShowId(void)
{
    QList<QGraphicsItem *> itemList = this->scene()->items();

    int showIdCnt = 1;
    for (int i = itemList.size() - 1; i >= 0; i--) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                    itemDesignCenterCircle->show();
                    itemDesignCenterCircle->setItemShowId(showIdCnt++);
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                    itemDesign3PCircle->show();
                    itemDesign3PCircle->setItemShowId(showIdCnt++);
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                if (itemDesignEllipse != nullptr) {
                    itemDesignEllipse->show();
                    itemDesignEllipse->setItemShowId(showIdCnt++);
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                if (itemDesignRectangle != nullptr) {
                    itemDesignRectangle->show();
                    itemDesignRectangle->setItemShowId(showIdCnt++);
                }
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                if (itemDesignLine != nullptr) {
                    itemDesignLine->show();
                    itemDesignLine->setItemShowId(showIdCnt++);
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                if (itemDesignPolygon != nullptr) {
                    itemDesignPolygon->show();
                    itemDesignPolygon->setItemShowId(showIdCnt++);
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                if (itemDesignArc != nullptr) {
                    itemDesignArc->show();
                    itemDesignArc->setItemShowId(showIdCnt++);
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                if (itemDesignCurve != nullptr) {
                    itemDesignCurve->show();
                    itemDesignCurve->setItemShowId(showIdCnt++);
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                    itemDesignEmptyLine->show();
                    itemDesignEmptyLine->setItemShowId(showIdCnt++);
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                    itemDesignSendClothesLine->show();
                    itemDesignSendClothesLine->setItemShowId(showIdCnt++);
                }
                break;
            }
            default: break;
        }
    }
    this->scene()->update();
}

/* 画缝纫点 */
ItemDesignStitchPoint *CanvasViewDesigner::drawStitchPoint(QPointF pos)
{
    ItemDesignStitchPoint *itemDesignStitchPoint = new ItemDesignStitchPoint(pos);
    itemDesignStitchPoint->setItemType(eITEM_DESIGN_STITCH_POINT);
    itemDesignStitchPoint->setFlags(QGraphicsItem::ItemIsSelectable |
                              QGraphicsItem::ItemIgnoresTransformations);
    this->scene()->addItem(itemDesignStitchPoint);

    return itemDesignStitchPoint;
}

/* 画关键点 */
ItemDesignPunchPoint *CanvasViewDesigner::drawPunchPoint(QPointF pos)
{
    ItemDesignPunchPoint *itemDesignPunchPoint = new ItemDesignPunchPoint(pos);
    itemDesignPunchPoint->setItemType(eITEM_DESIGN_PUNCH_POINT);
    itemDesignPunchPoint->setFlags(QGraphicsItem::ItemIsSelectable |
                              QGraphicsItem::ItemIgnoresTransformations);
    this->scene()->addItem(itemDesignPunchPoint);

    return itemDesignPunchPoint;
}

/* 画中心圆, 返回画的图元对象指针 */
ItemDesignCenterCircle *CanvasViewDesigner::drawCenterCircleToScene(
        QPointF firstPoint, QPointF secondPoint,
        QList<QPointF> &stitchesList,
        QList<QGraphicsItem *> stitchObjList,
        QList<QGraphicsItem *> punchObjList)
{
    ItemDesignCenterCircle *itemDesignCenterCircle = new ItemDesignCenterCircle(firstPoint, secondPoint,
                                                                                stitchesList,
                                                                                stitchObjList, punchObjList);
    itemDesignCenterCircle->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    itemDesignCenterCircle->setItemType(eITEM_DESIGN_CENTER_CIRCLE);
    this->scene()->addItem(itemDesignCenterCircle);

    return itemDesignCenterCircle;
}

/* 画三点圆 */
ItemDesign3PCircle *CanvasViewDesigner::draw3PCircleToScene(
        QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint,
        QList<QPointF> &stitchesList,
        QList<QGraphicsItem *> stitchObjList,
        QList<QGraphicsItem *> punchObjList)
{
    ItemDesign3PCircle *itemDesign3PCircle = new ItemDesign3PCircle(firstPoint, secondPoint, thirdPoint,
                                                                    stitchesList,
                                                                    stitchObjList, punchObjList);
    itemDesign3PCircle->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    itemDesign3PCircle->setItemType(eITEM_DESIGN_3P_CIRCLE);
    this->scene()->addItem(itemDesign3PCircle);

    return itemDesign3PCircle;
}

/* 画椭圆 */
ItemDesignEllipse *CanvasViewDesigner::drawEllipseToScene(
        QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint,
        QList<QPointF> &stitchesList,
        QList<QGraphicsItem *> stitchObjList,
        QList<QGraphicsItem *> punchObjList)
{
    ItemDesignEllipse *itemDesignEllipse = new ItemDesignEllipse(firstPoint, secondPoint, thirdPoint,
                                                                 stitchesList,
                                                                 stitchObjList, punchObjList);
    itemDesignEllipse->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    itemDesignEllipse->setItemType(eITEM_DESIGN_ELLIPSE);
    this->scene()->addItem(itemDesignEllipse);

    return itemDesignEllipse;
}

/* 画矩形 */
ItemDesignRectangle *CanvasViewDesigner::drawRectangleToScene(
        QPointF firstPoint, QPointF secondPoint, QPointF thirdPoint, QPointF fourthPoint,
        QList<QPointF> &stitchesList,
        QList<QGraphicsItem *> stitchObjList,
        QList<QGraphicsItem *> punchObjList)
{
    ItemDesignRectangle *itemDesignRectangle = new ItemDesignRectangle(firstPoint, secondPoint, thirdPoint, fourthPoint,
                                                                       stitchesList,
                                                                       stitchObjList, punchObjList);
    itemDesignRectangle->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    itemDesignRectangle->setItemType(eITEM_DESIGN_RECTANGLE);
    this->scene()->addItem(itemDesignRectangle);

    return itemDesignRectangle;
}

/* 画直线(折线) */
ItemDesignLine *CanvasViewDesigner::drawLineToScene(QList<QPointF> pList,
                                                    QList<QPointF> &stitchesList,
                                                    QList<QGraphicsItem *> stitchObjList,
                                                    QList<QGraphicsItem *> punchObjList)
{
    //只有一个点是无法画线的
    if (pList.size() <= 1) {
        return nullptr;
    }
    ItemDesignLine *itemDesignLine = new ItemDesignLine(pList,
                                                        stitchesList,
                                                        stitchObjList, punchObjList);
    itemDesignLine->setItemType(eITEM_DESIGN_LINE);
    itemDesignLine->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignLine);

    return itemDesignLine;
}

/* 画多边形 */
ItemDesignPolygon *CanvasViewDesigner::drawPolygonToScene(QList<QPointF> pList,
                                                          QList<QPointF> &stitchesList,
                                                          QList<QGraphicsItem *> stitchObjList,
                                                          QList<QGraphicsItem *> punchObjList)
{
    if (pList.size() <= 2) {
        return nullptr;
    }
    ItemDesignPolygon *itemDesignPolygon = new ItemDesignPolygon(pList,
                                                                 stitchesList,
                                                                 stitchObjList, punchObjList);
    itemDesignPolygon->setItemType(eITEM_DESIGN_POLYGON);
    itemDesignPolygon->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignPolygon);

    return itemDesignPolygon;
}

/* 画弧线 */
ItemDesignArc *CanvasViewDesigner::drawArcToScene(QList<QPointF> pList,
                           QList<QPointF> &stitchesList,
                           QList<QGraphicsItem *> stitchObjList,
                           QList<QGraphicsItem *> punchObjList)
{
    if (pList.size() <= 2) {
        return nullptr;
    }
    ItemDesignArc *itemDesignArc = new ItemDesignArc(pList,
                                                     stitchesList,
                                                     stitchObjList, punchObjList);
    itemDesignArc->setItemType(eITEM_DESIGN_ARC);
    itemDesignArc->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignArc);

    return itemDesignArc;
}

/* 画曲线 */
ItemDesignCurve *CanvasViewDesigner::drawCurveToScene(QList<QPointF> pList,
                                                      QList<QPointF> &stitchesList,
                                                      QList<QGraphicsItem *> stitchObjList,
                                                      QList<QGraphicsItem *> punchObjList)
{
    //只有一个点是无法画线的
    if (pList.size() <= 1) {
        return nullptr;
    }
    ItemDesignCurve *itemDesignCurve = new ItemDesignCurve(pList,
                                                           stitchesList,
                                                           stitchObjList, punchObjList);
    itemDesignCurve->setItemType(eITEM_DESIGN_CURVE);
    itemDesignCurve->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignCurve);

    return itemDesignCurve;
}

/* 画虚线 */
ItemDottedLine *CanvasViewDesigner::drawDottedLine(QList<QPointF> pList)
{
    //只有一个点是无法画线的
    if (pList.size() <= 1) {
        return nullptr;
    }
    ItemDottedLine *itemDottedLine = new ItemDottedLine(pList);
    itemDottedLine->setItemType(eITEM_DOTTED_LINE);
    this->scene()->addItem(itemDottedLine);

    return itemDottedLine;
}

/* 画空线 */
ItemDesignEmptyLine *CanvasViewDesigner::drawEmptyLineToScene(QList<QPointF> &stitchesList,
                                                              QList<QGraphicsItem *> stitchObjList)
{
    //只有一个点是无法画线的
    if (stitchesList.size() <= 1) {
        return nullptr;
    }
    ItemDesignEmptyLine *itemDesignEmptyLine = new ItemDesignEmptyLine(stitchesList, stitchObjList);
    itemDesignEmptyLine->setItemType(eITEM_DESIGN_EMPTY_LINE);
    itemDesignEmptyLine->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignEmptyLine);

    return itemDesignEmptyLine;
}

/* 画发送服装线 */
ItemDesignSendClothesLine *CanvasViewDesigner::drawSendClothesLineToScene(QList<QPointF> &stitchesList,
                                                                          QList<QGraphicsItem *> stitchObjList)
{
    //只有一个点是无法画线的
    if (stitchesList.size() <= 1) {
        return nullptr;
    }
    ItemDesignSendClothesLine *itemDesignSendClothesLine = new ItemDesignSendClothesLine(stitchesList, stitchObjList);
    itemDesignSendClothesLine->setItemType(eITEM_DESIGN_SEND_CLOTHES_LINE);
    itemDesignSendClothesLine->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    this->scene()->addItem(itemDesignSendClothesLine);

    return itemDesignSendClothesLine;
}

/* 栅格初始化 */
void CanvasViewDesigner::gridInit(int gridSize)
{
    gridClearance = gridSize;
    int width = this->scene()->width();
    int height = this->scene()->height();
    int leftX, rightX, topY, bottomY;
    leftX = -width / 2;
    rightX = width / 2;
    topY = height / 2;
    bottomY = -height / 2;

    this->gridLineList.clear();
    //绘制零位置线
    QPen pen;
    pen.setWidth(0);
    pen.setColor(QColor(255, 0, 0, 200));      //红
    QGraphicsLineItem *itemLineX = new QGraphicsLineItem(leftX, 0, rightX, 0);
    this->gridLineList.append(itemLineX);
    itemLineX->setPen(pen);
    this->scene()->addItem(itemLineX);
    QGraphicsLineItem *itemLineY = new QGraphicsLineItem(0, topY, 0, bottomY);
    this->gridLineList.append(itemLineY);
    itemLineY->setPen(pen);
    this->scene()->addItem(itemLineY);

    //绘制水平线
    pen.setColor(QColor(192, 192, 192, 128));  //灰
    int i = gridClearance;
    while (i < topY) {
        QGraphicsLineItem *itemLine1 = new QGraphicsLineItem(leftX, i, rightX, i);
        this->gridLineList.append(itemLine1);
        itemLine1->setPen(pen);
        this->scene()->addItem(itemLine1);

        QGraphicsLineItem *itemLine2 = new QGraphicsLineItem(leftX, -i, rightX, -i);
        this->gridLineList.append(itemLine2);
        itemLine2->setPen(pen);
        this->scene()->addItem(itemLine2);

        i += gridClearance;
    }

    //绘制垂直线
    i = gridClearance;
    while (i < rightX) {
        QGraphicsLineItem *itemLine1 = new QGraphicsLineItem(i, topY, i, bottomY);
        this->gridLineList.append(itemLine1);
        itemLine1->setPen(pen);
        this->scene()->addItem(itemLine1);

        QGraphicsLineItem *itemLine2 = new QGraphicsLineItem(-i, topY, -i, bottomY);
        this->gridLineList.append(itemLine2);
        itemLine2->setPen(pen);
        this->scene()->addItem(itemLine2);

        i += gridClearance;
    }

    //绘制中间的蓝色小正方形
    pen.setColor(QColor(0, 0, 255, 200)); //蓝
    squareItem = new QGraphicsRectItem(-5, -5, 10, 10);
    squareItem->setPen(pen);
    squareItem->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    this->scene()->addItem(squareItem);

    //更新显示
    this->scene()->update();
}

/* 删除栅格 */
void CanvasViewDesigner::gridDeInit(void)
{
    delete squareItem;
    for (int i = 0; i < gridLineList.size(); i++) {
        delete gridLineList.at(i);
    }
    gridLineList.clear();

    //更新显示
    this->scene()->update();
}

/* 栅格显示切换 */
void CanvasViewDesigner::toggleGrid(void)
{
    //重设指示变量
    if (isShowGrid == true) {
        isShowGrid = false;
    } else {
        isShowGrid = true;
    }

    if (isShowGrid == true) {
        for (int i = 0; i < this->gridLineList.size(); i++) {
            this->gridLineList.at(i)->show();
        }
    } else {
        //把除了中间红色的栅格线都hide
        for (int i = 2; i < this->gridLineList.size(); i++) {
            this->gridLineList.at(i)->hide();
        }
    }

    //更新显示
    this->scene()->update();
}

/* 右键菜单初始化 */
void CanvasViewDesigner::rightBtnMenuInit(void)
{
    menuSwitchRightBtn = new QMenu(this);
    actionStopDrawing = new QAction(QIcon(":/icon/designwindow/resource/designwindow/drawing/finish_drawing.png"), tr("结束"), menuSwitchRightBtn);
    actionSwitchToLine = new QAction(QIcon(":/icon/designwindow/resource/designwindow/23-straightLine.png"), tr("直线"), menuSwitchRightBtn);
    actionSwitchToArc = new QAction(QIcon(":/icon/designwindow/resource/designwindow/24-arc.png"), tr("弧线"), menuSwitchRightBtn);
    actionSwitchToCurve = new QAction(QIcon(":/icon/designwindow/resource/designwindow/25-curve.png"), tr("曲线"), menuSwitchRightBtn);
    actionSwitchToRect = new QAction(QIcon(":/icon/designwindow/resource/designwindow/26-rectangle.png"), tr("矩形"), menuSwitchRightBtn);
    actionSwitchToPolygon = new QAction(QIcon(":/icon/designwindow/resource/designwindow/27-polygon.png"), tr("多边形"), menuSwitchRightBtn);
    actionSwitchTo3PCircle = new QAction(QIcon(":/icon/designwindow/resource/designwindow/28-threePointCircle.png"), tr("三点圆"), menuSwitchRightBtn);
    actionSwitchToEllipse = new QAction(QIcon(":/icon/designwindow/resource/designwindow/29-Ellipse.png"), tr("椭圆"), menuSwitchRightBtn);
    actionSwitchToCenterCircle = new QAction(QIcon(":/icon/designwindow/resource/designwindow/30-centralCircle.png"), tr("中心圆"), menuSwitchRightBtn);
    menuSwitchRightBtn->addAction(actionStopDrawing);
    menuSwitchRightBtn->addSeparator();
    menuSwitchRightBtn->addAction(actionSwitchToLine);
    menuSwitchRightBtn->addAction(actionSwitchToArc);
    menuSwitchRightBtn->addAction(actionSwitchToCurve);
    menuSwitchRightBtn->addAction(actionSwitchToRect);
    menuSwitchRightBtn->addAction(actionSwitchToPolygon);
    menuSwitchRightBtn->addAction(actionSwitchTo3PCircle);
    menuSwitchRightBtn->addAction(actionSwitchToEllipse);
    menuSwitchRightBtn->addAction(actionSwitchToCenterCircle);
    menuSwitchRightBtn->hide();

    //连接槽
    connect(actionStopDrawing, &QAction::triggered, this, &CanvasViewDesigner::slotActionStopDrawingTriggered);
    connect(actionSwitchToLine, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToLineTriggered);
    connect(actionSwitchToArc, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToArcTriggered);
    connect(actionSwitchToCurve, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToCurveTriggered);
    connect(actionSwitchToRect, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToRectTriggered);
    connect(actionSwitchToPolygon, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToPolygonTriggered);
    connect(actionSwitchTo3PCircle, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchTo3PCircleTriggered);
    connect(actionSwitchToEllipse, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToEllipseTriggered);
    connect(actionSwitchToCenterCircle, &QAction::triggered, this, &CanvasViewDesigner::slotActionSwitchToCenterCircleTriggered);
}

/* 追加一个图层 */
void CanvasViewDesigner::appendLayer(void)
{
    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;

    layerCnt++;
    interface->ArmCreateLayer((char *)0);
    this->currentSelectedLayer = layerCnt;

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    dumpInterfaceToScene();

    //设置选中的图层
    interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
}

/* 删除某个图层 */
void CanvasViewDesigner::deleteLayer(int layerToBeDeleted)
{
    //至少保证有一个空图层
    int layerCount;
    this->interface->ArmGetLayerCount(&layerCount);
    if (layerCount == 1) {

        saveGraphicState(); //状态压栈
        isInterfaceChanged = true;
        interface->ArmDeleteLayerByIndex(0);
        layerCnt = 1;
        // interface->ArmCreateLayer((char *)0); //这里不用手动创建, 貌似删除的时候, 会自动创建一个空图层
        this->currentSelectedLayer = 1;
        interface->ArmSetCurrentLayer(currentSelectedLayer - 1);

        //设置右上角颜色示例
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);

        dumpInterfaceToScene();
        return;
    }

    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;

    layerCnt--;
    interface->ArmDeleteLayerByIndex(layerToBeDeleted - 1);

    //删除后, 选中第一个图层
    this->currentSelectedLayer = 1;
    interface->ArmSetCurrentLayer(currentSelectedLayer - 1);

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    dumpInterfaceToScene();
}

/* 删除接口中的空图层 */
void CanvasViewDesigner::deleteEmptyLayer(void)
{
    int layerCnt;
    interface->ArmGetLayerCount(&layerCnt);
    for (int i = layerCnt - 1; i >= 0; i--) {
        int unitCnt = interface->ArmGetUintCount(i);
        if (unitCnt == 0) {
            interface->ArmDeleteLayerByIndex(i);
        }
    }
}

/* 删除接口中的发送服装线和空线 */
void CanvasViewDesigner::deleteEmptyLineAndSendClothesLine(void)
{
    int layerCnt;
    interface->ArmGetLayerCount(&layerCnt);
    interface->ArmCleanSelectObject();
    for (int i = layerCnt - 1; i >= 0; i--) {
        int unitCnt = interface->ArmGetUintCount(i);
        for (int j = unitCnt - 1; j >= 0; j--) {
            int unitType;
            interface->ArmGetUintTypeByIndex(i, j, &unitType);
            if (unitType == stNullLine || unitType == stNull) {
                interface->ArmSelectUintByIndex(1, i, j);
            }
        }
    }
    interface->ArmDeleteSelectUint();
    interface->ArmCleanSelectObject();
}

/* 删除图元 */
void CanvasViewDesigner::deleteItems(void)
{
    //对多选图形的删除处理
    if (currentSelectedItemsList.isEmpty() == true) {
        return;
    }

    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;

    interface->ArmDeleteSelectUint();
    interface->ArmCleanSelectObject();
    currentSelectedItemsList.clear();
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();

    int layerCount;
    interface->ArmGetLayerCount(&layerCount);
    int unitCount = interface->ArmGetUintCount(0);

    //检查是否删除了所有图元
    if (layerCount == 1 && unitCount == 0) {
        //清屏
        clearScene();
        interface->ArmDeleteLayerByIndex(0);
        interface->ArmCleanSelectObject();

        this->currentSelectedLayer = 1;
        interface->ArmSetCurrentLayer(currentSelectedLayer - 1);

        //设置右上角颜色示例
        QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
        designWindow->setLayerColorDemo(color);

        dumpInterfaceToScene();
        return;
    }

    //检查是否有空图层
    for (int i = layerCount - 1; i >= 0; i--) {
        int unitCount = interface->ArmGetUintCount(i);
        if (unitCount == 0) {
            //删除这个图层
            interface->ArmDeleteLayerByIndex(i);
        }
    }

    this->currentSelectedLayer = 1;
    interface->ArmSetCurrentLayer(currentSelectedLayer - 1);

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    dumpInterfaceToScene();
}

/* 刷新某个图层 */
void CanvasViewDesigner::refreshLayer(int)
{
    qDebug() << "刷新图层";
}

/* 回设画板 */
void CanvasViewDesigner::setBackCanvasDesigner(void)
{
    //设置显示id
    this->setItemsShowId();

    //清空选中
    interface->ArmCleanSelectObject();
    currentSelectedItemsList.clear();
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    this->scene()->clearSelection();
    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    treeLayerView->clearSelection();
}

/* 设置currentSelectedLayer */
void CanvasViewDesigner::setCurrentSelectedLayer(int layerToBeSelected)
{
    //选中图层
    this->currentSelectedLayer = layerToBeSelected;
    interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);

    //清除选中
    currentSelectedItemsList.clear();
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();
    interface->ArmCleanSelectObject();

    //设置右上角颜色示例
    QString color = layerColorsString[(layerToBeSelected - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
    this->scene()->clearSelection();
    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 设置currentSelectedItemsList */
void CanvasViewDesigner::setCurrentSelectedItemsList(QPoint itemToBeSelected)
{
    int layer = itemToBeSelected.x();
    int itemId = itemToBeSelected.y();

    //选中图层
    this->currentSelectedLayer = layer;
    interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);

    //设置右上角颜色示例
    QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
    designWindow->setLayerColorDemo(color);

    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    //在场景中找到那个图元并选中它
    eItem_Type_t itemType;
    QGraphicsItem *item = this->getItem(itemToBeSelected.x(), itemToBeSelected.y(), &itemType);
    this->scene()->clearSelection();
    item->setSelected(true);

    //添加进list
    QList<int> listSelectItemTmp;
    listSelectItemTmp.clear();
    listSelectItemTmp << layer << itemId;
    currentSelectedItemsList.clear();
    currentSelectedItemsList.append(listSelectItemTmp);
    currentSelectedStitchesList.clear();
    currentSelectedPunchesList.clear();

    //在接口中选中
    interface->ArmCleanSelectObject();
    interface->ArmSelectUintByIndex(1, layer - 1, itemId - 1);

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 获取图层layer上, 识别号为id的item */
QGraphicsItem *CanvasViewDesigner::getItem(int layer, int id, eItem_Type_t *itemTypeOut)
{
    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                    int tmpLayer = itemDesignCenterCircle->getLayer();
                    int tmpId = itemDesignCenterCircle->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_CENTER_CIRCLE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                    int tmpLayer = itemDesign3PCircle->getLayer();
                    int tmpId = itemDesign3PCircle->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_3P_CIRCLE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                if (itemDesignEllipse != nullptr) {
                    int tmpLayer = itemDesignEllipse->getLayer();
                    int tmpId = itemDesignEllipse->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_ELLIPSE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                if (itemDesignRectangle != nullptr) {
                    int tmpLayer = itemDesignRectangle->getLayer();
                    int tmpId = itemDesignRectangle->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_RECTANGLE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                if (itemDesignLine != nullptr) {
                    int tmpLayer = itemDesignLine->getLayer();
                    int tmpId = itemDesignLine->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_LINE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                if (itemDesignPolygon != nullptr) {
                    int tmpLayer = itemDesignPolygon->getLayer();
                    int tmpId = itemDesignPolygon->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_POLYGON;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                if (itemDesignArc != nullptr) {
                    int tmpLayer = itemDesignArc->getLayer();
                    int tmpId = itemDesignArc->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_ARC;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                if (itemDesignCurve != nullptr) {
                    int tmpLayer = itemDesignCurve->getLayer();
                    int tmpId = itemDesignCurve->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_CURVE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                    int tmpLayer = itemDesignEmptyLine->getLayer();
                    int tmpId = itemDesignEmptyLine->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_EMPTY_LINE;
                        return itemList.at(i);
                    }
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                    int tmpLayer = itemDesignSendClothesLine->getLayer();
                    int tmpId = itemDesignSendClothesLine->getItemId();
                    if (tmpLayer == layer && tmpId == id) {
                        *itemTypeOut = eITEM_DESIGN_SEND_CLOTHES_LINE;
                        return itemList.at(i);
                    }
                }
                break;
            }

            default: break;
        }
    }

    return nullptr;
}

/* 获取当前选中的图层序号 */
int CanvasViewDesigner::getSelectedLayer(void)
{
    return this->currentSelectedLayer;
}

/* 场景中图元的选中情况有变化时调用的槽 */
void CanvasViewDesigner::slotSceneSelectionChanged(void)
{
    QList<QGraphicsItem *> selectedItemList = this->scene()->selectedItems();

    //在scene空白处点了一下(导致没有选中的图元)
    if (selectedItemList.isEmpty()) {

        //取消选中
        currentSelectedStitchesList.clear();
        currentSelectedItemsList.clear();
        currentSelectedPunchesList.clear();
        interface->ArmCleanSelectObject();
        treeLayerView->clearSelection();

        //刷新显示设计界面当前图大小
        designWindow->setStatusBarSizeOfCurrentPic(0, 0);

        //通知更新treeLayerView的选中情况
        emit signalSceneSelectedGraphic(currentSelectedItemsList);
    }
    //选中了scene中的图元
    else {

        //先取消选中
        currentSelectedStitchesList.clear();
        currentSelectedItemsList.clear();
        currentSelectedPunchesList.clear();
        interface->ArmCleanSelectObject();
        treeLayerView->clearSelection();

        for (int i = 0; i < selectedItemList.size(); i++) {
            ItemBase *itemBase = dynamic_cast<ItemBase *>(selectedItemList.at(i));
            if (itemBase == nullptr) {
                continue;
            }

            int layer, itemId;
            eItem_Type_t itemType = itemBase->getItemType();
            if (itemType == eITEM_DESIGN_STITCH_POINT) {

                ItemDesignStitchPoint *itemDesignStitchPoint = dynamic_cast<ItemDesignStitchPoint *>(selectedItemList.at(i));
                layer = itemDesignStitchPoint->getLayer();
                itemId = itemDesignStitchPoint->getItemId();
                int stitchNum = itemDesignStitchPoint->getStitchNum();

                QList<int> listSelectStitchTmp;
                listSelectStitchTmp.clear();
                listSelectStitchTmp << layer << itemId << stitchNum;
                currentSelectedStitchesList.append(listSelectStitchTmp);

                //在数据接口里选中
                interface->ArmAddStitchToCurrentSelectStitchList(layer - 1, itemId - 1, stitchNum - 1);

                //设置右上角颜色示例
                this->currentSelectedLayer = currentSelectedStitchesList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)

                interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
                QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
                designWindow->setLayerColorDemo(color);

                continue;

            } else if (itemType == eITEM_DESIGN_PUNCH_POINT) {

                ItemDesignPunchPoint *itemDesignPunchPoint = dynamic_cast<ItemDesignPunchPoint *>(selectedItemList.at(i));
                layer = itemDesignPunchPoint->getLayer();
                itemId = itemDesignPunchPoint->getItemId();
                int punchNum = itemDesignPunchPoint->getPunchPointNum();

                QList<int> listSelectPunchTmp;
                listSelectPunchTmp.clear();
                listSelectPunchTmp << layer << itemId << punchNum;
                currentSelectedPunchesList.append(listSelectPunchTmp);

                //在数据接口里选中
                interface->ArmAddPunchToCurrentSelectPunchList(layer - 1, itemId - 1, punchNum - 1);

                //设置右上角颜色示例
                this->currentSelectedLayer = currentSelectedPunchesList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)
                interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
                QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
                designWindow->setLayerColorDemo(color);

                continue;
            }

            switch (itemType) {
                //中心圆
                case eITEM_DESIGN_CENTER_CIRCLE: {
                    ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(selectedItemList.at(i));
                    layer = itemDesignCenterCircle->getLayer();
                    itemId = itemDesignCenterCircle->getItemId();
                    break;
                }
                //三点圆
                case eITEM_DESIGN_3P_CIRCLE: {
                    ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(selectedItemList.at(i));
                    layer = itemDesign3PCircle->getLayer();
                    itemId = itemDesign3PCircle->getItemId();
                    break;
                }
                //椭圆
                case eITEM_DESIGN_ELLIPSE: {
                    ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(selectedItemList.at(i));
                    layer = itemDesignEllipse->getLayer();
                    itemId = itemDesignEllipse->getItemId();
                    break;
                }
                //矩形
                case eITEM_DESIGN_RECTANGLE: {
                    ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(selectedItemList.at(i));
                    layer = itemDesignRectangle->getLayer();
                    itemId = itemDesignRectangle->getItemId();
                    break;
                }
                //直线
                case eITEM_DESIGN_LINE: {
                    ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(selectedItemList.at(i));
                    layer = itemDesignLine->getLayer();
                    itemId = itemDesignLine->getItemId();
                    break;
                }
                //多边形
                case eITEM_DESIGN_POLYGON: {
                    ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(selectedItemList.at(i));
                    layer = itemDesignPolygon->getLayer();
                    itemId = itemDesignPolygon->getItemId();
                    break;
                }
                //弧线
                case eITEM_DESIGN_ARC: {
                    ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(selectedItemList.at(i));
                    layer = itemDesignArc->getLayer();
                    itemId = itemDesignArc->getItemId();
                    break;
                }
                //曲线
                case eITEM_DESIGN_CURVE: {
                    ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(selectedItemList.at(i));
                    layer = itemDesignCurve->getLayer();
                    itemId = itemDesignCurve->getItemId();
                    break;
                }
                //空线
                case eITEM_DESIGN_EMPTY_LINE: {
                    ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(selectedItemList.at(i));
                    layer = itemDesignEmptyLine->getLayer();
                    itemId = itemDesignEmptyLine->getItemId();
                    break;
                }
                //发送服装线
                case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                    ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(selectedItemList.at(i));
                    layer = itemDesignSendClothesLine->getLayer();
                    itemId = itemDesignSendClothesLine->getItemId();
                    break;
                }
                default: break;
            }
            QList<int> listSelectItemTmp;
            listSelectItemTmp.clear();
            listSelectItemTmp << layer << itemId;
            currentSelectedItemsList.append(listSelectItemTmp);

            //在数据接口里选中
            interface->ArmSelectUintByIndex(1, layer - 1, itemId - 1);
        }

        if (currentSelectedItemsList.size() > 0) {
            //设置右上角颜色示例
            this->currentSelectedLayer = currentSelectedItemsList[0].at(0); //每次都选list[0]那个图元所在的图层(随机)
            interface->ArmSetCurrentLayer(this->currentSelectedLayer - 1);
            QString color = layerColorsString[(this->currentSelectedLayer - 1) % LAYER_COLOR_COUNT_D];
            designWindow->setLayerColorDemo(color);
        }

        //选中缝纫点/关键点时, 还要选中 缝纫点/关键点 所在的图元
        for (int i = 0; i < currentSelectedStitchesList.size(); i++) {
            QList<int> listTmp;
            listTmp = currentSelectedStitchesList[i];
            int layer = listTmp[0];
            int unit = listTmp[1];

            currentSelectedItemsList << listTmp;
            interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
            eItem_Type_t itemType;
            QGraphicsItem *item = this->getItem(layer, unit, &itemType);
            item->setSelected(true);
        }
        for (int i = 0; i < currentSelectedPunchesList.size(); i++) {
            QList<int> listTmp;
            listTmp = currentSelectedPunchesList[i];
            int layer = listTmp[0];
            int unit = listTmp[1];

            currentSelectedItemsList << listTmp;
            interface->ArmSelectUintByIndex(1, layer - 1, unit - 1);
            eItem_Type_t itemType;
            QGraphicsItem *item = this->getItem(layer, unit, &itemType);
            item->setSelected(true);
        }

        //刷新显示设计界面当前图大小
        qreal dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
        designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

        //通知更新treeLayerView的选中情况
        emit signalSceneSelectedGraphic(currentSelectedItemsList);
    }
}

/* 场景图形绘制结束槽 */
void CanvasViewDesigner::slotActionStopDrawingTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_NONE: {
            break;
        }
        case NOW_DRAWING_LINE: {
            isLineFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_ARC: {
            isArcFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_CURVE: {
            isCurveFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_RECTANGLE: {
            isRectFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_POLYGON: {
            isPolygonFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_3P_CIRCLE: {
            is3PCircleFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_ELLIPSE: {
            isEllipseFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE: {
            isCenterCircleFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_SENDING_CLOTHES_LINE: {
            isSendClothesLineFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_EMPTY_LINE: {
            isEmptyLineFinish = true;
            startStateMachine();
            break;
        }
        case NOW_DRAWING_DISTANCE_MEASUREMENT_LINE: {
            isMeasurementLineFinish = true;
            startStateMachine();
            break;
        }

        default: break;
    }
}

/* 切换至直线 */
void CanvasViewDesigner::slotActionSwitchToLineTriggered(void)
{
    //先看看之前在画什么
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = linePointsList.last();
            linePointsList.clear();
            linePointsList << lastP;
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_LINE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isLineFinish = false;
                this->linePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(4);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            linePointsList.clear();
            linePointsList << arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_LINE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isLineFinish = false;
                this->linePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(4);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = curvePointsList.last();
            curvePointsList.clear();
            linePointsList.clear();
            linePointsList << lastP;
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_LINE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isLineFinish = false;
            this->linePointsList.clear();
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(4);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->lineStatus = DRAWING_LINE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_LINE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isLineFinish = false;
                this->linePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(4);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = polygonPointsList.size() >= 3 ? polygonPointsList.last() : polygonPointsList.first();
            polygonPointsList.clear();
            linePointsList.clear();
            linePointsList << lastP;
            break;
        }
        default: break;
    }

    //直接设置直线绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->lineStatus = DRAWING_LINE_MOVING;
    this->currentDrawing = NOW_DRAWING_LINE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isLineFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(4);
}

/* 切换至弧线 */
void CanvasViewDesigner::slotActionSwitchToArcTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ARC;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isArcFinish = false;
                this->arcPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(5);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->arcFirstPoint = linePointsList.last();
            linePointsList.clear();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ARC;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isArcFinish = false;
                this->arcPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(5);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            arcFirstPoint = arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ARC;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isArcFinish = false;
                this->arcPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(5);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (curvePointsList.size() >= 3) {
                pLast = curvePointsList.last();
            } else {
                pLast = curvePointsList.first();
            }
            arcFirstPoint = pLast;
            curvePointsList.clear();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_ARC;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isArcFinish = false;
            this->arcPointsList.clear();
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(5);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->arcStatus = DRAWING_ARC_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ARC;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isArcFinish = false;
                this->arcPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(5);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (polygonPointsList.size() >= 3) {
                pLast = polygonPointsList.last();
            } else {
                pLast = polygonPointsList.first();
            }
            this->arcFirstPoint = pLast;
            polygonPointsList.clear();
            arcPointsList.clear();
            break;
        }
        default: break;
    }

    //直接设置弧线绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->arcStatus = DRAWING_ARC_MOVING_FIRST;
    this->arcPointsList.clear();
    this->currentDrawing = NOW_DRAWING_ARC;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isArcFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(5);
}

/* 切换至曲线 */
void CanvasViewDesigner::slotActionSwitchToCurveTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CURVE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCurveFinish = false;
                this->curvePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(6);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = linePointsList.last();
            linePointsList.clear();
            curvePointsList.clear();
            curvePointsList << lastP;
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CURVE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCurveFinish = false;
                this->curvePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(6);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            curvePointsList.clear();
            curvePointsList << arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP;
            if (curvePointsList.size() >= 3) {
                lastP = curvePointsList.last();
            } else {
                lastP = curvePointsList.first();
            }
            curvePointsList.clear();
            curvePointsList << lastP;
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_CURVE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isCurveFinish = false;
            this->curvePointsList.clear();
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(6);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->curveStatus = DRAWING_CURVE_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CURVE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCurveFinish = false;
                this->curvePointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(6);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = polygonPointsList.size() >= 3 ? polygonPointsList.last() : polygonPointsList.first();
            polygonPointsList.clear();
            curvePointsList.clear();
            curvePointsList << lastP;
            break;
        }
        default: break;
    }

    //直接设置曲线绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->curveStatus = DRAWING_CURVE_MOVING;
    this->currentDrawing = NOW_DRAWING_CURVE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isCurveFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(6);
}

/* 切换至矩形 */
void CanvasViewDesigner::slotActionSwitchToRectTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_RECTANGLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isRectFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(7);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->rectangleFirstPoint = linePointsList.last();
            linePointsList.clear();
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_RECTANGLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isRectFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(7);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->rectangleFirstPoint = arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_RECTANGLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isRectFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(7);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (curvePointsList.size() >= 3) {
                pLast = curvePointsList.last();
            } else {
                pLast = curvePointsList.first();
            }
            this->rectangleFirstPoint = pLast;
            curvePointsList.clear();
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE: {
            this->isDrawingLeftBtnClicked = false;
            this->rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_RECTANGLE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isRectFinish = false;
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(7);
            return;
        }
        case NOW_DRAWING_RECTANGLE: {
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->rectangleStatus = DRAWING_RECT_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_RECTANGLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isRectFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(7);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (polygonPointsList.size() >= 3) {
                pLast = polygonPointsList.last();
            } else {
                pLast = polygonPointsList.first();
            }
            this->rectangleFirstPoint = pLast;
            polygonPointsList.clear();
            break;
        }
        default: break;
    }

    //直接设置矩形绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->rectangleStatus = DRAWING_RECT_MOVING;
    this->currentDrawing = NOW_DRAWING_RECTANGLE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isRectFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(7);
}

/* 切换至多边形 */
void CanvasViewDesigner::slotActionSwitchToPolygonTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_POLYGON;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isPolygonFinish = false;
                this->polygonPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(8);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = linePointsList.last();
            linePointsList.clear();
            polygonPointsList.clear();
            polygonPointsList << lastP;
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_POLYGON;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isPolygonFinish = false;
                this->polygonPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(8);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            polygonPointsList.clear();
            polygonPointsList << arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_POLYGON;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isPolygonFinish = false;
                this->polygonPointsList.clear();
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(8);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP;
            if (curvePointsList.size() >= 3) {
                lastP = curvePointsList.last();
            } else {
                lastP = curvePointsList.first();
            }
            curvePointsList.clear();
            polygonPointsList.clear();
            polygonPointsList << lastP;
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->polygonStatus = DRAWING_POLYGON_WAIT_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_POLYGON;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isPolygonFinish = false;
            this->polygonPointsList.clear();
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(8);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF lastP = polygonPointsList.size() >= 3 ? polygonPointsList.last() : polygonPointsList.first();
            polygonPointsList.clear();
            polygonPointsList << lastP;
            break;
        }
        default: break;
    }

    //直接设置多边形绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->polygonStatus = DRAWING_POLYGON_MOVING;
    this->currentDrawing = NOW_DRAWING_POLYGON;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isPolygonFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(8);
}

/* 切换至三点圆 */
void CanvasViewDesigner::slotActionSwitchTo3PCircleTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->_3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->is3PCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(9);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->_3pCircleFirstPoint = linePointsList.last();
            linePointsList.clear();
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->_3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->is3PCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(9);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->_3pCircleFirstPoint = arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->_3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->is3PCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(9);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (curvePointsList.size() >= 3) {
                pLast = curvePointsList.last();
            } else {
                pLast = curvePointsList.first();
            }
            this->_3pCircleFirstPoint = pLast;
            curvePointsList.clear();
            break;
        }
        case NOW_DRAWING_3P_CIRCLE: {
            return;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->_3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->is3PCircleFinish = false;
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(9);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->_3pCircleStatus = DRAWING_3P_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->is3PCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(9);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (polygonPointsList.size() >= 3) {
                pLast = polygonPointsList.last();
            } else {
                pLast = polygonPointsList.first();
            }
            this->_3pCircleFirstPoint = pLast;
            polygonPointsList.clear();
            break;
        }
        default: break;
    }

    //直接设置三点圆绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->_3pCircleStatus = DRAWING_3P_CIRCLE_MOVING_FIRST;
    this->currentDrawing = NOW_DRAWING_3P_CIRCLE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->is3PCircleFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(9);
}

/* 切换至椭圆 */
void CanvasViewDesigner::slotActionSwitchToEllipseTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ELLIPSE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isEllipseFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(10);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->ellipseFirstPoint = linePointsList.last();
            linePointsList.clear();
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ELLIPSE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isEllipseFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(10);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->ellipseFirstPoint = arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ELLIPSE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isEllipseFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(10);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (curvePointsList.size() >= 3) {
                pLast = curvePointsList.last();
            } else {
                pLast = curvePointsList.first();
            }
            this->ellipseFirstPoint = pLast;
            curvePointsList.clear();
            break;
        }
        case NOW_DRAWING_ELLIPSE: {
            return;
        }
        case NOW_DRAWING_CENTER_CIRCLE:
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_ELLIPSE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isEllipseFinish = false;
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(10);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->ellipseStatus = DRAWING_ELLIPSE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_ELLIPSE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isEllipseFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(10);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (polygonPointsList.size() >= 3) {
                pLast = polygonPointsList.last();
            } else {
                pLast = polygonPointsList.first();
            }
            this->ellipseFirstPoint = pLast;
            polygonPointsList.clear();
            break;
        }
        default: break;
    }

    //直接设置椭圆绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->ellipseStatus = DRAWING_ELLIPSE_MOVING_FIRST;
    this->currentDrawing = NOW_DRAWING_ELLIPSE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isEllipseFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(10);
}

/* 切换至中心圆 */
void CanvasViewDesigner::slotActionSwitchToCenterCircleTriggered(void)
{
    switch (this->currentDrawing) {
        case NOW_DRAWING_LINE: {
            if (linePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCenterCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(11);
                return;
            }
            isLineFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->centerCircleFirstPoint = linePointsList.last();
            linePointsList.clear();
            break;
        }
        case NOW_DRAWING_ARC: {
            if (arcPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCenterCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(11);
                return;
            }
            isArcFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            this->centerCircleFirstPoint = arcPointsList.last();
            arcPointsList.clear();
            break;
        }
        case NOW_DRAWING_CURVE: {
            if (curvePointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCenterCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(11);
                return;
            }
            isCurveFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (curvePointsList.size() >= 3) {
                pLast = curvePointsList.last();
            } else {
                pLast = curvePointsList.first();
            }
            this->centerCircleFirstPoint = pLast;
            curvePointsList.clear();
            break;
        }
        case NOW_DRAWING_CENTER_CIRCLE: {
            return;
        }
        case NOW_DRAWING_3P_CIRCLE:
        case NOW_DRAWING_ELLIPSE:
        case NOW_DRAWING_RECTANGLE: {
            this->isDrawingLeftBtnClicked = false;
            this->centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
            this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
            this->selectionMode = SELECTION_MODE_DRAWING;
            this->isCenterCircleFinish = false;
            this->startStateMachine();
            designWindow->setLeftPanelBtnChosenQss(11);
            return;
        }
        case NOW_DRAWING_POLYGON: {
            if (polygonPointsList.isEmpty() == true) {
                this->isDrawingLeftBtnClicked = false;
                this->centerCircleStatus = DRAWING_CENTER_CIRCLE_WAIT_FIRST_CLIKCKED;
                this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
                this->selectionMode = SELECTION_MODE_DRAWING;
                this->isCenterCircleFinish = false;
                this->startStateMachine();
                designWindow->setLeftPanelBtnChosenQss(11);
                return;
            }
            isPolygonFinish = true;
            QTime dieTime = QTime::currentTime().addMSecs(50);
            while( QTime::currentTime() < dieTime ) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            }
            QPointF pLast;
            if (polygonPointsList.size() >= 3) {
                pLast = polygonPointsList.last();
            } else {
                pLast = polygonPointsList.first();
            }
            this->centerCircleFirstPoint = pLast;
            polygonPointsList.clear();
            break;
        }
        default: break;
    }

    //直接设置中心圆绘制状态
    this->isDrawingLeftBtnClicked = false;
    this->centerCircleStatus = DRAWING_CENTER_CIRCLE_MOVING;
    this->currentDrawing = NOW_DRAWING_CENTER_CIRCLE;
    this->selectionMode = SELECTION_MODE_DRAWING;
    this->isCenterCircleFinish = false;
    this->startStateMachine();
    designWindow->setLeftPanelBtnChosenQss(11);
}

/* 定时器事件, 用于反复update, 以便强制重复调用paintEvent, 运行状态机 */
void CanvasViewDesigner::timerEvent(QTimerEvent *)
{
    this->update(); //强制刷新, 调用paintEvent
}

/* 绘图事件 */
void CanvasViewDesigner::paintEvent(QPaintEvent *event)
{
    //继续分发, 必须先分发
    QGraphicsView::paintEvent(event);

    switch (this->currentDrawing) {

        //现在什么也没画
        case NOW_DRAWING_NONE: {
            if (selectionMode != SELECTION_MODE_DRAWING) {
                //非绘制模式下, 要能够绘制框选矩形, 用来选中图元, 缝纫点, 或者关键点
                drawFrameSelectionRectangle();
            }
            break;
        }
        //正在画直线
        case NOW_DRAWING_LINE: {
            drawLine();
            break;
        }
        //正在画弧线
        case NOW_DRAWING_ARC: {
            drawArc();
            break;
        }
        //正在画曲线
        case NOW_DRAWING_CURVE: {
            drawCurve();
            break;
        }
        //正在绘制矩形
        case NOW_DRAWING_RECTANGLE: {
            drawRectangle();
            break;
        }
        //正在绘制多边形
        case NOW_DRAWING_POLYGON: {
            drawPolygon();
            break;
        }
        //正在绘制三点圆
        case NOW_DRAWING_3P_CIRCLE: {
            drawThreePointCircle();
            break;
        }
        //正在绘制椭圆
        case NOW_DRAWING_ELLIPSE: {
            drawEllipse();
            break;
        }
        //正在绘制中心圆
        case NOW_DRAWING_CENTER_CIRCLE: {
            drawCenterCircle();
            break;
        }
        //正在绘制空线
        case NOW_DRAWING_EMPTY_LINE: {
            drawEmptyLine();
            break;
        }
        //正在绘制发送服装线
        case NOW_DRAWING_SENDING_CLOTHES_LINE: {
            drawSendClothesLine();
            break;
        }
        //正在绘制距离测量直线
        case NOW_DRAWING_DISTANCE_MEASUREMENT_LINE: {
            drawDistanceMeasurementLine();
            break;
        }
        //正在绘制复制镜像的参考线
        //case NOW_DRAWING_COPY_MIRROR:{
            //
        //    break;
        //}
        default: break;
    }
}

/* 鼠标滚轮事件, 用于控制按照鼠标指针位置缩放

执行流程:
1, 先获取指定视图上一个点的坐标 viewPos
2, 获取该点对应的场景坐标 scencePos
3, 执行缩放
4, 获取 scencePos 对应的新的视图坐标 curViewPoint
5, 移动滚动条到 curViewPoint 的位置

*/
void CanvasViewDesigner::wheelEvent(QWheelEvent *event)
{
    //获取当前鼠标相对于view的位置
    QPointF cursorPoint = event->pos();

    //获取当前鼠标相对于scene的位置
    QPointF scenePos = this->mapToScene(QPoint(cursorPoint.x(), cursorPoint.y()));

    //获取view的宽和高
    qreal viewWidth = this->viewport()->width();
    qreal viewHeight = this->viewport()->height();

    //获取当前鼠标位置相对于view大小的横纵比例
    qreal hScale = cursorPoint.x() / viewWidth;
    qreal vScale = cursorPoint.y() / viewHeight;

    //向上滚动, 放大
    if (event->delta() > 0) {
        setGlobalStepScale(true);
    } else {
        //向下滚动, 缩小
        setGlobalStepScale(false);
    }

    //将scene坐标转换为放大或缩小后的坐标(view坐标)
    QPointF curViewPoint = this->matrix().map(scenePos);

    //设置设计界面状态栏: 相对坐标, l a
    designWindow->setStatusBarRelativeCoord(
                this->mapToScene(this->mouseViewPos));

    //通过滚动条控制view放大缩小后的展示scene的位置
    horizontalScrollBar()->setValue(int(curViewPoint.x() - viewWidth * hScale));
    verticalScrollBar()->setValue(int(curViewPoint.y() - viewHeight * vScale));
    emit signalGlobalStepScale();
}

/* 鼠标按下事件 */
void CanvasViewDesigner::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        isDesignDrag == false &&
        selectionMode != SELECTION_MODE_DRAWING) {
        isFrameSelectionLeftBtnPressed = true; //左键按下表示框选左键已经按下
        this->startStateMachine();
    }

    //绘制CAD图形
    if (event->button() == Qt::LeftButton &&
        this->currentDrawing != NOW_DRAWING_NONE &&
        this->isDesignDrag == false) {
        return;
    }

    //主窗口的拖动按钮按下了, 此时支持左键拖动
    if (isDesignDrag == true && event->button() == Qt::LeftButton) {
        isLeftBtnClicked = true;

        //记录右键按下时鼠标指针在view的位置
        hViewPressCoord = event->pos().x();
        vViewPressCoord = event->pos().y();

        //记录滚动条在 右键按下时的位置
        hScrollBarPressValue = horizontalScrollBar()->value();
        vScrollBarPressValue = verticalScrollBar()->value();

        return;
    }
    //按下右键支持拖拽
    else if (event->button() == Qt::RightButton) {
        isRightBtnClicked = true;

        //改变鼠标指针为一个手
        this->setCursor(Qt::ClosedHandCursor);

        //记录右键按下时鼠标指针在view的位置
        hViewPressCoord = event->pos().x();
        vViewPressCoord = event->pos().y();

        //记录滚动条在 右键按下时的位置
        hScrollBarPressValue = horizontalScrollBar()->value();
        vScrollBarPressValue = verticalScrollBar()->value();

        return;
    }

    QGraphicsView::mousePressEvent(event);
}

/* 鼠标双击事件 */
void CanvasViewDesigner::mouseDoubleClickEvent(QMouseEvent *)
{
    if (currentSelectedItemsList.size() == 1) {

    }
}

/* 鼠标移动事件 */
void CanvasViewDesigner::mouseMoveEvent(QMouseEvent *event)
{
    //继续分发事件(下面有return, 所以这里先分发)
    QGraphicsView::mouseMoveEvent(event);

    //如果之前有选中的图形, 说明是鼠标拖动图形功能, 需要隐藏框选矩形
    if (this->scene()->selectedItems().size() != 0 &&
        event->buttons() & Qt::LeftButton) {
        this->isFrameSelectionAvailable = false;
    } else {
        this->isFrameSelectionAvailable = true;
    }

    //设置mainwindow状态栏
    this->mouseViewPos = QPoint(event->x(), event->y());

    //设置设计界面状态栏: 当前坐标
    designWindow->setStatusBarCurrentCoord(
                this->mapToScene(this->mouseViewPos));

    //设置设计界面状态栏: 相对坐标, l a
    designWindow->setStatusBarRelativeCoord(
                this->mapToScene(this->mouseViewPos));

    //处于CAD绘图状态, 记录完坐标后, 立即返回
    if (this->currentDrawing != NOW_DRAWING_NONE) {
        return;
    }

    //右键移动画板
    if ((isRightBtnClicked == true && this->currentDrawing == NOW_DRAWING_NONE) || isDesignDrag == true) {
        emit signalGlobalStepScale();
    }

    //按下鼠标才起作用
    if (isRightBtnClicked == false && isLeftBtnClicked == false) {
        return;
    }

    //获取当前鼠标相对于view的位置
    QPointF cursorViewPoint = event->pos();

    //计算要设置的值
    int hScrollValue = hScrollBarPressValue - (cursorViewPoint.x() - hViewPressCoord);
    int vScrollValue = vScrollBarPressValue - (cursorViewPoint.y() - vViewPressCoord);

    //设置滚动条位置, 以实现拖拽
    horizontalScrollBar()->setValue(hScrollValue);
    verticalScrollBar()->setValue(vScrollValue);
}

/* 鼠标释放事件 */
void CanvasViewDesigner::mouseReleaseEvent(QMouseEvent *event)
{
    //如果之前有选中的图形, 说明是左键鼠标拖动图形功能, 更新接口
    if (this->scene()->selectedItems().size() != 0 &&
        event->button() == Qt::LeftButton) {
        isFrameSelectionLeftBtnPressed = false; //左键松开表示框选左键已经松开
        this->updateMoveItemsOfInterface();
    }
    else if (event->button() == Qt::LeftButton &&
        isDesignDrag == false &&
        selectionMode != SELECTION_MODE_DRAWING) {
        isFrameSelectionLeftBtnPressed = false; //左键松开表示框选左键已经松开
    }

    //绘制中途右键菜单
    if (event->button() == Qt::RightButton && this->currentDrawing != NOW_DRAWING_NONE) {
        this->menuSwitchRightBtn->exec(QCursor::pos());
    }

    //继续分发事件
    QGraphicsView::mouseReleaseEvent(event);

    //如果处于绘制CAD状态
    if (this->currentDrawing != NOW_DRAWING_NONE &&
        event->button() == Qt::LeftButton) {

        this->isDrawingLeftBtnClicked = true;
        this->startStateMachine();
        return;
    }

    if (event->button() == Qt::RightButton) {
        isRightBtnClicked = false;

        if (isDesignDrag == true) {
            this->setCursor(Qt::ClosedHandCursor);
        } else {
            this->setCursor(Qt::ArrowCursor);
        }
    }
    else if (event->button() == Qt::LeftButton && isDesignDrag == true) {
        isLeftBtnClicked = false;
    }
}

/* 鼠标进入事件 */
void CanvasViewDesigner::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);

    if (isDesignDrag == true) {
        this->setCursor(Qt::ClosedHandCursor);
    }
    else {
        switch (this->currentDrawing) {
        case NOW_DRAWING_LINE:           //正在画直线
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_line.png")));
            break;
        case NOW_DRAWING_ARC:            //正在画弧线
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_arc.png")));
            break;
        case NOW_DRAWING_CURVE:          //正在画曲线
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_curve.png")));
            break;
        case NOW_DRAWING_RECTANGLE:      //正在绘制矩形
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_rectangle.png")));
            break;
        case NOW_DRAWING_POLYGON:        //正在绘制多边形
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_polygon.png")));
            break;
        case NOW_DRAWING_3P_CIRCLE:      //正在绘制三点圆
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_3p_circle.png")));
            break;
        case NOW_DRAWING_ELLIPSE:        //正在绘制椭圆
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_ellipse.png")));
            break;
        case NOW_DRAWING_CENTER_CIRCLE:  //正在绘制中心圆
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_center_circle.png")));
            break;
        case NOW_DRAWING_SENDING_CLOTHES_LINE:  //正在绘制发送服装
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_line.png")));
            break;
        case NOW_DRAWING_EMPTY_LINE:            //正在绘制空线
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_line.png")));
            break;
        case NOW_DRAWING_DISTANCE_MEASUREMENT_LINE:   //正在绘制距离测量直线
            this->setCursor(QCursor(QPixmap(":/icon/designwindow/drawing/resource/designwindow/drawing/cursor_cross.png")));
            break;
        //case NOW_DRAWING_COPY_MIRROR:   //正在绘制复制镜像
        //   this->setCursor(Qt::CrossCursor);
        //   break;
        default:
            this->setCursor(Qt::ArrowCursor);
            break;
        }
    }
}

/* 鼠标离开事件 */
void CanvasViewDesigner::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    this->setCursor(Qt::ArrowCursor);
}

/* 键盘按下事件 */
void CanvasViewDesigner::keyPressEvent(QKeyEvent *event)
{
    //完成当前图绘制
    if (this->currentDrawing != NOW_DRAWING_NONE &&
            event->key() == Qt::Key_Space) {
        this->slotActionStopDrawingTriggered();
        return;
    }

    //取消绘制操作
    if (this->currentDrawing != NOW_DRAWING_NONE &&
            event->key() == Qt::Key_Escape) {
        this->setCurrentDrawing(this->currentDrawing);
        return;
    }

    if (event->key() == Qt::Key_Delete) {
        deleteItems();
        return;
    }

    if (event->modifiers() == Qt::ControlModifier) {
        //如果是, 那么再检测A键是否按下
        if(event->key() == Qt::Key_A) {
            switch (selectionMode) {
                case SELECTION_MODE_ITEM:
                    this->selectAllItems();
                    break;
                case SELECTION_MODE_STITCH:
                    this->selectAllStitches();
                    break;
                case SELECTION_MODE_PUNCH:
                    this->selectAllPunches();
                    break;
                default: break;
            }
            return;
        }

        //Ctrl+Z，撤销
        if (event->key() == Qt::Key_Z) {
            this->undo();
            return;
        }

        //Ctrl+Y，重做
        if (event->key() == Qt::Key_Y) {
            this->redo();
            return;
        }

        //快捷键Ctrl+C，复制功能
        if (event->key() == Qt::Key_C &&  currentSelectedItemsList.size() > 0) {
            ctrlCopyItemsList.clear();
            ctrlCopyItemsList = currentSelectedItemsList;
            return;
        }

        //快捷键Ctrl+V，粘贴功能
        if (event->key() == Qt::Key_V && ctrlCopyItemsList.size() > 0) {
            saveGraphicState();         //状态压栈
            isInterfaceChanged = true;

            //图形重复
            interface->ArmCleanSelectObject();
            for (int i = 0; i < ctrlCopyItemsList.size(); i++){
                interface->ArmSelectUintByIndex(1, ctrlCopyItemsList.at(i).at(0) - 1,
                                                ctrlCopyItemsList.at(i).at(1) - 1);
            }
            interface->ArmCopyPasteSelectUint(5, -5);

            dumpInterfaceToScene();
            qSort(ctrlCopyItemsList.begin(), ctrlCopyItemsList.end(),
                [](const QList<int> &listA, QList<int> &listB) {
                return listA.at(1) < listB.at(1);
            });
            for (int i = 1; i < ctrlCopyItemsList.size(); i++) {
                int layer = ctrlCopyItemsList.at(i).at(0);
                int itemId = ctrlCopyItemsList.at(i).at(1);
                itemId += i;
                QList<int> tmpList;
                tmpList.clear();
                tmpList << layer << itemId;
                ctrlCopyItemsList.replace(i, tmpList);
            }
            reSelectCurrentItemsChosen(ctrlCopyItemsList);
            ctrlCopyItemsList.clear();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

/* 初始化撤销与重做功能 */
void CanvasViewDesigner::undoRedoInit(void)
{
    this->redoStack.clear();
    this->undoStack.clear();

    graphicFileSn = 1;
    QDir dir;
    dirTmpFilePath = PARAM_CONFIG_FILE_DIR + "designer.tmp/";
    dir.setPath(dirTmpFilePath);
    if (dir.exists() == true) {
        dir.removeRecursively();
    }

    QDir dirMake;
    dirMake.remove(dirTmpFilePath);
    if(dirMake.mkdir(dirTmpFilePath) == false) {
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("临时文件目录创建失败！"));
    }
}

/* 保存状态 */
int CanvasViewDesigner::graphicFileSn;
void CanvasViewDesigner::saveGraphicState(void)
{
    QString path = dirTmpFilePath + QString("gtmp%1").arg(graphicFileSn++);

    QByteArray fileName = path.toLocal8Bit();
    const char *strFileName = fileName.data();
    interface->ArmFileSaveInfo(100, strFileName);

    //把保存状态的文件的路径压栈
    undoStack.push(path);

    //只要有新的操作产生, 就要清空redoStack
    //同时清掉临时文件
    for (int i = 0; i < redoStack.size(); i++) {
        QFile::remove(redoStack.at(i));
    }
    redoStack.clear();
}

/* 撤销 */
void CanvasViewDesigner::undo(void)
{
    if (undoStack.isEmpty() == true) {
        isInterfaceChanged = false;
        return;
    } else {
        isInterfaceChanged = true;
    }

    //把当前状态压到redoStack里
    QString path = dirTmpFilePath + QString("gtmp%1").arg(graphicFileSn++);

    QByteArray fileName = path.toLocal8Bit();
    const char *strFileName = fileName.data();
    interface->ArmFileSaveInfo(100, strFileName);

    redoStack.push(path);

    //再恢复到上一状态
    QString lastPath = undoStack.pop();
    fileName = lastPath.toLocal8Bit();
    strFileName = fileName.data();
    if (interface != nullptr) {  //这里必须用新的interface来恢复
        delete interface;
        interface = DataFactory::createInstance();
    }
    interface->ArmFileOpenRead(100, strFileName);
    //恢复完成后, 就可以删掉临时文件了
    QFile::remove(lastPath);

    //重绘
    dumpInterfaceToScene();
}

/* 重做 */
void CanvasViewDesigner::redo(void)
{
    if (redoStack.isEmpty() == true) {
        return;
    } else {
        isInterfaceChanged = true;
    }

    //先要保存当前状态到undoStack
    QString path = dirTmpFilePath + QString("gtmp%1").arg(graphicFileSn++);

    QByteArray fileName = path.toLocal8Bit();
    const char *strFileName = fileName.data();
    interface->ArmFileSaveInfo(100, strFileName);
    undoStack.push(path);

    //再恢复到上一状态
    QString lastPath = redoStack.pop();
    fileName = lastPath.toLocal8Bit();
    strFileName = fileName.data();
    if (interface != nullptr) {  //这里必须用新的interface来恢复
        delete interface;
        interface = DataFactory::createInstance();
    }
    interface->ArmFileOpenRead(100, strFileName);
    //恢复完成后, 就可以删掉临时文件了
    QFile::remove(lastPath);

    //重绘
    dumpInterfaceToScene();
}

/* 选中图元水平镜像 */
void CanvasViewDesigner::setMirrorHorizontal(void)
{
    if(currentSelectedItemsList.size() == 0){
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("\n请选中图元！！\n"));
        return;
    }

    //获取选中图元最左和最右边界
    qreal minX, maxX;    //记录选中图元的最左和最右边界
    for(int i = 0; i < currentSelectedItemsList.size(); i++){
        //清除接口选中图元
        interface->ArmCleanSelectObject();
        QList<int> listSelectItemTmp = currentSelectedItemsList.at(i);
        //选中当前图元
        interface->ArmSelectUintByIndex(1, listSelectItemTmp.at(0) - 1, listSelectItemTmp.at(1) - 1);
        //获取图元边界
        double dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);

        if (i == 0) {
            minX = dbL;
            maxX = dbR;
        } else {
            if (minX > dbL) {
                minX = dbL;
            }
            if (maxX < dbR) {
                maxX = dbR;
            }
        }
    }

    //获取中心x位置
    qreal centerX = (minX + maxX) / 2;

    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;
    //保存选中图元
    QList< QList<int> >  tempSelectedItems = currentSelectedItemsList;

    //图元镜像处理
    for(int i = 0; i < currentSelectedItemsList.size(); i++){
        //清除接口选中图元
        interface->ArmCleanSelectObject();
        QList<int> listSelectItemTmp = currentSelectedItemsList.at(i);
        //选中当前图元
        interface->ArmSelectUintByIndex(1, listSelectItemTmp.at(0) - 1, listSelectItemTmp.at(1) - 1);
        //获取图元边界
        double dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
        //镜像操作
        qreal tmpX = (dbL + dbR) / 2;
        if (tmpX > centerX) {
            interface->ArmFlipMirrorSelectUint(3, 2, centerX, 0);
        } else {
            interface->ArmFlipMirrorSelectUint(4, 2, centerX, 0);
        }
    }

    dumpInterfaceToScene();
    reSelectCurrentItemsChosen(tempSelectedItems);
}

/* 选中图元垂直镜像 */
void CanvasViewDesigner::setMirrorVertical(void)
{
    if(currentSelectedItemsList.size() == 0){
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("\n请选中图元！！\n"));
        return;
    }

    //获取选中图元最上和最下边界
    qreal minY, maxY;    //记录选中图元的最上和最下边界
    for(int i = 0; i < currentSelectedItemsList.size(); i++){
        //清除接口选中图元
        interface->ArmCleanSelectObject();
        QList<int> listSelectItemTmp = currentSelectedItemsList.at(i);
        //选中当前图元
        interface->ArmSelectUintByIndex(1, listSelectItemTmp.at(0) - 1, listSelectItemTmp.at(1) - 1);
        //获取图元边界
        double dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);

        if (i == 0) {
            minY = dbB;
            maxY = dbT;
        } else {
            if (minY > dbB) {
                minY = dbB;
            }
            if (maxY < dbT) {
                maxY = dbT;
            }
        }
    }

    //获取中心x位置
    qreal centerY = (minY + maxY) / 2;

    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;
    //保存选中图元
    QList< QList<int> >  tempSelectedItems = currentSelectedItemsList;

    //图元镜像处理
    for(int i = 0; i < currentSelectedItemsList.size(); i++){
        //清除接口选中对象
        interface->ArmCleanSelectObject();
        QList<int> listSelectItemTmp = currentSelectedItemsList.at(i);
        //选中当前图元
        interface->ArmSelectUintByIndex(1, listSelectItemTmp.at(0) - 1, listSelectItemTmp.at(1) - 1);
        //获取图元边界
        double dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
        //镜像操作
        qreal tmpY = (dbT + dbB) / 2;
        if (tmpY > centerY) {
            interface->ArmFlipMirrorSelectUint(1, 2, 0, centerY);
        } else {
            interface->ArmFlipMirrorSelectUint(2, 2, 0, centerY);
        }
    }

    dumpInterfaceToScene();
    reSelectCurrentItemsChosen(tempSelectedItems);
}

/* 以旋转选中图元 */
void CanvasViewDesigner::setRotating(qreal degree)
{
    if(currentSelectedItemsList.size() == 0){
        QMessageBox::warning(this, tr("SewMake数控编制软件"), tr("\n请选中图元！！\n"));
        return;
    }

    saveGraphicState(); //状态压栈
    isInterfaceChanged = true;
    //保存选中图元
    QList< QList<int> >  tempSelectedItems = currentSelectedItemsList;

    for(int i = 0; i < currentSelectedItemsList.size(); i++){
        //清除接口选中
        interface->ArmCleanSelectObject();
        QList<int> listSelectItemTmp = currentSelectedItemsList.at(i);
        //选中当前图元
        interface->ArmSelectUintByIndex(1, listSelectItemTmp.at(0) - 1, listSelectItemTmp.at(1) - 1);
        //获取图元边界
        double dbL, dbT, dbR, dbB;
        interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);

        //中心位置
        qreal centerX = (dbL + dbR) / 2;
        qreal centerY = (dbT + dbB) / 2;

        //旋转
        interface->ArmSelectUintRevolve(degree, centerX, centerY);
    }

    dumpInterfaceToScene();
    reSelectCurrentItemsChosen(tempSelectedItems);
}

/* 再次选中重绘之前的选中列表 */
void CanvasViewDesigner::reSelectCurrentItemsChosen(QList<QList<int>> tempSelectedItems)
{
    //暂时屏蔽通知scene选中情况有变化
    disconnect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);

    currentSelectedItemsList = tempSelectedItems;
    interface->ArmCleanSelectObject();

    for (int i = 0; i < tempSelectedItems.size(); i++) {
        eItem_Type_t itemType;
        int layer = tempSelectedItems.at(i).at(0);
        int itemId = tempSelectedItems.at(i).at(1);
        QGraphicsItem *item = this->getItem(layer, itemId, &itemType);

        switch (itemType) {
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE:
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE:
            //椭圆
            case eITEM_DESIGN_ELLIPSE:
            //矩形
            case eITEM_DESIGN_RECTANGLE:
            //直线
            case eITEM_DESIGN_LINE:
            //多边形
            case eITEM_DESIGN_POLYGON:
            //弧线
            case eITEM_DESIGN_ARC:
            //曲线
            case eITEM_DESIGN_CURVE:
            //空线
            case eITEM_DESIGN_EMPTY_LINE:
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                if (this->selectionMode == SELECTION_MODE_ITEM) {
                    item->setSelected(true);
                }
                interface->ArmSelectUintByIndex(1, layer - 1, itemId - 1);
                break;
            }
            default: break;
        }
    }
    emit signalSceneSelectedGraphic(currentSelectedItemsList);

    //刷新显示设计界面当前图大小
    qreal dbL, dbT, dbR, dbB;
    interface->ArmGetSelectUintLimitRect(&dbL, &dbT, &dbR, &dbB);
    designWindow->setStatusBarSizeOfCurrentPic(fabs(dbL - dbR), fabs(dbT - dbB));

    //恢复屏蔽通知scene选中情况有变化
    connect(this->scene(), &QGraphicsScene::selectionChanged, this, &CanvasViewDesigner::slotSceneSelectionChanged);
}

/* 更新被鼠标挪动的图形的接口数据 */
void CanvasViewDesigner::updateMoveItemsOfInterface(void)
{
    QList<QGraphicsItem *> selectedItemList = this->scene()->selectedItems();
    if (selectedItemList.size() == 0) {
        return;
    }

    qreal moveX, moveY;

    for (int i = 0; i < selectedItemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(selectedItemList.at(i));
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {

            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(selectedItemList.at(i));
                if (itemDesignLine != nullptr) {
                   QPointF posLast = itemDesignLine->getScenePos();
                   moveX = posLast.x() - itemDesignLine->pos().x();
                   moveY = posLast.y() - itemDesignLine->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignLine->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignLine->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignLine->setScenePos(itemDesignLine->pos());
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(selectedItemList.at(i));
                if (itemDesignArc != nullptr) {
                   QPointF posLast = itemDesignArc->getScenePos();
                   moveX = posLast.x() - itemDesignArc->pos().x();
                   moveY = posLast.y() - itemDesignArc->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignArc->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignArc->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignArc->setScenePos(itemDesignArc->pos());
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(selectedItemList.at(i));
                if (itemDesignCurve != nullptr) {
                   QPointF posLast = itemDesignCurve->getScenePos();
                   moveX = posLast.x() - itemDesignCurve->pos().x();
                   moveY = posLast.y() - itemDesignCurve->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignCurve->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignCurve->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignCurve->setScenePos(itemDesignCurve->pos());
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(selectedItemList.at(i));
                if (itemDesignRectangle != nullptr) {
                   QPointF posLast = itemDesignRectangle->getScenePos();
                   moveX = posLast.x() - itemDesignRectangle->pos().x();
                   moveY = posLast.y() - itemDesignRectangle->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignRectangle->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignRectangle->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignRectangle->setScenePos(itemDesignRectangle->pos());
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(selectedItemList.at(i));
                if (itemDesignPolygon != nullptr) {
                   QPointF posLast = itemDesignPolygon->getScenePos();
                   moveX = posLast.x() - itemDesignPolygon->pos().x();
                   moveY = posLast.y() - itemDesignPolygon->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignPolygon->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignPolygon->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignPolygon->setScenePos(itemDesignPolygon->pos());
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(selectedItemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                   QPointF posLast = itemDesign3PCircle->getScenePos();
                   moveX = posLast.x() - itemDesign3PCircle->pos().x();
                   moveY = posLast.y() - itemDesign3PCircle->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesign3PCircle->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesign3PCircle->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesign3PCircle->setScenePos(itemDesign3PCircle->pos());
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(selectedItemList.at(i));
                if (itemDesignEllipse != nullptr) {
                   QPointF posLast = itemDesignEllipse->getScenePos();
                   moveX = posLast.x() - itemDesignEllipse->pos().x();
                   moveY = posLast.y() - itemDesignEllipse->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignEllipse->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignEllipse->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignEllipse->setScenePos(itemDesignEllipse->pos());
                }
                break;
            }
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(selectedItemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                   QPointF posLast = itemDesignCenterCircle->getScenePos();
                   moveX = posLast.x() - itemDesignCenterCircle->pos().x();
                   moveY = posLast.y() - itemDesignCenterCircle->pos().y();

                   //把缝纫点对象, 关键点对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignCenterCircle->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   QList<QGraphicsItem *> punchObjList = itemDesignCenterCircle->getPunchObjList();
                   for (int i = 0; i < punchObjList.size(); i++) {
                       QPointF pos = punchObjList[i]->pos();
                       punchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }

                   itemDesignCenterCircle->setScenePos(itemDesignCenterCircle->pos());
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(selectedItemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                   QPointF posLast = itemDesignEmptyLine->getScenePos();
                   moveX = posLast.x() - itemDesignEmptyLine->pos().x();
                   moveY = posLast.y() - itemDesignEmptyLine->pos().y();

                   //把缝纫点对象对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignEmptyLine->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   itemDesignEmptyLine->setScenePos(itemDesignEmptyLine->pos());
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(selectedItemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                   QPointF posLast = itemDesignSendClothesLine->getScenePos();
                   moveX = posLast.x() - itemDesignSendClothesLine->pos().x();
                   moveY = posLast.y() - itemDesignSendClothesLine->pos().y();

                   //把缝纫点对象对象也移动一下
                   QList<QGraphicsItem *> stitchObjList = itemDesignSendClothesLine->getStitchObjList();
                   for (int i = 0; i < stitchObjList.size(); i++) {
                       QPointF pos = stitchObjList[i]->pos();
                       stitchObjList[i]->setPos(pos.x() - moveX, pos.y() - moveY);
                   }
                   itemDesignSendClothesLine->setScenePos(itemDesignSendClothesLine->pos());
                }
                break;
            }

            default: break;
        }
    }

    this->saveGraphicState();
    interface->ArmMoveSelectUint(-moveX, moveY);

    //设置设计界面整图大小
    qreal leftTopX, leftTopY, rightBottomX, rightBottomY;
    interface->ArmGetScaleOfAllUints(&leftTopX, &leftTopY, &rightBottomX, &rightBottomY);
    designWindow->setStatusBarSizeOfWholePic(fabs(leftTopX - rightBottomX), fabs(leftTopY - rightBottomY));

    updateCenectionLine();
}

/* 更新图形之间的连接线 */
void CanvasViewDesigner::updateCenectionLine(void)
{
    QList<QGraphicsItem *> itemList = this->scene()->items();
    for (int i = 0; i < itemList.size(); i++) {
        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList[i]);
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        if (itemType == eITEM_DOTTED_LINE) {
            delete itemList[i];
        }
    }

    int layerCount;
    interface->ArmGetLayerCount(&layerCount);
    for (int i = 0; i < layerCount; i++) {          //遍历图层

        QList<QPointF> pListBetweenUnit; //保存同一个图层中, 各个图元之间的连接虚线
        pListBetweenUnit.clear();

        int unitCount = interface->ArmGetUintCount(i);
        for (int j = 0; j < unitCount; j++) {       //遍历图层下的图元

            int unitType;
            interface->ArmGetUintTypeByIndex(i, j, &unitType);
            if (unitType == stNullLine || unitType == stNull) {
                int stitchPointCount = interface->ArmGetStitchCount(i, j);
                for (int l = 0; l < stitchPointCount; l++) {
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, l, &tmpX, &tmpY);

                    //第一个图元只记录最后一个关键点
                    if (j == 0 && l == stitchPointCount - 1) {
                        pListBetweenUnit << QPointF(tmpX, -tmpY);
                    }
                    //最后一个图元只记录第一个关键点
                    else if (j == unitCount - 1 && l == 0) {
                        pListBetweenUnit << QPointF(tmpX, -tmpY);
                    }
                    //中间的图元, 第一个关键点和最后一个关键点都要记录
                    else if (0 < j && j < unitCount - 1) {
                        if (l == 0 || l == stitchPointCount - 1) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                    }
                }
            }
            else {
                int punchPointCount = interface->ArmGetPunchCount(i, j);
                for (int k = 0; k < punchPointCount; k++) {
                    double tmpX, tmpY;
                    interface->ArmGetPunchPoint(i, j, k, &tmpX, &tmpY);

                    //第一个图元只记录最后一个关键点
                    if (j == 0 && k == punchPointCount - 1) {
                        pListBetweenUnit << QPointF(tmpX, -tmpY);
                    }
                    //最后一个图元只记录第一个关键点
                    else if (j == unitCount - 1 && k == 0) {
                        pListBetweenUnit << QPointF(tmpX, -tmpY);
                    }
                    //中间的图元, 第一个关键点和最后一个关键点都要记录
                    else if (0 < j && j < unitCount - 1) {
                        if (k == 0 || k == punchPointCount - 1) {
                            pListBetweenUnit << QPointF(tmpX, -tmpY);
                        }
                    }
                }
            }
        }

        if (unitCount > 1) {
            //绘制同一个图层中的图元连接线
            for (int l = 0; l < pListBetweenUnit.size(); l += 2) {
                QList<QPointF> pListTmp;
                pListTmp.clear();
                pListTmp << pListBetweenUnit[l] << pListBetweenUnit[l + 1];
                ItemDottedLine *itemDottedLineGlobal = this->drawDottedLine(pListTmp);
                if (itemDottedLineGlobal != nullptr) {
                    itemDottedLineGlobal->setLayer(i + 1); //设置图层(跟随缝纫点, 也有图层信息)
                    itemDottedLineGlobal->setStyle("dash");
                    itemDottedLineGlobal->setIsShowDottedLine(isShowConnectLine);
                }
            }
        }
    }

    this->scene()->update();
}

/* 显示或隐藏图元序号 */
void CanvasViewDesigner::setShowId(void)
{
    if (isShowItemId == true) {
        isShowItemId = false;
    } else {
        isShowItemId = true;
    }

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                    itemDesignCenterCircle->setIsShowId(isShowItemId);
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                    itemDesign3PCircle->setIsShowId(isShowItemId);
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                if (itemDesignEllipse != nullptr) {
                    itemDesignEllipse->setIsShowId(isShowItemId);
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                if (itemDesignRectangle != nullptr) {
                    itemDesignRectangle->setIsShowId(isShowItemId);
                }
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                if (itemDesignLine != nullptr) {
                    itemDesignLine->setIsShowId(isShowItemId);
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                if (itemDesignPolygon != nullptr) {
                    itemDesignPolygon->setIsShowId(isShowItemId);
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                if (itemDesignArc != nullptr) {
                    itemDesignArc->setIsShowId(isShowItemId);
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                if (itemDesignCurve != nullptr) {
                    itemDesignCurve->setIsShowId(isShowItemId);
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                    itemDesignEmptyLine->setIsShowId(isShowItemId);
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                    itemDesignSendClothesLine->setIsShowId(isShowItemId);
                }
                break;
            }
            default: break;
        }
    }

    this->scene()->update();
}

/* 显示或隐藏轮廓线 */
void CanvasViewDesigner::setIsShowOutline(void)
{
    if (isShowOutline == true) {
        isShowOutline = false;
    } else {
        isShowOutline = true;
    }

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                    itemDesignCenterCircle->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                    itemDesign3PCircle->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                if (itemDesignEllipse != nullptr) {
                    itemDesignEllipse->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                if (itemDesignRectangle != nullptr) {
                    itemDesignRectangle->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                if (itemDesignLine != nullptr) {
                    itemDesignLine->setIsShowOutline(isShowOutline);                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                if (itemDesignPolygon != nullptr) {
                    itemDesignPolygon->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                if (itemDesignArc != nullptr) {
                    itemDesignArc->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                if (itemDesignCurve != nullptr) {
                    itemDesignCurve->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                    itemDesignEmptyLine->setIsShowOutline(isShowOutline);
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                    itemDesignSendClothesLine->setIsShowOutline(isShowOutline);
                }
                break;
            }

            default: break;
        }
    }

    this->scene()->update();
}

/* 显示缝纫点代码小框 */
void CanvasViewDesigner::setIsShowStitchSamllBox(void)
{
    if (isShowStitchSamllBox == true) {
        isShowStitchSamllBox = false;
    } else {
        isShowStitchSamllBox = true;
    }

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //缝纫点
            case eITEM_DESIGN_STITCH_POINT: {
                ItemDesignStitchPoint *itemDesignStitchPoint = dynamic_cast<ItemDesignStitchPoint *>(itemList.at(i));
                if (itemDesignStitchPoint != nullptr) {
                    itemDesignStitchPoint->setIsShowSmallBox(isShowStitchSamllBox);
                }
                break;
            }

            default: break;
        }
    }

    this->scene()->update();
}

/* 显示或隐藏连接线 */
void CanvasViewDesigner::setConnectLine(void)
{
    if (isShowConnectLine == true) {
        isShowConnectLine = false;
    } else {
        isShowConnectLine = true;
    }

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //连接线
            case eITEM_DOTTED_LINE: {
                ItemDottedLine *itemDottedLine = dynamic_cast<ItemDottedLine *>(itemList.at(i));
                if (itemDottedLine != nullptr) {
                    itemDottedLine->setIsShowDottedLine(isShowConnectLine);
                }
                break;
            }

            default: break;
        }
    }

    this->scene()->update();
}

/* 显示或隐藏起终点 */
void CanvasViewDesigner::setStartPoint(void)
{
    if (isShowStartPoint == true) {
        isShowStartPoint = false;
    } else {
        isShowStartPoint = true;
    }

    QList<QGraphicsItem *> itemList = this->scene()->items();

    for (int i = 0; i < itemList.size(); i++) {

        ItemBase *itemBase = dynamic_cast<ItemBase *>(itemList.at(i));
        if (itemBase == nullptr) {
            continue;
        }
        eItem_Type_t itemType = itemBase->getItemType();
        switch (itemType) {
            //直线
            case eITEM_DESIGN_LINE: {
                ItemDesignLine *itemDesignLine = dynamic_cast<ItemDesignLine *>(itemList.at(i));
                if (itemDesignLine != nullptr) {
                    itemDesignLine->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //弧线
            case eITEM_DESIGN_ARC: {
                ItemDesignArc *itemDesignArc = dynamic_cast<ItemDesignArc *>(itemList.at(i));
                if (itemDesignArc != nullptr) {
                    itemDesignArc->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //曲线
            case eITEM_DESIGN_CURVE: {
                ItemDesignCurve *itemDesignCurve = dynamic_cast<ItemDesignCurve *>(itemList.at(i));
                if (itemDesignCurve != nullptr) {
                    itemDesignCurve->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //矩形
            case eITEM_DESIGN_RECTANGLE: {
                ItemDesignRectangle *itemDesignRectangle = dynamic_cast<ItemDesignRectangle *>(itemList.at(i));
                if (itemDesignRectangle != nullptr) {
                    itemDesignRectangle->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //多边形
            case eITEM_DESIGN_POLYGON: {
                ItemDesignPolygon *itemDesignPolygon = dynamic_cast<ItemDesignPolygon *>(itemList.at(i));
                if (itemDesignPolygon != nullptr) {
                    itemDesignPolygon->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //三点圆
            case eITEM_DESIGN_3P_CIRCLE: {
                ItemDesign3PCircle *itemDesign3PCircle = dynamic_cast<ItemDesign3PCircle *>(itemList.at(i));
                if (itemDesign3PCircle != nullptr) {
                    itemDesign3PCircle->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //椭圆
            case eITEM_DESIGN_ELLIPSE: {
                ItemDesignEllipse *itemDesignEllipse = dynamic_cast<ItemDesignEllipse *>(itemList.at(i));
                if (itemDesignEllipse != nullptr) {
                    itemDesignEllipse->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //中心圆
            case eITEM_DESIGN_CENTER_CIRCLE: {
                ItemDesignCenterCircle *itemDesignCenterCircle = dynamic_cast<ItemDesignCenterCircle *>(itemList.at(i));
                if (itemDesignCenterCircle != nullptr) {
                    itemDesignCenterCircle->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //空线
            case eITEM_DESIGN_EMPTY_LINE: {
                ItemDesignEmptyLine *itemDesignEmptyLine = dynamic_cast<ItemDesignEmptyLine *>(itemList.at(i));
                if (itemDesignEmptyLine != nullptr) {
                    itemDesignEmptyLine->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }
            //发送服装线
            case eITEM_DESIGN_SEND_CLOTHES_LINE: {
                ItemDesignSendClothesLine *itemDesignSendClothesLine = dynamic_cast<ItemDesignSendClothesLine *>(itemList.at(i));
                if (itemDesignSendClothesLine != nullptr) {
                    itemDesignSendClothesLine->setIsShowStartPoint(isShowStartPoint);
                }
                break;
            }

            default: break;
        }
    }

    this->scene()->update();
}

/* 显示或隐藏图像 */
void CanvasViewDesigner::setGraphic(bool isSet)
{
    isShowGraphic = isSet;

    //具体功能待做
    //ToDo

}

/* 定位到网格 */
void CanvasViewDesigner::setLocateToGrid(bool isSet)
{
    isLocateToGrid = isSet;
}

/* 定位到针迹点 */
void CanvasViewDesigner::setLocateToStitch(bool isSet)
{
    isLocateToStitch = isSet;

    //更新缝纫点map映射位置
    if (isLocateToStitch == true) {

        mapStitch.clear();
        int layerCount;
        interface->ArmGetLayerCount(&layerCount);
        for (int i = 0; i < layerCount; i++) {

            //遍历图层
            int unitCount = interface->ArmGetUintCount(i);
            for (int j = 0; j < unitCount; j++) {

                //遍历图元
                int stitchCount = interface->ArmGetStitchCount(i, j);
                for (int k = 0; k < stitchCount; k++) {

                    //获取缝纫点位置
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, k, &tmpX, &tmpY);
                    //更新缝纫点map映射
                    int key = (int)tmpX / 10 * 1000000 + (int)tmpY / 10;
                    QList<QPointF> listPointTmp;
                    if (mapStitch.contains(key)) {
                        listPointTmp = mapStitch[key];
                    }
                    listPointTmp << QPointF(tmpX, -tmpY);
                    mapStitch[key] = listPointTmp;
                }
            }
        }
    }
}

/* 定位到轮廓线 */
void CanvasViewDesigner::setLocateToOutline(bool isSet)
{
    isLocateToOutline = isSet;
    updateMapStitchByLocateOutline();
}

/* 定位到轮廓线时更新缝纫点map映射 */
void CanvasViewDesigner::updateMapStitchByLocateOutline(void)
{
    //更新缝纫点map映射位置
    if (isLocateToOutline == true) {

        //保存起始状态
        QString path = dirTmpFilePath + QString("gtmp%1").arg(graphicFileSn++);
        QByteArray fileName = path.toLocal8Bit();
        const char *strFileName = fileName.data();
        interface->ArmFileSaveInfo(100, strFileName);

        int layerCount;
        interface->ArmGetLayerCount(&layerCount);
        interface->ArmCleanSelectObject();

        //调小针距
        for (int i = 0; i < layerCount; i++) {
            interface->ArmSetCurrentLayer(i);
            interface->ArmSelectCurrentLayerAllUnit();
            interface->ArmSetPropertyValue(dbPZzj, i, -1, 0, 0.5);
        }

        //重构
        interface->ArmReBuildSelectUint();
        interface->ArmCleanSelectObject();

        mapStitch.clear();
        for (int i = 0; i < layerCount; i++) {

            //遍历图层
            int unitCount = interface->ArmGetUintCount(i);
            for (int j = 0; j < unitCount; j++) {

                //遍历图元
                int stitchCount = interface->ArmGetStitchCount(i, j);
                for (int k = 0; k < stitchCount; k++) {

                    //获取缝纫点位置
                    qreal tmpX, tmpY;
                    interface->ArmGetStitchPoint(i, j, k, &tmpX, &tmpY);
                    //更新缝纫点map映射
                    int key = (int)tmpX / 10 * 1000000 + (int)tmpY / 10;
                    QList<QPointF> listPointTmp;
                    if (mapStitch.contains(key)) {
                        listPointTmp = mapStitch[key];
                    }
                    listPointTmp << QPointF(tmpX, -tmpY);
                    mapStitch[key] = listPointTmp;
                }
            }
        }

        //再恢复到上一状态
        QByteArray fileName1 = path.toLocal8Bit();
        const char *strFileName1 = fileName1.data();
        interface->ArmFileOpenRead(100, strFileName1);
        //恢复完成后, 就可以删掉临时文件了
        QFile::remove(path);
    }
}
