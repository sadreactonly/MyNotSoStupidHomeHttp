#ifndef Time_h
#define Time_h

#include <Arduino.h>

class Time
{
  public:
    Time();
    Time(int hours, int minutes, int seconds);
    int getHours();
    int getMinutes();
    int getSeconds();
    void setHours(int hours);
    void setMinutes(int minutes);
    void setSeconds(int seconds);
    bool isEqual(Time t);
    String toString();
   
  private:
    int _hours;
    int _minutes;
    int _seconds;
};

#endif
