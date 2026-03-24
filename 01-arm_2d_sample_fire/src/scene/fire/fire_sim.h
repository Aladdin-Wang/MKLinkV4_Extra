#ifndef __FIRE_SIM_H__
#define __FIRE_SIM_H__
#include "stdint.h"


// 
#define MAX_SWIRLS 60

// 
enum {
	U_FIELD = 0,    // X
	V_FIELD,        // Y
	T_FIELD,        // 
};
// 
typedef struct {
	int numX;           // X
	int numY;           // Y
	int numCells;       // 
	float h;            // 
	
	float *u;           // X
	float *v;           // Y
	float *newU;        // X
	float *newV;        // Y
	uint8_t *s;           // 1=0=
	float *t;           // 
	float *newT;        // 
	
	int numSwirls;              // 
	float swirlGlobalTime;       // 
	float swirlX[MAX_SWIRLS];   // X
	float swirlY[MAX_SWIRLS];   // Y
	float swirlOmega[MAX_SWIRLS];// 
	float swirlRadius[MAX_SWIRLS];// 
	float swirlTime[MAX_SWIRLS]; // 
} Fluid;
// 
typedef struct {
	float gravity;           // 
	float dt;                // 
	int numIters;            // 
	int frameNr;             // 
	float obstacleX;         // X
	float obstacleY;         // Y
	float obstacleRadius;    // 
	int burningObstacle;     // 
	int burningFloor;        // 
	int paused;              // 
	int showObstacle;        // 
	int showSwirls;          // 
	float swirlProbability;  // 
	float swirlMaxRadius;    // 
	Fluid *fluid;            // 
        float  sinW;
        int    close;
} Scene;
extern Scene scene;  // 
extern int fire_init(void) ;
extern uint16_t getFireColor_RGB565_Q12(float val);
extern void fire_sim_update(void);
extern uint16_t getFireColor_RGB565_Q12_gpt_version(float val);
extern uint16_t getFireColor_BlueGas_RGB565(float val);
extern uint16_t getFireColor_LUT(float val);
extern uint16_t getFireColor_RGB565_Fireplace(float val);
extern uint16_t getFireColor_Fireplace_Smoke(float val);
#endif