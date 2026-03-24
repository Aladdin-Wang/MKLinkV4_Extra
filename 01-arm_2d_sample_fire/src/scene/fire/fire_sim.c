#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fire_sim.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "board.h"

#define AXIS_INPUT



#define PI 3.1415926
int g_scale = 5;
int g_canvasWidth = 60;
int g_canvasHeight = 60;
float cScale = 0.0f;

static ATTR_RAMFUNC uint8_t fluid_mem[96*1024];
Scene scene;  // 全局场景实例



// 创建流体模拟实例
Fluid * Fluid_create(int numX, int numY, float h) {
	Fluid *f = (Fluid*)malloc(sizeof(Fluid));
	f->numX = numX + 2;      // 加上边界层
	f->numY = numY + 2;
	f->numCells = f->numX * f->numY;
	f->h = h;
	
// 使用静态内存池分配
	uint8_t *ptr = (uint8_t *)fluid_mem;
	
	f->u = (float *)ptr;
	ptr += f->numCells * sizeof(float);
	
	f->v = (float *)ptr;
	ptr += f->numCells * sizeof(float);
	
	f->newU = (float *)ptr;
	ptr += f->numCells * sizeof(float);
	
	f->newV = (float *)ptr;
	ptr += f->numCells * sizeof(float);
	
	f->s = (uint8_t *)ptr;
	ptr += f->numCells * sizeof(uint8_t);
	
	f->t = (float *)ptr;
	ptr += f->numCells * sizeof(float);
	
	f->newT = (float *)ptr;
	
	// 初始化：所有格子都是流体，温度为0
	for (int i = 0; i < f->numCells; i++) {
		f->s[i] = 1.0f;
		f->t[i] = 0.0f;
	}
	
	// 初始化旋涡
	f->numSwirls = 0;
	f->swirlGlobalTime = 0.0f;
	for (int i = 0; i < MAX_SWIRLS; i++) {
		f->swirlTime[i] = 0.0f;
	}
	
	return f;
}

// 释放流体模拟内存
void Fluid_free(Fluid *f) {
	free(f->u);
	free(f->v);
	free(f->newU);
	free(f->newV);
	free(f->s);
	free(f->t);
	free(f->newT);
	free(f);
}


//void deal_with_8688_angle(Fluid *f, float dt, float gravity) {
//   static float old_pitch = 0, old_roll = 0;
//   float ax = qmi8658_read_acc_x() / 16384.0f; // QMI8658默认±2g量程，除以16384转成g单位
//   float ay = qmi8658_read_acc_y() / 16384.0f;
//   float az = qmi8658_read_acc_z() / 16384.0f;

//   // 2. 算倾角，静态精度±0.5°完全够用

//   scene.sinW = (-ax) / sqrtf(ax*ax + az*az);
//   // 3. 低通滤波防抖动，系数0.1/0.9可以自己调，值越小越稳但响应越慢

//}
// 积分：应用重力
void Fluid_integrate(Fluid *f, float dt, float gravity) {
	int n = f->numY;
	for (int i = 1; i < f->numX; i++) {
		for (int j = 1; j < f->numY - 1; j++) {
			// 如果当前格子和下方格子都是流体，则应用重力
			if (f->s[i * n + j] != 0.0f && f->s[i * n + j - 1] != 0.0f) {
				f->v[i * n + j] += gravity * dt;
			}
		}
	}
}


// 压力求解：保证不可压缩性
void Fluid_solveIncompressibility(Fluid *f, int numIters, float dt) {
	int n = f->numY;
	float overRelaxation = 1.9f;  // 超松弛因子，加速收敛
	
	for (int iter = 0; iter < numIters; iter++) {
		for (int i = 1; i < f->numX - 1; i++) {
			for (int j = 1; j < f->numY - 1; j++) {
				if (f->s[i * n + j] == 0.0f) continue;  // 跳过固体格子
				
				// 获取相邻格子的固体标记
				float sx0 = f->s[(i - 1) * n + j];
				float sx1 = f->s[(i + 1) * n + j];
				float sy0 = f->s[i * n + j - 1];
				float sy1 = f->s[i * n + j + 1];
				float s = sx0 + sx1 + sy0 + sy1;
				if (s == 0.0f) continue;  // 周围都是固体，跳过
				
				// 计算速度散度
				float div = f->u[(i + 1) * n + j] - f->u[i * n + j] +
				f->v[i * n + j + 1] - f->v[i * n + j];
				
				// 计算压力并修正速度
				float p = -div / s;
				p *= overRelaxation;
				f->u[i * n + j] -= sx0 * p;
				f->u[(i + 1) * n + j] += sx1 * p;
				f->v[i * n + j] -= sy0 * p;
				f->v[i * n + j + 1] += sy1 * p;
			}
		}
	}
}

// 边界外推：扩展边界速度
void Fluid_extrapolate(Fluid *f) {
	int n = f->numY;
	// 上下边界
	for (int i = 0; i < f->numX; i++) {
		f->u[i * n + 0] = f->u[i * n + 1];
		f->u[i * n + f->numY - 1] = f->u[i * n + f->numY - 2];
	}
	// 左右边界
	for (int j = 0; j < f->numY; j++) {
		f->v[0 * n + j] = f->v[1 * n + j];
		f->v[(f->numX - 1) * n + j] = f->v[(f->numX - 2) * n + j];
	}
}

// 采样场：双线性插值
float Fluid_sampleField(Fluid *f, float x, float y, int field) {
	int n = f->numY;
	float h = f->h;
	float h1 = 1.0f / h;
	float h2 = 0.5f * h;
	
	// 限制坐标在有效范围内
	x = fmaxf(fminf(x, f->numX * h), h);
	y = fmaxf(fminf(y, f->numY * h), h);
	
	float dx = 0.0f;
	float dy = 0.0f;
	float *fieldData;
	
	// 根据场类型选择数据和偏移
	switch (field) {
		case U_FIELD: fieldData = f->u; dy = h2; break;
		case V_FIELD: fieldData = f->v; dx = h2; break;
		case T_FIELD: fieldData = f->t; dx = h2; dy = h2; break;
		default: return 0.0f;
	}
	
	// 计算四个采样点
	int x0 = (int)fminf(floorf((x - dx) * h1), f->numX - 1);
	float tx = ((x - dx) - x0 * h) * h1;
	int x1 = (int)fminf(x0 + 1, f->numX - 1);
	
	int y0 = (int)fminf(floorf((y - dy) * h1), f->numY - 1);
	float ty = ((y - dy) - y0 * h) * h1;
	int y1 = (int)fminf(y0 + 1, f->numY - 1);
	
	float sx = 1.0f - tx;
	float sy = 1.0f - ty;
	
	// 双线性插值
	float val = sx * sy * fieldData[x0 * n + y0] +
	tx * sy * fieldData[x1 * n + y0] +
	tx * ty * fieldData[x1 * n + y1] +
	sx * ty * fieldData[x0 * n + y1];
	
	return val;
}

// 计算平均U速度（用于MAC网格）
float Fluid_avgU(Fluid *f, int i, int j) {
	int n = f->numY;
	float u = (f->u[i * n + j - 1] + f->u[i * n + j] +
		f->u[(i + 1) * n + j - 1] + f->u[(i + 1) * n + j]) * 0.25f;
	return u;
}

// 计算平均V速度（用于MAC网格）
float Fluid_avgV(Fluid *f, int i, int j) {
	int n = f->numY;
	float v = (f->v[(i - 1) * n + j] + f->v[i * n + j] +
		f->v[(i - 1) * n + j + 1] + f->v[(i - 1) * n + j + 1]) * 0.25f;
	return v;
}

// 平流速度场（半拉格朗日方法）
void Fluid_advectVel(Fluid *f, float dt) {
	int n = f->numY;
	float h = f->h;
	float h2 = 0.5f * h;
	
	// 保存当前速度
	memcpy(f->newU, f->u, f->numCells * sizeof(float));
	memcpy(f->newV, f->v, f->numCells * sizeof(float));
	
	for (int i = 1; i < f->numX; i++) {
		for (int j = 1; j < f->numY; j++) {
			// 平流U分量
			if (f->s[i * n + j] != 0.0f && f->s[(i - 1) * n + j] != 0.0f && j < f->numY - 1) {
				float x = i * h;
				float y = j * h + h2;
				float u = f->u[i * n + j];
				float v = Fluid_avgV(f, i, j);
				x = x - dt * u;  // 回溯到上一时间步位置
				y = y - dt * v;
				u = Fluid_sampleField(f, x, y, U_FIELD);
				f->newU[i * n + j] = u;
			}
			
			// 平流V分量
			if (f->s[i * n + j] != 0.0f && f->s[i * n + j - 1] != 0.0f && i < f->numX - 1) {
				float x = i * h + h2;
				float y = j * h;
				float u = Fluid_avgU(f, i, j);
				float v = f->v[i * n + j];
				x = x - dt * u;
				y = y - dt * v;
				v = Fluid_sampleField(f, x, y, V_FIELD);
				f->newV[i * n + j] = v;
			}
		}
	}
	
	// 更新速度场
	memcpy(f->u, f->newU, f->numCells * sizeof(float));
	memcpy(f->v, f->newV, f->numCells * sizeof(float));
}

// 平流温度场
void Fluid_advectTemperature(Fluid *f, float dt) {
	int n = f->numY;
	float h = f->h;
	float h2 = 0.5f * h;
	
	memcpy(f->newT, f->t, f->numCells * sizeof(float));
	
	for (int i = 1; i < f->numX - 1; i++) {
		for (int j = 1; j < f->numY - 1; j++) {
			if (f->s[i * n + j] != 0.0f) {
				float u = (f->u[i * n + j] + f->u[(i + 1) * n + j]) * 0.5f;
				float v = (f->v[i * n + j] + f->v[i * n + j + 1]) * 0.5f;
				float x = i * h + h2 - dt * u;
				float y = j * h + h2 - dt * v;
				f->newT[i * n + j] = Fluid_sampleField(f, x, y, T_FIELD);
			}
		}
	}
	
	memcpy(f->t, f->newT, f->numCells * sizeof(float));
}

// 更新火焰效果
void Fluid_updateFire(Fluid *f, float dt) {
	float h = f->h;
	float swirlTimeSpan = 1.0f;           // 旋涡寿命
	float swirlOmega = 20.0f;             // 旋涡角速度
	float swirlDamping = 10.0f * dt;      // 旋涡阻尼
	float swirlProbability = scene.swirlProbability * h * h;  // 旋涡生成概率
	float fireCooling =  1.2f * dt;        // 火焰冷却速度
	float smokeCooling = 0.4f * dt;       // 烟雾冷却速度
	float lift = 3.5f;                     // 火焰升力
        float k_motion = 5.0;
        extern float_t DATA_GY_ACC[6];
        #ifdef AXIS_INPUT

        float ax, ay, az;
        ax = DATA_GY_ACC[0];
        ay = DATA_GY_ACC[1];
        az = DATA_GY_ACC[2];

        // 
        static float gxf = 0;
        static float gyf = 0;
        static float gzf = 0;


        float Ylift,Xlift = 0;

        float alpha = 0.1f;      //重力部分低通滤波

        gxf = (1 - alpha) * gxf + alpha * ax;
        gyf = (1 - alpha) * gyf + alpha * ay;
        gzf = (1 - alpha) * gzf + alpha * az;

        float norm = sqrtf(gyf*gyf + gzf*gzf) + 1e-6f;
        float gx = gyf / norm;   //棱镜反射平面内的水平和竖直加速度方向
        float gy = gzf / norm;
        
        float lin_x = ay - gyf;   //分离出高速抖动
        float lin_y = az - gzf;

        float lift_x = gx;
        float lift_y = gy;

        if(az < -0.1){
            Ylift = -sqrt(1 - (lift_x * lift_x)) * lift;
        }else{
            Ylift =  sqrt(1 - (lift_x * lift_x)) * lift;
        }
            Xlift = lift_x * lift;    

        #endif

	float acceleration = 5.0f * dt;        // 加速度
	float kernelRadius = scene.swirlMaxRadius;  // 旋涡影响半径

	int n = f->numY;
	float maxX = (f->numX - 1) * f->h;
	float maxY = (f->numY - 1) * f->h;
	
	// 清理过期旋涡
	int num = 0;
	for (int nr = 0; nr < f->numSwirls; nr++) {
		f->swirlTime[nr] -= dt;
		if (f->swirlTime[nr] > 0.0f) {
			f->swirlTime[num] = f->swirlTime[nr];
			f->swirlX[num] = f->swirlX[nr];
			f->swirlY[num] = f->swirlY[nr];
			f->swirlOmega[num] = f->swirlOmega[nr];
			num++;
		}
	}
	f->numSwirls = num;
	
	// 更新旋涡并影响速度场
	for (int nr = 0; nr < f->numSwirls; nr++) {
		float x = f->swirlX[nr];
		float y = f->swirlY[nr];
		float swirlU = (1.0f - swirlDamping) * Fluid_sampleField(f, x, y, U_FIELD);
		float swirlV = (1.0f - swirlDamping) * Fluid_sampleField(f, x, y, V_FIELD);
		x += swirlU * dt;
		y += swirlV * dt;
		x = fminf(fmaxf(x, h), maxX);
		y = fminf(fmaxf(y, h), maxY);
		
		f->swirlX[nr] = x;
		f->swirlY[nr] = y;
		float omega = f->swirlOmega[nr];
		
		// 计算旋涡影响范围
		int x0 = (int)fmaxf(floorf((x - kernelRadius) / h), 0);
		int y0 = (int)fmaxf(floorf((y - kernelRadius) / h), 0);
		int x1 = (int)fminf(floorf((x + kernelRadius) / h) + 1, f->numX - 1);
		int y1 = (int)fminf(floorf((y + kernelRadius) / h) + 1, f->numY - 1);
		
		// 更新周围速度场
		for (int i = x0; i <= x1; i++) {
			for (int j = y0; j <= y1; j++) {
				for (int dim = 0; dim < 2; dim++) {
					float vx = dim == 0 ? i * h : (i + 0.5f) * h;
					float vy = dim == 0 ? (j + 0.5f) * h : j * h;
					
					float rx = vx - x;
					float ry = vy - y;
					float r = sqrtf(rx * rx + ry * ry);
					
					if (r < kernelRadius) {
						float s = 1.0f;
						if (r > 0.8f * kernelRadius) {
							s = 5.0f - 5.0f / kernelRadius * r;
						}
						
						if (dim == 0) {
							float target = ry * omega + swirlU;
							float u = f->u[n * i + j];
							f->u[n * i + j] += (target - u) * s;
						} else {
							float target = -rx * omega + swirlV;
							float v = f->v[n * i + j];
							f->v[n * i + j] += (target - v) * s;
						}
					}
				}
			}
		}
	}
	
	// 更新温度场
	float minR = 0.85f * scene.obstacleRadius;
	float maxR = scene.obstacleRadius + h;
	
	for (int i = 0; i < f->numX; i++) {
		for (int j = 0; j < f->numY; j++) {
			float t = f->t[i * n + j];
			float cooling = t < 0.3f ? smokeCooling : fireCooling;
			f->t[i * n + j] = fmaxf(t - cooling, 0.0f);
			float u = f->u[i * n + j];
			float v = f->v[i * n + j];
              #ifdef AXIS_INPUT
                    float targetV = t * Ylift - k_motion * lin_y;
                    float targetU = t * Xlift + k_motion * lin_x;
                    f->v[i * n + j] += (targetV - v) * acceleration;
                    f->u[i * n + j] += (targetU - u) * acceleration;

              #else
                    float targetV =  t * lift;
                    f->v[i * n + j] += (targetV - v) * acceleration;


              #endif
	
			int numNewSwirls = 0;
			
			// 燃烧环（改为椭圆形，底部更宽）
			if (scene.burningObstacle) {
				float dx = (i + 0.5f) * f->h - scene.obstacleX;
				float dy = (j + 0.5f) * f->h - scene.obstacleY;
				float dxWide = dx * 0.5f;  // 水平方向压缩，形成椭圆
				float d = dxWide * dxWide + dy * dy;
				if (d < maxR * maxR) {
                                        if(scene.close){
                                           f->t[i * n + j] = 0.0f;
                                        }else{
                                           f->t[i * n + j] = 1.0f;
                                        }

					if (((double)rand() / RAND_MAX) < 0.5f * swirlProbability) {
						numNewSwirls++;
					}
				}
			}
			
			// 燃烧地板
			if (j < 4 && scene.burningFloor) {
				f->t[i * n + j] = 1.0f;
				f->u[i * n + j] = 0.0f;
				f->v[i * n + j] = 0.0f;
				if (((double)rand() / RAND_MAX) < swirlProbability) {
					numNewSwirls++;
				}
			}
			
			// 生成新旋涡
			for (int k = 0; k < numNewSwirls; k++) {
				if (f->numSwirls >= MAX_SWIRLS) break;
				int nr = f->numSwirls;
				f->swirlX[nr] = i * h;
				f->swirlY[nr] = j * h;
				f->swirlOmega[nr] = (-1.0f + 2.0f * ((double)rand() / RAND_MAX)) * swirlOmega;
				f->swirlTime[nr] = swirlTimeSpan;
				f->numSwirls++;
			}
		}
	}
	
	// 平滑温度（刚点燃的地方）
	for (int i = 1; i < f->numX - 1; i++) {
		for (int j = 1; j < f->numY - 1; j++) {
			float t = f->t[i * n + j];
			if (t == 1.0f) {
				float avg = (
					f->t[(i - 1) * n + (j - 1)] +
					f->t[(i + 1) * n + (j - 1)] +
					f->t[(i + 1) * n + (j + 1)] +
					f->t[(i - 1) * n + (j + 1)]) * 0.25f;
				f->t[i * n + j] = avg;
			}
		}
	}
}

// 执行一帧模拟
void Fluid_simulate(Fluid *f, float dt, float gravity, int numIters) {
	Fluid_integrate(f, dt, gravity);
	Fluid_solveIncompressibility(f, numIters, dt);
	Fluid_extrapolate(f);
	Fluid_advectVel(f, dt);
	Fluid_advectTemperature(f, dt);
	Fluid_updateFire(f, dt);
}
void fire_sim_update(void) {  //在 start 回调处调用
    if (scene.paused || !scene.fluid) return;
    
    float dt = 1/30.0f;  
    float gravity = (float)scene.gravity;
    
    Fluid_simulate(scene.fluid, dt, gravity, scene.numIters);
    scene.frameNr++;
}
uint16_t getFireColor_RGB565_Q12(float val)
{
	val = val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val);
	float fr, fg, fb;
	
	if (val < 0.3f) {
		float s = val / 0.3f;
		fr = 0.2f * s;
		fg = 0.2f * s;
		fb = 0.2f * s;
	} else if (val < 0.5f) {
		float s = (val - 0.3f) / 0.2f;
		fr = 0.2f + 0.8f * s;
		fg = 0.1f;
		fb = 0.1f;
	} else {
		float s = (val - 0.5f) / 0.48f;
		fr = 1.0f;
		fg = s;
		fb = 0.5f;
	}

    uint16_t R = ((uint32_t)fr * 31);
    uint16_t G = ((uint32_t)fg * 63);
    uint16_t B = ((uint32_t)fb * 31);

    return (R<<11)|(G<<5)|B;
}

uint16_t getFireColor_BlueGas_RGB565(float val)
{
    // 🔥 黑背景
    if (val <= 0.05f) return 0x0000;
    if (val > 1.0f) val = 1.0f;

    // 🔥 提高对比度（但不要太狠）
    val = val * val;

    float fr, fg, fb;

    if (val < 0.3f) {
        // 🔴 外层（很弱的红/暗）
        float s = val / 0.3f;
        fr = 0.4f * s;
        fg = 0.05f * s;
        fb = 0.1f * s;
    } 
    else if (val < 0.6f) {
        // 🔵 蓝区（降低纯蓝，偏青一点）
        float s = (val - 0.3f) / 0.3f;
        fr = 0.0f;
        fg = 0.2f * s;       // 加一点绿 → 更真实
        fb = 0.6f + 0.4f * s; // 蓝但不过饱和
    } 
    else if (val < 0.85f) {
        // 🔵→⚪ 蓝白过渡（关键！）
        float s = (val - 0.6f) / 0.25f;
        fr = 0.3f * s;
        fg = 0.4f + 0.6f * s;
        fb = 1.0f;
    } 
    else {
        // ⚪ 白核（高温）
        float s = (val - 0.85f) / 0.15f;
        fr = 0.8f + 0.2f * s;
        fg = 0.9f + 0.1f * s;
        fb = 1.0f;
    }

    uint16_t R = (uint16_t)(fr * 31.0f);
    uint16_t G = (uint16_t)(fg * 63.0f);
    uint16_t B = (uint16_t)(fb * 31.0f);

    return (R << 11) | (G << 5) | B;
}



uint16_t getFireColor_RGB565_Q12_gpt_version(float val)
{
    // clamp
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;

    float fr, fg, fb;

    // ===== ① 烟雾区（低温）=====
    if (val < 0.3f) {
        float s = val / 0.3f;

        // 非线性（让烟更柔）
        s = s * s;

        fr = 0.05f + 0.25f * s;
        fg = 0.05f + 0.25f * s;
        fb = 0.05f + 0.30f * s;   // 稍微偏冷一点（更像烟）
    }

    // ===== ② 红色燃烧区 =====
    else if (val < 0.6f) {
        float s = (val - 0.3f) / 0.3f;

        fr = 0.3f + 0.7f * s;     // 红快速拉满
        fg = 0.05f + 0.4f * s;    // 绿慢慢上
        fb = 0.02f;               // 几乎无蓝（避免发紫）
    }

    // ===== ③ 高温黄白区 =====
    else {
        float s = (val - 0.6f) / 0.4f;

        // 加一点非线性，避免大片纯黄
        s = s * s;

        fr = 1.0f;
        fg = 0.5f + 0.5f * s;     // 黄 → 白
        fb = 0.05f + 0.2f * s;    // 很少蓝，只用于提亮
    }

    // ===== 转 RGB565 =====
    uint16_t R = (uint16_t)(fr * 31.0f);
    uint16_t G = (uint16_t)(fg * 63.0f);
    uint16_t B = (uint16_t)(fb * 31.0f);

    return (R << 11) | (G << 5) | B;
}


uint16_t getFireColor_Fireplace_Smoke(float val)
{
    // ===== 轻微闪烁 =====
    val += ((rand() & 0xFF) / 255.0f - 0.5f) * 0.02f;

    // clamp
    if (val < 0.0f) val = 0.0f;
    if (val > 1.0f) val = 1.0f;

    // ===== 对比度 =====
    val = val * val;

    float fr, fg, fb;
    // ===== ① 烟雾区（改成灰白🔥）=====
    //if (val < 0.2f) {
    //    val = val * val * 0.9f;
    //    float s = val / 0.2f;

    //    // 关键：提升亮度 + 偏灰白
    //    fr = 0.15f + 0.35f * s;
    //    fg = 0.15f + 0.35f * s;
    //    fb = 0.18f + 0.40f * s;   // 微微偏蓝（像烟）
    //}
    if (val < 0.25f) {
        float s = val / 0.25f;

        // 🔥 平滑（去颗粒）
        s = s * s * (3.0f - 2.0f * s);   // smoothstep

        // 🔥 关键：加“底亮度”（否则黑背景看不见）
        float base = 0.12f;   // 👈 这个就是烟能不能看见的关键

        fr = base + 0.35f * s;
        fg = base + 0.35f * s;
        fb = base + 0.38f * s;   // 微偏冷一点像烟
    }
    // ===== ② 外焰红 =====
    else if (val < 0.4f) {
        float s = (val - 0.2f) / 0.2f;

        fr = 0.6f + 0.4f * s;
        fg = 0.05f + 0.10f * s;
        fb = 0.01f;
    }

    // ===== ③ 橙色主体 =====
    else if (val < 0.7f) {
        float s = (val - 0.4f) / 0.3f;

        fr = 1.0f;
        fg = 0.2f + 0.6f * s;
        fb = 0.02f;
    }

    // ===== ④ 暖白核心 =====
    else {
        float s = (val - 0.7f) / 0.3f;
        s = s * s;

        fr = 1.0f;
        fg = 0.75f + 0.25f * s;
        fb = 0.03f + 0.10f * s;
    }

    uint16_t R = (uint16_t)(fr * 31.0f);
    uint16_t G = (uint16_t)(fg * 63.0f);
    uint16_t B = (uint16_t)(fb * 31.0f);

    return (R << 11) | (G << 5) | B;
}

#ifdef _WIN32
HWND g_hwnd = NULL;
int g_scale = 5;
int g_canvasWidth = 240;
int g_canvasHeight = 135;
float cScale = 0.0f;

// 坐标转换：物理坐标 -> 屏幕坐标
float cX(float x) {
	return x * cScale;
}

float cY(float y) {
	return g_canvasHeight - y * cScale;
}

// 获取火焰颜色（黑→红→橙→黄）
void getFireColor(float val, unsigned char *r, unsigned char *g, unsigned char *b) {
	val = val < 0.0f ? 0.0f : (val > 1.0f ? 1.0f : val);
	float fr, fg, fb;
	
	if (val < 0.3f) {
		float s = val / 0.3f;
		fr = 0.2f * s;
		fg = 0.2f * s;
		fb = 0.2f * s;
	} else if (val < 0.5f) {
		float s = (val - 0.3f) / 0.2f;
		fr = 0.2f + 0.8f * s;
		fg = 0.1f;
		fb = 0.1f;
	} else {
		float s = (val - 0.5f) / 0.48f;
		fr = 1.0f;
		fg = s;
		fb = 0.5f;
	}
	
	*r = (unsigned char)(fr * 255);
	*g = (unsigned char)(fg * 255);
	*b = (unsigned char)(fb * 255);
}

// 窗口消息处理
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		
	case WM_ERASEBKGND:
		return 1;
		
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			
			// 双缓冲绘图，防止闪烁
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmMem = CreateCompatibleBitmap(hdc, g_canvasWidth, g_canvasHeight);
			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
			
			RECT rect = {0, 0, g_canvasWidth, g_canvasHeight};
			HBRUSH hbrBlack = CreateSolidBrush(RGB(0, 0, 0));
			FillRect(hdcMem, &rect, hbrBlack);
			DeleteObject(hbrBlack);
			
			// 绘制火焰
			if (scene.fluid) {
				Fluid *f = scene.fluid;
				int n = f->numY;
				float h = f->h;
				float cellScale = 1.1f;
				
				for (int i = 0; i < f->numX; i++) {
					for (int j = 0; j < f->numY; j++) {
						float t = f->t[i * n + j];
						if (t > 0.01f) {
							unsigned char r, g, b;
							getFireColor(t, &r, &g, &b);
							HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
							
							int x = (int)cX(i * h);
							int y = (int)cY((j + 1) * h);
							int cx = (int)(cScale * cellScale * h) + 1;
							int cy = (int)(cScale * cellScale * h) + 1;
							
							RECT cellRect = {x, y, x + cx, y + cy};
							FillRect(hdcMem, &cellRect, brush);
							DeleteObject(brush);
						}
					}
				}
			}
			
			// 复制到屏幕
			BitBlt(hdc, 0, 0, g_canvasWidth, g_canvasHeight, hdcMem, 0, 0, SRCCOPY);
			
			SelectObject(hdcMem, hbmOld);
			DeleteObject(hbmMem);
			DeleteDC(hdcMem);
			
			EndPaint(hwnd, &ps);
			return 0;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 创建窗口
void create_window() {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	
	WNDCLASS wc = {0};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = "FireSimWindow";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	
	RegisterClass(&wc);
	
	RECT rect = {0, 0, g_canvasWidth, g_canvasHeight};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	
	g_hwnd = CreateWindowEx(
		0, "FireSimWindow", "Fire Simulation",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left, rect.bottom - rect.top,
		NULL, NULL, hInstance, NULL
		);
	
	ShowWindow(g_hwnd, SW_SHOW);
	UpdateWindow(g_hwnd);
}

// 设置场景
void setupScene() {
	float simHeight = 1.0f;
	cScale = g_canvasHeight / simHeight;
	float simWidth = g_canvasWidth / cScale;
	
	int numCells = 3600;
	float h = sqrtf(simWidth * simHeight / numCells);
	
	int numX = (int)(simWidth / h);
	int numY = (int)(simHeight / h);
	
	scene.obstacleX = 0.5f * numX * h;
	scene.obstacleY = 0.12f * numY * h;
	scene.obstacleRadius = 0.08f;
	scene.swirlProbability = 60.0f;
	scene.showObstacle = scene.burningObstacle;
	
	scene.fluid = Fluid_create(numX, numY, h);
}

// 主循环
void run_window_loop() {
	MSG msg;
	
	while (1) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		if (!scene.paused && scene.fluid) {
			Fluid_simulate(scene.fluid, scene.dt, scene.gravity, scene.numIters);
			scene.frameNr++;
		}
		
		InvalidateRect(g_hwnd, NULL, FALSE);
		UpdateWindow(g_hwnd);
		
		Sleep(16);
	}
}
#endif

int fire_init() {   //在场景初始化处调用
	// 初始化场景参数
	scene.gravity = 0.0f;
	scene.dt = 1.0f / 60.0f;
	scene.numIters = 10;
	scene.frameNr = 0;
	scene.obstacleX = 0.0f;
	scene.obstacleY = 0.0f;
	scene.obstacleRadius = 0.1f;
	scene.burningObstacle = 1;
	scene.burningFloor = 0;
	scene.paused = 0;
	scene.showObstacle = 0;
	scene.showSwirls = 0;
	scene.swirlProbability = 50.0f;
	scene.swirlMaxRadius = 0.05f;
	scene.fluid = NULL;
	
#ifdef _WIN32
//	create_window();
//	setupScene();
//	run_window_loop();
#endif
	float simHeight = 1.0f;
	cScale = g_canvasHeight / simHeight;
	float simWidth = g_canvasWidth / cScale;
	
	int numCells = 3600;
	float h = sqrtf(simWidth * simHeight / numCells);
	
	int numX = (int)(simWidth / h);
	int numY = (int)(simHeight / h);
	
	scene.obstacleX = 0.5f * numX * h;
	scene.obstacleY = 0.12f * numY * h;
	scene.obstacleRadius = 0.1f;
	scene.swirlProbability = 60.0f;
	scene.showObstacle = scene.burningObstacle;
	scene.fluid = Fluid_create(numX, numY, h);
	if (!scene.fluid) {
		Fluid_free(scene.fluid);
	}
	
	return 0;
}

