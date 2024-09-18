

//author @ bilibili 民用级脑的研发记录
// 开发环境 小熊猫c++ 2.25.1  raylib 版本 4.5
// 2024-7-14
// AABB 碰撞检测 在拖拽，绘制，放大缩小中
// 2024-7-20
// 直线改每帧打印一个点，生长的直线，直线炮弹
// 2024-8-4
// 实现敌人追击与敌人死亡与mapv2 敌人轨迹不绘制
// 2024-8-14
// 实现敌人贴图与贴图内碰撞
// 2024-8-23
// 实现连接敌人贴图内的点——总长度最短——kruskal算法
// 2024-8-27
// BFS广度优先搜索实现出口入口寻路指引
#include <raylib.h>
#include <stdio.h>
#include <math.h>
//#include <stdlib.h>
#include <time.h>
#include <pthread.h>

//#include <algorithm>
//#include <cmath>
using namespace std;

#define MAX_VEX 12
#define MAX_EDGE 144

// 重整原因：解决新函数放大缩小之下，raylib 的网格采样部分，选择数组的一部分刷新倒缓冲区里
// 从直接建立缓冲区，到先在数组里进行移动，然后再设置检查缓冲区
// 最大距离 - 当前坐标，实现坐标系变化，同时对应最顶上，变成新坐标下的最底下，可750-1=749数数得到

// 炮弹
typedef struct lineboom {
	int playeriv2 = 0;						// 一开始发射炮弹的玩家坐标
	int playerjv2 = 0;
	int drawiv2 = 0;						// 发射目标位置坐标
	int drawjv2 = 0;
	int oldx = 0;							// 记录上一格子，用于覆盖旧黑色点，产生炮弹轨迹
	int oldy = 0;
	int atking = 0;							// 是否正在攻击，因为有好几个炮弹，命中之后就结束炮弹运行
	int timev3 = 0;							// 记录运行时间或者长度，到达位置，炮弹爆炸。v3是沿用time,和之前代码变量名字一样
//	int toward = -1;						// 朝向，在剥离代码发现不同朝向，直线代码不同，结束比较符号不同
	int isboom = 0;							// 是否到了爆炸的时机，在修改直线代码中发现的可以合并重复代码，产生的控制变量
} lineboom;
// 随机敌人
typedef struct target {
	int targeti = 0;							// 左上角网格坐标
	int targetj = 0;
	int targetwidth = 0;						// 长宽
	int targetheight = 0;
	int islive = 0;
	
//	RenderTexture2D* picture;					// 敌人贴图
	RenderTexture2D picture;					// 敌人贴图
	
	int** oldmap;								// 局部复原数据
	
	int** mapv3;								// 像素也当成网格，每个记录像素是否能穿过
	
	target* area;								// 敌人内部的点
	
	int arealenth;								// 用于记录范围，lenth是半径
	
	int pixnum;									// 洪水填充使用网格大小计算像素边界用
} target;
// 边
//typedef struct link {
//	int startx;									// 起点
//	int starty;
//	int endx;									// 终点
//	int endy;
//	int used;									// 是否已经使用
//	int lenth;									// 距离
//
//} link;

// 边
typedef struct edge {
	int startpoint;							// 起点在数组中的序号
	int endpoint;							// 终点在数组中的序号
	int weight;
} edge;
// 边
typedef struct link {
	int startx;									// 起点的坐标
	int starty;
	int endx;									// 终点的坐标
	int endy;
	int used;									// 是否已经使用
	int lenth;									// 距离
	edge edgelink;								// 需要查点，记录所在数组位置
} link;
// BFS寻路，返回跳转需求下复制粘贴别人的数据格式
typedef struct position {
	int x;
	int y;
	int pre;					// 在数组产生的时候记录来源地
} position;
// 线程需要打包的数据
typedef struct targetv2 {
	int* map;
	int x;
	int** mapv3;
} targetv2;
// 线程锁与线程数据
typedef struct message {
	int flag;
	int a;
	int user;
	void* work;
	int* map;
	// 测试void 类型转换会不会丢失信息
	// 二级指针丢失信息
	int** mapv2;
	target* p;
	void* workv2;
} message;

// 多线程洪水填充，打包数组和敌人数据
typedef struct info {
	int x;
	int y;
	target* enemy;
	int newColor;
	int oldColor;
} info;

// 其中一个线程刷新数组
void* showv2(void* msg) {
	
	static int cnt = 0;
	static int color = 1;
	message* m = (message*)msg;
	
	//成功
//	target* p = (target*)m->work;
	// 也成功
	targetv2* p = (targetv2*)m->workv2;
//	二维数组失效
//	int** meshmap = (int**)m->work;
	
	
	// 一维数组测试
//	int* map =(int*)(*m).work;
//	int* map = m->map;
	int* map = p->map;
	
	int** mapv2 = m->mapv2;
	
	
	// 原来需要存储数据
	int** mapv3 = new int*[500];
	
	
	
	printf("sizeof m->mapv2 %d\n", sizeof(m->mapv2));
	printf("address m->mapv2 %p\n", m->mapv2);
	printf("address m->mapv2[499] %p\n", m->mapv2[499]);
	printf("address m->mapv2[499][499] %p\n", m->mapv2[499][499]);
	
	
//	for(int i=0;i<500;i++){
//		for(int j=0;j<500;j++){
//			printf("%p\n",m->mapv2[i][j]);
//		}
//
//	}
	
//	for(int i=0;i<500;i++){
//		mapv2[i]=m->mapv2[i];
//	}
	
	
	// 发现要装箱，这是成功的测试
	for (int i = 0; i < 500; i++) {
//		mapv3[i]=(int*)m->mapv2[i];
		// 测试敌人数组转换，发现成功
		mapv3[i] = (int*)p->mapv3[i];
//		printf("%d %p\n",i,mapv3[i]);
	}
	
	
	
//	for(int i=0;i<500;i++){
//		for(int j=0;j<500;j++){
////			mapv2[i][j]=m->mapv2[i][j];
//			mapv3[i][j]=m->mapv2[i][j];
//		}
//	}
	
	
	while (1) {
		while (m->flag == 0 || m->a == 1) {
//			_sleep(1);
//			_sleep(0);
			// 休眠一毫秒其实有很大误差
			_sleep(1);
//			printf("B 发现占用中 %d %d %d %d %d\n", m->a, m->user, m->flag, cnt, color);
		}
		m->a = 1;
		m->user = 1;
//		printf("B 占用中\n");
		
		for (int p = 0; p < 1500; p++) {
			cnt += 1;
			if (cnt >= 500 * 500) {
				cnt = 0;
				color += 1;
			}
//			map[cnt] = color;
//			mapv2[cnt/499][cnt%499]=color;
//			mapv3[cnt/499][cnt%499]=color;
			mapv3[cnt / 500][cnt % 499] = color;
//			printf("%d\n", cnt);
		}
		
//		printf("%d %d\n", cnt, color);
		
//		printf("B 释放占用\n");
		m->a = 0;
		_sleep(30);
	}
}



// 并查集查是否连通
int find_root(int roots[], int n) {
	while (roots[n] != -1) {
		n = roots[n];
	}
	return n;
}

// 检测碰撞，AABB碰撞检测，检测到碰撞时，碰撞产生矩形，左上角的坐标是 bangi j ,碰撞长宽是 bangwidth, bangheight
// 复制粘贴自碰撞检测
int bang(int drawi, int drawj, int mousewidth, int mouseheight, int targeti, int targetj, int targetwidth, int targetheight, int *bangi, int *bangj, int *bangwidth, int *bangheight) {
	int isbang = 0;					// 如果碰撞，就返回1
// +变- i 变 j
	if (drawi >= targeti && targeti >= drawi - mouseheight) {
		// 左上角重叠
		if (targeti - targetheight <= drawi - mouseheight) {
			if (drawj <= targetj && targetj <= drawj + mousewidth) {
				// 左上角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i <  mouseheight - (drawi - targeti) ; i++) {
//						for (int j = 0; j <  mousewidth - ( targetj - drawj); j++) {
//							map[targeti - i][targetj + j] = 2024;
//						}
////							map[targeti - i][targetj] = 2024;
//					}
//						for (int j = 0; j <  mousewidth - ( targetj - drawj); j++) {
//							map[targeti][targetj + j] = 2024;
//						}
//					左上角位置
					*bangi = targeti;
					*bangj = targetj;
//					长度，就是注释掉的循环的次数
					*bangwidth = mousewidth - (targetj - drawj);
					*bangheight = -(mouseheight - (drawi - targeti));
					isbang = 1;
				}
				//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i <  mouseheight - (drawi - targeti) ; i++) {
////					map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth; j++) {
//							map[targeti - i][targetj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = targetj;
					// 注意负号，代表向下
					*bangwidth = targetwidth;
					*bangheight = -(mouseheight - (drawi - targeti));
					isbang = 1;
				}
				
			} else if (targetj <= drawj && drawj <= targetj + targetwidth) {
				// 右下角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i <  mouseheight - (drawi - targeti) ; i++) {
////					map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < mousewidth ; j++) {
//							map[targeti - i][drawj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = drawj;
					*bangwidth = mousewidth;
					*bangheight = -(mouseheight - (drawi - targeti));
					isbang = 1;
				}
//			//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
					
//					for (int i = 0; i <  mouseheight - (drawi - targeti) ; i++) {
////					map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth - (drawj - targetj); j++) {
//							map[targeti - i][drawj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = drawj;
					*bangheight = -(mouseheight - (drawi - targeti));
					*bangwidth = targetwidth - (drawj - targetj);
					isbang = 1;
				}
			}
//				for (int i = 0; i <  mouseheight - (drawi - targeti) ; i++) {
//					map[targeti - i][targetj] = 2024;
//				}
		}
		//被包围了
		else if (targeti - targetheight > drawi - mouseheight) {
//				for (int i = 0; i < targetheight; i++) {
//					map[targeti - i][targetj] = 2024;
//				}
			if (drawj <= targetj && targetj <= drawj + mousewidth) {
				// 左上角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < targetheight; i++) {
////							map[targeti - i][targetj] = 2024;
//						for (int j = 0; j <  mousewidth - ( targetj - drawj); j++) {
//							map[targeti - i][targetj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = targetj;
					*bangwidth = mousewidth - (targetj - drawj);
					*bangheight = -targetheight;
					isbang = 1;
				}
				//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < targetheight; i++) {
////							map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth; j++) {
//							map[targeti - i][targetj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = targetj;
					*bangwidth = targetwidth;
					*bangheight = targetheight;
					isbang = 1;
				}
				
			} else if (targetj <= drawj && drawj <= targetj + targetwidth) {
				// 右下角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < targetheight; i++) {
////							map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < mousewidth ; j++) {
//							map[targeti - i][drawj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = drawj;
					*bangwidth = mousewidth;
					*bangheight = targetheight;
					isbang = 1;
				}
//			//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < targetheight; i++) {
////							map[targeti - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth - (drawj - targetj); j++) {
//							map[targeti - i][drawj + j] = 2024;
//						}
//					}
					*bangi = targeti;
					*bangj = drawj;
					*bangwidth = targetwidth - (drawj - targetj);
					*bangheight = -targetheight;
					isbang = 1;
				}
			}
		}
	} else if (targeti >= drawi && drawi >= targeti - targetheight) {
		// 被包围了
		if (targeti - targetheight <= drawi - mouseheight) {
//				for (int i = 0; i < mouseheight  ; i++) {
//					map[drawi - i][targetj] = 2024;
//				}
			if (drawj <= targetj && targetj <= drawj + mousewidth) {
				// 左上角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < mouseheight  ; i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j <  mousewidth - ( targetj - drawj); j++) {
//							map[drawi - i][targetj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = targetj;
					*bangwidth = mousewidth - ( targetj - drawj);
					*bangheight = -mouseheight;
					isbang = 1;
				}
				//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < mouseheight  ; i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth; j++) {
//							map[drawi - i][targetj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = targetj;
					*bangwidth = targetwidth;
					*bangheight = -mouseheight;
					isbang = 1;
				}
				
			} else if (targetj <= drawj && drawj <= targetj + targetwidth) {
				// 右下角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < mouseheight  ; i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < mousewidth ; j++) {
//							map[drawi - i][drawj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = drawj;
					*bangwidth = mousewidth;
					*bangheight = -mouseheight;
					isbang = 1;
				}
//			//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < mouseheight  ; i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth - (drawj - targetj); j++) {
//							map[drawi - i][drawj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = drawj;
					*bangwidth = targetwidth - (drawj - targetj);
					*bangheight = -mouseheight;
					isbang = 1;
				}
			}
		}
		
//			//被包围了
		else if (targeti - targetheight > drawi - mouseheight) {
//			for (int i = 0; i < targetheight - ( targeti - drawi ); i++) {
//				map[drawi - i][targetj] = 2024;
//			}
			if (drawj <= targetj && targetj <= drawj + mousewidth) {
				// 左上角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < targetheight - ( targeti - drawi ); i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j <  mousewidth - ( targetj - drawj); j++) {
//							map[drawi - i][targetj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = targetj;
					*bangwidth = mousewidth - ( targetj - drawj);
					*bangheight = -(targetheight - ( targeti - drawi ));
					isbang = 1;
				}
				//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < targetheight - ( targeti - drawi ); i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth; j++) {
//							map[drawi - i][targetj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = targetj;
					*bangwidth = targetwidth;
					*bangheight = -(targetheight - ( targeti - drawi ));
					isbang = 1;
				}
				
			} else if (targetj <= drawj && drawj <= targetj + targetwidth) {
				// 右下角重叠
				if (targetj + targetwidth >= drawj + mousewidth) {
//					for (int i = 0; i < targetheight - ( targeti - drawi ); i++) {
////							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < mousewidth ; j++) {
////							map[drawi - i][drawj + j] = 2024;
//							map[drawi - i][drawj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = drawj;
					*bangwidth = mousewidth;
					*bangheight = -(targetheight - ( targeti - drawi ));
					isbang = 1;
				}
//			//被包围了
				else if (targetj + targetwidth < drawj + mousewidth) {
//					for (int i = 0; i < targetheight - ( targeti - drawi ); i++) {
//							map[drawi - i][targetj] = 2024;
//						for (int j = 0; j < targetwidth - (drawj - targetj); j++) {
//							map[drawi - i][drawj + j] = 2024;
//						}
//					}
					*bangi = drawi;
					*bangj = drawj;
					*bangwidth = targetwidth - (drawj - targetj);
					*bangheight = -(targetheight - ( targeti - drawi ));
					isbang = 1;
				}
			}
		}
	}
	
	if (isbang == 1) {
		return isbang;
	} else {
		return 0;
	}
}



// 直线攻击检测，恢复炮弹个数
void checkend(lineboom* booms, int boomsum, int* have, int** map, int mapi, int mapj) {
	
	for (int n = 0; n < boomsum; n++) {
		if (booms[n].isboom == 1) {
			for (int i = 0; i < 3; i++) {
				for (int  j = 0; j < 10; j++) {
					// 注意m 增加，数字增加，却是往上增加，所以需要往下走要减
					if (booms[n].drawiv2 - i < 0 || booms[n].drawiv2 - i >= mapi || booms[n].drawjv2 + j < 0 || booms[n].drawjv2 + j >= mapj) {
						break;
					}
					map[booms[n].drawiv2 - i][booms[n].drawjv2 + j] = 1224;
				}
			}
			
			booms[n].atking = 0;
			booms[n].timev3 = 0;
			booms[n].isboom = 0;
			*have += 1;
		}
	}
	
	
}

// 炮弹爆炸
void checkboom(lineboom booms, target* enemy, int** map, int enemysum, int mapi, int mapj) {
	// 直线炮弹碰撞检测
	for (int n = 0; n < enemysum; n++) {
		int bangi = 0;
		int bangj = 0;
		int bangwidth = 0;
		int bangheight = 0;
		// 最小是1，1，这样才绘制一次，此时绘制到炮弹点上
		if (bang(booms.oldy, booms.oldx, 10, 10, enemy[n].targeti, enemy[n].targetj, enemy[n].targetwidth, enemy[n].targetheight, &bangi, &bangj, &bangwidth, &bangheight)) {
			for (int i = 0; i > bangheight; i--) {
				for (int j = 0; j < bangwidth; j++) {
					// m 小于零，因为高度是负数，向下，原因是之前bang 函数代码直接记录符号了，都是向下负数小于零，加一个负数即可
					// 浅蓝色
					if (bangi + i < 0 || bangi + i > mapi || bangj + j < 0 || bangj + j > mapj) {
						break;
					}
					map[bangi + i][bangj + j] = 12;
				}
			}
			// 敌人死亡，这是在敌人移动之后追加的代码
			enemy[n].islive = 0;
//						printf("bang ji width height %d %d %d %d\n", bangj, bangi, bangwidth, bangheight);
			
		}
	}
}
// 直线炮弹移动攻击
void lineatk(lineboom* booms, int boomsum, int** map) {
	float k = 0;
	for (int n = 0; n < boomsum; n++) {
		
		// 如果是正在攻击的炮弹
		if (booms[n].atking == 1) {
			
		} else {
			continue;
		}
		// 如果炮弹已经发射一格
		if (booms[n].timev3 == 0) {
			
		} else {
			// 用黄色覆盖第一个点
			map[booms[n].oldy][booms[n].oldx] = 224;
		}
		
		
		int drawj = booms[n].drawjv2;
		int drawi = booms[n].drawiv2;
		int playeri = booms[n].playeriv2;
		int playerj = booms[n].playerjv2;
		
//			重整，向东走东北，东南偏
		// 追加两个大小情况
		if (drawj - playerj > 0 &&
			// >=0 >0 会导致炮弹不能移动，炮弹数目不恢复
			((drawi - playeri >= 0 && drawi - playeri <= drawj - playerj)
				|| (drawi - playeri < 0 && (-(drawi - playeri)) <= drawj - playerj))) {
			
			// 炮弹到达目的地，就爆炸，往东南北偏时
			if (drawj - playerj <= booms[n].timev3) {
				
				booms[n].isboom = 1;
			}
			
			// 东北偏
			// 注意大于0
			if (drawi - playeri >= 0 && drawi - playeri <= drawj - playerj) {
				k = (drawi - playeri) / (1.0 * ( drawj - playerj));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < drawj - playerj; lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + height][playerj + lenth] = 1949;
//						lenth == (drawj - playerj 改 >=
					if (lenth >= booms[n].timev3 || lenth >= (drawj - playerj)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + height][playerj + lenth] = 1949;
						booms[n].oldy = playeri + height;
						booms[n].oldx = playerj + lenth;
						break;
					}
				}
				
				
			}
			//东南偏
			else if (drawi - playeri < 0 && (-(drawi - playeri)) <= drawj - playerj) {
				k = -(drawi - playeri) / (1.0 * ( drawj - playerj));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < drawj - playerj; lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + (-height)][playerj + lenth] = 1949;
					
					if (lenth >= booms[n].timev3 || lenth == (drawj - playerj)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + (-height)][playerj + lenth] = 1949;
						booms[n].oldy = playeri + (- height);
						booms[n].oldx = playerj + lenth;
						break;
					}
				}
			}
		}
		// 向西走西北，西南偏
		else if (drawj - playerj < 0 &&
			// 注意 == 部分 drawi - playeri <= -(drawj - playerj) 可以等于，这个是斜线
			((drawi - playeri >= 0 && drawi - playeri <= -(drawj - playerj))
				|| (drawi - playeri < 0 && (-(drawi - playeri)) <= -(drawj - playerj)))) {
			// 炮弹到达目的地，就爆炸，往西南北偏时
			
			if (-(drawj - playerj) <= booms[n].timev3) {
				booms[n].isboom = 1;
			}
			// 西北偏
			// 注意drawi - playeri大于0
			if (drawi - playeri >= 0 && drawi - playeri <= -(drawj - playerj)) {
				k = (drawi - playeri) / (1.0 * (-( drawj - playerj)));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < -(drawj - playerj); lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + height][playerj + (-lenth)] = 1949;
					
					if (lenth >= booms[n].timev3 || lenth == -(drawj - playerj)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + height][playerj + (-lenth)] = 1949;
						booms[n].oldy = playeri + height;
						booms[n].oldx = playerj + (-lenth);
						break;
					}
				}
				
			}
			//西南偏
			else if (drawi - playeri < 0 && (-(drawi - playeri)) <= -(drawj - playerj)) {
				k = -(drawi - playeri) / (1.0 * (-( drawj - playerj)));
				int height = 0;
				float a = 0;
				// 注意在测试检测方位后发现没有增加-号
				for (int lenth = 0; lenth < -(drawj - playerj); lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + (-height)][playerj + (-lenth)] = 1949;
					
					if (lenth >= booms[n].timev3 || lenth == (drawj - playerj)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + (-height)][playerj + (-lenth)] = 1949;
						booms[n].oldy = playeri + (- height);
						booms[n].oldx = playerj + (-lenth);
						break;
					}
				}
			}
			
		}
		
		
//					 重整，向北走东北，西北偏
		if (drawi - playeri > 0 &&
			((drawi - playeri > 0 && drawi - playeri > drawj - playerj && drawj - playerj >= 0)
				|| (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj <= 0) )) {
			
			if (drawi - playeri <= booms[n].timev3) {
				booms[n].isboom = 1;
			}
			
			// 东北偏
			// 注意大于0
			
			if (drawi - playeri > 0 && drawi - playeri > drawj - playerj && drawj - playerj >= 0) {
				k = (drawj - playerj) / (1.0 * ( drawi - playeri));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < drawi - playeri; lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + lenth][playerj + height] = 1949;
//						lenth == (drawi - playeri 改>= 解决炮弹不回复问题
					if (lenth >= booms[n].timev3 || lenth >= (drawi - playeri)) {
						booms[n].timev3 = lenth + 1;
//							booms[n].timev3 = lenth + 10;
						map[playeri + lenth][playerj + height ] = 1949;
						booms[n].oldy = playeri + lenth;
						booms[n].oldx = playerj + height ;
						break;
					}
				}
			}
			// 西北偏
			
			// 注意drawi - playeri大于0
//				drawj - playerj <= 0 等于0是竖直的线
			// 发现bug 间距黑色线，改lenth 间隔发现黑色点间距变化
			// 注意垂直部分，大于0，但不是等于0，和西北偏和东北偏如果在一块，会有垂直的黑色间距线
//				if (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj <= 0) {
			if (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj < 0) {
				k = -(drawj - playerj) / (1.0 * ( drawi - playeri));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < drawi - playeri; lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + lenth][playerj + (-height)] = 1949;
					
					if (lenth >= booms[n].timev3 || lenth >= (drawi - playeri)) {
						booms[n].timev3 = lenth + 1;
//							booms[n].timev3 = lenth + 3;
						map[playeri + lenth][playerj + (-height)] = 1949;
						booms[n].oldy = playeri + lenth;
						booms[n].oldx = playerj + (-height);
						break;
					}
				}
//					map[drawi][drawj] = 224;
			}
		}
		// 向南走东南，西南偏
		else if (drawi - playeri < 0 &&
			((drawi - playeri < 0 && -(drawi - playeri) > drawj - playerj && drawj - playerj >= 0)
				|| (drawi - playeri < 0 && (-(drawi - playeri)) > -(drawj - playerj) && drawj - playerj <= 0))) {
			
			if (-(drawi - playeri) <= booms[n].timev3) {
				booms[n].isboom = 1;
			}
			
			// 东南偏
			// 注意drawi - playeri大于0
			if (drawi - playeri < 0 && -(drawi - playeri) > drawj - playerj && drawj - playerj >= 0) {
				k = (drawj - playerj) / (1.0 * (-( drawi - playeri)));
				int height = 0;
				float a = 0;
				for (int lenth = 0; lenth < -(drawi - playeri); lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + (-lenth)][playerj + height] = 1949;
					
					if (lenth >= booms[n].timev3 || lenth >= -(drawi - playeri)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + (-lenth)][playerj + height ] = 1949;
						booms[n].oldy = playeri + (-lenth);
						booms[n].oldx = playerj + height;
						break;
					}
				}
				
//					map[drawi][drawj] = 224;
			}
			//西南偏
			else if (drawi - playeri < 0 && (-(drawi - playeri)) > -(drawj - playerj) && drawj - playerj <= 0) {
				k = -(drawj - playerj) / (1.0 * (-( drawi - playeri)));
				int height = 0;
				float a = 0;
				// 注意在测试检测方位后发现没有增加-号
				for (int lenth = 0; lenth < -(drawi - playeri); lenth++) {
					a = a + k;
					if (a > 1) {
						a -= 1;
						height += 1;
					} else {
						
					}
//						map[playeri + (-lenth)][playerj + (-height)] = 1949;
					if (lenth >= booms[n].timev3 || lenth >= -(drawi - playeri)) {
						booms[n].timev3 = lenth + 1;
						map[playeri + (-lenth)][playerj + (-height)] = 1949;
						booms[n].oldy = playeri + (-lenth);
						booms[n].oldx = playerj + (-height);
						break;
					}
				}
			}
			
		}
	}
}
// 画直线
void drawline(int drawi, int drawj, int playeri, int playerj, int** map, int sign, int mapi, int mapj) {
	float k = 0;
	
//			重整，向东走东北，东南偏
	// 追加两个大小情况
	if (drawj - playerj > 0 &&
		// >=0 >0 会导致炮弹不能移动，炮弹数目不恢复
		((drawi - playeri >= 0 && drawi - playeri <= drawj - playerj)
			|| (drawi - playeri < 0 && (-(drawi - playeri)) <= drawj - playerj))) {
		
		// 东北偏
		// 注意大于0
		if (drawi - playeri >= 0 && drawi - playeri <= drawj - playerj) {
			k = (drawi - playeri) / (1.0 * ( drawj - playerj));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < drawj - playerj; lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
				}
				if (playeri + height >= 0 && playeri + height < mapi && playerj + lenth >= 0 && playerj + lenth < mapj) {
					map[playeri + height][playerj + lenth] = sign;
				}
				
			}
		}
		//东南偏
		else if (drawi - playeri < 0 && (-(drawi - playeri)) <= drawj - playerj) {
			k = -(drawi - playeri) / (1.0 * ( drawj - playerj));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < drawj - playerj; lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
				}
				if (playeri + (-height) >= 0 && playeri + (-height) < mapi && playerj + lenth >= 0 && playerj + lenth < mapj) {
					map[playeri + (-height)][playerj + lenth] = sign;
				}
				
			}
		}
	}
	// 向西走西北，西南偏
	else if (drawj - playerj < 0 &&
		// 注意 == 部分 drawi - playeri <= -(drawj - playerj) 可以等于，这个是斜线
		((drawi - playeri >= 0 && drawi - playeri <= -(drawj - playerj))
			|| (drawi - playeri < 0 && (-(drawi - playeri)) <= -(drawj - playerj)))) {
		// 炮弹到达目的地，就爆炸，往西南北偏时
		
		// 西北偏
		// 注意drawi - playeri大于0
		if (drawi - playeri >= 0 && drawi - playeri <= -(drawj - playerj)) {
			k = (drawi - playeri) / (1.0 * (-( drawj - playerj)));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < -(drawj - playerj); lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
				}
//				if (playeri + height >= 0 && playeri + height < mapi && playerj + (-lenth) >= 0 && playerj + (-lenth) < mapi) {  8-24-2024 修正划不来直线
				if (playeri + height >= 0 && playeri + height < mapi && playerj + (-lenth) >= 0 && playerj + (-lenth) < mapj) {
					map[playeri + height][playerj + (-lenth)] = sign;
				}
			}
		}
		//西南偏
		else if (drawi - playeri < 0 && (-(drawi - playeri)) <= -(drawj - playerj)) {
			k = -(drawi - playeri) / (1.0 * (-( drawj - playerj)));
			int height = 0;
			float a = 0;
			// 注意在测试检测方位后发现没有增加-号
			for (int lenth = 0; lenth < -(drawj - playerj); lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
				}
				if (playeri + (-height) >= 0 && playeri + (-height) < mapi && playerj + (-lenth) >= 0 && playerj + (-lenth) < mapj) {
					map[playeri + (-height)][playerj + (-lenth)] = sign;
				}
			}
		}
	}
	
//					 重整，向北走东北，西北偏
	if (drawi - playeri > 0 &&
		((drawi - playeri > 0 && drawi - playeri > drawj - playerj && drawj - playerj >= 0)
			|| (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj <= 0) )) {
		
		// 东北偏
		// 注意大于0
		if (drawi - playeri > 0 && drawi - playeri > drawj - playerj && drawj - playerj >= 0) {
			k = (drawj - playerj) / (1.0 * ( drawi - playeri));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < drawi - playeri; lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
				}
				if (playeri + lenth >= 0 && playeri + lenth < mapi && playerj + height >= 0 && playerj + height < mapj) {
					map[playeri + lenth][playerj + height] = sign;
				}
			}
		}
		// 西北偏
		
		// 注意drawi - playeri大于0
//				drawj - playerj <= 0 等于0是竖直的线
		// 发现bug 间距黑色线，改lenth 间隔发现黑色点间距变化
		// 注意垂直部分，大于0，但不是等于0，和西北偏和东北偏如果在一块，会有垂直的黑色间距线
//				if (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj <= 0) {
		if (drawi - playeri > 0 && drawi - playeri > -(drawj - playerj) && drawj - playerj < 0) {
			k = -(drawj - playerj) / (1.0 * ( drawi - playeri));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < drawi - playeri; lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
					
				}
				if (playeri + lenth >= 0 && playeri + lenth < mapi && playerj + (-height) >= 0 && playerj + (-height) < mapj) {
					map[playeri + lenth][playerj + (-height)] = sign;
				}
				
			}
		}
	}
	// 向南走东南，西南偏
	else if (drawi - playeri < 0 &&
		((drawi - playeri < 0 && -(drawi - playeri) > drawj - playerj && drawj - playerj >= 0)
			|| (drawi - playeri < 0 && (-(drawi - playeri)) > -(drawj - playerj) && drawj - playerj <= 0))) {
		
		// 东南偏
		// 注意drawi - playeri大于0
		if (drawi - playeri < 0 && -(drawi - playeri) > drawj - playerj && drawj - playerj >= 0) {
			k = (drawj - playerj) / (1.0 * (-( drawi - playeri)));
			int height = 0;
			float a = 0;
			for (int lenth = 0; lenth < -(drawi - playeri); lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
					
				}
				if (playeri + (-lenth) >= 0 && playeri + (-lenth) < mapi && playerj + height >= 0 && playerj + height < mapj) {
					map[playeri + (-lenth)][playerj + height] = sign;
				}
				
			}
			
		}
		//西南偏
		else if (drawi - playeri < 0 && (-(drawi - playeri)) > -(drawj - playerj) && drawj - playerj <= 0) {
			k = -(drawj - playerj) / (1.0 * (-( drawi - playeri)));
			int height = 0;
			float a = 0;
			// 注意在测试检测方位后发现没有增加-号
			for (int lenth = 0; lenth < -(drawi - playeri); lenth++) {
				a = a + k;
				if (a > 1) {
					a -= 1;
					height += 1;
				} else {
					
				}
				if (playeri + (-lenth) >= 0 && playeri + (-lenth) < mapi && playerj + (-height) >= 0 && playerj + (-height) < mapj) {
					map[playeri + (-lenth)][playerj + (-height)] = sign;
				}
				
			}
		}
	}
}

// 寻路，对照原始代码和游戏结构体参数改写
void BFS(int startx, int starty, int endx, int endy, target* enemys, int pixnum) {
	
	position toward[4] = {
		{1, 0, 99999},				// 东
		{0, 1, 99999},				// 北
		{-1, 0, 99999},				// 西
		{0, -1, 99999},				// 南
	};
	// 记录地图
	static int **mapv4;				// static 避免重复创建分配位置，实现重复利用
	static int flag = 0;
	if (flag == 0) {
		mapv4 = new int*[30000];
		for (int n = 0; n < 30000; n++) {
			mapv4[n] = new int[30000];
		}
		flag = 1;
	}
	
	// 复制地图
	// 注意 乘pinxum
	for (int i = 0; i < enemys->targetheight * pixnum; i++) {
		for (int j = 0; j < enemys->targetwidth * pixnum; j++) {
			mapv4[i][j] = enemys->mapv3[i][j];
		}
	}
	
//	开始点，和结束点，拿出一个点，是第一层，把周围相邻的点排队到后面，是第二层
	int front;
	int rear;
	front = -1;
	rear = 0;
	
	position queue[100000];
	position start;					// 穷举起点
	position check;					// 检测数据
	position goal;					// 目标数据
	
	goal.x = endx;
	goal.y = endy;
	
	
	start.x = startx;
	start.y = starty;
	start.pre = -1;					// 第一个点增加记录开头
	queue[0] = start;
	
	while (front != rear) {
		front++;
		check = queue[front];
		
		
		if (check.x == goal.x && check.y == goal.y) {
//			return 1;
			int k = front;
			position yes;
			
			// 往回跳转
			while (k != -1) {					// 跳转不到最开始的一个就一直跳转
				yes = queue[k];
				k = yes.pre;
				enemys->mapv3[yes.y][yes.x] = 1; // 记录为1
				
				for (int n = 0; n < 4; n++) {
//					int x = yes.x + toward[n].x;
					int x = yes.x + n;
//					int y = yes.y + toward[n].y;
					int y = yes.y + n;
					if (x >= 0 && x < enemys->targetwidth * pixnum && y >= 0 && y < enemys->targetheight * pixnum && enemys->mapv3[y][x] != 1) {
						enemys->mapv3[y][x] = 1;			// 1记录为走得通的路线
						
					}
				}
			}
			printf("find way BFS Yes\n");
			return ;
		}
		// 四个方向产生下一批点去排队
		for (int n = 0; n < 4; n++) {
			position next;
			next.x = check.x + toward[n].x;
			next.y = check.y + toward[n].y;
			// 边界检测，概括为 合规检测，包括但不限于：边界，可通过点
			if (next.y >= 0 && next.y < enemys->targetheight * pixnum
				&& next.x >= 0 && next.x < enemys->targetwidth * pixnum
				&& mapv4[next.y][next.x] == 254) {
				next.pre = front;						// 记录在上一层的来源
				rear++;
				queue[rear] = next;
				
//				queue[rear].pre = front;				// 记录在上一层的来源
				
				// mapv3 不能被覆盖，
				// 因为寻路会标记使用过的点，会改变一开始的地图，
//				 因为我要保持原貌，对比寻路效果
				mapv4[next.y][next.x] = 1;			// 1记录为走得通的路线
				
				
			}
//			printf("%d\n",rear);
		}
	}
	
	printf("BFS find no way\n");
	printf("front num = %d\n", front);
	enemys->islive = 0;								// 如果寻路失败，要塞不移动，方便看出来这个失败的图像
}


//热心网友赞助的凸包算法代码
//https://www.luogu.com.cn/article/td3ah746

// 检查叉积大于0，是的话，v1是逆时针转到v2
// cmp正常，叉积则也正常
// 最后是打印连线，发现没有错误，排序排除完，在查连线，结果发现 pos 替换point解决问题，
int checkv2(position startv1, position endv1, position startv2, position endv2) {
	return (endv1.x - startv1.x) * (endv2.y - startv2.y) - (endv2.x - startv2.x) * (endv1.y - startv1.y);
}
// 勾股定理两点间距离
double distance(position p1, position p2) {
	return sqrt((p2.y - p1.y) * (p2.y - p1.y) * 1.0 + (p2.x - p1.x) * (p2.x - p1.x));
}
// 比较角度大小——复制粘贴别人的代码
// 发现问题 对应p[0],p[1]
// 原来是别人的代码有误
// 打印连线之后正常
int cmp(position origin, position p1, position p2) {
//bool cmp(position origin, position p1, position p2) {
	int flag = 0;
//	double flag = 0;
	// 存储叉乘数据,理想情况是 return 1,代表 p2角度大于p1,p1顺时针转到p2,此时p2在数组最后，角越大，越在后
	flag = checkv2(origin, p1, origin, p2);
//	判断符号
	if (flag > 0) {
		return 1;
	}
	// 如果在一条直线上，数据保留按距离长的
	if (flag == 0 && distance(origin, p1) < distance(origin, p2)) {
		return 1;
	}
	
	return 0;
}


// 改变数据类型进行快速排序
void quicksortv3(int start, int end, position* pos) {
	int left;
	int right;
	left = start;
	right = end;
	// 注意一开始是最低点pos[0] 或者上一次循环刚排序完的点pos[left]
	// 不是了，是剩下的点排序
	position check = pos[left];
	
	// 注意start >end 没有检测会导致死循环
	if (start < end) {
//	if (left< right) {
		
		while (left < right) {
//			while (left < right && a[check] <= a[right]) {
//			while (left < right && check <= a[right]) {
//			while (left < right && (cmp(check, pos[left], pos[right]) == 1)) {
//			while (left < right && (cmp(pos[0], pos[left], pos[right]) == 1)) {
//			后面的角大，就改写顺序
			// 后面的角大，就不换，大角对应大序号
			while (left < right && (cmp(pos[0], check, pos[right]) == 1)) {
				right--;
			}
//		a[check]=a[right];
			pos[left] = pos[right];
//			while (left < right && a[left] <= a[check]) {
//			while (left < right && a[left] <= check) {
			// 增加取反，因为理想情况返回1，原来返回1是大角，现在返回1是小角，原来返回0是小角
//			while (left < right && (cmp(check, pos[left], pos[right]) == 0)) {
//			while (left < right && (cmp(pos[0], pos[left], pos[right]) == 0)) {
			// 后面的角大，就不改写顺序
//			while (left < right && (cmp(pos[0], pos[left], check) == 0)) {
			// 解决死循环，大的角就在最后
			while (left < right && (cmp(pos[0], pos[left], check) == 1)) {
				left++;
			}
			pos[right] = pos[left];
//			printf("ok\n");
		}
		
		pos[left] = check;
//	quicksort(0,left,a);
//	quicksort(left+1,right,a);
		
		quicksortv3(start, left - 1, pos);
		quicksortv3(left + 1, end, pos);
	}
}

// 洪水填充算法
void floodFill4(int x, int y, int newColor, int oldColor, int**map, int mapi, int mapj) {
//	if(x >= 0 && x < width && y >= 0 && y < height
//		&& getPixel(x, y) == oldColor && getPixel(x, y) != newColor)
	if (y >= 0 && y < mapi && x > 0 && x < mapj && map[y][x] == oldColor && map[y][x] != newColor) {
//		setPixel(x, y, newColor); //set color before starting recursion
		map[y][x] = newColor;
//		floodFill4(x + 1, y, newColor, oldColor);
//		floodFill4(x - 1, y, newColor, oldColor);
//		floodFill4(x, y + 1, newColor, oldColor);
//		floodFill4(x, y - 1, newColor, oldColor);
		
		floodFill4(x + 1, y, newColor, oldColor, map, mapi, mapj);
		floodFill4(x, y + 1, newColor, oldColor, map, mapi, mapj);
		floodFill4(x - 1, y, newColor, oldColor, map, mapi, mapj);
		floodFill4(x, y - 1, newColor, oldColor, map, mapi, mapj);
	}
}
// 多线程内部，发现可以使用旧函数慢慢跑数据
void* floodFill4v3(message* msg, int x, int y, int newColor, int oldColor, int**map, int mapi, int mapj) {
	
	while (msg->a == 1) {
//		_sleep(50);
		_sleep(10);
//		_sleep(20);
//		printf("%d 发现 %d 占用数据msg flag %d\n", pthread_self(), msg->user, msg->flag);
	}
//	printf("%d 正在执行洪水填充\n", pthread_self());
//	msg->a = 1;
	msg->user = pthread_self() * (-1);
	msg->flag += 1;
//	printf("%d 线程使用数据 次数：msg flag %d\n", pthread_self(), msg->flag);
	if (y >= 0 && y < mapi && x >= 0 && x < mapj && map[y][x] == oldColor && map[y][x] != newColor) {
//		printf("数组地址 %p\n", &map[y][x]);
		map[y][x] = newColor;
//		printf("%d %d %d正在绘制坐标\n", x, y, map[y][x]);
//		msg->a = 0;
		
		// 四个休眠必须有 100 配 0 cntv3 ,i=5刷新是 54帧率
//		_sleep(100);
//		10 配 0 cntv3 ,i=5刷新是 34帧率
//		30 配 0 cntv3 ,i=5刷新是 60帧率
		_sleep(10);
//		_sleep(30);
//		_sleep(60);
//		floodFill4v3(msg, x + 1, y, newColor, oldColor, map, mapi, mapj);
//		先上后下，然后坐标mpi - (y+1),可知是往下生长
		floodFill4v3(msg, x, y + 1, newColor, oldColor, map, mapi, mapj);
//		_sleep(100);
		_sleep(10);
//		_sleep(30);
//		_sleep(60);
//		floodFill4v3(msg, x, y + 1, newColor, oldColor, map, mapi, mapj);
		
		floodFill4v3(msg, x + 1, y, newColor, oldColor, map, mapi, mapj);
//		_sleep(100);
		_sleep(10);
//		_sleep(30);
//		_sleep(60);
		floodFill4v3(msg, x - 1, y, newColor, oldColor, map, mapi, mapj);
//		_sleep(100);
		_sleep(10);
//		_sleep(30);
//		_sleep(60);
		floodFill4v3(msg, x, y - 1, newColor, oldColor, map, mapi, mapj);
//		_sleep(100);
		_sleep(10);
//		_sleep(30);
//		_sleep(60);
	}
	
}
// 多线程洪水填充
void* floodfill4v2(void* m) {
//	printf("%d 线程正在执行\n", pthread_self());
	message* msg = (message*)m;
	info* have = (info*)msg->workv2;
	
	int x = have->x;
	int y = have->y;
	int newColor = have->newColor;
	int oldColor = have->oldColor;
	int mapi = have->enemy->targetheight * (have->enemy->pixnum);
	int mapj = have->enemy->targetwidth * (have->enemy->pixnum);
	
	int** map = new int*[mapi];
	for (int i = 0; i < mapi; i++) {
		map[i] = have->enemy->mapv3[i];
	}
	
	// 等待
	while (msg->a == 1) {
		_sleep(50);
//		printf("%d 正在等待\n", pthread_self());
	}
//	printf("%d 线程填充检测\n", pthread_self());
//	msg->a = 1;
	msg->user = pthread_self();
	_sleep(100);
	if (y >= 0 && y < mapi && x >= 0 && x < mapj && map[y][x] == oldColor && map[y][x] != newColor) {
		map[y][x] = newColor;
//		msg->a = 0;
//		printf("%d 线程填充一次完成\n", pthread_self());
//		floodFill4v3(msg, x + 1, y, newColor, oldColor, map, mapi, mapj);
		// 修改填充方向测试模拟水流
		floodFill4v3(msg, x, y + 1, newColor, oldColor, map, mapi, mapj);
		_sleep(100);
//		floodFill4v3(msg, x, y + 1, newColor, oldColor, map, mapi, mapj);
		floodFill4v3(msg, x + 1, y, newColor, oldColor, map, mapi, mapj);
		_sleep(100);
		floodFill4v3(msg, x - 1, y, newColor, oldColor, map, mapi, mapj);
		_sleep(100);
		floodFill4v3(msg, x, y - 1, newColor, oldColor, map, mapi, mapj);
	}
	
}


int main() {
	
	int boomsum;
	boomsum = 100;
	
	// 炮弹数目记录
	int have = boomsum;
	// 我方直线炮弹
	lineboom* booms = new lineboom[boomsum];
	for (int i = 0; i < boomsum; i++) {
		booms[i].playeriv2 = 0;
		booms[i].playerjv2 = 0;
		booms[i].drawiv2 = 0;
		booms[i].drawjv2 = 0;
		booms[i].oldx = 0;
		booms[i].oldy = 0;
		booms[i].timev3 = 0;
		booms[i].atking = 0;
		booms[i].isboom = 0;
	}
	
	// 敌人的炮弹爆炸
	int boomsumv2 = 100;
	int havev2 = 100;
	// 敌人发射的直线炮弹
	lineboom* boomsv2 = new lineboom[boomsumv2];
	for (int i = 0; i < boomsumv2; i++) {
		boomsv2[i].playeriv2 = 0;
		boomsv2[i].playerjv2 = 0;
		boomsv2[i].drawiv2 = 0;
		boomsv2[i].drawjv2 = 0;
		boomsv2[i].oldx = 0;
		boomsv2[i].oldy = 0;
		boomsv2[i].timev3 = 0;
		boomsv2[i].atking = 0;
		boomsv2[i].isboom = 0;
	}
	
	
	int pixnum;
	// 屏幕显示的方格个数
	int showi;
	int showj;
//	比屏幕多出的边距
	int side;																// 可变边距测试成功
	// 一个缓冲区的大小
	int bkmeshmapmaxi;
	int bkmeshmapmaxj;
	int bkmeshwidth;
	int bkmeshheight;
//	一个游戏地图的大小
	int mapi;
	int mapj;
	
//	pixnum = 30;
//	pixnum = 15;															// mesh中一个网格正方形的边长
//	pixnum = 64;															// mesh中一个网格正方形的边长
//	pixnum = 130;															// mesh中一个网格正方形的边长
	pixnum = 30;															// mesh中一个网格正方形的边长
//	pixnum = 3;																// mesh中一个网格正方形的边长
	int pixnumv2;
// 显示区的网格大小，是沿用之前的pixnum但是自己设定也没问题，于是产生新的变量，
//	其实是测试pixnum对帧率影响，结果发现是drawi j 影响大
	pixnumv2 = 30;
	
//	30*100大小测试结果
	// 参数：核显 41帧
//	Renderer: Intel(R) Iris(R) Xe Graphics
//	INFO:     > Version:  3.3.0 - Build 31.0.101.4502
//	INFO:     > GLSL:     3.30 - Build 31.0.101.4502
	
//	RTX3060 57帧，被遮挡47帧率
//	> Vendor:   NVIDIA Corporation
//	INFO:     > Renderer: NVIDIA GeForce RTX 3060 Laptop GPU/PCIe/SSE2
//	INFO:     > Version:  3.3.0 NVIDIA 512.78
//	INFO:     > GLSL:     3.30 NVIDIA via Cg compiler
	
	
//	showi = 30;
//	showj = 30;
//	showi = 600;
//	showj = 600;
//	showi = 150;
//	showj = 150;
//	showi = 200;
//	showj = 200;
//	showi = 100;												// 测试破案了，单个瓦片刷新没有影响，但是绘制Draw Texture 是主要限制，100*100每个循环，改pixnum不影响
//	showj = 100;
//	showi = 500;												// 测试破案了，单个瓦片刷新没有影响，但是绘制Draw Texture 是主要限制，100*100每个循环，改pixnum不影响
//	showj = 500;
//	showi = 1500;
//	showj = 1500;
//	showi = 1000;
//	showj = 1000;
//	showi = 1100;												// 1500*1500 1200*1200 超GPU显存了
//	showj = 1100;
	// 注意刷新规模上限70帧下，最多每帧发送100*100的绘制小正方形瓦片，密度是100*100/（1000*1000） =1/100=1%的平均密度
//	showi = 1000;												// 1000*1000*30*30 像素个数是界限了
//	showj = 1000;
//	showi = 100;												// 1000*1000*30*30 像素个数是界限了
//	showj = 100;
//	showi = 30;													// 1000*1000*30*30 像素个数是界限了
//	showj = 30;
//	showi = 1000;												// 测试大地图碰撞检测
//	showj = 1000;
	showi = 1001;												// 测试大地图碰撞检测
	showj = 1001;
//	150 50+帧率 99%GPU 82°C
//	还是100
//	showi = 150;
//	showj = 150;
//	showi = 10;													// 测试破案了，单个瓦片刷新没有影响，但是绘制Draw Texture 是主要限制，100*100每个循环，改pixnum不影响
//	showj = 10;
//	showi = 200;												// 测试破案了，单个瓦片刷新没有影响，但是绘制Draw Texture 是主要限制，100*100每个循环，改pixnum不影响
//	showj = 200;
//	去除刷新检测，可以100 持续57帧率
//	side=20;
//	side = showj*0.2;
//	side = 0;
//	side = 10;													// 绘制四个角的红白矩形抖动，原因是 后面的 if对draftx进行平移，平移后采样区域回到靠左或靠右，出现复位
//	side = 1;
//	side = 1;													// while 边界检测+1解决+0导致的死循环允许side=0
//	side = 1;													// while 边界检测+1解决+0导致的死循环允许side=0 改完由于偏移于是不准确了，又拒绝了+1，于是不能设置为0，保留窗口抖动
//	side = 10;													// while 边界检测+1解决+0导致的死循环允许side=0 改完由于偏移于是不准确了，又拒绝了+1，于是不能设置为0，保留窗口抖动,但是拒绝边界检测又测试允许0
	side = 0;
	if (showi * showj > 100 * 100) {							// 如果超出GPU网格刷新界限就变成单宫格模式
		side = 0;
	} else {
		side = 1;
	}
	// while 边界检测+1解决+0导致的死循环允许side=0 改完由于偏移于是不准确了，又拒绝了+1，于是不能设置为0，保留窗口抖动,但是拒绝边界检测又测试允许0
//	修正，原来是由于绘制GPU画布采样界限，多余部分会自动镜像绘制，数组越界却没有发生
//	再解释，破案了，数组越界但是没闪退，又有边界检测结果采样不会采样到出界部分。
//	于是增加边界检测
	
//	边界处理
	bkmeshmapmaxi = side + showi + side;								// 象形表示，左右各有这样的边距
	bkmeshmapmaxj = side + showj + side;
	bkmeshwidth = bkmeshmapmaxj * pixnum;								// 用于瓦片刷新
	bkmeshheight = bkmeshmapmaxi * pixnum;
	
//	mapi = bkmeshmapmaxi * 20;
//	mapj = bkmeshmapmaxj * 20;
	//测试边界
//	mapi = bkmeshmapmaxi * 1;
//	mapj = bkmeshmapmaxj * 1;
//	mapi = bkmeshmapmaxi * 2;
//	mapj = bkmeshmapmaxj * 2;
//	mapi = bkmeshmapmaxi * 20;
//	mapj = bkmeshmapmaxj * 20;
	
	// 加上else 出现黑边BUG
//	if (side == 0) {
//		mapi = bkmeshmapmaxi * 1;					// side=0 则只有一个宫格
//		mapj = bkmeshmapmaxj * 1;
//	} else {
//		mapi = bkmeshmapmaxi * 20;
//		mapj = bkmeshmapmaxj * 20;
//	}
	
	if (side == 0) {
		mapi = bkmeshmapmaxi * 1;					// side=0 则只有一个宫格
		mapj = bkmeshmapmaxj * 1;
	} else {
		mapi = bkmeshmapmaxi * 5;
		mapj = bkmeshmapmaxj * 5;
	}
	
	int **map;
	int **meshmap;
	int maxgamex;									// 允许的最大位置
	int maxgamey;
	
	int** mapv2;									// 解决敌人轨迹覆盖地图
	
	target* region;								// 记录像素玩家所在区域，在哪个敌人区域上，用于减少比较，性能优化
	region = NULL;
	int regionflag = 0;								// 不在区域里
	// 回滚代码，原因是采样区没法修改罢了
//	maxgamey = mapi * 30 - showi * 30;					// 左上角拖拽距离游戏边界差一个背景采样区大小
//	maxgamex = mapj * 30 - showj * 30;
	maxgamey = mapi * pixnum - showi * pixnum;			// 左上角拖拽距离游戏边界差一个背景采样区大小
	maxgamex = mapj * pixnum - showj * pixnum;
	// 但是发现可以修改粘贴的大小
	int showiv2;									// 解决showi，j导致显示不全，边距变化后，检测限制的BUG
	int showjv2;
//	showiv2 = 30;
//	showjv2 = 30;
	showiv2 = 40;
	showjv2 = 60;
	//	记录bkmeshmap 网格,用于出界刷新
	int bkmeshmapi;
	int bkmeshmapj;
	bkmeshmapi = 0;
//	bkmeshmapi = 0;									// 解决黑边BUG 原来是复制粘贴初始化重复了导致没初始化，对应在后面的参考点变化
	bkmeshmapj = 0;
	
	//	拖拽边界
	int limitleft;
	int limittop;
	
//	坐标系变化，现在是在底部进行，左下角是0，0原点
	limitleft = 0;
	limittop = 0;
	
	
	map = new int*[mapi];
	for (int i = 0; i < mapi; i++) {
		map[i] = new int[mapj];
	}
	for (int i = 0; i < mapi; i++) {
		for (int j = 0; j < mapj; j++) {
			map[i][j] = (j + i) % 27;					// 测试数据，渐变 25改105 柔和
		}
	}
	//	测试边界
	for (int j = 0; j < mapj; j++) {
		map[0][j] = 9999;
		map[mapi - 1][j] = 9999;
	}
	for (int i = 0; i < mapi; i++) {
		map[i][0] = 9999;
		map[i][mapj - 1] = 9999;
	}
	meshmap = new int*[bkmeshmapmaxi];
	for (int i = 0; i < bkmeshmapmaxi; i++) {
		meshmap[i] = new int[bkmeshmapmaxj];
	}
	for (int i = 0; i < bkmeshmapmaxi; i++) {
		for (int j = 0; j < bkmeshmapmaxj; j++) {
			meshmap[i][j] = -1;
		}
	}
	
	
	mapv2 = new int*[mapi];
	for (int i = 0; i < mapi; i++) {
		mapv2[i] = new int[mapj];
	}
	
	for (int i = 0; i < mapi; i++) {
		for (int j = 0; j < mapj; j++) {
			mapv2[i][j] = map[i][j];
		}
	}
	
	// 目标个数
	int enemysum;
//	enemysum=20;
//	enemysum=100;				// 100 个就闪退，原来是敌人绘制网格出界，这是在鼠标点击炮弹击中边界闪退得到的
//	enemysum=50;
//	enemysum=150;
//	enemysum = 50;
//	enemysum = 250;
//	enemysum = 50;
	
	// 多线程测试
//	enemysum = 10;
	enemysum = 50;
	// 随机数查的方法,_time64 获取时间，srand设置随机数种子
	
	// 这俩都行，保证每次开机都不一样
//	SetRandomSeed((unsigned)_time64(NULL));
	srand((unsigned)_time64(NULL));
	
//	a = rand();
	
	int enemylive;
	
	enemylive = enemysum;			// 存活敌人个数，用于敌人死亡不立即刷新
	
	target* enemy = new target[enemysum];
	for (int i = 0; i < enemysum; i++) {
		enemy[i].islive = 1;
		enemy[i].targeti = GetRandomValue(0, mapi - 1);
		enemy[i].targetj = GetRandomValue(0, mapj - 1);
		enemy[i].targetwidth = GetRandomValue(10, 20);
		enemy[i].targetheight = GetRandomValue(10, 20);
		
		// 敌人随机贴图
		// 在3. 测试Texture 之后发现需要在Initwindows 之后使用，原因是上下文Opengl 还没设置
//		enemy[i].picture = new RenderTexture2D();
//		enemy[i].picture =  LoadRenderTexture(enemy[i].targetwidth * pixnum, enemy[i].targetheight * pixnum);
//		RenderTexture mesh = LoadRenderTexture(enemy[i].targetwidth * pixnum, enemy[i].targetheight * pixnum);
//		enemy[i].picture = &mesh;
		
//		enemy[i].picture = LoadRenderTexture(enemy[i].targetwidth , enemy[i].targetheight );
		
//		BeginTextureMode(enemy[i].picture);
//		for (int y = 0; y < enemy[i].targetheight * pixnum; y++) {
//			for (int x = 0; x < enemy[i].targetwidth * pixnum; x++) {
//				DrawPixel(x, y, {x%255, y%255, x * y%255, 255});
//			}
//		}
//		EndTextureMode();
		
		// 记录采样数据，用于恢复网格地图，覆盖旧贴图
		enemy[i].oldmap = new int*[100];
		for (int n = 0; n < 100; n++) {
			enemy[i].oldmap[n] = new int[100];
		}
		for (int n = 0; n < enemy[i].targetheight; n++) {
			for (int m = 0; m < enemy[i].targetwidth; m++) {
//				enemy[i].oldmap[n][m] = map[enemy[i].targeti - n][enemy[i].targetj + m];
				// 追加边界检测，解决闪退问题
				if (enemy[i].targeti - n >= 0 && enemy[n].targeti - n < mapi && enemy[i].targetj + m >= 0 && enemy[i].targetj + m < mapj) {
					enemy[i].oldmap[n][m] = map[enemy[i].targeti - n][enemy[i].targetj + m];
				}
			}
		}
		
		// 记录局部区域像素网格
		enemy[i].mapv3 = new int*[1000];
		for (int n = 0; n < 1000; n++) {
			enemy[i].mapv3[n] = new int[1000];
		}
		
		for (int n = 0; n < enemy[i].targetheight * pixnum; n++) {
			for (int m = 0; m < enemy[i].targetwidth * pixnum; m++) {
//				enemy[i].mapv3[n][m] = (m + n) % 255;
//				enemy[i].mapv3[n][m] = (m + n) % 250;						// 254是白色，用于绘制房间，黑色覆盖时，不覆盖白色
				enemy[i].mapv3[n][m] = 0;						// 254是白色，用于绘制房间，黑色覆盖时，不覆盖白色
			}
		}
		
//		生成入口
		enemy[i].area = new target[100];
		int choose = GetRandomValue(1, 4);	//随机四个方向选一个方向
//		int choose = 4;
		enemy[i].area[0].targetwidth = GetRandomValue(36, 50);									// 先长宽
		enemy[i].area[0].targetheight = GetRandomValue(30, 50);
		enemy[i].area[0].arealenth = 0;														// 设置距离很大
		if (choose == 1) {																		// 东边，最右边产生一个区域，可以当成入口
//			enemy[i].area[0].targetwidth = GetRandomValue(6, 20);								// 先长宽
//			enemy[i].area[0].targetheight = GetRandomValue(10, 20);
			enemy[i].area[0].targetj = enemy[i].targetwidth * pixnum - enemy[i].area[0].targetwidth;										// 再坐标，防止越界
			enemy[i].area[0].targeti = GetRandomValue(0, enemy[i].targetheight * pixnum - enemy[i].area[0].targetheight);
		} else if (choose == 2) {																							// 北边，上边
//			enemy[i].area[0].targetj = enemy[i].targetj;
//			enemy[i].area[0].targeti = enemy[i].targeti;
			enemy[i].area[0].targetj = GetRandomValue(0, enemy[i].targetwidth * pixnum - enemy[i].area[0].targetwidth);										// 再坐标，防止越界
			enemy[i].area[0].targeti = 0;
			
		} else if (choose == 3) {																								// 西边
			enemy[i].area[0].targetj = 0;
			enemy[i].area[0].targeti = GetRandomValue(0, enemy[i].targetheight * pixnum - enemy[i].area[0].targetheight);
			
		} else if (choose == 4) {																								// 南边
			enemy[i].area[0].targetj = GetRandomValue(0, enemy[i].targetwidth * pixnum - enemy[i].area[0].targetwidth);
			enemy[i].area[0].targeti = enemy[i].targetheight * pixnum - enemy[i].area[0].targetheight;
		}
		
		
		
		link** linkside = new link*[110];
		
		for (int i = 0; i < 110; i++) {
			linkside[i] = new link[110];
		}
		
		for (int i = 0; i < 110; i++) {
			for (int j = 0; j < 110; j++) {
				linkside[i][j].used = 1;
			}
		}
		
		for (int n = 1; n <= 10; n++) {
			enemy[i].area[n].targetwidth = GetRandomValue(30, 50);									// 先长宽
			enemy[i].area[n].targetheight = GetRandomValue(30, 50);
			enemy[i].area[n].targetj = GetRandomValue(0, enemy[i].targetwidth * pixnum - enemy[i].area[n].targetwidth);
			enemy[i].area[n].targeti = GetRandomValue(0, enemy[i].targetheight * pixnum - enemy[i].area[n].targetheight);
		}
		
		// 生成出口
		int check = 1;
//		int minlenth = 99999;
		int goal = 0;
		choose = GetRandomValue(1, 4);	//随机四个方向选一个方向
		enemy[i].area[11].targetwidth = GetRandomValue(30, 50);									// 先长宽
		enemy[i].area[11].targetheight = GetRandomValue(30, 50);
		if (choose == 1) {																		// 东边，最右边产生一个区域，可以当成入口
//			enemy[i].area[0].targetwidth = GetRandomValue(6, 20);								// 先长宽
//			enemy[i].area[0].targetheight = GetRandomValue(10, 20);
			enemy[i].area[11].targetj = enemy[i].targetwidth * pixnum - enemy[i].area[11].targetwidth;										// 再坐标，防止越界
			enemy[i].area[11].targeti = GetRandomValue(0, enemy[i].targetheight * pixnum - enemy[i].area[11].targetheight);
		} else if (choose == 2) {																							// 北边，上边
//			enemy[i].area[0].targetj = enemy[i].targetj;
//			enemy[i].area[0].targeti = enemy[i].targeti;
			enemy[i].area[11].targetj = GetRandomValue(0, enemy[i].targetwidth * pixnum - enemy[i].area[11].targetwidth);										// 再坐标，防止越界
			enemy[i].area[11].targeti = 0;
			
		} else if (choose == 3) {																								// 西边
			enemy[i].area[11].targetj = 0;
			enemy[i].area[11].targeti = GetRandomValue(0, enemy[i].targetheight * pixnum - enemy[i].area[11].targetheight);
			
		} else if (choose == 4) {																								// 南边
			enemy[i].area[11].targetj = GetRandomValue(0, enemy[i].targetwidth * pixnum - enemy[i].area[11].targetwidth);
			enemy[i].area[11].targeti = enemy[i].targetheight * pixnum - enemy[i].area[11].targetheight;
		}
		
//		增加数据
		
		for (int n = 0; n <= 11; n++) {
			for (int m = 0; m <= 11; m++) {
				int startx = enemy[i].area[n].targetj;
				int starty = enemy[i].area[n].targeti;
				int endx = enemy[i].area[m].targetj;
				int endy = enemy[i].area[m].targeti;
				linkside[n][m].startx = enemy[i].area[n].targetj;
				linkside[n][m].starty = enemy[i].area[n].targeti;
				linkside[n][m].endx = enemy[i].area[m].targetj;
				linkside[n][m].endy = enemy[i].area[m].targeti;
				linkside[n][m].used = 0;
				if (n == m) {
					linkside[n][m].used = 1;
//					linkside[n][m].lenth=999999;
//					linkside[n][m].edgelink.weight=999999;
				} else {
					linkside[n][m].lenth = sqrt((startx - endx) * (startx - endx) + (starty - endy) * (starty - endy));
				}
				
//				printf("%d\n",linkside[n][m].lenth);
				
			}
//			printf("%d %d\n",linkside[n][0].startx,linkside[n][0].starty);
		}
		
		
		
		
//		for (int n = 0; n < 11; n++) {
//			int goal = 2;
//			int lenth = 999999;
//
//			// 找当前最小边
//			for (int m = 0; m < 11; m++) {
//				if (linkside[n][m].used == 1) {
//
//				} else {
//					if (linkside[n][m].lenth < lenth) {
//						lenth = linkside[n][m].lenth;
//						goal = m;
//					}
//				}
//			}
//
//			// 连线
//			for (int m = 0; m < 10; m++) {
//				drawline(linkside[n][goal].starty + m, linkside[n][goal].startx, linkside[n][goal].endy + m, linkside[n][goal].endx, enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
//				drawline(linkside[n][goal].starty, linkside[n][goal].startx + m, linkside[n][goal].endy, linkside[n][goal].endx + m, enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
////				drawline(0,0, 550,550,enemy[i].mapv3, 254, enemy[i].targetheight*pixnum, enemy[i].targetwidth*pixnum);
//
//			}
//			linkside[n][goal].used = 1;					// 连完的线
////			linkside[goal][n].used = 1;
//		}
		
		
		
		// 求最短总长度的连线Kruskal 算法
		
//		link linkside[12][12];			// 兼容之前参数
		int vex = 12;					//顶点个数
		int edgeside = 12 * 12;			//边个数
		
		static link graph[MAX_EDGE];	// static 避免重复分配内存
		static edge graphv2[MAX_EDGE];
		
		/*
		//	for (int k = 0; k < edgeside; k++) {
		//		for (int n = 0; n <= 11; n++) {
		//			for (int m = 0; m <= 11; m++) {
		//				if (linkside[n][m].used == 0) {
		//					graph[k].startx = linkside[n][m].startx;
		//					graph[k].starty = linkside[n][m].starty;
		//					graph[k].endx = linkside[n][m].endx;
		//					graph[k].endy = linkside[n][m].endy;
		//					graph[k].lenth = linkside[n][m].lenth;
		//					graph[k].used = 1;
		//
		//					graphv2[k].startpoint=n;
		//					graphv2[k].endpoint=m;
		//					graph[k].lenth=linkside[n][m].lenth;
		//				}
		//			}
		//		}
		//	}
		*/
		
		for (int n = 0; n < 144; n++) {
			graph[n].used = 0;
		}
//
		int k = 0;
		for (int n = 0; n <= 11; n++) {
			for (int m = 0; m <= 11; m++) {
				if (linkside[n][m].used == 0) {
					
					graph[k].startx = linkside[n][m].startx;
					graph[k].starty = linkside[n][m].starty;
					graph[k].endx = linkside[n][m].endx;
					graph[k].endy = linkside[n][m].endy;
					graph[k].lenth = linkside[n][m].lenth;
					graph[k].used = 1;
					
					graph[k].edgelink.startpoint = n;
					graph[k].edgelink.endpoint = m;
					graph[k].edgelink.weight = linkside[n][m].lenth;
					k++;
				}
			}
		}
		
//	标记不用数据
		for (int n = 0; n < MAX_EDGE; n++) {
			if (graph[n].used == 0) {
				graph[n].lenth = 999999;
				graph[n].edgelink.weight = 999999;
			}
		}
		
		// 插入排序
		link max;
		int index;
		for (int n = 0; n < MAX_EDGE; n++) {
			index = n;
			max = graph[n];
			for (int m = n + 1; m < MAX_EDGE; m++) {
				if (graph[m].lenth < max.lenth) {
					max = graph[m];
					index = m;
				}
			}
			graph[index] = graph[n];
			graph[n] = max;
		}
		
		
		static int roots[MAX_VEX];  	//根数组，存放各顶点的根节点，以区别是否属于同一个集合
		static edge MST[MAX_EDGE];		//存放最小生成树（minimum spanning tree）
		int count = 0;
		
		
		for (int n = 0; n < MAX_VEX; n++) {
			roots[n] = -1;
		}
		
		for (int n = 0; n < MAX_EDGE; n++) {
			
			if (graph[n].used == 1) {
				int vex_m = find_root(roots, graph[n].edgelink.startpoint);
				int vex_n = find_root(roots, graph[n].edgelink.endpoint);
				if (vex_m != vex_n) {				//如果两者的根节点不同，说明他们属于不同的集合，可以相连
					MST[count] = graph[n].edgelink;//将此边放入MST数组
					count++;
					roots[vex_m] = vex_n;			//将两个树合并，即将顶点vex_n作为vex_m的根节点
					
				}
				if (count == vex - 1) {
					break;
				}
				
			}
		}
		// 绘制最短连接路线
		for (int k = 0; k < vex - 1; k++) {
//			printf("(%d,%d)%d\n", MST[k].startpoint, MST[k].endpoint, MST[k].weight);   //打印最小生成树
			
			int start = MST[k].startpoint;
			int end = MST[k].endpoint;
			
			for (int n = 0; n < 10; n++) {
				drawline(linkside[start][end].starty + n,
					linkside[start][end].startx,
					linkside[start][end].endy + n,
					linkside[start][end].endx,
					enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
				drawline(linkside[start][end].starty,
					linkside[start][end].startx + n,
					linkside[start][end].endy,
					linkside[start][end].endx + n,
					enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
				
			}
			
		}
		
		
		// 绘制区域
		// 注意是等于11，第十二个
		for (int n = 0; n <= 11; n++) {
			for (int x = 0; x < enemy[i].area[n].targetwidth; x++) {
				for (int y = 0; y < enemy[i].area[n].targetheight; y++) {
//					0<= 漏掉等号导致最顶上像素没覆盖，以为是数组对应错位
					if (0 <= enemy[i].area[n].targeti + y
						&& enemy[i].area[n].targeti  + y < enemy[i].targetheight * pixnum
						&& 0 <= enemy[i].area[n].targetj + x
						&& enemy[i].area[n].targetj + x < enemy[i].targetwidth * pixnum) {
						enemy[i].mapv3[enemy[i].area[n].targeti + y][enemy[i].area[n].targetj + x] = 254;
					}
				}
			}
		}
		
		
		// 寻路部分
		int startx;
		int starty;
		int endx;
		int endy;
//
//		startx=enemy[0].targetj;
//		starty=enemy[0].targeti;
//		endx=enemy[11].targetj;
//		endy=enemy[11].targeti;
		
		// 注意是 area ，敌人要塞内部区域
		startx = enemy[i].area[0].targetj;
		starty = enemy[i].area[0].targeti;
		endx = enemy[i].area[11].targetj;
		endy = enemy[i].area[11].targeti;
		
		// 寻路，绘制轨迹
		BFS(startx, starty, endx, endy, &enemy[i], pixnum);
		
		
		
//		连接边界，形成包围圈，使用凸包算法
		
		
		position pos[1000];							// 一堆随机点
		position point[1000];						// 目标点，且有顺序
		// 发现要有最小角，需要拿最底下的顶点，于是需要比较，在输入数据时
//	pos[0].x = enemy.area[0].targetj;
//	pos[0].y = enemy.area[0].targeti;
		
		position temp;
		
		// 数据加载
		// 数组标号从0开始，沿用游戏代码从0开始，于是原来从1开始就改成0开始了
		
		for (int n = 0; n < 12; n++) {
			pos[n].x = enemy[i].area[n].targetj;
			pos[n].y = enemy[i].area[n].targeti;
			// 找到最小数据
			// 由于mpi -targeti 情况，导致最小的点，在最顶上边界
			if (n != 0 && pos[n].y < pos[0].y) {
				// 然后改成，因为后面连线是逆时针，后面连线不变，所以要改前面点
//			if (n != 0 && pos[n].y > pos[0].y) {
				temp = pos[0];
				pos[0] = pos[n];
				pos[n] = temp;
			}
		}
		// 数组标号从0开始，沿用游戏代码从0开始，于是原来从1开始就改成0开始了
		quicksortv3(1, 11, pos);
		
		
//		检查绘制线，因为之前没法打印数据，而且绘制结果有的点没绘制，查看数据
//		for (int n = 1; n < 12; n++) {
		// 少打印一个点看数据，发现总是绕到最左，说明数据排序正常
//		for (int n = 1; n < 11; n++) {
//		for (int n = 1; n < 5; n++) {
//			for (int m = 0; m < 10; m++) {
//				drawline(
//				    pos[0].y+m,
//				    pos[0].x,
//				    pos[n].y+m,
//				    pos[n].x,
//				    enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
//				drawline(
//				    pos[0].y,
//				    pos[0].x+m,
//				    pos[n].y,
//				    pos[n].x+m,
//				    enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
//
//			}
//		}
		
//		cmp参数多，原来只能两个
//		sort(pos+1,pos+12,cmp);
		
		// 先排序再加点
		// 最低点在凸包边界上，所以最低点直接加入
		point[0] = pos[0];
		
		// 把第二个点放入凸包记录区
//		point[1]=pos[1];
		
		// 数组标号从0开始，沿用游戏代码从0开始，于是原来从1开始就改成0开始了
//		quicksortv3(1, 12, pos);
		
		int cnt = 0;
//		for (int n = 1; n < 11; n++) {
//		for (int n = 1; n < 12; n++) {
//		for (int n = 2; n < 12; n++) {
		for (int n = 1; n < 12; n++) {
			// 如果叉乘小于0，就是又顺时针旋转了，说明有点在边界里面。
//			while (cnt > 1 && checkv2(point[cnt - 1], point[cnt], point[cnt + 1], point[cnt]) <= 0) {
			// 比较前一个直线和下一个直线叉乘正负判断点在线左右
//			while (cnt > 0 && checkv2(point[cnt - 1], point[cnt], point[cnt], point[n]) <= 0) {
//			while (cnt > 0 && checkv2(point[cnt - 1], point[cnt], point[cnt], point[n]) >= 0) {
//			while (cnt > 0 && checkv2(point[cnt - 1], point[cnt], point[cnt], point[n]) < 0) {
			// 是pos在剩下的点里选
//			while (cnt > 0 && checkv2(point[cnt - 1], point[cnt], point[cnt], pos[n]) <= 0) {
			while (cnt > 0 && checkv2(point[cnt - 1], point[cnt], point[cnt], pos[n]) < 0) {
				// 点回退
				cnt--;
			}
			// 加入点
			cnt++;
			point[cnt] = pos[n];
		}
		
		// 再把第一个点加入，相对于最后一个点连到开始处
//		point[cnt + 1] = pos[0];
		// 禁用调试完复用
		point[cnt + 1] = pos[0];
		
//		printf("%d\n", cnt);
//		double ans = 0.0f;
//		for (int n = 0; n < cnt; n++) {
//		for (int n = 0; n < cnt+1; n++) {
//		for (int n = 0; n < cnt; n++) {
		for (int n = 0; n < cnt + 1; n++) {
//			ans+=distance(point[i],point[i+1]);
			for (int m = 0; m < 10; m++) {
				drawline(
					point[n].y + m,
					point[n].x,
					point[n + 1].y + m,
					point[n + 1].x,
					enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
				drawline(
					point[n].y,
					point[n].x + m,
					point[n + 1].y,
					point[n + 1].x + m,
					enemy[i].mapv3, 254, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
				
			}
		}
		
		
		
		// 洪水填充
//		floodFill4(enemy[i].area[0].targetj + 15, enemy[i].area[0].targeti + 15, 777, 254, enemy[i].mapv3, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
		
	}
	
	
	// 随便一个敌人记录为所在地区，只是为了不让region为空，减少比较
	region = &enemy[0];
	
//	enemy[0].picture = LoadRenderTexture(50,50);
	
	// 绘制敌人
	for (int n = 0; n < enemysum; n++) {
		for (int i = 0; i < enemy[n].targetheight; i++) {
			for (int j = 0; j < enemy[n].targetwidth; j++) {
				// 发现是没有越界检测所以导致100 个时，碰到边界闪退
				if (enemy[n].targeti - i < 0 || enemy[n].targeti - i >= mapi || enemy[n].targetj + j < 0 || enemy[n].targetj + j >= mapj) {
					break;
				} else {
					map[enemy[n].targeti - i][enemy[n].targetj + j] = 1;
				}
				
			}
		}
	}
	
	
	
	
//	初始化窗口
//	InitWindow(1750, 1050, "test for location");
//	InitWindow(300+showiv2*pixnumv2, 300+showjv2*pixnumv2, "test for location");
	InitWindow(showjv2 * pixnumv2, showiv2 * pixnumv2, "test for location");
//	设置GPU可以操作的画布,一定要再初始化窗口之后才行，实际上是OpenGL的上下文，或者说默认环境设置
	RenderTexture2D mesh = LoadRenderTexture(bkmeshmapmaxi * pixnum, bkmeshmapmaxj * pixnum);
	
	// 这俩可以追加敌人贴图
	for (int n = 0; n < enemysum; n++) {
		enemy[n].picture = LoadRenderTexture(enemy[n].targetwidth * pixnum, enemy[n].targetheight * pixnum);
		BeginTextureMode(enemy[n].picture);
		for (int y = 0; y < enemy[n].targetheight * pixnum; y++) {
			for (int x = 0; x < enemy[n].targetwidth * pixnum; x++) {
//				DrawPixel(x, y, {x % 255, y % 255, x * y % 255, 255});
				// 数组增加后，是从下往上绘制，从下往上打印，数组对应从0到999
//				DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
				// 重新测试边界,查坐标系，看数组和像素是否对应
				if (enemy[n].mapv3[y][x] == 777) {
					DrawPixel(x, y, {125, 25, 175, 255});				// 8-29-2024 洪水填充
				} else if (enemy[n].mapv3[y][x] == 1) {	// 8-26-2024 增加寻路打印数据
					DrawPixel(x, y, {0, 255, 0, 255});
				} else if (enemy[n].mapv3[y][x] == 254) {
					DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
				} else if (y == 0) {
					DrawPixel(x, y, {0, 255, 255, 255});
				} else {
					DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
				}
				
				
			}
		}
		EndTextureMode();
	}
	
	// 设置缩放过滤器模式
	// 查找raylib.h得到
//	SetTextureFilter(mesh.texture,TEXTURE_FILTER_ANISOTROPIC_16X);			// 缩小，帧率会下降到50+
	
	// 这两个带明暗了
//	GenTextureMipmaps(&mesh.texture);                               //为Texture生成GPU mipmap
//	https://www.cnblogs.com/wuyuan2011woaini/p/15655883.html
//	SetTextureFilter(mesh.texture,TEXTURE_FILTER_TRILINEAR);		// 线性滤波
	
//	GenTextureMipmaps(&mesh.texture);                               //为Texture生成GPU mipmap
//	各向异性过滤
//	SetTextureFilter(mesh.texture,TEXTURE_FILTER_ANISOTROPIC_16X);
	
//	https://www.cnblogs.com/wuyuan2011woaini/p/15655883.html
//	SetTextureFilter(mesh.texture,TEXTURE_FILTER_TRILINEAR);			// 单独一行，没有mipmap就不是缩小变暗
//	SetTextureFilter(mesh.texture,TEXTURE_FILTER_POINT);				// 无滤波
//	SetTextureFilter(mesh.texture, TEXTURE_FILTER_BILINEAR);			// 线性滤波
	SetTextureFilter(mesh.texture, TEXTURE_FILTER_POINT);				// 无滤波
	
//	如果渲染器超出原始边界范围，则会复制边缘颜色对超出范围的区域进行着色
//	SetTextureWrap(mesh.texture,TEXTURE_WRAP_CLAMP);					// 这个相对于出界拉伸
//	SetTextureWrap(mesh.texture,TEXTURE_WRAP_REPEAT);					// 这个是默认的，也就是没有这一行命令也和之前一样
//	SetTextureWrap(mesh.texture,TEXTURE_WRAP_MIRROR_REPEAT);				// 这个是镜像出界
//	SetTextureWrap(mesh.texture,TEXTURE_WRAP_MIRROR_CLAMP);					// 这个是镜像出界，边线拉伸
	SetTextureWrap(mesh.texture, TEXTURE_WRAP_REPEAT);
	
//	SetTextureFilter(Texture2D texture, int 过滤器);                         //设置Texture缩放过滤器模式
	
	
	
	
	
//	设置帧率
//	SetTargetFPS(160);									// 低帧率，四角的矩形会平滑移动效果
//	100*30全面刷新帧率60+
//	100*15全面刷新帧率
	SetTargetFPS(70);
//		设置默认绘制到mesh
	BeginTextureMode(mesh);
//	******
//		取消绘制的GPU画布
	
	EndTextureMode();
//		设置默认绘制到桌面
	BeginDrawing();
//		黑色覆盖全部屏幕
	ClearBackground(BLACK);
	DrawTexturePro(mesh.texture, {0, 0, 750, 750}, {0, 0, 750, 750}, {0, 0}, 0, WHITE);
//		结束绘制的桌面
	EndDrawing();
	
	// 绘制，粘贴自8.测试DrawTexture 与11.备份 10测试
	int drawx;
	int drawy;
	int mousex;
	int mousey;
	int drawi;
	int drawj;
	mousex = 0;
	mousey = 0;
	drawx = 0;
	drawy = 0;
	drawi = 0;
	drawj = 0;
	
	
//	拖拽
	int draftflag;
	int draftx;
	int drafty;
	int gamex;
	int gamey;
//	记录长按时，鼠标按下去的位置，减出长按拖拽距离
	int oldx;
	int oldy;
	draftflag = 0;
	draftx = 0;
	drafty = 0;
//	draftx=bkmeshwidth*pixnum/2;
//	drafty=bkmeshheight*pixnum/2;
	gamex = 0;
	gamey = 0;
	
//	滚轮放大缩小
	float camerasize;
	camerasize = 1.0f;
	
	int zoom;
	int time;
	zoom = 0;
	//记录滚轮次数，滚动快，放缩快
	int cnt = 0;
	
	
	// 测试碰撞目标
	int mousewidth = 10;
//		int mouseheight = 0;
	int mouseheight = 3;
	int targetheight = 45;
	int targetwidth = 25;
	int targeti = mapi - 10;
//	int targetj = 10;
	int targetj = 20;
	
	
	
	int ax;								// 加速度，解决松开就停而产生的变量，延长运动时期，和按键时期不同
	int ay;
//	ax=0;
//	ay=0;
	ax = 0;
	ay = 0;
	int speedx;										// 玩家速度
	int speedy;
	speedx = 0;
	speedy = 0;
	
	
	int playeri;									// 玩家位置
	int playerj;
	
	
	int playerx;
	int playery;
	
	playery = mapi * pixnum / 2;
	playerx = mapj * pixnum / 2;
	
//	playeri = mapi - 1;								// 左上角
//	playerj = 0;
	playeri = mapi / 2;
	playerj = mapj / 2;
	
	int oldplayeri;									// 恢复原状
	int oldplayerj;
	
	oldplayeri = playeri;
	oldplayerj = playerj;
	
	// 限制滚轮滚动一次，实际多少循环 *0.99的次数
	int cntv2;										// 原来的一个cnt被摄像机使用了
	int timev2;										// time 也被摄像机滚轮放大zoom限制使用了
	cntv2 = 0;
	timev2 = 0;
	
	
	float k = 0.0f;
	
	
//	int timev3;
//	timev3 = 0;
	
//	int atking;											// 记录是否直线攻击
//	atking = 0;											// 不攻击记录为0
	
	
	// 第一个多线程测试
	// 复制粘贴自 2.基于42二维数组
	
	RenderTexture meshv2 = LoadRenderTexture(500, 500);
	BeginTextureMode(meshv2);
	ClearBackground(WHITE);
	EndTextureMode();
	
	message msg;
	msg.a = 0;
	msg.flag = 0;
	msg.user = -1;
	
	int*mapth = new int[500 * 500];
	for (int i = 0; i < 500 * 500; i++) {
		mapth[i] = 0;
	}
	
	int* oldmapth = new int[500 * 500];
	for (int i = 0; i < 500 * 500; i++) {
		oldmapth[i] = 0;
	}
	
	int** mapv3th = new int*[500];
	for (int i = 0; i < 500; i++) {
		mapv3th[i] = new int[500];
	}
	// 一维数组测试成功
	msg.work = (void*)&mapth;
	// 使用void 转换指针也成功
	msg.map = mapth;
	targetv2* enemyth = new targetv2();
	
	enemyth->map = mapth;
	// 二维数据传入后，重新分配指针列表并重新写入，才能实现绘制
	enemyth->mapv3 = mapv3th;
	// 测试结构体下的二维数组转换时信息不丢失
	msg.workv2 = (void*)enemyth;
	
	msg.mapv2 = mapv3th;
	printf("address map[][] %p\n", mapv3th);
	printf("address map[499][] %p\n", mapv3th[499]);
	printf("address map[499][499] %p\n", mapv3th[499][499]);
	printf("address map[499][498] %p\n", mapv3th[499][498]);
	
	// 开始线程
	pthread_t p;
	pthread_create(&p, NULL, showv2, (void*)&msg);
	
	
	
	
	
	message *m = new message[enemysum];
	info* havev3 = new info[enemysum];
	
//	floodFill4(enemy[i].area[0].targetj + 15, enemy[i].area[0].targeti + 15, 777, 254, enemy[i].mapv3, enemy[i].targetheight * pixnum, enemy[i].targetwidth * pixnum);
	
	for (int n = 0; n < enemysum; n++) {
		// 计算网格大小算像素大小
		enemy[n].pixnum = 30;
		havev3[n].x = enemy[n].area[0].targetj + 15;
		havev3[n].y = enemy[n].area[0].targeti + 15;
		havev3[n].newColor = 777;
		havev3[n].oldColor = 254;
		havev3[n].enemy = &enemy[n];
		m[n].workv2 = (void*)&havev3[n];
		m[n].a = 0;
		m[n].user = -1;
		m[n].flag = 1;
	}
	// 开始洪水填充多线程
	pthread_t* pk = new pthread_t[enemysum];
	for (int n = 0; n < enemysum; n++) {
		printf("正在创建线程\n");
		pthread_create(&pk[n], NULL, floodfill4v2, (void*)&m[n]);
	}
	
	// 每几次循环刷新一下敌人涂色
	int cntv3 = 0;
	int n = 0;						// 当前敌人刷新
	int ***mapbuff = new int**[enemysum];							// 刷新数组检测
	for (int n = 0; n < enemysum; n++) {
//		mapbuff[n] = new int*[enemy[n].targetheight * pixnum];
		// 参考 mapv3 发现数据上限，有敌人刷新死亡重生，长宽变化越界
		mapbuff[n] = new int*[1000];
	}
	for (int n = 0; n < enemysum; n++) {
//		for (int i = 0; i < enemy[n].targetheight * pixnum; i++) {
		for (int i = 0; i < 1000; i++) {
//			mapbuff[n][i] = new int[enemy[n].targetwidth * pixnum];
			mapbuff[n][i] = new int[1000];
		}
	}
	
	for (int n = 0; n < enemysum; n++) {
		for (int i = 0; i < enemy[n].targetheight * pixnum; i++) {
			for (int x = 0; x < enemy[n].targetwidth * pixnum; x++) {
				mapbuff[n][i][x] = enemy[n].mapv3[i][x];
				if (x == 100 && i == 100) {
					printf("%d\n", mapbuff[n][i][x]);
				}
				
			}
		}
	}
	
	
	
	
	
//	这里开始主循环
	while (!WindowShouldClose()) {
		
		// 线程锁检测
		while (msg.a == 1) {
//			printf("C 发现占用中%d\n", msg.user);
			_sleep(1);
		}
		msg.a = 1;
		msg.user = 999;
//		printf("C 占用中\n");
		BeginTextureMode(meshv2);
		
		for (int i = 0; i < 500 * 500; i++) {
//			if (oldmap[i] != map[i]) {
//			if (oldmap[i] == map[i]) {
//
//			} else {
//				DrawPixel(i % 500, i / 500, {map[i] * 10 % 255, map[i] * 50 % 255, map[i] * 30 % 255, 255});
//			}
//			DrawPixel(i % 500, i / 500, {mapv3[i/499][i%499] * 10 % 255, mapv3[i/499][i%499] * 50 % 255, mapv3[i/499][i%499] * 30 % 255, 255});
			DrawPixel(i % 499, i / 500, {mapv3th[i / 500][i % 499] * 10 % 255, mapv3th[i / 500][i % 499] * 50 % 255, mapv3th[i / 500][i % 499] * 30 % 255, 255});
			
		}
		EndTextureMode();
		msg.a = 0;
//		printf("C 释放\n");
		
		
		// 屏闪，因为两次begin
//		BeginDrawing();
		
		// 移植到一begin
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, WHITE);
////		DrawTexture(meshv2.texture, 0, 0, WHITE);
//
//		DrawText(TextFormat("fps %d", GetFPS()), 20, 30, 50, {155, 50, 210, 255});
//		EndDrawing();
		
		
		static int flagv2 = 0;
		if (flagv2 > 30) {
			flagv2 = 300;
			msg.flag = 1;
		} else {
			flagv2++;
		}
		
		
//		for (int n = 0; n < enemysum; n++) {
//			if (m[n].a == 0) {
		
//		cntv3++;
//		if (cntv3 > 30) {
//			cntv3 = 0;
//			n++;
//			if (n >= enemysum) {
//				n = 0;
//			}
//
//		}
		
//		while (m[n].a == 1) {
//			_sleep(1);
//		}
		
		// 闪退卡死，发现死锁了
		
		
		// 刷新敌人贴图
//		if (m[n].a == 0) {
//			m[n].a = 1;
//			m[n].user = 999;
//			printf("主函数正在使用数组绘制\n");
////			BeginTextureMode(enemy[n].picture);
//			for (int y = 0; y < enemy[n].targetheight * pixnum; y++) {
//				for (int x = 0; x < enemy[n].targetwidth * pixnum; x++) {
////				DrawPixel(x, y, {x % 255, y % 255, x * y % 255, 255});
//					// 数组增加后，是从下往上绘制，从下往上打印，数组对应从0到999
////				DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
//					// 重新测试边界,查坐标系，看数组和像素是否对应
//					// 加入缓冲，只绘制不同的地方像素
//					if (mapbuff[n][y][x] != enemy[n].mapv3[y][x]) {
//						printf("发现绘制不同点 %d %d %d\n",n,x,y);
//						mapbuff[n][y][x] = enemy[n].mapv3[y][x];
////						if (enemy[n].mapv3[y][x] == 777) {
////							DrawPixel(x, y, {125, 25, 175, 255});				// 8-29-2024 洪水填充
////						} else if (enemy[n].mapv3[y][x] == 1) {	// 8-26-2024 增加寻路打印数据
////							DrawPixel(x, y, {0, 255, 0, 255});
////						} else if (enemy[n].mapv3[y][x] == 254) {
////							DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
////						} else if (y == 0) {
////							DrawPixel(x, y, {0, 255, 255, 255});
////						} else {
////							DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
////						}
//					}
//				}
//			}
////			EndTextureMode();
////		_sleep(30);
//			//结束占用，写入控制锁中
//			m[n].a = 0;
//		}
		
//			}
//		}
		
		
		
		
		// 新版本v2多线程刷新贴图
//		printf("主函数执行\n");
		cntv3++;
		// 30 一次
//		if (cntv3 > 30) {
//		if (cntv3 > 3) {				// 60+帧率 配一个敌人图刷新
//		if (cntv3 > 0) {
		if (cntv3 > 0) {				// 60+ 配一个敌人图，41帧配10个敌人图
			cntv3 = 0;
			
			// 测试十个刷新，提速
//			for (int k = 0; k < 10; k++) {
//			for (int k = 0; k < 5; k++) {			// 53帧 0配 5个敌人图
//			for (int k = 0; k < 1; k++) {			// 53帧 0配 5个敌人图
//			for (int k = 0; k < 5; k++) {			// 30 - 53帧 0配 5个敌人图 30休眠
//			for (int k = 0; k < 5; k++) {			// 30 - 53帧 0配 5个敌人图 30休眠
//			for (int k = 0; k < 5; k++) {			// 30 - 53帧 0配 5个敌人图 30休眠 禁用主函数printf 注释后 60帧
//			for (int k = 0; k < 35; k++) {			// 0配 5个敌人图 30休眠 禁用主函数printf 注释后 35帧
//			for (int k = 0; k < 25; k++) {			// 0配 5个敌人图 30休眠 禁用主函数printf 注释后 40帧
//			for (int k = 0; k < 15; k++) {			// 0配 5个敌人图 30休眠 禁用主函数printf 注释后 50帧
			
			for (int k = 0; k < 10; k++) {			// 0配 5个敌人图 30休眠 禁用主函数printf 注释后 55帧
				
				
				n++;
				if (n >= enemysum) {
					n = 0;
				}
				if (m[n].a == 0) {
					m[n].a = 1;
					m[n].user = 999;
//					printf("主函数正在使用数组绘制\n");
					BeginTextureMode(enemy[n].picture);
					for (int y = 0; y < enemy[n].targetheight * pixnum; y++) {
						for (int x = 0; x < enemy[n].targetwidth * pixnum; x++) {
//				DrawPixel(x, y, {x % 255, y % 255, x * y % 255, 255});
							// 数组增加后，是从下往上绘制，从下往上打印，数组对应从0到999
//				DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
							// 重新测试边界,查坐标系，看数组和像素是否对应
							
							
							// 加入缓冲，只绘制不同的地方像素
							if (mapbuff[n][y][x] != enemy[n].mapv3[y][x]) {
//								printf("发现绘制不同点 %d %d %d\n", n, x, y);
								mapbuff[n][y][x] = enemy[n].mapv3[y][x];
								
								
								if (enemy[n].mapv3[y][x] == 777) {
									DrawPixel(x, y, {125, 25, 175, 255});				// 8-29-2024 洪水填充
								} else if (enemy[n].mapv3[y][x] == 1) {	// 8-26-2024 增加寻路打印数据
									DrawPixel(x, y, {0, 255, 0, 255});
								} else if (enemy[n].mapv3[y][x] == 254) {
									DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
								} else if (y == 0) {
									DrawPixel(x, y, {0, 255, 255, 255});
								} else {
									DrawPixel(x, y, {enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], enemy[n].mapv3[y][x], 255});
								}
								
								
							}
							
							
						}
					}
//					printf("%d\n", mapbuff[n][99][99]);
//					printf("%d\n", enemy[n].mapv3[99][99]);
					
//				EndTextureMode();
					//结束占用，写入控制锁中
					m[n].a = 0;
				}
				
				
			}
			
		}
		
		
		
		
		
//		按键后增加速度
		if (IsKeyDown(KEY_A)) {
			ax += -1;
		}
		if (IsKeyDown(KEY_D)) {
			ax += 1;
		}
		if (IsKeyDown(KEY_W)) {
			ay += 1;
		}
		if (IsKeyDown(KEY_S)) {
			ay += -1;
		}
		
		
		// 键盘松开，加速度消失
		// 四个 if 合并成两个
		if (IsKeyUp(KEY_A) && IsKeyUp(KEY_D)) {
			ax = 0;
			speedx = 0;
		}
		if (IsKeyUp(KEY_W) && IsKeyUp(KEY_S)) {
			ay = 0;
			speedy = 0;
		}
		
		// 加速度限制
		if (ax > 1) {
			ax = 1;
		} else if (ax < -1) {
			ax = -1;
		}
		if (ay > 1) {
			ay = 1;
		} else if (ay < -1) {
			ay = -1;
		}
		speedy += ay;
		speedx += ax;
		
		// 速度限制
		if (speedx > 1) {
			speedx = 1;
		} else if (speedx < -1) {
			speedx = -1;
		}
		
		if (speedy > 1) {
			speedy = 1;
		} else if (speedy < -1) {
			speedy = -1;
		}
		// playerj+speedx 解决复制粘贴 改名不全的导致的 playerj -speedx
		// 但是只修改网格，不是像素绘制上去，瓦片贴图是一直刷新的，
		
		// 禁用红色矩形移动
		// 不禁用，检测贴图移动
		if (0 < playeri + speedy && playeri + speedy < mapi && 0 < playerj + speedx && playerj + speedx < mapj) {
			playeri += speedy;											// 因为往下是减少，速度增加，速度往下是正的，变成负的是每帧的距离
			playerj += speedx;
			
//			map[oldplayeri][oldplayerj] = 224;
			mapv2[oldplayeri][oldplayerj] = 224;
			
			map[playeri][playerj] = 2024;
			oldplayeri = playeri;
			oldplayerj = playerj;
			
		}
		
		// 红色像素移动
		if (0 < playery + speedy && playery + speedy < mapi * 30 && 0 < playerx + speedx && playerx + speedx < mapj * 30) {
			playerx = playerx + speedx;
			playery = playery + speedy;
//			BeginTextureMode(mesh);
//			DrawPixel(playerx,playery,{255,0,0,255});
//			EndTextureMode();
		}
		
		// 如果玩家在区域里
		if (region != NULL) {
			if (playerx >= region->targetj * pixnum &&
				playerx < (region->targetj + region->targetwidth)*pixnum &&
				// 注意坐标系y向下数值会减小，所以是小于，等于是测试得到的，原来的BUG是移动一格任何出现轨迹
				// +1是发现高度差一行
				// 原来是+1到了上一格子。target是网格坐标，mapi是最顶上，所以+1是往上平移一格，因为绘制是GPU左下角绘制，所以是网格的左下角，所以需要更上一格的左下角。
				// 是小于上一格子的左下角，大于等于最底下一格子的左下角
				playery < (region->targeti + 1) * pixnum &&
				// 没有发现漏打region->targethieght
				playery >= (region->targeti + 1 - region->targetheight)*pixnum) {
				regionflag = 1;
				
			} else {
				int n = 0;
				for (n = 0; n < enemysum; n++) {
					if (playerx >= enemy[n].targetj * pixnum &&
						playerx < (enemy[n].targetj + enemy[n].targetwidth)*pixnum &&
						playery < (enemy[n].targeti + 1) * pixnum &&
						playery >= (enemy[n].targeti + 1 - enemy[n].targetheight)*pixnum) {
						region = &enemy[n];
						break;
					}
				}
				// 全部看一遍发现没有重叠的
				if (n == enemysum) {
					regionflag = 0;
				}
			}
		}
		
		// 把之前绘制像素碰撞检测分离到数据处理部分
		if (regionflag == 1) {
			// 测试撞墙
			int flag = 0;
			
			// 这样是横着撞墙可以上下移动
			if (region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
				playerx -= speedx;
//				playery -= speedy;
			}
			
			// 检测玩家是否在敌人内部，在数组出界追加之后再追加
			if (region != NULL) {
				if (playerx >= region->targetj * pixnum &&
					playerx < (region->targetj + region->targetwidth)*pixnum &&
					// 注意坐标系y向下数值会减小，所以是小于，等于是测试得到的，原来的BUG是移动一格任何出现轨迹
					// +1是发现高度差一行
					// 原来是+1到了上一格子。target是网格坐标，mapi是最顶上，所以+1是往上平移一格，因为绘制是GPU左下角绘制，所以是网格的左下角，所以需要更上一格的左下角。
					// 是小于上一格子的左下角，大于等于最底下一格子的左下角
					playery < (region->targeti + 1) * pixnum &&
					// 没有发现漏打region->targethieght
					playery >= (region->targeti + 1 - region->targetheight)*pixnum) {
					regionflag = 1;
					
				} else {
					int n = 0;
					for (n = 0; n < enemysum; n++) {
						if (playerx >= enemy[n].targetj * pixnum &&
							playerx < (enemy[n].targetj + enemy[n].targetwidth)*pixnum &&
							playery < (enemy[n].targeti + 1) * pixnum &&
							playery >= (enemy[n].targeti + 1 - enemy[n].targetheight)*pixnum) {
							region = &enemy[n];
							break;
						}
					}
					// 全部看一遍发现没有重叠的
					if (n == enemysum) {
						regionflag = 0;
					}
				}
			}
			// 发现顶端闪退，追加判断，但是没有解决问题，只是横着撞上边界可以解决竖着贴墙移动问题
//			if (regionflag==1&&region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
//			if (region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
			// 改完下面的，然后再改回来测试，左边撞墙没法竖着移动BUG
			// 发现确实解决了，
			// 但是竖着穿墙有BUG，偶尔会竖着卡进墙里
			// 但是游戏效果增加了，有就比没有强
			if (regionflag == 1 && region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
//				playerx -= speedx;
				playery -= speedy;
				flag = 1;
			}
			
			// 发现闪退，知道新增一小段代码就闪退，可知问题在代码上，重读，发现数组出界，于是检测
			// 检测玩家是否在敌人内部
			if (flag == 1) {
				
				playerx += speedx;
				if (region != NULL) {
					if (playerx >= region->targetj * pixnum &&
						playerx < (region->targetj + region->targetwidth)*pixnum &&
						// 注意坐标系y向下数值会减小，所以是小于，等于是测试得到的，原来的BUG是移动一格任何出现轨迹
						// +1是发现高度差一行
						// 原来是+1到了上一格子。target是网格坐标，mapi是最顶上，所以+1是往上平移一格，因为绘制是GPU左下角绘制，所以是网格的左下角，所以需要更上一格的左下角。
						// 是小于上一格子的左下角，大于等于最底下一格子的左下角
						playery < (region->targeti + 1) * pixnum &&
						// 没有发现漏打region->targethieght
						playery >= (region->targeti + 1 - region->targetheight)*pixnum) {
						regionflag = 1;
						
					} else {
						int n = 0;
						for (n = 0; n < enemysum; n++) {
							if (playerx >= enemy[n].targetj * pixnum &&
								playerx < (enemy[n].targetj + enemy[n].targetwidth)*pixnum &&
								playery < (enemy[n].targeti + 1) * pixnum &&
								playery >= (enemy[n].targeti + 1 - enemy[n].targetheight)*pixnum) {
								region = &enemy[n];
								break;
							}
						}
						// 全部看一遍发现没有重叠的
						if (n == enemysum) {
							regionflag = 0;
						}
					}
				}
				
//				if (region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
				// 发现可以解决竖着撞墙横着移动
				if (regionflag == 1 && region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
					playerx -= speedx;
//				playery -= speedy;
				}
			}
			
		}
		
		
		
		// 解决缩小放大绘制时出现的绘制情况，原因是旧位置放大倍数给了新数据，现在是新放大倍数给当前数据
//		版本7，继续版本5的调参数
		if (GetMouseWheelMove() < 0) {
			zoom = -1;
			cnt++;
		} else if (GetMouseWheelMove() > 0) {
			zoom = 1;
			cnt--;
		}
//		30改10，配合0.99改0.95，增加灵敏度
//		10改5，配合0.95改0.97，增加减少放大差距，提高鼠标滚动延长效果
//		5改5，配合0.97改0.98，增加减少放大差距，提高鼠标滚动延长效果
//		5改5，配合0.98改0.981，复位1.00001，解决无法复位，回不到1问题
//		5改5，配合0.981改0.9811，复位1.00000，解决无法复位，回不到1问题
//		5改5，配合0.981改0.99，复位1.00000，解决到0.01再返回无法复位，回不到1.0问题
		// 0.99改成0.98在4.版本后
		
		if (cnt != 0 && time < 5) {
//			改30参数，影响放缩时间
			time++;
			if (zoom > 0) {
//				加法改乘法，放大感觉平均，没有了越小，再缩小一次，缩小明显更小的追加情况，原因是数值小了，追加固定数据，倍数影响大
				camerasize = camerasize * 0.97;
//				改0.99缩小参数，影响每次循环缩小倍数，一次缩小的突然更小程度
				if (camerasize < 0.01) {
					camerasize = 0.01;
				}
			} else if (zoom < 0) {
				camerasize = camerasize / 0.97;
				if (camerasize > 40) {
					camerasize = 40;
				}
			}
//			解决复位之后，重复写0问题
		} else if (time != 0) {
			time = 0;
//			zoom = 0;
//			多次滚轮，则延长摄像机缩放
			if (cnt < 0) {
				cnt++;
			}
			if (cnt > 0) {
				cnt--;
			}
//			消耗完累计次数，停止滚动
			if (cnt == 0) {
				zoom = 0;
			}
		}
		
//		注意是pressed 不是 Down
		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			draftflag = 1;
			oldx = GetMouseX();
			oldy = GetMouseY();
		}
		if (draftflag == 1) {
			mousex = GetMouseX();
			mousey = GetMouseY();
			draftx = gamex - (mousex - oldx) / camerasize;
			drafty = gamey - (mousey - oldy) / camerasize;
		}
		if (IsMouseButtonUp(MOUSE_BUTTON_RIGHT)) {
			draftflag = 0;
			oldx = 0;
			oldy = 0;
			gamex = draftx;
			gamey = drafty;
		}
		
		
		
		// 完成角色切换控制，缩小倍数锁定玩家，不能拖拽
		
		// 记录旧数据,缩小还原
		static int olddraftx;
		static int olddrafty;
		static int recoverflag = 0;							// 摄像机回到原来位置的控制信号，原来是靠camerasize if 不够用
		static int followflag = 0;							// 追随
//		olddraftx = draftx;
//		olddrafty = drafty;
		// 在确定网格左上角坐标之前覆盖玩家追随数据
		// 追加追随，在放大倍数一定后，数据覆盖屏蔽旧拖拽
		if (camerasize > 1) {
//			draftx = playerx + bkmeshwidth / 2 - showjv2 * pixnumv2 / camerasize;
//			drafty = playery + bkmeshheight / 2 - showiv2 * pixnumv2 / camerasize;
//			gamex=playerx;
//			gamey=playery;
			
//			DrawTexturePro(mesh.texture, {(draftx - limitleft ) + bkmeshwidth / 2 - showjv2*pixnumv2 / 2 / camerasize,
//			(drafty - limittop + bkmeshheight / 2) - showiv2*pixnumv2 / 2 / camerasize,
//			showjv2*pixnumv2 / camerasize, showiv2*pixnumv2 / camerasize},
//		{0, 0, showjv2 * pixnumv2, showiv2 * pixnumv2}, {0, 0}, 0, WHITE);
			
//		 采样中心 是玩家	draftx + bkmeshwidth / 2 - showjv2*pixnumv2 / 2 / camerasize = playerx
			
			// 根据旧的记录可知，数据draftx 是一开始的左上角区域坐标，draftx+bkmeshwidth/2是中心 ，这是延续当时开发先产生的结论,本质是一开始draftx 是0 ，但是playerx是mapi*pixnum/2
//			draftx = playerx - bkmeshwidth / 2;
//			drafty =( mapi*pixnum-playery) - bkmeshheight / 2;
			
			if (followflag == 0) {
				olddraftx = draftx;
				olddrafty = drafty;
				followflag = 1;
			}
			
			// 根据旧的记录可知，数据draftx 是一开始的左上角区域坐标，draftx+bkmeshwidth/2是中心 ，这是延续当时开发先产生的结论,本质是一开始draftx 是0 ，但是playerx是mapi*pixnum/2
			draftx = playerx - bkmeshwidth / 2;
			drafty = ( mapi * pixnum - playery) - bkmeshheight / 2;
			
			followflag = 1;
			recoverflag = 1;							// 记录覆盖数据已更新
		} else {
			followflag = 0;
		}
		
		// 第一次切换
		if (followflag == 0 && recoverflag == 1) {
			draftx = olddraftx;
			drafty = olddrafty;
			recoverflag = 0;
		}
		
		
		
		
		if (side != 0) {
			// 新增边界检测
			if (draftx < 0) {
				draftx = 0;
			} else if (draftx > maxgamex ) {										// 配合limitleft+60 可以看见边界白边
				draftx = maxgamex;
			}
//打表发现左上角出界刷新，实际比较矩形限制在很小的区域 60*60，原来的表示则是750*750大小的区域
//		0<0不会执行，不能等于，否则出界
//		if 又改wihle 解决拖拽时出现绘制点，原因是旧坐标没来得及更新
//		+1解决side=0时去除采样复位导致的抖动，避免死循环
//		但是导致偏移，鼠标尖点击不准确，于是回滚代码去除+1
			while (draftx < limitleft ) {
//			limitleft -= side * pixnum+1;											// side*pixnum 替换 pixnum 但是 limitleft 不变化
				limitleft -= side * pixnum;											// side*pixnum 替换 pixnum 但是 limitleft 不变化
			}
			while (draftx > limitleft + 2 * side * pixnum) {						//+60 改 +30 又改 +60 左上角移动两个边距就是2*30
//			limitleft += side * pixnum+1;
				limitleft += side * pixnum;
			}
//		新增边界检测
			if (drafty < 0) {
				drafty = 0;
			} else if (drafty > maxgamey) {
				drafty = maxgamey;
			}
			
			while (drafty < limittop ) {
//			limittop -= side * pixnum+1;
				limittop -= side * pixnum;
			}
			while (drafty > limittop + 2 * side * pixnum) {							// +60 改 +30 又改 +60，见到底部白边
//			limittop += side * pixnum+1;
				limittop += side * pixnum;
			}
			
			bkmeshmapj = limitleft / pixnum;												// 关键在只要保证draftx<=limitleft + 2 * side * pixnum,边界就不会出来刚好靠边，数组打印
//		bkmeshmapi = mapi - 1 -  bkmeshmapmaxi - limittop / pixnum;							// 269 -27 -0 + 27 -i = 269 -i =269 -0 =269 = 数组最后一个
			bkmeshmapi = mapi  -  bkmeshmapmaxi - limittop / pixnum;						// 270 -27 -0 + 27 -i = 270 -i =270 -1 =269 = 数组最后一个
		}
		
//		// 追加追随，在放大倍数一定后，数据覆盖屏蔽旧拖拽
//		if (camerasize > 0.1) {
//			draftx = playerx + bkmeshwidth / 2 - showjv2 * pixnumv2 / camerasize;
//			drafty = playery + bkmeshheight / 2 - showiv2 * pixnumv2 / camerasize;
////			gamex=playerx;
////			gamey=playery;
//
////			DrawTexturePro(mesh.texture, {(draftx - limitleft ) + bkmeshwidth / 2 - showjv2*pixnumv2 / 2 / camerasize,
////			(drafty - limittop + bkmeshheight / 2) - showiv2*pixnumv2 / 2 / camerasize,
////			showjv2*pixnumv2 / camerasize, showiv2*pixnumv2 / camerasize},
////		{0, 0, showjv2 * pixnumv2, showiv2 * pixnumv2}, {0, 0}, 0, WHITE);
//
//	}
		
		
//		改变顺序，计算完偏移再算绘制，解决用上一次偏移坐标组合这一处偏移绘制
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
//			drawx = GetMousePosition().x;
//			无中生有坐标系变化过程
//			750,实际穷举描述得到，在点一下，在鼠标坐标系是0，100，靠顶边，
//			在GPU坐标系里0，100则是靠底边100像素，
//			然后实际上是直接取反然后加上高度上限，发现原来可以
//			最后重整旧测试代码实现
//			drawy = 750 - GetMousePosition().y;
//			drawy = 751 - GetMousePosition().y;
//			if (drawx < 0 || drawx > 750 || drawy < 0 || drawy > 750)
//			发现可以直接写，标注数据变化，于是重新命名变量
			mousex = GetMousePosition().x;
			mousey = GetMousePosition().y;
//			测试之后，追加等号，发现等号设置在0处解决问题，750-0=750，750/30=25，数组出界，750-1=749，749/30=24，
//			可知减的多，不出界，剩下的少，于是就不出界
//			if (mousex < 0 || mousex > showiv2 * pixnum || mousey <= 0 || mousey > showiv2 * pixnum) {
//			side=0 时进行检测，节约性能
			if (mousex < 0 || mousex > showjv2 * pixnumv2 || mousey <= 0 || mousey > showiv2 * pixnumv2) {
				
			} else {
//				int positionx = 0 + draftx/ camerasize;
//				int positionx = 0 + draftx + showjv2 * pixnum / 2 - showjv2 * pixnum / 2 / camerasize;
//				解决采样中心是mesh中心后，绘制偏移也修改
//				int positionx = 0 + draftx + bkmeshwidth / 2 - showjv2 * pixnum / 2 / camerasize;
				int positionx = 0 + draftx + bkmeshwidth / 2 - showjv2 * pixnumv2 / 2 / camerasize;
//				int positiony = 0 + drafty / camerasize;
//				int positiony = 0 + drafty + showiv2 * pixnum / 2 - showiv2 * pixnum / 2 / camerasize;
//				int positiony = 0 + drafty + bkmeshheight / 2 - showiv2 * pixnum / 2 / camerasize;
				int positiony = 0 + drafty + bkmeshheight / 2 - showiv2 * pixnumv2 / 2 / camerasize;
//				相对于左下角位置，水平方向就是当前左下角距离加鼠标横着距离左边界
				drawx = mousex / camerasize + positionx;			// 乘camerasize不行，测试除法，发现也不行,换测试位置
//				回滚，测试发现x 除法可以在水平移动多少，对应绘制多少
//				竖直方向，可以理解成先算距离左下角距离，然后再加上左下角实际对应的地图点
//				缩小后，750变化，可以理解成坐标上限增加，原来坐标系变化需要750-，现在变成1500- 对应两倍坐标系变化
				drawy =  mousey / camerasize + positiony;		// 750/camerasize
//				测试750/camerasize,发现偏移随放大倍数增加，改成乘camerasize 发现刚好适配
				drawj = drawx / pixnum;
				drawi = (mapi * pixnum - drawy) / pixnum;
//				出界闪退BUG解决方案，drawj没有问题，受到边界限制 maxgamex
				// 但是由于side =0 时会禁用之前的拖拽检测，于是丢失了上面的限制，所以还是得增加 j x 方向的越界检测
				if (drawi >= mapi || drawi < 0 || drawj < 0 || drawj >= mapj) {
					
				} else {
//					for (int j = 0; j < 10; j++) {
//						map[drawi][drawj + j] = 1949;
//					}
					
					for (int i = 0; i < mouseheight; i++) {
						for (int j = 0; j < mousewidth; j++) {
							if (drawi - i > 0 && drawi - i < mapi) {
								if (drawj + j > 0 && drawj + j < mapj) {
									map[drawi - i][drawj + j] = 1949;
								} else {
									break;
								}
							} else {
								break;
							}
							
						}
					}
					
//					atking = 1;
					// 找到一个没有运行的炮弹并启动
					for (int i = 0; i < boomsum; i++) {
						// 发现没有运行
						if (booms[i].atking == 0) {
//							启动运行
							booms[i].atking = 1;
							// 开始运行时间重置为0
							booms[i].timev3 = 0;
							// 记录位置，可描述为起点，实际是记录所在直线
							booms[i].playerjv2 = playerj;
							booms[i].playeriv2 = playeri;
//							终点
							booms[i].drawiv2 = drawi;
							booms[i].drawjv2 = drawj;
							// 记录覆盖点，重置为0，避免旧数据覆盖到其他地方
							booms[i].oldx = 0;
							booms[i].oldy = 0;
							// 炮弹数目
							have--;
							break;
						}
					}
					
				}
			}
		}
		
		// 恢复旧贴图数据
		for (int n = 0; n < enemysum; n++) {
			for (int i = 0; i < enemy[n].targetheight; i++) {
				for (int j = 0; j < enemy[n].targetwidth; j++) {
//					map[enemy[n].targeti - i][enemy[n].targetj + j] = enemy[n].oldmap[i][j];
					// 追加边界检测，解决闪退问题
					if (enemy[n].targeti - i >= 0 && enemy[n].targeti - i < mapi && enemy[n].targetj + j >= 0 && enemy[n].targetj + j < mapj) {
						map[enemy[n].targeti - i][enemy[n].targetj + j] = enemy[n].oldmap[i][j];
					}
				}
			}
		}
		
//		lineatk(booms, boomsum, map);
//		lineatk(booms, boomsum, mapv2);
		lineatk(booms, boomsum, map);												// 敌人map重新替换mapv2 ,实现局部刷新
		
		
//		for (int n = 0; n < boomsum; n++) {
////			checkboom(booms[n], enemy, map, enemysum, mapi, mapj);
//			checkboom(booms[n], enemy, mapv2, enemysum, mapi, mapj);
//		}
////		checkend(booms, boomsum, &have, map, mapi, mapj);
//		checkend(booms, boomsum, &have, mapv2, mapi, mapj);
		
		
		
		// 敌人伏击检测
		for (int n = 0; n < enemysum; n++) {
			int bangi;
			int bangj;
			int bangwidth;
			int bangheight;
			
			if (bang(playeri, playerj, 10, 10, enemy[n].targeti + 50, enemy[n].targetj - 50, enemy[n].targetwidth + 50 + 50, enemy[n].targetheight + 50 + 50, &bangi, &bangj, &bangwidth, &bangheight)) {
				for (int m = 0; m < boomsumv2; m++) {
					if (boomsv2[m].atking == 0) {
						boomsv2[m].atking = 1;
						boomsv2[m].playerjv2 = enemy[n].targetj;
						boomsv2[m].playeriv2 = enemy[n].targeti;
						boomsv2[m].drawiv2 = playeri;
						boomsv2[m].drawjv2 = playerj;
						boomsv2[m].timev3 = 0;
						havev2--;
						break;
					}
				}
			}
		}
		
		// 敌人的炮弹移动
//		lineatk(boomsv2, 100, map);
//		lineatk(boomsv2, 100, mapv2);
		lineatk(boomsv2, 100, map);
		
		
		// 炮弹命中检测
		for (int n = 0; n < 100; n++) {
			int bangi;
			int bangj;
			int bangwidth;
			int bangheight;
			if (boomsv2[n].atking == 1) {
				if (bang(playeri, playerj, 10, 10, boomsv2[n].oldy - 10 / 2, boomsv2[n].oldx - 10 / 2, 10, 10, &bangi, &bangj, &bangwidth, &bangheight)) {
					boomsv2[n].isboom = 1;
					
				}
			}
			
		}
		
//		checkend(boomsv2, boomsumv2, &havev2, map, mapi, mapj);
//		checkend(boomsv2, boomsumv2, &havev2, mapv2, mapi, mapj);
		checkend(boomsv2, boomsumv2, &havev2, map, mapi, mapj);
		
		
		
		// 敌人移动
		for (int n = 0; n < enemysum; n++) {
			if (enemy[n].islive == 1) {
				if (enemy[n].targetj < playerj) {
					enemy[n].targetj += 1;
				} else if (enemy[n].targetj > playerj) {
					enemy[n].targetj -= 1;
				}
				
				if (enemy[n].targeti > playeri) {
					enemy[n].targeti -= 1;
				} else if (enemy[n].targeti < playeri) {
					enemy[n].targeti += 1;
				}
			}
			
		}
		
		// 敌人绘制，但是帧率低，没有复制粘贴过来的跑到帧率高
//		for (int n = 0; n < 100; n++) {
//			if (enemy[n].islive) {
//				for (int i = 0; i < enemy[n].targetheight; i++) {
//					for (int j = 0; j < enemy[n].targetwidth; j++) {
//						map[enemy[n].targeti - i][enemy[n].targetj + j] = 7788;
//					}
//				}
//			}
//		}
		
		// 重新采样数据，要在炮弹发射之后，这样不会覆盖炮弹轨迹，敌人移动之后重新采样
		for (int n = 0; n < enemysum; n++) {
			for (int i = 0; i < enemy[n].targetheight; i++) {
				for (int j = 0; j < enemy[n].targetwidth; j++) {
//					enemy[n].oldmap[i][j] = map[enemy[n].targeti - i][enemy[n].targetj + j];
					// 追加边界检测，解决闪退问题
					if (enemy[n].targeti - i >= 0 && enemy[n].targeti - i < mapi && enemy[n].targetj + j >= 0 && enemy[n].targetj + j < mapj) {
						enemy[n].oldmap[i][j] = map[enemy[n].targeti - i][enemy[n].targetj + j];
					}
				}
			}
		}
		
		// 绘制敌人
		for (int n = 0; n < enemysum; n++) {
			for (int i = 0; i < enemy[n].targetheight; i++) {
				for (int j = 0; j < enemy[n].targetwidth; j++) {
					// 发现是没有越界检测所以导致100 个时，碰到边界闪退
					if (enemy[n].targeti - i < 0 || enemy[n].targeti - i >= mapi || enemy[n].targetj + j < 0 || enemy[n].targetj + j >= mapj) {
						break;
					} else {
//						map[enemy[n].targeti - i][enemy[n].targetj + j] = 1;
//						mapv2[enemy[n].targeti - i][enemy[n].targetj + j] = 1;
						// 之前覆盖，原来是重新绘制时，有的颜色所在网格序号相同，导致不会重绘，留下贴图
						// 已经使用另一个序号覆盖，和之前的map 序号不同，总是会重新绘制
						map[enemy[n].targeti - i][enemy[n].targetj + j] = 2024;
					}
					
				}
			}
		}
		
		
		// 敌人复活检测
		// 发现每次复活都会有残留贴图，原来是复活后先贴图，玩家再移动，于是没有覆盖第一次贴图，可知复活在贴图之后
//		enemylive = 0;
//		for (int n = 0; n < enemysum; n++) {
//			if (enemy[n].islive == 1) {
//				enemylive++;
//			}
//		}
//
//
//		if (enemylive < 5) {
//			for (int n = 0; n < 5; n++) {
//				if (enemy[n].islive == 0) {
//					enemy[n].islive = 1;
//					enemy[n].targeti = GetRandomValue(0, mapi - 1);
//					enemy[n].targetj = GetRandomValue(0, mapj - 1);
//					enemy[n].targetwidth = GetRandomValue(10, 20);
//					enemy[n].targetheight = GetRandomValue(10, 20);
//					enemylive++;
//
//					// 敌人复活，对应贴图重新设置大小
//					enemy[n].picture.texture.width = enemy[n].targetwidth * pixnum;
//					enemy[n].picture.texture.height = enemy[n].targetheight * pixnum;
//				}
//			}
//		}
		
		
		
		for (int n = 0; n < boomsum; n++) {
			// 只检测正在运行的炮弹
			if (booms[n].atking == 1) {
				checkboom(booms[n], enemy, map, enemysum, mapi, mapj);
			}
			
//			checkboom(booms[n], enemy, mapv2, enemysum, mapi, mapj);
		}
//		checkend(booms, boomsum, &have, map, mapi, mapj);
//		checkend(booms, boomsum, &have, mapv2, mapi, mapj);
		checkend(booms, boomsum, &have, map, mapi, mapj);									// 重新启用map,敌人重绘贴图替换mapv2
		
		
		
		//碰撞加局部显示
		// 两两排列组合-四宫格 左边端点在区间里，左边端点不在区间里，右边端点在区间里，右边端点不在区间里
		static int bangi = 0;
		static int bangj = 0;
		static int bangwidth = 40;
		static int bangheight = 30;
		
		int isbang = bang(drawi, drawj, mousewidth, mouseheight, targeti, targetj, targetwidth, targetheight, &bangi, &bangj, &bangwidth, &bangheight);
		
		if (isbang) {
			for (int i = 0; i > bangheight; i--) {
				for (int j = 0; j < bangwidth; j++) {
					map[bangi + i][bangj + j] = 2024;
				}
			}
		}
		
		// 把玩家绘制到敌人贴图里面
		if (regionflag == 1) {
			BeginTextureMode(region->picture);
			// 高度-距离左下角位置，因为playex是往下时数值增加，减去的多，则越靠近GPU画布的左下角
			// ，+1是一格网格，测试出现的。-1是像素偏移
//			DrawPixel(playerx - region->targetj * pixnum, -1+(region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight+1) * pixnum ), {255, 0, 0, 255});
			
//			DrawPixel(playerx - region->targetj * pixnum, -1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum ), {255, 0, 0, 255});
			// 增加区域碰撞,如果遇到，就恢复原状，模拟像素碰撞
//			if (region->mapv3[-1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum )][playerx - region->targetj * pixnum] == 0) {
//				playerx -= speedx;
//				playery -= speedy;
//			} else {
//				DrawPixel(playerx - region->targetj * pixnum, -1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum ), {255, 0, 0, 255});
//			}
			DrawPixel(playerx - region->targetj * pixnum, -1 + (region->targetheight) * pixnum - ( playery - (region->targeti - region->targetheight + 1) * pixnum ), {255, 0, 0, 255});
			
			
			EndTextureMode();
		}
		
//		设置默认绘制到mesh
		BeginTextureMode(mesh);
		
//		DrawPixel(playerx, playery, {255, 0, 0, 255});
		
//		改成了1，并且最后是等于号因为等于时，对应数组0，0格子，其实是绘制时发现对不上号和更新总是偏下
		for (int i = 1; i <= bkmeshmapmaxi; i++) {
			if (bkmeshmapmaxi - i + bkmeshmapi < 0) {
				break;
			}
			for (int j = 0; j < bkmeshmapmaxj; j++) {
				if (j + bkmeshmapj > mapj) {						// 如果出界就取消绘制
					break;
				}
				// 增加side 是0 为单宫格检测，才进行比较检测，选择刷新小片网格
				if (side == 0 && meshmap[bkmeshmapmaxi  - i ][j] == map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj]) {
					
				} else {
					meshmap[bkmeshmapmaxi  - i ][j] = map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj];
					// 绘制矩形,原点(0,0)在左下角,现在是从左下角一行一行往上绘制
					if (map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] == 9999) {
//						DrawRectangle(j * pixnum, bkmeshmapmaxi * pixnum - i * pixnum - pixnum, pixnum, pixnum, { 255,  255, 255, 255});
						DrawRectangle(j * pixnum, bkmeshmapmaxi * pixnum - i * pixnum, pixnum, pixnum, { 255,  255, 255, 255});
//						DrawRectangle(j * pixnum, bkmeshmapmaxi*pixnum-i * pixnum, pixnum, pixnum, {255, 0, 0, 255});
					} else if (map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] == 2024) {
						DrawRectangle(j * 30, bkmeshheight - i  * 30, pixnum, pixnum, {255, 0, 0, 255});
						
					} else if (map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] == 224) {
						DrawRectangle(j * 30, bkmeshheight - i  * 30, pixnum, pixnum, {125, 125, 0, 255});
						
					} else if (map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] == 1224) {
						DrawRectangle(j * 30, bkmeshheight - i  * 30, pixnum, pixnum, {0, 255, 0, 255});
						
					} else if (map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] == 1949) {
						DrawRectangle(j * pixnum, bkmeshheight  - i * pixnum, pixnum, pixnum, {0, 0, 0, 255});
					} else if (map[bkmeshmapmaxi - i + bkmeshmapi][bkmeshmapj + j] == 7788) {
						DrawRectangle(j * pixnum, bkmeshheight - i * pixnum, pixnum, pixnum, {255, 255, 255, 255});
					} else {
//					60是边界，-30是i=0时，要760打印就没有空间了，730打印30高正方形，刚好是760，同时对应底部的黑边消失了，这就是整体下移动30像素
//						DrawRectangle(j * 30, 750 + 60 - i * 30 - 30, 30, 30, {map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 1 % 255, map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 21 % 255, 255, 255});
						// 替换常数
//						DrawRectangle(j * pixnum, bkmeshmapmaxi * pixnum - i * pixnum -pixnum, pixnum, pixnum, {map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 1 % 255, map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 21 % 255, 255, 255});
						DrawRectangle(j * pixnum, bkmeshmapmaxi * pixnum - i * pixnum, pixnum, pixnum, {map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 1 % 255, map[bkmeshmapmaxi - i + bkmeshmapi][j + bkmeshmapj] * 21 % 255, 255, 255});
					}
//				左下角
					DrawRectangle(0, 0, 150, 150, {154, 154, 154, 255});
//				左上角
					DrawRectangle(0, bkmeshmapmaxi * pixnum - 100,  30, 100, {255, 255, 255, 255});
//				屏幕右上角对应的白色矩形
					DrawRectangle(bkmeshmapmaxi * pixnum - pixnum, bkmeshmapmaxi * pixnum - 100,  30, 100, {255, 255, 255, 255});
//				屏幕右下角对应的红色矩形
					DrawRectangle(bkmeshmapmaxi * pixnum - pixnum, 0,  pixnum, 100, {255, 0, 0, 255});
//				绘制坐标系是左下角0，0）y正方向向上
				}
				
			}
		}
		
// 颜色复位
		// 新增复位在绘制后
//		for (int i = 0; i < mapi; i++) {
//			for (int j = 0; j < mapj; j++) {
//				map[i][j] = mapv2[i][j];
//			}
//		}
		
		// 绘制敌人贴图，不再是网格
		for (int n = 0; n < enemysum; n++) {
			DrawTexture(enemy[n].picture.texture, enemy[n].targetj * pixnum, (enemy[n].targeti - enemy[n].targetheight + 1)*pixnum, WHITE);
		}
		// 绘制像素玩家
//		DrawPixel(playerx, playery, {255, 0, 0, 255});
		if (regionflag == 0) {
			DrawPixel(playerx, playery, {255, 0, 0, 255});
		}
		
		
		
//		取消绘制的GPU画布
		EndTextureMode();
		
		// 敌人复活
		enemylive = 0;
		for (int n = 0; n < enemysum; n++) {
			if (enemy[n].islive == 1) {
				enemylive++;
			}
		}
		
		
		if (enemylive < 5) {
			for (int n = 0; n < 5; n++) {
				if (enemy[n].islive == 0) {
					enemy[n].islive = 1;
					enemy[n].targeti = GetRandomValue(0, mapi - 1);
					enemy[n].targetj = GetRandomValue(0, mapj - 1);
					enemy[n].targetwidth = GetRandomValue(10, 20);
					enemy[n].targetheight = GetRandomValue(10, 20);
					enemylive++;
					
					// 敌人复活，对应贴图重新设置大小
					enemy[n].picture.texture.width = enemy[n].targetwidth * pixnum;
					enemy[n].picture.texture.height = enemy[n].targetheight * pixnum;
					
					
					// 重新采样地图数据
					for (int i = 0; i < enemy[n].targetheight; i++) {
						for (int j = 0; j < enemy[n].targetwidth; j++) {
//							enemy[n].oldmap[i][j] = map[enemy[n].targeti - i][enemy[n].targetj + j];
							// 追加边界检测，解决闪退问题
							if (enemy[n].targeti - i >= 0 && enemy[n].targeti - i < mapi && enemy[n].targetj + j >= 0 && enemy[n].targetj + j < mapj) {
								enemy[n].oldmap[i][j] = map[enemy[n].targeti - i][enemy[n].targetj + j];
							}
						}
					}
					
					
					// 复活，重新设置数组,在追加mapv3,仿照采样地图数据，初始化，打印，复位
					for (int y = 0; y < enemy[n].targetheight; y++) {
						for (int x = 0; x < enemy[n].targetwidth; x++) {
							enemy[n].mapv3[x][y] = (x + y) % 255;
						}
					}
				}
			}
		}
		
		
//		设置默认绘制到桌面
		BeginDrawing();
//		黑色覆盖全部屏幕
		ClearBackground(BLACK);
//		采样坐标系是左上角0，0，y轴正方向向下
//		重新理解：由于在不改这行代码的情况下，改其他地方的代码，跑通了。实现了效果，再次理解数据变化：draftx>60时，draftx-limitleft<60,实现在网格缓冲区采样
//		可通过边界白边左上角和右上角左右拖拽可得实际采样位置在缓冲区左右
//		DrawTexturePro(mesh.texture, {draftx - limitleft, drafty - limittop, 750 / camerasize, 750 / camerasize}, {0, 0, 750, 750}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {draftx - limitleft, drafty - limittop, showj*pixnum / camerasize, showi*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {draftx - limitleft, drafty - limittop, showj*pixnum / camerasize+700, showi*pixnum / camerasize+700}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
// 关于draftx y 和之前的正面没有区别，其实是因为坐标系y轴变化，但是可以变成同一个坐标系，就是一个人倒着看地图，头朝地脚朝天，代码中保留旧形式，只有数组的坐标参考是从最大开始打印的
//		DrawTexturePro(mesh.texture, {draftx - limitleft, drafty - limittop, showj*pixnum / camerasize, showi*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {draftx - limitleft, drafty - limittop, showj*pixnum / camerasize, showi*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {(-draftx+750/2)-750/2/camerasize, (-drafty+750/2)-750/2/camerasize, 750 / camerasize, 750 / camerasize}, {0, 0, 750, 750}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {(draftx- limitleft +showjv2*pixnum/2)-showjv2*pixnum/2/camerasize, drafty - limittop, showj*pixnum / camerasize, showi*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {(draftx - limitleft + showjv2*pixnum / 2) - showjv2*pixnum / 2 / camerasize, (drafty - limittop + showiv2 * pixnum / 2) - showiv2*pixnum / 2 / camerasize, showj*pixnum / camerasize, showi*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//		DrawTexturePro(mesh.texture, {(draftx - limitleft + showjv2*pixnum / 2) - showjv2*pixnum / 2 / camerasize, (drafty - limittop + showiv2 * pixnum / 2) - showiv2*pixnum / 2 / camerasize, showjv2*pixnum / camerasize, showiv2*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
//	解决bug 绘制出来是把左上角粘贴到中间，实际改完，增加距离，原来是采样采样多了，中心是mesh的中心所以要加上meshwidth/2
//		DrawTexturePro(mesh.texture, {(draftx - limitleft )+ bkmeshwidth / 2 - showjv2*pixnum / 2 / camerasize, (drafty - limittop + bkmeshheight / 2) - showiv2*pixnum / 2 / camerasize, showjv2*pixnum / camerasize, showiv2*pixnum / camerasize}, {0, 0, showjv2 * pixnum, showiv2 * pixnum}, {0, 0}, 0, WHITE);
		DrawTexturePro(mesh.texture, {(draftx - limitleft ) + bkmeshwidth / 2 - showjv2*pixnumv2 / 2 / camerasize, (drafty - limittop + bkmeshheight / 2) - showiv2*pixnumv2 / 2 / camerasize, showjv2*pixnumv2 / camerasize, showiv2*pixnumv2 / camerasize}, {0, 0, showjv2 * pixnumv2, showiv2 * pixnumv2}, {0, 0}, 0, WHITE);
		
		// 多线程追加
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, WHITE);
		// 测试特效叠加
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, {255,25,25,200});
		// 测试透明度
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, {255,255,255,200});
		// 测试颜色
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, {255,0,255,255});
		
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, {255,255,255,125});
		
//		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, showjv2 * pixnumv2, showiv2 * pixnumv2}, {0, 0}, 0, {255,255,255,125});
		
		DrawTexturePro(meshv2.texture, {0, 0, 500, 500}, {0, 0, 500, 500}, {0, 0}, 0, {255, 255, 255, 150});
		
		DrawText(TextFormat("mouseV1 %.0f,%.0f", GetMousePosition().x, GetMousePosition().y), 35, 12, 30, BLUE);
		DrawText(TextFormat("mouseV2 %.0f,%.0f", GetMousePosition().x, 750 - GetMousePosition().y), 35, 62, 30, BLUE);
		
		DrawText(TextFormat("draftxyV2 %d,%d", draftx, drafty), 35, 152, 30, RED);
		DrawText(TextFormat("camerasize %f", camerasize), 35, 192, 30, BLACK);
		DrawText(TextFormat("limitleft,right %d %d", limitleft, limitleft + 60), 35, 222, 30, BLACK);
		DrawText(TextFormat("limittop,bottom %d %d", limittop, limittop + 60 ), 35, 252, 30, BLACK);
		DrawText(TextFormat("%i FPS", GetFPS()), 300, 2 + 10, 40, WHITE);
		DrawText(TextFormat("maxside j %d", bkmeshmapj + bkmeshmapmaxj ), 340, 20 + 60, 40, WHITE);
		DrawText(TextFormat("maxside i %d", bkmeshmapi + bkmeshmapmaxi ), 340, 20 + 100, 40, WHITE);
		
		DrawText(TextFormat("drawjiV2 %d,%d", drawj, drawi), 35, 92, 30, RED);
		DrawText(TextFormat("drawxyV2 %d,%d", drawx, drawy), 35, 122, 30, RED);
		DrawText(TextFormat("playerijV2 %d,%d", playeri, playerj), 665, 12, 40, RED);
		DrawText(TextFormat("playerspeed %d,%d", speedx, speedy), 665, 62, 40, RED);
		DrawText(TextFormat("playeraxy %d,%d", ax, ay), 665, 102, 40, RED);
		DrawText(TextFormat("k %f", k), 695, 192, 40, RED);
		DrawText(TextFormat("have boom %d", have), 1195, 12, 40, RED);
		
		DrawText(TextFormat("have boomv2 %d", havev2), 1195, 62, 40, RED);
		DrawText(TextFormat("playerx y %d %d", playerx, playery), 1195, 102, 40, RED);
		DrawText(TextFormat("regonflag %d", regionflag), 1195, 162, 40, RED);
		
//		DrawText(TextFormat("have boom %d %d %d %d", bangi, bangj, bangwidth, bangheight), 1195, 62, 40, RED);
		// 如果碰撞了才打印，否则不打印，来模拟检测是否撞上了
		if (isbang) {
			DrawText(TextFormat("have boom %d %d %d %d", bangi, bangj, bangwidth, bangheight), 1195, 62, 40, RED);
		}
//		结束绘制的桌面
		EndDrawing();
	}
// 卸载GPU画布，释放GPU内存
	UnloadRenderTexture(mesh);
// 关闭后台的其他GPU 数据
	CloseWindow();
	return 0;
}
