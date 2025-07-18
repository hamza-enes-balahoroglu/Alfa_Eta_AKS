#ifndef PID_H_
#define PID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float kd;

    float iTerm;
    float prevErr;
    float outMin;
    float outMax;
} PID_Handle;

void  PID_Init(PID_Handle* pid,
               float kp, float ki, float kd,
               float outMin, float outMax);

float PID_Update(PID_Handle* pid, float setpoint,
                 float measurement, float dt_sec);

#ifdef __cplusplus
}
#endif
#endif
