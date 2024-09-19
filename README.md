# raylibVirtualMicroscropeRPGgame
English discript:



use raylib to create virtual microscope RPG game with some useful algorithm.
zoom 0.01 ~ 40.
algorithm： quicksort camera.
with c11 pthread to visual floodfill method.
simple AABB collision method.
camera funciton:  draft,draw,zoom,follow player.
GPU 5.8 GB as large game map (1000*1000) grid *(30*30) pixel.
or change main function mapi instead of 1000.
click this to know its whole history.
https://www.bilibili.com/video/BV1EDthe7E7Q/?vd_source=6dd875bda9ddfb1fd649643372db5479


my development enviroment

develop version : raylib 4.5
actually only use DrawTexturePro,Initwindow, drawText,Textfomat CLoseWindow raylib API,so there's no special version depending

redpandaC++2.25.1 
windows 10
or lookthrough the vedio link above to find same code compile software installer in vedio discription below the vedio

attention：
Use  Discrete graphics card  with Power when running in laptop could get 50+ FPS

中文描述：
使用Raylib 开发的 显微镜RPG游戏 并使用有用算法。
放大倍数 0.01 到 40。
使用算法包括 快速排序 凸包算法二维极角排序 kruskal 算法 。
简单C11多线程 可视化递归洪水填充。
简单AABB碰撞检测。
摄像机功能： 拖拽，绘制，放大缩小，追踪玩家。
摄像机可拖拽，绘制，滚轮放大缩小。

点击链接可了解开发完整历史。
https://www.bilibili.com/video/BV1EDthe7E7Q/?vd_source=6dd875bda9ddfb1fd649643372db5479

我的开发环境
raylib 4.5
实际上只用到 DrawTexturePro,Initwindow, drawText,Textfomat CLoseWindow raylib 接口，所以没有特别的版本依赖
小熊猫C++ 2.25.1
windows 10
或可以通过链接进入视频下方的简介获取相同代码编译器安装包

注意：
使用独显直连,并接通电源 才可调用GeForce之类的 GPU加速大地图加载 50FPS
