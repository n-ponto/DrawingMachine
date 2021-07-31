#include <Arduino.h>
#include "StepperController.h"

StepperController::StepperController()
{
    Serial.print("Stepper controller initialized.");
    pinMode(STEP_TOP, OUTPUT);
    pinMode(DIR_TOP,  OUTPUT);
    pinMode(STEP_BOT, OUTPUT);
    pinMode(DIR_BOT,  OUTPUT);
    pinMode(ENABLE,OUTPUT);
    digitalWrite(ENABLE, HIGH); // disable at the beginning
    wait = 700;
    enabled = false;
}

void StepperController::move(Direction d, uint16_t steps)
{
    if (d < 1 || d > 12)
    {
        // Can't be zero or have 3 bits selected
        return;
    }
    
    // If only one then both steppers will have to work
    bool b, t;
    switch(d)
    {
        case (UP):    t = b = CW;      break;
        case (DOWN):  t = b = CCW;     break;
        case (LEFT):  t = CCW; b = CW; break;
        case (RIGHT): t = CW; b = CCW; break;
        default:
            // Two directions selected
            bool spin = d & DOWN;
            bool stepper = (d & RIGHT) ^ spin;
            uint8_t stepPin = stepper ? STEP_TOP : STEP_BOT;
            uint8_t dirPin =  stepper ? DIR_TOP  : DIR_BOT;
            digitalWrite(dirPin, spin);
            for (uint16_t i = 0; i < steps; i++)
            {
                // Set both high
                digitalWrite(stepPin, HIGH);
                delayMicroseconds(wait);
                // Set both low
                digitalWrite(stepPin, LOW); 
                delayMicroseconds(wait);
            }
            return;
    }
    // Only one of the directions was selected

    // Set direction for both
    digitalWrite(DIR_TOP, t);
    digitalWrite(DIR_BOT, b);
    // Step both
    for (uint16_t i = 0; i < steps; i++)
    {
        // Set both high
        digitalWrite(STEP_TOP, HIGH);
        digitalWrite(STEP_BOT, HIGH);
        delayMicroseconds(wait);
        // Set both low
        digitalWrite(STEP_TOP, LOW);
        digitalWrite(STEP_BOT, LOW); 
        delayMicroseconds(wait);
    }
}

bool StepperController::areEnabled()
{
    return enabled;
}

void StepperController::enable()
{ 
    if (!enabled)
    {
        Serial.println("Steppers enabled");
        enabled = true;
        digitalWrite(ENABLE, LOW);
    } 
}

void StepperController::disable()
{
    if (enabled)
    {
        Serial.println("Steppers disabled");
        enabled = false;
        digitalWrite(ENABLE, HIGH);
    }
}

void StepperController::square(uint16_t steps)
{
    move(UP, steps);
    move(RIGHT, steps);
    move(DOWN, steps);
    move(LEFT, steps);
}

void StepperController::diamond(uint16_t steps)
{
    move(UP + RIGHT, steps);
    move(DOWN + RIGHT, steps);
    move(DOWN + LEFT, steps);
    move(UP + LEFT, steps);
}

void StepperController::home()
{
    disable();
    char c;
    while(Serial.available()) { Serial.readBytes(&c, 1); }
    Serial.print("Move the end to the bottom left and press enter when complete...");
    while(!Serial.available()) {}
    Serial.read();
    position.x = position.y = 0;

    printCoordinates();
}

void StepperController::printCoordinates()
{
    String output = "Current position: (" + String(position.x) + ", " + String(position.y) + ")\n";
    Serial.print(output);
}

void StepperController::travel(const Point &pt)
{
    int16_t x, y;
    x = pt.x - position.x;
    y = pt.y - position.y;
    localTravel(x, y);
    position.x = pt.x;
    position.y = pt.y;
    printCoordinates();
}

void StepperController::localTravel(int16_t x, int16_t y)
{
    Serial.println("Travelling locally to: (" + String(x) + ", " + String(y) + ")");
    bool back = x < 0;
    bool down = y < 0;
    int16_t absx, absy;
    absx = back ? -1*x : x;
    absy = down ? -1*y : y;
    bool steep = absy > absx;

    // Set the output command
    Direction x_chng, y_chng;
    x_chng = back ? LEFT : RIGHT;
    y_chng = down ? DOWN : UP;
    Direction moveOne, moveBoth;
    moveBoth = x_chng + y_chng;
    moveOne = steep ? y_chng : x_chng;

    // Check if need to switch x and y
    if (steep){
        int16_t save = absx;
        absx = absy;
        absy = save; 
    }

    int16_t m, slope_error, c;
    m = 2 * absy;
    slope_error = m - absx;
    c = slope_error - absx;
    for (int16_t i = 0; i < absx; i++)
    {
        if (slope_error >= 0)
        {
            move(moveBoth, 1);
            slope_error += c;
        }
        else
        {
            move(moveOne, 1);
            slope_error += m;
        }
    }



}