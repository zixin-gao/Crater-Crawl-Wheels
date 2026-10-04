#include "DCMotor.h"
#include <TimerOne.h>

// motor pins
#define PWM 6
#define IN1 8
#define IN2 7
#define STBY 10

// encoder pins
#define EN_A 2
#define EN_B 3

#define ENCODER_CPR 1320
#define PERIOD_MS   50
#define PERIOD_S    0.05

#define MAX_PWM     255
#define MIN_PWM     -255      // minimum PWM required to overcome static friction

DCMotor Motor;

// encoder
volatile long EncoderCNT = 0;
volatile long Last_EncoderCNT = 0;
volatile bool sampleReady = false;

// PID
double targetSpeed = 0;      // RPM (change as needed)
double speedError = 0;
double speedError_Integral = 0;
double speedError_Derivative = 0;
double lastSpeedError = 0;

double kp = 7.0;
double ki = 1.0;
double kd = 0;

int PWMOutput = 0;
double speed = 0;

//---------------- Encoder ISRs ----------------

void encoderA_ISR()
{
    if (digitalRead(EN_A) ^ digitalRead(EN_B))
        EncoderCNT--;
    else
        EncoderCNT++;
}

void encoderB_ISR()
{
    if (digitalRead(EN_A) ^ digitalRead(EN_B))
        EncoderCNT++;
    else
        EncoderCNT--;
}

//---------------- Timer ISR ----------------

void timerISR()
{
    sampleReady = true;
}

//---------------- Setup ----------------

void setup()
{
    Serial.begin(115200);

    Motor.attach(IN1, IN2, PWM, STBY);

    pinMode(EN_A, INPUT_PULLUP);
    pinMode(EN_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(EN_A), encoderA_ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(EN_B), encoderB_ISR, CHANGE);

    Timer1.initialize(PERIOD_MS * 1000UL);
    Timer1.attachInterrupt(timerISR);
}

//---------------- Loop ----------------

void loop()
{
    if (sampleReady)
    {
        sampleReady = false;

        noInterrupts();
        long count = EncoderCNT;
        interrupts();

        // angle and rpm
        double angle = count * 360.0 / ENCODER_CPR;
        speed =
            (count - Last_EncoderCNT) *
            (60000.0 / PERIOD_MS) /
            ENCODER_CPR;

        // calculate error
        speedError = targetSpeed - speed;
        speedError_Integral = speedError * PERIOD_S;
        speedError_Derivative = (speedError - lastSpeedError) / PERIOD_S;

        // calculate PID
        double pid_output =
            kp * speedError +
            ki * speedError_Integral +
            kd * speedError_Derivative;

        PWMOutput = (int)pid_output;

        // deadband
        if (PWMOutput > 255) PWMOutput = 255;
        if (PWMOutput < -255) PWMOutput = -255;

        // output to motor
        if (PWMOutput > 0)
        {
            Motor.setSpeed(0, PWMOutput);
        }
        else if (PWMOutput < 0)
        {
            PWMOutput = -PWMOutput;
            Motor.setSpeed(1, PWMOutput);
        }

        // update
        Last_EncoderCNT = EncoderCNT;
        lastSpeedError = speedError;

        // printing
        // Serial.print("Angle = ");
        // Serial.print(angle);

        Serial.print("Target Speed = ");
        Serial.print(targetSpeed);

        Serial.print(" | Actual Speed = ");
        Serial.print(speed);

        Serial.print(" | PWM = ");
        Serial.println(PWMOutput);
    }
}