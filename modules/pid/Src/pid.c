#include "pid.h"

void PID_Init(PID_Handle *pid,
              float kp, float ki, float kd,
              float outMin, float outMax)
{
    pid->kp = kp; pid->ki = ki; pid->kd = kd;
    pid->iTerm = 0.0f;
    pid->prevErr = 0.0f;
    pid->outMin = outMin;
    pid->outMax = outMax;
}

float PID_Update(PID_Handle *pid,
                 float setpoint, float measurement,
                 float dt)
{
    float err   = setpoint - measurement;
    float pTerm = pid->kp * err;

    pid->iTerm += pid->ki * err * dt;
    if (pid->iTerm > pid->outMax) pid->iTerm = pid->outMax;
    else if (pid->iTerm < pid->outMin) pid->iTerm = pid->outMin;

    float dTerm = pid->kd * (err - pid->prevErr) / dt;
    pid->prevErr = err;

    float out = pTerm + pid->iTerm + dTerm;

    if (out > pid->outMax) out = pid->outMax;
    else if (out < pid->outMin) out = pid->outMin;

    return out;
}
