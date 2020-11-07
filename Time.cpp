#include "Time.h"

Time::Time()
{
  _hours = -1;
  _minutes = -1;
  _seconds = -1;

}

Time::Time(int hours, int minutes, int seconds)
{
  _hours = hours;
  _minutes = minutes;
  _seconds = seconds;

}

String Time::toString(){
  return String(_hours)+":"+ String(_minutes)+":"+String(_seconds);
}
int Time::getHours()
{
   return _hours;
}

int Time::getMinutes()
{
   return _minutes;
}

int Time::getSeconds()
{
   return _seconds;
}

void Time::setHours(int hours)
{
   _hours=hours;
}

void Time::setMinutes(int minutes)
{
   _minutes=minutes;
}

void Time::setSeconds(int seconds)
{
   _seconds=seconds;
}

bool Time::isEqual(Time t)
{
  if( _hours == t.getHours() &&
    _minutes == t.getMinutes() &&
    _seconds == t.getSeconds()
    )
    {
      return true;
    }
    else
    {
      return false;
    }
}
