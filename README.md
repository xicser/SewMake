# 概述

- 这个仓库展示了`工业缝制花型设计软件SewMake`项目的一些功能，主要包含`远程升级`、`CAD绘图`、`花型设计`、`撤销与重做`等功能的展示，由于该项目`已经商用`，经过询问源码只能开源`部分代码`，同时展示一些`使用功能`，请您见谅🙏。

- 目录说明：
  > `web_server`：远程升级用的web服务器部分源代码。

  > `sew_client`：客户端软件部分源代码。

  > `images`：本README.md相关图片资源文件。

- 下面的动图如果看不清，您可以点击图片<font color=#FF0000 > 放大 </font>查看。


# 主要功能
SewMake的功能主要分为两个主要部分。
- <font color=#FF0000 >花型浏览：</font>可以实现对已有花型文件的移动、旋转、镜像、图层变换、缝纫点针距设置等功能。同时可以设置批量处理（同时对同一个图层上的所有花型进行统一设置）。
<img src="images/主界面.jpg" alt="formation flight" width="1920">
- <font color=#FF0000 >花型设计：</font>当现有花型不能满足要求的时候，用户可以设计自己需要的花型。SewMake支持可以像CAD绘图一样的花型设计。
<img src="images/花型设计界面.jpg" alt="formation flight" width="1920">

# 重要功能展示
针对上面提到的两大功能模块，下面展示一些SewMake的重要功能。

## CAD绘图功能
SewMake可以像CAD一样设计自己需要的缝纫花型，目前支持的花型包括：`直线`、`弧线`、`曲线`、`矩形`、`多边形`、`三点圆`、`椭圆`、`中心圆`。
<img src="images/CAD绘图.gif" alt="formation flight" width="1920">

绘图原理是使用了状态机的思想，对每个图形的绘制状态进行枚举，鼠标点击引起状态切换。

## 远程升级
SewMake在启动的时候会自动检查更新，当然也可以手动更新，下面这幅图展示了它的自动更新功能。
<img src="images/远程升级.gif" alt="formation flight" width="1920">
远程升级的时候，SewMake会去http服务器上获取最新版本的软件，然后经过MD5校验，此后解压软件包，确认无误才会更新本地的旧版本软件。

>使用libevent库在云服务器上实现了远程升级中要使用的http服务器。

## 模拟缝纫
设计好缝纫花型后，可以进行模拟缝纫，以观察实际的缝纫效果是否符合预期。
<img src="images/模拟缝纫.gif" alt="formation flight" width="1920">

## 花型设计
除了可以打开现有的缝纫花型文件，当当前花型文件不满足要求时，用户可以设计自己需要的花型，因为我们开发了花型设计功能供用户使用。
<img src="images/花型设计.gif" alt="formation flight" width="1920">

## 定位设置
在花型设计过程中，有时候需要把线连接到已有的线上，通过定位设置，可以让鼠标自动附着在已有的线上，不用用户自己点击线，能方便用户使用。
<img src="images/定位到轮廓线.gif" alt="formation flight" width="1920">
上图中可以看到，当选中`定位到轮廓线`时，在绘制图形的过程中，鼠标会自动定位到已有的线上。

## 撤销与重做
既然时图形编辑，难免存在做错了想要撤销或者重做，我们实现了这一功能。
<img src="images/撤销与重做.gif" alt="formation flight" width="1920">

## 批量处理
批量处理可以对同一图层上的所有图形进行设置，如下图所示，当前缝纫花型共有6个图层，图层1上共有19个图形，现在可以方便的使用批量处理功能对图层1上的所有19个图形进行一次性设置。
<img src="images/批量处理.gif" alt="formation flight" width="1920">
可设置的内容包括改变图形头尾长度、镜像、旋转等等。

# 重点功能实现原理
## 远程升级
<center  class="half">
    <img src="images/原理说明/Upgrade更新流程.jpg" alt="formation flight" width="300">
    <img src="images/原理说明/SewMake更新流程.jpg" alt="formation flight" width="400">
</center>

- 客户端分为两部分，`主程序SewMake`和`升级程序Upgrade`，主程序启动时会自动去web服务器上获取最新版本号，和本地对比，如果发现可升级就提示用户是否升级。

- 当用户确认升级时，启动升级程序这个子进程。升级程序会去web服务器上GET新版本主程序zip包，还会获取这个新版主程序zip包的MD5校验值。整个过程主程序处于等待`wait upgrade`状态。

- 如果MD5校验成功，那么升级程序发送`request close`消息来请求关闭主程序。主程序关闭前会通知升级程序，当主程序关闭后，升级程序解压最新版主程序zip，替换掉旧版本主程序，然后启动新的主程序完成升级。

- 期间只要任何一步出错，如MD5校验失败，或者解压失败等，都会给主程序发送upgrade cancel消息，让主程序结束等待，然后升级程序退出，主程序提示用户本次升级失败错误原因。

## 撤销和重做
<center  class="half">
    <img src="images/原理说明/撤销和重做.png" alt="formation flight" width="800">
</center>

- 每次在改变当前画板内容之前，先保存当前画板状态到undo栈，以便后续能够撤销。同时清空redo栈。
- 撤销undo操作：当前画板状态压入redo栈，undo栈弹出一个状态s，把当前画板状态设置为状态s。
- 重做redo操作：当前画板状态压入undo栈，redo栈弹出一个状态s，把当前画板状态设置为状态s。

## CAD绘图状态机
<center  class="half">
    <img src="images/原理说明/撤销和重做.png" alt="formation flight" width="800">
</center>

- 每次在改变当前画板内容之前，先保存当前画板状态到undo栈，以便后续能够撤销。同时清空redo栈。
- 撤销undo操作：当前画板状态压入redo栈，undo栈弹出一个状态s，把当前画板状态设置为状态s。
- 重做redo操作：当前画板状态压入undo栈，redo栈弹出一个状态s，把当前画板状态设置为状态s。
